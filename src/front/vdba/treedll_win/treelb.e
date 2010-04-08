/********************************************************************
//
//    Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
//
//    Project  : CA/OpenIngres Visual DBA
//
//    Source   : treelb.e
//
//    
//
********************************************************************/
// Drag/Drop features added by Emb starting from 18/4/95:
// LM_NOTIFY_STARTDRAGDROP         
// LM_NOTIFY_DRAGDROP_MOUSEMOVE    
// LM_NOTIFY_ENDDRAGDROP           
// LM_GETMOUSEITEM                 
// LM_CANCELDRAGDROP               

// Modified for CPP 2/2/96
#ifdef __cplusplus
extern "C"
{
#endif

//////////////////////////////////////////////////////////////////////////////
// Message : LM_NOTIFY_STARTDRAGDROP               
// Description: This message is send by the list to the owner when the user
//              starts a drag/drop
// Parameters:
// lParam: record ID 
// wParam: drag/drop mode : 0 if move, 1 if copy (according to Ctrl key)
// return value: no return value is expected
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Message : LM_NOTIFY_DRAGDROP_MOUSEMOVE
// Description: This message is send by the list to the owner when the user
//              moves the mouse while drag/drop mode is set
// Parameters:
// lParam: POINT: mouse position (see WM_MOUSEMOVE for details)
// wParam: HWND: list window handle
// return value: no return value is expected
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Message : LM_NOTIFY_ENDDRAGDROP               
// Description: This message is send by the list to the owner when the user
//              drops, i.e. releases the mouse while in drag/drop mode
// Parameters:
// lParam: 0L
// wParam: HWND: list window handle
// return value: no return value is expected
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Message : LM_GETMOUSEITEM
// Description: returns the item under the mouse
// Parameters:
// lParam: not used
// wParam: not used
// return value: ID of record under the mouse, or 0L if none
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Message : LM_CANCELDRAGDROP
// Description: cancels the drag/drop mode
// Parameters:
// lParam: not used
// wParam: not used
// return value: no return value is expected
//////////////////////////////////////////////////////////////////////////////

//
// End of Drag/Drop features added by Emb starting from 18/4/95
//

//////////////////////////////////////////////////////////////////////////////
// Message : LM_ADDRECORD              
// Description: Add a record in the list
// Parameters:
// lParam: Points to a LINK_RECORD structure
// wParam: not used
// return value: TRUE if successfull, FALSE otherwise
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Message : LM_DELETERECORD           
// Description: Delete a record in the list. 
// Parameters:
// lParam: contains the record ID
// wParam: not used
// return value: TRUE if successfull, FALSE otherwise
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Message : LM_SETSEL                 
// Description: Change The current selection
// Parameters:
// lParam: contains the record ID
// wParam: not used
// return value: TRUE if successfull, FALSE otherwise
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Message : LM_GETSEL                 
// Description: Return the current selected record
// Parameters:
// lParam: not used
// wParam: not used
// return value: ID of current selected record
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Message : LM_GETFONT                
// Description: return the font used by the list
// Parameters:
// lParam: not used
// wParam: not used
// return value: HFONT , font used by the list
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Message : LM_SETFONT                
// Description: set the list current font
// Parameters:
// lParam: not used
// wParam: HFONT
// return value: the previous font
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Message : LM_EXPAND                 
// Description: expand one record
// Parameters:
// lParam: record ID
// wParam: not used
// return value: TRUE if successfull. FALSE if the record is already expanded
//               or if it contains no childs
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Message : LM_COLLAPSE               
// Description: collapse one record
// Parameters:
// lParam: record ID
// wParam: not used
// return value: TRUE if successfull. FALSE if the record is already collapsed
//               or if it contains no childs
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Message : LM_EXPANDBRANCH           
// Description: expand one record and all its childs
// Parameters:
// lParam: record ID
// wParam: not used
// return value: TRUE if successfull. FALSE if the record is already expanded
//               or if it contains no childs
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Message : LM_COLLAPSEBRANCH         
// Description: collapse one record and all its childs
// Parameters:
// lParam: record ID
// wParam: not used
// return value: TRUE if successfull. FALSE if the record is already collapsed
//               or if it contains no childs
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Message : LM_EXPANDALL              
// Description: expand the entire tree
// Parameters:
// lParam: not used
// wParam: not used
// return value: TRUE if successfull, FALSE otherwise
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Message : LM_COLLAPSEALL            
// Description: collapse the entire tree
// Parameters:
// lParam: not used
// wParam: not used
// return value: TRUE if successfull, FALSE otherwise
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Message : LM_GETSTATE               
// Description: return the state of a record
// Parameters:
// lParam: record ID 
// wParam: not used
// return value: TRUE if the record is expanded, FALSE if it is collapsed
//               -1 if the record does not exist.
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Message : LM_GETVISSTATE               
// Description: return the visibility state of a record
// Parameters:
// lParam: record ID 
// wParam: not used
// return value: TRUE if the record is candidate for visibility
// (none of it's parents are collapsed),
//               FALSE if it is collapsed,
//               -1 if the record does not exist.
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Message : LM_GETSTRING              
// Description: return a pointer to the record string
// Parameters:
// lParam: record ID 
// wParam: not used
// return value: pointer to the record string. this string can't be modified directly
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Message : LM_NOTIFY_LBUTTONCLICK    
// Description: This message is send by the list to the owner when the user
//              presses the left mouse button
// Parameters:
// lParam: record ID 
// wParam: HWND: list window handle
// return value: no return value is expected
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Message : LM_NOTIFY_LBUTTONDBLCLK   
// Description: This message is send by the list to the owner when the user
//              double-clicks the left mouse button
// Parameters:
// lParam: record ID 
// wParam: HWND: list window handle
// return value: no return value is expected
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Message : LM_NOTIFY_RBUTTONCLICK    
// Description: This message is send by the list to the owner when the user
//              presses the right mouse button
// Parameters:
// lParam: record ID 
// wParam: HWND: list window handle
// return value: no return value is expected
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Message : LM_NOTIFY_RLBUTTONDBLCLK   
// Description: This message is send by the list to the owner when the user
//              double-clicks the right mouse button
// Parameters:
// lParam: record ID 
// wParam: HWND: list window handle
// return value: no return value is expected
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Message : LM_NOTIFY_ONITEM          
// Description: This message is send by the list to the owner when the mouse
//              pointer position  change
// Parameters:
// lParam: record ID 
// wParam: HWND: list window handle
// return value: no return value is expected
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Message : LM_NOTIFY_SELCHANGE       
// Description: This message is send by the list to the owner when the selection
//              changes
// Parameters:
// lParam: selected record ID 
// wParam: HWND: list window handle
// return value: no return value is expected
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Message : LM_NOTIFY_EXPAND          
// Description: This message is send by the list to the owner when a record
//              is expanded
// Parameters:
// lParam: record ID 
// wParam: HWND: list window handle
// return value: no return value is expected
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Message : LM_NOTIFY_COLLAPSE        
// Message : LM_NOTIFY_EXPAND          
// Description: This message is send by the list to the owner when a record
//              is collapsed
// Parameters:
// lParam: record ID 
// wParam: HWND: list window handle
// return value: no return value is expected
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Message : LM_NOTIFY_RETURNKEY       
// Description: This message is send by the list to the owner when the user 
//              press the return key
// Parameters:
// lParam: record ID 
// wParam: HWND: list window handle
// return value: no return value is expected
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Message : LM_NOTIFY_TABKEY          
// Description: This message is send by the list to the owner when the user 
//              press the tab key
// Parameters:
// lParam: record ID 
// wParam: HWND: list window handle
// return value: no return value is expected
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Message : LM_NOTIFY_ESCKEY          
// Description: This message is send by the list to the owner when the user 
//              press the esc key
// Parameters:
// lParam: record ID 
// wParam: HWND: list window handle
// return value: no return value is expected
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Message : LM_DELETEALL              
// Description: Delete the whole list
// Parameters:
// lParam: not used
// wParam: not used
// return value: always TRUE
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Message : LM_GETITEMDATA            
// Description:
// Parameters:
// lParam: record ID
// wParam: not used
// return value: the field "lpItemData" of the structure used to add the record
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Message : LM_GETFIRST               
// Description: return the first record of the list
// Parameters:
// lParam: not used
// wParam: not used
// return value: the ID of the first record
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Message : LM_GETNEXT                
// Description: get next record (this record can be collapsed).
// Parameters:
// lParam: record ID. if 0, 
//
// wParam: not used
// return value: ID of the next record
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Message : LM_GETPREV                
// Description: get previous record (this record can be collapsed).
// Parameters:
// lParam: record ID. if 0, 
//
// wParam: not used
// return value: ID of the previous record
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Message : LM_GETPARENT              
// Description: return the parent of a record
// Parameters:
// lParam: record ID
// wParam: not used
// return value: the parent record ID if successfull, 0 otherwise
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Message : LM_NOTIFY_STRINGHASCHANGE 
// Description: If the list has the LMS_CANEDIT style, this message is send 
//              to the owner when the selection or focus change and the edit
//              control contents has changed.
//
// Parameters:
// lParam: record ID 
// wParam: HWND: list window handle
// return value: if the owner returns TRUE, the new string is stored with the record
//               if the owner returns TRUE, the changes are discarded
//
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Message : LM_GETEDITHANDLE          
// Description: If the list has the LMS_CANEDIT style, this message returns  
//              the window handle of the edit control
// Parameters:
// lParam: not used
// wParam: not used
// return value: the window handle of the edit control
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Message : LM_ITEMFROMPOINT          
// Description: returns the ID of the record containing a point
// Parameters:
// lParam: POINT (LOWORD=x, HIWORD=y)
// wParam: not used
// return value: record ID if successfull
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Message : LM_SETITEMDATA            
// Description: associate an item data with a record
// Parameters:
// lParam: points to a SETITEMDATA structure
// wParam: not used
// return value: the old value if successfull
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Message : LM_SETSTRING              
// Description: change the record string
// Parameters:
// lParam: points to a SETSTRING structure
// wParam: not used
// return value: TRUE if successfull, FALSE otherwise
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Message : LM_FINDFROMITEMDATA       
// Description: return a record ID form the record Itemdata
// Parameters:
// lParam: item data
// wParam: not used
// return value: return the record ID if successfull, 0 otherwise
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Message : LM_GETTOPITEM             
// Description: return the first visible record
// Parameters:
// lParam: not used
// wParam: not used
// return value: record ID of the first visible record
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Message : LM_SETTOPITEM             
// Description: set the first visible record
// Parameters:
// lParam: record ID
// wParam: not used
// return value: TRUE if successfull, FALSE otherwise
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Message : LM_FINDFIRST              
// Description: search a record 
// Parameters:
// lParam: pointer to a string to search. ? and * are allowed
// wParam: if TRUE the string must match the record string
// return value: record ID if successfull, 0 if not
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Message : LM_FINDNEXT               
// Description: continue searching after the LM_FINDFIRST message
// Parameters:
// lParam: pointer to a string to search. if 0 the string passed to LM_FINDFIRST is used
// wParam: if TRUE the string must match the record string
// return value: record ID if successfull, 0 if not
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Message : LM_GETSEARCHSTRING        
// Description: return the string currently use by the LM_FINDNEXT message
// Parameters:
// lParam: not used
// wParam: not used
// return value: pointer to the string. this string must not be changed.
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Message : LM_QUERYICONS             
// Description: the list can display two icons before the string
//              if it has the style LMS_WITHICONS. this message is send
//              to the owner to get these icons
// Parameters:
// lParam: record ID 
// wParam: HWND: list window handle
// return value: the owner window return the first icon handle in the LOWORD
//               and the second in the HIWORD. The icon can bel NULL
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Message : LM_SETBKCOLOR             
// Description: change the default background color of the list
// Parameters:  
// lParam: COLORREF new background color
// wParam: not used
// return value: old backgound color
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Message : LM_GETITEMRECT            
// Description: return, in client coordinates, the rectangle of a record
// Parameters:
// lParam: points to a PITEMRECT structure
// wParam: not used
// return value: TRUE if the rectangle if visible, FALSE otherwise
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Message : LM_SETTABSTOP             
// Description: set the tab stop size
// Parameters:
// lParam: not used
// wParam: UINT tab stop size, in characters
// return value: old tab stop size
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Message : LM_SETPRINTFLAGS          
// Description: change the printing flag
// Parameters:
// lParam: not used
// wParam: can be PRINT_CURRENTSTATE (print the list as displayed)
//                or PRINT_ALL (print also collapsed records)
// return value: always TRUE
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Message : LM_MEASUREITEM            
// Description: This message is send by the list to the owner if the list
//              has the LMS_OWNERDRAW style
// Parameters:
// lParam: not used
// wParam: HWND: list window handle
// return value: HIWORD is the heigth of a record LOWORD is the indentation
//               size   
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Message : LM_DRAWITEM               
// Description: This message is send by the list to the owner if the list
//              has the LMS_OWNERDRAW style, each time a record is displayed
// Parameters:
// lParam: points to a LMDRAWITEMSTRUCT structure
// wParam: HWND: list window handle
// return value: combination of flags indicating which parts of the record
//               have been drawn. These flags are :
//                  
//                LMDRAWNOTHING the list will paint everything
//                LMDRAWSTRING  the list will paint the backgound and the lines
//                LMDRAWLINES   the listbox will paint the string
//                LMDRAWALL     the list will paint nothing
//                  
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Message : LM_QUERYSTRING            
// Description: This message is send by the list to the owner if a record has been
//              added without string
// Parameters:
// lParam: record ID
// wParam: HWND: list window handle
// return value: a pointer to the string
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Message : LM_GETLEVEL               
// Description: return the level (number of parents) of a record
// Parameters:
// lParam: record ID
// wParam: not used
// return value: the record level. 0 means no parent.
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Message : LM_GETFIRSTCHILD          
// Description: return the first child of a record
// Parameters:
// lParam: record ID
// wParam: not used
// return value: record ID of the first child. 0 if no childs.
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Message : LM_EXISTSTRING            
// Description: search if a string exists int the list
// Parameters:
// lParam: pointer to the string
// wParam: not used
// return value: TRUE if this string exist, FALSE otherwise
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Message : LM_GETMAXEXTENT           
// Description: return the maximum witdh of the list
// Parameters:
// lParam: not used
// wParam: not used
// return value: LOWORD return the maximum witdh of the list. this
//               value can change when collapsing or expanding records
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Message : LM_PRINT_GETFIRST         
// Description:
// Parameters:
// lParam: pointer on LMDRAWITEMSTRUCT
// wParam: MUST be set to TRUE
// return value:
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Message : LM_PRINT_GETNEXT          
// Description:
// Parameters:
// lParam: pointer on LMDRAWITEMSTRUCT
// wParam: MUST be set to TRUE
// return value:
//////////////////////////////////////////////////////////////////////////////



// Messages

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
#define LM_PRINT_GETFIRST           LM_GETFIRSTPRESENT
#define LM_PRINT_GETNEXT            LM_GETNEXTPRESENT
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
#define LM_GETITEMRECT              LM_BASE+55  // lParam points to a ITEMRECT structure
#define LM_SETTABSTOP               LM_BASE+56
#define LM_GETSTYLE                                       LM_BASE+57
#define LM_SETSTYLE                                       LM_BASE+58
#define LM_GETPREV                  LM_BASE+59 
#define LM_SETPRINTFLAGS            LM_BASE+60
#define LM_GETCOUNT                             LM_BASE+61
#define LM_EPILOGDRAW                                   LM_BASE+62
#define LM_SETICONSPACING                             LM_BASE+63
#define LM_ADDRECORDBEFORE          LM_BASE+64
#define LM_GETEDITCHANGES           LM_BASE+65
#define LM_DISABLEEXPANDALL         LM_BASE+66
#define LM_GETLISTBOXSTRING         LM_BASE+67         //390@0001 MS 08/24/94
#define LM_GETVISSTATE              LM_BASE+68  // Emb 13/2/95

// Drag/Drop features Emb 18/4/95
#define LM_NOTIFY_STARTDRAGDROP         LM_BASE+69
#define LM_NOTIFY_DRAGDROP_MOUSEMOVE    LM_BASE+70
#define LM_NOTIFY_ENDDRAGDROP           LM_BASE+71
#define LM_GETMOUSEITEM                 LM_BASE+72
#define LM_CANCELDRAGDROP               LM_BASE+73

// Debug Emb 26/6/95 : performance measurement data
#define LM_RESET_DEBUG_EMBCOUNTS        LM_BASE+74

// speed optimizations for windows95 tree control
#define LM_GETNEXTSIBLING               LM_BASE+75
#define LM_GETPREVSIBLING               LM_BASE+76

#define LM_END                          LM_BASE+76

//
// UK Sotheavut 31-Aug-1998:
// Return the HWND of the Tree View
// WPARAM (ignore), LPARAM (ignore)
#define LM_GET_TREEVIEW_HWND            LM_BASE+80


typedef struct _LMQUERYICONS
{
    DWORD  ulID;
    HICON hIcon1;
    HICON hIcon2;
} LMQUERYICONS, FAR * LPLMQUERYICONS;

// list styles. These styles can be combined
// with standard windows styles

#define LMS_SORT             LBS_SORT            // the list is sorted
#define LMS_OWNERDRAW        LBS_OWNERDRAWFIXED  // owner draw list
#define LMS_CANEDIT          0x0200L             // the list can be edited.
#define LMS_AUTOVSCROLL      ES_AUTOVSCROLL       
#define LMS_AUTOHSCROLL      ES_AUTOHSCROLL       
#define LMS_WITHICONS        0x00000100L
#define LMS_ALLONITEMMESSAGES   0x0400L
#define LMS_EPILOGDRAW            0x10000L

// constants used by the LM_GETCOUNT message
#define COUNT_VISIBLE                           0x0000
#define COUNT_DISPLAYABLE                       0x0001
#define COUNT_ALL                                       0x0002

// constants used by LM_SETPRINTFLAGS message
#define PRINT_CURRENTSTATE          0x0000 // print the list "as is"
#define PRINT_ALL                   0x0001 // print the entire list

// structure used by LM_GETITEMRECT message
typedef struct _ITEMRECT
{
  DWORD  ulRecID;     // record ID
  RECT   rect;        // rectangle
} ITEMRECT,FAR *LPITEMRECT;


// structure used for LM_SETITEMDATA message

typedef struct _SETITEMDATA
{
  DWORD  ulRecID;    // record ID
  LPVOID lpItemData; // new itemdata
} SETITEMDATA,FAR * LPSETITEMDATA;


// structure used for LM_SETSTRING message
typedef struct _SETSTRING
{
  DWORD  ulRecID;    // record ID
  LPSTR  lpString;   // new string
} SETSTRING,FAR * LPSETSTRING;


// structure used for printing and for owner draw list
// to print, the owner must fill the hDC and rcItem members

typedef struct _LMDRAWITEMSTRUCT
{
  HWND     hWndItem;      // Window handle of the list
  HDC              hDC;           // DC used
  DWORD    ulID;          // record ID
  LPSTR    lpString;      // pointer to the record string
  UINT     uiItemState;   // see constants below 
  UINT     uiLevel;       // number of parents
  UINT     uiIndentSize;  // indentation size
  BOOL     bExpanded;     // TRUE if the record is expanded
  BOOL     bParent;       // TRUE if the record has childs
  RECT     rcItem;        // rectangle containing the record
  RECT     rcPaint;       // DC invalid rectangle
  LPINT    lpiPosLines;   // array of int values describing lines in the margin
  COLORREF textColor;     // color used to draw the text and the lines
  COLORREF bkColor;       // color used to draw the background
  int      iTabStopSize;  // size of tab stops in pixels
} LMDRAWITEMSTRUCT,FAR * LPLMDRAWITEMSTRUCT;

// flags used in the uiItemState member of the LMDRAWITEMSTRUCT structure
#define ITEMSTATENORMAL       0x0000   // normal
#define ITEMSTATESELECTED     0x0001   // selected
#define ITEMSTATEFOCUS        0x0002   // the list has the focus
#define ITEMSTATEDRAWENTIRE   0x0004
#define LMPRINTSELECTONLY     0x0100

// possible values of the lpiPosLines member of the LMDRAWITEMSTRUCT structure 
// the size of the array if equal to the level of the record 

#define LM_NOLINE              0  

#define LM_VERTICALLINE        1 // ³

#define LM_CHILDLINE           2 // Ã

#define LM_TERMLINE            3 // À

/*********************************************************
Example :

 RECORD1     uiLevel=0  lpiPosLines=NULL
 ÃRECORD2    uiLevel=1  lpiPosLines=LM_CHILDLINE
 ³ÃRECORD3   uiLevel=2  lpiPosLines=LM_VERTICALLINE,LM_CHILDLINE
 ³³ÀRECORD4  uiLevel=3  lpiPosLines=LM_VERTICALLINE,LM_VERTICALLINE,LM_TERMLINE
 ³³ ÀRECORD5 uiLevel=4  lpiPosLines=LM_VERTICALLINE,LM_VERTICALLINE,LM_NOLINE,LM_TERMLINE
 ³ÀRECORD6   uiLevel=2  lpiPosLines=LM_VERTICALLINE,LM_TERMLINE
 ÀRECORD7    uiLevel=1  lpiPosLines=LM_TERMLINE
 
*********************************************************/


// return value of the LM_DRAWITEM message
#define LMDRAWNOTHING         0x0000L
#define LMDRAWSTRING          0x0001L
#define LMDRAWLINES           0x0002L
#define LMDRAWALL             LMDRAWSTRING|LMDRAWLINES


// structure used by the LM_ADDRECORD message
typedef struct _LINK_RECORD
{
  DWORD  ulRecID;         // record ID 
  DWORD  ulParentID;      // parent record ID, 0 if no parent
  DWORD  ulInsertAfter;   // ID of the record to be insert after
  LPSTR  lpString;        // Optional. this string is copied
  LPVOID lpItemData;      // item data 
} LINK_RECORD,FAR * LPLINK_RECORD;


#ifdef WIN32
#ifdef __cplusplus
    #define LBTREE_LIBAPI
#else
    #define LBTREE_LIBAPI __declspec(dllimport)
#endif
#else
    #define LBTREE_LIBAPI
#endif

LBTREE_LIBAPI extern HWND CALLBACK LRCreateWindow(HWND hOwner,LPRECT rcLocation,DWORD dwStyle,BOOL bVODrawStyle);
LBTREE_LIBAPI extern HWND CALLBACK LRCreateWindowPrint(HWND hOwner,LPRECT rcLocation,DWORD dwStyle,BOOL bVODrawStyle);
  
// Modified for CPP 2/2/96
#ifdef __cplusplus
}
#endif


  
