// Lab9Main.c - Geometry Dash Clone
// Runs on MSPM0G3507
// ECE319K Lab 9 - Spring 2026
// Hardware:
//   ST7735R LCD + SD card: SPI1 PB6/PB7/PB8/PB9, PA13, PB15, PB17(SD_CS)
//   Slide pot: PB18 (ADC1 ch5)
//   12-bit DAC: PA15
//   Jump: PB12  VolUp: PB13  VolDn: PB14  Pause: PB16
//   LEDs: PA0(Red1) PB22(Blue) PB26(Red2) PB27(Green/debug)

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <ti/devices/msp/msp.h>
#include "../inc/ST7735.h"
#include "../inc/Clock.h"
#include "../inc/LaunchPad.h"
#include "../inc/Timer.h"
#include "../inc/ADC1.h"
#include "../inc/Arabic.h"
#include "../inc/TExaS.h"
#include "SmallFont.h"
#include "LED.h"
#include "Switch.h"
#include "Sound.h"
#include "images/images.h"
#include "images/GDsprites.h"

// ═══════════════════════════════════════════════════════════════════════════
// CONSTANTS
// ═══════════════════════════════════════════════════════════════════════════
#define SCREEN_W     128
#define SCREEN_H     160
#define HUD_H        12
#define GROUND_Y     148   // y coordinate of ground surface (top of ground strip)
#define GAME_TOP     HUD_H
#define GAME_BOT     GROUND_Y

#define PLAYER_X     20    // fixed x position of player
#define SCROLL_SPD   3     // pixels scrolled per game tick
#define GRAVITY      2     // pixels/tick added to vy each tick (downward)
#define JUMP_VEL     (-17) // pixels/tick upward on jump
#define MAX_OBS      8     // max simultaneous obstacles on screen
#define DOUBLE_JUMP  0     // no double jump (authentic GD behavior)

// Visual theme colors
#define MENU_BG      0x3010  // dark indigo-purple for all menus/title
#define STAR_COL     0x480F  // faint purple bg elements (parallax)
#define GATE_COL     0xF81F  // neon magenta/purple gate color

// Ship mode slide pot range
#define SHIP_Y_MIN   (GAME_TOP + 2)
#define SHIP_Y_MAX   (GROUND_Y - SHIP_H - 2)

// ═══════════════════════════════════════════════════════════════════════════
// TYPES
// ═══════════════════════════════════════════════════════════════════════════
typedef enum { LANG_EN = 0, LANG_ES = 1 } Lang_t;
typedef enum { LVL_STEREO = 0, LVL_DRYOUT = 1, LVL_JUMPER = 2 } Level_t;
typedef enum { MODE_CUBE, MODE_SHIP } GameMode_t;
typedef enum {
    ST_LANG, ST_TITLE, ST_LVLSEL, ST_SETTINGS,
    ST_PLAYING, ST_PAUSED, ST_DEAD, ST_WIN, ST_CHARS
} GState_t;

typedef enum {
    OBS_NONE = 0,
    OBS_SPIKE_UP,   // spike on ground pointing up
    OBS_SPIKE_DN,   // spike on ceiling pointing down
    OBS_BLOCK,      // solid square block on ground (lethal)
    OBS_BLOCK_TALL, // double-height block on ground (lethal)
    OBS_BLOCK_3,    // three-block stack on ground (lethal)
    OBS_BLOCK_QUAD, // four-block stack on ground (lethal)
    OBS_BLOCK_5,    // five-block stack on ground (lethal)
    OBS_BLOCK_6,    // six-block stack on ground (lethal)
    OBS_BLOCK_7,    // seven-block stack on ground (lethal — Jumper only)
    OBS_BLOCK_WIDE3,// three blocks side-by-side on ground (lethal, requires timed jump)
    OBS_STEP,       // elevated block you can land on top of (lethal from side/below)
    OBS_GATE,       // ceiling + floor walls with gap (ship mode)
    OBS_PORTAL_SHIP,// switches to ship mode
    OBS_PORTAL_CUBE // switches to cube mode
} ObsType_t;

typedef struct {
    int16_t   x;       // screen x (moves left each tick)
    int16_t   y;       // screen y (top of obstacle)
    ObsType_t type;
    uint8_t   active;
    uint8_t   gap;     // for OBS_GATE: gap height in px (0 → GATE_GAP default)
} Obstacle_t;

typedef struct {
    uint32_t  tick;    // game tick when obstacle enters right edge
    int16_t   y;       // screen y position
    ObsType_t type;
    uint8_t   param;   // OBS_GATE: gap size in px (0 → GATE_GAP default); others: 0
} LevelEvent_t;

// ═══════════════════════════════════════════════════════════════════════════
// LANGUAGE STRINGS
// ═══════════════════════════════════════════════════════════════════════════
typedef enum {
    PH_SELECT_LANG, PH_PLAY, PH_SETTINGS, PH_BACK,
    PH_LEVEL1, PH_LEVEL2, PH_LEVEL3, PH_SELECT_LVL,
    PH_PAUSED, PH_RESUME, PH_RETRY, PH_MENU,
    PH_YOU_DIED, PH_YOU_WIN, PH_SCORE, PH_VOLUME,
    PH_SFX, PH_ON, PH_OFF, PH_ENGLISH, PH_SPANISH,
    PH_LANGUAGE,
    PH_COUNT
} Phrase_t;

static const char *Phrases[PH_COUNT][2] = {
    /* PH_SELECT_LANG  */ {"Select Language", "Selec. Idioma"},
    /* PH_PLAY         */ {"Play",            "Jugar"},
    /* PH_SETTINGS     */ {"Settings",        "Ajustes"},
    /* PH_BACK         */ {"Back",            "Atras"},
    /* PH_LEVEL1       */ {"Stereo Madness",  "Stereo Madness"},
    /* PH_LEVEL2       */ {"Dry Out",         "Dry Out"},
    /* PH_LEVEL3       */ {"Jumper",          "Jumper"},
    /* PH_SELECT_LVL   */ {"Select Level",    "Selec. Nivel"},
    /* PH_PAUSED       */ {"PAUSED",          "PAUSADO"},
    /* PH_RESUME       */ {"Resume",          "Continuar"},
    /* PH_RETRY        */ {"Retry",           "Reintentar"},
    /* PH_MENU         */ {"Menu",            "Menu"},
    /* PH_YOU_DIED     */ {"You Died!",       "Moriste!"},
    /* PH_YOU_WIN      */ {"Level Complete!", "Nivel Completo!"},
    /* PH_SCORE        */ {"Score:",          "Punt:"},
    /* PH_VOLUME       */ {"Volume:",         "Volumen:"},
    /* PH_SFX          */ {"SFX:",            "Efectos:"},
    /* PH_ON           */ {"ON",              "SI"},
    /* PH_OFF          */ {"OFF",             "NO"},
    /* PH_ENGLISH      */ {"English",         "Ingles"},
    /* PH_SPANISH      */ {"Spanish",         "Espanol"},
    /* PH_LANGUAGE     */ {"Language",        "Idioma"},
};

// ═══════════════════════════════════════════════════════════════════════════
// LEVEL DATA
// Each entry: {game_tick_when_spawned, screen_y, obstacle_type}
// Obstacle enters from right (x=128) at that tick; y is top of sprite
// ═══════════════════════════════════════════════════════════════════════════

// Beat-synced level data (generated by gen_levels.py)
#include "BeatLevels.h"

// LEVEL 1 – Stereo Madness (hand-crafted, cube → ship → cube)
static const LevelEvent_t Lvl1_Events_OLD[] = {
    // warm-up: isolated obstacles, easy rhythm
    {  60, GROUND_Y - SPIKE_H,           OBS_SPIKE_UP},
    { 120, GROUND_Y - BLOCK_H,           OBS_BLOCK},
    { 180, GROUND_Y - SPIKE_H,           OBS_SPIKE_UP},
    { 240, GROUND_Y - SPIKE_H,           OBS_SPIKE_UP},
    // first double
    { 310, GROUND_Y - BLOCK_H,           OBS_BLOCK},
    { 370, GROUND_Y - SPIKE_H,           OBS_SPIKE_UP},
    { 385, GROUND_Y - SPIKE_H,           OBS_SPIKE_UP},
    // block+tall combo
    { 440, GROUND_Y - BLOCK_H,           OBS_BLOCK},
    { 460, GROUND_Y - BLOCK_H - BLOCK_H, OBS_BLOCK_TALL},
    // triple spike (needs double jump)
    { 520, GROUND_Y - SPIKE_H,           OBS_SPIKE_UP},
    { 535, GROUND_Y - SPIKE_H,           OBS_SPIKE_UP},
    { 550, GROUND_Y - SPIKE_H,           OBS_SPIKE_UP},
    // ── ship section ─────────────────────────────────────────────────────
    { 600, GROUND_Y - 52,                OBS_PORTAL_SHIP},
    { 650, 55,  OBS_GATE},
    { 710, 80,  OBS_GATE},
    { 770, 40,  OBS_GATE},
    { 830, 65,  OBS_GATE},
    { 890, 50,  OBS_GATE},
    // ── back to cube ─────────────────────────────────────────────────────
    { 930, GROUND_Y - 52,                OBS_PORTAL_CUBE},
    // harder cube section with ceiling spikes
    {1000, GROUND_Y - SPIKE_H,           OBS_SPIKE_UP},
    {1015, GROUND_Y - SPIKE_H,           OBS_SPIKE_UP},
    {1070, GROUND_Y - BLOCK_H - BLOCK_H, OBS_BLOCK_TALL},
    {1130, GROUND_Y - SPIKE_H,           OBS_SPIKE_UP},
    {1145, GAME_TOP,                      OBS_SPIKE_DN},
    {1200, GROUND_Y - BLOCK_H,           OBS_BLOCK},
    {1260, GROUND_Y - SPIKE_H,           OBS_SPIKE_UP},
    {1275, GROUND_Y - SPIKE_H,           OBS_SPIKE_UP},
    {1290, GROUND_Y - SPIKE_H,           OBS_SPIKE_UP},
    {1350, GROUND_Y - BLOCK_H,           OBS_BLOCK},
    {1400, GROUND_Y - SPIKE_H,           OBS_SPIKE_UP},
    {1415, GAME_TOP,                      OBS_SPIKE_DN},
    {1470, GROUND_Y - BLOCK_H - BLOCK_H, OBS_BLOCK_TALL},
    {1530, GROUND_Y - SPIKE_H,           OBS_SPIKE_UP},
    {1545, GROUND_Y - SPIKE_H,           OBS_SPIKE_UP},
    {1600, GROUND_Y - BLOCK_H,           OBS_BLOCK},
    {1660, GROUND_Y - SPIKE_H,           OBS_SPIKE_UP},
    {1675, GROUND_Y - SPIKE_H,           OBS_SPIKE_UP},
    {1690, GROUND_Y - SPIKE_H,           OBS_SPIKE_UP},
    {1730, GROUND_Y - BLOCK_H,           OBS_BLOCK},
    {1760, GROUND_Y - SPIKE_H,           OBS_SPIKE_UP},
};
#define LVL1_COUNT_OLD (sizeof(Lvl1_Events_OLD)/sizeof(Lvl1_Events_OLD[0]))
#define LVL1_END_OLD   1800

// LEVEL 2 – Dry Out (cube → ship mode @ tick 600, back to cube @ tick 1200)
static const LevelEvent_t Lvl2_Events_OLD[] = {
    { 60,  GROUND_Y - SPIKE_H,      OBS_SPIKE_UP},
    {110,  GROUND_Y - BLOCK_H,      OBS_BLOCK},
    {160,  GROUND_Y - SPIKE_H,      OBS_SPIKE_UP},
    {175,  GROUND_Y - SPIKE_H,      OBS_SPIKE_UP},
    {230,  GROUND_Y - BLOCK_H,      OBS_BLOCK},
    {280,  GROUND_Y - SPIKE_H,      OBS_SPIKE_UP},
    {340,  GROUND_Y - BLOCK_H,      OBS_BLOCK},
    {360,  GROUND_Y - BLOCK_H - BLOCK_H, OBS_BLOCK_TALL},
    {420,  GROUND_Y - SPIKE_H,      OBS_SPIKE_UP},
    {435,  GROUND_Y - SPIKE_H,      OBS_SPIKE_UP},
    {490,  GROUND_Y - BLOCK_H,      OBS_BLOCK},
    {545,  GROUND_Y - SPIKE_H,      OBS_SPIKE_UP},
    // portal to ship mode at tick 600
    {600,  GROUND_Y - 52,           OBS_PORTAL_SHIP},
    // ship mode: gates (gap_y is the center of the gap, gap size 30px)
    // OBS_GATE: y = top of gap; gap e7xtends 30px down
    {660,  50,   OBS_GATE},
    {720,  70,   OBS_GATE},
    {780,  40,   OBS_GATE},
    {840,  80,   OBS_GATE},
    {900,  55,   OBS_GATE},
    {960,  65,   OBS_GATE},
    {1020, 45,   OBS_GATE},
    {1080, 75,   OBS_GATE},
    {1140, 60,   OBS_GATE},
    // portal back to cube at tick 1200
    {1200, GROUND_Y - 52,           OBS_PORTAL_CUBE},
    // cube section resumes
    {1260, GROUND_Y - SPIKE_H,      OBS_SPIKE_UP},
    {1310, GROUND_Y - BLOCK_H,      OBS_BLOCK},
    {1360, GROUND_Y - SPIKE_H,      OBS_SPIKE_UP},
    {1375, GROUND_Y - SPIKE_H,      OBS_SPIKE_UP},
    {1420, GROUND_Y - BLOCK_H - BLOCK_H, OBS_BLOCK_TALL},
    {1480, GROUND_Y - SPIKE_H,      OBS_SPIKE_UP},
    {1540, GROUND_Y - SPIKE_H,      OBS_SPIKE_UP},
    {1555, GROUND_Y - SPIKE_H,      OBS_SPIKE_UP},
    {1570, GROUND_Y - SPIKE_H,      OBS_SPIKE_UP},
    {1630, GROUND_Y - BLOCK_H,      OBS_BLOCK},
    {1690, GROUND_Y - SPIKE_H,      OBS_SPIKE_UP},
    {1750, GROUND_Y - BLOCK_H,      OBS_BLOCK},
};
#define LVL2_COUNT_OLD (sizeof(Lvl2_Events_OLD)/sizeof(Lvl2_Events_OLD[0]))
#define LVL2_END_OLD   1800

// LEVEL 3 – Jumper (cube with fast patterns, brief ship section)
static const LevelEvent_t Lvl3_Events_OLD[] = {
    { 45,  GROUND_Y - SPIKE_H,      OBS_SPIKE_UP},
    { 80,  GROUND_Y - SPIKE_H,      OBS_SPIKE_UP},
    {120,  GROUND_Y - BLOCK_H,      OBS_BLOCK},
    {135,  GROUND_Y - SPIKE_H,      OBS_SPIKE_UP},
    {165,  GROUND_Y - SPIKE_H,      OBS_SPIKE_UP},
    {180,  GROUND_Y - SPIKE_H,      OBS_SPIKE_UP},
    {220,  GROUND_Y - BLOCK_H,      OBS_BLOCK},
    {235,  GROUND_Y - BLOCK_H - BLOCK_H, OBS_BLOCK_TALL},
    {280,  GROUND_Y - SPIKE_H,      OBS_SPIKE_UP},
    {295,  GROUND_Y - SPIKE_H,      OBS_SPIKE_UP},
    {310,  GROUND_Y - SPIKE_H,      OBS_SPIKE_UP},
    {360,  GROUND_Y - BLOCK_H,      OBS_BLOCK},
    {375,  GROUND_Y - SPIKE_H,      OBS_SPIKE_UP},
    {420,  GROUND_Y - SPIKE_H,      OBS_SPIKE_UP},
    {435,  GROUND_Y - SPIKE_H,      OBS_SPIKE_UP},
    {450,  GROUND_Y - SPIKE_H,      OBS_SPIKE_UP},
    {465,  GROUND_Y - SPIKE_H,      OBS_SPIKE_UP},
    {510,  GROUND_Y - BLOCK_H,      OBS_BLOCK},
    {530,  GROUND_Y - BLOCK_H - BLOCK_H, OBS_BLOCK_TALL},
    // ship portal at tick 570
    {570,  GROUND_Y - 52,           OBS_PORTAL_SHIP},
    {620,  45,   OBS_GATE},
    {660,  80,   OBS_GATE},
    {700,  55,   OBS_GATE},
    {740,  70,   OBS_GATE},
    {780,  40,   OBS_GATE},
    {820,  75,   OBS_GATE},
    // cube portal at tick 860
    {860,  GROUND_Y - 52,           OBS_PORTAL_CUBE},
    {910,  GROUND_Y - SPIKE_H,      OBS_SPIKE_UP},
    {925,  GROUND_Y - SPIKE_H,      OBS_SPIKE_UP},
    {940,  GROUND_Y - SPIKE_H,      OBS_SPIKE_UP},
    {970,  GROUND_Y - BLOCK_H,      OBS_BLOCK},
    {985,  GROUND_Y - SPIKE_H,      OBS_SPIKE_UP},
    {1020, GROUND_Y - SPIKE_H,      OBS_SPIKE_UP},
    {1035, GROUND_Y - SPIKE_H,      OBS_SPIKE_UP},
    {1060, GROUND_Y - BLOCK_H,      OBS_BLOCK},
    {1075, GROUND_Y - BLOCK_H - BLOCK_H, OBS_BLOCK_TALL},
    {1120, GROUND_Y - SPIKE_H,      OBS_SPIKE_UP},
    {1135, GROUND_Y - SPIKE_H,      OBS_SPIKE_UP},
    {1150, GROUND_Y - SPIKE_H,      OBS_SPIKE_UP},
    {1165, GROUND_Y - SPIKE_H,      OBS_SPIKE_UP},
    {1200, GROUND_Y - BLOCK_H,      OBS_BLOCK},
    {1240, GROUND_Y - SPIKE_H,      OBS_SPIKE_UP},
    {1280, GROUND_Y - SPIKE_H,      OBS_SPIKE_UP},
    {1295, GROUND_Y - SPIKE_H,      OBS_SPIKE_UP},
    {1340, GROUND_Y - BLOCK_H,      OBS_BLOCK},
    {1355, GROUND_Y - BLOCK_H - BLOCK_H, OBS_BLOCK_TALL},
    {1400, GROUND_Y - SPIKE_H,      OBS_SPIKE_UP},
    {1415, GROUND_Y - SPIKE_H,      OBS_SPIKE_UP},
    {1430, GROUND_Y - SPIKE_H,      OBS_SPIKE_UP},
    {1445, GROUND_Y - SPIKE_H,      OBS_SPIKE_UP},
    {1460, GROUND_Y - SPIKE_H,      OBS_SPIKE_UP},
    {1510, GROUND_Y - BLOCK_H,      OBS_BLOCK},
    {1555, GROUND_Y - SPIKE_H,      OBS_SPIKE_UP},
    {1600, GROUND_Y - SPIKE_H,      OBS_SPIKE_UP},
    {1615, GROUND_Y - SPIKE_H,      OBS_SPIKE_UP},
    {1650, GROUND_Y - BLOCK_H,      OBS_BLOCK},
    {1665, GROUND_Y - BLOCK_H - BLOCK_H, OBS_BLOCK_TALL},
    {1710, GROUND_Y - SPIKE_H,      OBS_SPIKE_UP},
    {1725, GROUND_Y - SPIKE_H,      OBS_SPIKE_UP},
    {1760, GROUND_Y - SPIKE_H,      OBS_SPIKE_UP},
    {1775, GROUND_Y - SPIKE_H,      OBS_SPIKE_UP},
};
#define LVL3_COUNT_OLD (sizeof(Lvl3_Events_OLD)/sizeof(Lvl3_Events_OLD[0]))
#define LVL3_END_OLD   1800

static const char *MusicFiles[3] = {"STEREO.BIN", "DRYOUT.BIN", "JUMPER.BIN"};
// per-level background: Stereo=dark purple, Dry Out=dark red, Jumper=dark navy
static const uint16_t LvlBG[3]   = {0x3010, 0x6000, 0x0018};
static uint16_t SceneBG = 0x3010;

// ═══════════════════════════════════════════════════════════════════════════
// GLOBALS
// ═══════════════════════════════════════════════════════════════════════════
void PLL_Init(void){ Clock_Init80MHz(0); }

static volatile uint32_t Semaphore = 0;
static GState_t   GameState  = ST_TITLE;
static Lang_t     MyLang     = LANG_EN;
static Level_t    MyLevel    = LVL_STEREO;
static GameMode_t GameMode   = MODE_CUBE;
static uint8_t    SFX_On     = 1;
static uint8_t    VolLevel   = 6;

// player state
static int16_t  PlayerY   = 0;
static int16_t  PlayerVY  = 0;
static uint8_t  JumpsLeft = 0;   // for double-jump
static uint8_t  OnGround  = 0;

// game state
static uint32_t GameTick  = 0;
static uint32_t Score     = 0;
static uint32_t HighScore[3] = {0, 0, 0};
static uint32_t EvtIdx    = 0;   // next event index in current level
static uint32_t LvlEnd    = 0;
static Obstacle_t Obs[MAX_OBS];

// input (ISR-written, main reads)
static volatile uint32_t ISR_Switches = 0;
static volatile uint32_t ISR_SlidePos = 2048;

// prev switch state for edge detection
static uint32_t PrevSw = 0;

// cube rotation (0-7 = ×45° CW; accumulates across jumps — 2 steps per jump = 90°)
static uint8_t  CubeRot      = 0;
static uint8_t  CubeRotTimer = 0;

// character color selection (substituted into cube sprite at draw time)
static uint16_t CubeBody = 0xFEE0;  // Default golden-yellow
static uint16_t CubeFace = 0xFFE0;  // Default cyan face
static const uint16_t CharColors[6][2] = {
    {0xFEE0, 0xFFE0},  // Default: Gold body, Cyan face
    {GD_BLUE, GD_WHITE},
    {GD_RED, GD_WHITE},
    {GD_YELLOW, GD_BLUE},
    {GD_PURPLE, GD_WHITE},
    {0x0000, 0x0000},  // Rainbow — color set dynamically at runtime
};
static const char *CharNames[6] = {"Default", "Blue  ", "Red   ", "Yellow", "Purple", "Rainbow"};
#define NUM_CHARS 6
static uint8_t Invincible = 0;  // 1 = rainbow debug mode, collisions ignored
// 6-step rainbow palette cycled each game tick when Invincible is set
static const uint16_t RainbowPal[6] = {
    0x001F,  // red   (BGR565)
    0x03FF,  // orange
    0x07E0,  // green
    0xFFE0,  // cyan
    0xF800,  // blue
    0xF81F,  // magenta
};

// set to 1 when starting a fresh level; 0 when resuming from pause
static uint8_t  NeedLevelSetup = 1;

// background parallax elements
#define N_STARS 32
static uint8_t StarX[N_STARS], StarY[N_STARS];

// ═══════════════════════════════════════════════════════════════════════════
// UTILITY
// ═══════════════════════════════════════════════════════════════════════════
static uint32_t Rand32(void){
    static uint32_t M = 0xDEADBEEF;
    M = 1664525*M + 1013904223;
    return M;
}

static inline const char *Ph(Phrase_t p){ return Phrases[p][MyLang]; }

// ═══════════════════════════════════════════════════════════════════════════
// DRAWING
// ═══════════════════════════════════════════════════════════════════════════
static uint8_t DrawOpaque = 0;
static uint16_t DrawOpaqueBG = 0;

static inline uint16_t SubstCubeColor(uint16_t px){
    if(px == 0xFEE0) return CubeBody;
    if(px == 0x07E0) return CubeFace;
    return px;
}

static void DrawBitmap16(int16_t x, int16_t y, const uint16_t *data, uint8_t w, uint8_t h){
    for(int row = 0; row < h; row++){
        for(int col = 0; col < w; col++){
            uint16_t px = data[row * w + col];
            if(px != GD_BLACK)        ST7735_DrawPixel(x + col, y + row, px);
            else if(DrawOpaque)       ST7735_DrawPixel(x + col, y + row, DrawOpaqueBG);
        }
    }
}

static void DrawBitmapColorized(int16_t x, int16_t y, const uint16_t *data, uint8_t w, uint8_t h){
    for(int row = 0; row < h; row++){
        for(int col = 0; col < w; col++){
            uint16_t px = SubstCubeColor(data[row * w + col]);
            if(px != GD_BLACK)        ST7735_DrawPixel(x + col, y + row, px);
            else if(DrawOpaque)       ST7735_DrawPixel(x + col, y + row, DrawOpaqueBG);
        }
    }
}

static void EraseRect(int16_t x, int16_t y, uint8_t w, uint8_t h, uint16_t bg){
    ST7735_FillRect(x, y, w, h, bg);
}

static uint16_t Blend565(uint16_t a, uint16_t b, uint8_t t, uint8_t max){
    uint16_t ar = (a >> 11) & 0x1F, ag = (a >> 5) & 0x3F, ab = a & 0x1F;
    uint16_t br = (b >> 11) & 0x1F, bg = (b >> 5) & 0x3F, bb = b & 0x1F;
    uint16_t r = (uint16_t)((ar * (max - t) + br * t) / max);
    uint16_t g = (uint16_t)((ag * (max - t) + bg * t) / max);
    uint16_t bl = (uint16_t)((ab * (max - t) + bb * t) / max);
    uint16_t out = (uint16_t)((r << 11) | (g << 5) | bl);
    return out ? out : 0x0001;
}

static void DrawCube(int16_t x, int16_t y){
    for(int row = 0; row < CUBE_H; row++)
        for(int col = 0; col < CUBE_W; col++){
            uint16_t px = SubstCubeColor(CubeSprite[row][col]);
            if(px != GD_BLACK) ST7735_DrawPixel(x+col, y+row, px);
        }
}

// 8-state rotation: sin/cos * 1024, indexed by rot=0..7 (each 45° CW)
static const int16_t RotSin8[8] = {   0,  724, 1024,  724,    0, -724, -1024, -724};
static const int16_t RotCos8[8] = {1024,  724,    0, -724, -1024, -724,     0,  724};

// Draw cube rotated by rot*45° CW (rot=0..7).
// Uses inverse-map nearest-neighbor: for each output pixel, back-project to source.
// Pixels mapping outside sprite bounds become transparent (0x0000).
static void DrawCubeRotated(int16_t x, int16_t y, uint8_t rot){
    if(rot == 0){ DrawCube(x, y); return; }
    uint16_t tmp[CUBE_H * CUBE_W];
    const uint16_t *src = (const uint16_t*)CubeSprite;
    int16_t sn = RotSin8[rot & 7];
    int16_t cs = RotCos8[rot & 7];
    for(int or_ = 0; or_ < CUBE_H; or_++){
        for(int oc = 0; oc < CUBE_W; oc++){
            // 2x fixed-point coords relative to sprite center (4.5, 4.5)
            int32_t dr2 = 2*or_ - 9;
            int32_t dc2 = 2*oc  - 9;
            // back-project for CW rotation in screen (y-down) coords
            int32_t sr2 = ((int32_t)dr2 * cs - (int32_t)dc2 * sn) / 1024;
            int32_t sc2 = ((int32_t)dr2 * sn + (int32_t)dc2 * cs) / 1024;
            // convert 2x coords back to pixel index (nearest neighbor)
            int16_t sr = (int16_t)((sr2 + 9) / 2);
            int16_t sc = (int16_t)((sc2 + 9) / 2);
            tmp[or_ * CUBE_W + oc] =
                (sr >= 0 && sr < CUBE_H && sc >= 0 && sc < CUBE_W)
                ? SubstCubeColor(src[sr * CUBE_W + sc]) : 0x0000;
        }
    }
    DrawBitmap16(x, y, tmp, CUBE_W, CUBE_H);
}

static void DrawShip(int16_t x, int16_t y){
    DrawBitmapColorized(x, y, (const uint16_t*)ShipSprite, SHIP_W, SHIP_H);
}

static void DrawSpikeUp(int16_t x, int16_t y){
    for(int row = 0; row < SPIKE_H; row++){
        int left = 3 - row / 2;
        int right = 4 + row / 2;
        if(left < 0) left = 0;
        if(right >= SPIKE_W) right = SPIKE_W - 1;
        for(int col = left; col <= right; col++){
            uint16_t px = (col == left || col == right || row == SPIKE_H - 1)
                          ? GD_WHITE : Blend565(GD_BLACK, SceneBG, (uint8_t)row, SPIKE_H - 1);
            ST7735_DrawPixel(x + col, y + row, px);
        }
    }
}

static void DrawSpikeDn(int16_t x, int16_t y){
    for(int row = 0; row < SPIKE_H; row++){
        int srcRow = SPIKE_H - 1 - row;
        int left = 3 - srcRow / 2;
        int right = 4 + srcRow / 2;
        if(left < 0) left = 0;
        if(right >= SPIKE_W) right = SPIKE_W - 1;
        for(int col = left; col <= right; col++){
            uint16_t px = (col == left || col == right || row == 0)
                          ? GD_WHITE : Blend565(GD_BLACK, SceneBG, (uint8_t)srcRow, SPIKE_H - 1);
            ST7735_DrawPixel(x + col, y + row, px);
        }
    }
}

static void DrawBlock(int16_t x, int16_t y){
    for(int row = 0; row < BLOCK_H; row++){
        for(int col = 0; col < BLOCK_W; col++){
            uint16_t px;
            if(row == 0 || row == BLOCK_H - 1 || col == 0 || col == BLOCK_W - 1){
                px = GD_WHITE;
            } else {
                px = Blend565(GD_BLACK, SceneBG, (uint8_t)row, BLOCK_H - 1);
            }
            ST7735_DrawPixel(x + col, y + row, px);
        }
    }
}

static void DrawBlockTall(int16_t x, int16_t y){
    DrawBlock(x, y);
    DrawBlock(x, y + BLOCK_H);
}

static void DrawBlock3(int16_t x, int16_t y){
    DrawBlock(x, y);
    DrawBlock(x, y + BLOCK_H);
    DrawBlock(x, y + BLOCK_H*2);
}

static void DrawBlockQuad(int16_t x, int16_t y){
    DrawBlock(x, y);
    DrawBlock(x, y + BLOCK_H);
    DrawBlock(x, y + BLOCK_H*2);
    DrawBlock(x, y + BLOCK_H*3);
}

static void DrawBlock5(int16_t x, int16_t y){
    DrawBlock(x, y);
    DrawBlock(x, y + BLOCK_H);
    DrawBlock(x, y + BLOCK_H*2);
    DrawBlock(x, y + BLOCK_H*3);
    DrawBlock(x, y + BLOCK_H*4);
}

static void DrawBlock6(int16_t x, int16_t y){
    DrawBlock(x, y);
    DrawBlock(x, y + BLOCK_H);
    DrawBlock(x, y + BLOCK_H*2);
    DrawBlock(x, y + BLOCK_H*3);
    DrawBlock(x, y + BLOCK_H*4);
    DrawBlock(x, y + BLOCK_H*5);
}

static void DrawBlock7(int16_t x, int16_t y){
    DrawBlock(x, y);
    DrawBlock(x, y + BLOCK_H);
    DrawBlock(x, y + BLOCK_H*2);
    DrawBlock(x, y + BLOCK_H*3);
    DrawBlock(x, y + BLOCK_H*4);
    DrawBlock(x, y + BLOCK_H*5);
    DrawBlock(x, y + BLOCK_H*6);
}

static void DrawBlockWide3(int16_t x, int16_t y){
    if(x + BLOCK_W > 0 && x < SCREEN_W)                        DrawBlock(x, y);
    if(x + BLOCK_W*2 > 0 && x + BLOCK_W < SCREEN_W)            DrawBlock(x + BLOCK_W, y);
    if(x + BLOCK_W*3 > 0 && x + BLOCK_W*2 < SCREEN_W)          DrawBlock(x + BLOCK_W*2, y);
}

static void DrawBlockStep(int16_t x, int16_t y){
    DrawBlock(x, y);
    // cyan top-edge line marks it as landable
    ST7735_DrawFastHLine(x, y, BLOCK_W, GD_CYAN);
}

// Gate: spike-tipped walls with variable gap.
// SpikeDn tip sits at y (gap top edge); SpikeUp tip sits at y+gap (gap bot edge).
#define GATE_GAP 42  // default gap used by non-cascade gates
static void DrawGateSpiked(int16_t x, int16_t y, uint8_t gap, uint16_t col){
    if(x + SPIKE_W <= 0 || x >= SCREEN_W) return;
    int16_t rx = (x < 0) ? 0 : x;
    int16_t rw = SPIKE_W - (rx - x);
    if(rw <= 0) return;
    // top pillar body (GAME_TOP → spike base)
    int16_t top_spike_y = y - (SPIKE_H - 1);
    int16_t top_body_h  = top_spike_y - GAME_TOP;
    if(top_body_h > 0) ST7735_FillRect(rx, GAME_TOP, rw, top_body_h, col);
    if(top_spike_y >= GAME_TOP) DrawSpikeDn(x, top_spike_y);
    // bottom spike: tip at y+gap
    DrawSpikeUp(x, y + gap);
    int16_t bot_body_y = y + gap + SPIKE_H;
    int16_t bot_body_h = GROUND_Y - bot_body_y;
    if(bot_body_h > 0) ST7735_FillRect(rx, bot_body_y, rw, bot_body_h, col);
}

// Portal: 14x50px oval ring with gear teeth — GD portal aesthetic
static void DrawPortal(int16_t x, int16_t y, uint8_t toShip){
    uint16_t ring  = toShip ? GD_YELLOW : GD_CYAN;
    uint16_t glow  = GD_WHITE;
    uint16_t tooth = GD_BLACK;
    for(int16_t yy = 4; yy <= 42; yy += 5){
        ST7735_FillRect(x + 14, y + yy, 4, 2, tooth);
    }

    static const int8_t left[50] = {
        6,5,4,3,3,2,2,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,2,2,3,3,4,5,6
    };
    static const int8_t right[50] = {
        8,9,10,11,11,12,12,13,13,13,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,
        14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,13,13,13,12,12,11,11,10,9,8
    };

    // Optimize: segments 0-9 and 40-49 are curved, segments 10-39 are vertical lines
    for(int16_t row = 0; row < 10; row++){
        ST7735_DrawPixel(x + left[row],     y + row, ring);
        ST7735_DrawPixel(x + left[row] + 1, y + row, glow);
        ST7735_DrawPixel(x + right[row],     y + row, ring);
        ST7735_DrawPixel(x + right[row] - 1, y + row, glow);
    }
    // Vertical straight segments
    ST7735_FillRect(x,     y + 10, 1, 30, ring);
    ST7735_FillRect(x + 1, y + 10, 1, 30, glow);
    ST7735_FillRect(x + 14, y + 10, 1, 30, ring);
    ST7735_FillRect(x + 13, y + 10, 1, 30, glow);

    for(int16_t row = 40; row < 50; row++){
        ST7735_DrawPixel(x + left[row],     y + row, ring);
        ST7735_DrawPixel(x + left[row] + 1, y + row, glow);
        ST7735_DrawPixel(x + right[row],     y + row, ring);
        ST7735_DrawPixel(x + right[row] - 1, y + row, glow);
    }

    ST7735_DrawFastHLine(x + 5, y,      5, glow);
    ST7735_DrawFastHLine(x + 5, y + 49, 5, glow);
    ST7735_DrawFastHLine(x + 4, y + 1,  7, ring);
    ST7735_DrawFastHLine(x + 4, y + 48, 7, ring);

    ST7735_FillRect(x - 6, y + 10, 2, 2, ring);
    ST7735_FillRect(x - 9, y + 18, 2, 2, glow);
    ST7735_DrawPixel(x - 5, y + 27, ring);
    ST7735_FillRect(x - 8, y + 36, 2, 2, ring);
    ST7735_DrawPixel(x + 6, y + 22, glow);
    ST7735_DrawPixel(x + 9, y + 29, ring);
}

static void DrawHUD(uint16_t bg){
    ST7735_FillRect(0, 0, SCREEN_W, HUD_H, GD_BLACK);
    ST7735_SetCursor(0, 0);
    ST7735_SetTextColor(GD_YELLOW);  ST7735_OutString("S:");
    ST7735_SetTextColor(GD_WHITE);   ST7735_OutUDec(Score);
    ST7735_SetTextColor(GD_CYAN);    ST7735_OutString(" Hi:");
    ST7735_SetTextColor(GD_WHITE);   ST7735_OutUDec(HighScore[MyLevel]);
    uint16_t barW = (LvlEnd > 0) ? (uint16_t)(GameTick * 50u / LvlEnd) : 0;
    if(barW > 50) barW = 50;
    ST7735_FillRect(76, 2, 50, HUD_H - 4, 0x2104);
    if(barW > 0) ST7735_FillRect(76, 2, barW, HUD_H - 4, GD_GREEN);
}

static void DrawGround(uint16_t bg){
    ST7735_FillRect(0, GROUND_Y, SCREEN_W, SCREEN_H - GROUND_Y, GD_GBROWN);
    ST7735_DrawFastHLine(0, GROUND_Y, SCREEN_W, GD_WHITE);
}

static void InitStars(uint16_t bg){
    for(int i = 0; i < N_STARS; i++){
        StarX[i] = (uint8_t)(Rand32() % SCREEN_W);
        StarY[i] = (uint8_t)(GAME_TOP + Rand32() % (GROUND_Y - GAME_TOP));
        uint16_t sc = (i % 5 == 0) ? GD_WHITE : STAR_COL;
        ST7735_DrawPixel(StarX[i], StarY[i], sc);
    }
}

static void ScrollStars(uint16_t bg){
    for(int i = 0; i < N_STARS; i++){
        uint16_t sc = (i % 5 == 0) ? GD_WHITE : STAR_COL;
        ST7735_DrawPixel(StarX[i], StarY[i], bg); // erase
        StarX[i] -= 1;
        if((int8_t)StarX[i] < 0){
            StarX[i] = SCREEN_W - 1;
            StarY[i] = (uint8_t)(GAME_TOP + Rand32() % (GROUND_Y - GAME_TOP));
        }
        ST7735_DrawPixel(StarX[i], StarY[i], sc);
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// COLLISION DETECTION  (AABB, inset hitbox for forgiveness)
// ═══════════════════════════════════════════════════════════════════════════
static uint8_t AABBHit(int16_t ax, int16_t ay, int16_t aw, int16_t ah,
                        int16_t bx, int16_t by, int16_t bw, int16_t bh){
    return (ax < bx+bw) && (ax+aw > bx) &&
           (ay < by+bh) && (ay+ah > by);
}

static uint8_t PlayerHit(Obstacle_t *o){
    int16_t pw, ph;
    // Cube: 2px left inset only; right edge flush with sprite right (PLAYER_X+CUBE_W=30).
    // This prevents the 6px visual-overlap bug caused by the old 4px-wide hitbox.
    if(GameMode == MODE_CUBE){ pw = CUBE_W - 2; ph = CUBE_H - 4; }
    else                      { pw = SHIP_W - 4; ph = SHIP_H - 2; }
    int16_t px = PLAYER_X + 2, py = PlayerY + 2;

    switch(o->type){
        case OBS_SPIKE_UP:
            return AABBHit(px, py, pw, ph, o->x, o->y+2, SPIKE_W-2, SPIKE_H-3);
        case OBS_SPIKE_DN:
            return AABBHit(px, py, pw, ph, o->x+1, o->y, SPIKE_W-2, SPIKE_H-2);
        case OBS_BLOCK:
            return AABBHit(px, py, pw, ph, o->x, o->y+1, BLOCK_W-2, BLOCK_H-2);
        case OBS_BLOCK_TALL:
            return AABBHit(px, py, pw, ph, o->x, o->y+1, BLOCK_W-2, BLOCK_H*2-2);
        case OBS_BLOCK_3:
            return AABBHit(px, py, pw, ph, o->x, o->y+1, BLOCK_W-2, BLOCK_H*3-2);
        case OBS_BLOCK_QUAD:
            return AABBHit(px, py, pw, ph, o->x, o->y+1, BLOCK_W-2, BLOCK_H*4-2);
        case OBS_BLOCK_5:
            return AABBHit(px, py, pw, ph, o->x, o->y+1, BLOCK_W-2, BLOCK_H*5-2);
        case OBS_BLOCK_6:
            return AABBHit(px, py, pw, ph, o->x, o->y+1, BLOCK_W-2, BLOCK_H*6-2);
        case OBS_BLOCK_7:
            return AABBHit(px, py, pw, ph, o->x, o->y+1, BLOCK_W-2, BLOCK_H*7-2);
        case OBS_BLOCK_WIDE3:
            return AABBHit(px, py, pw, ph, o->x, o->y+1, BLOCK_W*3-2, BLOCK_H-2);
        case OBS_STEP:
            // safe from above (landing handled by physics); lethal from side/below
            if(PlayerY + ph <= o->y + 2) return 0;
            return AABBHit(px, py, pw, ph, o->x, o->y+1, BLOCK_W-2, BLOCK_H-2);
        case OBS_GATE:{
            // hit if outside the gap (use per-obstacle gap, default GATE_GAP)
            uint8_t g = o->gap ? o->gap : GATE_GAP;
            int16_t gapBot = o->y + g;
            if(AABBHit(px, py, pw, ph, o->x, GAME_TOP, SPIKE_W, o->y - GAME_TOP))
                return 1;
            if(AABBHit(px, py, pw, ph, o->x, gapBot, SPIKE_W, GROUND_Y - gapBot))
                return 1;
            return 0;
        }
        default: return 0;
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// GAME LOGIC
// ═══════════════════════════════════════════════════════════════════════════
static const LevelEvent_t *CurEvents = 0;
static uint32_t CurEvtCount = 0;

static void LevelSetup(Level_t lv, uint16_t bg){
    SceneBG = bg;
    GameTick = 0; Score = 0; EvtIdx = 0;
    PlayerY  = GROUND_Y - CUBE_H;
    PlayerVY = 0; OnGround = 1; JumpsLeft = DOUBLE_JUMP;
    GameMode = MODE_CUBE;
    CubeRot = 0; CubeRotTimer = 0;
    for(int i = 0; i < MAX_OBS; i++) Obs[i].active = 0;

    switch(lv){
        case LVL_STEREO: CurEvents=Lvl1_Events; CurEvtCount=LVL1_COUNT; LvlEnd=LVL1_END; break;
        case LVL_DRYOUT: CurEvents=Lvl2_Events; CurEvtCount=LVL2_COUNT; LvlEnd=LVL2_END; break;
        case LVL_JUMPER: CurEvents=Lvl3_Events; CurEvtCount=LVL3_COUNT; LvlEnd=LVL3_END; break;
    }

    ST7735_FillScreen(bg);
    DrawGround(bg);
    DrawHUD(bg);
    InitStars(bg);
}

static void SpawnObstacle(ObsType_t type, int16_t y, uint8_t gap){
    for(int i = 0; i < MAX_OBS; i++){
        if(!Obs[i].active){
            Obs[i].x      = SCREEN_W;
            Obs[i].y      = y;
            Obs[i].type   = type;
            Obs[i].active = 1;
            Obs[i].gap    = gap ? gap : GATE_GAP;
            return;
        }
    }
}

// Returns 1 if player died this tick
static uint8_t GameTick_Update(uint32_t sw, uint32_t slide, uint16_t bg){
    uint32_t newSw = sw;
    uint32_t risingEdge = newSw & ~PrevSw;
    PrevSw = newSw;

    // ── player physics (cube mode) ──────────────────────────────────────
    if(GameMode == MODE_CUBE){
        // jump on rising edge of jump button
        if(risingEdge & 0x01){
            if(OnGround || JumpsLeft > 0){
                PlayerVY = JUMP_VEL;
                if(!OnGround) JumpsLeft--;
                if(SFX_On) Sound_Jump();
            }
        }
        PlayerY  += PlayerVY;
        PlayerVY += GRAVITY;
        // ground collision
        int16_t groundLand = GROUND_Y - CUBE_H;
        if(PlayerY >= groundLand){
            PlayerY  = groundLand;
            PlayerVY = 0;
            OnGround  = 1;
            JumpsLeft = DOUBLE_JUMP;
            if(CubeRot & 1) CubeRot = (CubeRot + 1) & 7;  // snap to nearest 90° so cube lands flat
            CubeRotTimer = 0;
        } else {
            OnGround = 0;
        }
        // ceiling clamp — bounce down, consume extra jumps so cube doesn't stick
        if(PlayerY < GAME_TOP){ PlayerY = GAME_TOP; PlayerVY = GRAVITY; JumpsLeft = 0; }
        // step block landing: check each active OBS_STEP for top collision
        for(int si = 0; si < MAX_OBS; si++){
            if(!Obs[si].active || Obs[si].type != OBS_STEP) continue;
            int16_t blockTop = Obs[si].y;
            int16_t playerBot = PlayerY + CUBE_H;
            // landing: descending player's bottom enters block top this tick
            if(PlayerVY >= 0 &&
               playerBot >= blockTop && playerBot <= blockTop + SCROLL_SPD + 4 &&
               PLAYER_X + CUBE_W - 2 > Obs[si].x && PLAYER_X + 2 < Obs[si].x + BLOCK_W){
                PlayerY      = blockTop - CUBE_H;
                PlayerVY     = 0;
                OnGround     = 1;
                JumpsLeft    = DOUBLE_JUMP;
                if(CubeRot & 1) CubeRot = (CubeRot + 1) & 7;
                CubeRotTimer = 0;
            }
            // keep player pinned on top while block scrolls under them
            if(OnGround && PlayerY + CUBE_H == blockTop &&
               PLAYER_X + CUBE_W - 2 > Obs[si].x && PLAYER_X + 2 < Obs[si].x + BLOCK_W){
                PlayerY = blockTop - CUBE_H;
            }
        }
        // rotate cube while airborne — threshold=8 → 2 steps per jump = 90° total
        if(!OnGround){
            if(++CubeRotTimer >= 8){ CubeRotTimer = 0; CubeRot = (CubeRot + 1) & 7; }
        }
        // hold-jump: button held when landing → auto-jump again (real GD behaviour)
        if((sw & 0x01) && !(risingEdge & 0x01) && OnGround){
            PlayerVY = JUMP_VEL;
            OnGround  = 0;
            JumpsLeft = DOUBLE_JUMP;
            if(SFX_On) Sound_Jump();
        }
    } else {
        // ── ship mode: slide pot controls y ────────────────────────────
        // IIR low-pass filter (75% old + 25% new) eliminates ADC noise jitter
        static uint32_t ShipSlideFiltered = 2048;
        ShipSlideFiltered = (ShipSlideFiltered * 3 + slide) >> 2;
        int32_t mapped = SHIP_Y_MIN + (int32_t)(SHIP_Y_MAX - SHIP_Y_MIN) * (int32_t)ShipSlideFiltered / 4095;
        PlayerY = (int16_t)mapped;
    }

    // ── erase player trailing edge (prevents flicker via opaque blitting) ──────
    {
        static int16_t OldShipY = GROUND_Y - SHIP_H;
        static int16_t OldCubeY = GROUND_Y - CUBE_H;
        static GameMode_t PrevMode = MODE_CUBE;

        // On portal transition, erase the entire ghost of the previous form
        if(GameMode != PrevMode){
            if(PrevMode == MODE_CUBE) EraseRect(PLAYER_X, OldCubeY, CUBE_W, CUBE_H, bg);
            else                      EraseRect(PLAYER_X, OldShipY, SHIP_W, SHIP_H, bg);
            PrevMode = GameMode;
        }

        if(GameMode == MODE_SHIP){
            if(OldShipY < PlayerY){
                int16_t eh = PlayerY - OldShipY;
                if(eh > SHIP_H) eh = SHIP_H;
                if(eh > 0) EraseRect(PLAYER_X, OldShipY, SHIP_W, (uint8_t)eh, bg);
            } else if(OldShipY > PlayerY){
                int16_t eh = OldShipY - PlayerY;
                if(eh > SHIP_H) eh = SHIP_H;
                if(eh > 0) EraseRect(PLAYER_X, OldShipY + SHIP_H - eh, SHIP_W, (uint8_t)eh, bg);
            }
            OldShipY = PlayerY;
        } else {
            if(OldCubeY < PlayerY){
                int16_t eh = PlayerY - OldCubeY;
                if(eh > CUBE_H) eh = CUBE_H;
                if(eh > 0) EraseRect(PLAYER_X, OldCubeY, CUBE_W, (uint8_t)eh, bg);
            } else if(OldCubeY > PlayerY){
                int16_t eh = OldCubeY - PlayerY;
                if(eh > CUBE_H) eh = CUBE_H;
                if(eh > 0) EraseRect(PLAYER_X, OldCubeY + CUBE_H - eh, CUBE_W, (uint8_t)eh, bg);
            }
            OldCubeY = PlayerY;
        }
    }

    // ── spawn new obstacles ──────────────────────────────────────────────
    while(EvtIdx < CurEvtCount && CurEvents[EvtIdx].tick <= GameTick){
        const LevelEvent_t *e = &CurEvents[EvtIdx];
        // param field = gate gap for OBS_GATE (0 → default GATE_GAP)
        SpawnObstacle(e->type, e->y, e->param);
        EvtIdx++;
    }

    // ── move & draw obstacles ─────────────────────────────────────────────
    uint8_t died = 0;
    for(int i = 0; i < MAX_OBS; i++){
        if(!Obs[i].active) continue;

        // erase at current position
        switch(Obs[i].type){
            case OBS_SPIKE_UP: case OBS_SPIKE_DN:
                EraseRect(Obs[i].x, GAME_TOP, SPIKE_W, GROUND_Y - GAME_TOP, bg); break;
            case OBS_BLOCK:
                EraseRect(Obs[i].x, GAME_TOP, BLOCK_W, GROUND_Y - GAME_TOP, bg); break;
            case OBS_BLOCK_TALL:
                EraseRect(Obs[i].x, GAME_TOP, BLOCK_W, GROUND_Y - GAME_TOP, bg); break;
            case OBS_BLOCK_3:
                EraseRect(Obs[i].x, GAME_TOP, BLOCK_W, GROUND_Y - GAME_TOP, bg); break;
            case OBS_BLOCK_QUAD:
                EraseRect(Obs[i].x, GAME_TOP, BLOCK_W, GROUND_Y - GAME_TOP, bg); break;
            case OBS_BLOCK_5:
                EraseRect(Obs[i].x, GAME_TOP, BLOCK_W, GROUND_Y - GAME_TOP, bg); break;
            case OBS_BLOCK_6:
                EraseRect(Obs[i].x, GAME_TOP, BLOCK_W, GROUND_Y - GAME_TOP, bg); break;
            case OBS_BLOCK_7:
                EraseRect(Obs[i].x, GAME_TOP, BLOCK_W, GROUND_Y - GAME_TOP, bg); break;
            case OBS_BLOCK_WIDE3: {
                // Trailing edge only — 30px wide block drawn over itself, only erase the strip being left behind
                int16_t trail_x = Obs[i].x + BLOCK_W*3 - SCROLL_SPD;
                if(trail_x < SCREEN_W && trail_x + SCROLL_SPD > 0){
                    int16_t ex = (trail_x < 0) ? 0 : trail_x;
                    uint8_t ew = (uint8_t)(SCROLL_SPD - (ex - trail_x));
                    if(ex + ew > SCREEN_W) ew = (uint8_t)(SCREEN_W - ex);
                    if(ew > 0) EraseRect(ex, GAME_TOP, ew, GROUND_Y - GAME_TOP, bg);
                }
                break;
            }
            case OBS_STEP:
                EraseRect(Obs[i].x, GAME_TOP, BLOCK_W, GROUND_Y - GAME_TOP, bg); break;
            case OBS_GATE: {
                // Only erase the trailing SCROLL_SPD pixels (right edge leaving the gate).
                // The gate body is redrawn directly over the old position, so no full-column
                // erase is needed — that erase-then-redraw cycle was causing the flicker.
                int16_t trail_x = Obs[i].x + SPIKE_W - SCROLL_SPD;
                if(trail_x < SCREEN_W && trail_x + SCROLL_SPD > 0){
                    int16_t ex = (trail_x < 0) ? 0 : trail_x;
                    uint8_t ew = (uint8_t)(SCROLL_SPD - (ex - trail_x));
                    if(ex + ew > SCREEN_W) ew = (uint8_t)(SCREEN_W - ex);
                    if(ew > 0) ST7735_FillRect(ex, GAME_TOP, ew, GROUND_Y - GAME_TOP, bg);
                }
                break;
            }
            case OBS_PORTAL_SHIP: case OBS_PORTAL_CUBE:
                EraseRect(Obs[i].x - 10, GAME_TOP, 30, GROUND_Y - GAME_TOP, bg); break;
            default: break;
        }

        // scroll left
        Obs[i].x -= SCROLL_SPD;

        // off-screen left → deactivate (WIDE3 is 30px so needs larger threshold)
        int16_t _offThresh = (Obs[i].type == OBS_BLOCK_WIDE3) ? BLOCK_W*3 : 20;
        if(Obs[i].x + _offThresh < 0){ Obs[i].active = 0; continue; }

        // mode-change portals (trigger when player reaches them)
        if(Obs[i].type == OBS_PORTAL_SHIP && Obs[i].x <= PLAYER_X + CUBE_W){
            GameMode = MODE_SHIP;
            Obs[i].active = 0;
            LED_On(0x02); LED_Off(0x08); // blue LED on = ship mode
            continue;
        }
        if(Obs[i].type == OBS_PORTAL_CUBE && Obs[i].x <= PLAYER_X + SHIP_W){
            GameMode = MODE_CUBE;
            PlayerY  = GROUND_Y - CUBE_H;
            PlayerVY = 0; OnGround = 1; JumpsLeft = DOUBLE_JUMP;
            CubeRot = 0; CubeRotTimer = 0;
            Obs[i].active = 0;
            LED_Off(0x02); LED_On(0x08); // green LED on = cube mode
            continue;
        }

        // draw at new position
        switch(Obs[i].type){
            case OBS_SPIKE_UP:   DrawSpikeUp(Obs[i].x, Obs[i].y); break;
            case OBS_SPIKE_DN:   DrawSpikeDn(Obs[i].x, Obs[i].y); break;
            case OBS_BLOCK:      DrawBlock(Obs[i].x, Obs[i].y);   break;
            case OBS_BLOCK_TALL:  DrawBlockTall(Obs[i].x, Obs[i].y);  break;
            case OBS_BLOCK_3:     DrawBlock3(Obs[i].x, Obs[i].y);     break;
            case OBS_BLOCK_QUAD:  DrawBlockQuad(Obs[i].x, Obs[i].y);  break;
            case OBS_BLOCK_5:     DrawBlock5(Obs[i].x, Obs[i].y);     break;
            case OBS_BLOCK_6:     DrawBlock6(Obs[i].x, Obs[i].y);     break;
            case OBS_BLOCK_7:     DrawBlock7(Obs[i].x, Obs[i].y);     break;
            case OBS_BLOCK_WIDE3: DrawBlockWide3(Obs[i].x, Obs[i].y); break;
            case OBS_STEP:        DrawBlockStep(Obs[i].x, Obs[i].y);  break;
            case OBS_GATE:
                DrawGateSpiked(Obs[i].x, Obs[i].y, Obs[i].gap, GATE_COL);
                break;
            case OBS_PORTAL_SHIP: DrawPortal(Obs[i].x, Obs[i].y, 1); break;
            case OBS_PORTAL_CUBE: DrawPortal(Obs[i].x, Obs[i].y, 0); break;
            default: break;
        }

        // collision check (only active non-portal obstacles; skipped in rainbow debug mode)
        if(Obs[i].type != OBS_PORTAL_SHIP && Obs[i].type != OBS_PORTAL_CUBE){
            if(!Invincible && PlayerHit(&Obs[i])) died = 1;
        }
    }

    // ── draw player (rainbow: cycle body color each tick) ───────────────────
    if(Invincible){
        CubeBody = RainbowPal[(GameTick / 4) % 6];
        CubeFace = GD_WHITE;
    }
    if(GameMode == MODE_CUBE) {
        DrawOpaque = 1; DrawOpaqueBG = bg;
        DrawCubeRotated(PLAYER_X, PlayerY, CubeRot);
        DrawOpaque = 0;
    } else {
        DrawOpaque = 1; DrawOpaqueBG = bg;
        DrawShip(PLAYER_X, PlayerY);
        DrawOpaque = 0;
    }
    if(Invincible){
        CubeBody = RainbowPal[0]; CubeFace = GD_WHITE; // restore to first slot (arbitrary)
    }

    // redraw ground line (may have been erased)
    DrawGround(bg);

    // ── score & HUD ──────────────────────────────────────────────────────
    Score = GameTick / 30; // seconds elapsed
    DrawHUD(bg);

    // scroll stars every 2 ticks
    if((GameTick & 1) == 0) ScrollStars(bg);

    GameTick++;

    // level complete?
    if(GameTick >= LvlEnd) return 2; // 2 = win

    return died ? 1 : 0;
}

// ═══════════════════════════════════════════════════════════════════════════
// SCREEN DRAWING HELPERS FOR MENUS
// ═══════════════════════════════════════════════════════════════════════════
static uint8_t MenuSel = 0;

// Level select: bottom-half preview scene for each level
// Preview occupies y=80..159, with mini ground at y=150
#define PREV_Y      80   // top of preview area
#define PREV_GND    150  // ground y in preview
static void DrawLevelPreview(uint8_t lvl){
    uint16_t bg = LvlBG[lvl];
    // fill preview background
    ST7735_FillRect(0, PREV_Y, SCREEN_W, SCREEN_H - PREV_Y, bg);
    // ground strip
    ST7735_FillRect(0, PREV_GND, SCREEN_W, SCREEN_H - PREV_GND, GD_GBROWN);
    ST7735_DrawFastHLine(0, PREV_GND, SCREEN_W, GD_WHITE);
    // a few static stars
    ST7735_DrawPixel(15,  85, GD_WHITE);
    ST7735_DrawPixel(42,  88, STAR_COL);
    ST7735_DrawPixel(70,  83, GD_WHITE);
    ST7735_DrawPixel(98,  90, STAR_COL);
    ST7735_DrawPixel(118, 86, GD_WHITE);
    // player cube at fixed position
    DrawCube(10, PREV_GND - CUBE_H);
    // level-specific obstacles
    static const char *bpmStr[3] = {"140 BPM", "116 BPM", "172 BPM"};
    if(lvl == 0){           // Stereo Madness: spike + block
        DrawSpikeUp(50, PREV_GND - SPIKE_H);
        DrawBlock(72, PREV_GND - BLOCK_H);
        DrawSpikeUp(93, PREV_GND - SPIKE_H);
    } else if(lvl == 1){   // Dry Out: double spike + tall block
        DrawSpikeUp(48, PREV_GND - SPIKE_H);
        DrawSpikeUp(61, PREV_GND - SPIKE_H);
        DrawBlockTall(80, PREV_GND - BLOCK_H * 2);
    } else {               // Jumper: tall block + spike + spike
        DrawBlockTall(50, PREV_GND - BLOCK_H * 2);
        DrawSpikeUp(73, PREV_GND - SPIKE_H);
        DrawBlockTall(90, PREV_GND - BLOCK_H * 2);
    }
    // BPM label in preview area
    ST7735_SetCursor(14, 18);   // row 18 = y=144, inside preview but above ground
    ST7735_SetTextColor(GD_WHITE);
    ST7735_OutStringTransparent((char*)bpmStr[lvl]);
}

// Full level-select screen: uniform level-bg color top to bottom
static void DrawLevelSelect(uint8_t sel){
    ST7735_FillRect(0, 0, SCREEN_W, SCREEN_H, LvlBG[sel]);
    ST7735_SetCursor(0, 0);
    ST7735_SetTextColor(GD_WHITE);
    ST7735_OutStringTransparent((char*)Ph(PH_SELECT_LVL));
    const char *lvlNames[3] = {Ph(PH_LEVEL1), Ph(PH_LEVEL2), Ph(PH_LEVEL3)};
    for(int i = 0; i < 3; i++){
        ST7735_SetCursor(0, 2 + i * 2);
        if(i == sel){ ST7735_SetTextColor(GD_CYAN);  ST7735_OutStringTransparent("> "); }
        else         { ST7735_SetTextColor(GD_WHITE); ST7735_OutStringTransparent("  "); }
        ST7735_OutStringTransparent((char*)lvlNames[i]);
    }
    DrawLevelPreview(sel);
}

static void ClearScreen(uint16_t col){
    ST7735_FillScreen(col);
}

// GD_CYAN (0x07FF) appears YELLOW on this BGR display — use for highlights
static void DrawMenu(const char *title, const char **items, uint8_t n, uint8_t sel){
    ClearScreen(MENU_BG);
    ST7735_SetCursor(0, 0);
    ST7735_SetTextColor(GD_WHITE);
    ST7735_OutStringTransparent((char*)title);
    for(int i = 0; i < n; i++){
        int row = 2 + i;
        ST7735_SetCursor(0, row);
        if(i == sel){
            ST7735_SetTextColor(GD_CYAN);
            ST7735_OutStringTransparent("> ");
        } else {
            ST7735_SetTextColor(GD_WHITE);
            ST7735_OutStringTransparent("  ");
        }
        ST7735_OutStringTransparent((char*)items[i]);
    }
}

// Boot/title screen with big "GEOMETRY DASH" text (size-2 DrawChar = 12×16px each)
static void DrawTitleScreen(uint8_t sel){
    ClearScreen(MENU_BG);
    // "GEOMETRY": 8 chars × 12px = 96px wide, centred at x=16
    const char *l1 = "GEOMETRY";
    for(int i = 0; l1[i]; i++)
        ST7735_DrawChar(16 + i*12, 18, l1[i], GD_CYAN, MENU_BG, 2);
    // "DASH": 4 chars × 12px = 48px wide, centred at x=40
    const char *l2 = "DASH";
    for(int i = 0; l2[i]; i++)
        ST7735_DrawChar(40 + i*12, 36, l2[i], GD_CYAN, MENU_BG, 2);
    // decorative ground strip at bottom
    ST7735_FillRect(0, SCREEN_H - 14, SCREEN_W, 14, GD_GBROWN);
    ST7735_DrawFastHLine(0, SCREEN_H - 14, SCREEN_W, GD_WHITE);
    DrawCube(10, SCREEN_H - 14 - CUBE_H);
    DrawSpikeUp(40, SCREEN_H - 14 - SPIKE_H);
    // menu items (rows 8-11 = y 64-88)
    const char *items[4] = {Ph(PH_PLAY), Ph(PH_SETTINGS), "Characters", Ph(PH_LANGUAGE)};
    for(int i = 0; i < 4; i++){
        ST7735_SetCursor(0, 8 + i);
        if(i == sel){
            ST7735_SetTextColor(GD_CYAN);
            ST7735_OutStringTransparent("> ");
        } else {
            ST7735_SetTextColor(GD_WHITE);
            ST7735_OutStringTransparent("  ");
        }
        ST7735_OutStringTransparent((char*)items[i]);
    }
}

// Settings screen: Vol / SFX / Back with scrollable cursor
static void DrawSettings(uint8_t sel){
    ClearScreen(MENU_BG);
    ST7735_SetCursor(0, 0);
    ST7735_SetTextColor(GD_WHITE);
    ST7735_OutStringTransparent((char*)Ph(PH_SETTINGS));
    // Volume row
    ST7735_SetCursor(0, 2);
    ST7735_SetTextColor(sel == 0 ? GD_CYAN : GD_WHITE);
    ST7735_OutStringTransparent(sel == 0 ? "> " : "  ");
    ST7735_OutStringTransparent((char*)Ph(PH_VOLUME));
    ST7735_OutUDec(VolLevel);
    // SFX row
    ST7735_SetCursor(0, 4);
    ST7735_SetTextColor(sel == 1 ? GD_CYAN : GD_WHITE);
    ST7735_OutStringTransparent(sel == 1 ? "> " : "  ");
    ST7735_OutStringTransparent((char*)Ph(PH_SFX));
    ST7735_OutStringTransparent((char*)(SFX_On ? Ph(PH_ON) : Ph(PH_OFF)));
    // Back row
    ST7735_SetCursor(0, 6);
    ST7735_SetTextColor(sel == 2 ? GD_CYAN : GD_WHITE);
    ST7735_OutStringTransparent(sel == 2 ? "> " : "  ");
    ST7735_OutStringTransparent((char*)Ph(PH_BACK));
}

static void DrawCharSelect(uint8_t sel){
    ClearScreen(MENU_BG);
    const char *title = "Characters";
    for(int k = 0; title[k]; k++)
        ST7735_DrawChar(k*6, 2, title[k], GD_WHITE, MENU_BG, 1);
    for(int i = 0; i < NUM_CHARS; i++){
        int16_t py = 14 + i * 16;
        uint16_t fg = (i == sel) ? GD_CYAN : GD_WHITE;
        ST7735_DrawChar(0, py, i == sel ? '>' : ' ', fg, MENU_BG, 1);
        uint16_t savedBody = CubeBody, savedFace = CubeFace;
        if(i == NUM_CHARS - 1){
            // Rainbow: draw cube body cycling through palette steps
            CubeFace = GD_WHITE;
            for(int s = 0; s < 6; s++){
                CubeBody = RainbowPal[s];
                DrawCube(12 + s * 2, py);  // overlapping slices give rainbow shimmer
            }
        } else {
            CubeBody = CharColors[i][0]; CubeFace = CharColors[i][1];
            DrawCube(12, py);
            DrawShip(86, py - 2);
        }
        CubeBody = savedBody; CubeFace = savedFace;
        int16_t tx = 24;
        for(const char *c = CharNames[i]; *c; c++, tx += 6)
            ST7735_DrawChar(tx, py, *c, fg, MENU_BG, 1);
        // Rainbow entry: show "[DEBUG]" tag
        if(i == NUM_CHARS - 1){
            const char *tag = " [DBG]";
            for(const char *c = tag; *c; c++, tx += 6)
                ST7735_DrawChar(tx, py, *c, GD_YELLOW, MENU_BG, 1);
        }
    }
    const char *hint = "Jump=OK Pause=Back";
    for(int k = 0; hint[k]; k++)
        ST7735_DrawChar(k*6, 110, hint[k], GD_WHITE, MENU_BG, 1);
}

static void DrawPauseScreen(uint8_t sel){
    ClearScreen(MENU_BG);
    ST7735_SetCursor(0, 0);
    ST7735_SetTextColor(GD_WHITE);
    ST7735_OutStringTransparent((char*)Ph(PH_PAUSED));
    const char *items[2] = {Ph(PH_RESUME), Ph(PH_MENU)};
    for(int i = 0; i < 2; i++){
        int row = 2 + i;
        ST7735_SetCursor(0, row);
        if(i == sel){
            ST7735_SetTextColor(GD_CYAN);
            ST7735_OutStringTransparent("> ");
        } else {
            ST7735_SetTextColor(GD_WHITE);
            ST7735_OutStringTransparent("  ");
        }
        ST7735_OutStringTransparent((char*)items[i]);
    }
    ST7735_SetCursor(0, 6);
    ST7735_SetTextColor(GD_WHITE); ST7735_OutStringTransparent("S:");
    ST7735_OutUDec(Score);
    ST7735_SetTextColor(GD_WHITE); ST7735_OutStringTransparent("  Hi:");
    ST7735_OutUDec(HighScore[MyLevel]);
}

static void DrawWinScreen(uint8_t sel){
    ClearScreen(MENU_BG);
    ST7735_SetCursor(0, 0);
    ST7735_SetTextColor(GD_GREEN);
    ST7735_OutStringTransparent((char*)Ph(PH_YOU_WIN));
    ST7735_SetCursor(0, 2);
    ST7735_SetTextColor(GD_WHITE); ST7735_OutStringTransparent("Score:"); ST7735_OutUDec(Score); ST7735_OutStringTransparent("s");
    ST7735_SetCursor(0, 3);
    ST7735_SetTextColor(GD_WHITE); ST7735_OutStringTransparent("Best:"); ST7735_OutUDec(HighScore[MyLevel]); ST7735_OutStringTransparent("s");
    const char *items[2] = {Ph(PH_RETRY), Ph(PH_MENU)};
    for(int i = 0; i < 2; i++){
        int row = 5 + i;
        ST7735_SetCursor(0, row);
        if(i == sel){
            ST7735_SetTextColor(GD_CYAN);
            ST7735_OutStringTransparent("> ");
        } else {
            ST7735_SetTextColor(GD_WHITE);
            ST7735_OutStringTransparent("  ");
        }
        ST7735_OutStringTransparent((char*)items[i]);
    }
}

static void RedrawGameScene(uint16_t bg){
    SceneBG = bg;
    ST7735_FillScreen(bg);
    DrawGround(bg);
    DrawHUD(bg);
    InitStars(bg);
    for(int i = 0; i < MAX_OBS; i++){
        if(!Obs[i].active) continue;
        switch(Obs[i].type){
            case OBS_SPIKE_UP:   DrawSpikeUp(Obs[i].x, Obs[i].y);   break;
            case OBS_SPIKE_DN:   DrawSpikeDn(Obs[i].x, Obs[i].y);   break;
            case OBS_BLOCK:      DrawBlock(Obs[i].x, Obs[i].y);      break;
            case OBS_BLOCK_TALL:  DrawBlockTall(Obs[i].x, Obs[i].y);   break;
            case OBS_BLOCK_QUAD:  DrawBlockQuad(Obs[i].x, Obs[i].y);   break;
            case OBS_BLOCK_5:     DrawBlock5(Obs[i].x, Obs[i].y);      break;
            case OBS_BLOCK_WIDE3: DrawBlockWide3(Obs[i].x, Obs[i].y);  break;
            case OBS_STEP:        DrawBlockStep(Obs[i].x, Obs[i].y);   break;
            case OBS_GATE:        DrawGateSpiked(Obs[i].x, Obs[i].y, Obs[i].gap, GATE_COL); break;
            case OBS_PORTAL_SHIP: DrawPortal(Obs[i].x, Obs[i].y, 1); break;
            case OBS_PORTAL_CUBE: DrawPortal(Obs[i].x, Obs[i].y, 0); break;
            default: break;
        }
    }
    if(GameMode == MODE_CUBE) DrawCubeRotated(PLAYER_X, PlayerY, CubeRot);
    else                      DrawShip(PLAYER_X, PlayerY);
    DrawGround(bg); // re-draw ground over anything that extended below it
}

// ═══════════════════════════════════════════════════════════════════════════
// GAME TIMER ISR  (30 Hz)
// ═══════════════════════════════════════════════════════════════════════════
void TIMG12_IRQHandler(void){
    if((TIMG12->CPU_INT.IIDX) == 1){
        GPIOB->DOUTTGL31_0 = GREEN; // debug toggle
        if(GameState == ST_PLAYING){
            ISR_Switches = Switch_In();
            ISR_SlidePos = ADCin();
        }
        GPIOB->DOUTTGL31_0 = GREEN;
        Semaphore = 1;
        GPIOB->DOUTTGL31_0 = GREEN;
    }
}

uint8_t TExaS_LaunchPadLogicPB27PB26(void){
    return (0x80|((GPIOB->DOUT31_0>>26)&0x03));
}

// ═══════════════════════════════════════════════════════════════════════════
// PART B – main1: language test
// ═══════════════════════════════════════════════════════════════════════════
int main1(void){
    __disable_irq();
    PLL_Init();
    LaunchPad_Init();
    ST7735_InitPrintf(INITR_BLACKTAB);
    ST7735_FillScreen(GD_BLACK);
    // English
    ST7735_SetTextColor(GD_WHITE);
    ST7735_SetCursor(0,0); ST7735_OutString("English:"); ST7735_OutChar(13);
    for(int p = 0; p < PH_COUNT; p++){
        ST7735_OutString((char*)Phrases[p][LANG_EN]);
        ST7735_OutChar(13);
    }
    Clock_Delay1ms(4000);
    ST7735_FillScreen(GD_BLACK);
    ST7735_SetCursor(0,0); ST7735_OutString("Spanish:"); ST7735_OutChar(13);
    for(int p = 0; p < PH_COUNT; p++){
        ST7735_OutString((char*)Phrases[p][LANG_ES]);
        ST7735_OutChar(13);
    }
    while(1){}
}

// ═══════════════════════════════════════════════════════════════════════════
// PART C – main2: sprite test
// ═══════════════════════════════════════════════════════════════════════════
int main2(void){
    __disable_irq();
    PLL_Init();
    LaunchPad_Init();
    ST7735_InitPrintf(INITR_BLACKTAB);
    ST7735_FillScreen(GD_DBLUE);
    // draw all sprites
    DrawCube(10,  20);
    DrawShip(30,  20);
    DrawSpikeUp(10, 60);
    DrawSpikeDn(30, 60);
    DrawBlock(10, 90);
    DrawBlockTall(30, 90);
    DrawPortal(50, 20, 1);
    DrawPortal(80, 20, 0);
    DrawGround(GD_DBLUE);
    ST7735_SetCursor(0, 0);
    ST7735_SetTextColor(GD_WHITE);
    ST7735_OutString("Sprite test");
    while(1){}
}

// ═══════════════════════════════════════════════════════════════════════════
// PART D – main3: switch and LED test
// ═══════════════════════════════════════════════════════════════════════════
int main3(void){
    __disable_irq();
    PLL_Init();
    LaunchPad_Init();
    Switch_Init();
    LED_Init();
    ADCinit();
    ST7735_InitPrintf(INITR_BLACKTAB);
    ST7735_FillScreen(GD_BLACK);
    ST7735_SetCursor(0, 0);
    ST7735_OutString("Switch/LED test\n");
    ST7735_OutString("Jump=bit0 VolUp=bit1\n");
    ST7735_OutString("VolDn=bit2 Pause=bit3\n");
    ST7735_OutString("Sw:   \n");
    ST7735_OutString("Pot:  \n");
    while(1){
        uint32_t sw  = Switch_In();
        uint32_t pot = ADCin();
        LED_Off(0x0F);
        if(sw & 0x01) LED_On(0x08);
        if(sw & 0x02) LED_On(0x01);
        if(sw & 0x04) LED_On(0x04);
        if(sw & 0x08) LED_On(0x02);
        ST7735_SetCursor(3, 3);
        ST7735_OutUDec(sw);  ST7735_OutString("    ");
        ST7735_SetCursor(5, 4);
        ST7735_OutUDec(pot); ST7735_OutString("    ");
        Clock_Delay1ms(100);
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// PART E – main4: sound test
// ═══════════════════════════════════════════════════════════════════════════
int main4(void){
    __disable_irq();
    PLL_Init();
    LaunchPad_Init();
    Switch_Init();
    LED_Init();
    Sound_Init();
    ST7735_InitPrintf(INITR_BLACKTAB);
    ST7735_FillScreen(GD_BLACK);
    ST7735_SetCursor(0,0);
    ST7735_OutString("Sound test\n");
    ST7735_OutString("Jump=jump sfx\n");
    ST7735_OutString("VolUp=death sfx\n");
    ST7735_OutString("VolDn=win sfx\n");
    ST7735_OutString("Pause=music\n");
    __enable_irq();
    uint32_t prev = 0;
    while(1){
        uint32_t sw = Switch_In();
        uint32_t edge = sw & ~prev;
        prev = sw;
        if(edge & 0x01) Sound_Jump();
        if(edge & 0x02) Sound_Death();
        if(edge & 0x04) Sound_PlayWin();
        if(edge & 0x08) Sound_PlayMusic("STEREO.BIN");
        Sound_Task();
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// PART F – main5: full game
// ═══════════════════════════════════════════════════════════════════════════
int main(void){
    __disable_irq();
    PLL_Init();
    LaunchPad_Init();
    ST7735_InitPrintf(INITR_BLACKTAB);
    ST7735_FillScreen(GD_BLACK);
    { extern int LCDresetFlag; LCDresetFlag = 1; } // lock SPI1 against diskio re-init
    ADCinit();
    Switch_Init();
    LED_Init();
    Sound_Init();
    Sound_SetVolume(VolLevel);
    TExaS_Init(0, 0, &TExaS_LaunchPadLogicPB27PB26);
    TimerG12_IntArm(80000000/30, 2); // 30 Hz game tick
    __enable_irq(); 

    LED_On(0x08); // green = cube mode default

    // ── title screen (boot screen) ────────────────────────────────────────
    {
        Sound_PlayMusic("MENU.BIN");
        MenuSel = 0;
        DrawTitleScreen(MenuSel);
        Clock_Delay1ms(200);          // debounce: discard any button state at boot 
        uint32_t prev = Switch_In();  // capture current state so first edge is real
        while(GameState == ST_TITLE){
            uint32_t sw = Switch_In();
            uint32_t edge = sw & ~prev; prev = sw;
            Sound_Task();
            if(edge & 0x01){
                Sound_StopMusic();
                if(MenuSel == 0)      GameState = ST_LVLSEL;
                else if(MenuSel == 1) GameState = ST_SETTINGS;
                else if(MenuSel == 2) GameState = ST_CHARS;
                else                  GameState = ST_LANG;
            }
            if(edge & 0x02){
                MenuSel = (MenuSel == 0) ? 3 : MenuSel - 1;
                DrawTitleScreen(MenuSel);
                if(SFX_On) Sound_MenuBeep();
            }
            if(edge & 0x04){
                MenuSel = (MenuSel == 3) ? 0 : MenuSel + 1;
                DrawTitleScreen(MenuSel);
                if(SFX_On) Sound_MenuBeep();
            }
            Clock_Delay1ms(30);
        }
    }

    // ── settings screen ───────────────────────────────────────────────────
    if(GameState == ST_SETTINGS){
        uint8_t settSel = 0; uint8_t needRedraw = 1;
        uint32_t settPrev = Switch_In();
        while(GameState == ST_SETTINGS){
            uint32_t settSw = Switch_In(); uint32_t settEdge = settSw & ~settPrev; settPrev = settSw;
            if(needRedraw){ DrawSettings(settSel); needRedraw = 0; }
            Sound_Task();
            Clock_Delay1ms(50);
            if(settSel == 0){
                if(settSw & 0x02){ if(VolLevel < 8){ VolLevel++; Sound_SetVolume(VolLevel); needRedraw = 1; } }
                if(settSw & 0x04){ if(VolLevel > 0){ VolLevel--; Sound_SetVolume(VolLevel); needRedraw = 1; } }
                if(settEdge & 0x01){ settSel = 1; needRedraw = 1; }
            } else if(settSel == 1){
                if(settEdge & 0x02){ settSel = 0; needRedraw = 1; }
                if(settEdge & 0x04){ settSel = 2; needRedraw = 1; }
                if(settEdge & 0x01){ SFX_On ^= 1; needRedraw = 1; }
            } else {
                if(settEdge & 0x02){ settSel = 1; needRedraw = 1; }
                if(settEdge & 0x04){ settSel = 0; needRedraw = 1; }
                if(settEdge & 0x01){ Sound_PlayMusic("MENU.BIN"); GameState = ST_TITLE; }
            }
            if(settEdge & 0x08){ Sound_PlayMusic("MENU.BIN"); GameState = ST_TITLE; }
        }
    }

    // ── character select (from boot path) ────────────────────────────────
    if(GameState == ST_CHARS){
        uint8_t charSel = 0;
        DrawCharSelect(charSel);
        uint32_t prevC = Switch_In();
        while(GameState == ST_CHARS){
            Sound_Task();
            uint32_t swC = Switch_In(); uint32_t eC = swC & ~prevC; prevC = swC;
            if(eC & 0x02){ charSel = (charSel == 0) ? NUM_CHARS-1 : charSel-1; DrawCharSelect(charSel); if(SFX_On) Sound_MenuBeep(); }
            if(eC & 0x04){ charSel = (charSel == NUM_CHARS-1) ? 0 : charSel+1; DrawCharSelect(charSel); if(SFX_On) Sound_MenuBeep(); }
            if(eC & 0x01){ CubeBody = CharColors[charSel][0]; CubeFace = CharColors[charSel][1]; Invincible = (charSel == NUM_CHARS-1); GameState = ST_TITLE; }
            if(eC & 0x08){ GameState = ST_TITLE; }
            Clock_Delay1ms(30);
        }
        Sound_PlayMusic("MENU.BIN");
        MenuSel = 0; DrawTitleScreen(MenuSel);
        // re-enter title loop inline
        uint32_t prevTb = Switch_In();
        while(GameState == ST_TITLE){
            uint32_t swTb = Switch_In(); uint32_t eTb = swTb & ~prevTb; prevTb = swTb;
            Sound_Task();
            if(eTb & 0x01){
                Sound_StopMusic();
                if(MenuSel == 0)      GameState = ST_LVLSEL;
                else if(MenuSel == 1) GameState = ST_SETTINGS;
                else if(MenuSel == 2) GameState = ST_CHARS;
                else                  GameState = ST_LANG;
            }
            if(eTb & 0x02){ MenuSel = (MenuSel == 0) ? 3 : MenuSel-1; DrawTitleScreen(MenuSel); if(SFX_On) Sound_MenuBeep(); }
            if(eTb & 0x04){ MenuSel = (MenuSel == 3) ? 0 : MenuSel+1; DrawTitleScreen(MenuSel); if(SFX_On) Sound_MenuBeep(); }
            Clock_Delay1ms(30);
        }
    }

    // ── level select ──────────────────────────────────────────────────────
    if(GameState == ST_LVLSEL){
        Sound_PlayMusic("MENU.BIN");
        MenuSel = 0;
        DrawLevelSelect(MenuSel);
        uint32_t prev = Switch_In();  // capture current state; avoids spurious edge
        while(GameState == ST_LVLSEL){
            uint32_t sw = Switch_In(); uint32_t edge = sw & ~prev; prev = sw;
            Sound_Task();
            if(edge & 0x01){ if(SFX_On) Sound_PlayButton(); Sound_StopMusic(); MyLevel = (Level_t)MenuSel; NeedLevelSetup = 1; GameState = ST_PLAYING; }
            if(edge & 0x02){ MenuSel = (MenuSel == 0) ? 2 : MenuSel - 1; DrawLevelSelect(MenuSel); if(SFX_On) Sound_MenuBeep(); }
            if(edge & 0x04){ MenuSel = (MenuSel == 2) ? 0 : MenuSel + 1; DrawLevelSelect(MenuSel); if(SFX_On) Sound_MenuBeep(); }
            if(edge & 0x08){ Sound_StopMusic(); GameState = ST_TITLE; }
            Clock_Delay1ms(30);
        }
    }

    // ── main game loop ────────────────────────────────────────────────────
    while(1){
        if(GameState == ST_PLAYING){
            uint16_t bg = LvlBG[MyLevel];
            if(NeedLevelSetup){
                NeedLevelSetup = 0;
                LevelSetup(MyLevel, bg);
                Sound_PlayMusic(MusicFiles[MyLevel]);
                LED_On(0x08); LED_Off(0x02);
                while(Switch_In() & 0x0F){}  // drain any button held from menu
                Clock_Delay1ms(30);
                PrevSw = Switch_In();         // re-prime edge detector
            }

            uint8_t result = 0;
            while(GameState == ST_PLAYING && result == 0){
                // wait for 30Hz semaphore
                while(!Semaphore){}
                Semaphore = 0;

                uint32_t sw  = ISR_Switches;
                uint32_t sl  = ISR_SlidePos;

                // pause button (non-static so stale state from menus never carries over)
                {
                    uint32_t vedge = sw & ~PrevSw;
                    if(vedge & 0x08){ GameState = ST_PAUSED; break; }
                }

                result = GameTick_Update(sw, sl, bg);
                Sound_Task(); // refill SD buffer if needed
            }

            if(result == 1){
                if(Score > HighScore[MyLevel]) HighScore[MyLevel] = Score;
                Sound_StopMusic();
                if(SFX_On) Sound_Death();
                LED_Off(0x0F); LED_On(0x04);
                Clock_Delay1ms(600); // let death SFX play
                NeedLevelSetup = 1;
                GameState = ST_PLAYING;
            } else if(result == 2){
                if(Score > HighScore[MyLevel]) HighScore[MyLevel] = Score;
                Sound_StopMusic();
                if(SFX_On) Sound_PlayWin();
                LED_Off(0x0F); LED_On(0x08);
                GameState = ST_WIN;
                uint8_t winSel = 0;
                DrawWinScreen(winSel);
                Clock_Delay1ms(500);
                uint32_t prev4 = Switch_In();
                while(GameState == ST_WIN){
                    Sound_Task();
                    uint32_t sw4 = Switch_In();
                    uint32_t e4  = sw4 & ~prev4; prev4 = sw4;
                    if(e4 & 0x02){ winSel = (winSel == 0) ? 1 : 0; DrawWinScreen(winSel); if(SFX_On) Sound_MenuBeep(); }
                    if(e4 & 0x04){ winSel = (winSel == 1) ? 0 : 1; DrawWinScreen(winSel); if(SFX_On) Sound_MenuBeep(); }
                    if(e4 & 0x01){
                        if(winSel == 0){ NeedLevelSetup = 1; GameState = ST_PLAYING; }
                        else             GameState = ST_LVLSEL;
                    }
                    Clock_Delay1ms(30);
                }
            }
        }

        if(GameState == ST_PAUSED){
            Sound_PauseMusic();
            uint8_t pauseSel = 0;
            DrawPauseScreen(pauseSel);
            // wait for all buttons to be released before entering pause menu
            while(Switch_In() & 0x0F){}
            Clock_Delay1ms(50);
            uint32_t prev5 = Switch_In();
            while(GameState == ST_PAUSED){
                uint32_t sw5 = Switch_In();
                uint32_t e5  = sw5 & ~prev5; prev5 = sw5;
                if(e5 & 0x02){ pauseSel = (pauseSel == 0) ? 1 : 0; DrawPauseScreen(pauseSel); if(SFX_On) Sound_MenuBeep(); }
                if(e5 & 0x04){ pauseSel = (pauseSel == 1) ? 0 : 1; DrawPauseScreen(pauseSel); if(SFX_On) Sound_MenuBeep(); }
                if(e5 & 0x01){
                    if(pauseSel == 0){
                        // resume: wait for button release, redraw scene, then resume music
                        while(Switch_In() & 0x01){}
                        Clock_Delay1ms(30);
                        PrevSw = Switch_In(); // re-prime game-loop edge detector
                        uint16_t bg = LvlBG[MyLevel];
                        RedrawGameScene(bg);
                        Sound_ResumeMusic();
                        GameState = ST_PLAYING;
                    } else {
                        if(SFX_On) Sound_Quit(); NeedLevelSetup = 1; GameState = ST_LVLSEL;
                    }
                }
                if(e5 & 0x08){
                    // pause button again = resume
                    while(Switch_In() & 0x08){}
                    Clock_Delay1ms(30);
                    PrevSw = Switch_In();
                    uint16_t bg = LvlBG[MyLevel];
                    RedrawGameScene(bg);
                    Sound_ResumeMusic();
                    GameState = ST_PLAYING;
                }
                Clock_Delay1ms(30);
            }
        }

        if(GameState == ST_LVLSEL){
            Sound_PlayMusic("MENU.BIN");
            MenuSel = (uint8_t)MyLevel;
            DrawLevelSelect(MenuSel);
            uint32_t prev6 = Switch_In();
            while(GameState == ST_LVLSEL){
                uint32_t sw6 = Switch_In(); uint32_t e6 = sw6 & ~prev6; prev6 = sw6;
                Sound_Task();
                if(e6 & 0x01){ if(SFX_On) Sound_PlayButton(); Sound_StopMusic(); MyLevel = (Level_t)MenuSel; NeedLevelSetup = 1; GameState = ST_PLAYING; }
                if(e6 & 0x02){ MenuSel = (MenuSel == 0) ? 2 : MenuSel - 1; DrawLevelSelect(MenuSel); if(SFX_On) Sound_MenuBeep(); }
                if(e6 & 0x04){ MenuSel = (MenuSel == 2) ? 0 : MenuSel + 1; DrawLevelSelect(MenuSel); if(SFX_On) Sound_MenuBeep(); }
                if(e6 & 0x08){ Sound_StopMusic(); GameState = ST_TITLE; }
                Clock_Delay1ms(30);
            }
        }

        if(GameState == ST_TITLE){
            Sound_PlayMusic("MENU.BIN");
            MenuSel = 0;
            DrawTitleScreen(MenuSel);
            uint32_t prevT = Switch_In();
            while(GameState == ST_TITLE){
                uint32_t swT = Switch_In(); uint32_t eT = swT & ~prevT; prevT = swT;
                Sound_Task();
                if(eT & 0x01){
                    Sound_StopMusic();
                    if(MenuSel == 0)      GameState = ST_LVLSEL;
                    else if(MenuSel == 1) GameState = ST_SETTINGS;
                    else if(MenuSel == 2) GameState = ST_CHARS;
                    else                  GameState = ST_LANG;
                }
                if(eT & 0x02){ MenuSel = (MenuSel == 0) ? 3 : MenuSel - 1; DrawTitleScreen(MenuSel); if(SFX_On) Sound_MenuBeep(); }
                if(eT & 0x04){ MenuSel = (MenuSel == 3) ? 0 : MenuSel + 1; DrawTitleScreen(MenuSel); if(SFX_On) Sound_MenuBeep(); }
                Clock_Delay1ms(30);
            }
        }

        if(GameState == ST_SETTINGS){
            uint8_t settSel = 0; uint8_t needRedraw = 1;
            uint32_t settPrev = Switch_In();
            while(GameState == ST_SETTINGS){
                uint32_t settSw = Switch_In(); uint32_t settEdge = settSw & ~settPrev; settPrev = settSw;
                if(needRedraw){ DrawSettings(settSel); needRedraw = 0; }
                Sound_Task();
                Clock_Delay1ms(50);
                if(settSel == 0){
                    if(settSw & 0x02){ if(VolLevel < 8){ VolLevel++; Sound_SetVolume(VolLevel); needRedraw = 1; } }
                    if(settSw & 0x04){ if(VolLevel > 0){ VolLevel--; Sound_SetVolume(VolLevel); needRedraw = 1; } }
                    if(settEdge & 0x01){ settSel = 1; needRedraw = 1; }
                } else if(settSel == 1){
                    if(settEdge & 0x02){ settSel = 0; needRedraw = 1; }
                    if(settEdge & 0x04){ settSel = 2; needRedraw = 1; }
                    if(settEdge & 0x01){ SFX_On ^= 1; needRedraw = 1; }
                } else {
                    if(settEdge & 0x02){ settSel = 1; needRedraw = 1; }
                    if(settEdge & 0x04){ settSel = 0; needRedraw = 1; }
                    if(settEdge & 0x01){ Sound_PlayMusic("MENU.BIN"); GameState = ST_TITLE; }
                }
                if(settEdge & 0x08){ Sound_PlayMusic("MENU.BIN"); GameState = ST_TITLE; }
            }
        }

        if(GameState == ST_LANG){
            const char *langItems[2] = {Ph(PH_ENGLISH), Ph(PH_SPANISH)};
            MenuSel = (uint8_t)MyLang;
            DrawMenu("Select Language", langItems, 2, MenuSel);
            uint32_t prevL = Switch_In();
            while(GameState == ST_LANG){
                uint32_t swL = Switch_In(); uint32_t eL = swL & ~prevL; prevL = swL;
                Sound_Task();
                if(eL & 0x01){ MyLang = (Lang_t)MenuSel; GameState = ST_TITLE; }
                if(eL & 0x02){ MenuSel = (MenuSel == 0) ? 1 : 0; DrawMenu("Select Language", langItems, 2, MenuSel); if(SFX_On) Sound_MenuBeep(); }
                if(eL & 0x04){ MenuSel = (MenuSel == 1) ? 0 : 1; DrawMenu("Select Language", langItems, 2, MenuSel); if(SFX_On) Sound_MenuBeep(); }
                Clock_Delay1ms(30);
            }
        }

        if(GameState == ST_CHARS){
            uint8_t charSel = 0;
            DrawCharSelect(charSel);
            uint32_t prevCh = Switch_In();
            while(GameState == ST_CHARS){
                uint32_t swCh = Switch_In(); uint32_t eCh = swCh & ~prevCh; prevCh = swCh;
                Sound_Task();
                if(eCh & 0x02){ charSel = (charSel == 0) ? NUM_CHARS-1 : charSel-1; DrawCharSelect(charSel); if(SFX_On) Sound_MenuBeep(); }
                if(eCh & 0x04){ charSel = (charSel == NUM_CHARS-1) ? 0 : charSel+1; DrawCharSelect(charSel); if(SFX_On) Sound_MenuBeep(); }
                if(eCh & 0x01){ CubeBody = CharColors[charSel][0]; CubeFace = CharColors[charSel][1]; Invincible = (charSel == NUM_CHARS-1); GameState = ST_TITLE; }
                if(eCh & 0x08){ GameState = ST_TITLE; }
                Clock_Delay1ms(30);
            }
        }
    }
}
