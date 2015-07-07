#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <wingdi.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <commctrl.h>
#include <tchar.h>
#include <math.h>
#include "main.h"
#include "messages.h"
#include "graphics.h"
#include "resource.h"


// Grid constants
#define GRIDWIDTH		18
#define GRIDHEIGHT		18
// Graphics sizes
#define TILEWIDTH		22
#define TILEHEIGHT		22
#define GRIDPXWIDTH		(GRIDWIDTH*TILEWIDTH)
#define GRIDPXHEIGHT	(GRIDHEIGHT*TILEHEIGHT)
#define SIDEBARWIDTH	100
#define CLIENTWIDTH		645
#define CLIENTHEIGHT	462
// Misc Constants
#define MAXSIDEBARITEMS	10
#define MAXHIGHSCORES	10
#define BRICKNUM		4
#define PATH_SETTINGS	"settings.bin"
#define PATH_SCORES		"scores.bin"
#define EFFECT_MAG		5
// Windows procedures
long FAR PASCAL WndProc   (HWND, UINT, WPARAM, LPARAM);
int  FAR PASCAL NameProc  (HWND, UINT, WPARAM, LPARAM);
int  FAR PASCAL AboutProc (HWND, UINT, WPARAM, LPARAM);
int  FAR PASCAL SplashProc(HWND, UINT, WPARAM, LPARAM);
int  FAR PASCAL OptionProc(HWND, UINT, WPARAM, LPARAM);
int  FAR PASCAL ScoresProc(HWND, UINT, WPARAM, LPARAM);
// File I/O & Settings
void SaveSettings (void);
void LoadSettings (void);
void ResetSettings(void);
void SaveScores   (void);
void LoadScores   (void);
void SaveGame     (char *szFile);
void LoadGame     (char *szFile);
void OpenDoc(HWND, char *);
// Brick Matrix
void GridEffect (void);
void DrawGrid   (HDC hDC);
void InitGrid   (int nColors);
bool IsInBounds (int x, int y);
void BrickEffect(int x, int y);
bool IsTop      (int x, int y, int nColor);
bool IsBottom   (int x, int y, int nColor);
bool IsLeft	    (int x, int y, int nColor);
bool IsRight    (int x, int y, int nColor);
void FindBrick  (int x, int y, POINTS *p);
bool CanRemove  (int x, int y);
void SetGridHighlight(bool highlight);
// Brick Queue
void HighlightArea(int x, int y);
void CreateQueue(int x, int y);
void CollapseGrid(void);
int  GetBrickCount(void);
bool CheckForMoves(void);
int  CheckListItem(int x, int y);
void ClearBrickOffset(void);
// Sidebar
void DrawSidebarItems(HDC hdc, int x, int y);
void UpdateSidebarItems(void);
void UpdatePlayerStatus(int nIndex);
void UpdateScoreStatus(int nIndex);
int  AddSidebarItem(char *szTitle, char *szCaption);
void ModifySidebarItem(int nIndex, char *szNewTitle, char *szNewCaption);
void HideSidebarItem(int nIndex);
void ShowSidebarItem(int nIndex);
void ToggleSidebarVisible(int nIndex);
int  GetHeaderByPixel(int x, int y);
// High Scores
void LoadScores(void);
void SaveScores(void);
void ClearScores(void);
bool IsHighScore(long lScore);
bool InsertScore(char *szName, long lScore);

// GDI Handles
HBRUSH		hTheme[3];
HBITMAP		hBrick[4];
HBITMAP		hbHighlight;
HBITMAP		hShadow;
HBITMAP		hbHeadUp, hbHeadUpSel;
HBITMAP		hbHeadDn, hbHeadDnSel;
HBITMAP		hbHeadLeft;
// Global Handles
HINSTANCE	g_hInst;
int			nPlayerBar;
int			nScoreBar;
int			nDebugBar;
// Gameplay Vars
t_Brick			brGrid[GRIDWIDTH][GRIDHEIGHT];	// Brick Matrix
t_Brick			brUndo[GRIDWIDTH][GRIDHEIGHT];	// Undo Brick Matrix
t_HighScore		hsList[MAXHIGHSCORES];			// High scores list
t_BrickQueue	bqList[GRIDWIDTH * GRIDHEIGHT]; // Brick area queue
t_SidebarItem	Sidebar[MAXSIDEBARITEMS];		// Sidebar items
long			lScore;							// Current Score
long			lUndoScore;						// Undo Score
bool			bGameOver;						// Is Game Over?
bool			bCanUndo;						// Can player undo move?
int				nEndStep;						// Game over rectangle resizing
int				nPieceCount;					// Number of pieces left in the grid
// Settings
typedef struct BSettings {
	bool bSplash;		// Show splash screen at startup? [def: true]
	bool bShadow;		// Draw brick shadows? [def: true]
	bool bSound;		// Play sound effects? [def: false]
	bool bAutoSave;		// Automatically save game on exit? [def: true]
	char szName[64];	// Name of player [def: Anonymous]
} t_BSettings;

t_BSettings Settings;

//-----------------------------------------------------------------------------
// Name: WinMain()
// Desc: Code entry point.
//-----------------------------------------------------------------------------
int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
	HACCEL		hAccelTable;
	WNDCLASS	wc;
	HWND		hWnd;
	MSG			msg;
	
	g_hInst = hInstance;
	m_pSetLayeredWindowAttributes = (lpfnSetLayeredWindowAttributes)GetProcAddress(hUser32, "SetLayeredWindowAttributes");

	if(!hPrevInstance) {
		wc.style			= CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc		= WndProc;
		wc.cbClsExtra		= 0;
		wc.cbWndExtra		= 0;
		wc.hbrBackground	= CreateSolidBrush(BCOLOR_LIGHT);
		wc.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MAIN));
		wc.hCursor			= LoadCursor(g_hInst, IDC_ARROW);
		wc.hInstance		= hInstance;
		wc.lpszMenuName		= MAKEINTRESOURCE(IDM_MAIN);
		wc.lpszClassName	= "Bricksv2";
		RegisterClass(&wc);
	}

	hWnd = CreateWindow("Bricksv2", "Bricks v2.0", 
		WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE,
		(GetSystemMetrics(SM_CXSCREEN) - 640) / 2,
		(GetSystemMetrics(SM_CYSCREEN) - 480) / 2,
		640, 500, NULL, NULL, hInstance, NULL);

	ShowWindow(hWnd, nShowCmd);
	UpdateWindow(hWnd);

	hAccelTable = LoadAccelerators(hInstance, (LPCTSTR)IDC_MAIN);

	while(GetMessage(&msg, NULL, 0, 0)) {
		if(!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return msg.wParam;
}

//-----------------------------------------------------------------------------
// Name: WndProc()
// Desc: Procedure for the main window.
//-----------------------------------------------------------------------------
long FAR PASCAL WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static HPEN			hBlank;
	static HPEN			hpOutline;
	static HFONT		hfNormal;
	static BITMAP		bmBuffer;
	static HBITMAP		hParticle;
	static HBITMAP		hBuffer;
	static HCURSOR		hcArrow;
	static HCURSOR		hcHand;
	static char			szEgg[6];
	PAINTSTRUCT			ps;
	SYSTEMTIME			st;
	RECT				rc;
	HDC					hDC, hdcMem;
	int					i;
	int					x, y;
	static int			nFrame;
	static bool			bDrawing = false;
	char				szBuf[1024];
	DWORD				dwError = 0;
		
	switch(message) {
		case WM_CREATE:
		{
			nFrame = 0;
			nEndStep = 0;
			ZeroMemory(szEgg, sizeof(szEgg));
			hBlank = CreatePen(PS_NULL, 0, 0);
			hpOutline = CreatePen(PS_SOLID, 1, RGB(150, 150, 150));
			bCanUndo = false;
			hfNormal = MyCreateFont(GetDC(hWnd), "Verdana", 13);
			LoadScores();
			LoadSettings();
			PlaySound(NULL, 0, 0);
			// Resize window (Achieve 645 x 462 client area)
			GetClientRect(hWnd, &rc);
			rc.right = CLIENTWIDTH + (640 - rc.right);
			rc.bottom = CLIENTHEIGHT + (480 - rc.bottom);
			rc.left = (GetSystemMetrics(SM_CXSCREEN) - rc.right)  / 2;
			rc.top =  (GetSystemMetrics(SM_CYSCREEN) - rc.bottom) / 2;
			MoveWindow(hWnd, rc.left, rc.top, rc.right, rc.bottom, false);

			if(Settings.bSplash) DialogBoxParam(g_hInst, MAKEINTRESOURCE(IDD_SPLASH), hWnd, SplashProc, 0);
			// Load Colors
			hTheme[0] = CreateSolidBrush(BCOLOR_LIGHT);
			hTheme[1] = CreateSolidBrush(BCOLOR_MED);
			hTheme[2] = CreateSolidBrush(BCOLOR_DARK);
			// Load Bricks
			hBrick[0] = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_BLUE));
			hBrick[1] = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_GREEN));
			hBrick[2] = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_RED));
			hBrick[3] = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_YELLOW));
			// Load sidebar headers
			hbHeadLeft	= LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_HEAD_LEFT));
			hbHeadUp	= LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_HEAD_UP));
			hbHeadUpSel = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_HEAD_UP_SEL));
			hbHeadDn	= LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_HEAD_DOWN));
			hbHeadDnSel = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_HEAD_DOWN_SEL));
			// Load Overlays
			hShadow	    = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_SHADOW));
			hParticle   = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_PARTICLE));
			hbHighlight = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_BLANK));
			// Create Sidebars
			nPlayerBar = AddSidebarItem("Player Status", "");
			nScoreBar = AddSidebarItem("High Scores", "No High Scores Yet!");
			nDebugBar = AddSidebarItem("Debug Information", "No Errors Reported");
			HideSidebarItem(nDebugBar);
			// Load Cursors
			hcArrow = LoadCursor(NULL, IDC_ARROW);
			hcHand   = LoadCursor(NULL, IDC_HAND);
			// Set Timers
			SetTimer(hWnd, 1033,   20, (TIMERPROC)WndProc);
			SetTimer(hWnd, 1034, 1000, (TIMERPROC)WndProc);
			// Set randomization seed
			GetSystemTime(&st);
			srand((st.wDay + st.wDayOfWeek + st.wHour + st.wMilliseconds + st.wMinute + st.wMonth + st.wSecond + st.wYear) % 36000);
			// Misc Initialization
			ClearMessages();
			InitGrid(BRICKNUM);
			if(Settings.bAutoSave) {
				LoadGame("restore.bin");
				GridEffect();
			}
			if(bGameOver) InitGrid(BRICKNUM);
			break;
		}
		case WM_TIMER:
		{
			switch(wParam) {
				case 1033: // Update bricks & drawing
					for(y = 0; y < GRIDWIDTH; y++) {
						for(x = 0; x < GRIDHEIGHT; x ++) {
							brGrid[x][y].nStep+=20;
							if(brGrid[x][y].nStep > 360) brGrid[x][y].nStep = 0;
							if(brGrid[x][y].x != 0) brGrid[x][y].x /= 1.5f;
							if(brGrid[x][y].y != 0) brGrid[x][y].y /= 1.5f;
						}
					}
					UpdateMessages();
					UpdatePlayerStatus(nPlayerBar);
					UpdateScoreStatus(nScoreBar);
					UpdateSidebarItems();
					if(bGameOver && nEndStep < 50) nEndStep++;
					if(!bGameOver && nEndStep > 0)  nEndStep--;
					if(!bDrawing) InvalidateRect(hWnd, NULL, false);
					break;
				case 1034: // Update FPS counter
				{
					// sprintf(szBuf, "Bricks v2.0 - FPS: %d", nFrame);
					// SetWindowText(hWnd, szBuf);
					nFrame = 0;
					break;
				}
			}
			break;
		}
		case WM_COMMAND:
		{
			switch(LOWORD(wParam)) {
				case IDM_NEW:
					if(!bGameOver) {
						InitGrid(BRICKNUM);
					} else {
						InitGrid(BRICKNUM);
					}
					break;
				case IDM_OPTIONS:
					DialogBoxParam(g_hInst, MAKEINTRESOURCE(IDD_OPTIONS), hWnd, OptionProc, 0L);
					break;
				case IDM_UNDOMOVE:
					if(bCanUndo && !bGameOver) {
						memcpy(brGrid, brUndo, sizeof(brUndo));
						lScore = lUndoScore;
						bCanUndo = false;
						SetGridHighlight(false);
						ClearBrickOffset();
					}
					break;
				case IDM_HIGHSCORES:
					DialogBoxParam(g_hInst, MAKEINTRESOURCE(IDD_SCORES), hWnd, ScoresProc, 0L);
					break;
				case IDM_CHANGES:
					OpenDoc(hWnd, "Changes.txt");
					break;
				case IDM_EXIT:
					SendMessage(hWnd, WM_DESTROY, 0, 0);
					break;
				case IDM_ABOUT:
					DialogBoxParam(g_hInst, MAKEINTRESOURCE(IDD_ABOUT), hWnd, AboutProc, 0);
					break;
			}
			break;
		}
		case WM_KEYDOWN:
		{
			if(wParam != VK_BACK) {
				for(i = 0; i < sizeof(szEgg)-1; i++) {
					szEgg[i] = szEgg[i + 1];
				}
				szEgg[5] = wParam;
			}
			if(!strcmp(szEgg, "MHGAME")) {
				bGameOver = true;
			}
			if(!strcmp(szEgg, "MHSIDE")) {
				ToggleSidebarVisible(nDebugBar);
			}
			if(!strcmp(szEgg, "BRICKS")) {
				for(int y = 0; y < GRIDHEIGHT; y++) {
					for(int x = 0; x < GRIDWIDTH; x++) {
						if(brGrid[x][y].color == 3) {
							brGrid[x][y].color = rand()%2;
						}
					}
				}
			}
			if(!strcmp(szEgg, "CHEATS")) {
				for(int y = 0; y < GRIDHEIGHT; y++) {
					for(int x = 0; x < GRIDWIDTH; x++) {
						brGrid[x][y].color = 0;
					}
				}
			}
			return 0;
		}
		case WM_PAINT:
		{
			if(bDrawing) return false;
			bDrawing = true;

			GetClientRect(hWnd, &rc);
			rc.right++;
			rc.bottom++;

			hDC = BeginPaint(hWnd, &ps);
			hdcMem = CreateCompatibleDC(hDC);
			hBuffer = CreateCompatibleBitmap(hDC, rc.right, rc.bottom);
			SelectObject(hdcMem, hBuffer);
			
			SelectObject(hdcMem, hBlank);
			SelectObject(hdcMem, hTheme[0]);

			Rectangle(hdcMem, 0, 0, rc.right, rc.bottom);
			
			DrawGrid(hdcMem);
			
			SelectObject(hdcMem, hBlank);
			SelectObject(hdcMem, hTheme[1]);
			Rectangle(hdcMem, rc.right - 205, 0, rc.right, rc.bottom);

			DrawSidebarItems(hdcMem, rc.right - 205, 0);

			SetBkMode(hdcMem, TRANSPARENT);

			DrawMessages(hdcMem);

			if(bGameOver || nEndStep > 0) {
				HDC hdcFade = CreateCompatibleDC(hdcMem);
				HBITMAP hbFade = CreateCompatibleBitmap(hdcMem, 330, 66);
				
				SelectObject(hdcFade, hbFade);		
				SelectObject(hdcFade, hpOutline);
				SelectObject(hdcFade, hTheme[0]);
				
				Rectangle(hdcFade, 0, 0, 330, 66);
				strcpy(szBuf, "Select New Game from the file menu to start over.");
				DrawShadowText(hdcFade, 5, 5, BCOLOR_MED, BCOLOR_DARK, "Game Over");
				SelectObject(hdcFade, hfNormal);
				TextOut(hdcFade, 5, 45, szBuf, strlen(szBuf));

				DeleteDC(hdcFade);

				SimpleBlend(hdcMem, hbFade, 61, 193, (int)(200.0f * ((float)nEndStep / 50.0f)));
				DeleteObject(hbFade);
			}

			BitBlt(hDC, 0, 0, rc.right, rc.bottom, hdcMem, 0, 0, SRCCOPY);

			DeleteObject(hBuffer);
			DeleteDC(hdcMem);
			EndPaint(hWnd, &ps);

			nFrame++;
			bDrawing = false;
			return true;
		}
		case WM_MOUSEMOVE:
		{
			POINTS p;
			p = MAKEPOINTS(lParam);
			x = p.x;
			y = p.y;

			// Check the sidebar first
			for(i = 0; i < MAXSIDEBARITEMS; i++) {
				Sidebar[i].bHover = false;
			}
			i = GetHeaderByPixel(x, y);
			if(i != -1) {
				SetCursor(hcHand);
				Sidebar[i].bHover = true;
				break;
			}

			if(!bGameOver) {
				FindBrick(x, y, &p);
				SetGridHighlight(false);
				if(p.x != -1) {
					if(IsLeft(p.x, p.y, brGrid[p.x][p.y].color) || IsRight (p.x, p.y, brGrid[p.x][p.y].color) ||
						IsTop(p.x, p.y, brGrid[p.x][p.y].color) || IsBottom(p.x, p.y, brGrid[p.x][p.y].color)) {
						SetCursor(hcHand);
						HighlightArea(p.x, p.y);
					} else {
						SetCursor(hcArrow);
					}
				} else {
					SetCursor(hcArrow);
				}
			} else {
				if(p.x >= 61 && p.x <= 391 && p.y >= 193 && p.y <= 259) {
					SetCursor(hcHand);
				} else {
					SetCursor(hcArrow);
				}
			}
			break;
		}
		case WM_LBUTTONUP:
		{
			static POINTS p;
			ZeroMemory(&p, sizeof(POINTS));
			p = MAKEPOINTS(lParam);
			x = p.x;
			y = p.y;

			// Check the sidebar first
			i = GetHeaderByPixel(x, y);
			if(i != -1) {
				Sidebar[i].bHover = true;
				Sidebar[i].bDown = !Sidebar[i].bDown;
				if(Settings.bSound) PlaySound(MAKEINTRESOURCE(IDR_CLICK), g_hInst, SND_ASYNC | SND_RESOURCE);
				break;
			}

			if(!bGameOver && x < GRIDPXWIDTH + (TILEWIDTH * 2)) {
				FindBrick(x, y, &p);
				if(CanRemove(p.x, p.y)) {
					if(Settings.bSound) PlaySound(MAKEINTRESOURCE(IDR_CLICK), g_hInst, SND_ASYNC | SND_RESOURCE);
					// Prepare for undo
					memcpy(brUndo, brGrid, sizeof(brGrid));
					lUndoScore = lScore;
					bCanUndo = true;

					// Remove Pieces & Collapse Grid
					CreateQueue(p.x, p.y);
					for(i = 0; i < GRIDWIDTH*GRIDHEIGHT; i++) {
						if(bqList[i].bDoesExist) {
							brGrid[bqList[i].x][bqList[i].y].bDoesExist = false;
							BrickEffect(bqList[i].x, bqList[i].y);
						} else {
							break;
						}
					}
					CollapseGrid();

					nPieceCount = GetBrickCount();
					SetGridHighlight(false);
					lScore += i * i;
					AddMessage((i * i), x, y - 10);
					if(!CheckForMoves()) {
						if(!brGrid[0][GRIDHEIGHT - 1].bDoesExist) { // Player has cleared entire grid
							MakeMessageBonus(AddMessage(4000, (GRIDPXWIDTH + (TILEWIDTH * 2)) / 2, (GRIDPXHEIGHT + (TILEHEIGHT * 2)) / 2));
							lScore += 4000;
						}
						bGameOver = true;
						if(IsHighScore(lScore)) {
							DialogBoxParam(g_hInst, MAKEINTRESOURCE(IDD_NAME), hWnd, NameProc, 0l);
							InsertScore(Settings.szName, lScore);
							UpdateScoreStatus(nScoreBar);
							DialogBoxParam(g_hInst, MAKEINTRESOURCE(IDD_SCORES), hWnd, ScoresProc, 0l);
							SaveScores();
						}
					}
				} else {
					if(Settings.bSound) PlaySound(MAKEINTRESOURCE(IDR_CLICKINV), g_hInst, SND_ASYNC | SND_RESOURCE);
				}
			} else {
				if(p.x >= 61 && p.x <= 391 && p.y >= 193 && p.y <= 259) {
					InitGrid(BRICKNUM);
				}
			}
			break;
		}
		case WM_DESTROY:
		{
			for(i = 0; i < 3; i++) DeleteObject(hTheme[i]);
			for(i = 0; i < 4; i++) DeleteObject(hBrick[i]);
			SaveSettings();
			SaveScores();
			if(Settings.bAutoSave) SaveGame("restore.bin");
			DeleteObject(hpOutline);
			DeleteObject(hbHighlight);
			DeleteObject(hShadow);
			DeleteObject(hParticle);
			DeleteObject(hBlank);
			DeleteObject(hcHand);
			DeleteObject(hcArrow);
			DeleteObject(hfNormal);
			DeleteObject(hbHeadLeft);
			DeleteObject(hbHeadUp);
			DeleteObject(hbHeadUpSel);
			DeleteObject(hbHeadDn);
			DeleteObject(hbHeadDnSel);
			KillTimer(hWnd, 1033);
			KillTimer(hWnd, 1034);
			PostQuitMessage(0);
			break;
		}
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}

//-----------------------------------------------------------------------------
// Name: AboutProc()
// Desc: Procedure for the about dialog box.
//-----------------------------------------------------------------------------
int FAR PASCAL AboutProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static char		szText[] = "Bricks v2.0\r\n\r\nCopyright© 2003-2013, Mark Hamilton. All rights reserved.\r\n\r\n";
	static HBITMAP	hbTitle, hbIcon, hbWShad;
	static HBRUSH	hbBG;
	static HFONT	hfText;
	static HWND		hOK;
	PAINTSTRUCT		ps;
	static RECT		rcWin, rcText, rcOK;
	HDC				hDC;
	int				nTop = 0;

	switch(message) {
		case WM_INITDIALOG:
		{
			// To prevent anomolies...
			ZeroMemory(&rcWin, sizeof(RECT));
			ZeroMemory(&rcText, sizeof(RECT));
			ZeroMemory(&rcOK, sizeof(RECT));

			// Load Graphics and fonts
			hDC = GetDC(hDlg);
			hbTitle = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_LOGO));
			hbIcon  = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_RED));
			hbWShad = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_WSHAD));
			hfText = CreateFont(HEIGHTFROMPOINTS(8), 0, 0, 0, 0, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS,
								CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, FF_DONTCARE, "Verdana");
			hbBG = CreateSolidBrush(RGB(255, 255, 255));

			// Get handles to dialog controls
			hOK = GetDlgItem(hDlg, IDOK);
			GetClientRect(hOK, &rcOK);

			// Window sizing
			MoveWindow(hDlg, 0, 0, 640, 480, false);
			GetClientRect(hDlg, &rcWin);
			int nXDif = abs(640 - rcWin.right);
			int nYDif = abs(480 - rcWin.bottom);

			// Text sizing
			DrawText(hDC, szText, sizeof(szText), &rcText, DT_CALCRECT);
			rcText.right += (rcText.left = 62);
			rcText.bottom += (rcText.top = 122);

			// Move Button
			nTop = rcText.bottom;
			MoveWindow(hOK, 512 - (rcOK.right + 10), nTop, rcOK.right, rcOK.bottom, true);
			nTop += rcOK.bottom + 10;

			// Size dialog
			int nXDiag = 512 + nXDif;
			int nYDiag = nTop + nYDif;
			MoveWindow(hDlg, (GetSystemMetrics(SM_CXSCREEN) - nXDiag) / 2, (GetSystemMetrics(SM_CYSCREEN) - nYDiag) / 2, nXDiag, nYDiag, true);

			return true;
		}
		case WM_COMMAND:
			switch(LOWORD(wParam)) {
				case IDOK:
				case IDCANCEL:
					SendMessage(hDlg, WM_CLOSE, 0, 0);
					break;
			}
			break;
		case WM_PAINT:
			hDC = BeginPaint(hDlg, &ps);
			GetClientRect(hDlg, &rcWin);
			SelectObject(hDC, hbBG);
			Rectangle(hDC, -1, -1, rcWin.right + 1, rcWin.bottom + 1);
			DrawOverlay(hDC, hbWShad, 20, 132);
			DrawBitmap(hDC,	hbIcon, 20, 132);
			DrawBitmap(hDC, hbTitle, 0, 0);
			SetBkMode(hDC, TRANSPARENT);
			SelectObject(hDC, hfText);
			DrawText(hDC, szText, sizeof(szText) - 1, &rcText, DT_LEFT);
			EndPaint(hDlg, &ps);
			return true;
		case WM_CLOSE:
			DeleteObject(hfText);
			DeleteObject(hbIcon);
			DeleteObject(hbBG);
			DeleteObject(hbTitle);
			EndDialog(hDlg, 0);
			break;
	}

	return false;
}
//-----------------------------------------------------------------------------
// Name: SplashProc()
// Desc: Procedure for the splash dialog box.
//-----------------------------------------------------------------------------
int FAR PASCAL SplashProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	OSVERSIONINFO os = { sizeof(os) };
	static bool bDir = true;
	static int nStep = 0;
	static bool bWin2k;
	static int nInv;
	int nAlpha;

	switch(message) {
		case WM_INITDIALOG:
			// Check if win2k or greater before using transparent windows
			GetVersionEx(&os);
			bWin2k = (VER_PLATFORM_WIN32_NT == os.dwPlatformId && os.dwMajorVersion >= 5); 

			SetWindowLong(hDlg, GWL_EXSTYLE, GetWindowLong(hDlg, GWL_EXSTYLE) | WS_EX_LAYERED);
			MoveWindow(hDlg,(GetSystemMetrics(SM_CXSCREEN) - 512) / 2,
							(GetSystemMetrics(SM_CYSCREEN) - 112) / 2, 512, 112, TRUE);
			if(bWin2k) {
				nInv = 30;
			} else {
				nInv = 5;
			}
			SetTimer(hDlg, 5, nInv, (TIMERPROC)SplashProc);
			return true;
		case WM_TIMER:
			nStep++;
			if(nStep >= 100) {
				SendMessage(hDlg, WM_CLOSE, 0, 0);
			}

			// It's a parabola!
			nAlpha = (int)((-0.1f*(((float)nStep-50.0f)*((float)nStep-50.0f))+255.0f));

			if(bWin2k) {
				m_pSetLayeredWindowAttributes(hDlg, NULL, nAlpha, LWA_ALPHA);
			}
			break;
		case WM_CLOSE:
			KillTimer(hDlg, 5);
			EndDialog(hDlg, 0);
			break;
	}

	return false;
}
//-----------------------------------------------------------------------------
// Name: OptionProc()
// Desc: Procedure for the option dialog box.
//-----------------------------------------------------------------------------
int FAR PASCAL OptionProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static HBRUSH hbWindow;

	switch(message) {
		case WM_INITDIALOG:
			hbWindow = CreateSolidBrush(GetSysColor(COLOR_WINDOW));
			// General Options
			CheckDlgButton(hDlg, IDC_CHKSPLASH,  Settings.bSplash);
			CheckDlgButton(hDlg, IDC_CHKSHADOWS, Settings.bShadow);
			CheckDlgButton(hDlg, IDC_CHKSOUND,   Settings.bSound);
			CheckDlgButton(hDlg, IDC_CHKSAVE,    Settings.bAutoSave);
			return true;
		case WM_COMMAND:
			switch(LOWORD(wParam)) {
				case IDOK:
					// General options
					Settings.bSplash   = IsDlgButtonChecked(hDlg, IDC_CHKSPLASH)  == BST_CHECKED ? true : false;
					Settings.bShadow   = IsDlgButtonChecked(hDlg, IDC_CHKSHADOWS) == BST_CHECKED ? true : false;
					Settings.bSound    = IsDlgButtonChecked(hDlg, IDC_CHKSOUND)   == BST_CHECKED ? true : false;
					Settings.bAutoSave = IsDlgButtonChecked(hDlg, IDC_CHKSAVE)    == BST_CHECKED ? true : false;
				case IDCANCEL:
					EndDialog(hDlg, 0);
					break;
			}
			break;
		case WM_CTLCOLORDLG:
		case WM_CTLCOLORSTATIC:
			return (int)hbWindow;
		case WM_CLOSE:
			DeleteObject(hbWindow);
			EndDialog(hDlg, 0);
			break;
	}

	return false;
}
//-----------------------------------------------------------------------------
// Name: ScoresProc()
// Desc: Procedure for the high scores dialog box.
//-----------------------------------------------------------------------------
int FAR PASCAL ScoresProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
 	static HFONT	hfText;
	static HBRUSH	hbWindow;
	static HPEN		hpBlank;
	static HPEN		hpOutline;
	PAINTSTRUCT		ps;
	RECT			rc;
	RECT			rcTextSize;
	RECT			rcBoundBox;
	bool			bHighlight;
	char			szBuffer[64];
	HDC				hDC;
	int				i;
	int				nTop;

	switch(message) {
		case WM_INITDIALOG:
			hDC = GetDC(hDlg);
			hfText = CreateFont(HEIGHTFROMPOINTS(8), 0, 0, 0, FW_DONTCARE, false, false, false, ANSI_CHARSET, OUT_DEFAULT_PRECIS,
								CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, FF_DONTCARE, "Verdana");
			hbWindow = CreateSolidBrush(GetSysColor(COLOR_WINDOW));
			hpBlank = CreatePen(PS_NULL, 0, 0);
			hpOutline = CreatePen(PS_SOLID, 2, BCOLOR_LIGHT);
			return true;
		case WM_PAINT:
			hDC = BeginPaint(hDlg, &ps);
			SelectObject(hDC, hfText);
			SetBkMode(hDC, TRANSPARENT);
			GetClientRect(hDlg, &rc);
			SelectObject(hDC, hbWindow);
			SelectObject(hDC, hpBlank);
			Rectangle(hDC, 0, 0, rc.right + 1, rc.bottom + 1);

			// Draw the outline
			SelectObject(hDC, hpOutline);
			ZeroMemory(&rcTextSize, sizeof(rcTextSize));
			DrawText(hDC, hsList[0].sName, strlen(hsList[0].sName), &rcTextSize, DT_CALCRECT);
			Rectangle(hDC, 10, 10, rc.right - 10, rcTextSize.bottom * (2 * (MAXHIGHSCORES)) + 10);

			nTop = 15;
			bHighlight = true;
			SelectObject(hDC, hpBlank);
			for(i = 0; i < MAXHIGHSCORES; i++) {
				ZeroMemory(&rcTextSize, sizeof(rcTextSize));
				ZeroMemory(&rcBoundBox, sizeof(rcBoundBox));
				DrawText(hDC, hsList[i].sName, strlen(hsList[i].sName), &rcTextSize, DT_CALCRECT);
				rcBoundBox.left = 15;
				rcBoundBox.top = nTop;
				rcBoundBox.right = rc.right - 14;
				rcBoundBox.bottom = rcTextSize.bottom + rcBoundBox.top;
				if(bHighlight) {
					SelectObject(hDC, hTheme[0]);
					Rectangle(hDC, rcBoundBox.left - 5, rcBoundBox.top - 5, rcBoundBox.right + 5, rcBoundBox.bottom + 5);
				}
				bHighlight = !bHighlight;
				_ltoa(hsList[i].lScore, (char *)szBuffer, 10);
				DrawText(hDC, hsList[i].sName, strlen(hsList[i].sName), &rcBoundBox, DT_LEFT);
				DrawText(hDC, szBuffer,        strlen(szBuffer),        &rcBoundBox, DT_RIGHT);
				nTop += rcTextSize.bottom * 2;
			}

			EndPaint(hDlg, &ps);
			return 0;
		case WM_COMMAND:
			switch(wParam) {
				case IDC_OK:
					SendMessage(hDlg, WM_CLOSE, 0, 0l);
					break;
				case IDCANCEL:
					SendMessage(hDlg, WM_CLOSE, 0, 0l);
					break;
				case IDC_CLEAR:
					if(HIWORD(wParam) == 0) {
						if(MessageBox(hDlg, "Are you sure you want to clear the high score list?", "Confirm", MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2) == IDYES) {
							ClearScores();
							InvalidateRect(hDlg, NULL, true);
						}
					}
					break;
			}
			break;
		case WM_CLOSE:
			DeleteObject(hpBlank);
			DeleteObject(hbWindow);
			DeleteObject(hfText);
			EndDialog(hDlg, 0);
			break;
	}

	return false;
}
//-----------------------------------------------------------------------------
// Name: NameProc()
// Desc: Procedure for the high score name dialog box.
//-----------------------------------------------------------------------------
int FAR PASCAL NameProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static HBRUSH hbWindow;

	switch(message) {
		case WM_INITDIALOG:
			hbWindow = CreateSolidBrush(GetSysColor(COLOR_WINDOW));
			SetWindowText(GetDlgItem(hDlg, IDC_NAME), (LPCTSTR)Settings.szName);
			return true;
		case WM_CTLCOLORDLG:
		case WM_CTLCOLORSTATIC:
			return (int)hbWindow;
		case WM_COMMAND:
			switch(LOWORD(wParam)) {
				case IDOK:
					GetWindowText(GetDlgItem(hDlg, IDC_NAME), (LPSTR)Settings.szName, 64);
					if(strlen(Settings.szName) == 0) {
						strcpy(Settings.szName, "Anonymous");
					}
					EndDialog(hDlg, 0);
					break;
			}
			break;
		case WM_CLOSE:
			DeleteObject(hbWindow);
			EndDialog(hDlg, 0);
			break;
	}

	return false;
}
//-----------------------------------------------------------------------------
// Name: DrawSidebarItems()
// Desc: Draws the whole sidebar.
//-----------------------------------------------------------------------------
void DrawSidebarItems(HDC hdc, int x, int y)
{
	int i;
	int nAlpha;
	int nHeight;
	int nTop = 0;
	RECT rc;
	HDC hdcMem;
	HPEN	hpBlank = CreatePen(PS_NULL, 0, 0);
	HFONT	hfTitle = CreateFont(15, 0, 0, 0, FW_HEAVY,    false, false, false, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, FF_DONTCARE, "Verdana");
	HFONT	hfBody  = CreateFont(13, 0, 0, 0, FW_DONTCARE, false, false, false, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, FF_DONTCARE, "Verdana");
	HBITMAP	hbBuffer = NULL;

	for(i = 0; i < MAXSIDEBARITEMS; i++) {
		if(Sidebar[i].bDoesExist && Sidebar[i].bVisible) {
			ZeroMemory(&rc, sizeof(rc));
			// Draw Header
			nTop += 10;
			DrawBitmap(hdc, hbHeadLeft, x + 10, nTop);
			if(Sidebar[i].bDown) {
				if(Sidebar[i].bHover) {
					DrawBitmap(hdc, hbHeadDnSel, x + 172, nTop);
				} else {
					DrawBitmap(hdc, hbHeadDn, x + 172, nTop);
				}
			} else {
				if(Sidebar[i].bHover) {
					DrawBitmap(hdc, hbHeadUpSel, x + 172, nTop);
				} else {
					DrawBitmap(hdc, hbHeadUp, x + 172, nTop);
				}
			}
			Sidebar[i].rcCalc.right = 185 + (Sidebar[i].rcCalc.left = x + 10);
			Sidebar[i].rcCalc.bottom = 25 + (Sidebar[i].rcCalc.top = nTop);
			// Determine text area for header
			ZeroMemory(&rc, sizeof(rc));
			rc.left = x + 15;
			rc.right = x + 205;
			rc.top = y + nTop;
			rc.bottom = y + nTop + 25;
			// Draw Header text
			SetBkMode(hdc, TRANSPARENT);
			SelectObject(hdc, hfTitle);
			if(Sidebar[i].bHover) {
				SetTextColor(hdc, BCOLOR_LIGHT);
			} else {
				SetTextColor(hdc, BCOLOR_MED);
			}
			DrawText(hdc, Sidebar[i].szTitle, strlen(Sidebar[i].szTitle), &rc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
			nTop += 25;

			// Draw Body
			ZeroMemory(&rc, sizeof(rc));
			DrawText(hdc, Sidebar[i].szCaption, strlen(Sidebar[i].szCaption), &rc, DT_CALCRECT);
			if(rc.bottom) rc.bottom += 20;

			nHeight = (int)((float)rc.bottom * ((float)Sidebar[i].nStep / 100.0f));
			nAlpha = (int)(255.0f * ((float)Sidebar[i].nStep / 100.0f));
			hdcMem = CreateCompatibleDC(hdc);
			hbBuffer = CreateCompatibleBitmap(hdc, 185, nHeight);
			SelectObject(hdcMem, hbBuffer);
			SelectObject(hdcMem, hpBlank);
			Rectangle(hdcMem, 0, 0, 186, nHeight + 1);
			rc.left = 15;
			rc.top = 10 + nHeight - rc.bottom;
			rc.right = 170;
			rc.bottom = nHeight + 15;
			SelectObject(hdcMem, hfBody);
			DrawText(hdcMem, Sidebar[i].szCaption, strlen(Sidebar[i].szCaption), &rc, DT_LEFT);
			DeleteDC(hdcMem);
			SimpleBlend(hdc, hbBuffer, x + 10, y + nTop, nAlpha);
			
			nTop += nHeight;
	
			DeleteObject(hbBuffer);
		}
	}
	DeleteObject(hfTitle);
	DeleteObject(hfBody);
	DeleteObject(hpBlank);
}
//-----------------------------------------------------------------------------
// Name: UpdatePlayerStatus()
// Desc: Updates the player sidebar text.
//-----------------------------------------------------------------------------
void UpdatePlayerStatus(int nIndex)
{
	char szBuf[1024];
	sprintf_s(szBuf, "Total Score: %d\r\nPieces Left: %d", lScore, nPieceCount);
	ModifySidebarItem(nIndex, "Player Status", szBuf);
}
//-----------------------------------------------------------------------------
// Name: UpdateScoreStatus()
// Desc: Updates the high score sidebar text.
//-----------------------------------------------------------------------------
void UpdateScoreStatus(int nIndex)
{
	char szBuf[2048];
	char szScore[1024];
	int i;
	szBuf[0] = 0;

	for(i = 0; i < MAXHIGHSCORES; i++) {
		szScore[0] = 0;
		_ltoa(hsList[i].lScore, (char *)szScore, 10);
		strcat(szBuf, szScore);
		strcat(szBuf, " - ");
		strcat(szBuf, hsList[i].sName);
		if(i < MAXHIGHSCORES - 1) {
			strcat(szBuf, "\r\n");
		}
	}

	ModifySidebarItem(nIndex, "High Scores", szBuf);
}
//-----------------------------------------------------------------------------
// Name: UpdateSidebarItems()
// Desc: Updates the sidebar items.
//-----------------------------------------------------------------------------
void UpdateSidebarItems(void)
{
	int i;

	for(i = 0; i < MAXSIDEBARITEMS; i++) {
		if(Sidebar[i].bDoesExist) {
			if(Sidebar[i].bDown) {
				if(Sidebar[i].nStep < 100) Sidebar[i].nStep+=10;
			} else {
				if(Sidebar[i].nStep > 0) Sidebar[i].nStep-=10;
			}
		}
	}
}
//-----------------------------------------------------------------------------
// Name: GetHeaderByPixel()
// Desc: Collision detection with sidebar headers.
//-----------------------------------------------------------------------------
int GetHeaderByPixel(int x, int y)
{
	int i;

	for(i = 0; i < MAXSIDEBARITEMS; i++) {
		if(Sidebar[i].bDoesExist) {
			if(x >= Sidebar[i].rcCalc.left && x <= Sidebar[i].rcCalc.right &&
			   y >= Sidebar[i].rcCalc.top  && y <= Sidebar[i].rcCalc.bottom)
			   return i;
		}
	}

	return -1;
}
//-----------------------------------------------------------------------------
// Name: AddSidebarItem()
// Desc: Add a sidebar item to the list.
//-----------------------------------------------------------------------------
int AddSidebarItem(char *szTitle, char *szCaption)
{
	int i;

	for(i = 0; i < MAXSIDEBARITEMS; i++) {
		if(!Sidebar[i].bDoesExist) {
			Sidebar[i].bDoesExist = true;
			Sidebar[i].bVisible = true;
			Sidebar[i].bDown = true;
			Sidebar[i].bHover = false;
			Sidebar[i].nStep = 0;
			strcpy(Sidebar[i].szTitle, szTitle);
			strcpy(Sidebar[i].szCaption, szCaption);
			return i;
		}
	}

	return -1;
}
//-----------------------------------------------------------------------------
// Name: ModifySidebarItem()
// Desc: Modify the specified sidebar item.
//-----------------------------------------------------------------------------
void ModifySidebarItem(int nIndex, char *szNewTitle, char *szNewCaption)
{
	strcpy(Sidebar[nIndex].szTitle, szNewTitle);
	strcpy(Sidebar[nIndex].szCaption, szNewCaption);
}
//-----------------------------------------------------------------------------
// Name: HideSidebarItem()
// Desc: Hides the specified sidebar item.
//-----------------------------------------------------------------------------
void HideSidebarItem(int nIndex)
{
	Sidebar[nIndex].bVisible = false;
}
//-----------------------------------------------------------------------------
// Name: ShowSidebarItem()
// Desc: Shows the specified sidebar item.
//-----------------------------------------------------------------------------
void ShowSidebarItem(int nIndex)
{
	Sidebar[nIndex].bVisible = true;
}
//-----------------------------------------------------------------------------
// Name: ShowSidebarItem()
// Desc: Shows the specified sidebar item.
//-----------------------------------------------------------------------------
void ToggleSidebarVisible(int nIndex)
{
	Sidebar[nIndex].bVisible = !Sidebar[nIndex].bVisible;
}
//-----------------------------------------------------------------------------
// Name: CheckForMoves()
// Desc: Checks for moves in the grid.
//-----------------------------------------------------------------------------
bool CheckForMoves(void)
{
	int x, y;
	int n;

	for(y = 0; y < GRIDWIDTH; y++) {
		for(x = 0; x < GRIDWIDTH; x++) {
			n = brGrid[x][y].color;
			if(brGrid[x][y].bDoesExist && (IsRight(x, y, n) || IsBottom(x, y, n))) {
				return true;
			}
		}
	}

	return false;
}
//-----------------------------------------------------------------------------
// Name: GetBrickCount()
// Desc: Returns the number of bricks that exist in the grid.
//-----------------------------------------------------------------------------
int GetBrickCount(void)
{
	int x, y;
	int nIndex = 0;

	for(y = 0; y < GRIDWIDTH; y++) {
		for(x = 0; x < GRIDWIDTH; x++) {
			if(brGrid[x][y].bDoesExist) nIndex++;
		}
	}

	return nIndex;
}
//-----------------------------------------------------------------------------
// Name: HighlightArea()
// Desc: Highlights/Un-highlights the specified brick and surrounding area.
//-----------------------------------------------------------------------------
void HighlightArea(int x, int y)
{
	int i;

	CreateQueue(x, y);

	for(i = 0; i < GRIDWIDTH * GRIDHEIGHT; i++) {
		if(bqList[i].bDoesExist) {
			brGrid[bqList[i].x][bqList[i].y].bHighlight = true;
		} else {
			return;
		}
	}
}
//-----------------------------------------------------------------------------
// Name: CheckListItem()
// Desc: Checks if specified brick is already in the queue.
//-----------------------------------------------------------------------------
int CheckListItem(int x, int y)
{
	int i;

	for(i = 0; i < GRIDWIDTH * GRIDHEIGHT; i++) {
		if(bqList[i].bDoesExist && bqList[i].x == x && bqList[i].y == y) {
			return i;
		}
	}
	return -1;
}
//-----------------------------------------------------------------------------
// Name: AddListItem()
// Desc: Adds an item to the queue
//-----------------------------------------------------------------------------
bool AddListItem(int x, int y)
{
	int i;

	if(CheckListItem(x, y) == -1) {
		for(i = 0; i < GRIDWIDTH * GRIDHEIGHT; i++) {
			if(!bqList[i].bDoesExist) {
				bqList[i].x = x;
				bqList[i].y = y;
				bqList[i].bDoesExist = true;
				bqList[i].bDone = false;
				return true;
			}
		}
	}
	return false;
}
//-----------------------------------------------------------------------------
// Name: GetFirstItem()
// Desc: Gets the first item in the list
//-----------------------------------------------------------------------------
int GetFirstItem(void)
{
	int i;

	for(i = 0; i < GRIDWIDTH * GRIDHEIGHT; i++) {
		if(bqList[i].bDoesExist && !bqList[i].bDone) {
			return i;
		}
	}

	return -1;
}
//-----------------------------------------------------------------------------
// Name: CreateQueue()
// Desc: Creates a list of surrounding bricks of the same color
//-----------------------------------------------------------------------------
void CreateQueue(int x, int y)
{
	int nColor;
	int i;
	bool bDone = false;
	POINT p;

	// Out of bounds
	if(!IsInBounds(x, y)) return;

	// Clear list
	ZeroMemory(bqList, sizeof(bqList));

	nColor = brGrid[x][y].color;

	AddListItem(x, y);

	p.x = x;
	p.y = y;

	while(!bDone) {
		if(IsTop(p.x, p.y, brGrid[p.x][p.y].color)) {
			AddListItem(p.x, p.y - 1);
		}
		if(IsBottom(p.x, p.y, brGrid[p.x][p.y].color)) {
			AddListItem(p.x, p.y + 1);
		}
		if(IsLeft(p.x, p.y, brGrid[p.x][p.y].color)) {
			AddListItem(p.x - 1, p.y);
		}
		if(IsRight(p.x, p.y, brGrid[p.x][p.y].color)) {
			AddListItem(p.x + 1, p.y);
		}
		i = GetFirstItem();
		if(i != -1) {
			p.x = bqList[i].x;
			p.y = bqList[i].y;
			bqList[i].bDone = true;
		} else {
			return;
		}
	}
}
//-----------------------------------------------------------------------------
// Name: CollapseGrid()
// Desc: Makes the bricks fall into place.
//-----------------------------------------------------------------------------
void CollapseGrid(void)
{
	int x, y;
	int nCount;

	for(nCount = 0; nCount < GRIDHEIGHT; nCount++) {
		for(y = 0; y < GRIDHEIGHT - 1; y++) {
			for(x = 0; x < GRIDWIDTH; x++) {
				if(brGrid[x][y].bDoesExist && !brGrid[x][y + 1].bDoesExist) {
					brGrid[x][y + 1].color		= brGrid[x][y].color;
					brGrid[x][y + 1].bDoesExist = true;
					brGrid[x][y + 1].y		   += -TILEWIDTH;
					brGrid[x][y].bDoesExist		= false;
				}
			}
		}
	}

	for(nCount = 0; nCount < GRIDWIDTH; nCount++) {
		for(x = 0; x < GRIDWIDTH - 1; x++) {
			if(brGrid[x][GRIDHEIGHT - 1].bDoesExist == FALSE && brGrid[x + 1][GRIDHEIGHT - 1].bDoesExist == TRUE) {
				memcpy(brGrid[x], brGrid[x + 1], sizeof(brGrid[x]));
				for(y = 0; y < GRIDHEIGHT; y++) {
					brGrid[x + 1][y].bDoesExist = false;
					brGrid[x][y].x += 22;
				}
			}
		}
	}
}
//-----------------------------------------------------------------------------
// Name: SetGridHighlight()
// Desc: Highlights/Un-highlights the entire grid.
//-----------------------------------------------------------------------------
void SetGridHighlight(bool highlight)
{
	int x, y;

	for(y = 0; y < GRIDHEIGHT; y++)
		for(x = 0; x < GRIDWIDTH; x++)
			brGrid[x][y].bHighlight = highlight;
}
//-----------------------------------------------------------------------------
// Name: ClearBrickOffset()
// Desc: Moves the bricks back into place.
//-----------------------------------------------------------------------------
void ClearBrickOffset(void)
{
	int x, y;

	for(y = 0; y < GRIDHEIGHT; y++) {
		for(x = 0; x < GRIDWIDTH; x++) {
			brGrid[x][y].x = 0;
			brGrid[x][y].y = 0;
		}
	}
}
//-----------------------------------------------------------------------------
// Name: FindBrick()
// Desc: Finds an existing brick at pixel coordinates (x, y).
//-----------------------------------------------------------------------------
void FindBrick(int x, int y, POINTS *p)
{
	int xp, yp;
	int vx, vy;

	for(yp = GRIDHEIGHT - 1; yp >= 0; yp--) {
		for(xp = GRIDWIDTH - 1; xp >= 0; xp--) {
			if(brGrid[xp][yp].bDoesExist) {
				vx = (xp * TILEWIDTH)  + TILEWIDTH  + (int)brGrid[xp][yp].x;
				vy = (yp * TILEHEIGHT) + TILEHEIGHT + (int)brGrid[xp][yp].y;
				if(x >= vx && x < vx + TILEWIDTH && y >= vy && y < vy + TILEHEIGHT) {
					p->x = xp;
					p->y = yp;
					return;
				}
			}
		}
	}
	// Nothing found at (x,y)
	p->x = -1;
	p->y = -1;
	return;
}
//-----------------------------------------------------------------------------
// Name: BrickEffect()
// Desc: Moves the bricks around the exploding brick.
//-----------------------------------------------------------------------------
void BrickEffect(int x, int y)
{

	if(IsInBounds(x - 1, y - 1)) {				// top left
		brGrid[x-1][y-1].x -= EFFECT_MAG / 2;
		brGrid[x-1][y-1].y -= EFFECT_MAG / 2;
	}

	if(IsInBounds(x, y - 1)) {					// top
		brGrid[x][y-1].y -= EFFECT_MAG;
	}

	if(IsInBounds(x + 1, y - 1)) {				// top right
		brGrid[x+1][y-1].x += EFFECT_MAG / 2;
		brGrid[x+1][y-1].y -= EFFECT_MAG / 2;
	}

	if(IsInBounds(x-1, y)) {					// mid left
		brGrid[x-1][y].x -= EFFECT_MAG;
	}

	if(IsInBounds(x+1, y)) {					// mid right
		brGrid[x+1][y].x += EFFECT_MAG;
	}

	if(IsInBounds(x-1, y+1)) {					// bot left
		brGrid[x-1][y+1].x -= EFFECT_MAG / 2;
		brGrid[x-1][y+1].y += EFFECT_MAG / 2;
	}

	if(IsInBounds(x, y+1)) {					// bot
		brGrid[x][y+1].y += EFFECT_MAG;
	}

	if(IsInBounds(x+1, y+1)) {					// bot right
		brGrid[x+1][y+1].x += EFFECT_MAG / 2;
		brGrid[x+1][y+1].y += EFFECT_MAG / 2;
	}
}
//-----------------------------------------------------------------------------
// Name: DrawGrid()
// Desc: Draws the grid & shadows.
//-----------------------------------------------------------------------------
void DrawGrid(HDC hDC)
{	
	int x, y;
	int nAlpha;

	if(Settings.bShadow) {
		for(y = 0; y < GRIDHEIGHT; y++) {
			for(x = 0; x < GRIDWIDTH; x++) {
				if(brGrid[x][y].bDoesExist) {
					DrawOverlay(hDC, hShadow, (x * TILEWIDTH) + TILEWIDTH + (int)brGrid[x][y].x, (y * TILEHEIGHT) + TILEHEIGHT + (int)brGrid[x][y].y);
				}
			}
		}
	}
	
	for(y = 0; y < GRIDHEIGHT; y++) {
		for(x = 0; x < GRIDWIDTH; x++) {
			if(brGrid[x][y].bDoesExist) {
				//if(Settings.bShadow) DrawOverlay(hDC, hShadow, (x * TILEWIDTH) + TILEWIDTH + (int)brGrid[x][y].x, (y * TILEHEIGHT) + TILEHEIGHT + (int)brGrid[x][y].y);
				DrawBitmap(hDC, hBrick[brGrid[x][y].color], (x * TILEWIDTH) + 22 + (int)brGrid[x][y].x, (y * TILEWIDTH) + 22 + (int)brGrid[x][y].y);
				if(brGrid[x][y].bHighlight) {
					nAlpha = (int)(50.0f * sin(brGrid[x][y].nStep * 3.14f/180) + 80.0f);
					SimpleBlend(hDC, hbHighlight,
						(x * TILEWIDTH) + TILEWIDTH + (int)brGrid[x][y].x,
						(y * TILEHEIGHT) + TILEHEIGHT + (int)brGrid[x][y].y, nAlpha);
				}
			}
		}
	}
}
//-----------------------------------------------------------------------------
// Name: CanRemove()
// Desc: Checks all sides of a brick to see if there are more pieces around it.
//-----------------------------------------------------------------------------
bool CanRemove(int x, int y)
{
	int n;
	
	n = brGrid[x][y].color;

	if(IsTop(x, y, n) || IsBottom(x, y, n) || IsLeft(x, y, n) || IsRight(x, y, n))
		return true;

	return false;
}
//-----------------------------------------------------------------------------
// Name: IsTop()
// Desc: Checks above specified coordinate for a brick of the specified color.
//-----------------------------------------------------------------------------
bool IsTop(int x, int y, int nColor)
{
	if(IsInBounds(x, y) && y > 0) {
		if(brGrid[x][y - 1].color == nColor && brGrid[x][y - 1].bDoesExist) {
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Name: IsBottom()
// Desc: Checks below specified coordinate for a brick of the specified color.
//-----------------------------------------------------------------------------
bool IsBottom(int x, int y, int nColor)
{
	if(IsInBounds(x, y) && y < GRIDHEIGHT - 1) {
		if(brGrid[x][y + 1].color == nColor && brGrid[x][y + 1].bDoesExist) {
			return true;
		}
	}

	return false;
}
//-----------------------------------------------------------------------------
// Name: IsRight()
// Desc: Checks to the right of specified coordinate for a brick of the specified color.
//-----------------------------------------------------------------------------
bool IsRight(int x, int y, int nColor)
{
	if(IsInBounds(x, y) && x < GRIDWIDTH - 1) {
		if(brGrid[x + 1][y].color == nColor && brGrid[x + 1][y].bDoesExist) {
			return true;
		}
	}

	return false;
}
//-----------------------------------------------------------------------------
// Name: IsLeft()
// Desc: Checks to the left of specified coordinate for a brick of the specified color.
//-----------------------------------------------------------------------------
bool IsLeft(int x, int y, int nColor)
{
	if(IsInBounds(x, y) && x > 0) {
		if(brGrid[x - 1][y].color == nColor && brGrid[x - 1][y].bDoesExist) {
			return true;
		}
	}

	return false;
}
//-----------------------------------------------------------------------------
// Name: IsInBounds()
// Desc: Checks to see if the brick specified is within the grid bounds
//-----------------------------------------------------------------------------
bool IsInBounds(int x, int y)
{
	if(x >= 0 && y >= 0 && x < GRIDWIDTH && y < GRIDHEIGHT) {
		return true;
	} else {
		return false;
	}
}
//-----------------------------------------------------------------------------
// Name: InitGrid()
// Desc: Initializes the Bricks grid.
//-----------------------------------------------------------------------------
void InitGrid(int nColors)
{
	int x, y;

	for(y = 0; y < GRIDWIDTH; y++) {
		for(x = 0; x < GRIDHEIGHT; x++) {
			brGrid[x][y].bDoesExist = true;
			brGrid[x][y].color		= rand()%nColors;
			brGrid[x][y].bHighlight	= false;
			brGrid[x][y].nStep		= 0;
		}
	}
	GridEffect();
	lScore = 0;
	nPieceCount = GetBrickCount();
	bGameOver = false;
	bCanUndo = false;
	UpdatePlayerStatus(nPlayerBar);
}
//-----------------------------------------------------------------------------
// Name: GridEffect()
// Desc: Initializes the brick fly-in effect.
//-----------------------------------------------------------------------------
void GridEffect(void)
{
	int x, y;
	int nInitEffect;

	nInitEffect = rand()%7;

	for(y = 0; y < GRIDWIDTH; y++) {
		for(x = 0; x < GRIDHEIGHT; x++) {
			switch(nInitEffect) {
				case 0:
					brGrid[x][y].x	= 0;
					brGrid[x][y].y	= -640.0f + ((float)y * TILEHEIGHT) * (float)x;
					break;
				case 1:
					brGrid[x][y].x	= -640.0f + ((float)y * TILEHEIGHT) * (float)x;
					brGrid[x][y].y	= 0;
					break;
				case 2:
					brGrid[x][y].x	= -TILEWIDTH * (float)x * (float)y;
					brGrid[x][y].y	= TILEHEIGHT * (float)y * (float)y;
					break;
				case 3:
					brGrid[x][y].x	= TILEWIDTH * (float)x * (float)y;
					brGrid[x][y].y	= TILEHEIGHT * (float)y * (float)y;
					break;
				case 4:
					brGrid[x][y].x	= (float)x*TILEWIDTH + 640 * x;
					brGrid[x][y].y	= (float)x*TILEWIDTH + 640 * x;
					break;
				case 5:
					brGrid[x][y].x	= 0;
					brGrid[x][y].y	= (float)x*TILEWIDTH + 640 * x * x / 2;
					break;
				case 6:
					brGrid[x][y].x	= -640 + ((float)y * TILEHEIGHT) * x;
					brGrid[x][y].y	= -640 + ((float)y * TILEHEIGHT) * x;
					break;
			}
		}
	}
}
//-----------------------------------------------------------------------------
// Name: ResetSettings()
// Desc: Resets settings to their original values.
//-----------------------------------------------------------------------------
void ResetSettings(void)
{
	strcpy(Settings.szName, "Anonymous");
	Settings.bSound = true;
	Settings.bSplash = true;
	Settings.bShadow = true;
	Settings.bAutoSave = true;
}
//-----------------------------------------------------------------------------
// Name: InsertScore()
// Desc: Inserts a score into the high score list
//-----------------------------------------------------------------------------
bool InsertScore(char *szName, long lScore)
{
	int i;
	int n;

	for(i = 0; i < MAXHIGHSCORES; i++) {
		if(lScore > hsList[i].lScore) {
			// Shift the high scores down
			for(n = MAXHIGHSCORES - 1; n > i; n--) {
				memcpy(&hsList[n], &hsList[n - 1], sizeof(hsList[0]));
			}
			hsList[i].lScore = lScore;
			strcpy(hsList[i].sName, szName);
			return true;
		}
	}
    
	return false;
}
//-----------------------------------------------------------------------------
// Name: IsHighScore()
// Desc: Returns true if the score is in the top ten list, false if not.
//-----------------------------------------------------------------------------
bool IsHighScore(long lScore)
{
	if(lScore > hsList[MAXHIGHSCORES - 1].lScore) return true;
	else return false;
}
//-----------------------------------------------------------------------------
// Name: LoadScores()
// Desc: Load High Scores from File.
//-----------------------------------------------------------------------------
void LoadScores(void)
{
	FILE *fScr;

	fScr = fopen(PATH_SCORES, "rb");
	if(fScr == NULL || fScr == 0) {
		ClearScores();
	} else {
		fread(&hsList, sizeof(hsList), 1, fScr);
		fclose(fScr);
	}
}
//-----------------------------------------------------------------------------
// Name: SaveScores()
// Desc: Save High Scores to File.
//-----------------------------------------------------------------------------
void SaveScores(void)
{
	FILE *fScr;

	fScr = fopen(PATH_SCORES, "wb");
	if(fScr == NULL || fScr == 0) {
		// Do nothing
	} else {
		fwrite(&hsList, sizeof(hsList), 1, fScr);
		fclose(fScr);
	}
}
//-----------------------------------------------------------------------------
// Name: ClearScores()
// Desc: Clear high score list.
//-----------------------------------------------------------------------------
void ClearScores(void)
{
	int i;
	for(i = 0; i < MAXHIGHSCORES; i++) {
		switch(i) {
			case 0:
				strcpy(hsList[i].sName, "Trent");
				break;
			case 1:
				strcpy(hsList[i].sName, "Bro");
				break;
			case 2:
				strcpy(hsList[i].sName, "Sr");
				break;
			case 3:
				strcpy(hsList[i].sName, "Laura");
				break;
			case 4:
				strcpy(hsList[i].sName, "Finn");
				break;
			case 5:
				strcpy(hsList[i].sName, "Mark");
				break;
			case 6:
				strcpy(hsList[i].sName, "Eve");
				break;
			case 7:
				strcpy(hsList[i].sName, "Jake");
				break;
			case 8:
				strcpy(hsList[i].sName, "Jo");
				break;
			case 9:
				strcpy(hsList[i].sName, "Bitcoin");
				break;
		}
		
		hsList[i].lScore = (MAXHIGHSCORES / (i + 1)) * (1500 / MAXHIGHSCORES);
	}
}
//-----------------------------------------------------------------------------
// Name: LoadSettings()
// Desc: Loads the settings from file.
//-----------------------------------------------------------------------------
void LoadSettings(void)
{
	FILE *fSet;

	fSet = fopen(PATH_SETTINGS, "rb");
	if(fSet == NULL || fSet == 0) {
		ResetSettings();
	} else {
		fread(&Settings, sizeof(Settings), 1, fSet);
		fclose(fSet);
	}
}
//-----------------------------------------------------------------------------
// Name: SaveSettings()
// Desc: Saves the settings to file.
//-----------------------------------------------------------------------------
void SaveSettings(void)
{
	FILE *fSet;

	fSet = fopen(PATH_SETTINGS, "wb");
	if(fSet == NULL || fSet == 0) {
		// Do nothing :/
	} else {
		fwrite(&Settings, sizeof(Settings), 1, fSet);
		fclose(fSet);
	}
}
//-----------------------------------------------------------------------------
// Name: SaveGame()
// Desc: Saves the game to be restored later.
//-----------------------------------------------------------------------------
void SaveGame(char *szFile)
{
	FILE *fGame;

	fGame = fopen(szFile, "wb");
	if(fGame == NULL || fGame == 0) {
		// Do nothing
	} else {
		fwrite(&bGameOver, sizeof(bGameOver), 1, fGame);
		fwrite(&lScore,    sizeof(lScore),    1, fGame);
		fwrite(&brGrid,    sizeof(brGrid),    1, fGame);
		fclose(fGame);
	}
}
//-----------------------------------------------------------------------------
// Name: LoadGame()
// Desc: Restores a saved game.
//-----------------------------------------------------------------------------
void LoadGame(char *szFile)
{
	FILE *fGame;

	fGame = fopen(szFile, "rb");
	if(fGame == NULL || fGame == 0) {
		// Do nothing
	} else {
		fread(&bGameOver, sizeof(bGameOver), 1, fGame);
		fread(&lScore,    sizeof(lScore),    1, fGame);
		fread(&brGrid,    sizeof(brGrid),    1, fGame);
		fclose(fGame);
	}
}
//-----------------------------------------------------------------------------
// Name: VisitOurSite()
// Desc: Open the browser to my website.
//-----------------------------------------------------------------------------
void VisitOurSite(HWND hWnd)
{
	if(FAILED(ShellExecute(hWnd, _T("open"), _T("http://www.markhamilton.info"), '\0', '\0', SW_SHOW))) {
		MessageBox(hWnd, _T("Could not open web browser!\r\n\r\nPlease visit http://www.markhamilton.info"), _T("Error"), MB_ICONERROR);
	}
}
//-----------------------------------------------------------------------------
// Name: OpenDoc()
// Desc: Opens specified document.
//-----------------------------------------------------------------------------
void OpenDoc(HWND hWnd, char *strFileName)
{
	
	if(FAILED(ShellExecute(hWnd, "open", strFileName, '\0', '\0', SW_SHOW))) {
		MessageBox(hWnd, _T("Could not open text document!"), _T("Error"), MB_ICONERROR);
	}
}