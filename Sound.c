// Sound.c - Runs on MSPM0G3507
// 12-bit DAC on PA15 via DAC_Out()
// Music: double-buffered 8-bit PCM from SD card, streamed in main loop
// SFX:   short RAM buffers, mixed on top of music in ISR
#include <stdint.h>
#include <ti/devices/msp/msp.h>
#include "../inc/DAC.h"
#include "../inc/LaunchPad.h"
#include "../inc/Clock.h"
#include "Sound.h"
#include "GDsounds.h"
#include "ff.h"
#include "diskio.h"

// ── audio double buffer ─────────────────────────────────────────────────────
#define BUFSIZE 512
static uint8_t  BufA[BUFSIZE], BufB[BUFSIZE];
static uint8_t *FrontBuf = BufA;
static uint8_t *BackBuf  = BufB;
static volatile uint32_t BufIdx  = 0;
static volatile uint8_t  NeedFill = 0;

// ── SFX overlay ─────────────────────────────────────────────────────────────
static const uint8_t      *SFX_Ptr   = 0;
static volatile uint32_t   SFX_Count = 0;
static volatile uint32_t   SFX_Idx   = 0;

// ── volume & state ───────────────────────────────────────────────────────────
static volatile uint8_t Volume      = 6;
static volatile uint8_t MusicActive = 0;

// ── FatFS ────────────────────────────────────────────────────────────────────
static FATFS  FatFS;
static FIL    MusicFile;
static uint8_t FileOpen = 0;

// ── synthesised SFX (8-bit PCM, 11025 Hz) ───────────────────────────────────
#define JUMP_LEN  1102
#define BEEP_LEN   441   // ~40 ms short UI beep

static uint8_t JumpSFX[JUMP_LEN];
static uint8_t BeepSFX[BEEP_LEN];

static void BuildSFX(void){
    uint32_t phase = 0, freq = 20;
    for(int i = 0; i < JUMP_LEN; i++){
        phase += freq;
        if(phase > 255){ phase -= 256; if(freq < 60) freq++; }
        JumpSFX[i] = (uint8_t)(128 + ((int32_t)phase - 128)*3/4);
    }
    // short 880 Hz square-wave blip for menu navigation
    for(int i = 0; i < BEEP_LEN; i++){
        BeepSFX[i] = ((i / 6) & 1) ? 192 : 64;
    }
}

// ── SysTick ISR at 11025 Hz ─────────────────────────────────────────────────
void SysTick_Handler(void){
    uint32_t sample = 128;
    if(MusicActive){
        sample = FrontBuf[BufIdx++];
        if(BufIdx >= BUFSIZE){
            BufIdx = 0;
            uint8_t *tmp = FrontBuf; FrontBuf = BackBuf; BackBuf = tmp;
            NeedFill = 1;
        }
    }
    if(SFX_Count > 0){
        sample = (sample + SFX_Ptr[SFX_Idx++]) >> 1;
        SFX_Count--;
    }
    DAC_Out(((sample * Volume) >> 3) << 4);
}

static void SysTick_IntArm(uint32_t period, uint32_t priority){
    SysTick->CTRL = 0;
    SysTick->LOAD = period - 1;
    SCB->SHP[1]   = (SCB->SHP[1] & ~0xC0000000u) | (priority << 30);
    SysTick->VAL  = 0;
    SysTick->CTRL = 0x07;
}

// ── SPI mode switching ───────────────────────────────────────────────────────
// LCD init (SPI.c) sets CTL0=0x0027 (4-wire hardware CS, PB6=SPI1_CS0).
// SD card needs CTL0=0x0007 (3-wire Motorola, software CS) and PB6 as GPIO.
// Switch before/after every FatFS call so both LCD and SD work correctly.

static void SPI_SD_Enter(void){
    while(SPI1->STAT & 0x10){}              // wait SPI idle
    IOMUX->SECCFG.PINCM[22] = 0x00000081;  // PB6 = GPIO output (LCD CS off)
    GPIOB->DOE31_0   |= (1U << 6);
    GPIOB->DOUTSET31_0 = (1U << 6);         // PB6 HIGH = LCD deselected
    SPI1->CTL1 &= ~1U;                      // disable SPI to change frame format
    SPI1->CTL0  =  0x0007;                  // FRF=00 Motorola 3-wire, 8-bit
    SPI1->CTL1 |=  1U;                      // re-enable SPI
    while(!(SPI1->STAT & 0x04)){ (void)SPI1->RXDATA; }  // flush RXFIFO
}

static void SPI_LCD_Enter(void){
    while(SPI1->STAT & 0x10){}              // wait SPI idle
    SPI1->CTL1 &= ~1U;                      // disable SPI to change frame format
    SPI1->CTL0  =  0x0027;                  // FRF=01 4-wire hardware CS
    SPI1->CTL1 |=  1U;                      // re-enable SPI
    IOMUX->SECCFG.PINCM[22] = 0x00000083;  // PB6 = SPI1_CS0 (hardware-driven)
    SPI1->CLKCTL = 4;                       // restore 8 MHz for LCD
}

// ── public API ───────────────────────────────────────────────────────────────
static uint8_t SDMounted = 0;

static void SD_LazyInit(void){
    if(SDMounted) return;
    IOMUX->SECCFG.PINCM[23] = 0x00040083;  // PB7 = SPI1 POCI, input enable
    Clock_Delay1ms(100);  // SD cards need time after pin config — 24 cycles was ~0.3 µs, far too short
    // Retry up to 5 times: SD cold-start timing is unreliable, especially after reset
    for(int attempt = 0; attempt < 5; attempt++){
        SPI_SD_Enter();
        int ok = (disk_initialize(0) == 0) &&
                 (f_mount(&FatFS, "", 0) == FR_OK);
        SPI_LCD_Enter();
        if(ok){ SDMounted = 1; return; }
        Clock_Delay1ms(50);  // brief wait before retry
    }
}

void Sound_Init(void){
    DAC_Init();
    BuildSFX();
    for(int i = 0; i < BUFSIZE; i++) BufA[i] = BufB[i] = 128;
    // SD card init is deferred to first Sound_PlayMusic call
    SysTick_IntArm(80000000/11025, 1);
}

void Sound_PlayMusic(const char *filename){
    SD_LazyInit();
    if(!SDMounted) return;
    __disable_irq();
    MusicActive = 0;
    __enable_irq();
    if(FileOpen){
        SPI_SD_Enter();
        f_close(&MusicFile);
        SPI_LCD_Enter();
        FileOpen = 0;
    }
    SPI_SD_Enter();
    FRESULT res = f_open(&MusicFile, filename, FA_READ);
    if(res != FR_OK){ SPI_LCD_Enter(); return; }
    UINT br;
    f_read(&MusicFile, FrontBuf, BUFSIZE, &br);
    f_read(&MusicFile, BackBuf,  BUFSIZE, &br);
    SPI_LCD_Enter();
    FileOpen = 1;
    __disable_irq();
    BufIdx = 0; NeedFill = 0; MusicActive = 1;
    __enable_irq();
}

void Sound_StopMusic(void){
    __disable_irq();
    MusicActive = 0;
    __enable_irq();
    if(FileOpen){
        SPI_SD_Enter();
        f_close(&MusicFile);
        SPI_LCD_Enter();
        FileOpen = 0;
    }
}

void Sound_PlaySFX(const uint8_t *pt, uint32_t count){
    __disable_irq();
    SFX_Ptr = pt; SFX_Idx = 0; SFX_Count = count;
    __enable_irq();
}

void Sound_SetVolume(uint8_t vol){
    if(vol > 8) vol = 8;
    Volume = vol;
}

uint8_t Sound_Task(void){
    if(!NeedFill || !FileOpen) return 0;
    NeedFill = 0;
    UINT br;
    SPI_SD_Enter();
    if(f_read(&MusicFile, BackBuf, BUFSIZE, &br) != FR_OK || br == 0){
        f_lseek(&MusicFile, 0);
        f_read(&MusicFile, BackBuf, BUFSIZE, &br);
    }
    SPI_LCD_Enter();
    return 1;
}

void Sound_Jump(void)       { Sound_PlaySFX(JumpSFX,   JUMP_LEN);       }
void Sound_Death(void)      { Sound_PlaySFX(ExplodeSFX, ExplodeSFX_LEN); }
void Sound_Quit(void)       { Sound_PlaySFX(QuitSFX,   QuitSFX_LEN);    }
void Sound_PlayButton(void) { Sound_PlaySFX(PlaySFX,   PlaySFX_LEN);    }
void Sound_MenuBeep(void)   { Sound_PlaySFX(BeepSFX,   BEEP_LEN);       }

void Sound_PlayWin(void){ Sound_PlaySFX(WinSFX, WinSFX_LEN); }

void Sound_PauseMusic(void){
    __disable_irq();
    MusicActive = 0;
    __enable_irq();
    // do NOT close file — Sound_ResumeMusic() resumes from exact position
}

void Sound_ResumeMusic(void){
    if(!FileOpen) return;
    __disable_irq();
    MusicActive = 1;  // BufIdx keeps its value — resume from exact pause point
    __enable_irq();
}

uint8_t Sound_SDStatus(void){
    if(!SDMounted)   return 0;
    if(!MusicActive) return 1;
    return 2;
}
