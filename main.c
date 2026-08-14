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


#define MaxBullets 10
#define BulletSpeedY 700
#define BulletWidth 6
#define BulletHeight 20

typedef struct {
    Vector2 pos;
    bool active;
} Bullet;

void DownAlien(int AlienInX, int AlienInY, Vector2 AlienPos[AlienInX][AlienInY]) {
    for (int X = 0; X < AlienInX; X++) {
        for (int Y = 0; Y < AlienInY; Y++) {
            AlienPos[X][Y].y += AlienSize / 2.0f;
        }
    }
}

int main(void)
{
    InitWindow(WindowWidth, WindowHeight, "Space Invaders");
    SetTargetFPS(60);
    
    // Alien grid dimension calculation
    int AlienInX = (WindowWidth / (AlienSize + AlienDistance)) - 2;
    int AlienInY = (WindowHeight / (2 * (AlienSize + AlienDistance)));
    
    Vector2 AlienPos[AlienInX][AlienInY];
    bool AlienAlive[AlienInX][AlienInY];

    AlienPos[0][0] = (Vector2){ 150, 50 };
    Vector2 AlienSpeed = { AlienSpeedX, AlienSpeedY };

    for (int X = 0; X < AlienInX; X++) {
        for (int Y = 0; Y < AlienInY; Y++) { 
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

    // Bullet initialization
    Bullet bullets[MaxBullets] = { 0 };

    
    while (!WindowShouldClose())
    {
        BeginDrawing();
        ClearBackground(BLACK);
        float Time = GetFrameTime();

        // 1. Firing Bullets
        if (IsKeyPressed(KEY_SPACE))
        {
            for (int i = 0; i < MaxBullets; i++)
            {
                if (!bullets[i].active)
                {
                    bullets[i].active = true;
                    bullets[i].pos = (Vector2){ HeroPos.x, HeroPos.y };
                    break;
                }
            }
        }

        // 2. Updating Bullets & Collision
        for (int i = 0; i < MaxBullets; i++)
        {
            if (bullets[i].active)
            {
                bullets[i].pos.y -= BulletSpeedY * Time;

                if (bullets[i].pos.y < -BulletHeight)
                {
                    bullets[i].active = false;
                    continue;
                }

                Rectangle bulletRec = { bullets[i].pos.x - BulletWidth / 2.0f, bullets[i].pos.y, BulletWidth, BulletHeight };
                
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
                                bullets[i].active = false;
                                break;
                            }
                        }
                    }
                    if (!bullets[i].active) break;
                }
            }
        }

        // 3. Movement of Aliens
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

        // 4. Drawing Aliens
        for (int X = 0; X < AlienInX; X++) {
            for (int Y = 0; Y < AlienInY; Y++) {
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

        // 5. Draw Bullets
        for (int i = 0; i < MaxBullets; i++)
        {
            if (bullets[i].active)
            {
                DrawRectangle(bullets[i].pos.x - BulletWidth / 2.0f, bullets[i].pos.y, BulletWidth, BulletHeight, YELLOW);
            }
        }

        // 6. Hero Movement
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

        // Edge Detection
        HeroPos = Vector2Add(HeroPos, Vector2Scale(HeroSpeed, Time));
        if (HeroPos.x <= HeroWidth / 2.0f)
        {
            HeroPos.x = HeroWidth / 2.0f;
        }
        else if (HeroPos.x + HeroWidth / 2.0f >= WindowWidth)
        {
            HeroPos.x = WindowWidth - HeroWidth / 2.0f;
        }

        // 7. Draw Hero
        Rectangle Hero = { HeroPos.x - HeroWidth / 2.0f, HeroPos.y, HeroWidth, HeroHeight };
        DrawTexturePro(HeroTexture, (Rectangle){ 0, 0, (float)HeroTexture.width, (float)HeroTexture.height }, Hero, (Vector2){ 0, 0 }, 0.0f, WHITE);

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