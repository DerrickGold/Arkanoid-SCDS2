#ifndef _QUICK2D_H_
#define _QUICK2D_H_

#include <libBAG.h>

#ifdef __cplusplus
extern "C" {
#endif

#define GAME_WIDTH SCREEN_WIDTH
#define GAME_HEIGHT (SCREEN_HEIGHT<<1)

#define fix_norm(x) ((x)>>8)
#define norm_fix(x) ((x)<<8)
#define FLIP_ANGLE(x) (((x) + 256)&511)
//left to right flip
#define FLIP_LTR(x) (((x) + 128)&511)
//right to left
#define FLIP_RTL(x) (((x) - 128)&511)

typedef enum{
    ANGLE_UP = 384,
    ANGLE_UP_RIGHT = 448,
    ANGLE_UP_LEFT = 320,
    ANGLE_DOWN = 128,
    ANGLE_DOWN_RIGHT = 64,
    ANGLE_DOWN_LEFT = 192,
    ANGLE_RIGHT = 0,
    ANGLE_LEFT = 256,
}ANGLES;

extern short angle_horFlip(short angle);
extern short angle_vertFlip(short angle);

typedef struct Point_t{
    int x, y, speed, angle;

    int *(*getX)(struct Point_t *);
    int *(*getY)(struct Point_t *);
    int *(*getSpeed)(struct Point_t *);
    int *(*getAngle)(struct Point_t *);
    char *(*onScreen)(struct Point_t *);
    void (*update)(struct Point_t *);
}Point_t;


extern void initPoint(Point_t *pt);

typedef enum{
    COLLISION_UP = (1<<1),
    COLLISION_DOWN = (1<<2),
    COLLISION_LEFT = (1<<3),
    COLLISION_RIGHT = (1<<4),
}COLLISION_DIRECTIONS;

extern int obj_collision(GFXObj_t *spr1, GFXObj_t *spr2);
extern int obj_collision_PtObj(Point_t *p1, GFXObj_t *spr1, GFXObj_t *spr2);
extern int obj_collision_PtPt(Point_t *p1, GFXObj_t *spr1, Point_t *p2, GFXObj_t *spr2);
extern int obj_collisionArea(GFXObj_t *gfx, int x1, int y1, int x2, int y2);
extern int obj_collisionArea_Pt(Point_t *pt, GFXObj_t *gfx, int x1, int y1, int x2, int y2);
extern int obj_collisionTile(GFXObj_t *gfx, Point_t *bgPos, TiledBG_t *bg, unsigned int *matrix[4][3]);
extern int obj_collisionTile_Pt(Point_t *pPos, GFXObj_t *gfx, Point_t *bgPos, TiledBG_t *bg, unsigned int *matrix[4][3]);



#ifdef __cplusplus
}
#endif


#endif
