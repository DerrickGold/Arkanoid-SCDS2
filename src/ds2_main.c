//ds2_main.c
#include <libBAG.h>
#include "animations.h"
#include "quick2dEngine.h"
#include "filesys.h"


//important file paths
const char RootDir[] = "/arkanoid/";
const char SkinDir[] = "skins/";
const char LevelDir[] = "levels/";

/*//===============================================
Template object
//==============================================
typedef struct Object_t{
    GFXObj_t *gfx;
    Point_t Pos;

    void (*reset)(struct Bullet_t *);
    void (*update)(struct Bullet_t *, int);
    void (*setXY)(struct Bullet_t *, int, int);
    void (*draw)(unsigned short *, struct Bullet_t *);
}Object_t;


static void objectSetXY(Bullet_t *obj, int x, int y){
    Point_t *pos = &obj->Pos;
    (*pos->getX(pos)) = norm_fix(x);
    (*pos->getY(pos)) = norm_fix(y);
}

static void objectReset(Object_t *obj){
    //set objects initial valus
    Point_t *pos = &obj->Pos;
    pos->speed = 0;//no moving
    pos->x = -256;//move offscreen
    pos->y = -256;
}

static void objectUpdate(Object_t *obj, int update){
    //update object here (controls or collisions)
    obj->Pos.update(&obj->Pos);
}

static void objectDraw(unsigned short *dest, Object_t *obj){
    GFXObj_t *gfx = bullet->gfx;
    Point_t *pos = &bullet->Pos;
    BAG_Display_SetGfxBlitXY(gfx, fix_norm(*pos->getX(pos)), fix_norm(*pos->getY(pos)));
    BAG_Display_DrawObjSlowEx(gfx, dest, GAME_WIDTH, GAME_HEIGHT);
}

static void objectInit(Object_t *obj, GFXObj_t *gfx){
    //initialize object
    memset(obj, 0, sizeof(Object_t));
    obj->gfx = gfx;
    initPoint(&obj->Pos);
    //assign functions
    obj->setXY = (void*)&objectSetXY;
    obj->reset = (void*)&objectReset;
    obj->update = (void*)&objectUpdate;
    obj->draw = (void*)&objectDraw;
    objectReset(obj);
}
*/



/*==========================================================================
Game stuff
==========================================================================*/

//#define BULLET_SPEED 1024
#define PLAYER_SPEED 1024
#define BALL_BASE_SPEED 1024


typedef struct Ball_t{
    GFXObj_t *gfx;
    Point_t Pos;

    char died;
    char (*collisionObj)(struct Ball_t *, GFXObj_t *);
    int (*collisionBG)(struct Ball_t *, Point_t *, TiledBG_t *, unsigned int ***);
    void (*launch)(struct Ball_t *, int, int);
    void (*reset)(struct Ball_t *);
    void (*update)(struct Ball_t *, int);
    void (*setXY)(struct Ball_t *, int, int);
    void (*draw)(unsigned short *, struct Ball_t *);
}Ball_t;



static char ballCollisionObj(Ball_t * ball, GFXObj_t *target){
    return obj_collision_PtObj(&ball->Pos, ball->gfx, target);
}

static int ballCollisionBG(Ball_t *ball, Point_t *bgPos, TiledBG_t *bg, unsigned int *matrix[4][3]){
    return obj_collisionTile_Pt(&ball->Pos, ball->gfx, bgPos, bg, matrix);
}

static void ballSetXY(Ball_t *ball, int x, int y){
    Point_t *pos = &ball->Pos;
    (*pos->getX(pos)) = norm_fix(x);
    (*pos->getY(pos)) = norm_fix(y);
}

static void ballLaunch(Ball_t *ball, int speed, int angle){
    ball->died = 0;

    Point_t *pos = &ball->Pos;
    pos->speed = speed;
    pos->angle = angle;
}

static void ballReset(Ball_t *ball){
    Point_t *pos = &ball->Pos;
    pos->speed = 0;//no moving
    ballSetXY(ball, 128-4, 359);
}

static void ballUpdate(Ball_t *ball, int update){
    Point_t *pos = &ball->Pos;
    //reset ball if it goes off bottom
    if(fix_norm(*pos->getY(pos)) >= GAME_HEIGHT)
        ball->died = 1;

    if(ball->died)
        return;

    //top wall collision
    if(fix_norm(*pos->getY(pos)) < 0)
        *pos->getAngle(pos) = angle_vertFlip(*pos->getAngle(pos));

    //left and right wall collisions
    if(fix_norm(*pos->getX(pos)) < 0 || fix_norm(*pos->getX(pos)) > GAME_WIDTH - (*BAG_Display_GetGfxFrameWd(ball->gfx))){
        *pos->getAngle(pos) = angle_horFlip(*pos->getAngle(pos));
        ball->Pos.update(&ball->Pos);
    }

    ball->Pos.update(&ball->Pos);
}

static void ballDraw(unsigned short *dest, Ball_t *ball){
    GFXObj_t *gfx = ball->gfx;
    Point_t *pos = &ball->Pos;
    BAG_Display_SetGfxBlitXY(gfx, fix_norm(*pos->getX(pos)), fix_norm(*pos->getY(pos)));
    BAG_Display_DrawObjSlowEx(gfx, dest, GAME_WIDTH, GAME_HEIGHT);
}

static void ballInit(Ball_t *ball, GFXObj_t *gfx){
    memset(ball, 0, sizeof(Ball_t));

    ball->gfx = gfx;
    initPoint(&ball->Pos);

    ball->collisionObj = (void*)&ballCollisionObj;
    ball->collisionBG = (void*)&ballCollisionBG;
    ball->launch = (void*)&ballLaunch;
    ball->reset = (void*)&ballReset;
    ball->update = (void*)&ballUpdate;
    ball->draw = (void*)&ballDraw;
    ball->setXY = (void*)&ballSetXY;
    ballReset(ball);
}
/*==========================================================================
Ship Info
==========================================================================*/
typedef enum{
    SMALL_IDLE_ANIM,
    BIG_IDLE_ANIM,
    SPAWN_ANIM,
    DEATH_ANIM,
    TOTAL_ANIM,
}PADDLE_ANIMS;

typedef struct Player_t{
    GFXObj_t *gfx, *ball_gfx;
    AnimData Animations[TOTAL_ANIM];

    Point_t Pos;
    Ball_t Ball;

    unsigned int score;
    char lives, hit, isBig;

    void (*update)(struct Player_t *, void (*extra)(void));
    void (*draw) (unsigned short *, struct Player_t *);
    void (*reset)(struct Player_t *);
    void (*resetPos)(struct Player_t *);
}Player_t;

static void playerResetAnim(Player_t *p){
    for(int i = 0; i < TOTAL_ANIM; i++)
        Animation_ResetProfile(&p->Animations[i]);
}

static void playerResetPos(Player_t *p){
    Point_t *pos = &p->Pos;
    (*pos->getX(pos)) = norm_fix( (SCREEN_WIDTH - (*BAG_Display_GetGfxFrameWd(p->gfx)))>>1 );
    (*pos->getY(pos)) = norm_fix(GAME_HEIGHT - 12);
    (*pos->getSpeed(pos)) = 0;
    p->Ball.reset(&p->Ball);

    //set ball position
    int tempX = fix_norm(*pos->getX(pos)) + (((*BAG_Display_GetGfxFrameWd(p->gfx)) + (*BAG_Display_GetGfxFrameWd(p->Ball.gfx)))>>1);
    int tempY = fix_norm(*pos->getY(pos)) - (*BAG_Display_GetGfxFrameHt(p->Ball.gfx));
    p->Ball.setXY(&p->Ball, tempX, tempY);
}

static void playerReset(Player_t *p){
    p->lives = 3;
    p->score = 0;
    playerResetPos(p);
    playerResetAnim(p);
}

static void playerUpdate(Player_t *p, void (*extra)(void)){

    if(*p->Ball.Pos.getSpeed(&p->Ball.Pos) == 0 || p->Ball.died /*&& p->Animations[SPAWN_ANIM].done)*/){
        if(Pad.Newpress.A){
            p->Ball.launch(&p->Ball, BALL_BASE_SPEED, ANGLE_UP_RIGHT);
            p->Ball.died = 0;
            p->lives--;
            playerResetAnim(p);
        }
    }

    if(p->Ball.died){//handle death animations
        if(!p->Animations[DEATH_ANIM].done)
            Animation_RunProfile(&p->Animations[DEATH_ANIM]);
        else if(!p->Animations[SPAWN_ANIM].done)
            Animation_RunProfile(&p->Animations[SPAWN_ANIM]);
        else{
            playerResetPos(p);
            Animation_RunProfile(&p->Animations[SMALL_IDLE_ANIM]);
        }
        return;
    }

    Point_t *pos = &p->Pos;
    (*pos->getSpeed(pos)) = 0;//no movement when no pads are held
    if(Pad.Held.Left){
        (*pos->getSpeed(pos)) = PLAYER_SPEED;
        (*pos->getAngle(pos)) = ANGLE_LEFT;
    }
    else if(Pad.Held.Right){
        (*pos->getSpeed(pos)) = PLAYER_SPEED;
        (*pos->getAngle(pos)) = ANGLE_RIGHT;
    }

    //ball and paddle collision
    if(p->Ball.collisionObj(&p->Ball, p->gfx))
        *p->Ball.Pos.getAngle(&p->Ball.Pos) = angle_vertFlip(*p->Ball.Pos.getAngle(&p->Ball.Pos));

    //update bullet if it is moving
    p->Ball.update(&p->Ball, 0);

    if(extra != NULL)
        extra();
    //update player movement
    pos->update(pos);
}

static void playerDraw(unsigned short *dest, Player_t *p){
    //draw player sprite
    GFXObj_t *gfx = p->gfx;
    Point_t *pos = &p->Pos;

    //paddle animations
    if(!p->Ball.died){
        if(p->isBig)
            Animation_RunProfile(&p->Animations[BIG_IDLE_ANIM]);
        else
            Animation_RunProfile(&p->Animations[SMALL_IDLE_ANIM]);
    }

    BAG_Display_SetGfxBlitXY(gfx, fix_norm(*pos->getX(pos)), fix_norm(*pos->getY(pos)));
    BAG_Display_DrawObjSlowEx(gfx, dest, GAME_WIDTH, GAME_HEIGHT);

    //draw ball
    p->Ball.draw(dest, &p->Ball);
}


void Player_Init(Player_t *p, GFXObj_t *sprite_gfx, GFXObj_t *ball_gfx){
    memset(p, 0, sizeof(Player_t));

    p->gfx = sprite_gfx;
    p->ball_gfx = ball_gfx;

    initPoint(&p->Pos);
    ballInit(&p->Ball, ball_gfx);

    p->update = (void*)&playerUpdate;
    p->draw = (void*)&playerDraw;
    p->reset = (void*)playerReset;
    p->resetPos = (void*)playerResetPos;


    //small idle anim
    {
        AnimData tempAnim = {
            sprite_gfx,/*gfx*/ 0,/*first frame*/ 3,/*last frame*/ 0,/*idle frame*/ 64,/*speed*/
            256,/*frames*/ -1,/*loop*/ 0,/*done*/ 0,/*vertical offset*/ 0,/*timer*/ 0,/*loopTimes*/
            0,/*loop Increment*/
            36,/*frame wd*/ 12,/*frame height*/
        };
        memcpy(&p->Animations[SMALL_IDLE_ANIM], &tempAnim, sizeof(AnimData));
    }
    //big idle anim
    {
        AnimData tempAnim = {
            sprite_gfx,/*gfx*/ 0,/*first frame*/ 3,/*last frame*/ 0,/*idle frame*/ 64,/*speed*/
            256,/*frames*/ -1,/*loop*/ 0,/*done*/ 43,/*vertical offset*/ 0,/*timer*/ 0,/*loopTimes*/
            0,/*loop Increment*/
            52,/*frame wd*/ 12,/*frame height*/
        };
        memcpy(&p->Animations[BIG_IDLE_ANIM], &tempAnim, sizeof(AnimData));
    }
    //respawn anim
    {
        AnimData tempAnim = {
            sprite_gfx,/*gfx*/ 0,/*first frame*/ 3,/*last frame*/ 0,/*idle frame*/ 128,/*speed*/
            256,/*frames*/ 1,/*loop*/ 0,/*done*/ 12,/*vertical offset*/ 0,/*timer*/ 0,/*loopTimes*/
            0,/*loop Increment*/
            32,/*frame wd*/ 15,/*frame height*/
        };
        memcpy(&p->Animations[SPAWN_ANIM], &tempAnim, sizeof(AnimData));
    }
    //death anim
    {
        AnimData tempAnim = {
            sprite_gfx,/*gfx*/ 0,/*first frame*/ 3,/*last frame*/ 0,/*idle frame*/ 128,/*speed*/
            256,/*frames*/ 1,/*loop*/ 0,/*done*/ 28,/*vertical offset*/ 0,/*timer*/ 0,/*loopTimes*/
            0,/*loop Increment*/
            36,/*frame wd*/ 16,/*frame height*/
        };
        memcpy(&p->Animations[DEATH_ANIM], &tempAnim, sizeof(AnimData));
    }
    Animation_RunProfile(&p->Animations[SMALL_IDLE_ANIM]);
    playerReset(p);
}


/*==========================================================================
Invaders
==========================================================================*/
typedef struct Level_t{
    TiledBG_t *gfx;
    Point_t Pos;
    void (*draw) (unsigned short *, struct Level_t *);
}Level_t;


static void levelDraw(unsigned short *dest, Level_t *a){
    Point_t *pos = &a->Pos;
    BAG_TileBG_DrawBGEx(dest, a->gfx, fix_norm(*pos->getX(pos)), fix_norm(*pos->getY(pos)), GAME_WIDTH, GAME_HEIGHT);
}

void Level_Init(Level_t *a, TiledBG_t *bg){
    memset(a, 0, sizeof(Level_t));
    a->gfx = bg;
    initPoint(&a->Pos);

    a->draw = (void*)&levelDraw;
}

/*==========================================================================
Logic
==========================================================================*/
TiledBG_t level_tiles = {0};//aliens tiled background

static GFXObj_t Canvas,//main buffer to blit to
                Paddle,
                Ball,
                Background,
                PowerUps;


static Player_t Player = {0};
static Level_t Level = {0};


/*
Bullet collision for the players shot
-need to add bunker collisions
*/
static char processTile(unsigned int *tile){
    if(!tile || *tile == 0)//tile is dead
        return 0;

    //gold tiles take 4 hits to break and silver tiles 2
    if(*tile == 14 || *tile == 13 || *tile == 12 || *tile == 10){
        (*tile)--;
        return 1;
    }
    *tile = 0;
    return 1;
}


static void BallBrickCollision(void){
    //get alien information
    Point_t *aPos = &Level.Pos;
    TiledBG_t *aGfx = Level.gfx;

    unsigned int *tiles[4][3];
    int flags = obj_collisionTile_Pt(&Player.Ball.Pos, Player.ball_gfx, aPos, aGfx, tiles);
    for(int i = 0; i < 3; i++){
        if(GET_FLAG(flags, COLLISION_UP)){
            if(processTile(tiles[0][i])){
                *Player.Ball.Pos.getAngle(&Player.Ball.Pos) = angle_vertFlip(*Player.Ball.Pos.getAngle(&Player.Ball.Pos));
                break;
            }
        }
        else if(GET_FLAG(flags, COLLISION_DOWN)){
            if(processTile(tiles[1][i])){
                *Player.Ball.Pos.getAngle(&Player.Ball.Pos) = angle_vertFlip(*Player.Ball.Pos.getAngle(&Player.Ball.Pos));
                break;
            }
        }

        if(GET_FLAG(flags, COLLISION_LEFT)){
            if(processTile(tiles[2][i])){
                *Player.Ball.Pos.getAngle(&Player.Ball.Pos) = angle_horFlip(*Player.Ball.Pos.getAngle(&Player.Ball.Pos));
                break;
            }
        }
        else if(GET_FLAG(flags, COLLISION_RIGHT)){
            if(processTile(tiles[3][i])){
                *Player.Ball.Pos.getAngle(&Player.Ball.Pos) = angle_horFlip(*Player.Ball.Pos.getAngle(&Player.Ball.Pos));
                break;
            }
        }
    }

}



//check if level complete
static char levelCompleted(TiledBG_t *bg){
    //scan through ever tile to see if they have been destroyed
    for(int x = 0; x < bg->width; x++){
        for(int y = 0; y < bg->height; y++){
            if(BAG_TileBG_GetTile(bg, x, y) > 0)
                return 0;
        }
    }
    return 1;
}




void loadLevel(const char *curSkin, const char *level){
    char path[MAX_PATH<<1];
    memset(&path, 0, sizeof(path));

    sprintf(path, "%s%s%s", RootDir, LevelDir, level);
    path[MAX_PATH] = '\0';

    sprintf(&path[MAX_PATH+1], "%s%s%s/brickTiles", RootDir, SkinDir, curSkin);
    if(!BAG_TileBG_LoadBG(&path[MAX_PATH+1], path, &level_tiles))
        printf("error loading level\n");
    BAG_TileBG_SetProperties(&level_tiles, GAME_HEIGHT >> level_tiles.divY, GAME_WIDTH >> level_tiles.divX, 0, 0);
    //Invaders.forceMode = 1;
}

void LoadGraphics(const char *curSkin){
    char path[MAX_PATH];
    memset(path, 0, sizeof(path));

    sprintf(path, "%s%s%s/paddle", RootDir, SkinDir, curSkin);
    if(BAG_Display_LoadObjExt(path, &Paddle) != NO_ERR)
        printf("error loading paddle\n");
    //BAG_Display_SetGfxFrameDim(&Paddle, 36, 12);


    sprintf(path, "%s%s%s/ball", RootDir, SkinDir, curSkin);
    if(BAG_Display_LoadObjExt(path, &Ball) != NO_ERR)
        printf("error loading ball\n");
    //BAG_Display_SetGfxFrameDim(&Bullets, 4, 7);

    sprintf(path, "%s%s%s/background", RootDir, SkinDir, curSkin);
    if(BAG_Display_LoadObjExt(path, &Background) != NO_ERR)
        printf("error loading background\n");
}




void Flip_Screen(GFXObj_t *screen){
    BAG_Display_SetGfxFrameDim(screen, SCREEN_WIDTH, SCREEN_HEIGHT);

    //set top screen
    BAG_Display_SetObjFrame(screen, FRAME_VERT, 0);
    BAG_Display_DrawObjFast(screen, up_screen_addr, 0, 0);

    //set top screen
    BAG_Display_SetObjFrame(screen, FRAME_VERT, 1);
    BAG_Display_DrawObjFast(screen, down_screen_addr, 0, 0);

    BAG_Display_SetGfxFrameDim(screen, GAME_WIDTH, GAME_HEIGHT);
    //flip screens
    ds2_flipScreen(DUAL_SCREEN, 1);
}


void DrawScreen(GFXObj_t *screen){
    unsigned short *Screen_Buffer = BAG_Display_GetGfxBuf(screen);
    if(Screen_Buffer == NULL){
        printf("screen buffer error!\n");
        while(1);
    }
    BAG_Display_DrawObjFastEx(&Background, Screen_Buffer, GAME_WIDTH, GAME_HEIGHT);


    Level.draw(Screen_Buffer, &Level);
    Player.draw(Screen_Buffer, &Player);
    Flip_Screen(&Canvas);
}




/*
Back to normal programming stuff
*/
void update(void){
    Player.update(&Player, &BallBrickCollision);
}



void ds2_main(void){
    if(!BAG_Init(1))
        ds2_plug_exit();

    BAG_Core_SetFPS(120);
    ds2_setCPUclocklevel(13);
    //play area
    BAG_Display_CreateObj(&Canvas, 16, GAME_WIDTH, GAME_HEIGHT, GAME_WIDTH, GAME_HEIGHT);
    printf("play screen created!\n");
    //load graphics
    loadLevel("default", "level_1.tbag");
    LoadGraphics("default");
    printf("graphics loaded\n");

    //initiate player
    Player_Init(&Player, &Paddle, &Ball);
    printf("player initiated\n");

    //initiate aliens
    Level_Init(&Level, &level_tiles);
    printf("bricks initiated\n");
    DrawScreen(&Canvas);

    while(1){
        update();
        DrawScreen(&Canvas);
        if(Pad.Newpress.L)
            BAG_Display_ScrnCap(DUAL_SCREEN, RootDir);

        BAG_Update();
    }
}


