/********************************************************************
//
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
//
//    Project : CA/OpenIngres Visual DBA
//
//    Source : statbar.h
//    ribbon status bar
//
********************************************************************/

#ifndef __STATBAR_H__
#define __STATBAR_H__

#include "port.h"

/* Status bar field types */
#define SBT_NORMAL     0x0000
#define SBT_FRAME      0x0001
#define SBT_THICKFRAME 0x0002
#define SBT_3D         0x0004
#define SBT_VISIBLE    0x0100
#define SBT_SUPER      0x1000


/* Status bar messages */
#define SBM_SETFIELD           (WM_USER+300)
#define SBM_GETFIELD           (WM_USER+301)
#define SBM_SETMODE            (WM_USER+302)
#define SBM_SHOWFIELD          (WM_USER+303)
#define SBM_SIZEFIELD          (WM_USER+304)
#define SBM_SETBARBKGCOLOR     (WM_USER+305)
#define SBM_SETFIELDFACECOLOR  (WM_USER+306)
#define SBM_SETFIELDTEXTCOLOR  (WM_USER+307)


typedef struct tagsb_fielddef
{
  WORD     wID;
  WORD     wType;
  RECT     boundRect;
  COLORREF lTextColor;
  COLORREF lFaceColor;
  HFONT    hFont;
  WORD     wFormat;
  WORD     wLength;
} SB_FIELDDEF, FAR *LPSB_FIELDDEF;

typedef struct tagsb_fieldlist
{
  WORD         x;
  WORD         y;
  WORD         cx;
  WORD         cy;
  COLORREF     lFrameColor;
  COLORREF     lLightColor;
  COLORREF     lShadowColor;
  COLORREF     lBkgnColor;
  WORD         nFields;
  SB_FIELDDEF  fields[1];
} SB_DEFINITION, FAR *LPSB_DEFINITION;

HWND NewStatusBar( HANDLE hInst, HWND hwndParent, LPSB_DEFINITION lpStatBarDef );

#endif

