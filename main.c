#include "raylib/raylib-6.0_macos/include/raylib.h"
#include "raylib/raylib-6.0_macos/include/raymath.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#if defined(__APPLE__)
extern void* objc_getClass(const char* name);
extern void* sel_registerName(const char* name);
extern void* objc_msgSend(void* self, void* op, ...);  //macOS bug fixes for this game...

// Unlocks the macOS For Fullscreen...
void EnableMacOSNativeFullscreen(void)
{
    void* appClass = objc_getClass("NSApplication");
    if (!appClass) return;

    void* app = ((void* (*)(void*, void*))objc_msgSend)(appClass, sel_registerName("sharedApplication"));
    if (!app) return;

    void* windowsList = ((void* (*)(void*, void*))objc_msgSend)(app, sel_registerName("windows"));
    if (!windowsList) return;

    unsigned long count = ((unsigned long (*)(void*, void*))objc_msgSend)(windowsList, sel_registerName("count"));
    for (unsigned long i = 0; i < count; i++)
    {
        void* win = ((void* (*)(void*, void*, unsigned long))objc_msgSend)(windowsList, sel_registerName("objectAtIndex:"), i);
        if (win)
        {
            unsigned long behavior = ((unsigned long (*)(void*, void*))objc_msgSend)(win, sel_registerName("collectionBehavior"));
            behavior &= ~(1UL << 8);
            behavior |= (1UL << 7);
            ((void (*)(void*, void*, unsigned long))objc_msgSend)(win, sel_registerName("setCollectionBehavior:"), behavior);
        }
    }
}

// Programmatically triggers the exact same green button action
void ToggleGameFullscreen(void)
{
    void* appClass = objc_getClass("NSApplication");
    if (!appClass) return;

    void* app = ((void* (*)(void*, void*))objc_msgSend)(appClass, sel_registerName("sharedApplication"));
    if (!app) return;

    void* windowsList = ((void* (*)(void*, void*))objc_msgSend)(app, sel_registerName("windows"));
    if (!windowsList) return;

    unsigned long count = ((unsigned long (*)(void*, void*))objc_msgSend)(windowsList, sel_registerName("count"));
    if (count > 0)
    {
        void* win = ((void* (*)(void*, void*, unsigned long))objc_msgSend)(windowsList, sel_registerName("objectAtIndex:"), 0);
        if (win)
        {
            ((void (*)(void*, void*, void*))objc_msgSend)(win, sel_registerName("toggleFullScreen:"), NULL);
        }
    }
}

bool IsGameFullscreen(void)
{
    void* appClass = objc_getClass("NSApplication");
    if (appClass)
    {
        void* app = ((void* (*)(void*, void*))objc_msgSend)(appClass, sel_registerName("sharedApplication"));
        if (app)
        {
            void* windowsList = ((void* (*)(void*, void*))objc_msgSend)(app, sel_registerName("windows"));
            if (windowsList)
            {
                unsigned long count = ((unsigned long (*)(void*, void*))objc_msgSend)(windowsList, sel_registerName("count"));
                if (count > 0)
                {
                    void* win = ((void* (*)(void*, void*, unsigned long))objc_msgSend)(windowsList, sel_registerName("objectAtIndex:"), 0);
                    if (win)
                    {
                        unsigned long mask = ((unsigned long (*)(void*, void*))objc_msgSend)(win, sel_registerName("styleMask"));
                        return (mask & (1UL << 14)) != 0; // NSWindowStyleMaskFullScreen
                    }
                }
            }
        }
    }
    return IsWindowFullscreen();
}
#else
void EnableMacOSNativeFullscreen(void) {}
void ToggleGameFullscreen(void) { ToggleBorderlessWindowed(); }
bool IsGameFullscreen(void) { return IsWindowFullscreen(); }
#endif

bool SpecialReady = false;
bool starttimer = false;
float ShoabSpecialTime = 0.0f;  //shoab's special bullet

bool NayemulSpecialReady = false;
float NayemulSpecialTime = 0.0f;
bool clusterBossDamageDealt = false; //Nayemul's special cluster missile...

// Visuals and Edge Flash Timers
float shoabEdgeFlashTimer = 0.0f;
float nayemulEdgeFlashTimer = 0.0f;
float bossHitFlashTimer = 0.0f;

// Freezing effect (Hit-Stop / Super Pause) :(
float superPauseTimer = 0.0f; // Option A: Activation Super-Freeze (0.18s)
float hitStopTimer = 0.0f;    // Option B: Impact Hit-Stop (0.04s)

// Interceptor Canopy Reflections and Space Lightning
float canopyGlintTimer = 0.0f;
float spaceLightningTimer = 0.0f;

// Last Stand Overdrive (2.5s surge when partner is killed!!!!) ;)
float hero1OverdriveTimer = 0.0f;
float hero2OverdriveTimer = 0.0f;

// Team Shielding Feature Variables (Fixed size half-sphere trapping heroes)
bool teamShieldActive = false;
float teamShieldActiveTimer = 0.0f;
float teamShieldCooldownTimer = 0.0f;
float teamShieldCenterX = 0.0f;
#define TeamShieldRadius 240.0f

// Dreadnought Hyperspace Drop and  Evacuation Feature Variables :)
bool bossWarpActive = false;
float bossWarpTimer = 0.0f;
bool evacActive = false;
float evacTimer = 0.0f;

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
#define BossPodMaxHp 75

// Medium Alien Minion dimensions and capacities
#define MAX_MINIONS 8
#define MinionSize 68
#define MAX_MINION_BULLETS 16

#define COMMANDER_SIZE 72
#define MAX_COMMANDER_BULLETS 8

// Cluster missile Capacities
#define MAX_CLUSTER_MISSILES 8
#define MAX_CLUSTER_BLASTS 8

// Space Hazards animation type bg: Metallic Asteroids and Particles
#define MAX_ASTEROIDS 6
#define MAX_ASTEROID_DEBRIS 32
#define MAX_SMOKE_PARTICLES 64
#define MAX_SPARK_PARTICLES 32

typedef struct {  //Asteroid variables,struct is used to reduce the lines :)
    Vector2 pos;
    Vector2 speed;
    float hp;
    float maxHp;
    float radius;
    float rotation;
    float rotSpeed;
    bool active;
} Asteroid;

typedef struct {
    Vector2 pos;
    Vector2 vel;
    float rotation;
    float rotSpeed;
    float life;
    float maxLife;
    bool active;
} AsteroidDebris;

typedef struct {  //space particles!!!
    Vector2 pos;
    Vector2 vel;
    float life;
    float maxLife;
    float size;
    bool active;
} SmokeParticle;

typedef struct {
    Vector2 pos;
    Vector2 vel;
    float life;
    float maxLife;
    bool active;
} SparkParticle;

// Micro-Singularity Anomaly (Black Hole) with smooth drift and collapse
typedef struct {
    Vector2 pos;
    float basePosX;
    float pullRadius;      // Accretion halo pull limit (450px)
    float shearRadius;     // Tidal shear heavy drag zone (180px)
    float coreRadius;      // Event horizon destruction zone (35px)
    float maxPullForce;    // Maximum inward velocity (380px/s)
    float driftTimer;
    float rotationAngle;
    float scale;           // Shrinks from 1.0f -> 0.0f when collapsing
    float alpha;           // Smooth fade out
    bool active;
    bool spawned;
    bool collapsing;
    float collapseTimer;
} BlackHole;

// Mysterious Black Entity (Void Phantom) that emerges from the collapsed black hole
#define MAX_ENTITY_ORBS 6

typedef struct {
    Vector2 pos;
    Vector2 vel;
    bool active;
} EntityOrb;

typedef struct {
    Vector2 pos;
    Vector2 speed;
    int hp;
    int maxHp;
    bool active;
    bool spawned;
    float animTimer;
    int currentFrame;
    float attackTimer;
    float phaseTimer;
    bool phasing;
    float screamTimer;
    bool screaming;
    float alpha;
    float targetedHeroTimer;
    int targetedHero;      // 1 = Shoab, 2 = Nayemul
} VoidEntity;

// Permanent Match History Record Structure (with Pilot Names)
#define MAX_MATCH_RECORDS 100
#define HISTORY_FILE "match_history.dat"
#define CAREER_FILE "hero_profiles.dat"

typedef struct {
    bool isVictory;
    float runTime;
    char hero1PilotName[24];
    char hero2PilotName[24];
    int hero1Score;
    int hero2Score;
    int hero1Kills;
    int hero2Kills;
    int hero1BossDamage;
    int hero2BossDamage;
    int hero1HitsTaken;
    int hero2HitsTaken;
} MatchRecord;

// Permanent Career State for Both Hero Ships
typedef struct {
    char callsign[32];
    char shipTitle[32];
    int totalMissions;
    int totalVictories;
    int totalDefeats;
    long totalScore;
    int totalAlienKills;
    int totalMinionKills;
    int totalBossDamage;
    int totalHitsTaken;
} HeroCareerProfile;

// Load all saved gameplay records (most recent first)
int LoadMatchRecords(MatchRecord records[], int maxCount)
{
    FILE* f = fopen(HISTORY_FILE, "rb");
    if (!f) return 0;
    int count = (int)fread(records, sizeof(MatchRecord), maxCount, f);
    fclose(f);
    return count;
}

// Permanently prepend the newest match record to the top of the file
void SaveMatchRecord(MatchRecord newRec)
{
    MatchRecord existing[MAX_MATCH_RECORDS];
    int count = LoadMatchRecords(existing, MAX_MATCH_RECORDS - 1);

    FILE* f = fopen(HISTORY_FILE, "wb");
    if (!f) return;

    // Write new record at index 0 (top/most recent)
    fwrite(&newRec, sizeof(MatchRecord), 1, f);

    // Append previous records behind it
    if (count > 0)
    {
        fwrite(existing, sizeof(MatchRecord), count, f);
    }
    fclose(f);
}

// Load Hero Profiles from permanent disk
void LoadHeroProfiles(HeroCareerProfile profiles[2])
{
    FILE* f = fopen(CAREER_FILE, "rb");
    if (f)
    {
        fread(profiles, sizeof(HeroCareerProfile), 2, f);
        fclose(f);
    }
    else
    {
        // Default Initialization
        strcpy(profiles[0].callsign, "AEGIS-1");
        strcpy(profiles[0].shipTitle, "Shoab's Interceptor Alpha");
        profiles[0].totalMissions = 0;
        profiles[0].totalVictories = 0;
        profiles[0].totalDefeats = 0;
        profiles[0].totalScore = 0;
        profiles[0].totalAlienKills = 0;
        profiles[0].totalMinionKills = 0;
        profiles[0].totalBossDamage = 0;
        profiles[0].totalHitsTaken = 0;

        strcpy(profiles[1].callsign, "SHADOW-2");
        strcpy(profiles[1].shipTitle, "Nayemul's Stealth Beta");
        profiles[1].totalMissions = 0;
        profiles[1].totalVictories = 0;
        profiles[1].totalDefeats = 0;
        profiles[1].totalScore = 0;
        profiles[1].totalAlienKills = 0;
        profiles[1].totalMinionKills = 0;
        profiles[1].totalBossDamage = 0;
        profiles[1].totalHitsTaken = 0;

        FILE* fw = fopen(CAREER_FILE, "wb");
        if (fw) { fwrite(profiles, sizeof(HeroCareerProfile), 2, fw); fclose(fw); }
    }
}

// Save Hero Profiles to permanent disk
void SaveHeroProfiles(HeroCareerProfile profiles[2])
{
    FILE* f = fopen(CAREER_FILE, "wb");
    if (f) {
        fwrite(profiles, sizeof(HeroCareerProfile), 2, f);
        fclose(f);
    }
}

#ifndef PI
#define PI 3.14159265358979323846f //finally,fixed this problem,thanks to MMAK sir :)
#endif

// Game States
#define STATE_LOADING 0
#define STATE_MENU 1
#define STATE_GAMEPLAY 2
#define STATE_OPTIONS 3
#define STATE_CREDITS 4
#define STATE_STORY 5
#define STATE_LAUNCH 6
#define STATE_CINEMATIC 7
#define STATE_SCORES 8
#define STATE_HOWTOPLAY 9
#define STATE_VIDEO_PLAY 10
#define STATE_HISTORY 11
#define STATE_CAREER 12
#define STATE_NAME_ENTRY 13

#define STAR_COUNT 90

#define VID_W 640
#define VID_H 360

static FILE* videoPipe = NULL;
static Texture2D videoFrameTex = { 0 };
static Color* videoFramePixels = NULL;
static float videoTimer = 0.0f;
static float videoDuration = 0.0f;
static float videoFrameTimer = 0.0f;
static int currentVideoType = 0;       //finally,made the varibles fixed!!!!!!!!!!!!
static bool videoIsPlaying = false;
static bool hasReceivedFrames = false;
static bool ffmpegInstalled = true;

const char* GetFFmpegBinary(void)
{
    if (FileExists("./ffmpeg")) return "./ffmpeg";
    if (FileExists("/opt/homebrew/bin/ffmpeg")) return "/opt/homebrew/bin/ffmpeg";  //macOS video failure fixed!!! thanks to ADR Sir..
    if (FileExists("/usr/local/bin/ffmpeg")) return "/usr/local/bin/ffmpeg";
    if (FileExists("/usr/bin/ffmpeg")) return "/usr/bin/ffmpeg";
    return "ffmpeg";
}

const char* GetVideoPath(int vidType)
{
    if (vidType == 0)
    {
        if (FileExists("assets/sprites/Starting_Vid.mp4")) return "assets/sprites/Starting_Vid.mp4";
        if (FileExists("assets/sprites/starting_vid.mp4")) return "assets/sprites/starting_vid.mp4"; //macOS video failure fixed!!! thanks to ADR Sir..
        if (FileExists("assets/sprites/Starting_vid.mp4")) return "assets/sprites/Starting_vid.mp4";
        if (FileExists("assets/sprites/Start_Vid.mp4")) return "assets/sprites/Start_Vid.mp4";
        if (FileExists("assets/sprites/launch.mp4")) return "assets/sprites/launch.mp4";
        if (FileExists("Starting_Vid.mp4")) return "Starting_Vid.mp4";
        return "assets/sprites/Starting_Vid.mp4";
    }
    else if (vidType == 1)
    {
        if (FileExists("assets/sprites/Game_over_vid.mp4")) return "assets/sprites/Game_over_vid.mp4";
        if (FileExists("assets/sprites/game_over_vid.mp4")) return "assets/sprites/game_over_vid.mp4";
        if (FileExists("assets/sprites/Game_Over_Vid.mp4")) return "assets/sprites/Game_Over_Vid.mp4";
        if (FileExists("assets/sprites/game_over.mp4")) return "assets/sprites/game_over.mp4";
        if (FileExists("Game_over_vid.mp4")) return "Game_over_vid.mp4";
        return "assets/sprites/Game_over_vid.mp4";
    }
    else
    {
        if (FileExists("assets/sprites/Game_win_vid.mp4")) return "assets/sprites/Game_win_vid.mp4";
        if (FileExists("assets/sprites/game_win_vid.mp4")) return "assets/sprites/game_win_vid.mp4";
        if (FileExists("assets/sprites/Game_Win_Vid.mp4")) return "assets/sprites/Game_Win_Vid.mp4";
        if (FileExists("assets/sprites/game_win.mp4")) return "assets/sprites/game_win.mp4";
        if (FileExists("Game_win_vid.mp4")) return "Game_win_vid.mp4";
        return "assets/sprites/Game_win_vid.mp4";
    }
}

static bool ReadExactBytes(FILE* stream, void* buffer, size_t total_bytes)
{
    if (stream == NULL || buffer == NULL) return false;
    size_t total_read = 0;
    char* ptr = (char*)buffer;
    while (total_read < total_bytes)
    {
        size_t bytes_to_read = total_bytes - total_read;
        size_t n = fread(ptr + total_read, 1, bytes_to_read, stream);  //macOS video failure fixed!!! thanks to ADR Sir..
        if (n == 0)
        {
            if (feof(stream) || ferror(stream)) return false;
        }
        total_read += n;
    }
    return true;
}

void StartVideo(int vidType, float duration)
{
    currentVideoType = vidType;
    videoDuration = duration;
    videoTimer = 0.0f;
    videoFrameTimer = 0.0f;         //macOS video failure fixed!!! thanks to ADR Sir..
    videoIsPlaying = true;
    hasReceivedFrames = false;

    system("killall afplay 2>/dev/null");

    const char* vPath = GetVideoPath(vidType);
    char afplayCmd[512];
    snprintf(afplayCmd, sizeof(afplayCmd), "afplay \"%s\" &", vPath);
    system(afplayCmd);

    int ffmpegCheck = system("export PATH=\"/opt/homebrew/bin:/usr/local/bin:$PATH\"; which ffmpeg > /dev/null 2>&1");
    if (ffmpegCheck != 0 && !FileExists("/opt/homebrew/bin/ffmpeg") && !FileExists("/usr/local/bin/ffmpeg") && !FileExists("./ffmpeg"))
    {
        ffmpegInstalled = false;
        videoPipe = NULL;
        return;
    }
    ffmpegInstalled = true;

    char ffmpegCmd[1024];
    snprintf(ffmpegCmd, sizeof(ffmpegCmd),
        "export PATH=\"/opt/homebrew/bin:/usr/local/bin:$PATH\"; "
        "%s -loglevel error -i \"%s\" -an -sn -r 30 -s %dx%d -pix_fmt rgba -f rawvideo -",
        GetFFmpegBinary(), vPath, VID_W, VID_H);

    videoPipe = popen(ffmpegCmd, "r");
}

void StopVideo(void)
{
    if (videoPipe != NULL)
    {
        pclose(videoPipe);
        videoPipe = NULL;
    }
    system("killall afplay 2>/dev/null");
    videoIsPlaying = false;
}

void DownAlien(int AlienInX, int AlienInY, Vector2 AlienPos[AlienInX][AlienInY], bool AlienAlive[AlienInX][AlienInY])
{
    float maxAlienY = WindowHeight * 0.70f;
    bool canMoveDown = true;

    for (int X = 0; X < AlienInX; X++)
    {
        for (int Y = 0; Y < AlienInY; Y++)
        {
            if (AlienAlive[X][Y] && (AlienPos[X][Y].y + AlienSize + 20.0f > maxAlienY)) //Alien teleport to down layers...thanks to Shoab!
            {
                canMoveDown = false;
                break;
            }
        }
        if (!canMoveDown) break;
    }

    if (canMoveDown)
    {
        for (int X = 0; X < AlienInX; X++)
        {
            for (int Y = 0; Y < AlienInY; Y++)  //setting the conditional!!
            {
                AlienPos[X][Y].y += 20.0f;
            }
        }
    }
}

// Stealth hero(nayemul's one) sprite flame animations
void DrawStealthFlames(Vector2 heroCenterPos, float planeWidth, float planeHeight)
{
    float time = (float)GetTime();
    Vector2 leftNozzle  = { heroCenterPos.x - planeWidth * 0.125f, heroCenterPos.y + planeHeight * 0.42f };
    Vector2 rightNozzle = { heroCenterPos.x + planeWidth * 0.125f, heroCenterPos.y + planeHeight * 0.42f };
    Vector2 nozzles[2] = { leftNozzle, rightNozzle };

    for (int n = 0; n < 2; n++)
    {
        float pulse = sinf(time * 35.0f + (n * 2.0f)) * 5.0f;
        float jitterX = (float)GetRandomValue(-2, 2);
        float baseLen = 32.0f + pulse + (float)GetRandomValue(0, 6);
        float nozzleHalfW = 6.5f;

        Vector2 v1_out = { nozzles[n].x - nozzleHalfW - 2.5f, nozzles[n].y };
        Vector2 v2_out = { nozzles[n].x + nozzleHalfW + 2.5f, nozzles[n].y };
        Vector2 v3_out = { nozzles[n].x + jitterX, nozzles[n].y + baseLen + 8.0f };
        DrawTriangle(v1_out, v3_out, v2_out, Fade((Color){ 140, 80, 255, 255 }, 0.40f));

        Vector2 v1_mid = { nozzles[n].x - nozzleHalfW, nozzles[n].y };
        Vector2 v2_mid = { nozzles[n].x + nozzleHalfW, nozzles[n].y };
        Vector2 v3_mid = { nozzles[n].x + jitterX * 0.5f, nozzles[n].y + baseLen };  //taken help from various sample games!!!
        DrawTriangle(v1_mid, v3_mid, v2_mid, (Color){ 90, 170, 255, 230 });

        Vector2 v1_in = { nozzles[n].x - nozzleHalfW * 0.45f, nozzles[n].y };
        Vector2 v2_in = { nozzles[n].x + nozzleHalfW * 0.45f, nozzles[n].y };
        Vector2 v3_in = { nozzles[n].x, nozzles[n].y + (baseLen * 0.55f) };
        DrawTriangle(v1_in, v3_in, v2_in, WHITE);

        DrawCircle((int)nozzles[n].x, (int)nozzles[n].y + 4, 8.0f, Fade(SKYBLUE, 0.6f));
        DrawCircle((int)nozzles[n].x, (int)nozzles[n].y + 2, 4.0f, WHITE);
    }
}

// Dialogue box for story mode :) Nayemul's idea 
void DrawCyberBox(Rectangle rec, Color borderColor, Color bgColor, const char* title, Color titleColor)
{
    DrawRectangleRec(rec, bgColor);
    DrawRectangleLinesEx(rec, 2.0f, borderColor);

    float arm = 16.0f;
    DrawLineEx((Vector2){ rec.x - 3, rec.y - 3 }, (Vector2){ rec.x + arm, rec.y - 3 }, 3.0f, titleColor);
    DrawLineEx((Vector2){ rec.x - 3, rec.y - 3 }, (Vector2){ rec.x - 3, rec.y + arm }, 3.0f, titleColor);

    DrawLineEx((Vector2){ rec.x + rec.width + 3, rec.y - 3 }, (Vector2){ rec.x + rec.width - arm, rec.y - 3 }, 3.0f, titleColor);
    DrawLineEx((Vector2){ rec.x + rec.width + 3, rec.y - 3 }, (Vector2){ rec.x + rec.width + 3, rec.y + arm }, 3.0f, titleColor);

    DrawLineEx((Vector2){ rec.x - 3, rec.y + rec.height + 3 }, (Vector2){ rec.x + arm, rec.y + rec.height + 3 }, 3.0f, titleColor);
    DrawLineEx((Vector2){ rec.x - 3, rec.y + rec.height + 3 }, (Vector2){ rec.x - 3, rec.y + rec.height - arm }, 3.0f, titleColor);

    DrawLineEx((Vector2){ rec.x + rec.width + 3, rec.y + rec.height + 3 }, (Vector2){ rec.x + rec.width - arm, rec.y + rec.height + 3 }, 3.0f, titleColor);
    DrawLineEx((Vector2){ rec.x + rec.width + 3, rec.y + rec.height + 3 }, (Vector2){ rec.x + rec.width + 3, rec.y + arm }, 3.0f, titleColor);

    if (title != NULL && strlen(title) > 0)
    {
        int tWidth = MeasureText(title, 18);
        DrawRectangle((int)rec.x + 24, (int)rec.y - 13, tWidth + 24, 26, Fade(BLACK, 0.95f));
        DrawRectangleLines((int)rec.x + 24, (int)rec.y - 13, tWidth + 24, 26, borderColor);
        DrawText(title, (int)rec.x + 36, (int)rec.y - 9, 18, titleColor);
    }
}

// Alien Dreadnought Encrypted fancy dialogue box design,thanks to alien shooter game!!!
void DrawAlienGlitchBox(Rectangle rec)
{
    DrawRectangleRec(rec, Fade((Color){ 18, 5, 8, 255 }, 0.95f));
    DrawRectangleLinesEx(rec, 3.0f, RED);
    DrawRectangleLinesEx((Rectangle){ rec.x + 4, rec.y + 4, rec.width - 8, rec.height - 8 }, 1.5f, MAROON);

    for (int x = (int)rec.x + 12; x < (int)(rec.x + rec.width - 24); x += 28)
    {
        DrawLineEx((Vector2){ (float)x, rec.y + 4 }, (Vector2){ (float)(x + 14), rec.y + 16 }, 2.0f, Fade(RED, 0.6f));
    }

    if (((int)(GetTime() * 14.0f)) % 3 == 0)
    {
        float gY = rec.y + (float)GetRandomValue(30, (int)rec.height - 30);
        DrawLineEx((Vector2){ rec.x + 8, gY }, (Vector2){ rec.x + rec.width - 8, gY }, 1.5f, Fade(RED, 0.7f));
    }

    const char* alertTag = ">> INCOMING OVERLORD TRANSMISSION // ENCRYPTED THREAT FREQUENCY <<";
    int atW = MeasureText(alertTag, 18);
    DrawRectangle((int)rec.x + 30, (int)rec.y - 13, atW + 24, 26, Fade(BLACK, 0.95f));
    DrawRectangleLines((int)rec.x + 30, (int)rec.y - 13, atW + 24, 26, RED);
    DrawText(alertTag, (int)rec.x + 42, (int)rec.y - 9, 18, RED);
}

// Real-Time International News fancy dialogue box,hehehehehehe...
void DrawBreakingNewsBox(Rectangle rec, const char* headline, const char* locationTag, bool isVictory)
{
    DrawRectangleRec(rec, Fade(isVictory ? (Color){ 6, 20, 16, 255 } : (Color){ 20, 6, 8, 255 }, 0.96f));
    DrawRectangleLinesEx(rec, 2.5f, isVictory ? LIME : RED);

    DrawRectangle((int)rec.x, (int)rec.y, (int)rec.width, 42, isVictory ? DARKBLUE : MAROON);
    DrawLine((int)rec.x, (int)rec.y + 42, (int)(rec.x + rec.width), (int)rec.y + 42, isVictory ? SKYBLUE : RED);

    float livePulse = fabsf(sinf((float)GetTime() * 6.0f));
    DrawRectangle((int)rec.x + 18, (int)rec.y + 9, 74, 24, RED);
    DrawCircle((int)rec.x + 32, (int)rec.y + 21, 5.0f, Fade(WHITE, livePulse));
    DrawText("LIVE", (int)rec.x + 44, (int)rec.y + 13, 16, WHITE);

    const char* gnn = "GNN // GLOBAL NEWS NETWORK SPECIAL REPORT";
    DrawText(gnn, (int)rec.x + 105, (int)rec.y + 12, 18, GOLD);

    if (locationTag != NULL)
    {
        int locW = MeasureText(locationTag, 14);
        DrawText(locationTag, (int)(rec.x + rec.width - locW - 20), (int)rec.y + 14, 14, SKYBLUE);
    }

    int bannerY = (int)rec.y + 54;
    DrawRectangle((int)rec.x + 18, bannerY, (int)rec.width - 36, 44, Fade(BLACK, 0.75f));
    DrawRectangleLines((int)rec.x + 18, bannerY, (int)rec.width - 36, 44, isVictory ? GREEN : RED);
    DrawText(headline, (int)rec.x + 34, bannerY + 10, 24, isVictory ? LIME : RED);

    int tickerY = (int)(rec.y + rec.height - 32);
    DrawRectangle((int)rec.x, tickerY, (int)rec.width, 32, Fade(BLACK, 0.85f));
    DrawLine((int)rec.x, tickerY, (int)(rec.x + rec.width), tickerY, DARKGRAY);

    // fixed the ticker bug :)
    const char* tickerTxt = isVictory ?
        "TICKER >>> SECTOR 04 ALL-CLEAR ... CELEBRATIONS IN DHAKA, TOKYO, LONDON, NYC ... HEROES RETURNING SAFE >>>" :
        "TICKER >>> DEFENSE GRID ZERO BREACHED ... EXOSPHERIC SENSORS OFFLINE ... CITIZEN EVACUATION DIRECTIVE IN EFFECT >>>";
    DrawText(tickerTxt, (int)rec.x + 20, tickerY + 8, 14, isVictory ? SKYBLUE : ORANGE);
}

int main(void)
{
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(WindowWidth, WindowHeight, "Space Invaders");
    SetExitKey(KEY_NULL);
    
    // Unlocked nmacOS Fullscreen mode on traffic light green button
    EnableMacOSNativeFullscreen();

    InitAudioDevice();
    SetTargetFPS(FPS);

    int curMon = GetCurrentMonitor();
    int monW = GetMonitorWidth(curMon);
    int monH = GetMonitorHeight(curMon);
    SetWindowPosition((monW - WindowWidth) / 2, (monH - WindowHeight) / 2);

    if (!DirectoryExists("assets") && DirectoryExists("../assets"))
    {
        ChangeDirectory("..");
    }

    //rendering visuals :)
    RenderTexture2D renderTarget = LoadRenderTexture(WindowWidth, WindowHeight);
    SetTextureFilter(renderTarget.texture, TEXTURE_FILTER_BILINEAR);

    Image dummyImg = GenImageColor(VID_W, VID_H, BLACK);
    videoFrameTex = LoadTextureFromImage(dummyImg);
    UnloadImage(dummyImg);
    videoFramePixels = (Color*)malloc(VID_W * VID_H * sizeof(Color));

    Vector2 StarPos[STAR_COUNT]; //stars will be flowing in background,giving space vibes :)
    float StarSpeed[STAR_COUNT];
    for (int i = 0; i < STAR_COUNT; i++)
    {
        StarPos[i] = (Vector2){ (float)GetRandomValue(0, WindowWidth), (float)GetRandomValue(0, WindowHeight) };
        if (i < 50) StarSpeed[i] = (float)GetRandomValue(28, 42);
        else if (i < 80) StarSpeed[i] = (float)GetRandomValue(85, 115);
        else StarSpeed[i] = (float)GetRandomValue(210, 260);
    }

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
    Sound AlienShoot = LoadSound("assets/audio/alienshoot1.wav"); //all sounds and bgm loading....
    Sound menuMove = LoadSound("assets/audio/sfx_menu_move4.wav");
    Sound menuSelect = LoadSound("assets/audio/sfx_menu_select2.wav");
    Sound heroDeath = LoadSound("assets/audio/sfx_deathscream_robot4.wav");
    Sound pauseIn = LoadSound("assets/audio/sfx_sounds_pause4_in.wav");
    Sound pauseOut = LoadSound("assets/audio/sfx_sounds_pause4_out.wav");
    Sound damage = LoadSound("assets/audio/sfx_sounds_damage3.wav");
    Sound cheer = LoadSound("assets/audio/cheering.wav");

    Sound heroOuch = LoadSound("assets/audio/hero_hit.wav");
    Sound bossLaserSound = LoadSound("assets/audio/laser_beam.wav");

    Sound sndMagRailCharge   = LoadSound("assets/audio/sfx_mag_rail_charge.wav");
    Sound sndLaserCharge     = LoadSound("assets/audio/sfx_laser_charge.wav");
    Sound sndBossWarpIn      = LoadSound("assets/audio/sfx_boss_warp_in.wav");
    Sound sndAlienStep       = LoadSound("assets/audio/sfx_alien_step.wav");
    Sound sndDryFire         = LoadSound("assets/audio/sfx_dry_fire.wav");
    Sound sndTetherConnect   = LoadSound("assets/audio/sfx_tether_connect.wav");
    Sound sndAirdropIncoming = LoadSound("assets/audio/sfx_airdrop_incoming.wav");

    Sound sndAsteroidHitHero = LoadSound("assets/audio/sfx_asteroid_hit_hero.wav");
    Sound sndAsteroidRicochet= LoadSound("assets/audio/sfx_asteroid_ricochet.wav"); //setting every sfx and bgm
    Sound sndAsteroidShatter = LoadSound("assets/audio/sfx_asteroid_shatter.wav");
    Sound sndCockpitSpark    = LoadSound("assets/audio/sfx_cockpit_spark.wav");
    Sound sndHullAlarm       = LoadSound("assets/audio/sfx_hull_alarm.wav");
    Sound sndRankS           = LoadSound("assets/audio/sfx_rank_s.wav");
    Sound sndRankBadge       = LoadSound("assets/audio/sfx_rank_badge.wav");

    // Load Black Hole Visuals and Sounds safely
    Texture2D texBlackHoleCore = LoadTexture("assets/sprites/black_hole_core.png");
    Texture2D texBlackHoleDisk = LoadTexture("assets/sprites/black_hole_disk.png");

    Sound sndBlackHoleDrone = LoadSound("assets/audio/sfx_blackhole_drone.wav");
    Sound sndBlackHolePull  = LoadSound("assets/audio/sfx_blackhole_pull.wav");
    Sound sndBlackHoleCrush = LoadSound("assets/audio/sfx_blackhole_crush.wav");

    // Load Mysterious Black Entity (Void Phantom) Visuals and Haunted Sounds
    Texture2D texEntityPhantom[4];
    for (int ep = 0; ep < 4; ep++) {
        texEntityPhantom[ep] = LoadTexture(TextFormat("assets/sprites/entity_phantom%d.png", ep + 1));
    }
    Texture2D texEntityOrb = LoadTexture("assets/sprites/entity_void_orb.png");

    Sound sndEntitySpawn  = LoadSound("assets/audio/sfx_entity_spawn.wav");
    Sound sndEntityDrone  = LoadSound("assets/audio/sfx_entity_drone.wav");
    Sound sndEntityAttack = LoadSound("assets/audio/sfx_entity_attack.wav");
    Sound sndEntityScream = LoadSound("assets/audio/sfx_entity_scream.wav");

    BlackHole blackHole = {
        .pos = (Vector2){ WindowWidth / 2.0f, 60.0f },
        .basePosX = WindowWidth / 2.0f,
        .pullRadius = 450.0f,
        .shearRadius = 180.0f,
        .coreRadius = 35.0f,
        .maxPullForce = 380.0f,
        .driftTimer = 0.0f,
        .rotationAngle = 0.0f,
        .scale = 1.0f,
        .alpha = 1.0f,
        .active = false,
        .spawned = false,
        .collapsing = false,
        .collapseTimer = 0.0f
    };

    VoidEntity voidEntity = {
        .pos = (Vector2){ WindowWidth / 2.0f, 180.0f },
        .speed = (Vector2){ 110.0f, 0.0f },
        .hp = 60,
        .maxHp = 60,
        .active = false,
        .spawned = false,
        .animTimer = 0.0f,
        .currentFrame = 0,
        .attackTimer = 2.0f,
        .phaseTimer = 5.0f,
        .phasing = false,
        .screamTimer = 8.0f,
        .screaming = false,
        .alpha = 0.0f,
        .targetedHeroTimer = 0.0f,
        .targetedHero = 1
    };

    EntityOrb entityOrbs[MAX_ENTITY_ORBS];
    for (int eo = 0; eo < MAX_ENTITY_ORBS; eo++) {
        entityOrbs[eo].active = false;
        entityOrbs[eo].pos = (Vector2){ 0, 0 };
        entityOrbs[eo].vel = (Vector2){ 0, 0 };
    }

    MatchRecord historyList[MAX_MATCH_RECORDS];
    int historyCount = 0;
    int historyScroll = 0;
    bool currentMatchSaved = false;

    HeroCareerProfile careerProfiles[2];
    LoadHeroProfiles(careerProfiles);

    char inputHero1Name[24] = "Shoab";
    char inputHero2Name[24] = "Nayemul";
    int nameEntryStep = 0; // 0 = Shoab ship, 1 = Nayemul ship
    int letterCount1 = 5;
    int letterCount2 = 7;

    bool magRailSoundPlayed = false;
    bool laserChargeSoundPlayed = false;
    bool prevHeroesTethered = false;

    Music bgmStory = LoadMusicStream("assets/audio/2-air-strike-2-ost-track-2-ih-23-xz.wav");
    Music bgmMenu  = LoadMusicStream("assets/audio/air-strike-3-d-ii-gulf-thunder-main-ost-best-quality-mf-0-j-34.wav");
    Music bgmBoss  = LoadMusicStream("assets/audio/air-strike-3-d-ost-fear-drigto.wav");
    Music bgmWin   = LoadMusicStream("assets/audio/game_win_bgm.wav");
    Music bgmLost  = LoadMusicStream("assets/audio/game_lost_bgm.wav");

    Music bgmGameplay[3];
    bgmGameplay[0] = LoadMusicStream(FileExists("assets/audio/aggressive_bgm.wav") ? "assets/audio/aggressive_bgm.wav" : (FileExists("assets/audio/bgm_combat_aggressive.wav") ? "assets/audio/bgm_combat_aggressive.wav" : "assets/audio/bgm_combat_1.wav"));
    bgmGameplay[1] = LoadMusicStream(FileExists("assets/audio/air_strike_bgm.wav") ? "assets/audio/air_strike_bgm.wav" : (FileExists("assets/audio/bgm_combat_casual.wav") ? "assets/audio/bgm_combat_casual.wav" : "assets/audio/bgm_combat_2.wav"));
    
    // Fixed: Robust multi-format fallback for Track 3 (.wav prioritized over .mp3)
    const char* softBgmPath = "assets/audio/bgm_combat_soft.wav";
    if (FileExists("assets/audio/casual_retro_bgm_fixed.wav")) softBgmPath = "assets/audio/casual_retro_bgm_fixed.wav";
    else if (FileExists("assets/audio/bgm_combat_soft.wav")) softBgmPath = "assets/audio/bgm_combat_soft.wav";
    else if (FileExists("assets/audio/casual_retro_bgm.wav")) softBgmPath = "assets/audio/casual_retro_bgm.wav";
    else if (FileExists("assets/audio/bgm_combat_3.wav")) softBgmPath = "assets/audio/bgm_combat_3.wav";
    else if (FileExists("assets/audio/bgm_combat_soft.mp3")) softBgmPath = "assets/audio/bgm_combat_soft.mp3";
    else if (FileExists("assets/audio/bgm_combat_soft_2.mp3")) softBgmPath = "assets/audio/bgm_combat_soft_2.mp3";
    else if (FileExists("assets/audio/casual_retro_bgm.mp3")) softBgmPath = "assets/audio/casual_retro_bgm.mp3";
    else softBgmPath = "assets/audio/bgm_combat_soft.wav";
    bgmGameplay[2] = LoadMusicStream(softBgmPath);

    bool gameplayBgmActive = false;

    Texture2D BossTexture[5];
    BossTexture[0] = LoadTexture("assets/sprites/boss_idle.png");
    BossTexture[1] = LoadTexture("assets/sprites/boss_down.png");
    BossTexture[2] = LoadTexture("assets/sprites/boss_up.png");
    BossTexture[3] = LoadTexture("assets/sprites/boss_pulse1.png");
    BossTexture[4] = LoadTexture("assets/sprites/boss_pulse2.png");

    Texture2D HeroTexture = LoadTexture("assets/sprites/Hero.png");
    Texture2D StealthHeroTexture = LoadTexture("assets/sprites/Hero_stealth.png"); //all sprites and image loading....

    Texture2D loadingBg = LoadTexture("assets/sprites/loading_screen.png");
    if (loadingBg.id == 0) loadingBg = LoadTexture("assets/sprites/loading_screen.jpg");
    if (loadingBg.id == 0) loadingBg = LoadTexture("assets/sprites/loading_screen.jpeg");

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

    Texture2D portraitShoabCrit = LoadTexture("assets/sprites/shoab_hero_crit.png");
    if (portraitShoabCrit.id == 0) portraitShoabCrit = portraitShoab;

    Texture2D portraitNayemulCrit = LoadTexture("assets/sprites/nayemul_hero_crit.png");
    if (portraitNayemulCrit.id == 0) portraitNayemulCrit = portraitNayemul;

    // Load & precision-crop developer portraits into clean 300x300 textures
    Texture2D credPortraitNayemul = { 0 };
    if (FileExists("assets/sprites/Nayemul.png"))
    {
        Image nImg = LoadImage("assets/sprites/Nayemul.png");
        if (nImg.data != NULL)
        {
            // Zoom in directly onto Nayemul (focusing on head, glasses, and torso)
            int cropSize = (int)(nImg.width * 0.32f);
            if (cropSize > nImg.height) cropSize = nImg.height;
            int cropX = (int)(nImg.width * 0.48f) - (cropSize / 2);
            int cropY = (int)(nImg.height * 0.53f) - (cropSize / 2);
            if (cropX < 0) cropX = 0;
            if (cropY < 0) cropY = 0;
            if (cropX + cropSize > nImg.width) cropX = nImg.width - cropSize;
            if (cropY + cropSize > nImg.height) cropY = nImg.height - cropSize;

            ImageCrop(&nImg, (Rectangle){ (float)cropX, (float)cropY, (float)cropSize, (float)cropSize });
            ImageResize(&nImg, 300, 300);
            credPortraitNayemul = LoadTextureFromImage(nImg);
            SetTextureFilter(credPortraitNayemul, TEXTURE_FILTER_BILINEAR);
            UnloadImage(nImg);
        }
    }
    if (credPortraitNayemul.id == 0) credPortraitNayemul = portraitNayemul;

    Texture2D credPortraitShoab = { 0 };
    if (FileExists("assets/sprites/Shoab.png"))
    {
        Image sImg = LoadImage("assets/sprites/Shoab.png");
        if (sImg.data != NULL)
        {
            // Zoomed portrait focused clearly on Shoab's face and upper chest
            int cropSize = (int)(sImg.width * 0.70f);
            if (cropSize > sImg.height) cropSize = sImg.height;
            int cropX = (int)(sImg.width * 0.50f) - (cropSize / 2);
            int cropY = (int)(sImg.height * 0.38f) - (cropSize / 2);
            if (cropX < 0) cropX = 0;
            if (cropY < 0) cropY = 0;
            if (cropX + cropSize > sImg.width) cropX = sImg.width - cropSize;
            if (cropY + cropSize > sImg.height) cropY = sImg.height - cropSize;

            ImageCrop(&sImg, (Rectangle){ (float)cropX, (float)cropY, (float)cropSize, (float)cropSize });
            ImageResize(&sImg, 300, 300);
            credPortraitShoab = LoadTextureFromImage(sImg);
            SetTextureFilter(credPortraitShoab, TEXTURE_FILTER_BILINEAR);
            UnloadImage(sImg);
        }
    }
    if (credPortraitShoab.id == 0) credPortraitShoab = portraitShoab;

    Texture2D clusterBombTex   = LoadTexture("assets/sprites/cluster_bomb_idle.png");
    Texture2D clusterBlastTex  = LoadTexture("assets/sprites/cluster_bomb_blast.png");

    Texture2D teamShieldDomeTex = LoadTexture("assets/sprites/team_shield_dome.png");
    Sound sndShieldActivate     = LoadSound("assets/audio/sfx_shield_activate.wav");
    Sound sndShieldDeflect      = LoadSound("assets/audio/sfx_shield_deflect.wav");

    Sound sndHudGlitch          = LoadSound("assets/audio/sfx_hud_glitch.wav");
    Sound sndEvacBoom           = LoadSound("assets/audio/sfx_evac_boom.wav");

    Texture2D texAsteroid    = LoadTexture("assets/sprites/asteroid_metallic.png");
    Texture2D texDebris      = LoadTexture("assets/sprites/asteroid_debris.png");
    Texture2D texSmoke       = LoadTexture("assets/sprites/smoke_puff.png");
    Texture2D texSpark       = LoadTexture("assets/sprites/spark_particle.png");
    Texture2D texRankBadgeS  = LoadTexture("assets/sprites/rank_badge_s.png");
    Texture2D texRankBadgeA  = LoadTexture("assets/sprites/rank_badge_a.png");
    Texture2D texRankBadgeB  = LoadTexture("assets/sprites/rank_badge_b.png");
    Texture2D texRankBadgeC  = LoadTexture("assets/sprites/rank_badge_c.png");

    Asteroid asteroids[MAX_ASTEROIDS];
    for (int a = 0; a < MAX_ASTEROIDS; a++) {
        asteroids[a].active = false;
        asteroids[a].hp = 0.0f;
    }
    float asteroidSpawnTimer = 0.0f;

    AsteroidDebris debrisPool[MAX_ASTEROID_DEBRIS];
    for (int d = 0; d < MAX_ASTEROID_DEBRIS; d++) debrisPool[d].active = false;

    SmokeParticle smokePool[MAX_SMOKE_PARTICLES];
    for (int s = 0; s < MAX_SMOKE_PARTICLES; s++) smokePool[s].active = false; //space particles conditional logic!!

    SparkParticle sparkPool[MAX_SPARK_PARTICLES];
    for (int sp = 0; sp < MAX_SPARK_PARTICLES; sp++) sparkPool[sp].active = false;

    float sparkTimer1 = 0.0f, sparkTimer2 = 0.0f;
    float alarmSoundTimer = 0.0f;
    float runPlayTime = 0.0f;
    bool rankAudioPlayed = false;

    bool commandersTriggered  = false;
    bool commanderIntroActive = false;
    float commanderIntroTimer = 0.0f;
    float gameTimeDilation    = 1.0f;

    Vector2 jammerPos            = { 0, 0 };
    Vector2 jammerSpeed          = { 0, 0 };
    int jammerHp                 = 0;
    int jammerMaxHp              = 35;
    bool jammerActive            = false;
    float jammerShootTimer       = 0.0f;
    float jammerActionCooldown   = 0.0f;
    float jammerPowerTimer       = 0.0f;
    int jammerAnimState          = 0;

    Vector2 warpPos              = { 0, 0 };
    Vector2 warpSpeed            = { 0, 0 };
    int warpHp                   = 0;
    int warpMaxHp                = 30;
    bool warpActive              = false;
    float warpShootTimer         = 0.0f;
    float warpActionCooldown     = 0.0f;
    float warpPowerTimer         = 0.0f;
    int warpAnimState            = 0;

    float radarJammedTimer       = 0.0f;

    Vector2 jammerBulletPos[MAX_COMMANDER_BULLETS];
    Vector2 jammerBulletVel[MAX_COMMANDER_BULLETS];
    bool jammerBulletActive[MAX_COMMANDER_BULLETS] = { false };
    bool jammerBulletLocked[MAX_COMMANDER_BULLETS] = { false };

    Vector2 warpBulletPos[MAX_COMMANDER_BULLETS];
    bool warpBulletActive[MAX_COMMANDER_BULLETS] = { false };

    Texture2D Hero1SpecialBulletTex[7];
    Hero1SpecialBulletTex[0] = LoadTexture("assets/sprites/1.png");
    Hero1SpecialBulletTex[1] = LoadTexture("assets/sprites/2.png");
    Hero1SpecialBulletTex[2] = LoadTexture("assets/sprites/3.png");
    Hero1SpecialBulletTex[3] = LoadTexture("assets/sprites/4.png");
    Hero1SpecialBulletTex[4] = LoadTexture("assets/sprites/5.png");
    Hero1SpecialBulletTex[5] = LoadTexture("assets/sprites/6.png");
    Hero1SpecialBulletTex[6] = LoadTexture("assets/sprites/7.png");

    Texture2D jammerTex[4];
    jammerTex[0] = LoadTexture("assets/sprites/commander_jammer_idle.png");
    jammerTex[1] = LoadTexture("assets/sprites/commander_jammer_move_y.png"); //alien commander sprites fixed!!!
    jammerTex[2] = LoadTexture("assets/sprites/commander_jammer_move_x.png");
    jammerTex[3] = LoadTexture("assets/sprites/commander_jammer_power.png");

    Texture2D warpTex[4];
    warpTex[0] = LoadTexture("assets/sprites/commander_warp_idle.png");
    warpTex[1] = LoadTexture("assets/sprites/commander_warp_move_y.png");
    warpTex[2] = LoadTexture("assets/sprites/commander_warp_move_x.png");
    warpTex[3] = LoadTexture("assets/sprites/commander_warp_power.png");

    Sound sndJammerHum   = LoadSound("assets/audio/sfx_jammer_hum.wav");
    Sound sndJammerShot  = LoadSound("assets/audio/sfx_jammer_shot.wav");
    Sound sndEmpBlast    = LoadSound("assets/audio/sfx_emp_blast.wav");
    Sound sndJammerDeath = LoadSound("assets/audio/sfx_jammer_death.wav");
    Sound sndWarpGlide   = LoadSound("assets/audio/sfx_warp_glide.wav");
    Sound sndWarpShot    = LoadSound("assets/audio/sfx_warp_shot.wav");
    Sound sndTeleport    = LoadSound("assets/audio/sfx_teleport.wav");
    Sound sndWarpDeath   = LoadSound("assets/audio/sfx_warp_death.wav");

    Sound sndSpecialBeam  = LoadSound("assets/audio/sfx_special_beam.wav");
    Sound sndChargeReady  = LoadSound("assets/audio/sfx_charge_ready.wav");
    Sound sndPodDestroy   = LoadSound("assets/audio/sfx_pod_destroy.wav"); //sfx from different games!!!
    Sound sndOrbLaunch    = LoadSound("assets/audio/sfx_orb_launch.wav");
    Sound sndWarningSiren = LoadSound("assets/audio/sfx_warning_siren.wav");
    Sound sndDefibHum     = LoadSound("assets/audio/sfx_defib_hum.wav");
    Sound bossEnrage      = LoadSound("assets/audio/sfx_boss_enrage.wav");
    Sound sndRocketBoost  = LoadSound("assets/audio/sfx_rocket_boost.wav");
    Sound sndClusterLaunch  = LoadSound("assets/audio/sfx_cluster_launch.wav");
    Sound sndClusterExplode = LoadSound("assets/audio/sfx_cluster_explode.wav");

    bool bossEnraged = false;

    Vector2 clusterMissilePos[MAX_CLUSTER_MISSILES];
    Vector2 clusterMissileVel[MAX_CLUSTER_MISSILES];
    bool clusterMissileActive[MAX_CLUSTER_MISSILES] = { false };

    Vector2 clusterBlastPos[MAX_CLUSTER_BLASTS];
    float clusterBlastRadius[MAX_CLUSTER_BLASTS] = { 0.0f };
    float clusterBlastTimer[MAX_CLUSTER_BLASTS] = { 0.0f };
    bool clusterBlastActive[MAX_CLUSTER_BLASTS] = { false };

    float explosionShakeTimer = 0.0f;
    Camera2D screenCamera = { 0 }; //camera shaking effect,got from raylib cheatsheet :)
    screenCamera.zoom = 1.0f;

    int Hero1Lives = 4, Hero1Score = 0;
    Vector2 Hero1Pos = { WindowWidth * 0.35f, WindowHeight - HeroHeight };
    Vector2 Hero1Speed = { 0, 0 };
    Vector2 Hero1BulletPos[2] = { {0, 0}, {0, 0} };
    Vector2 Hero1SpecialBulletPos = { 0, 0 };
    bool Hero1BulletActive[2] = { false, false };
    bool Hero1SpecialBulletActive = false;
    bool hero1Debuffed = false;
    float hero1DebuffTimer = 0.0f;
    Vector2 Hero1CrashPos = { 0, 0 };
    float hero1ReviveTimer = 0.0f, hero1HitFlashTimer = 0.0f;

    int Hero2Lives = 4, Hero2Score = 0;
    Vector2 Hero2Pos = { WindowWidth * 0.65f, WindowHeight - HeroHeight };
    Vector2 Hero2Speed = { 0, 0 };
    Vector2 Hero2BulletPos[2] = { {0, 0}, {0, 0} };
    bool Hero2BulletActive[2] = { false, false };
    bool hero2Debuffed = false;
    float hero2DebuffTimer = 0.0f;
    Vector2 Hero2CrashPos = { 0, 0 };
    float hero2ReviveTimer = 0.0f, hero2HitFlashTimer = 0.0f;

    bool empDropped = false, empActive = false;
    Vector2 empPos = { 0, 0 };
    float empBuffTimer = 0.0f, empShockwaveRadius = 0.0f;
    Vector2 empShockwaveCenter = { 0, 0 };

    float cinematicTimer = 0.0f;
    int Hero1Kills = 0, Hero2Kills = 0;
    int Hero1MinionKills = 0, Hero2MinionKills = 0;
    int Hero1BossDamage = 0, Hero2BossDamage = 0;
    int Hero1HitsTaken = 0, Hero2HitsTaken = 0;

    Vector2 AlienBulletPos = { 0, 0 };
    bool AlienBulletActive = false;
    float AlienShootTimer = 0.0f;

    int AliensKilled = 0;
    bool GameOver = false, isPaused = false;
    float deathDelayTimer = 0.0f;
    bool deathSequenceActive = false;

    bool cheerPlayed = false, winSoundPlayed = false;
    bool winBgmStarted = false, lostBgmStarted = false;
    int winDialogueIndex = 0, lostDialogueIndex = 0, storyDialogueIndex = 0;

    int launchDialogueIndex = 0;
    float launchCountdownTimer = 3.0f;
    bool launchCountdownActive = false;
    Vector2 launchShip1Pos = { WindowWidth * 0.40f, WindowHeight - 240 };
    Vector2 launchShip2Pos = { WindowWidth * 0.60f, WindowHeight - 240 };

    bool bossSpawned = false, bossWarningActive = false;
    float bossWarningTimer = 0.0f;
    bool bossActive = false, bossDefeated = false;
    int bossHp = 100, bossLeftPodHp = BossPodMaxHp, bossRightPodHp = BossPodMaxHp;
    Vector2 bossPos = { (WindowWidth - BossWidth) / 2.0f, 60.0f };
    Vector2 bossSpeed = { 160.0f, 75.0f };
    float bossAnimTimer = 0.0f, bossDirChangeTimer = 0.0f;

    Vector2 minionPos[MAX_MINIONS], minionSpeed[MAX_MINIONS];
    bool minionActive[MAX_MINIONS] = { false };                      //structs weren't used earlier,so many variables are declared here...
    int minionHp[MAX_MINIONS] = { 0 };
    float minionShootTimer[MAX_MINIONS] = { 0 };
    float minionDeployTimer = 0.0f;

    Vector2 minionBulletPos[MAX_MINION_BULLETS];
    bool minionBulletActive[MAX_MINION_BULLETS] = { false };
    float bossShootTimer = 0.0f;
    Vector2 bossBulletPos[MAX_BOSS_BULLETS];
    bool bossBulletActive[MAX_BOSS_BULLETS] = { false };

    float bossLaserTimer = 0.0f;
    bool bossLaserActive = false;
    float bossLaserDuration = 0.0f, bossOrbTimer = 0.0f;
    Vector2 bossOrbPos[MAX_BOSS_ORBS], bossOrbVel[MAX_BOSS_ORBS];
    bool bossOrbActive[MAX_BOSS_ORBS] = { false };

    int CurrentState = STATE_LOADING;
    float LoadingTimer = 0.0f;
    int MenuSelection = 0, OptionsTab = 0, SoundSelection = 0, VideoSelection = 0, BgmTrackIndex = 0;
    int howToPlayTab = 0, creditsTab = 0;
    float BgmVolume = 0.8f, SfxVolume = 0.8f, Brightness = 1.0f;
    bool ScanlinesOn = true, StarfieldOn = true;

    while (!WindowShouldClose())
    {
        float rawTime = GetFrameTime();
        gameTimeDilation = commanderIntroActive ? 0.12f : 1.0f;
        float Time = rawTime * gameTimeDilation;

        // Native Spaces Fullscreen Toggle;works for windows but not for macOS :(
        if (IsKeyPressed(KEY_F) || IsKeyPressed(KEY_F11)) ToggleGameFullscreen();

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

        SetSoundVolume(shoot, SfxVolume); SetSoundVolume(AlienShoot, SfxVolume);
        SetSoundVolume(menuMove, SfxVolume); SetSoundVolume(menuSelect, SfxVolume);
        SetSoundVolume(heroDeath, SfxVolume); SetSoundVolume(pauseIn, SfxVolume);
        SetSoundVolume(pauseOut, SfxVolume); SetSoundVolume(damage, SfxVolume);
 
        SetSoundVolume(heroOuch, SfxVolume); SetSoundVolume(bossLaserSound, SfxVolume);
        SetSoundVolume(sndJammerHum, SfxVolume); SetSoundVolume(sndJammerShot, SfxVolume);
        SetSoundVolume(sndEmpBlast, SfxVolume); SetSoundVolume(sndJammerDeath, SfxVolume);
        SetSoundVolume(sndWarpGlide, SfxVolume); SetSoundVolume(sndWarpShot, SfxVolume);
        SetSoundVolume(sndTeleport, SfxVolume); SetSoundVolume(sndWarpDeath, SfxVolume);
        SetSoundVolume(sndSpecialBeam, SfxVolume); SetSoundVolume(sndChargeReady, SfxVolume);
        SetSoundVolume(sndPodDestroy, SfxVolume); SetSoundVolume(sndOrbLaunch, SfxVolume);
        SetSoundVolume(sndWarningSiren, SfxVolume); SetSoundVolume(sndDefibHum, SfxVolume);
        SetSoundVolume(bossEnrage, SfxVolume); SetSoundVolume(sndRocketBoost, SfxVolume);
        SetSoundVolume(sndClusterLaunch, SfxVolume); SetSoundVolume(sndClusterExplode, SfxVolume);
        SetSoundVolume(sndShieldActivate, SfxVolume); SetSoundVolume(sndShieldDeflect, SfxVolume);
        SetSoundVolume(sndHudGlitch, SfxVolume); SetSoundVolume(sndEvacBoom, SfxVolume);

        SetSoundVolume(sndMagRailCharge, SfxVolume);
        SetSoundVolume(sndLaserCharge, SfxVolume);
        SetSoundVolume(sndBossWarpIn, SfxVolume);
        SetSoundVolume(sndAlienStep, SfxVolume);
        SetSoundVolume(sndDryFire, SfxVolume);
        SetSoundVolume(sndTetherConnect, SfxVolume);
        SetSoundVolume(sndAirdropIncoming, SfxVolume);

        SetSoundVolume(sndAsteroidHitHero, SfxVolume);
        SetSoundVolume(sndAsteroidRicochet, SfxVolume); //setting every sfx and bgm
        SetSoundVolume(sndAsteroidShatter, SfxVolume);
        SetSoundVolume(sndCockpitSpark, SfxVolume);
        SetSoundVolume(sndHullAlarm, SfxVolume);
        SetSoundVolume(sndRankS, SfxVolume);
        SetSoundVolume(sndRankBadge, SfxVolume);

        // Black Hole Audio Levels (Increased slightly so drone & pull are clear and audible!)
        SetSoundVolume(sndBlackHoleDrone, SfxVolume * 1.50f);
        SetSoundVolume(sndBlackHolePull, SfxVolume * 1.40f);
        SetSoundVolume(sndBlackHoleCrush, SfxVolume * 1.40f);

        // Void Entity Audio Levels
        SetSoundVolume(sndEntitySpawn, SfxVolume);
        SetSoundVolume(sndEntityDrone, SfxVolume * 0.70f);
        SetSoundVolume(sndEntityAttack, SfxVolume);
        SetSoundVolume(sndEntityScream, SfxVolume);

        SetMusicVolume(bgmStory, BgmVolume);
        SetMusicVolume(bgmMenu, BgmVolume);
        SetMusicVolume(bgmBoss, BgmVolume);
        SetMusicVolume(bgmWin, BgmVolume);
        SetMusicVolume(bgmLost, BgmVolume);

        for (int g = 0; g < 3; g++)
        {
            SetMusicVolume(bgmGameplay[g], BgmVolume * 0.40f);
        }

        if (explosionShakeTimer > 0.0f && CurrentState == STATE_GAMEPLAY && !GameOver && !deathSequenceActive)
        {
            explosionShakeTimer -= rawTime;
            screenCamera.offset = (Vector2){ (float)GetRandomValue(-4, 4), (float)GetRandomValue(-4, 4) };
        }
        else if (bossLaserActive && CurrentState == STATE_GAMEPLAY && !GameOver && !deathSequenceActive)
        {
            screenCamera.offset = (Vector2){ (float)GetRandomValue(-6, 6), (float)GetRandomValue(-6, 6) };
        }
        else if (bossWarpActive && bossWarpTimer <= 0.8f && CurrentState == STATE_GAMEPLAY && !GameOver && !deathSequenceActive)
        {
            screenCamera.offset = (Vector2){ (float)GetRandomValue(-8, 8), (float)GetRandomValue(-8, 8) };
        }
        else
        {
            screenCamera.offset = (Vector2){ 0, 0 };
        }

        BeginTextureMode(renderTarget);
        ClearBackground(BLACK);
        BeginMode2D(screenCamera);

        if (StarfieldOn && CurrentState != STATE_LOADING) //star-field background
        {
            float nTime = (float)GetTime() * 0.04f;
            DrawCircleGradient((Vector2){ WindowWidth * 0.25f + sinf(nTime) * 60.0f, WindowHeight * 0.35f }, 320.0f, Fade(PURPLE, 0.09f), BLANK);
            DrawCircleGradient((Vector2){ WindowWidth * 0.78f - cosf(nTime * 0.8f) * 70.0f, WindowHeight * 0.55f }, 360.0f, Fade(DARKBLUE, 0.12f), BLANK);
            DrawCircleGradient((Vector2){ WindowWidth * 0.50f, WindowHeight * 0.20f }, 280.0f, Fade(SKYBLUE, 0.06f), BLANK);

            if (spaceLightningTimer > 0.0f)
            {
                spaceLightningTimer -= rawTime;
                DrawRectangle(0, 0, WindowWidth, WindowHeight, Fade(WHITE, 0.35f));
            }
        }

        if (StarfieldOn && CurrentState != STATE_LOADING)
        {
            for (int i = 0; i < STAR_COUNT; i++)
            {
                if (i < 50)
                {
                    DrawCircle((int)StarPos[i].x, (int)StarPos[i].y, 1.0f, DARKGRAY);
                }
                else if (i < 80)
                {
                    DrawCircle((int)StarPos[i].x, (int)StarPos[i].y, 1.6f, GRAY);
                }
                else
                {
                    DrawLine((int)StarPos[i].x, (int)StarPos[i].y, (int)StarPos[i].x, (int)StarPos[i].y + 7, SKYBLUE);
                }
            }
        }

        // State-loading screen (starting....)
        if (CurrentState == STATE_LOADING)
        {
            LoadingTimer += Time;
            float progress = LoadingTimer / 5.0f;
            if (progress > 1.0f) progress = 1.0f;

            if (loadingBg.id > 0)
            {
                DrawTexturePro(loadingBg, (Rectangle){ 0, 0, (float)loadingBg.width, (float)loadingBg.height },
                               (Rectangle){ 0, 0, (float)WindowWidth, (float)WindowHeight }, (Vector2){ 0, 0 }, 0.0f, WHITE);
            }

            char titleText[] = "SPACE INVADERS"; //game title!!
            int titleWidth = MeasureText(titleText, 58);
            DrawText(titleText, (WindowWidth - titleWidth) / 2 + 2, 42, 58, Fade(BLACK, 0.85f));
            DrawText(titleText, (WindowWidth - titleWidth) / 2, 40, 58, SKYBLUE);

            int barWidth = 640, barHeight = 28;
            int barX = (WindowWidth - barWidth) / 2, barY = WindowHeight - 96;

            DrawRectangle(0, WindowHeight - 165, WindowWidth, 165, Fade(BLACK, 0.65f));
            DrawLine(0, WindowHeight - 165, WindowWidth, WindowHeight - 165, Fade(SKYBLUE, 0.40f));

            char subText[] = "INITIALIZING DEFENSE SATELLITES & RADAR...";
            DrawText(subText, (WindowWidth - MeasureText(subText, 20)) / 2, barY - 32, 20, GREEN);
            DrawRectangleLines(barX - 4, barY - 4, barWidth + 8, barHeight + 8, DARKBLUE);
            DrawRectangle(barX, barY, (int)(barWidth * progress), barHeight, LIME);
            DrawText(TextFormat("%d%%", (int)(progress * 100)), barX + barWidth / 2 - 20, barY + 5, 20, BLACK);

            char hintText[] = "System is preparing pure C runtime environment...";
            DrawText(hintText, (WindowWidth - MeasureText(hintText, 16)) / 2, barY + 36, 16, LIGHTGRAY);

            if (LoadingTimer >= 5.0f) //5 seconds of loading...
            {
                CurrentState = STATE_CINEMATIC;
                cinematicTimer = 0.0f;
                PlayMusicStream(bgmStory);
            }
        }
        // STATE-CINEMATIC;taken ideas fully from AirStrike game ;)
        else if (CurrentState == STATE_CINEMATIC)
        {
            UpdateMusicStream(bgmStory);
            cinematicTimer += Time;

            if (cinematicTimer < 6.8f)
            {
                Vector2 center = { WindowWidth / 2.0f, WindowHeight / 2.0f + 20 };
                for (int r = 80; r <= 380; r += 75) DrawCircleLines((int)center.x, (int)center.y, (float)r, DARKGREEN);
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
                    float angle = b * 0.72f + 0.35f, dist = 110.0f + (b * 9.0f);
                    float bx = center.x + cosf(angle) * dist, by = center.y + sinf(angle) * dist;
                    DrawCircle((int)bx, (int)by, 4.5f, RED);
                    DrawCircleLines((int)bx, (int)by, 8.0f + sinf(cinematicTimer * 8.0f + b) * 3.0f, Fade(RED, 0.7f));
                }

                if (((int)(cinematicTimer * 4)) % 2 == 0)
                    DrawRectangleLinesEx((Rectangle){ 18, 18, WindowWidth - 36, WindowHeight - 36 }, 4, RED);

                const char* rTitle = "[ PROJECT EARTH SHIELD - DEEP SPACE RADAR ]";
                DrawText(rTitle, (WindowWidth - MeasureText(rTitle, 30)) / 2, 45, 30, GREEN);
                const char* rAlert = "!! PRIORITY RED: MULTIPLE EXOSPHERIC INVASION SIGNATURES DETECTED !!";
                DrawText(rAlert, (WindowWidth - MeasureText(rAlert, 20)) / 2, 90, 20, RED);
                DrawText("ORBITAL SECTOR: 04-BARRICADE CRITICAL", 60, WindowHeight - 90, 18, YELLOW);
                DrawText(TextFormat("HOSTILE SIGNATURE COUNT: %d (MULTIPLYING EXPONENTIALLY)", blipsToShow * 24), 60, WindowHeight - 65, 18, ORANGE);
            }
            else if (cinematicTimer < 13.5f)
            {
                if (!magRailSoundPlayed)
                {
                    PlaySound(sndMagRailCharge);
                    magRailSoundPlayed = true;
                }

                float strobe = fabsf(sinf(cinematicTimer * 7.0f));
                DrawRectangle(0, 0, WindowWidth, WindowHeight, Fade((Color){ 255, 150, 0, 255 }, 0.08f * strobe));

                DrawLineEx((Vector2){ WindowWidth * 0.32f, WindowHeight }, (Vector2){ WindowWidth * 0.37f, 180 }, 8.0f, DARKGRAY);
                DrawLineEx((Vector2){ WindowWidth * 0.38f, WindowHeight }, (Vector2){ WindowWidth * 0.43f, 180 }, 8.0f, DARKGRAY);
                DrawLineEx((Vector2){ WindowWidth * 0.62f, WindowHeight }, (Vector2){ WindowWidth * 0.57f, 180 }, 8.0f, DARKGRAY);
                DrawLineEx((Vector2){ WindowWidth * 0.68f, WindowHeight }, (Vector2){ WindowWidth * 0.63f, 180 }, 8.0f, DARKGRAY);

                float rise = (cinematicTimer - 6.8f) / 6.7f;
                float liftY = WindowHeight - 160.0f - (rise * 220.0f);

                DrawTexturePro(HeroTexture, (Rectangle){ 0, 0, (float)HeroTexture.width, (float)HeroTexture.height },
                               (Rectangle){ WindowWidth * 0.37f, liftY, HeroWidth, HeroHeight }, (Vector2){ HeroWidth/2.0f, HeroHeight/2.0f }, -6.0f, (Color){ 30, 45, 60, 255 });
                DrawTexturePro(StealthHeroTexture, (Rectangle){ 0, 0, (float)StealthHeroTexture.width, (float)StealthHeroTexture.height },
                               (Rectangle){ WindowWidth * 0.63f, liftY, HeroWidth, HeroHeight }, (Vector2){ HeroWidth/2.0f, HeroHeight/2.0f }, 6.0f, (Color){ 30, 45, 60, 255 });

                DrawCircle((int)(WindowWidth * 0.20f), 220, 16.0f, Fade(ORANGE, strobe));
                DrawCircle((int)(WindowWidth * 0.80f), 220, 16.0f, Fade(ORANGE, strobe));

                const char* gzTitle = "[ GROUND ZERO : SUB-ORBITAL HANGAR BAY 09 ]";
                DrawText(gzTitle, (WindowWidth - MeasureText(gzTitle, 32)) / 2, 55, 32, GOLD);
                const char* gzSub = "EMERGENCY ALERT: DUAL INTERCEPTORS ELEVATING TO MAGNETIC LAUNCH RAILS";
                DrawText(gzSub, (WindowWidth - MeasureText(gzSub, 20)) / 2, 100, 20, RAYWHITE);
                DrawText("MAG-RAIL INDUCTION: 98% CHARGED", (WindowWidth - MeasureText("MAG-RAIL INDUCTION: 98% CHARGED", 18)) / 2, WindowHeight - 85, 18, LIME);
            }
            else
            {
                if (IsSoundPlaying(sndMagRailCharge)) StopSound(sndMagRailCharge); //magnetic charging of planes(show-off ;)

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
                DrawTexturePro(StealthHeroTexture, (Rectangle){ 0, 0, (float)StealthHeroTexture.width, (float)StealthHeroTexture.height },
                               (Rectangle){ shipCenter.x, shipCenter.y, HeroWidth, HeroHeight }, (Vector2){ HeroWidth/2.0f, HeroHeight/2.0f }, 0.0f, WHITE);
            }

            const char* skipText = "PRESS [ENTER] OR [SPACE] TO ADVANCE TO TRANSMISSIONS";
            if (((int)(GetTime() * 3)) % 2 == 0)
                DrawText(skipText, (WindowWidth - MeasureText(skipText, 17)) / 2, WindowHeight - 45, 17, GREEN);

            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) || cinematicTimer >= 20.0f)
            {
                if (IsSoundPlaying(sndMagRailCharge)) StopSound(sndMagRailCharge);
                PlaySound(menuSelect);
                CurrentState = STATE_STORY;
                storyDialogueIndex = 0;
            }
        }
        // STATE-STORY DIALOGUES (script fully written by Nayemul......)
        else if (CurrentState == STATE_STORY)
        {
            UpdateMusicStream(bgmStory);
            int panelW = 1260, panelH = 540;
            int panelX = (WindowWidth - panelW) / 2, panelY = (WindowHeight - panelH) / 2;

            if (storyDialogueIndex == 0)
            {
                DrawBreakingNewsBox((Rectangle){ (float)panelX, (float)panelY, (float)panelW, (float)panelH },
                                    "EMERGENCY BROADCAST: SECTOR 4 ORBITAL DEFENSE OVERRUN",
                                    "UPLINK: DEFENSE SATELLITE 04 [CRITICAL]", false);

                int textY = panelY + 130;
                DrawText("EMERGENCY COMMUNICATIONS NETWORK ALERT:", panelX + 50, textY, 22, ORANGE);
                DrawText("\"Mayday! Mayday! Sector 4 orbital defense has collapsed completely!", panelX + 50, textY + 45, 23, RAYWHITE);
                DrawText("Alien swarms have breached low Earth orbit and surrounding airspace.", panelX + 50, textY + 85, 21, LIGHTGRAY);
                DrawText("All primary ground defense platforms are offline! Interceptors, scramble!\"", panelX + 50, textY + 125, 22, RED);
            }
            else if (storyDialogueIndex == 4)
            {
                DrawAlienGlitchBox((Rectangle){ (float)panelX, (float)panelY, (float)panelW, (float)panelH });

                int textY = panelY + 115;
                DrawText("Alien Dreadnought Overlord (Encrypted Signal Intrusion):", panelX + 60, textY, 26, MAROON);
                DrawText("\"Insignificant insects! Look up at your burning skies!\"", panelX + 60, textY + 55, 24, RED);
                DrawText("\"Your planetary shield is shattered. Earth will fall before sunrise!\"", panelX + 60, textY + 105, 23, RED);
                DrawText("\"You fly straight into your own doom, heroes of Earth!\"", panelX + 60, textY + 155, 24, ORANGE);
            }
            else
            {
                DrawCyberBox((Rectangle){ (float)panelX, (float)panelY, (float)panelW, (float)panelH },
                             SKYBLUE, Fade((Color){ 10, 15, 30, 255 }, 0.94f),
                             "SECURE TACTICAL INTERCEPTOR FREQUENCY // 142.80 MHz", GOLD);

                int portW = 145, portH = 175, portY = panelY + 115;
                int p1FrameX = panelX + 45, p2FrameX = panelX + panelW - portW - 45;
                bool shoabSpeaking = (storyDialogueIndex == 1 || storyDialogueIndex == 3);
                bool nayemulSpeaking = (storyDialogueIndex == 2);

                DrawRectangle(p1FrameX - 4, portY - 4, portW + 8, portH + 8, Fade(BLACK, 0.85f));
                if (portraitShoab.id > 0)
                    DrawTexturePro(portraitShoab, (Rectangle){ 0, 0, (float)portraitShoab.width, (float)portraitShoab.height },
                                   (Rectangle){ p1FrameX, portY, (float)portW, (float)portH }, (Vector2){0,0}, 0.0f, shoabSpeaking ? WHITE : Fade(GRAY, 0.40f));
                else
                    DrawRectangle(p1FrameX, portY, portW, portH, Fade(DARKGREEN, 0.35f));
                DrawRectangleLinesEx((Rectangle){ p1FrameX, portY, (float)portW, (float)portH }, shoabSpeaking ? 3.5f : 1.5f, shoabSpeaking ? LIME : DARKGRAY);
                DrawText("SHOAB", p1FrameX + (portW - MeasureText("SHOAB", 18)) / 2, portY + portH + 10, 18, shoabSpeaking ? LIME : GRAY);

                DrawRectangle(p2FrameX - 4, portY - 4, portW + 8, portH + 8, Fade(BLACK, 0.85f));
                if (portraitNayemul.id > 0)
                    DrawTexturePro(portraitNayemul, (Rectangle){ 0, 0, (float)portraitNayemul.width, (float)portraitNayemul.height },
                                   (Rectangle){ p2FrameX, portY, (float)portW, (float)portH }, (Vector2){0,0}, 0.0f, nayemulSpeaking ? WHITE : Fade(GRAY, 0.40f));
                else
                    DrawRectangle(p2FrameX, portY, portW, portH, Fade(DARKBROWN, 0.35f));
                DrawRectangleLinesEx((Rectangle){ p2FrameX, portY, (float)portW, (float)portH }, nayemulSpeaking ? 3.5f : 1.5f, nayemulSpeaking ? YELLOW : DARKGRAY);
                DrawText("NAYEMUL", p2FrameX + (portW - MeasureText("NAYEMUL", 18)) / 2, portY + portH + 10, 18, nayemulSpeaking ? YELLOW : GRAY);

                int speechW = panelW - (portW * 2) - 150;
                int speechX = panelX + portW + 75;
                int speechY = panelY + 115;
                DrawRectangle(speechX, speechY, speechW, 280, Fade(BLACK, 0.65f));
                DrawRectangleLines(speechX, speechY, speechW, 280, Fade(SKYBLUE, 0.5f));

                int dTextX = speechX + 24;
                if (storyDialogueIndex == 1)
                {
                    DrawText("[ TACTICAL RADAR COMMS ]", dTextX, speechY + 20, 22, LIME);
                    DrawText("Shoab (Aegis-1):", dTextX, speechY + 60, 24, LIME);
                    DrawText("\"Radar is flooded with hostiles! They didn't come to talk,", dTextX, speechY + 105, 22, RAYWHITE);
                    DrawText("Nayemul... they are surrounding the whole planet!\"", dTextX, speechY + 140, 22, YELLOW);
                }
                else if (storyDialogueIndex == 2)
                {
                    DrawText("[ STEALTH INTERCEPTOR COCKPIT ]", dTextX, speechY + 20, 22, YELLOW);
                    DrawText("Nayemul (Shadow-2):", dTextX, speechY + 60, 24, YELLOW);
                    DrawText("\"Let them come! My stealth wings are locked, and plasma", dTextX, speechY + 105, 22, RAYWHITE);
                    DrawText("cannons are charged. Earth will not fall today!\"", dTextX, speechY + 140, 22, SKYBLUE);
                }
                else if (storyDialogueIndex == 3)
                {
                    DrawText("[ LAUNCH RAIL CONTROL ]", dTextX, speechY + 20, 22, LIME);
                    DrawText("Shoab:", dTextX, speechY + 60, 24, LIME);
                    DrawText("\"Launch rails release in 3... 2... 1...", dTextX, speechY + 105, 23, LIME);
                    DrawText("Break their lines and protect our home!\"", dTextX, speechY + 145, 23, RAYWHITE);
                }
            }

            char nextPrompt[] = "PRESS [ENTER] OR [SPACE] TO CONTINUE";
            if (((int)(GetTime() * 3)) % 2 == 0)
                DrawText(nextPrompt, (WindowWidth - MeasureText(nextPrompt, 20)) / 2, panelY + panelH - 52, 20, GREEN);

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
        // STATE-MAIN MENU (according to instructions of our Sir...)
        else if (CurrentState == STATE_MENU)
        {
            UpdateMusicStream(bgmMenu);
            int panelW = 1260, panelH = 760;
            int panelX = (WindowWidth - panelW) / 2, panelY = (WindowHeight - panelH) / 2;

            DrawCyberBox((Rectangle){ (float)panelX, (float)panelY, (float)panelW, (float)panelH },
                         SKYBLUE, Fade((Color){ 12, 16, 32, 255 }, 0.90f),
                         "EARTH ALLIANCE TACTICAL DEFENSE BUS", GOLD);

            char menuTitle[] = "SPACE INVADERS";
            DrawText(menuTitle, (WindowWidth - MeasureText(menuTitle, 46)) / 2, panelY + 36, 46, GOLD);
            char menuSub[] = "SAVE THE EARTH....";
            DrawText(menuSub, (WindowWidth - MeasureText(menuSub, 19)) / 2, panelY + 86, 19, RAYWHITE);
            DrawLine(panelX + 100, panelY + 118, panelX + panelW - 100, panelY + 118, SKYBLUE);

            if (IsKeyPressed(KEY_UP)) { PlaySound(menuMove); MenuSelection--; if (MenuSelection < 0) MenuSelection = 7; }
            if (IsKeyPressed(KEY_DOWN)) { PlaySound(menuMove); MenuSelection++; if (MenuSelection > 7) MenuSelection = 0; }

            char itemNames[8][32] = {
                "PLAY",
                "HOW TO PLAY?",
                "SCORES & STATS",
                "HERO CAREER DOSSIERS",
                "PREVIOUS GAMEPLAYS",
                "OPTIONS",
                "CREDENTIALS",
                "EXIT"
            };

            for (int i = 0; i < 8; i++)
            {
                int btnW = 560, btnH = 48, btnX = (WindowWidth - btnW) / 2, btnY = panelY + 130 + (i * 64);
                if (MenuSelection == i)
                {
                    DrawRectangle(btnX, btnY, btnW, btnH, Fade(SKYBLUE, 0.25f));
                    DrawRectangleLines(btnX, btnY, btnW, btnH, LIME);
                    DrawRectangle(btnX - 18, btnY + 10, 8, 28, YELLOW);
                    DrawRectangle(btnX + btnW + 10, btnY + 10, 8, 28, YELLOW);
                    DrawText(itemNames[i], (WindowWidth - MeasureText(itemNames[i], 22)) / 2, btnY + 13, 22, YELLOW);
                }
                else
                {
                    DrawRectangle(btnX, btnY, btnW, btnH, Fade(BLACK, 0.70f));
                    DrawRectangleLines(btnX, btnY, btnW, btnH, DARKGRAY);
                    DrawText(itemNames[i], (WindowWidth - MeasureText(itemNames[i], 20)) / 2, btnY + 14, 20, LIGHTGRAY);
                }
            }

            char footerText[] = "USE [UP / DOWN] TO NAVIGATE   |   [ENTER / SPACE] TO EXECUTE";
            DrawText(footerText, (WindowWidth - MeasureText(footerText, 18)) / 2, panelY + panelH - 35, 18, GREEN);

            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE))
            {
                PlaySound(menuSelect);
                if (MenuSelection == 0)
                {
                    CurrentState = STATE_NAME_ENTRY;
                    nameEntryStep = 0;
                }
                else if (MenuSelection == 1) { CurrentState = STATE_HOWTOPLAY; howToPlayTab = 0; }
                else if (MenuSelection == 2) { CurrentState = STATE_SCORES; }
                else if (MenuSelection == 3) {
                    CurrentState = STATE_CAREER;
                    LoadHeroProfiles(careerProfiles);
                }
                else if (MenuSelection == 4) {
                    CurrentState = STATE_HISTORY;
                    historyScroll = 0;
                    historyCount = LoadMatchRecords(historyList, MAX_MATCH_RECORDS);
                }
                else if (MenuSelection == 5) { CurrentState = STATE_OPTIONS; }
                else if (MenuSelection == 6) { CurrentState = STATE_CREDITS; creditsTab = 0; }
                else if (MenuSelection == 7) break;
            }
        }
        // STATE: PILOT NAME ENTRY (Pre-Flight Registration)
        else if (CurrentState == STATE_NAME_ENTRY)
        {
            UpdateMusicStream(bgmMenu);
            int panelW = 1200, panelH = 620;
            int panelX = (WindowWidth - panelW) / 2, panelY = (WindowHeight - panelH) / 2;

            DrawCyberBox((Rectangle){ (float)panelX, (float)panelY, (float)panelW, (float)panelH },
                         SKYBLUE, Fade((Color){ 10, 16, 32, 255 }, 0.94f),
                         "DEFENSE COMMAND // PILOT IDENTITY REGISTRATION", GOLD);

            char regTitle[] = "PILOT FLIGHT ROSTER REGISTRATION";
            DrawText(regTitle, (WindowWidth - MeasureText(regTitle, 36)) / 2, panelY + 40, 36, GOLD);
            char regSub[] = "ENTER CUSTOM CALLSIGNS FOR INTERCEPTOR CREW BEFORE LAUNCH";
            DrawText(regSub, (WindowWidth - MeasureText(regSub, 18)) / 2, panelY + 85, 18, RAYWHITE);
            DrawLine(panelX + 80, panelY + 115, panelX + panelW - 80, panelY + 115, SKYBLUE);

            // Input handling for names (M and m are safely typed without triggering menu return)
            int key = GetCharPressed();
            while (key > 0)
            {
                if ((key >= 32) && (key <= 125))
                {
                    if (nameEntryStep == 0 && letterCount1 < 18) {
                        inputHero1Name[letterCount1] = (char)key;
                        inputHero1Name[letterCount1 + 1] = '\0';
                        letterCount1++;
                    }
                    else if (nameEntryStep == 1 && letterCount2 < 18) {
                        inputHero2Name[letterCount2] = (char)key;
                        inputHero2Name[letterCount2 + 1] = '\0';
                        letterCount2++;
                    }
                }
                key = GetCharPressed();
            }

            if (IsKeyPressed(KEY_BACKSPACE))
            {
                if (nameEntryStep == 0 && letterCount1 > 0) {
                    letterCount1--;
                    inputHero1Name[letterCount1] = '\0';
                }
                else if (nameEntryStep == 1 && letterCount2 > 0) {
                    letterCount2--;
                    inputHero2Name[letterCount2] = '\0';
                }
            }

            // Cards for Ship 1 and Ship 2
            int boxW = 500, boxH = 260, boxY = panelY + 150;
            int box1X = panelX + 70;
            int box2X = panelX + panelW - boxW - 70;

            // Box 1: Shoab's Ship
            bool active1 = (nameEntryStep == 0);
            DrawRectangle(box1X, boxY, boxW, boxH, Fade(BLACK, 0.75f));
            DrawRectangleLinesEx((Rectangle){ (float)box1X, (float)boxY, (float)boxW, (float)boxH }, active1 ? 3.0f : 1.5f, active1 ? LIME : DARKGRAY);
            DrawText("SPACESHIP 1: SHOAB'S INTERCEPTOR", box1X + 25, boxY + 22, 20, LIME);
            DrawText("Class: Aegis-1 Plasma Heavy Fighter", box1X + 25, boxY + 50, 15, LIGHTGRAY);

            int inputBarW = boxW - 50, inputBarH = 55;
            int ib1X = box1X + 25, ib1Y = boxY + 95;
            DrawRectangle(ib1X, ib1Y, inputBarW, inputBarH, Fade(DARKBLUE, 0.40f));
            DrawRectangleLines(ib1X, ib1Y, inputBarW, inputBarH, active1 ? YELLOW : GRAY);

            // Render beautifully with drop-shadow
            DrawText(inputHero1Name, ib1X + 18, ib1Y + 16, 26, Fade(BLACK, 0.85f));
            DrawText(inputHero1Name, ib1X + 16, ib1Y + 14, 26, active1 ? LIME : RAYWHITE);

            if (active1 && ((int)(GetTime() * 2.5f) % 2 == 0))
            {
                int cursorX = ib1X + 18 + MeasureText(inputHero1Name, 26);
                DrawRectangle(cursorX, ib1Y + 12, 3, 30, YELLOW);
            }
            DrawText("Status: [ READY FOR PILOT NAME ]", box1X + 25, boxY + 180, 16, active1 ? YELLOW : GRAY);

            // Box 2: Nayemul's Ship
            bool active2 = (nameEntryStep == 1);
            DrawRectangle(box2X, boxY, boxW, boxH, Fade(BLACK, 0.75f));
            DrawRectangleLinesEx((Rectangle){ (float)box2X, (float)boxY, (float)boxW, (float)boxH }, active2 ? 3.0f : 1.5f, active2 ? YELLOW : DARKGRAY);
            DrawText("SPACESHIP 2: NAYEMUL'S INTERCEPTOR", box2X + 25, boxY + 22, 20, YELLOW);
            DrawText("Class: Shadow-2 Stealth Interceptor", box2X + 25, boxY + 50, 15, LIGHTGRAY);

            int ib2X = box2X + 25, ib2Y = boxY + 95;
            DrawRectangle(ib2X, ib2Y, inputBarW, inputBarH, Fade(DARKBLUE, 0.40f));
            DrawRectangleLines(ib2X, ib2Y, inputBarW, inputBarH, active2 ? YELLOW : GRAY);

            DrawText(inputHero2Name, ib2X + 18, ib2Y + 16, 26, Fade(BLACK, 0.85f));
            DrawText(inputHero2Name, ib2X + 16, ib2Y + 14, 26, active2 ? YELLOW : RAYWHITE);

            if (active2 && ((int)(GetTime() * 2.5f) % 2 == 0))
            {
                int cursorX = ib2X + 18 + MeasureText(inputHero2Name, 26);
                DrawRectangle(cursorX, ib2Y + 12, 3, 30, YELLOW);
            }
            DrawText("Status: [ PENDING REGISTRATION ]", box2X + 25, boxY + 180, 16, active2 ? YELLOW : GRAY);

            const char* promptEntry = (nameEntryStep == 0) ?
                "TYPE NAME FOR SHOAB'S SPACESHIP AND PRESS [ENTER]" :
                "TYPE NAME FOR NAYEMUL'S SPACESHIP AND PRESS [ENTER] TO LAUNCH";
            DrawText(promptEntry, (WindowWidth - MeasureText(promptEntry, 20)) / 2, panelY + panelH - 85, 20, GOLD);

            char entryFooter[] = "PRESS [ENTER] TO CONFIRM   |   [BACKSPACE] TO DELETE   |   [ESC] RETURN TO MENU";
            DrawText(entryFooter, (WindowWidth - MeasureText(entryFooter, 17)) / 2, panelY + panelH - 40, 17, GREEN);

            if (IsKeyPressed(KEY_ENTER))
            {
                PlaySound(menuSelect);
                if (nameEntryStep == 0)
                {
                    if (strlen(inputHero1Name) == 0) { strcpy(inputHero1Name, "Shoab"); letterCount1 = 5; }
                    nameEntryStep = 1;
                }
                else
                {
                    if (strlen(inputHero2Name) == 0) { strcpy(inputHero2Name, "Nayemul"); letterCount2 = 7; }
                    StopMusicStream(bgmMenu);
                    CurrentState = STATE_LAUNCH;
                    launchDialogueIndex = 0; launchCountdownTimer = 3.0f; launchCountdownActive = false;
                    launchShip1Pos = (Vector2){ WindowWidth * 0.40f, WindowHeight - 240 };
                    launchShip2Pos = (Vector2){ WindowWidth * 0.60f, WindowHeight - 240 };
                    PlayMusicStream(bgmStory);
                }
            }

            // Exiting back to menu is handled strictly by ESC so typing M/m never kicks out
            if (IsKeyPressed(KEY_ESCAPE))
            {
                PlaySound(menuSelect);
                CurrentState = STATE_MENU;
            }
        }
        // STATE: HERO CAREER DOSSIERS (Permanent Lifetime Career Records)
        else if (CurrentState == STATE_CAREER)
        {
            UpdateMusicStream(bgmMenu);
            int panelW = 1320, panelH = 760;
            int panelX = (WindowWidth - panelW) / 2, panelY = (WindowHeight - panelH) / 2;

            DrawCyberBox((Rectangle){ (float)panelX, (float)panelY, (float)panelW, (float)panelH },
                         SKYBLUE, Fade((Color){ 10, 15, 30, 255 }, 0.94f),
                         "HERO CAREER DOSSIERS // LIFETIME FLIGHT LOGS", GOLD);

            char carTitle[] = "PERMANENT HERO CAREER RECORDS";
            DrawText(carTitle, (WindowWidth - MeasureText(carTitle, 36)) / 2, panelY + 36, 36, GOLD);
            char carSub[] = "CUMULATIVE COMBAT TELEMETRY ACROSS ALL RECORDED SESSIONS";
            DrawText(carSub, (WindowWidth - MeasureText(carSub, 17)) / 2, panelY + 80, 17, RAYWHITE);
            DrawLine(panelX + 60, panelY + 110, panelX + panelW - 60, panelY + 110, SKYBLUE);

            int cardW = 560, cardH = 540, cardY = panelY + 130;
            int card1X = panelX + 60;
            int card2X = panelX + panelW - cardW - 60;

            // Profile 1: Shoab (Aegis-1)
            DrawRectangle(card1X, cardY, cardW, cardH, Fade(BLACK, 0.75f));
            DrawRectangleLines(card1X, cardY, cardW, cardH, LIME);
            DrawText("SPACESHIP: SHOAB'S INTERCEPTOR", card1X + 30, cardY + 24, 22, LIME);
            DrawText("Callsign: AEGIS-1  |  Class: Interceptor Alpha", card1X + 30, cardY + 54, 15, LIGHTGRAY);
            DrawLine(card1X + 25, cardY + 80, card1X + cardW - 25, cardY + 80, DARKGRAY);

            int statY = cardY + 105;
            int col1 = card1X + 35, colVal1 = card1X + cardW - 60;
            DrawText("TOTAL MISSIONS SORTIED:", col1, statY, 18, RAYWHITE);
            DrawText(TextFormat("%d MISSIONS", careerProfiles[0].totalMissions), colVal1 - MeasureText(TextFormat("%d MISSIONS", careerProfiles[0].totalMissions), 18), statY, 18, WHITE);

            statY += 40;
            DrawText("PLANETARY VICTORIES:", col1, statY, 18, RAYWHITE);
            DrawText(TextFormat("%d VICTORIES", careerProfiles[0].totalVictories), colVal1 - MeasureText(TextFormat("%d VICTORIES", careerProfiles[0].totalVictories), 18), statY, 18, GREEN);

            statY += 40;
            DrawText("CASUALTIES / DEFEATS:", col1, statY, 18, RAYWHITE);
            DrawText(TextFormat("%d DEFEATS", careerProfiles[0].totalDefeats), colVal1 - MeasureText(TextFormat("%d DEFEATS", careerProfiles[0].totalDefeats), 18), statY, 18, (careerProfiles[0].totalDefeats > 0) ? RED : GRAY);

            statY += 40;
            DrawText("LIFETIME COMBAT SCORE:", col1, statY, 18, RAYWHITE);
            DrawText(TextFormat("%07ld PTS", careerProfiles[0].totalScore), colVal1 - MeasureText(TextFormat("%07ld PTS", careerProfiles[0].totalScore), 18), statY, 18, LIME);

            statY += 40;
            DrawText("LIFETIME ALIEN KILLS:", col1, statY, 18, RAYWHITE);
            DrawText(TextFormat("%d KILLS", careerProfiles[0].totalAlienKills), colVal1 - MeasureText(TextFormat("%d KILLS", careerProfiles[0].totalAlienKills), 18), statY, 18, YELLOW);

            statY += 40;
            DrawText("MINIONS ELIMINATED:", col1, statY, 18, RAYWHITE);
            DrawText(TextFormat("%d MINIONS", careerProfiles[0].totalMinionKills), colVal1 - MeasureText(TextFormat("%d MINIONS", careerProfiles[0].totalMinionKills), 18), statY, 18, WHITE);

            statY += 40;
            DrawText("DREADNOUGHT DAMAGE DEALT:", col1, statY, 18, RAYWHITE);
            DrawText(TextFormat("%d HP", careerProfiles[0].totalBossDamage), colVal1 - MeasureText(TextFormat("%d HP", careerProfiles[0].totalBossDamage), 18), statY, 18, ORANGE);

            statY += 40;
            DrawText("TOTAL DAMAGE SUSTAINED:", col1, statY, 18, RAYWHITE);
            DrawText(TextFormat("%d HITS", careerProfiles[0].totalHitsTaken), colVal1 - MeasureText(TextFormat("%d HITS", careerProfiles[0].totalHitsTaken), 18), statY, 18, SKYBLUE);

            // Profile 2: Nayemul (Shadow-2)
            DrawRectangle(card2X, cardY, cardW, cardH, Fade(BLACK, 0.75f));
            DrawRectangleLines(card2X, cardY, cardW, cardH, YELLOW);
            DrawText("SPACESHIP: NAYEMUL'S INTERCEPTOR", card2X + 30, cardY + 24, 22, YELLOW);
            DrawText("Callsign: SHADOW-2  |  Class: Stealth Fighter Beta", card2X + 30, cardY + 54, 15, LIGHTGRAY);
            DrawLine(card2X + 25, cardY + 80, card2X + cardW - 25, cardY + 80, DARKGRAY);

            statY = cardY + 105;
            int col2 = card2X + 35, colVal2 = card2X + cardW - 60;
            DrawText("TOTAL MISSIONS SORTIED:", col2, statY, 18, RAYWHITE);
            DrawText(TextFormat("%d MISSIONS", careerProfiles[1].totalMissions), colVal2 - MeasureText(TextFormat("%d MISSIONS", careerProfiles[1].totalMissions), 18), statY, 18, WHITE);

            statY += 40;
            DrawText("PLANETARY VICTORIES:", col2, statY, 18, RAYWHITE);
            DrawText(TextFormat("%d VICTORIES", careerProfiles[1].totalVictories), colVal2 - MeasureText(TextFormat("%d VICTORIES", careerProfiles[1].totalVictories), 18), statY, 18, GREEN);

            statY += 40;
            DrawText("CASUALTIES / DEFEATS:", col2, statY, 18, RAYWHITE);
            DrawText(TextFormat("%d DEFEATS", careerProfiles[1].totalDefeats), colVal2 - MeasureText(TextFormat("%d DEFEATS", careerProfiles[1].totalDefeats), 18), statY, 18, (careerProfiles[1].totalDefeats > 0) ? RED : GRAY);

            statY += 40;
            DrawText("LIFETIME COMBAT SCORE:", col2, statY, 18, RAYWHITE);
            DrawText(TextFormat("%07ld PTS", careerProfiles[1].totalScore), colVal2 - MeasureText(TextFormat("%07ld PTS", careerProfiles[1].totalScore), 18), statY, 18, YELLOW);

            statY += 40;
            DrawText("LIFETIME ALIEN KILLS:", col2, statY, 18, RAYWHITE);
            DrawText(TextFormat("%d KILLS", careerProfiles[1].totalAlienKills), colVal2 - MeasureText(TextFormat("%d KILLS", careerProfiles[1].totalAlienKills), 18), statY, 18, LIME);

            statY += 40;
            DrawText("MINIONS ELIMINATED:", col2, statY, 18, RAYWHITE);
            DrawText(TextFormat("%d MINIONS", careerProfiles[1].totalMinionKills), colVal2 - MeasureText(TextFormat("%d MINIONS", careerProfiles[1].totalMinionKills), 18), statY, 18, WHITE);

            statY += 40;
            DrawText("DREADNOUGHT DAMAGE DEALT:", col2, statY, 18, RAYWHITE);
            DrawText(TextFormat("%d HP", careerProfiles[1].totalBossDamage), colVal2 - MeasureText(TextFormat("%d HP", careerProfiles[1].totalBossDamage), 18), statY, 18, ORANGE);

            statY += 40;
            DrawText("TOTAL DAMAGE SUSTAINED:", col2, statY, 18, RAYWHITE);
            DrawText(TextFormat("%d HITS", careerProfiles[1].totalHitsTaken), colVal2 - MeasureText(TextFormat("%d HITS", careerProfiles[1].totalHitsTaken), 18), statY, 18, SKYBLUE);

            char carFooter[] = "ALL STATS PERMANENTLY STORED TO DISK   |   [BACKSPACE / M] RETURN TO MENU";
            DrawText(carFooter, (WindowWidth - MeasureText(carFooter, 18)) / 2, panelY + panelH - 35, 18, GREEN);

            if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressed(KEY_M))
            {
                PlaySound(menuSelect);
                CurrentState = STATE_MENU;
            }
        }
        // STATE: HOW TO PLAY??? (provided all the rules and technique of the game...)
        else if (CurrentState == STATE_HOWTOPLAY)
        {
            UpdateMusicStream(bgmMenu);
            int panelW = 1320, panelH = 760;
            int panelX = (WindowWidth - panelW) / 2, panelY = (WindowHeight - panelH) / 2;

            DrawCyberBox((Rectangle){ (float)panelX, (float)panelY, (float)panelW, (float)panelH },
                         SKYBLUE, Fade((Color){ 10, 15, 30, 255 }, 0.92f),
                         "OPERATION MANUAL : FLIGHT & COMBAT TACTICS", GOLD);

            if (IsKeyPressed(KEY_LEFT))  { PlaySound(menuMove); howToPlayTab--; if (howToPlayTab < 0) howToPlayTab = 3; }
            if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_TAB)) { PlaySound(menuMove); howToPlayTab++; if (howToPlayTab > 3) howToPlayTab = 0; }

            const char* tabNames[4] = { "1. BASIC CONTROLS", "2. ENEMY & HAZARD INTEL", "3. ARSENAL & SKILLS", "4. BOSS RAID TACTICS" };
            int tabW = 280, tabH = 42, tabGap = 15;
            int startTabX = (WindowWidth - (4 * tabW + 3 * tabGap)) / 2;
            int tabY = panelY + 85;

            for (int t = 0; t < 4; t++)
            {
                int curTabX = startTabX + t * (tabW + tabGap);
                bool isCur = (howToPlayTab == t);
                DrawRectangle(curTabX, tabY, tabW, tabH, isCur ? Fade(SKYBLUE, 0.35f) : Fade(BLACK, 0.65f));
                DrawRectangleLines(curTabX, tabY, tabW, tabH, isCur ? YELLOW : DARKGRAY);
                DrawText(tabNames[t], curTabX + (tabW - MeasureText(tabNames[t], 17)) / 2, tabY + 12, 17, isCur ? YELLOW : LIGHTGRAY);
            }
            DrawLine(panelX + 40, tabY + 58, panelX + panelW - 40, tabY + 58, SKYBLUE);

            int contentY = tabY + 75;
            int textX = panelX + 70;

            if (howToPlayTab == 0)
            {
                DrawText("PILOT ASSIGNMENTS & FLIGHT ENVELOPE", textX, contentY, 22, YELLOW);
                DrawText("> PILOT 1: SHOAB MAHMUD [AEGIS-1] (Interceptor Alpha)", textX + 20, contentY + 38, 20, LIME);
                DrawText("  - Controls: [A] Move Left  |  [D] Move Right  |  [W] or [SPACE] Fire Twin Plasma Blasters", textX + 40, contentY + 66, 18, RAYWHITE);
                DrawText("  - Special:  [Q] High-Energy Hyper Laser Beam (Triggers Super-Freeze & cuts through all armors).", textX + 40, contentY + 94, 18, SKYBLUE);

                DrawText("> PILOT 2: NAYEMUL ISLAM [SHADOW-2] (Stealth Interceptor Beta)", textX + 20, contentY + 138, 20, YELLOW);
                DrawText("  - Controls: [LEFT ARROW] Move Left  |  [RIGHT ARROW] Move Right  |  [UP ARROW] Fire Blasters", textX + 40, contentY + 166, 18, RAYWHITE);
                DrawText("  - Special:  [RIGHT SHIFT] 8-Way Guided Cluster Missiles (Triggers Super-Freeze & blanket kinetic salvo).", textX + 40, contentY + 194, 18, SKYBLUE);

                DrawText("> SYNCHRONIZED TEAM SHIELD DOME [ENTER]", textX + 20, contentY + 238, 20, GOLD);
                DrawText("  - Fly within 375 pixels of each other to establish a quantum energy tether.", textX + 40, contentY + 266, 18, RAYWHITE);
                DrawText("  - Press [ENTER] to project an indestructible 240px half-sphere dome for 4.0 seconds (25s recharge).", textX + 40, contentY + 294, 18, LIME);
                DrawText("  - Deflects all regular bullets, commander ordnance, and vaporizes any grunt alien touching the dome!", textX + 40, contentY + 322, 18, SKYBLUE);

                DrawText("> SYSTEM UTILITY:  [P] Pause  |  [F / F11] Fullscreen  |  [M] Tactical Main Menu", textX + 20, contentY + 372, 18, LIGHTGRAY);
            }
            else if (howToPlayTab == 1)
            {
                DrawText("HOSTILE FORCES & INTERACTIVE SPACE HAZARDS", textX, contentY, 21, YELLOW);

                // 1. ASTEROIDS
                DrawText("> DRIFTING METALLIC ASTEROIDS (Spawns every 13 seconds):", textX + 20, contentY + 28, 17, (Color){ 200, 220, 255, 255 });
                DrawText("  - Metallic asteroids tumble diagonally across orbit acting as neutral dynamic cover!", textX + 40, contentY + 49, 15, RAYWHITE);
                DrawText("  - Absorbs alien blasters & commander fire. CAUTION: Ramming an asteroid slows your ship by 60% for 3s!", textX + 40, contentY + 69, 15, ORANGE);
                DrawText("  - Shattering an asteroid awards +100 Points and shaves 1.0 second off your Special Cooldown!", textX + 40, contentY + 89, 15, LIME);

                // 2. REGULAR SWARM 
                DrawText("> REGULAR INVASION SWARM:", textX + 20, contentY + 115, 17, RED);
                DrawText("  - Descends from deep space in synchronized formation with variable thruster speeds.", textX + 40, contentY + 136, 15, RAYWHITE);
                DrawText("  - Bounces off screen boundaries and steps 20 pixels lower each bounce. Defend the 70% orbital line!", textX + 40, contentY + 156, 15, LIGHTGRAY);

                // 3. ELITE COMMANDERS 
                DrawText("> ELITE ALIEN COMMANDERS (Spawn at 65% wave eradication):", textX + 20, contentY + 182, 17, MAGENTA);
                DrawText("  1. COMMANDER JAMMER (35 HP): Heavy tracking bolts & EMP space lightning that jams cockpit HUD.", textX + 40, contentY + 203, 15, RAYWHITE);
                DrawText("  2. COMMANDER WARP (30 HP): Dual rapid blasters and automatic reactive emergency micro-teleportation.", textX + 40, contentY + 223, 15, LIGHTGRAY);

                // MICRO-SINGULARITY ANOMALY (BLACK HOLE)
                DrawText("> MICRO-SINGULARITY ANOMALY (BLACK HOLE - Spawns upon Pod Collapse):", textX + 20, contentY + 249, 17, (Color){ 210, 130, 255, 255 });
                DrawText("  - Sinks slowly at 25px/s with a 380px lateral sine drift; pulls ships, bullets, and asteroids.", textX + 40, contentY + 270, 15, RAYWHITE);
                DrawText("  - Accretion Halo (450px drag) | Tidal Shear (180px, -70% speed) | Core (35px lethal event horizon, -1 Life).", textX + 40, contentY + 290, 15, ORANGE);
                DrawText("  - Absorbs boss bullets & orbs. Collapses smoothly at bottom of screen. Team Shield [ENTER] negates pull.", textX + 40, contentY + 310, 15, SKYBLUE);

                // THE VOID PHANTOM spooky ;)
                DrawText("> THE VOID PHANTOM (60 HP Cosmic Anomaly):", textX + 20, contentY + 336, 17, (Color){ 230, 90, 255, 255 });
                DrawText("  - Materializes at the exact collapse point with a haunted infrasonic presence.", textX + 40, contentY + 357, 15, RAYWHITE);
                DrawText("  - Dimensional Phasing: Dissolves and reforms directly above the targeted hero every 6-9 seconds.", textX + 40, contentY + 377, 15, YELLOW);
                DrawText("  - Void Orbs deal 1 DMG & 2.5s weapons jam. Gravitational Scream causes white lightning & jams HUD for 3.5s.", textX + 40, contentY + 397, 15, RED);
                DrawText("  - Eliminate using blasters (2 DMG), Clusters (15 DMG), or Hyper Beam (20 burst DMG) for +800 Points!", textX + 40, contentY + 417, 15, LIME);
            }
            else if (howToPlayTab == 2)
            {
                DrawText("HERO ARSENAL, HIT-STOP & LAST STAND OVERDRIVE", textX, contentY, 22, YELLOW);
                DrawText("> TWO FLAVORS OF FREEZE (HIT-STOP / SUPER-PAUSE):", textX + 20, contentY + 38, 20, GOLD);
                DrawText("  - Activation Freeze: Pressing [Q] or [RSHIFT] freezes time for 0.18s with a blinding chromatic flare!", textX + 40, contentY + 66, 18, RAYWHITE);
                DrawText("  - Impact Hit-Stop: Massive detonations & Hyper Beam impacts briefly lock frames to deliver devastating weight.", textX + 40, contentY + 94, 18, SKYBLUE);

                DrawText("> LAST STAND OVERDRIVE SURGE:", textX + 20, contentY + 138, 20, RED);
                DrawText("  - If your wingman is shot down, the surviving pilot gains 2.5s of Rapid Plasma Overcharge and shield flaring!", textX + 40, contentY + 166, 18, RAYWHITE);
                DrawText("  - Use this adrenaline window to fight through swarms and reach the crash beacon for defibrillator revival!", textX + 40, contentY + 194, 18, LIME);

                DrawText("> COMBAT DEFIBRILLATOR & CRASH REVIVAL:", textX + 20, contentY + 238, 20, GOLD);
                DrawText("  - If your companion is down and you have >1 life, fly onto their beacon and hold position for 1.5-2.0s.", textX + 40, contentY + 266, 18, RAYWHITE);
                DrawText("  - Revives your partner instantly into battle at the cost of 1 shared extra life.", textX + 40, contentY + 294, 18, YELLOW);

                DrawText("> ORBITAL EMP AIRDROP:", textX + 20, contentY + 338, 20, SKYBLUE);
                DrawText("  - Destroying both boss pods drops an EMP core for 8.5s of Rapid Plasma Overcharge (35% boost)!", textX + 40, contentY + 366, 18, LIME);
            }
            else
            {
                DrawText("BOSS RAID GUIDE: THE DREADNOUGHT CARRIER", textX, contentY, 22, YELLOW);
                DrawText("> PHASE 1: BILATERAL DEFLECTOR PODS (75 HP Each)", textX + 20, contentY + 38, 20, ORANGE);
                DrawText("  - The boss core is IMMUNE while deflector pods remain online. Regular bullets deal zero core damage!", textX + 40, contentY + 66, 18, RAYWHITE);
                DrawText("  - Focus fire exclusively on the Left Deflector Pod (Red) and Right Deflector Pod (Magenta).", textX + 40, contentY + 94, 18, YELLOW);

                DrawText("> MINION ESCORTS & HOMING DISRUPTION ORBS:", textX + 20, contentY + 138, 20, MAGENTA);
                DrawText("  - The Dreadnought launches 2-HP minion escorts. Destroy pods quickly to stop reinforcements.", textX + 40, contentY + 166, 18, RAYWHITE);
                DrawText("  - Dodge purple homing orbs! Getting struck will disable and jam your weapons for 4.5 seconds.", textX + 40, contentY + 194, 18, RED);

                DrawText("> MASTERING THE MAIN LASER WARNING POINTER (CRITICAL!):", textX + 20, contentY + 238, 20, RED);
                DrawText("  - A red aiming line and implosion rings telegraph the impending mega-beam for 0.8s.", textX + 40, contentY + 266, 18, YELLOW);
                DrawText("  - The warning pointer deals NO DAMAGE. Use this 0.8s window to steer clear of its firing path!", textX + 40, contentY + 294, 18, LIME);

                DrawText("> PHASE 2: EXPOSED REACTOR CORE & FINAL SURGE (100 HP)", textX + 20, contentY + 338, 20, GOLD);
                DrawText("  - With pods destroyed, the center core opens! Below 40 HP, the Dreadnought enrages with boosted thrusters.", textX + 40, contentY + 366, 18, RAYWHITE);
            }

            char htpFooter[] = "[< LEFT / RIGHT >] SWITCH MANUAL TAB   |   [BACKSPACE / M] RETURN TO MENU";
            DrawText(htpFooter, (WindowWidth - MeasureText(htpFooter, 18)) / 2, panelY + panelH - 40, 18, GREEN);

            if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressed(KEY_M))
            {
                PlaySound(menuSelect);
                CurrentState = STATE_MENU;
            }
        }
        // STATE: SCORES and records (but it's temporary....)
        else if (CurrentState == STATE_SCORES)
        {
            UpdateMusicStream(bgmMenu);
            int panelW = 1260, panelH = 740;
            int panelX = (WindowWidth - panelW) / 2, panelY = (WindowHeight - panelH) / 2;

            DrawCyberBox((Rectangle){ (float)panelX, (float)panelY, (float)panelW, (float)panelH },
                         SKYBLUE, Fade((Color){ 10, 15, 30, 255 }, 0.92f),
                         "TACTICAL COMBAT RECORDS & TELEMETRY", GOLD);

            char scrTitle[] = "TACTICAL COMBAT RECORDS & SCORES";
            DrawText(scrTitle, (WindowWidth - MeasureText(scrTitle, 38)) / 2, panelY + 40, 38, GOLD);
            char scrSub[] = "HEAD-TO-HEAD BATTLE TELEMETRY & PERFORMANCE METRICS";
            DrawText(scrSub, (WindowWidth - MeasureText(scrSub, 19)) / 2, panelY + 90, 19, RAYWHITE);
            DrawLine(panelX + 80, panelY + 120, panelX + panelW - 80, panelY + 120, SKYBLUE);

            int cardW = 540, cardH = 500, cardY = panelY + 145;
            int card1X = panelX + 60;
            int card2X = panelX + panelW - cardW - 60;

            // CARD 1: NAYEMUL's one!!!
            DrawRectangle(card1X, cardY, cardW, cardH, Fade(BLACK, 0.75f));
            DrawRectangleLines(card1X, cardY, cardW, cardH, YELLOW);
            DrawRectangleLines(card1X + 4, cardY + 4, cardW - 8, cardH - 8, Fade(DARKBLUE, 0.7f));

            DrawText(TextFormat("PILOT: %s", inputHero2Name), card1X + (cardW - MeasureText(TextFormat("PILOT: %s", inputHero2Name), 24)) / 2, cardY + 22, 24, YELLOW);
            DrawText("CALLSIGN: SHADOW-2 [STEALTH]", card1X + (cardW - MeasureText("CALLSIGN: SHADOW-2 [STEALTH]", 16)) / 2, cardY + 52, 16, RAYWHITE);

            int pSize = 110, pY = cardY + 80;
            int p1X = card1X + (cardW - pSize) / 2;
            DrawRectangle(p1X - 2, pY - 2, pSize + 4, pSize + 4, Fade(BLACK, 0.8f));
            if (portraitNayemul.id > 0)
                DrawTexturePro(portraitNayemul, (Rectangle){ 0, 0, (float)portraitNayemul.width, (float)portraitNayemul.height },
                               (Rectangle){ (float)p1X, (float)pY, (float)pSize, (float)pSize }, (Vector2){0,0}, 0.0f, WHITE);
            else
                DrawRectangle(p1X, pY, pSize, pSize, Fade(DARKBROWN, 0.5f));
            DrawRectangleLinesEx((Rectangle){ (float)p1X, (float)pY, (float)pSize, (float)pSize }, 2.5f, YELLOW);

            int sY = pY + pSize + 25;
            int col1 = card1X + 50, colVal1 = card1X + cardW - 80;
            DrawText("TOTAL COMBAT SCORE:", col1, sY, 19, RAYWHITE);
            DrawText(TextFormat("%05d", Hero2Score), colVal1 - MeasureText(TextFormat("%05d", Hero2Score), 19), sY, 20, YELLOW);

            sY += 40;
            DrawText("ALIENS ELIMINATED:", col1, sY, 18, LIGHTGRAY);
            DrawText(TextFormat("%d", Hero2Kills), colVal1 - MeasureText(TextFormat("%d", Hero2Kills), 18), sY, 19, WHITE);

            sY += 38;
            DrawText("MINIONS DESTROYED:", col1, sY, 18, LIGHTGRAY);
            DrawText(TextFormat("%d", Hero2MinionKills), colVal1 - MeasureText(TextFormat("%d", Hero2MinionKills), 18), sY, 19, WHITE);

            sY += 38;
            DrawText("BOSS DAMAGE DEALT:", col1, sY, 18, LIGHTGRAY);
            DrawText(TextFormat("%d HP", Hero2BossDamage), colVal1 - MeasureText(TextFormat("%d HP", Hero2BossDamage), 18), sY, 19, RED);

            sY += 38;
            DrawText("DAMAGE HITS TAKEN:", col1, sY, 18, LIGHTGRAY);
            DrawText(TextFormat("%d HITS", Hero2HitsTaken), colVal1 - MeasureText(TextFormat("%d HITS", Hero2HitsTaken), 18), sY, 19, (Hero2HitsTaken <= Hero1HitsTaken) ? GREEN : ORANGE);

            sY += 38;
            DrawText("SURVIVAL STATUS:", col1, sY, 18, LIGHTGRAY);
            DrawText((Hero2Lives > 0) ? TextFormat("ACTIVE (%d LIVES)", Hero2Lives) : "CRASHED / OFFLINE",
                     colVal1 - MeasureText((Hero2Lives > 0) ? TextFormat("ACTIVE (%d LIVES)", Hero2Lives) : "CRASHED / OFFLINE", 17), sY, 17, (Hero2Lives > 0) ? LIME : RED);

            // CARD 2: SHOAB
            DrawRectangle(card2X, cardY, cardW, cardH, Fade(BLACK, 0.75f));
            DrawRectangleLines(card2X, cardY, cardW, cardH, LIME);
            DrawRectangleLines(card2X + 4, cardY + 4, cardW - 8, cardH - 8, Fade(DARKBLUE, 0.7f));

            DrawText(TextFormat("PILOT: %s", inputHero1Name), card2X + (cardW - MeasureText(TextFormat("PILOT: %s", inputHero1Name), 24)) / 2, cardY + 22, 24, LIME);
            DrawText("CALLSIGN: AEGIS-1 [INTERCEPTOR]", card2X + (cardW - MeasureText("CALLSIGN: AEGIS-1 [INTERCEPTOR]", 16)) / 2, cardY + 52, 16, RAYWHITE);

            int p2X = card2X + (cardW - pSize) / 2;
            DrawRectangle(p2X - 2, pY - 2, pSize + 4, pSize + 4, Fade(BLACK, 0.8f));
            if (portraitShoab.id > 0)
                DrawTexturePro(portraitShoab, (Rectangle){ 0, 0, (float)portraitShoab.width, (float)portraitShoab.height },
                               (Rectangle){ (float)p2X, (float)pY, (float)pSize, (float)pSize }, (Vector2){0,0}, 0.0f, WHITE);
            else
                DrawRectangle(p2X, pY, pSize, pSize, Fade(DARKGREEN, 0.5f));
            DrawRectangleLinesEx((Rectangle){ (float)p2X, (float)pY, (float)pSize, (float)pSize }, 2.5f, LIME);

            sY = pY + pSize + 25;
            int col2 = card2X + 50, colVal2 = card2X + cardW - 80;
            DrawText("TOTAL COMBAT SCORE:", col2, sY, 19, RAYWHITE);
            DrawText(TextFormat("%05d", Hero1Score), colVal2 - MeasureText(TextFormat("%05d", Hero1Score), 19), sY, 20, LIME);

            sY += 40;
            DrawText("ALIENS ELIMINATED:", col2, sY, 18, LIGHTGRAY);
            DrawText(TextFormat("%d", Hero1Kills), colVal2 - MeasureText(TextFormat("%d", Hero1Kills), 18), sY, 19, WHITE);

            sY += 38;
            DrawText("MINIONS DESTROYED:", col2, sY, 18, LIGHTGRAY);
            DrawText(TextFormat("%d", Hero1MinionKills), colVal2 - MeasureText(TextFormat("%d", Hero1MinionKills), 18), sY, 19, WHITE);

            sY += 38;
            DrawText("BOSS DAMAGE DEALT:", col2, sY, 18, LIGHTGRAY);
            DrawText(TextFormat("%d HP", Hero1BossDamage), colVal2 - MeasureText(TextFormat("%d HP", Hero1BossDamage), 18), sY, 19, RED);

            sY += 38;
            DrawText("DAMAGE HITS TAKEN:", col2, sY, 18, LIGHTGRAY);
            DrawText(TextFormat("%d HITS", Hero1HitsTaken), colVal2 - MeasureText(TextFormat("%d HITS", Hero1HitsTaken), 18), sY, 19, (Hero1HitsTaken <= Hero2HitsTaken) ? GREEN : ORANGE);

            sY += 38;
            DrawText("SURVIVAL STATUS:", col2, sY, 18, LIGHTGRAY);
            DrawText((Hero1Lives > 0) ? TextFormat("ACTIVE (%d LIVES)", Hero1Lives) : "CRASHED / OFFLINE",
                     colVal2 - MeasureText((Hero1Lives > 0) ? TextFormat("ACTIVE (%d LIVES)", Hero1Lives) : "CRASHED / OFFLINE", 17), sY, 17, (Hero1Lives > 0) ? LIME : RED);

            char scrBack[] = "PRESS [BACKSPACE] OR [M] TO RETURN TO MENU";
            DrawText(scrBack, (WindowWidth - MeasureText(scrBack, 18)) / 2, panelY + panelH - 35, 18, GREEN);

            if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressed(KEY_M))
            {
                PlaySound(menuSelect);
                CurrentState = STATE_MENU;
            }
        }
        // STATE: PREVIOUS GAMEPLAYS (PERMANENT MATCH REPOSITORY)
        else if (CurrentState == STATE_HISTORY)
        {
            UpdateMusicStream(bgmMenu);
            int panelW = 1320, panelH = 760;
            int panelX = (WindowWidth - panelW) / 2, panelY = (WindowHeight - panelH) / 2;

            DrawCyberBox((Rectangle){ (float)panelX, (float)panelY, (float)panelW, (float)panelH },
                         SKYBLUE, Fade((Color){ 10, 15, 30, 255 }, 0.92f),
                         "PERMANENT FLIGHT LOGS // HISTORICAL BATTLE ARCHIVE", GOLD);

            char histTitle[] = "PREVIOUS GAMEPLAYS & BATTLE TELEMETRY";
            DrawText(histTitle, (WindowWidth - MeasureText(histTitle, 36)) / 2, panelY + 36, 36, GOLD);
            char histSub[] = "RECENT MATCHES AT TOP  |  USE [UP / DOWN ARROWS] TO SCROLL ARCHIVE";
            DrawText(histSub, (WindowWidth - MeasureText(histSub, 17)) / 2, panelY + 82, 17, RAYWHITE);
            DrawLine(panelX + 60, panelY + 110, panelX + panelW - 60, panelY + 110, SKYBLUE);

            int visibleSlots = 4;
            int maxScroll = (historyCount > visibleSlots) ? (historyCount - visibleSlots) : 0;

            if (IsKeyPressed(KEY_UP))   { PlaySound(menuMove); historyScroll--; if (historyScroll < 0) historyScroll = 0; }
            if (IsKeyPressed(KEY_DOWN)) { PlaySound(menuMove); historyScroll++; if (historyScroll > maxScroll) historyScroll = maxScroll; }

            if (historyCount == 0)
            {
                const char* noData = "NO RECORDED COMBAT MISSIONS FOUND IN PERMANENT STORAGE.";
                DrawText(noData, (WindowWidth - MeasureText(noData, 22)) / 2, panelY + 320, 22, GRAY);
            }
            else
            {
                int cardW = panelW - 120;
                int cardH = 125;
                int cardX = panelX + 60;
                int startY = panelY + 125;

                for (int v = 0; v < visibleSlots; v++)
                {
                    int recIdx = historyScroll + v;
                    if (recIdx >= historyCount) break;

                    MatchRecord r = historyList[recIdx];
                    int cardY = startY + (v * (cardH + 16));

                    DrawRectangle(cardX, cardY, cardW, cardH, Fade(BLACK, 0.75f));
                    DrawRectangleLines(cardX, cardY, cardW, cardH, r.isVictory ? LIME : RED);

                    // Entry Index & Status
                    const char* tag = (recIdx == 0) ? TextFormat("MISSION #%d [LATEST]", historyCount - recIdx) : TextFormat("MISSION #%d", historyCount - recIdx);
                    DrawText(tag, cardX + 20, cardY + 14, 18, YELLOW);

                    if (r.isVictory)
                    {
                        DrawText("[ VICTORY - EARTH SAVED ]", cardX + 240, cardY + 14, 18, LIME);
                        int mins = (int)r.runTime / 60;
                        int secs = (int)r.runTime % 60;
                        DrawText(TextFormat("TIME TO WIN: %02d:%02d", mins, secs), cardX + cardW - 240, cardY + 14, 18, GOLD);
                    }
                    else
                    {
                        DrawText("[ DEFEAT - EARTH OVERRUN ]", cardX + 240, cardY + 14, 18, RED);
                    }

                    DrawLine(cardX + 15, cardY + 38, cardX + cardW - 15, cardY + 38, DARKGRAY);

                    int col1 = cardX + 24;
                    int col2 = cardX + 430;
                    int col3 = cardX + 850;
                    int row1Y = cardY + 48;
                    int row2Y = cardY + 74;
                    int row3Y = cardY + 98;

                    // Most Aliens Killed (Top Gun)
                    if (r.hero1Kills > r.hero2Kills)      DrawText(TextFormat("MOST KILLS: %s (%d)", r.hero1PilotName, r.hero1Kills), col1, row1Y, 16, LIME);
                    else if (r.hero2Kills > r.hero1Kills) DrawText(TextFormat("MOST KILLS: %s (%d)", r.hero2PilotName, r.hero2Kills), col1, row1Y, 16, YELLOW);
                    else                                  DrawText(TextFormat("MOST KILLS: TIED (%d)", r.hero1Kills), col1, row1Y, 16, WHITE);

                    // Dreadnought Breaker (Most Boss Damage)
                    if (r.hero1BossDamage > r.hero2BossDamage)      DrawText(TextFormat("DREADNOUGHT BREAKER: %s (%d HP)", r.hero1PilotName, r.hero1BossDamage), col2, row1Y, 16, LIME);
                    else if (r.hero2BossDamage > r.hero1BossDamage) DrawText(TextFormat("DREADNOUGHT BREAKER: %s (%d HP)", r.hero2PilotName, r.hero2BossDamage), col2, row1Y, 16, YELLOW);
                    else                                            DrawText(TextFormat("DREADNOUGHT BREAKER: TIED (%d HP)", r.hero1BossDamage), col2, row1Y, 16, WHITE);

                    // Fewest Hits Taken (Untouchable Ace)
                    if (r.hero1HitsTaken < r.hero2HitsTaken)      DrawText(TextFormat("FEWEST HITS: %s (%d)", r.hero1PilotName, r.hero1HitsTaken), col3, row1Y, 16, LIME);
                    else if (r.hero2HitsTaken < r.hero1HitsTaken) DrawText(TextFormat("FEWEST HITS: %s (%d)", r.hero2PilotName, r.hero2HitsTaken), col3, row1Y, 16, YELLOW);
                    else                                          DrawText(TextFormat("FEWEST HITS: TIED (%d)", r.hero1HitsTaken), col3, row1Y, 16, WHITE);

                    // Row 2: Final Scores
                    DrawText(TextFormat("Shoab Score: %05d", r.hero1Score), col1, row2Y, 15, RAYWHITE);
                    DrawText(TextFormat("Nayemul Score: %05d", r.hero2Score), col2, row2Y, 15, RAYWHITE);

                    // Row 3: Custom Pilot Names Underneath Ship Titles
                    DrawText(TextFormat("Shoab's Spaceship [Pilot: %s]", r.hero1PilotName), col1, row3Y, 15, LIME);
                    DrawText(TextFormat("Nayemul's Spaceship [Pilot: %s]", r.hero2PilotName), col2, row3Y, 15, YELLOW);
                }

                // Scroll Bar Indicator
                if (maxScroll > 0)
                {
                    DrawText(TextFormat("SCROLL: %d / %d", historyScroll + 1, maxScroll + 1), panelX + panelW - 190, panelY + panelH - 42, 16, SKYBLUE);
                }
            }

            char histFooter[] = "USE [UP / DOWN ARROWS] TO SCROLL LOGS   |   [BACKSPACE / M] RETURN TO MENU";
            DrawText(histFooter, (WindowWidth - MeasureText(histFooter, 18)) / 2, panelY + panelH - 38, 18, GREEN);

            if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressed(KEY_M))
            {
                PlaySound(menuSelect);
                CurrentState = STATE_MENU;
            }
        }
        // STATE: CREDITS SCREEN for both developers ;) ,it's totally honest!!!
        else if (CurrentState == STATE_CREDITS)
        {
            UpdateMusicStream(bgmMenu);
            int panelW = 1260, panelH = 750;
            int panelX = (WindowWidth - panelW) / 2, panelY = (WindowHeight - panelH) / 2;

            DrawCyberBox((Rectangle){ (float)panelX, (float)panelY, (float)panelW, (float)panelH },
                         SKYBLUE, Fade((Color){ 10, 15, 30, 255 }, 0.92f),
                         "PROJECT DEVELOPERS & CREDENTIALS", GOLD);

            char credHeader[] = "PROJECT DEVELOPERS & CREDENTIALS";
            DrawText(credHeader, (WindowWidth - MeasureText(credHeader, 38)) / 2, panelY + 30, 38, GOLD);

            if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_TAB))
            {
                PlaySound(menuMove);
                creditsTab = 1 - creditsTab;
            }

            int tabW = 340, tabH = 44, tabY = panelY + 80;
            int t1X = (WindowWidth - (tabW * 2 + 20)) / 2;
            int t2X = t1X + tabW + 20;

            DrawRectangle(t1X, tabY, tabW, tabH, (creditsTab == 0) ? Fade(SKYBLUE, 0.35f) : Fade(BLACK, 0.65f));
            DrawRectangleLines(t1X, tabY, tabW, tabH, (creditsTab == 0) ? YELLOW : DARKGRAY);
            DrawText("1. NAYEMUL ISLAM (LEAD)", t1X + (tabW - MeasureText("1. NAYEMUL ISLAM (LEAD)", 18)) / 2, tabY + 13, 18, (creditsTab == 0) ? YELLOW : LIGHTGRAY);

            DrawRectangle(t2X, tabY, tabW, tabH, (creditsTab == 1) ? Fade(SKYBLUE, 0.35f) : Fade(BLACK, 0.65f));
            DrawRectangleLines(t2X, tabY, tabW, tabH, (creditsTab == 1) ? LIME : DARKGRAY);
            DrawText("2. MD. SHOAB MAHMUD", t2X + (tabW - MeasureText("2. MD. SHOAB MAHMUD", 18)) / 2, tabY + 13, 18, (creditsTab == 1) ? LIME : LIGHTGRAY);

            DrawLine(panelX + 80, tabY + 54, panelX + panelW - 80, tabY + 54, SKYBLUE);

            int cardW = 1140, cardH = 530, cardX = (WindowWidth - cardW) / 2, cardY = tabY + 65;
            DrawRectangle(cardX, cardY, cardW, cardH, Fade(BLACK, 0.85f));
            DrawRectangleLines(cardX, cardY, cardW, cardH, (creditsTab == 0) ? YELLOW : LIME);

            if (creditsTab == 0)
            {
                // NAYEMUL ISLAM'S tab with Aspect-Ratio Preserved High-Resolution Framing
                int pSize = 135, pX = cardX + 45, pY = cardY + 50;
                DrawText("Nayemul Islam", pX + (pSize - MeasureText("Nayemul Islam", 22)) / 2, cardY + 18, 22, YELLOW);

                DrawRectangle(pX - 3, pY - 3, pSize + 6, pSize + 6, Fade(BLACK, 0.85f));
                if (credPortraitNayemul.id > 0)
                {
                    DrawTexturePro(credPortraitNayemul,
                                   (Rectangle){ 0, 0, (float)credPortraitNayemul.width, (float)credPortraitNayemul.height },
                                   (Rectangle){ (float)pX, (float)pY, (float)pSize, (float)pSize },
                                   (Vector2){ 0, 0 }, 0.0f, WHITE);
                }
                else
                {
                    DrawRectangle(pX, pY, pSize, pSize, Fade(DARKBROWN, 0.5f));
                }
                DrawRectangleLinesEx((Rectangle){ (float)pX, (float)pY, (float)pSize, (float)pSize }, 2.5f, YELLOW);

                DrawText("Roll ID: 2505087", pX + (pSize - MeasureText("Roll ID: 2505087", 18)) / 2, pY + pSize + 14, 18, GREEN);
                DrawText("CSE, Section B", pX + (pSize - MeasureText("CSE, Section B", 16)) / 2, pY + pSize + 38, 16, SKYBLUE);

                DrawLine(pX + pSize + 35, cardY + 20, pX + pSize + 35, cardY + cardH - 20, DARKGRAY);

                int listX = pX + pSize + 55;
                int itemY = cardY + 16;
                DrawText("CORE CONTRIBUTIONS & ARCHITECTURE:", listX, itemY, 18, GOLD);
                itemY += 24;

                const char* nayemulContribs[20] = {
                    "1. MacBook native Spaces borderless fullscreen engine & Cocoa window bridge",
                    "2. Unified [M] key navigation across all game states, menus, and debriefing screens",
                    "3. Interactive drifting metallic asteroids, dynamic space cover & physics deflection",
                    "4. Asteroid 60% engine slowdown penalties, collision sparks & shatter rewards",
                    "5. Two Flavors of Freeze: 0.18s Activation Super-Pause with chromatic screen flash",
                    "6. Kinetic Impact Hit-Stop frame freeze on laser piercing and missile impacts",
                    "7. Interceptor canopy glass specular reflections responding to laser blaster fire",
                    "8. Last Stand Overdrive: 2.5s rapid plasma surge and shield flaring on wingman crash",
                    "9. Layered dynamic Nebula gas clouds drifting across exospheric deep space",
                    "10. Atmospheric space lightning flashes triggered by enemy telemetry pulses",
                    "11. High-tech glassmorphic cockpit comms dialogue boxes and cybernetic brackets",
                    "12. Real-time international breaking news broadcast chyron interface with live bugs",
                    "13. Alien Dreadnought Overlord encrypted glitch-frequency warning boxes",
                    "14. Full interactive Story Mode, narrative, and emergency defense broadcasts",
                    "15. Dreadnought Carrier Boss mechanics, bilateral deflector pods & rage modes",
                    "16. Alien Commanders: Jammer HUD jamming and Warp micro-teleportation systems",
                    "17. Hero 8-Way Guided Cluster Salvo with expanding destructive kinetic blast radii",
                    "18. Directed and balanced 98% of in-game audio sound effects and multi-track combat BGM",
                    "19. 3-layer parallax star engine, CRT scanlines, and End-of-Run Rank Plaque Grading",
                    "20. Micro-Singularity Black Hole drift mechanics, smooth collapse & haunted Void Phantom AI"
                };

                for (int c = 0; c < 20; c++)
                {
                    DrawText(nayemulContribs[c], listX, itemY, 14, (c % 2 == 0) ? RAYWHITE : LIGHTGRAY);
                    itemY += 24;
                }
            }
            else
            {
                // MD. SHOAB MAHMUD'S TAB with Aspect-Ratio Preserved High-Resolution Framing
                int pSize = 135, pX = cardX + 45, pY = cardY + 50;
                DrawText("Md. Shoab Mahmud", pX + (pSize - MeasureText("Md. Shoab Mahmud", 22)) / 2, cardY + 18, 22, LIME);

                DrawRectangle(pX - 3, pY - 3, pSize + 6, pSize + 6, Fade(BLACK, 0.85f));
                if (credPortraitShoab.id > 0)
                {
                    DrawTexturePro(credPortraitShoab,
                                   (Rectangle){ 0, 0, (float)credPortraitShoab.width, (float)credPortraitShoab.height },
                                   (Rectangle){ (float)pX, (float)pY, (float)pSize, (float)pSize },
                                   (Vector2){ 0, 0 }, 0.0f, WHITE);
                }
                else
                {
                    DrawRectangle(pX, pY, pSize, pSize, Fade(DARKGREEN, 0.5f));
                }
                DrawRectangleLinesEx((Rectangle){ (float)pX, (float)pY, (float)pSize, (float)pSize }, 2.5f, LIME);

                DrawText("Roll ID: 2505066", pX + (pSize - MeasureText("Roll ID: 2505066", 18)) / 2, pY + pSize + 14, 18, GREEN);
                DrawText("CSE, Section B", pX + (pSize - MeasureText("CSE, Section B", 16)) / 2, pY + pSize + 38, 16, SKYBLUE);

                DrawLine(pX + pSize + 35, cardY + 20, pX + pSize + 35, cardY + cardH - 20, DARKGRAY);

                int listX = pX + pSize + 55;
                int itemY = cardY + 24;
                DrawText("CORE CONTRIBUTIONS & ARCHITECTURE:", listX, itemY, 19, GOLD);
                itemY += 32;

                const char* shoabContribs[] = {
                    "1. Built the basic core foundation and base loop of the game",
                    "2. Worked with regular alien sprites, grid positioning, and sound fx",
                    "3. Added the sprite and core mechanisms of the Scorpion Hero (Aegis-1)",
                    "4. Added the devastating special Hyper Laser Beam feature of Scorpion",
                    "5. Added custom sprites and sound effects for the Scorpion Laser Beam",
                    "6. Coordinated and assisted Nayemul in various feature implementations & debugging",
                    "7. Defined initial functions and mechanisms that were reused throughout the game",
                    "8. Identified and fixed the primary aliens movement and boundary bounce bug"
                };

                for (int c = 0; c < 8; c++)
                {
                    DrawText(shoabContribs[c], listX, itemY, 16, (c % 2 == 0) ? RAYWHITE : LIGHTGRAY);
                    itemY += 28;
                }
            }

            char credBack[] = "[< LEFT / RIGHT >] SWITCH DEVELOPER TAB   |   [BACKSPACE / M] RETURN TO MENU";
            DrawText(credBack, (WindowWidth - MeasureText(credBack, 18)) / 2, panelY + panelH - 30, 18, GREEN);

            if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressed(KEY_M))
            {
                PlaySound(menuSelect);
                CurrentState = STATE_MENU;
            }
        }
        // STATE: OPTIONS (mainly video and sound setting!!)
        else if (CurrentState == STATE_OPTIONS)
        {
            UpdateMusicStream(bgmMenu);
            int panelW = 1260, panelH = 740;
            int panelX = (WindowWidth - panelW) / 2, panelY = (WindowHeight - panelH) / 2;

            DrawCyberBox((Rectangle){ (float)panelX, (float)panelY, (float)panelW, (float)panelH },
                         SKYBLUE, Fade((Color){ 10, 15, 30, 255 }, 0.92f),
                         "SYSTEM & TACTICAL CONFIGURATION", GOLD);

            char optTitle[] = "SYSTEM & TACTICAL CONFIGURATION";
            DrawText(optTitle, (WindowWidth - MeasureText(optTitle, 40)) / 2, panelY + 40, 40, GOLD);

            if (IsKeyPressed(KEY_TAB)) { PlaySound(menuMove); OptionsTab = 1 - OptionsTab; }

            int tabW = 280, tabH = 46, soundTabX = panelX + 320, videoTabX = panelX + 660, tabY = panelY + 110;
            DrawRectangle(soundTabX, tabY, tabW, tabH, (OptionsTab == 0) ? Fade(SKYBLUE, 0.35f) : Fade(BLACK, 0.6f));
            DrawRectangleLines(soundTabX, tabY, tabW, tabH, (OptionsTab == 0) ? YELLOW : DARKGRAY);
            DrawText("1. AUDIO SETTINGS", soundTabX + 35, tabY + 14, 20, (OptionsTab == 0) ? YELLOW : LIGHTGRAY);

            DrawRectangle(videoTabX, tabY, tabW, tabH, (OptionsTab == 1) ? Fade(SKYBLUE, 0.35f) : Fade(BLACK, 0.6f));
            DrawRectangleLines(videoTabX, tabY, tabW, tabH, (OptionsTab == 1) ? YELLOW : DARKGRAY);
            DrawText("2. VIDEO SETTINGS", videoTabX + 35, tabY + 14, 20, (OptionsTab == 1) ? YELLOW : LIGHTGRAY);
            DrawLine(panelX + 60, tabY + 65, panelX + panelW - 60, tabY + 65, DARKBLUE);

            if (OptionsTab == 0)
            {
                if (IsKeyPressed(KEY_UP))   { PlaySound(menuMove); SoundSelection--; if (SoundSelection < 0) SoundSelection = 2; }
                if (IsKeyPressed(KEY_DOWN)) { PlaySound(menuMove); SoundSelection++; if (SoundSelection > 2) SoundSelection = 0; }
                char bgmTracks[3][32] = { "1. AGGRESSIVE (COMBAT)", "2. CASUAL (SPACE WALK)", "3. SOFT (DEEP SPACE)" };

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
                DrawText("STAGE COMBAT BGM", panelX + 160, rowY, 24, (SoundSelection == 0) ? YELLOW : WHITE);
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
            else
            {
                if (IsKeyPressed(KEY_UP))   { PlaySound(menuMove); VideoSelection--; if (VideoSelection < 0) VideoSelection = 3; }
                if (IsKeyPressed(KEY_DOWN)) { PlaySound(menuMove); VideoSelection++; if (VideoSelection > 3) VideoSelection = 0; }

                if (VideoSelection == 0)
                {
                    if (IsKeyDown(KEY_LEFT))  { Brightness -= 0.3f * Time; if (Brightness < 0.5f) Brightness = 0.5f; }
                    if (IsKeyDown(KEY_RIGHT)) { Brightness += 0.3f * Time; if (Brightness > 1.5f) Brightness = 1.5f; }
                }
                else if (VideoSelection == 1 && (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_ENTER)))
                {
                    PlaySound(menuSelect); ScanlinesOn = !ScanlinesOn;
                }
                else if (VideoSelection == 2 && (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_ENTER)))
                {
                    PlaySound(menuSelect); StarfieldOn = !StarfieldOn;
                }
                else if (VideoSelection == 3 && (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_ENTER)))
                {
                    PlaySound(menuSelect); ToggleGameFullscreen();
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
                DrawText("FULLSCREEN [F / F11]", panelX + 160, rowY, 24, (VideoSelection == 3) ? YELLOW : WHITE);
                DrawText(IsGameFullscreen() ? "[ ENABLED ]" : "[ DISABLED ]", panelX + 540, rowY, 24, IsGameFullscreen() ? LIME : RED);
            }

            char optFooter[] = "[TAB] SWITCH TAB   |   [LEFT / RIGHT / ENTER] ADJUST   |   [BACKSPACE / M] RETURN";
            DrawText(optFooter, (WindowWidth - MeasureText(optFooter, 18)) / 2, panelY + panelH - 45, 18, GREEN);

            if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressed(KEY_M))
            {
                PlaySound(menuSelect);
                CurrentState = STATE_MENU;
            }
        }
        // STATE: LAUNCHing BRIEFING
        else if (CurrentState == STATE_LAUNCH)
        {
            UpdateMusicStream(bgmStory);
            DrawCircle(WindowWidth / 2, WindowHeight + 620, 840, DARKBLUE);
            DrawCircle(WindowWidth / 2, WindowHeight + 620, 830, (Color){ 20, 50, 110, 255 });
            DrawCircle(WindowWidth / 2 - 200, WindowHeight - 30, 120, (Color){ 30, 90, 45, 255 });
            DrawCircle(WindowWidth / 2 + 180, WindowHeight - 50, 140, (Color){ 35, 100, 50, 255 }); //drawing structure similar to earth...
            DrawCircleLines(WindowWidth / 2, WindowHeight + 620, 842, Fade(SKYBLUE, 0.45f));
            DrawCircleLines(WindowWidth / 2, WindowHeight + 620, 846, Fade(SKYBLUE, 0.20f));

            if (launchCountdownActive)
            {
                launchCountdownTimer -= Time;
                launchShip1Pos.y -= 180.0f * Time; launchShip2Pos.y -= 180.0f * Time;
                if (launchCountdownTimer <= 0.0f)
                {
                    StopMusicStream(bgmStory);
                    CurrentState = STATE_VIDEO_PLAY;
                    StartVideo(0, 12.6f);
                    Hero1Pos = (Vector2){ WindowWidth * 0.35f, WindowHeight - HeroHeight };
                    Hero2Pos = (Vector2){ WindowWidth * 0.65f, WindowHeight - HeroHeight };
                }
            }

            DrawTexturePro(HeroTexture, (Rectangle){ 0, 0, (float)HeroTexture.width, (float)HeroTexture.height },
                           (Rectangle){ launchShip1Pos.x, launchShip1Pos.y, HeroWidth, HeroHeight }, (Vector2){ HeroWidth / 2.0f, HeroHeight / 2.0f }, -12.0f, WHITE);
            DrawStealthFlames(launchShip2Pos, HeroWidth, HeroHeight);
            DrawTexturePro(StealthHeroTexture, (Rectangle){ 0, 0, (float)StealthHeroTexture.width, (float)StealthHeroTexture.height },
                           (Rectangle){ launchShip2Pos.x, launchShip2Pos.y, HeroWidth, HeroHeight }, (Vector2){ HeroWidth / 2.0f, HeroHeight / 2.0f }, 12.0f, WHITE);

            int dlgW = 1200, dlgH = 260, dlgX = (WindowWidth - dlgW) / 2, dlgY = 60;
            DrawCyberBox((Rectangle){ (float)dlgX, (float)dlgY, (float)dlgW, (float)dlgH },
                         SKYBLUE, Fade((Color){ 10, 15, 30, 255 }, 0.94f),
                         "OPERATION SKYFALL // LAUNCH TELEMETRY", GOLD);

            if (launchDialogueIndex == 0)
            {
                DrawText("[ MISSION BRIEFING: OPERATION SKYFALL ]", dlgX + 40, dlgY + 28, 24, GOLD);
                DrawText(TextFormat("Shoab [%s]: \"Nayemul, long-range radar confirms the invasion grid!\"", inputHero1Name), dlgX + 40, dlgY + 75, 21, LIME);
                DrawText("\"The swarm is dropping fast. Millions of lives are on the line!\"", dlgX + 40, dlgY + 115, 20, LIGHTGRAY);
                DrawText(TextFormat("Nayemul [%s]: \"Our ships are ready. We charge straight through their front lines!\"", inputHero2Name), dlgX + 40, dlgY + 160, 21, YELLOW);
            }
            else if (launchDialogueIndex == 1)
            {
                DrawText("[ LAUNCH RAIL PRESSURE & DIAGNOSTICS ]", dlgX + 40, dlgY + 28, 24, GOLD);
                DrawText(TextFormat("Shoab [%s]: \"Cooling line two is leaking, but we have no time to fix it. Nayemul, check your flight systems!\"", inputHero1Name), dlgX + 40, dlgY + 75, 20, LIME);
                DrawText(TextFormat("Nayemul [%s]: \"Diagnostics cleared! Thrusters are hot. Let's make sure Earth is safe when we return!\"", inputHero2Name), dlgX + 40, dlgY + 130, 20, YELLOW);
            }
            else if (launchDialogueIndex == 2)
            {
                DrawText("[ PRE-FLIGHT AUTHORIZATION - DUAL STRIKE FLEET ]", dlgX + 40, dlgY + 28, 24, GOLD);
                DrawText(TextFormat("Shoab [%s]: \"Cannons hot, shields linked! Here is the plan:\"", inputHero1Name), dlgX + 40, dlgY + 75, 21, LIME);
                DrawText("\"I will take the left side [A/D to Move, W to Fire]. You take the right!\"", dlgX + 40, dlgY + 115, 20, LIGHTGRAY);
                DrawText(TextFormat("Nayemul [%s]: \"Got it! Stealth wings locked [Arrow Keys to Move, UP to Fire]. Let's go!\"", inputHero2Name), dlgX + 40, dlgY + 160, 21, YELLOW);
            }
            else if (launchDialogueIndex == 3)
            {
                DrawText("[ ATMOSPHERIC BREACH - DEFENSE COMMAND ]", dlgX + 40, dlgY + 28, 24, GOLD);
                DrawText("Command: \"Command to Strike Flight: You have cleared the atmosphere. Sector Alpha is open. Weapons free!\"", dlgX + 40, dlgY + 75, 22, SKYBLUE);
                DrawText(TextFormat("%s & %s: \"Orbit reached! Weapons free! Let's save Earth!\"", inputHero2Name, inputHero1Name), dlgX + 40, dlgY + 135, 22, GREEN);
            }

            if (!launchCountdownActive)
            {
                char promptText[] = "PRESS [ENTER] OR [SPACE] TO ADVANCE LAUNCH SEQUENCE";
                if (((int)(GetTime() * 3)) % 2 == 0)
                    DrawText(promptText, (WindowWidth - MeasureText(promptText, 18)) / 2, dlgY + dlgH - 40, 18, GREEN);

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
                DrawText(launchText, (WindowWidth - MeasureText(launchText, 26)) / 2, dlgY + dlgH - 50, 26, YELLOW);
            }
        }

        // STATE: GAMEPLAY (main!!!)
        else if (CurrentState == STATE_GAMEPLAY)
        {
            static float hero1AsteroidSlowTimer = 0.0f;
            static float hero2AsteroidSlowTimer = 0.0f;
            static float smokeSpawnTimer = 0.0f;

            if (!isPaused && !GameOver && !bossDefeated)
            {
                runPlayTime += Time;
            }

            // Timers for Two freezing effect!!
            if (superPauseTimer > 0.0f) superPauseTimer -= rawTime;
            if (hitStopTimer > 0.0f)    hitStopTimer -= rawTime;
            bool isFrozen = (superPauseTimer > 0.0f) || (hitStopTimer > 0.0f);

            // Canopy Specular Glint and Overdrive Timers(took this feature from another game...)
            if (canopyGlintTimer > 0.0f) canopyGlintTimer -= rawTime;
            if (hero1OverdriveTimer > 0.0f && !isPaused) hero1OverdriveTimer -= Time;
            if (hero2OverdriveTimer > 0.0f && !isPaused) hero2OverdriveTimer -= Time;

            // Stop all enemy, weapon, and movement sounds on Game Over or Win logic!!!!! (fixed this bug ;)
            if (deathSequenceActive || GameOver || bossDefeated)
            {
                StopSound(shoot);
                StopSound(AlienShoot);
                StopSound(sndAlienStep);
                StopSound(bossLaserSound);
                StopSound(sndDefibHum);
                StopSound(sndWarningSiren);
                StopSound(sndJammerHum);
                StopSound(sndJammerShot);
                StopSound(sndEmpBlast);
                StopSound(sndWarpGlide);
                StopSound(sndWarpShot);
                StopSound(sndTeleport);
                StopSound(bossEnrage);
                StopSound(sndRocketBoost);
                StopSound(sndClusterLaunch);
                StopSound(sndClusterExplode);
                StopSound(sndShieldActivate);
                StopSound(sndShieldDeflect);
                StopSound(sndHudGlitch);
                StopSound(sndEvacBoom);
                StopSound(sndMagRailCharge);
                StopSound(sndLaserCharge);
                StopSound(sndBossWarpIn);
                StopSound(sndAlienStep);
                StopSound(sndDryFire);
                StopSound(sndTetherConnect);
                StopSound(sndAirdropIncoming);
                StopSound(sndAsteroidHitHero);
                StopSound(sndAsteroidRicochet);
                StopSound(sndAsteroidShatter);
                StopSound(sndCockpitSpark);
                StopSound(sndHullAlarm);

                // Stop Black Hole and Void Entity sounds
                StopSound(sndBlackHoleDrone);
                StopSound(sndBlackHolePull);
                StopSound(sndBlackHoleCrush);
                StopSound(sndEntityDrone);
                StopSound(sndEntitySpawn);
                StopSound(sndEntityAttack);
                StopSound(sndEntityScream);
            }

            // BGM Management,one for regular one and another for boss fighting
            if (bossWarningActive || bossWarpActive || bossActive)
            {
                if (gameplayBgmActive)
                {
                    StopMusicStream(bgmGameplay[BgmTrackIndex]);
                    gameplayBgmActive = false;
                }
                UpdateMusicStream(bgmBoss);
            }
            else if (!GameOver && !bossDefeated && !deathSequenceActive)
            {
                if (!gameplayBgmActive)
                {
                    PlayMusicStream(bgmGameplay[BgmTrackIndex]);
                    gameplayBgmActive = true;
                }
                UpdateMusicStream(bgmGameplay[BgmTrackIndex]);
            }

            if (!GameOver && !bossDefeated && !deathSequenceActive && IsKeyPressed(KEY_P))
            {
                isPaused = !isPaused;
                if (isPaused) {
                    PlaySound(pauseIn);
                    StopSound(sndBlackHoleDrone);
                    StopSound(sndEntityDrone);
                }
                else PlaySound(pauseOut);
            }

            float heroDistX = fabsf(Hero1Pos.x - Hero2Pos.x);
            bool heroesTethered = (Hero1Lives > 0 && Hero2Lives > 0 && heroDistX <= (WindowWidth / 4.0f));

            if (heroesTethered && !prevHeroesTethered && !isPaused && !GameOver && !bossDefeated && !deathSequenceActive)
            {
                PlaySound(sndTetherConnect);
            }
            prevHeroesTethered = heroesTethered;

            if (teamShieldCooldownTimer > 0.0f && !isPaused && !isFrozen && !deathSequenceActive && !GameOver && !bossDefeated)
            {
                teamShieldCooldownTimer -= Time;
            }
            if (teamShieldActive && !isPaused && !isFrozen && !deathSequenceActive && !GameOver && !bossDefeated)
            {
                teamShieldActiveTimer -= Time;
                if (teamShieldActiveTimer <= 0.0f) teamShieldActive = false;
            }

            if (bossWarpActive && !isPaused && !isFrozen && !deathSequenceActive && !GameOver)
            {
                bossWarpTimer -= Time;
                if (bossWarpTimer <= 0.0f)
                {
                    bossWarpActive = false;
                    bossActive = true;
                    screenCamera.offset = (Vector2){ 0, 0 }; //shaking!!!
                }
            }

            if (evacActive && !isPaused && !isFrozen)
            {
                evacTimer -= Time;
                if (evacTimer <= 0.0f)
                {
                    evacActive = false;
                    CurrentState = STATE_VIDEO_PLAY;
                    StartVideo(2, 10.4f);
                }
            }

            if (IsKeyPressed(KEY_ENTER) && heroesTethered && teamShieldCooldownTimer <= 0.0f && !teamShieldActive &&
                !GameOver && !bossDefeated && !deathSequenceActive && !bossWarningActive && !bossWarpActive && !evacActive && !isPaused)
            {
                teamShieldActive = true;
                teamShieldActiveTimer = 4.0f;
                teamShieldCooldownTimer = 25.0f;
                
                float midX = (Hero1Pos.x + Hero2Pos.x) / 2.0f;
                if (midX < TeamShieldRadius) midX = TeamShieldRadius;
                if (midX > WindowWidth - TeamShieldRadius) midX = WindowWidth - TeamShieldRadius;
                teamShieldCenterX = midX;
                PlaySound(sndShieldActivate);
            }

            float domeCenterX = teamShieldCenterX, domeBaseY = WindowHeight, domeRadius = TeamShieldRadius;

            // Trigger Black Hole when Boss Bilateral Deflector Pods are broken
            if (bossActive && !blackHole.spawned && bossLeftPodHp <= 0 && bossRightPodHp <= 0)
            {
                blackHole.spawned = true;
                blackHole.active = true;
                blackHole.collapsing = false;
                blackHole.scale = 1.0f;
                blackHole.alpha = 1.0f;
                blackHole.basePosX = WindowWidth / 2.0f;
                blackHole.pos = (Vector2){ WindowWidth / 2.0f, 60.0f };
                blackHole.driftTimer = 0.0f;
                PlaySound(sndBlackHolePull);
            }

            // Black Hole Slow Downward Movement, Lateral Oscillation, and Smooth Despawn
            if (blackHole.active && !isPaused && !isFrozen && !deathSequenceActive && !GameOver && !bossDefeated)
            {
                blackHole.driftTimer += Time;
                blackHole.rotationAngle += 115.0f * Time;

                if (!IsSoundPlaying(sndBlackHoleDrone) && !blackHole.collapsing) {
                    PlaySound(sndBlackHoleDrone);
                }

                // Slow downward drift & gentle lateral sine wave swaying
                if (!blackHole.collapsing)
                {
                    blackHole.pos.y += 25.0f * Time;
                    blackHole.pos.x = blackHole.basePosX + sinf(blackHole.driftTimer * 0.55f) * 190.0f;

                    // When crossing near the bottom window, initiate smooth collapse
                    if (blackHole.pos.y >= WindowHeight - 130.0f)
                    {
                        blackHole.collapsing = true;
                        blackHole.collapseTimer = 1.5f;
                    }
                }
                else
                {
                    // Smooth vanishing transition: shrink scale and fade alpha
                    blackHole.collapseTimer -= Time;
                    blackHole.scale = blackHole.collapseTimer / 1.5f;
                    blackHole.alpha = blackHole.scale;

                    if (blackHole.collapseTimer <= 0.0f)
                    {
                        blackHole.active = false;
                        blackHole.collapsing = false;
                        StopSound(sndBlackHoleDrone);

                        // Spawn the Mysterious Black Entity (Void Phantom) at the collapse coordinates
                        voidEntity.active = true;
                        voidEntity.spawned = true;
                        voidEntity.pos = blackHole.pos;
                        voidEntity.hp = voidEntity.maxHp;
                        voidEntity.alpha = 1.0f;
                        voidEntity.phasing = false;
                        voidEntity.attackTimer = 2.0f;
                        voidEntity.phaseTimer = 5.0f;
                        voidEntity.screamTimer = 9.0f;
                        PlaySound(sndEntitySpawn);
                        PlaySound(sndEntityDrone);
                    }
                }

                // Gravitational pull on Heroes (Team Shield dome negates gravity)
                if (!teamShieldActive && blackHole.scale > 0.1f)
                {
                    for (int h = 0; h < 2; h++)
                    {
                        Vector2* hPos = (h == 0) ? &Hero1Pos : &Hero2Pos;
                        int* lives = (h == 0) ? &Hero1Lives : &Hero2Lives;

                        if (*lives > 0)
                        {
                            Vector2 diff = Vector2Subtract(blackHole.pos, *hPos);
                            float dist = Vector2Length(diff);

                            if (dist < (blackHole.pullRadius * blackHole.scale) && dist > (blackHole.coreRadius * blackHole.scale))
                            {
                                Vector2 dir = Vector2Normalize(diff);
                                float pullFactor = (1.0f - (dist / (blackHole.pullRadius * blackHole.scale)));
                                float currentForce = pullFactor * pullFactor * blackHole.maxPullForce * blackHole.scale;
                                *hPos = Vector2Add(*hPos, Vector2Scale(dir, currentForce * Time));
                            }
                            else if (dist <= (blackHole.coreRadius * blackHole.scale))
                            {
                                (*lives)--;
                                *hPos = (h == 0) ? (Vector2){ WindowWidth * 0.35f, WindowHeight - HeroHeight } : (Vector2){ WindowWidth * 0.65f, WindowHeight - HeroHeight };
                                if (h == 0) hero1HitFlashTimer = 0.45f; else hero2HitFlashTimer = 0.45f;
                                hitStopTimer = 0.08f;
                                PlaySound(sndBlackHoleCrush);
                                PlaySound(sndHudGlitch);

                                if (*lives <= 0)
                                {
                                    if (h == 0) { Hero1CrashPos = *hPos; PlaySound(heroDeath); if (Hero2Lives > 0) { hero2OverdriveTimer = 2.5f; PlaySound(sndShieldActivate); } }
                                    else { Hero2CrashPos = *hPos; PlaySound(heroDeath); if (Hero1Lives > 0) { hero1OverdriveTimer = 2.5f; PlaySound(sndShieldActivate); } }
                                    if (Hero1Lives <= 0 && Hero2Lives <= 0) { deathSequenceActive = true; deathDelayTimer = 0.0f; StopSound(sndBlackHoleDrone); }
                                }
                            }
                        }
                    }
                }

                // Black Hole consumes Boss bullets, minion bullets, and disruption orbs (Boss core remains immune)
                for (int k = 0; k < MAX_BOSS_BULLETS; k++)
                    if (bossBulletActive[k] && Vector2Distance(bossBulletPos[k], blackHole.pos) < (blackHole.coreRadius + 22.0f) * blackHole.scale) bossBulletActive[k] = false;
                for (int mb = 0; mb < MAX_MINION_BULLETS; mb++)
                    if (minionBulletActive[mb] && Vector2Distance(minionBulletPos[mb], blackHole.pos) < (blackHole.coreRadius + 22.0f) * blackHole.scale) minionBulletActive[mb] = false;
                for (int k = 0; k < MAX_BOSS_ORBS; k++)
                    if (bossOrbActive[k] && Vector2Distance(bossOrbPos[k], blackHole.pos) < (blackHole.coreRadius + 28.0f) * blackHole.scale) bossOrbActive[k] = false;
            }

            // Mysterious Black Entity (Void Phantom) AI, Attacks & Phasing
            if (voidEntity.active && !isPaused && !isFrozen && !deathSequenceActive && !GameOver && !bossDefeated)
            {
                if (!IsSoundPlaying(sndEntityDrone)) PlaySound(sndEntityDrone);

                // Animate shifting ethereal mantle
                voidEntity.animTimer += Time;
                if (voidEntity.animTimer >= 0.16f)
                {
                    voidEntity.animTimer = 0.0f;
                    voidEntity.currentFrame = (voidEntity.currentFrame + 1) % 4;
                }

                // Phase Teleportation Mechanics (Dissolve & Reform above heroes)
                voidEntity.phaseTimer -= Time;
                if (voidEntity.phaseTimer <= 0.0f)
                {
                    voidEntity.phaseTimer = (float)GetRandomValue(6, 9);
                    voidEntity.targetedHero = (GetRandomValue(0, 1) == 0 && Hero1Lives > 0) ? 1 : (Hero2Lives > 0 ? 2 : 1);
                    float targetX = (voidEntity.targetedHero == 1 && Hero1Lives > 0) ? Hero1Pos.x : Hero2Pos.x;
                    voidEntity.pos = (Vector2){ targetX + (float)GetRandomValue(-40, 40), (float)GetRandomValue(90, 240) };
                    PlaySound(sndEntityAttack);
                    spaceLightningTimer = 0.05f;
                }

                // Gentle floating drift
                voidEntity.pos.x += voidEntity.speed.x * Time;
                if (voidEntity.pos.x <= 90) { voidEntity.pos.x = 90; voidEntity.speed.x = fabsf(voidEntity.speed.x); }
                else if (voidEntity.pos.x >= WindowWidth - 90) { voidEntity.pos.x = WindowWidth - 90; voidEntity.speed.x = -fabsf(voidEntity.speed.x); }

                // Void Distortion Orb Attacks
                voidEntity.attackTimer -= Time;
                if (voidEntity.attackTimer <= 0.0f)
                {
                    voidEntity.attackTimer = 2.6f;
                    Vector2 targetPos = (Hero1Lives > 0) ? Hero1Pos : Hero2Pos;
                    Vector2 shootDir = Vector2Normalize(Vector2Subtract(targetPos, voidEntity.pos));

                    for (int eo = 0; eo < MAX_ENTITY_ORBS; eo++)
                    {
                        if (!entityOrbs[eo].active)
                        {
                            entityOrbs[eo].active = true;
                            entityOrbs[eo].pos = voidEntity.pos;
                            entityOrbs[eo].vel = Vector2Scale(shootDir, 320.0f);
                            PlaySound(sndEntityAttack);
                            break;
                        }
                    }
                }

                // Eldritch Gravitational Scream Attack
                voidEntity.screamTimer -= Time;
                if (voidEntity.screamTimer <= 0.0f)
                {
                    voidEntity.screamTimer = 10.5f;
                    PlaySound(sndEntityScream);
                    radarJammedTimer = 3.5f;
                    explosionShakeTimer = 0.35f;
                    spaceLightningTimer = 0.12f;
                }
            }

            // Update Void Distortion Orbs
            for (int eo = 0; eo < MAX_ENTITY_ORBS; eo++)
            {
                if (entityOrbs[eo].active && !isFrozen && !deathSequenceActive && !GameOver && !bossDefeated)
                {
                    entityOrbs[eo].pos = Vector2Add(entityOrbs[eo].pos, Vector2Scale(entityOrbs[eo].vel, Time));

                    if (entityOrbs[eo].pos.y > WindowHeight + 30 || entityOrbs[eo].pos.x < -40 || entityOrbs[eo].pos.x > WindowWidth + 40)
                    {
                        entityOrbs[eo].active = false;
                    }
                    else if (!teamShieldActive && !evacActive)
                    {
                        Rectangle h1Rec = { Hero1Pos.x - HeroWidth / 2.0f, Hero1Pos.y, HeroWidth, HeroHeight };
                        Rectangle h2Rec = { Hero2Pos.x - HeroWidth / 2.0f, Hero2Pos.y, HeroWidth, HeroHeight };

                        if (Hero1Lives > 0 && CheckCollisionCircleRec(entityOrbs[eo].pos, 16.0f, h1Rec))
                        {
                            entityOrbs[eo].active = false;
                            Hero1Lives--; Hero1HitsTaken++; hero1HitFlashTimer = 0.40f;
                            hero1Debuffed = true; hero1DebuffTimer = 2.5f;
                            PlaySound(heroOuch); PlaySound(sndHudGlitch);
                            if (Hero1Lives <= 0) {
                                Hero1CrashPos = Hero1Pos; PlaySound(heroDeath);
                                if (Hero2Lives > 0) { hero2OverdriveTimer = 2.5f; PlaySound(sndShieldActivate); }
                                if (Hero2Lives <= 0) { deathSequenceActive = true; deathDelayTimer = 0.0f; StopSound(sndEntityDrone); }
                            }
                        }
                        else if (Hero2Lives > 0 && CheckCollisionCircleRec(entityOrbs[eo].pos, 16.0f, h2Rec))
                        {
                            entityOrbs[eo].active = false;
                            Hero2Lives--; Hero2HitsTaken++; hero2HitFlashTimer = 0.40f;
                            hero2Debuffed = true; hero2DebuffTimer = 2.5f;
                            PlaySound(heroOuch); PlaySound(sndHudGlitch);
                            if (Hero2Lives <= 0) {
                                Hero2CrashPos = Hero2Pos; PlaySound(heroDeath);
                                if (Hero1Lives > 0) { hero1OverdriveTimer = 2.5f; PlaySound(sndShieldActivate); }
                                if (Hero1Lives <= 0) { deathSequenceActive = true; deathDelayTimer = 0.0f; StopSound(sndEntityDrone); }
                            }
                        }
                    }
                }
            }

            // Asteroid coming down...
            if (!bossSpawned && !bossActive && !evacActive && !isPaused && !GameOver && !bossDefeated && !deathSequenceActive && !isFrozen)
            {
                asteroidSpawnTimer += Time;
                if (asteroidSpawnTimer >= 13.0f)
                {
                    asteroidSpawnTimer = 0.0f;
                    int spawned = 0;
                    for (int a = 0; a < MAX_ASTEROIDS && spawned < 2; a++)
                    {
                        if (!asteroids[a].active)
                        {
                            asteroids[a].active = true;
                            asteroids[a].hp = 25.0f;
                            asteroids[a].maxHp = 25.0f;
                            asteroids[a].radius = 42.0f;
                            asteroids[a].rotation = (float)GetRandomValue(0, 360);
                            asteroids[a].rotSpeed = (float)GetRandomValue(-45, 45);

                            if (spawned == 0)
                            {
                                asteroids[a].pos = (Vector2){ (float)GetRandomValue(80, WindowWidth / 2 - 80), -60.0f };
                                asteroids[a].speed = (Vector2){ (float)GetRandomValue(35, 85), (float)GetRandomValue(75, 120) };
                            }
                            else
                            {
                                asteroids[a].pos = (Vector2){ (float)GetRandomValue(WindowWidth / 2 + 80, WindowWidth - 80), -60.0f };
                                asteroids[a].speed = (Vector2){ (float)GetRandomValue(-85, -35), (float)GetRandomValue(75, 120) };
                            }
                            spawned++;
                        }
                    }
                }
            }

            // Updates on  Asteroids and Collision Handling (Movement stops on game over or victory)-bug fix....
            if (!isPaused && !isFrozen && !deathSequenceActive && !GameOver && !bossDefeated)
            {
                for (int a = 0; a < MAX_ASTEROIDS; a++)
                {
                    if (!asteroids[a].active) continue;
                    asteroids[a].pos = Vector2Add(asteroids[a].pos, Vector2Scale(asteroids[a].speed, Time));
                    asteroids[a].rotation += asteroids[a].rotSpeed * Time;

                    // Black hole pull and destruction of asteroids
                    if (blackHole.active && blackHole.scale > 0.1f)
                    {
                        Vector2 diff = Vector2Subtract(blackHole.pos, asteroids[a].pos);
                        float dist = Vector2Length(diff);
                        if (dist < (blackHole.pullRadius * blackHole.scale) && dist > (blackHole.coreRadius * blackHole.scale))
                        {
                            Vector2 dir = Vector2Normalize(diff);
                            float pullFactor = (1.0f - (dist / (blackHole.pullRadius * blackHole.scale)));
                            asteroids[a].pos = Vector2Add(asteroids[a].pos, Vector2Scale(dir, pullFactor * 260.0f * Time));
                        }
                        else if (dist <= (blackHole.coreRadius * blackHole.scale))
                        {
                            asteroids[a].active = false;
                            PlaySound(sndBlackHoleCrush);
                            hitStopTimer = 0.05f;
                            for (int d = 0; d < 6; d++)
                            {
                                for (int dp = 0; dp < MAX_ASTEROID_DEBRIS; dp++)
                                {
                                    if (!debrisPool[dp].active)
                                    {
                                        debrisPool[dp].active = true;
                                        debrisPool[dp].pos = asteroids[a].pos;
                                        debrisPool[dp].vel = (Vector2){ (float)GetRandomValue(-180, 180), (float)GetRandomValue(-180, 180) };
                                        debrisPool[dp].rotation = (float)GetRandomValue(0, 360);
                                        debrisPool[dp].rotSpeed = (float)GetRandomValue(-140, 140);
                                        debrisPool[dp].life = 0.8f;
                                        debrisPool[dp].maxLife = 0.8f;
                                        break;
                                    }
                                }
                            }
                            continue;
                        }
                    }

                    if (asteroids[a].pos.y > WindowHeight + 80 || asteroids[a].pos.x < -100 || asteroids[a].pos.x > WindowWidth + 100)
                    {
                        asteroids[a].active = false;
                        continue;
                    }

                    // Collision with Hero 1 Body
                    Rectangle h1Rec = { Hero1Pos.x - HeroWidth / 2.0f, Hero1Pos.y, HeroWidth, HeroHeight };
                    if (Hero1Lives > 0 && CheckCollisionCircleRec(asteroids[a].pos, asteroids[a].radius, h1Rec))
                    {
                        hero1AsteroidSlowTimer = 3.0f;
                        PlaySound(sndAsteroidHitHero);
                    }

                    // Collision with Hero 2 
                    Rectangle h2Rec = { Hero2Pos.x - HeroWidth / 2.0f, Hero2Pos.y, HeroWidth, HeroHeight };
                    if (Hero2Lives > 0 && CheckCollisionCircleRec(asteroids[a].pos, asteroids[a].radius, h2Rec))
                    {
                        hero2AsteroidSlowTimer = 3.0f;
                        PlaySound(sndAsteroidHitHero);
                    }

                    // Asteroid Covering and absorbing Alien Laser Fire
                    if (AlienBulletActive && CheckCollisionCircles(asteroids[a].pos, asteroids[a].radius, AlienBulletPos, AlienBulletWidth))
                    {
                        AlienBulletActive = false;
                        PlaySound(sndAsteroidRicochet);
                    }
                    for (int b = 0; b < MAX_COMMANDER_BULLETS; b++)
                    {
                        if (jammerBulletActive[b] && CheckCollisionCircleRec(asteroids[a].pos, asteroids[a].radius, (Rectangle){ jammerBulletPos[b].x - 4, jammerBulletPos[b].y, 8, 22 }))
                        {
                            jammerBulletActive[b] = false;
                            PlaySound(sndAsteroidRicochet);
                        }
                        if (warpBulletActive[b] && CheckCollisionCircleRec(asteroids[a].pos, asteroids[a].radius, (Rectangle){ warpBulletPos[b].x - 3, warpBulletPos[b].y, 6, 26 }))
                        {
                            warpBulletActive[b] = false;
                            PlaySound(sndAsteroidRicochet);
                        }
                    }

                    // Asteroid Cover, Absorbing Hero Bullets 
                    for (int h = 0; h < 2; h++)
                    {
                        Vector2* bPos = (h == 0) ? Hero1BulletPos : Hero2BulletPos;
                        bool* bActive = (h == 0) ? Hero1BulletActive : Hero2BulletActive;
                        int* hScore = (h == 0) ? &Hero1Score : &Hero2Score;
                        float* hSpecTime = (h == 0) ? &ShoabSpecialTime : &NayemulSpecialTime;

                        for (int i = 0; i < 2; i++)
                        {
                            if (bActive[i] && CheckCollisionCircleRec(asteroids[a].pos, asteroids[a].radius, (Rectangle){ bPos[i].x - BulletWidth / 2.0f, bPos[i].y, BulletWidth, BulletHeight }))
                            {
                                bActive[i] = false;
                                asteroids[a].hp -= 5.0f;
                                PlaySound(sndAsteroidRicochet);

                                if (asteroids[a].hp <= 0.0f)
                                {
                                    asteroids[a].active = false;
                                    *hScore += 100;
                                    *hSpecTime += FPS * 1.0f;
                                    PlaySound(sndAsteroidShatter);
                                    hitStopTimer = 0.04f;

                                    for (int d = 0; d < 6; d++)
                                    {
                                        for (int dp = 0; dp < MAX_ASTEROID_DEBRIS; dp++)
                                        {
                                            if (!debrisPool[dp].active)
                                            {
                                                debrisPool[dp].active = true;
                                                debrisPool[dp].pos = asteroids[a].pos;
                                                debrisPool[dp].vel = (Vector2){ (float)GetRandomValue(-160, 160), (float)GetRandomValue(-140, 140) };
                                                debrisPool[dp].rotation = (float)GetRandomValue(0, 360);
                                                debrisPool[dp].rotSpeed = (float)GetRandomValue(-120, 120);
                                                debrisPool[dp].life = 0.8f;
                                                debrisPool[dp].maxLife = 0.8f;
                                                break;
                                            }
                                        }
                                    }
                                    break;
                                }
                            }
                        }
                    }

                    // Shoab Special Beam Piercing Asteroid
                    if (Hero1SpecialBulletActive && CheckCollisionCircleRec(asteroids[a].pos, asteroids[a].radius, (Rectangle){ Hero1SpecialBulletPos.x - HeroWidth / 2.0f, Hero1SpecialBulletPos.y, HeroWidth, HeroHeight * 3 }))
                    {
                        asteroids[a].hp -= 15.0f;
                        hitStopTimer = 0.04f;
                        if (asteroids[a].hp <= 0.0f && asteroids[a].active)
                        {
                            asteroids[a].active = false;
                            Hero1Score += 100;
                            ShoabSpecialTime += FPS * 1.0f;
                            PlaySound(sndAsteroidShatter);
                        }
                    }

                    // Nayemul Cluster Missile Collision with Asteroid
                    for (int m = 0; m < MAX_CLUSTER_MISSILES; m++)
                    {
                        if (clusterMissileActive[m] && CheckCollisionCircles(asteroids[a].pos, asteroids[a].radius, clusterMissilePos[m], 10.0f))
                        {
                            clusterMissileActive[m] = false;
                            asteroids[a].hp -= 18.0f;
                            PlaySound(sndClusterExplode);
                            hitStopTimer = 0.05f;
                            if (asteroids[a].hp <= 0.0f && asteroids[a].active)
                            {
                                asteroids[a].active = false;
                                Hero2Score += 100;
                                NayemulSpecialTime += FPS * 1.0f;
                                PlaySound(sndAsteroidShatter);
                            }
                        }
                    }
                }

                // Update Debris Particles
                for (int dp = 0; dp < MAX_ASTEROID_DEBRIS; dp++)
                {
                    if (debrisPool[dp].active)
                    {
                        debrisPool[dp].life -= Time;
                        debrisPool[dp].pos = Vector2Add(debrisPool[dp].pos, Vector2Scale(debrisPool[dp].vel, Time));
                        debrisPool[dp].rotation += debrisPool[dp].rotSpeed * Time;
                        if (debrisPool[dp].life <= 0.0f) debrisPool[dp].active = false;
                    }
                }
            }

            // Smoke and  Sparks Emitters
            if (!isPaused && !GameOver && !bossDefeated && !deathSequenceActive && !isFrozen)
            {
                smokeSpawnTimer += Time;
                if (smokeSpawnTimer >= 0.08f)
                {
                    smokeSpawnTimer = 0.0f;
                    if (Hero1Lives == 2 || Hero1Lives == 1)
                    {
                        for (int s = 0; s < MAX_SMOKE_PARTICLES; s++)
                        {
                            if (!smokePool[s].active)
                            {
                                smokePool[s].active = true;
                                smokePool[s].pos = (Vector2){ Hero1Pos.x - 22.0f + (float)GetRandomValue(-4, 4), Hero1Pos.y + 40.0f };
                                smokePool[s].vel = (Vector2){ (float)GetRandomValue(-15, 15), (float)GetRandomValue(-45, -75) };
                                smokePool[s].size = (float)GetRandomValue(12, 22);
                                smokePool[s].life = 0.65f;
                                smokePool[s].maxLife = 0.65f;
                                break;
                            }
                        }
                    }
                    if (Hero2Lives == 2 || Hero2Lives == 1)
                    {
                        for (int s = 0; s < MAX_SMOKE_PARTICLES; s++)
                        {
                            if (!smokePool[s].active)
                            {
                                smokePool[s].active = true;
                                smokePool[s].pos = (Vector2){ Hero2Pos.x + 22.0f + (float)GetRandomValue(-4, 4), Hero2Pos.y + 40.0f };
                                smokePool[s].vel = (Vector2){ (float)GetRandomValue(-15, 15), (float)GetRandomValue(-45, -75) };
                                smokePool[s].size = (float)GetRandomValue(12, 22);
                                smokePool[s].life = 0.65f;
                                smokePool[s].maxLife = 0.65f;
                                break;
                            }
                        }
                    }
                }

                // Hero 1 Cockpit Spark sound and visuals
                if (Hero1Lives == 1)
                {
                    sparkTimer1 += Time;
                    if (sparkTimer1 >= 0.45f)
                    {
                        sparkTimer1 = 0.0f;
                        PlaySound(sndCockpitSpark);
                        for (int sp = 0; sp < MAX_SPARK_PARTICLES; sp++)
                        {
                            if (!sparkPool[sp].active)
                            {
                                sparkPool[sp].active = true;
                                sparkPool[sp].pos = (Vector2){ Hero1Pos.x + (float)GetRandomValue(-16, 16), Hero1Pos.y + 20.0f };
                                sparkPool[sp].vel = (Vector2){ (float)GetRandomValue(-80, 80), (float)GetRandomValue(-80, 80) };
                                sparkPool[sp].life = 0.22f;
                                sparkPool[sp].maxLife = 0.22f;
                                break;
                            }
                        }
                    }
                }

                // Hero 2 Cockpit Sparks
                if (Hero2Lives == 1)
                {
                    sparkTimer2 += Time;
                    if (sparkTimer2 >= 0.45f)
                    {
                        sparkTimer2 = 0.0f;
                        PlaySound(sndCockpitSpark);
                        for (int sp = 0; sp < MAX_SPARK_PARTICLES; sp++)
                        {
                            if (!sparkPool[sp].active)
                            {
                                sparkPool[sp].active = true;
                                sparkPool[sp].pos = (Vector2){ Hero2Pos.x + (float)GetRandomValue(-16, 16), Hero2Pos.y + 20.0f };
                                sparkPool[sp].vel = (Vector2){ (float)GetRandomValue(-80, 80), (float)GetRandomValue(-80, 80) };
                                sparkPool[sp].life = 0.22f;
                                sparkPool[sp].maxLife = 0.22f;
                                break;
                            }
                        }
                    }
                }

                if (Hero1Lives == 1 || Hero2Lives == 1)
                {
                    alarmSoundTimer += Time;
                    if (alarmSoundTimer >= 2.4f)
                    {
                        alarmSoundTimer = 0.0f;
                        PlaySound(sndHullAlarm);
                    }
                }

                for (int s = 0; s < MAX_SMOKE_PARTICLES; s++)
                {
                    if (smokePool[s].active)
                    {
                        smokePool[s].life -= Time;
                        smokePool[s].pos = Vector2Add(smokePool[s].pos, Vector2Scale(smokePool[s].vel, Time));
                        smokePool[s].size += 18.0f * Time;
                        if (smokePool[s].life <= 0.0f) smokePool[s].active = false;
                    }
                }

                for (int sp = 0; sp < MAX_SPARK_PARTICLES; sp++)
                {
                    if (sparkPool[sp].active)
                    {
                        sparkPool[sp].life -= Time;
                        sparkPool[sp].pos = Vector2Add(sparkPool[sp].pos, Vector2Scale(sparkPool[sp].vel, Time));
                        if (sparkPool[sp].life <= 0.0f) sparkPool[sp].active = false;
                    }
                }
            }

            if (teamShieldActive && !bossActive && !bossSpawned && !isPaused && !isFrozen && !deathSequenceActive && !GameOver && !bossDefeated)
            {
                for (int X = 0; X < AlienInX; X++)
                {
                    for (int Y = 0; Y < AlienInY; Y++)
                    {
                        if (AlienAlive[X][Y])
                        {
                            Vector2 alienCenter = { AlienPos[X][Y].x + AlienSize / 2.0f, AlienPos[X][Y].y + AlienSize / 2.0f };
                            if (Vector2Distance(alienCenter, (Vector2){ domeCenterX, domeBaseY }) <= domeRadius && alienCenter.y <= domeBaseY)
                            {
                                AlienAlive[X][Y] = false; AliensKilled++;
                                Hero1Score += 50; Hero2Score += 50;
                                PlaySound(damage);
                            }
                        }
                    }
                }
            }

            if (teamShieldActive && !isPaused && !isFrozen && !deathSequenceActive && !GameOver && !bossDefeated)
            {
                if (AlienBulletActive && Vector2Distance(AlienBulletPos, (Vector2){ domeCenterX, domeBaseY }) <= domeRadius && AlienBulletPos.y <= domeBaseY)
                {
                    AlienBulletActive = false; PlaySound(sndShieldDeflect);
                }

                for (int b = 0; b < MAX_COMMANDER_BULLETS; b++)
                {
                    if (jammerBulletActive[b] && Vector2Distance(jammerBulletPos[b], (Vector2){ domeCenterX, domeBaseY }) <= domeRadius && jammerBulletPos[b].y <= domeBaseY)
                    {
                        jammerBulletActive[b] = false; PlaySound(sndShieldDeflect);
                    }
                    if (warpBulletActive[b] && Vector2Distance(warpBulletPos[b], (Vector2){ domeCenterX, domeBaseY }) <= domeRadius && warpBulletPos[b].y <= domeBaseY)
                    {
                        warpBulletActive[b] = false; PlaySound(sndShieldDeflect);
                    }
                }

                if (bossActive)
                {
                    for (int mb = 0; mb < MAX_MINION_BULLETS; mb++)
                    {
                        if (minionBulletActive[mb] && Vector2Distance(minionBulletPos[mb], (Vector2){ domeCenterX, domeBaseY }) <= domeRadius && minionBulletPos[mb].y <= domeBaseY)
                        {
                            minionBulletActive[mb] = false; PlaySound(sndShieldDeflect);
                        }
                    }
                    for (int k = 0; k < MAX_BOSS_BULLETS; k++)
                    {
                        if (bossBulletActive[k] && Vector2Distance(bossBulletPos[k], (Vector2){ domeCenterX, domeBaseY }) <= domeRadius && bossBulletPos[k].y <= domeBaseY)
                        {
                            bossBulletActive[k] = false; PlaySound(sndShieldDeflect);
                        }
                    }
                    for (int k = 0; k < MAX_BOSS_ORBS; k++)
                    {
                        if (bossOrbActive[k] && Vector2Distance(bossOrbPos[k], (Vector2){ domeCenterX, domeBaseY }) <= domeRadius && bossOrbPos[k].y <= domeBaseY)
                        {
                            bossOrbActive[k] = false; PlaySound(sndShieldDeflect);
                        }
                    }
                }
            }

            // game pause system...
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
                                (Rectangle){ AStyle*(AlienSWidth[AlienLooks[X][Y]-1]+1), 0, AlienSWidth[AlienLooks[X][Y]-1], (-1)*AlienSHeight[AlienLooks[X][Y]-1] },
                                Alien, (Vector2){ 0, 0 }, 0.0f, WHITE);
                        }
                    }
                }

                if (jammerActive)
                    DrawTexturePro(jammerTex[jammerAnimState], (Rectangle){ 0, 0, (float)jammerTex[jammerAnimState].width, (float)jammerTex[jammerAnimState].height },
                                   (Rectangle){ jammerPos.x, jammerPos.y, COMMANDER_SIZE, COMMANDER_SIZE }, (Vector2){ 0, 0 }, 0.0f, WHITE);
                if (warpActive)
                    DrawTexturePro(warpTex[warpAnimState], (Rectangle){ 0, 0, (float)warpTex[warpAnimState].width, (float)warpTex[warpAnimState].height },
                                   (Rectangle){ warpPos.x, warpPos.y, COMMANDER_SIZE, COMMANDER_SIZE }, (Vector2){ 0, 0 }, 0.0f, WHITE);

                for (int b = 0; b < MAX_COMMANDER_BULLETS; b++)
                {
                    if (jammerBulletActive[b]) DrawRectangle((int)(jammerBulletPos[b].x - 4), (int)jammerBulletPos[b].y, 8, 22, (Color){ 200, 70, 255, 255 });
                    if (warpBulletActive[b]) DrawRectangle((int)(warpBulletPos[b].x - 3), (int)warpBulletPos[b].y, 6, 26, SKYBLUE);
                }

                if (bossActive)
                {
                    DrawTexturePro(BossTexture[0], (Rectangle){ 0, 0, (float)BossTexture[0].width, (float)BossTexture[0].height },
                                   (Rectangle){ bossPos.x, bossPos.y, BossWidth, BossHeight }, (Vector2){ 0, 0 }, 0.0f, WHITE);
                    int AStyle = (int)(GetTime() / 0.1) % AlienSpriteStyle[19];
                    for (int m = 0; m < MAX_MINIONS; m++)
                    {
                        if (minionActive[m])
                            DrawTexturePro(AlienTexture[12], (Rectangle){ (float)(AStyle * (AlienSWidth[12] + 1)), 0.0f, (float)AlienSWidth[12], (float)AlienSHeight[12] },
                                           (Rectangle){ minionPos[m].x, minionPos[m].y, (float)MinionSize, (float)MinionSize }, (Vector2){ 0, 0 }, 0.0f, WHITE);
                    }
                }

                // Render Black Hole in Pause overlay if active
                if (blackHole.active)
                {
                    BeginBlendMode(BLEND_ADDITIVE);
                    if (texBlackHoleDisk.id > 0)
                    {
                        DrawTexturePro(texBlackHoleDisk, (Rectangle){ 0, 0, (float)texBlackHoleDisk.width, (float)texBlackHoleDisk.height },
                                       (Rectangle){ blackHole.pos.x, blackHole.pos.y, 440.0f * blackHole.scale, 440.0f * blackHole.scale },
                                       (Vector2){ 220.0f * blackHole.scale, 220.0f * blackHole.scale }, blackHole.rotationAngle, Fade(WHITE, 0.85f * blackHole.alpha));
                        DrawTexturePro(texBlackHoleDisk, (Rectangle){ 0, 0, (float)texBlackHoleDisk.width, (float)texBlackHoleDisk.height },
                                       (Rectangle){ blackHole.pos.x, blackHole.pos.y, 360.0f * blackHole.scale, 360.0f * blackHole.scale },
                                       (Vector2){ 180.0f * blackHole.scale, 180.0f * blackHole.scale }, -blackHole.rotationAngle * 1.4f, Fade(PURPLE, 0.70f * blackHole.alpha));
                    }
                    EndBlendMode();
                    if (texBlackHoleCore.id > 0)
                    {
                        DrawTexturePro(texBlackHoleCore, (Rectangle){ 0, 0, (float)texBlackHoleCore.width, (float)texBlackHoleCore.height },
                                       (Rectangle){ blackHole.pos.x, blackHole.pos.y, 140.0f * blackHole.scale, 140.0f * blackHole.scale },
                                       (Vector2){ 70.0f * blackHole.scale, 70.0f * blackHole.scale }, 0.0f, Fade(WHITE, blackHole.alpha));
                    }
                }

                // Render Void Phantom in Pause overlay if active
                if (voidEntity.active)
                {
                    DrawTexturePro(texEntityPhantom[voidEntity.currentFrame],
                                   (Rectangle){ 0, 0, (float)texEntityPhantom[voidEntity.currentFrame].width, (float)texEntityPhantom[voidEntity.currentFrame].height },
                                   (Rectangle){ voidEntity.pos.x, voidEntity.pos.y, 120.0f, 120.0f },
                                   (Vector2){ 60.0f, 60.0f }, 0.0f, Fade(WHITE, voidEntity.alpha));
                }

                for (int i = 0; i < 2; i++)
                {
                    if (Hero1BulletActive[i]) DrawRectangle((int)(Hero1BulletPos[i].x - BulletWidth / 2.0f), (int)Hero1BulletPos[i].y, BulletWidth, BulletHeight, YELLOW);
                    if (Hero2BulletActive[i]) DrawRectangle((int)(Hero2BulletPos[i].x - BulletWidth / 2.0f), (int)Hero2BulletPos[i].y, BulletWidth, BulletHeight, SKYBLUE);
                }
                
                Rectangle Hero1SpecialBulletRec = { Hero1SpecialBulletPos.x - HeroWidth / 2.0f, Hero1SpecialBulletPos.y, HeroWidth, HeroWidth * 3 };
                if (Hero1SpecialBulletActive) DrawTexturePro(Hero1SpecialBulletTex[0], (Rectangle){ 0, 0, (float)Hero1SpecialBulletTex[0].width, (float)Hero1SpecialBulletTex[0].height }, Hero1SpecialBulletRec, (Vector2){ 0, 0 }, 0.0f, WHITE);

                for (int m = 0; m < MAX_CLUSTER_MISSILES; m++)
                {
                    if (clusterMissileActive[m])
                    {
                        float rotAngle = atan2f(clusterMissileVel[m].y, clusterMissileVel[m].x) * RAD2DEG + 90.0f;
                        if (clusterBombTex.id > 0)
                            DrawTexturePro(clusterBombTex, (Rectangle){ 0.0f, 0.0f, (float)clusterBombTex.width, (float)clusterBombTex.height },
                                           (Rectangle){ clusterMissilePos[m].x, clusterMissilePos[m].y, 14.0f, 28.0f }, (Vector2){ 7.0f, 14.0f }, rotAngle, WHITE);
                        else
                            DrawRectangle((int)clusterMissilePos[m].x - 3, (int)clusterMissilePos[m].y - 7, 6, 14, SKYBLUE);
                    }
                }

                if (AlienBulletActive) DrawRectangle((int)(AlienBulletPos.x - AlienBulletWidth / 2.0f), (int)AlienBulletPos.y, AlienBulletWidth, AlienBulletHeight, RED);
                for (int mb = 0; mb < MAX_MINION_BULLETS; mb++)
                    if (minionBulletActive[mb]) DrawRectangle((int)(minionBulletPos[mb].x - AlienBulletWidth / 2.0f), (int)minionBulletPos[mb].y, AlienBulletWidth, AlienBulletHeight, ORANGE);

                if (Hero1Lives > 0)
                    DrawTexturePro(HeroTexture, (Rectangle){ 0, 0, (float)HeroTexture.width, (float)HeroTexture.height },
                                   (Rectangle){ Hero1Pos.x - HeroWidth / 2.0f, Hero1Pos.y, HeroWidth, HeroHeight }, (Vector2){ 0, 0 }, 0.0f, WHITE);
                if (Hero2Lives > 0)
                {
                    DrawStealthFlames((Vector2){ Hero2Pos.x, Hero2Pos.y + HeroHeight / 2.0f }, HeroWidth, HeroHeight);
                    DrawTexturePro(StealthHeroTexture, (Rectangle){ 0, 0, (float)StealthHeroTexture.width, (float)StealthHeroTexture.height },
                                   (Rectangle){ Hero2Pos.x - HeroWidth / 2.0f, Hero2Pos.y, HeroWidth, HeroHeight }, (Vector2){ 0, 0 }, 0.0f, WHITE);
                }

                if (teamShieldActive)
                {
                    if (teamShieldDomeTex.id > 0)
                        DrawTexturePro(teamShieldDomeTex, (Rectangle){ 0, 0, (float)teamShieldDomeTex.width, (float)teamShieldDomeTex.height },
                                       (Rectangle){ domeCenterX, domeBaseY, domeRadius * 2.0f, domeRadius }, (Vector2){ domeRadius, domeRadius }, 0.0f, Fade(WHITE, 0.85f));
                    else
                        DrawCircleSector((Vector2){ domeCenterX, domeBaseY }, domeRadius, 180.0f, 360.0f, 36, Fade(SKYBLUE, 0.35f));
                }

                DrawRectangle(0, 0, WindowWidth, WindowHeight, Fade(BLACK, 0.6f));
                int pBoxW = 450, pBoxH = 180, pBoxX = (WindowWidth - pBoxW) / 2, pBoxY = (WindowHeight - pBoxH) / 2;
                DrawRectangle(pBoxX, pBoxY, pBoxW, pBoxH, Fade(DARKBLUE, 0.90f));
                DrawRectangleLines(pBoxX, pBoxY, pBoxW, pBoxH, SKYBLUE);
                char pauseTitle[] = "GAME PAUSED";
                DrawText(pauseTitle, pBoxX + (pBoxW - MeasureText(pauseTitle, 36)) / 2, pBoxY + 35, 36, YELLOW);
                char pauseSub[] = "Press [P] to Resume   |   [M] for Menu";
                DrawText(pauseSub, pBoxX + (pBoxW - MeasureText(pauseSub, 18)) / 2, pBoxY + 110, 18, RAYWHITE);

                if (IsKeyPressed(KEY_M))
                {
                    PlaySound(menuSelect); isPaused = false;
                    StopMusicStream(bgmBoss);
                    if (gameplayBgmActive) { StopMusicStream(bgmGameplay[BgmTrackIndex]); gameplayBgmActive = false; }
                    StopSound(sndBlackHoleDrone); StopSound(sndBlackHolePull); StopSound(sndBlackHoleCrush);
                    StopSound(sndEntityDrone); StopSound(sndEntitySpawn); StopSound(sndEntityAttack); StopSound(sndEntityScream);
                    CurrentState = STATE_MENU; PlayMusicStream(bgmMenu);
                }
                EndMode2D();
                EndTextureMode();

                BeginDrawing();
                ClearBackground(BLACK);
                float curScale = fminf((float)GetScreenWidth() / WindowWidth, (float)GetScreenHeight() / WindowHeight);
                Rectangle srcRec = { 0.0f, 0.0f, (float)renderTarget.texture.width, -(float)renderTarget.texture.height };
                Rectangle destRec = {
                    ((float)GetScreenWidth() - ((float)WindowWidth * curScale)) * 0.5f,
                    ((float)GetScreenHeight() - ((float)WindowHeight * curScale)) * 0.5f, //bug fix for graphics failure!!!!!!
                    (float)WindowWidth * curScale,
                    (float)WindowHeight * curScale
                };
                DrawTexturePro(renderTarget.texture, srcRec, destRec, (Vector2){ 0, 0 }, 0.0f, WHITE);
                EndDrawing();
                continue;
            }

            if (hero1HitFlashTimer > 0.0f) hero1HitFlashTimer -= Time;
            if (hero2HitFlashTimer > 0.0f) hero2HitFlashTimer -= Time;
            if (radarJammedTimer > 0.0f)   radarJammedTimer -= Time;
            if (bossHitFlashTimer > 0.0f)  bossHitFlashTimer -= Time;
            if (shoabEdgeFlashTimer > 0.0f) shoabEdgeFlashTimer -= rawTime;
            if (nayemulEdgeFlashTimer > 0.0f) nayemulEdgeFlashTimer -= rawTime;

            if (!isPaused && Hero1Lives > 0 && !isFrozen && !deathSequenceActive && !GameOver && !bossDefeated) ShoabSpecialTime += 1.0f;
            if (!isPaused && Hero2Lives > 0 && !isFrozen && !deathSequenceActive && !GameOver && !bossDefeated) NayemulSpecialTime += 1.0f;

            int totalAliens = AlienInX * AlienInY;
            if (!commandersTriggered && !bossSpawned && AliensKilled >= (int)(totalAliens * 0.65f))
            {
                commandersTriggered  = true; commanderIntroActive = true; commanderIntroTimer = 3.0f;
                PlaySound(sndEmpBlast);
            }

            if (commanderIntroActive)
            {
                commanderIntroTimer -= rawTime;
                if (commanderIntroTimer <= 0.0f)
                {
                    commanderIntroActive = false; gameTimeDilation = 1.0f;
                    jammerPos = (Vector2){ WindowWidth * 0.25f - COMMANDER_SIZE / 2.0f, 130.0f };
                    jammerSpeed = (Vector2){ 120.0f, 0.0f }; jammerHp = jammerMaxHp; jammerActive = true;
                    jammerShootTimer = 3.0f; jammerActionCooldown = 5.0f; jammerPowerTimer = 0.0f; jammerAnimState = 0;

                    warpPos = (Vector2){ WindowWidth * 0.75f - COMMANDER_SIZE / 2.0f, 130.0f };
                    warpSpeed = (Vector2){ 140.0f, 0.0f }; warpHp = warpMaxHp; warpActive = true;
                    warpShootTimer = 2.0f; warpActionCooldown = 4.0f; warpPowerTimer = 0.0f; warpAnimState = 0;
                    PlaySound(sndTeleport);
                }
            }

            // COMMANDER 1: jammer (Shoots every 3.0s, smart tracks hero until 1/4th height)
            if (jammerActive && !isFrozen && !deathSequenceActive && !GameOver && !bossDefeated)
            {
                jammerPos.x += jammerSpeed.x * Time;
                if (jammerPos.x <= 40) { jammerPos.x = 40; jammerSpeed.x = fabsf(jammerSpeed.x); }
                else if (jammerPos.x >= WindowWidth - COMMANDER_SIZE - 40) { jammerPos.x = WindowWidth - COMMANDER_SIZE - 40; jammerSpeed.x = -fabsf(jammerSpeed.x); }

                jammerShootTimer -= Time;
                if (jammerShootTimer <= 0.0f)
                {
                    jammerShootTimer = 3.0f; // Exactly every 3 seconds!
                    for (int b = 0; b < MAX_COMMANDER_BULLETS; b++)
                    {
                        if (!jammerBulletActive[b])
                        {
                            jammerBulletActive[b] = true;
                            jammerBulletLocked[b] = false;
                            jammerBulletPos[b] = (Vector2){ jammerPos.x + COMMANDER_SIZE / 2.0f, jammerPos.y + COMMANDER_SIZE };

                            // Determine target hero for initial tracking trajectory
                            Vector2 targetHeroPos = (Hero1Lives > 0 && (GetRandomValue(0, 1) == 0 || Hero2Lives <= 0)) ? Hero1Pos : Hero2Pos;
                            Vector2 shootDir = Vector2Normalize(Vector2Subtract(targetHeroPos, jammerBulletPos[b]));
                            if (shootDir.y < 0.2f) shootDir.y = 0.2f; // Ensure downward propulsion
                            shootDir = Vector2Normalize(shootDir);
                            jammerBulletVel[b] = Vector2Scale(shootDir, 420.0f);

                            PlaySound(sndJammerShot); 
                            break;
                        }
                    }
                }
                jammerActionCooldown -= Time;
                if (jammerActionCooldown <= 0.0f)
                {
                    jammerActionCooldown = 8.5f; jammerPowerTimer = 1.4f; radarJammedTimer = 5.0f;
                    spaceLightningTimer = 0.08f;
                    PlaySound(sndEmpBlast); PlaySound(sndJammerHum);
                }
                if (jammerPowerTimer > 0.0f) { jammerPowerTimer -= Time; jammerAnimState = 3; }
                else jammerAnimState = (fabsf(jammerSpeed.x) > 0.0f) ? 2 : 0;
            }

            // COMMANDER 2: warp (Frozen and silenced when heroes die or game is won) -bugs fixed !!!!
            if (warpActive && !isFrozen && !deathSequenceActive && !GameOver && !bossDefeated)
            {
                warpPos.x += warpSpeed.x * Time;
                if (warpPos.x <= 40) { warpPos.x = 40; warpSpeed.x = fabsf(warpSpeed.x); }
                else if (warpPos.x >= WindowWidth - COMMANDER_SIZE - 40) { warpPos.x = WindowWidth - COMMANDER_SIZE - 40; warpSpeed.x = -fabsf(warpSpeed.x); }

                warpShootTimer -= Time;
                if (warpShootTimer <= 0.0f)
                {
                    warpShootTimer = 1.4f; int spawned = 0;
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

                bool threatened = false;
                for (int h = 0; h < 2; h++)
                {
                    Vector2 *bPos = (h == 0) ? Hero1BulletPos : Hero2BulletPos;
                    bool *bActive = (h == 0) ? Hero1BulletActive : Hero2BulletActive;
                    for (int i = 0; i < 2; i++)
                    {
                        if (bActive[i] && fabsf(bPos[i].x - (warpPos.x + COMMANDER_SIZE / 2.0f)) < 36.0f && bPos[i].y < warpPos.y + 220.0f && bPos[i].y > warpPos.y)
                        {
                            threatened = true; break;
                        }
                    }
                    if (threatened) break;
                }

                warpActionCooldown -= Time;
                if ((warpActionCooldown <= 0.0f) || (threatened && warpActionCooldown < 3.2f))
                {
                    warpActionCooldown = 5.0f; warpPowerTimer = 0.65f;
                    PlaySound(sndTeleport);
                    warpPos = (Vector2){ (float)GetRandomValue(60, WindowWidth - COMMANDER_SIZE - 60), (float)GetRandomValue(90, 200) };
                }
                if (warpPowerTimer > 0.0f) { warpPowerTimer -= Time; warpAnimState = 3; }
                else warpAnimState = (fabsf(warpSpeed.x) > 0.0f) ? 2 : 0;
            }

            // commander laser beam and priority check (Jammer tracking until 1/4th height: Y = 225px)
            for (int b = 0; b < MAX_COMMANDER_BULLETS; b++)
            {
                if (jammerBulletActive[b] && !isFrozen && !deathSequenceActive && !GameOver && !bossDefeated)
                {
                    float trackingThresholdY = WindowHeight * 0.25f; // 225px

                    if (jammerBulletPos[b].y < trackingThresholdY && !jammerBulletLocked[b])
                    {
                        // Actively steering and tracking toward one of the living heroes
                        Vector2 targetHero = (Hero1Lives > 0 && (b % 2 == 0 || Hero2Lives <= 0)) ? Hero1Pos : Hero2Pos;
                        Vector2 desiredDir = Vector2Normalize(Vector2Subtract(targetHero, jammerBulletPos[b]));
                        if (desiredDir.y < 0.25f) desiredDir.y = 0.25f;
                        desiredDir = Vector2Normalize(desiredDir);
                        jammerBulletVel[b] = Vector2Scale(desiredDir, 420.0f);
                    }
                    else
                    {
                        // Crosses 1/4th height: locked straight on its existing trajectory!
                        jammerBulletLocked[b] = true;
                    }

                    jammerBulletPos[b] = Vector2Add(jammerBulletPos[b], Vector2Scale(jammerBulletVel[b], Time));

                    if (jammerBulletPos[b].y > WindowHeight || jammerBulletPos[b].x < -30 || jammerBulletPos[b].x > WindowWidth + 30)
                    {
                        jammerBulletActive[b] = false;
                    }
                    else if (!teamShieldActive && !evacActive)
                    {
                        Rectangle jbRec = { jammerBulletPos[b].x - 4, jammerBulletPos[b].y, 8, 22 };
                        Rectangle h1Rec = { Hero1Pos.x - HeroWidth / 2.0f, Hero1Pos.y, HeroWidth, HeroHeight };
                        Rectangle h2Rec = { Hero2Pos.x - HeroWidth / 2.0f, Hero2Pos.y, HeroWidth, HeroHeight };

                        if (Hero1Lives > 0 && CheckCollisionRecs(jbRec, h1Rec))
                        {
                            jammerBulletActive[b] = false; Hero1Lives--; Hero1HitsTaken++;
                            hero1HitFlashTimer = 0.35f; PlaySound(heroOuch); PlaySound(sndHudGlitch);
                            if (Hero1Lives <= 0)
                            {
                                Hero1CrashPos = Hero1Pos; PlaySound(heroDeath);
                                if (Hero2Lives > 0) { hero2OverdriveTimer = 2.5f; PlaySound(sndShieldActivate); }
                                if (Hero2Lives <= 0)
                                {
                                    deathSequenceActive = true; deathDelayTimer = 0.0f;
                                    StopSound(AlienShoot); StopSound(sndAlienStep);
                                }
                            }
                        }
                        else if (Hero2Lives > 0 && CheckCollisionRecs(jbRec, h2Rec))
                        {
                            jammerBulletActive[b] = false; Hero2Lives--; Hero2HitsTaken++;
                            hero2HitFlashTimer = 0.35f; PlaySound(heroOuch); PlaySound(sndHudGlitch);
                            if (Hero2Lives <= 0)
                            {
                                Hero2CrashPos = Hero2Pos; PlaySound(heroDeath);
                                if (Hero1Lives > 0) { hero1OverdriveTimer = 2.5f; PlaySound(sndShieldActivate); }
                                if (Hero1Lives <= 0)
                                {
                                    deathSequenceActive = true; deathDelayTimer = 0.0f;
                                    StopSound(AlienShoot); StopSound(sndAlienStep);
                                }
                            }
                        }
                    }
                }
                if (warpBulletActive[b] && !isFrozen && !deathSequenceActive && !GameOver && !bossDefeated)
                {
                    warpBulletPos[b].y += 500.0f * Time;
                    if (warpBulletPos[b].y > WindowHeight) warpBulletActive[b] = false;
                    else if (!teamShieldActive && !evacActive)
                    {
                        Rectangle wbRec = { warpBulletPos[b].x - 3, warpBulletPos[b].y, 6, 26 };
                        Rectangle h1Rec = { Hero1Pos.x - HeroWidth / 2.0f, Hero1Pos.y, HeroWidth, HeroHeight };
                        Rectangle h2Rec = { Hero2Pos.x - HeroWidth / 2.0f, Hero2Pos.y, HeroWidth, HeroHeight };

                        if (Hero1Lives > 0 && CheckCollisionRecs(wbRec, h1Rec))
                        {
                            warpBulletActive[b] = false; Hero1Lives--; Hero1HitsTaken++;
                            hero1HitFlashTimer = 0.35f; PlaySound(heroOuch); PlaySound(sndHudGlitch);
                            if (Hero1Lives <= 0)
                            {
                                Hero1CrashPos = Hero1Pos; PlaySound(heroDeath);
                                if (Hero2Lives > 0) { hero2OverdriveTimer = 2.5f; PlaySound(sndShieldActivate); }
                                if (Hero2Lives <= 0)
                                {
                                    deathSequenceActive = true; deathDelayTimer = 0.0f;
                                    StopSound(AlienShoot); StopSound(sndAlienStep);
                                }
                            }
                        }
                        else if (Hero2Lives > 0 && CheckCollisionRecs(wbRec, h2Rec))
                        {
                            warpBulletActive[b] = false; Hero2Lives--; Hero2HitsTaken++;
                            hero2HitFlashTimer = 0.35f; PlaySound(heroOuch); PlaySound(sndHudGlitch);
                            if (Hero2Lives <= 0)
                            {
                                Hero2CrashPos = Hero2Pos; PlaySound(heroDeath);
                                if (Hero1Lives > 0) { hero1OverdriveTimer = 2.5f; PlaySound(sndShieldActivate); }
                                if (Hero1Lives <= 0)
                                {
                                    deathSequenceActive = true; deathDelayTimer = 0.0f;
                                    StopSound(AlienShoot); StopSound(sndAlienStep);
                                }
                            }
                        }
                    }
                }
            }

            // Dead - stops everything and Game Over video
            if (deathSequenceActive && !evacActive && !bossDefeated)
            {
                deathDelayTimer += Time;
                if (deathDelayTimer >= 1.2f)
                {
                    deathSequenceActive = false;
                    bossLaserActive = false;
                    screenCamera.offset = (Vector2){ 0, 0 };
                    StopMusicStream(bgmBoss);
                    if (gameplayBgmActive) { StopMusicStream(bgmGameplay[BgmTrackIndex]); gameplayBgmActive = false; }
                    StopSound(bossLaserSound);
                    StopSound(sndDefibHum);
                    StopSound(sndWarningSiren);
                    StopSound(AlienShoot);
                    StopSound(sndAlienStep);
                    StopSound(sndBlackHoleDrone);
                    StopSound(sndEntityDrone);

                    CurrentState = STATE_VIDEO_PLAY;
                    StartVideo(1, 10.5f);
                }
            }

            // Unified Menu Return Key [M] (esc wasn't working for macOS but for windows!!!)
            if (IsKeyPressed(KEY_M) && !deathSequenceActive && !GameOver && !bossDefeated)
            {
                PlaySound(menuSelect); StopMusicStream(bgmBoss);
                if (gameplayBgmActive) { StopMusicStream(bgmGameplay[BgmTrackIndex]); gameplayBgmActive = false; }
                StopSound(bossLaserSound); StopSound(sndDefibHum); StopSound(sndWarningSiren);
                StopSound(AlienShoot); StopSound(sndAlienStep);
                StopSound(sndBlackHoleDrone); StopSound(sndBlackHolePull); StopSound(sndBlackHoleCrush);
                StopSound(sndEntityDrone); StopSound(sndEntitySpawn); StopSound(sndEntityAttack); StopSound(sndEntityScream);
                CurrentState = STATE_MENU; PlayMusicStream(bgmMenu);
            }

            // Revival logic and defibrillator sound handling
            if (Hero1Lives <= 0 && Hero2Lives > 1 && !GameOver && !bossDefeated)
            {
                if (Vector2Distance(Hero2Pos, Hero1CrashPos) < 85.0f)
                {
                    if (!IsSoundPlaying(sndDefibHum)) PlaySound(sndDefibHum);
                    hero1ReviveTimer += Time;
                    if (hero1ReviveTimer >= 1.5f)
                    {
                        StopSound(sndDefibHum); Hero2Lives--; Hero1Lives = 1; hero1ReviveTimer = 0.0f;
                        Hero1Pos = Hero1CrashPos; PlaySound(cheer);
                    }
                }
                else { if (hero1ReviveTimer > 0.0f) StopSound(sndDefibHum); hero1ReviveTimer = 0.0f; }
            }
            else { if (hero1ReviveTimer > 0.0f) StopSound(sndDefibHum); hero1ReviveTimer = 0.0f; }

            if (Hero2Lives <= 0 && Hero1Lives > 1 && !GameOver && !bossDefeated)
            {
                if (Vector2Distance(Hero1Pos, Hero2CrashPos) < 85.0f)
                {
                    if (!IsSoundPlaying(sndDefibHum)) PlaySound(sndDefibHum);
                    hero2ReviveTimer += Time;
                    if (hero2ReviveTimer >= 2.0f)
                    {
                        StopSound(sndDefibHum); Hero1Lives--; Hero2Lives = 1; hero2ReviveTimer = 0.0f;
                        Hero2Pos = Hero2CrashPos; PlaySound(cheer);
                    }
                }
                else { if (hero2ReviveTimer > 0.0f) StopSound(sndDefibHum); hero2ReviveTimer = 0.0f; }
            }
            else { if (hero2ReviveTimer > 0.0f) StopSound(sndDefibHum); hero2ReviveTimer = 0.0f; }

            // DRAW REVIVAL ROUND COLORED VISUALS (Defibrillator Energy Dome & Pulse Beacons)
            if (Hero1Lives <= 0 && !GameOver && !bossDefeated && Hero1CrashPos.x != 0)
            {
                float beaconPulse = fabsf(sinf((float)GetTime() * 6.0f));
                DrawCircleLines((int)Hero1CrashPos.x, (int)Hero1CrashPos.y + HeroHeight / 2.0f, 40.0f + beaconPulse * 12.0f, Fade(LIME, 0.65f));
                DrawCircle((int)Hero1CrashPos.x, (int)Hero1CrashPos.y + HeroHeight / 2.0f, 16.0f, Fade(LIME, 0.35f + beaconPulse * 0.35f));
                DrawText("CRASH BEACON", (int)Hero1CrashPos.x - MeasureText("CRASH BEACON", 13) / 2, (int)Hero1CrashPos.y - 12, 13, LIME);

                if (hero1ReviveTimer > 0.0f)
                {
                    float reviveRatio = hero1ReviveTimer / 1.5f;
                    if (reviveRatio > 1.0f) reviveRatio = 1.0f;
                    Vector2 defibCenter = { Hero1CrashPos.x, Hero1CrashPos.y + HeroHeight / 2.0f };

                    BeginBlendMode(BLEND_ADDITIVE);
                    DrawCircleSector(defibCenter, 72.0f, 0.0f, reviveRatio * 360.0f, 36, Fade(LIME, 0.40f));
                    DrawCircleLines((int)defibCenter.x, (int)defibCenter.y, 72.0f, LIME);
                    DrawRing(defibCenter, 66.0f, 75.0f, 0.0f, reviveRatio * 360.0f, 36, YELLOW);
                    EndBlendMode();
                }
            }

            if (Hero2Lives <= 0 && !GameOver && !bossDefeated && Hero2CrashPos.x != 0)
            {
                float beaconPulse = fabsf(sinf((float)GetTime() * 6.0f));
                DrawCircleLines((int)Hero2CrashPos.x, (int)Hero2CrashPos.y + HeroHeight / 2.0f, 40.0f + beaconPulse * 12.0f, Fade(YELLOW, 0.65f));
                DrawCircle((int)Hero2CrashPos.x, (int)Hero2CrashPos.y + HeroHeight / 2.0f, 16.0f, Fade(YELLOW, 0.35f + beaconPulse * 0.35f));
                DrawText("CRASH BEACON", (int)Hero2CrashPos.x - MeasureText("CRASH BEACON", 13) / 2, (int)Hero2CrashPos.y - 12, 13, YELLOW);

                if (hero2ReviveTimer > 0.0f)
                {
                    float reviveRatio = hero2ReviveTimer / 2.0f;
                    if (reviveRatio > 1.0f) reviveRatio = 1.0f;
                    Vector2 defibCenter = { Hero2CrashPos.x, Hero2CrashPos.y + HeroHeight / 2.0f };

                    BeginBlendMode(BLEND_ADDITIVE);
                    DrawCircleSector(defibCenter, 72.0f, 0.0f, reviveRatio * 360.0f, 36, Fade(YELLOW, 0.40f));
                    DrawCircleLines((int)defibCenter.x, (int)defibCenter.y, 72.0f, YELLOW);
                    DrawRing(defibCenter, 66.0f, 75.0f, 0.0f, reviveRatio * 360.0f, 36, LIME);
                    EndBlendMode();
                }
            }

            // GAME LOST SCREEN (ALL MOVEMENT and AUDIO stopped EXCEPT BGM)-bug fixed...
            if (GameOver)
            {
                bossLaserActive = false; screenCamera.offset = (Vector2){ 0, 0 };
                StopMusicStream(bgmBoss);
                if (gameplayBgmActive) { StopMusicStream(bgmGameplay[BgmTrackIndex]); gameplayBgmActive = false; }
                StopSound(bossLaserSound); StopSound(sndDefibHum); StopSound(sndWarningSiren);
                StopSound(AlienShoot); StopSound(sndAlienStep);
                StopSound(sndBlackHoleDrone);
                StopSound(sndEntityDrone);
                if (!lostBgmStarted) { PlayMusicStream(bgmLost); lostBgmStarted = true; }
                UpdateMusicStream(bgmLost);

                if (!rankAudioPlayed)
                {
                    PlaySound(sndRankBadge);
                    rankAudioPlayed = true;
                }

                int panelW = 1260, panelH = 580, panelX = (WindowWidth - panelW) / 2, panelY = (WindowHeight - panelH) / 2;

                if (lostDialogueIndex == 0)
                {
                    DrawAlienGlitchBox((Rectangle){ (float)panelX, (float)panelY, (float)panelW, (float)panelH });

                    int textY = panelY + 115;
                    DrawText("Alien Dreadnought Overlord (Exospheric Invasion Flagship):", panelX + 50, textY, 26, MAROON);
                    DrawText("\"HA HA HA! Look at your interceptors burning in the void!\"", panelX + 50, textY + 60, 24, RED);
                    DrawText(TextFormat("\"Your greatest pilots - %s and %s - have been completely crushed!\"", inputHero2Name, inputHero1Name), panelX + 50, textY + 110, 23, RED);
                    DrawText("\"Your frontline defenses are eradicated. Earth is completely defenseless!\"", panelX + 50, textY + 160, 23, ORANGE);
                    DrawText("\"Bow down and surrender now. Your entire world belongs to us!\"", panelX + 50, textY + 210, 24, RED);
                }
                else if (lostDialogueIndex == 1)
                {
                    DrawBreakingNewsBox((Rectangle){ (float)panelX, (float)panelY, (float)panelW, (float)panelH },
                                        "GLOBAL EMERGENCY BROADCAST: EARTH DEFENSE HAS COLLAPSED!",
                                        "DEFENSE COMMAND // PLANETARY OVERRUN", false);

                    int textY = panelY + 125;
                    DrawText("GLOBAL RADAR TELEMETRY CONFIRMS ALL ORBITAL DEFENSES OFFLINE.", panelX + 45, textY, 22, RED);
                    DrawText("Alien armada forces are currently descending upon major metropolitan centers.", panelX + 45, textY + 45, 22, RAYWHITE);
                    DrawText(TextFormat("Intercept command confirms: %s and %s have fallen in battle.", inputHero1Name, inputHero2Name), panelX + 45, textY + 90, 22, LIGHTGRAY);
                    DrawText("All remaining citizens are advised to seek subterranean shelter immediately.", panelX + 45, textY + 135, 21, LIGHTGRAY);
                    DrawText("May humanity find hope in this darkest hour...", panelX + 45, textY + 180, 23, YELLOW);
                }
                else
                {
                    DrawCyberBox((Rectangle){ (float)panelX, (float)panelY, (float)panelW, (float)panelH },
                                 RED, Fade((Color){ 20, 8, 10, 255 }, 0.94f),
                                 "EARTH DEFENSE DEBRIEFING // CASUALTY REPORT", MAROON);

                    char titleText[] = "GAME OVER - EARTH HAS FALLEN";
                    DrawText(titleText, (WindowWidth - MeasureText(titleText, 40)) / 2, panelY + 35, 40, RED);
                    DrawLine(panelX + 80, panelY + 80, panelX + panelW - 80, panelY + 80, RED);
                    
                    int mins = (int)runPlayTime / 60;
                    int secs = (int)runPlayTime % 60;
                    DrawText(TextFormat("COMBAT ENGAGEMENT TIME: %02d:%02d", mins, secs), (WindowWidth - MeasureText(TextFormat("COMBAT ENGAGEMENT TIME: %02d:%02d", mins, secs), 20)) / 2, panelY + 95, 20, LIGHTGRAY);

                    DrawText(TextFormat("Shoab's Interceptor [%s]: %05d", inputHero1Name, Hero1Score), panelX + 160, panelY + 140, 22, LIME);
                    DrawText(TextFormat("Nayemul's Stealth [%s]: %05d", inputHero2Name, Hero2Score), panelX + 680, panelY + 140, 22, YELLOW);

                    float bossDmgRatio = (float)(Hero1BossDamage + Hero2BossDamage) / 250.0f;
                    Texture2D lossBadge = (bossDmgRatio >= 0.45f) ? texRankBadgeB : texRankBadgeC;
                    const char* lossRankTxt = (bossDmgRatio >= 0.45f) ? "RANK: B [VETERAN VALOR]" : "RANK: C [SURVIVOR]";
                    
                    int bW = 240, bH = 80;
                    DrawTexturePro(lossBadge, (Rectangle){ 0, 0, (float)lossBadge.width, (float)lossBadge.height },
                                   (Rectangle){ (WindowWidth - bW) / 2.0f, (float)(panelY + 185), (float)bW, (float)bH }, (Vector2){0,0}, 0.0f, WHITE);
                    DrawText(lossRankTxt, (WindowWidth - MeasureText(lossRankTxt, 20)) / 2, panelY + 280, 20, (bossDmgRatio >= 0.45f) ? LIME : ORANGE);

                    char restartText[] = "Press [R] to Restart Defense   |   [M] Main Menu";
                    DrawText(restartText, (WindowWidth - MeasureText(restartText, 22)) / 2, panelY + 510, 22, GOLD);
                }

                if (lostDialogueIndex < 2)
                {
                    char promptText[] = "PRESS [ENTER] OR [SPACE] TO ADVANCE TRANSMISSION & SAVE LOGS";
                    if (((int)(GetTime() * 3)) % 2 == 0)
                        DrawText(promptText, (WindowWidth - MeasureText(promptText, 18)) / 2, panelY + panelH - 45, 18, RED);
                    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE))
                    {
                        PlaySound(menuSelect);
                        lostDialogueIndex++;
                        if (lostDialogueIndex == 2 && !currentMatchSaved)
                        {
                            MatchRecord rec = {
                                .isVictory = false,
                                .runTime = 0.0f,
                                .hero1Score = Hero1Score,
                                .hero2Score = Hero2Score,
                                .hero1Kills = Hero1Kills,
                                .hero2Kills = Hero2Kills,
                                .hero1BossDamage = Hero1BossDamage,
                                .hero2BossDamage = Hero2BossDamage,
                                .hero1HitsTaken = Hero1HitsTaken,
                                .hero2HitsTaken = Hero2HitsTaken
                            };
                            strncpy(rec.hero1PilotName, inputHero1Name, sizeof(rec.hero1PilotName) - 1);
                            strncpy(rec.hero2PilotName, inputHero2Name, sizeof(rec.hero2PilotName) - 1);
                            SaveMatchRecord(rec);

                            // Update Permanent Career Profiles
                            careerProfiles[0].totalMissions++;
                            careerProfiles[0].totalDefeats++;
                            careerProfiles[0].totalScore += Hero1Score;
                            careerProfiles[0].totalAlienKills += Hero1Kills;
                            careerProfiles[0].totalMinionKills += Hero1MinionKills;
                            careerProfiles[0].totalBossDamage += Hero1BossDamage;
                            careerProfiles[0].totalHitsTaken += Hero1HitsTaken;

                            careerProfiles[1].totalMissions++;
                            careerProfiles[1].totalDefeats++;
                            careerProfiles[1].totalScore += Hero2Score;
                            careerProfiles[1].totalAlienKills += Hero2Kills;
                            careerProfiles[1].totalMinionKills += Hero2MinionKills;
                            careerProfiles[1].totalBossDamage += Hero2BossDamage;
                            careerProfiles[1].totalHitsTaken += Hero2HitsTaken;

                            SaveHeroProfiles(careerProfiles);
                            currentMatchSaved = true;
                        }
                    }
                }

                if (IsKeyPressed(KEY_R))
                {
                    PlaySound(menuSelect); StopMusicStream(bgmLost); lostBgmStarted = false; lostDialogueIndex = 0;
                    if (gameplayBgmActive) { StopMusicStream(bgmGameplay[BgmTrackIndex]); gameplayBgmActive = false; }
                    Hero1Lives = 4; Hero2Lives = 4; Hero1Score = 0; Hero2Score = 0; AliensKilled = 0;
                    GameOver = false; deathSequenceActive = false; deathDelayTimer = 0.0f;
                    cheerPlayed = false; winSoundPlayed = false;
                    Hero1Kills = 0; Hero2Kills = 0; Hero1MinionKills = 0; Hero2MinionKills = 0;
                    Hero1BossDamage = 0; Hero2BossDamage = 0; Hero1HitsTaken = 0; Hero2HitsTaken = 0;
                    bossSpawned = false; bossWarningActive = false; bossWarningTimer = 0.0f; bossActive = false; bossDefeated = false;
                    bossWarpActive = false; bossWarpTimer = 0.0f; evacActive = false; evacTimer = 0.0f;
                    bossHp = 100; bossLeftPodHp = BossPodMaxHp; bossRightPodHp = BossPodMaxHp;
                    bossPos = (Vector2){ (WindowWidth - BossWidth) / 2.0f, 60.0f };
                    bossShootTimer = 0.0f; bossLaserTimer = 0.0f; bossLaserActive = false; bossLaserDuration = 0.0f; bossOrbTimer = 0.0f;
                    hero1Debuffed = false; hero1DebuffTimer = 0.0f; hero2Debuffed = false; hero2DebuffTimer = 0.0f;
                    empDropped = false; empActive = false; empBuffTimer = 0.0f; empShockwaveRadius = 0.0f;
                    hero1HitFlashTimer = 0.0f; hero2HitFlashTimer = 0.0f; SpecialReady = false; starttimer = false;
                    ShoabSpecialTime = 0.0f; Hero1SpecialBulletActive = false; NayemulSpecialReady = false; NayemulSpecialTime = 0.0f;
                    clusterBossDamageDealt = false;
                    shoabEdgeFlashTimer = 0.0f; nayemulEdgeFlashTimer = 0.0f; bossHitFlashTimer = 0.0f;
                    magRailSoundPlayed = false; laserChargeSoundPlayed = false; prevHeroesTethered = false;
                    hero1AsteroidSlowTimer = 0.0f; hero2AsteroidSlowTimer = 0.0f;
                    superPauseTimer = 0.0f; hitStopTimer = 0.0f; canopyGlintTimer = 0.0f; spaceLightningTimer = 0.0f;
                    hero1OverdriveTimer = 0.0f; hero2OverdriveTimer = 0.0f;
                    asteroidSpawnTimer = 0.0f; runPlayTime = 0.0f; rankAudioPlayed = false;

                    // Reset Black Hole & Void Entity
                    blackHole.active = false;
                    blackHole.spawned = false;
                    blackHole.collapsing = false;
                    blackHole.scale = 1.0f;
                    blackHole.alpha = 1.0f;
                    StopSound(sndBlackHoleDrone);

                    voidEntity.active = false;
                    voidEntity.spawned = false;
                    StopSound(sndEntityDrone);
                    for (int eo = 0; eo < MAX_ENTITY_ORBS; eo++) entityOrbs[eo].active = false;

                    currentMatchSaved = false;

                    for (int a = 0; a < MAX_ASTEROIDS; a++) asteroids[a].active = false;
                    for (int d = 0; d < MAX_ASTEROID_DEBRIS; d++) debrisPool[d].active = false;
                    for (int s = 0; s < MAX_SMOKE_PARTICLES; s++) smokePool[s].active = false;
                    for (int sp = 0; sp < MAX_SPARK_PARTICLES; sp++) sparkPool[sp].active = false;

                    for (int m = 0; m < MAX_CLUSTER_MISSILES; m++) clusterMissileActive[m] = false;
                    for (int b = 0; b < MAX_CLUSTER_BLASTS; b++) clusterBlastActive[b] = false;
                    explosionShakeTimer = 0.0f; teamShieldActive = false; teamShieldActiveTimer = 0.0f; teamShieldCooldownTimer = 0.0f; teamShieldCenterX = 0.0f;
                    commandersTriggered = false; commanderIntroActive = false; commanderIntroTimer = 0.0f; gameTimeDilation = 1.0f;
                    jammerActive = false; warpActive = false; jammerHp = 0; warpHp = 0; radarJammedTimer = 0.0f;
                    jammerPowerTimer = 0.0f; warpPowerTimer = 0.0f; bossEnraged = false;
                    for (int b = 0; b < MAX_COMMANDER_BULLETS; b++) { jammerBulletActive[b] = false; warpBulletActive[b] = false; jammerBulletLocked[b] = false; }
                    minionDeployTimer = 0.0f;
                    for (int m = 0; m < MAX_MINIONS; m++) minionActive[m] = false;
                    for (int mb = 0; mb < MAX_MINION_BULLETS; mb++) minionBulletActive[mb] = false;
                    for (int k = 0; k < MAX_BOSS_BULLETS; k++) bossBulletActive[k] = false;
                    for (int k = 0; k < MAX_BOSS_ORBS; k++) bossOrbActive[k] = false;
                    Hero1Pos = (Vector2){ WindowWidth * 0.35f, WindowHeight - HeroHeight };
                    Hero2Pos = (Vector2){ WindowWidth * 0.65f, WindowHeight - HeroHeight };
                    Hero1Speed = (Vector2){ 0, 0 }; Hero2Speed = (Vector2){ 0, 0 }; AlienSpeed = (Vector2){ AlienSpeedX, AlienSpeedY };
                    Hero1BulletActive[0] = false; Hero1BulletActive[1] = false;
                    Hero2BulletActive[0] = false; Hero2BulletActive[1] = false;
                    AlienBulletActive = false; AlienShootTimer = 0.0f;
                    AlienPos[0][0] = (Vector2){ 150, 50 };
                    for (int X = 0; X < AlienInX; X++)
                    {
                        for (int Y = 0; Y < AlienInY; Y++)
                        {
                            AlienPos[X][Y].x = AlienPos[0][0].x + (AlienSize + AlienDistance) * X;
                            AlienPos[X][Y].y = AlienPos[0][0].y + (AlienSize + AlienDistance) * Y;
                            AlienAlive[X][Y] = true; SpeedBuff[Y] = GetRandomValue(0, 20);
                        }
                    }
                }
                if (IsKeyPressed(KEY_M))
                {
                    PlaySound(menuSelect); StopMusicStream(bgmLost); lostBgmStarted = false; lostDialogueIndex = 0;
                    if (gameplayBgmActive) { StopMusicStream(bgmGameplay[BgmTrackIndex]); gameplayBgmActive = false; }
                    StopSound(sndBlackHoleDrone); StopSound(sndEntityDrone);
                    CurrentState = STATE_MENU; PlayMusicStream(bgmMenu);
                }
                EndMode2D();
                EndTextureMode();

                BeginDrawing();
                ClearBackground(BLACK);
                float curScale = fminf((float)GetScreenWidth() / WindowWidth, (float)GetScreenHeight() / WindowHeight);
                Rectangle srcRec = { 0.0f, 0.0f, (float)renderTarget.texture.width, -(float)renderTarget.texture.height };
                Rectangle destRec = {
                    ((float)GetScreenWidth() - ((float)WindowWidth * curScale)) * 0.5f,
                    ((float)GetScreenHeight() - ((float)WindowHeight * curScale)) * 0.5f,
                    (float)WindowWidth * curScale,
                    (float)WindowHeight * curScale
                };
                DrawTexturePro(renderTarget.texture, srcRec, destRec, (Vector2){ 0, 0 }, 0.0f, WHITE);
                EndDrawing();
                continue;
            }

            // GAME WON SCREEN (ALL MOVEMENT and AUDIO HALTED EXCEPT BGM)-bug fixed!!!!
            if (bossDefeated)
            {
                bossLaserActive = false; screenCamera.offset = (Vector2){ 0, 0 };
                StopMusicStream(bgmBoss);
                if (gameplayBgmActive) { StopMusicStream(bgmGameplay[BgmTrackIndex]); gameplayBgmActive = false; }
                StopSound(bossLaserSound); StopSound(sndDefibHum); StopSound(sndWarningSiren);
                StopSound(AlienShoot); StopSound(sndAlienStep);
                StopSound(sndBlackHoleDrone); StopSound(sndEntityDrone);
                if (!winSoundPlayed) { winSoundPlayed = true; }
                if (!winBgmStarted) { PlayMusicStream(bgmWin); winBgmStarted = true; }
                UpdateMusicStream(bgmWin);

                int panelW = 1260, panelH = 640, panelX = (WindowWidth - panelW) / 2, panelY = (WindowHeight - panelH) / 2;

                if (winDialogueIndex == 0)
                {
                    DrawCyberBox((Rectangle){ (float)panelX, (float)panelY, (float)panelW, (float)panelH },
                                 GOLD, Fade((Color){ 10, 20, 28, 255 }, 0.94f),
                                 "EARTH ALLIANCE COCKPIT COMMS // VICTORY CONFIRMED", LIME);

                    DrawLine(panelX + 50, panelY + 70, panelX + panelW - 50, panelY + 70, SKYBLUE);
                    DrawText(TextFormat("Shoab [%s]: \"We did it, %s! Look, the dreadnought is breaking apart!\"", inputHero1Name, inputHero2Name), panelX + 50, panelY + 110, 22, LIME);
                    DrawText(TextFormat("Nayemul [%s]: \"Target destroyed! Earth is safe! The sky is clear!\"", inputHero2Name), panelX + 50, panelY + 165, 22, YELLOW);
                    DrawText("Shoab: \"The alien fleet is retreating at full speed!\"", panelX + 50, panelY + 220, 22, LIME);
                    DrawText("Nayemul: \"Engaging thrusters. Let's head home, partner. We won!\"", panelX + 50, panelY + 275, 22, YELLOW);
                }
                else if (winDialogueIndex == 1)
                {
                    DrawBreakingNewsBox((Rectangle){ (float)panelX, (float)panelY, (float)panelW, (float)panelH },
                                        "GLOBAL NEWS ALERT: THE ALIEN INVASION IS DEFEATED!",
                                        "DHAKA - TOKYO - LONDON - NEW YORK", true);

                    int textY = panelY + 125;
                    DrawText("MASSIVE GLOBAL CELEBRATIONS HAVE ERUPTED ACROSS ALL CONTINENTS!", panelX + 45, textY, 22, LIME);
                    DrawText("Earth Defense Command confirms all exospheric sectors are completely secure.", panelX + 45, textY + 45, 22, RAYWHITE);
                    DrawText(TextFormat("Pilots %s and %s have successfully saved humanity from planetary extinction!", inputHero2Name, inputHero1Name), panelX + 45, textY + 90, 22, SKYBLUE);
                    DrawText("Tonight, fireworks illuminate the skyline across Dhaka and the entire planet!", panelX + 45, textY + 135, 22, YELLOW);
                    DrawText("All orbital defense satellites have resumed normal autonomous operations.", panelX + 45, textY + 180, 21, LIGHTGRAY);
                }
                else
                {
                    DrawCyberBox((Rectangle){ (float)panelX, (float)panelY, (float)panelW, (float)panelH },
                                 GREEN, Fade((Color){ 10, 22, 14, 255 }, 0.94f),
                                 "EARTH ALLIANCE DEBRIEFING // COMBAT CITATION", GOLD);

                    int combinedLives = (Hero1Lives > 0 ? Hero1Lives : 0) + (Hero2Lives > 0 ? Hero2Lives : 0);
                    int totalHits = Hero1HitsTaken + Hero2HitsTaken;
                    
                    Texture2D winBadge = texRankBadgeB;
                    const char* winRankTxt = "RANK: B [VETERAN DEFENDER]";
                    bool isRankS = false;

                    if (combinedLives >= 5 && totalHits <= 5 && runPlayTime <= 210.0f)
                    {
                        winBadge = texRankBadgeS;
                        winRankTxt = "RANK: S [EARTH'S SUPREME SAVIOR]";
                        isRankS = true;
                    }
                    else if (combinedLives >= 3 && totalHits <= 12)
                    {
                        winBadge = texRankBadgeA;
                        winRankTxt = "RANK: A [ELITE INTERCEPTOR]";
                    }

                    if (!rankAudioPlayed)
                    {
                        PlaySound(sndRankBadge);
                        if (isRankS) PlaySound(sndRankS);
                        rankAudioPlayed = true;
                    }

                    char titleText[] = "VICTORY! MISSION ACCOMPLISHED!";
                    DrawText(titleText, (WindowWidth - MeasureText(titleText, 38)) / 2, panelY + 25, 38, GREEN);
                    DrawLine(panelX + 80, panelY + 70, panelX + panelW - 80, panelY + 70, SKYBLUE);

                    int mins = (int)runPlayTime / 60;
                    int secs = (int)runPlayTime % 60;
                    DrawText(TextFormat("CLEAR TIME: %02d:%02d  |  LIVES PRESERVED: %d  |  HITS TAKEN: %d", mins, secs, combinedLives, totalHits),
                             (WindowWidth - MeasureText(TextFormat("CLEAR TIME: %02d:%02d  |  LIVES PRESERVED: %d  |  HITS TAKEN: %d", mins, secs, combinedLives, totalHits), 17)) / 2, panelY + 80, 17, RAYWHITE);

                    int bW = 240, bH = 80;
                    DrawTexturePro(winBadge, (Rectangle){ 0, 0, (float)winBadge.width, (float)winBadge.height },
                                   (Rectangle){ (WindowWidth - bW) / 2.0f, (float)(panelY + 110), (float)bW, (float)bH }, (Vector2){0,0}, 0.0f, WHITE);
                    DrawText(winRankTxt, (WindowWidth - MeasureText(winRankTxt, 21)) / 2, panelY + 198, 21, GOLD);

                    DrawText(TextFormat("Shoab's Interceptor [%s]: %05d", inputHero1Name, Hero1Score), panelX + 160, panelY + 225, 20, LIME);
                    DrawText(TextFormat("Nayemul's Stealth [%s]: %05d", inputHero2Name, Hero2Score), panelX + 740, panelY + 225, 20, YELLOW);

                    DrawText("[ COMBAT PERFORMANCE BADGES ]", (WindowWidth - MeasureText("[ COMBAT PERFORMANCE BADGES ]", 19)) / 2, panelY + 255, 19, GOLD);

                    int cardW = 340, cardH = 135, cardY = panelY + 285, card1X = panelX + 60;
                    DrawRectangle(card1X, cardY, cardW, cardH, Fade(DARKBLUE, 0.45f));
                    DrawRectangleLines(card1X, cardY, cardW, cardH, SKYBLUE);
                    DrawText("TOP GUN", card1X + 20, cardY + 15, 20, GOLD);
                    DrawText("Most Alien Kills", card1X + 20, cardY + 42, 15, LIGHTGRAY);
                    if (Hero1Kills > Hero2Kills) {
                        DrawText(TextFormat("WINNER: %s (%d)", inputHero1Name, Hero1Kills), card1X + 20, cardY + 75, 19, LIME);
                        DrawText(TextFormat("Runner Up: %s (%d)", inputHero2Name, Hero2Kills), card1X + 20, cardY + 102, 14, GRAY);
                    } else if (Hero2Kills > Hero1Kills) {
                        DrawText(TextFormat("WINNER: %s (%d)", inputHero2Name, Hero2Kills), card1X + 20, cardY + 75, 19, YELLOW);
                        DrawText(TextFormat("Runner Up: %s (%d)", inputHero1Name, Hero1Kills), card1X + 20, cardY + 102, 14, GRAY);
                    } else DrawText(TextFormat("TIED: BOTH (%d Kills)", Hero1Kills), card1X + 20, cardY + 85, 19, WHITE);

                    int card2X = panelX + 460;
                    DrawRectangle(card2X, cardY, cardW, cardH, Fade(DARKPURPLE, 0.45f));
                    DrawRectangleLines(card2X, cardY, cardW, cardH, RED);
                    DrawText("DREADNOUGHT BREAKER", card2X + 15, cardY + 15, 19, RED);
                    DrawText("Most Boss Damage Dealt", card2X + 20, cardY + 42, 15, LIGHTGRAY);
                    if (Hero1BossDamage > Hero2BossDamage) {
                        DrawText(TextFormat("WINNER: %s (%d HP)", inputHero1Name, Hero1BossDamage), card2X + 20, cardY + 75, 19, LIME);
                        DrawText(TextFormat("Runner Up: %s (%d HP)", inputHero2Name, Hero2BossDamage), card2X + 20, cardY + 102, 14, GRAY);
                    } else if (Hero2BossDamage > Hero1BossDamage) {
                        DrawText(TextFormat("WINNER: %s (%d HP)", inputHero2Name, Hero2BossDamage), card2X + 20, cardY + 75, 19, YELLOW);
                        DrawText(TextFormat("Runner Up: %s (%d HP)", inputHero1Name, Hero1BossDamage), card2X + 20, cardY + 102, 14, GRAY);
                    } else DrawText(TextFormat("TIED: BOTH (%d HP)", Hero1BossDamage), card2X + 20, cardY + 85, 19, WHITE);

                    int card3X = panelX + 860;
                    DrawRectangle(card3X, cardY, cardW, cardH, Fade(DARKGREEN, 0.45f));
                    DrawRectangleLines(card3X, cardY, cardW, cardH, LIME);
                    DrawText("UNTOUCHABLE ACE", card3X + 20, cardY + 15, 19, GREEN);
                    DrawText("Fewest Damage Hits Taken", card3X + 20, cardY + 42, 15, LIGHTGRAY);
                    if (Hero1HitsTaken < Hero2HitsTaken) {
                        DrawText(TextFormat("WINNER: %s (%d Hits)", inputHero1Name, Hero1HitsTaken), card3X + 20, cardY + 75, 19, LIME);
                        DrawText(TextFormat("%s: %d Hits Taken", inputHero2Name, Hero2HitsTaken), card3X + 20, cardY + 102, 14, GRAY);
                    } else if (Hero2HitsTaken < Hero1HitsTaken) {
                        DrawText(TextFormat("WINNER: %s (%d Hits)", inputHero2Name, Hero2HitsTaken), card3X + 20, cardY + 75, 19, YELLOW);
                        DrawText(TextFormat("%s: %d Hits Taken", inputHero1Name, Hero1HitsTaken), card3X + 20, cardY + 102, 14, GRAY);
                    } else DrawText(TextFormat("TIED: BOTH (%d Hits)", Hero1HitsTaken), card3X + 20, cardY + 85, 19, WHITE);

                    char restartText[] = "Press [R] to Play Again   |   [M] Main Menu";
                    DrawText(restartText, (WindowWidth - MeasureText(restartText, 22)) / 2, panelY + panelH - 35, 22, GOLD);
                }

                if (winDialogueIndex < 2)
                {
                    char promptText[] = "PRESS [ENTER] OR [SPACE] TO ADVANCE BROADCAST & SAVE LOGS";
                    if (((int)(GetTime() * 3)) % 2 == 0)
                        DrawText(promptText, (WindowWidth - MeasureText(promptText, 18)) / 2, panelY + panelH - 45, 18, GREEN);
                    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE))
                    {
                        PlaySound(menuSelect);
                        winDialogueIndex++;
                        if (winDialogueIndex == 2 && !currentMatchSaved)
                        {
                            MatchRecord rec = {
                                .isVictory = true,
                                .runTime = runPlayTime, // Saved clear time
                                .hero1Score = Hero1Score,
                                .hero2Score = Hero2Score,
                                .hero1Kills = Hero1Kills,
                                .hero2Kills = Hero2Kills,
                                .hero1BossDamage = Hero1BossDamage,
                                .hero2BossDamage = Hero2BossDamage,
                                .hero1HitsTaken = Hero1HitsTaken,
                                .hero2HitsTaken = Hero2HitsTaken
                            };
                            strncpy(rec.hero1PilotName, inputHero1Name, sizeof(rec.hero1PilotName) - 1);
                            strncpy(rec.hero2PilotName, inputHero2Name, sizeof(rec.hero2PilotName) - 1);
                            SaveMatchRecord(rec);

                            // Update Permanent Career Profiles
                            careerProfiles[0].totalMissions++;
                            careerProfiles[0].totalVictories++;
                            careerProfiles[0].totalScore += Hero1Score;
                            careerProfiles[0].totalAlienKills += Hero1Kills;
                            careerProfiles[0].totalMinionKills += Hero1MinionKills;
                            careerProfiles[0].totalBossDamage += Hero1BossDamage;
                            careerProfiles[0].totalHitsTaken += Hero1HitsTaken;

                            careerProfiles[1].totalMissions++;
                            careerProfiles[1].totalVictories++;
                            careerProfiles[1].totalScore += Hero2Score;
                            careerProfiles[1].totalAlienKills += Hero2Kills;
                            careerProfiles[1].totalMinionKills += Hero2MinionKills;
                            careerProfiles[1].totalBossDamage += Hero2BossDamage;
                            careerProfiles[1].totalHitsTaken += Hero2HitsTaken;

                            SaveHeroProfiles(careerProfiles);
                            currentMatchSaved = true;
                        }
                    }
                }

                if (IsKeyPressed(KEY_R))
                {
                    PlaySound(menuSelect); StopMusicStream(bgmWin); winBgmStarted = false; winDialogueIndex = 0;
                    if (gameplayBgmActive) { StopMusicStream(bgmGameplay[BgmTrackIndex]); gameplayBgmActive = false; }
                    AliensKilled = 0; Hero1Lives = 4; Hero2Lives = 4; Hero1Score = 0; Hero2Score = 0;
                    cheerPlayed = false; winSoundPlayed = false;
                    Hero1Kills = 0; Hero2Kills = 0; Hero1MinionKills = 0; Hero2MinionKills = 0;
                    Hero1BossDamage = 0; Hero2BossDamage = 0; Hero1HitsTaken = 0; Hero2HitsTaken = 0;
                    bossSpawned = false; bossWarningActive = false; bossWarningTimer = 0.0f; bossActive = false; bossDefeated = false;
                    bossWarpActive = false; bossWarpTimer = 0.0f; evacActive = false; evacTimer = 0.0f;
                    bossHp = 100; bossLeftPodHp = BossPodMaxHp; bossRightPodHp = BossPodMaxHp;
                    bossPos = (Vector2){ (WindowWidth - BossWidth) / 2.0f, 60.0f };
                    bossShootTimer = 0.0f; bossLaserTimer = 0.0f; bossLaserActive = false; bossLaserDuration = 0.0f; bossOrbTimer = 0.0f;
                    hero1Debuffed = false; hero1DebuffTimer = 0.0f; hero2Debuffed = false; hero2DebuffTimer = 0.0f;
                    empDropped = false; empActive = false; empBuffTimer = 0.0f; empShockwaveRadius = 0.0f;
                    hero1HitFlashTimer = 0.0f; hero2HitFlashTimer = 0.0f; SpecialReady = false; starttimer = false;
                    ShoabSpecialTime = 0.0f; Hero1SpecialBulletActive = false; NayemulSpecialReady = false; NayemulSpecialTime = 0.0f;
                    clusterBossDamageDealt = false;
                    shoabEdgeFlashTimer = 0.0f; nayemulEdgeFlashTimer = 0.0f; bossHitFlashTimer = 0.0f;
                    magRailSoundPlayed = false; laserChargeSoundPlayed = false; prevHeroesTethered = false;
                    hero1AsteroidSlowTimer = 0.0f; hero2AsteroidSlowTimer = 0.0f;
                    superPauseTimer = 0.0f; hitStopTimer = 0.0f; canopyGlintTimer = 0.0f; spaceLightningTimer = 0.0f;
                    hero1OverdriveTimer = 0.0f; hero2OverdriveTimer = 0.0f;
                    asteroidSpawnTimer = 0.0f; runPlayTime = 0.0f; rankAudioPlayed = false;

                    // Reset Black Hole & Void Entity
                    blackHole.active = false;
                    blackHole.spawned = false;
                    blackHole.collapsing = false;
                    blackHole.scale = 1.0f;
                    blackHole.alpha = 1.0f;
                    StopSound(sndBlackHoleDrone);

                    voidEntity.active = false;
                    voidEntity.spawned = false;
                    StopSound(sndEntityDrone);
                    for (int eo = 0; eo < MAX_ENTITY_ORBS; eo++) entityOrbs[eo].active = false;

                    currentMatchSaved = false;

                    for (int a = 0; a < MAX_ASTEROIDS; a++) asteroids[a].active = false;
                    for (int d = 0; d < MAX_ASTEROID_DEBRIS; d++) debrisPool[d].active = false;
                    for (int s = 0; s < MAX_SMOKE_PARTICLES; s++) smokePool[s].active = false;
                    for (int sp = 0; sp < MAX_SPARK_PARTICLES; sp++) sparkPool[sp].active = false;

                    for (int m = 0; m < MAX_CLUSTER_MISSILES; m++) clusterMissileActive[m] = false;
                    for (int b = 0; b < MAX_CLUSTER_BLASTS; b++) clusterBlastActive[b] = false;
                    explosionShakeTimer = 0.0f; teamShieldActive = false; teamShieldActiveTimer = 0.0f; teamShieldCooldownTimer = 0.0f; teamShieldCenterX = 0.0f;
                    commandersTriggered = false; commanderIntroActive = false; commanderIntroTimer = 0.0f; gameTimeDilation = 1.0f;
                    jammerActive = false; warpActive = false; jammerHp = 0; warpHp = 0; radarJammedTimer = 0.0f;
                    jammerPowerTimer = 0.0f; warpPowerTimer = 0.0f; bossEnraged = false;
                    for (int b = 0; b < MAX_COMMANDER_BULLETS; b++) { jammerBulletActive[b] = false; warpBulletActive[b] = false; jammerBulletLocked[b] = false; }
                    minionDeployTimer = 0.0f;
                    for (int m = 0; m < MAX_MINIONS; m++) minionActive[m] = false;
                    for (int mb = 0; mb < MAX_MINION_BULLETS; mb++) minionBulletActive[mb] = false;
                    for (int k = 0; k < MAX_BOSS_BULLETS; k++) bossBulletActive[k] = false;
                    for (int k = 0; k < MAX_BOSS_ORBS; k++) bossOrbActive[k] = false;
                    Hero1Pos = (Vector2){ WindowWidth * 0.35f, WindowHeight - HeroHeight };
                    Hero2Pos = (Vector2){ WindowWidth * 0.65f, WindowHeight - HeroHeight };
                    Hero1Speed = (Vector2){ 0, 0 }; Hero2Speed = (Vector2){ 0, 0 }; AlienSpeed = (Vector2){ AlienSpeedX, AlienSpeedY };
                    Hero1BulletActive[0] = false; Hero1BulletActive[1] = false;
                    Hero2BulletActive[0] = false; Hero2BulletActive[1] = false;
                    AlienBulletActive = false; AlienShootTimer = 0.0f;
                    AlienPos[0][0] = (Vector2){ 150, 50 };
                    for (int X = 0; X < AlienInX; X++)
                    {
                        for (int Y = 0; Y < AlienInY; Y++)
                        {
                            AlienPos[X][Y].x = AlienPos[0][0].x + (AlienSize + AlienDistance) * X;
                            AlienPos[X][Y].y = AlienPos[0][0].y + (AlienSize + AlienDistance) * Y;
                            AlienAlive[X][Y] = true; SpeedBuff[Y] = GetRandomValue(0, 20);
                        }
                    }
                }
                if (IsKeyPressed(KEY_M))
                {
                    PlaySound(menuSelect); StopMusicStream(bgmWin); winBgmStarted = false; winDialogueIndex = 0;
                    if (gameplayBgmActive) { StopMusicStream(bgmGameplay[BgmTrackIndex]); gameplayBgmActive = false; }
                    StopSound(sndBlackHoleDrone); StopSound(sndEntityDrone);
                    CurrentState = STATE_MENU; PlayMusicStream(bgmMenu);
                }
                EndMode2D();
                EndTextureMode();

                BeginDrawing();
                ClearBackground(BLACK);
                float curScale = fminf((float)GetScreenWidth() / WindowWidth, (float)GetScreenHeight() / WindowHeight);
                Rectangle srcRec = { 0.0f, 0.0f, (float)renderTarget.texture.width, -(float)renderTarget.texture.height };
                Rectangle destRec = {
                    ((float)GetScreenWidth() - ((float)WindowWidth * curScale)) * 0.5f,
                    ((float)GetScreenHeight() - ((float)WindowHeight * curScale)) * 0.5f,
                    (float)WindowWidth * curScale,
                    (float)WindowHeight * curScale
                };
                DrawTexturePro(renderTarget.texture, srcRec, destRec, (Vector2){ 0, 0 }, 0.0f, WHITE);
                EndDrawing();
                continue;
            }

            // Boss Trigger
            if (AliensKilled == AlienInX * AlienInY && !bossSpawned && !jammerActive && !warpActive && !commanderIntroActive)
            {
                if ((Hero1Lives > 0 || Hero2Lives > 0) && !GameOver && !deathSequenceActive)
                {
                    bossSpawned = true; bossWarningActive = true; bossWarningTimer = 0.0f;
                    if (gameplayBgmActive) { StopMusicStream(bgmGameplay[BgmTrackIndex]); gameplayBgmActive = false; }
                    PlayMusicStream(bgmBoss); PlaySound(sndWarningSiren);
                    if (Hero1Lives <= 0) { Hero1Lives = 2; Hero1Pos = (Hero1CrashPos.x != 0) ? Hero1CrashPos : (Vector2){ WindowWidth * 0.35f, WindowHeight - HeroHeight }; hero1ReviveTimer = 0.0f; }
                    else Hero1Lives += 2;
                    if (Hero2Lives <= 0) { Hero2Lives = 2; Hero2Pos = (Hero2CrashPos.x != 0) ? Hero2CrashPos : (Vector2){ WindowWidth * 0.65f, WindowHeight - HeroHeight }; hero2ReviveTimer = 0.0f; }
                    else Hero2Lives += 2;
                    PlaySound(cheer);
                }
            }

            if (bossWarningActive)
            {
                bossWarningTimer += Time;
                if (((int)(GetTime() * 5)) % 2 == 0)
                {
                    char warnText[] = "WARNING!! that boss has arrived";
                    DrawText(warnText, (WindowWidth - MeasureText(warnText, 48)) / 2, WindowHeight / 2 - 30, 48, RED);
                }
                const char* extraLifeTxt = "2 extra lives have been added";
                int elw = MeasureText(extraLifeTxt, 18);
                if (Hero1Lives > 0) DrawText(extraLifeTxt, (int)Hero1Pos.x - elw / 2, (int)Hero1Pos.y - 48, 18, LIME);
                if (Hero2Lives > 0)
                {
                    float yOffset = (fabsf(Hero1Pos.x - Hero2Pos.x) < elw) ? 72.0f : 48.0f;
                    DrawText(extraLifeTxt, (int)Hero2Pos.x - elw / 2, (int)Hero2Pos.y - (int)yOffset, 18, YELLOW);
                }
                if (bossWarningTimer >= 3.0f)
                {
                    StopSound(sndWarningSiren);
                    bossWarningActive = false;
                    bossWarpActive = true;
                    bossWarpTimer = 2.5f;
                    PlaySound(sndBossWarpIn);
                    bossHp = 100; bossLeftPodHp = BossPodMaxHp; bossRightPodHp = BossPodMaxHp;
                    bossPos = (Vector2){ (WindowWidth - BossWidth) / 2.0f, 60.0f };
                    bossLaserTimer = 0.0f; bossOrbTimer = 0.0f; minionDeployTimer = 0.0f;
                }
            }

            // Hero Movement Speed
            float curSpeed1 = HeroSpeedX;
            if (hero1Debuffed) { hero1DebuffTimer -= Time; if (hero1DebuffTimer <= 0.0f) hero1Debuffed = false; curSpeed1 = HeroSpeedX * 0.30f; }
            if (hero1AsteroidSlowTimer > 0.0f) { hero1AsteroidSlowTimer -= Time; curSpeed1 *= 0.40f; }
            if (hero1OverdriveTimer > 0.0f) { curSpeed1 *= 1.25f; }

            float curSpeed2 = HeroSpeedX;
            if (hero2Debuffed) { hero2DebuffTimer -= Time; if (hero2DebuffTimer <= 0.0f) hero2Debuffed = false; curSpeed2 = HeroSpeedX * 0.30f; }
            if (hero2AsteroidSlowTimer > 0.0f) { hero2AsteroidSlowTimer -= Time; curSpeed2 *= 0.40f; }
            if (hero2OverdriveTimer > 0.0f) { curSpeed2 *= 1.25f; }

            if (!deathSequenceActive && !bossWarpActive && !evacActive && !isFrozen && !GameOver && !bossDefeated)
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
            else { Hero1Speed.x = 0; Hero2Speed.x = 0; }

            if (Hero1Lives > 0 && !GameOver && !bossDefeated) Hero1Pos = Vector2Add(Hero1Pos, Vector2Scale(Hero1Speed, Time));
            if (Hero2Lives > 0 && !GameOver && !bossDefeated) Hero2Pos = Vector2Add(Hero2Pos, Vector2Scale(Hero2Speed, Time));

            float minX = HeroWidth / 2.0f, maxX = WindowWidth - HeroWidth / 2.0f;
            if (teamShieldActive)
            {
                float innerRadius = TeamShieldRadius - (HeroWidth / 2.0f);
                float trapMinX = teamShieldCenterX - innerRadius, trapMaxX = teamShieldCenterX + innerRadius;
                if (trapMinX < minX) trapMinX = minX;
                if (trapMaxX > maxX) trapMaxX = maxX;

                if (Hero1Lives > 0) { if (Hero1Pos.x < trapMinX) Hero1Pos.x = trapMinX; if (Hero1Pos.x > trapMaxX) Hero1Pos.x = trapMaxX; }
                if (Hero2Lives > 0) { if (Hero2Pos.x < trapMinX) Hero2Pos.x = trapMinX; if (Hero2Pos.x > trapMaxX) Hero2Pos.x = trapMaxX; }
            }
            else
            {
                if (Hero1Lives > 0) { if (Hero1Pos.x < minX) Hero1Pos.x = minX; if (Hero1Pos.x > maxX) Hero1Pos.x = maxX; }
                if (Hero2Lives > 0) { if (Hero2Pos.x < minX) Hero2Pos.x = minX; if (Hero2Pos.x > maxX) Hero2Pos.x = maxX; }
            }

            if (empBuffTimer > 0.0f && !isFrozen) empBuffTimer -= Time;

            // Hero 1 Shooting
            bool canShoot1 = !hero1Debuffed && (Hero1Lives > 0) && !GameOver && !bossDefeated;
            if (empBuffTimer <= 0.0f && hero1OverdriveTimer <= 0.0f)
            {
                for (int i = 0; i < 2; i++)
                    if (Hero1BulletActive[i] && Hero1BulletPos[i].y > WindowHeight / 2.0f) { canShoot1 = false; break; }
            }
            if ((IsKeyPressed(KEY_W) || IsKeyPressed(KEY_SPACE)) && canShoot1 && !deathSequenceActive && !bossWarningActive && !bossWarpActive && !evacActive)
            {
                PlaySound(shoot);
                canopyGlintTimer = 0.12f;
                for (int i = 0; i < 2; i++)
                {
                    if (!Hero1BulletActive[i]) { Hero1BulletActive[i] = true; Hero1BulletPos[i] = (Vector2){ Hero1Pos.x, Hero1Pos.y }; break; }
                }
            }

            // Hero 2 Shooting
            bool canShoot2 = !hero2Debuffed && (Hero2Lives > 0) && !GameOver && !bossDefeated;
            if (empBuffTimer <= 0.0f && hero2OverdriveTimer <= 0.0f)
            {
                for (int i = 0; i < 2; i++)
                    if (Hero2BulletActive[i] && Hero2BulletPos[i].y > WindowHeight / 2.0f) { canShoot2 = false; break; }
            }
            if (IsKeyPressed(KEY_UP) && canShoot2 && !deathSequenceActive && !bossWarningActive && !bossWarpActive && !evacActive)
            {
                PlaySound(shoot);
                canopyGlintTimer = 0.12f;
                for (int i = 0; i < 2; i++)
                {
                    if (!Hero2BulletActive[i]) { Hero2BulletActive[i] = true; Hero2BulletPos[i] = (Vector2){ Hero2Pos.x, Hero2Pos.y }; break; }
                }
            }

            // Shoab Special Beam 
            if (IsKeyPressed(KEY_Q) && Hero1Lives > 0 && !deathSequenceActive && !bossWarningActive && !bossWarpActive && !evacActive && !GameOver && !bossDefeated)
            {
                if (SpecialReady)
                {
                    Hero1SpecialBulletPos = (Vector2){ Hero1Pos.x, Hero1Pos.y };
                    Hero1SpecialBulletActive = true; ShoabSpecialTime = 0.0f; SpecialReady = false;
                    superPauseTimer = 0.18f;
                    canopyGlintTimer = 0.35f;
                    PlaySound(sndSpecialBeam);
                }
                else PlaySound(sndDryFire);
            }

            // Nayemul Cluster Salvo/missile!!!!!!!!!!!!!!!!!!!!!!!!!!
            if (IsKeyPressed(KEY_RIGHT_SHIFT) && Hero2Lives > 0 && !deathSequenceActive && !bossWarningActive && !bossWarpActive && !evacActive && !GameOver && !bossDefeated)
            {
                if (NayemulSpecialReady)
                {
                    float spreadVx[8] = { -150.0f, -107.0f, -64.0f, -21.0f, 21.0f, 64.0f, 107.0f, 150.0f };
                    for (int m = 0; m < MAX_CLUSTER_MISSILES; m++)
                    {
                        clusterMissileActive[m] = true;
                        clusterMissilePos[m] = (Vector2){ Hero2Pos.x, Hero2Pos.y };
                        clusterMissileVel[m] = (Vector2){ spreadVx[m], -950.0f };
                    }
                    NayemulSpecialTime = 0.0f; NayemulSpecialReady = false;
                    clusterBossDamageDealt = false;
                    superPauseTimer = 0.18f;
                    canopyGlintTimer = 0.35f;
                    PlaySound(sndClusterLaunch);
                }
                else PlaySound(sndDryFire);
            }

            // EMP Drop(when two pods destroyed+trigger )-bug fixed...
            if (bossActive && !empDropped && bossLeftPodHp <= 0 && bossRightPodHp <= 0)
            {
                empDropped = true; empActive = true;
                empPos = (Vector2){ (float)GetRandomValue(350, WindowWidth - 350), -40.0f };
                PlaySound(sndAirdropIncoming);
            }
            if (empActive && !isFrozen && !deathSequenceActive && !GameOver && !bossDefeated)
            {
                empPos.y += 110.0f * Time;
                if (empPos.y > WindowHeight + 40) empActive = false;
                Rectangle empRec = { empPos.x - 22, empPos.y - 22, 44, 44 };
                Rectangle h1Rec = { Hero1Pos.x - HeroWidth / 2.0f, Hero1Pos.y, HeroWidth, HeroHeight };
                Rectangle h2Rec = { Hero2Pos.x - HeroWidth / 2.0f, Hero2Pos.y, HeroWidth, HeroHeight };

                if ((Hero1Lives > 0 && CheckCollisionRecs(empRec, h1Rec)) || (Hero2Lives > 0 && CheckCollisionRecs(empRec, h2Rec)))
                {
                    empActive = false; empBuffTimer = 8.5f; empShockwaveRadius = 10.0f; empShockwaveCenter = empPos;
                    PlaySound(sndEmpBlast);
                    AlienBulletActive = false;
                    for (int mb = 0; mb < MAX_MINION_BULLETS; mb++) minionBulletActive[mb] = false;
                    for (int k = 0; k < MAX_BOSS_BULLETS; k++) bossBulletActive[k] = false;
                    for (int k = 0; k < MAX_BOSS_ORBS; k++) bossOrbActive[k] = false;
                }
            }

            // Hero Regular Bullets Update (Movement stops on game over or victory)-bugs fixed!!!!!
            for (int h = 0; h < 2; h++)
            {
                Vector2 *bPos = (h == 0) ? Hero1BulletPos : Hero2BulletPos;
                bool *bActive = (h == 0) ? Hero1BulletActive : Hero2BulletActive;
                int *hScore = (h == 0) ? &Hero1Score : &Hero2Score;
                int *hKills = (h == 0) ? &Hero1Kills : &Hero2Kills;
                int *hBossDmg = (h == 0) ? &Hero1BossDamage : &Hero2BossDamage;

                for (int i = 0; i < 2; i++)
                {
                    if (bActive[i] && !isFrozen && !deathSequenceActive && !GameOver && !bossDefeated)
                    {
                        float currentBulletSpeed = (empBuffTimer > 0.0f || (h == 0 && hero1OverdriveTimer > 0.0f) || (h == 1 && hero2OverdriveTimer > 0.0f)) ? BulletSpeedY * 1.35f : BulletSpeedY;
                        bPos[i].y -= currentBulletSpeed * Time;
                        if (bPos[i].y < -BulletHeight) { bActive[i] = false; continue; }

                        Rectangle bRec = { bPos[i].x - BulletWidth / 2.0f, bPos[i].y, BulletWidth, BulletHeight };
                        if (!bossActive && !bossSpawned)
                        {
                            for (int X = 0; X < AlienInX; X++)
                            {
                                for (int Y = 0; Y < AlienInY; Y++)
                                {
                                    if (AlienAlive[X][Y] && CheckCollisionRecs(bRec, (Rectangle){ AlienPos[X][Y].x, AlienPos[X][Y].y, AlienSize, AlienSize }))
                                    {
                                        PlaySound(damage); AlienAlive[X][Y] = false; bActive[i] = false;
                                        AliensKilled++; *hScore += 100; *hKills += 1;
                                        if (!cheerPlayed && AliensKilled >= (int)(AlienInX * AlienInY * 0.70f) && (Hero1Lives + Hero2Lives >= 3))
                                        {
                                            PlaySound(cheer); cheerPlayed = true;
                                        }
                                        break;
                                    }
                                }
                                if (!bActive[i]) break;
                            }
                        }

                        if (jammerActive && bActive[i] && CheckCollisionRecs(bRec, (Rectangle){ jammerPos.x, jammerPos.y, COMMANDER_SIZE, COMMANDER_SIZE }))
                        {
                            PlaySound(damage); bActive[i] = false; jammerHp--; *hScore += 50;
                            if (jammerHp <= 0) { jammerActive = false; *hScore += 500; PlaySound(sndJammerDeath); }
                        }
                        if (warpActive && bActive[i] && CheckCollisionRecs(bRec, (Rectangle){ warpPos.x, warpPos.y, COMMANDER_SIZE, COMMANDER_SIZE }))
                        {
                            PlaySound(damage); bActive[i] = false; warpHp--; *hScore += 50;
                            if (warpHp <= 0) { warpActive = false; *hScore += 500; PlaySound(sndWarpDeath); }
                        }

                        // Damage Mysterious Void Entity with regular blasters
                        if (voidEntity.active && bActive[i])
                        {
                            Rectangle veRec = { voidEntity.pos.x - 50.0f, voidEntity.pos.y - 50.0f, 100.0f, 100.0f };
                            if (CheckCollisionRecs(bRec, veRec))
                            {
                                PlaySound(damage); bActive[i] = false; voidEntity.hp -= 2; *hScore += 60;
                                if (voidEntity.hp <= 0)
                                {
                                    voidEntity.active = false; *hScore += 800;
                                    StopSound(sndEntityDrone);
                                    PlaySound(sndEntityScream);
                                    hitStopTimer = 0.08f;
                                }
                            }
                        }

                        if (bossActive && bActive[i])
                        {
                            for (int m = 0; m < MAX_MINIONS; m++)
                            {
                                if (minionActive[m] && CheckCollisionRecs(bRec, (Rectangle){ minionPos[m].x, minionPos[m].y, MinionSize, MinionSize }))
                                {
                                    PlaySound(damage); bActive[i] = false; minionHp[m]--;
                                    if (minionHp[m] <= 0)
                                    {
                                        minionActive[m] = false; *hScore += 150; *hKills += 1;
                                        if (h == 0) Hero1MinionKills++; else Hero2MinionKills++;
                                    }
                                    break;
                                }
                            }
                        }

                        if (bossActive && bActive[i])
                        {
                            Rectangle leftPodRec  = { bossPos.x + 8, bossPos.y + 70, 75, 110 };
                            Rectangle rightPodRec = { bossPos.x + BossWidth - 83, bossPos.y + 70, 75, 110 };
                            Rectangle coreRec     = { bossPos.x + 85, bossPos.y + 35, 130, 130 };

                            if (bossLeftPodHp > 0 && CheckCollisionRecs(bRec, leftPodRec))
                            {
                                PlaySound(damage); bActive[i] = false; bossLeftPodHp -= 5; *hScore += 25; *hBossDmg += 5; bossHitFlashTimer = 0.06f;
                                if (bossLeftPodHp <= 0) { bossLeftPodHp = 0; *hScore += 250; PlaySound(sndPodDestroy); }
                            }
                            else if (bossRightPodHp > 0 && CheckCollisionRecs(bRec, rightPodRec))
                            {
                                PlaySound(damage); bActive[i] = false; bossRightPodHp -= 5; *hScore += 25; *hBossDmg += 5; bossHitFlashTimer = 0.06f;
                                if (bossRightPodHp <= 0) { bossRightPodHp = 0; *hScore += 250; PlaySound(sndPodDestroy); }
                            }
                            else if (CheckCollisionRecs(bRec, coreRec))
                            {
                                bActive[i] = false;
                                if (bossLeftPodHp > 0 || bossRightPodHp > 0) PlaySound(damage);
                                else
                                {
                                    if (!deathSequenceActive && !GameOver)
                                    {
                                        PlaySound(damage); bossHp -= 3; *hScore += 50; *hBossDmg += 3; bossHitFlashTimer = 0.06f;
                                        if (bossHp <= 40 && !bossEnraged) { bossEnraged = true; PlaySound(bossEnrage); }
                                        if (bossHp <= 0)
                                        {
                                            bossHp = 0; bossActive = false; evacActive = true; evacTimer = 3.8f;
                                            PlaySound(sndEvacBoom);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // SHOAB SPECIAL BULLET (visuals are fixed now...)
            if (Hero1SpecialBulletActive && !isFrozen && !deathSequenceActive && !GameOver && !bossDefeated)
            {
                Hero1SpecialBulletPos.y -= BulletSpeedY * Time;
                Rectangle SpecialBulletRec = { Hero1SpecialBulletPos.x - HeroWidth / 2.0f, Hero1SpecialBulletPos.y, HeroWidth, HeroHeight * 3 };

                if (!bossActive && !bossSpawned)
                {
                    for (int i = 0; i < AlienInX; i++)
                    {
                        for (int j = 0; j < AlienInY; j++)
                        {
                            if (AlienAlive[i][j] && CheckCollisionRecs(SpecialBulletRec, (Rectangle){ AlienPos[i][j].x, AlienPos[i][j].y, AlienSize, AlienSize }))
                            {
                                AlienAlive[i][j] = false; AliensKilled++; Hero1Score += 100; Hero1Kills++; PlaySound(damage);
                                hitStopTimer = 0.04f;
                            }
                        }
                    }
                }
                if (jammerActive && CheckCollisionRecs(SpecialBulletRec, (Rectangle){ jammerPos.x, jammerPos.y, COMMANDER_SIZE, COMMANDER_SIZE }))
                {
                    PlaySound(damage); jammerHp -= 2; Hero1Score += 50; hitStopTimer = 0.04f;
                    if (jammerHp <= 0) { jammerActive = false; Hero1Score += 500; PlaySound(sndJammerDeath); }
                }
                if (warpActive && CheckCollisionRecs(SpecialBulletRec, (Rectangle){ warpPos.x, warpPos.y, COMMANDER_SIZE, COMMANDER_SIZE }))
                {
                    PlaySound(damage); warpHp -= 2; Hero1Score += 50; hitStopTimer = 0.04f;
                    if (warpHp <= 0) { warpActive = false; Hero1Score += 500; PlaySound(sndWarpDeath); }
                }

                // Shoab Hyper Beam damages Void Entity
                if (voidEntity.active && CheckCollisionRecs(SpecialBulletRec, (Rectangle){ voidEntity.pos.x - 50.0f, voidEntity.pos.y - 50.0f, 100.0f, 100.0f }))
                {
                    PlaySound(damage); voidEntity.hp -= 20; Hero1Score += 250; hitStopTimer = 0.05f;
                    if (voidEntity.hp <= 0)
                    {
                        voidEntity.active = false; Hero1Score += 800;
                        StopSound(sndEntityDrone);
                        PlaySound(sndEntityScream);
                    }
                }

                if (bossActive)
                {
                    for (int m = 0; m < MAX_MINIONS; m++)
                    {
                        if (minionActive[m] && CheckCollisionRecs(SpecialBulletRec, (Rectangle){ minionPos[m].x, minionPos[m].y, MinionSize, MinionSize }))
                        {
                            minionActive[m] = false; Hero1Kills += 1; Hero1MinionKills++; Hero1Score += 200; PlaySound(damage);
                        }
                    }

                    Rectangle leftPodRec  = { bossPos.x + 8, bossPos.y + 70, 75, 110 };
                    Rectangle rightPodRec = { bossPos.x + BossWidth - 83, bossPos.y + 70, 75, 110 };
                    Rectangle coreRec     = { bossPos.x + 85, bossPos.y + 35, 130, 130 };

                    if (bossLeftPodHp > 0 && CheckCollisionRecs(SpecialBulletRec, leftPodRec))
                    {
                        Hero1SpecialBulletActive = false; PlaySound(damage); bossLeftPodHp -= 50; Hero1Score += 250; Hero1BossDamage += 50; bossHitFlashTimer = 0.06f;
                        hitStopTimer = 0.05f;
                        if (bossLeftPodHp <= 0)
                        {
                            PlaySound(sndPodDestroy); bossRightPodHp += bossLeftPodHp;
                            if (bossRightPodHp < 0) { bossHp += bossRightPodHp; bossRightPodHp = 0; }
                            bossLeftPodHp = 0; Hero1Score += 250;
                            if (bossHp <= 40 && !bossEnraged) { bossEnraged = true; PlaySound(bossEnrage); }
                            if (bossHp <= 0 && !deathSequenceActive && !GameOver)
                            {
                                bossHp = 0; bossActive = false; evacActive = true; evacTimer = 3.8f;
                                PlaySound(sndEvacBoom);
                            }
                        }
                    }
                    else if (bossRightPodHp > 0 && CheckCollisionRecs(SpecialBulletRec, rightPodRec))
                    {
                        Hero1SpecialBulletActive = false; PlaySound(damage); bossRightPodHp -= 50; Hero1Score += 250; Hero1BossDamage += 50; bossHitFlashTimer = 0.06f;
                        hitStopTimer = 0.05f;
                        if (bossRightPodHp <= 0)
                        {
                            PlaySound(sndPodDestroy); bossLeftPodHp += bossRightPodHp;
                            if (bossLeftPodHp < 0) { bossHp += bossLeftPodHp; bossLeftPodHp = 0; }
                            bossRightPodHp = 0; Hero1Score += 250;
                            if (bossHp <= 40 && !bossEnraged) { bossEnraged = true; PlaySound(bossEnrage); }
                            if (bossHp <= 0 && !deathSequenceActive && !GameOver)
                            {
                                bossHp = 0; bossActive = false; evacActive = true; evacTimer = 3.8f;
                                PlaySound(sndEvacBoom);
                            }
                        }
                    }
                    else if (CheckCollisionRecs(SpecialBulletRec, coreRec))
                    {
                        Hero1SpecialBulletActive = false;
                        hitStopTimer = 0.05f;
                        if (bossLeftPodHp > 0 || bossRightPodHp > 0)
                        {
                            PlaySound(damage); bossLeftPodHp -= 50; Hero1Score += 250; Hero1BossDamage += 50; bossHitFlashTimer = 0.06f;
                            if (bossLeftPodHp <= 0)
                            {
                                PlaySound(sndPodDestroy); bossRightPodHp += bossLeftPodHp;
                                if (bossRightPodHp < 0) { bossHp += bossRightPodHp; bossRightPodHp = 0; }
                                bossLeftPodHp = 0; Hero1Score += 250;
                                if (bossHp <= 40 && !bossEnraged) { bossEnraged = true; PlaySound(bossEnrage); }
                                if (bossHp <= 0 && !deathSequenceActive && !GameOver)
                                {
                                    bossHp = 0; bossActive = false; evacActive = true; evacTimer = 3.8f;
                                    PlaySound(sndEvacBoom);
                                }
                            }
                        }
                        else
                        {
                            if (!deathSequenceActive && !GameOver)
                            {
                                PlaySound(damage); bossHp -= 30; Hero1Score += 500; Hero1BossDamage += 30; bossHitFlashTimer = 0.06f;
                                if (bossHp <= 40 && !bossEnraged) { bossEnraged = true; PlaySound(bossEnrage); }
                                if (bossHp <= 0)
                                {
                                    bossHp = 0; bossActive = false; evacActive = true; evacTimer = 3.8f;
                                    PlaySound(sndEvacBoom);
                                }
                            }
                        }
                    }
                }
                if (Hero1SpecialBulletPos.y < 0) Hero1SpecialBulletActive = false;
            }

            // NAYEMUL CLUSTER MISSILES ,re-designed and also bugs fixed...
            for (int m = 0; m < MAX_CLUSTER_MISSILES; m++)
            {
                if (!clusterMissileActive[m]) continue;
                if (!isFrozen && !deathSequenceActive && !GameOver && !bossDefeated)
                {
                    clusterMissilePos[m] = Vector2Add(clusterMissilePos[m], Vector2Scale(clusterMissileVel[m], Time));
                }

                if (clusterMissilePos[m].y < -20 || clusterMissilePos[m].x < 0 || clusterMissilePos[m].x > WindowWidth)
                {
                    clusterMissileActive[m] = false; continue;
                }

                Rectangle mRec = { clusterMissilePos[m].x - 6, clusterMissilePos[m].y - 12, 12, 24 };
                bool hitDetonated = false;

                if (!bossActive && !bossSpawned)
                {
                    for (int X = 0; X < AlienInX && !hitDetonated; X++)
                        for (int Y = 0; Y < AlienInY && !hitDetonated; Y++)
                            if (AlienAlive[X][Y] && CheckCollisionRecs(mRec, (Rectangle){ AlienPos[X][Y].x, AlienPos[X][Y].y, AlienSize, AlienSize })) hitDetonated = true;
                }
                if (jammerActive && !hitDetonated && CheckCollisionRecs(mRec, (Rectangle){ jammerPos.x, jammerPos.y, COMMANDER_SIZE, COMMANDER_SIZE })) hitDetonated = true;
                if (warpActive && !hitDetonated && CheckCollisionRecs(mRec, (Rectangle){ warpPos.x, warpPos.y, COMMANDER_SIZE, COMMANDER_SIZE })) hitDetonated = true;

                // Cluster collision against Void Entity
                if (voidEntity.active && !hitDetonated && CheckCollisionRecs(mRec, (Rectangle){ voidEntity.pos.x - 50.0f, voidEntity.pos.y - 50.0f, 100.0f, 100.0f }))
                {
                    hitDetonated = true;
                }

                if (bossActive && !hitDetonated)
                {
                    for (int minIdx = 0; minIdx < MAX_MINIONS; minIdx++)
                    {
                        if (minionActive[minIdx] && CheckCollisionRecs(mRec, (Rectangle){ minionPos[minIdx].x, minionPos[minIdx].y, MinionSize, MinionSize }))
                        {
                            hitDetonated = true; minionActive[minIdx] = false; Hero2Score += 200; Hero2Kills++; Hero2MinionKills++; break;
                        }
                    }
                }

                if (bossActive && !hitDetonated)
                {
                    Rectangle leftPodRec  = { bossPos.x + 8, bossPos.y + 70, 75, 110 };
                    Rectangle rightPodRec = { bossPos.x + BossWidth - 83, bossPos.y + 70, 75, 110 };
                    Rectangle coreRec     = { bossPos.x + 85, bossPos.y + 35, 130, 130 };

                    if ((bossLeftPodHp > 0 && CheckCollisionRecs(mRec, leftPodRec)) ||
                        (bossRightPodHp > 0 && CheckCollisionRecs(mRec, rightPodRec)) ||
                        CheckCollisionRecs(mRec, coreRec))
                    {
                        hitDetonated = true;
                    }
                }

                if (hitDetonated)
                {
                    clusterMissileActive[m] = false;
                    PlaySound(sndClusterExplode);
                    explosionShakeTimer = 0.28f;
                    hitStopTimer = 0.05f;

                    for (int b = 0; b < MAX_CLUSTER_BLASTS; b++)
                    {
                        if (!clusterBlastActive[b])
                        {
                            clusterBlastActive[b] = true;
                            clusterBlastPos[b] = clusterMissilePos[m];
                            clusterBlastRadius[b] = 8.0f;
                            clusterBlastTimer[b] = 0.25f;
                            break;
                        }
                    }

                    Vector2 blastCenter = clusterMissilePos[m];

                    // Blast AoE on Void Entity
                    if (voidEntity.active && CheckCollisionCircleRec(blastCenter, 65.0f, (Rectangle){ voidEntity.pos.x - 50.0f, voidEntity.pos.y - 50.0f, 100.0f, 100.0f }))
                    {
                        voidEntity.hp -= 15; Hero2Score += 150;
                        if (voidEntity.hp <= 0)
                        {
                            voidEntity.active = false; Hero2Score += 800;
                            StopSound(sndEntityDrone);
                            PlaySound(sndEntityScream);
                        }
                    }

                    if (!bossActive && !bossSpawned)
                    {
                        for (int X = 0; X < AlienInX; X++)
                        {
                            for (int Y = 0; Y < AlienInY; Y++)
                            {
                                if (AlienAlive[X][Y] && CheckCollisionCircleRec(blastCenter, 60.0f, (Rectangle){ AlienPos[X][Y].x, AlienPos[X][Y].y, AlienSize, AlienSize }))
                                {
                                    AlienAlive[X][Y] = false; AliensKilled++; Hero2Score += 100; Hero2Kills++;
                                }
                            }
                        }
                    }

                    if (jammerActive && CheckCollisionCircleRec(blastCenter, 50.0f, (Rectangle){ jammerPos.x, jammerPos.y, COMMANDER_SIZE, COMMANDER_SIZE }))
                    {
                        jammerHp -= 2; Hero2Score += 50;
                        if (jammerHp <= 0) { jammerActive = false; Hero2Score += 500; PlaySound(sndJammerDeath); }
                    }
                    if (warpActive && CheckCollisionCircleRec(blastCenter, 50.0f, (Rectangle){ warpPos.x, warpPos.y, COMMANDER_SIZE, COMMANDER_SIZE }))
                    {
                        warpHp -= 2; Hero2Score += 50;
                        if (warpHp <= 0) { warpActive = false; Hero2Score += 500; PlaySound(sndWarpDeath); }
                    }

                    if (bossActive)
                    {
                        for (int minIdx = 0; minIdx < MAX_MINIONS; minIdx++)
                        {
                            if (minionActive[minIdx] && CheckCollisionCircleRec(blastCenter, 60.0f, (Rectangle){ minionPos[minIdx].x, minionPos[minIdx].y, MinionSize, MinionSize }))
                            {
                                minionActive[minIdx] = false; Hero2Score += 200; Hero2Kills++; Hero2MinionKills++;
                            }
                        }

                        if (!clusterBossDamageDealt)
                        {
                            Rectangle leftPodRec  = { bossPos.x + 8, bossPos.y + 70, 75, 110 };
                            Rectangle rightPodRec = { bossPos.x + BossWidth - 83, bossPos.y + 70, 75, 110 };
                            Rectangle coreRec     = { bossPos.x + 85, bossPos.y + 35, 130, 130 };

                            bool hitLeftPod  = bossLeftPodHp > 0 && (CheckCollisionRecs(mRec, leftPodRec) || CheckCollisionCircleRec(blastCenter, 40.0f, leftPodRec));
                            bool hitRightPod = bossRightPodHp > 0 && (CheckCollisionRecs(mRec, rightPodRec) || CheckCollisionCircleRec(blastCenter, 40.0f, rightPodRec));

                            if (bossLeftPodHp > 0 || bossRightPodHp > 0)
                            {
                                if (hitLeftPod && !hitRightPod)
                                {
                                    bossLeftPodHp -= 40; Hero2Score += 250; Hero2BossDamage += 40;
                                    clusterBossDamageDealt = true; PlaySound(damage); bossHitFlashTimer = 0.06f;
                                    if (bossLeftPodHp <= 0) { bossLeftPodHp = 0; PlaySound(sndPodDestroy); } //pods damage logic...
                                }
                                else if (hitRightPod && !hitLeftPod)
                                {
                                    bossRightPodHp -= 40; Hero2Score += 250; Hero2BossDamage += 40;
                                    clusterBossDamageDealt = true; PlaySound(damage); bossHitFlashTimer = 0.06f;
                                    if (bossRightPodHp <= 0) { bossRightPodHp = 0; PlaySound(sndPodDestroy); }
                                }
                                else if (hitLeftPod && hitRightPod)
                                {
                                    float distLeft = Vector2Distance(blastCenter, (Vector2){ leftPodRec.x + leftPodRec.width / 2.0f, leftPodRec.y + leftPodRec.height / 2.0f });
                                    float distRight = Vector2Distance(blastCenter, (Vector2){ rightPodRec.x + rightPodRec.width / 2.0f, rightPodRec.y + rightPodRec.height / 2.0f });

                                    if (distLeft <= distRight)
                                    {
                                        bossLeftPodHp -= 40;
                                        if (bossLeftPodHp <= 0) { bossLeftPodHp = 0; PlaySound(sndPodDestroy); }
                                    }
                                    else
                                    {
                                        bossRightPodHp -= 40;
                                        if (bossRightPodHp <= 0) { bossRightPodHp = 0; PlaySound(sndPodDestroy); }
                                    }
                                    Hero2Score += 250; Hero2BossDamage += 40; clusterBossDamageDealt = true; PlaySound(damage); bossHitFlashTimer = 0.06f;
                                }
                                else if (CheckCollisionRecs(mRec, coreRec) || CheckCollisionCircleRec(blastCenter, 50.0f, coreRec))
                                {
                                    PlaySound(damage);
                                }
                            }
                            else
                            {
                                if (CheckCollisionRecs(mRec, coreRec) || CheckCollisionCircleRec(blastCenter, 50.0f, coreRec))
                                {
                                    if (!deathSequenceActive && !GameOver)
                                    {
                                        PlaySound(damage); bossHp -= 35; Hero2Score += 500; Hero2BossDamage += 35;
                                        clusterBossDamageDealt = true; bossHitFlashTimer = 0.06f;
                                        if (bossHp <= 40 && !bossEnraged) { bossEnraged = true; PlaySound(bossEnrage); } //core collision check
                                        if (bossHp <= 0)
                                        {
                                            bossHp = 0; bossActive = false; evacActive = true; evacTimer = 3.8f;
                                            PlaySound(sndEvacBoom);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // Alien Shooting (Silenced when dead or game over)-bugs fixed...
            if (!bossSpawned && !isFrozen && !deathSequenceActive && !GameOver && !bossDefeated)
            {
                AlienShootTimer += Time;
                if (!AlienBulletActive && AlienShootTimer >= 1.0f)
                {
                    AlienShootTimer = 0.0f;
                    int randomX = GetRandomValue(0, AlienInX - 1);
                    for (int Y = AlienInY - 1; Y >= 0; Y--)
                    {
                        if (AlienAlive[randomX][Y])
                        {
                            PlaySound(AlienShoot); AlienBulletActive = true;
                            AlienBulletPos = (Vector2){ AlienPos[randomX][Y].x + AlienSize / 2.0f, AlienPos[randomX][Y].y + AlienSize };
                            break;
                        }
                    }
                }
            }

            // Alien Bullet Damage (With Last Stand Overdrive Trigger)-special feature added by Nayemul!!!!
            if (AlienBulletActive && !isFrozen && !deathSequenceActive && !GameOver && !bossDefeated)
            {
                AlienBulletPos.y += AlienBulletSpeedY * Time;
                if (AlienBulletPos.y > WindowHeight) AlienBulletActive = false;
                else if (!teamShieldActive && !evacActive)
                {
                    Rectangle aBulletRec = { AlienBulletPos.x - AlienBulletWidth / 2.0f, AlienBulletPos.y, AlienBulletWidth, AlienBulletHeight };
                    Rectangle h1Rec = { Hero1Pos.x - HeroWidth / 2.0f, Hero1Pos.y, HeroWidth, HeroHeight };
                    Rectangle h2Rec = { Hero2Pos.x - HeroWidth / 2.0f, Hero2Pos.y, HeroWidth, HeroHeight };

                    if (Hero1Lives > 0 && CheckCollisionRecs(aBulletRec, h1Rec))
                    {
                        AlienBulletActive = false; Hero1Lives--; Hero1HitsTaken++; hero1HitFlashTimer = 0.35f; PlaySound(heroOuch); PlaySound(sndHudGlitch);
                        if (Hero1Lives <= 0)
                        {
                            Hero1CrashPos = Hero1Pos; PlaySound(heroDeath);
                            if (Hero2Lives > 0) { hero2OverdriveTimer = 2.5f; PlaySound(sndShieldActivate); }
                            if (Hero2Lives <= 0)
                            {
                                deathSequenceActive = true; deathDelayTimer = 0.0f;
                                StopSound(AlienShoot); StopSound(sndAlienStep);
                            }
                        }
                    }
                    else if (Hero2Lives > 0 && CheckCollisionRecs(aBulletRec, h2Rec))
                    {
                        AlienBulletActive = false; Hero2Lives--; Hero2HitsTaken++; hero2HitFlashTimer = 0.35f; PlaySound(heroOuch); PlaySound(sndHudGlitch);
                        if (Hero2Lives <= 0)
                        {
                            Hero2CrashPos = Hero2Pos; PlaySound(heroDeath);
                            if (Hero1Lives > 0) { hero1OverdriveTimer = 2.5f; PlaySound(sndShieldActivate); }
                            if (Hero1Lives <= 0)
                            {
                                deathSequenceActive = true; deathDelayTimer = 0.0f;
                                StopSound(AlienShoot); StopSound(sndAlienStep);
                            }
                        }
                    }
                }
            }

            // Boss Minion Deploy after every 7 secs(Silenced when dead or game over)-bugs fixed!!!
            if (bossActive && (bossLeftPodHp > 0 || bossRightPodHp > 0) && !deathSequenceActive && !GameOver && !bossDefeated && !isFrozen)
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
                                minionActive[m] = true; minionHp[m] = 2;
                                minionPos[m] = (Vector2){ bossPos.x + 15, bossPos.y + BossHeight - 20 };
                                minionSpeed[m] = (Vector2){ (float)GetRandomValue(-90, -45), (float)GetRandomValue(35, 75) };
                                minionShootTimer[m] = 1.0f; break;
                            }
                        }
                    }
                    if (bossRightPodHp > 0)
                    {
                        for (int m = 0; m < MAX_MINIONS; m++)
                        {
                            if (!minionActive[m])
                            {
                                minionActive[m] = true; minionHp[m] = 2;
                                minionPos[m] = (Vector2){ bossPos.x + BossWidth - MinionSize - 15, bossPos.y + BossHeight - 20 };
                                minionSpeed[m] = (Vector2){ (float)GetRandomValue(45, 90), (float)GetRandomValue(35, 75) };
                                minionShootTimer[m] = 1.5f; break;
                            }
                        }
                    }
                }
            }

            // Minions Movement and Shooting logic...... (Silenced and halted when dead or game over)
            if (bossActive && !isFrozen && !deathSequenceActive && !GameOver && !bossDefeated)
            {
                for (int m = 0; m < MAX_MINIONS; m++)
                {
                    if (minionActive[m])
                    {
                        minionPos[m].x += minionSpeed[m].x * Time; minionPos[m].y += minionSpeed[m].y * Time;
                        if (minionPos[m].x <= 40) { minionPos[m].x = 40; minionSpeed[m].x = fabsf(minionSpeed[m].x); }
                        else if (minionPos[m].x >= WindowWidth - MinionSize - 40) { minionPos[m].x = WindowWidth - MinionSize - 40; minionSpeed[m].x = -fabsf(minionSpeed[m].x); }
                        if (minionPos[m].y <= 110) { minionPos[m].y = 110; minionSpeed[m].y = fabsf(minionSpeed[m].y); }
                        else if (minionPos[m].y >= WindowHeight * 0.65f - MinionSize) { minionPos[m].y = WindowHeight * 0.65f - MinionSize; minionSpeed[m].y = -fabsf(minionSpeed[m].y); }

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
                                    PlaySound(AlienShoot); break;
                                }
                            }
                        }
                    }
                }
            }

            // Minion Bullet Damage
            for (int mb = 0; mb < MAX_MINION_BULLETS; mb++)
            {
                if (minionBulletActive[mb] && !isFrozen && !deathSequenceActive && !GameOver && !bossDefeated)
                {
                    minionBulletPos[mb].y += AlienBulletSpeedY * Time;
                    if (minionBulletPos[mb].y > WindowHeight) minionBulletActive[mb] = false;
                    else if (!teamShieldActive && !evacActive)
                    {
                        Rectangle mbRec = { minionBulletPos[mb].x - AlienBulletWidth / 2.0f, minionBulletPos[mb].y, AlienBulletWidth, AlienBulletHeight };
                        Rectangle h1Rec = { Hero1Pos.x - HeroWidth / 2.0f, Hero1Pos.y, HeroWidth, HeroHeight };
                        Rectangle h2Rec = { Hero2Pos.x - HeroWidth / 2.0f, Hero2Pos.y, HeroWidth, HeroHeight };

                        if (Hero1Lives > 0 && CheckCollisionRecs(mbRec, h1Rec))
                        {
                            minionBulletActive[mb] = false; Hero1Lives--; Hero1HitsTaken++; hero1HitFlashTimer = 0.35f; PlaySound(heroOuch); PlaySound(sndHudGlitch);
                            if (Hero1Lives <= 0)
                            {
                                Hero1CrashPos = Hero1Pos; PlaySound(heroDeath);
                                if (Hero2Lives > 0) { hero2OverdriveTimer = 2.5f; PlaySound(sndShieldActivate); }
                                if (Hero2Lives <= 0)
                                {
                                    deathSequenceActive = true; deathDelayTimer = 0.0f;
                                    StopSound(AlienShoot); StopSound(sndAlienStep);
                                }
                            }
                        }
                        else if (Hero2Lives > 0 && CheckCollisionRecs(mbRec, h2Rec))
                        {
                            minionBulletActive[mb] = false; Hero2Lives--; Hero2HitsTaken++; hero2HitFlashTimer = 0.35f; PlaySound(heroOuch); PlaySound(sndHudGlitch);
                            if (Hero2Lives <= 0)
                            {
                                Hero2CrashPos = Hero2Pos; PlaySound(heroDeath);
                                if (Hero1Lives > 0) { hero1OverdriveTimer = 2.5f; PlaySound(sndShieldActivate); }
                                if (Hero1Lives <= 0)
                                {
                                    deathSequenceActive = true; deathDelayTimer = 0.0f;
                                    StopSound(AlienShoot); StopSound(sndAlienStep);
                                }
                            }
                        }
                    }
                }
            }

            float currentBossSpeedX = (bossHp <= 40) ? 250.0f : 160.0f;
            float currentBossSpeedY = (bossHp <= 40) ? 120.0f : 75.0f;

            if (bossActive && !isFrozen && !deathSequenceActive && !GameOver && !bossDefeated)
            {
                bossAnimTimer += Time; bossDirChangeTimer += Time;
                if (bossDirChangeTimer > 1.8f)
                {
                    bossDirChangeTimer = 0.0f;
                    if (GetRandomValue(0, 10) > 4) bossSpeed.y = (float)GetRandomValue(-(int)currentBossSpeedY, (int)currentBossSpeedY);
                }

                bossPos.x += ((bossSpeed.x > 0) ? currentBossSpeedX : -currentBossSpeedX) * Time;
                bossPos.y += bossSpeed.y * Time;

                if (bossPos.x <= 40) { bossPos.x = 40; bossSpeed.x = fabsf(bossSpeed.x); }
                else if (bossPos.x + BossWidth >= WindowWidth - 40) { bossPos.x = WindowWidth - BossWidth - 40; bossSpeed.x = -fabsf(bossSpeed.x); }

                float maxBossY = (WindowHeight * 0.75f) - BossHeight;
                if (bossPos.y <= 30) { bossPos.y = 30; bossSpeed.y = fabsf(bossSpeed.y); }
                else if (bossPos.y >= maxBossY) { bossPos.y = maxBossY; bossSpeed.y = -fabsf(bossSpeed.y); }

                bossShootTimer += Time;
                if (bossShootTimer >= 1.7f)
                {
                    bossShootTimer = 0.0f; float offsets[3] = { 35.0f, BossWidth / 2.0f, BossWidth - 35.0f }; int spawned = 0;
                    for (int k = 0; k < MAX_BOSS_BULLETS && spawned < 3; k++)
                    {
                        if (!bossBulletActive[k])
                        {
                            bossBulletActive[k] = true; bossBulletPos[k] = (Vector2){ bossPos.x + offsets[spawned], bossPos.y + BossHeight };
                            spawned++;
                        }
                    }
                    PlaySound(AlienShoot);
                }

                bossLaserTimer += Time;
                if (!bossLaserActive)
                {
                    if (bossLaserTimer >= 7.0f)
                    {
                        bossLaserActive = true; bossLaserDuration = 0.0f; bossLaserTimer = 0.0f;
                        canopyGlintTimer = 0.40f;
                        PlaySound(bossLaserSound);
                    }
                }
                else
                {
                    bossLaserDuration += Time;
                    if (bossLaserDuration >= 2.0f) { bossLaserActive = false; bossLaserDuration = 0.0f; StopSound(bossLaserSound); }
                }

                bossOrbTimer += Time;
                if (bossOrbTimer >= 4.0f)
                {
                    bossOrbTimer = 0.0f; PlaySound(sndOrbLaunch);
                    float spreadVx[6] = { -160.0f, -95.0f, -30.0f, 30.0f, 95.0f, 160.0f }; int spawned = 0;
                    for (int k = 0; k < MAX_BOSS_ORBS && spawned < 6; k++)
                    {
                        if (!bossOrbActive[k])
                        {
                            bossOrbActive[k] = true; bossOrbPos[k] = (Vector2){ bossPos.x + BossWidth / 2.0f, bossPos.y + BossHeight - 15.0f };
                            bossOrbVel[k] = (Vector2){ spreadVx[spawned], 270.0f }; spawned++;
                        }
                    }
                }
            }

            // Boss Bullet Damage
            for (int k = 0; k < MAX_BOSS_BULLETS; k++)
            {
                if (bossBulletActive[k] && !isFrozen && !deathSequenceActive && !GameOver && !bossDefeated)
                {
                    bossBulletPos[k].y += AlienBulletSpeedY * Time;
                    if (bossBulletPos[k].y > WindowHeight) bossBulletActive[k] = false;
                    else if (!teamShieldActive && !evacActive)
                    {
                        Rectangle bRec = { bossBulletPos[k].x - AlienBulletWidth / 2.0f, bossBulletPos[k].y, AlienBulletWidth, AlienBulletHeight };
                        Rectangle h1Rec = { Hero1Pos.x - HeroWidth / 2.0f, Hero1Pos.y, HeroWidth, HeroHeight };
                        Rectangle h2Rec = { Hero2Pos.x - HeroWidth / 2.0f, Hero2Pos.y, HeroWidth, HeroHeight };

                        if (Hero1Lives > 0 && CheckCollisionRecs(bRec, h1Rec))
                        {
                            bossBulletActive[k] = false; Hero1Lives--; Hero1HitsTaken++; hero1HitFlashTimer = 0.35f; PlaySound(heroOuch); PlaySound(sndHudGlitch);
                            if (Hero1Lives <= 0)
                            {
                                Hero1CrashPos = Hero1Pos; PlaySound(heroDeath);
                                if (Hero2Lives > 0) { hero2OverdriveTimer = 2.5f; PlaySound(sndShieldActivate); }
                                if (Hero2Lives <= 0)
                                {
                                    deathSequenceActive = true; deathDelayTimer = 0.0f;
                                    StopSound(AlienShoot); StopSound(sndAlienStep);
                                }
                            }
                        }
                        else if (Hero2Lives > 0 && CheckCollisionRecs(bRec, h2Rec))
                        {
                            bossBulletActive[k] = false; Hero2Lives--; Hero2HitsTaken++; hero2HitFlashTimer = 0.35f; PlaySound(heroOuch); PlaySound(sndHudGlitch);
                            if (Hero2Lives <= 0)
                            {
                                Hero2CrashPos = Hero2Pos; PlaySound(heroDeath);
                                if (Hero1Lives > 0) { hero1OverdriveTimer = 2.5f; PlaySound(sndShieldActivate); }
                                if (Hero1Lives <= 0)
                                {
                                    deathSequenceActive = true; deathDelayTimer = 0.0f;
                                    StopSound(AlienShoot); StopSound(sndAlienStep);
                                }
                            }
                        }
                    }
                }
            }

            // Boss Orb Debuff(visuals taken from airStrike game!!!!!!!!!!!!)
            for (int k = 0; k < MAX_BOSS_ORBS; k++)
            {
                if (bossOrbActive[k] && !isFrozen && !deathSequenceActive && !GameOver && !bossDefeated)
                {
                    bossOrbPos[k] = Vector2Add(bossOrbPos[k], Vector2Scale(bossOrbVel[k], Time));
                    if (bossOrbPos[k].y > WindowHeight || bossOrbPos[k].x < 0 || bossOrbPos[k].x > WindowWidth) bossOrbActive[k] = false;
                    else if (!teamShieldActive && !evacActive)
                    {
                        Rectangle h1Rec = { Hero1Pos.x - HeroWidth / 2.0f, Hero1Pos.y, HeroWidth, HeroHeight };
                        Rectangle h2Rec = { Hero2Pos.x - HeroWidth / 2.0f, Hero2Pos.y, HeroWidth, HeroHeight };

                        if (Hero1Lives > 0 && CheckCollisionCircleRec(bossOrbPos[k], 12.0f, h1Rec))
                        {
                            bossOrbActive[k] = false; hero1Debuffed = true; hero1DebuffTimer = 4.5f; Hero1HitsTaken++;
                            hero1HitFlashTimer = 0.35f; PlaySound(heroOuch); PlaySound(sndHudGlitch);
                        }
                        else if (Hero2Lives > 0 && CheckCollisionCircleRec(bossOrbPos[k], 12.0f, h2Rec))
                        {
                            bossOrbActive[k] = false; hero2Debuffed = true; hero2DebuffTimer = 4.5f; Hero2HitsTaken++;
                            hero2HitFlashTimer = 0.35f; PlaySound(heroOuch); PlaySound(sndHudGlitch);
                        }
                    }
                }
            }

            // Boss Laser Damage
            if (bossLaserActive && !deathSequenceActive && !evacActive && !bossDefeated && !isFrozen && !GameOver)
            {
                float laserW = 42.0f, laserX = bossPos.x + (BossWidth - laserW) / 2.0f, laserY = bossPos.y + BossHeight - 10.0f;
                float laserH = (teamShieldActive) ? ((domeBaseY - domeRadius) - laserY) : (WindowHeight - laserY);
                if (laserH < 0.0f) laserH = 0.0f;
                Rectangle laserRec = { laserX, laserY, laserW, laserH };
                Rectangle h1Rec = { Hero1Pos.x - HeroWidth / 2.0f, Hero1Pos.y, HeroWidth, HeroHeight };
                Rectangle h2Rec = { Hero2Pos.x - HeroWidth / 2.0f, Hero2Pos.y, HeroWidth, HeroHeight };

                bool hit1 = (Hero1Lives > 0 && CheckCollisionRecs(laserRec, h1Rec) && !teamShieldActive);
                bool hit2 = (Hero2Lives > 0 && CheckCollisionRecs(laserRec, h2Rec) && !teamShieldActive);

                if (hit1) { Hero1Lives = 0; Hero1CrashPos = Hero1Pos; Hero1HitsTaken += 4; hero1HitFlashTimer = 0.45f; PlaySound(heroOuch); PlaySound(sndHudGlitch); }
                if (hit2) { Hero2Lives = 0; Hero2CrashPos = Hero2Pos; Hero2HitsTaken += 4; hero2HitFlashTimer = 0.45f; PlaySound(heroOuch); PlaySound(sndHudGlitch); }
                if (hit1 || hit2)
                {
                    PlaySound(heroDeath);
                    if (Hero1Lives <= 0 && Hero2Lives <= 0)
                    {
                        deathSequenceActive = true; deathDelayTimer = 0.0f;
                        StopSound(bossLaserSound); screenCamera.offset = (Vector2){ 0, 0 };
                        StopSound(AlienShoot); StopSound(sndAlienStep);
                        StopSound(sndBlackHoleDrone);
                        StopSound(sndEntityDrone);
                    }
                }
            }

            // ALIEN GRID SINGLE BOUNCE CHECK PER FRAME (Gated: Frozen and silenced when dead or game over)-by Shoab!!!!!! insane debug!!!
            if (!bossSpawned)
            {
                if (!isFrozen && !deathSequenceActive && !GameOver && !bossDefeated)
                {
                    bool hitWall = false;
                    for (int X = 0; X < AlienInX && !hitWall; X++)
                    {
                        for (int Y = 0; Y < AlienInY; Y++)
                        {
                            if (AlienAlive[X][Y])
                            {
                                if ((AlienPos[X][Y].x + AlienSize >= WindowWidth - 10 && AlienSpeed.x > 0) || (AlienPos[X][Y].x <= 10 && AlienSpeed.x < 0))
                                {
                                    hitWall = true; break;
                                }
                            }
                        }
                    }
                    if (hitWall)
                    {
                        AlienSpeed.x *= -1.0f;
                        DownAlien(AlienInX, AlienInY, AlienPos, AlienAlive);
                        PlaySound(sndAlienStep);
                    }

                    for (int X = 0; X < AlienInX; X++)
                    {
                        for (int Y = 0; Y < AlienInY; Y++)
                        {
                            float speedMod = (AlienSpeed.x > 0) ? (AlienSpeed.x + SpeedBuff[Y]) : (AlienSpeed.x - SpeedBuff[Y]);
                            AlienPos[X][Y].x += speedMod * Time;
                        }
                    }
                }

                // Render Alien Swarm-shoab!!!!!!!!
                for (int X = 0; X < AlienInX; X++)
                {
                    for (int Y = 0; Y < AlienInY; Y++)
                    {
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
                    DrawRectangle((int)(Hero1BulletPos[i].x - BulletWidth / 2.0f), (int)Hero1BulletPos[i].y, BulletWidth, BulletHeight, (empBuffTimer > 0.0f || hero1OverdriveTimer > 0.0f) ? LIME : YELLOW);
                if (Hero2BulletActive[i])
                    DrawRectangle((int)(Hero2BulletPos[i].x - BulletWidth / 2.0f), (int)Hero2BulletPos[i].y, BulletWidth, BulletHeight, (empBuffTimer > 0.0f || hero2OverdriveTimer > 0.0f) ? WHITE : SKYBLUE);
            }
            
            // DRAW SHOAB'S SPECIAL PIERCING BEAM
            if (Hero1SpecialBulletActive)
            {
                int sFrame = ((int)(GetTime() * 18.0f)) % 7;
                Rectangle Hero1SpecialBulletRec = { Hero1SpecialBulletPos.x - HeroWidth / 2.0f, Hero1SpecialBulletPos.y, HeroWidth, HeroWidth * 3 };
                
                BeginBlendMode(BLEND_ADDITIVE);
                DrawTexturePro(Hero1SpecialBulletTex[sFrame], (Rectangle){ 0, 0, (float)Hero1SpecialBulletTex[sFrame].width, (float)Hero1SpecialBulletTex[sFrame].height }, Hero1SpecialBulletRec, (Vector2){ 0, 0 }, 0.0f, WHITE);
                EndBlendMode();
            }

            // DRAW ASTEROIDS and some space features....
            for (int a = 0; a < MAX_ASTEROIDS; a++)
            {
                if (asteroids[a].active)
                {
                    float d = asteroids[a].radius * 2.0f;
                    if (texAsteroid.id > 0)
                    {
                        DrawTexturePro(texAsteroid, (Rectangle){ 0, 0, (float)texAsteroid.width, (float)texAsteroid.height },
                                       (Rectangle){ asteroids[a].pos.x, asteroids[a].pos.y, d, d },
                                       (Vector2){ asteroids[a].radius, asteroids[a].radius }, asteroids[a].rotation, WHITE);
                    }
                    else
                    {
                        DrawCircle((int)asteroids[a].pos.x, (int)asteroids[a].pos.y, asteroids[a].radius, DARKGRAY);
                        DrawCircleLines((int)asteroids[a].pos.x, (int)asteroids[a].pos.y, asteroids[a].radius, GRAY);
                    }
                }
            }

            for (int dp = 0; dp < MAX_ASTEROID_DEBRIS; dp++)
            {
                if (debrisPool[dp].active)
                {
                    float alpha = debrisPool[dp].life / debrisPool[dp].maxLife;
                    if (texDebris.id > 0)
                    {
                        DrawTexturePro(texDebris, (Rectangle){ 0, 0, (float)texDebris.width, (float)texDebris.height },
                                       (Rectangle){ debrisPool[dp].pos.x, debrisPool[dp].pos.y, 16, 16 },
                                       (Vector2){ 8, 8 }, debrisPool[dp].rotation, Fade(WHITE, alpha));
                    }
                    else
                    {
                        DrawCircle((int)debrisPool[dp].pos.x, (int)debrisPool[dp].pos.y, 3.0f, Fade(GRAY, alpha));
                    }
                }
            }

            // RENDER BLACK HOLE IF ACTIVE (With smooth scale and fade-out alpha)
            if (blackHole.active)
            {
                BeginBlendMode(BLEND_ADDITIVE);
                if (texBlackHoleDisk.id > 0)
                {
                    DrawTexturePro(texBlackHoleDisk, (Rectangle){ 0, 0, (float)texBlackHoleDisk.width, (float)texBlackHoleDisk.height },
                                   (Rectangle){ blackHole.pos.x, blackHole.pos.y, 440.0f * blackHole.scale, 440.0f * blackHole.scale },
                                   (Vector2){ 220.0f * blackHole.scale, 220.0f * blackHole.scale }, blackHole.rotationAngle, Fade(WHITE, 0.85f * blackHole.alpha));
                    DrawTexturePro(texBlackHoleDisk, (Rectangle){ 0, 0, (float)texBlackHoleDisk.width, (float)texBlackHoleDisk.height },
                                   (Rectangle){ blackHole.pos.x, blackHole.pos.y, 360.0f * blackHole.scale, 360.0f * blackHole.scale },
                                   (Vector2){ 180.0f * blackHole.scale, 180.0f * blackHole.scale }, -blackHole.rotationAngle * 1.4f, Fade(PURPLE, 0.70f * blackHole.alpha));
                }
                EndBlendMode();

                if (texBlackHoleCore.id > 0)
                {
                    DrawTexturePro(texBlackHoleCore, (Rectangle){ 0, 0, (float)texBlackHoleCore.width, (float)texBlackHoleCore.height },
                                   (Rectangle){ blackHole.pos.x, blackHole.pos.y, 140.0f * blackHole.scale, 140.0f * blackHole.scale },
                                   (Vector2){ 70.0f * blackHole.scale, 70.0f * blackHole.scale }, 0.0f, Fade(WHITE, blackHole.alpha));
                }
            }

            // RENDER MYSTERIOUS VOID ENTITY (PHANTOM) & ORBS IF ACTIVE
            if (voidEntity.active)
            {
                if (texEntityPhantom[voidEntity.currentFrame].id > 0)
                {
                    DrawTexturePro(texEntityPhantom[voidEntity.currentFrame],
                                   (Rectangle){ 0, 0, (float)texEntityPhantom[voidEntity.currentFrame].width, (float)texEntityPhantom[voidEntity.currentFrame].height },
                                   (Rectangle){ voidEntity.pos.x, voidEntity.pos.y, 120.0f, 120.0f },
                                   (Vector2){ 60.0f, 60.0f }, 0.0f, Fade(WHITE, voidEntity.alpha));
                }
                else
                {
                    DrawCircle((int)voidEntity.pos.x, (int)voidEntity.pos.y, 38.0f, Fade(PURPLE, 0.75f));
                    DrawCircleLines((int)voidEntity.pos.x, (int)voidEntity.pos.y, 44.0f, Fade(WHITE, 0.85f));
                }

                // Entity Health Bar
                int eBarW = 80, eBarH = 6;
                DrawRectangle((int)voidEntity.pos.x - eBarW / 2, (int)voidEntity.pos.y - 70, eBarW, eBarH, DARKGRAY);
                DrawRectangle((int)voidEntity.pos.x - eBarW / 2, (int)voidEntity.pos.y - 70, (int)(eBarW * ((float)voidEntity.hp / voidEntity.maxHp)), eBarH, (Color){ 200, 60, 255, 255 });
                DrawRectangleLines((int)voidEntity.pos.x - eBarW / 2, (int)voidEntity.pos.y - 70, eBarW, eBarH, WHITE);
            }

            // Render Void Distortion Orbs
            for (int eo = 0; eo < MAX_ENTITY_ORBS; eo++)
            {
                if (entityOrbs[eo].active)
                {
                    if (texEntityOrb.id > 0)
                    {
                        DrawTexturePro(texEntityOrb,
                                       (Rectangle){ 0, 0, (float)texEntityOrb.width, (float)texEntityOrb.height },
                                       (Rectangle){ entityOrbs[eo].pos.x, entityOrbs[eo].pos.y, 32.0f, 32.0f },
                                       (Vector2){ 16.0f, 16.0f }, (float)GetTime() * 180.0f, WHITE);
                    }
                    else
                    {
                        DrawCircle((int)entityOrbs[eo].pos.x, (int)entityOrbs[eo].pos.y, 12.0f, (Color){ 180, 40, 255, 255 });
                        DrawCircleLines((int)entityOrbs[eo].pos.x, (int)entityOrbs[eo].pos.y, 14.0f, WHITE);
                    }
                }
            }

            for (int m = 0; m < MAX_CLUSTER_MISSILES; m++)
            {
                if (clusterMissileActive[m])
                {
                    float rotAngle = atan2f(clusterMissileVel[m].y, clusterMissileVel[m].x) * RAD2DEG + 90.0f;
                    if (clusterBombTex.id > 0)
                        DrawTexturePro(clusterBombTex, (Rectangle){ 0.0f, 0.0f, (float)clusterBombTex.width, (float)clusterBombTex.height },
                                       (Rectangle){ clusterMissilePos[m].x, clusterMissilePos[m].y, 14.0f, 28.0f }, (Vector2){ 7.0f, 14.0f }, rotAngle, WHITE);
                    else
                        DrawRectangle((int)clusterMissilePos[m].x - 3, (int)clusterMissilePos[m].y - 7, 6, 14, SKYBLUE);
                }
            }

            // DRAWings of cluster missiles for Nayemul!!!
            for (int b = 0; b < MAX_CLUSTER_BLASTS; b++)
            {
                if (clusterBlastActive[b])
                {
                    clusterBlastTimer[b] -= Time; clusterBlastRadius[b] += 130.0f * Time;
                    float alpha = clusterBlastTimer[b] / 0.25f;
                    if (clusterBlastTimer[b] <= 0.0f) clusterBlastActive[b] = false;
                    else
                    {
                        float diameter = clusterBlastRadius[b] * 2.0f;
                        BeginBlendMode(BLEND_ADDITIVE);
                        if (clusterBlastTex.id > 0)
                            DrawTexturePro(clusterBlastTex, (Rectangle){ 0.0f, 0.0f, (float)clusterBlastTex.width, (float)clusterBlastTex.height },
                                           (Rectangle){ clusterBlastPos[b].x, clusterBlastPos[b].y, diameter, diameter }, (Vector2){ diameter / 2.0f, diameter / 2.0f }, (float)(GetTime() * 180.0f), Fade(WHITE, alpha));
                        else
                        {
                            DrawCircle((int)clusterBlastPos[b].x, (int)clusterBlastPos[b].y, clusterBlastRadius[b], Fade(SKYBLUE, alpha * 0.6f));
                            DrawCircleLines((int)clusterBlastPos[b].x, (int)clusterBlastPos[b].y, clusterBlastRadius[b], Fade(WHITE, alpha * 0.85f));
                        }
                        EndBlendMode();
                    }
                }
            }

            if (AlienBulletActive) DrawRectangle((int)(AlienBulletPos.x - AlienBulletWidth / 2.0f), (int)AlienBulletPos.y, AlienBulletWidth, AlienBulletHeight, RED);
            for (int b = 0; b < MAX_COMMANDER_BULLETS; b++)
            {
                if (jammerBulletActive[b])
                {
                    DrawRectangle((int)(jammerBulletPos[b].x - 4), (int)jammerBulletPos[b].y, 8, 22, (Color){ 200, 70, 255, 255 });
                    float pulseAura = jammerBulletLocked[b] ? 4.0f : (6.0f + sinf((float)GetTime() * 15.0f) * 2.5f);
                    DrawCircle((int)jammerBulletPos[b].x, (int)jammerBulletPos[b].y + 11, pulseAura, Fade((Color){ 230, 120, 255, 255 }, jammerBulletLocked[b] ? 0.35f : 0.70f));
                }
                if (warpBulletActive[b])
                {
                    DrawRectangle((int)(warpBulletPos[b].x - 3), (int)warpBulletPos[b].y, 6, 26, SKYBLUE);
                    DrawRectangle((int)(warpBulletPos[b].x - 1), (int)warpBulletPos[b].y + 4, 2, 18, WHITE);
                }
            }

            if (bossActive)
            {
                for (int mb = 0; mb < MAX_MINION_BULLETS; mb++)
                    if (minionBulletActive[mb]) DrawRectangle((int)(minionBulletPos[mb].x - AlienBulletWidth / 2.0f), (int)minionBulletPos[mb].y, AlienBulletWidth, AlienBulletHeight, ORANGE);
            }

            if (empActive) //trigger logics :)
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
                    BeginBlendMode(BLEND_ADDITIVE);
                    DrawCircleLines((int)empShockwaveCenter.x, (int)empShockwaveCenter.y, empShockwaveRadius, Fade(SKYBLUE, shockFade));
                    DrawCircleLines((int)empShockwaveCenter.x, (int)empShockwaveCenter.y, empShockwaveRadius + 4.0f, Fade(WHITE, shockFade * 0.7f));
                    EndBlendMode();
                }
            }

            if (jammerActive)
            {
                DrawTexturePro(jammerTex[jammerAnimState], (Rectangle){ 0, 0, (float)jammerTex[jammerAnimState].width, (float)jammerTex[jammerAnimState].height },
                               (Rectangle){ jammerPos.x, jammerPos.y, COMMANDER_SIZE, COMMANDER_SIZE }, (Vector2){ 0, 0 }, 0.0f, WHITE);
                DrawRectangle((int)jammerPos.x + 8, (int)jammerPos.y - 10, (int)((COMMANDER_SIZE - 16) * ((float)jammerHp / jammerMaxHp)), 5, (Color){ 200, 80, 255, 255 });
                DrawRectangleLines((int)jammerPos.x + 8, (int)jammerPos.y - 10, COMMANDER_SIZE - 16, 5, WHITE);
            }
            if (warpActive)
            {
                DrawTexturePro(warpTex[warpAnimState], (Rectangle){ 0, 0, (float)warpTex[warpAnimState].width, (float)warpTex[warpAnimState].height },
                               (Rectangle){ warpPos.x, warpPos.y, COMMANDER_SIZE, COMMANDER_SIZE }, (Vector2){ 0, 0 }, 0.0f, WHITE);
                DrawRectangle((int)warpPos.x + 8, (int)warpPos.y - 10, (int)((COMMANDER_SIZE - 16) * ((float)warpHp / warpMaxHp)), 5, SKYBLUE);
                DrawRectangleLines((int)warpPos.x + 8, (int)warpPos.y - 10, COMMANDER_SIZE - 16, 5, WHITE);
            }

            if (commanderIntroActive)
            {
                float vignettePulse = fabsf(sinf((float)GetTime() * 10.0f));
                DrawRectangle(0, 0, WindowWidth, WindowHeight, Fade(DARKPURPLE, 0.18f + 0.10f * vignettePulse));
                for (int y = 0; y < WindowHeight; y += 45)
                    DrawLine(0, y, WindowWidth, y, Fade(SKYBLUE, ((int)(GetTime() * 20.0f) % 2 == 0) ? 0.25f : 0.08f));
                DrawCircleLines((int)(WindowWidth * 0.25f), 130 + COMMANDER_SIZE / 2, 38.0f, Fade((Color){ 200, 80, 255, 255 }, vignettePulse));
                DrawCircleLines((int)(WindowWidth * 0.75f), 130 + COMMANDER_SIZE / 2, 38.0f, Fade(SKYBLUE, vignettePulse));
            }

            // Dreadnought Hyperspace Drop logic and visuals!!!-Nayemul!!!!
            if (bossWarpActive)
            {
                if (bossWarpTimer > 0.8f)
                {
                    float t = (2.5f - bossWarpTimer) / 1.7f;
                    if (t < 0.0f) t = 0.0f; if (t > 1.0f) t = 1.0f;
                    float slitW = 100.0f * (1.0f - t) + 6.0f;
                    float slitH = 30.0f + t * ((float)WindowHeight - 30.0f);
                    float slitY = (WindowHeight - slitH) / 2.0f;

                    DrawRectangle((int)(750.0f - slitW / 2.0f - 4.0f), (int)slitY, (int)slitW, (int)slitH, Fade(RED, 0.55f));
                    DrawRectangle((int)(750.0f - slitW / 2.0f + 4.0f), (int)slitY, (int)slitW, (int)slitH, Fade(SKYBLUE, 0.55f));
                    DrawRectangle((int)(750.0f - slitW / 2.0f), (int)slitY, (int)slitW, (int)slitH, Fade(WHITE, 0.95f));
                }
                else
                {
                    float p = (0.8f - bossWarpTimer) / 0.8f;
                    if (p < 0.0f) p = 0.0f; if (p > 1.0f) p = 1.0f;
                    float scaleX = 0.2f + 0.8f * p;
                    float scaleY = 2.5f - 1.5f * p;
                    float curW = BossWidth * scaleX;
                    float curH = BossHeight * scaleY;
                    float curX = 750.0f - curW / 2.0f;
                    float curY = 60.0f + (BossHeight - curH) / 2.0f;

                    DrawTexturePro(BossTexture[0], (Rectangle){ 0, 0, (float)BossTexture[0].width, (float)BossTexture[0].height },
                                   (Rectangle){ curX, curY, curW, curH }, (Vector2){ 0, 0 }, 0.0f, WHITE);

                    Vector2 warpCenter = { 750.0f, 60.0f + BossHeight / 2.0f };
                    for (int w = 1; w <= 3; w++)
                    {
                        float shockRadius = (p * 450.0f) + (w * 60.0f);
                        float alpha = (1.0f - p) * 0.7f;
                        DrawCircleLines((int)warpCenter.x, (int)warpCenter.y, shockRadius, Fade(SKYBLUE, alpha));
                        DrawCircleLines((int)warpCenter.x, (int)warpCenter.y, shockRadius + 2.0f, Fade(WHITE, alpha * 0.7f));
                    }
                }
            }

            // Dreadnought Evacuation Sequence Visuals and cut into half visuals-Nayemul!!!
            if (evacActive)
            {
                if (evacTimer <= 3.4f)
                {
                    float fractureTime = 3.4f - evacTimer;
                    float halfW = BossWidth / 2.0f;
                    float texHalfW = (float)BossTexture[0].width / 2.0f;
                    float texH = (float)BossTexture[0].height;

                    Rectangle srcL = { 0, 0, texHalfW, texH };
                    Vector2 posL = { (bossPos.x + halfW / 2.0f) - fractureTime * 65.0f, bossPos.y + BossHeight / 2.0f };
                    float rotL = -fractureTime * 12.0f;
                    DrawTexturePro(BossTexture[0], srcL, (Rectangle){ posL.x, posL.y, halfW, (float)BossHeight }, (Vector2){ halfW / 2.0f, BossHeight / 2.0f }, rotL, WHITE);

                    Rectangle srcR = { texHalfW, 0, texHalfW, texH };
                    Vector2 posR = { (bossPos.x + BossWidth - halfW / 2.0f) + fractureTime * 65.0f, bossPos.y + BossHeight / 2.0f };
                    float rotR = fractureTime * 12.0f;
                    DrawTexturePro(BossTexture[0], srcR, (Rectangle){ posR.x, posR.y, halfW, (float)BossHeight }, (Vector2){ halfW / 2.0f, BossHeight / 2.0f }, rotR, WHITE);

                    for (int sp = 0; sp < 8; sp++)
                    {
                        float midX = (posL.x + posR.x) / 2.0f + (float)GetRandomValue(-25, 25);
                        float midY = bossPos.y + (float)GetRandomValue(0, BossHeight);
                        float rRadius = (float)GetRandomValue(8, 26);
                        Color rCol = (sp % 2 == 0) ? Fade(ORANGE, 0.75f) : Fade(RED, 0.75f);
                        DrawCircle((int)midX, (int)midY, rRadius, rCol);
                        DrawCircleLines((int)midX, (int)midY, rRadius + 3.0f, Fade(YELLOW, 0.6f));
                    }
                }

                if (evacTimer <= 2.2f)
                {
                    float climbProgress = (2.2f - evacTimer) / 2.2f;
                    if (climbProgress < 0.0f) climbProgress = 0.0f;
                    if (climbProgress > 1.0f) climbProgress = 1.0f;

                    float scale = 1.0f + climbProgress * 2.8f;
                    float shipBaseY = WindowHeight - HeroHeight;
                    float climbY = shipBaseY - climbProgress * 700.0f;

                    float targetCenterX1 = WindowWidth * 0.44f;
                    float targetCenterX2 = WindowWidth * 0.56f;
                    float climbX1 = Hero1Pos.x + (targetCenterX1 - Hero1Pos.x) * climbProgress;
                    float climbX2 = Hero2Pos.x + (targetCenterX2 - Hero2Pos.x) * climbProgress;

                    float scaledW = HeroWidth * scale;
                    float scaledH = HeroHeight * scale;

                    if (Hero1Lives > 0)
                    {
                        DrawTexturePro(HeroTexture, (Rectangle){ 0, 0, (float)HeroTexture.width, (float)HeroTexture.height },
                                       (Rectangle){ climbX1, climbY, scaledW, scaledH },
                                       (Vector2){ scaledW / 2.0f, scaledH / 2.0f }, 0.0f, WHITE);
                    }
                    if (Hero2Lives > 0)
                    {
                        DrawStealthFlames((Vector2){ climbX2, climbY }, scaledW, scaledH);
                        DrawTexturePro(StealthHeroTexture, (Rectangle){ 0, 0, (float)StealthHeroTexture.width, (float)StealthHeroTexture.height },
                                       (Rectangle){ climbX2, climbY, scaledW, scaledH },
                                       (Vector2){ scaledW / 2.0f, scaledH / 2.0f }, 0.0f, WHITE);
                    }
                }
            }

            if (bossActive)
            {
                int bFrame = (bossSpeed.y > 25.0f) ? 1 : ((bossSpeed.y < -25.0f) ? 2 : (((int)(bossAnimTimer / 0.35f)) % 4 == 1 ? 3 : (((int)(bossAnimTimer / 0.35f)) % 4 == 3 ? 4 : 0)));
                
                Color bossTint = (bossHp <= 40) ? (Color){ 255, 120, 120, 255 } : WHITE;
                if (bossHitFlashTimer > 0.0f) bossTint = (Color){ 255, 255, 255, 220 };

                DrawTexturePro(BossTexture[bFrame], (Rectangle){ 0, 0, (float)BossTexture[bFrame].width, (float)BossTexture[bFrame].height },
                               (Rectangle){ bossPos.x, bossPos.y, BossWidth, BossHeight }, (Vector2){ 0, 0 }, 0.0f, bossTint);

                int AStyle = (int)(GetTime() / 0.1) % AlienSpriteStyle[19];
                for (int m = 0; m < MAX_MINIONS; m++)
                {
                    if (minionActive[m])
                    {
                        DrawTexturePro(AlienTexture[19], (Rectangle){ AStyle * (AlienSWidth[19] + 1), 0, (float)(AlienSWidth[19]), (float)AlienSHeight[19] },
                                       (Rectangle){ minionPos[m].x, minionPos[m].y, MinionSize, MinionSize }, (Vector2){ 0, 0 }, 0.0f, WHITE);
                        DrawRectangle((int)minionPos[m].x + 10, (int)minionPos[m].y - 8, (int)((MinionSize - 20) * (minionHp[m] / 2.0f)), 4, LIME);
                    }
                }

                if (bossHp <= 40)
                {
                    DrawLineEx((Vector2){ bossPos.x + 90, bossPos.y + 40 }, (Vector2){ bossPos.x + 130, bossPos.y + 110 }, 2.5f, MAROON);
                    DrawLineEx((Vector2){ bossPos.x + 130, bossPos.y + 110 }, (Vector2){ bossPos.x + 170, bossPos.y + 70 }, 2.0f, RED);
                    DrawLineEx((Vector2){ bossPos.x + 200, bossPos.y + 50 }, (Vector2){ bossPos.x + 160, bossPos.y + 140 }, 2.5f, BLACK);

                    for (int s = 0; s < 4; s++)
                        DrawCircle((int)(bossPos.x + 60 + (s * 55) + GetRandomValue(-8, 8)), (int)(bossPos.y + 40 + GetRandomValue(-10, 20)), (float)GetRandomValue(8, 16), Fade(DARKGRAY, 0.40f));
                    for (int sp = 0; sp < 5; sp++)
                        DrawLineEx((Vector2){ bossPos.x + GetRandomValue(30, BossWidth - 30), bossPos.y + GetRandomValue(30, BossHeight - 40) },
                                   (Vector2){ bossPos.x + GetRandomValue(30, BossWidth - 30), bossPos.y + GetRandomValue(30, BossHeight - 40) }, 2.0f, YELLOW);
                }

                if (bossLeftPodHp > 0 && bossLeftPodHp < 25)
                {
                    if (GetRandomValue(0, 3) == 0)
                    {
                        Vector2 sp = { bossPos.x + 8 + (float)GetRandomValue(0, 75), bossPos.y + 70 + (float)GetRandomValue(0, 110) };
                        DrawLineEx(sp, (Vector2){ sp.x + (float)GetRandomValue(-15, 15), sp.y + (float)GetRandomValue(-15, 15) }, 2.0f, ORANGE);
                    }
                }
                if (bossRightPodHp > 0 && bossRightPodHp < 25)
                {
                    if (GetRandomValue(0, 3) == 0)
                    {
                        Vector2 sp = { bossPos.x + BossWidth - 83 + (float)GetRandomValue(0, 75), bossPos.y + 70 + (float)GetRandomValue(0, 110) };
                        DrawLineEx(sp, (Vector2){ sp.x + (float)GetRandomValue(-15, 15), sp.y + (float)GetRandomValue(-15, 15) }, 2.0f, MAGENTA);
                    }
                }

                if (bossLeftPodHp > 0)
                {
                    DrawRectangleLines(bossPos.x + 8, bossPos.y + 185, 75, 8, DARKGRAY);
                    DrawRectangle(bossPos.x + 8, bossPos.y + 185, (int)(75 * ((float)bossLeftPodHp / BossPodMaxHp)), 8, RED);
                    DrawText(TextFormat("SHIELD: %d", bossLeftPodHp), bossPos.x + 10, bossPos.y + 196, 11, RED);
                }
                else DrawText("BROKEN", bossPos.x + 16, bossPos.y + 190, 12, DARKGRAY);

                if (bossRightPodHp > 0)
                {
                    DrawRectangleLines(bossPos.x + BossWidth - 83, bossPos.y + 185, 75, 8, DARKGRAY);
                    DrawRectangle(bossPos.x + BossWidth - 83, bossPos.y + 185, (int)(75 * ((float)bossRightPodHp / BossPodMaxHp)), 8, MAGENTA);
                    DrawText(TextFormat("SHIELD: %d", bossRightPodHp), bossPos.x + BossWidth - 81, bossPos.y + 196, 11, MAGENTA);
                }
                else DrawText("BROKEN", bossPos.x + BossWidth - 72, bossPos.y + 190, 12, DARKGRAY);

                if (bossLeftPodHp > 0 || bossRightPodHp > 0)
                {
                    DrawCircleLines((int)(bossPos.x + BossWidth / 2.0f), (int)(bossPos.y + 100), 55.0f, Fade(SKYBLUE, 0.45f));
                    DrawCircleLines((int)(bossPos.x + BossWidth / 2.0f), (int)(bossPos.y + 100), 58.0f, Fade(SKYBLUE, 0.25f));
                }

                for (int k = 0; k < MAX_BOSS_BULLETS; k++)
                    if (bossBulletActive[k]) DrawRectangle((int)(bossBulletPos[k].x - AlienBulletWidth / 2.0f), (int)bossBulletPos[k].y, AlienBulletWidth, AlienBulletHeight, RED);

                // Laser Implosion throughtout staright window!!!!
                if (!bossLaserActive && bossLaserTimer >= 6.2f && bossLaserTimer < 7.0f)
                {
                    if (!laserChargeSoundPlayed)
                    {
                        PlaySound(sndLaserCharge);
                        laserChargeSoundPlayed = true;
                    }

                    float laserW = 42.0f, guideX = bossPos.x + BossWidth / 2.0f, guideY = bossPos.y + BossHeight - 10.0f;
                    float telegraphAlpha = (float)GetRandomValue(25, 65) / 100.0f;
                    DrawLineEx((Vector2){ guideX, guideY }, (Vector2){ guideX, (float)WindowHeight }, 2.5f, Fade(RED, telegraphAlpha));
                    DrawRectangle((int)(guideX - laserW / 2.0f), (int)guideY, (int)laserW, WindowHeight - (int)guideY, Fade(RED, telegraphAlpha * 0.18f));

                    float suckProgress = (bossLaserTimer - 6.2f) / 0.8f;
                    for (int r = 1; r <= 3; r++)
                    {
                        float ringRadius = (1.0f - suckProgress) * (70.0f * r);
                        DrawCircleLines((int)guideX, (int)guideY, ringRadius, Fade(RED, suckProgress * 0.85f));
                    }

                    const char* warnBeam = ">> DANGER: LASER ALIGNING <<";
                    DrawText(warnBeam, (int)(guideX - MeasureText(warnBeam, 16) / 2.0f), (int)(guideY + 30.0f), 16, YELLOW);
                }

                // Draw Active Laser Beam;sprites removed for it ;)
                if (bossLaserActive)
                {
                    laserChargeSoundPlayed = false;
                    float laserW = 42.0f + (float)GetRandomValue(-4, 5);
                    float laserX = bossPos.x + (BossWidth - laserW) / 2.0f, laserY = bossPos.y + BossHeight - 10.0f;
                    float laserH = (teamShieldActive) ? ((domeBaseY - domeRadius) - laserY) : (WindowHeight - laserY);
                    if (laserH < 0.0f) laserH = 0.0f;

                    BeginBlendMode(BLEND_ADDITIVE);
                    DrawRectangle((int)laserX - 16, (int)laserY, (int)laserW + 32, (int)laserH, Fade(RED, 0.35f));
                    DrawRectangle((int)laserX - 8, (int)laserY, (int)laserW + 16, (int)laserH, Fade(RED, 0.65f));
                    DrawRectangle((int)laserX, (int)laserY, (int)laserW, (int)laserH, Fade(ORANGE, 0.90f));
                    DrawRectangle((int)laserX + 8, (int)laserY, (int)laserW - 16, (int)laserH, WHITE);
                    EndBlendMode();
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

            // DRAW SMOKE and ELECTRICAL SPARK PARTICLES visuals,by Nayemul!!!
            for (int s = 0; s < MAX_SMOKE_PARTICLES; s++)
            {
                if (smokePool[s].active)
                {
                    float alpha = smokePool[s].life / smokePool[s].maxLife;
                    if (texSmoke.id > 0)
                    {
                        DrawTexturePro(texSmoke, (Rectangle){ 0, 0, (float)texSmoke.width, (float)texSmoke.height },
                                       (Rectangle){ smokePool[s].pos.x, smokePool[s].pos.y, smokePool[s].size, smokePool[s].size },
                                       (Vector2){ smokePool[s].size / 2.0f, smokePool[s].size / 2.0f }, 0.0f, Fade(WHITE, alpha * 0.75f));
                    }
                    else DrawCircle((int)smokePool[s].pos.x, (int)smokePool[s].pos.y, smokePool[s].size / 2.0f, Fade(DARKGRAY, alpha * 0.5f));
                }
            }

            BeginBlendMode(BLEND_ADDITIVE);
            for (int sp = 0; sp < MAX_SPARK_PARTICLES; sp++)
            {
                if (sparkPool[sp].active)
                {
                    float alpha = sparkPool[sp].life / sparkPool[sp].maxLife; //taken help from both cheatsheet of raylib and airStrike game.....
                    if (texSpark.id > 0)
                    {
                        DrawTexturePro(texSpark, (Rectangle){ 0, 0, (float)texSpark.width, (float)texSpark.height },
                                       (Rectangle){ sparkPool[sp].pos.x, sparkPool[sp].pos.y, 16, 16 },
                                       (Vector2){ 8, 8 }, 0.0f, Fade(WHITE, alpha));
                    }
                    else DrawCircle((int)sparkPool[sp].pos.x, (int)sparkPool[sp].pos.y, 3.5f, Fade(ORANGE, alpha));
                }
            }
            EndBlendMode();

            // HERO 1
            if (Hero1Lives > 0 && !(evacActive && evacTimer <= 2.2f))
            {
                DrawTexturePro(HeroTexture, (Rectangle){ 0, 0, (float)HeroTexture.width, (float)HeroTexture.height },
                               (Rectangle){ Hero1Pos.x - HeroWidth / 2.0f, Hero1Pos.y, HeroWidth, HeroHeight }, (Vector2){ 0, 0 }, 0.0f, hero1Debuffed ? PURPLE : WHITE);
                DrawText(TextFormat("Shoab [%s]", inputHero1Name), (int)Hero1Pos.x - MeasureText(TextFormat("Shoab [%s]", inputHero1Name), 18) / 2, (int)Hero1Pos.y - 24, 18, LIME);
                if (hero1Debuffed)
                {
                    DrawCircleLines((int)Hero1Pos.x, (int)(Hero1Pos.y + HeroHeight / 2.0f), 65.0f, MAGENTA);
                    char dAlert[] = "! WEAPONS JAMMED !";
                    DrawText(dAlert, (int)Hero1Pos.x - MeasureText(dAlert, 14) / 2, (int)Hero1Pos.y - 42, 14, YELLOW);
                }
                if (hero1AsteroidSlowTimer > 0.0f)
                {
                    const char* slwTxt = "! ENGINE SLOW (60%) !";
                    DrawText(slwTxt, (int)Hero1Pos.x - MeasureText(slwTxt, 13) / 2, (int)Hero1Pos.y + HeroHeight + 6, 13, ORANGE);
                }
                if (hero1OverdriveTimer > 0.0f)
                {
                    DrawCircleLines((int)Hero1Pos.x, (int)(Hero1Pos.y + HeroHeight / 2.0f), 70.0f + sinf((float)GetTime() * 20.0f) * 8.0f, LIME);
                    const char* ovrTxt = ">> LAST STAND OVERDRIVE <<";
                    DrawText(ovrTxt, (int)Hero1Pos.x - MeasureText(ovrTxt, 14) / 2, (int)Hero1Pos.y - 46, 14, LIME);
                }
            }

            // HERO 2
            if (Hero2Lives > 0 && !(evacActive && evacTimer <= 2.2f))
            {
                DrawStealthFlames((Vector2){ Hero2Pos.x, Hero2Pos.y + HeroHeight / 2.0f }, HeroWidth, HeroHeight);
                DrawTexturePro(StealthHeroTexture, (Rectangle){ 0, 0, (float)StealthHeroTexture.width, (float)StealthHeroTexture.height },
                               (Rectangle){ Hero2Pos.x - HeroWidth / 2.0f, Hero2Pos.y, HeroWidth, HeroHeight }, (Vector2){ 0, 0 }, 0.0f, hero2Debuffed ? PURPLE : WHITE);
                DrawText(TextFormat("Nayemul [%s]", inputHero2Name), (int)Hero2Pos.x - MeasureText(TextFormat("Nayemul [%s]", inputHero2Name), 18) / 2, (int)Hero2Pos.y - 24, 18, YELLOW);
                if (hero2Debuffed)
                {
                    DrawCircleLines((int)Hero2Pos.x, (int)(Hero2Pos.y + HeroHeight / 2.0f), 65.0f, MAGENTA);
                    char dAlert[] = "! WEAPONS JAMMED !";
                    DrawText(dAlert, (int)Hero2Pos.x - MeasureText(dAlert, 14) / 2, (int)Hero2Pos.y - 42, 14, YELLOW);
                }
                if (hero2AsteroidSlowTimer > 0.0f)
                {
                    const char* slwTxt = "! ENGINE SLOW (60%) !";
                    DrawText(slwTxt, (int)Hero2Pos.x - MeasureText(slwTxt, 13) / 2, (int)Hero2Pos.y + HeroHeight + 6, 13, ORANGE);
                }
                if (hero2OverdriveTimer > 0.0f)
                {
                    DrawCircleLines((int)Hero2Pos.x, (int)(Hero2Pos.y + HeroHeight / 2.0f), 70.0f + sinf((float)GetTime() * 20.0f) * 8.0f, YELLOW);
                    const char* ovrTxt = ">> LAST STAND OVERDRIVE <<";
                    DrawText(ovrTxt, (int)Hero2Pos.x - MeasureText(ovrTxt, 14) / 2, (int)Hero2Pos.y - 46, 14, YELLOW);
                }
            }

            // TEAM SHIELDing effect after pressing enter key...
            if (heroesTethered && teamShieldCooldownTimer <= 0.0f && !teamShieldActive)
            {
                DrawLineEx((Vector2){ Hero1Pos.x, Hero1Pos.y + HeroHeight * 0.4f }, (Vector2){ Hero2Pos.x, Hero2Pos.y + HeroHeight * 0.4f },
                           2.5f, Fade(SKYBLUE, 0.40f + 0.30f * sinf((float)GetTime() * 10.0f)));
            }
            if (teamShieldActive)
            {
                BeginBlendMode(BLEND_ADDITIVE);
                if (teamShieldDomeTex.id > 0)
                    DrawTexturePro(teamShieldDomeTex, (Rectangle){ 0, 0, (float)teamShieldDomeTex.width, (float)teamShieldDomeTex.height },
                                   (Rectangle){ domeCenterX, domeBaseY, domeRadius * 2.0f, domeRadius }, (Vector2){ domeRadius, domeRadius }, 0.0f, Fade(WHITE, 0.82f + 0.18f * sinf((float)GetTime() * 14.0f)));
                else
                {
                    DrawCircleSector((Vector2){ domeCenterX, domeBaseY }, domeRadius, 180.0f, 360.0f, 36, Fade(SKYBLUE, 0.45f));
                    DrawCircleLines((int)domeCenterX, (int)domeBaseY, domeRadius, Fade(WHITE, 0.90f));
                }
                if (bossLaserActive)
                {
                    DrawCircle((int)domeCenterX, (int)(domeBaseY - domeRadius), 28.0f, Fade(WHITE, 0.85f));
                    DrawCircle((int)domeCenterX, (int)(domeBaseY - domeRadius), 42.0f, Fade(SKYBLUE, 0.60f));
                }
                EndBlendMode();
            }

            // HUD of SHOAB and scoring...
            int b1X = 30, b1Y = 18, bSize = 64;
            DrawRectangle(b1X - 2, b1Y - 2, bSize + 4, bSize + 4, Fade(BLACK, 0.7f));
            if (Hero1Lives <= 0)
            {
                DrawRectangle(b1X, b1Y, bSize, bSize, DARKGRAY);
                for (int s = 0; s < 12; s++) DrawRectangle(b1X + GetRandomValue(0, bSize - 8), b1Y + GetRandomValue(0, bSize - 4), GetRandomValue(6, 14), 2, RAYWHITE);
                DrawRectangleLines(b1X, b1Y, bSize, bSize, RED); DrawText("LOST", b1X + 12, b1Y + 24, 15, RED);
            }
            else
            {
                bool shoabCrit = (Hero1Lives <= 1 || hero1HitFlashTimer > 0.0f);
                Texture2D curShoabTex = shoabCrit ? portraitShoabCrit : portraitShoab;

                if (hero1HitFlashTimer > 0.0f)
                {
                    for (int s = 0; s < 4; s++)
                    {
                        float srcH = (float)curShoabTex.height / 4.0f;
                        Rectangle srcSlice = { 0.0f, s * srcH, (float)curShoabTex.width, srcH };
                        float jitX = (float)GetRandomValue(-5, 5);
                        Rectangle dstSlice = { b1X + jitX, b1Y + s * 16.0f, (float)bSize, 16.0f };
                        DrawTexturePro(curShoabTex, srcSlice, dstSlice, (Vector2){ 0, 0 }, 0.0f, Fade(RED, 0.75f));
                    }
                }
                else
                {
                    if (curShoabTex.id > 0)
                        DrawTexturePro(curShoabTex, (Rectangle){ 0, 0, (float)curShoabTex.width, (float)curShoabTex.height },
                                       (Rectangle){ b1X, b1Y, (float)bSize, (float)bSize }, (Vector2){ 0, 0 }, 0.0f, WHITE);
                    else
                        DrawRectangle(b1X, b1Y, bSize, bSize, Fade(DARKGREEN, 0.4f));
                }

                Color border1Col = LIME;
                if (Hero1Lives == 1)
                {
                    border1Col = ((int)(GetTime() * 5.0f) % 2 == 0) ? RED : Fade(RED, 0.35f);
                }
                else if (shoabCrit) border1Col = RED;

                DrawRectangleLinesEx((Rectangle){ b1X, b1Y, (float)bSize, (float)bSize }, shoabCrit ? 2.5f : 2.0f, border1Col);

                if (Hero1Lives <= 2)
                {
                    DrawLineEx((Vector2){ b1X + 10, b1Y + 4 }, (Vector2){ b1X + 32, b1Y + 26 }, 1.5f, Fade(WHITE, 0.75f));
                    DrawLineEx((Vector2){ b1X + 32, b1Y + 26 }, (Vector2){ b1X + 54, b1Y + 18 }, 1.2f, Fade(WHITE, 0.60f));
                    DrawLineEx((Vector2){ b1X + 32, b1Y + 26 }, (Vector2){ b1X + 24, b1Y + 50 }, 1.5f, Fade(WHITE, 0.70f));
                    DrawLineEx((Vector2){ b1X + 24, b1Y + 50 }, (Vector2){ b1X + 8, b1Y + 58 }, 1.0f, Fade(WHITE, 0.45f));
                    DrawLineEx((Vector2){ b1X + 32, b1Y + 26 }, (Vector2){ b1X + 46, b1Y + 54 }, 1.2f, Fade(WHITE, 0.50f));
                }

                float chargeRatio1 = ShoabSpecialTime / (FPS * 15.0f);
                if (chargeRatio1 > 1.0f) chargeRatio1 = 1.0f;
                Vector2 portraitCenter1 = { b1X + bSize / 2.0f, b1Y + bSize / 2.0f };
                DrawRing(portraitCenter1, (float)bSize * 0.72f, (float)bSize * 0.78f, 0.0f, chargeRatio1 * 360.0f, 36, (chargeRatio1 >= 1.0f) ? LIME : SKYBLUE);
            }

            const char* telemShoabStr = "SIGNAL: 99.8% // OK";
            Color telemShoabCol = LIME;
            if (Hero1Lives == 3) { telemShoabStr = "SIGNAL: 81.4% // WARN"; telemShoabCol = YELLOW; }
            else if (Hero1Lives == 2) { telemShoabStr = "SIGNAL: 48.0% // LOSS"; telemShoabCol = ORANGE; }
            else if (Hero1Lives == 1) { telemShoabStr = "SIGNAL: 12.2% // CRIT"; telemShoabCol = (((int)(GetTime() * 6)) % 2 == 0) ? RED : MAROON; }
            else if (Hero1Lives <= 0) { telemShoabStr = "SIGNAL: 00.0% // LOST"; telemShoabCol = DARKGRAY; }
            DrawText(telemShoabStr, b1X, b1Y + bSize + 5, 10, telemShoabCol);

            int text1X = b1X + bSize + 14;
            DrawText(TextFormat("PILOT 1: %s", inputHero1Name), text1X, 18, 20, LIME);
            if (radarJammedTimer > 0.0f)
            {
                int jamW = 240;
                DrawRectangle(text1X - 4, 40, jamW, 68, Fade(BLACK, 0.88f));
                DrawRectangleLines(text1X - 4, 40, jamW, 68, Fade((Color){ 200, 80, 255, 255 }, 0.8f));
                DrawText("[ RADAR JAMMED ]", text1X + 10, 48, 17, (Color){ 200, 80, 255, 255 });
                DrawText(TextFormat("SIGNAL LOST // 0x%04X", GetRandomValue(0x1000, 0xFFFF)), text1X + 10, 72, 14, RED);
            }
            else
            {
                DrawText(TextFormat("LIVES: %d / 4", Hero1Lives), text1X, 42, 20, (Hero1Lives <= 1) ? RED : LIME);
                DrawText(TextFormat("SCORE: %05d", Hero1Score), text1X, 66, 20, (Color){ 180, 255, 180, 255 });
                DrawText("[A/D] Move  |  [W/SPACE] Shoot", text1X, 90, 14, LIGHTGRAY);
            }

            if (ShoabSpecialTime >= FPS * 15.0f)
            {
                if (!SpecialReady) { PlaySound(sndChargeReady); shoabEdgeFlashTimer = 0.25f; }
                SpecialReady = true;
                float pulse = (sinf((float)GetTime() * 10.0f) + 1.0f) * 0.5f;
                int dynamicFontSize = 14 + (int)(pulse * 4.0f);
                Color pulseCol = ColorAlphaBlend(LIME, YELLOW, Fade(WHITE, pulse));
                DrawText("[Q] HYPER BEAM PRIMED!", 30, 120, dynamicFontSize, pulseCol);
            }
            else
            {
                DrawText(TextFormat("[Q] READY IN %.1f s", 15.0f - ShoabSpecialTime / FPS), 30, 122, 14, RED);
                SpecialReady = false;
            }

            // HUD of NAYEMUL!!!!!!!!!!!!!!!!!!!!!!
            int b2X = WindowWidth - 30 - bSize, b2Y = 18;
            DrawRectangle(b2X - 2, b2Y - 2, bSize + 4, bSize + 4, Fade(BLACK, 0.7f));
            if (Hero2Lives <= 0)
            {
                DrawRectangle(b2X, b2Y, bSize, bSize, DARKGRAY);
                for (int s = 0; s < 12; s++) DrawRectangle(b2X + GetRandomValue(0, bSize - 8), b2Y + GetRandomValue(0, bSize - 4), GetRandomValue(6, 14), 2, RAYWHITE);
                DrawRectangleLines(b2X, b2Y, bSize, bSize, RED); DrawText("LOST", b2X + 12, b2Y + 24, 15, RED);
            }
            else
            {
                bool nayemulCrit = (Hero2Lives <= 1 || hero2HitFlashTimer > 0.0f);
                Texture2D curNayemulTex = nayemulCrit ? portraitNayemulCrit : portraitNayemul;

                if (hero2HitFlashTimer > 0.0f)
                {
                    for (int s = 0; s < 4; s++)
                    {
                        float srcH = (float)curNayemulTex.height / 4.0f;
                        Rectangle srcSlice = { 0.0f, s * srcH, (float)curNayemulTex.width, srcH };
                        float jitX = (float)GetRandomValue(-5, 5);
                        Rectangle dstSlice = { b2X + jitX, b2Y + s * 16.0f, (float)bSize, 16.0f };
                        DrawTexturePro(curNayemulTex, srcSlice, dstSlice, (Vector2){ 0, 0 }, 0.0f, Fade(RED, 0.75f));
                    }
                }
                else
                {
                    if (curNayemulTex.id > 0)
                        DrawTexturePro(curNayemulTex, (Rectangle){ 0, 0, (float)curNayemulTex.width, (float)curNayemulTex.height },
                                       (Rectangle){ b2X, b2Y, (float)bSize, (float)bSize }, (Vector2){ 0, 0 }, 0.0f, WHITE);
                    else
                        DrawRectangle(b2X, b2Y, bSize, bSize, Fade(DARKBROWN, 0.4f));
                }

                Color border2Col = YELLOW;
                if (Hero2Lives == 1)
                {
                    border2Col = ((int)(GetTime() * 5.0f) % 2 == 0) ? RED : Fade(RED, 0.35f);
                }
                else if (nayemulCrit) border2Col = RED;

                DrawRectangleLinesEx((Rectangle){ b2X, b2Y, (float)bSize, (float)bSize }, nayemulCrit ? 2.5f : 2.0f, border2Col);

                if (Hero2Lives <= 2)
                {
                    DrawLineEx((Vector2){ b2X + 14, b2Y + 6 }, (Vector2){ b2X + 36, b2Y + 28 }, 1.5f, Fade(WHITE, 0.75f));
                    DrawLineEx((Vector2){ b2X + 36, b2Y + 28 }, (Vector2){ b2X + 56, b2Y + 22 }, 1.2f, Fade(WHITE, 0.60f));
                    DrawLineEx((Vector2){ b2X + 36, b2Y + 28 }, (Vector2){ b2X + 22, b2Y + 52 }, 1.5f, Fade(WHITE, 0.70f));
                    DrawLineEx((Vector2){ b2X + 22, b2Y + 52 }, (Vector2){ b2X + 6, b2Y + 60 }, 1.0f, Fade(WHITE, 0.45f));
                    DrawLineEx((Vector2){ b2X + 36, b2Y + 28 }, (Vector2){ b2X + 50, b2Y + 54 }, 1.2f, Fade(WHITE, 0.50f));
                }

                float chargeRatio2 = NayemulSpecialTime / (FPS * 15.0f);
                if (chargeRatio2 > 1.0f) chargeRatio2 = 1.0f;
                Vector2 portraitCenter2 = { b2X + bSize / 2.0f, b2Y + bSize / 2.0f };
                DrawRing(portraitCenter2, (float)bSize * 0.72f, (float)bSize * 0.78f, 0.0f, chargeRatio2 * 360.0f, 36, (chargeRatio2 >= 1.0f) ? YELLOW : ORANGE);
            }

            const char* telemNayemulStr = "SIGNAL: 99.8% // OK";
            Color telemNayemulCol = LIME;
            if (Hero2Lives == 3) { telemNayemulStr = "SIGNAL: 81.4% // WARN"; telemNayemulCol = YELLOW; }
            else if (Hero2Lives == 2) { telemNayemulStr = "SIGNAL: 48.0% // LOSS"; telemNayemulCol = ORANGE; }
            else if (Hero2Lives == 1) { telemNayemulStr = "SIGNAL: 12.2% // CRIT"; telemNayemulCol = (((int)(GetTime() * 6)) % 2 == 0) ? RED : MAROON; }
            else if (Hero2Lives <= 0) { telemNayemulStr = "SIGNAL: 00.0% // LOST"; telemNayemulCol = DARKGRAY; }
            DrawText(telemNayemulStr, b2X + bSize - MeasureText(telemNayemulStr, 10), b2Y + bSize + 5, 10, telemNayemulCol);

            const char* pilot2Title = TextFormat("PILOT 2: %s", inputHero2Name);
            DrawText(pilot2Title, b2X - 14 - MeasureText(pilot2Title, 20), 18, 20, YELLOW);
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
                const char* p2LivesText = TextFormat("LIVES: %d / 4", Hero2Lives); //scoreboard drawing according to instructions of our sir....
                DrawText(p2LivesText, b2X - 14 - MeasureText(p2LivesText, 20), 42, 20, (Hero2Lives <= 1) ? RED : YELLOW);
                const char* p2ScoreText = TextFormat("SCORE: %05d", Hero2Score);
                DrawText(p2ScoreText, b2X - 14 - MeasureText(p2ScoreText, 20), 66, 20, (Color){ 255, 245, 160, 255 });
                const char* p2Controls = "[ARROWS] Move  |  [UP] Shoot";
                DrawText(p2Controls, b2X - 14 - MeasureText(p2Controls, 14), 90, 14, LIGHTGRAY);
            }

            if (NayemulSpecialTime >= FPS * 15.0f)
            {
                if (!NayemulSpecialReady) { PlaySound(sndChargeReady); nayemulEdgeFlashTimer = 0.25f; }
                NayemulSpecialReady = true;
                float pulse = (sinf((float)GetTime() * 10.0f) + 1.0f) * 0.5f;
                int dynamicFontSize = 14 + (int)(pulse * 4.0f);
                Color pulseCol = ColorAlphaBlend(YELLOW, ORANGE, Fade(WHITE, pulse));
                const char* rReadyText = "[RSHIFT] CLUSTER SALVO PRIMED!";
                DrawText(rReadyText, WindowWidth - 30 - MeasureText(rReadyText, dynamicFontSize), 120, dynamicFontSize, pulseCol);
            }
            else
            {
                const char* rChargeText = TextFormat("[RSHIFT] READY IN %.1f s", 15.0f - NayemulSpecialTime / FPS);
                DrawText(rChargeText, WindowWidth - 30 - MeasureText(rChargeText, 14), 122, 14, RED);
                NayemulSpecialReady = false;
            }

            if (shoabEdgeFlashTimer > 0.0f)
                DrawRectangleLinesEx((Rectangle){ 0, 0, WindowWidth, WindowHeight }, 8, Fade(LIME, shoabEdgeFlashTimer / 0.25f));
            if (nayemulEdgeFlashTimer > 0.0f)
                DrawRectangleLinesEx((Rectangle){ 0, 0, WindowWidth, WindowHeight }, 8, Fade(YELLOW, nayemulEdgeFlashTimer / 0.25f));

            if (empBuffTimer > 0.0f)
            {
                const char* empStatus = TextFormat("RAPID PLASMA OVERCHARGE: %.1fs", empBuffTimer);
                DrawText(empStatus, (WindowWidth - MeasureText(empStatus, 18)) / 2, 92, 18, LIME);
            }

            if (teamShieldActive)
            {
                const char* shldActiveTxt = TextFormat("TEAM SHIELD ACTIVE: %.1fs", teamShieldActiveTimer);
                DrawText(shldActiveTxt, (WindowWidth - MeasureText(shldActiveTxt, 18)) / 2, 115, 18, SKYBLUE);
            }
            else if (teamShieldCooldownTimer > 0.0f)
            {
                const char* shldCdTxt = TextFormat("TEAM SHIELD RECHARGING: %.1fs", teamShieldCooldownTimer);
                DrawText(shldCdTxt, (WindowWidth - MeasureText(shldCdTxt, 14)) / 2, 118, 14, DARKGRAY);
            }
            else if (heroesTethered)
            {
                const char* shldRdyTxt = "[ENTER] TEAM SHIELD READY";
                DrawText(shldRdyTxt, (WindowWidth - MeasureText(shldRdyTxt, 16)) / 2, 116, 16, LIME);
            }
            else if (Hero1Lives > 0 && Hero2Lives > 0)
            {
                const char* shldProxTxt = "TEAM SHIELD: FLY CLOSER (<375px)";
                DrawText(shldProxTxt, (WindowWidth - MeasureText(shldProxTxt, 13)) / 2, 118, 13, GRAY);
            }

            const char* pauseNotice = "[P] PAUSE  |  [F / F11] FULLSCREEN  |  [M] MENU";
            int pauseW = MeasureText(pauseNotice, 15);
            if (bossActive)
            {
                int barW = 460, barH = 22, barX = (WindowWidth - barW) / 2, barY = 36;
                const char* bossHeader = (bossHp <= 40) ? "!! DREADNOUGHT ENRAGED (PHASE 2) !!" : "DREADNOUGHT CARRIER BOSS";
                DrawText(bossHeader, (WindowWidth - MeasureText(bossHeader, 18)) / 2, 14, 18, (bossHp <= 40) ? YELLOW : RED);

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

            // Super-Pause Blinding Flash;exceptional visual feature taken from alien shooter....
            if (superPauseTimer > 0.0f)
            {
                DrawRectangle(0, 0, WindowWidth, WindowHeight, Fade(WHITE, (superPauseTimer / 0.18f) * 0.35f));
            }

            // Canopy Glass Specular Corner Sheen Glints
            if (canopyGlintTimer > 0.0f)
            {
                float glintAlpha = (canopyGlintTimer / 0.35f) * 0.18f;
                Vector2 g1_a = { 0, 0 }, g1_b = { 280, 0 }, g1_c = { 0, 180 };
                DrawTriangle(g1_a, g1_c, g1_b, Fade(SKYBLUE, glintAlpha));

                Vector2 g2_a = { (float)WindowWidth, 0 }, g2_b = { (float)WindowWidth - 280, 0 }, g2_c = { (float)WindowWidth, 180 };
                DrawTriangle(g2_a, g2_b, g2_c, Fade(SKYBLUE, glintAlpha));
            }
        }
        // STATE: VIDEO PLAYBACK for both gameOVER and gameWIN!!!!!!!!!! thanks to my small brother!!!!!!!
        else if (CurrentState == STATE_VIDEO_PLAY)
        {
            videoTimer += rawTime;
            videoFrameTimer += rawTime;

            if (videoPipe != NULL && videoFramePixels != NULL)
            {
                while (videoFrameTimer >= 1.0f / 30.0f)
                {
                    videoFrameTimer -= 1.0f / 30.0f;
                    if (ReadExactBytes(videoPipe, videoFramePixels, VID_W * VID_H * sizeof(Color)))
                    {
                        UpdateTexture(videoFrameTex, videoFramePixels);
                        hasReceivedFrames = true;
                    }
                    else
                    {
                        break;
                    }
                }
            }

            if (hasReceivedFrames && videoFrameTex.id > 0)
            {
                DrawTexturePro(videoFrameTex, (Rectangle){ 0, 0, (float)VID_W, (float)VID_H },
                               (Rectangle){ 0, 0, (float)WindowWidth, (float)WindowHeight },
                               (Vector2){ 0, 0 }, 0.0f, WHITE);
            }
            else
            {
                if (!ffmpegInstalled) //video not playing fixed for macOS!!!!!!!! thanks to ADR sir...
                {
                    const char* err1 = "FFMPEG NOT FOUND IN SYSTEM PATH";
                    const char* err2 = "Run 'brew install ffmpeg' in Terminal to enable video rendering.";
                    const char* err3 = "(Audio playback is currently running in background)";
                    DrawText(err1, (WindowWidth - MeasureText(err1, 24)) / 2, WindowHeight / 2 - 40, 24, RED);
                    DrawText(err2, (WindowWidth - MeasureText(err2, 18)) / 2, WindowHeight / 2, 18, YELLOW);
                    DrawText(err3, (WindowWidth - MeasureText(err3, 16)) / 2, WindowHeight / 2 + 30, 16, LIGHTGRAY);
                }
                else
                {
                    DrawText("BUFFERING VIDEO STREAM...", (WindowWidth - MeasureText("BUFFERING VIDEO STREAM...", 24)) / 2, WindowHeight / 2 - 12, 24, DARKGRAY);
                }
            }

            if (currentVideoType == 1 || currentVideoType == 2)
            {
                DrawText("Replay", 42, 42, 38, Fade(BLACK, 0.85f));
                DrawText("Replay", 40, 42, 38, MAROON);
                DrawText("Replay", 40, 40, 38, RED);
                DrawText("Replay", 41, 40, 38, RED);
            }

            const char* skipVidTxt = "PRESS [SPACE] OR [ENTER] TO SKIP";
            if (((int)(GetTime() * 3)) % 2 == 0)
            {
                DrawText(skipVidTxt, (WindowWidth - MeasureText(skipVidTxt, 18)) / 2, WindowHeight - 45, 18, Fade(RAYWHITE, 0.85f));
            }

            bool skipPressed = IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER);
            if (skipPressed || videoTimer >= videoDuration)
            {
                StopVideo();
                if (currentVideoType == 0)
                {
                    CurrentState = STATE_GAMEPLAY;
                }
                else if (currentVideoType == 1)
                {
                    CurrentState = STATE_GAMEPLAY;
                    GameOver = true;
                }
                else if (currentVideoType == 2)
                {
                    CurrentState = STATE_GAMEPLAY;
                    bossDefeated = true;
                }
            }
        }

        // Full Screen White-Out Blind for Evacuation Sequence after alien bossssssssss ;)
        if (evacActive && evacTimer > 3.4f)
        {
            float blindAlpha = (evacTimer - 3.4f) / 0.4f;
            if (blindAlpha > 1.0f) blindAlpha = 1.0f;
            if (blindAlpha < 0.0f) blindAlpha = 0.0f;
            DrawRectangle(0, 0, WindowWidth, WindowHeight, Fade(WHITE, blindAlpha));
        }

        EndMode2D();
        
        // Scanlines;basically some vibes for retro arcade game;(taken from sample game...)
        if (ScanlinesOn)
        {
            for (int y = 0; y < WindowHeight; y += 4) DrawRectangle(0, y, WindowWidth, 1, Fade(BLACK, 0.22f));
            
            DrawRectangleGradientV(0, 0, WindowWidth, 40, Fade(BLACK, 0.50f), BLANK);
            DrawRectangleGradientV(0, WindowHeight - 40, WindowWidth, 40, BLANK, Fade(BLACK, 0.50f));
            DrawRectangleGradientH(0, 0, 40, WindowHeight, Fade(BLACK, 0.50f), BLANK);
            DrawRectangleGradientH(WindowWidth - 40, 0, 40, WindowHeight, BLANK, Fade(BLACK, 0.50f));
        }

        if (Brightness < 1.0f) DrawRectangle(0, 0, WindowWidth, WindowHeight, Fade(BLACK, 1.0f - Brightness));
        else if (Brightness > 1.0f) DrawRectangle(0, 0, WindowWidth, WindowHeight, Fade(WHITE, (Brightness - 1.0f) * 0.25f));

        EndTextureMode();

        // fixed the bug of unexpected crash of game in macOS....
        BeginDrawing();
        ClearBackground(BLACK);
        float scale = fminf((float)GetScreenWidth() / WindowWidth, (float)GetScreenHeight() / WindowHeight);
        Rectangle srcRec = { 0.0f, 0.0f, (float)renderTarget.texture.width, -(float)renderTarget.texture.height };
        Rectangle destRec = {
            ((float)GetScreenWidth() - ((float)WindowWidth * scale)) * 0.5f,
            ((float)GetScreenHeight() - ((float)WindowHeight * scale)) * 0.5f,
            (float)WindowWidth * scale,
            (float)WindowHeight * scale
        };
        DrawTexturePro(renderTarget.texture, srcRec, destRec, (Vector2){ 0, 0 }, 0.0f, WHITE);
        EndDrawing();
    }

    // Stop and cleanup active video playback
    StopVideo();
    if (videoFrameTex.id > 0) UnloadTexture(videoFrameTex);
    if (videoFramePixels != NULL) free(videoFramePixels);

    // Cleanup Textures 
    UnloadRenderTexture(renderTarget);
    UnloadTexture(HeroTexture); UnloadTexture(StealthHeroTexture);
    if (loadingBg.id > 0) UnloadTexture(loadingBg);
    if (portraitShoab.id > 0) UnloadTexture(portraitShoab);
    if (portraitNayemul.id > 0) UnloadTexture(portraitNayemul);
    if (portraitShoabCrit.id > 0 && portraitShoabCrit.id != portraitShoab.id) UnloadTexture(portraitShoabCrit);
    if (portraitNayemulCrit.id > 0 && portraitNayemulCrit.id != portraitNayemul.id) UnloadTexture(portraitNayemulCrit);

    // Cleanup Developer Portraits
    if (credPortraitShoab.id > 0 && credPortraitShoab.id != portraitShoab.id) UnloadTexture(credPortraitShoab);
    if (credPortraitNayemul.id > 0 && credPortraitNayemul.id != portraitNayemul.id) UnloadTexture(credPortraitNayemul);

    if (clusterBombTex.id > 0) UnloadTexture(clusterBombTex);
    if (clusterBlastTex.id > 0) UnloadTexture(clusterBlastTex);
    if (teamShieldDomeTex.id > 0) UnloadTexture(teamShieldDomeTex);

    if (texAsteroid.id > 0)   UnloadTexture(texAsteroid);
    if (texDebris.id > 0)     UnloadTexture(texDebris);
    if (texSmoke.id > 0)      UnloadTexture(texSmoke);
    if (texSpark.id > 0)      UnloadTexture(texSpark);
    if (texRankBadgeS.id > 0) UnloadTexture(texRankBadgeS);
    if (texRankBadgeA.id > 0) UnloadTexture(texRankBadgeA);
    if (texRankBadgeB.id > 0) UnloadTexture(texRankBadgeB);
    if (texRankBadgeC.id > 0) UnloadTexture(texRankBadgeC);

    // Black Hole cleanup
    if (texBlackHoleCore.id > 0) UnloadTexture(texBlackHoleCore);
    if (texBlackHoleDisk.id > 0) UnloadTexture(texBlackHoleDisk);
    UnloadSound(sndBlackHoleDrone);
    UnloadSound(sndBlackHolePull);
    UnloadSound(sndBlackHoleCrush);

    // Void Entity cleanup
    for (int ep = 0; ep < 4; ep++) {
        if (texEntityPhantom[ep].id > 0) UnloadTexture(texEntityPhantom[ep]);
    }
    if (texEntityOrb.id > 0) UnloadTexture(texEntityOrb);
    UnloadSound(sndEntitySpawn);
    UnloadSound(sndEntityDrone);
    UnloadSound(sndEntityAttack);
    UnloadSound(sndEntityScream);

    for (int s = 0; s < 7; s++)
    {
        if (Hero1SpecialBulletTex[s].id > 0) UnloadTexture(Hero1SpecialBulletTex[s]);
    }

    for (int c = 0; c < 4; c++) { UnloadTexture(jammerTex[c]); UnloadTexture(warpTex[c]); }
    for (int b = 0; b < 5; b++) UnloadTexture(BossTexture[b]);
    for (int Asprite = 0; Asprite < AlienSprite; Asprite++) UnloadTexture(AlienTexture[Asprite]);

    // Cleanup Sounds
    UnloadSound(shoot); UnloadSound(AlienShoot); UnloadSound(menuMove); UnloadSound(menuSelect);
    UnloadSound(heroDeath); UnloadSound(pauseIn); UnloadSound(pauseOut); UnloadSound(damage);
    
    UnloadSound(heroOuch); UnloadSound(bossLaserSound); UnloadSound(sndJammerHum); UnloadSound(sndJammerShot);
    UnloadSound(sndEmpBlast); UnloadSound(sndJammerDeath); UnloadSound(sndWarpGlide); UnloadSound(sndWarpShot);
    UnloadSound(sndTeleport); UnloadSound(sndWarpDeath); UnloadSound(sndSpecialBeam); UnloadSound(sndChargeReady);
    UnloadSound(sndPodDestroy); UnloadSound(sndOrbLaunch); UnloadSound(sndWarningSiren); UnloadSound(sndDefibHum);
    UnloadSound(bossEnrage); UnloadSound(sndRocketBoost); UnloadSound(sndClusterLaunch); UnloadSound(sndClusterExplode);
    UnloadSound(sndShieldActivate); UnloadSound(sndShieldDeflect);
    UnloadSound(sndHudGlitch); UnloadSound(sndEvacBoom);

    UnloadSound(sndMagRailCharge);
    UnloadSound(sndLaserCharge);
    UnloadSound(sndBossWarpIn);
    UnloadSound(sndAlienStep);
    UnloadSound(sndDryFire);
    UnloadSound(sndTetherConnect);
    UnloadSound(sndAirdropIncoming);

    UnloadSound(sndAsteroidHitHero);
    UnloadSound(sndAsteroidRicochet);
    UnloadSound(sndAsteroidShatter);
    UnloadSound(sndCockpitSpark);
    UnloadSound(sndHullAlarm);
    UnloadSound(sndRankS);
    UnloadSound(sndRankBadge);

    // Unload Music
    for (int g = 0; g < 3; g++)
    {
        UnloadMusicStream(bgmGameplay[g]);
    }

    UnloadMusicStream(bgmStory); UnloadMusicStream(bgmMenu); UnloadMusicStream(bgmBoss);
    UnloadMusicStream(bgmWin); UnloadMusicStream(bgmLost);

    CloseAudioDevice();
    CloseWindow();
    return 0;
}
