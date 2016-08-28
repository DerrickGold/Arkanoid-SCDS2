#include "animations.h"

void Animation_RunProfile(AnimData *profile){
    //set to idle frame if loop is over and wait one cycle
    if(profile->loop > -1 && !profile->done && profile->loopTimes >= profile->loop){
        BAG_Display_SetObjFrame(profile->gfx, FRAME_HOR, profile->idleFrame);

        if(profile->loopTimes >= profile->loop + 1){
            profile->done = 1;
            profile->loopTimes = 0;
            return;
        }
        profile->loopTimes++;
        return;
    }
    profile->done = 0;
    //profile->loopTimes++;
    #ifndef FRAME_USE_PIXEL_OFFSET
        BAG_Display_SetObjFrame(profile->gfx, FRAME_VERT, profile->verticalOffset);
    #else
        (*BAG_Display_GetGfxOffsetY(profile->gfx)) = profile->verticalOffset;
    #endif

    #ifdef FRAME_USE_VARIABLE_SIZE
        BAG_Display_SetGfxFrameDim(profile->gfx, profile->frameWd, profile->frameHt);
    #endif
    BAG_Display_UpdateAnim(profile->gfx, profile->firstFrame, profile->lastFrame, profile->speed, profile->frames, FRAME_HOR);
    profile->loopIncrement++;
    if(profile->loopIncrement > abs(profile->firstFrame - profile->lastFrame)){
        profile->loopIncrement = 0;
        profile->loopTimes++;
    }
    
}

void Animation_ResetProfile(AnimData *profile){
    profile->done = 0;
    profile->loopTimes = 0;
    profile->loopIncrement = 0;
    BAG_Display_StartAnim(profile->gfx);//enable animation for graphics set
}
