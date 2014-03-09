//
// Copyright (C) 2002 Davide De Rosa
// License: http://www.gnu.org/licenses/gpl.html GPL version 3 or higher
//

#include "Snake.h"

static void SnakeDrawFood();
static void CALLBACK SnakeTimerProc(HWND, UINT, UINT, DWORD);
static void __fastcall SnakeDrawPiece(LPCSNAKEPIECE, const HBRUSH);

/* window and device context */
static HWND                 sWnd;
static HDC                  sDC;

/* window size */
static USHORT               sWndWidth;
static USHORT               sWndHeight;
static USHORT               sWndMidX;
static USHORT               sWndMidY;

/* colors */
static COLORREF             sSnakeColor;
static COLORREF             sFoodColor;

/* brushes for drawing */
static HBRUSH               sBackBrush;
static HBRUSH               sSnakeBrush;
static HBRUSH               sFoodBrush;

/* game status */
static UCHAR                sSpeed;
/*static BOOL                 sGameOver;*/
static USHORT               sInterval;

/* snake and food pointers */
static LPSNAKEPIECE         sHead;
static LPSNAKEPIECE         sTail;
static SNAKEPIECE           sFood;

/* snake/board piece size */
static USHORT               sSize;

static UCHAR                sPcX;
static UCHAR                sPcY;

/* handle to the process heap */
static HANDLE               sHeap;

/* flags bitmask about current status */
BYTE                        sFlags = 0;

void SnakeStart(const HWND wnd, const UCHAR pcX, const UCHAR pcY,
                const COLORREF snakeColor, const COLORREF foodColor,
                const HBRUSH backBrush, const UCHAR speed) {

    /* window and device context */
    sWnd = wnd;
    sDC = GetDC(sWnd);

    /* board size */
    sPcX = pcX;
    sPcY = pcY;

    /* resize window */
    SnakeResize(FALSE);

    /* start moving right */
    sFlags = FLAG_STARTED | FLAG_MOVABLE | FLAG_RIGHT;

    /* configuration */
    sSnakeColor = snakeColor;
    sFoodColor = foodColor;
    sSpeed = speed;

    /* save brushes */
    sBackBrush = backBrush;
    sSnakeBrush = CreateSolidBrush(snakeColor);
    sFoodBrush = CreateSolidBrush(foodColor);

    /* select null pen */
    SelectObject(sDC, (HPEN) GetStockObject(NULL_PEN));

    /* allocate snake pieces */
    {
        LPSNAKEPIECE p;
        UCHAR i;

        /* start from the tail */
        sHeap = GetProcessHeap();
        sTail = (LPSNAKEPIECE) HeapAlloc(sHeap, HEAP_ZERO_MEMORY, sizeof(SNAKEPIECE));

        /* iterate and allocate */
        p = sTail;
        for (i = 0; i < SNAKE_STARTING_LENGTH; ++i) {
            p->pX = sWndMidX + i * sSize;
            p->pY = sWndMidY;

            /* last piece */
            if (i == SNAKE_STARTING_LENGTH - 1) {
                break;
            }

            /* allocate new piece and move on */
            p->pPrev = (LPSNAKEPIECE) HeapAlloc(sHeap, HEAP_ZERO_MEMORY, sizeof(SNAKEPIECE));
            p = p->pPrev;
        }
        sHead = p;
        sHead->pPrev = sTail;
    }

    /* draw some food */
    SnakeDrawFood();

    /* setup timer interval by speed (quadratic) */
    sInterval = 40 + (SNAKE_MAX_SPEED - sSpeed) * (SNAKE_MAX_SPEED - sSpeed);

    /* start game timer */
    SetTimer(sWnd, ID_TIMER, sInterval, SnakeTimerProc);
}

void SnakePause(const BOOL bPause) {

    /* toggle timer */
    if (bPause) {
        KillTimer(sWnd, ID_TIMER);
    } else {
        SetTimer(sWnd, ID_TIMER, sInterval, SnakeTimerProc);
    }
}

void SnakeStop() {
    static wchar_t gameOver[100];

    /* stop timer */
    KillTimer(sWnd, ID_TIMER);

    {
        /* deallocate snake pieces */
        LPSNAKEPIECE p = sTail;
        LPSNAKEPIECE toFree = NULL;
        USHORT length = 0;
        do {
            toFree = p;
            p = p->pPrev;
            HeapFree(sHeap, 0, toFree);

            /* count snake length */
            ++length;
        } while (p != sTail);

        /* show game over message */
        if (sFlags & FLAG_GAMEOVER) {
            wsprintf(gameOver, STR_GAME_OVER, sSpeed * (length - SNAKE_STARTING_LENGTH));
            MessageBox(sWnd, gameOver, STR_SNAKE, MB_ICONSTOP);
        }
    }

    /* invalidate heap and flags */
    sHeap = NULL;
    sFlags = 0;

    /* erase background and release device context */
    SendMessage(sWnd, WM_ERASEBKGND, (WPARAM) sDC, 0);
    ReleaseDC(sWnd, sDC);
}

void SnakeResize(const BOOL bActive) {
    USHORT oldSize = sSize;

    /* snake piece size based on window size */
    sSize = (GetSystemMetrics(SM_CXSCREEN) - 300) / 45;

    /* resize snake if currently playing */
    if (bActive) {
        LPSNAKEPIECE p = sTail;
        do {
            p->pX = p->pX / oldSize * sSize;
            p->pY = p->pY / oldSize * sSize;
            p = p->pPrev;
        } while (p != sTail);

        sFood.pX = sFood.pX / oldSize * sSize;
        sFood.pY = sFood.pY / oldSize * sSize;
    }

    /* window size by board size */
    sWndWidth = sSize * sPcX;
    sWndHeight = sSize * sPcY;
    sWndMidX = sSize * ((sPcX - 4) >> 1);
    sWndMidY = sSize * ((sPcY - 1) >> 1);

    /* resize and center window */
    ResizeCenteredWindow(sWnd, sWndWidth + GetSystemMetrics(SM_CXDLGFRAME) * 2,
            sWndHeight + GetSystemMetrics(SM_CYDLGFRAME) * 2 +
            GetSystemMetrics(SM_CYSIZE) + GetSystemMetrics(SM_CYMENU) + 1);
}

void SnakeRedraw() {

    /* redraw every single snake piece */
    LPSNAKEPIECE p = sTail;
    do {
        SnakeDrawPiece(p, sSnakeBrush);
        p = p->pPrev;
    } while (p != sTail);

    /* redraw food */
    SnakeDrawPiece(&sFood, sFoodBrush);
}

void CALLBACK SnakeTimerProc(HWND hWndSnake, UINT uMsg, UINT idEvent, DWORD dwTime) {
    USHORT x = 0;
    USHORT y = 0;

    /* move snake according to current direction (and possibly wrap) */
    if (sFlags & FLAG_LEFT) {
        x = (sHead->pX ? sHead->pX : sWndWidth) - sSize;
        y = sHead->pY;
    } else if (sFlags & FLAG_UP) {
        x = sHead->pX;
        y = (sHead->pY ? sHead->pY : sWndHeight) - sSize;
    } else if (sFlags & FLAG_RIGHT) {
        x = sHead->pX + sSize;
        y = sHead->pY;
        if (x == sWndWidth) {
            x = 0;
        }
    } else if (sFlags & FLAG_DOWN) {
        x = sHead->pX;
        y = sHead->pY + sSize;
        if (y == sWndHeight) {
            y = 0;
        }
    } else {

        /* FIXME: program error, a direction must be set */
    }

    /* end game on clash */
    if (!((x == sTail->pX) && (y == sTail->pY)) &&
            (GetPixel(sDC, x + 1, y + 1) == sSnakeColor)) {

        sFlags |= FLAG_GAMEOVER;
        PostMessage(sWnd, SM_GAMEOVER, 0, 0);
        return;
    }

    /* food taken */
    else if ((x == sFood.pX) && (y == sFood.pY)) {

        /* add a piece to the snake */
        sHead->pPrev = (LPSNAKEPIECE) HeapAlloc(sHeap, HEAP_ZERO_MEMORY, sizeof(SNAKEPIECE));
        sHead = sHead->pPrev;
        sHead->pPrev = sTail;

        /* draw a new piece of food */
        SnakeDrawFood();
    }

    /* just move */
    else {
        SnakeDrawPiece(sTail, sBackBrush);

        /* swapping head and tail does the trick */
        sHead = sTail;
        sTail = sTail->pPrev;
    }

    /* update current coordinates */
    sHead->pX = x;
    sHead->pY = y;

    /* redraw new head */
    SnakeDrawPiece(sHead, sSnakeBrush);

    /* can only move after draw */
    sFlags |= FLAG_MOVABLE;
}

/* randomly draw some food */
void SnakeDrawFood() {
    do {
        sFood.pX = (rand() % sPcX) * sSize;
        sFood.pY = (rand() % sPcY) * sSize;
    } while (GetPixel(sDC, sFood.pX + 1, sFood.pY + 1) == sSnakeColor);

    SnakeDrawPiece(&sFood, sFoodBrush);
}

/* draw a piece (snake, food or empty) */
void __fastcall SnakeDrawPiece(LPCSNAKEPIECE piece, const HBRUSH brush) {
    SelectObject(sDC, brush);
    Rectangle(sDC,
              piece->pX + 1, piece->pY + 1,
              piece->pX + sSize, piece->pY + sSize);
}
