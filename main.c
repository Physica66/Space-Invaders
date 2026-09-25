#include "raylib/raylib-6.0_macos/include/raylib.h"
#include "raylib/raylib-6.0_macos/include/raymath.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

bool SpecialReady = false;
bool starttimer = false;
float ShoabSpecialTime = 0.0f;

bool NayemulSpecialReady = false;
float NayemulSpecialTime = 0.0f;
bool clusterBossDamageDealt = false; // Salvo gate: ensures 1 damage event per special trigger

// Team-Shield Feature Variables (Fixed size half-sphere trapping heroes)
bool teamShieldActive = false;
float teamShieldActiveTimer = 0.0f;
float teamShieldCooldownTimer = 0.0f;
float teamShieldCenterX = 0.0f;
#define TeamShieldRadius 240.0f

// Dreadnought Hyperspace Drop & Evacuation Feature Variables
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

// Medium Alien Minion dimensions & capacities
#define MAX_MINIONS 8
#define MinionSize 68
#define MAX_MINION_BULLETS 16

#define COMMANDER_SIZE 72
#define MAX_COMMANDER_BULLETS 8

// Cluster missile Capacities
#define MAX_CLUSTER_MISSILES 8
#define MAX_CLUSTER_BLASTS 8

#ifndef PI
#define PI 3.14159265358979323846f
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

#define STAR_COUNT 90

#define VID_W 640
#define VID_H 360

static FILE* videoPipe = NULL;
static Texture2D videoFrameTex = { 0 };
static Color* videoFramePixels = NULL;
static float videoTimer = 0.0f;
static float videoDuration = 0.0f;
static float videoFrameTimer = 0.0f;
static int currentVideoType = 0; // 0 = Start Launch, 1 = Game Over, 2 = Game Win
static bool videoIsPlaying = false;
static bool hasReceivedFrames = false;
static bool ffmpegInstalled = true;

const char* GetFFmpegBinary(void)
{
    if (FileExists("/opt/homebrew/bin/ffmpeg")) return "/opt/homebrew/bin/ffmpeg";
    if (FileExists("/usr/local/bin/ffmpeg")) return "/usr/local/bin/ffmpeg";
    if (FileExists("/usr/bin/ffmpeg")) return "/usr/bin/ffmpeg";
    return "ffmpeg";
}

const char* GetVideoPath(int vidType)
{
    if (vidType == 0)
    {
        if (FileExists("assets/sprites/Starting_Vid.mp4")) return "assets/sprites/Starting_Vid.mp4";
        if (FileExists("assets/sprites/starting_vid.mp4")) return "assets/sprites/starting_vid.mp4";
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
        size_t n = fread(ptr + total_read, 1, bytes_to_read, stream);
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
    videoFrameTimer = 0.0f;
    videoIsPlaying = true;
    hasReceivedFrames = false;

    system("killall afplay 2>/dev/null");

    const char* vPath = GetVideoPath(vidType);
    char afplayCmd[512];
    snprintf(afplayCmd, sizeof(afplayCmd), "afplay \"%s\" &", vPath);
    system(afplayCmd);

    int ffmpegCheck = system("export PATH=\"/opt/homebrew/bin:/usr/local/bin:$PATH\"; which ffmpeg > /dev/null 2>&1");
    if (ffmpegCheck != 0 && !FileExists("/opt/homebrew/bin/ffmpeg") && !FileExists("/usr/local/bin/ffmpeg"))
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
            if (AlienAlive[X][Y] && (AlienPos[X][Y].y + AlienSize + 20.0f > maxAlienY))
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
            for (int Y = 0; Y < AlienInY; Y++)
            {
                AlienPos[X][Y].y += 20.0f;
            }
        }
    }
}

// Stealth hero sprite flame animations
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
        Vector2 v3_mid = { nozzles[n].x + jitterX * 0.5f, nozzles[n].y + baseLen };
        DrawTriangle(v1_mid, v3_mid, v2_mid, (Color){ 90, 170, 255, 230 });

        Vector2 v1_in = { nozzles[n].x - nozzleHalfW * 0.45f, nozzles[n].y };
        Vector2 v2_in = { nozzles[n].x + nozzleHalfW * 0.45f, nozzles[n].y };
        Vector2 v3_in = { nozzles[n].x, nozzles[n].y + (baseLen * 0.55f) };
        DrawTriangle(v1_in, v3_in, v2_in, WHITE);

        DrawCircle((int)nozzles[n].x, (int)nozzles[n].y + 4, 8.0f, Fade(SKYBLUE, 0.6f));
        DrawCircle((int)nozzles[n].x, (int)nozzles[n].y + 2, 4.0f, WHITE);
    }
}

int main(void)
{
    InitWindow(WindowWidth, WindowHeight, "Space Invaders");
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

    Image dummyImg = GenImageColor(VID_W, VID_H, BLACK);
    videoFrameTex = LoadTextureFromImage(dummyImg);
    UnloadImage(dummyImg);
    videoFramePixels = (Color*)malloc(VID_W * VID_H * sizeof(Color));

    Vector2 StarPos[STAR_COUNT];
    float StarSpeed[STAR_COUNT];
    for (int i = 0; i < STAR_COUNT; i++)
    {
        StarPos[i] = (Vector2){ (float)GetRandomValue(0, WindowWidth), (float)GetRandomValue(0, WindowHeight) };
        StarSpeed[i] = (float)GetRandomValue(40, 180);
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
    Sound AlienShoot = LoadSound("assets/audio/alienshoot1.wav");
    Sound menuMove = LoadSound("assets/audio/sfx_menu_move4.wav");
    Sound menuSelect = LoadSound("assets/audio/sfx_menu_select2.wav");
    Sound heroDeath = LoadSound("assets/audio/sfx_deathscream_robot4.wav");
    Sound pauseIn = LoadSound("assets/audio/sfx_sounds_pause4_in.wav");
    Sound pauseOut = LoadSound("assets/audio/sfx_sounds_pause4_out.wav");
    Sound damage = LoadSound("assets/audio/sfx_sounds_damage3.wav");
    Sound cheer = LoadSound("assets/audio/cheering.wav");

    Sound heroOuch = LoadSound("assets/audio/hero_hit.wav");
    Sound bossLaserSound = LoadSound("assets/audio/laser_beam.wav");

    Music bgmStory = LoadMusicStream("assets/audio/2-air-strike-2-ost-track-2-ih-23-xz.wav");
    Music bgmMenu  = LoadMusicStream("assets/audio/air-strike-3-d-ii-gulf-thunder-main-ost-best-quality-mf-0-j-34.wav");
    Music bgmBoss  = LoadMusicStream("assets/audio/air-strike-3-d-ost-fear-drigto.wav");
    Music bgmWin   = LoadMusicStream("assets/audio/game_win_bgm.wav");
    Music bgmLost  = LoadMusicStream("assets/audio/game_lost_bgm.wav");

    Texture2D BossTexture[5];
    BossTexture[0] = LoadTexture("assets/sprites/boss_idle.png");
    BossTexture[1] = LoadTexture("assets/sprites/boss_down.png");
    BossTexture[2] = LoadTexture("assets/sprites/boss_up.png");
    BossTexture[3] = LoadTexture("assets/sprites/boss_pulse1.png");
    BossTexture[4] = LoadTexture("assets/sprites/boss_pulse2.png");

    Texture2D HeroTexture = LoadTexture("assets/sprites/Hero.png");
    Texture2D StealthHeroTexture = LoadTexture("assets/sprites/Hero_stealth.png");

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

    Texture2D clusterBombTex   = LoadTexture("assets/sprites/cluster_bomb_idle.png");
    Texture2D clusterBlastTex  = LoadTexture("assets/sprites/cluster_bomb_blast.png");

    Texture2D teamShieldDomeTex = LoadTexture("assets/sprites/team_shield_dome.png");
    Sound sndShieldActivate     = LoadSound("assets/audio/sfx_shield_activate.wav");
    Sound sndShieldDeflect      = LoadSound("assets/audio/sfx_shield_deflect.wav");

    Sound sndHudGlitch          = LoadSound("assets/audio/sfx_hud_glitch.wav");
    Sound sndEvacBoom           = LoadSound("assets/audio/sfx_evac_boom.wav");

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
    bool jammerBulletActive[MAX_COMMANDER_BULLETS] = { false };
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
    jammerTex[1] = LoadTexture("assets/sprites/commander_jammer_move_y.png");
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
    Sound sndPodDestroy   = LoadSound("assets/audio/sfx_pod_destroy.wav");
    Sound sndOrbLaunch    = LoadSound("assets/audio/sfx_orb_launch.wav");
    Sound sndWarningSiren = LoadSound("assets/audio/sfx_warning_siren.wav");
    Sound sndDefibHum     = LoadSound("assets/audio/sfx_defib_hum.wav");
    Sound sndBossEnrage   = LoadSound("assets/audio/sfx_boss_enrage.wav");
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
    Camera2D screenCamera = { 0 };
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
    bool minionActive[MAX_MINIONS] = { false };
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

        if (IsKeyPressed(KEY_F11)) ToggleFullscreen();

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
        SetSoundVolume(sndBossEnrage, SfxVolume); SetSoundVolume(sndRocketBoost, SfxVolume);
        SetSoundVolume(sndClusterLaunch, SfxVolume); SetSoundVolume(sndClusterExplode, SfxVolume);
        SetSoundVolume(sndShieldActivate, SfxVolume); SetSoundVolume(sndShieldDeflect, SfxVolume);
        SetSoundVolume(sndHudGlitch, SfxVolume); SetSoundVolume(sndEvacBoom, SfxVolume);

        SetMusicVolume(bgmStory, BgmVolume); SetMusicVolume(bgmMenu, BgmVolume);
        SetMusicVolume(bgmBoss, BgmVolume); SetMusicVolume(bgmWin, BgmVolume);
        SetMusicVolume(bgmLost, BgmVolume);

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

        BeginDrawing();
        ClearBackground(BLACK);
        BeginMode2D(screenCamera);

        if (StarfieldOn && CurrentState != STATE_LOADING)
        {
            for (int i = 0; i < STAR_COUNT; i++)
            {
                DrawCircle((int)StarPos[i].x, (int)StarPos[i].y, (StarSpeed[i] > 110) ? 2.0f : 1.2f, (StarSpeed[i] > 110) ? SKYBLUE : DARKGRAY);
            }
        }

        // STATE: LOADING SCREEN
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

            char titleText[] = "SPACE INVADERS";
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

            if (LoadingTimer >= 5.0f)
            {
                CurrentState = STATE_CINEMATIC;
                cinematicTimer = 0.0f;
                PlayMusicStream(bgmStory);
            }
        }
        // STATE: CINEMATIC
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
                PlaySound(menuSelect);
                CurrentState = STATE_STORY;
                storyDialogueIndex = 0;
            }
        }
        // STATE: STORY DIALOGUES
        else if (CurrentState == STATE_STORY)
        {
            UpdateMusicStream(bgmStory);
            int panelW = 1260, panelH = 540;
            int panelX = (WindowWidth - panelW) / 2, panelY = (WindowHeight - panelH) / 2;

            DrawRectangle(panelX, panelY, panelW, panelH, Fade(DARKPURPLE, 0.40f));
            DrawRectangleLines(panelX, panelY, panelW, panelH, SKYBLUE);
            DrawRectangleLines(panelX + 6, panelY + 6, panelW - 12, panelH - 12, DARKBLUE);

            char headerText[] = "TRANSMISSION FREQUENCY 142.80 - ORBITAL DEFENSE COMMAND";
            DrawText(headerText, (WindowWidth - MeasureText(headerText, 32)) / 2, panelY + 30, 32, GOLD);
            DrawLine(panelX + 80, panelY + 75, panelX + panelW - 80, panelY + 75, SKYBLUE);

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
            if (((int)(GetTime() * 3)) % 2 == 0)
                DrawText(nextPrompt, (WindowWidth - MeasureText(nextPrompt, 20)) / 2, panelY + panelH - 45, 20, GREEN);

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
            int panelW = 1260, panelH = 760;
            int panelX = (WindowWidth - panelW) / 2, panelY = (WindowHeight - panelH) / 2;

            DrawRectangle(panelX, panelY, panelW, panelH, Fade(DARKPURPLE, 0.25f));
            DrawRectangleLines(panelX, panelY, panelW, panelH, SKYBLUE);
            DrawRectangleLines(panelX + 6, panelY + 6, panelW - 12, panelH - 12, DARKBLUE);

            char menuTitle[] = "SPACE INVADERS : EARTH ALLIANCE";
            DrawText(menuTitle, (WindowWidth - MeasureText(menuTitle, 48)) / 2, panelY + 35, 48, GOLD);
            char menuSub[] = "COMMAND CONSOLE & INTERCEPTION SYSTEM";
            DrawText(menuSub, (WindowWidth - MeasureText(menuSub, 19)) / 2, panelY + 95, 19, RAYWHITE);
            DrawLine(panelX + 100, panelY + 130, panelX + panelW - 100, panelY + 130, SKYBLUE);

            if (IsKeyPressed(KEY_UP)) { PlaySound(menuMove); MenuSelection--; if (MenuSelection < 0) MenuSelection = 5; }
            if (IsKeyPressed(KEY_DOWN)) { PlaySound(menuMove); MenuSelection++; if (MenuSelection > 5) MenuSelection = 0; }

            char itemNames[6][32] = { "PLAY", "HOW TO PLAY?", "SCORES & STATS", "OPTIONS", "CREDENTIALS", "EXIT" };
            for (int i = 0; i < 6; i++)
            {
                int btnW = 560, btnH = 56, btnX = (WindowWidth - btnW) / 2, btnY = panelY + 155 + (i * 82);
                if (MenuSelection == i)
                {
                    DrawRectangle(btnX, btnY, btnW, btnH, Fade(SKYBLUE, 0.25f));
                    DrawRectangleLines(btnX, btnY, btnW, btnH, LIME);
                    DrawRectangle(btnX - 18, btnY + 14, 8, 28, YELLOW);
                    DrawRectangle(btnX + btnW + 10, btnY + 14, 8, 28, YELLOW);
                    DrawText(itemNames[i], (WindowWidth - MeasureText(itemNames[i], 24)) / 2, btnY + 16, 24, YELLOW);
                }
                else
                {
                    DrawRectangle(btnX, btnY, btnW, btnH, Fade(BLACK, 0.70f));
                    DrawRectangleLines(btnX, btnY, btnW, btnH, DARKGRAY);
                    DrawText(itemNames[i], (WindowWidth - MeasureText(itemNames[i], 22)) / 2, btnY + 17, 22, LIGHTGRAY);
                }
            }

            char footerText[] = "USE [UP / DOWN] TO NAVIGATE   or   [ENTER / SPACE] TO EXECUTE";
            DrawText(footerText, (WindowWidth - MeasureText(footerText, 18)) / 2, panelY + panelH - 40, 18, GREEN);

            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE))
            {
                PlaySound(menuSelect);
                if (MenuSelection == 0)
                {
                    StopMusicStream(bgmMenu);
                    CurrentState = STATE_LAUNCH;
                    launchDialogueIndex = 0; launchCountdownTimer = 3.0f; launchCountdownActive = false;
                    launchShip1Pos = (Vector2){ WindowWidth * 0.40f, WindowHeight - 240 };
                    launchShip2Pos = (Vector2){ WindowWidth * 0.60f, WindowHeight - 240 };
                    PlayMusicStream(bgmStory);
                }
                else if (MenuSelection == 1) { CurrentState = STATE_HOWTOPLAY; howToPlayTab = 0; }
                else if (MenuSelection == 2) { CurrentState = STATE_SCORES; }
                else if (MenuSelection == 3) { CurrentState = STATE_OPTIONS; }
                else if (MenuSelection == 4) { CurrentState = STATE_CREDITS; creditsTab = 0; }
                else if (MenuSelection == 5) break;
            }
        }
        // STATE: HOW TO PLAY?
        else if (CurrentState == STATE_HOWTOPLAY)
        {
            UpdateMusicStream(bgmMenu);
            int panelW = 1320, panelH = 760;
            int panelX = (WindowWidth - panelW) / 2, panelY = (WindowHeight - panelH) / 2;

            DrawRectangle(panelX, panelY, panelW, panelH, Fade(DARKBLUE, 0.25f));
            DrawRectangleLines(panelX, panelY, panelW, panelH, SKYBLUE);
            DrawRectangleLines(panelX + 6, panelY + 6, panelW - 12, panelH - 12, DARKBLUE);

            char htpTitle[] = "OPERATION MANUAL : FLIGHT & COMBAT TACTICS";
            DrawText(htpTitle, (WindowWidth - MeasureText(htpTitle, 36)) / 2, panelY + 30, 36, GOLD);

            if (IsKeyPressed(KEY_LEFT))  { PlaySound(menuMove); howToPlayTab--; if (howToPlayTab < 0) howToPlayTab = 3; }
            if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_TAB)) { PlaySound(menuMove); howToPlayTab++; if (howToPlayTab > 3) howToPlayTab = 0; }

            const char* tabNames[4] = { "1. BASIC CONTROLS", "2. ENEMY INTEL", "3. ARSENAL & SKILLS", "4. BOSS RAID TACTICS" };
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
                DrawText("> PILOT 1: SHOAB MAHMUD [AEGIS-1] (Interceptor Alpha)", textX + 20, contentY + 40, 20, LIME);
                DrawText("  - Controls: [A] Move Left  |  [D] Move Right  |  [W] or [SPACE] Fire Twin Plasma Blasters", textX + 40, contentY + 70, 18, RAYWHITE);
                DrawText("  - Special:  [Q] High-Energy Hyper Laser Beam (Fires a piercing vertical death ray through all aliens).", textX + 40, contentY + 98, 18, SKYBLUE);

                DrawText("> PILOT 2: NAYEMUL ISLAM [SHADOW-2] (Stealth Interceptor Beta)", textX + 20, contentY + 145, 20, YELLOW);
                DrawText("  - Controls: [LEFT ARROW] Move Left  |  [RIGHT ARROW] Move Right  |  [UP ARROW] Fire Blasters", textX + 40, contentY + 175, 18, RAYWHITE);
                DrawText("  - Special:  [RIGHT SHIFT] 8-Way Guided Cluster Missiles (Blankets the airspace with kinetic salvo).", textX + 40, contentY + 203, 18, SKYBLUE);

                DrawText("> SYNCHRONIZED TEAM SHIELD DOME [ENTER]", textX + 20, contentY + 250, 20, GOLD);
                DrawText("  - Fly within 375 pixels of each other to establish a quantum energy tether.", textX + 40, contentY + 280, 18, RAYWHITE);
                DrawText("  - Press [ENTER] to project an indestructible 240px half-sphere dome for 4.0 seconds (25s recharge).", textX + 40, contentY + 308, 18, LIME);
                DrawText("  - Deflects all regular bullets, commander ordnance, and vaporizes any grunt alien touching the dome!", textX + 40, contentY + 336, 18, SKYBLUE);

                DrawText("> SYSTEM UTILITY:  [P] Pause Simulation  |  [F11] Fullscreen Mode  |  [ESC] Tactical Menu", textX + 20, contentY + 390, 18, LIGHTGRAY);
            }
            else if (howToPlayTab == 1)
            {
                DrawText("EXOSPHERIC HOSTILE RECONNAISSANCE", textX, contentY, 22, YELLOW);
                DrawText("> REGULAR INVASION SWARM:", textX + 20, contentY + 40, 20, RED);
                DrawText("  - Descends from deep space in synchronized formation with variable thruster speeds.", textX + 40, contentY + 70, 18, RAYWHITE);
                DrawText("  - Bounces off screen boundaries and steps 20 pixels lower each bounce.", textX + 40, contentY + 98, 18, LIGHTGRAY);
                DrawText("  - Tactical Alert: Do NOT allow the armada to descend past the 70% orbital defense barricade!", textX + 40, contentY + 126, 18, ORANGE);

                DrawText("> ELITE ALIEN COMMANDERS (Spawn at 65% wave eradication):", textX + 20, contentY + 175, 20, MAGENTA);
                DrawText("  1. COMMANDER JAMMER (Left Sector Patrol - 35 HP):", textX + 40, contentY + 205, 19, (Color){ 220, 100, 255, 255 });
                DrawText("     - Fires heavy tracking purple bolts.", textX + 60, contentY + 233, 18, RAYWHITE);
                DrawText("     - Periodically discharges high-frequency EMP waves that scramble cockpit radar and HUD telemetry.", textX + 60, contentY + 261, 18, LIGHTGRAY);
                DrawText("  2. COMMANDER WARP (Right Sector Patrol - 30 HP):", textX + 40, contentY + 305, 19, SKYBLUE);
                DrawText("     - High-speed spatial interceptor armed with dual high-velocity blue accelerators.", textX + 60, contentY + 333, 18, RAYWHITE);
                DrawText("     - Senses incoming hero projectiles and executes emergency micro-teleports across the screen!", textX + 60, contentY + 361, 18, LIGHTGRAY);
            }
            else if (howToPlayTab == 2)
            {
                DrawText("HERO ARSENAL, FIELD REVIVAL & TACTICAL EMP", textX, contentY, 22, YELLOW);
                DrawText("> SHOAB'S HYPER LASER CANNON [Q]:", textX + 20, contentY + 40, 20, LIME);
                DrawText("  - Takes 15 seconds to charge. Pierces through entire alien columns in a single devastating strike.", textX + 40, contentY + 70, 18, RAYWHITE);
                DrawText("  - Deals 50 instant damage directly into the Dreadnought's Shield Pods!", textX + 40, contentY + 98, 18, SKYBLUE);

                DrawText("> NAYEMUL'S GUIDED CLUSTER BARRAGE [RIGHT SHIFT]:", textX + 20, contentY + 145, 20, YELLOW);
                DrawText("  - Launches an 8-missile kinetic salvo that detonates on contact into massive expanding blast radii.", textX + 40, contentY + 175, 18, RAYWHITE);
                DrawText("  - Obliterates swarms and deals 40 focused damage to the targeted boss pod.", textX + 40, contentY + 203, 18, SKYBLUE);

                DrawText("> COMBAT DEFIBRILLATOR & CRASH REVIVAL:", textX + 20, contentY + 250, 20, GOLD);
                DrawText("  - If either hero is shot down, a distress beacon marks their crash site.", textX + 40, contentY + 280, 18, RAYWHITE);
                DrawText("  - If the survivor has lives remaining, fly directly onto the beacon and hold position for 1.5-2.0s.", textX + 40, contentY + 308, 18, LIME);
                DrawText("  - Your companion is revived instantly into battle at the cost of 1 shared extra life!", textX + 40, contentY + 336, 18, YELLOW);

                DrawText("> ORBITAL EMP AIRDROP:", textX + 20, contentY + 380, 20, SKYBLUE);
                DrawText("  - Destroying both boss pods prompts Earth Command to drop an EMP power core. Fly into it to trigger", textX + 40, contentY + 410, 18, RAYWHITE);
                DrawText("    an orbital shockwave and gain 8.5 seconds of Rapid Plasma Overcharge with 35% boosted fire rate!", textX + 40, contentY + 438, 18, LIME);
            }
            else
            {
                DrawText("BOSS RAID GUIDE: THE DREADNOUGHT CARRIER", textX, contentY, 22, YELLOW);
                DrawText("> PHASE 1: BILATERAL DEFLECTOR PODS (75 HP Each)", textX + 20, contentY + 40, 20, ORANGE);
                DrawText("  - The boss core is IMMUNE while deflector pods remain online. Regular bullets deal zero core damage!", textX + 40, contentY + 70, 18, RAYWHITE);
                DrawText("  - Focus fire exclusively on the Left Deflector Pod (Red) and Right Deflector Pod (Magenta).", textX + 40, contentY + 98, 18, YELLOW);
                DrawText("  - Deploy Shoab's Laser and Nayemul's Cluster bombs directly onto the pods to tear down shields.", textX + 40, contentY + 126, 18, LIME);

                DrawText("> MINION ESCORTS & HOMING DISRUPTION ORBS:", textX + 20, contentY + 175, 20, MAGENTA);
                DrawText("  - The Dreadnought launches 2-HP minion escorts. Destroy pods quickly to stop endless reinforcements.", textX + 40, contentY + 205, 18, RAYWHITE);
                DrawText("  - Dodge purple homing orbs! Getting struck will disable and jam your weapons for 4.5 seconds.", textX + 40, contentY + 233, 18, RED);

                DrawText("> MASTERING THE MAIN LASER WARNING POINTER (CRITICAL!):", textX + 20, contentY + 280, 20, RED);
                DrawText("  - Before firing its lethal mega beam, a red aiming pointer renders for 0.8 seconds as a warning.", textX + 40, contentY + 308, 18, YELLOW);
                DrawText("  - The warning pointer deals NO DAMAGE. However, the boss will move while aiming!", textX + 40, contentY + 336, 18, LIME);
                DrawText("  - Use this 0.8s warning window to predict the boss's trajectory and steer clear of its firing path.", textX + 40, contentY + 364, 18, RAYWHITE);

                DrawText("> PHASE 2: EXPOSED REACTOR CORE & FINAL SURGE (100 HP)", textX + 20, contentY + 410, 20, GOLD);
                DrawText("  - With both pods destroyed, the center core opens! Below 40 HP, the Dreadnought enrages with boosted", textX + 40, contentY + 440, 18, RAYWHITE);
                DrawText("    thrusters. Concentrate all fire onto the center core to trigger the evacuation sequence!", textX + 40, contentY + 468, 18, LIME);
            }

            char htpFooter[] = "[< LEFT / RIGHT >] SWITCH MANUAL TAB   |   [BACKSPACE / ESC] RETURN TO MENU";
            DrawText(htpFooter, (WindowWidth - MeasureText(htpFooter, 18)) / 2, panelY + panelH - 40, 18, GREEN);

            if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressed(KEY_ESCAPE))
            {
                PlaySound(menuSelect);
                CurrentState = STATE_MENU;
            }
        }
        // STATE: SCORES & COMBAT RECORDS
        else if (CurrentState == STATE_SCORES)
        {
            UpdateMusicStream(bgmMenu);
            int panelW = 1260, panelH = 740;
            int panelX = (WindowWidth - panelW) / 2, panelY = (WindowHeight - panelH) / 2;

            DrawRectangle(panelX, panelY, panelW, panelH, Fade(DARKBLUE, 0.25f));
            DrawRectangleLines(panelX, panelY, panelW, panelH, SKYBLUE);
            DrawRectangleLines(panelX + 6, panelY + 6, panelW - 12, panelH - 12, DARKBLUE);

            char scrTitle[] = "TACTICAL COMBAT RECORDS & SCORES";
            DrawText(scrTitle, (WindowWidth - MeasureText(scrTitle, 38)) / 2, panelY + 40, 38, GOLD);
            char scrSub[] = "HEAD-TO-HEAD BATTLE TELEMETRY & PERFORMANCE METRICS";
            DrawText(scrSub, (WindowWidth - MeasureText(scrSub, 19)) / 2, panelY + 90, 19, RAYWHITE);
            DrawLine(panelX + 80, panelY + 120, panelX + panelW - 80, panelY + 120, SKYBLUE);

            int cardW = 540, cardH = 500, cardY = panelY + 145;
            int card1X = panelX + 60; // Nayemul on Left
            int card2X = panelX + panelW - cardW - 60; // Shoab on Right

            // CARD 1: NAYEMUL (LEFT)
            DrawRectangle(card1X, cardY, cardW, cardH, Fade(BLACK, 0.75f));
            DrawRectangleLines(card1X, cardY, cardW, cardH, YELLOW);
            DrawRectangleLines(card1X + 4, cardY + 4, cardW - 8, cardH - 8, Fade(DARKBLUE, 0.7f));

            DrawText("PILOT: NAYEMUL ISLAM", card1X + (cardW - MeasureText("PILOT: NAYEMUL ISLAM", 24)) / 2, cardY + 22, 24, YELLOW);
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

            // CARD 2: SHOAB (RIGHT)
            DrawRectangle(card2X, cardY, cardW, cardH, Fade(BLACK, 0.75f));
            DrawRectangleLines(card2X, cardY, cardW, cardH, LIME);
            DrawRectangleLines(card2X + 4, cardY + 4, cardW - 8, cardH - 8, Fade(DARKBLUE, 0.7f));

            DrawText("PILOT: MD. SHOAB MAHMUD", card2X + (cardW - MeasureText("PILOT: MD. SHOAB MAHMUD", 24)) / 2, cardY + 22, 24, LIME);
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

            char scrBack[] = "PRESS [BACKSPACE] OR [ESC] TO RETURN TO MENU";
            DrawText(scrBack, (WindowWidth - MeasureText(scrBack, 18)) / 2, panelY + panelH - 35, 18, GREEN);

            if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressed(KEY_ESCAPE))
            {
                PlaySound(menuSelect);
                CurrentState = STATE_MENU;
            }
        }
        // STATE: CREDITS SCREEN (TWO INTERACTIVE TABS)
        else if (CurrentState == STATE_CREDITS)
        {
            UpdateMusicStream(bgmMenu);
            int panelW = 1260, panelH = 750;
            int panelX = (WindowWidth - panelW) / 2, panelY = (WindowHeight - panelH) / 2;

            DrawRectangle(panelX, panelY, panelW, panelH, Fade(DARKGRAY, 0.25f));
            DrawRectangleLines(panelX, panelY, panelW, panelH, SKYBLUE);
            DrawRectangleLines(panelX + 6, panelY + 6, panelW - 12, panelH - 12, DARKBLUE);

            char credHeader[] = "PROJECT DEVELOPERS & CREDENTIALS";
            DrawText(credHeader, (WindowWidth - MeasureText(credHeader, 38)) / 2, panelY + 35, 38, GOLD);

            if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_TAB))
            {
                PlaySound(menuMove);
                creditsTab = 1 - creditsTab;
            }

            int tabW = 340, tabH = 44, tabY = panelY + 88;
            int t1X = (WindowWidth - (tabW * 2 + 20)) / 2;
            int t2X = t1X + tabW + 20;

            DrawRectangle(t1X, tabY, tabW, tabH, (creditsTab == 0) ? Fade(SKYBLUE, 0.35f) : Fade(BLACK, 0.65f));
            DrawRectangleLines(t1X, tabY, tabW, tabH, (creditsTab == 0) ? YELLOW : DARKGRAY);
            DrawText("1. NAYEMUL ISLAM (LEAD)", t1X + (tabW - MeasureText("1. NAYEMUL ISLAM (LEAD)", 18)) / 2, tabY + 13, 18, (creditsTab == 0) ? YELLOW : LIGHTGRAY);

            DrawRectangle(t2X, tabY, tabW, tabH, (creditsTab == 1) ? Fade(SKYBLUE, 0.35f) : Fade(BLACK, 0.65f));
            DrawRectangleLines(t2X, tabY, tabW, tabH, (creditsTab == 1) ? LIME : DARKGRAY);
            DrawText("2. MD. SHOAB MAHMUD", t2X + (tabW - MeasureText("2. MD. SHOAB MAHMUD", 18)) / 2, tabY + 13, 18, (creditsTab == 1) ? LIME : LIGHTGRAY);

            DrawLine(panelX + 80, tabY + 58, panelX + panelW - 80, tabY + 58, SKYBLUE);

            int cardW = 1120, cardH = 500, cardX = (WindowWidth - cardW) / 2, cardY = tabY + 70;
            DrawRectangle(cardX, cardY, cardW, cardH, Fade(BLACK, 0.80f));
            DrawRectangleLines(cardX, cardY, cardW, cardH, (creditsTab == 0) ? YELLOW : LIME);

            if (creditsTab == 0)
            {
                // NAYEMUL ISLAM'S TAB
                int pSize = 135, pX = cardX + 50, pY = cardY + 55;
                DrawText("Nayemul Islam", pX + (pSize - MeasureText("Nayemul Islam", 22)) / 2, cardY + 22, 22, YELLOW);

                DrawRectangle(pX - 3, pY - 3, pSize + 6, pSize + 6, Fade(BLACK, 0.85f));
                if (portraitNayemul.id > 0)
                    DrawTexturePro(portraitNayemul, (Rectangle){ 0, 0, (float)portraitNayemul.width, (float)portraitNayemul.height },
                                   (Rectangle){ (float)pX, (float)pY, (float)pSize, (float)pSize }, (Vector2){0,0}, 0.0f, WHITE);
                else
                    DrawRectangle(pX, pY, pSize, pSize, Fade(DARKBROWN, 0.5f));
                DrawRectangleLinesEx((Rectangle){ (float)pX, (float)pY, (float)pSize, (float)pSize }, 2.5f, YELLOW);

                DrawText("Roll ID: 2505087", pX + (pSize - MeasureText("Roll ID: 2505087", 18)) / 2, pY + pSize + 15, 18, GREEN);
                DrawText("CSE, Section B", pX + (pSize - MeasureText("CSE, Section B", 16)) / 2, pY + pSize + 40, 16, SKYBLUE);

                DrawLine(pX + pSize + 40, cardY + 25, pX + pSize + 40, cardY + cardH - 25, DARKGRAY);

                int listX = pX + pSize + 65;
                int itemY = cardY + 25;
                DrawText("CORE CONTRIBUTIONS & ARCHITECTURE:", listX, itemY, 20, GOLD);
                itemY += 28;

                const char* nayemulContribs[] = {
                    "1. Introduced the full interactive Story Mode & Narrative into the game",
                    "2. Added the Alien Dreadnought Carrier Boss mechanics, pods & rage states",
                    "3. Added the Alien Commanders (Jammer EMP & Warp micro-teleportation)",
                    "4. Added shooting logic & projectile physics for all alien types and heroes",
                    "5. Added Hero's 8-Way Guided Cluster Missile feature with area blasts",
                    "6. Added tactical dialogues, coms, and emergency planetary broadcasts",
                    "7. Designed the loading screen, visual effects, and CRT scanline filter engine",
                    "8. Added 98% of in-game audio sound effects (SFX) and background music tracks",
                    "9. Integrated authentic in-game voice recordings for both Nayemul & Shoab",
                    "10. Designed and edited the menu buttons and layout of the starting scene",
                    "11. Tested, debunked, and resolved critical system bugs and gameplay mechanisms",
                    "12. Implemented experimental prototypes and innovative feature mechanics"
                };

                for (int c = 0; c < 12; c++)
                {
                    DrawText(nayemulContribs[c], listX, itemY, 16, (c % 2 == 0) ? RAYWHITE : LIGHTGRAY);
                    itemY += 24;
                }
            }
            else
            {
                // MD. SHOAB MAHMUD'S TAB
                int pSize = 135, pX = cardX + 50, pY = cardY + 55;
                DrawText("Md. Shoab Mahmud", pX + (pSize - MeasureText("Md. Shoab Mahmud", 22)) / 2, cardY + 22, 22, LIME);

                DrawRectangle(pX - 3, pY - 3, pSize + 6, pSize + 6, Fade(BLACK, 0.85f));
                if (portraitShoab.id > 0)
                    DrawTexturePro(portraitShoab, (Rectangle){ 0, 0, (float)portraitShoab.width, (float)portraitShoab.height },
                                   (Rectangle){ (float)pX, (float)pY, (float)pSize, (float)pSize }, (Vector2){0,0}, 0.0f, WHITE);
                else
                    DrawRectangle(pX, pY, pSize, pSize, Fade(DARKGREEN, 0.5f));
                DrawRectangleLinesEx((Rectangle){ (float)pX, (float)pY, (float)pSize, (float)pSize }, 2.5f, LIME);

                DrawText("Roll ID: 2505066", pX + (pSize - MeasureText("Roll ID: 2505066", 18)) / 2, pY + pSize + 15, 18, GREEN);
                DrawText("CSE, Section B", pX + (pSize - MeasureText("CSE, Section B", 16)) / 2, pY + pSize + 40, 16, SKYBLUE);

                DrawLine(pX + pSize + 40, cardY + 25, pX + pSize + 40, cardY + cardH - 25, DARKGRAY);

                int listX = pX + pSize + 65;
                int itemY = cardY + 35;
                DrawText("CORE CONTRIBUTIONS & ARCHITECTURE:", listX, itemY, 21, GOLD);
                itemY += 38;

                const char* shoabContribs[] = {
                    "1. Built the basic core foundation and base loop of the game",
                    "2. Worked with regular alien sprites, grid positioning, and sound fx",
                    "3. Added the sprite and core mechanisms of the Scorpion Hero (Aegis-1)",
                    "4. Added the devastating special Hyper Laser Beam feature of Scorpion",
                    "5. Added custom sprites and sound effects for the Scorpion Laser Beam",
                    "6. Coordinated and assisted Nayemul in various feature implementations & debugging"
                };

                for (int c = 0; c < 6; c++)
                {
                    DrawText(shoabContribs[c], listX, itemY, 18, (c % 2 == 0) ? RAYWHITE : LIGHTGRAY);
                    itemY += 34;
                }
            }

            char credBack[] = "[< LEFT / RIGHT >] SWITCH DEVELOPER TAB   |   [BACKSPACE / ESC] RETURN TO MENU";
            DrawText(credBack, (WindowWidth - MeasureText(credBack, 18)) / 2, panelY + panelH - 35, 18, GREEN);

            if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressed(KEY_ESCAPE))
            {
                PlaySound(menuSelect);
                CurrentState = STATE_MENU;
            }
        }
        // STATE: OPTIONS
        else if (CurrentState == STATE_OPTIONS)
        {
            UpdateMusicStream(bgmMenu);
            int panelW = 1260, panelH = 740;
            int panelX = (WindowWidth - panelW) / 2, panelY = (WindowHeight - panelH) / 2;

            DrawRectangle(panelX, panelY, panelW, panelH, Fade(DARKBLUE, 0.20f));
            DrawRectangleLines(panelX, panelY, panelW, panelH, SKYBLUE);
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
                    PlaySound(menuSelect); ToggleFullscreen();
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
            DrawText(optFooter, (WindowWidth - MeasureText(optFooter, 18)) / 2, panelY + panelH - 45, 18, GREEN);

            if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressed(KEY_ESCAPE))
            {
                PlaySound(menuSelect);
                CurrentState = STATE_MENU;
            }
        }
        // STATE: LAUNCH BRIEFING
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
                launchShip1Pos.y -= 180.0f * Time; launchShip2Pos.y -= 180.0f * Time;
                if (launchCountdownTimer <= 0.0f)
                {
                    StopMusicStream(bgmStory);
                    // Launch cutscene video begins immediately when ships leave screen[cite: 10]
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

        // STATE: ACTIVE GAMEPLAY
        else if (CurrentState == STATE_GAMEPLAY)
        {
            if (bossWarningActive || bossWarpActive || bossActive) UpdateMusicStream(bgmBoss);

            if (!GameOver && !bossDefeated && !deathSequenceActive && IsKeyPressed(KEY_P))
            {
                isPaused = !isPaused;
                if (isPaused) PlaySound(pauseIn);
                else PlaySound(pauseOut);
            }

            float heroDistX = fabsf(Hero1Pos.x - Hero2Pos.x);
            bool heroesTethered = (Hero1Lives > 0 && Hero2Lives > 0 && heroDistX <= (WindowWidth / 4.0f));

            if (teamShieldCooldownTimer > 0.0f && !isPaused) teamShieldCooldownTimer -= Time;
            if (teamShieldActive && !isPaused)
            {
                teamShieldActiveTimer -= Time;
                if (teamShieldActiveTimer <= 0.0f) teamShieldActive = false;
            }

            if (bossWarpActive && !isPaused)
            {
                bossWarpTimer -= Time;
                if (bossWarpTimer <= 0.0f)
                {
                    bossWarpActive = false;
                    bossActive = true;
                    screenCamera.offset = (Vector2){ 0, 0 };
                }
            }

            if (evacActive && !isPaused)
            {
                evacTimer -= Time;
                if (evacTimer <= 0.0f)
                {
                    evacActive = false;
                    // Trigger game victory cutscene video
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

            if (teamShieldActive && !bossActive && !bossSpawned && !isPaused)
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

            if (teamShieldActive && !isPaused)
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
                char pauseSub[] = "Press [P] to Resume   |   [ESC] for Menu";
                DrawText(pauseSub, pBoxX + (pBoxW - MeasureText(pauseSub, 18)) / 2, pBoxY + 110, 18, RAYWHITE);

                if (IsKeyPressed(KEY_ESCAPE))
                {
                    PlaySound(menuSelect); isPaused = false;
                    StopMusicStream(bgmBoss); CurrentState = STATE_MENU; PlayMusicStream(bgmMenu);
                }
                EndMode2D(); EndDrawing(); continue;
            }

            if (hero1HitFlashTimer > 0.0f) hero1HitFlashTimer -= Time;
            if (hero2HitFlashTimer > 0.0f) hero2HitFlashTimer -= Time;
            if (radarJammedTimer > 0.0f)   radarJammedTimer -= Time;
            if (!isPaused && Hero1Lives > 0) ShoabSpecialTime += 1.0f;
            if (!isPaused && Hero2Lives > 0) NayemulSpecialTime += 1.0f;

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
                    jammerShootTimer = 1.5f; jammerActionCooldown = 5.0f; jammerPowerTimer = 0.0f; jammerAnimState = 0;

                    warpPos = (Vector2){ WindowWidth * 0.75f - COMMANDER_SIZE / 2.0f, 130.0f };
                    warpSpeed = (Vector2){ 140.0f, 0.0f }; warpHp = warpMaxHp; warpActive = true;
                    warpShootTimer = 2.0f; warpActionCooldown = 4.0f; warpPowerTimer = 0.0f; warpAnimState = 0;
                    PlaySound(sndTeleport);
                }
            }

            // COMMANDER 1: JAMMER
            if (jammerActive)
            {
                jammerPos.x += jammerSpeed.x * Time;
                if (jammerPos.x <= 40) { jammerPos.x = 40; jammerSpeed.x = fabsf(jammerSpeed.x); }
                else if (jammerPos.x >= WindowWidth - COMMANDER_SIZE - 40) { jammerPos.x = WindowWidth - COMMANDER_SIZE - 40; jammerSpeed.x = -fabsf(jammerSpeed.x); }

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
                                PlaySound(sndJammerShot); break;
                            }
                        }
                    }
                    jammerActionCooldown -= Time;
                    if (jammerActionCooldown <= 0.0f)
                    {
                        jammerActionCooldown = 8.5f; jammerPowerTimer = 1.4f; radarJammedTimer = 5.0f;
                        PlaySound(sndEmpBlast); PlaySound(sndJammerHum);
                    }
                }
                if (jammerPowerTimer > 0.0f) { jammerPowerTimer -= Time; jammerAnimState = 3; }
                else jammerAnimState = (fabsf(jammerSpeed.x) > 0.0f) ? 2 : 0;
            }

            // COMMANDER 2: WARP
            if (warpActive)
            {
                warpPos.x += warpSpeed.x * Time;
                if (warpPos.x <= 40) { warpPos.x = 40; warpSpeed.x = fabsf(warpSpeed.x); }
                else if (warpPos.x >= WindowWidth - COMMANDER_SIZE - 40) { warpPos.x = WindowWidth - COMMANDER_SIZE - 40; warpSpeed.x = -fabsf(warpSpeed.x); }

                if (!deathSequenceActive)
                {
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
                }
                if (warpPowerTimer > 0.0f) { warpPowerTimer -= Time; warpAnimState = 3; }
                else warpAnimState = (fabsf(warpSpeed.x) > 0.0f) ? 2 : 0;
            }

            // COMMANDER PROJECTILES
            for (int b = 0; b < MAX_COMMANDER_BULLETS; b++)
            {
                if (jammerBulletActive[b])
                {
                    jammerBulletPos[b].y += 420.0f * Time;
                    if (jammerBulletPos[b].y > WindowHeight) jammerBulletActive[b] = false;
                    else if (!deathSequenceActive && !teamShieldActive)
                    {
                        Rectangle jbRec = { jammerBulletPos[b].x - 4, jammerBulletPos[b].y, 8, 22 };
                        Rectangle h1Rec = { Hero1Pos.x - HeroWidth / 2.0f, Hero1Pos.y, HeroWidth, HeroHeight };
                        Rectangle h2Rec = { Hero2Pos.x - HeroWidth / 2.0f, Hero2Pos.y, HeroWidth, HeroHeight };

                        if (Hero1Lives > 0 && CheckCollisionRecs(jbRec, h1Rec))
                        {
                            jammerBulletActive[b] = false; Hero1Lives--; Hero1HitsTaken++;
                            hero1HitFlashTimer = 0.35f; PlaySound(heroOuch); PlaySound(sndHudGlitch);
                            if (Hero1Lives <= 0) { Hero1CrashPos = Hero1Pos; PlaySound(heroDeath); if (Hero2Lives <= 0) { deathSequenceActive = true; deathDelayTimer = 0.0f; } }
                        }
                        else if (Hero2Lives > 0 && CheckCollisionRecs(jbRec, h2Rec))
                        {
                            jammerBulletActive[b] = false; Hero2Lives--; Hero2HitsTaken++;
                            hero2HitFlashTimer = 0.35f; PlaySound(heroOuch); PlaySound(sndHudGlitch);
                            if (Hero2Lives <= 0) { Hero2CrashPos = Hero2Pos; PlaySound(heroDeath); if (Hero1Lives <= 0) { deathSequenceActive = true; deathDelayTimer = 0.0f; } }
                        }
                    }
                }
                if (warpBulletActive[b])
                {
                    warpBulletPos[b].y += 500.0f * Time;
                    if (warpBulletPos[b].y > WindowHeight) warpBulletActive[b] = false;
                    else if (!deathSequenceActive && !teamShieldActive)
                    {
                        Rectangle wbRec = { warpBulletPos[b].x - 3, warpBulletPos[b].y, 6, 26 };
                        Rectangle h1Rec = { Hero1Pos.x - HeroWidth / 2.0f, Hero1Pos.y, HeroWidth, HeroHeight };
                        Rectangle h2Rec = { Hero2Pos.x - HeroWidth / 2.0f, Hero2Pos.y, HeroWidth, HeroHeight };

                        if (Hero1Lives > 0 && CheckCollisionRecs(wbRec, h1Rec))
                        {
                            warpBulletActive[b] = false; Hero1Lives--; Hero1HitsTaken++;
                            hero1HitFlashTimer = 0.35f; PlaySound(heroOuch); PlaySound(sndHudGlitch);
                            if (Hero1Lives <= 0) { Hero1CrashPos = Hero1Pos; PlaySound(heroDeath); if (Hero2Lives <= 0) { deathSequenceActive = true; deathDelayTimer = 0.0f; } }
                        }
                        else if (Hero2Lives > 0 && CheckCollisionRecs(wbRec, h2Rec))
                        {
                            warpBulletActive[b] = false; Hero2Lives--; Hero2HitsTaken++;
                            hero2HitFlashTimer = 0.35f; PlaySound(heroOuch); PlaySound(sndHudGlitch);
                            if (Hero2Lives <= 0) { Hero2CrashPos = Hero2Pos; PlaySound(heroDeath); if (Hero1Lives <= 0) { deathSequenceActive = true; deathDelayTimer = 0.0f; } }
                        }
                    }
                }
            }

            // Death sequence: starts Game Over cutscene video
            if (deathSequenceActive)
            {
                deathDelayTimer += Time;
                if (deathDelayTimer >= 1.2f)
                {
                    deathSequenceActive = false;
                    bossLaserActive = false;
                    screenCamera.offset = (Vector2){ 0, 0 };
                    StopMusicStream(bgmBoss);
                    StopSound(bossLaserSound);
                    StopSound(sndDefibHum);
                    StopSound(sndWarningSiren);

                    CurrentState = STATE_VIDEO_PLAY;
                    StartVideo(1, 10.5f);
                }
            }

            if (IsKeyPressed(KEY_ESCAPE) && !deathSequenceActive && !GameOver && !bossDefeated)
            {
                PlaySound(menuSelect); StopMusicStream(bgmBoss);
                StopSound(bossLaserSound); StopSound(sndDefibHum); StopSound(sndWarningSiren);
                CurrentState = STATE_MENU; PlayMusicStream(bgmMenu);
            }

            // REVIVAL LOGIC
            if (Hero1Lives <= 0 && Hero2Lives > 1)
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

            if (Hero2Lives <= 0 && Hero1Lives > 1)
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

            // GAME LOST
            if (GameOver)
            {
                bossLaserActive = false; screenCamera.offset = (Vector2){ 0, 0 };
                StopMusicStream(bgmBoss); StopSound(bossLaserSound); StopSound(sndDefibHum); StopSound(sndWarningSiren);
                if (!lostBgmStarted) { PlayMusicStream(bgmLost); lostBgmStarted = true; }
                UpdateMusicStream(bgmLost);

                int panelW = 1200, panelH = 540, panelX = (WindowWidth - panelW) / 2, panelY = (WindowHeight - panelH) / 2;
                DrawRectangle(panelX, panelY, panelW, panelH, Fade(BLACK, 0.90f));
                DrawRectangleLines(panelX, panelY, panelW, panelH, RED);
                DrawRectangleLines(panelX + 4, panelY + 4, panelW - 8, panelH - 8, MAROON);

                if (lostDialogueIndex == 0)
                {
                    DrawText("[ INCOMING ENCRYPTED TRANSMISSION - ALIEN DREADNOUGHT OVERLORD ]", panelX + 50, panelY + 35, 23, RED);
                    DrawLine(panelX + 50, panelY + 70, panelX + panelW - 50, panelY + 70, RED);
                    DrawText("Alien Dreadnought Boss:", panelX + 50, panelY + 105, 26, MAROON);
                    DrawText("\"HA HA HA! Look at your pathetic interceptors burning in orbital debris!\"", panelX + 50, panelY + 155, 23, RED);
                    DrawText("\"Both of your 'heroes' - Nayemul and Shoab - have been obliterated!\"", panelX + 50, panelY + 205, 23, RED);
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
                    DrawText(titleText, (WindowWidth - MeasureText(titleText, 42)) / 2, panelY + 45, 42, RED);
                    DrawLine(panelX + 80, panelY + 105, panelX + panelW - 80, panelY + 105, RED);
                    char subText[] = "Both heroes fell in battle. The alien fleet has conquered our world!";
                    DrawText(subText, (WindowWidth - MeasureText(subText, 24)) / 2, panelY + 145, 24, WHITE);
                    DrawText(TextFormat("Shoab's Final Score: %05d", Hero1Score), panelX + 180, panelY + 220, 26, LIME);
                    DrawText(TextFormat("Nayemul's Final Score: %05d", Hero2Score), panelX + 680, panelY + 220, 26, YELLOW);
                    char restartText[] = "Press [R] to Restart Defense   |   [ESC] Main Menu";
                    DrawText(restartText, (WindowWidth - MeasureText(restartText, 22)) / 2, panelY + 360, 22, GOLD);
                }

                if (lostDialogueIndex < 2)
                {
                    char promptText[] = "PRESS [ENTER] OR [SPACE] TO ADVANCE TRANSMISSION";
                    if (((int)(GetTime() * 3)) % 2 == 0)
                        DrawText(promptText, (WindowWidth - MeasureText(promptText, 18)) / 2, panelY + panelH - 45, 18, RED);
                    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) { PlaySound(menuSelect); lostDialogueIndex++; }
                }

                if (IsKeyPressed(KEY_R))
                {
                    PlaySound(menuSelect); StopMusicStream(bgmLost); lostBgmStarted = false; lostDialogueIndex = 0;
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
                    for (int m = 0; m < MAX_CLUSTER_MISSILES; m++) clusterMissileActive[m] = false;
                    for (int b = 0; b < MAX_CLUSTER_BLASTS; b++) clusterBlastActive[b] = false;
                    explosionShakeTimer = 0.0f; teamShieldActive = false; teamShieldActiveTimer = 0.0f; teamShieldCooldownTimer = 0.0f; teamShieldCenterX = 0.0f;
                    commandersTriggered = false; commanderIntroActive = false; commanderIntroTimer = 0.0f; gameTimeDilation = 1.0f;
                    jammerActive = false; warpActive = false; jammerHp = 0; warpHp = 0; radarJammedTimer = 0.0f;
                    jammerPowerTimer = 0.0f; warpPowerTimer = 0.0f; bossEnraged = false;
                    for (int b = 0; b < MAX_COMMANDER_BULLETS; b++) { jammerBulletActive[b] = false; warpBulletActive[b] = false; }
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
                if (IsKeyPressed(KEY_ESCAPE))
                {
                    PlaySound(menuSelect); StopMusicStream(bgmLost); lostBgmStarted = false; lostDialogueIndex = 0;
                    CurrentState = STATE_MENU; PlayMusicStream(bgmMenu);
                }
                EndMode2D(); EndDrawing(); continue;
            }

            // GAME WON
            if (bossDefeated)
            {
                bossLaserActive = false; screenCamera.offset = (Vector2){ 0, 0 };
                StopMusicStream(bgmBoss); StopSound(bossLaserSound); StopSound(sndDefibHum); StopSound(sndWarningSiren);
                if (!winSoundPlayed) { winSoundPlayed = true; }
                if (!winBgmStarted) { PlayMusicStream(bgmWin); winBgmStarted = true; }
                UpdateMusicStream(bgmWin);

                int panelW = 1260, panelH = 580, panelX = (WindowWidth - panelW) / 2, panelY = (WindowHeight - panelH) / 2;
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
                    DrawText(titleText, (WindowWidth - MeasureText(titleText, 38)) / 2, panelY + 30, 38, GREEN);
                    DrawLine(panelX + 80, panelY + 75, panelX + panelW - 80, panelY + 75, SKYBLUE);
                    DrawText(TextFormat("Shoab's Final Score: %05d", Hero1Score), panelX + 160, panelY + 95, 24, LIME);
                    DrawText(TextFormat("Nayemul's Final Score: %05d", Hero2Score), panelX + 740, panelY + 95, 24, YELLOW);
                    DrawText("[ COMBAT PERFORMANCE BADGES ]", (WindowWidth - MeasureText("[ COMBAT PERFORMANCE BADGES ]", 20)) / 2, panelY + 145, 20, GOLD);

                    int cardW = 340, cardH = 150, cardY = panelY + 185, card1X = panelX + 60;
                    DrawRectangle(card1X, cardY, cardW, cardH, Fade(DARKBLUE, 0.45f));
                    DrawRectangleLines(card1X, cardY, cardW, cardH, SKYBLUE);
                    DrawText("TOP GUN", card1X + 20, cardY + 18, 22, GOLD);
                    DrawText("Most Alien Kills", card1X + 20, cardY + 50, 16, LIGHTGRAY);
                    if (Hero1Kills > Hero2Kills) {
                        DrawText(TextFormat("WINNER: SHOAB (%d)", Hero1Kills), card1X + 20, cardY + 85, 20, LIME);
                        DrawText(TextFormat("Runner Up: Nayemul (%d)", Hero2Kills), card1X + 20, cardY + 115, 14, GRAY);
                    } else if (Hero2Kills > Hero1Kills) {
                        DrawText(TextFormat("WINNER: NAYEMUL (%d)", Hero2Kills), card1X + 20, cardY + 85, 20, YELLOW);
                        DrawText(TextFormat("Runner Up: Shoab (%d)", Hero1Kills), card1X + 20, cardY + 115, 14, GRAY);
                    } else DrawText(TextFormat("TIED: BOTH (%d Kills)", Hero1Kills), card1X + 20, cardY + 95, 20, WHITE);

                    int card2X = panelX + 460;
                    DrawRectangle(card2X, cardY, cardW, cardH, Fade(DARKPURPLE, 0.45f));
                    DrawRectangleLines(card2X, cardY, cardW, cardH, RED);
                    DrawText("DREADNOUGHT BREAKER", card2X + 15, cardY + 18, 20, RED);
                    DrawText("Most Boss Damage Dealt", card2X + 20, cardY + 50, 16, LIGHTGRAY);
                    if (Hero1BossDamage > Hero2BossDamage) {
                        DrawText(TextFormat("WINNER: SHOAB (%d HP)", Hero1BossDamage), card2X + 20, cardY + 85, 20, LIME);
                        DrawText(TextFormat("Runner Up: Nayemul (%d HP)", Hero2BossDamage), card2X + 20, cardY + 115, 14, GRAY);
                    } else if (Hero2BossDamage > Hero1BossDamage) {
                        DrawText(TextFormat("WINNER: NAYEMUL (%d HP)", Hero2BossDamage), card2X + 20, cardY + 85, 20, YELLOW);
                        DrawText(TextFormat("Runner Up: Shoab (%d HP)", Hero1BossDamage), card2X + 20, cardY + 115, 14, GRAY);
                    } else DrawText(TextFormat("TIED: BOTH (%d HP)", Hero1BossDamage), card2X + 20, cardY + 95, 20, WHITE);

                    int card3X = panelX + 860;
                    DrawRectangle(card3X, cardY, cardW, cardH, Fade(DARKGREEN, 0.45f));
                    DrawRectangleLines(card3X, cardY, cardW, cardH, LIME);
                    DrawText("UNTOUCHABLE ACE", card3X + 20, cardY + 18, 20, GREEN);
                    DrawText("Fewest Damage Hits Taken", card3X + 20, cardY + 50, 16, LIGHTGRAY);
                    if (Hero1HitsTaken < Hero2HitsTaken) {
                        DrawText(TextFormat("WINNER: SHOAB (%d Hits)", Hero1HitsTaken), card3X + 20, cardY + 85, 20, LIME);
                        DrawText(TextFormat("Nayemul: %d Hits Taken", Hero2HitsTaken), card3X + 20, cardY + 115, 14, GRAY);
                    } else if (Hero2HitsTaken < Hero1HitsTaken) {
                        DrawText(TextFormat("WINNER: NAYEMUL (%d Hits)", Hero2HitsTaken), card3X + 20, cardY + 85, 20, YELLOW);
                        DrawText(TextFormat("Shoab: %d Hits Taken", Hero1HitsTaken), card3X + 20, cardY + 115, 14, GRAY);
                    } else DrawText(TextFormat("TIED: BOTH (%d Hits)", Hero1HitsTaken), card3X + 20, cardY + 95, 20, WHITE);

                    char restartText[] = "Press [R] to Play Again   |   [ESC] Main Menu";
                    DrawText(restartText, (WindowWidth - MeasureText(restartText, 22)) / 2, panelY + panelH - 45, 22, GOLD);
                }

                if (winDialogueIndex < 2)
                {
                    char promptText[] = "PRESS [ENTER] OR [SPACE] TO ADVANCE BROADCAST";
                    if (((int)(GetTime() * 3)) % 2 == 0)
                        DrawText(promptText, (WindowWidth - MeasureText(promptText, 18)) / 2, panelY + panelH - 45, 18, GREEN);
                    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) { PlaySound(menuSelect); winDialogueIndex++; }
                }

                if (IsKeyPressed(KEY_R))
                {
                    PlaySound(menuSelect); StopMusicStream(bgmWin); winBgmStarted = false; winDialogueIndex = 0;
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
                    for (int m = 0; m < MAX_CLUSTER_MISSILES; m++) clusterMissileActive[m] = false;
                    for (int b = 0; b < MAX_CLUSTER_BLASTS; b++) clusterBlastActive[b] = false;
                    explosionShakeTimer = 0.0f; teamShieldActive = false; teamShieldActiveTimer = 0.0f; teamShieldCooldownTimer = 0.0f; teamShieldCenterX = 0.0f;
                    commandersTriggered = false; commanderIntroActive = false; commanderIntroTimer = 0.0f; gameTimeDilation = 1.0f;
                    jammerActive = false; warpActive = false; jammerHp = 0; warpHp = 0; radarJammedTimer = 0.0f;
                    jammerPowerTimer = 0.0f; warpPowerTimer = 0.0f; bossEnraged = false;
                    for (int b = 0; b < MAX_COMMANDER_BULLETS; b++) { jammerBulletActive[b] = false; warpBulletActive[b] = false; }
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
                if (IsKeyPressed(KEY_ESCAPE))
                {
                    PlaySound(menuSelect); StopMusicStream(bgmWin); winBgmStarted = false; winDialogueIndex = 0;
                    CurrentState = STATE_MENU; PlayMusicStream(bgmMenu);
                }
                EndMode2D(); EndDrawing(); continue;
            }

            // Boss Trigger
            if (AliensKilled == AlienInX * AlienInY && !bossSpawned && !jammerActive && !warpActive && !commanderIntroActive)
            {
                if ((Hero1Lives > 0 || Hero2Lives > 0) && !GameOver && !deathSequenceActive)
                {
                    bossSpawned = true; bossWarningActive = true; bossWarningTimer = 0.0f;
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
                    bossHp = 100; bossLeftPodHp = BossPodMaxHp; bossRightPodHp = BossPodMaxHp;
                    bossPos = (Vector2){ (WindowWidth - BossWidth) / 2.0f, 60.0f };
                    bossLaserTimer = 0.0f; bossOrbTimer = 0.0f; minionDeployTimer = 0.0f;
                }
            }

            float curSpeed1 = HeroSpeedX;
            if (hero1Debuffed) { hero1DebuffTimer -= Time; if (hero1DebuffTimer <= 0.0f) hero1Debuffed = false; curSpeed1 = HeroSpeedX * 0.30f; }
            float curSpeed2 = HeroSpeedX;
            if (hero2Debuffed) { hero2DebuffTimer -= Time; if (hero2DebuffTimer <= 0.0f) hero2Debuffed = false; curSpeed2 = HeroSpeedX * 0.30f; }

            if (!deathSequenceActive && !bossWarpActive && !evacActive)
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

            if (Hero1Lives > 0) Hero1Pos = Vector2Add(Hero1Pos, Vector2Scale(Hero1Speed, Time));
            if (Hero2Lives > 0) Hero2Pos = Vector2Add(Hero2Pos, Vector2Scale(Hero2Speed, Time));

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

            if (empBuffTimer > 0.0f) empBuffTimer -= Time;

            // Hero 1 shooting
            bool canShoot1 = !hero1Debuffed && (Hero1Lives > 0);
            if (empBuffTimer <= 0.0f)
            {
                for (int i = 0; i < 2; i++)
                    if (Hero1BulletActive[i] && Hero1BulletPos[i].y > WindowHeight / 2.0f) { canShoot1 = false; break; }
            }
            if ((IsKeyPressed(KEY_W) || IsKeyPressed(KEY_SPACE)) && canShoot1 && !deathSequenceActive && !bossWarningActive && !bossWarpActive && !evacActive)
            {
                PlaySound(shoot);
                for (int i = 0; i < 2; i++)
                {
                    if (!Hero1BulletActive[i]) { Hero1BulletActive[i] = true; Hero1BulletPos[i] = (Vector2){ Hero1Pos.x, Hero1Pos.y }; break; }
                }
            }

            // Hero 2 shooting
            bool canShoot2 = !hero2Debuffed && (Hero2Lives > 0);
            if (empBuffTimer <= 0.0f)
            {
                for (int i = 0; i < 2; i++)
                    if (Hero2BulletActive[i] && Hero2BulletPos[i].y > WindowHeight / 2.0f) { canShoot2 = false; break; }
            }
            if (IsKeyPressed(KEY_UP) && canShoot2 && !deathSequenceActive && !bossWarningActive && !bossWarpActive && !evacActive)
            {
                PlaySound(shoot);
                for (int i = 0; i < 2; i++)
                {
                    if (!Hero2BulletActive[i]) { Hero2BulletActive[i] = true; Hero2BulletPos[i] = (Vector2){ Hero2Pos.x, Hero2Pos.y }; break; }
                }
            }

            // Shoab special beam
            if (SpecialReady && IsKeyPressed(KEY_Q) && Hero1Lives > 0 && !deathSequenceActive && !bossWarningActive && !bossWarpActive && !evacActive)
            {
                Hero1SpecialBulletPos = (Vector2){ Hero1Pos.x, Hero1Pos.y };
                Hero1SpecialBulletActive = true; ShoabSpecialTime = 0.0f; SpecialReady = false;
                PlaySound(sndSpecialBeam);
            }

            // Nayemul cluster missile launch
            if (NayemulSpecialReady && IsKeyPressed(KEY_RIGHT_SHIFT) && Hero2Lives > 0 && !deathSequenceActive && !bossWarningActive && !bossWarpActive && !evacActive)
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
                PlaySound(sndClusterLaunch);
            }

            // EMP Drop
            if (bossActive && !empDropped && bossLeftPodHp <= 0 && bossRightPodHp <= 0)
            {
                empDropped = true; empActive = true;
                empPos = (Vector2){ (float)GetRandomValue(350, WindowWidth - 350), -40.0f };
            }
            if (empActive)
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

            // Hero regular bullets update
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
                                PlaySound(damage); bActive[i] = false; bossLeftPodHp -= 5; *hScore += 25; *hBossDmg += 5;
                                if (bossLeftPodHp <= 0) { bossLeftPodHp = 0; *hScore += 250; PlaySound(sndPodDestroy); }
                            }
                            else if (bossRightPodHp > 0 && CheckCollisionRecs(bRec, rightPodRec))
                            {
                                PlaySound(damage); bActive[i] = false; bossRightPodHp -= 5; *hScore += 25; *hBossDmg += 5;
                                if (bossRightPodHp <= 0) { bossRightPodHp = 0; *hScore += 250; PlaySound(sndPodDestroy); }
                            }
                            else if (CheckCollisionRecs(bRec, coreRec))
                            {
                                bActive[i] = false;
                                if (bossLeftPodHp > 0 || bossRightPodHp > 0) PlaySound(damage);
                                else
                                {
                                    PlaySound(damage); bossHp -= 3; *hScore += 50; *hBossDmg += 3;
                                    if (bossHp <= 40 && !bossEnraged) { bossEnraged = true; PlaySound(sndBossEnrage); }
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

            // SHOAB SPECIAL BULLET MOVEMENT and COLLISION (MODIFIED EXPANDED HITBOX)
            if (Hero1SpecialBulletActive)
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
                            }
                        }
                    }
                }
                if (jammerActive && CheckCollisionRecs(SpecialBulletRec, (Rectangle){ jammerPos.x, jammerPos.y, COMMANDER_SIZE, COMMANDER_SIZE }))
                {
                    PlaySound(damage); jammerHp -= 2; Hero1Score += 50;
                    if (jammerHp <= 0) { jammerActive = false; Hero1Score += 500; PlaySound(sndJammerDeath); }
                }
                if (warpActive && CheckCollisionRecs(SpecialBulletRec, (Rectangle){ warpPos.x, warpPos.y, COMMANDER_SIZE, COMMANDER_SIZE }))
                {
                    PlaySound(damage); warpHp -= 2; Hero1Score += 50;
                    if (warpHp <= 0) { warpActive = false; Hero1Score += 500; PlaySound(sndWarpDeath); }
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
                        Hero1SpecialBulletActive = false; PlaySound(damage); bossLeftPodHp -= 50; Hero1Score += 250; Hero1BossDamage += 50;
                        if (bossLeftPodHp <= 0)
                        {
                            PlaySound(sndPodDestroy); bossRightPodHp += bossLeftPodHp;
                            if (bossRightPodHp < 0) { bossHp += bossRightPodHp; bossRightPodHp = 0; }
                            bossLeftPodHp = 0; Hero1Score += 250;
                            if (bossHp <= 40 && !bossEnraged) { bossEnraged = true; PlaySound(sndBossEnrage); }
                            if (bossHp <= 0)
                            {
                                bossHp = 0; bossActive = false; evacActive = true; evacTimer = 3.8f;
                                PlaySound(sndEvacBoom);
                            }
                        }
                    }
                    else if (bossRightPodHp > 0 && CheckCollisionRecs(SpecialBulletRec, rightPodRec))
                    {
                        Hero1SpecialBulletActive = false; PlaySound(damage); bossRightPodHp -= 50; Hero1Score += 250; Hero1BossDamage += 50;
                        if (bossRightPodHp <= 0)
                        {
                            PlaySound(sndPodDestroy); bossLeftPodHp += bossRightPodHp;
                            if (bossLeftPodHp < 0) { bossHp += bossLeftPodHp; bossLeftPodHp = 0; }
                            bossRightPodHp = 0; Hero1Score += 250;
                            if (bossHp <= 40 && !bossEnraged) { bossEnraged = true; PlaySound(sndBossEnrage); }
                            if (bossHp <= 0)
                            {
                                bossHp = 0; bossActive = false; evacActive = true; evacTimer = 3.8f;
                                PlaySound(sndEvacBoom);
                            }
                        }
                    }
                    else if (CheckCollisionRecs(SpecialBulletRec, coreRec))
                    {
                        Hero1SpecialBulletActive = false;
                        if (bossLeftPodHp > 0 || bossRightPodHp > 0)
                        {
                            PlaySound(damage); bossLeftPodHp -= 50; Hero1Score += 250; Hero1BossDamage += 50;
                            if (bossLeftPodHp <= 0)
                            {
                                PlaySound(sndPodDestroy); bossRightPodHp += bossLeftPodHp;
                                if (bossRightPodHp < 0) { bossHp += bossRightPodHp; bossRightPodHp = 0; }
                                bossLeftPodHp = 0; Hero1Score += 250;
                                if (bossHp <= 40 && !bossEnraged) { bossEnraged = true; PlaySound(sndBossEnrage); }
                                if (bossHp <= 0)
                                {
                                    bossHp = 0; bossActive = false; evacActive = true; evacTimer = 3.8f;
                                    PlaySound(sndEvacBoom);
                                }
                            }
                        }
                        else
                        {
                            PlaySound(damage); bossHp -= 30; Hero1Score += 500; Hero1BossDamage += 30;
                            if (bossHp <= 40 && !bossEnraged) { bossEnraged = true; PlaySound(sndBossEnrage); }
                            if (bossHp <= 0)
                            {
                                bossHp = 0; bossActive = false; evacActive = true; evacTimer = 3.8f;
                                PlaySound(sndEvacBoom);
                            }
                        }
                    }
                }
                if (Hero1SpecialBulletPos.y < 0) Hero1SpecialBulletActive = false;
            }

            // NAYEMUL CLUSTER MISSILES UPDATE & BALANCED BOSS DAMAGE
            for (int m = 0; m < MAX_CLUSTER_MISSILES; m++)
            {
                if (!clusterMissileActive[m]) continue;
                clusterMissilePos[m] = Vector2Add(clusterMissilePos[m], Vector2Scale(clusterMissileVel[m], Time));

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
                                    clusterBossDamageDealt = true; PlaySound(damage);
                                    if (bossLeftPodHp <= 0) { bossLeftPodHp = 0; PlaySound(sndPodDestroy); }
                                }
                                else if (hitRightPod && !hitLeftPod)
                                {
                                    bossRightPodHp -= 40; Hero2Score += 250; Hero2BossDamage += 40;
                                    clusterBossDamageDealt = true; PlaySound(damage);
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
                                    Hero2Score += 250; Hero2BossDamage += 40; clusterBossDamageDealt = true; PlaySound(damage);
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
                                    PlaySound(damage); bossHp -= 35; Hero2Score += 500; Hero2BossDamage += 35;
                                    clusterBossDamageDealt = true;
                                    if (bossHp <= 40 && !bossEnraged) { bossEnraged = true; PlaySound(sndBossEnrage); }
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

            // Alien shooting
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
                            PlaySound(AlienShoot); AlienBulletActive = true;
                            AlienBulletPos = (Vector2){ AlienPos[randomX][Y].x + AlienSize / 2.0f, AlienPos[randomX][Y].y + AlienSize };
                            break;
                        }
                    }
                }
            }

            if (AlienBulletActive)
            {
                AlienBulletPos.y += AlienBulletSpeedY * Time;
                if (AlienBulletPos.y > WindowHeight) AlienBulletActive = false;
                else if (!deathSequenceActive && !teamShieldActive)
                {
                    Rectangle aBulletRec = { AlienBulletPos.x - AlienBulletWidth / 2.0f, AlienBulletPos.y, AlienBulletWidth, AlienBulletHeight };
                    Rectangle h1Rec = { Hero1Pos.x - HeroWidth / 2.0f, Hero1Pos.y, HeroWidth, HeroHeight };
                    Rectangle h2Rec = { Hero2Pos.x - HeroWidth / 2.0f, Hero2Pos.y, HeroWidth, HeroHeight };

                    if (Hero1Lives > 0 && CheckCollisionRecs(aBulletRec, h1Rec))
                    {
                        AlienBulletActive = false; Hero1Lives--; Hero1HitsTaken++; hero1HitFlashTimer = 0.35f; PlaySound(heroOuch); PlaySound(sndHudGlitch);
                        if (Hero1Lives <= 0) { Hero1CrashPos = Hero1Pos; PlaySound(heroDeath); if (Hero2Lives <= 0) { deathSequenceActive = true; deathDelayTimer = 0.0f; } }
                    }
                    else if (Hero2Lives > 0 && CheckCollisionRecs(aBulletRec, h2Rec))
                    {
                        AlienBulletActive = false; Hero2Lives--; Hero2HitsTaken++; hero2HitFlashTimer = 0.35f; PlaySound(heroOuch); PlaySound(sndHudGlitch);
                        if (Hero2Lives <= 0) { Hero2CrashPos = Hero2Pos; PlaySound(heroDeath); if (Hero1Lives <= 0) { deathSequenceActive = true; deathDelayTimer = 0.0f; } }
                    }
                }
            }

            // Boss Minion Deploy
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

            // Minions movement & shooting
            if (bossActive)
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
                                        PlaySound(AlienShoot); break;
                                    }
                                }
                            }
                        }
                    }
                }
            }

            for (int mb = 0; mb < MAX_MINION_BULLETS; mb++)
            {
                if (minionBulletActive[mb])
                {
                    minionBulletPos[mb].y += AlienBulletSpeedY * Time;
                    if (minionBulletPos[mb].y > WindowHeight) minionBulletActive[mb] = false;
                    else if (!deathSequenceActive && !teamShieldActive)
                    {
                        Rectangle mbRec = { minionBulletPos[mb].x - AlienBulletWidth / 2.0f, minionBulletPos[mb].y, AlienBulletWidth, AlienBulletHeight };
                        Rectangle h1Rec = { Hero1Pos.x - HeroWidth / 2.0f, Hero1Pos.y, HeroWidth, HeroHeight };
                        Rectangle h2Rec = { Hero2Pos.x - HeroWidth / 2.0f, Hero2Pos.y, HeroWidth, HeroHeight };

                        if (Hero1Lives > 0 && CheckCollisionRecs(mbRec, h1Rec))
                        {
                            minionBulletActive[mb] = false; Hero1Lives--; Hero1HitsTaken++; hero1HitFlashTimer = 0.35f; PlaySound(heroOuch); PlaySound(sndHudGlitch);
                            if (Hero1Lives <= 0) { Hero1CrashPos = Hero1Pos; PlaySound(heroDeath); if (Hero2Lives <= 0) { deathSequenceActive = true; deathDelayTimer = 0.0f; } }
                        }
                        else if (Hero2Lives > 0 && CheckCollisionRecs(mbRec, h2Rec))
                        {
                            minionBulletActive[mb] = false; Hero2Lives--; Hero2HitsTaken++; hero2HitFlashTimer = 0.35f; PlaySound(heroOuch); PlaySound(sndHudGlitch);
                            if (Hero2Lives <= 0) { Hero2CrashPos = Hero2Pos; PlaySound(heroDeath); if (Hero1Lives <= 0) { deathSequenceActive = true; deathDelayTimer = 0.0f; } }
                        }
                    }
                }
            }

            float currentBossSpeedX = (bossHp <= 40) ? 250.0f : 160.0f;
            float currentBossSpeedY = (bossHp <= 40) ? 120.0f : 75.0f;

            if (bossActive)
            {
                bossAnimTimer += Time; bossDirChangeTimer += Time;
                if (bossDirChangeTimer > 1.8f)
                {
                    bossDirChangeTimer = 0.0f;
                    if (GetRandomValue(0, 10) > 4) bossSpeed.y = (float)GetRandomValue(-(int)currentBossSpeedY, (int)currentBossSpeedY);
                }

                // Boss maintains continuous flight during alignment, warning pointer, and active laser beam
                bossPos.x += ((bossSpeed.x > 0) ? currentBossSpeedX : -currentBossSpeedX) * Time;
                bossPos.y += bossSpeed.y * Time;

                if (bossPos.x <= 40) { bossPos.x = 40; bossSpeed.x = fabsf(bossSpeed.x); }
                else if (bossPos.x + BossWidth >= WindowWidth - 40) { bossPos.x = WindowWidth - BossWidth - 40; bossSpeed.x = -fabsf(bossSpeed.x); }

                float maxBossY = (WindowHeight * 0.75f) - BossHeight;
                if (bossPos.y <= 30) { bossPos.y = 30; bossSpeed.y = fabsf(bossSpeed.y); }
                else if (bossPos.y >= maxBossY) { bossPos.y = maxBossY; bossSpeed.y = -fabsf(bossSpeed.y); }

                bossShootTimer += Time;
                if (bossShootTimer >= 1.7f && !deathSequenceActive)
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
                    if (bossLaserTimer >= 7.0f) { bossLaserActive = true; bossLaserDuration = 0.0f; bossLaserTimer = 0.0f; PlaySound(bossLaserSound); }
                }
                else
                {
                    bossLaserDuration += Time;
                    if (bossLaserDuration >= 2.0f) { bossLaserActive = false; bossLaserDuration = 0.0f; StopSound(bossLaserSound); }
                }

                bossOrbTimer += Time;
                if (bossOrbTimer >= 4.0f && !deathSequenceActive)
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

            for (int k = 0; k < MAX_BOSS_BULLETS; k++)
            {
                if (bossBulletActive[k])
                {
                    bossBulletPos[k].y += AlienBulletSpeedY * Time;
                    if (bossBulletPos[k].y > WindowHeight) bossBulletActive[k] = false;
                    else if (!deathSequenceActive && !teamShieldActive)
                    {
                        Rectangle bRec = { bossBulletPos[k].x - AlienBulletWidth / 2.0f, bossBulletPos[k].y, AlienBulletWidth, AlienBulletHeight };
                        Rectangle h1Rec = { Hero1Pos.x - HeroWidth / 2.0f, Hero1Pos.y, HeroWidth, HeroHeight };
                        Rectangle h2Rec = { Hero2Pos.x - HeroWidth / 2.0f, Hero2Pos.y, HeroWidth, HeroHeight };

                        if (Hero1Lives > 0 && CheckCollisionRecs(bRec, h1Rec))
                        {
                            bossBulletActive[k] = false; Hero1Lives--; Hero1HitsTaken++; hero1HitFlashTimer = 0.35f; PlaySound(heroOuch); PlaySound(sndHudGlitch);
                            if (Hero1Lives <= 0) { Hero1CrashPos = Hero1Pos; PlaySound(heroDeath); if (Hero2Lives <= 0) { deathSequenceActive = true; deathDelayTimer = 0.0f; } }
                        }
                        else if (Hero2Lives > 0 && CheckCollisionRecs(bRec, h2Rec))
                        {
                            bossBulletActive[k] = false; Hero2Lives--; Hero2HitsTaken++; hero2HitFlashTimer = 0.35f; PlaySound(heroOuch); PlaySound(sndHudGlitch);
                            if (Hero2Lives <= 0) { Hero2CrashPos = Hero2Pos; PlaySound(heroDeath); if (Hero1Lives <= 0) { deathSequenceActive = true; deathDelayTimer = 0.0f; } }
                        }
                    }
                }
            }

            for (int k = 0; k < MAX_BOSS_ORBS; k++)
            {
                if (bossOrbActive[k])
                {
                    bossOrbPos[k] = Vector2Add(bossOrbPos[k], Vector2Scale(bossOrbVel[k], Time));
                    if (bossOrbPos[k].y > WindowHeight || bossOrbPos[k].x < 0 || bossOrbPos[k].x > WindowWidth) bossOrbActive[k] = false;
                    else if (!deathSequenceActive && !teamShieldActive)
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

            if (bossLaserActive && !deathSequenceActive)
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
                    }
                }
            }

            // ALIEN GRID SINGLE BOUNCE CHECK PER FRAME
            if (!bossSpawned)
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
                }

                for (int X = 0; X < AlienInX; X++)
                {
                    for (int Y = 0; Y < AlienInY; Y++)
                    {
                        float speedMod = (AlienSpeed.x > 0) ? (AlienSpeed.x + SpeedBuff[Y]) : (AlienSpeed.x - SpeedBuff[Y]);
                        AlienPos[X][Y].x += speedMod * Time;

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
                    DrawRectangle((int)(Hero1BulletPos[i].x - BulletWidth / 2.0f), (int)Hero1BulletPos[i].y, BulletWidth, BulletHeight, (empBuffTimer > 0.0f) ? LIME : YELLOW);
                if (Hero2BulletActive[i])
                    DrawRectangle((int)(Hero2BulletPos[i].x - BulletWidth / 2.0f), (int)Hero2BulletPos[i].y, BulletWidth, BulletHeight, (empBuffTimer > 0.0f) ? WHITE : SKYBLUE);
            }
            
            // DRAW SHOAB'S SPECIAL PIERCING BEAM (ANIMATED THROUGH 7 FLAME TEXTURES)
            if (Hero1SpecialBulletActive)
            {
                int sFrame = ((int)(GetTime() * 18.0f)) % 7;
                Rectangle Hero1SpecialBulletRec = { Hero1SpecialBulletPos.x - HeroWidth / 2.0f, Hero1SpecialBulletPos.y, HeroWidth, HeroWidth * 3 };
                DrawTexturePro(Hero1SpecialBulletTex[sFrame], (Rectangle){ 0, 0, (float)Hero1SpecialBulletTex[sFrame].width, (float)Hero1SpecialBulletTex[sFrame].height }, Hero1SpecialBulletRec, (Vector2){ 0, 0 }, 0.0f, WHITE);
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
                        if (clusterBlastTex.id > 0)
                            DrawTexturePro(clusterBlastTex, (Rectangle){ 0.0f, 0.0f, (float)clusterBlastTex.width, (float)clusterBlastTex.height },
                                           (Rectangle){ clusterBlastPos[b].x, clusterBlastPos[b].y, diameter, diameter }, (Vector2){ diameter / 2.0f, diameter / 2.0f }, (float)(GetTime() * 180.0f), Fade(WHITE, alpha));
                        else
                        {
                            DrawCircle((int)clusterBlastPos[b].x, (int)clusterBlastPos[b].y, clusterBlastRadius[b], Fade(SKYBLUE, alpha * 0.35f));
                            DrawCircleLines((int)clusterBlastPos[b].x, (int)clusterBlastPos[b].y, clusterBlastRadius[b], Fade(WHITE, alpha * 0.85f));
                        }
                    }
                }
            }

            if (AlienBulletActive) DrawRectangle((int)(AlienBulletPos.x - AlienBulletWidth / 2.0f), (int)AlienBulletPos.y, AlienBulletWidth, AlienBulletHeight, RED);
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

            if (bossActive)
            {
                for (int mb = 0; mb < MAX_MINION_BULLETS; mb++)
                    if (minionBulletActive[mb]) DrawRectangle((int)(minionBulletPos[mb].x - AlienBulletWidth / 2.0f), (int)minionBulletPos[mb].y, AlienBulletWidth, AlienBulletHeight, ORANGE);
            }

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

            // Dreadnought Hyperspace Drop Mechanics
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

            // Dreadnought Evacuation Sequence Visuals
            if (evacActive)
            {
                if (evacTimer <= 3.4f)
                {
                    float fractureTime = 3.4f - evacTimer;
                    float halfW = BossWidth / 2.0f;
                    float texHalfW = (float)BossTexture[0].width / 2.0f;
                    float texH = (float)BossTexture[0].height;

                    // Bilateral Hull Fracture - Left Half
                    Rectangle srcL = { 0, 0, texHalfW, texH };
                    Vector2 posL = { (bossPos.x + halfW / 2.0f) - fractureTime * 65.0f, bossPos.y + BossHeight / 2.0f };
                    float rotL = -fractureTime * 12.0f;
                    DrawTexturePro(BossTexture[0], srcL, (Rectangle){ posL.x, posL.y, halfW, (float)BossHeight }, (Vector2){ halfW / 2.0f, BossHeight / 2.0f }, rotL, WHITE);

                    // Bilateral Hull Fracture - Right Half
                    Rectangle srcR = { texHalfW, 0, texHalfW, texH };
                    Vector2 posR = { (bossPos.x + BossWidth - halfW / 2.0f) + fractureTime * 65.0f, bossPos.y + BossHeight / 2.0f };
                    float rotR = fractureTime * 12.0f;
                    DrawTexturePro(BossTexture[0], srcR, (Rectangle){ posR.x, posR.y, halfW, (float)BossHeight }, (Vector2){ halfW / 2.0f, BossHeight / 2.0f }, rotR, WHITE);

                    // Burning wreckage debris
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

                // Synchronized High-G Foreground Climb
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

                if (!bossLaserActive && bossLaserTimer >= 6.2f && bossLaserTimer < 7.0f)
                {
                    float laserW = 42.0f, guideX = bossPos.x + BossWidth / 2.0f, guideY = bossPos.y + BossHeight - 10.0f;
                    float telegraphAlpha = (float)GetRandomValue(25, 65) / 100.0f;
                    DrawLineEx((Vector2){ guideX, guideY }, (Vector2){ guideX, (float)WindowHeight }, 2.5f, Fade(RED, telegraphAlpha));
                    DrawRectangle((int)(guideX - laserW / 2.0f), (int)guideY, (int)laserW, WindowHeight - (int)guideY, Fade(RED, telegraphAlpha * 0.18f));
                    const char* warnBeam = ">> DANGER: LASER ALIGNING <<";
                    DrawText(warnBeam, (int)(guideX - MeasureText(warnBeam, 16) / 2.0f), (int)(guideY + 30.0f), 16, YELLOW);
                }

                if (bossLaserActive)
                {
                    float laserW = 42.0f + (float)GetRandomValue(-4, 5);
                    float laserX = bossPos.x + (BossWidth - laserW) / 2.0f, laserY = bossPos.y + BossHeight - 10.0f;
                    float laserH = (teamShieldActive) ? ((domeBaseY - domeRadius) - laserY) : (WindowHeight - laserY);
                    if (laserH < 0.0f) laserH = 0.0f;

                    DrawRectangle((int)laserX - 16, (int)laserY, (int)laserW + 32, (int)laserH, Fade(RED, 0.25f));
                    DrawRectangle((int)laserX - 8, (int)laserY, (int)laserW + 16, (int)laserH, Fade(RED, 0.55f));
                    DrawRectangle((int)laserX, (int)laserY, (int)laserW, (int)laserH, Fade(ORANGE, 0.90f));
                    DrawRectangle((int)laserX + 8, (int)laserY, (int)laserW - 16, (int)laserH, WHITE);
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

            // CRASH BEACONS
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

            // HERO 1
            if (Hero1Lives > 0 && !(evacActive && evacTimer <= 2.2f))
            {
                DrawTexturePro(HeroTexture, (Rectangle){ 0, 0, (float)HeroTexture.width, (float)HeroTexture.height },
                               (Rectangle){ Hero1Pos.x - HeroWidth / 2.0f, Hero1Pos.y, HeroWidth, HeroHeight }, (Vector2){ 0, 0 }, 0.0f, hero1Debuffed ? PURPLE : WHITE);
                DrawText("Shoab", (int)Hero1Pos.x - MeasureText("Shoab", 18) / 2, (int)Hero1Pos.y - 24, 18, LIME);
                if (hero1Debuffed)
                {
                    DrawCircleLines((int)Hero1Pos.x, (int)(Hero1Pos.y + HeroHeight / 2.0f), 65.0f, MAGENTA);
                    char dAlert[] = "! WEAPONS JAMMED !";
                    DrawText(dAlert, (int)Hero1Pos.x - MeasureText(dAlert, 14) / 2, (int)Hero1Pos.y - 42, 14, YELLOW);
                }
            }

            // HERO 2
            if (Hero2Lives > 0 && !(evacActive && evacTimer <= 2.2f))
            {
                DrawStealthFlames((Vector2){ Hero2Pos.x, Hero2Pos.y + HeroHeight / 2.0f }, HeroWidth, HeroHeight);
                DrawTexturePro(StealthHeroTexture, (Rectangle){ 0, 0, (float)StealthHeroTexture.width, (float)StealthHeroTexture.height },
                               (Rectangle){ Hero2Pos.x - HeroWidth / 2.0f, Hero2Pos.y, HeroWidth, HeroHeight }, (Vector2){ 0, 0 }, 0.0f, hero2Debuffed ? PURPLE : WHITE);
                DrawText("Nayemul", (int)Hero2Pos.x - MeasureText("Nayemul", 18) / 2, (int)Hero2Pos.y - 24, 18, YELLOW);
                if (hero2Debuffed)
                {
                    DrawCircleLines((int)Hero2Pos.x, (int)(Hero2Pos.y + HeroHeight / 2.0f), 65.0f, MAGENTA);
                    char dAlert[] = "! WEAPONS JAMMED !";
                    DrawText(dAlert, (int)Hero2Pos.x - MeasureText(dAlert, 14) / 2, (int)Hero2Pos.y - 42, 14, YELLOW);
                }
            }

            // TEAM SHIELD
            if (heroesTethered && teamShieldCooldownTimer <= 0.0f && !teamShieldActive)
            {
                DrawLineEx((Vector2){ Hero1Pos.x, Hero1Pos.y + HeroHeight * 0.4f }, (Vector2){ Hero2Pos.x, Hero2Pos.y + HeroHeight * 0.4f },
                           2.5f, Fade(SKYBLUE, 0.40f + 0.30f * sinf((float)GetTime() * 10.0f)));
            }
            if (teamShieldActive)
            {
                if (teamShieldDomeTex.id > 0)
                    DrawTexturePro(teamShieldDomeTex, (Rectangle){ 0, 0, (float)teamShieldDomeTex.width, (float)teamShieldDomeTex.height },
                                   (Rectangle){ domeCenterX, domeBaseY, domeRadius * 2.0f, domeRadius }, (Vector2){ domeRadius, domeRadius }, 0.0f, Fade(WHITE, 0.82f + 0.18f * sinf((float)GetTime() * 14.0f)));
                else
                {
                    DrawCircleSector((Vector2){ domeCenterX, domeBaseY }, domeRadius, 180.0f, 360.0f, 36, Fade(SKYBLUE, 0.35f));
                    DrawCircleLines((int)domeCenterX, (int)domeBaseY, domeRadius, Fade(WHITE, 0.85f));
                }
                if (bossLaserActive)
                {
                    DrawCircle((int)domeCenterX, (int)(domeBaseY - domeRadius), 28.0f, Fade(WHITE, 0.85f));
                    DrawCircle((int)domeCenterX, (int)(domeBaseY - domeRadius), 42.0f, Fade(SKYBLUE, 0.50f));
                }
            }

            // HUD SHOAB
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

                DrawRectangleLinesEx((Rectangle){ b1X, b1Y, (float)bSize, (float)bSize }, shoabCrit ? 2.5f : 2.0f, shoabCrit ? RED : LIME);

                if (Hero1Lives <= 2)
                {
                    DrawLineEx((Vector2){ b1X + 10, b1Y + 4 }, (Vector2){ b1X + 32, b1Y + 26 }, 1.5f, Fade(WHITE, 0.75f));
                    DrawLineEx((Vector2){ b1X + 32, b1Y + 26 }, (Vector2){ b1X + 54, b1Y + 18 }, 1.2f, Fade(WHITE, 0.60f));
                    DrawLineEx((Vector2){ b1X + 32, b1Y + 26 }, (Vector2){ b1X + 24, b1Y + 50 }, 1.5f, Fade(WHITE, 0.70f));
                    DrawLineEx((Vector2){ b1X + 24, b1Y + 50 }, (Vector2){ b1X + 8, b1Y + 58 }, 1.0f, Fade(WHITE, 0.45f));
                    DrawLineEx((Vector2){ b1X + 32, b1Y + 26 }, (Vector2){ b1X + 46, b1Y + 54 }, 1.2f, Fade(WHITE, 0.50f));
                }
            }

            const char* telemShoabStr = "SIGNAL: 99.8% // OK";
            Color telemShoabCol = LIME;
            if (Hero1Lives == 3) { telemShoabStr = "SIGNAL: 81.4% // WARN"; telemShoabCol = YELLOW; }
            else if (Hero1Lives == 2) { telemShoabStr = "SIGNAL: 48.0% // LOSS"; telemShoabCol = ORANGE; }
            else if (Hero1Lives == 1) { telemShoabStr = "SIGNAL: 12.2% // CRIT"; telemShoabCol = (((int)(GetTime() * 6)) % 2 == 0) ? RED : MAROON; }
            else if (Hero1Lives <= 0) { telemShoabStr = "SIGNAL: 00.0% // LOST"; telemShoabCol = DARKGRAY; }
            DrawText(telemShoabStr, b1X, b1Y + bSize + 5, 10, telemShoabCol);

            int text1X = b1X + bSize + 14;
            DrawText("PILOT 1: SHOAB", text1X, 18, 20, LIME);
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

            if (ShoabSpecialTime >= FPS * 15.0f)
            {
                if (!SpecialReady) PlaySound(sndChargeReady);
                DrawText("[Q] SPECIAL READY", 30, 122, 14, LIME); SpecialReady = true;
            }
            else
            {
                DrawText(TextFormat("[Q] SPECIAL READY IN %.1f s", 15.0f - ShoabSpecialTime / FPS), 30, 122, 14, RED);
                SpecialReady = false;
            }

            // HUD NAYEMUL
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

                DrawRectangleLinesEx((Rectangle){ b2X, b2Y, (float)bSize, (float)bSize }, nayemulCrit ? 2.5f : 2.0f, nayemulCrit ? RED : YELLOW);

                if (Hero2Lives <= 2)
                {
                    DrawLineEx((Vector2){ b2X + 14, b2Y + 6 }, (Vector2){ b2X + 36, b2Y + 28 }, 1.5f, Fade(WHITE, 0.75f));
                    DrawLineEx((Vector2){ b2X + 36, b2Y + 28 }, (Vector2){ b2X + 56, b2Y + 22 }, 1.2f, Fade(WHITE, 0.60f));
                    DrawLineEx((Vector2){ b2X + 36, b2Y + 28 }, (Vector2){ b2X + 22, b2Y + 52 }, 1.5f, Fade(WHITE, 0.70f));
                    DrawLineEx((Vector2){ b2X + 22, b2Y + 52 }, (Vector2){ b2X + 6, b2Y + 60 }, 1.0f, Fade(WHITE, 0.45f));
                    DrawLineEx((Vector2){ b2X + 36, b2Y + 28 }, (Vector2){ b2X + 50, b2Y + 54 }, 1.2f, Fade(WHITE, 0.50f));
                }
            }

            const char* telemNayemulStr = "SIGNAL: 99.8% // OK";
            Color telemNayemulCol = LIME;
            if (Hero2Lives == 3) { telemNayemulStr = "SIGNAL: 81.4% // WARN"; telemNayemulCol = YELLOW; }
            else if (Hero2Lives == 2) { telemNayemulStr = "SIGNAL: 48.0% // LOSS"; telemNayemulCol = ORANGE; }
            else if (Hero2Lives == 1) { telemNayemulStr = "SIGNAL: 12.2% // CRIT"; telemNayemulCol = (((int)(GetTime() * 6)) % 2 == 0) ? RED : MAROON; }
            else if (Hero2Lives <= 0) { telemNayemulStr = "SIGNAL: 00.0% // LOST"; telemNayemulCol = DARKGRAY; }
            DrawText(telemNayemulStr, b2X + bSize - MeasureText(telemNayemulStr, 10), b2Y + bSize + 5, 10, telemNayemulCol);

            const char* pilot2Title = "PILOT 2: NAYEMUL";
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
                const char* p2LivesText = TextFormat("LIVES: %d / 4", Hero2Lives);
                DrawText(p2LivesText, b2X - 14 - MeasureText(p2LivesText, 20), 42, 20, (Hero2Lives <= 1) ? RED : YELLOW);
                const char* p2ScoreText = TextFormat("SCORE: %05d", Hero2Score);
                DrawText(p2ScoreText, b2X - 14 - MeasureText(p2ScoreText, 20), 66, 20, (Color){ 255, 245, 160, 255 });
                const char* p2Controls = "[ARROWS] Move  |  [UP] Shoot";
                DrawText(p2Controls, b2X - 14 - MeasureText(p2Controls, 14), 90, 14, LIGHTGRAY);
            }

            if (NayemulSpecialTime >= FPS * 15.0f)
            {
                if (!NayemulSpecialReady) PlaySound(sndChargeReady);
                const char* rReadyText = "[RSHIFT] CLUSTER READY";
                DrawText(rReadyText, WindowWidth - 30 - MeasureText(rReadyText, 14), 122, 14, YELLOW);
                NayemulSpecialReady = true;
            }
            else
            {
                const char* rChargeText = TextFormat("[RSHIFT] READY IN %.1f s", 15.0f - NayemulSpecialTime / FPS);
                DrawText(rChargeText, WindowWidth - 30 - MeasureText(rChargeText, 14), 122, 14, RED);
                NayemulSpecialReady = false;
            }

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

            const char* pauseNotice = "[P] PAUSE  |  [F11] FULLSCREEN  |  [ESC] MENU";
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
        }
        // STATE: VIDEO PLAYBACK (CUSTOM VIDEO ENGINE)
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
                if (!ffmpegInstalled)
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

            // Bold "Replay" in Red at top-left corner for Game Over (1) and Game Win (2)
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

        // Full-Screen White-Out Blind for Evacuation Sequence
        if (evacActive && evacTimer > 3.4f)
        {
            float blindAlpha = (evacTimer - 3.4f) / 0.4f;
            if (blindAlpha > 1.0f) blindAlpha = 1.0f;
            if (blindAlpha < 0.0f) blindAlpha = 0.0f;
            DrawRectangle(0, 0, WindowWidth, WindowHeight, Fade(WHITE, blindAlpha));
        }

        EndMode2D();
        if (ScanlinesOn)
        {
            for (int y = 0; y < WindowHeight; y += 4) DrawRectangle(0, y, WindowWidth, 1, Fade(BLACK, 0.22f));
        }

        if (Brightness < 1.0f) DrawRectangle(0, 0, WindowWidth, WindowHeight, Fade(BLACK, 1.0f - Brightness));
        else if (Brightness > 1.0f) DrawRectangle(0, 0, WindowWidth, WindowHeight, Fade(WHITE, (Brightness - 1.0f) * 0.25f));

        EndDrawing();
    }

    // Stop and cleanup active video playback
    StopVideo();
    if (videoFrameTex.id > 0) UnloadTexture(videoFrameTex);
    if (videoFramePixels != NULL) free(videoFramePixels);

    // Cleanup
    UnloadTexture(HeroTexture); UnloadTexture(StealthHeroTexture);
    if (loadingBg.id > 0) UnloadTexture(loadingBg);
    if (portraitShoab.id > 0) UnloadTexture(portraitShoab);
    if (portraitNayemul.id > 0) UnloadTexture(portraitNayemul);
    if (portraitShoabCrit.id > 0 && portraitShoabCrit.id != portraitShoab.id) UnloadTexture(portraitShoabCrit);
    if (portraitNayemulCrit.id > 0 && portraitNayemulCrit.id != portraitNayemul.id) UnloadTexture(portraitNayemulCrit);
    if (clusterBombTex.id > 0) UnloadTexture(clusterBombTex);
    if (clusterBlastTex.id > 0) UnloadTexture(clusterBlastTex);
    if (teamShieldDomeTex.id > 0) UnloadTexture(teamShieldDomeTex);

    // Unload Shoab's 7 Special Bullet flame textures
    for (int s = 0; s < 7; s++)
    {
        if (Hero1SpecialBulletTex[s].id > 0) UnloadTexture(Hero1SpecialBulletTex[s]);
    }

    for (int c = 0; c < 4; c++) { UnloadTexture(jammerTex[c]); UnloadTexture(warpTex[c]); }
    for (int b = 0; b < 5; b++) UnloadTexture(BossTexture[b]);
    for (int Asprite = 0; Asprite < AlienSprite; Asprite++) UnloadTexture(AlienTexture[Asprite]);

    UnloadSound(shoot); UnloadSound(AlienShoot); UnloadSound(menuMove); UnloadSound(menuSelect);
    UnloadSound(heroDeath); UnloadSound(pauseIn); UnloadSound(pauseOut); UnloadSound(damage);
    
    UnloadSound(heroOuch); UnloadSound(bossLaserSound); UnloadSound(sndJammerHum); UnloadSound(sndJammerShot);
    UnloadSound(sndEmpBlast); UnloadSound(sndJammerDeath); UnloadSound(sndWarpGlide); UnloadSound(sndWarpShot);
    UnloadSound(sndTeleport); UnloadSound(sndWarpDeath); UnloadSound(sndSpecialBeam); UnloadSound(sndChargeReady);
    UnloadSound(sndPodDestroy); UnloadSound(sndOrbLaunch); UnloadSound(sndWarningSiren); UnloadSound(sndDefibHum);
    UnloadSound(sndBossEnrage); UnloadSound(sndRocketBoost); UnloadSound(sndClusterLaunch); UnloadSound(sndClusterExplode);
    UnloadSound(sndShieldActivate); UnloadSound(sndShieldDeflect);
    UnloadSound(sndHudGlitch); UnloadSound(sndEvacBoom);

    UnloadMusicStream(bgmStory); UnloadMusicStream(bgmMenu); UnloadMusicStream(bgmBoss);
    UnloadMusicStream(bgmWin); UnloadMusicStream(bgmLost);

    CloseAudioDevice();
    CloseWindow();
    return 0;
}
