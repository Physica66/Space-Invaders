#include "raylib/raylib-6.0_macos/include/raylib.h"
#include "raylib/raylib-6.0_macos/include/raymath.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

bool SpecialReady = false;
bool starttimer = false;
float ShoabSpecialTime = 0.0f;

#define FPS 60
#define WindowWidth 1500
#define WindowHeight 900

#define AlienSize 50
#define AlienDistance 50
#define AlienSpeedX 20
#define AlienSpeedY 0
#define AlienSprite 20

#define HeroWidth 100
#define HeroHeight 135
#define HeroSpeedX 500

// Bullet dimensions and speeds
#define BulletSpeedY 1500
#define BulletWidth 6
#define BulletHeight 20

#define AlienBulletSpeedY 350
#define AlienBulletWidth 6
#define AlienBulletHeight 18

// Boss dimensions and bullet capacities
#define BossWidth 300
#define BossHeight 240
#define MAX_BOSS_BULLETS 24
#define MAX_BOSS_ORBS 12
#define BossPodMaxHp 75 // High-durability shield life pool for each weapon pod(to make the game harder :)

// Medium Alien Minion dimensions & capacities
#define MAX_MINIONS 8
#define MinionSize 68
#define MAX_MINION_BULLETS 16

#define COMMANDER_SIZE 72
#define MAX_COMMANDER_BULLETS 8

#ifndef PI
#define PI 3.14159265358979323846f
#endif

// Game Stats
#define STATE_LOADING 0
#define STATE_MENU 1
#define STATE_GAMEPLAY 2
#define STATE_OPTIONS 3
#define STATE_CREDITS 4
#define STATE_STORY 5
#define STATE_LAUNCH 6
#define STATE_CINEMATIC 7 

// Visual Starfield while loading,it just gives some vibes :)
#define STAR_COUNT 90

void DownAlien(int AlienInX, int AlienInY, Vector2 AlienPos[AlienInX][AlienInY])
{
    for (int X = 0; X < AlienInX; X++)
    {
        for (int Y = 0; Y < AlienInY; Y++)
        {
            AlienPos[X][Y].y += AlienSize / 0.5f;
        }
    }
}

// Dynamic afterburner jet flame renderer for the Stealth Bomber;this part is only for show-off thurst for stealth bomber :(
void DrawStealthFlames(Vector2 heroCenterPos, float planeWidth, float planeHeight)
{
    float time = (float)GetTime();

    // Twin nozzle anchor coordinates relative to plane center
    Vector2 leftNozzle  = { heroCenterPos.x - planeWidth * 0.125f, heroCenterPos.y + planeHeight * 0.42f };
    Vector2 rightNozzle = { heroCenterPos.x + planeWidth * 0.125f, heroCenterPos.y + planeHeight * 0.42f };

    Vector2 nozzles[2] = { leftNozzle, rightNozzle };

    for (int n = 0; n < 2; n++)
    {
        float pulse = sinf(time * 35.0f + (n * 2.0f)) * 5.0f;
        float jitterX = (float)GetRandomValue(-2, 2);
        float baseLen = 32.0f + pulse + (float)GetRandomValue(0, 6);

        float nozzleHalfW = 6.5f;

        // 1. Outermost Violet Plasma Corona
        Vector2 v1_out = { nozzles[n].x - nozzleHalfW - 2.5f, nozzles[n].y };
        Vector2 v2_out = { nozzles[n].x + nozzleHalfW + 2.5f, nozzles[n].y };
        Vector2 v3_out = { nozzles[n].x + jitterX, nozzles[n].y + baseLen + 8.0f };
        DrawTriangle(v1_out, v3_out, v2_out, Fade((Color){ 140, 80, 255, 255 }, 0.40f));

        // 2. Main Electric Blue Flame
        Vector2 v1_mid = { nozzles[n].x - nozzleHalfW, nozzles[n].y };
        Vector2 v2_mid = { nozzles[n].x + nozzleHalfW, nozzles[n].y };
        Vector2 v3_mid = { nozzles[n].x + jitterX * 0.5f, nozzles[n].y + baseLen };
        DrawTriangle(v1_mid, v3_mid, v2_mid, (Color){ 90, 170, 255, 230 });

        // 3. Ultra-hot White-Cyan Center Spear
        Vector2 v1_in = { nozzles[n].x - nozzleHalfW * 0.45f, nozzles[n].y };
        Vector2 v2_in = { nozzles[n].x + nozzleHalfW * 0.45f, nozzles[n].y };
        Vector2 v3_in = { nozzles[n].x, nozzles[n].y + (baseLen * 0.55f) };
        DrawTriangle(v1_in, v3_in, v2_in, WHITE);

        // Ambient thrust glow ring
        DrawCircle((int)nozzles[n].x, (int)nozzles[n].y + 4, 8.0f, Fade(SKYBLUE, 0.6f));
        DrawCircle((int)nozzles[n].x, (int)nozzles[n].y + 2, 4.0f, WHITE);
    }
}

int main(void)
{
    InitWindow(WindowWidth, WindowHeight, "Space Invaders");
    InitAudioDevice();
    SetTargetFPS(FPS);

    // Center game window on active macOS display
    int curMon = GetCurrentMonitor();
    int monW = GetMonitorWidth(curMon);
    int monH = GetMonitorHeight(curMon);
    SetWindowPosition((monW - WindowWidth) / 2, (monH - WindowHeight) / 2);

    // Working directory safety fallback for macOS build configurations
    if (!DirectoryExists("assets") && DirectoryExists("../assets"))
    {
        ChangeDirectory("..");
    }

    // Dynamic Starfield Background
    Vector2 StarPos[STAR_COUNT];
    float StarSpeed[STAR_COUNT];
    for (int i = 0; i < STAR_COUNT; i++)
    {
        StarPos[i] = (Vector2){ (float)GetRandomValue(0, WindowWidth), (float)GetRandomValue(0, WindowHeight) };
        StarSpeed[i] = (float)GetRandomValue(40, 180);
    }

    // Alien speed and position
    int AlienInX = (WindowWidth / (AlienSize + AlienDistance)) - 2;
    int AlienInY = (WindowHeight / (2 * (AlienSize + AlienDistance)));
    Vector2 AlienPos[AlienInX][AlienInY];
    bool AlienAlive[AlienInX][AlienInY];

    AlienPos[0][0] = (Vector2){ 150, 50 };
    Vector2 AlienSpeed = { AlienSpeedX, AlienSpeedY };
    int AlienLooks[AlienInX][AlienInY];
    int SpeedBuff[AlienInY];
    for (int X = 0; X < AlienInX; X++)
    {
        for (int Y = 0; Y < AlienInY; Y++)
        {
            AlienPos[X][Y].x = AlienPos[0][0].x + (AlienSize + AlienDistance) * X;
            AlienPos[X][Y].y = AlienPos[0][0].y + (AlienSize + AlienDistance) * Y;
            AlienAlive[X][Y] = true;
            AlienLooks[X][Y] = GetRandomValue(1, AlienSprite);
            SpeedBuff[Y] = GetRandomValue(0, 20);
        }
    }

    Texture2D AlienTexture[AlienSprite];
    int AlienSWidth[AlienSprite] = {24, 34, 26, 28, 34, 40, 46, 32, 34, 40, 36, 26, 36, 52, 38, 28, 34, 32, 42, 44};
    int AlienSHeight[AlienSprite] = {27, 36, 27, 23, 38, 22, 36, 30, 31, 29, 28, 27, 41, 32, 26, 28, 25, 30, 28, 44};
    int AlienSpriteStyle[AlienSprite] = {6,6,5,7,5,5,5,6,5,4,7,4,8,4,6,4,6,4,4,5};
    for (int i = 0; i < AlienSprite; i++)
    {
        AlienTexture[i] = LoadTexture(TextFormat("assets/sprites/tinyShip%d.png", i + 1));
    }
    Sound shoot = LoadSound("assets/audio/alienshoot2.wav");
    Sound AlienShoot = LoadSound("assets/audio/alienshoot1.wav");

    Sound menuMove = LoadSound("assets/audio/sfx_menu_move4.wav");
    Sound menuSelect = LoadSound("assets/audio/sfx_menu_select2.wav");
    Sound heroDeath = LoadSound("assets/audio/sfx_deathscream_robot4.wav");
    Sound pauseIn = LoadSound("assets/audio/sfx_sounds_pause4_in.wav");
    Sound pauseOut = LoadSound("assets/audio/sfx_sounds_pause4_out.wav");
    Sound damage = LoadSound("assets/audio/sfx_sounds_damage3.wav");

    Sound cheer = LoadSound("assets/audio/cheering.wav");
    Sound gameWin = LoadSound("assets/audio/game_win.wav");
    Sound gameOverChild = LoadSound("assets/audio/universfield-game-over-kid-voice-clip-352738.mp3");
    Sound gameOverCommunity = LoadSound("assets/audio/freesound_community-game-over-38511.mp3");

    // Hero hit ouch sound
    Sound heroOuch = LoadSound("assets/audio/hero_hit.wav");

    // Scary Boss Laser Sound
    Sound bossLaserSound = LoadSound("assets/audio/laser_beam.wav");

    // Background Music Streams
    Music bgmStory = LoadMusicStream("assets/audio/2-air-strike-2-ost-track-2-ih-23-xz.wav");
    Music bgmMenu  = LoadMusicStream("assets/audio/air-strike-3-d-ii-gulf-thunder-main-ost-best-quality-mf-0-j-34.wav");
    Music bgmBoss  = LoadMusicStream("assets/audio/air-strike-3-d-ost-fear-drigto.wav");
    Music bgmWin   = LoadMusicStream("assets/audio/game_win_bgm.wav");
    Music bgmLost  = LoadMusicStream("assets/audio/game_lost_bgm.wav");

    // Boss Textures
    Texture2D BossTexture[5];
    BossTexture[0] = LoadTexture("assets/sprites/boss_idle.png");
    BossTexture[1] = LoadTexture("assets/sprites/boss_down.png");
    BossTexture[2] = LoadTexture("assets/sprites/boss_up.png");
    BossTexture[3] = LoadTexture("assets/sprites/boss_pulse1.png");
    BossTexture[4] = LoadTexture("assets/sprites/boss_pulse2.png");

    Texture2D HeroTexture = LoadTexture("assets/sprites/Hero.png");
    Texture2D StealthHeroTexture = LoadTexture("assets/sprites/Hero_stealth.png");

    // Loading Screen Background Artwork (PNG prioritized, fallbacks included) [the crash of image loading has been fixed :)]
    Texture2D loadingBg = LoadTexture("assets/sprites/loading_screen.png");
    if (loadingBg.id == 0) loadingBg = LoadTexture("assets/sprites/loading_screen.jpg");
    if (loadingBg.id == 0) loadingBg = LoadTexture("assets/sprites/loading_screen.jpeg");

    // Multi-format Safe Loader for Pilot Portraits
    Texture2D portraitShoab = LoadTexture("assets/sprites/shoab_hero.png");
    if (portraitShoab.id == 0) portraitShoab = LoadTexture("assets/sprites/Shoab_Hero.png");
    if (portraitShoab.id == 0) portraitShoab = LoadTexture("assets/sprites/Shoab_hero.png");
    if (portraitShoab.id == 0) portraitShoab = LoadTexture("assets/sprites/Shoab_Hero.jpeg");
    if (portraitShoab.id == 0) portraitShoab = LoadTexture("assets/sprites/Shoab_Hero.jpg");

    Texture2D portraitNayemul = LoadTexture("assets/sprites/nayemul_hero.png");
    if (portraitNayemul.id == 0) portraitNayemul = LoadTexture("assets/sprites/Nayemul_hero.png");
    if (portraitNayemul.id == 0) portraitNayemul = LoadTexture("assets/sprites/Nayemul_Hero.png");
    if (portraitNayemul.id == 0) portraitNayemul = LoadTexture("assets/sprites/Nayemul_hero.jpeg");
    if (portraitNayemul.id == 0) portraitNayemul = LoadTexture("assets/sprites/Nayemul_hero.jpg");

    // Commander Intro & Slowdown Control
    bool commandersTriggered  = false;
    bool commanderIntroActive = false;
    float commanderIntroTimer = 0.0f;
    float gameTimeDilation    = 1.0f;

    // Commander 1 (Null-Vector Jammer)
    Vector2 jammerPos            = { 0, 0 };
    Vector2 jammerSpeed          = { 0, 0 };
    int jammerHp                 = 0;
    int jammerMaxHp              = 35;
    bool jammerActive            = false;
    float jammerShootTimer       = 0.0f;
    float jammerActionCooldown   = 0.0f;
    float jammerPowerTimer       = 0.0f;
    int jammerAnimState          = 0;

    // Commander 2 (Chrono-Phase Warp)
    Vector2 warpPos              = { 0, 0 };
    Vector2 warpSpeed            = { 0, 0 };
    int warpHp                   = 0;
    int warpMaxHp                = 30;
    bool warpActive              = false;
    float warpShootTimer         = 0.0f;
    float warpActionCooldown     = 0.0f;
    float warpPowerTimer         = 0.0f;
    int warpAnimState            = 0;

    // Commander Special Power States
    float radarJammedTimer       = 0.0f; // Jammer power: hides player HUD score/status

    // Commander Bullets
    Vector2 jammerBulletPos[MAX_COMMANDER_BULLETS];
    bool jammerBulletActive[MAX_COMMANDER_BULLETS] = { false };
    Vector2 warpBulletPos[MAX_COMMANDER_BULLETS];
    bool warpBulletActive[MAX_COMMANDER_BULLETS] = { false };

    // Load Commander Textures
    Texture2D jammerTex[4];
    jammerTex[0] = LoadTexture("assets/sprites/commander_jammer_idle.png");
    jammerTex[1] = LoadTexture("assets/sprites/commander_jammer_move_y.png");
    jammerTex[2] = LoadTexture("assets/sprites/commander_jammer_move_x.png");
    jammerTex[3] = LoadTexture("assets/sprites/commander_jammer_power.png");

    Texture2D warpTex[4];
    warpTex[0] = LoadTexture("assets/sprites/commander_warp_idle.png");
    warpTex[1] = LoadTexture("assets/sprites/commander_warp_move_y.png");
    warpTex[2] = LoadTexture("assets/sprites/commander_warp_move_x.png");
    warpTex[3] = LoadTexture("assets/sprites/commander_warp_power.png");

    // Load Commander Audio
    Sound sndJammerHum   = LoadSound("assets/audio/sfx_jammer_hum.wav");
    Sound sndJammerShot  = LoadSound("assets/audio/sfx_jammer_shot.wav");
    Sound sndEmpBlast    = LoadSound("assets/audio/sfx_emp_blast.wav");
    Sound sndJammerDeath = LoadSound("assets/audio/sfx_jammer_death.wav");

    Sound sndWarpGlide   = LoadSound("assets/audio/sfx_warp_glide.wav");
    Sound sndWarpShot    = LoadSound("assets/audio/sfx_warp_shot.wav");
    Sound sndTeleport    = LoadSound("assets/audio/sfx_teleport.wav");
    Sound sndWarpDeath   = LoadSound("assets/audio/sfx_warp_death.wav");

    // Newly Integrated Gameplay & Immersion Sound Effects
    Sound sndSpecialBeam  = LoadSound("assets/audio/sfx_special_beam.wav");
    Sound sndChargeReady  = LoadSound("assets/audio/sfx_charge_ready.wav");
    Sound sndPodDestroy   = LoadSound("assets/audio/sfx_pod_destroy.wav");
    Sound sndOrbLaunch    = LoadSound("assets/audio/sfx_orb_launch.wav");
    Sound sndWarningSiren = LoadSound("assets/audio/sfx_warning_siren.wav");
    Sound sndDefibHum     = LoadSound("assets/audio/sfx_defib_hum.wav");
    Sound sndBossEnrage   = LoadSound("assets/audio/sfx_boss_enrage.wav");
    Sound sndRocketBoost  = LoadSound("assets/audio/sfx_rocket_boost.wav");

    bool bossEnraged = false; // State flag to ensure boss enrage roar triggers once

    // Screen Shake Camera
    Camera2D screenCamera = { 0 };
    screenCamera.zoom = 1.0f;

    // HERO 1 (Shoab - Left Side, A/D movement, W / Space shoot) - 4 Lives
    int Hero1Lives = 4;
    int Hero1Score = 0;
    Vector2 Hero1Pos = { WindowWidth * 0.35f, WindowHeight - HeroHeight };
    Vector2 Hero1Speed = { 0, 0 };
    Vector2 Hero1BulletPos[2] = { {0, 0}, {0, 0} };
    Vector2 Hero1SpecialBulletPos = { 0, 0 };
    bool Hero1BulletActive[2] = { false, false };
    bool Hero1SpecialBulletActive = false;
    bool hero1Debuffed = false;
    float hero1DebuffTimer = 0.0f;
    Vector2 Hero1CrashPos = { 0, 0 };
    float hero1ReviveTimer = 0.0f;
    float hero1HitFlashTimer = 0.0f;

    // HERO 2 (Nayemul - Right Side, Arrow keys movement, UP arrow shoot) - 4 Lives
    int Hero2Lives = 4;
    int Hero2Score = 0;
    Vector2 Hero2Pos = { WindowWidth * 0.65f, WindowHeight - HeroHeight };
    Vector2 Hero2Speed = { 0, 0 };
    Vector2 Hero2BulletPos[2] = { {0, 0}, {0, 0} };
    bool Hero2BulletActive[2] = { false, false };
    bool hero2Debuffed = false;
    float hero2DebuffTimer = 0.0f;
    Vector2 Hero2CrashPos = { 0, 0 };
    float hero2ReviveTimer = 0.0f;
    float hero2HitFlashTimer = 0.0f;

    // Tactical Feature 2: Orbital EMP Drop & Rapid Plasma Overcharge
    bool empDropped = false;
    bool empActive = false;
    Vector2 empPos = { 0, 0 };
    float empBuffTimer = 0.0f;
    float empShockwaveRadius = 0.0f;
    Vector2 empShockwaveCenter = { 0, 0 };

    // Opening Vision: Project Earth Shield Cinematic State
    float cinematicTimer = 0.0f;

    // Post-Game Tracking
    int Hero1Kills = 0, Hero2Kills = 0;
    int Hero1BossDamage = 0, Hero2BossDamage = 0;
    int Hero1HitsTaken = 0, Hero2HitsTaken = 0;

    // Alien bullets and shooting timer
    Vector2 AlienBulletPos = { 0, 0 };
    bool AlienBulletActive = false;
    float AlienShootTimer = 0.0f;

    // Game state
    int AliensKilled = 0;
    bool GameOver = false;

    // Additional state handling for sound and pause
    bool isPaused = false;
    float deathDelayTimer = 0.0f;
    bool deathSequenceActive = false;

    // Sound sequence tracking flags
    bool cheerPlayed = false;
    bool winSoundPlayed = false;
    bool winBgmStarted = false;
    bool lostBgmStarted = false;
    int winDialogueIndex = 0;
    int lostDialogueIndex = 0;

    // Story Dialogues state (Post-Loading)
    int storyDialogueIndex = 0;

    // Pre-Gameplay Launch Sequence State
    int launchDialogueIndex = 0;
    float launchCountdownTimer = 3.0f;
    bool launchCountdownActive = false;
    Vector2 launchShip1Pos = { WindowWidth * 0.40f, WindowHeight - 240 };
    Vector2 launchShip2Pos = { WindowWidth * 0.60f, WindowHeight - 240 };

    // Boss State Management and Destructible Shields
    bool bossSpawned = false;
    bool bossWarningActive = false;
    float bossWarningTimer = 0.0f;
    bool bossActive = false;
    bool bossDefeated = false;
    int bossHp = 100;
    int bossLeftPodHp = BossPodMaxHp;   // Left Shield Pod
    int bossRightPodHp = BossPodMaxHp;  // Right Shield Pod
    Vector2 bossPos = { (WindowWidth - BossWidth) / 2.0f, 60.0f };
    Vector2 bossSpeed = { 160.0f, 75.0f };
    float bossAnimTimer = 0.0f;
    float bossDirChangeTimer = 0.0f;

    // Medium Alien Minions deployed by Boss Shields every 7 seconds
    Vector2 minionPos[MAX_MINIONS];
    Vector2 minionSpeed[MAX_MINIONS];
    bool minionActive[MAX_MINIONS] = { false };
    int minionHp[MAX_MINIONS] = { 0 };
    float minionShootTimer[MAX_MINIONS] = { 0 };
    float minionDeployTimer = 0.0f;

    Vector2 minionBulletPos[MAX_MINION_BULLETS];
    bool minionBulletActive[MAX_MINION_BULLETS] = { false };

    // Boss Attack 1: Triple standard bullets
    float bossShootTimer = 0.0f;
    Vector2 bossBulletPos[MAX_BOSS_BULLETS];
    bool bossBulletActive[MAX_BOSS_BULLETS] = { false };

    // Boss Attack 2: Repeating Giant Laser Beam every 7s (persists 2s, 0.8s warning beam)
    float bossLaserTimer = 0.0f;
    bool bossLaserActive = false;
    float bossLaserDuration = 0.0f;

    // Boss Attack 3: 6 Circular Projectiles every 4s (causes jamming and slow)
    float bossOrbTimer = 0.0f;
    Vector2 bossOrbPos[MAX_BOSS_ORBS];
    Vector2 bossOrbVel[MAX_BOSS_ORBS];
    bool bossOrbActive[MAX_BOSS_ORBS] = { false };

    // State loader
    int CurrentState = STATE_LOADING;
    float LoadingTimer = 0.0f;
    int MenuSelection = 0;

    // Options Menu Config
    int OptionsTab = 0;
    int SoundSelection = 0;
    int VideoSelection = 0;

    int BgmTrackIndex = 0;
    float BgmVolume = 0.8f;
    float SfxVolume = 0.8f;

    float Brightness = 1.0f;
    bool ScanlinesOn = true;
    bool StarfieldOn = true;

    // Main Game Loop
    while (!WindowShouldClose())
    {
        float rawTime = GetFrameTime();
        gameTimeDilation = commanderIntroActive ? 0.12f : 1.0f;
        float Time = rawTime * gameTimeDilation;

        // Global Fullscreen Shortcut
        if (IsKeyPressed(KEY_F11))
        {
            ToggleFullscreen();
        }

        // Background starfield animation
        if (StarfieldOn)
        {
            for (int i = 0; i < STAR_COUNT; i++)
            {
                StarPos[i].y += StarSpeed[i] * Time;
                if (StarPos[i].y > WindowHeight)
                {
                    StarPos[i].y = 0;
                    StarPos[i].x = (float)GetRandomValue(0, WindowWidth);
                }
            }
        }

        // Adjust dynamic sound effect and music volumes
        SetSoundVolume(shoot, SfxVolume);
        SetSoundVolume(AlienShoot, SfxVolume);
        SetSoundVolume(menuMove, SfxVolume);
        SetSoundVolume(menuSelect, SfxVolume);
        SetSoundVolume(heroDeath, SfxVolume);
        SetSoundVolume(pauseIn, SfxVolume);
        SetSoundVolume(pauseOut, SfxVolume);
        SetSoundVolume(damage, SfxVolume);
        SetSoundVolume(cheer, SfxVolume);
        SetSoundVolume(gameWin, SfxVolume);
        SetSoundVolume(gameOverChild, SfxVolume);
        SetSoundVolume(gameOverCommunity, SfxVolume);
        SetSoundVolume(heroOuch, SfxVolume);
        SetSoundVolume(bossLaserSound, SfxVolume);

        SetSoundVolume(sndJammerHum, SfxVolume);
        SetSoundVolume(sndJammerShot, SfxVolume);
        SetSoundVolume(sndEmpBlast, SfxVolume);
        SetSoundVolume(sndJammerDeath, SfxVolume);
        SetSoundVolume(sndWarpGlide, SfxVolume);
        SetSoundVolume(sndWarpShot, SfxVolume);
        SetSoundVolume(sndTeleport, SfxVolume);
        SetSoundVolume(sndWarpDeath, SfxVolume);

        SetSoundVolume(sndSpecialBeam, SfxVolume);
        SetSoundVolume(sndChargeReady, SfxVolume);
        SetSoundVolume(sndPodDestroy, SfxVolume);
        SetSoundVolume(sndOrbLaunch, SfxVolume);
        SetSoundVolume(sndWarningSiren, SfxVolume);
        SetSoundVolume(sndDefibHum, SfxVolume);
        SetSoundVolume(sndBossEnrage, SfxVolume);
        SetSoundVolume(sndRocketBoost, SfxVolume);

        SetMusicVolume(bgmStory, BgmVolume);
        SetMusicVolume(bgmMenu, BgmVolume);
        SetMusicVolume(bgmBoss, BgmVolume);
        SetMusicVolume(bgmWin, BgmVolume);
        SetMusicVolume(bgmLost, BgmVolume);

        // Screen Shake during boss laser deployment (STOPS IMMEDIATELY on GameOver or death)
        if (bossLaserActive && CurrentState == STATE_GAMEPLAY && !GameOver && !deathSequenceActive)
        {
            screenCamera.offset = (Vector2){ (float)GetRandomValue(-6, 6), (float)GetRandomValue(-6, 6) };
        }
        else
        {
            screenCamera.offset = (Vector2){ 0, 0 };
        }

        BeginDrawing();
        ClearBackground(BLACK);

        BeginMode2D(screenCamera);

        // Animating Background Stars
        if (StarfieldOn && CurrentState != STATE_LOADING)
        {
            for (int i = 0; i < STAR_COUNT; i++)
            {
                DrawCircle((int)StarPos[i].x, (int)StarPos[i].y, (StarSpeed[i] > 110) ? 2.0f : 1.2f, (StarSpeed[i] > 110) ? SKYBLUE : DARKGRAY);
            }
        }

        // STATE: LOADING SCREEN (5 SECONDS)
        if (CurrentState == STATE_LOADING)
        {
            LoadingTimer += Time;
            float progress = LoadingTimer / 5.0f;
            if (progress > 1.0f) progress = 1.0f;

            // 1. Draw full-screen background image without altering image geometry
            if (loadingBg.id > 0)
            {
                Rectangle src = { 0, 0, (float)loadingBg.width, (float)loadingBg.height };
                Rectangle dest = { 0, 0, (float)WindowWidth, (float)WindowHeight };
                DrawTexturePro(loadingBg, src, dest, (Vector2){ 0, 0 }, 0.0f, WHITE);
            }

            // 2. Top Header: SPACE INVADERS
            char titleText[] = "SPACE INVADERS";
            int titleWidth = MeasureText(titleText, 58);
            DrawText(titleText, (WindowWidth - titleWidth) / 2 + 2, 42, 58, Fade(BLACK, 0.85f));
            DrawText(titleText, (WindowWidth - titleWidth) / 2, 40, 58, SKYBLUE);

            // 3. Bottom Loading Console & Progress Bar
            int barWidth = 640;
            int barHeight = 28;
            int barX = (WindowWidth - barWidth) / 2;
            int barY = WindowHeight - 96;

            DrawRectangle(0, WindowHeight - 165, WindowWidth, 165, Fade(BLACK, 0.65f));
            DrawLine(0, WindowHeight - 165, WindowWidth, WindowHeight - 165, Fade(SKYBLUE, 0.40f));

            char subText[] = "INITIALIZING DEFENSE SATELLITES & RADAR...";
            int subWidth = MeasureText(subText, 20);
            DrawText(subText, (WindowWidth - subWidth) / 2, barY - 32, 20, GREEN);

            DrawRectangleLines(barX - 4, barY - 4, barWidth + 8, barHeight + 8, DARKBLUE);
            DrawRectangle(barX, barY, (int)(barWidth * progress), barHeight, LIME);

            DrawText(TextFormat("%d%%", (int)(progress * 100)), barX + barWidth / 2 - 20, barY + 5, 20, BLACK);

            char hintText[] = "System is preparing pure C runtime environment...";
            int hintWidth = MeasureText(hintText, 16);
            DrawText(hintText, (WindowWidth - hintWidth) / 2, barY + 36, 16, LIGHTGRAY);

            if (LoadingTimer >= 5.0f)
            {
                CurrentState = STATE_CINEMATIC;
                cinematicTimer = 0.0f;
                PlayMusicStream(bgmStory);
            }
        }

        // STATE: OPENING VISION: "PROJECT EARTH SHIELD" (20-SECOND CINEMATIC SEQUENCE)
        else if (CurrentState == STATE_CINEMATIC)
        {
            UpdateMusicStream(bgmStory);
            cinematicTimer += Time;

            // Scene 1: Deep Space Radar (0.0s - 6.8s)
            if (cinematicTimer < 6.8f)
            {
                Vector2 center = { WindowWidth / 2.0f, WindowHeight / 2.0f + 20 };

                for (int r = 80; r <= 380; r += 75)
                {
                    DrawCircleLines((int)center.x, (int)center.y, (float)r, DARKGREEN);
                }
                DrawLine((int)center.x - 420, (int)center.y, (int)center.x + 420, (int)center.y, Fade(DARKGREEN, 0.6f));
                DrawLine((int)center.x, (int)center.y - 400, (int)center.x, (int)center.y + 400, Fade(DARKGREEN, 0.6f));

                float sweepAngle = cinematicTimer * 3.8f;
                Vector2 sweepEnd = { center.x + cosf(sweepAngle) * 380.0f, center.y + sinf(sweepAngle) * 380.0f };
                DrawLineEx(center, sweepEnd, 3.0f, LIME);
                DrawCircleSector(center, 380.0f, (sweepAngle - 0.45f) * RAD2DEG, sweepAngle * RAD2DEG, 24, Fade(LIME, 0.18f));

                int blipsToShow = (int)(cinematicTimer * 4.5f) + 3;
                if (blipsToShow > 30) blipsToShow = 30;
                for (int b = 0; b < blipsToShow; b++)
                {
                    float angle = b * 0.72f + 0.35f;
                    float dist = 110.0f + (b * 9.0f);
                    float bx = center.x + cosf(angle) * dist;
                    float by = center.y + sinf(angle) * dist;

                    DrawCircle((int)bx, (int)by, 4.5f, RED);
                    DrawCircleLines((int)bx, (int)by, 8.0f + sinf(cinematicTimer * 8.0f + b) * 3.0f, Fade(RED, 0.7f));
                }

                if (((int)(cinematicTimer * 4)) % 2 == 0)
                {
                    DrawRectangleLinesEx((Rectangle){ 18, 18, WindowWidth - 36, WindowHeight - 36 }, 4, RED);
                }

                const char* rTitle = "[ PROJECT EARTH SHIELD - DEEP SPACE RADAR ]";
                DrawText(rTitle, (WindowWidth - MeasureText(rTitle, 30)) / 2, 45, 30, GREEN);

                const char* rAlert = "!! PRIORITY RED: MULTIPLE EXOSPHERIC INVASION SIGNATURES DETECTED !!";
                DrawText(rAlert, (WindowWidth - MeasureText(rAlert, 20)) / 2, 90, 20, RED);

                DrawText("ORBITAL SECTOR: 04-BARRICADE CRITICAL", 60, WindowHeight - 90, 18, YELLOW);
                DrawText(TextFormat("HOSTILE SIGNATURE COUNT: %d (MULTIPLYING EXPONENTIALLY)", blipsToShow * 24), 60, WindowHeight - 65, 18, ORANGE);
            }
            // Scene 2: Ground Zero Hangar (6.8s - 13.5s)
            else if (cinematicTimer < 13.5f)
            {
                float strobe = fabsf(sinf(cinematicTimer * 7.0f));
                DrawRectangle(0, 0, WindowWidth, WindowHeight, Fade((Color){ 255, 150, 0, 255 }, 0.08f * strobe));

                DrawLineEx((Vector2){ WindowWidth * 0.32f, WindowHeight }, (Vector2){ WindowWidth * 0.37f, 180 }, 8.0f, DARKGRAY);
                DrawLineEx((Vector2){ WindowWidth * 0.38f, WindowHeight }, (Vector2){ WindowWidth * 0.43f, 180 }, 8.0f, DARKGRAY);

                DrawLineEx((Vector2){ WindowWidth * 0.62f, WindowHeight }, (Vector2){ WindowWidth * 0.57f, 180 }, 8.0f, DARKGRAY);
                DrawLineEx((Vector2){ WindowWidth * 0.68f, WindowHeight }, (Vector2){ WindowWidth * 0.63f, 180 }, 8.0f, DARKGRAY);

                float rise = (cinematicTimer - 6.8f) / 6.7f;
                float liftY = WindowHeight - 160.0f - (rise * 220.0f);

                Rectangle h1Dest = { WindowWidth * 0.37f, liftY, HeroWidth, HeroHeight };
                DrawTexturePro(HeroTexture, (Rectangle){ 0, 0, (float)HeroTexture.width, (float)HeroTexture.height }, h1Dest, (Vector2){ HeroWidth/2.0f, HeroHeight/2.0f }, -6.0f, (Color){ 30, 45, 60, 255 });

                Rectangle h2Dest = { WindowWidth * 0.63f, liftY, HeroWidth, HeroHeight };
                DrawTexturePro(StealthHeroTexture, (Rectangle){ 0, 0, (float)StealthHeroTexture.width, (float)StealthHeroTexture.height }, h2Dest, (Vector2){ HeroWidth/2.0f, HeroHeight/2.0f }, 6.0f, (Color){ 30, 45, 60, 255 });

                DrawCircle((int)(WindowWidth * 0.20f), 220, 16.0f, Fade(ORANGE, strobe));
                DrawCircle((int)(WindowWidth * 0.80f), 220, 16.0f, Fade(ORANGE, strobe));

                const char* gzTitle = "[ GROUND ZERO : SUB-ORBITAL HANGAR BAY 09 ]";
                DrawText(gzTitle, (WindowWidth - MeasureText(gzTitle, 32)) / 2, 55, 32, GOLD);

                const char* gzSub = "EMERGENCY ALERT: DUAL INTERCEPTORS ELEVATING TO MAGNETIC LAUNCH RAILS";
                DrawText(gzSub, (WindowWidth - MeasureText(gzSub, 20)) / 2, 100, 20, RAYWHITE);

                DrawText("MAG-RAIL INDUCTION: 98% CHARGED", (WindowWidth - MeasureText("MAG-RAIL INDUCTION: 98% CHARGED", 18)) / 2, WindowHeight - 85, 18, LIME);
            }
            // Scene 3: Neural Link & Thruster Ignition (13.5s - 20.0s)
            else
            {
                DrawRectangle(120, 70, WindowWidth - 240, WindowHeight - 170, Fade(DARKBLUE, 0.40f));
                DrawRectangleLines(120, 70, WindowWidth - 240, WindowHeight - 170, SKYBLUE);

                const char* nTitle = "[ TACTICAL COCKPIT BOOT & NEURAL LINK ]";
                DrawText(nTitle, (WindowWidth - MeasureText(nTitle, 28)) / 2, 95, 28, GOLD);

                DrawText("SYSTEM KERNEL: ACTIVE DEFENSE BUS 6.0_MACOS", 160, 155, 19, LIGHTGRAY);
                DrawText("NEURAL TELEMETRY LINK: STABLE (0.4ms LATENCY)", 160, 185, 19, SKYBLUE);
                DrawText(">> PILOT: SHOAB Mahmud [AEGIS-1] - STATUS: [ ONLINE ]", 160, 225, 22, LIME);
                DrawText(">> PILOT: NAYEMUL Islam [SHADOW-2] - STATUS: [ ONLINE ]", 160, 260, 22, YELLOW);
                DrawText("REAR BLAST DOORS: DISENGAGED... DEEP SPACE VACUUM EXPOSED", 160, 305, 20, RAYWHITE);
                DrawText("AFTERBURNER IGNITION: DUAL BLUE PLASMA CORES FIRING AT MAXIMUM THRUST!", 160, 340, 20, SKYBLUE);

                // Preview portraits with fallback
                DrawRectangleLines(WindowWidth - 360, 150, 96, 96, LIME);
                if (portraitShoab.id > 0)
                    DrawTexturePro(portraitShoab, (Rectangle){ 0, 0, (float)portraitShoab.width, (float)portraitShoab.height }, (Rectangle){ WindowWidth - 360, 150, 96, 96 }, (Vector2){0,0}, 0.0f, WHITE);
                else
                    DrawRectangle(WindowWidth - 360, 150, 96, 96, Fade(DARKGREEN, 0.4f));
                DrawText("AEGIS-1", WindowWidth - 345, 252, 16, LIME);

                DrawRectangleLines(WindowWidth - 240, 150, 96, 96, YELLOW);
                if (portraitNayemul.id > 0)
                    DrawTexturePro(portraitNayemul, (Rectangle){ 0, 0, (float)portraitNayemul.width, (float)portraitNayemul.height }, (Rectangle){ WindowWidth - 240, 150, 96, 96 }, (Vector2){0,0}, 0.0f, WHITE);
                else
                    DrawRectangle(WindowWidth - 240, 150, 96, 96, Fade(DARKBROWN, 0.4f));
                DrawText("SHADOW-2", WindowWidth - 232, 252, 16, YELLOW);

                Vector2 shipCenter = { WindowWidth / 2.0f, WindowHeight - 210.0f };
                DrawStealthFlames(shipCenter, HeroWidth * 1.2f, HeroHeight * 1.2f);
                Rectangle sDest = { shipCenter.x, shipCenter.y, HeroWidth, HeroHeight };
                DrawTexturePro(StealthHeroTexture, (Rectangle){ 0, 0, (float)StealthHeroTexture.width, (float)StealthHeroTexture.height }, sDest, (Vector2){ HeroWidth/2.0f, HeroHeight/2.0f }, 0.0f, WHITE);
            }

            const char* skipText = "PRESS [ENTER] OR [SPACE] TO ADVANCE TO TRANSMISSIONS";
            int skW = MeasureText(skipText, 17);
            if (((int)(GetTime() * 3)) % 2 == 0)
            {
                DrawText(skipText, (WindowWidth - skW) / 2, WindowHeight - 45, 17, GREEN);
            }

            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) || cinematicTimer >= 20.0f)
            {
                PlaySound(menuSelect);
                CurrentState = STATE_STORY;
                storyDialogueIndex = 0;
            }
        }

        // STATE: STORY DIALOGUES (POST-LOADING)
        else if (CurrentState == STATE_STORY)
        {
            UpdateMusicStream(bgmStory);

            int panelW = 1260;
            int panelH = 540;
            int panelX = (WindowWidth - panelW) / 2;
            int panelY = (WindowHeight - panelH) / 2;

            DrawRectangle(panelX, panelY, panelW, panelH, Fade(DARKPURPLE, 0.40f));
            DrawRectangleLines(panelX, panelY, panelW, panelH, SKYBLUE);
            DrawRectangleLines(panelX + 6, panelY + 6, panelW - 12, panelH - 12, DARKBLUE);

            char headerText[] = "TRANSMISSION FREQUENCY 142.80 - ORBITAL DEFENSE COMMAND";
            int hW = MeasureText(headerText, 32);
            DrawText(headerText, (WindowWidth - hW) / 2, panelY + 30, 32, GOLD);
            DrawLine(panelX + 80, panelY + 75, panelX + panelW - 80, panelY + 75, SKYBLUE);

            // Left Portrait Frame: Md. Shoab Mahmud
            int portW = 145, portH = 175;
            int portY = panelY + 115;
            int p1FrameX = panelX + 45;

            // Right Portrait Frame: Nayemul Islam[cite: 1]
            int p2FrameX = panelX + panelW - portW - 45;

            bool shoabSpeaking = (storyDialogueIndex == 1 || storyDialogueIndex == 3);
            bool nayemulSpeaking = (storyDialogueIndex == 2);

            // Draw Shoab Transmission Frame with Safety Fallback
            Color p1Border = shoabSpeaking ? LIME : DARKGRAY;
            Color p1Tint = shoabSpeaking ? WHITE : Fade(GRAY, 0.40f);
            DrawRectangle(p1FrameX - 4, portY - 4, portW + 8, portH + 8, Fade(BLACK, 0.85f));
            if (portraitShoab.id > 0)
            {
                DrawTexturePro(portraitShoab, (Rectangle){ 0, 0, (float)portraitShoab.width, (float)portraitShoab.height },
                               (Rectangle){ p1FrameX, portY, (float)portW, (float)portH }, (Vector2){0,0}, 0.0f, p1Tint);
            }
            else
            {
                DrawRectangle(p1FrameX, portY, portW, portH, Fade(DARKGREEN, 0.35f));
                DrawText("SHOAB", p1FrameX + 25, portY + portH / 2 - 10, 20, LIME);
            }
            DrawRectangleLinesEx((Rectangle){ p1FrameX, portY, (float)portW, (float)portH }, shoabSpeaking ? 3.5f : 1.5f, p1Border);
            DrawText("SHOAB", p1FrameX + (portW - MeasureText("SHOAB", 18)) / 2, portY + portH + 10, 18, shoabSpeaking ? LIME : GRAY);

            // Draw Nayemul Transmission Frame with Safety Fallback[cite: 1]
            Color p2Border = nayemulSpeaking ? YELLOW : DARKGRAY;
            Color p2Tint = nayemulSpeaking ? WHITE : Fade(GRAY, 0.40f);
            DrawRectangle(p2FrameX - 4, portY - 4, portW + 8, portH + 8, Fade(BLACK, 0.85f));
            if (portraitNayemul.id > 0)
            {
                DrawTexturePro(portraitNayemul, (Rectangle){ 0, 0, (float)portraitNayemul.width, (float)portraitNayemul.height },
                               (Rectangle){ p2FrameX, portY, (float)portW, (float)portH }, (Vector2){0,0}, 0.0f, p2Tint);
            }
            else
            {
                DrawRectangle(p2FrameX, portY, portW, portH, Fade(DARKBROWN, 0.35f));
                DrawText("NAYEMUL", p2FrameX + 15, portY + portH / 2 - 10, 20, YELLOW);
            }
            DrawRectangleLinesEx((Rectangle){ p2FrameX, portY, (float)portW, (float)portH }, nayemulSpeaking ? 3.5f : 1.5f, p2Border);
            DrawText("NAYEMUL", p2FrameX + (portW - MeasureText("NAYEMUL", 18)) / 2, portY + portH + 10, 18, nayemulSpeaking ? YELLOW : GRAY);

            int dTextX = panelX + 225;

            if (storyDialogueIndex == 0)
            {
                DrawText("[ GLOBAL BROADCAST NETWORK - EMERGENCY COMMS ]", dTextX, panelY + 115, 23, RED);
                DrawText("Global Comms:", dTextX, panelY + 160, 24, ORANGE);
                DrawText("\"Mayday! Mayday! Sector 4 orbital barricade has collapsed.", dTextX, panelY + 205, 22, RAYWHITE);
                DrawText("Hostile alien swarms have entered low Earth orbit.", dTextX, panelY + 240, 22, LIGHTGRAY);
                DrawText("All ground planetary defense platforms are offline!\"", dTextX, panelY + 275, 22, RED);
            }
            else if (storyDialogueIndex == 1)
            {
                DrawText("[ TACTICAL RADAR COMMS - INTERCEPTOR 01 ]", dTextX, panelY + 115, 23, LIME);
                DrawText("Shoab (Portrait Glitching):", dTextX, panelY + 160, 24, LIME);
                DrawText("\"Radar is flooded with signatures. They didn't come to", dTextX, panelY + 205, 22, RAYWHITE);
                DrawText("negotiate, Nayemul... they're completely surrounding", dTextX, panelY + 240, 22, RAYWHITE);
                DrawText("the entire hemisphere!\"", dTextX, panelY + 275, 22, YELLOW);
            }
            else if (storyDialogueIndex == 2)
            {
                DrawText("[ STEALTH COCKPIT COMMS - INTERCEPTOR 02 ]", dTextX, panelY + 115, 23, YELLOW);
                DrawText("Nayemul (Engines Roaring):", dTextX, panelY + 160, 24, YELLOW);
                DrawText("\"Let them come. Stealth wings are locked, and dual plasma", dTextX, panelY + 205, 22, RAYWHITE);
                DrawText("accelerators are fully charged. Earth is not going down", dTextX, panelY + 240, 22, RAYWHITE);
                DrawText("on our watch!\"", dTextX, panelY + 275, 22, SKYBLUE);
            }
            else if (storyDialogueIndex == 3)
            {
                DrawText("[ LAUNCH RAIL CONTROL - FINAL BRIEF ]", dTextX, panelY + 115, 23, LIME);
                DrawText("Shoab:", dTextX, panelY + 160, 24, LIME);
                DrawText("\"Launch rail magnets disengaged in 3... 2... 1...", dTextX, panelY + 205, 23, LIME);
                DrawText("Break through their formation and protect Earth!\"", dTextX, panelY + 245, 23, RAYWHITE);
            }
            else if (storyDialogueIndex == 4)
            {
                DrawText("[ INCOMING ENCRYPTED THREAT TRANSMISSION ]", dTextX, panelY + 115, 23, MAROON);
                DrawText("Alien Overlord (Ominous Static Intrusion):", dTextX, panelY + 160, 24, RED);
                DrawText("\"Insignificant specks. Your atmosphere will burn, and your", dTextX, panelY + 205, 22, RED);
                DrawText("civilization will be extinguished before the sun rises.\"", dTextX, panelY + 240, 22, RED);
                DrawText("\"You fly directly into your own extinction!\"", dTextX, panelY + 280, 22, ORANGE);
            }

            char nextPrompt[] = "PRESS [ENTER] OR [SPACE] TO CONTINUE";
            int pW = MeasureText(nextPrompt, 20);
            if (((int)(GetTime() * 3)) % 2 == 0)
            {
                DrawText(nextPrompt, (WindowWidth - pW) / 2, panelY + panelH - 45, 20, GREEN);
            }

            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE))
            {
                PlaySound(menuSelect);
                storyDialogueIndex++;
                if (storyDialogueIndex > 4)
                {
                    StopMusicStream(bgmStory);
                    CurrentState = STATE_MENU;
                    PlayMusicStream(bgmMenu);
                }
            }
        }

        // STATE: MAIN MENU
        else if (CurrentState == STATE_MENU)
        {
            UpdateMusicStream(bgmMenu);

            int panelW = 1260;
            int panelH = 740;
            int panelX = (WindowWidth - panelW) / 2;
            int panelY = (WindowHeight - panelH) / 2;

            DrawRectangle(panelX, panelY, panelW, panelH, Fade(DARKPURPLE, 0.25f));
            DrawRectangleLines(panelX, panelY, panelW, panelH, SKYBLUE);
            DrawRectangleLines(panelX + 6, panelY + 6, panelW - 12, panelH - 12, DARKBLUE);

            char menuTitle[] = "SPACE INVADERS : EARTH ALLIANCE";
            int mTitleW = MeasureText(menuTitle, 52);
            DrawText(menuTitle, (WindowWidth - mTitleW) / 2, panelY + 45, 52, GOLD);

            char menuSub[] = "COMMAND CONSOLE & INTERCEPTION SYSTEM";
            int mSubW = MeasureText(menuSub, 20);
            DrawText(menuSub, (WindowWidth - mSubW) / 2, panelY + 110, 20, RAYWHITE);

            DrawLine(panelX + 100, panelY + 150, panelX + panelW - 100, panelY + 150, SKYBLUE);

            if (IsKeyPressed(KEY_UP))
            {
                PlaySound(menuMove);
                MenuSelection--;
                if (MenuSelection < 0) MenuSelection = 3;
            }
            if (IsKeyPressed(KEY_DOWN))
            {
                PlaySound(menuMove);
                MenuSelection++;
                if (MenuSelection > 3) MenuSelection = 0;
            }

            char itemNames[4][32] = { "PLAY", "OPTIONS", "CREDENTIALS", "EXIT" };

            for (int i = 0; i < 4; i++)
            {
                int btnW = 560;
                int btnH = 68;
                int btnX = (WindowWidth - btnW) / 2;
                int btnY = panelY + 215 + (i * 105);

                if (MenuSelection == i)
                {
                    DrawRectangle(btnX, btnY, btnW, btnH, Fade(SKYBLUE, 0.25f));
                    DrawRectangleLines(btnX, btnY, btnW, btnH, LIME);
                    DrawRectangle(btnX - 18, btnY + 18, 8, 32, YELLOW);
                    DrawRectangle(btnX + btnW + 10, btnY + 18, 8, 32, YELLOW);

                    int textW = MeasureText(itemNames[i], 26);
                    DrawText(itemNames[i], (WindowWidth - textW) / 2, btnY + 20, 26, YELLOW);
                }
                else
                {
                    DrawRectangle(btnX, btnY, btnW, btnH, Fade(BLACK, 0.70f));
                    DrawRectangleLines(btnX, btnY, btnW, btnH, DARKGRAY);

                    int textW = MeasureText(itemNames[i], 24);
                    DrawText(itemNames[i], (WindowWidth - textW) / 2, btnY + 22, 24, LIGHTGRAY);
                }
            }

            char footerText[] = "USE [UP / DOWN] TO NAVIGATE   or   [ENTER / SPACE] TO EXECUTE";
            int footerW = MeasureText(footerText, 18);
            DrawText(footerText, (WindowWidth - footerW) / 2, panelY + panelH - 45, 18, GREEN);

            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE))
            {
                PlaySound(menuSelect);
                if (MenuSelection == 0)
                {
                    StopMusicStream(bgmMenu);
                    CurrentState = STATE_LAUNCH;
                    launchDialogueIndex = 0;
                    launchCountdownTimer = 3.0f;
                    launchCountdownActive = false;
                    launchShip1Pos = (Vector2){ WindowWidth * 0.40f, WindowHeight - 240 };
                    launchShip2Pos = (Vector2){ WindowWidth * 0.60f, WindowHeight - 240 };
                    PlayMusicStream(bgmStory);
                }
                else if (MenuSelection == 1) CurrentState = STATE_OPTIONS;
                else if (MenuSelection == 2) CurrentState = STATE_CREDITS;
                else if (MenuSelection == 3) break;
            }
        }

        // STATE: PRE-GAMEPLAY LAUNCH & DIALOGUE (EARTH ORBIT VIEW)
        else if (CurrentState == STATE_LAUNCH)
        {
            UpdateMusicStream(bgmStory);

            DrawCircle(WindowWidth / 2, WindowHeight + 620, 840, DARKBLUE);
            DrawCircle(WindowWidth / 2, WindowHeight + 620, 830, (Color){ 20, 50, 110, 255 });
            DrawCircle(WindowWidth / 2 - 200, WindowHeight - 30, 120, (Color){ 30, 90, 45, 255 });
            DrawCircle(WindowWidth / 2 + 180, WindowHeight - 50, 140, (Color){ 35, 100, 50, 255 });
            DrawCircleLines(WindowWidth / 2, WindowHeight + 620, 842, Fade(SKYBLUE, 0.45f));
            DrawCircleLines(WindowWidth / 2, WindowHeight + 620, 846, Fade(SKYBLUE, 0.20f));

            if (launchCountdownActive)
            {
                launchCountdownTimer -= Time;
                launchShip1Pos.y -= 180.0f * Time;
                launchShip2Pos.y -= 180.0f * Time;

                if (launchCountdownTimer <= 0.0f)
                {
                    StopMusicStream(bgmStory);
                    CurrentState = STATE_GAMEPLAY;
                    Hero1Pos = (Vector2){ WindowWidth * 0.35f, WindowHeight - HeroHeight };
                    Hero2Pos = (Vector2){ WindowWidth * 0.65f, WindowHeight - HeroHeight };
                }
            }

            Rectangle h1Src = { 0, 0, (float)HeroTexture.width, (float)HeroTexture.height };
            Rectangle h1Dest = { launchShip1Pos.x, launchShip1Pos.y, HeroWidth, HeroHeight };
            DrawTexturePro(HeroTexture, h1Src, h1Dest, (Vector2){ HeroWidth / 2.0f, HeroHeight / 2.0f }, -12.0f, WHITE);

            DrawStealthFlames(launchShip2Pos, HeroWidth, HeroHeight);
            Rectangle h2Src = { 0, 0, (float)StealthHeroTexture.width, (float)StealthHeroTexture.height };
            Rectangle h2Dest = { launchShip2Pos.x, launchShip2Pos.y, HeroWidth, HeroHeight };
            DrawTexturePro(StealthHeroTexture, h2Src, h2Dest, (Vector2){ HeroWidth / 2.0f, HeroHeight / 2.0f }, 12.0f, WHITE);

            int dlgW = 1200;
            int dlgH = 260;
            int dlgX = (WindowWidth - dlgW) / 2;
            int dlgY = 60;

            DrawRectangle(dlgX, dlgY, dlgW, dlgH, Fade(BLACK, 0.82f));
            DrawRectangleLines(dlgX, dlgY, dlgW, dlgH, SKYBLUE);
            DrawRectangleLines(dlgX + 4, dlgY + 4, dlgW - 8, dlgH - 8, DARKBLUE);

            if (launchDialogueIndex == 0)
            {
                DrawText("[ MISSION BRIEFING: OPERATION SKYFALL ]", dlgX + 40, dlgY + 25, 24, GOLD);
                DrawText("Shoab: \"Nayemul, long-range orbital radar has confirmed the invasion grid!\"", dlgX + 40, dlgY + 75, 22, LIME);
                DrawText("\"Alien swarm is descending from the exosphere. They are threatening billions of lives on Earth.\"", dlgX + 40, dlgY + 115, 20, LIGHTGRAY);
                DrawText("Nayemul: \"Our interceptors are primed. We're launching right through their frontline!\"", dlgX + 40, dlgY + 160, 22, YELLOW);
            }
            else if (launchDialogueIndex == 1)
            {
                DrawText("[ LAUNCH RAIL PRESSURE & DIAGNOSTICS ]", dlgX + 40, dlgY + 25, 24, GOLD);
                DrawText("Shoab: \"Auxiliary cooling line 2 is leaking pressure, but we don't have time for repairs.", dlgX + 40, dlgY + 75, 21, LIME);
                DrawText("Nayemul, verify your nav-matrix!\"", dlgX + 40, dlgY + 105, 21, LIME);
                DrawText("Nayemul: \"Pre-flight diagnostics bypassed. All thrusters responding.", dlgX + 40, dlgY + 150, 21, YELLOW);
                DrawText("Let's make sure Earth is still here when we get back down.\"", dlgX + 40, dlgY + 180, 21, YELLOW);
            }
            else if (launchDialogueIndex == 2)
            {
                DrawText("[ PRE-FLIGHT AUTHORIZATION - DUAL STRIKE FLEET ]", dlgX + 40, dlgY + 25, 24, GOLD);
                DrawText("Shoab: \"Plasma cannons energized and hull shields synchronized. Remember the formation:\"", dlgX + 40, dlgY + 75, 22, LIME);
                DrawText("\"I will control the left sector [A/D to Move, W to Fire]. You take the right sector!\"", dlgX + 40, dlgY + 115, 20, LIGHTGRAY);
                DrawText("Nayemul: \"Understood! Stealth wings locked [Arrow Keys to Move, UP to Fire]. Let's ride!\"", dlgX + 40, dlgY + 160, 22, YELLOW);
            }
            else if (launchDialogueIndex == 3)
            {
                DrawText("[ ATMOSPHERIC BREACH - DEFENSE COMMAND ]", dlgX + 40, dlgY + 25, 24, GOLD);
                DrawText("Command: \"Alliance actual to Strike Flight: You have cleared the thermosphere.", dlgX + 40, dlgY + 75, 22, SKYBLUE);
                DrawText("Grid Sector Alpha is compromised. Weapons free.\"", dlgX + 40, dlgY + 115, 22, SKYBLUE);
                DrawText("Nayemul & Shoab: \"Interceptors through the orbital barrier! Weapons free!\"", dlgX + 40, dlgY + 160, 22, GREEN);
            }

            if (!launchCountdownActive)
            {
                char promptText[] = "PRESS [ENTER] OR [SPACE] TO ADVANCE LAUNCH SEQUENCE";
                int prW = MeasureText(promptText, 18);
                if (((int)(GetTime() * 3)) % 2 == 0)
                {
                    DrawText(promptText, (WindowWidth - prW) / 2, dlgY + dlgH - 40, 18, GREEN);
                }

                if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE))
                {
                    PlaySound(menuSelect);
                    launchDialogueIndex++;
                    if (launchDialogueIndex > 3)
                    {
                        launchCountdownActive = true;
                        PlaySound(sndRocketBoost);
                    }
                }
            }
            else
            {
                const char* launchText = TextFormat("INTERCEPTORS LAUNCHING IN %.1f SECONDS...", launchCountdownTimer);
                int lw = MeasureText(launchText, 26);
                DrawText(launchText, (WindowWidth - lw) / 2, dlgY + dlgH - 50, 26, YELLOW);
            }
        }

        // STATE: OPTIONS (SOUND and VIDEO SETTINGS)
        else if (CurrentState == STATE_OPTIONS)
        {
            UpdateMusicStream(bgmMenu);

            int panelW = 1260;
            int panelH = 740;
            int panelX = (WindowWidth - panelW) / 2;
            int panelY = (WindowHeight - panelH) / 2;

            DrawRectangle(panelX, panelY, panelW, panelH, Fade(DARKBLUE, 0.20f));
            DrawRectangleLines(panelX, panelY, panelW, panelH, SKYBLUE);

            char optTitle[] = "SYSTEM & TACTICAL CONFIGURATION";
            int optW = MeasureText(optTitle, 40);
            DrawText(optTitle, (WindowWidth - optW) / 2, panelY + 40, 40, GOLD);

            if (IsKeyPressed(KEY_TAB))
            {
                PlaySound(menuMove);
                OptionsTab = 1 - OptionsTab;
            }

            int tabW = 280;
            int tabH = 46;
            int soundTabX = panelX + 320;
            int videoTabX = panelX + 660;
            int tabY = panelY + 110;

            DrawRectangle(soundTabX, tabY, tabW, tabH, (OptionsTab == 0) ? Fade(SKYBLUE, 0.35f) : Fade(BLACK, 0.6f));
            DrawRectangleLines(soundTabX, tabY, tabW, tabH, (OptionsTab == 0) ? YELLOW : DARKGRAY);
            DrawText("1. AUDIO SETTINGS", soundTabX + 35, tabY + 14, 20, (OptionsTab == 0) ? YELLOW : LIGHTGRAY);

            DrawRectangle(videoTabX, tabY, tabW, tabH, (OptionsTab == 1) ? Fade(SKYBLUE, 0.35f) : Fade(BLACK, 0.6f));
            DrawRectangleLines(videoTabX, tabY, tabW, tabH, (OptionsTab == 1) ? YELLOW : DARKGRAY);
            DrawText("2. VIDEO SETTINGS", videoTabX + 35, tabY + 14, 20, (OptionsTab == 1) ? YELLOW : LIGHTGRAY);

            DrawLine(panelX + 60, tabY + 65, panelX + panelW - 60, tabY + 65, DARKBLUE);

            // AUDIO TAB
            if (OptionsTab == 0)
            {
                if (IsKeyPressed(KEY_UP))   { PlaySound(menuMove); SoundSelection--; if (SoundSelection < 0) SoundSelection = 2; }
                if (IsKeyPressed(KEY_DOWN)) { PlaySound(menuMove); SoundSelection++; if (SoundSelection > 2) SoundSelection = 0; }

                char bgmTracks[3][32] = { "SYNTHWAVE ODYSSEY", "8-BIT ARCADE VIBE", "COSMIC VOID AMBIENCE" };

                if (SoundSelection == 0)
                {
                    if (IsKeyPressed(KEY_LEFT))  { PlaySound(menuMove); BgmTrackIndex--; if (BgmTrackIndex < 0) BgmTrackIndex = 2; }
                    if (IsKeyPressed(KEY_RIGHT)) { PlaySound(menuMove); BgmTrackIndex++; if (BgmTrackIndex > 2) BgmTrackIndex = 0; }
                }
                else if (SoundSelection == 1)
                {
                    if (IsKeyDown(KEY_LEFT))  { BgmVolume -= 0.3f * Time; if (BgmVolume < 0.0f) BgmVolume = 0.0f; }
                    if (IsKeyDown(KEY_RIGHT)) { BgmVolume += 0.3f * Time; if (BgmVolume > 1.0f) BgmVolume = 1.0f; }
                }
                else if (SoundSelection == 2)
                {
                    if (IsKeyDown(KEY_LEFT))  { SfxVolume -= 0.3f * Time; if (SfxVolume < 0.0f) SfxVolume = 0.0f; }
                    if (IsKeyDown(KEY_RIGHT)) { SfxVolume += 0.3f * Time; if (SfxVolume > 1.0f) SfxVolume = 1.0f; }
                }

                int rowY = panelY + 240;
                DrawText("BGM SOUNDTRACK", panelX + 160, rowY, 24, (SoundSelection == 0) ? YELLOW : WHITE);
                DrawText("<", panelX + 540, rowY, 24, (SoundSelection == 0) ? LIME : DARKGRAY);
                DrawText(bgmTracks[BgmTrackIndex], panelX + 580, rowY, 24, (SoundSelection == 0) ? SKYBLUE : RAYWHITE);
                DrawText(">", panelX + 960, rowY, 24, (SoundSelection == 0) ? LIME : DARKGRAY);

                rowY += 100;
                DrawText("BGM VOLUME", panelX + 160, rowY, 24, (SoundSelection == 1) ? YELLOW : WHITE);
                DrawRectangle(panelX + 540, rowY + 4, 380, 20, DARKGRAY);
                DrawRectangle(panelX + 540, rowY + 4, (int)(380 * BgmVolume), 20, GREEN);
                DrawRectangleLines(panelX + 540, rowY + 4, 380, 20, WHITE);
                DrawText(TextFormat("%d%%", (int)(BgmVolume * 100)), panelX + 940, rowY, 22, YELLOW);

                rowY += 100;
                DrawText("SFX [SHOOT/KILL] VOLUME", panelX + 160, rowY, 24, (SoundSelection == 2) ? YELLOW : WHITE);
                DrawRectangle(panelX + 540, rowY + 4, 380, 20, DARKGRAY);
                DrawRectangle(panelX + 540, rowY + 4, (int)(380 * SfxVolume), 20, ORANGE);
                DrawRectangleLines(panelX + 540, rowY + 4, 380, 20, WHITE);
                DrawText(TextFormat("%d%%", (int)(SfxVolume * 100)), panelX + 940, rowY, 22, YELLOW);
            }
            // VIDEO TAB
            else
            {
                if (IsKeyPressed(KEY_UP))   { PlaySound(menuMove); VideoSelection--; if (VideoSelection < 0) VideoSelection = 3; }
                if (IsKeyPressed(KEY_DOWN)) { PlaySound(menuMove); VideoSelection++; if (VideoSelection > 3) VideoSelection = 0; }

                if (VideoSelection == 0)
                {
                    if (IsKeyDown(KEY_LEFT))  { Brightness -= 0.3f * Time; if (Brightness < 0.5f) Brightness = 0.5f; }
                    if (IsKeyDown(KEY_RIGHT)) { Brightness += 0.3f * Time; if (Brightness > 1.5f) Brightness = 1.5f; }
                }
                else if (VideoSelection == 1)
                {
                    if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_ENTER))
                    {
                        PlaySound(menuSelect);
                        ScanlinesOn = !ScanlinesOn;
                    }
                }
                else if (VideoSelection == 2)
                {
                    if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_ENTER))
                    {
                        PlaySound(menuSelect);
                        StarfieldOn = !StarfieldOn;
                    }
                }
                else if (VideoSelection == 3)
                {
                    if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_ENTER))
                    {
                        PlaySound(menuSelect);
                        ToggleFullscreen();
                    }
                }

                int rowY = panelY + 220;
                DrawText("DISPLAY BRIGHTNESS", panelX + 160, rowY, 24, (VideoSelection == 0) ? YELLOW : WHITE);
                DrawRectangle(panelX + 540, rowY + 4, 380, 20, DARKGRAY);
                DrawRectangle(panelX + 540, rowY + 4, (int)(380 * ((Brightness - 0.5f) / 1.0f)), 20, SKYBLUE);
                DrawRectangleLines(panelX + 540, rowY + 4, 380, 20, WHITE);
                DrawText(TextFormat("%d%%", (int)(Brightness * 100)), panelX + 940, rowY, 22, YELLOW);

                rowY += 80;
                DrawText("CRT SCANLINE FILTER", panelX + 160, rowY, 24, (VideoSelection == 1) ? YELLOW : WHITE);
                DrawText(ScanlinesOn ? "[ ENABLED ]" : "[ DISABLED ]", panelX + 540, rowY, 24, ScanlinesOn ? LIME : RED);

                rowY += 80;
                DrawText("SPACE STARFIELD ENGINE", panelX + 160, rowY, 24, (VideoSelection == 2) ? YELLOW : WHITE);
                DrawText(StarfieldOn ? "[ ACTIVE ]" : "[ OFFLINE ]", panelX + 540, rowY, 24, StarfieldOn ? LIME : RED);

                rowY += 80;
                DrawText("FULLSCREEN DISPLAY [F11]", panelX + 160, rowY, 24, (VideoSelection == 3) ? YELLOW : WHITE);
                DrawText(IsWindowFullscreen() ? "[ ENABLED ]" : "[ DISABLED ]", panelX + 540, rowY, 24, IsWindowFullscreen() ? LIME : RED);
            }

            char optFooter[] = "[TAB] SWITCH TAB   |   [LEFT / RIGHT / ENTER] ADJUST   |   [BACKSPACE / ESC] RETURN";
            int optFW = MeasureText(optFooter, 18);
            DrawText(optFooter, (WindowWidth - optFW) / 2, panelY + panelH - 45, 18, GREEN);

            if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressed(KEY_ESCAPE))
            {
                PlaySound(menuSelect);
                CurrentState = STATE_MENU;
            }
        }

        // STATE: CREDENTIALS SCREEN
        else if (CurrentState == STATE_CREDITS)
        {
            UpdateMusicStream(bgmMenu);

            int panelW = 1260;
            int panelH = 740;
            int panelX = (WindowWidth - panelW) / 2;
            int panelY = (WindowHeight - panelH) / 2;

            DrawRectangle(panelX, panelY, panelW, panelH, Fade(DARKGRAY, 0.25f));
            DrawRectangleLines(panelX, panelY, panelW, panelH, SKYBLUE);
            DrawRectangleLines(panelX + 6, panelY + 6, panelW - 12, panelH - 12, DARKBLUE);

            char credHeader[] = "PROJECT DEVELOPERS & CREDENTIALS";
            int headW = MeasureText(credHeader, 38);
            DrawText(credHeader, (WindowWidth - headW) / 2, panelY + 50, 38, GOLD);

            DrawLine(panelX + 100, panelY + 110, panelX + panelW - 100, panelY + 110, SKYBLUE);

            int cardW = 520;
            int cardH = 260;
            int cardY = panelY + 170;

            // Left Card: Md. Shoab Mahmud
            int card1X = panelX + 80;
            DrawRectangle(card1X, cardY, cardW, cardH, Fade(BLACK, 0.75f));
            DrawRectangleLines(card1X, cardY, cardW, cardH, SKYBLUE);

            DrawText("Md. Shoab Mahmud", card1X + 35, cardY + 30, 26, YELLOW);
            DrawText("Roll ID: 2505066", card1X + 35, cardY + 70, 20, GREEN);
            DrawLine(card1X + 35, cardY + 102, card1X + cardW - 35, cardY + 102, DARKGRAY);

            DrawText("Contributions:", card1X + 35, cardY + 120, 19, RAYWHITE);
            DrawText("- Rendering of Aliens and Hero", card1X + 35, cardY + 155, 17, LIGHTGRAY);
            DrawText("- Sprite Collection", card1X + 35, cardY + 185, 17, LIGHTGRAY);
            DrawText("- Alien Movements", card1X + 35, cardY + 215, 17, LIGHTGRAY);

            // Right Card: Nayemul Islam[cite: 1]
            int card2X = panelX + 660;
            DrawRectangle(card2X, cardY, cardW, cardH, Fade(BLACK, 0.75f));
            DrawRectangleLines(card2X, cardY, cardW, cardH, SKYBLUE);

            DrawText("Nayemul Islam", card2X + 35, cardY + 30, 26, YELLOW);
            DrawText("Roll ID: 2505087", card2X + 35, cardY + 70, 20, GREEN);
            DrawLine(card2X + 35, cardY + 102, card2X + cardW - 35, cardY + 102, DARKGRAY);

            DrawText("Contributions:", card2X + 35, cardY + 120, 19, RAYWHITE);
            DrawText("- Hero and Alien Shooting Logic", card2X + 35, cardY + 155, 17, LIGHTGRAY);
            DrawText("- Collision Detections", card2X + 35, cardY + 185, 17, LIGHTGRAY);
            DrawText("- Game Restart and Game Menu", card2X + 35, cardY + 215, 17, LIGHTGRAY);

            char badge[] = "Built natively with Raylib Framework in Pure C Language";
            int badgeW = MeasureText(badge, 20);
            DrawText(badge, (WindowWidth - badgeW) / 2, panelY + 490, 20, SKYBLUE);

            char credBack[] = "PRESS [BACKSPACE] OR [ESC] TO RETURN TO MENU";
            int backW = MeasureText(credBack, 18);
            DrawText(credBack, (WindowWidth - backW) / 2, panelY + panelH - 50, 18, GREEN);

            if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressed(KEY_ESCAPE))
            {
                PlaySound(menuSelect);
                CurrentState = STATE_MENU;
            }
        }

        // STATE: ACTIVE GAMEPLAY
        else if (CurrentState == STATE_GAMEPLAY)
        {
            if (bossWarningActive || bossActive)
            {
                UpdateMusicStream(bgmBoss);
            }

            // Toggle pause state with 'P' key
            if (!GameOver && !bossDefeated && !deathSequenceActive)
            {
                if (IsKeyPressed(KEY_P))
                {
                    isPaused = !isPaused;
                    if (isPaused) PlaySound(pauseIn);
                    else PlaySound(pauseOut);
                }
            }

            if (isPaused)
            {
                for (int X = 0; X < AlienInX; X++)
                {
                    for (int Y = 0; Y < AlienInY; Y++)
                    {
                        if (AlienAlive[X][Y])
                        {
                            Rectangle Alien = { AlienPos[X][Y].x, AlienPos[X][Y].y, AlienSize, AlienSize };
                            int AStyle = (int)(GetTime() / 0.1) % AlienSpriteStyle[AlienLooks[X][Y]-1];
                            DrawTexturePro(AlienTexture[AlienLooks[X][Y]-1],
                                (Rectangle){ AStyle*(AlienSWidth[AlienLooks[X][Y]-1]+1), 
                                    0,
                                    AlienSWidth[AlienLooks[X][Y]-1], 
                                    (-1)*AlienSHeight[AlienLooks[X][Y]-1] 
                                },
                                Alien, (Vector2){ 0, 0 }, 0.0f, WHITE);
                        }
                    }
                }

                if (jammerActive)
                {
                    Rectangle jRec = { jammerPos.x, jammerPos.y, COMMANDER_SIZE, COMMANDER_SIZE };
                    DrawTexturePro(jammerTex[jammerAnimState], (Rectangle){ 0, 0, (float)jammerTex[jammerAnimState].width, (float)jammerTex[jammerAnimState].height }, jRec, (Vector2){ 0, 0 }, 0.0f, WHITE);
                }
                if (warpActive)
                {
                    Rectangle wRec = { warpPos.x, warpPos.y, COMMANDER_SIZE, COMMANDER_SIZE };
                    DrawTexturePro(warpTex[warpAnimState], (Rectangle){ 0, 0, (float)warpTex[warpAnimState].width, (float)warpTex[warpAnimState].height }, wRec, (Vector2){ 0, 0 }, 0.0f, WHITE);
                }

                for (int b = 0; b < MAX_COMMANDER_BULLETS; b++)
                {
                    if (jammerBulletActive[b]) DrawRectangle((int)(jammerBulletPos[b].x - 4), (int)jammerBulletPos[b].y, 8, 22, (Color){ 200, 70, 255, 255 });
                    if (warpBulletActive[b]) DrawRectangle((int)(warpBulletPos[b].x - 3), (int)warpBulletPos[b].y, 6, 26, SKYBLUE);
                }

                if (bossActive)
                {
                    Rectangle bossRec = { bossPos.x, bossPos.y, BossWidth, BossHeight };
                    DrawTexturePro(BossTexture[0], (Rectangle){ 0, 0, (float)BossTexture[0].width, (float)BossTexture[0].height }, bossRec, (Vector2){ 0, 0 }, 0.0f, WHITE);

                    int AStyle = (int)(GetTime() / 0.1) % AlienSpriteStyle[19];
                    for (int m = 0; m < MAX_MINIONS; m++)
                    {
                        if (minionActive[m])
                        {
                            Rectangle mDest = { minionPos[m].x, minionPos[m].y, (float)MinionSize, (float)MinionSize };
                            float frameX = (float)(AStyle * (AlienSWidth[12] + 1));
                            DrawTexturePro(
                                AlienTexture[12],
                                (Rectangle){ frameX, 0.0f, (float)AlienSWidth[12], (float)AlienSHeight[12] },
                                mDest, (Vector2){ 0, 0 }, 0.0f, WHITE
                            );
                        }
                    }
                }

                for (int i = 0; i < 2; i++)
                {
                    if (Hero1BulletActive[i]) DrawRectangle((int)(Hero1BulletPos[i].x - BulletWidth / 2.0f), (int)Hero1BulletPos[i].y, BulletWidth, BulletHeight, YELLOW);
                    if (Hero2BulletActive[i]) DrawRectangle((int)(Hero2BulletPos[i].x - BulletWidth / 2.0f), (int)Hero2BulletPos[i].y, BulletWidth, BulletHeight, SKYBLUE);
                }
                if (Hero1SpecialBulletActive) DrawRectangle((int)(Hero1SpecialBulletPos.x - BulletWidth / 2.0f), (int)Hero1SpecialBulletPos.y, BulletWidth, BulletHeight * 5, WHITE);

                if (AlienBulletActive) DrawRectangle((int)(AlienBulletPos.x - AlienBulletWidth / 2.0f), (int)AlienBulletPos.y, AlienBulletWidth, AlienBulletHeight, RED);

                for (int mb = 0; mb < MAX_MINION_BULLETS; mb++)
                {
                    if (minionBulletActive[mb])
                    {
                        DrawRectangle((int)(minionBulletPos[mb].x - AlienBulletWidth / 2.0f), (int)minionBulletPos[mb].y, AlienBulletWidth, AlienBulletHeight, ORANGE);
                    }
                }

                if (Hero1Lives > 0)
                {
                    Rectangle h1 = { Hero1Pos.x - HeroWidth / 2.0f, Hero1Pos.y, HeroWidth, HeroHeight };
                    DrawTexturePro(HeroTexture, (Rectangle){ 0, 0, (float)HeroTexture.width, (float)HeroTexture.height }, h1, (Vector2){ 0, 0 }, 0.0f, WHITE);
                }

                if (Hero2Lives > 0)
                {
                    Vector2 h2Center = { Hero2Pos.x, Hero2Pos.y + HeroHeight / 2.0f };
                    DrawStealthFlames(h2Center, HeroWidth, HeroHeight);
                    Rectangle h2 = { Hero2Pos.x - HeroWidth / 2.0f, Hero2Pos.y, HeroWidth, HeroHeight };
                    DrawTexturePro(StealthHeroTexture, (Rectangle){ 0, 0, (float)StealthHeroTexture.width, (float)StealthHeroTexture.height }, h2, (Vector2){ 0, 0 }, 0.0f, WHITE);
                }

                // Pause Modal Overlay
                DrawRectangle(0, 0, WindowWidth, WindowHeight, Fade(BLACK, 0.6f));
                int pBoxW = 450;
                int pBoxH = 180;
                int pBoxX = (WindowWidth - pBoxW) / 2;
                int pBoxY = (WindowHeight - pBoxH) / 2;

                DrawRectangle(pBoxX, pBoxY, pBoxW, pBoxH, Fade(DARKBLUE, 0.90f));
                DrawRectangleLines(pBoxX, pBoxY, pBoxW, pBoxH, SKYBLUE);

                char pauseTitle[] = "GAME PAUSED";
                DrawText(pauseTitle, pBoxX + (pBoxW - MeasureText(pauseTitle, 36)) / 2, pBoxY + 35, 36, YELLOW);

                char pauseSub[] = "Press [P] to Resume   |   [ESC] for Menu";
                DrawText(pauseSub, pBoxX + (pBoxW - MeasureText(pauseSub, 18)) / 2, pBoxY + 110, 18, RAYWHITE);

                if (IsKeyPressed(KEY_ESCAPE))
                {
                    PlaySound(menuSelect);
                    isPaused = false;
                    StopMusicStream(bgmBoss);
                    CurrentState = STATE_MENU;
                    PlayMusicStream(bgmMenu);
                }

                EndMode2D();
                EndDrawing();
                continue;
            }

            // Timers
            if (hero1HitFlashTimer > 0.0f) hero1HitFlashTimer -= Time;
            if (hero2HitFlashTimer > 0.0f) hero2HitFlashTimer -= Time;
            if (radarJammedTimer > 0.0f)   radarJammedTimer -= Time;

            // SPECIAL ABILITY TIMER For Shoab and Special Bullet
            if (!isPaused && Hero1Lives > 0)
            {
                ShoabSpecialTime += 1;
            }

            // Trigger Commanders when 65% of the alien swarm is eliminated
            int totalAliens = AlienInX * AlienInY;
            if (!commandersTriggered && !bossSpawned && AliensKilled >= (int)(totalAliens * 0.65f))
            {
                commandersTriggered  = true;
                commanderIntroActive = true;
                commanderIntroTimer  = 3.0f;
                PlaySound(sndEmpBlast);
            }

            // 3-Second Slowdown Countdown & Spawn Execution
            if (commanderIntroActive)
            {
                commanderIntroTimer -= rawTime;
                if (commanderIntroTimer <= 0.0f)
                {
                    commanderIntroActive = false;
                    gameTimeDilation = 1.0f;

                    // Initialize Null-Vector Jammer
                    jammerPos            = (Vector2){ WindowWidth * 0.25f - COMMANDER_SIZE / 2.0f, 130.0f };
                    jammerSpeed          = (Vector2){ 120.0f, 0.0f };
                    jammerHp             = jammerMaxHp;
                    jammerActive         = true;
                    jammerShootTimer     = 1.5f;
                    jammerActionCooldown = 5.0f;
                    jammerPowerTimer     = 0.0f;
                    jammerAnimState      = 0;

                    // Initialize Chrono-Phase Warp Commander
                    warpPos              = (Vector2){ WindowWidth * 0.75f - COMMANDER_SIZE / 2.0f, 130.0f };
                    warpSpeed            = (Vector2){ 140.0f, 0.0f };
                    warpHp               = warpMaxHp;
                    warpActive           = true;
                    warpShootTimer       = 2.0f;
                    warpActionCooldown   = 4.0f;
                    warpPowerTimer       = 0.0f;
                    warpAnimState        = 0;

                    PlaySound(sndTeleport);
                }
            }

            // COMMANDER 1: NULL-VECTOR JAMMER UPDATE (Movement, Shooting & Radar Jamming Power)
            if (jammerActive)
            {
                jammerPos.x += jammerSpeed.x * Time;
                if (jammerPos.x <= 40)
                {
                    jammerPos.x = 40;
                    jammerSpeed.x = fabsf(jammerSpeed.x);
                }
                else if (jammerPos.x >= WindowWidth - COMMANDER_SIZE - 40)
                {
                    jammerPos.x = WindowWidth - COMMANDER_SIZE - 40;
                    jammerSpeed.x = -fabsf(jammerSpeed.x);
                }

                // Jammer Shooting
                if (!deathSequenceActive)
                {
                    jammerShootTimer -= Time;
                    if (jammerShootTimer <= 0.0f)
                    {
                        jammerShootTimer = 1.8f;
                        for (int b = 0; b < MAX_COMMANDER_BULLETS; b++)
                        {
                            if (!jammerBulletActive[b])
                            {
                                jammerBulletActive[b] = true;
                                jammerBulletPos[b] = (Vector2){ jammerPos.x + COMMANDER_SIZE / 2.0f, jammerPos.y + COMMANDER_SIZE };
                                PlaySound(sndJammerShot);
                                break;
                            }
                        }
                    }

                    // Jammer Special Power: Radar Jamming EMP Field
                    jammerActionCooldown -= Time;
                    if (jammerActionCooldown <= 0.0f)
                    {
                        jammerActionCooldown = 8.5f;
                        jammerPowerTimer = 1.4f;
                        radarJammedTimer = 5.0f; // Blinds HUD telemetry for 5 seconds
                        PlaySound(sndEmpBlast);
                        PlaySound(sndJammerHum);
                    }
                }

                if (jammerPowerTimer > 0.0f)
                {
                    jammerPowerTimer -= Time;
                    jammerAnimState = 3; // Power deployment sprite
                }
                else
                {
                    jammerAnimState = (fabsf(jammerSpeed.x) > 0.0f) ? 2 : 0;
                }
            }

            // COMMANDER 2: CHRONO-PHASE WARP COMMANDER UPDATE (Movement, Shooting & Quantum Teleport Power)
            if (warpActive)
            {
                warpPos.x += warpSpeed.x * Time;
                if (warpPos.x <= 40)
                {
                    warpPos.x = 40;
                    warpSpeed.x = fabsf(warpSpeed.x);
                }
                else if (warpPos.x >= WindowWidth - COMMANDER_SIZE - 40)
                {
                    warpPos.x = WindowWidth - COMMANDER_SIZE - 40;
                    warpSpeed.x = -fabsf(warpSpeed.x);
                }

                // Warp Commander Shooting (Twin needle shots)
                if (!deathSequenceActive)
                {
                    warpShootTimer -= Time;
                    if (warpShootTimer <= 0.0f)
                    {
                        warpShootTimer = 1.4f;
                        int spawned = 0;
                        for (int b = 0; b < MAX_COMMANDER_BULLETS && spawned < 2; b++)
                        {
                            if (!warpBulletActive[b])
                            {
                                warpBulletActive[b] = true;
                                warpBulletPos[b] = (Vector2){ warpPos.x + (spawned == 0 ? 18.0f : COMMANDER_SIZE - 18.0f), warpPos.y + COMMANDER_SIZE };
                                spawned++;
                            }
                        }
                        PlaySound(sndWarpShot);
                    }

                    // Reactive Warp Evasion: If hero bullet gets within direct trajectory
                    bool threatened = false;
                    for (int h = 0; h < 2; h++)
                    {
                        Vector2 *bPos = (h == 0) ? Hero1BulletPos : Hero2BulletPos;
                        bool *bActive = (h == 0) ? Hero1BulletActive : Hero2BulletActive;
                        for (int i = 0; i < 2; i++)
                        {
                            if (bActive[i])
                            {
                                float dx = fabsf(bPos[i].x - (warpPos.x + COMMANDER_SIZE / 2.0f));
                                if (dx < 36.0f && bPos[i].y < warpPos.y + 220.0f && bPos[i].y > warpPos.y)
                                {
                                    threatened = true;
                                    break;
                                }
                            }
                        }
                        if (threatened) break;
                    }

                    // Warp Special Power: Spatial Teleport
                    warpActionCooldown -= Time;
                    if ((warpActionCooldown <= 0.0f) || (threatened && warpActionCooldown < 3.2f))
                    {
                        warpActionCooldown = 5.0f;
                        warpPowerTimer = 0.65f;
                        PlaySound(sndTeleport);

                        // Dimensional repositioning
                        float newX = (float)GetRandomValue(60, WindowWidth - COMMANDER_SIZE - 60);
                        float newY = (float)GetRandomValue(90, 200);
                        warpPos = (Vector2){ newX, newY };
                    }
                }

                if (warpPowerTimer > 0.0f)
                {
                    warpPowerTimer -= Time;
                    warpAnimState = 3; // Teleport phase animation sprite
                }
                else
                {
                    warpAnimState = (fabsf(warpSpeed.x) > 0.0f) ? 2 : 0;
                }
            }

            // UPDATE COMMANDER PROJECTILES & HERO COLLISION
            for (int b = 0; b < MAX_COMMANDER_BULLETS; b++)
            {
                // Jammer Bullets
                if (jammerBulletActive[b])
                {
                    jammerBulletPos[b].y += 420.0f * Time;
                    if (jammerBulletPos[b].y > WindowHeight)
                    {
                        jammerBulletActive[b] = false;
                    }
                    else if (!deathSequenceActive)
                    {
                        Rectangle jbRec = { jammerBulletPos[b].x - 4, jammerBulletPos[b].y, 8, 22 };
                        Rectangle h1Rec = { Hero1Pos.x - HeroWidth / 2.0f, Hero1Pos.y, HeroWidth, HeroHeight };
                        Rectangle h2Rec = { Hero2Pos.x - HeroWidth / 2.0f, Hero2Pos.y, HeroWidth, HeroHeight };

                        if (Hero1Lives > 0 && CheckCollisionRecs(jbRec, h1Rec))
                        {
                            jammerBulletActive[b] = false;
                            Hero1Lives--;
                            Hero1HitsTaken++;
                            hero1HitFlashTimer = 0.35f;
                            PlaySound(heroOuch);
                            if (Hero1Lives <= 0)
                            {
                                Hero1CrashPos = Hero1Pos;
                                PlaySound(heroDeath);
                                if (Hero2Lives <= 0) { deathSequenceActive = true; deathDelayTimer = 0.0f; }
                            }
                        }
                        else if (Hero2Lives > 0 && CheckCollisionRecs(jbRec, h2Rec))
                        {
                            jammerBulletActive[b] = false;
                            Hero2Lives--;
                            Hero2HitsTaken++;
                            hero2HitFlashTimer = 0.35f;
                            PlaySound(heroOuch);
                            if (Hero2Lives <= 0)
                            {
                                Hero2CrashPos = Hero2Pos;
                                PlaySound(heroDeath);
                                if (Hero1Lives <= 0) { deathSequenceActive = true; deathDelayTimer = 0.0f; }
                            }
                        }
                    }
                }

                // Warp Bullets
                if (warpBulletActive[b])
                {
                    warpBulletPos[b].y += 500.0f * Time;
                    if (warpBulletPos[b].y > WindowHeight)
                    {
                        warpBulletActive[b] = false;
                    }
                    else if (!deathSequenceActive)
                    {
                        Rectangle wbRec = { warpBulletPos[b].x - 3, warpBulletPos[b].y, 6, 26 };
                        Rectangle h1Rec = { Hero1Pos.x - HeroWidth / 2.0f, Hero1Pos.y, HeroWidth, HeroHeight };
                        Rectangle h2Rec = { Hero2Pos.x - HeroWidth / 2.0f, Hero2Pos.y, HeroWidth, HeroHeight };

                        if (Hero1Lives > 0 && CheckCollisionRecs(wbRec, h1Rec))
                        {
                            warpBulletActive[b] = false;
                            Hero1Lives--;
                            Hero1HitsTaken++;
                            hero1HitFlashTimer = 0.35f;
                            PlaySound(heroOuch);
                            if (Hero1Lives <= 0)
                            {
                                Hero1CrashPos = Hero1Pos;
                                PlaySound(heroDeath);
                                if (Hero2Lives <= 0) { deathSequenceActive = true; deathDelayTimer = 0.0f; }
                            }
                        }
                        else if (Hero2Lives > 0 && CheckCollisionRecs(wbRec, h2Rec))
                        {
                            warpBulletActive[b] = false;
                            Hero2Lives--;
                            Hero2HitsTaken++;
                            hero2HitFlashTimer = 0.35f;
                            PlaySound(heroOuch);
                            if (Hero2Lives <= 0)
                            {
                                Hero2CrashPos = Hero2Pos;
                                PlaySound(heroDeath);
                                if (Hero1Lives <= 0) { deathSequenceActive = true; deathDelayTimer = 0.0f; }
                            }
                        }
                    }
                }
            }

            // Hero death scream sequence
            if (deathSequenceActive)
            {
                deathDelayTimer += Time;
                if (deathDelayTimer >= 1.2f)
                {
                    deathSequenceActive = false;
                    GameOver = true;
                    bossLaserActive = false;
                    screenCamera.offset = (Vector2){ 0, 0 };
                    StopMusicStream(bgmBoss);
                    StopSound(bossLaserSound);
                    StopSound(sndDefibHum);
                    StopSound(sndWarningSiren);
                    PlaySound(gameOverCommunity);
                }
            }

            if (IsKeyPressed(KEY_ESCAPE) && !deathSequenceActive && !GameOver && !bossDefeated)
            {
                PlaySound(menuSelect);
                StopMusicStream(bgmBoss);
                StopSound(bossLaserSound);
                StopSound(sndDefibHum);
                StopSound(sndWarningSiren);
                CurrentState = STATE_MENU;
                PlayMusicStream(bgmMenu);
            }

            // EMERGENCY LIFE TRANSFER 
            // Channeling logic when one pilot is down
            if (Hero1Lives <= 0 && Hero2Lives > 1)
            {
                float distToCrash = Vector2Distance(Hero2Pos, Hero1CrashPos);
                if (distToCrash < 85.0f)
                {
                    if (!IsSoundPlaying(sndDefibHum)) PlaySound(sndDefibHum);
                    hero1ReviveTimer += Time;
                    if (hero1ReviveTimer >= 1.5f)
                    {
                        StopSound(sndDefibHum);
                        Hero2Lives--;
                        Hero1Lives = 1;
                        hero1ReviveTimer = 0.0f;
                        Hero1Pos = Hero1CrashPos;
                        PlaySound(cheer);
                    }
                }
                else
                {
                    if (hero1ReviveTimer > 0.0f) StopSound(sndDefibHum);
                    hero1ReviveTimer = 0.0f;
                }
            }
            else
            {
                if (hero1ReviveTimer > 0.0f) StopSound(sndDefibHum);
                hero1ReviveTimer = 0.0f;
            }

            if (Hero2Lives <= 0 && Hero1Lives > 1)
            {
                float distToCrash = Vector2Distance(Hero1Pos, Hero2CrashPos);
                if (distToCrash < 85.0f)
                {
                    if (!IsSoundPlaying(sndDefibHum)) PlaySound(sndDefibHum);
                    hero2ReviveTimer += Time;
                    if (hero2ReviveTimer >= 2.0f)
                    {
                        StopSound(sndDefibHum);
                        Hero1Lives--;
                        Hero2Lives = 1;
                        hero2ReviveTimer = 0.0f;
                        Hero2Pos = Hero2CrashPos;
                        PlaySound(cheer);
                    }
                }
                else
                {
                    if (hero2ReviveTimer > 0.0f) StopSound(sndDefibHum);
                    hero2ReviveTimer = 0.0f;
                }
            }
            else
            {
                if (hero2ReviveTimer > 0.0f) StopSound(sndDefibHum);
                hero2ReviveTimer = 0.0f;
            }

            // GAME LOST: BACKGROUND MUSIC, ALIEN BOSS DIALOGUES and WORLD INVASION NEWS
            if (GameOver)
            {
                bossLaserActive = false;
                screenCamera.offset = (Vector2){ 0, 0 };
                StopMusicStream(bgmBoss);
                StopSound(bossLaserSound);
                StopSound(sndDefibHum);
                StopSound(sndWarningSiren);

                if (!lostBgmStarted)
                {
                    PlayMusicStream(bgmLost);
                    lostBgmStarted = true;
                }
                UpdateMusicStream(bgmLost);

                int panelW = 1200;
                int panelH = 540;
                int panelX = (WindowWidth - panelW) / 2;
                int panelY = (WindowHeight - panelH) / 2;

                DrawRectangle(panelX, panelY, panelW, panelH, Fade(BLACK, 0.90f));
                DrawRectangleLines(panelX, panelY, panelW, panelH, RED);
                DrawRectangleLines(panelX + 4, panelY + 4, panelW - 8, panelH - 8, MAROON);

                if (lostDialogueIndex == 0)
                {
                    DrawText("[ INCOMING ENCRYPTED TRANSMISSION - ALIEN DREADNOUGHT OVERLORD ]", panelX + 50, panelY + 35, 23, RED);
                    DrawLine(panelX + 50, panelY + 70, panelX + panelW - 50, panelY + 70, RED);

                    DrawText("Alien Dreadnought Boss:", panelX + 50, panelY + 105, 26, MAROON);
                    DrawText("\"HA HA HA! Look at your pathetic interceptors burning in orbital debris!\"", panelX + 50, panelY + 155, 23, RED);
                    DrawText("\"Both of your 'heroes'—Nayemul and Shoab—have been obliterated!\"", panelX + 50, panelY + 205, 23, RED);
                    DrawText("\"Now that your frontline is wiped out, we will capture and conquer your world easily!\"", panelX + 50, panelY + 255, 23, ORANGE);
                    DrawText("\"Humanity will bow to our supreme dominion. Earth belongs to us now!\"", panelX + 50, panelY + 305, 23, RED);
                }
                else if (lostDialogueIndex == 1)
                {
                    DrawText("[ GLOBAL EMERGENCY BROADCAST - ALL DEFENSE CHANNELS OVERRUN ]", panelX + 50, panelY + 35, 23, RED);
                    DrawLine(panelX + 50, panelY + 70, panelX + panelW - 50, panelY + 70, RED);

                    DrawText("INTERNATIONAL NEWS BULLETIN: PLANET EARTH HAS BEEN INVADED BY ALIENS!", panelX + 50, panelY + 105, 24, RED);
                    DrawText("News channels worldwide confirm the complete downfall of all orbital defenses.", panelX + 50, panelY + 160, 22, RAYWHITE);
                    DrawText("Extraterrestrial armadas have entered the atmosphere over major capital cities.", panelX + 50, panelY + 205, 22, LIGHTGRAY);
                    DrawText("The loss of Shoab and Nayemul has triggered an immediate global state of emergency.", panelX + 50, panelY + 250, 22, LIGHTGRAY);
                    DrawText("Planetary surrender protocols have been initiated as darkness consumes the Earth...", panelX + 50, panelY + 295, 22, LIGHTGRAY);
                }
                else
                {
                    char titleText[] = "GAME OVER - EARTH HAS FALLEN";
                    int tW = MeasureText(titleText, 42);
                    DrawText(titleText, (WindowWidth - tW) / 2, panelY + 45, 42, RED);
                    DrawLine(panelX + 80, panelY + 105, panelX + panelW - 80, panelY + 105, RED);

                    char subText[] = "Both heroes fell in battle. The alien fleet has conquered our world!";
                    int subW = MeasureText(subText, 24);
                    DrawText(subText, (WindowWidth - subW) / 2, panelY + 145, 24, WHITE);

                    DrawText(TextFormat("Shoab's Final Score: %05d", Hero1Score), panelX + 180, panelY + 220, 26, LIME);
                    DrawText(TextFormat("Nayemul's Final Score: %05d", Hero2Score), panelX + 680, panelY + 220, 26, YELLOW);

                    char restartText[] = "Press [R] to Restart Defense   |   [ESC] Main Menu";
                    int rstW = MeasureText(restartText, 22);
                    DrawText(restartText, (WindowWidth - rstW) / 2, panelY + 360, 22, GOLD);
                }

                if (lostDialogueIndex < 2)
                {
                    char promptText[] = "PRESS [ENTER] OR [SPACE] TO ADVANCE TRANSMISSION";
                    int prW = MeasureText(promptText, 18);
                    if (((int)(GetTime() * 3)) % 2 == 0)
                    {
                        DrawText(promptText, (WindowWidth - prW) / 2, panelY + panelH - 45, 18, RED);
                    }

                    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE))
                    {
                        PlaySound(menuSelect);
                        lostDialogueIndex++;
                    }
                }

                if (IsKeyPressed(KEY_R))
                {
                    PlaySound(menuSelect);
                    StopMusicStream(bgmLost);
                    lostBgmStarted = false;
                    lostDialogueIndex = 0;

                    Hero1Lives = 4;
                    Hero2Lives = 4;
                    Hero1Score = 0;
                    Hero2Score = 0;
                    AliensKilled = 0;
                    GameOver = false;
                    deathSequenceActive = false;
                    deathDelayTimer = 0.0f;
                    cheerPlayed = false;
                    winSoundPlayed = false;

                    Hero1Kills = 0; Hero2Kills = 0;
                    Hero1BossDamage = 0; Hero2BossDamage = 0;
                    Hero1HitsTaken = 0; Hero2HitsTaken = 0;

                    bossSpawned = false;
                    bossWarningActive = false;
                    bossWarningTimer = 0.0f;
                    bossActive = false;
                    bossDefeated = false;
                    bossHp = 100;
                    bossLeftPodHp = BossPodMaxHp;
                    bossRightPodHp = BossPodMaxHp;
                    bossPos = (Vector2){ (WindowWidth - BossWidth) / 2.0f, 60.0f };
                    bossShootTimer = 0.0f;
                    bossLaserTimer = 0.0f;
                    bossLaserActive = false;
                    bossLaserDuration = 0.0f;
                    bossOrbTimer = 0.0f;

                    hero1Debuffed = false;
                    hero1DebuffTimer = 0.0f;
                    hero2Debuffed = false;
                    hero2DebuffTimer = 0.0f;

                    empDropped = false;
                    empActive = false;
                    empBuffTimer = 0.0f;
                    empShockwaveRadius = 0.0f;
                    hero1HitFlashTimer = 0.0f;
                    hero2HitFlashTimer = 0.0f;

                    SpecialReady = false;
                    starttimer = false;
                    ShoabSpecialTime = 0.0f;
                    Hero1SpecialBulletActive = false;

                    commandersTriggered  = false;
                    commanderIntroActive = false;
                    commanderIntroTimer  = 0.0f;
                    gameTimeDilation     = 1.0f;
                    jammerActive         = false;
                    warpActive           = false;
                    jammerHp             = 0;
                    warpHp               = 0;
                    radarJammedTimer     = 0.0f;
                    jammerPowerTimer     = 0.0f;
                    warpPowerTimer       = 0.0f;
                    bossEnraged          = false;

                    for (int b = 0; b < MAX_COMMANDER_BULLETS; b++)
                    {
                        jammerBulletActive[b] = false;
                        warpBulletActive[b] = false;
                    }

                    minionDeployTimer = 0.0f;
                    for (int m = 0; m < MAX_MINIONS; m++) minionActive[m] = false;
                    for (int mb = 0; mb < MAX_MINION_BULLETS; mb++) minionBulletActive[mb] = false;

                    for (int k = 0; k < MAX_BOSS_BULLETS; k++) bossBulletActive[k] = false;
                    for (int k = 0; k < MAX_BOSS_ORBS; k++) bossOrbActive[k] = false;

                    Hero1Pos = (Vector2){ WindowWidth * 0.35f, WindowHeight - HeroHeight };
                    Hero2Pos = (Vector2){ WindowWidth * 0.65f, WindowHeight - HeroHeight };
                    Hero1Speed = (Vector2){ 0, 0 };
                    Hero2Speed = (Vector2){ 0, 0 };
                    AlienSpeed = (Vector2){ AlienSpeedX, AlienSpeedY };

                    Hero1BulletActive[0] = false; Hero1BulletActive[1] = false;
                    Hero2BulletActive[0] = false; Hero2BulletActive[1] = false;
                    AlienBulletActive = false;
                    AlienShootTimer = 0.0f;

                    AlienPos[0][0] = (Vector2){ 150, 50 };
                    for (int X = 0; X < AlienInX; X++)
                    {
                        for (int Y = 0; Y < AlienInY; Y++)
                        {
                            AlienPos[X][Y].x = AlienPos[0][0].x + (AlienSize + AlienDistance) * X;
                            AlienPos[X][Y].y = AlienPos[0][0].y + (AlienSize + AlienDistance) * Y;
                            AlienAlive[X][Y] = true;
                            SpeedBuff[Y] = GetRandomValue(0, 20);
                        }
                    }
                }

                if (IsKeyPressed(KEY_ESCAPE))
                {
                    PlaySound(menuSelect);
                    StopMusicStream(bgmLost);
                    lostBgmStarted = false;
                    lostDialogueIndex = 0;
                    CurrentState = STATE_MENU;
                    PlayMusicStream(bgmMenu);
                }

                EndMode2D();
                EndDrawing();
                continue;
            }

            // GAME WON: BACKGROUND MUSIC, VICTORY DIALOGUES and WORLDWIDE NEWS
            if (bossDefeated)
            {
                bossLaserActive = false;
                screenCamera.offset = (Vector2){ 0, 0 };
                StopMusicStream(bgmBoss);
                StopSound(bossLaserSound);
                StopSound(sndDefibHum);
                StopSound(sndWarningSiren);

                if (!winSoundPlayed)
                {
                    PlaySound(gameWin);
                    winSoundPlayed = true;
                }

                if (!winBgmStarted)
                {
                    PlayMusicStream(bgmWin);
                    winBgmStarted = true;
                }
                UpdateMusicStream(bgmWin);

                int panelW = 1260;
                int panelH = 580;
                int panelX = (WindowWidth - panelW) / 2;
                int panelY = (WindowHeight - panelH) / 2;

                DrawRectangle(panelX, panelY, panelW, panelH, Fade(BLACK, 0.90f));
                DrawRectangleLines(panelX, panelY, panelW, panelH, GREEN);
                DrawRectangleLines(panelX + 4, panelY + 4, panelW - 8, panelH - 8, DARKBLUE);

                if (winDialogueIndex == 0)
                {
                    DrawText("[ EARTH ALLIANCE COCKPIT COMMS - VICTORY CONFIRMED ]", panelX + 50, panelY + 35, 23, GOLD);
                    DrawLine(panelX + 50, panelY + 70, panelX + panelW - 50, panelY + 70, SKYBLUE);

                    DrawText("Shoab: \"We did it!!! Nayemul, look! Their dreadnought carrier is collapsing!\"", panelX + 50, panelY + 110, 24, LIME);
                    DrawText("Nayemul: \"We did it!!! We saved the world! The skies are finally clear!\"", panelX + 50, panelY + 165, 24, YELLOW);
                    DrawText("Shoab: \"All alien fighter remnants are scattering in total retreat from our orbit!\"", panelX + 50, panelY + 220, 22, LIME);
                    DrawText("Nayemul: \"Sub-space thrusters engaged. Let's return home heroes! Humanity survives!\"", panelX + 50, panelY + 275, 22, YELLOW);
                }
                else if (winDialogueIndex == 1)
                {
                    DrawText("[ GLOBAL BROADCAST NETWORK - INTERNATIONAL NEWS SPECIAL ]", panelX + 50, panelY + 35, 23, GOLD);
                    DrawLine(panelX + 50, panelY + 70, panelX + panelW - 50, panelY + 70, SKYBLUE);

                    DrawText("INTERNATIONAL NEWS HEADLINE: THE ALIEN ARMADA HAS BEEN DECIMATED!", panelX + 50, panelY + 105, 24, GREEN);
                    DrawText("Millions pour into the streets celebrating across Dhaka, Tokyo, London, and New York!", panelX + 50, panelY + 160, 22, RAYWHITE);
                    DrawText("United Nations Command confirms all invasion sectors have been permanently liberated.", panelX + 50, panelY + 205, 22, LIGHTGRAY);
                    DrawText("Nayemul and Shoab are hailed as international heroes who saved mankind from extinction!", panelX + 50, panelY + 250, 22, SKYBLUE);
                    DrawText("Planetary fireworks light up night skies worldwide in honor of the Earth Defense Fleet!", panelX + 50, panelY + 295, 22, YELLOW);
                }
                else
                {
                    char titleText[] = "VICTORY! MISSION ACCOMPLISHED!";
                    int tW = MeasureText(titleText, 38);
                    DrawText(titleText, (WindowWidth - tW) / 2, panelY + 30, 38, GREEN);
                    DrawLine(panelX + 80, panelY + 75, panelX + panelW - 80, panelY + 75, SKYBLUE);

                    DrawText(TextFormat("Shoab's Final Score: %05d", Hero1Score), panelX + 160, panelY + 95, 24, LIME);
                    DrawText(TextFormat("Nayemul's Final Score: %05d", Hero2Score), panelX + 740, panelY + 95, 24, YELLOW);

                    DrawText("[ COMBAT PERFORMANCE BADGES ]", (WindowWidth - MeasureText("[ COMBAT PERFORMANCE BADGES ]", 20)) / 2, panelY + 145, 20, GOLD);

                    int cardW = 340;
                    int cardH = 150;
                    int cardY = panelY + 185;

                    // Award 1: Top Gun
                    int card1X = panelX + 60;
                    DrawRectangle(card1X, cardY, cardW, cardH, Fade(DARKBLUE, 0.45f));
                    DrawRectangleLines(card1X, cardY, cardW, cardH, SKYBLUE);
                    DrawText("★ TOP GUN ★", card1X + 20, cardY + 18, 22, GOLD);
                    DrawText("Most Alien Kills", card1X + 20, cardY + 50, 16, LIGHTGRAY);
                    if (Hero1Kills > Hero2Kills) {
                        DrawText(TextFormat("WINNER: SHOAB (%d)", Hero1Kills), card1X + 20, cardY + 85, 20, LIME);
                        DrawText(TextFormat("Runner Up: Nayemul (%d)", Hero2Kills), card1X + 20, cardY + 115, 14, GRAY);
                    } else if (Hero2Kills > Hero1Kills) {
                        DrawText(TextFormat("WINNER: NAYEMUL (%d)", Hero2Kills), card1X + 20, cardY + 85, 20, YELLOW);
                        DrawText(TextFormat("Runner Up: Shoab (%d)", Hero1Kills), card1X + 20, cardY + 115, 14, GRAY);
                    } else {
                        DrawText(TextFormat("TIED: BOTH (%d Kills)", Hero1Kills), card1X + 20, cardY + 95, 20, WHITE);
                    }

                    // Award 2: Dreadnought Breaker
                    int card2X = panelX + 460;
                    DrawRectangle(card2X, cardY, cardW, cardH, Fade(DARKPURPLE, 0.45f));
                    DrawRectangleLines(card2X, cardY, cardW, cardH, RED);
                    DrawText("☠ DREADNOUGHT BREAKER ☠", card2X + 15, cardY + 18, 20, RED);
                    DrawText("Most Boss Damage Dealt", card2X + 20, cardY + 50, 16, LIGHTGRAY);
                    if (Hero1BossDamage > Hero2BossDamage) {
                        DrawText(TextFormat("WINNER: SHOAB (%d HP)", Hero1BossDamage), card2X + 20, cardY + 85, 20, LIME);
                        DrawText(TextFormat("Runner Up: Nayemul (%d HP)", Hero2BossDamage), card2X + 20, cardY + 115, 14, GRAY);
                    } else if (Hero2BossDamage > Hero1BossDamage) {
                        DrawText(TextFormat("WINNER: NAYEMUL (%d HP)", Hero2BossDamage), card2X + 20, cardY + 85, 20, YELLOW);
                        DrawText(TextFormat("Runner Up: Shoab (%d HP)", Hero1BossDamage), card2X + 20, cardY + 115, 14, GRAY);
                    } else {
                        DrawText(TextFormat("TIED: BOTH (%d HP)", Hero1BossDamage), card2X + 20, cardY + 95, 20, WHITE);
                    }

                    // Award 3: Untouchable Ace
                    int card3X = panelX + 860;
                    DrawRectangle(card3X, cardY, cardW, cardH, Fade(DARKGREEN, 0.45f));
                    DrawRectangleLines(card3X, cardY, cardW, cardH, LIME);
                    DrawText("🛡 UNTOUCHABLE ACE 🛡", card3X + 20, cardY + 18, 20, GREEN);
                    DrawText("Fewest Damage Hits Taken", card3X + 20, cardY + 50, 16, LIGHTGRAY);
                    if (Hero1HitsTaken < Hero2HitsTaken) {
                        DrawText(TextFormat("WINNER: SHOAB (%d Hits)", Hero1HitsTaken), card3X + 20, cardY + 85, 20, LIME);
                        DrawText(TextFormat("Nayemul: %d Hits Taken", Hero2HitsTaken), card3X + 20, cardY + 115, 14, GRAY);
                    } else if (Hero2HitsTaken < Hero1HitsTaken) {
                        DrawText(TextFormat("WINNER: NAYEMUL (%d Hits)", Hero2HitsTaken), card3X + 20, cardY + 85, 20, YELLOW);
                        DrawText(TextFormat("Shoab: %d Hits Taken", Hero1HitsTaken), card3X + 20, cardY + 115, 14, GRAY);
                    } else {
                        DrawText(TextFormat("TIED: BOTH (%d Hits)", Hero1HitsTaken), card3X + 20, cardY + 95, 20, WHITE);
                    }

                    char restartText[] = "Press [R] to Play Again   |   [ESC] Main Menu";
                    int rstW = MeasureText(restartText, 22);
                    DrawText(restartText, (WindowWidth - rstW) / 2, panelY + panelH - 45, 22, GOLD);
                }

                if (winDialogueIndex < 2)
                {
                    char promptText[] = "PRESS [ENTER] OR [SPACE] TO ADVANCE BROADCAST";
                    int prW = MeasureText(promptText, 18);
                    if (((int)(GetTime() * 3)) % 2 == 0)
                    {
                        DrawText(promptText, (WindowWidth - prW) / 2, panelY + panelH - 45, 18, GREEN);
                    }

                    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE))
                    {
                        PlaySound(menuSelect);
                        winDialogueIndex++;
                    }
                }

                if (IsKeyPressed(KEY_R))
                {
                    PlaySound(menuSelect);
                    StopMusicStream(bgmWin);
                    winBgmStarted = false;
                    winDialogueIndex = 0;

                    AliensKilled = 0;
                    Hero1Lives = 4;
                    Hero2Lives = 4;
                    Hero1Score = 0;
                    Hero2Score = 0;
                    cheerPlayed = false;
                    winSoundPlayed = false;

                    Hero1Kills = 0; Hero2Kills = 0;
                    Hero1BossDamage = 0; Hero2BossDamage = 0;
                    Hero1HitsTaken = 0; Hero2HitsTaken = 0;

                    bossSpawned = false;
                    bossWarningActive = false;
                    bossWarningTimer = 0.0f;
                    bossActive = false;
                    bossDefeated = false;
                    bossHp = 100;
                    bossLeftPodHp = BossPodMaxHp;
                    bossRightPodHp = BossPodMaxHp;
                    bossPos = (Vector2){ (WindowWidth - BossWidth) / 2.0f, 60.0f };
                    bossShootTimer = 0.0f;
                    bossLaserTimer = 0.0f;
                    bossLaserActive = false;
                    bossLaserDuration = 0.0f;
                    bossOrbTimer = 0.0f;

                    hero1Debuffed = false;
                    hero1DebuffTimer = 0.0f;
                    hero2Debuffed = false;
                    hero2DebuffTimer = 0.0f;

                    empDropped = false;
                    empActive = false;
                    empBuffTimer = 0.0f;
                    empShockwaveRadius = 0.0f;
                    hero1HitFlashTimer = 0.0f;
                    hero2HitFlashTimer = 0.0f;

                    SpecialReady = false;
                    starttimer = false;
                    ShoabSpecialTime = 0.0f;
                    Hero1SpecialBulletActive = false;

                    commandersTriggered  = false;
                    commanderIntroActive = false;
                    commanderIntroTimer  = 0.0f;
                    gameTimeDilation     = 1.0f;
                    jammerActive         = false;
                    warpActive           = false;
                    jammerHp             = 0;
                    warpHp               = 0;
                    radarJammedTimer     = 0.0f;
                    jammerPowerTimer     = 0.0f;
                    warpPowerTimer       = 0.0f;
                    bossEnraged          = false;

                    for (int b = 0; b < MAX_COMMANDER_BULLETS; b++)
                    {
                        jammerBulletActive[b] = false;
                        warpBulletActive[b] = false;
                    }

                    minionDeployTimer = 0.0f;
                    for (int m = 0; m < MAX_MINIONS; m++) minionActive[m] = false;
                    for (int mb = 0; mb < MAX_MINION_BULLETS; mb++) minionBulletActive[mb] = false;

                    for (int k = 0; k < MAX_BOSS_BULLETS; k++) bossBulletActive[k] = false;
                    for (int k = 0; k < MAX_BOSS_ORBS; k++) bossOrbActive[k] = false;

                    Hero1Pos = (Vector2){ WindowWidth * 0.35f, WindowHeight - HeroHeight };
                    Hero2Pos = (Vector2){ WindowWidth * 0.65f, WindowHeight - HeroHeight };
                    Hero1Speed = (Vector2){ 0, 0 };
                    Hero2Speed = (Vector2){ 0, 0 };
                    AlienSpeed = (Vector2){ AlienSpeedX, AlienSpeedY };

                    Hero1BulletActive[0] = false; Hero1BulletActive[1] = false;
                    Hero2BulletActive[0] = false; Hero2BulletActive[1] = false;
                    AlienBulletActive = false;
                    AlienShootTimer = 0.0f;

                    AlienPos[0][0] = (Vector2){ 150, 50 };
                    for (int X = 0; X < AlienInX; X++)
                    {
                        for (int Y = 0; Y < AlienInY; Y++)
                        {
                            AlienPos[X][Y].x = AlienPos[0][0].x + (AlienSize + AlienDistance) * X;
                            AlienPos[X][Y].y = AlienPos[0][0].y + (AlienSize + AlienDistance) * Y;
                            AlienAlive[X][Y] = true;
                            SpeedBuff[Y] = GetRandomValue(0, 20);
                        }
                    }
                }

                if (IsKeyPressed(KEY_ESCAPE))
                {
                    PlaySound(menuSelect);
                    StopMusicStream(bgmWin);
                    winBgmStarted = false;
                    winDialogueIndex = 0;
                    CurrentState = STATE_MENU;
                    PlayMusicStream(bgmMenu);
                }

                EndMode2D();
                EndDrawing();
                continue;
            }

            // Boss Trigger check: Boss will only spawn if all aliens are dead AND both commanders are eliminated AND at least one hero is alive
            if (AliensKilled == AlienInX * AlienInY && !bossSpawned && !jammerActive && !warpActive && !commanderIntroActive)
            {
                if ((Hero1Lives > 0 || Hero2Lives > 0) && !GameOver && !deathSequenceActive)
                {
                    bossSpawned = true;
                    bossWarningActive = true;
                    bossWarningTimer = 0.0f;
                    PlayMusicStream(bgmBoss);
                    PlaySound(sndWarningSiren);

                    // Add 2 extra lives to both heroes (Revives fallen teammate if one survived)
                    if (Hero1Lives <= 0)
                    {
                        Hero1Lives = 2;
                        Hero1Pos = (Hero1CrashPos.x != 0) ? Hero1CrashPos : (Vector2){ WindowWidth * 0.35f, WindowHeight - HeroHeight };
                        hero1ReviveTimer = 0.0f;
                    }
                    else
                    {
                        Hero1Lives += 2;
                    }

                    if (Hero2Lives <= 0)
                    {
                        Hero2Lives = 2;
                        Hero2Pos = (Hero2CrashPos.x != 0) ? Hero2CrashPos : (Vector2){ WindowWidth * 0.65f, WindowHeight - HeroHeight };
                        hero2ReviveTimer = 0.0f;
                    }
                    else
                    {
                        Hero2Lives += 2;
                    }

                    PlaySound(cheer);
                }
            }

            // Boss Warning Screen (3 seconds)
            if (bossWarningActive)
            {
                bossWarningTimer += Time;
                if (((int)(GetTime() * 5)) % 2 == 0)
                {
                    char warnText[] = "WARNING!! that boss has arrived";
                    int wWidth = MeasureText(warnText, 48);
                    DrawText(warnText, (WindowWidth - wWidth) / 2, WindowHeight / 2 - 30, 48, RED);
                }

                // Visual text notification shown directly above the heroes
                const char* extraLifeTxt = "2 extra lives have been added";
                int elw = MeasureText(extraLifeTxt, 18);
                if (Hero1Lives > 0)
                {
                    DrawText(extraLifeTxt, (int)Hero1Pos.x - elw / 2, (int)Hero1Pos.y - 48, 18, LIME);
                }
                if (Hero2Lives > 0)
                {
                    float yOffset = (fabsf(Hero1Pos.x - Hero2Pos.x) < elw) ? 72.0f : 48.0f;
                    DrawText(extraLifeTxt, (int)Hero2Pos.x - elw / 2, (int)Hero2Pos.y - (int)yOffset, 18, YELLOW);
                }

                if (bossWarningTimer >= 3.0f)
                {
                    StopSound(sndWarningSiren);
                    bossWarningActive = false;
                    bossActive = true;
                    bossHp = 100;
                    bossLeftPodHp = BossPodMaxHp;
                    bossRightPodHp = BossPodMaxHp;
                    bossPos = (Vector2){ (WindowWidth - BossWidth) / 2.0f, 60.0f };
                    bossLaserTimer = 0.0f;
                    bossOrbTimer = 0.0f;
                    minionDeployTimer = 0.0f;
                }
            }

            // Hero Debuffs
            float curSpeed1 = HeroSpeedX;
            if (hero1Debuffed)
            {
                hero1DebuffTimer -= Time;
                if (hero1DebuffTimer <= 0.0f) hero1Debuffed = false;
                curSpeed1 = HeroSpeedX * 0.30f;
            }

            float curSpeed2 = HeroSpeedX;
            if (hero2Debuffed)
            {
                hero2DebuffTimer -= Time;
                if (hero2DebuffTimer <= 0.0f) hero2Debuffed = false;
                curSpeed2 = HeroSpeedX * 0.30f;
            }

            // HERO CONTROLS and MOVEMENTS
            if (!deathSequenceActive)
            {
                if (Hero1Lives > 0)
                {
                    if (IsKeyDown(KEY_D)) Hero1Speed.x = curSpeed1;
                    else if (IsKeyDown(KEY_A)) Hero1Speed.x = -curSpeed1;
                    else Hero1Speed.x = 0;
                }
                else Hero1Speed.x = 0;

                if (Hero2Lives > 0)
                {
                    if (IsKeyDown(KEY_RIGHT)) Hero2Speed.x = curSpeed2;
                    else if (IsKeyDown(KEY_LEFT)) Hero2Speed.x = -curSpeed2;
                    else Hero2Speed.x = 0;
                }
                else Hero2Speed.x = 0;
            }
            else
            {
                Hero1Speed.x = 0;
                Hero2Speed.x = 0;
            }

            // Update Positions
            if (Hero1Lives > 0) Hero1Pos = Vector2Add(Hero1Pos, Vector2Scale(Hero1Speed, Time));
            if (Hero2Lives > 0) Hero2Pos = Vector2Add(Hero2Pos, Vector2Scale(Hero2Speed, Time));

            // Boundary crossing
            float minX = HeroWidth / 2.0f;
            float maxX = WindowWidth - HeroWidth / 2.0f;
            if (Hero1Pos.x < minX) Hero1Pos.x = minX;
            if (Hero1Pos.x > maxX) Hero1Pos.x = maxX;
            if (Hero2Pos.x < minX) Hero2Pos.x = minX;
            if (Hero2Pos.x > maxX) Hero2Pos.x = maxX;

            // EMP Buff Rapid-Fire calculation
            if (empBuffTimer > 0.0f) empBuffTimer -= Time;

            // SHOOTING: Shoab (W or Space)
            bool canShoot1 = !hero1Debuffed && (Hero1Lives > 0);
            if (empBuffTimer <= 0.0f)
            {
                for (int i = 0; i < 2; i++)
                {
                    if (Hero1BulletActive[i] && Hero1BulletPos[i].y > WindowHeight / 2.0f)
                    {
                        canShoot1 = false;
                        break;
                    }
                }
            }

            if ((IsKeyPressed(KEY_W) || IsKeyPressed(KEY_SPACE)) && canShoot1 && !deathSequenceActive && !bossWarningActive)
            {
                PlaySound(shoot);
                for (int i = 0; i < 2; i++)
                {
                    if (!Hero1BulletActive[i])
                    {
                        Hero1BulletActive[i] = true;
                        Hero1BulletPos[i] = (Vector2){ Hero1Pos.x, Hero1Pos.y };
                        break;
                    }
                }
            }

            // SHOOTING: Nayemul (UP Arrow)
            bool canShoot2 = !hero2Debuffed && (Hero2Lives > 0);
            if (empBuffTimer <= 0.0f)
            {
                for (int i = 0; i < 2; i++)
                {
                    if (Hero2BulletActive[i] && Hero2BulletPos[i].y > WindowHeight / 2.0f)
                    {
                        canShoot2 = false;
                        break;
                    }
                }
            }

            if (IsKeyPressed(KEY_UP) && canShoot2 && !deathSequenceActive && !bossWarningActive)
            {
                PlaySound(shoot);
                for (int i = 0; i < 2; i++)
                {
                    if (!Hero2BulletActive[i])
                    {
                        Hero2BulletActive[i] = true;
                        Hero2BulletPos[i] = (Vector2){ Hero2Pos.x, Hero2Pos.y };
                        break;
                    }
                }
            }

            // SHOAB SPECIAL BULLET FIRING INPUT
            if (SpecialReady && IsKeyPressed(KEY_Q) && Hero1Lives > 0 && !deathSequenceActive && !bossWarningActive)
            {
                Hero1SpecialBulletPos = (Vector2){ Hero1Pos.x, Hero1Pos.y };
                Hero1SpecialBulletActive = true;
                ShoabSpecialTime = 0;
                SpecialReady = false;
                PlaySound(sndSpecialBeam);
            }

            // Tactical Feature 2: Orbital EMP Drop Trigger & Update
            if (bossActive && !empDropped && bossLeftPodHp <= 0 && bossRightPodHp <= 0)
            {
                empDropped = true;
                empActive = true;
                empPos = (Vector2){ (float)GetRandomValue(350, WindowWidth - 350), -40.0f };
            }

            if (empActive)
            {
                empPos.y += 110.0f * Time;
                if (empPos.y > WindowHeight + 40) empActive = false;

                Rectangle empRec = { empPos.x - 22, empPos.y - 22, 44, 44 };
                Rectangle h1Rec = { Hero1Pos.x - HeroWidth / 2.0f, Hero1Pos.y, HeroWidth, HeroHeight };
                Rectangle h2Rec = { Hero2Pos.x - HeroWidth / 2.0f, Hero2Pos.y, HeroWidth, HeroHeight };

                if ((Hero1Lives > 0 && CheckCollisionRecs(empRec, h1Rec)) ||
                    (Hero2Lives > 0 && CheckCollisionRecs(empRec, h2Rec)))
                {
                    empActive = false;
                    empBuffTimer = 8.5f;
                    empShockwaveRadius = 10.0f;
                    empShockwaveCenter = empPos;
                    PlaySound(sndEmpBlast);

                    AlienBulletActive = false;
                    for (int mb = 0; mb < MAX_MINION_BULLETS; mb++) minionBulletActive[mb] = false;
                    for (int k = 0; k < MAX_BOSS_BULLETS; k++) bossBulletActive[k] = false;
                    for (int k = 0; k < MAX_BOSS_ORBS; k++) bossOrbActive[k] = false;
                }
            }

            // UPDATE HERO BULLETS, HIGH-DURABILITY PODS, MINIONS & INDIVIDUAL SCORING
            for (int h = 0; h < 2; h++)
            {
                Vector2 *bPos = (h == 0) ? Hero1BulletPos : Hero2BulletPos;
                bool *bActive = (h == 0) ? Hero1BulletActive : Hero2BulletActive;
                int *hScore = (h == 0) ? &Hero1Score : &Hero2Score;
                int *hKills = (h == 0) ? &Hero1Kills : &Hero2Kills;
                int *hBossDmg = (h == 0) ? &Hero1BossDamage : &Hero2BossDamage;

                for (int i = 0; i < 2; i++)
                {
                    if (bActive[i])
                    {
                        float currentBulletSpeed = (empBuffTimer > 0.0f) ? BulletSpeedY * 1.35f : BulletSpeedY;
                        bPos[i].y -= currentBulletSpeed * Time;
                        if (bPos[i].y < -BulletHeight)
                        {
                            bActive[i] = false;
                            continue;
                        }

                        Rectangle bRec = { bPos[i].x - BulletWidth / 2.0f, bPos[i].y, BulletWidth, BulletHeight };

                        // Regular alien collision
                        if (!bossActive && !bossSpawned)
                        {
                            for (int X = 0; X < AlienInX; X++)
                            {
                                for (int Y = 0; Y < AlienInY; Y++)
                                {
                                    if (AlienAlive[X][Y])
                                    {
                                        Rectangle aRec = { AlienPos[X][Y].x, AlienPos[X][Y].y, AlienSize, AlienSize };
                                        if (CheckCollisionRecs(bRec, aRec))
                                        {
                                            PlaySound(damage);
                                            AlienAlive[X][Y] = false;
                                            bActive[i] = false;
                                            AliensKilled++;
                                            *hScore += 100;
                                            *hKills += 1;

                                            if (!cheerPlayed && AliensKilled >= (int)(AlienInX * AlienInY * 0.70f) && (Hero1Lives + Hero2Lives >= 3))
                                            {
                                                PlaySound(cheer);
                                                cheerPlayed = true;
                                            }
                                            break;
                                        }
                                    }
                                }
                                if (!bActive[i]) break;
                            }
                        }

                        // Collision with Jammer Commander
                        if (jammerActive && bActive[i])
                        {
                            Rectangle jRec = { jammerPos.x, jammerPos.y, COMMANDER_SIZE, COMMANDER_SIZE };
                            if (CheckCollisionRecs(bRec, jRec))
                            {
                                PlaySound(damage);
                                bActive[i] = false;
                                jammerHp--;
                                *hScore += 50;
                                if (jammerHp <= 0)
                                {
                                    jammerActive = false;
                                    *hScore += 500;
                                    PlaySound(sndJammerDeath);
                                }
                            }
                        }

                        // Collision with Warp Commander
                        if (warpActive && bActive[i])
                        {
                            Rectangle wRec = { warpPos.x, warpPos.y, COMMANDER_SIZE, COMMANDER_SIZE };
                            if (CheckCollisionRecs(bRec, wRec))
                            {
                                PlaySound(damage);
                                bActive[i] = false;
                                warpHp--;
                                *hScore += 50;
                                if (warpHp <= 0)
                                {
                                    warpActive = false;
                                    *hScore += 500;
                                    PlaySound(sndWarpDeath);
                                }
                            }
                        }

                        // Collision with Medium Minion Aliens
                        if (bossActive && bActive[i])
                        {
                            for (int m = 0; m < MAX_MINIONS; m++)
                            {
                                if (minionActive[m])
                                {
                                    Rectangle mRec = { minionPos[m].x, minionPos[m].y, MinionSize, MinionSize };
                                    if (CheckCollisionRecs(bRec, mRec))
                                    {
                                        PlaySound(damage);
                                        bActive[i] = false;
                                        minionHp[m]--;
                                        if (minionHp[m] <= 0)
                                        {
                                            minionActive[m] = false;
                                            *hScore += 150;
                                            *hKills += 1;
                                        }
                                        break;
                                    }
                                }
                            }
                        }

                        // Boss Shields
                        if (bossActive && bActive[i])
                        {
                            Rectangle leftPodRec  = { bossPos.x + 8, bossPos.y + 70, 75, 110 };
                            Rectangle rightPodRec = { bossPos.x + BossWidth - 83, bossPos.y + 70, 75, 110 };
                            Rectangle coreRec     = { bossPos.x + 85, bossPos.y + 35, 130, 130 };

                            if (bossLeftPodHp > 0 && CheckCollisionRecs(bRec, leftPodRec))
                            {
                                PlaySound(damage);
                                bActive[i] = false;
                                bossLeftPodHp -= 5;
                                *hScore += 25;
                                *hBossDmg += 5;
                                if (bossLeftPodHp <= 0)
                                {
                                    bossLeftPodHp = 0;
                                    *hScore += 250;
                                    PlaySound(sndPodDestroy);
                                }
                            }
                            else if (bossRightPodHp > 0 && CheckCollisionRecs(bRec, rightPodRec))
                            {
                                PlaySound(damage);
                                bActive[i] = false;
                                bossRightPodHp -= 5;
                                *hScore += 25;
                                *hBossDmg += 5;
                                if (bossRightPodHp <= 0)
                                {
                                    bossRightPodHp = 0;
                                    *hScore += 250;
                                    PlaySound(sndPodDestroy);
                                }
                            }
                            else if (CheckCollisionRecs(bRec, coreRec))
                            {
                                bActive[i] = false;
                                if (bossLeftPodHp > 0 || bossRightPodHp > 0)
                                {
                                    PlaySound(damage);
                                }
                                else
                                {
                                    PlaySound(damage);
                                    bossHp -= 3;
                                    *hScore += 50;
                                    *hBossDmg += 3;

                                    if (bossHp <= 40 && !bossEnraged)
                                    {
                                        bossEnraged = true;
                                        PlaySound(sndBossEnrage);
                                    }

                                    if (bossHp <= 0)
                                    {
                                        bossHp = 0;
                                        bossActive = false;
                                        bossDefeated = true;
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // SHOAB SPECIAL BULLET MOVEMENT & COLLISION (WITH SHOAB'S BOSS DAMAGE & OVERFLOW MECHANICS)
            if (Hero1SpecialBulletActive)
            {
                Hero1SpecialBulletPos.y -= BulletSpeedY * Time;
                Rectangle SpecialBulletRec = { Hero1SpecialBulletPos.x - BulletWidth / 2.0f, Hero1SpecialBulletPos.y, BulletWidth, BulletHeight * 5 };

                // 1. Regular Aliens Piercing Strike
                if (!bossActive && !bossSpawned)
                {
                    for (int i = 0; i < AlienInX; i++)
                    {
                        for (int j = 0; j < AlienInY; j++)
                        {
                            if (AlienAlive[i][j])
                            {
                                Rectangle AlienRec = { AlienPos[i][j].x, AlienPos[i][j].y, AlienSize, AlienSize };
                                if (CheckCollisionRecs(SpecialBulletRec, AlienRec))
                                {
                                    AlienAlive[i][j] = false;
                                    AliensKilled++;
                                    Hero1Score += 100;
                                    Hero1Kills++;
                                    PlaySound(damage);
                                }
                            }
                        }
                    }
                }

                // 2. Piercing Hit on Jammer Commander
                if (jammerActive)
                {
                    Rectangle jRec = { jammerPos.x, jammerPos.y, COMMANDER_SIZE, COMMANDER_SIZE };
                    if (CheckCollisionRecs(SpecialBulletRec, jRec))
                    {
                        PlaySound(damage);
                        jammerHp -= 2;
                        Hero1Score += 50;
                        if (jammerHp <= 0)
                        {
                            jammerActive = false;
                            Hero1Score += 500;
                            PlaySound(sndJammerDeath);
                        }
                    }
                }

                // 3. Piercing Hit on Warp Commander
                if (warpActive)
                {
                    Rectangle wRec = { warpPos.x, warpPos.y, COMMANDER_SIZE, COMMANDER_SIZE };
                    if (CheckCollisionRecs(SpecialBulletRec, wRec))
                    {
                        PlaySound(damage);
                        warpHp -= 2;
                        Hero1Score += 50;
                        if (warpHp <= 0)
                        {
                            warpActive = false;
                            Hero1Score += 500;
                            PlaySound(sndWarpDeath);
                        }
                    }
                }

                // 4. Shoab's Instant Minion Destruction & Boss Shield Overflow Mechanics
                if (bossActive)
                {
                    // Instant kill on minions
                    for (int m = 0; m < MAX_MINIONS; m++)
                    {
                        if (minionActive[m])
                        {
                            Rectangle mRec = { minionPos[m].x, minionPos[m].y, MinionSize, MinionSize };
                            if (CheckCollisionRecs(SpecialBulletRec, mRec))
                            {
                                minionActive[m] = false;
                                Hero1Kills += 1;
                                Hero1Score += 200;
                                PlaySound(damage);
                            }
                        }
                    }

                    Rectangle leftPodRec  = { bossPos.x + 8, bossPos.y + 70, 75, 110 };
                    Rectangle rightPodRec = { bossPos.x + BossWidth - 83, bossPos.y + 70, 75, 110 };
                    Rectangle coreRec     = { bossPos.x + 85, bossPos.y + 35, 130, 130 };

                    // Strike Left Shield Pod (50 Damage + Overflow to Right Pod & Boss Core)
                    if (bossLeftPodHp > 0 && CheckCollisionRecs(SpecialBulletRec, leftPodRec))
                    {
                        Hero1SpecialBulletActive = false;
                        PlaySound(damage);
                        bossLeftPodHp -= 50;
                        Hero1Score += 250;
                        Hero1BossDamage += 50;
                        if (bossLeftPodHp <= 0)
                        {
                            PlaySound(sndPodDestroy);
                            bossRightPodHp += bossLeftPodHp;
                            if (bossRightPodHp < 0) {
                                bossHp += bossRightPodHp;
                                bossRightPodHp = 0;
                            }
                            bossLeftPodHp = 0;
                            Hero1Score += 250;
                            if (bossHp <= 40 && !bossEnraged)
                            {
                                bossEnraged = true;
                                PlaySound(sndBossEnrage);
                            }
                            if (bossHp <= 0) {
                                bossHp = 0;
                                bossActive = false;
                                bossDefeated = true;
                            }
                        }
                    }
                    // Strike Right Shield Pod (50 Damage + Overflow to Left Pod & Boss Core)
                    else if (bossRightPodHp > 0 && CheckCollisionRecs(SpecialBulletRec, rightPodRec))
                    {
                        Hero1SpecialBulletActive = false;
                        PlaySound(damage);
                        bossRightPodHp -= 50;
                        Hero1Score += 250;
                        Hero1BossDamage += 50;
                        if (bossRightPodHp <= 0)
                        {
                            PlaySound(sndPodDestroy);
                            bossLeftPodHp += bossRightPodHp;
                            if (bossLeftPodHp < 0) {
                                bossHp += bossLeftPodHp;
                                bossLeftPodHp = 0;
                            }
                            bossRightPodHp = 0;
                            Hero1Score += 250;
                            if (bossHp <= 40 && !bossEnraged)
                            {
                                bossEnraged = true;
                                PlaySound(sndBossEnrage);
                            }
                            if (bossHp <= 0) {
                                bossHp = 0;
                                bossActive = false;
                                bossDefeated = true;
                            }
                        }
                    }
                    // Strike Boss Center Core
                    else if (CheckCollisionRecs(SpecialBulletRec, coreRec))
                    {
                        Hero1SpecialBulletActive = false;
                        if (bossLeftPodHp > 0 || bossRightPodHp > 0)
                        {
                            PlaySound(damage);
                            bossLeftPodHp -= 50;
                            Hero1Score += 250;
                            Hero1BossDamage += 50;
                            if (bossLeftPodHp <= 0)
                            {
                                PlaySound(sndPodDestroy);
                                bossRightPodHp += bossLeftPodHp;
                                if (bossRightPodHp < 0) {
                                    bossHp += bossRightPodHp;
                                    bossRightPodHp = 0;
                                }
                                bossLeftPodHp = 0;
                                Hero1Score += 250;
                                if (bossHp <= 40 && !bossEnraged)
                                {
                                    bossEnraged = true;
                                    PlaySound(sndBossEnrage);
                                }
                                if (bossHp <= 0) {
                                    bossHp = 0;
                                    bossActive = false;
                                    bossDefeated = true;
                                }
                            }
                        }
                        else
                        {
                            // Shields down: 30 direct damage to boss core
                            PlaySound(damage);
                            bossHp -= 30;
                            Hero1Score += 500;
                            Hero1BossDamage += 30;
                            if (bossHp <= 40 && !bossEnraged)
                            {
                                bossEnraged = true;
                                PlaySound(sndBossEnrage);
                            }
                            if (bossHp <= 0)
                            {
                                bossHp = 0;
                                bossActive = false;
                                bossDefeated = true;
                            }
                        }
                    }
                }

                if (Hero1SpecialBulletPos.y < 0)
                {
                    Hero1SpecialBulletActive = false;
                }
            }

            // RANDOM ALIEN SHOOTING
            if (!bossSpawned)
            {
                AlienShootTimer += Time;
                if (!AlienBulletActive && AlienShootTimer >= 1.0f && !deathSequenceActive)
                {
                    AlienShootTimer = 0.0f;
                    int randomX = GetRandomValue(0, AlienInX - 1);

                    for (int Y = AlienInY - 1; Y >= 0; Y--)
                    {
                        if (AlienAlive[randomX][Y])
                        {
                            PlaySound(AlienShoot);
                            AlienBulletActive = true;
                            AlienBulletPos = (Vector2){ AlienPos[randomX][Y].x + AlienSize / 2.0f, AlienPos[randomX][Y].y + AlienSize };
                            break;
                        }
                    }
                }
            }

            // UPDATE ALIEN BULLET and HERO HIT
            if (AlienBulletActive)
            {
                AlienBulletPos.y += AlienBulletSpeedY * Time;
                if (AlienBulletPos.y > WindowHeight)
                {
                    AlienBulletActive = false;
                }
                else if (!deathSequenceActive)
                {
                    Rectangle aBulletRec = { AlienBulletPos.x - AlienBulletWidth / 2.0f, AlienBulletPos.y, AlienBulletWidth, AlienBulletHeight };
                    Rectangle h1Rec = { Hero1Pos.x - HeroWidth / 2.0f, Hero1Pos.y, HeroWidth, HeroHeight };
                    Rectangle h2Rec = { Hero2Pos.x - HeroWidth / 2.0f, Hero2Pos.y, HeroWidth, HeroHeight };

                    if (Hero1Lives > 0 && CheckCollisionRecs(aBulletRec, h1Rec))
                    {
                        AlienBulletActive = false;
                        Hero1Lives--;
                        Hero1HitsTaken++;
                        hero1HitFlashTimer = 0.35f;
                        PlaySound(heroOuch);

                        if (Hero1Lives <= 0)
                        {
                            Hero1CrashPos = Hero1Pos;
                            PlaySound(heroDeath);
                            if (Hero2Lives <= 0) { deathSequenceActive = true; deathDelayTimer = 0.0f; }
                        }
                    }
                    else if (Hero2Lives > 0 && CheckCollisionRecs(aBulletRec, h2Rec))
                    {
                        AlienBulletActive = false;
                        Hero2Lives--;
                        Hero2HitsTaken++;
                        hero2HitFlashTimer = 0.35f;
                        PlaySound(heroOuch);

                        if (Hero2Lives <= 0)
                        {
                            Hero2CrashPos = Hero2Pos;
                            PlaySound(heroDeath);
                            if (Hero1Lives <= 0) { deathSequenceActive = true; deathDelayTimer = 0.0f; }
                        }
                    }
                }
            }

            // Deploy medium-sized aliens every 7 seconds
            if (bossActive && (bossLeftPodHp > 0 || bossRightPodHp > 0) && !deathSequenceActive)
            {
                minionDeployTimer += Time;
                if (minionDeployTimer >= 7.0f)
                {
                    minionDeployTimer = 0.0f;

                    if (bossLeftPodHp > 0)
                    {
                        for (int m = 0; m < MAX_MINIONS; m++)
                        {
                            if (!minionActive[m])
                            {
                                minionActive[m] = true;
                                minionHp[m] = 2;
                                minionPos[m] = (Vector2){ bossPos.x + 15, bossPos.y + BossHeight - 20 };
                                minionSpeed[m] = (Vector2){ (float)GetRandomValue(-90, -45), (float)GetRandomValue(35, 75) };
                                minionShootTimer[m] = 1.0f;
                                break;
                            }
                        }
                    }

                    if (bossRightPodHp > 0)
                    {
                        for (int m = 0; m < MAX_MINIONS; m++)
                        {
                            if (!minionActive[m])
                            {
                                minionActive[m] = true;
                                minionHp[m] = 2;
                                minionPos[m] = (Vector2){ bossPos.x + BossWidth - MinionSize - 15, bossPos.y + BossHeight - 20 };
                                minionSpeed[m] = (Vector2){ (float)GetRandomValue(45, 90), (float)GetRandomValue(35, 75) };
                                minionShootTimer[m] = 1.5f;
                                break;
                            }
                        }
                    }
                }
            }

            // UPDATE MEDIUM-SIZED MINION ALIENS
            if (bossActive)
            {
                for (int m = 0; m < MAX_MINIONS; m++)
                {
                    if (minionActive[m])
                    {
                        minionPos[m].x += minionSpeed[m].x * Time;
                        minionPos[m].y += minionSpeed[m].y * Time;

                        if (minionPos[m].x <= 40)
                        {
                            minionPos[m].x = 40;
                            minionSpeed[m].x = fabsf(minionSpeed[m].x);
                        }
                        else if (minionPos[m].x >= WindowWidth - MinionSize - 40)
                        {
                            minionPos[m].x = WindowWidth - MinionSize - 40;
                            minionSpeed[m].x = -fabsf(minionSpeed[m].x);
                        }

                        if (minionPos[m].y <= 110)
                        {
                            minionPos[m].y = 110;
                            minionSpeed[m].y = fabsf(minionSpeed[m].y);
                        }
                        else if (minionPos[m].y >= WindowHeight * 0.65f - MinionSize)
                        {
                            minionPos[m].y = WindowHeight * 0.65f - MinionSize;
                            minionSpeed[m].y = -fabsf(minionSpeed[m].y);
                        }

                        if (!deathSequenceActive)
                        {
                            minionShootTimer[m] += Time;
                            if (minionShootTimer[m] >= 2.4f)
                            {
                                minionShootTimer[m] = 0.0f;
                                for (int mb = 0; mb < MAX_MINION_BULLETS; mb++)
                                {
                                    if (!minionBulletActive[mb])
                                    {
                                        minionBulletActive[mb] = true;
                                        minionBulletPos[mb] = (Vector2){ minionPos[m].x + MinionSize / 2.0f, minionPos[m].y + MinionSize };
                                        PlaySound(AlienShoot);
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // UPDATE MINION BULLETS and HERO DAMAGE
            for (int mb = 0; mb < MAX_MINION_BULLETS; mb++)
            {
                if (minionBulletActive[mb])
                {
                    minionBulletPos[mb].y += AlienBulletSpeedY * Time;
                    if (minionBulletPos[mb].y > WindowHeight)
                    {
                        minionBulletActive[mb] = false;
                    }
                    else if (!deathSequenceActive)
                    {
                        Rectangle mbRec = { minionBulletPos[mb].x - AlienBulletWidth / 2.0f, minionBulletPos[mb].y, AlienBulletWidth, AlienBulletHeight };
                        Rectangle h1Rec = { Hero1Pos.x - HeroWidth / 2.0f, Hero1Pos.y, HeroWidth, HeroHeight };
                        Rectangle h2Rec = { Hero2Pos.x - HeroWidth / 2.0f, Hero2Pos.y, HeroWidth, HeroHeight };

                        if (Hero1Lives > 0 && CheckCollisionRecs(mbRec, h1Rec))
                        {
                            minionBulletActive[mb] = false;
                            Hero1Lives--;
                            Hero1HitsTaken++;
                            hero1HitFlashTimer = 0.35f;
                            PlaySound(heroOuch);

                            if (Hero1Lives <= 0)
                            {
                                Hero1CrashPos = Hero1Pos;
                                PlaySound(heroDeath);
                                if (Hero2Lives <= 0) { deathSequenceActive = true; deathDelayTimer = 0.0f; }
                            }
                        }
                        else if (Hero2Lives > 0 && CheckCollisionRecs(mbRec, h2Rec))
                        {
                            minionBulletActive[mb] = false;
                            Hero2Lives--;
                            Hero2HitsTaken++;
                            hero2HitFlashTimer = 0.35f;
                            PlaySound(heroOuch);

                            if (Hero2Lives <= 0)
                            {
                                Hero2CrashPos = Hero2Pos;
                                PlaySound(heroDeath);
                                if (Hero1Lives <= 0) { deathSequenceActive = true; deathDelayTimer = 0.0f; }
                            }
                        }
                    }
                }
            }

            float currentBossSpeedX = (bossHp <= 40) ? 250.0f : 160.0f;
            float currentBossSpeedY = (bossHp <= 40) ? 120.0f : 75.0f;

            // Boss combat execution
            if (bossActive)
            {
                bossAnimTimer += Time;
                bossDirChangeTimer += Time;

                if (bossDirChangeTimer > 1.8f)
                {
                    bossDirChangeTimer = 0.0f;
                    if (GetRandomValue(0, 10) > 4)
                    {
                        bossSpeed.y = (float)GetRandomValue(-(int)currentBossSpeedY, (int)currentBossSpeedY);
                    }
                }

                bossPos.x += ((bossSpeed.x > 0) ? currentBossSpeedX : -currentBossSpeedX) * Time;
                bossPos.y += bossSpeed.y * Time;

                if (bossPos.x <= 40)
                {
                    bossPos.x = 40;
                    bossSpeed.x = fabsf(bossSpeed.x);
                }
                else if (bossPos.x + BossWidth >= WindowWidth - 40)
                {
                    bossPos.x = WindowWidth - BossWidth - 40;
                    bossSpeed.x = -fabsf(bossSpeed.x);
                }

                float maxBossY = (WindowHeight * 0.75f) - BossHeight;
                if (bossPos.y <= 30)
                {
                    bossPos.y = 30;
                    bossSpeed.y = fabsf(bossSpeed.y);
                }
                else if (bossPos.y >= maxBossY)
                {
                    bossPos.y = maxBossY;
                    bossSpeed.y = -fabsf(bossSpeed.y);
                }

                // Attack 1: Triple Bullets
                bossShootTimer += Time;
                if (bossShootTimer >= 1.7f && !deathSequenceActive)
                {
                    bossShootTimer = 0.0f;
                    float offsets[3] = { 35.0f, BossWidth / 2.0f, BossWidth - 35.0f };
                    int spawned = 0;

                    for (int k = 0; k < MAX_BOSS_BULLETS && spawned < 3; k++)
                    {
                        if (!bossBulletActive[k])
                        {
                            bossBulletActive[k] = true;
                            bossBulletPos[k] = (Vector2){ bossPos.x + offsets[spawned], bossPos.y + BossHeight };
                            spawned++;
                        }
                    }
                    PlaySound(AlienShoot);
                }

                // Attack 2: Laser Beam every 7s
                bossLaserTimer += Time;
                if (!bossLaserActive)
                {
                    if (bossLaserTimer >= 7.0f)
                    {
                        bossLaserActive = true;
                        bossLaserDuration = 0.0f;
                        bossLaserTimer = 0.0f;
                        PlaySound(bossLaserSound);
                    }
                }
                else
                {
                    bossLaserDuration += Time;
                    if (bossLaserDuration >= 2.0f)
                    {
                        bossLaserActive = false;
                        bossLaserDuration = 0.0f;
                        StopSound(bossLaserSound);
                    }
                }

                // Attack 3: 6 Circular Projectiles every 4s (causes jamming and slow)
                bossOrbTimer += Time;
                if (bossOrbTimer >= 4.0f && !deathSequenceActive)
                {
                    bossOrbTimer = 0.0f;
                    PlaySound(sndOrbLaunch);

                    float spreadVx[6] = { -160.0f, -95.0f, -30.0f, 30.0f, 95.0f, 160.0f };
                    int spawned = 0;

                    for (int k = 0; k < MAX_BOSS_ORBS && spawned < 6; k++)
                    {
                        if (!bossOrbActive[k])
                        {
                            bossOrbActive[k] = true;
                            bossOrbPos[k] = (Vector2){ bossPos.x + BossWidth / 2.0f, bossPos.y + BossHeight - 15.0f };
                            bossOrbVel[k] = (Vector2){ spreadVx[spawned], 270.0f };
                            spawned++;
                        }
                    }
                }
            }

            // Update Boss Bullets
            for (int k = 0; k < MAX_BOSS_BULLETS; k++)
            {
                if (bossBulletActive[k])
                {
                    bossBulletPos[k].y += AlienBulletSpeedY * Time;
                    if (bossBulletPos[k].y > WindowHeight)
                    {
                        bossBulletActive[k] = false;
                    }
                    else if (!deathSequenceActive)
                    {
                        Rectangle bRec = { bossBulletPos[k].x - AlienBulletWidth / 2.0f, bossBulletPos[k].y, AlienBulletWidth, AlienBulletHeight };
                        Rectangle h1Rec = { Hero1Pos.x - HeroWidth / 2.0f, Hero1Pos.y, HeroWidth, HeroHeight };
                        Rectangle h2Rec = { Hero2Pos.x - HeroWidth / 2.0f, Hero2Pos.y, HeroWidth, HeroHeight };

                        if (Hero1Lives > 0 && CheckCollisionRecs(bRec, h1Rec))
                        {
                            bossBulletActive[k] = false;
                            Hero1Lives--;
                            Hero1HitsTaken++;
                            hero1HitFlashTimer = 0.35f;
                            PlaySound(heroOuch);

                            if (Hero1Lives <= 0)
                            {
                                Hero1CrashPos = Hero1Pos;
                                PlaySound(heroDeath);
                                if (Hero2Lives <= 0) { deathSequenceActive = true; deathDelayTimer = 0.0f; }
                            }
                        }
                        else if (Hero2Lives > 0 && CheckCollisionRecs(bRec, h2Rec))
                        {
                            bossBulletActive[k] = false;
                            Hero2Lives--;
                            Hero2HitsTaken++;
                            hero2HitFlashTimer = 0.35f;
                            PlaySound(heroOuch);

                            if (Hero2Lives <= 0)
                            {
                                Hero2CrashPos = Hero2Pos;
                                PlaySound(heroDeath);
                                if (Hero1Lives <= 0) { deathSequenceActive = true; deathDelayTimer = 0.0f; }
                            }
                        }
                    }
                }
            }

            // Update Boss Circular Projectiles
            for (int k = 0; k < MAX_BOSS_ORBS; k++)
            {
                if (bossOrbActive[k])
                {
                    bossOrbPos[k] = Vector2Add(bossOrbPos[k], Vector2Scale(bossOrbVel[k], Time));
                    if (bossOrbPos[k].y > WindowHeight || bossOrbPos[k].x < 0 || bossOrbPos[k].x > WindowWidth)
                    {
                        bossOrbActive[k] = false;
                    }
                    else if (!deathSequenceActive)
                    {
                        Rectangle h1Rec = { Hero1Pos.x - HeroWidth / 2.0f, Hero1Pos.y, HeroWidth, HeroHeight };
                        Rectangle h2Rec = { Hero2Pos.x - HeroWidth / 2.0f, Hero2Pos.y, HeroWidth, HeroHeight };

                        if (Hero1Lives > 0 && CheckCollisionCircleRec(bossOrbPos[k], 12.0f, h1Rec))
                        {
                            bossOrbActive[k] = false;
                            hero1Debuffed = true;
                            hero1DebuffTimer = 4.5f;
                            Hero1HitsTaken++;
                            hero1HitFlashTimer = 0.35f;
                            PlaySound(heroOuch);
                        }
                        else if (Hero2Lives > 0 && CheckCollisionCircleRec(bossOrbPos[k], 12.0f, h2Rec))
                        {
                            bossOrbActive[k] = false;
                            hero2Debuffed = true;
                            hero2DebuffTimer = 4.5f;
                            Hero2HitsTaken++;
                            hero2HitFlashTimer = 0.35f;
                            PlaySound(heroOuch);
                        }
                    }
                }
            }

            // Update Boss Laser Beam Collision
            if (bossLaserActive && !deathSequenceActive)
            {
                float laserW = 42.0f;
                float laserX = bossPos.x + (BossWidth - laserW) / 2.0f;
                float laserY = bossPos.y + BossHeight - 10.0f;
                float laserH = WindowHeight - laserY;

                Rectangle laserRec = { laserX, laserY, laserW, laserH };
                Rectangle h1Rec = { Hero1Pos.x - HeroWidth / 2.0f, Hero1Pos.y, HeroWidth, HeroHeight };
                Rectangle h2Rec = { Hero2Pos.x - HeroWidth / 2.0f, Hero2Pos.y, HeroWidth, HeroHeight };

                bool hit1 = (Hero1Lives > 0 && CheckCollisionRecs(laserRec, h1Rec));
                bool hit2 = (Hero2Lives > 0 && CheckCollisionRecs(laserRec, h2Rec));

                if (hit1)
                {
                    Hero1Lives = 0;
                    Hero1CrashPos = Hero1Pos;
                    Hero1HitsTaken += 4;
                    hero1HitFlashTimer = 0.45f;
                    PlaySound(heroOuch);
                }
                if (hit2)
                {
                    Hero2Lives = 0;
                    Hero2CrashPos = Hero2Pos;
                    Hero2HitsTaken += 4;
                    hero2HitFlashTimer = 0.45f;
                    PlaySound(heroOuch);
                }

                if (hit1 || hit2)
                {
                    PlaySound(heroDeath);
                    if (Hero1Lives <= 0 && Hero2Lives <= 0)
                    {
                        deathSequenceActive = true;
                        deathDelayTimer = 0.0f;
                        bossLaserActive = false;
                        StopSound(bossLaserSound);
                        screenCamera.offset = (Vector2){ 0, 0 };
                    }
                }
            }

            // ALIEN GRID MOVEMENT (VARIED ROW & COLUMN SPEED)
            if (!bossSpawned)
            {
                for (int Y = 0; Y < AlienInY; Y++)
                {
                    if (AlienPos[AlienInX - 1][Y].x + AlienSize >= WindowWidth && AlienSpeed.x > 0)
                    {
                        AlienSpeed.x *= -1;
                        DownAlien(AlienInX, AlienInY, AlienPos);
                    }
                    else if (AlienPos[0][Y].x <= 0 && AlienSpeed.x < 0)
                    {
                        AlienSpeed.x *= -1;
                        DownAlien(AlienInX, AlienInY, AlienPos);
                    }
                }

                for (int X = 0; X < AlienInX; X++)
                {
                    for (int Y = 0; Y < AlienInY; Y++)
                    {
                        if (AlienSpeed.x > 0) AlienPos[X][Y] = Vector2Add(AlienPos[X][Y], Vector2Scale(Vector2Add(AlienSpeed, (Vector2){ (float)SpeedBuff[Y], 0 }), Time));
                        else AlienPos[X][Y] = Vector2Add(AlienPos[X][Y], Vector2Scale(Vector2Add(AlienSpeed, (Vector2){ (float)SpeedBuff[Y] * (-1), 0 }), Time));

                        if (AlienAlive[X][Y])
                        {
                            Rectangle Alien = { AlienPos[X][Y].x, AlienPos[X][Y].y, AlienSize, AlienSize };
                            int AStyle = (int)(GetTime() / 0.1) % AlienSpriteStyle[AlienLooks[X][Y]-1];
                            DrawTexturePro(AlienTexture[AlienLooks[X][Y]-1],
                                (Rectangle){ AStyle*(AlienSWidth[AlienLooks[X][Y]-1]+1), 0, AlienSWidth[AlienLooks[X][Y]-1], (-1)*AlienSHeight[AlienLooks[X][Y]-1] },
                                Alien, (Vector2){ 0, 0 }, 0.0f, WHITE);
                        }
                    }
                }
            }

            // DRAW BULLETS
            for (int i = 0; i < 2; i++)
            {
                if (Hero1BulletActive[i])
                {
                    Color bCol = (empBuffTimer > 0.0f) ? LIME : YELLOW;
                    DrawRectangle((int)(Hero1BulletPos[i].x - BulletWidth / 2.0f), (int)Hero1BulletPos[i].y, BulletWidth, BulletHeight, bCol);
                }
                if (Hero2BulletActive[i])
                {
                    Color bCol = (empBuffTimer > 0.0f) ? WHITE : SKYBLUE;
                    DrawRectangle((int)(Hero2BulletPos[i].x - BulletWidth / 2.0f), (int)Hero2BulletPos[i].y, BulletWidth, BulletHeight, bCol);
                }
            }

            // DRAW SHOAB'S SPECIAL PIERCING BEAM
            if (Hero1SpecialBulletActive)
            {
                DrawRectangle((int)(Hero1SpecialBulletPos.x - BulletWidth / 2.0f), (int)Hero1SpecialBulletPos.y, BulletWidth, BulletHeight * 5, WHITE);
            }

            if (AlienBulletActive)
            {
                DrawRectangle((int)(AlienBulletPos.x - AlienBulletWidth / 2.0f), (int)AlienBulletPos.y, AlienBulletWidth, AlienBulletHeight, RED);
            }

            // DRAW COMMANDER BULLETS
            for (int b = 0; b < MAX_COMMANDER_BULLETS; b++)
            {
                if (jammerBulletActive[b])
                {
                    DrawRectangle((int)(jammerBulletPos[b].x - 4), (int)jammerBulletPos[b].y, 8, 22, (Color){ 200, 70, 255, 255 });
                    DrawCircle((int)jammerBulletPos[b].x, (int)jammerBulletPos[b].y + 11, 6.0f, Fade((Color){ 230, 120, 255, 255 }, 0.5f));
                }
                if (warpBulletActive[b])
                {
                    DrawRectangle((int)(warpBulletPos[b].x - 3), (int)warpBulletPos[b].y, 6, 26, SKYBLUE);
                    DrawRectangle((int)(warpBulletPos[b].x - 1), (int)warpBulletPos[b].y + 4, 2, 18, WHITE);
                }
            }

            // DRAW MEDIUM-SIZED MINION BULLETS
            if (bossActive)
            {
                for (int mb = 0; mb < MAX_MINION_BULLETS; mb++)
                {
                    if (minionBulletActive[mb])
                    {
                        DrawRectangle((int)(minionBulletPos[mb].x - AlienBulletWidth / 2.0f), (int)minionBulletPos[mb].y, AlienBulletWidth, AlienBulletHeight, ORANGE);
                    }
                }
            }

            // DRAW ORBITAL EMP SUPPLY CRATE & SHOCKWAVE
            if (empActive)
            {
                DrawCircleLines((int)empPos.x, (int)empPos.y - 20, 22.0f, Fade(SKYBLUE, 0.8f));
                DrawLine((int)empPos.x - 20, (int)empPos.y - 20, (int)empPos.x - 8, (int)empPos.y - 4, SKYBLUE);
                DrawLine((int)empPos.x + 20, (int)empPos.y - 20, (int)empPos.x + 8, (int)empPos.y - 4, SKYBLUE);

                DrawRectangle((int)empPos.x - 16, (int)empPos.y - 8, 32, 28, DARKBLUE);
                DrawRectangleLines((int)empPos.x - 16, (int)empPos.y - 8, 32, 28, SKYBLUE);
                DrawText("EMP", (int)empPos.x - 12, (int)empPos.y - 2, 11, YELLOW);
            }

            if (empShockwaveRadius > 0.0f)
            {
                empShockwaveRadius += 1600.0f * Time;
                float shockFade = 1.0f - (empShockwaveRadius / 1800.0f);
                if (shockFade <= 0.0f) empShockwaveRadius = 0.0f;
                else
                {
                    DrawCircleLines((int)empShockwaveCenter.x, (int)empShockwaveCenter.y, empShockwaveRadius, Fade(SKYBLUE, shockFade));
                    DrawCircleLines((int)empShockwaveCenter.x, (int)empShockwaveCenter.y, empShockwaveRadius + 4.0f, Fade(WHITE, shockFade * 0.7f));
                }
            }

            // DRAW ACTIVE ELITE COMMANDERS
            if (jammerActive)
            {
                Rectangle jRec = { jammerPos.x, jammerPos.y, COMMANDER_SIZE, COMMANDER_SIZE };
                DrawTexturePro(jammerTex[jammerAnimState], (Rectangle){ 0, 0, (float)jammerTex[jammerAnimState].width, (float)jammerTex[jammerAnimState].height }, jRec, (Vector2){ 0, 0 }, 0.0f, WHITE);
                DrawRectangle((int)jammerPos.x + 8, (int)jammerPos.y - 10, (int)((COMMANDER_SIZE - 16) * ((float)jammerHp / jammerMaxHp)), 5, (Color){ 200, 80, 255, 255 });
                DrawRectangleLines((int)jammerPos.x + 8, (int)jammerPos.y - 10, COMMANDER_SIZE - 16, 5, WHITE);
            }

            if (warpActive)
            {
                Rectangle wRec = { warpPos.x, warpPos.y, COMMANDER_SIZE, COMMANDER_SIZE };
                DrawTexturePro(warpTex[warpAnimState], (Rectangle){ 0, 0, (float)warpTex[warpAnimState].width, (float)warpTex[warpAnimState].height }, wRec, (Vector2){ 0, 0 }, 0.0f, WHITE);
                DrawRectangle((int)warpPos.x + 8, (int)warpPos.y - 10, (int)((COMMANDER_SIZE - 16) * ((float)warpHp / warpMaxHp)), 5, SKYBLUE);
                DrawRectangleLines((int)warpPos.x + 8, (int)warpPos.y - 10, COMMANDER_SIZE - 16, 5, WHITE);
            }

            // DRAW COMMANDER INTRO EFFECTS
            if (commanderIntroActive)
            {
                float vignettePulse = fabsf(sinf((float)GetTime() * 10.0f));

                // Temporal distortion overlay
                DrawRectangle(0, 0, WindowWidth, WindowHeight, Fade(DARKPURPLE, 0.18f + 0.10f * vignettePulse));

                // Horizontal glitch scanlines
                for (int y = 0; y < WindowHeight; y += 45)
                {
                    float alpha = ((int)(GetTime() * 20.0f) % 2 == 0) ? 0.25f : 0.08f;
                    DrawLine(0, y, WindowWidth, y, Fade(SKYBLUE, alpha));
                }

                // Target reticles at spawn positions
                Vector2 jammerTarget = { WindowWidth * 0.25f, 130.0f + COMMANDER_SIZE / 2.0f };
                Vector2 warpTarget   = { WindowWidth * 0.75f, 130.0f + COMMANDER_SIZE / 2.0f };
                DrawCircleLines((int)jammerTarget.x, (int)jammerTarget.y, 38.0f, Fade((Color){ 200, 80, 255, 255 }, vignettePulse));
                DrawCircleLines((int)warpTarget.x, (int)warpTarget.y, 38.0f, Fade(SKYBLUE, vignettePulse));
            }

            // DRAW BOSS, SHIELDS and MEDIUM-SIZED MINION ALIENS
            if (bossActive)
            {
                int bFrame = 0;
                if (bossSpeed.y > 25.0f) bFrame = 1;
                else if (bossSpeed.y < -25.0f) bFrame = 2;
                else
                {
                    int pulseCycle = ((int)(bossAnimTimer / 0.35f)) % 4;
                    if (pulseCycle == 0) bFrame = 0;
                    else if (pulseCycle == 1) bFrame = 3;
                    else if (pulseCycle == 2) bFrame = 0;
                    else bFrame = 4;
                }

                Rectangle bossRec = { bossPos.x, bossPos.y, BossWidth, BossHeight };
                Color bossTint = (bossHp <= 40) ? (Color){ 255, 120, 120, 255 } : WHITE;

                DrawTexturePro(
                    BossTexture[bFrame],
                    (Rectangle){ 0, 0, (float)BossTexture[bFrame].width, (float)BossTexture[bFrame].height },
                    bossRec, (Vector2){ 0, 0 }, 0.0f, bossTint
                );

                int AStyle = (int)(GetTime() / 0.1) % AlienSpriteStyle[19];
                for (int m = 0; m < MAX_MINIONS; m++)
                {
                    if (minionActive[m])
                    {
                        Rectangle mDest = { minionPos[m].x, minionPos[m].y, MinionSize, MinionSize };
                        DrawTexturePro(AlienTexture[19],
                            (Rectangle){ AStyle * (AlienSWidth[19] + 1), 0, (float)(AlienSWidth[19]), (float)AlienSHeight[19] },
                            mDest, (Vector2){ 0, 0 }, 0.0f, WHITE);

                        DrawRectangle((int)minionPos[m].x + 10, (int)minionPos[m].y - 8, (int)((MinionSize - 20) * (minionHp[m] / 2.0f)), 4, LIME);
                    }
                }

                if (bossHp <= 40)
                {
                    DrawLineEx((Vector2){ bossPos.x + 90, bossPos.y + 40 }, (Vector2){ bossPos.x + 130, bossPos.y + 110 }, 2.5f, MAROON);
                    DrawLineEx((Vector2){ bossPos.x + 130, bossPos.y + 110 }, (Vector2){ bossPos.x + 170, bossPos.y + 70 }, 2.0f, RED);
                    DrawLineEx((Vector2){ bossPos.x + 200, bossPos.y + 50 }, (Vector2){ bossPos.x + 160, bossPos.y + 140 }, 2.5f, BLACK);

                    for (int s = 0; s < 4; s++)
                    {
                        float smkX = bossPos.x + 60 + (s * 55) + GetRandomValue(-8, 8);
                        float smkY = bossPos.y + 40 + GetRandomValue(-10, 20);
                        DrawCircle((int)smkX, (int)smkY, (float)GetRandomValue(8, 16), Fade(DARKGRAY, 0.40f));
                    }

                    for (int sp = 0; sp < 5; sp++)
                    {
                        Vector2 sp1 = { bossPos.x + GetRandomValue(30, BossWidth - 30), bossPos.y + GetRandomValue(30, BossHeight - 40) };
                        Vector2 sp2 = { sp1.x + GetRandomValue(-18, 18), sp1.y + GetRandomValue(-18, 18) };
                        DrawLineEx(sp1, sp2, 2.0f, (GetRandomValue(0, 1) == 0) ? YELLOW : SKYBLUE);
                    }
                }

                if (bossLeftPodHp > 0)
                {
                    DrawRectangleLines(bossPos.x + 8, bossPos.y + 185, 75, 8, DARKGRAY);
                    DrawRectangle(bossPos.x + 8, bossPos.y + 185, (int)(75 * ((float)bossLeftPodHp / BossPodMaxHp)), 8, RED);
                    DrawText(TextFormat("SHIELD: %d", bossLeftPodHp), bossPos.x + 10, bossPos.y + 196, 11, RED);
                }
                else
                {
                    DrawText("BROKEN", bossPos.x + 16, bossPos.y + 190, 12, DARKGRAY);
                }

                if (bossRightPodHp > 0)
                {
                    DrawRectangleLines(bossPos.x + BossWidth - 83, bossPos.y + 185, 75, 8, DARKGRAY);
                    DrawRectangle(bossPos.x + BossWidth - 83, bossPos.y + 185, (int)(75 * ((float)bossRightPodHp / BossPodMaxHp)), 8, MAGENTA);
                    DrawText(TextFormat("SHIELD: %d", bossRightPodHp), bossPos.x + BossWidth - 81, bossPos.y + 196, 11, MAGENTA);
                }
                else
                {
                    DrawText("BROKEN", bossPos.x + BossWidth - 72, bossPos.y + 190, 12, DARKGRAY);
                }

                if (bossLeftPodHp > 0 || bossRightPodHp > 0)
                {
                    DrawCircleLines((int)(bossPos.x + BossWidth / 2.0f), (int)(bossPos.y + 100), 55.0f, Fade(SKYBLUE, 0.45f));
                    DrawCircleLines((int)(bossPos.x + BossWidth / 2.0f), (int)(bossPos.y + 100), 58.0f, Fade(SKYBLUE, 0.25f));
                }

                for (int k = 0; k < MAX_BOSS_BULLETS; k++)
                {
                    if (bossBulletActive[k])
                    {
                        DrawRectangle((int)(bossBulletPos[k].x - AlienBulletWidth / 2.0f), (int)bossBulletPos[k].y, AlienBulletWidth, AlienBulletHeight, RED);
                    }
                }

                // Laser Alignment Guide Beam
                if (!bossLaserActive && bossLaserTimer >= 6.1f && bossLaserTimer < 7.0f)
                {
                    float laserW = 42.0f;
                    float guideX = bossPos.x + BossWidth / 2.0f;
                    float guideY = bossPos.y + BossHeight - 10.0f;

                    float telegraphAlpha = (float)GetRandomValue(25, 65) / 100.0f;
                    DrawLineEx((Vector2){ guideX, guideY }, (Vector2){ guideX, (float)WindowHeight }, 2.5f, Fade(RED, telegraphAlpha));
                    DrawRectangle((int)(guideX - laserW / 2.0f), (int)guideY, (int)laserW, WindowHeight - (int)guideY, Fade(RED, telegraphAlpha * 0.18f));

                    const char* warnBeam = ">> DANGER: LASER ALIGNING <<";
                    int wB = MeasureText(warnBeam, 16);
                    DrawText(warnBeam, (int)(guideX - wB / 2.0f), (int)(guideY + 30.0f), 16, YELLOW);
                }

                if (bossLaserActive)
                {
                    float jitterW = (float)GetRandomValue(-4, 5);
                    float laserW = 42.0f + jitterW;
                    float laserX = bossPos.x + (BossWidth - laserW) / 2.0f;
                    float laserY = bossPos.y + BossHeight - 10.0f;
                    float laserH = WindowHeight - laserY;

                    DrawRectangle((int)laserX - 16, (int)laserY, (int)laserW + 32, (int)laserH, Fade(RED, 0.25f));
                    DrawRectangle((int)laserX - 8, (int)laserY, (int)laserW + 16, (int)laserH, Fade(RED, 0.55f));
                    DrawRectangle((int)laserX, (int)laserY, (int)laserW, (int)laserH, Fade(ORANGE, 0.90f));
                    DrawRectangle((int)laserX + 8, (int)laserY, (int)laserW - 16, (int)laserH, WHITE);

                    for (int p = 0; p < 6; p++)
                    {
                        int sparkY = (int)laserY + GetRandomValue(0, (int)laserH);
                        int sparkX = (int)laserX + GetRandomValue(-12, (int)laserW + 12);
                        DrawCircle(sparkX, sparkY, (float)GetRandomValue(2, 5), YELLOW);
                    }
                }

                for (int k = 0; k < MAX_BOSS_ORBS; k++)
                {
                    if (bossOrbActive[k])
                    {
                        DrawCircle((int)bossOrbPos[k].x, (int)bossOrbPos[k].y, 14.0f, PURPLE);
                        DrawCircle((int)bossOrbPos[k].x, (int)bossOrbPos[k].y, 9.0f, MAGENTA);
                        DrawCircle((int)bossOrbPos[k].x, (int)bossOrbPos[k].y, 4.0f, WHITE);
                    }
                }
            }

            // DRAW CRASH BEACONS and ENERGY CHANNELING
            if (Hero1Lives <= 0 && Hero2Lives > 0)
            {
                DrawCircleLines((int)Hero1CrashPos.x, (int)(Hero1CrashPos.y + 40), 38.0f, Fade(LIME, 0.7f));
                DrawCircle((int)Hero1CrashPos.x, (int)(Hero1CrashPos.y + 40), 6.0f, RED);
                DrawText("SHOAB DOWN", (int)Hero1CrashPos.x - 45, (int)Hero1CrashPos.y - 10, 14, RED);

                if (Hero2Lives > 1)
                {
                    DrawText("FLY HERE TO REVIVE", (int)Hero1CrashPos.x - 65, (int)Hero1CrashPos.y + 85, 13, YELLOW);
                    if (hero1ReviveTimer > 0.0f)
                    {
                        DrawLineEx(Hero2Pos, Hero1CrashPos, 3.0f, Fade(LIME, 0.75f));
                        DrawRectangle((int)Hero1CrashPos.x - 35, (int)Hero1CrashPos.y - 25, (int)(70 * (hero1ReviveTimer / 2.0f)), 8, LIME);
                        DrawRectangleLines((int)Hero1CrashPos.x - 35, (int)Hero1CrashPos.y - 25, 70, 8, WHITE);
                    }
                }
            }

            if (Hero2Lives <= 0 && Hero1Lives > 0)
            {
                DrawCircleLines((int)Hero2CrashPos.x, (int)(Hero2CrashPos.y + 40), 38.0f, Fade(YELLOW, 0.7f));
                DrawCircle((int)Hero2CrashPos.x, (int)(Hero2CrashPos.y + 40), 6.0f, RED);
                DrawText("NAYEMUL DOWN", (int)Hero2CrashPos.x - 52, (int)Hero2CrashPos.y - 10, 14, RED);

                if (Hero1Lives > 1)
                {
                    DrawText("FLY HERE TO REVIVE", (int)Hero2CrashPos.x - 65, (int)Hero2CrashPos.y + 85, 13, YELLOW);
                    if (hero2ReviveTimer > 0.0f)
                    {
                        DrawLineEx(Hero1Pos, Hero2CrashPos, 3.0f, Fade(YELLOW, 0.75f));
                        DrawRectangle((int)Hero2CrashPos.x - 35, (int)Hero2CrashPos.y - 25, (int)(70 * (hero2ReviveTimer / 2.0f)), 8, YELLOW);
                        DrawRectangleLines((int)Hero2CrashPos.x - 35, (int)Hero2CrashPos.y - 25, 70, 8, WHITE);
                    }
                }
            }

            // DRAW HERO 1: SHOAB
            if (Hero1Lives > 0)
            {
                Rectangle h1Dest = { Hero1Pos.x - HeroWidth / 2.0f, Hero1Pos.y, HeroWidth, HeroHeight };
                Color h1Tint = hero1Debuffed ? PURPLE : WHITE;

                DrawTexturePro(HeroTexture, (Rectangle){ 0, 0, (float)HeroTexture.width, (float)HeroTexture.height }, h1Dest, (Vector2){ 0, 0 }, 0.0f, h1Tint);

                const char* tagShoab = "Shoab";
                int t1W = MeasureText(tagShoab, 18);
                DrawText(tagShoab, (int)Hero1Pos.x - t1W / 2, (int)Hero1Pos.y - 24, 18, LIME);

                if (hero1Debuffed)
                {
                    DrawCircleLines((int)Hero1Pos.x, (int)(Hero1Pos.y + HeroHeight / 2.0f), 65.0f, MAGENTA);
                    char dAlert[] = "! WEAPONS JAMMED !";
                    DrawText(dAlert, (int)Hero1Pos.x - MeasureText(dAlert, 14) / 2, (int)Hero1Pos.y - 42, 14, YELLOW);
                }
            }

            // DRAW HERO 2: NAYEMUL[cite: 1]
            if (Hero2Lives > 0)
            {
                Vector2 h2Center = { Hero2Pos.x, Hero2Pos.y + HeroHeight / 2.0f };
                DrawStealthFlames(h2Center, HeroWidth, HeroHeight);

                Rectangle h2Dest = { Hero2Pos.x - HeroWidth / 2.0f, Hero2Pos.y, HeroWidth, HeroHeight };
                Color h2Tint = hero2Debuffed ? PURPLE : WHITE;

                DrawTexturePro(StealthHeroTexture, (Rectangle){ 0, 0, (float)StealthHeroTexture.width, (float)StealthHeroTexture.height }, h2Dest, (Vector2){ 0, 0 }, 0.0f, h2Tint);

                const char* tagNayemul = "Nayemul";
                int t2W = MeasureText(tagNayemul, 18);
                DrawText(tagNayemul, (int)Hero2Pos.x - t2W / 2, (int)Hero2Pos.y - 24, 18, YELLOW);

                if (hero2Debuffed)
                {
                    DrawCircleLines((int)Hero2Pos.x, (int)(Hero2Pos.y + HeroHeight / 2.0f), 65.0f, MAGENTA);
                    char dAlert[] = "! WEAPONS JAMMED !";
                    DrawText(dAlert, (int)Hero2Pos.x - MeasureText(dAlert, 14) / 2, (int)Hero2Pos.y - 42, 14, YELLOW);
                }
            }

            // TOP-LEFT HUD: SHOAB'S REACTIVE PORTRAIT BADGE and STATS
            int b1X = 30, b1Y = 18, bSize = 64;
            DrawRectangle(b1X - 2, b1Y - 2, bSize + 4, bSize + 4, Fade(BLACK, 0.7f));

            if (Hero1Lives <= 0)
            {
                DrawRectangle(b1X, b1Y, bSize, bSize, DARKGRAY);
                for (int s = 0; s < 12; s++)
                {
                    DrawRectangle(b1X + GetRandomValue(0, bSize - 8), b1Y + GetRandomValue(0, bSize - 4), GetRandomValue(6, 14), 2, RAYWHITE);
                }
                DrawRectangleLines(b1X, b1Y, bSize, bSize, RED);
                DrawText("LOST", b1X + 12, b1Y + 24, 15, RED);
            }
            else if (hero1HitFlashTimer > 0.0f)
            {
                if (portraitShoab.id > 0)
                {
                    DrawTexturePro(portraitShoab, (Rectangle){ 0, 0, (float)portraitShoab.width, (float)portraitShoab.height },
                                   (Rectangle){ b1X, b1Y, (float)bSize, (float)bSize }, (Vector2){0,0}, 0.0f, (Color){ 255, 80, 80, 255 });
                }
                else
                {
                    DrawRectangle(b1X, b1Y, bSize, bSize, RED);
                    DrawText("SHOAB", b1X + 6, b1Y + 24, 14, WHITE);
                }
                DrawRectangleLinesEx((Rectangle){ b1X, b1Y, (float)bSize, (float)bSize }, 2.5f, RED);
            }
            else
            {
                if (portraitShoab.id > 0)
                {
                    DrawTexturePro(portraitShoab, (Rectangle){ 0, 0, (float)portraitShoab.width, (float)portraitShoab.height },
                                   (Rectangle){ b1X, b1Y, (float)bSize, (float)bSize }, (Vector2){0,0}, 0.0f, WHITE);
                }
                else
                {
                    DrawRectangle(b1X, b1Y, bSize, bSize, Fade(DARKGREEN, 0.4f));
                    DrawText("SHOAB", b1X + 6, b1Y + 24, 14, LIME);
                }
                DrawRectangleLinesEx((Rectangle){ b1X, b1Y, (float)bSize, (float)bSize }, 2.0f, LIME);
            }

            int text1X = b1X + bSize + 14;
            DrawText("PILOT 1: SHOAB", text1X, 18, 20, LIME);

            // Null-Vector Jammer Radar Disruption: Hides HUD status
            if (radarJammedTimer > 0.0f)
            {
                DrawRectangle(text1X - 4, 40, 240, 68, Fade(BLACK, 0.88f));
                DrawRectangleLines(text1X - 4, 40, 240, 68, Fade((Color){ 200, 80, 255, 255 }, 0.8f));
                DrawText("[ RADAR JAMMED ]", text1X + 10, 48, 17, (Color){ 200, 80, 255, 255 });
                DrawText(TextFormat("SIGNAL LOST // 0x%04X", GetRandomValue(0x1000, 0xFFFF)), text1X + 10, 72, 14, RED);
            }
            else
            {
                DrawText(TextFormat("LIVES: %d / 4", Hero1Lives), text1X, 42, 20, (Hero1Lives <= 1) ? RED : LIME);
                DrawText(TextFormat("SCORE: %05d", Hero1Score), text1X, 66, 20, (Color){ 180, 255, 180, 255 });
                DrawText("[A/D] Move  |  [W/SPACE] Shoot", text1X, 90, 14, LIGHTGRAY);
            }

            // SHOAB SPECIAL ABILITY HUD STATUS
            if (ShoabSpecialTime >= FPS * 15.0f)
            {
                if (!SpecialReady) PlaySound(sndChargeReady);
                DrawText("[Q] SPECIAL READY", 30, 122, 14, LIME);
                SpecialReady = true;
            }
            else
            {
                DrawText(TextFormat("[Q] SPECIAL READY IN %.1f s", 15.0f - ShoabSpecialTime / FPS), 30, 122, 14, RED);
                SpecialReady = false;
            }

            // TOP-RIGHT HUD: NAYEMUL'S REACTIVE PORTRAIT BADGE and STATS[cite: 1]
            int b2X = WindowWidth - 30 - bSize, b2Y = 18;
            DrawRectangle(b2X - 2, b2Y - 2, bSize + 4, bSize + 4, Fade(BLACK, 0.7f));

            if (Hero2Lives <= 0)
            {
                DrawRectangle(b2X, b2Y, bSize, bSize, DARKGRAY);
                for (int s = 0; s < 12; s++)
                {
                    DrawRectangle(b2X + GetRandomValue(0, bSize - 8), b2Y + GetRandomValue(0, bSize - 4), GetRandomValue(6, 14), 2, RAYWHITE);
                }
                DrawRectangleLines(b2X, b2Y, bSize, bSize, RED);
                DrawText("LOST", b2X + 12, b2Y + 24, 15, RED);
            }
            else if (hero2HitFlashTimer > 0.0f)
            {
                if (portraitNayemul.id > 0)
                {
                    DrawTexturePro(portraitNayemul, (Rectangle){ 0, 0, (float)portraitNayemul.width, (float)portraitNayemul.height },
                                   (Rectangle){ b2X, b2Y, (float)bSize, (float)bSize }, (Vector2){0,0}, 0.0f, (Color){ 255, 80, 80, 255 });
                }
                else
                {
                    DrawRectangle(b2X, b2Y, bSize, bSize, RED);
                    DrawText("NAYEMUL", b2X + 2, b2Y + 24, 13, WHITE);
                }
                DrawRectangleLinesEx((Rectangle){ b2X, b2Y, (float)bSize, (float)bSize }, 2.5f, RED);
            }
            else
            {
                if (portraitNayemul.id > 0)
                {
                    DrawTexturePro(portraitNayemul, (Rectangle){ 0, 0, (float)portraitNayemul.width, (float)portraitNayemul.height },
                                   (Rectangle){ b2X, b2Y, (float)bSize, (float)bSize }, (Vector2){0,0}, 0.0f, WHITE);
                }
                else
                {
                    DrawRectangle(b2X, b2Y, bSize, bSize, Fade(DARKBROWN, 0.4f));
                    DrawText("NAYEMUL", b2X + 2, b2Y + 24, 13, YELLOW);
                }
                DrawRectangleLinesEx((Rectangle){ b2X, b2Y, (float)bSize, (float)bSize }, 2.0f, YELLOW);
            }

            const char* pilot2Title = "PILOT 2: NAYEMUL";
            int p2tW = MeasureText(pilot2Title, 20);
            DrawText(pilot2Title, b2X - 14 - p2tW, 18, 20, YELLOW);

            // Null-Vector Jammer Radar Disruption: Hides HUD status
            if (radarJammedTimer > 0.0f)
            {
                int jamW = 240;
                DrawRectangle(b2X - 14 - jamW, 40, jamW, 68, Fade(BLACK, 0.88f));
                DrawRectangleLines(b2X - 14 - jamW, 40, jamW, 68, Fade((Color){ 200, 80, 255, 255 }, 0.8f));
                DrawText("[ RADAR JAMMED ]", b2X - jamW, 48, 17, (Color){ 200, 80, 255, 255 });
                DrawText(TextFormat("SIGNAL LOST // 0x%04X", GetRandomValue(0x1000, 0xFFFF)), b2X - jamW, 72, 14, RED);
            }
            else
            {
                const char* p2LivesText = TextFormat("LIVES: %d / 4", Hero2Lives);
                int p2lW = MeasureText(p2LivesText, 20);
                DrawText(p2LivesText, b2X - 14 - p2lW, 42, 20, (Hero2Lives <= 1) ? RED : YELLOW);

                const char* p2ScoreText = TextFormat("SCORE: %05d", Hero2Score);
                int p2sW = MeasureText(p2ScoreText, 20);
                DrawText(p2ScoreText, b2X - 14 - p2sW, 66, 20, (Color){ 255, 245, 160, 255 });

                const char* p2Controls = "[ARROWS] Move  |  [UP] Shoot";
                int p2cW = MeasureText(p2Controls, 14);
                DrawText(p2Controls, b2X - 14 - p2cW, 90, 14, LIGHTGRAY);
            }

            if (empBuffTimer > 0.0f)
            {
                const char* empStatus = TextFormat("⚡ RAPID PLASMA OVERCHARGE: %.1fs ⚡", empBuffTimer);
                int empW = MeasureText(empStatus, 18);
                DrawText(empStatus, (WindowWidth - empW) / 2, 92, 18, LIME);
            }

            // TOP-CENTER: BOSS HP BAR
            const char* pauseNotice = "[P] PAUSE   |   [F11] FULLSCREEN   |   [ESC] MENU";
            int pauseW = MeasureText(pauseNotice, 15);

            if (bossActive)
            {
                int barW = 460;
                int barH = 22;
                int barX = (WindowWidth - barW) / 2;
                int barY = 36;

                const char* bossHeader = (bossHp <= 40) ? "!! DREADNOUGHT ENRAGED (PHASE 2) !!" : "DREADNOUGHT CARRIER BOSS";
                int hpTextW = MeasureText(bossHeader, 18);
                DrawText(bossHeader, (WindowWidth - hpTextW) / 2, 14, 18, (bossHp <= 40) ? YELLOW : RED);

                DrawRectangle(barX - 3, barY - 3, barW + 6, barH + 6, DARKGRAY);
                DrawRectangle(barX, barY, (int)(barW * ((float)bossHp / 100.0f)), barH, MAROON);
                DrawRectangle(barX, barY, (int)(barW * ((float)bossHp / 100.0f)), barH / 2, RED);
                DrawRectangleLines(barX, barY, barW, barH, WHITE);

                const char* shieldState = (bossLeftPodHp <= 0 && bossRightPodHp <= 0) ? "[CORE EXPOSED]" : "[IMMUNE - SHIELDED]";
                DrawText(TextFormat("%d / 100  %s", bossHp, shieldState), barX + barW / 2 - 75, barY + 3, 16, YELLOW);

                DrawText(pauseNotice, (WindowWidth - pauseW) / 2, 68, 15, LIGHTGRAY);
            }
            else
            {
                DrawText(pauseNotice, (WindowWidth - pauseW) / 2, 22, 16, DARKGRAY);
            }
        }

        EndMode2D();

        if (ScanlinesOn)
        {
            for (int y = 0; y < WindowHeight; y += 4)
            {
                DrawRectangle(0, y, WindowWidth, 1, Fade(BLACK, 0.22f));
            }
        }

        if (Brightness < 1.0f)
        {
            DrawRectangle(0, 0, WindowWidth, WindowHeight, Fade(BLACK, 1.0f - Brightness));
        }
        else if (Brightness > 1.0f)
        {
            DrawRectangle(0, 0, WindowWidth, WindowHeight, Fade(WHITE, (Brightness - 1.0f) * 0.25f));
        }

        EndDrawing();
    }

    // Cleanup
    UnloadTexture(HeroTexture);
    UnloadTexture(StealthHeroTexture);
    if (loadingBg.id > 0) UnloadTexture(loadingBg);
    if (portraitShoab.id > 0) UnloadTexture(portraitShoab);
    if (portraitNayemul.id > 0) UnloadTexture(portraitNayemul);

    for (int c = 0; c < 4; c++)
    {
        UnloadTexture(jammerTex[c]);
        UnloadTexture(warpTex[c]);
    }

    UnloadSound(shoot);
    UnloadSound(AlienShoot);
    UnloadSound(menuMove);
    UnloadSound(menuSelect);
    UnloadSound(heroDeath);
    UnloadSound(pauseIn);
    UnloadSound(pauseOut);
    UnloadSound(damage);
    UnloadSound(cheer);
    UnloadSound(gameWin);
    UnloadSound(gameOverChild);
    UnloadSound(gameOverCommunity);
    UnloadSound(heroOuch);
    UnloadSound(bossLaserSound);

    UnloadSound(sndJammerHum);
    UnloadSound(sndJammerShot);
    UnloadSound(sndEmpBlast);
    UnloadSound(sndJammerDeath);
    UnloadSound(sndWarpGlide);
    UnloadSound(sndWarpShot);
    UnloadSound(sndTeleport);
    UnloadSound(sndWarpDeath);

    UnloadSound(sndSpecialBeam);
    UnloadSound(sndChargeReady);
    UnloadSound(sndPodDestroy);
    UnloadSound(sndOrbLaunch);
    UnloadSound(sndWarningSiren);
    UnloadSound(sndDefibHum);
    UnloadSound(sndBossEnrage);
    UnloadSound(sndRocketBoost);

    UnloadMusicStream(bgmStory);
    UnloadMusicStream(bgmMenu);
    UnloadMusicStream(bgmBoss);
    UnloadMusicStream(bgmWin);
    UnloadMusicStream(bgmLost);

    for (int b = 0; b < 5; b++)
    {
        UnloadTexture(BossTexture[b]);
    }

    for (int Asprite = 0; Asprite < AlienSprite; Asprite++)
    {
        UnloadTexture(AlienTexture[Asprite]);
    }

    CloseAudioDevice();
    CloseWindow();
    return 0;
}
