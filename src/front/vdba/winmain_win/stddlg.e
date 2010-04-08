/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**
**  Name: stddlg.e
**
**  Description:
**	Originaly taken from VO, adapted by Emb
**
**  History:
**	18-feb-2000 (somsa01)
**	    Removed redefine of LPCOLORREF.
*/

typedef struct _FONTINFO
{
	CHAR    achFaceName[LF_FACESIZE];
	int     iPoints;					// MS0001 - change from int to float
										// MS0002 - change back to int
	int     iWeight;
	BYTE    bPitch;
	BYTE    bCharSet;
	HFONT   hFont;
	BOOL    bItalics;

  //JFS003  - add somr usefull informations ...
  int     lfEscapement;
  BYTE    lfOutPrecision;
  BYTE    lfClipPrecision;
  BYTE    lfQuality;
}       FONTINFO, FAR * LPFONTINFO;

#define FONT_NAME(p)            ((p)->achFaceName)
#define FONT_POINTS(p)          ((p)->iPoints)
#define FONT_WEIGHT(p)          ((p)->iWeight)
#define FONT_PITCH(p)           ((p)->bPitch)
#define FONT_CHARSET(p)         ((p)->bCharSet)
#define FONT_HFONT(p)           ((p)->hFont)

BOOL	FAR	   GetSaveName(HWND hwnd, PSZ pszFileName, LPCHAR lpchDefExt[], USHORT usEntries);
//420@WM005 add oem parm
BOOL	FAR	   GetOpenName(HWND hwnd, PSZ pszFileName, LPCHAR lpchDefExt[],
			       USHORT usUseDir, PSZ pszTitle, LPINT lpiOEM2ANSI);  //377@WM006 add title parm
BOOL    FAR        SelectColor(HWND hwnd, LPCOLORREF lpColor, LPSTR lpstrTitle);
BOOL    FAR        StdDlgSetInitDir(PSZ pszInitDir, PSZ pszVODir);
BOOL    EXPORT                  InputDlg(HWND hwnd, WORD msg, WORD wParam, LONG lParam);
VOID    FAR                             CreateLogFont(LPFONTINFO lpFontInfo, LPLOGFONT lpLogFont, HDC hDCGiven);
VOID    FAR                             CopyLogFontToFontInfo(LPLOGFONT lpLogFont, LPFONTINFO lpFontInfo);
VOID    FAR                             StringToFontInfo(LPFONTINFO lpFontInfo, LPSTR lpstrFont);
VOID    FAR                     FontInfoToString(LPFONTINFO lpFontInfo, LPSTR lpstr);


typedef struct _INPUTDATA
{
	PSZ             pszTitle;
	PSZ             pszStore;
	BOOL            fInit;
	USHORT  usSize;
} INPUTDATA, FAR * PINPUTDATA;

#define INPUT_TITLE(p)  ((p)->pszTitle)
#define INPUT_STORE(p)  ((p)->pszStore)
#define INPUT_INIT(p)           ((p)->fInit)
#define INPUT_SIZE(p)           ((p)->usSize)

#define USE_PRG    1  /* use prg directory */  
#define USE_VO     2
