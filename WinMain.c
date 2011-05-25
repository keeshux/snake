#include "Snake.h"
#include "Resource.h"

/* virtual key is arrow key */
#define IS_ARROW_KEY(key)       ((key >= VK_LEFT) && (key <= VK_DOWN))

/* board size */
#define BOARD_WIDTH             40
#define BOARD_HEIGHT            25

/* colors (green and yellow) */
#define SNAKE_COLOR             RGB(0, 0xFF, 0)
#define FOOD_COLOR              RGB(0xFF, 0xFF, 0)

/* global handles */
static HINSTANCE                ghInstance;
static HWND                     ghWndSnake;

/* message loops */
static LRESULT CALLBACK         SnakeProc(HWND, UINT, WPARAM, LPARAM);
static BOOL CALLBACK            OptionsProc(HWND, UINT, WPARAM, LPARAM);
static BOOL CALLBACK            AboutProc(HWND, UINT, WPARAM, LPARAM);

/* start/stop game */
static void                     StartGame();
static void                     StopGame();

/* background */
static HBRUSH                   backBrush;

/* starting speed */
static UCHAR                    speed = SNAKE_DEFAULT_SPEED;

/* game paused flag */
static BOOL                     bIsPaused = FALSE;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
        LPSTR lpCmdLine, int nCmdShow) {

    WNDCLASS wc;
    MSG msg;

    /* initialize random seed */
    srand((unsigned) GetTickCount());

    /* save global HINSTANCE */
    ghInstance = hInstance;

    /* black background */
    backBrush = (HBRUSH) GetStockObject(BLACK_BRUSH);

    /* window properties */
    wc.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = SnakeProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SNAKE));
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = backBrush;
    wc.lpszMenuName = MAKEINTRESOURCE(IDR_SNAKE);
    wc.lpszClassName = STR_SNAKE;

    /* register window class */
    if (!RegisterClass(&wc)) {
        MessageBox(NULL, STR_FAILED_START, STR_ERROR, MB_ICONSTOP);
        return 0;
    }

    /* create window */
    ghWndSnake = CreateWindow(STR_SNAKE,
                              STR_SNAKE,
                              WS_SYSMENU | WS_MINIMIZEBOX,
                              0,
                              0,
                              GetSystemMetrics(SM_CXSCREEN) / 2,
                              GetSystemMetrics(SM_CYSCREEN) / 2,
                              NULL,
                              NULL,
                              hInstance,
                              NULL);
    if(!ghWndSnake) {
        MessageBox(NULL, STR_FAILED_START, STR_ERROR, MB_ICONSTOP);
        return 0;
    }

    /* show window */
    ShowWindow(ghWndSnake, SW_SHOW);
    UpdateWindow(ghWndSnake);

    /* message loop */
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 1;
}

/* message loop */
LRESULT CALLBACK SnakeProc(HWND hWndSnake, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    ghWndSnake = hWndSnake;

    switch (uMsg) {

    /* finalize on game over */
    case SM_GAMEOVER:
        StopGame();
        break;

    /* center window on creation */
    case WM_CREATE:
        CenterWindow(hWndSnake);
        ModifyMenu(GetMenu(hWndSnake), IDR_PAUSE,
                MF_BYCOMMAND | MF_GRAYED, IDR_PAUSE, STR_MENU_PAUSE);
        break;

    /* redraw snake on request */
    case WM_PAINT:
        if (sFlags & FLAG_STARTED) {
             SnakeRedraw();
        }
        return DefWindowProc(hWndSnake, WM_PAINT, wParam, lParam);

    /* menu handler */
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDR_START:
            StartGame();
            break;

        case IDR_PAUSE:
            SnakePause(bIsPaused = !bIsPaused);
            ModifyMenu(GetMenu(hWndSnake), IDR_PAUSE, MF_BYCOMMAND,
                    IDR_PAUSE, bIsPaused ? STR_MENU_RESUME : STR_MENU_PAUSE);
            break;

        case IDR_STOP:
            StopGame();
            break;

        case IDR_OPTIONS:
            if (sFlags & FLAG_STARTED) {
                bIsPaused = TRUE;
                SnakePause(bIsPaused);
            }
            DialogBox(ghInstance, MAKEINTRESOURCE(IDD_OPTIONS),
                    hWndSnake,(DLGPROC) OptionsProc);

            if (sFlags & FLAG_STARTED) {
                bIsPaused = FALSE;
                SnakePause(bIsPaused);
            }
            break;

        case IDR_ABOUT:
            if (sFlags & FLAG_STARTED) {
                bIsPaused = TRUE;
                SnakePause(bIsPaused);
            }
            DialogBox(ghInstance, MAKEINTRESOURCE(IDD_ABOUT),
                    hWndSnake, (DLGPROC) AboutProc);

            if (sFlags & FLAG_STARTED) {
                bIsPaused = FALSE;
                SnakePause(bIsPaused);
            }
            break;

        case IDR_EXIT:
            StopGame();
            DestroyWindow(hWndSnake);
            break;
        }

        break;

    /* keyboard input */
    case WM_KEYDOWN:

        /* game started, movable flag (set on timer interrupt), arrow key */
        if (!(sFlags & FLAG_STARTED) || !(sFlags & FLAG_MOVABLE) || !IS_ARROW_KEY(wParam)) {
            break;
        }

        /* resume game on keydown */
        if (bIsPaused) {
            bIsPaused = FALSE;
            SnakePause(bIsPaused);
            ModifyMenu(GetMenu(hWndSnake), IDR_PAUSE,
                    MF_BYCOMMAND, IDR_PAUSE, STR_MENU_PAUSE);
        }

        /* determine legal move and update direction */
        {
            const UCHAR keyCode = wParam;
            switch (keyCode) {
            case VK_LEFT:
                if (sFlags & (FLAG_UP | FLAG_DOWN)) {
                    sFlags &= 0x0F;
                    sFlags |= FLAG_LEFT;
                }
                break;

            case VK_UP:
                if (sFlags & (FLAG_LEFT | FLAG_RIGHT)) {
                    sFlags &= 0x0F;
                    sFlags |= FLAG_UP;
                }
                break;

            case VK_RIGHT:
                if (sFlags & (FLAG_UP | FLAG_DOWN)) {
                    sFlags &= 0x0F;
                    sFlags |= FLAG_RIGHT;
                }
                break;

            case VK_DOWN:
                if (sFlags & (FLAG_LEFT | FLAG_RIGHT)) {
                    sFlags &= 0x0F;
                    sFlags |= FLAG_DOWN;
                }
                break;
            }
        }

        /* prevent move until redraw */
        sFlags &= ~FLAG_MOVABLE;

        break;

    /* resize on resolution change */
    case WM_DISPLAYCHANGE:
        if (sFlags & FLAG_STARTED) {
            SnakeResize(TRUE);
        } else {
            ResizeCenteredWindow(hWndSnake,
                    GetSystemMetrics(SM_CXSCREEN) / 2,
                    GetSystemMetrics(SM_CYSCREEN) / 2);
        }
        break;

    /* leave on destroy */
    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWndSnake, uMsg, wParam, lParam);
    }

    return 0;
}

BOOL CALLBACK OptionsProc(HWND hDlgOptions, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static HWND hSpeed;

    switch(uMsg) {
    case WM_INITDIALOG:
        CenterWindow(hDlgOptions);

        InitCommonControls();
        hSpeed = GetDlgItem(hDlgOptions, IDC_SPEED);

        SendMessage(hSpeed, TBM_SETRANGE, (WPARAM) FALSE, (LPARAM) MAKELONG(1, SNAKE_MAX_SPEED));
        SendMessage(hSpeed, TBM_SETPOS, (WPARAM) TRUE, (LPARAM) speed);
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK:
            speed = (UCHAR) SendMessage(hSpeed, TBM_GETPOS, (WPARAM) 0, (LPARAM) 0);
            EndDialog(hDlgOptions, 0);
            break;

        case IDCANCEL:
            EndDialog(hDlgOptions, 0);
            break;
        }

        return TRUE;

    case WM_CLOSE:
        EndDialog(hDlgOptions, 0);
        return TRUE;
    }

    return FALSE;
}

BOOL CALLBACK AboutProc(HWND hDlgAbout, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_INITDIALOG:
        CenterWindow(hDlgAbout);
        return TRUE;

    case WM_COMMAND:
    case WM_CLOSE:
        EndDialog(hDlgAbout, 0);
        return TRUE;
    }

    return FALSE;
}

void StartGame() {
    if (sFlags & FLAG_STARTED) {
        SnakeStop();
    }

    SnakeStart(ghWndSnake, BOARD_WIDTH, BOARD_HEIGHT, SNAKE_COLOR, FOOD_COLOR, backBrush, speed);
    ModifyMenu(GetMenu(ghWndSnake), IDR_PAUSE,
            MF_BYCOMMAND | MF_ENABLED, IDR_PAUSE, STR_MENU_PAUSE);
}

void StopGame() {
    if (sFlags & FLAG_STARTED) {
        SnakeStop();
    }

    ModifyMenu(GetMenu(ghWndSnake), IDR_PAUSE,
            MF_BYCOMMAND | MF_GRAYED, IDR_PAUSE, STR_MENU_PAUSE);
    ResizeCenteredWindow(ghWndSnake,
            GetSystemMetrics(SM_CXSCREEN) / 2,
            GetSystemMetrics(SM_CYSCREEN) / 2);
}

void CenterWindow(const HWND hWnd) {
    RECT rc;

    GetWindowRect(hWnd, &rc);
    MoveWindow(hWnd,
               (GetSystemMetrics(SM_CXSCREEN) + rc.left - rc.right) / 2,
               (GetSystemMetrics(SM_CYSCREEN) + rc.top - rc.bottom) / 2,
               rc.right - rc.left,
               rc.bottom - rc.top,
               TRUE);
}

void ResizeCenteredWindow(const HWND hWnd, const UINT width, const UINT height) {
    RECT rc;

    GetWindowRect(hWnd, &rc);
    MoveWindow(hWnd,
               (GetSystemMetrics(SM_CXSCREEN) - width) / 2,
               (GetSystemMetrics(SM_CYSCREEN) - height) / 2,
               width,
               height,
               TRUE);
}
