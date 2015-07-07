#ifndef IDC_HAND
#define IDC_HAND MAKEINTRESOURCE(32649)
#endif
// Theme colors
#define BCOLOR_DARK		0x670306 // High Score Header
#define BCOLOR_MED		0x94413E // High Score Sidebar Background
#define BCOLOR_LIGHT	0xCDB7BA // Main background, High score backgrounds
typedef struct Brick {
	bool bDoesExist;
	bool bHighlight;
	int nStep;
	float x;
	float y;
	int color;
} t_Brick;
typedef struct BrickQueue {
	int x;
	int y;
	bool bDoesExist;
	bool bDone;
} t_BrickQueue;
typedef struct HighScore {
	long lScore;
	char sName[65];
} t_HighScore;
typedef struct SidebarItem {
	char szTitle[1024];
	char szCaption[1024];
	RECT rcCalc;
	bool bVisible;
	bool bDoesExist;
	bool bHover;
	bool bDown;
	int nStep;
} t_SidebarItem;

// Transparent Window Crap
HMODULE hUser32 = GetModuleHandle("USER32.DLL");
typedef BOOL (WINAPI *lpfnSetLayeredWindowAttributes)(HWND hWnd, COLORREF crKey, BYTE bAlpha, DWORD dwFlags);
lpfnSetLayeredWindowAttributes m_pSetLayeredWindowAttributes;
#define WS_EX_LAYERED           0x00080000
#define LWA_COLORKEY            0x00000001
#define LWA_ALPHA               0x00000002