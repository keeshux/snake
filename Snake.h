#ifndef __SNAKE_H
#define __SNAKE_H

#include "Common.h"

#define SM_GAMEOVER             1000
#define ID_TIMER                2000

#define FLAG_STARTED            0x01
/*#define FLAG_PAUSED             0x02*/
#define FLAG_MOVABLE            0x04
#define FLAG_GAMEOVER           0x08
#define FLAG_LEFT               0x10
#define FLAG_UP                 0x20
#define FLAG_RIGHT              0x40
#define FLAG_DOWN               0x80

#define SNAKE_STARTING_LENGTH   4
#define SNAKE_MAX_SPEED         10
#define SNAKE_DEFAULT_SPEED     8

/* linked snake piece */
typedef struct tagSNAKEPIECE {
    USHORT pX;
    USHORT pY;
    struct tagSNAKEPIECE *pPrev;
} SNAKEPIECE;

typedef SNAKEPIECE *LPSNAKEPIECE;
typedef const SNAKEPIECE *LPCSNAKEPIECE;

void SnakeStart(const HWND, const UCHAR, const UCHAR,
                const COLORREF, const COLORREF,
                const HBRUSH, const UCHAR);
void SnakePause(const BOOL);
void SnakeStop();
void SnakeResize(const BOOL);
void SnakeRedraw();

extern BYTE sFlags;

#endif
