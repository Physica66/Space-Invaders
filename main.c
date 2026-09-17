#include "raylib/raylib-6.0_macos/include/raylib.h"

#include "raylib.h"
#include "raymath.h"
#include <stdbool.h>

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

// Game Stats
#define STATE_LOADING 0
#define STATE_MENU 1
#define STATE_GAMEPLAY 2
#define STATE_OPTIONS 3
#define STATE_CREDITS 4

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

int main(void)
{
    InitWindow(WindowWidth, WindowHeight, "Space Invaders");
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
    Sound lowHeart = LoadSound("assets/audio/sfx_lowhealth_alarmloop4.wav");
    Sound heroDeath = LoadSound("assets/audio/sfx_deathscream_robot4.wav");
    Sound pauseIn = LoadSound("assets/audio/sfx_sounds_pause4_in.wav");
    Sound pauseOut = LoadSound("assets/audio/sfx_sounds_pause4_out.wav");
    Sound damage = LoadSound("assets/audio/sfx_sounds_damage3.wav");

    
    Sound cheer = LoadSound("assets/audio/cheering.wav");
    Sound gameWin = LoadSound("assets/audio/game_win.wav");
    Sound gameOverChild = LoadSound("assets/audio/universfield-game-over-kid-voice-clip-352738.mp3");
    Sound gameOverCommunity = LoadSound("assets/audio/freesound_community-game-over-38511.mp3");

    Sound heroOuch = LoadSound("assets/audio/hero_hit .wav");


    Texture2D HeroTexture = LoadTexture("assets/sprites/Hero.png");
    AlienTexture[0][0] = LoadTexture("assets/sprites/Alien1style1.png");
    AlienTexture[0][1] = LoadTexture("assets/sprites/Alien1style2.png");
    AlienTexture[1][0] = LoadTexture("assets/sprites/Alien2style1.png");
    AlienTexture[1][1] = LoadTexture("assets/sprites/Alien2style2.png");
    AlienTexture[2][0] = LoadTexture("assets/sprites/Alien3style1.png");
    AlienTexture[2][1] = LoadTexture("assets/sprites/Alien3style2.png");

    // Hero position and speed
    Vector2 HeroPos = { WindowWidth / 2.0f, WindowHeight - HeroHeight };
    Vector2 HeroSpeed = { 0, 0 };

    // Hero bullets
    Vector2 HeroBulletPos[2] = { {0, 0}, {0, 0} };
    bool HeroBulletActive[2] = { false, false };

    // Alien bullets and shooting timer
    Vector2 AlienBulletPos = { 0, 0 };
    bool AlienBulletActive = false;
    float AlienShootTimer = 0.0f;

    // Game state, lives, and score
    int HeroLives = 3;
    int Score = 0;
    int AliensKilled = 0;
    bool GameOver = false;

    // Additional state handling for sound and pause
    bool isPaused = false;
    float lowHeartTimer = 0.0f;
    float deathDelayTimer = 0.0f;
    bool deathSequenceActive = false;

    // Sound sequence tracking flag/boolean varibles :(
    bool cheerPlayed = false;
    bool winSoundPlayed = false;
    bool gameOverChildPlayed = false;
    bool gameOverCommPlayed = false;
    float gameOverSoundTimer = 0.0f;

    // State loader
    int CurrentState = STATE_LOADING;
    float LoadingTimer = 0.0f;
    int MenuSelection = 0; // 0: Play, 1: Options, 2: Credentials, 3: Exit ;)

    // Options Menu Config
    int OptionsTab = 0;     // 0: Sound, 1: Video
    int SoundSelection = 0; // 0: Track, 1: BGM Volume, 2: SFX Volume
    int VideoSelection = 0; // 0: Brightness, 1: Scanlines, 2: Starfield

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

        //background starfield video(animation)
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

        // Adjustment of dynamic sound effect volume
        SetSoundVolume(shoot, SfxVolume);
        SetSoundVolume(AlienShoot, SfxVolume);
        SetSoundVolume(menuMove, SfxVolume);
        SetSoundVolume(menuSelect, SfxVolume);
        SetSoundVolume(lowHeart, SfxVolume);
        SetSoundVolume(heroDeath, SfxVolume);
        SetSoundVolume(pauseIn, SfxVolume);
        SetSoundVolume(pauseOut, SfxVolume);
        SetSoundVolume(damage, SfxVolume);
        SetSoundVolume(cheer, SfxVolume);
        SetSoundVolume(gameWin, SfxVolume);
        SetSoundVolume(gameOverChild, SfxVolume);
        SetSoundVolume(gameOverCommunity, SfxVolume);
        SetSoundVolume(heroOuch, SfxVolume);

        BeginDrawing();
        ClearBackground(BLACK);

        // animating Background Stars
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
                CurrentState = STATE_MENU;
            }
        }

        // STATE: MAIN MENU
        else if (CurrentState == STATE_MENU)
        {
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

            char itemNames[4][32] = {
                "PLAY",
                "OPTIONS",
                "CREDENTIALS",
                "EXIT"
            };

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
                if (MenuSelection == 0) CurrentState = STATE_GAMEPLAY;
                else if (MenuSelection == 1) CurrentState = STATE_OPTIONS;
                else if (MenuSelection == 2) CurrentState = STATE_CREDITS;
                else if (MenuSelection == 3) break;
            }
        }

        //  STATE: OPTIONS (SOUND and VIDEO SETTINGS)
        else if (CurrentState == STATE_OPTIONS)
        {
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

                char bgmTracks[3][32] = { "not available", "not available :(", "not available :)" };

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

        //STATE: CREDENTIALS SCREEN
        else if (CurrentState == STATE_CREDITS)
        {
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
            // Toggle pause state with 'P' key
            if (!GameOver && AliensKilled != AlienInX * AlienInY && !deathSequenceActive)
            {
                if (IsKeyPressed(KEY_P))
                {
                    isPaused = !isPaused;
                    if (isPaused)
                    {
                        PlaySound(pauseIn);
                    }
                    else
                    {
                        PlaySound(pauseOut);
                    }
                }
            }

            if (isPaused)
            {
                // Draw current game frame in background
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

                for (int i = 0; i < 2; i++)
                {
                    if (HeroBulletActive[i])
                        DrawRectangle((int)(HeroBulletPos[i].x - BulletWidth / 2.0f), (int)HeroBulletPos[i].y, BulletWidth, BulletHeight, YELLOW);
                }

                if (AlienBulletActive)
                    DrawRectangle((int)(AlienBulletPos.x - AlienBulletWidth / 2.0f), (int)AlienBulletPos.y, AlienBulletWidth, AlienBulletHeight, RED);

                Rectangle Hero = { HeroPos.x - HeroWidth / 2.0f, HeroPos.y, HeroWidth, HeroHeight };
                DrawTexturePro(HeroTexture, (Rectangle){ 0, 0, (float)HeroTexture.width, (float)HeroTexture.height }, Hero, (Vector2){ 0, 0 }, 0.0f, WHITE);

                DrawText(TextFormat("SCORE: %05d", Score), 30, 20, 28, YELLOW);
                DrawText(TextFormat("LIVES: %d", HeroLives), 30, 55, 24, (HeroLives == 1) ? RED : GREEN);

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
                    CurrentState = STATE_MENU;
                }

                EndDrawing();
                continue;
            }

            // Low-heart alert sound playing every 0.85s when only 1 life remains
            if (HeroLives == 1 && !GameOver && !deathSequenceActive)
            {
                lowHeartTimer += Time;
                if (lowHeartTimer >= 0.85f)
                {
                    lowHeartTimer = 0.0f;
                    PlaySound(lowHeart);
                }
            }

            // Hero-death scream sequence handling before showing game over
            if (deathSequenceActive)
            {
                deathDelayTimer += Time;
                if (deathDelayTimer >= 1.2f)
                {
                    deathSequenceActive = false;
                    GameOver = true;
                }
            }

            if (IsKeyPressed(KEY_ESCAPE) && !deathSequenceActive)
            {
                PlaySound(menuSelect);
                CurrentState = STATE_MENU;
            }

            // GAME OVER POPUP and  RESTART
            if (GameOver)
            {
                // Play game over child sound, then community sound sequence
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

                char subText[] = "You lost all 3 lives! You Are A Noob!!";
                DrawText(subText, popupX + (popupWidth - MeasureText(subText, 22)) / 2, popupY + 105, 22, WHITE);

                char restartText[] = "Press [R] to Restart  |  [ESC] Menu";
                DrawText(restartText, popupX + (popupWidth - MeasureText(restartText, 20)) / 2, popupY + 160, 20, YELLOW);

                if (IsKeyPressed(KEY_R))
                {
                    PlaySound(menuSelect);
                    HeroLives = 3;
                    Score = 0;
                    AliensKilled = 0;
                    GameOver = false;
                    deathSequenceActive = false;
                    deathDelayTimer = 0.0f;
                    lowHeartTimer = 0.0f;
                    cheerPlayed = false;
                    winSoundPlayed = false;
                    gameOverChildPlayed = false;
                    gameOverCommPlayed = false;
                    gameOverSoundTimer = 0.0f;
                    HeroPos = (Vector2){ WindowWidth / 2.0f, WindowHeight - HeroHeight };
                    HeroSpeed = (Vector2){ 0, 0 };
                    AlienSpeed = (Vector2){ AlienSpeedX, AlienSpeedY };

                    HeroBulletActive[0] = false;
                    HeroBulletActive[1] = false;
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

                EndDrawing();
                continue;
            }

            //game Won
            if (AliensKilled == AlienInX * AlienInY)
            {
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

                char subText[] = "You defeated all aliens! ;)";
                DrawText(subText, popupX + (popupWidth - MeasureText(subText, 22)) / 2, popupY + 105, 22, WHITE);

                char restartText[] = "Press [R] to Restart  |  [ESC] Menu";
                DrawText(restartText, popupX + (popupWidth - MeasureText(restartText, 20)) / 2, popupY + 160, 20, YELLOW);

                if (IsKeyPressed(KEY_R))
                {
                    PlaySound(menuSelect);
                    AliensKilled = 0;
                    cheerPlayed = false;
                    winSoundPlayed = false;
                    gameOverChildPlayed = false;
                    gameOverCommPlayed = false;
                    gameOverSoundTimer = 0.0f;
                    HeroPos = (Vector2){ WindowWidth / 2.0f, WindowHeight - HeroHeight };
                    HeroSpeed = (Vector2){ 0, 0 };
                    AlienSpeed = (Vector2){ AlienSpeedX, AlienSpeedY };

                    HeroBulletActive[0] = false;
                    HeroBulletActive[1] = false;
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

                EndDrawing();
                continue;
            }

            // HERO SHOOTING logic
            bool canShoot = true;
            for (int i = 0; i < 2; i++)
            {
                if (HeroBulletActive[i] && HeroBulletPos[i].y > WindowHeight / 2.0f)
                {
                    canShoot = false;
                    break;
                }
            }

            if (IsKeyPressed(KEY_SPACE) && canShoot && !deathSequenceActive)
            {
                PlaySound(shoot);
                for (int i = 0; i < 2; i++)
                {
                    if (!HeroBulletActive[i])
                    {
                        HeroBulletActive[i] = true;
                        HeroBulletPos[i] = (Vector2){ HeroPos.x, HeroPos.y };
                        break;
                    }
                }
            }

            // UPDATE HERO BULLETS and COLLISION
            for (int i = 0; i < 2; i++)
            {
                if (HeroBulletActive[i])
                {
                    HeroBulletPos[i].y -= BulletSpeedY * Time;

                    if (HeroBulletPos[i].y < -BulletHeight)
                    {
                        HeroBulletActive[i] = false;
                        continue;
                    }

                    Rectangle bulletRec = { HeroBulletPos[i].x - BulletWidth / 2.0f, HeroBulletPos[i].y, BulletWidth, BulletHeight };
                    for (int X = 0; X < AlienInX; X++)
                    {
                        for (int Y = 0; Y < AlienInY; Y++)
                        {
                            if (AlienAlive[X][Y])
                            {
                                Rectangle alienRec = { AlienPos[X][Y].x, AlienPos[X][Y].y, AlienSize, AlienSize };
                                if (CheckCollisionRecs(bulletRec, alienRec))
                                {
                                    PlaySound(damage);
                                    AlienAlive[X][Y] = false;
                                    HeroBulletActive[i] = false;
                                    AliensKilled++;
                                    Score += 100;

                                    // Cheering sound plays once when 70% of aliens are dead and hero has 2+ lives
                                    if (!cheerPlayed && AliensKilled >= (int)(AlienInX * AlienInY * 0.70f) && HeroLives >= 2)
                                    {
                                        PlaySound(cheer);
                                        cheerPlayed = true;
                                    }
                                    break;
                                }
                            }
                        }
                        if (!HeroBulletActive[i]) break;
                    }
                }
            }

            // RANDOM ALIEN SHOOTING [only the bottom layer aliens will shoot,it'll be updated per frame] -Nayem
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

            // UPDATE ALIEN BULLET and HERO HIT [collision checker basically]
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
                    Rectangle heroHitbox = { HeroPos.x - HeroWidth / 2.0f, HeroPos.y, HeroWidth, HeroHeight };

                    if (CheckCollisionRecs(aBulletRec, heroHitbox))
                    {
                        AlienBulletActive = false;
                        HeroLives--;
                        if (HeroLives <= 0)
                        {
                            PlaySound(heroDeath);
                            deathSequenceActive = true;
                            deathDelayTimer = 0.0f;
                        }
                        else
                        {
                            PlaySound(heroOuch);
                        }
                    }
                }
            }

            // ALIEN GRID MOVEMENT
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

            // DRAW ALIENS
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

            // DRAW BULLETS
            for (int i = 0; i < 2; i++)
            {
                if (HeroBulletActive[i])
                {
                    DrawRectangle((int)(HeroBulletPos[i].x - BulletWidth / 2.0f), (int)HeroBulletPos[i].y, BulletWidth, BulletHeight, YELLOW);
                }
            }

            if (AlienBulletActive)
            {
                DrawRectangle((int)(AlienBulletPos.x - AlienBulletWidth / 2.0f), (int)AlienBulletPos.y, AlienBulletWidth, AlienBulletHeight, RED);
            }

            // HERO MOVEMENT & BOUNDS
            if (!deathSequenceActive)
            {
                if (IsKeyDown(KEY_RIGHT))
                {
                    HeroSpeed.x = HeroSpeedX;
                }
                else if (IsKeyDown(KEY_LEFT))
                {
                    HeroSpeed.x = -HeroSpeedX;
                }
                else
                {
                    HeroSpeed.x = 0;
                }
            }
            else
            {
                HeroSpeed.x = 0;
            }

            HeroPos = Vector2Add(HeroPos, Vector2Scale(HeroSpeed, Time));
            if (HeroPos.x <= HeroWidth / 2.0f)
            {
                HeroPos.x = HeroWidth / 2.0f;
            }
            else if (HeroPos.x + HeroWidth / 2.0f >= WindowWidth)
            {
                HeroPos.x = WindowWidth - HeroWidth / 2.0f;
            }

            Rectangle Hero = { HeroPos.x - HeroWidth / 2.0f, HeroPos.y, HeroWidth, HeroHeight };
            DrawTexturePro(HeroTexture, (Rectangle){ 0, 0, (float)HeroTexture.width, (float)HeroTexture.height }, Hero, (Vector2){ 0, 0 }, 0.0f, WHITE);

            // DRAW SCOREBOARD and LIVES
            DrawText(TextFormat("SCORE: %05d", Score), 30, 20, 28, YELLOW);
            DrawText(TextFormat("LIVES: %d", HeroLives), 30, 55, 24, (HeroLives == 1) ? RED : GREEN);
            DrawText("[P] PAUSE   |   [ESC] MENU", WindowWidth - 340, 25, 18, LIGHTGRAY);
        }

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
    UnloadSound(shoot);
    UnloadSound(AlienShoot);
    UnloadSound(menuMove);
    UnloadSound(menuSelect);
    UnloadSound(lowHeart);
    UnloadSound(heroDeath);
    UnloadSound(pauseIn);
    UnloadSound(pauseOut);
    UnloadSound(damage);
    UnloadSound(cheer);
    UnloadSound(gameWin);
    UnloadSound(gameOverChild);
    UnloadSound(gameOverCommunity);
    UnloadSound(heroOuch);

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
