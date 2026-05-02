// Switch.c - Runs on MSPM0G3507
// PB12 = Jump      (PINCM 28) active low, internal pull-up
// PB13 = Volume Up (PINCM 29) active low, internal pull-up
// PB0  = Volume Dn (PINCM 11) active low, internal pull-up
// PB16 = Pause     (PINCM 32) active low, internal pull-up
#include <ti/devices/msp/msp.h>
#include "../inc/LaunchPad.h"

#define JUMP_PIN   (1UL<<12)
#define VOLUP_PIN  (1UL<<13)
#define VOLDN_PIN  (1UL<<0)   // PB0
#define PAUSE_PIN  (1UL<<16)

void Switch_Init(void){
    IOMUX->SECCFG.PINCM[28] = 0x00050081; // PB12 GPIO input, pull-down
    IOMUX->SECCFG.PINCM[29] = 0x00050081; // PB13 GPIO input, pull-down
    IOMUX->SECCFG.PINCM[11] = 0x00050081; // PB0  GPIO input, pull-down
    IOMUX->SECCFG.PINCM[32] = 0x00050081; // PB16 GPIO input, pull-down
    GPIOB->DOE31_0 &= ~(JUMP_PIN|VOLUP_PIN|VOLDN_PIN|PAUSE_PIN);
}

// Returns bitmask: bit0=jump, bit1=volup, bit2=voldn, bit3=pause  (1=pressed)
uint32_t Switch_In(void){
    uint32_t d = GPIOB->DIN31_0;
    return ((d & JUMP_PIN)  ? 0x01 : 0) |
           ((d & VOLUP_PIN) ? 0x02 : 0) |
           ((d & VOLDN_PIN) ? 0x04 : 0) |
           ((d & PAUSE_PIN) ? 0x08 : 0);
}
