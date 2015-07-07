#define MESSAGES_H

#include <windows.h>
#include <string.h>
#include <stdio.h>
#include "messages.h"

t_Message mqList[MESSAGENUM];

//-----------------------------------------------------------------------------
// Name: AddMessage()
// Desc: Adds a message to the message queue.
//-----------------------------------------------------------------------------
int AddMessage(int nScore, int x, int y)
{
	int i;

	for(i = 0; i < MESSAGENUM; i++) {
		if(!mqList[i].bDoesExist) {
			mqList[i].bDoesExist = true;
			mqList[i].bBonus = false;
			mqList[i].nScore = nScore;
			mqList[i].nLife = 20;
			mqList[i].nStep = 0;
			mqList[i].x = x;
			mqList[i].y = y;
			mqList[i].xspeed = 0;
			mqList[i].yspeed = -1;
			return i;
		}
	}
	return -1;
}
//-----------------------------------------------------------------------------
// Name: ClearMessages()
// Desc: Clears the message queue (done at initialization).
//-----------------------------------------------------------------------------
void ClearMessages(void)
{
	ZeroMemory(mqList, sizeof(mqList));
}
//-----------------------------------------------------------------------------
// Name: UpdateMessages()
// Desc: Handles the messages every frame (updating, translating, et cetera.)
//-----------------------------------------------------------------------------
void UpdateMessages(void)
{
	int i;

	for(i = 0; i < MESSAGENUM; i++) {
		mqList[i].nStep++;
		if(mqList[i].nStep > mqList[i].nLife) mqList[i].bDoesExist = false;
		mqList[i].x += mqList[i].xspeed;
		mqList[i].y += mqList[i].yspeed;
	}
}
//-----------------------------------------------------------------------------
// Name: MakeMessageBonus()
// Desc: Makes a message a bonus message identifier (bigger font, different colors, etc.)
//-----------------------------------------------------------------------------
void MakeMessageBonus(int nIndex)
{
	if(nIndex >= 0 && nIndex < MESSAGENUM) {
		mqList[nIndex].bBonus = true;
		mqList[nIndex].nLife = 70;
	}
}
//-----------------------------------------------------------------------------
// Name: DrawMessages()
// Desc: Draws score messages.
//-----------------------------------------------------------------------------
void DrawMessages(HDC hdc)
{
	HFONT hfText;
	RECT  rc;
	char s[1024];
	float f;
	int i;
	int nFontSize;

	for(i = 0; i < MESSAGENUM; i++) {
		if(mqList[i].bDoesExist) {
			f = (float)mqList[i].nStep / (float)mqList[i].nLife;
			if(f > 1) f = 0;
			SetTextColor(hdc, RGB((int)((f)*150.0f), (int)((f)*150.0f), (int)(255)));
			SetBkColor(hdc, TRANSPARENT);
			
			if(mqList[i].bBonus) {
				nFontSize = 80;
			} else {
				nFontSize = 25;
			}
			hfText = CreateFont(nFontSize, 0, 0, 0, FW_BLACK, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS,
							CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, FF_DONTCARE, "Verdana");
			SelectObject(hdc, hfText);
			ZeroMemory(&rc, sizeof(rc));
			sprintf_s(s, "+%d", mqList[i].nScore);
			DrawText(hdc, s, strlen(s), &rc, DT_CALCRECT);
			TextOut(hdc, mqList[i].x - (rc.right / 2), mqList[i].y, s, strlen(s));
			DeleteObject(hfText);
		}
	}
}