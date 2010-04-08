/*
**
** File: siwinpr.h
**
** Description:
**     A temporary workaround for the fact that NT doesn't support an
**     ANSII terminal driver.
*/

#define ID_EDIT		100
#define WTITLE		"Ingres History"
#define WCLASS		"IITRACE"
#define WM_IIPRINTF	WM_USER+123
#define WM_IIGETPROMPT	WM_USER+124

void IIprintf(char *);
void TbufAlloc(void);
void TbufSave(char *str);
void TbufAdjust(void);
void TbufMark(void);
void TbufUpdate(void);
