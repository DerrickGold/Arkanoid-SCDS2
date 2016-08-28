#ifndef _ANIMATIONS_H_
#define _ANIMATIONS_H_

#include <libBAG.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FRAME_USE_VARIABLE_SIZE
#define FRAME_USE_PIXEL_OFFSET

typedef struct AnimData{
    GFXObj_t *gfx;
    char firstFrame, lastFrame, idleFrame;
    int speed, frames;
    char loop, done;
    int verticalOffset, timer, loopTimes, loopIncrement;

    #ifdef FRAME_USE_VARIABLE_SIZE
        int frameWd, frameHt;
    #endif

}AnimData;

extern void Animation_RunProfile(AnimData *profile);
extern void Animation_ResetProfile(AnimData *profile);

#ifdef __cplusplus
}
#endif


#endif
