/********************************************************************
//
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
//
//    Project  : CA/OpenIngres Visual DBA
//
//    Source   : subedit.h
//
//    
//
********************************************************************/

#ifndef SUBEDIT_H
#define SUBEDIT_H

#define CSC_NUMERIC		0x0001
#define CSC_ALPHA			0x0002
#define CSC_NATIONAL		0x0004
#define CSC_SPACE			0x0008
#define CSC_QUOTES			0x0010
#define CSC_ALPHANUMERIC	CSC_NUMERIC | CSC_ALPHA
#define CSC_UNDERSCORE	    0x0020
#define CSC_KEYUP			0x0040
#define CSC_SLASH			0x0080  // Vut adds
#define CSC_COLON			0x0100  // Vut adds
#define CSC_BACKSLASH      0x0200  // Vut adds

#define SUBEDITPROP	"SEP"

typedef struct _SUBCLASSINFO
{
	WORD	wType;
	WNDPROC lpFn;
} SUBCLASSINFO, FAR * LPSUBCLASSINFO;



LRESULT WINAPI __export SubEditCntlProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
WNDPROC SubClassEditCntl( HWND hDlg, int nDlgItem, WORD wType);
void ChangeEditCntlType( HWND hDlg, int nDlgItem, WORD wType);
void ResetSubClassEditCntl(HWND hDlg, int nDlgItem);


#endif
