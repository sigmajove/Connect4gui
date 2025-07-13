// Connect4gui.cpp : Defines the entry point for the application.
//
#define _CRT_SECURE_NO_WARNINGS
#include "Connect4gui.h"

#include <array>
#include <format>
#include <iostream>

#include "board.h"
#include "framework.h"

#define MAX_LOADSTRING 100

const int kMargin = 5;
const int kRows = 6;
const int kColumns = 7;
const int kButtonHeight = 20;
const int kButtonWidth = 40;
const int kTileSize = 54;
const std::array<int, 7> kButtons = {IDR_MENU1, IDR_MENU2, IDR_MENU3, IDR_MENU4,
                                     IDR_MENU5, IDR_MENU6, IDR_MENU7};
// Global Variables:
HINSTANCE hInst;                      // current instance
WCHAR szTitle[MAX_LOADSTRING];        // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];  // the main window class name

HBITMAP white_circle;
HBITMAP red_circle;
HBITMAP yellow_circle;

// TODO: Figure out how to attach this data to the main window.
std::array<HWND, kColumns> drop_buttons;

HWND restart_button;
const char* const kStartOver = "Start Over";
const char* const kGoSecond = "Go Second";

// The label applied to the button on the bottom of the screen.
const char* restart_label = kGoSecond;

Board app_data;

// Forward declarations of functions included in this code module:
ATOM MyRegisterClass(HINSTANCE hInstance);
BOOL InitInstance(HINSTANCE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK About(HWND, UINT, WPARAM, LPARAM);

void message(const char* msg, const char* title) {
  MessageBoxA(NULL,  // no parent
              msg, title,
              MB_OK | MB_ICONINFORMATION  // Buttons and icon type
  );
}

void CheckResult(bool check, const char* msg) {
  if (!check) {
    message(msg, "error");
  }
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                      _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine,
                      _In_ int nCmdShow) {
  UNREFERENCED_PARAMETER(hPrevInstance);
  UNREFERENCED_PARAMETER(lpCmdLine);

#if 0
  // For debugging
  AllocConsole();
  freopen("CONOUT$", "w", stdout);
  std::cout << "Debug console\n";
#endif

  // Initialize global strings
  LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
  LoadStringW(hInstance, IDC_CONNECT4GUI, szWindowClass, MAX_LOADSTRING);
  MyRegisterClass(hInstance);

  // Perform application initialization:
  if (!InitInstance(hInstance, nCmdShow)) {
    return FALSE;
  }

  HACCEL hAccelTable =
      LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_CONNECT4GUI));

  MSG msg;

  // Main message loop:
  while (GetMessage(&msg, nullptr, 0, 0)) {
    if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }

  return (int)msg.wParam;
}

// Registers the window class.
ATOM MyRegisterClass(HINSTANCE hInstance) {
  WNDCLASSEXW wcex;

  wcex.cbSize = sizeof(WNDCLASSEX);

  wcex.style = CS_HREDRAW | CS_VREDRAW;
  wcex.lpfnWndProc = WndProc;
  wcex.cbClsExtra = 0;
  wcex.cbWndExtra = 0;
  wcex.hInstance = hInstance;
  wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_CONNECT4GUI));
  wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
  wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
  wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_CONNECT4GUI);
  wcex.lpszClassName = szWindowClass;
  wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

  return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow) {
  hInst = hInstance;  // Store instance handle in our global variable

  const int kDesiredWidth = kColumns * kTileSize;
  const int kDesiredHeight =
      4 * kMargin + 2 * kButtonHeight + kRows * kTileSize;

  RECT rect = {0, 0, kDesiredWidth, kDesiredHeight};
  BOOL result = AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, TRUE);
  CheckResult(result, "adjust window rect");

  white_circle = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_WHITE));
  red_circle = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_RED));
  yellow_circle = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_YELLOW));

  HWND hWnd = CreateWindowW(szWindowClass, L"Connect 4", WS_OVERLAPPEDWINDOW,
                            CW_USEDEFAULT, CW_USEDEFAULT,
                            rect.right - rect.left, rect.bottom - rect.top,
                            nullptr, nullptr, hInstance, nullptr);

  if (!hWnd) {
    return FALSE;
  }

  for (int c = 0; c < kColumns; ++c) {
    HWND buttonWnd = CreateWindowW(
        L"BUTTON", L"Drop",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        (kTileSize - kButtonWidth) / 2 + kTileSize * c, kMargin, kButtonWidth,
        kButtonHeight, hWnd, (HMENU)(INT_PTR)kButtons[c], hInstance, nullptr);
    if (buttonWnd == NULL) {
      return FALSE;
    }
    drop_buttons[c] = buttonWnd;
  }

  restart_button = CreateWindowA(
      "BUTTON", restart_label,
      WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, kMargin,
      3 * kMargin + kButtonHeight + kRows * kTileSize, 110, kButtonHeight, hWnd,
      (HMENU)(INT_PTR)IDR_RESTART, hInstance, nullptr);
  if (restart_button == NULL) {
    return FALSE;
  }

  ShowWindow(hWnd, nCmdShow);
  UpdateWindow(hWnd);

  return TRUE;
}

void hide_drop_buttons() {
  for (HWND b : drop_buttons) {
    ShowWindow(b, SW_HIDE);
  }
}

void show_drop_buttons() {
  for (HWND b : drop_buttons) {
    ShowWindow(b, SW_SHOW);
  }
}

void DrawCircles(HDC hdc) {
  const HDC white = CreateCompatibleDC(hdc);
  const HGDIOBJ old_white = SelectObject(white, white_circle);

  const HDC red = CreateCompatibleDC(hdc);
  const HGDIOBJ old_red = SelectObject(red, red_circle);

  const HDC yellow = CreateCompatibleDC(hdc);
  const HGDIOBJ old_yellow = SelectObject(yellow, yellow_circle);

  for (int r = 0; r < kRows; ++r) {
    for (int c = 0; c < kColumns; ++c) {
      const int x = kTileSize * c;
      const int y = kMargin + kButtonHeight + kMargin + kTileSize * r;
      HDC color;
      switch (app_data.get_value(kRows - 1 - r, c)) {
        case 1:
          color = red;
          break;
        case 2:
          color = yellow;
          break;
        default:
          color = white;
          break;
      }
      const BOOL success =
          BitBlt(hdc, x, y, kTileSize, kTileSize, color, 0, 0, SRCCOPY);
      CheckResult(success, "bitblt");
    }
  }

  // Cleanup.
  SelectObject(white, old_white);
  SelectObject(red, old_red);
  SelectObject(yellow, old_yellow);
  DeleteDC(white);
  DeleteDC(red);
  DeleteDC(yellow);
}

// Checks whether the game is over.
// If it is, outputs the appropriate message and returns true.
// Otherwise, returns false.
bool CheckGameOver() {
  static const char* const kGameOver = "Game Over";

  const Board::Outcome id = app_data.IsGameOver();
  if (id != Board::Outcome::kContested) {
    hide_drop_buttons();
  }
  switch (id) {
    case Board::Outcome::kRedWins:
      message("Red Wins", kGameOver);
      break;
    case Board::Outcome::kYellowWins:
      message("Yellow Wins", kGameOver);
      break;
    case Board::Outcome::kDraw:
      message("Tie Game", kGameOver);
      break;
    case Board::Outcome::kContested:
      return false;
  }
  return true;
}

void ChangeRestartLabel(const char* label) {
  if (restart_label != label) {
    restart_label = label;
    CheckResult(SendMessageA(restart_button, WM_SETTEXT, 0,
                             reinterpret_cast<LPARAM>(restart_label)),
                "WM_SETTEXT failed");
  }
}

void Drop(HWND hWnd, std::size_t col) {
  const std::size_t row = app_data.drop(col);
  if (row == Board::kNumRows - 1) {
    ShowWindow(drop_buttons[col], SW_HIDE);
  }
  RedrawWindow(hWnd, NULL, NULL,
               RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam,
                         LPARAM lParam) {
  switch (message) {
    case WM_COMMAND: {
      int wmId = LOWORD(wParam);
      // Parse the menu selections:
      switch (wmId) {
        case IDM_ABOUT:
          DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
          break;

        case IDM_EXIT:
          DestroyWindow(hWnd);
          break;

        case IDR_MENU1:
        case IDR_MENU2:
        case IDR_MENU3:
        case IDR_MENU4:
        case IDR_MENU5:
        case IDR_MENU6:
        case IDR_MENU7:
          if (restart_label == kGoSecond) {
            // Initialize a new game.
            ChangeRestartLabel(kStartOver);
          }

          Drop(hWnd, wmId - IDR_MENU1);
          if (!CheckGameOver()) {
            Drop(hWnd, app_data.find_move(/*depth=*/6));
            CheckGameOver();
          }
          break;

        case IDR_RESTART:
          if (restart_label == kStartOver) {
            // Restart the game.
            app_data.clear();
            RedrawWindow(hWnd, NULL, NULL,
                         RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
            show_drop_buttons();
            ChangeRestartLabel(kGoSecond);
          } else {
            // Go Second function
            app_data.set_favorite(1);  // The computer goes first.
            Drop(hWnd, 3);
          }
        default:
          return DefWindowProc(hWnd, message, wParam, lParam);
      }
    } break;
    case WM_PAINT: {
      PAINTSTRUCT ps;
      const HDC hdc = BeginPaint(hWnd, &ps);
      DrawCircles(hdc);
      EndPaint(hWnd, &ps);
    } break;
    case WM_DESTROY:
      PostQuitMessage(0);
      break;
    default:
      return DefWindowProc(hWnd, message, wParam, lParam);
  }
  return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
  UNREFERENCED_PARAMETER(lParam);
  switch (message) {
    case WM_INITDIALOG:
      return (INT_PTR)TRUE;

    case WM_COMMAND:
      if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
        EndDialog(hDlg, LOWORD(wParam));
        return (INT_PTR)TRUE;
      }
      break;
  }
  return (INT_PTR)FALSE;
}
