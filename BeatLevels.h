// BeatLevels.h - Level data for ECE319K Lab9 Geometry Dash clone
#ifndef BEATLEVELS_H
#define BEATLEVELS_H

// ═══════════════════════════════════════════════════════════════════════════
// EASY LEVEL EDITING — read this section to add your own obstacles
// ═══════════════════════════════════════════════════════════════════════════
//
// AT(px) converts a "ruler" pixel position to a spawn tick.
//   Each game tick = SCROLL_SPD=3 pixels of scroll.
//   Obstacle spawns at screen right (x=128) and reaches the player ~36 ticks later.
//   To match old tick T, write AT(T*3).  To add something new, pick any px > 108.
//
//   Example:
//     {AT(180),  140, OBS_SPIKE_UP}   // spike at ruler-position 180
//     {AT(540),  138, OBS_BLOCK}       // block  at ruler-position 540
//     {AT(1200), 128, OBS_BLOCK_TALL}  // tall block
//     {AT(1500), 108, OBS_BLOCK_QUAD}  // 4-stack (big jump!)
//     {AT(2100), GROUND_Y-52, OBS_PORTAL_SHIP} // ship portal
//     {AT(2310), 65,  OBS_GATE, 42}   // ship gate, gap-top y=65, gap=42px
//
// Obstacle y-coordinates (top-left of sprite):
#define AT(px)         ((px) / SCROLL_SPD)
#define Y_SPIKE        (GROUND_Y - SPIKE_H)       // 140 — ground spike
#define Y_BLOCK        (GROUND_Y - BLOCK_H)       // 138 — single block
#define Y_BLOCK_TALL   (GROUND_Y - BLOCK_H*2)    // 128 — double-stack block
#define Y_BLOCK_3      (GROUND_Y - BLOCK_H*3)    // 118 — three-stack block
#define Y_BLOCK_QUAD   (GROUND_Y - BLOCK_H*4)    // 108 — four-stack block
#define Y_BLOCK_5      (GROUND_Y - BLOCK_H*5)    //  98 — five-stack block
#define Y_BLOCK_6      (GROUND_Y - BLOCK_H*6)    //  88 — six-stack block (Dry Out)
#define Y_BLOCK_7      (GROUND_Y - BLOCK_H*7)    //  78 — seven-stack block (Jumper)
#define Y_PORTAL       (GROUND_Y - 52)            //  96 — portal ring (50px tall oval)
// Gate y = top of gap opening (lower y = closer to ceiling).
//   Valid range ~40..90.  Gap height set by 4th param (default 42px).
//   y=40 → gap near ceiling (hard)   y=85 → gap near floor (hard)   y=65 → center (easy)
//
// GAUNTLET obstacle: consecutive OBS_SPIKE_UP at Δt=3 (≈1px apart, essentially flush).
//   Jump physics: air time = 17 ticks → horizontal travel = 51px.
//   5-spike gauntlet = 44px (7px forgiveness — L3 hard version).
//   4-spike gauntlet = 35px (16px forgiveness — L2 easier version).

// ═══════════════════════════════════════════════════════════════════════════
// BEAT + SPACING REFERENCE
//   Level 1 (140 BPM, 30 Hz): 1 beat = 12.86 t,  4 beats = 51 t
//   Level 2 (116 BPM, 30 Hz): 1 beat = 15.52 t,  4 beats = 62 t
//   Level 3 (172 BPM, 30 Hz): 1 beat = 10.47 t,  4 beats = 42 t
//
//   Consecutive run spacing (pixel gap = ΔT×3 - 8):
//     doubles:  +14 t = 34 px (L1/L2), +11 t = 25 px (L3)
//     triples:  +20 t = 52 px (L1), +17 t = 43 px (L2), +14 t = 34 px (L3)
//     quads:    +23 t = 61 px (L1), +19 t = 49 px (L2/L3 spike-runs)
//   Group starts are beat-aligned; within-run spacing uses fixed pixel spacing above.
// ═══════════════════════════════════════════════════════════════════════════

// ═══════════════════════════════════════════════════════════════════════════
// LEVEL 1 — Stereo Madness (140 BPM, easy → medium)
// ═══════════════════════════════════════════════════════════════════════════
static const LevelEvent_t Lvl1_Events[] = {
    // ── pre-ship cube section ────────────────────────────────────────────
    {  67, 140, OBS_SPIKE_UP},   // beat  8 — single
    { 118, 138, OBS_BLOCK},      // beat 12
    { 170, 140, OBS_SPIKE_UP},   // beat 16 — single
    { 273, 138, OBS_BLOCK},      // beat 24
    { 324, 140, OBS_SPIKE_UP},   // beat 28 — double →
    { 338, 140, OBS_SPIKE_UP},   //           +14 t (34 px gap)
    { 401, 138, OBS_BLOCK},      // beat 34
    { 427, 140, OBS_SPIKE_UP},   // beat 36 — single
    { 478, 140, OBS_SPIKE_UP},   // beat 40 — triple →
    { 498, 140, OBS_SPIKE_UP},   //           +20 t (52 px gap)
    { 518, 140, OBS_SPIKE_UP},   //           +20 t
    { 555, 128, OBS_BLOCK_TALL}, // beat 46 — rest
    { 581, 140, OBS_SPIKE_UP},   // beat 48 — single
    { 607, 140, OBS_SPIKE_UP},   // beat 50 — double →
    { 621, 140, OBS_SPIKE_UP},   //           +14 t (34 px gap)
    { 658, 140, OBS_SPIKE_UP},   // beat 54 — triple →
    { 678, 140, OBS_SPIKE_UP},   //           +20 t (52 px gap)
    { 698, 140, OBS_SPIKE_UP},   //           +20 t — last before portal
    // ── ship section (unchanged) ───────────────────────────────────────────────
    { 721, GROUND_Y - 52, OBS_PORTAL_SHIP},
    { 751,  80, OBS_GATE},
    { 764,  55, OBS_GATE},
    { 777,  70, OBS_GATE},
    { 790,  40, OBS_GATE},
    { 803,  75, OBS_GATE},
    { 816,  50, OBS_GATE},
    { 829,  85, OBS_GATE},
    { 841,  60, OBS_GATE},
    { 854,  48, OBS_GATE},
    { 867,  72, OBS_GATE},
    { 880,  45, OBS_GATE},
    { 893,  65, OBS_GATE},
    { 906,  80, OBS_GATE},
    { 919,  55, OBS_GATE},
    { 931,  70, OBS_GATE},
    { 944,  40, OBS_GATE},
    { 957,  75, OBS_GATE},
    { 970,  50, OBS_GATE},
    { 983,  85, OBS_GATE},
    { 996,  60, OBS_GATE},
    {1009,  48, OBS_GATE},
    {1021,  72, OBS_GATE},
    {1034,  45, OBS_GATE},
    {1047,  65, OBS_GATE},
    {1060,  80, OBS_GATE},
    {1073,  55, OBS_GATE},
    {1086,  70, OBS_GATE},
    {1099,  40, OBS_GATE},
    {1111,  75, OBS_GATE},
    {1124,  50, OBS_GATE},
    {1137,  85, OBS_GATE},
    {1150,  60, OBS_GATE},
    {1180, GROUND_Y - 52, OBS_PORTAL_CUBE},
    // ── post-ship cube section (beat 96+) ────────────────────────────────
    {1198, 140, OBS_SPIKE_UP},   // beat  96 — single
    {1250, 138, OBS_BLOCK},      // beat 100
    {1301, 140, OBS_SPIKE_UP},   // beat 104 — double →
    {1315, 140, OBS_SPIKE_UP},   //            +14 t (34 px gap)
    {1353, 128, OBS_BLOCK_TALL}, // beat 108
    {1404, 140, OBS_SPIKE_UP},   // beat 112 — triple →
    {1424, 140, OBS_SPIKE_UP},   //            +20 t (52 px gap)
    {1444, 140, OBS_SPIKE_UP},   //            +20 t
    {1477, 138, OBS_BLOCK},      // beat 116 — rest
    {1507, 140, OBS_SPIKE_UP},   // beat 120 — triple →
    {1527, 140, OBS_SPIKE_UP},   //            +20 t (52 px gap)
    {1547, 140, OBS_SPIKE_UP},   //            +20 t
    {1584, 140, OBS_SPIKE_UP},   // beat 126 — quad! →
    {1607, 140, OBS_SPIKE_UP},   //            +23 t (61 px gap)
    {1630, 140, OBS_SPIKE_UP},   //            +23 t
    {1653, 140, OBS_SPIKE_UP},   //            +23 t
    {1661, 128, OBS_BLOCK_TALL}, // beat 132 — rest
    {1713, 140, OBS_SPIKE_UP},   // beat 136 — triple →
    {1733, 140, OBS_SPIKE_UP},   //            +20 t (52 px gap)
    {1753, 140, OBS_SPIKE_UP},   //            +20 t
    {1786, 138, OBS_BLOCK},      // beat 140 — rest
    {1815, 140, OBS_SPIKE_UP},   // beat 144 — double →
    {1829, 140, OBS_SPIKE_UP},   //            +14 t (34 px gap)
    {1867, 138, OBS_BLOCK},      // beat 148 — end
};
#define LVL1_COUNT (sizeof(Lvl1_Events)/sizeof(Lvl1_Events[0]))
#define LVL1_END   2000

// ═══════════════════════════════════════════════════════════════════════════
// LEVEL 2 — Dry Out (116 BPM, medium)
// ═══════════════════════════════════════════════════════════════════════════
static const LevelEvent_t Lvl2_Events[] = {
    // ── pre-ship cube section ────────────────────────────────────────────
    {  30, 108, OBS_BLOCK_QUAD}, // opening — 4-stack jump!
    {  57, 140, OBS_SPIKE_UP},   // beat  6 — single
    { 119, 138, OBS_BLOCK},      // beat 10
    { 150, 140, OBS_SPIKE_UP},   // beat 12 — double →
    { 164, 140, OBS_SPIKE_UP},   //           +14 t (34 px gap)
    { 243, 138, OBS_BLOCK},      // beat 18 — rest
    { 305, 140, OBS_SPIKE_UP},   // beat 22 — double →
    { 319, 140, OBS_SPIKE_UP},   //           +14 t (34 px gap)
    { 367, 138, OBS_BLOCK},      // beat 26
    { 398, 128, OBS_BLOCK_TALL}, // beat 28
    { 429, 140, OBS_SPIKE_UP},   // beat 30 — triple →
    { 448, 140, OBS_SPIKE_UP},   //           +19 t (49 px gap)
    { 467, 140, OBS_SPIKE_UP},   //           +19 t
    { 523, 138, OBS_BLOCK},      // beat 36 — rest
    { 554, 140, OBS_SPIKE_UP},   // beat 38 — triple →
    { 573, 140, OBS_SPIKE_UP},   //           +19 t (49 px gap)
    { 592, 140, OBS_SPIKE_UP},   //           +19 t
    { 647, 138, OBS_BLOCK},      // beat 44 — rest
    { 678, 140, OBS_SPIKE_UP},   // beat 46 — double →
    { 692, 140, OBS_SPIKE_UP},   //           +14 t (34 px gap), last before portal
    // ── ship section (unchanged) ─────────────────────────────────────────
    { 717, GROUND_Y - 52, OBS_PORTAL_SHIP},
    { 747,  45, OBS_GATE},
    { 762,  70, OBS_GATE},
    { 778,  90, OBS_GATE},
    { 794,  50, OBS_GATE},
    { 809,  75, OBS_GATE},
    { 825,  40, OBS_GATE},
    { 840,  85, OBS_GATE},
    { 856,  55, OBS_GATE},
    { 871,  70, OBS_GATE},
    { 887,  45, OBS_GATE},
    { 902,  80, OBS_GATE},
    { 918,  80, OBS_GATE},
    { 933,  45, OBS_GATE},
    { 949,  70, OBS_GATE},
    { 964,  90, OBS_GATE},
    { 980,  50, OBS_GATE},
    { 995,  75, OBS_GATE},
    {1011,  40, OBS_GATE},
    {1026,  85, OBS_GATE},
    {1042,  55, OBS_GATE},
    {1057,  70, OBS_GATE},
    {1073,  45, OBS_GATE},
    {1088,  80, OBS_GATE},
    {1104,  80, OBS_GATE},
    {1119,  45, OBS_GATE},
    {1135,  70, OBS_GATE},
    {1150,  90, OBS_GATE},
    {1180, GROUND_Y - 52, OBS_PORTAL_CUBE},
    // ── post-ship cube section (beat 80+) ────────────────────────────────
    {1205, 140, OBS_SPIKE_UP},   // beat  80 — single
    // ── 4-spike gauntlet (easier, ~35px wide = 16px forgiveness on jump) ─
    {1230, 140, OBS_SPIKE_UP},   // gauntlet spike 1 of 4 →
    {1233, 140, OBS_SPIKE_UP},   //   +3 t (≈flush)
    {1236, 140, OBS_SPIKE_UP},   //   +3 t
    {1239, 140, OBS_SPIKE_UP},   //   +3 t (last spike)
    // ─────────────────────────────────────────────────────────────────────
    {1267, 138, OBS_BLOCK},      // beat  84
    {1298, 140, OBS_SPIKE_UP},   // beat  86 — double →
    {1312, 140, OBS_SPIKE_UP},   //            +14 t (34 px gap)
    {1361,  88, OBS_BLOCK_6},   // beat  90 — 6-stack! (big jump)
    {1392, 140, OBS_SPIKE_UP},   // beat  92 — double →
    {1406, 140, OBS_SPIKE_UP},   //            +14 t (34 px gap)
    // ── 5-stack sandwich (1 spike each side, flush) ──────────────────────
    {1451, 140, OBS_SPIKE_UP},   // spike 1
    {1454,  98, OBS_BLOCK_5},    // 5-stack (flush)
    {1457, 140, OBS_SPIKE_UP},   // spike 2 (flush)
    {1485, 140, OBS_SPIKE_UP},   // beat  98 — triple →
    {1504, 140, OBS_SPIKE_UP},   //            +19 t (49 px gap)
    {1523, 140, OBS_SPIKE_UP},   //            +19 t
    {1578, 140, OBS_SPIKE_UP},   // beat 104 — quad! →
    {1597, 140, OBS_SPIKE_UP},   //            +19 t (49 px gap)
    {1616, 140, OBS_SPIKE_UP},   //            +19 t
    {1635, 140, OBS_SPIKE_UP},   //            +19 t
    {1671, 128, OBS_BLOCK_TALL}, // beat 110 — rest
    {1702, 140, OBS_SPIKE_UP},   // beat 112 — triple →
    {1721, 140, OBS_SPIKE_UP},   //            +19 t (49 px gap)
    {1740, 140, OBS_SPIKE_UP},   //            +19 t
    {1795, 138, OBS_BLOCK},      // beat 118 — rest
    {1826, 140, OBS_SPIKE_UP},   // beat 120 — double →
    {1840, 140, OBS_SPIKE_UP},   //            +14 t (34 px gap)
};
#define LVL2_COUNT (sizeof(Lvl2_Events)/sizeof(Lvl2_Events[0]))
#define LVL2_END   1900

// ═══════════════════════════════════════════════════════════════════════════
// LEVEL 3 — Jumper (172 BPM, hardest)
// ═══════════════════════════════════════════════════════════════════════════
// Ship section: 28 gates at Δt=16 (~1.5 beats), beat-synced to 172 BPM.
// Cascade gap: 45→40→35→30→25→20→16 px (4 gates each stage).
// Gate param field = gap size in px (passed to SpawnObstacle → Obs[i].gap).
static const LevelEvent_t Lvl3_Events[] = {
    // ── pre-ship cube section ────────────────────────────────────────────
    // Doubles unchanged (+11 t = 25 px). Triples tightened to +14 t (34 px).
    {  48, 140, OBS_SPIKE_UP},   // beat  8 — single
    {  90, 138, OBS_BLOCK},      // beat 12
    { 121, 140, OBS_SPIKE_UP},   // beat 15 — double →
    { 132, 140, OBS_SPIKE_UP},   //           +11 t (25 px, unchanged)
    { 163, 140, OBS_SPIKE_UP},   // beat 19 — single
    { 184, 140, OBS_SPIKE_UP},   // beat 21 — double →
    { 195, 140, OBS_SPIKE_UP},   //           +11 t
    { 215, 138, OBS_BLOCK},      // beat 24
    // ── 5-spike gauntlet (~44px wide = 7px forgiveness on jump) ──────────
    { 241, 140, OBS_SPIKE_UP},   // gauntlet spike 1 of 5 →
    { 244, 140, OBS_SPIKE_UP},   //   +3 t (≈flush)
    { 247, 140, OBS_SPIKE_UP},   //   +3 t
    { 250, 140, OBS_SPIKE_UP},   //   +3 t
    { 253, 140, OBS_SPIKE_UP},   //   +3 t (last spike)
    // ─────────────────────────────────────────────────────────────────────
    { 295, 140, OBS_SPIKE_UP},   // beat 30 — double → (shifted +17t for gauntlet clearance)
    { 306, 140, OBS_SPIKE_UP},   //           +11 t
    { 337, 138, OBS_BLOCK},      // beat 34 (shifted +17t)
    { 362, 140, OBS_SPIKE_UP},   // beat 38 — triple →
    { 376, 140, OBS_SPIKE_UP},   //           +14 t (34 px gap)
    { 390, 140, OBS_SPIKE_UP},   //           +14 t
    { 425,  78, OBS_BLOCK_7},   // beat 44 — 7-stack! (35 t rest after triple)
    { 466, 140, OBS_SPIKE_UP},   // beat 48 — triple → (41 t rest after quad)
    { 480, 140, OBS_SPIKE_UP},   //           +14 t (34 px gap)
    { 494, 140, OBS_SPIKE_UP},   //           +14 t
    { 520, 138, OBS_BLOCK},      // beat 50 — rest
    { 540, 140, OBS_SPIKE_UP},   // beat 52 — single
    { 555, 140, OBS_SPIKE_UP},   // beat 54 — triple →
    { 569, 140, OBS_SPIKE_UP},   //           +14 t (34 px gap)
    { 587, 140, OBS_SPIKE_UP},   //    changed here       +14 t
    { 605, 138, OBS_BLOCK},      // beat 58 — rest
    { 655, 140, OBS_SPIKE_UP},   // beat 66 — double →
    { 666, 140, OBS_SPIKE_UP},   //           +11 t
    { 687, 140, OBS_SPIKE_UP},   // beat 68 — last before portal
    // ── ship section: 28 beat-synced gates (Δt=16, ~1.5 beats @ 172 BPM) ─
    // Cascade gap: 45→40→35→30→25→20→16 px, 4 gates per stage.
    // y = gap-top position; param (4th field) = gap size in px.
    { 717, GROUND_Y - 52, OBS_PORTAL_SHIP},
    // Stage 1: gap=45 (wide open — orientate)
    { 733,  42, OBS_GATE, 45},
    { 749,  78, OBS_GATE, 45},
    { 765,  40, OBS_GATE, 45},
    { 781,  80, OBS_GATE, 45},
    // Stage 2: gap=40
    { 797,  44, OBS_GATE, 40},
    { 813,  80, OBS_GATE, 40},
    { 829,  42, OBS_GATE, 40},
    { 845,  80, OBS_GATE, 40},
    // Stage 3: gap=35
    { 861,  48, OBS_GATE, 35},
    { 877,  78, OBS_GATE, 35},
    { 893,  50, OBS_GATE, 35},
    { 909,  76, OBS_GATE, 35},
    // Stage 4: gap=30
    { 925,  54, OBS_GATE, 30},
    { 941,  74, OBS_GATE, 30},
    { 957,  54, OBS_GATE, 30},
    { 973,  72, OBS_GATE, 30},
    // Stage 5: gap=25
    { 989,  57, OBS_GATE, 25},
    {1005,  71, OBS_GATE, 25},
    {1021,  57, OBS_GATE, 25},
    {1037,  69, OBS_GATE, 25},
    // Stage 6: gap=20
    {1053,  60, OBS_GATE, 20},
    {1069,  68, OBS_GATE, 20},
    {1085,  60, OBS_GATE, 20},
    {1101,  66, OBS_GATE, 20},
    // Stage 7: gap=16 (hardest — squeeze!)
    {1117,  62, OBS_GATE, 16},
    {1133,  65, OBS_GATE, 16},
    {1149,  62, OBS_GATE, 16},
    {1165,  64, OBS_GATE, 16},
    {1175, GROUND_Y - 52, OBS_PORTAL_CUBE},
    // ── post-ship cube section (beat 120+) ───────────────────────────────
    {1220, 140, OBS_SPIKE_UP},   // beat 120 — triple →
    {1234, 140, OBS_SPIKE_UP},   //            +14 t (34 px gap)
    {1248, 140, OBS_SPIKE_UP},   //            +14 t
    {1304, 138, OBS_BLOCK},      // beat 128 — rest
    {1325, 140, OBS_SPIKE_UP},   // beat 130 — double →
    {1336, 140, OBS_SPIKE_UP},   //            +11 t (unchanged)
    {1367,  78, OBS_BLOCK_7},   // beat 134 — 7-stack! (31 t after double)
    {1388, 140, OBS_SPIKE_UP},   // beat 136 — double → (21 t after quad)
    {1399, 140, OBS_SPIKE_UP},   //            +11 t
    {1430, 140, OBS_SPIKE_UP},   // beat 140 — triple →
    {1444, 140, OBS_SPIKE_UP},   //            +14 t (34 px gap)
    {1458, 140, OBS_SPIKE_UP},   //            +14 t
    {1493, 138, OBS_BLOCK},      // beat 146 — rest
    {1514, 140, OBS_SPIKE_UP},   // beat 148 — triple →
    {1528, 140, OBS_SPIKE_UP},   //            +14 t (34 px gap)
    {1542, 140, OBS_SPIKE_UP},   //            +14 t
    {1577, 140, OBS_SPIKE_UP},   // beat 154 — quad! →
    {1663, 128, OBS_BLOCK_TALL}, // beat 160 — rest
    {1684, 140, OBS_SPIKE_UP},   // beat 162 — triple →
    {1698, 140, OBS_SPIKE_UP},   //            +14 t (34 px gap)
    {1712, 140, OBS_SPIKE_UP},   //            +14 t
    {1747, 138, OBS_BLOCK},      // beat 168 — rest
    {1768, 140, OBS_SPIKE_UP},   // beat 170 — triple →
    {1782, 140, OBS_SPIKE_UP},   //            +14 t (34 px gap)
    {1796, 140, OBS_SPIKE_UP},   //            +14 t
};
#define LVL3_COUNT (sizeof(Lvl3_Events)/sizeof(Lvl3_Events[0]))
#define LVL3_END   1900

#endif // BEATLEVELS_H