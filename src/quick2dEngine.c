#include "quick2dEngine.h"

//flip horizontal component from left to right or right to left
short angle_horFlip(short angle){
    if(angle == ANGLE_LEFT)
        return ANGLE_RIGHT;
    else if(angle == ANGLE_RIGHT)
        return ANGLE_LEFT;
    else if(angle == ANGLE_UP_RIGHT)
        return ANGLE_UP_LEFT;
    else if(angle == ANGLE_UP_LEFT)
        return ANGLE_UP_RIGHT;
    else if(angle == ANGLE_DOWN_RIGHT)
        return ANGLE_DOWN_LEFT;
    else if(angle == ANGLE_DOWN_LEFT)
        return ANGLE_DOWN_RIGHT;
    return angle;
}

short angle_vertFlip(short angle){
    if(angle == ANGLE_UP)
        return ANGLE_DOWN;
    else if(angle == ANGLE_UP_LEFT)
        return ANGLE_DOWN_LEFT;
    else if(angle == ANGLE_DOWN_LEFT)
        return ANGLE_UP_LEFT;
    else if(angle == ANGLE_UP_RIGHT)
        return ANGLE_DOWN_RIGHT;
    else if(angle == ANGLE_DOWN_RIGHT)
        return ANGLE_UP_RIGHT;
    else if(angle == ANGLE_DOWN)
        return ANGLE_UP;

    return angle;
}

/*
Positioning stuff and movement
*/
static int *getPointX(Point_t *pt){
    return &pt->x;
}

static int *getPointY(Point_t *pt){
    return &pt->y;
}

static int *getPointSpeed(Point_t *pt){
    return &pt->speed;
}

static int *getPointAngle(Point_t *pt){
    return &pt->angle;
}

static void updatePoint(Point_t *pt){
    //speed is based on fixed point maths
    (*getPointY(pt)) += ( BAG_Sin(*getPointAngle(pt)) * fix_norm(*getPointSpeed(pt)) );
    (*getPointX(pt)) += ( BAG_Cos(*getPointAngle(pt)) * fix_norm(*getPointSpeed(pt)) );
}

static char onscreenPoint(Point_t *pt){
    return (fix_norm(*pt->getX(pt)) >= 0 && fix_norm(*pt->getX(pt)) <= GAME_WIDTH &&
            fix_norm(*pt->getY(pt)) >= 0 && fix_norm(*pt->getY(pt)) <= GAME_HEIGHT);
}

void initPoint(Point_t *pt){
    memset(pt, 0, sizeof(Point_t));
    pt->x = -32;
    pt->y = -32;
    pt->speed = 0;
    pt->angle = 0;

    pt->getX = (void*)&getPointX;
    pt->getY = (void*)&getPointY;
    pt->getSpeed = (void*)&getPointSpeed;
    pt->getAngle = (void*)&getPointAngle;
    pt->onScreen = (void*)&onscreenPoint;
    pt->update = (void*)&updatePoint;
}



/*==========================================================================
Collision Methods
==========================================================================*/
//object on object collision
int obj_collision(GFXObj_t *spr1, GFXObj_t *spr2){
    register int w1 = *BAG_Display_GetGfxFrameWd(spr1),
                 h1 = *BAG_Display_GetGfxFrameHt(spr1),
                 x1 = *BAG_Display_GetGfxBlitX(spr1) + (w1 >> 1),
                 y1 = *BAG_Display_GetGfxBlitY(spr1) + (h1 >> 1),
                 w2 = *BAG_Display_GetGfxFrameWd(spr2),
                 h2 = *BAG_Display_GetGfxFrameHt(spr2),
                 x2 = *BAG_Display_GetGfxBlitX(spr2) + (w2 >> 1),
                 y2 = *BAG_Display_GetGfxBlitY(spr2) + (h2 >> 1);
    //rectangular collision logic
    return((x2 >= x1 - ((w1 + w2)>>1)) && (x2 <= x1 + ((w1 + w2)>>1)) && (y2 >= y1 - ((h1 + h2)>>1)) && (y2 <= y1 + ((h1 + h2)>>1)));
}
//object with point data on object collision
int obj_collision_PtObj(Point_t *p1, GFXObj_t *spr1, GFXObj_t *spr2){
    register int w1 = *BAG_Display_GetGfxFrameWd(spr1),
                 h1 = *BAG_Display_GetGfxFrameHt(spr1),
                 x1 = fix_norm(*p1->getX(p1)) + (w1 >> 1),
                 y1 = fix_norm(*p1->getY(p1)) + (h1 >> 1),
                 w2 = *BAG_Display_GetGfxFrameWd(spr2),
                 h2 = *BAG_Display_GetGfxFrameHt(spr2),
                 x2 = *BAG_Display_GetGfxBlitX(spr2) + (w2 >> 1),
                 y2 = *BAG_Display_GetGfxBlitY(spr2) + (h2 >> 1);
    //rectangular collision logic
    return((x2 >= x1 - ((w1 + w2)>>1)) && (x2 <= x1 + ((w1 + w2)>>1)) && (y2 >= y1 - ((h1 + h2)>>1)) && (y2 <= y1 + ((h1 + h2)>>1)));   
}
//objects with point data collision
int obj_collision_PtPt(Point_t *p1, GFXObj_t *spr1, Point_t *p2, GFXObj_t *spr2){
    register int w1 = *BAG_Display_GetGfxFrameWd(spr1),
                 h1 = *BAG_Display_GetGfxFrameHt(spr1),
                 x1 = fix_norm(*p1->getX(p1)) + (w1 >> 1),
                 y1 = fix_norm(*p1->getY(p1)) + (h1 >> 1),
                 w2 = *BAG_Display_GetGfxFrameWd(spr2),
                 h2 = *BAG_Display_GetGfxFrameHt(spr2),
                 x2 = fix_norm(*p2->getX(p2)) + (w2 >> 1),
                 y2 = fix_norm(*p2->getY(p2)) + (h2 >> 1);
    //rectangular collision logic
    return((x2 >= x1 - ((w1 + w2)>>1)) && (x2 <= x1 + ((w1 + w2)>>1)) && (y2 >= y1 - ((h1 + h2)>>1)) && (y2 <= y1 + ((h1 + h2)>>1)));  
}

//check if a graphics is rendered in an area on screen
int obj_collisionArea(GFXObj_t *gfx, int x1, int y1, int x2, int y2){
    int x = *BAG_Display_GetGfxBlitX(gfx),
        y = *BAG_Display_GetGfxBlitY(gfx),
        wd = *BAG_Display_GetGfxFrameWd(gfx),
        ht = *BAG_Display_GetGfxFrameHt(gfx);
    return ( ((x >= x1 && x <= x2) || (x+wd >= x1 && x+wd <= x2)) && 
             ((y >= y1 && y <= y2) || (y+ht >= y1 && y+ht <= y2)) );
}
//check if an object is in an area based on its point on screen
int obj_collisionArea_Pt(Point_t *pt, GFXObj_t *gfx, int x1, int y1, int x2, int y2){
    int x = fix_norm(*pt->getX(pt)),
        y = fix_norm(*pt->getY(pt)),
        wd = *BAG_Display_GetGfxFrameWd(gfx),
        ht = *BAG_Display_GetGfxFrameHt(gfx);

    return ( ((x >= x1 && x < x2) || (x+wd >= x1 && x+wd < x2)) && 
             ((y >= y1 && y < y2) || (y+ht >= y1 && y+ht < y2)) );
}

/*=====================================
Tile Collision
=======================================*/
static int getTileCol(TiledBG_t *bg, int scrollX, int scrollY, int x, int y, int wd, int ht, unsigned int *matrix[4][3]){
    int conditions = 0;
    int cX = x - scrollX;
    int cY = y - scrollY;

    //check top collision
    matrix[0][0] = BAG_TileBG_SetTile_GetTilePixAddr(bg, cX + 1, cY);
    matrix[0][1] = BAG_TileBG_SetTile_GetTilePixAddr(bg, cX + (wd>>1), cY);
    matrix[0][2] = BAG_TileBG_SetTile_GetTilePixAddr(bg, cX + wd - 1, cY);
    if( (matrix[0][0] && *matrix[0][0]) || (matrix[0][1] && *matrix[0][1]) || (matrix[0][2] && *matrix[0][2]))
        SET_FLAG(conditions, COLLISION_UP);

    //check bottom collision
    matrix[1][0] = BAG_TileBG_SetTile_GetTilePixAddr(bg, cX + 1, cY + ht);
    matrix[1][1] = BAG_TileBG_SetTile_GetTilePixAddr(bg, cX + (wd>>1), cY + ht);
    matrix[1][2] = BAG_TileBG_SetTile_GetTilePixAddr(bg, cX + wd - 1, cY + ht);
    if( (matrix[1][0] && *matrix[1][0]) || (matrix[1][1] && *matrix[1][1]) || (matrix[1][2] && *matrix[1][2]))
        SET_FLAG(conditions, COLLISION_DOWN);

    //check left collision
    matrix[2][0] = BAG_TileBG_SetTile_GetTilePixAddr(bg, cX, cY + 1);
    matrix[2][1] = BAG_TileBG_SetTile_GetTilePixAddr(bg, cX, cY + (ht>>1));
    matrix[2][2] = BAG_TileBG_SetTile_GetTilePixAddr(bg, cX, cY + ht - 1);
    if( (matrix[2][0] && *matrix[2][0]) || (matrix[2][1] && *matrix[2][1]) || (matrix[2][2] && *matrix[2][2]))
        SET_FLAG(conditions, COLLISION_LEFT);

    //check right collision
    matrix[3][0] = BAG_TileBG_SetTile_GetTilePixAddr(bg, cX + wd, cY + 1);
    matrix[3][1] = BAG_TileBG_SetTile_GetTilePixAddr(bg, cX + wd, cY + (ht>>1));
    matrix[3][2] = BAG_TileBG_SetTile_GetTilePixAddr(bg, cX + wd, cY + ht - 1);
    if( (matrix[3][0] && *matrix[3][0]) || (matrix[3][1] && *matrix[3][1]) || (matrix[3][2] && *matrix[3][2]))
        SET_FLAG(conditions, COLLISION_RIGHT);

    return conditions;   
}

//general object on tile background collision
int obj_collisionTile(GFXObj_t *gfx, Point_t *bgPos, TiledBG_t *bg, unsigned int *matrix[4][3]){
    int conditions = 0;//collision conditions

    int x = fix_norm(*bgPos->getX(bgPos));
    int y = fix_norm(*bgPos->getY(bgPos));

    //check if bullet is in alien area
    if(obj_collisionArea(gfx, x, y, x + (bg->width * bg->tileWd), y + (bg->height * bg->tileHt))){
        int cX = (*BAG_Display_GetGfxBlitX(gfx));
        int cY = (*BAG_Display_GetGfxBlitY(gfx));
        int wd = (*BAG_Display_GetGfxFrameWd(gfx));
        int ht = (*BAG_Display_GetGfxFrameHt(gfx));

        conditions = getTileCol(bg, x, y, cX, cY, wd, ht, matrix);       
    }
    return conditions;   
}

int obj_collisionTile_Pt(Point_t *pPos, GFXObj_t *gfx, Point_t *bgPos, TiledBG_t *bg, unsigned int *matrix[4][3]){
    int conditions = 0;//collision conditions

    int x = fix_norm(*bgPos->getX(bgPos));
    int y = fix_norm(*bgPos->getY(bgPos));

    //check if bullet is in alien area
    if(obj_collisionArea_Pt(pPos, gfx, x, y, x + (bg->width * bg->tileWd), y + (bg->height * bg->tileHt))){
        int cX = fix_norm(*pPos->getX(pPos));
        int cY = fix_norm(*pPos->getY(pPos));
        int wd = (*BAG_Display_GetGfxFrameWd(gfx));
        int ht = (*BAG_Display_GetGfxFrameHt(gfx));
        conditions = getTileCol(bg, x, y, cX, cY, wd, ht, matrix);   
    }
    return conditions;   
}
