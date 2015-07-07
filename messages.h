#define MESSAGENUM 100

typedef struct Message {
	bool bDoesExist;
	bool bBonus;
	int x;
	int y;
	int nScore;
	int xspeed;
	int yspeed;
	int nStep;
	int nLife;
} t_Message;

int  AddMessage(int nScore, int x, int y);
void DrawMessages(HDC hdc);
void ClearMessages(void);
void UpdateMessages(void);
void MakeMessageBonus(int nIndex);