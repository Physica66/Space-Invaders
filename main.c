#include "raylib/raylib-6.0_macos/include/raylib.h"

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
#define BulletSpeedY 700
#define BulletWidth 6
#define BulletHeight 20

#define AlienBulletSpeedY 350
#define AlienBulletWidth 6
#define AlienBulletHeight 18

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
    SetTargetFPS(60);

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

    // Hero bullets;2 bullets maximum and first bullet should reach half of screen
    Vector2 HeroBulletPos[2] = { {0, 0}, {0, 0} };
    bool HeroBulletActive[2] = { false, false };

    // Alien bullets and shooting timer
    Vector2 AlienBulletPos = { 0, 0 };
    bool AlienBulletActive = false;
    float AlienShootTimer = 0.0f;

    // Game state, lives, and score
    int HeroLives = 3;
    int Score = 0;
    bool GameOver = false;

    // Main Game Loop
    while (!WindowShouldClose())
    {
        BeginDrawing();
        ClearBackground(BLACK);
        float Time = GetFrameTime();

        
        // GAME OVER POPUP and  RESTART
        
        if (GameOver)
        {
            int popupWidth = 500;
            int popupHeight = 240;
            int popupX = (WindowWidth - popupWidth) / 2;
            int popupY = (WindowHeight - popupHeight) / 2;

            DrawRectangle(popupX, popupY, popupWidth, popupHeight, Fade(DARKGRAY, 0.95f));
            DrawRectangleLines(popupX, popupY, popupWidth, popupHeight, RED);

            char titleText[] = "GAME OVER";
            DrawText(titleText, popupX + (popupWidth - MeasureText(titleText, 45)) / 2, popupY + 35, 45, RED);

            char subText[] = "You lost all 3 lives!";
            DrawText(subText, popupX + (popupWidth - MeasureText(subText, 22)) / 2, popupY + 105, 22, WHITE);

            char restartText[] = "Press [R] to Restart";
            DrawText(restartText, popupX + (popupWidth - MeasureText(restartText, 20)) / 2, popupY + 160, 20, YELLOW);

            if (IsKeyPressed(KEY_R))
            {
                HeroLives = 3;
                Score = 0;
                GameOver = false;
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

        if (IsKeyPressed(KEY_SPACE) && canShoot)
        {
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
                                AlienAlive[X][Y] = false;
                                HeroBulletActive[i] = false;
                                Score += 100;
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
        if (!AlienBulletActive && AlienShootTimer >= 1.0f)
        {
            AlienShootTimer = 0.0f;
            int randomX = GetRandomValue(0, AlienInX - 1);

            for (int Y = AlienInY - 1; Y >= 0; Y--)
            {
                if (AlienAlive[randomX][Y])
                {
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
            else
            {
                Rectangle aBulletRec = { AlienBulletPos.x - AlienBulletWidth / 2.0f, AlienBulletPos.y, AlienBulletWidth, AlienBulletHeight };
                Rectangle heroHitbox = { HeroPos.x - HeroWidth / 2.0f, HeroPos.y, HeroWidth, HeroHeight };

                if (CheckCollisionRecs(aBulletRec, heroHitbox))
                {
                    AlienBulletActive = false;
                    HeroLives--;
                    if (HeroLives <= 0)
                    {
                        GameOver = true;
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

        
        // DRAW SCOREBOARD and LIVES on top-left corner
        
        DrawText(TextFormat("SCORE: %05d", Score), 30, 20, 28, YELLOW);
        DrawText(TextFormat("LIVES: %d", HeroLives), 30, 55, 24, GREEN);

        EndDrawing();
    }

    // Cleanup
    UnloadTexture(HeroTexture);
    for (int Asprite = 0; Asprite < AlienSprite; Asprite++)
    {
        for (int AStyle = 0; AStyle < AlienSpriteStyle; AStyle++)
        {
            UnloadTexture(AlienTexture[Asprite][AStyle]);
        }
    }

    CloseWindow();
    return 0;
}
