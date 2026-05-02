// Sound.h - Runs on MSPM0G3507
// 12-bit DAC on PA15, SysTick at 11025 Hz
// Music streamed from SD card; SFX played from flash/RAM
#ifndef SOUND_H
#define SOUND_H
#include <stdint.h>

// Initialize 12-bit DAC, SysTick at 11025 Hz, build synthesised SFX
void Sound_Init(void);

// Open a music file on the SD card and begin looping playback
// filename: 8.3 format (e.g. "STEREO.BIN")
void Sound_PlayMusic(const char *filename);

// Stop music playback (SFX still work)
void Sound_StopMusic(void);

// Play a one-shot SFX from a flash/RAM buffer (replaces current SFX)
// pt: pointer to 8-bit unsigned PCM samples, count: number of samples
void Sound_PlaySFX(const uint8_t *pt, uint32_t count);

// Set volume 0 (silent) to 8 (full)
void Sound_SetVolume(uint8_t vol);

// Call from main loop: refills SD buffer when flagged by ISR
// Returns 1 if SD read occurred this call
uint8_t Sound_Task(void);

// Diagnostic: 0=SD not mounted, 1=mounted idle, 2=file playing
uint8_t Sound_SDStatus(void);

// One-shot SFX (real GD audio from GDsounds.h)
void Sound_Jump(void);        // synthesised rising chirp
void Sound_Death(void);       // explode_11.ogg - crash/death
void Sound_Quit(void);        // quitSound_01.ogg - exit level to menu
void Sound_PlayButton(void);  // playSound_01.ogg - press play to enter level
void Sound_MenuBeep(void);    // short UI navigation beep

// Win jingle from flash (endStart_02.ogg)
void Sound_PlayWin(void);

// Pause music playback (mute only — does NOT close the SD file)
void Sound_PauseMusic(void);

// Resume previously paused music without rewinding the file
void Sound_ResumeMusic(void);

#endif
