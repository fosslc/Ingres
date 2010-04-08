#ifndef __FLOATBAR_INCLUDED__
#define __FLOATBAR_INCLUDED__

extern "C" 
{
#include <windows.h>
#include "floatbar.e"
}

#define FB_MARGINS 2 // buttons margins inside the window

// wrap of windows brush gdi object
class  Brush
{
  HBRUSH  hbr;
public:
  Brush(int color)       { hbr = CreateSolidBrush(GetSysColor(color));}
  ~Brush()               { if (hbr) DeleteObject(hbr);}
  HBRUSH _near Handle()  { return hbr;}
};

// base class for linked list of buttons
class Button
{
  HWND     hOwner;       // Window that recieve message when the button is clicked
  HBITMAP  hBitmap;      // passed when created
  WORD     wCommandID;   // passed when created
  HCURSOR  hCursor;      // passed when created
  BOOL     bPushed;      // pushed flag
  BOOL     bEnabled;     // enable flag
  Button * next;         // next entry in the link list
  RECT     rc;           // rectangle, relative to the client area of parent

public:
  Button(HWND hWndOwner,HBITMAP hBit,WORD wCommand,POINT pt,HCURSOR hCur);
  ~Button();
  void     Paint(HDC hDC,HDC hMemDC);
  void     Clic();

  Button * Next()                 { return  next;}
  void     SetNext(Button *b)     { next    = b;}
  HWND     Owner()                { return  hOwner;}
  LPRECT   Rect()                 { return  &rc;}
  void     Select()               { bPushed =TRUE;}
  void     UnSelect()             { bPushed =FALSE;}
  WORD     ID()                   { return   wCommandID;}
  HBITMAP  Bitmap()               { return   hBitmap;}
  HCURSOR  Cursor()               { return   hCursor;}
  BOOL     IsInabled()            { return   bEnabled;}
  VOID     Enable(BOOL bNewState) { bEnabled=bNewState;}

};

// base class for floatbar window

class Floatbar
{
  HWND      hWnd;            // The window itself
  HWND      hDrag;           // The window that accept drag an drop
  HDC       hMemDC;          // So it does not have to be recreated for each redraw
  Button *  first;           // Linked list head
  Button *  current;         // Current pushed button
  Button *  underTheMouse;   // see FB_MOUSEONBUTTON message
  HBITMAP   hSysMenuBitmap;  // bitmap use to draw the "mini" system menu
  RECT      rcCaption;       // "mini" window caption rectangle
  int       iBitmapW;        // horizontal size of the System menu bitmap
  POINT     ptClicPos;       // used to change the cursor only after a certain displacement
  int       nbColums;        // current number of column
  HCURSOR   hDragCursor;     // cursor use to drag a button (if the button itself does not have his cursor)



public:
  Floatbar(HWND hOwn,HWND hwndDrag);
  ~Floatbar();
  void Paint();
  void Add(HWND hWndOwner,HBITMAP hBit,WORD wCommand,HCURSOR hCur);
  void MouseDown(POINT where);
  void Move     (POINT where);
  void MouseUp  (POINT where);
  int  NCHitTest(POINT where);
  void NcMove   ();
  void FocusChange(BOOL bSetFocus);
  int  SysCommand(WPARAM wParam,LPARAM lParam);
  int  Size(POINT ptNewSize);
  void MenuSelect(WORD fwMenu,WORD hMenu);
  void NcDblClks (POINT ptMouse,BOOL bInc);
  HWND Owner();

  void Select(Button *b);
  Button * Cursel() {return current;};
  BOOL Setsel(WORD wBtnID);

  int  SetColumn(int nb);
  int  SetOwner(HWND hNew);
  void DeleteAll(int bFirst); //366@0001 MS 05/12/94
  int  Close(void);
  int  Count();
  VOID Enable(WORD wBtnID,BOOL bEnable);
};

#endif  __FLOATBAR_INCLUDED__
