#define HEIGHTFROMPOINTS(PointSize) -MulDiv(PointSize, GetDeviceCaps(hDC, LOGPIXELSY), 72)


VOID  DrawBitmap      (HDC hdc, HBITMAP hBitmap, short xStart, short yStart);
VOID  DrawOverlay     (HDC hdc, HBITMAP hBitmap, short xStart, short yStart);
VOID  SimpleBlend     (HDC hdc, HBITMAP hBitmap, short xStart, short yStart, int nAlpha);
HFONT MyCreateFont    (HDC, char *, int);
void  TransparentCopy (HDC hDestDC, int iXDest, int iYDest, int iWidthDest, int iHeightDest, HDC hSrcDC, int iXSrc, int iYSrc, int iWidthSrc, int iHeightSrc, COLORREF crMaskColour);
void  DrawShadowText(HDC hDC, int x, int y, COLORREF crTextColor, COLORREF crShadowColor, char *text);