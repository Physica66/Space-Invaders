#include "raylib/raylib-6.0_macos/include/raylib.h"
#include "raylib/raylib-6.0_macos/include/raymath.h"
#include <stdbool.h>
#include <stdio.h>
#include <math.h>

#define WindowWidth 1500
#define WindowHeight 900

#define AlienSize 50
#define AlienDistance 50
#define AlienSpeedX 20
#define AlienSpeedY 0
#define AlienSprite 3
#define AlienSpriteStyle 2

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
#define BossWidth 380
#define BossHeight 320
#define MAX_BOSS_BULLETS 24
#define MAX_BOSS_ORBS 12

// Game Stats
#define STATE_LOADING 0
#define STATE_MENU 1
#define STATE_GAMEPLAY 2
#define STATE_OPTIONS 3
#define STATE_CREDITS 4
#define STATE_STORY 5
#define STATE_LAUNCH 6

// Visual Starfield while loading,it just gives some vibes :)
#define STAR_COUNT 90

void DownAlien(int AlienInX, int AlienInY, Vector2 AlienPos[AlienInX][AlienInY])
{
    for (int X = 0; X < AlienInX; X++)
    {
        for (int Y = 0; Y < AlienInY; Y++)
        {
            AlienPos[X][Y].y += AlienSize / 2.0f;
        }
    }
}

// Dynamic afterburner jet flame renderer for the Stealth Bomber ;Hero2
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
    InitWindow(WindowWidth, WindowHeight, "Space Invaders - Earth Defense Fleet");
    InitAudioDevice();
    SetTargetFPS(60);

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
    for (int X = 0; X < AlienInX; X++)
    {
        for (int Y = 0; Y < AlienInY; Y++)
        {
            AlienPos[X][Y].x = AlienPos[0][0].x + (AlienSize + AlienDistance) * X;
            AlienPos[X][Y].y = AlienPos[0][0].y + (AlienSize + AlienDistance) * Y;
            AlienAlive[X][Y] = true;
        }
    }

    Texture2D AlienTexture[AlienSprite][AlienSpriteStyle];
    Sound shoot = LoadSound("assets/audio/alienshoot2.wav");
    Sound AlienShoot = LoadSound("assets/audio/alienshoot1.wav");

    Sound menuMove = LoadSound("assets/audio/sfx_menu_move4.wav");
    Sound menuSelect = LoadSound("assets/audio/sfx_menu_select2.wav");
    Sound heroDeath = LoadSound("assets/audio/sfx_deathscream_robot4.wav");
    Sound pauseIn = LoadSound("assets/audio/sfx_sounds_pause4_in.wav");
    Sound pauseOut = LoadSound("assets/audio/sfx_sounds_pause4_out.wav");
    Sound damage = LoadSound("assets/audio/sfx_sounds_damage3.wav");

    // 4 newly added sounds
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
    Music bgmMenu = LoadMusicStream("assets/audio/air-strike-3-d-ii-gulf-thunder-main-ost-best-quality-mf-0-j-34.wav");
    Music bgmBoss = LoadMusicStream("assets/audio/air-strike-3-d-ost-fear-drigto.wav");

    // Boss Textures
    Texture2D BossTexture[5];
    BossTexture[0] = LoadTexture("assets/sprites/boss_idle.png");
    BossTexture[1] = LoadTexture("assets/sprites/boss_down.png");
    BossTexture[2] = LoadTexture("assets/sprites/boss_up.png");
    BossTexture[3] = LoadTexture("assets/sprites/boss_pulse1.png");
    BossTexture[4] = LoadTexture("assets/sprites/boss_pulse2.png");

    Texture2D HeroTexture = LoadTexture("assets/sprites/Hero.png");
    Texture2D StealthHeroTexture = LoadTexture("assets/sprites/Hero_stealth.png");
    AlienTexture[0][0] = LoadTexture("assets/sprites/Alien1style1.png");
    AlienTexture[0][1] = LoadTexture("assets/sprites/Alien1style2.png");
    AlienTexture[1][0] = LoadTexture("assets/sprites/Alien2style1.png");
    AlienTexture[1][1] = LoadTexture("assets/sprites/Alien2style2.png");
    AlienTexture[2][0] = LoadTexture("assets/sprites/Alien3style1.png");
    AlienTexture[2][1] = LoadTexture("assets/sprites/Alien3style2.png");

    // Screen Shake Camera
    Camera2D screenCamera = { 0 };
    screenCamera.zoom = 1.0f;

    // HERO 1 (Shoab - Left Side, A/D movement, W / Space shoot) - 4 Lives
    int Hero1Lives = 4;
    int Hero1Score = 0;
    Vector2 Hero1Pos = { WindowWidth * 0.35f, WindowHeight - HeroHeight };
    Vector2 Hero1Speed = { 0, 0 };
    Vector2 Hero1BulletPos[2] = { {0, 0}, {0, 0} };
    bool Hero1BulletActive[2] = { false, false };
    bool hero1Debuffed = false;
    float hero1DebuffTimer = 0.0f;

    // HERO 2 (Nayemul - Right Side, Arrow keys movement, UP arrow shoot) - 4 Lives
    int Hero2Lives = 4;
    int Hero2Score = 0;
    Vector2 Hero2Pos = { WindowWidth * 0.65f, WindowHeight - HeroHeight };
    Vector2 Hero2Speed = { 0, 0 };
    Vector2 Hero2BulletPos[2] = { {0, 0}, {0, 0} };
    bool Hero2BulletActive[2] = { false, false };
    bool hero2Debuffed = false;
    float hero2DebuffTimer = 0.0f;

    // Alien bullets and shooting timer
    Vector2 AlienBulletPos = { 0, 0 };
    bool AlienBulletActive = false;
    float AlienShootTimer = 0.0f;

    // Game state
    int AliensKilled = 0;
    bool GameOver = false;

    // for sound and pause
    bool isPaused = false;
    float deathDelayTimer = 0.0f;
    bool deathSequenceActive = false;

    // Sound sequence tracking flags
    bool cheerPlayed = false;
    bool winSoundPlayed = false;
    bool gameOverChildPlayed = false;
    bool gameOverCommPlayed = false;
    float gameOverSoundTimer = 0.0f;

    // Story Dialogues state 
    int storyDialogueIndex = 0;

    // Pre-Gameplay Launch Sequence State
    int launchDialogueIndex = 0;
    float launchCountdownTimer = 3.0f;
    bool launchCountdownActive = false;
    Vector2 launchShip1Pos = { WindowWidth * 0.40f, WindowHeight - 240 };
    Vector2 launchShip2Pos = { WindowWidth * 0.60f, WindowHeight - 240 };

    // Boss State Management
    bool bossSpawned = false;
    bool bossWarningActive = false;
    float bossWarningTimer = 0.0f;
    bool bossActive = false;
    bool bossDefeated = false;
    int bossHp = 100;
    Vector2 bossPos = { (WindowWidth - BossWidth) / 2.0f, 60.0f };
    Vector2 bossSpeed = { 160.0f, 75.0f };
    float bossAnimTimer = 0.0f;
    float bossDirChangeTimer = 0.0f;

    // boss attacxk 1: Triple standard bullets
    float bossShootTimer = 0.0f;
    Vector2 bossBulletPos[MAX_BOSS_BULLETS];
    bool bossBulletActive[MAX_BOSS_BULLETS] = { false };

    // Boss attack 2: Repeating Giant Laser Beam every 7s (persists 2s)
    float bossLaserTimer = 0.0f;
    bool bossLaserActive = false;
    float bossLaserDuration = 0.0f;

    // Boss Attack 3: 6 Circular Projectiles every 4s (causes jamming & slow)
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
        float Time = GetFrameTime();

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

        SetMusicVolume(bgmStory, BgmVolume);
        SetMusicVolume(bgmMenu, BgmVolume);
        SetMusicVolume(bgmBoss, BgmVolume);

        // Screen Shake during boss laser deployment
        if (bossLaserActive && CurrentState == STATE_GAMEPLAY)
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
        if (StarfieldOn)
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

            char titleText[] = "SPACE INVADERS";
            int titleWidth = MeasureText(titleText, 65);
            DrawText(titleText, (WindowWidth - titleWidth) / 2, 280, 65, SKYBLUE);

            char subText[] = "INITIALIZING DEFENSE SATELLITES & RADAR...";
            int subWidth = MeasureText(subText, 22);
            DrawText(subText, (WindowWidth - subWidth) / 2, 375, 22, GREEN);

            int barWidth = 600;
            int barHeight = 36;
            int barX = (WindowWidth - barWidth) / 2;
            int barY = 440;

            DrawRectangleLines(barX - 4, barY - 4, barWidth + 8, barHeight + 8, DARKBLUE);
            DrawRectangle(barX, barY, (int)(barWidth * progress), barHeight, LIME);

            DrawText(TextFormat("%d%%", (int)(progress * 100)), barX + barWidth / 2 - 25, barY + 7, 22, BLACK);

            char hintText[] = "System is preparing pure C runtime environment...";
            int hintWidth = MeasureText(hintText, 18);
            DrawText(hintText, (WindowWidth - hintWidth) / 2, 510, 18, LIGHTGRAY);

            if (LoadingTimer >= 5.0f)
            {
                CurrentState = STATE_STORY;
                PlayMusicStream(bgmStory);
            }
        }

        // STATE: STORY DIALOGUES (POST-LOADING)
        else if (CurrentState == STATE_STORY)
        {
            UpdateMusicStream(bgmStory);

            int panelW = 1200;
            int panelH = 500;
            int panelX = (WindowWidth - panelW) / 2;
            int panelY = (WindowHeight - panelH) / 2;

            DrawRectangle(panelX, panelY, panelW, panelH, Fade(DARKPURPLE, 0.40f));
            DrawRectangleLines(panelX, panelY, panelW, panelH, SKYBLUE);
            DrawRectangleLines(panelX + 6, panelY + 6, panelW - 12, panelH - 12, DARKBLUE);

            char headerText[] = "GLOBAL EMERGENCY TRANSMISSION";
            int hW = MeasureText(headerText, 38);
            DrawText(headerText, (WindowWidth - hW) / 2, panelY + 35, 38, GOLD);
            DrawLine(panelX + 80, panelY + 85, panelX + panelW - 80, panelY + 85, SKYBLUE);

            if (storyDialogueIndex == 0)
            {
                DrawText("[ GLOBAL BROADCAST NETWORK ]", panelX + 70, panelY + 125, 26, RED);
                DrawText("URGENT BULLETIN: Planet Earth is completely surrounded by hostile alien fleets!", panelX + 70, panelY + 185, 22, RAYWHITE);
                DrawText("News channels report worldwide chaos and terror. Orbital defense satellites have been obliterated.", panelX + 70, panelY + 225, 22, LIGHTGRAY);
                DrawText("Civilization stands at the brink of total annihilation as enemy armadas descend...", panelX + 70, panelY + 265, 22, LIGHTGRAY);
            }
            else if (storyDialogueIndex == 1)
            {
                DrawText("[ NAYEMUL & SHOAB - EARTH DEFENSE ALLIANCE ]", panelX + 70, panelY + 125, 26, YELLOW);
                DrawText("Nayemul: \"Do not lose hope! We will defend our homeworld and annihilate every single invader!\"", panelX + 70, panelY + 185, 22, SKYBLUE);
                DrawText("Shoab: \"We've monitored their grid formation. We are not letting humanity fall today!\"", panelX + 70, panelY + 240, 22, LIME);
            }
            else if (storyDialogueIndex == 2)
            {
                DrawText("[ DEFENSE FLEET - HANGAR DECK ]", panelX + 70, panelY + 125, 26, YELLOW);
                DrawText("Shoab: \"Interceptors are fully prepped, shields calibrated, and dual plasma cannons loaded!\"", panelX + 70, panelY + 185, 22, LIME);
                DrawText("Nayemul: \"Sub-space thrusters at maximum thrust. Earth Alliance interceptors launching NOW!\"", panelX + 70, panelY + 240, 22, SKYBLUE);
            }
            else if (storyDialogueIndex == 3)
            {
                DrawText("[ INCOMING ENCRYPTED THREAT TRANSMISSION ]", panelX + 70, panelY + 125, 26, MAROON);
                DrawText("Alien Dreadnought Boss:", panelX + 70, panelY + 185, 24, RED);
                DrawText("\"PUNY MORTALS! Nayemul... Shoab... heed this final warning. Do NOT dare enter this airspace!\"", panelX + 70, panelY + 230, 22, RED);
                DrawText("\"You fly directly into your own extinction! Your fleet will burn to ash in our plasma wake!\"", panelX + 70, panelY + 270, 22, ORANGE);
            }

            char nextPrompt[] = "PRESS [ENTER] OR [SPACE] TO CONTINUE";
            int pW = MeasureText(nextPrompt, 20);
            if (((int)(GetTime() * 3)) % 2 == 0)
            {
                DrawText(nextPrompt, (WindowWidth - pW) / 2, panelY + panelH - 55, 20, GREEN);
            }

            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE))
            {
                PlaySound(menuSelect);
                storyDialogueIndex++;
                if (storyDialogueIndex > 3)
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

            // Draw curved Earth from space
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

            // Draw Hero 1 (Shoab) Ship angled outward
            Rectangle h1Src = { 0, 0, (float)HeroTexture.width, (float)HeroTexture.height };
            Rectangle h1Dest = { launchShip1Pos.x, launchShip1Pos.y, HeroWidth, HeroHeight };
            DrawTexturePro(HeroTexture, h1Src, h1Dest, (Vector2){ HeroWidth / 2.0f, HeroHeight / 2.0f }, -12.0f, WHITE);

            // Draw Hero 2 (Nayemul) Stealth Bomber angled outward with afterburners
            DrawStealthFlames(launchShip2Pos, HeroWidth, HeroHeight);
            Rectangle h2Src = { 0, 0, (float)StealthHeroTexture.width, (float)StealthHeroTexture.height };
            Rectangle h2Dest = { launchShip2Pos.x, launchShip2Pos.y, HeroWidth, HeroHeight };
            DrawTexturePro(StealthHeroTexture, h2Src, h2Dest, (Vector2){ HeroWidth / 2.0f, HeroHeight / 2.0f }, 12.0f, WHITE);

            // Dialogue Box
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
                DrawText("[ PRE-FLIGHT AUTHORIZATION - DUAL STRIKE FLEET ]", dlgX + 40, dlgY + 25, 24, GOLD);
                DrawText("Shoab: \"Plasma cannons energized and hull shields synchronized. Remember the formation:\"", dlgX + 40, dlgY + 75, 22, LIME);
                DrawText("\"I will control the left sector [A/D to Move, W to Fire]. You take the right sector!\"", dlgX + 40, dlgY + 115, 20, LIGHTGRAY);
                DrawText("Nayemul: \"Understood! Stealth wings locked [Arrow Keys to Move, UP to Fire]. Let's ride!\"", dlgX + 40, dlgY + 160, 22, YELLOW);
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
                    if (launchDialogueIndex > 1)
                    {
                        launchCountdownActive = true;
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

                char bgmTracks[3][32] = { "not yet", "not yet1", ":(" };

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
                if (IsKeyPressed(KEY_UP))   { PlaySound(menuMove); VideoSelection--; if (VideoSelection < 0) VideoSelection = 2; }
                if (IsKeyPressed(KEY_DOWN)) { PlaySound(menuMove); VideoSelection++; if (VideoSelection > 2) VideoSelection = 0; }

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

                int rowY = panelY + 240;
                DrawText("DISPLAY BRIGHTNESS", panelX + 160, rowY, 24, (VideoSelection == 0) ? YELLOW : WHITE);
                DrawRectangle(panelX + 540, rowY + 4, 380, 20, DARKGRAY);
                DrawRectangle(panelX + 540, rowY + 4, (int)(380 * ((Brightness - 0.5f) / 1.0f)), 20, SKYBLUE);
                DrawRectangleLines(panelX + 540, rowY + 4, 380, 20, WHITE);
                DrawText(TextFormat("%d%%", (int)(Brightness * 100)), panelX + 940, rowY, 22, YELLOW);

                rowY += 100;
                DrawText("CRT SCANLINE FILTER", panelX + 160, rowY, 24, (VideoSelection == 1) ? YELLOW : WHITE);
                DrawText(ScanlinesOn ? "[ ENABLED ]" : "[ DISABLED ]", panelX + 540, rowY, 24, ScanlinesOn ? LIME : RED);

                rowY += 100;
                DrawText("SPACE STARFIELD ENGINE", panelX + 160, rowY, 24, (VideoSelection == 2) ? YELLOW : WHITE);
                DrawText(StarfieldOn ? "[ ACTIVE ]" : "[ OFFLINE ]", panelX + 540, rowY, 24, StarfieldOn ? LIME : RED);
            }

            char optFooter[] = "[TAB] SWITCH TAB   |   [LEFT / RIGHT] ADJUST   |   [BACKSPACE / ESC] RETURN";
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

            // Right Card: Nayemul Islam
            int card2X = panelX + 660;
            DrawRectangle(card2X, cardY, cardW, cardH, Fade(BLACK, 0.75f));
            DrawRectangleLines(card2X, cardY, cardW, cardH, SKYBLUE);

            DrawText("Nayemul Islam", card2X + 35, cardY + 30, 26, YELLOW);
            DrawText("Roll ID: 2505087", card2X + 35, cardY + 70, 20, GREEN);
            DrawLine(card2X + 35, cardY + 102, card2X + cardW - 35, cardY + 102, DARKGRAY);

            DrawText("Contributions:", card2X + 35, cardY + 120, 19, RAYWHITE);
            DrawText("- Hero and Alien Shooting Logic", card2X + 35, cardY + 155, 17, LIGHTGRAY);
            DrawText("- Collision检测", card2X + 35, cardY + 185, 17, LIGHTGRAY);
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
                            int AStyle = (int)(GetTime() / 0.1) % 2;
                            DrawTexturePro(AlienTexture[Y % AlienSprite][AStyle],
                                (Rectangle){ 0, 0, (float)AlienTexture[Y % AlienSprite][AStyle].width, (float)AlienTexture[Y % AlienSprite][AStyle].height },
                                Alien, (Vector2){ 0, 0 }, 0.0f, WHITE);
                        }
                    }
                }

                if (bossActive)
                {
                    Rectangle bossRec = { bossPos.x, bossPos.y, BossWidth, BossHeight };
                    DrawTexturePro(BossTexture[0], (Rectangle){ 0, 0, (float)BossTexture[0].width, (float)BossTexture[0].height }, bossRec, (Vector2){ 0, 0 }, 0.0f, WHITE);
                }

                for (int i = 0; i < 2; i++)
                {
                    if (Hero1BulletActive[i]) DrawRectangle((int)(Hero1BulletPos[i].x - BulletWidth / 2.0f), (int)Hero1BulletPos[i].y, BulletWidth, BulletHeight, YELLOW);
                    if (Hero2BulletActive[i]) DrawRectangle((int)(Hero2BulletPos[i].x - BulletWidth / 2.0f), (int)Hero2BulletPos[i].y, BulletWidth, BulletHeight, SKYBLUE);
                }

                if (AlienBulletActive) DrawRectangle((int)(AlienBulletPos.x - AlienBulletWidth / 2.0f), (int)AlienBulletPos.y, AlienBulletWidth, AlienBulletHeight, RED);

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

                char pauseSub[] = "Press [P] to Resume  |  [ESC] for Menu";
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

            // Hero-death scream sequence
            if (deathSequenceActive)
            {
                deathDelayTimer += Time;
                if (deathDelayTimer >= 1.2f)
                {
                    deathSequenceActive = false;
                    GameOver = true;
                    StopMusicStream(bgmBoss);
                    StopSound(bossLaserSound);
                }
            }

            if (IsKeyPressed(KEY_ESCAPE) && !deathSequenceActive)
            {
                PlaySound(menuSelect);
                StopMusicStream(bgmBoss);
                StopSound(bossLaserSound);
                CurrentState = STATE_MENU;
                PlayMusicStream(bgmMenu);
            }

            // GAME OVER POPUP and RESTART
            if (GameOver)
            {
                if (!gameOverChildPlayed)
                {
                    PlaySound(gameOverChild);
                    gameOverChildPlayed = true;
                    gameOverSoundTimer = 0.0f;
                }
                else if (!gameOverCommPlayed)
                {
                    gameOverSoundTimer += Time;
                    if (gameOverSoundTimer >= 1.5f)
                    {
                        PlaySound(gameOverCommunity);
                        gameOverCommPlayed = true;
                    }
                }

                int popupWidth = 500;
                int popupHeight = 240;
                int popupX = (WindowWidth - popupWidth) / 2;
                int popupY = (WindowHeight - popupHeight) / 2;

                DrawRectangle(popupX, popupY, popupWidth, popupHeight, Fade(DARKGRAY, 0.95f));
                DrawRectangleLines(popupX, popupY, popupWidth, popupHeight, RED);

                char titleText[] = "GAME OVER";
                DrawText(titleText, popupX + (popupWidth - MeasureText(titleText, 45)) / 2, popupY + 35, 45, RED);

                char subText[] = "Both heroes lost all lives! You Are Noobs!!";
                DrawText(subText, popupX + (popupWidth - MeasureText(subText, 22)) / 2, popupY + 105, 22, WHITE);

                char restartText[] = "Press [R] to Restart  |  [ESC] Menu";
                DrawText(restartText, popupX + (popupWidth - MeasureText(restartText, 20)) / 2, popupY + 160, 20, YELLOW);

                if (IsKeyPressed(KEY_R))
                {
                    PlaySound(menuSelect);
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
                    gameOverChildPlayed = false;
                    gameOverCommPlayed = false;
                    gameOverSoundTimer = 0.0f;

                    bossSpawned = false;
                    bossWarningActive = false;
                    bossWarningTimer = 0.0f;
                    bossActive = false;
                    bossDefeated = false;
                    bossHp = 100;
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

                    StopMusicStream(bgmBoss);
                    StopSound(bossLaserSound);

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

                    for (int X = 0; X < AlienInX; X++)
                    {
                        for (int Y = 0; Y < AlienInY; Y++)
                        {
                            AlienPos[X][Y].x = AlienPos[0][0].x + (AlienSize + AlienDistance) * X;
                            AlienPos[X][Y].y = AlienPos[0][0].y + (AlienSize + AlienDistance) * Y;
                            AlienAlive[X][Y] = true;
                        }
                    }
                }

                EndMode2D();
                EndDrawing();
                continue;
            }

            // Game Won
            if (bossDefeated)
            {
                StopMusicStream(bgmBoss);
                StopSound(bossLaserSound);
                if (!winSoundPlayed)
                {
                    PlaySound(gameWin);
                    winSoundPlayed = true;
                }

                int popupWidth = 500;
                int popupHeight = 240;
                int popupX = (WindowWidth - popupWidth) / 2;
                int popupY = (WindowHeight - popupHeight) / 2;

                DrawRectangle(popupX, popupY, popupWidth, popupHeight, Fade(DARKGRAY, 0.95f));
                DrawRectangleLines(popupX, popupY, popupWidth, popupHeight, RED);

                char titleText[] = "GAME WON";
                DrawText(titleText, popupX + (popupWidth - MeasureText(titleText, 45)) / 2, popupY + 35, 45, GREEN);

                char subText[] = "You defeated the Alien Dreadnought Boss! ;)";
                DrawText(subText, popupX + (popupWidth - MeasureText(subText, 22)) / 2, popupY + 105, 22, WHITE);

                char restartText[] = "Press [R] to Restart  |  [ESC] Menu";
                DrawText(restartText, popupX + (popupWidth - MeasureText(restartText, 20)) / 2, popupY + 160, 20, YELLOW);

                if (IsKeyPressed(KEY_R))
                {
                    PlaySound(menuSelect);
                    AliensKilled = 0;
                    Hero1Lives = 4;
                    Hero2Lives = 4;
                    Hero1Score = 0;
                    Hero2Score = 0;
                    cheerPlayed = false;
                    winSoundPlayed = false;
                    gameOverChildPlayed = false;
                    gameOverCommPlayed = false;
                    gameOverSoundTimer = 0.0f;

                    bossSpawned = false;
                    bossWarningActive = false;
                    bossWarningTimer = 0.0f;
                    bossActive = false;
                    bossDefeated = false;
                    bossHp = 100;
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
                    StopSound(bossLaserSound);

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

                    for (int X = 0; X < AlienInX; X++)
                    {
                        for (int Y = 0; Y < AlienInY; Y++)
                        {
                            AlienPos[X][Y].x = AlienPos[0][0].x + (AlienSize + AlienDistance) * X;
                            AlienPos[X][Y].y = AlienPos[0][0].y + (AlienSize + AlienDistance) * Y;
                            AlienAlive[X][Y] = true;
                        }
                    }
                }

                EndMode2D();
                EndDrawing();
                continue;
            }

            // Boss Trigger check
            if (AliensKilled == AlienInX * AlienInY && !bossSpawned)
            {
                bossSpawned = true;
                bossWarningActive = true;
                bossWarningTimer = 0.0f;
                PlayMusicStream(bgmBoss);
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

                if (bossWarningTimer >= 3.0f)
                {
                    bossWarningActive = false;
                    bossActive = true;
                    bossHp = 100;
                    bossPos = (Vector2){ (WindowWidth - BossWidth) / 2.0f, 60.0f };
                    bossLaserTimer = 0.0f;
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

            // HERO CONTROLS & MOVEMENT
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

            // Boundary clamping
            float minX = HeroWidth / 2.0f;
            float maxX = WindowWidth - HeroWidth / 2.0f;
            if (Hero1Pos.x < minX) Hero1Pos.x = minX;
            if (Hero1Pos.x > maxX) Hero1Pos.x = maxX;
            if (Hero2Pos.x < minX) Hero2Pos.x = minX;
            if (Hero2Pos.x > maxX) Hero2Pos.x = maxX;

            // MUTUAL PUSH COLLISION
            if (Hero1Lives > 0 && Hero2Lives > 0)
            {
                float distBetweenHeroes = Hero2Pos.x - Hero1Pos.x;
                if (fabsf(distBetweenHeroes) < HeroWidth)
                {
                    float overlap = HeroWidth - fabsf(distBetweenHeroes);
                    if (distBetweenHeroes >= 0)
                    {
                        Hero1Pos.x -= overlap / 2.0f;
                        Hero2Pos.x += overlap / 2.0f;
                    }
                    else
                    {
                        Hero1Pos.x += overlap / 2.0f;
                        Hero2Pos.x -= overlap / 2.0f;
                    }

                    if (Hero1Pos.x < minX)
                    {
                        Hero1Pos.x = minX;
                        if (Hero2Pos.x < Hero1Pos.x + HeroWidth) Hero2Pos.x = Hero1Pos.x + HeroWidth;
                    }
                    if (Hero2Pos.x > maxX)
                    {
                        Hero2Pos.x = maxX;
                        if (Hero1Pos.x > Hero2Pos.x - HeroWidth) Hero1Pos.x = Hero2Pos.x - HeroWidth;
                    }
                    if (Hero2Pos.x < minX)
                    {
                        Hero2Pos.x = minX;
                        if (Hero1Pos.x < Hero2Pos.x + HeroWidth) Hero1Pos.x = Hero2Pos.x + HeroWidth;
                    }
                    if (Hero1Pos.x > maxX)
                    {
                        Hero1Pos.x = maxX;
                        if (Hero2Pos.x > Hero1Pos.x - HeroWidth) Hero2Pos.x = Hero1Pos.x - HeroWidth;
                    }
                }
            }

            // SHOOTING: Shoab (W or Space)
            bool canShoot1 = !hero1Debuffed && (Hero1Lives > 0);
            for (int i = 0; i < 2; i++)
            {
                if (Hero1BulletActive[i] && Hero1BulletPos[i].y > WindowHeight / 2.0f)
                {
                    canShoot1 = false;
                    break;
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
            for (int i = 0; i < 2; i++)
            {
                if (Hero2BulletActive[i] && Hero2BulletPos[i].y > WindowHeight / 2.0f)
                {
                    canShoot2 = false;
                    break;
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

            // UPDATE HERO BULLETS and individual scoring
            for (int h = 0; h < 2; h++)
            {
                Vector2 *bPos = (h == 0) ? Hero1BulletPos : Hero2BulletPos;
                bool *bActive = (h == 0) ? Hero1BulletActive : Hero2BulletActive;
                int *hScore = (h == 0) ? &Hero1Score : &Hero2Score;

                for (int i = 0; i < 2; i++)
                {
                    if (bActive[i])
                    {
                        bPos[i].y -= BulletSpeedY * Time;
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
                                            *hScore += 100; // Points rewarded to individual hero

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

                        // Boss collision (-3 HP per hit)
                        if (bossActive && bActive[i])
                        {
                            Rectangle bossRec = { bossPos.x, bossPos.y, BossWidth, BossHeight };
                            if (CheckCollisionRecs(bRec, bossRec))
                            {
                                PlaySound(damage);
                                bActive[i] = false;
                                bossHp -= 3; // Boss health reduced by 3 per hit
                                *hScore += 50;

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

            // UPDATE ALIEN BULLET & HERO HIT
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
                        PlaySound(heroOuch);
                        if (Hero1Lives <= 0 && Hero2Lives <= 0)
                        {
                            PlaySound(heroDeath);
                            deathSequenceActive = true;
                            deathDelayTimer = 0.0f;
                        }
                        else if (Hero1Lives <= 0)
                        {
                            PlaySound(heroDeath);
                        }
                    }
                    else if (Hero2Lives > 0 && CheckCollisionRecs(aBulletRec, h2Rec))
                    {
                        AlienBulletActive = false;
                        Hero2Lives--;
                        PlaySound(heroOuch);
                        if (Hero1Lives <= 0 && Hero2Lives <= 0)
                        {
                            PlaySound(heroDeath);
                            deathSequenceActive = true;
                            deathDelayTimer = 0.0f;
                        }
                        else if (Hero2Lives <= 0)
                        {
                            PlaySound(heroDeath);
                        }
                    }
                }
            }

            // BOSS AI, ATTACKS & MOVEMENT
            if (bossActive)
            {
                bossAnimTimer += Time;
                bossDirChangeTimer += Time;

                if (bossDirChangeTimer > 1.8f)
                {
                    bossDirChangeTimer = 0.0f;
                    if (GetRandomValue(0, 10) > 4)
                    {
                        bossSpeed.y = (float)GetRandomValue(-90, 90);
                    }
                }

                bossPos.x += bossSpeed.x * Time;
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

                // Attack 2: Laser Beam every 7s (persists 2s)
                if (!bossLaserActive)
                {
                    bossLaserTimer += Time;
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

                // Attack 3: 6 Circular Projectiles every 4s
                bossOrbTimer += Time;
                if (bossOrbTimer >= 4.0f && !deathSequenceActive)
                {
                    bossOrbTimer = 0.0f;
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
                            PlaySound(heroOuch);
                            if (Hero1Lives <= 0 && Hero2Lives <= 0)
                            {
                                PlaySound(heroDeath);
                                deathSequenceActive = true;
                                deathDelayTimer = 0.0f;
                            }
                            else if (Hero1Lives <= 0)
                            {
                                PlaySound(heroDeath);
                            }
                        }
                        else if (Hero2Lives > 0 && CheckCollisionRecs(bRec, h2Rec))
                        {
                            bossBulletActive[k] = false;
                            Hero2Lives--;
                            PlaySound(heroOuch);
                            if (Hero1Lives <= 0 && Hero2Lives <= 0)
                            {
                                PlaySound(heroDeath);
                                deathSequenceActive = true;
                                deathDelayTimer = 0.0f;
                            }
                            else if (Hero2Lives <= 0)
                            {
                                PlaySound(heroDeath);
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
                            PlaySound(heroOuch);
                        }
                        else if (Hero2Lives > 0 && CheckCollisionCircleRec(bossOrbPos[k], 12.0f, h2Rec))
                        {
                            bossOrbActive[k] = false;
                            hero2Debuffed = true;
                            hero2DebuffTimer = 4.5f;
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
                    PlaySound(heroOuch);
                }
                if (hit2)
                {
                    Hero2Lives = 0;
                    PlaySound(heroOuch);
                }

                if (hit1 || hit2)
                {
                    PlaySound(heroDeath);
                    if (Hero1Lives <= 0 && Hero2Lives <= 0)
                    {
                        deathSequenceActive = true;
                        deathDelayTimer = 0.0f;
                    }
                }
            }

            // ALIEN GRID MOVEMENT
            if (!bossSpawned)
            {
                if (AlienPos[AlienInX - 1][0].x + AlienSize >= WindowWidth && AlienSpeed.x > 0)
                {
                    AlienSpeed.x *= -1;
                    DownAlien(AlienInX, AlienInY, AlienPos);
                }
                else if (AlienPos[0][0].x <= 0 && AlienSpeed.x < 0)
                {
                    AlienSpeed.x *= -1;
                    DownAlien(AlienInX, AlienInY, AlienPos);
                }

                for (int X = 0; X < AlienInX; X++)
                {
                    for (int Y = 0; Y < AlienInY; Y++)
                    {
                        AlienPos[X][Y] = Vector2Add(AlienPos[X][Y], Vector2Scale(AlienSpeed, Time));

                        if (AlienAlive[X][Y])
                        {
                            Rectangle Alien = { AlienPos[X][Y].x, AlienPos[X][Y].y, AlienSize, AlienSize };
                            int AStyle = (int)(GetTime() / 0.1) % 2;
                            DrawTexturePro(
                                AlienTexture[Y % AlienSprite][AStyle],
                                (Rectangle){ 0, 0, (float)AlienTexture[Y % AlienSprite][AStyle].width, (float)AlienTexture[Y % AlienSprite][AStyle].height },
                                Alien, 
                                (Vector2){ 0, 0 },
                                0.0f,
                                WHITE
                            );
                        }
                    }
                }
            }

            // DRAW BULLETS
            for (int i = 0; i < 2; i++)
            {
                if (Hero1BulletActive[i])
                    DrawRectangle((int)(Hero1BulletPos[i].x - BulletWidth / 2.0f), (int)Hero1BulletPos[i].y, BulletWidth, BulletHeight, YELLOW);
                if (Hero2BulletActive[i])
                    DrawRectangle((int)(Hero2BulletPos[i].x - BulletWidth / 2.0f), (int)Hero2BulletPos[i].y, BulletWidth, BulletHeight, SKYBLUE);
            }

            if (AlienBulletActive)
            {
                DrawRectangle((int)(AlienBulletPos.x - AlienBulletWidth / 2.0f), (int)AlienBulletPos.y, AlienBulletWidth, AlienBulletHeight, RED);
            }

            // DRAW BOSS & ATTACKS
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
                DrawTexturePro(
                    BossTexture[bFrame],
                    (Rectangle){ 0, 0, (float)BossTexture[bFrame].width, (float)BossTexture[bFrame].height },
                    bossRec, (Vector2){ 0, 0 }, 0.0f, WHITE
                );

                // Triple Bullets
                for (int k = 0; k < MAX_BOSS_BULLETS; k++)
                {
                    if (bossBulletActive[k])
                    {
                        DrawRectangle((int)(bossBulletPos[k].x - AlienBulletWidth / 2.0f), (int)bossBulletPos[k].y, AlienBulletWidth, AlienBulletHeight, RED);
                    }
                }

                // Laser Beam
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

                // Circular Disruptors
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

            // DRAW HERO 1: SHOAB (Left side, only if alive)
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

            // DRAW HERO 2: NAYEMUL (Right side, only if alive)
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

            // TOP-LEFT HUD: SHOAB'S STATUS, INDIVIDUAL SCORE & CONTROLS
            DrawText("PILOT 1: SHOAB", 30, 18, 20, LIME);
            DrawText(TextFormat("LIVES: %d / 4", Hero1Lives), 30, 44, 22, (Hero1Lives <= 1) ? RED : LIME);
            DrawText(TextFormat("SCORE: %05d", Hero1Score), 30, 72, 22, (Color){ 180, 255, 180, 255 });
            DrawText("[A / D] Move  |  [W / SPACE] Shoot", 30, 100, 15, LIGHTGRAY);

            // TOP-RIGHT HUD: NAYEMUL'S STATUS, INDIVIDUAL SCORE & CONTROLS
            const char* pilot2Title = "PILOT 2: NAYEMUL";
            int p2tW = MeasureText(pilot2Title, 20);
            DrawText(pilot2Title, WindowWidth - p2tW - 30, 18, 20, YELLOW);

            const char* p2LivesText = TextFormat("LIVES: %d / 4", Hero2Lives);
            int p2lW = MeasureText(p2LivesText, 22);
            DrawText(p2LivesText, WindowWidth - p2lW - 30, 44, 22, (Hero2Lives <= 1) ? RED : YELLOW);

            const char* p2ScoreText = TextFormat("SCORE: %05d", Hero2Score);
            int p2sW = MeasureText(p2ScoreText, 22);
            DrawText(p2ScoreText, WindowWidth - p2sW - 30, 72, 22, (Color){ 255, 245, 160, 255 });

            const char* p2Controls = "[LEFT / RIGHT] Move  |  [UP] Shoot";
            int p2cW = MeasureText(p2Controls, 15);
            DrawText(p2Controls, WindowWidth - p2cW - 30, 100, 15, LIGHTGRAY);

            // TOP-CENTER: BOSS HP BAR (Positioned clearly with zero HUD overlap)
            const char* pauseNotice = "[P] PAUSE   |   [ESC] MENU";
            int pauseW = MeasureText(pauseNotice, 15);

            if (bossActive)
            {
                int barW = 460;
                int barH = 22;
                int barX = (WindowWidth - barW) / 2;
                int barY = 36;

                char bossHpText[] = "DREADNOUGHT CARRIER BOSS";
                int hpTextW = MeasureText(bossHpText, 18);
                DrawText(bossHpText, (WindowWidth - hpTextW) / 2, 14, 18, RED);

                DrawRectangle(barX - 3, barY - 3, barW + 6, barH + 6, DARKGRAY);
                DrawRectangle(barX, barY, (int)(barW * ((float)bossHp / 100.0f)), barH, MAROON);
                DrawRectangle(barX, barY, (int)(barW * ((float)bossHp / 100.0f)), barH / 2, RED);
                DrawRectangleLines(barX, barY, barW, barH, WHITE);
                DrawText(TextFormat("%d / 100", bossHp), barX + barW / 2 - 28, barY + 3, 16, YELLOW);

                // Pause notice located cleanly below the health bar
                DrawText(pauseNotice, (WindowWidth - pauseW) / 2, 68, 15, LIGHTGRAY);
            }
            else
            {
                // Pause notice displayed neatly at the top when boss has not appeared
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

    UnloadMusicStream(bgmStory);
    UnloadMusicStream(bgmMenu);
    UnloadMusicStream(bgmBoss);

    for (int b = 0; b < 5; b++)
    {
        UnloadTexture(BossTexture[b]);
    }

    for (int Asprite = 0; Asprite < AlienSprite; Asprite++)
    {
        for (int AStyle = 0; AStyle < AlienSpriteStyle; AStyle++)
        {
            UnloadTexture(AlienTexture[Asprite][AStyle]);
        }
    }

    CloseAudioDevice();
    CloseWindow();
    return 0;
}
