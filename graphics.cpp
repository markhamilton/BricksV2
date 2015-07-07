#include <windows.h>
#include "graphics.h"

//-----------------------------------------------------------------------------
// Name: MyCreateFont()
// Desc: Simplified version of CreateFont.
//-----------------------------------------------------------------------------
HFONT MyCreateFont(HDC hDC, char *szFontName, int PointSize)
{
	return CreateFont(PointSize, 0, 0, 0, 0, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS,
	                    CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, FF_DONTCARE, szFontName);
}

//-----------------------------------------------------------------------------
// Name: DrawShadowText()
// Desc: Draws shadowed text on the screen at specified coordinates
//-----------------------------------------------------------------------------
void DrawShadowText(HDC hDC, int x, int y, COLORREF crTextColor, COLORREF crShadowColor, char *text)
{
	HFONT hf;
	hf = MyCreateFont(hDC, "Impact", 40);

	SelectObject(hDC, hf);
    SetBkMode(hDC, TRANSPARENT);

    SetTextColor(hDC, crShadowColor);
	TextOut(hDC, x + 1, y + 1, text, strlen(text));
	SetTextColor(hDC, crTextColor);
	TextOut(hDC, x, y, text, strlen(text));

	DeleteObject(hf);
}

//-----------------------------------------------------------------------------
// Name: DrawBitmap()
// Desc: Draws a bitmap at specified location.
//-----------------------------------------------------------------------------
VOID DrawBitmap(HDC hdc, HBITMAP hBitmap, short xStart, short yStart)
{
	BITMAP	bm;
	HDC		hdcMem;
	POINT	ptSize, ptOrg;

	hdcMem = CreateCompatibleDC(hdc);
	SelectObject(hdcMem, hBitmap);
	SetMapMode(hdcMem, GetMapMode(hdc));

	GetObject(hBitmap, sizeof(BITMAP), (LPSTR)&bm);

	ptSize.x = bm.bmWidth;
	ptSize.y = bm.bmHeight;
	DPtoLP(hdc, &ptSize, 1);

	ptOrg.x = 0;
	ptOrg.y = 0;
	DPtoLP(hdcMem, &ptOrg, 1);

	BitBlt(hdc, xStart, yStart, ptSize.x, ptSize.y, hdcMem, ptOrg.x, ptOrg.y, SRCCOPY);

	DeleteDC(hdcMem);
}

//-----------------------------------------------------------------------------
// Name: DrawOverlay()
// Desc: Overlays a bitmap at specified coordinates by alphablending it
//-----------------------------------------------------------------------------
VOID DrawOverlay(HDC hdc, HBITMAP hBitmap, short xStart, short yStart)
{
	HDC		hdcMem;
	BITMAP	bm;
	BLENDFUNCTION bf;

	hdcMem = CreateCompatibleDC(hdc);
	GetObject(hBitmap, sizeof(BITMAP), &bm);

	bf.AlphaFormat = 0;
	bf.BlendFlags = 0;
	bf.BlendOp = AC_SRC_OVER;
	bf.SourceConstantAlpha = 100;

	SelectObject(hdcMem, hBitmap);

	AlphaBlend(hdc, xStart, yStart, bm.bmWidth, bm.bmHeight, hdcMem, 0, 0, bm.bmWidth, bm.bmHeight, bf);

	DeleteDC(hdcMem);
}

//-----------------------------------------------------------------------------
// Name: SimpleBlend()
// Desc: Alphablends a bitmap at specified coordinates with specified alpha level
//-----------------------------------------------------------------------------
VOID SimpleBlend(HDC hdc, HBITMAP hBitmap, short xStart, short yStart, int nAlpha)
{
	HDC		hdcMem;
	BITMAP	bm;
	BLENDFUNCTION bf;

	hdcMem = CreateCompatibleDC(hdc);
	GetObject(hBitmap, sizeof(BITMAP), &bm);

	bf.AlphaFormat = 0;
	bf.BlendFlags = 0;
	bf.BlendOp = AC_SRC_OVER;
	bf.SourceConstantAlpha = nAlpha;

	SelectObject(hdcMem, hBitmap);

	AlphaBlend(hdc, xStart, yStart, bm.bmWidth, bm.bmHeight, hdcMem, 0, 0, bm.bmWidth, bm.bmHeight, bf);

	DeleteDC(hdcMem);
}

//-----------------------------------------------------------------------------
// Name: TransparentCopy()
// Desc: Blits an image with a masked color
//-----------------------------------------------------------------------------
void TransparentCopy(HDC hDestDC, int iXDest, int iYDest, int iWidthDest, int iHeightDest, HDC hSrcDC, int iXSrc, int iYSrc, int iWidthSrc, int iHeightSrc, COLORREF crMaskColour)
{
	int oldsrcmode, olddestmode;
	COLORREF oldsrccolour, olddestcolour;
	oldsrcmode = SetBkMode(hSrcDC, OPAQUE);
	olddestmode = SetBkMode(hDestDC, OPAQUE);;
	oldsrccolour = SetBkColor(hSrcDC, crMaskColour);
	olddestcolour = SetBkColor(hDestDC, crMaskColour);


	HDC maskmem = CreateCompatibleDC(hDestDC);
	HBITMAP maskbmp = CreateCompatibleBitmap(hDestDC, iWidthDest, iHeightDest);
	HDC tempmem = CreateCompatibleDC(hDestDC);
	HBITMAP tempbmp = CreateBitmap(iWidthDest, iHeightDest, 1, 1, NULL);
	HBITMAP oldmaskbmp = (HBITMAP)SelectObject(maskmem, (HBITMAP)maskbmp);
	HBITMAP oldtempbmp = (HBITMAP)SelectObject(tempmem, (HBITMAP)tempbmp);
	StretchBlt(tempmem, 0,0, iWidthDest,iHeightDest, hSrcDC, iXSrc, iYSrc, iWidthSrc, iHeightSrc, SRCCOPY);
	BitBlt(maskmem, 0,0, iWidthDest,iHeightDest, tempmem, 0, 0,  SRCCOPY);
	StretchBlt(hDestDC, iXDest,iYDest, iWidthDest,iHeightDest, hSrcDC, iXSrc, iYSrc, iWidthSrc, iHeightSrc, SRCINVERT);
	BitBlt(hDestDC,iXDest,iYDest, iWidthDest,iHeightDest, maskmem,0,0, SRCAND);
	StretchBlt(hDestDC, iXDest,iYDest, iWidthDest,iHeightDest, hSrcDC, iXSrc, iYSrc, iWidthSrc, iHeightSrc-1, SRCINVERT);
	SelectObject(maskmem, (HBITMAP)oldmaskbmp);
	SelectObject(tempmem, (HBITMAP)oldtempbmp);
	SetBkMode(hSrcDC, oldsrcmode);
	SetBkMode(hDestDC, olddestmode);
	SetBkColor(hSrcDC, oldsrccolour);
	SetBkColor(hDestDC, olddestcolour);
	DeleteObject(oldmaskbmp);
	DeleteObject(oldtempbmp);
	DeleteObject(maskbmp);
	DeleteObject(tempbmp);
	DeleteDC(maskmem);
	DeleteDC(tempmem);
}
