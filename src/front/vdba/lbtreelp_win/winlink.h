#ifndef __WINLINK_INCLUDED__
#define __WINLINK_INCLUDED__

#ifdef WIN32
#define LONG2POINT(l,pt)\
            ((pt).x=(SHORT)LOWORD(l),(pt).y=(SHORT)HIWORD(l))

__inline unsigned long GetTextExtent(HDC hdc, LPSTR lpstr, size_t cb);
__inline unsigned long GetTextExtent(HDC hdc, LPSTR lpstr, size_t cb)
{
    SIZE size;
    GetTextExtentPoint32(hdc, lpstr, cb, &size);
    return (DWORD)MAKELONG(size.cx, size.cy);
}
#endif

#define DISTANCE  4

typedef unsigned long ULONG;

#define LM_BASE WM_USER+1000

#define LM_ADDRECORD                LM_BASE+1
#define LM_DELETERECORD             LM_BASE+2
#define LM_SETSEL                   LM_BASE+3
#define LM_GETSEL                   LM_BASE+4
#define LM_GETFONT                  LM_BASE+5
#define LM_SETFONT                  LM_BASE+6
#define LM_EXPAND                   LM_BASE+7
#define LM_COLLAPSE                 LM_BASE+8
#define LM_EXPANDBRANCH             LM_BASE+9
#define LM_COLLAPSEBRANCH           LM_BASE+10
#define LM_EXPANDALL                LM_BASE+11
#define LM_COLLAPSEALL              LM_BASE+12
#define LM_GETSTATE                 LM_BASE+13
#define LM_GETSTRING                LM_BASE+14
#define LM_QUERYSTRING              LM_BASE+15
#define LM_MEASUREITEM              LM_BASE+16
#define LM_DRAWITEM                 LM_BASE+17
#define LM_NOTIFY_LBUTTONCLICK      LM_BASE+18
#define LM_NOTIFY_LBUTTONDBLCLK     LM_BASE+19
#define LM_NOTIFY_RBUTTONCLICK      LM_BASE+20
#define LM_NOTIFY_RBUTTONDBLCLK     LM_BASE+21
#define LM_NOTIFY_ONITEM            LM_BASE+22
#define LM_NOTIFY_SELCHANGE         LM_BASE+23
#define LM_NOTIFY_EXPAND            LM_BASE+24
#define LM_NOTIFY_COLLAPSE          LM_BASE+25
#define LM_NOTIFY_RETURNKEY         LM_BASE+26
#define LM_NOTIFY_TABKEY            LM_BASE+27
#define LM_DELETEALL                LM_BASE+28
#define LM_GETITEMDATA              LM_BASE+29
#define LM_GETFIRST                 LM_BASE+30
#define LM_GETNEXT                  LM_BASE+31
#define LM_GETNEXTPRESENT           LM_BASE+32
#define LM_GETFIRSTPRESENT          LM_BASE+33
#define LM_GETPARENT                LM_BASE+34
#define LM_GETLEVEL                 LM_BASE+35
#define LM_NOTIFY_GETMINMAXINFOS    LM_BASE+36
#define LM_NOTIFY_STRINGHASCHANGE   LM_BASE+37
#define LM_GETEDITHANDLE            LM_BASE+38
#define LM_ITEMFROMPOINT            LM_BASE+39
#define LM_SETITEMDATA              LM_BASE+40
#define LM_SETSTRING                LM_BASE+41

#define LM_GETFIRSTCHILD            LM_BASE+42


#define LM_NOTIFY_DESTROY           LM_BASE+43
#define LM_EXISTSTRING              LM_BASE+44

#define LM_GETMAXEXTENT             LM_BASE+45
#define LM_FINDFROMITEMDATA         LM_BASE+46
#define LM_NOTIFY_ESCKEY            LM_BASE+47

#define LM_GETTOPITEM               LM_BASE+48
#define LM_SETTOPITEM               LM_BASE+49


#define LM_FINDFIRST                LM_BASE+50
#define LM_FINDNEXT                 LM_BASE+51

#define LM_GETSEARCHSTRING          LM_BASE+52
#define LM_QUERYICONS               LM_BASE+53

#define LM_SETBKCOLOR               LM_BASE+54
#define LM_GETITEMRECT              LM_BASE+55
#define LM_SETTABSTOP               LM_BASE+56

#define LM_GETSTYLE			            LM_BASE+57
#define LM_SETSTYLE				          LM_BASE+58
#define LM_GETPREV                  LM_BASE+59 

#define LM_SETPRINTFLAGS            LM_BASE+60

#define LM_GETCOUNT					        LM_BASE+61

#define LM_EPILOGDRAW				        LM_BASE+62
#define LM_SETICONSPACING			      LM_BASE+63
#define LM_ADDRECORDBEFORE          LM_BASE+64

#define LM_GETEDITCHANGES           LM_BASE+65
#define LM_DISABLEEXPANDALL         LM_BASE+66

#define LM_GETLISTBOXSTRING         LM_BASE+67
#define LM_GETVISSTATE              LM_BASE+68

// Drag/Drop features Emb 18/4/95
#define LM_NOTIFY_STARTDRAGDROP         LM_BASE+69
#define LM_NOTIFY_DRAGDROP_MOUSEMOVE    LM_BASE+70
#define LM_NOTIFY_ENDDRAGDROP           LM_BASE+71
#define LM_GETMOUSEITEM                 LM_BASE+72
#define LM_CANCELDRAGDROP               LM_BASE+73

// Debug Emb 26/6/95 : performance measurement data
#define LM_RESET_DEBUG_EMBCOUNTS        LM_BASE+74

#define LM_END                          LM_CANCELDRAGDROP+1+1

#define PRINT_CURRENTSTATE          0x0000
#define PRINT_ALL                   0x0001

#define COUNT_VISIBLE				0x0000
#define COUNT_DISPLAYABLE			0x0001
#define COUNT_ALL					0x0002

#define LMS_SORT        LBS_SORT
#define LMS_OWNERDRAW   LBS_OWNERDRAWFIXED
#define LMS_AUTOVSCROLL ES_AUTOVSCROLL	  
#define LMS_AUTOHSCROLL ES_AUTOHSCROLL	  
#define LMS_WITHICONS   0x00000100L
#define LMS_CANEDIT     0x0200L
#define LMS_ALLONITEMMESSAGES	0x0400L
#define LMS_EPILOGDRAW	0x10000L

#define IDI_ICON_PLUS   100
#define IDI_ICON_MINUS  101

#define IDI_ICON_PLUS2   200
#define IDI_ICON_MINUS2  201

#define MAX_SCROLL      0x4000
#define SIZE_ICONH      14
#define SIZE_ICONV      12

#define MAX_EDIT_TEXT_LENGTH 1024

typedef struct _LMDRAWITEMSTRUCT
{
  HWND	 hWndItem;
  HDC		 hDC;
  DWORD  ulID;
  LPSTR  lpString;
  UINT   uiItemState;    
  UINT   uiLevel;
  UINT   uiIndentSize;
  BOOL   bExpanded;
  BOOL   bParent;
  RECT	 rcItem;
  RECT	 rcPaint;
  LPINT  lpiPosLines;
  COLORREF textColor;
  COLORREF bkColor;
  int    iTabStopSize;
} LMDRAWITEMSTRUCT,FAR * LPLMDRAWITEMSTRUCT;

typedef struct _LMQUERYICONS
{
    DWORD  ulID;
    HICON hIcon1;
    HICON hIcon2;
} LMQUERYICONS, FAR * LPLMQUERYICONS;

#define ITEMSTATENORMAL       0x0000
#define ITEMSTATESELECTED     0x0001
#define ITEMSTATEFOCUS        0x0002
#define ITEMSTATEDRAWENTIRE   0x0004
#define LMPRINTMODE           0x0010
#define LMPRINTSELECTONLY     0x0100

#define LMDRAWNOTHING         0x0000L
#define LMDRAWSTRING          0x0001L
#define LMDRAWLINES           0x0002L

#define LMDRAWALL             (LMDRAWSTRING|LMDRAWLINES)

extern HINSTANCE  ghDllInstance;
extern LPSTR      pszGlobalDefaultString;
extern int        CalcScrollPos(ULONG nElem,ULONG nFirstVisible);
extern ULONG      CalcThumbPos(ULONG nElem,ULONG thumbPosition);
extern void PaintItem(LPLMDRAWITEMSTRUCT lm,DWORD ulFlags,HICON hIcon1,HICON hIcon2,int iIconsMargin);
extern void PaintVODrawStyle(LPLMDRAWITEMSTRUCT lm,DWORD ulFlags,HICON hIcon1,HICON hIcon2,int iIconsMargin);


typedef struct _LINK_RECORD
{
  ULONG  ulRecID;
  ULONG  ulParentID;
  ULONG  ulInsertAfter;
  LPSTR  lpString;
  LPVOID lpItemData;
} LINK_RECORD,FAR * LPLINK_RECORD;


typedef struct _ITEMRECT
{
  DWORD  ulRecID;
  RECT   rect;
} ITEMRECT,FAR *LPITEMRECT;


typedef struct _SETITEMDATA
{
  DWORD  ulRecID;    // record ID
  LPVOID lpItemData; // new itemdata
} SETITEMDATA,FAR * LPSETITEMDATA;

typedef struct _SETSTRING
{
  DWORD  ulRecID;    // record ID
  LPSTR  lpString;   // new string
} SETSTRING,FAR * LPSETSTRING;


class LinkedListWindow : public Linked_List
{
  private:
    HWND        hWnd;
    HWND        hWndOwner;
    HWND	      hWndEdit;
    LONG        lpOldEditProc;
    POINT       ptSizeWin;
    POINT       ptPete;
    BOOL        bEditChanged;
    int         sizeItem;
    int         sizeIndent;
    int         nbVisible;
    Link *      firstvisible;
    Link *      cursel;
    Link *      curMessage;
    Link *      curAsk;
    BOOL        inDrag;
    void        RetrieveLines();
    void        VerifyScroll();
    void        VerifyScrollH();
    void        SetVisible(Link *l);
    BOOL        SetExpOneLevel(Link * pParent,BOOL bExpanded);
    HFONT       hFont;
    int         bDirty; // values 1: need scroll updates 2 : need all updates
    int         maxExtent;
    BOOL        VODrawStyle;
    BOOL        ownerDraw;
  	BOOL		epilogDraw;
    void        GetExtents();
    void        RetrievePaintInfos(LPLMDRAWITEMSTRUCT lpLM);
    char *      pStrToSearch;
    BOOL        bSearchMatch;
    int         iIconsMargin;
    COLORREF    bkColor;
    int         iTabStopSize;
    unsigned int uiPrintFlag;
  	BOOL		    bDisableExpandAll;

    // Emb 18/4/95 for drag/drop management
    BOOL        inDragDrop;
    int         DragDropMode;     // 0=move, 1=copy

    // Emb 26/6/95 for LinkedListWindow::ON_MOUSEMOVE() optimization
    POINT       lastpt;           // last point behind the mouse

  public:
    BOOL        bPaintSelectOnFocus;

    LinkedListWindow(HWND hWnd=0);
    ~LinkedListWindow();

    VOID    MouseClick        (POINT ptMouse,BOOL bRightButton);

    LRESULT ON_CREATE         (WPARAM wParam,LPARAM lParam);
    LRESULT ON_DESTROY        (WPARAM wParam,LPARAM lParam);
    LRESULT ON_PAINT          (WPARAM wParam,LPARAM lParam);
    LRESULT ON_SIZE           (WPARAM wParam,LPARAM lParam);
    LRESULT ON_VSCROLL        (WPARAM wParam,LPARAM lParam);
    LRESULT ON_HSCROLL        (WPARAM wParam,LPARAM lParam);

    LRESULT ON_ADDRECORD      (WPARAM wParam,LPARAM lParam);
    LRESULT ON_DELETERECORD   (WPARAM wParam,LPARAM lParam);
    LRESULT ON_SETSEL         (WPARAM wParam,LPARAM lParam);
    LRESULT ON_GETSEL         (WPARAM wParam,LPARAM lParam);
    LRESULT ON_MOUSEMOVE      (WPARAM wParam,LPARAM lParam);
    LRESULT ON_TIMER          (WPARAM wParam,LPARAM lParam);
    LRESULT ON_LBUTTONUP      (WPARAM wParam,LPARAM lParam);
    LRESULT ON_SETFOCUS       (WPARAM wParam,LPARAM lParam);
    LRESULT ON_KILLFOCUS      (WPARAM wParam,LPARAM lParam);

    LRESULT ON_LBUTTONDOWN    (WPARAM wParam,LPARAM lParam);
    LRESULT ON_LBUTTONDBLCLK  (WPARAM wParam,LPARAM lParam);

    LRESULT ON_RBUTTONDOWN    (WPARAM wParam,LPARAM lParam);
    LRESULT ON_RBUTTONDBLCLK  (WPARAM wParam,LPARAM lParam);

    LRESULT ON_SETFONT        (WPARAM wParam,LPARAM lParam);
    LRESULT ON_GETFONT        (WPARAM wParam,LPARAM lParam);
    LRESULT ON_KEYDOWN        (WPARAM wParam,LPARAM lParam);
    LRESULT ON_CHAR           (WPARAM wParam,LPARAM lParam);
    LRESULT ON_EXPAND         (WPARAM wParam,LPARAM lParam);
    LRESULT ON_COLLAPSE       (WPARAM wParam,LPARAM lParam);
    LRESULT ON_USER           (WPARAM wParam,LPARAM lParam);

    LRESULT ON_GETSTRING      (WPARAM wParam,LPARAM lParam);

    LRESULT ON_EXPANDBRANCH   (WPARAM wParam,LPARAM lParam);
    LRESULT ON_COLLAPSEBRANCH (WPARAM wParam,LPARAM lParam);
    LRESULT ON_EXPANDALL      (WPARAM wParam,LPARAM lParam);
    LRESULT ON_COLLAPSEALL    (WPARAM wParam,LPARAM lParam);
    LRESULT ON_GETMINMAXINFOS (WPARAM wParam,LPARAM lParam);

    LRESULT ON_COMMAND        (WPARAM wParam,LPARAM lParam);

    LRESULT ON_GETITEMDATA    (WPARAM wParam,LPARAM lParam);
    LRESULT ON_SETITEMDATA    (WPARAM wParam,LPARAM lParam);
    
    LRESULT ON_GETFIRST       (WPARAM wParam,LPARAM lParam);

    LRESULT ON_GETNEXT        (WPARAM wParam,LPARAM lParam);

    LRESULT ON_GETFIRSTPRESENT(WPARAM wParam,LPARAM lParam);
    LRESULT ON_GETNEXTPRESENT (WPARAM wParam,LPARAM lParam);


    LRESULT ON_GETPARENT      (WPARAM wParam,LPARAM lParam);
    LRESULT ON_GETLEVEL       (WPARAM wParam,LPARAM lParam);
    LRESULT ON_SETSTRING      (WPARAM wParam,LPARAM lParam);

    LRESULT ON_GETSTATE       (WPARAM wParam,LPARAM lParam);
    
    LRESULT ON_GETVISSTATE       (WPARAM wParam,LPARAM lParam);

    


    LRESULT ON_DELETEALL      (WPARAM wParam,LPARAM lParam);
    LRESULT ON_GETEDITHANDLE  (WPARAM wParam,LPARAM lParam);

    LRESULT ON_ITEMFROMPOINT  (WPARAM wParam,LPARAM lParam);
    LRESULT ON_GETFIRSTCHILD  (WPARAM wParam,LPARAM lParam);

    LRESULT ON_EXISTSTRING    (WPARAM wParam,LPARAM lParam);
    LRESULT ON_GETMAXEXTENT   (WPARAM wParam,LPARAM lParam);

    LRESULT ON_FINDFROMITEMDATA   (WPARAM wParam,LPARAM lParam);

    LRESULT ON_GETTOPITEM   (WPARAM wParam,LPARAM lParam);
    LRESULT ON_SETTOPITEM   (WPARAM wParam,LPARAM lParam);


    LRESULT ON_FINDFIRST  (WPARAM wParam,LPARAM lParam);
    LRESULT ON_FINDNEXT   (WPARAM wParam,LPARAM lParam);

    LRESULT ON_GETSEARCHSTRING (WPARAM wParam,LPARAM lParam);

    LRESULT ON_SETBKCOLOR  (WPARAM wParam,LPARAM lParam);
    LRESULT ON_GETITEMRECT (WPARAM wParam,LPARAM lParam);

    LRESULT ON_SETTABSTOP (WPARAM wParam,LPARAM lParam);

  	LRESULT ON_GETSTYLE (WPARAM wParam,LPARAM lParam);
    LRESULT ON_SETSTYLE (WPARAM wParam,LPARAM lParam);

    LRESULT ON_GETPREV (WPARAM wParam,LPARAM lParam);
    LRESULT ON_SETPRINTFLAGS(WPARAM wParam,LPARAM lParam);

    LRESULT ON_GETCOUNT(WPARAM wParam,LPARAM lParam);
    LRESULT ON_SETICONSPACING(WPARAM wParam,LPARAM lParam);

    LRESULT ON_ADDRECORDBEFORE(WPARAM wParam,LPARAM lParam);
    LRESULT ON_GETEDITCHANGES(WPARAM wParam,LPARAM lParam);

    LRESULT ON_DISABLEEXPANDALL(WPARAM wParam,LPARAM lParam);

    // Drag/Drop features Emb 18/4/95
    LRESULT ON_GETMOUSEITEM(WPARAM wParam,LPARAM lParam);
    LRESULT ON_CANCELDRAGDROP(WPARAM wParam,LPARAM lParam);
    
    // Debug Emb 26/6/95 : performance measurement data
    LRESULT ON_RESET_DEBUG_EMBCOUNTS(WPARAM wParam,LPARAM lParam);

    LRESULT             ChangeSelection(Link * lNew,BOOL bRepaint);
    LONG                GetEditProc() {return lpOldEditProc;}
    BOOL                GetItemRect    (Link * pl, LPRECT lpRC);
    ULONG               GetIndex       (Link * l);
    char FAR *         GetString      (Link * l);
    char FAR *         RetrieveEditString(void);
    void                EditNotify     (BOOL bNewItem); 
    void                EditMove(void);
    void                NotifyReturn(void);
    HWND                HWnd()    { return hWnd;}
    Link              * Cursel()  { return cursel;}
    LONG                FindFromString(Link * From);
    LONG                FindFromString(Link * From,char * pStr,BOOL bMatch);
    void                CalcTabStop(HDC hDC,int * lpIPixelSize);
};

#endif // __WINLINK_INCLUDED__
  
  
  

