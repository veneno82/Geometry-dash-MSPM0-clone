# Geometry Dash Clone for MSPM0G3507

A feature-rich, rhythm-based platformer built for the **Texas Instruments MSPM0G3507** microcontroller. This project was developed as part of **ECE319K (Introduction to Embedded Systems)** at UT Austin.

![Geometry Dash MSPM0](https://img.shields.io/badge/Platform-MSPM0G3507-blue)
![Language-C](https://img.shields.io/badge/Language-C-green)
![Status-Complete](https://img.shields.io/badge/Status-Complete-brightgreen)

## 🚀 Overview

This project is a high-performance clone of the popular game *Geometry Dash*. It features smooth 30 FPS gameplay, parallax backgrounds, high-quality audio playback from an SD card, and multiple game modes (Cube & Ship).

### 🏆 Achievements & Showcase

- **1st Place Finish:** This project won 1st place at the ECE319K Final Project competition across all 319K sections in Spring 2026

**Watch the Full Showcase + a top 100 geometry dash player beat my hardest level:** [YouTube Link](https://www.youtube.com/watch?v=lGc8fcmo3Zs)

| Superfinals Presentation | Gameplay Showcase |
| :---: | :---: |
| ![Superfinals Win](55240298703_ed22175d89_o.jpg) | ![Gameplay Showcase 1](55240157116_c718bddbe4_o.jpg) |

### Key Features
- **Smooth Gameplay:** 30Hz fixed-update game loop with optimized AABB collision detection.
- **Dual Game Modes:**
    - **Cube Mode:** Physics-based jumping with automatic rotation.
    - **Ship Mode:** Analog vertical control using a slide potentiometer.
- **Beat-Synced Levels:** Includes three iconic levels (*Stereo Madness*, *Dry Out*, and *Jumper*) with obstacles synced to the music.
- **High-Fidelity Audio:** 12-bit DAC output for music and sound effects, streamed from an SD card.
- **Customization:** Multiple character skins, including a special "Rainbow" debug mode.
- **Internationalization:** Full support for both **English** and **Spanish** languages.
- **Visual Polish:** Parallax star backgrounds, smooth screen transitions, and detailed sprite graphics.

## 🛠️ Hardware Requirements

- **Microcontroller:** TI MSPM0G3507 LaunchPad
- **Display:** ST7735R 160x128 LCD with integrated SD card slot
- **Audio:** 12-bit DAC (Internal PA15) + Amplifier/Speaker
- **Input:** 
    - 4x Tactile Switches (Jump, VolUp, VolDn, Pause)
    - 1x Slide Potentiometer (for Ship mode)
- **Indicators:** 4x LEDs for status and debugging

### Pin Assignments

| Component | Pin | Function |
| :--- | :--- | :--- |
| **LCD SCLK** | PB9 | SPI Clock |
| **LCD PICO** | PB8 | SPI Data (MOSI) |
| **LCD CS** | PB6 | Chip Select |
| **LCD RS** | PA13 | Register Select (Data/Cmd) |
| **LCD RST** | PB15 | Reset |
| **SD CS** | PB17 | SD Card Chip Select |
| **Slide Pot** | PB18 | ADC1 Channel 5 |
| **12-bit DAC**| PA15 | Audio Output |
| **Jump Sw** | PB12 | Digital Input |
| **VolUp Sw** | PB13 | Digital Input |
| **VolDn Sw** | PB14 | Digital Input |
| **Pause Sw** | PB16 | Digital Input |

## 🧠 How it Works

### 1. Game Architecture
The software is built around a **30Hz periodic interrupt** (TIMG12). This ensures consistent physics and animation regardless of the complexity of the scene. The main loop handles state transitions (Menus, Game, Win/Loss) while the ISR handles input sampling.

### 2. Graphics & Parallax
To achieve smooth performance on a resource-constrained MCU:
- **Opaque Blitting:** Instead of expensive transparency math, sprites are drawn using opaque blitting with a background-color replacement.
- **Parallax Background:** Stars are moved at a slower rate than the foreground obstacles, creating a sense of depth.
- **Inverse-Map Rotation:** The cube's rotation during jumps uses a fixed-point nearest-neighbor algorithm to avoid floating-point overhead.

### 3. Sound System
Audio is handled via two channels:
- **Background Music (BGM):** 11kHz 12-bit raw audio streamed from the SD card using the FatFs file system.
- **Sound Effects (SFX):** Short waveforms stored in Flash for low-latency playback (Jump, Death, Menu Beeps).

### 4. Level Design
Levels are **hand-crafted** to ensure perfect rhythm and timing. The level data is stored in `BeatLevels.h` using a custom event system that maps game ticks to obstacle spawns. This allows for precise sync between the obstacles and the 11kHz background music.

## 🎮 Controls

- **Jump (PB12):** Jump in Cube mode. Hold for multi-jumps.
- **Slide Pot (PB18):** Control vertical height in Ship mode.
- **Pause (PB16):** Pause the game and open the menu.
- **Vol Up/Dn (PB13/14):** Adjust volume levels or navigate menus.

## 📁 Project Structure

```text
├── Lab9Main.c        # Main game logic and state machine
├── Sound.c/h         # SD card audio streaming & SFX
├── DAC.c/h           # 12-bit DAC driver
├── images/           # Sprite and UI graphics
├── sounds/           # RAW audio files for SD card
├── BeatLevels.h      # Hand-crafted beat-synced level data
```

## 📜 License
This project is licensed under the **Simplified BSD License**. See `README.html` for full copyright details.

---
*Developed by Mauro Brito (Spring 2026) for ECE319K at UT Austin.*
