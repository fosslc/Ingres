/****************************************************************************
 * Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
 ****************************************************************************/

/****************************************************************************
 * CATOCNTR.H
 *
 * Public definitions for applications that use CA-Container control: 
 *
 *   -  Container class name (used in CreateWindow call)
 *   -  Container control styles.
 *   -  Container notification codes.
 *   -  Container color indices.
 *   -  Container attributes.
 *   -  Container record structures.
 *   -  Prototypes for Container API Functions
 *
 ****************************************************************************/

/*
**  History:
**  02-Jan-2002 (noifr01)
**      (SIR 99596) removed definitions used for the drag and drop
**      functionalities, in order to ensure that the corresponding 
**      removed resources are not used
*/

#include <stdlib.h>

#ifndef CATOCNTR_H   /* wrapper */
#define CATOCNTR_H 

#ifdef  __cplusplus
extern "C" {
#endif

/*--------------------------------------------------------------------------*
 * Class name for the container control.
 *--------------------------------------------------------------------------*/

#ifdef _WIN32
#define CONTAINER_CLASS      "CA_Container32"
#else
#define CONTAINER_CLASS      "CA_Container"
#endif

/*--------------------------------------------------------------------------*
 * Container control styles.
 *--------------------------------------------------------------------------*/

#define CTS_READONLY         0x00000100   // container is read-only
#define CTS_SINGLESEL        0x00000200   // container has single record selection
#define CTS_MULTIPLESEL      0x00000400   // container has multiple record selection
#define CTS_EXTENDEDSEL      0x00000800   // container has extended record selection
#define CTS_BLOCKSEL         0x00001000   // container has block field selection
#define CTS_SPLITBAR         0x00002000   // container can have a split bar
#define CTS_VERTSCROLL       0x00004000   // container has vert scroll bar
#define CTS_HORZSCROLL       0x00008000   // container has horz scroll bar
#define CTS_INTEGRALWIDTH    0x00000001   // no unused bk area to the right of last field
#define CTS_INTEGRALHEIGHT   0x00000002   // no partial records at bottom of display area
#define CTS_EXPANDLASTFLD    0x00000004   // expand last field if container increases in width
#define CTS_ASYNCNOTIFY      0x00000080   // use asynchronous notification method
#define CTS_ICONMOVEONLY     0x00000008   // container in icon view supports moving icon only (no drag drop)


// The following standard window styles may also be specified:

//      WS_BORDER                            container has a border  
//      WS_DLGFRAME                          container has a thick border  
//      WS_THICKFRAME                        container has a sizing border  
//      WS_CAPTION                           container has a title bar
//      WS_SYSMENU                           container has a system menu box
//      WS_MAXIMIZEBOX                       container has a maximize button
//      WS_MINIMIZEBOX                       container has a minimize button

/*--------------------------------------------------------------------------*
 * Notification codes sent to application via WM_COMMAND from the control.
 *--------------------------------------------------------------------------*/

#define CN_ASSOCIATEGAIN     501     // wnd is now the container associate
#define CN_ASSOCIATELOSS     502     // wnd is no longer container associate
#define CN_RANGECHANGE       503     // container's range and position chged
#define CN_BEGTTLEDIT        504     // title is about to be directly edited        
#define CN_ENDTTLEDIT        505     // direct edit of title has ended
#define CN_BEGFLDTTLEDIT     506     // a field title is about to be directly edited        
#define CN_ENDFLDTTLEDIT     507     // direct edit of a field title has ended
#define CN_BEGRECEDIT        508     // a record is about to be directly edited        
#define CN_ENDRECEDIT        509     // direct edit of a record has ended
#define CN_BEGFLDEDIT        510     // a field cell is about to be directly edited        
#define CN_ENDFLDEDIT        511     // direct edit of a field cell has ended
#define CN_EMPHASIS          512     // a record's attributes have changed
#define CN_SETFOCUS          513     // container wnd is getting focus
#define CN_KILLFOCUS         514     // container wnd is losing focus
#define CN_QUERYDELTA        515     // container has been scrolled to delta
#define CN_ENTER             516     // container received an ENTER keystroke
#define CN_INSERT            517     // container received INSERT keystroke
#define CN_DELETE            518     // container received DELETE keystroke
#define CN_ESCAPE            519     // container received ESCAPE keystroke
#define CN_TAB               520     // container received TAB keystroke
#define CN_RECSELECTED       521     // a record has been selected
#define CN_RECUNSELECTED     522     // a record has been unselected
#define CN_CHARHIT           523     // a character has been pressed
#define CN_ROTTLDBLCLK       524     // dblclicked on readonly title 
#define CN_ROFLDTTLDBLCLK    525     // dblclicked on readonly field title
#define CN_ROFLDDBLCLK       526     // dblclicked on readonly record field
#define CN_NEWFOCUS          527     // a new record and field have received the focus
#define CN_NEWFOCUSREC       528     // a new record has received the focus (focus field unchanged)
#define CN_NEWFOCUSFLD       529     // a new field has received the focus (focus record unchanged)
#define CN_QUERYFOCUS        530     // container needs the focus record from app
#define CN_F1                531     // container received F1 keystroke
#define CN_F2                532     // container received F2 keystroke
#define CN_F3                533     // container received F3 keystroke
#define CN_F4                534     // container received F4 keystroke
#define CN_F5                535     // container received F5 keystroke
#define CN_F6                536     // container received F6 keystroke
#define CN_F7                537     // container received F7 keystroke
#define CN_F8                538     // container received F8 keystroke
#define CN_F9                539     // container received F9 keystroke
#define CN_F10               540     // container received F10 keystroke
#define CN_F11               541     // container received F11 keystroke
#define CN_F12               542     // container received F12 keystroke
#define CN_F13               543     // container received F13 keystroke
#define CN_F14               544     // container received F14 keystroke
#define CN_F15               545     // container received F15 keystroke
#define CN_F16               546     // container received F16 keystroke
#define CN_F17               547     // container received F17 keystroke
#define CN_F18               548     // container received F18 keystroke
#define CN_F19               549     // container received F19 keystroke
#define CN_F20               550     // container received F20 keystroke
#define CN_F21               551     // container received F21 keystroke
#define CN_F22               552     // container received F22 keystroke
#define CN_F23               553     // container received F23 keystroke
#define CN_F24               554     // container received F24 keystroke
#define CN_TTLBTNCLK         555     // clicked on container title button
#define CN_FLDTTLBTNCLK      556     // clicked on field title button
#define CN_VSCROLL_TOP       557     // container received vert scroll to top
#define CN_VSCROLL_BOTTOM    558     // container received vert scroll to bottom
#define CN_VSCROLL_PAGEUP    559     // container received vert page up
#define CN_VSCROLL_PAGEDOWN  560     // container received vert page down
#define CN_VSCROLL_LINEUP    561     // container received vert line up
#define CN_VSCROLL_LINEDOWN  562     // container received vert line down
#define CN_VSCROLL_THUMBPOS  563     // container received vert thumb position
#define CN_HSCROLL_PAGEUP    564     // container received horz page up   (left)
#define CN_HSCROLL_PAGEDOWN  565     // container received horz page down (right)
#define CN_HSCROLL_LINEUP    566     // container received horz line up   (left)
#define CN_HSCROLL_LINEDOWN  567     // container received horz line down (right)
#define CN_HSCROLL_THUMBPOS  568     // container received horz thumb position
#define CN_FLDSIZED          569     // a field is being dynamically resized
#define CN_FLDMOVED          570     // a field is being dynamically moved  
#define CN_FLDSELECTED       571     // a field has been selected (CTS_BLOCKSEL only)
#define CN_FLDUNSELECTED     572     // a field has been unselected (CTS_BLOCKSEL only)
#define CN_SPLITBAR_CREATED  573     // a split bar was created
#define CN_SPLITBAR_MOVED    574     // a split bar was moved  
#define CN_SPLITBAR_DELETED  575     // a split bar was deleted
#define CN_NEWRECSELECTLIST  576     // starting a new record selection list (CTS_EXTENDEDSEL only)
#define CN_BGNRECSELECTION   577     // beginning a record selection (CTS_EXTENDEDSEL only)
#define CN_ENDRECSELECTION   578     // ending a record selection    (CTS_EXTENDEDSEL only)
#define CN_NEWFLDSELECTLIST  579     // starting a new field selection list (CTS_BLOCKSEL only)
#define CN_BGNFLDSELECTION   580     // beginning a field selection (CTS_BLOCKSEL only)
#define CN_ENDFLDSELECTION   581     // ending a field selection    (CTS_BLOCKSEL only)
#define CN_LK_ARROW_UP       582     // up arrow key pressed while focus was locked
#define CN_LK_ARROW_DOWN     583     // down arrow key pressed while focus was locked
#define CN_LK_ARROW_LEFT     584     // left arrow key pressed while focus was locked
#define CN_LK_ARROW_RIGHT    585     // right arrow key pressed while focus was locked
#define CN_LK_HOME           586     // home key pressed while focus was locked
#define CN_LK_END            587     // end key pressed while focus was locked
#define CN_LK_PAGEUP         588     // pageup key pressed while focus was locked
#define CN_LK_PAGEDOWN       589     // pageup key pressed while focus was locked
#define CN_LK_NEWFOCUS       590     // user clicked on a new record/field while focus record or field was locked
#define CN_LK_NEWFOCUSREC    591     // user clicked on a new record while focus record was locked
#define CN_LK_NEWFOCUSFLD    592     // user clicked on a new field while focus field was locked
#define CN_LK_VS_TOP         593     // trying to vert scroll to top while focus record was locked
#define CN_LK_VS_BOTTOM      594     // trying to vert scroll to bottom while focus record was locked
#define CN_LK_VS_PAGEUP      595     // trying to vert scroll page up while focus record was locked
#define CN_LK_VS_PAGEDOWN    596     // trying to vert scroll page down while focus record was locked
#define CN_LK_VS_LINEUP      597     // trying to vert scroll line up while focus record was locked
#define CN_LK_VS_LINEDOWN    598     // trying to vert scroll line down while focus record was locked
#define CN_LK_VS_THUMBPOS    599     // trying to vert scroll thumb position while focus record was locked
#define CN_LK_HS_PAGEUP      600     // trying to horz scroll page up   (left)  while focus field was locked
#define CN_LK_HS_PAGEDOWN    601     // trying to horz scroll page down (right) while focus field was locked
#define CN_LK_HS_LINEUP      602     // trying to horz scroll line up   (left)  while focus field was locked
#define CN_LK_HS_LINEDOWN    603     // trying to horz scroll line down (right) while focus field was locked
#define CN_LK_HS_THUMBPOS    604     // trying to horz scroll thumb position while focus field was locked
#define CN_OWNERSETFOCUSTOP  605     // associate should call CntTopFocusSet
#define CN_OWNERSETFOCUSBOT  606     // associate should call CntBotFocusSet
#define CN_CUT               607     // container received a WM_CUT   message
#define CN_COPY              608     // container received a WM_COPY  message
#define CN_PASTE             609     // container received a WM_PASTE message
#define CN_CLEAR             610     // container received a WM_CLEAR message
#define CN_UNDO              611     // container received a WM_UNDO  message
#define CN_RBTNCLK           612     // container received a right button click
#define CN_VSCROLL_THUMBTRK  613     // container received vertical thumb track message
#define CN_FLDSIZECHANGED    614     // a field display width has been changed
#define CN_SIZECHANGED       615     // container width and/or height has changed
#define CN_BACK              616     // container received backspace key
#define CN_RECDBLCLK         617     // record in text or name view was double clicked upon
#define CN_RORECDBLCLK       618     // read-only record in text or name view was double clicked upon
#define CN_INITDRAG          619     // drag operation has started
#define CN_DRAGOVER          620     // container has received a WM_CNTDRAGOVER message from another container
#define CN_DROP              621     // some object was dropped on a container
#define CN_DROPCOMPLETE      622     // target of drop has completed processing dropped object
#define CN_DRAGLEAVE         623     // object being dragged has left our window
#define CN_DRAGCOMPLETE      624     // drag operation has completed
#define CN_RENDERDATA        625     // container received wm_cntrenderdata message
#define CN_RENDERCOMPLETE    626     // container received wm_cntrendercomplete message
#define CN_HSCROLL_TOP       627     // container received horz scroll to left
#define CN_HSCROLL_BOTTOM    628     // container received horz scroll to right
#define CN_LK_HS_TOP         629     // trying to horz scroll to left while focus record was locked
#define CN_LK_HS_BOTTOM      630     // trying to horz scroll to right while focus record was locked
#define CN_DRAGCTRLDOWN      631     // ctrl key is pressed during drag operation
#define CN_DRAGCTRLUP        632     // ctrl key is released during drag operation
#define CN_DRAGSHFTDOWN      633     // shift key is pressed during drag operation
#define CN_DRAGSHFTUP        634     // shift key is released during drag operation
#define CN_DONTSETFOCUSREC   635     // sent before focus record set by adjusting vertical scroll
#define CN_ICONPOSCHANGE     636     // icon view only - icon has been moved

#define LAST_CN_MSG          3000    // last CN_* notification value

/*--------------------------------------------------------------------------*
 * Container color indices (passed thru CntColorSet/Get calls).
 *--------------------------------------------------------------------------*/

#define CNTCOLOR_TITLE       0       // container title color
#define CNTCOLOR_FLDTITLES   1       // field title color
#define CNTCOLOR_TEXT        2       // container text color
#define CNTCOLOR_GRID        3       // container grid lines color
#define CNTCOLOR_CNTBKGD     4       // container background color
#define CNTCOLOR_HIGHLIGHT   5       // selected text bkgrnd color
#define CNTCOLOR_HITEXT      6       // selected text color
#define CNTCOLOR_TTLBKGD     7       // container title background color
#define CNTCOLOR_FLDTTLBKGD  8       // field title background color
#define CNTCOLOR_FLDBKGD     9       // field data background color
#define CNTCOLOR_3DHIGH      10      // highlight color used for 3D look
#define CNTCOLOR_3DSHADOW    11      // shadow color used for 3D look
#define CNTCOLOR_TTLBTNTXT   12      // title button text color
#define CNTCOLOR_TTLBTNBKGD  13      // title button background color
#define CNTCOLOR_FLDBTNTXT   14      // field button text color
#define CNTCOLOR_FLDBTNBKGD  15      // field button background color
#define CNTCOLOR_UNUSEDBKGD  16      // unused field space color (right of the last field)
#define CNTCOLOR_INUSE       17      // in-use hatching color

#define CNTCOLORS            18      // total settable colors


/*--------------------------------------------------------------------------*
 * Container Drag/Drop message return codes
 *--------------------------------------------------------------------------*/
#define DOR_NODROP      0
#define DOR_DROP        1
#define DOR_NODROPOP    2
#define DOR_NEVERDROP   3

#define DO_COPY         0x0010    // 16
#define DO_DEFAULT      0xBFFE    // 49150
#define DO_LINK         0x0018    // 24
#define DO_MOVE         0x0020    // 32
#define DO_UNKNOWN      0xBFFF    // 49151


/*--------------------------------------------------------------------------*
 * Container font setting flags (passed thru CntFontSet/Get calls).
 *--------------------------------------------------------------------------*/

#define CF_GENERAL           0       // general container font
#define CF_TITLE             1       // container title font
#define CF_FLDTITLE          2       // field title font

/*--------------------------------------------------------------------------*
 * Container cursor setting flags (passed thru CntCursorSet calls).
 *--------------------------------------------------------------------------*/

#define CC_GENERAL           0       // general container cursor
#define CC_TITLE             1       // container title area cursor
#define CC_FLDTITLE          2       // field title area cursor

/*--------------------------------------------------------------------------*
 * Container focus moving flags (passed thru CntFocusMove calls).
 *--------------------------------------------------------------------------*/

#define CFM_LEFT             0       // move focus left
#define CFM_RIGHT            1       // move focus right
#define CFM_UP               2       // move focus up
#define CFM_DOWN             3       // move focus down
#define CFM_PAGEUP           4       // move focus 1 page up
#define CFM_PAGEDOWN         5       // move focus 1 page down
#define CFM_FIRSTFLD         6       // move focus to first field
#define CFM_LASTFLD          7       // move focus to last  field
#define CFM_HOME             8       // move focus to first record
#define CFM_END              9       // move focus to last  record
#define CFM_NEXTSPLITWND     10      // move focus to next split child window

/*--------------------------------------------------------------------------*
 * Container title and field title button placement flags.
 * (passed thru CntTtlBtnSet/CntFldTtlBtnSet calls)
 *--------------------------------------------------------------------------*/

#define CB_LEFT              0       // place button on left side
#define CB_RIGHT             1       // place button on right side

/*--------------------------------------------------------------------------*
 * Container field Auto Sizing methods (passed thru CntFldAutoSize calls).
 *--------------------------------------------------------------------------*/

#define AS_AVGCHAR           1       // nbr of characters x avg char width 
#define AS_MAXCHAR           2       // nbr of characters x max char width
#define AS_PIXELS            3       // string is measured in pixels

/*--------------------------------------------------------------------------*
 * Container background painting flags
 *--------------------------------------------------------------------------*/

#define BK_GENERAL           0       // general container bkgd
#define BK_UNUSED            1       // unused container bkgd
#define BK_TITLE             2       // container title area bkgd
#define BK_FLDTITLE          3       // field title area bkgd
#define BK_FLD               4       // field data area bkgd

/*--------------------------------------------------------------------------*
 * Container Split Bar flags
 *--------------------------------------------------------------------------*/

#define CSB_SHOW             0       // show the draggable split bar
#define CSB_LEFT             1       // create split bar on left side
#define CSB_MIDDLE           2       // create split bar in the middle
#define CSB_RIGHT            3       // create split bar on right side
#define CSB_XCOORD           4       // create split bar at passed in coordinate
#define CSB_FIRST            5       // future use
#define CSB_LAST             6       // future use
#define CSB_NEXT             7       // future use
#define CSB_PREV             8       // future use

/*--------------------------------------------------------------------------*
 * Container Line Spacing flags.
 *--------------------------------------------------------------------------*/

#define CA_LS_NONE           0       // no extra space added to line height
#define CA_LS_NARROW         1       // narrow line spacing (1/4 char ht added)
#define CA_LS_MEDIUM         2       // medium line spacing (1/2 char ht added)
#define CA_LS_WIDE           3       // wide   line spacing (3/4 char ht added)
#define CA_LS_DOUBLE         4       // double line spacing ( 1  char ht added)

/*--------------------------------------------------------------------------*
 * Container Edit control IDs (associate to use when creating edit controls)
 *--------------------------------------------------------------------------*/

#define CE_ID_EDIT1          1001
#define CE_ID_EDIT2          1002
#define CE_ID_EDIT3          1003
#define CE_ID_EDIT4          1004
#define CE_ID_EDIT5          1005
#define CE_ID_EDIT6          1006
#define CE_ID_EDIT7          1007
#define CE_ID_EDIT8          1008
#define CE_ID_EDIT9          1009
#define CE_ID_EDIT10         1010
#define CE_ID_EDIT11         1011
#define CE_ID_EDIT12         1012

/*--------------------------------------------------------------------------*
 * Container view attributes.
 *--------------------------------------------------------------------------*/

#define CV_ICON              0x0001      // icon view (default)
#define CV_NAME              0x0002      // name view
#define CV_TEXT              0x0004      // text view
#define CV_DETAIL            0x0008      // detail view
#define CV_MINI              0x0010      // valid with icon or name views
#define CV_FLOW              0x0020      // valid with text or name views

/*--------------------------------------------------------------------------*
 * Container Text Alignment flags.
 *--------------------------------------------------------------------------*/

#define CA_TA_TOP            0x00000001  // top justify
#define CA_TA_VCENTER        0x00000002  // vertical center
#define CA_TA_BOTTOM         0x00000004  // bottom justify
#define CA_TA_LEFT           0x00000008  // left justify
#define CA_TA_HCENTER        0x00000010  // horizontal center
#define CA_TA_RIGHT          0x00000020  // right justify

/*--------------------------------------------------------------------------*
 * General container attributes.
 *--------------------------------------------------------------------------*/

#define CA_TTLREADONLY       0x00000001  // title cannot be edited directly
#define CA_TITLE             0x00000002  // container has a title
#define CA_FLDTITLES         0x00000004  // cnt has field titles (detail view only)
#define CA_OWNERPNTBK        0x00000008  // owner will paint cnt background
#define CA_OWNERPNTUNBK      0x00000010  // owner will paint cnt unused background
#define CA_TTLSEPARATOR      0x00000020  // horz title separator line
#define CA_FLDSEPARATOR      0x00000040  // horz field title separator line (detail view only)
#define CA_RECSEPARATOR      0x00000080  // horz record separator line (detail view only)
#define CA_DRAWBMP           0x00000100  // use bitmaps for name, icon, or detail view
#define CA_DRAWICON          0x00000200  // use icons for name, icon, or detail view
#define CA_TTLBTNPRESSED     0x00000400  // title button is pressed (internal use only)
#define CA_TITLE3D           0x00000800  // cnt title has 3d look (detail view only)
#define CA_FLDTTL3D          0x00001000  // field titles have 3d look (detail view only)
#define CA_VERTFLDSEP        0x00002000  // all fields have vert separator line (detail view only)
#define CA_TRANTTLBMP        0x00004000  // title bmp has transparent bkgrnd
#define CA_TRANTTLBTNBMP     0x00008000  // title button bmp has transparent bkgrnd
#define CA_OWNERVSCROLL      0x00010000  // application will manage record scrolling
#define CA_SIZEABLEFLDS      0x00020000  // all container fields are dynamically sizeable
#define CA_MOVEABLEFLDS      0x00040000  // all container fields are dynamically moveable
#define CA_APPSPLITABLE      0x00080000  // only the application can create/delete splitbars
#define CA_OWNERPNTTTLBK     0x00100000  // owner will paint cnt title background
#define CA_WIDEFOCUSRECT     0x00200000  // focus rect spans the container (like a listbox)
#define CA_HSCROLLBYCHAR     0x00400000  // do horz scrolling by character (instead of field)
#define CA_WANTVTHUMBTRK     0x00800000  // send CN_VSCROLL_THUMBTRK notifications
#define CA_DYNAMICVSCROLL    0x01000000  // do vertical scrolling while scroll thumb is being moved
#define CA_SHVERTSCROLL      0x02000000  // always show the vertical scrollbar (32 BIT ONLY!)
/* reserve the msb for a set of second attributes, if we get to 31 attributes, we
   can pair an extra 32 with the msb
 */

/*--------------------------------------------------------------------------*
 * Container field attributes.
 *--------------------------------------------------------------------------*/

#define CFA_FLDREADONLY      0x00000001  // field data cannot be edited directly
#define CFA_FLDTTLREADONLY   0x00000002  // field title cannot be edited directly
#define CFA_HORZSEPARATOR    0x00000004  // horz field title separator line (detail view only)
#define CFA_VERTSEPARATOR    0x00000008  // vert field separator line (after this field)
#define CFA_CURSORED         0x00000010  // field has the focus rect
#define CFA_FLDTTL3D         0x00000020  // field title has 3d look (detail view only)
#define CFA_TRANFLDTTLBMP    0x00000040  // field title bmp has transparent bkgrnd
#define CFA_OWNERDRAW        0x00000080  // owner will draw field data
#define CFA_HEX              0x00000100  // display field types WORD,UINT,INT,DWORD, or LONG in hex format
#define CFA_OCTAL            0x00000200  // display field types WORD,UINT,INT,DWORD, or LONG in octal format
#define CFA_BINARY           0x00000400  // display field types WORD,UINT,INT,DWORD, or LONG in binary format
#define CFA_SCIENTIFIC       0x00000800  // display field types FLOAT or DOUBLE in scientific notation
#define CFA_TRANFLDBTNBMP    0x00001000  // field title button bmp has transparent bkgrnd
#define CFA_FLDBTNPRESSED    0x00002000  // field title button is pressed (internal use only)
#define CFA_SIZEABLEFLD      0x00004000  // field width is dynamically sizeable
#define CFA_MOVEABLEFLD      0x00008000  // field is dynamically moveable
#define CFA_NONSIZEABLEFLD   0x00010000  // field width NOT is dynamically sizeable
#define CFA_NONMOVEABLEFLD   0x00020000  // field is NOT dynamically moveable
#define CFA_OWNERPNTFTBK     0x00040000  // owner will paint field title background
#define CFA_OWNERPNTFLDBK    0x00080000  // owner will paint field background

/*--------------------------------------------------------------------------*
 * Container field data types.
 *--------------------------------------------------------------------------*/

#define CFT_STRING           0           // character data (default)
#define CFT_LPSTRING         1           // far pointer to char data
#define CFT_WORD             2           // unsigned short
#define CFT_UINT             3           // unsigned int
#define CFT_INT              4           // signed int
#define CFT_DWORD            5           // unsigned long
#define CFT_LONG             6           // signed long
#define CFT_FLOAT            7           // float
#define CFT_DOUBLE           8           // double
#define CFT_BCD              9           // binary coded data
#define CFT_DATE             10          // date data (use CDATE struct)
#define CFT_TIME             11          // time data (use CTIME struct)
#define CFT_BMP              12          // bitmap data
#define CFT_ICON             13          // icon data
#define CFT_CUSTOM           18          // custom (hooked)
#define CFT_CHAR             19          // single character
 
/*--------------------------------------------------------------------------*
 * Container record attributes.
 *--------------------------------------------------------------------------*/

#define CRA_RECREADONLY      0x00000001  // record cannot be edited directly
#define CRA_CURSORED         0x00000002  // record has the focus rect
#define CRA_DROPONABLE       0x00000004  // record can be target for direct manipulation
#define CRA_FILTERED         0x00000008  // record has been filtered
#define CRA_FLDSELECTED      0x00000010  // one or more record fields are selected
#define CRA_SELECTED         0x00000020  // record is selected
#define CRA_TARGET           0x00000040  // target emphasis
#define CRA_FIRSTREC         0x00000080  // 1st record in total list
#define CRA_LASTREC          0x00000100  // last record in total list
#define CRA_INUSE            0x00000200  // record is in-use
#define CRA_DRAGINUSE        0x00000400  // record is being dragged

/*--------------------------------------------------------------------------*
 * flags used for CntRecSetIconEx - tells what the image is that user is passing in
 *--------------------------------------------------------------------------*/
#define CNTIMAGE_ICON        0
#define CNTIMAGE_MINIICON    1
#define CNTIMAGE_BMP         2
#define CNTIMAGE_MINIBMP     3

/*--------------------------------------------------------------------------*
 * Arrange Icons attributes
 *--------------------------------------------------------------------------*/
#define CAI_FIXED            0x00000001  // width and height of icon space is defined by user
#define CAI_LARGEST          0x00000002  // width and height of icon space determined by largest text string
#define CAI_FIXEDWIDTH       0x00000004  // width defined by user - height by largest text string
#define CAI_WORDWRAP         0x00000008  // Can be or'ed with FIXED and FIXEDWIDTH to wrap text so it stays within the rect

typedef struct tagARRANGEICONINFO
{
  int aiType;      // CAI_FIXED, CAI_FIXEDWIDTH, CAI_LARGEST, CAI_WORDWRAP (or'ed with fixed types)
  int nHeight;     // height of icon record (FIXED only)
  int nWidth;      // width of icon record (FIXED and FIXED WIDTH)
  int nHorSpacing; // horizontal spacing between icon records when arranged
  int nVerSpacing; // vertical spacing between icon records when arranged
  int xIndent;     // amount of space to indent from left when arranged
  int yIndent;     // amount of space to indent from top when arranged
} ARRANGEICONINFO;

/*--------------------------------------------------------------------------*
 * Record image transparency structure - in name view and icon view, user
 * can set the bitmaps to be transparent - defines transparency point and 
 * flags whether to use transparency or not.  Note that icons can have a
 * transparent color internally defined so you don't really need to use
 * this if you are using icons.
 *--------------------------------------------------------------------------*/
typedef struct tagRECTRANSPINFO
{
  POINT ptIcon;          // point on the icon that defines background color
  POINT ptMiniIcon;      // point on the mini-icon that defines background color
  POINT ptBmp;           // point on the bitmap that defines background color
  POINT ptMiniBmp;       // point on the mini-bitmap that defines background color
  BOOL  fIconTransp;     // if TRUE, icon has transparent background
  BOOL  fMiniIconTransp; // if TRUE, mini-icon has transparent background
  BOOL  fBmpTransp;      // if TRUE, bitmap has transparent background
  BOOL  fMiniBmpTransp;  // if TRUE, mini-bitmap has transparent background
} RECTRANSPINFO;

typedef RECTRANSPINFO      *PRECTRANSPINFO;
typedef RECTRANSPINFO FAR *LPRECTRANSPINFO;

#define LEN_RECTRANSPINFO sizeof(RECTRANSPINFO)

/*--------------------------------------------------------------------------*
 * Container field structure - one per field in the container.
 *--------------------------------------------------------------------------*/

typedef unsigned short UINT16;   // type for 16 bit unsigned quantities

#define MAX_BTNTXT_LEN      64   // max len of title or field button text

// Prototype for conversion function used for CFT_CUSTOM fields.
typedef int (CALLBACK* CVTPROC)( HWND, LPVOID, LPVOID, UINT, LPSTR );

// Prototype for drawing function used for CFA_OWNERDRAW fields.
typedef int (CALLBACK* DRAWPROC)( HWND, struct tagFIELDINFO FAR *,
          struct tagRECORDCORE FAR *,
          LPVOID, HDC, int, int, UINT, 
          LPRECT, LPSTR, UINT );

typedef struct tagFIELDINFO
    {
    struct tagFIELDINFO FAR *lpNext;   // pointer to next field
    struct tagFIELDINFO FAR *lpPrev;   // pointer to previous field
    DWORD    dwFieldInfoLen;   // length of FIELDINFO stuct
    DWORD    flColAttr;        // field attribute flags
    UINT     wColType;         // field data type
    UINT     wIndex;           // field index (user reference only)
    UINT     cxWidth;          // display width of the field in characters
    UINT     cxPxlWidth;       // display width of the field in pixels
    LPVOID   lpFTitle;         // field title string
    UINT     wTitleLen;        // field title length in bytes
    UINT     wFTitleLines;     // nbr of lines in field title (if string)
    DWORD    flFTitleAlign;    // field title alignment flags
    DWORD    flFDataAlign;     // field data alignment flags
    HBITMAP  hBmpFldTtl;       // bitmap for field title area
    DWORD    flFTBmpAlign;     // field title bitmap alignment flags
    int      xOffFldTtlBmp;    // horz offset for field title bitmap
    int      yOffFldTtlBmp;    // vert offset for field title bitmap
    int      xTPxlFldTtlBmp;   // xpos of transparent bkgrnd pixel for field title bitmap 
    int      yTPxlFldTtlBmp;   // ypos of transparent bkgrnd pixel for field title bitmap 
    int      nFldBtnWidth;     // width of field button in chars (if pos) or pixels (if neg)
    BOOL     bFldBtnAlignRt;   // if true field button is on right (else left)
    char     szFldBtnTxt[MAX_BTNTXT_LEN];  // text for field button
    DWORD    flFldBtnTxtAlign; // field button text alignment flags
    HBITMAP  hBmpFldBtn;       // bitmap for field button
    DWORD    flFldBtnBmpAlign; // field button bitmap alignment flags
    int      xOffFldBtnBmp;    // horz offset for field button bitmap
    int      yOffFldBtnBmp;    // vert offset for field button bitmap
    int      xTPxlFldBtnBmp;   // xpos of transparent bg pixel for field btn bitmap 
    int      yTPxlFldBtnBmp;   // ypos of transparent bg pixel for field btn bitmap 
    UINT     xEditWidth;       // edit width of the field in characters
    UINT     yEditLines;       // nbr of lines to use when editing
    UINT     wOffStruct;       // offset from lpRecData to field data
    UINT     wDataBytes;       // nbr of bytes used for storing data
    LPVOID   lpUserData;       // user defined field data
    UINT     wUserBytes;       // size of user defined field data
    LPVOID   lpDescriptor;     // user data for converting custom data
    UINT     wDescBytes;       // bytes for the descriptor
    BOOL     bDescEnabled;     // flag indicating whether descriptor is enabled
    CVTPROC  lpfnCvtToStr;     // function for converting custom data to strings
    DRAWPROC lpfnDrawData;     // function for drawing CFA_OWNERDRAW fields
    BOOL     bClrFldText;      // flag for text color for this field
    COLORREF crFldText;        // text color for this field
    BOOL     bClrFldBk;        // flag for background color for this field
    COLORREF crFldBk;          // background color for this field
    BOOL     bClrFldTtlText;   // flag for title text color for this field
    COLORREF crFldTtlText;     // title text color for this field
    BOOL     bClrFldTtlBk;     // flag for title background color for this field
    COLORREF crFldTtlBk;       // title background color for this field
    BOOL     bClrFldBtnText;   // flag for field button text color for this field
    COLORREF crFldBtnText;     // field button text color for this field
    BOOL     bClrFldBtnBk;     // flag for field button background color for this field
    COLORREF crFldBtnBk;       // field button background color for this field
    } FIELDINFO;

typedef FIELDINFO      *PFIELDINFO;
typedef FIELDINFO FAR *LPFIELDINFO;

#define LEN_FIELDINFO sizeof(FIELDINFO)

/*--------------------------------------------------------------------------*
 * Container selected field structure (used when CTS_BLOCKSEL is enabled).
 *--------------------------------------------------------------------------*/

typedef struct tagSELECTEDFLD
    {
    struct tagSELECTEDFLD FAR *lpNext; // pointer to next selected field node
    struct tagSELECTEDFLD FAR *lpPrev; // pointer to prev selected field node
    LPFIELDINFO lpFld;                 // pointer to container field
    LPVOID      lpUserData;            // future: user defined data
    UINT        wUserBytes;            // future: size of user defined data
    } SELECTEDFLD;

typedef SELECTEDFLD      *PSELECTEDFLD;
typedef SELECTEDFLD FAR *LPSELECTEDFLD;

#define LEN_SELECTEDFLD sizeof(SELECTEDFLD)

/*--------------------------------------------------------------------------*
 * Container record structure - one per item in the container.
 *--------------------------------------------------------------------------*/

typedef struct tagRECORDCORE
    {
    struct tagRECORDCORE FAR *lpNext;  // pointer to next record
    struct tagRECORDCORE FAR *lpPrev;  // pointer to previous record
    LPSTR    lpszText;         // text for TEXT view
    LPSTR    lpszName;         // text for NAME view
    LPSTR    lpszIcon;         // text for ICON view
    DWORD    flRecAttr;        // record attributes
    LPVOID   lpRecData;        // pointer to record data for all fields
    DWORD    dwRecSize;        // size of record data
    POINT    ptIconOrg;        // upper left origin for CV_ICON item
    HICON    hIcon;            // icon   to display when CV_MINI is not set
    HICON    hMiniIcon;        // icon   to display when CV_MINI is set
    HBITMAP  hBmp;             // bitmap to display when CV_MINI is not set
    HBITMAP  hMiniBmp;         // bitmap to display when CV_MINI is set
    LPVOID   lpUserData;       // user defined data
    UINT     wUserBytes;       // size of user defined data
    BOOL     bClrRecText;      // flag for text color for this record
    COLORREF crRecText;        // text color for this record
    BOOL     bClrRecBk;        // flag for background color for this record
    DWORD    dwDragId;         // drag index for record being dragged
    COLORREF crRecBk;          // background color for this record
    LPSELECTEDFLD lpSelFldHead; // pointer to head of selected field list
    LPSELECTEDFLD lpSelFldTail; // pointer to tail of selected field list
    LPRECTRANSPINFO lpTransp;  // pointer to transparency structure for record images
    } RECORDCORE;

typedef RECORDCORE      *PRECORDCORE;
typedef RECORDCORE FAR *LPRECORDCORE;

#define LEN_RECORDCORE sizeof(RECORDCORE)

/*--------------------------------------------------------------------------*
 * Container icon z-order - used in determining the z-order of the records 
 * (icon view - icons are painted in the order of the z-order)
 *--------------------------------------------------------------------------*/
typedef struct tagZORDERREC
{
    struct tagZORDERREC FAR *lpNext; // pointer to next selected field node
    struct tagZORDERREC FAR *lpPrev; // pointer to prev selected field node
    LPRECORDCORE lpRec;              // pointer to container record
//    RECT         rcText;             // rectangle bounding the text
} ZORDERREC;

typedef ZORDERREC      *PZORDERREC;
typedef ZORDERREC FAR *LPZORDERREC;

#define LEN_ZORDERREC sizeof(ZORDERREC)

/*--------------------------------------------------------------------------*
 * Container info structure - one per container.
 *--------------------------------------------------------------------------*/

// Prototype for background painting function used for containers with the
// CA_OWNERPNTBK, CA_OWNERPNTUNBK, CA_OWNERPNTTTLBK, or CFA_OWNERPNTFTBK
// attribute enabled.
typedef int (CALLBACK* PAINTPROC)( HWND, LPFIELDINFO, HDC, LPRECT, UINT );

#define MAX_SPLITBARS    20    // maximum of nbr splitbars per container

typedef struct tagCNTINFO
    {
    DWORD    dwCntInfoLen;     // length of CNTINFO stuct
    DWORD    flCntAttr;        // general container attributes
    UINT     fViewAttr;        // container view attributes
    HWND     hAuxWnd;          // Auxiliary window handle
    HFONT    hFont;            // handle to general container font
    HFONT    hFontTitle;       // handle to container title font
    HFONT    hFontColTitle;    // handle to field title font
    HCURSOR  hCursor;          // handle to general container cursor
    HCURSOR  hCurTtl;          // handle to title area cursor
    HCURSOR  hCurFldTtl;       // handle to field title area cursor 
    HCURSOR  hCurTtlBtn;       // handle to title button cursor
    HCURSOR  hCurFldTtlBtn;    // handle to field title button cursor 
    HCURSOR  hCurFldSizing;    // handle to field sizing cursor 
    HCURSOR  hCurFldMoving;    // handle to field moving cursor 
    HCURSOR  hDropNotAllow;    // drop not allowed cursor for drag and drop
    HCURSOR  hDropCopySingle;  // drop cursor for single copy drag object
    HCURSOR  hDropCopyMulti;   // drop cursor for multiple copy drag objects
    HCURSOR  hDropMoveSingle;  // drop cursor for single move drag ojbect
    HCURSOR  hDropMoveMulti;   // drop cursor for multiple move drag objects
    LPVOID   lpUserData;       // user defined data
    UINT     wUserBytes;       // size of user defined data
    POINT    ptOrigin;         // upper left origin of the container
    UINT     wLineSpacing;     // container row line spacing flag
    LPSTR    lpszTitle;        // container title
    DWORD    flTitleAlign;     // container title alignment flags
    UINT     wTitleLines;      // nbr of lines for the title area
    UINT     wColTitleLines;   // nbr of lines for the field title area
    UINT     wRowLines;        // nbr of lines per data row
    int      cyTitleArea;      // client area used for the container title
    int      cyColTitleArea;   // client area used for the field titles
    int      nMaxWidth;        // max scrollable width of container in pixels
    HBITMAP  hBmpTtl;          // bitmap for title area
    int      xOffTtlBmp;       // horz offset for title bitmap
    int      yOffTtlBmp;       // vert offset for title bitmap
    int      xTPxlTtlBmp;      // xpos of transparent bkgrnd pixel for title bitmap 
    int      yTPxlTtlBmp;      // ypos of transparent bkgrnd pixel for title bitmap 
    DWORD    flTtlBmpAlign;    // title bitmap alignment flags
    int      nTtlBtnWidth;     // width of the title button in chars (if pos) or pixels (if neg)
    BOOL     bTtlBtnAlignRt;   // if true title button is on right (else left)
    char     szTtlBtnTxt[MAX_BTNTXT_LEN];  // text for title button
    DWORD    flTtlBtnTxtAlign; // title button text alignment flags
    HBITMAP  hBmpTtlBtn;       // bitmap for title button
    DWORD    flTtlBtnBmpAlign; // title button bitmap alignment flags
    int      xOffTtlBtnBmp;    // horz offset for title button bitmap
    int      yOffTtlBtnBmp;    // vert offset for title button bitmap
    int      xTPxlTtlBtnBmp;   // xpos of transparent bg pixel for title btn bitmap 
    int      yTPxlTtlBtnBmp;   // ypos of transparent bg pixel for title btn bitmap 
    int      cxCharTtl;        // avg char width of title font
    int      cxCharTtl2;       // half char width of title font
    int      cyCharTtl;        // char height of title font
    int      cyCharTtl2;       // half char height of title font
    int      cyCharTtl4;       // quarter char height of title font
    int      cxCharFldTtl;     // avg char width of field title font
    int      cxCharFldTtl2;    // half char width of field title font
    int      cyCharFldTtl;     // char height of field title font
    int      cyCharFldTtl2;    // half char height of field title font
    int      cyCharFldTtl4;    // quarter char height of field title font
    int      cxChar;           // avg char width of general font
    int      cxChar2;          // half char width of general font
    int      cxChar4;          // quarter char width of general font
    int      cyChar;           // char height of general font
    int      cyChar2;          // half char height of general font
    int      cyLineSpace;      // value added to cyChar to make row height
    int      cyRow;            // record row height
    int      cyRow2;           // record row half height
    int      cyRow4;           // record row quarter height
    int      nDelta;           // scrolling limit from either end of list
    int      nDeltaPos;        // starting position for delta calculation
    POINT    ptBmpOrIcon;      // pixel size of bitmap or icon
    int      nFieldNbr;        // nbr of fields in the container
    int      nRecsDisplayed;   // nbr of records in display area
    DWORD    dwRecordNbr;      // nbr of records currently in the list
    UINT     wCurNotify;       // current notification value
    UINT     wCurAnsiChar;     // current ansi char value (for CN_CHARHIT)
    HWND     hWndCurCNSender;  // child wnd sending the current notification
    int      nCurCNInc;        // current scrolling increment
    BOOL     bCurCNShiftKey;   // state of the shift key for current notify
    BOOL     bCurCNCtrlKey;    // state of the control key for current notify
    BOOL     bFocusRecLocked;  // if TRUE focus will not move to new record
    BOOL     bFocusFldLocked;  // if TRUE focus will not move to new field
    BOOL     bSimCtrlKey;      // flag for simulating a pressed Control key
    BOOL     bSimShiftKey;     // flag for simulating a pressed Shift key
    BOOL     bSendFocusMsg;    // flag for sending focus changing notifications
    BOOL     bSendKBEvents;    // flag for sending keyboard events
    BOOL     bSendMouseEvents; // flag for sending mouse events
    BOOL     bDrawFocusRect;   // flag for drawing focus rectangle
    LPVOID   lpCurCNUser;      // current notification user data
    LPRECORDCORE lpCurCNRec;   // current notification record
    LPFIELDINFO  lpCurCNFld;   // current notification field
    LPFIELDINFO  lpFieldHead;  // pointer to beginning of field list
    LPFIELDINFO  lpFieldTail;  // pointer to end of field list
    LPFIELDINFO  lpFieldObj;   // pointer to field representing the object
    LPRECORDCORE lpRecHead;    // pointer to beginning of record list
    LPRECORDCORE lpRecTail;    // pointer to end of record list
    LPRECORDCORE lpTopRec;     // 1st record currently displayed on the screen
    LPRECORDCORE lpSelRec;     // record currently selected (single select only)
    LPRECORDCORE lpFocusRec;   // record currently has focus rectangle
    LPFIELDINFO  lpFocusFld;   // field currently has focus rectangle
    int         cxFlowSpacing; // amount of space added to columns in flowed view
    LPZORDERREC lpRecOrderHead;// pointer to head of z-order (icon view)
    LPZORDERREC lpRecOrderTail;// pointer to tail of z-order (icon view)
    BOOL        bArranged;     // flags if icons have been arranged - used for scrolling
    ARRANGEICONINFO aiInfo;    // information for arranging icons
    RECT        rcWorkSpace;   // the icon workspace rectangle
    RECT        rcViewSpace;   // the portion of the icon workspace that is visible
    } CNTINFO;

typedef CNTINFO      *PCNTINFO;
typedef CNTINFO FAR *LPCNTINFO;

#define LEN_CNTINFO sizeof(CNTINFO)

/*--------------------------------------------------------------------------*
 * Container date structure.
 *--------------------------------------------------------------------------*/

typedef struct tagCDATE
    {
    BYTE     byDay;            // current day
    BYTE     byMonth;          // current month
    UINT     wYear;            // current year (fully specified as in 1992) 
    } CDATE;

typedef CDATE      *PCDATE;
typedef CDATE FAR *LPCDATE;

#define LEN_CDATE sizeof(CDATE)

/*--------------------------------------------------------------------------*
 * Container time structure.
 *--------------------------------------------------------------------------*/

typedef struct tagCTIME
    {
    BYTE     byHour;           // current hour
    BYTE     byMin;            // current minute
    BYTE     bySec;            // current second
    } CTIME;

typedef CTIME      *PCTIME;
typedef CTIME FAR *LPCTIME;

#define LEN_CTIME sizeof(CTIME)

/*--------------------------------------------------------------------------*
 * Container drag and drop data custom messages
 *--------------------------------------------------------------------------*/
// these are the custom message that the container will register with the system
#define WM_CNTDRAGOVER        "WM_CNTDRAGOVER"
#define WM_CNTDROP            "WM_CNTDROP"
#define WM_CNTDRAGLEAVE       "WM_CNTDRAGLEAVE"
#define WM_CNTDROPCOMPLETE    "WM_CNTDROPCOMPLETE"
#define WM_CNTRENDERCOMPLETE  "WM_CNTRENDERCOMPLETE"
#define WM_CNTRENDERDATA      "WM_CNTRENDERDATA"


/*--------------------------------------------------------------------------*
 * Container drag info structures and defines
 *--------------------------------------------------------------------------*/
typedef struct tagDRAGINFO
{
  DWORD nDragInfo;
  DWORD nDragItem;
  UINT  uDragOp;
  HWND  hwndSource;
  int   xDrop;
  int   yDrop;
  char  fName[_MAX_FNAME];
  DWORD nItems;
  int   nDropIndex;
}CDRAGINFO;

typedef CDRAGINFO FAR *LPCDRAGINFO;

#define LEN_DRAGINFO sizeof(CDRAGINFO)

/*--------------------------------------------------------------------------*
 * Container drag item structures and defines
 *--------------------------------------------------------------------------*/
#define MAX_TYPE          128
#define MAX_RMF           128
#define MAX_CONTAINERNAME 128
#define MAX_NAME          128

  typedef struct tagDRAGITEM
  {
  HWND hwndItem;
  DWORD dwItemId;
  char szType[MAX_TYPE];
  char szRmf[MAX_RMF];
  char szContName[MAX_CONTAINERNAME];
  char szSourceName[MAX_NAME];
  char szTargetName[MAX_NAME];
  int  cxOffset;
  int  cyOffset;
  UINT uControl;
  UINT uSupportedOps;
  }DRAGITEM;

typedef  DRAGITEM FAR *LPDRAGITEM;

#define LEN_DRAGITEM sizeof(DRAGITEM)


/*--------------------------------------------------------------------------*
 * Container drag init data structures and defines
 *--------------------------------------------------------------------------*/
typedef struct tagCNTDRAGINIT
{
  LPRECORDCORE lpRecord;    // pointer to record where drag started
  int   x;                  // X-coordinate of the cursor in screen coordinates
  int   y;                  // Y-coordinate of the cursor in screen coordinates
  int   cx;                 // X-offset from the cursor to the record origin
  int   cy;                 // Y-offset from the cursor to the record origin
  DWORD nIndex;             // unique id for this drag operation
}CNTDRAGINIT;

typedef  CNTDRAGINIT FAR *LPCNTDRAGINIT;

#define LEN_CNTDRAGINIT sizeof(CNTDRAGINIT)

/*--------------------------------------------------------------------------*
 * Container drag info data structures and defines
 *--------------------------------------------------------------------------*/

typedef struct tagCNTDRAGINFO
{
  LPCDRAGINFO lpDragInfo;     // pointer to draginfo structure
  LPRECORDCORE lpRecord;    // pointer to record where drag started
}CNTDRAGINFO;

typedef CNTDRAGINFO FAR *LPCNTDRAGINFO;
#define LEN_CNTDRAGINFO sizeof(CNTDRAGINFO)

/*--------------------------------------------------------------------------*
 * Container drag transfer structures and defines
 *--------------------------------------------------------------------------*/

  typedef struct tagCNTDRAGTRANSFER
  {
  DWORD       nSize;                    // size of data structure
  HWND              hwndClient;               // handle of target requesting rendering
  DWORD           dwItem;                   // index to item to be rendered
  DWORD     dwDropIndex;              // index to draginfo struct that was dropped on target
  char              szSelectedRMF[MAX_RMF];   // string describing rendering format requested
  char              szRenderName[MAX_RMF];    // name of shared memory area for rendering WIN32 only
  LPVOID    lpRenderAddr;             // address for rendered data WIN16 only
  UINT              uOperation;               // operation to be performed
  UINT              uReply;
  }CNTDRAGTRANSFER;

typedef CNTDRAGTRANSFER FAR *LPCNTDRAGTRANSFER;
#define LEN_CNTDRAGTRANSFER sizeof(CNTDRAGTRANSFER)

/*--------------------------------------------------------------------------*
 * Container control API Functions
 *--------------------------------------------------------------------------*/

LPCNTINFO WINAPI CntInfoGet( HWND hWnd );
WORD     WINAPI CntGetVersion( void );
HWND     WINAPI CntAssociateSet( HWND hWnd, HWND hWndAssociate );
HWND     WINAPI CntAssociateGet( HWND hWnd );
HWND     WINAPI CntAuxWndSet( HWND hWnd, HWND hAuxWnd );
HWND     WINAPI CntAuxWndGet( HWND hWnd );
BOOL     WINAPI CntRangeExSet( HWND hWnd, LONG lMin, LONG lMax );
DWORD    WINAPI CntRangeSet( HWND hWnd, UINT iMin, UINT iMax );
LONG     WINAPI CntRangeMinGet( HWND hWnd );
LONG     WINAPI CntRangeMaxGet( HWND hWnd );
DWORD    WINAPI CntRangeGet( HWND hWnd );
DWORD    WINAPI CntRangeInc( HWND hWnd );
DWORD    WINAPI CntRangeDec( HWND hWnd );
LONG     WINAPI CntCurrentPosExSet( HWND hWnd, LONG lPos );
LONG     WINAPI CntCurrentPosExGet( HWND hWnd );
int      WINAPI CntCurrentPosSet( HWND hWnd, int iPos );
int      WINAPI CntCurrentPosGet( HWND hWnd );
COLORREF WINAPI CntColorSet( HWND hWnd, UINT iColor, COLORREF cr );
COLORREF WINAPI CntColorGet( HWND hWnd, UINT iColor );
BOOL     WINAPI CntColorReset( HWND hWnd );
COLORREF WINAPI CntFldColorSet( HWND hWnd, LPFIELDINFO lpFld, UINT iColor, COLORREF cr );
COLORREF WINAPI CntFldColorGet( HWND hWnd, LPFIELDINFO lpFld, UINT iColor );
COLORREF WINAPI CntRecColorSet( HWND hWnd, LPRECORDCORE lpRec, UINT iColor, COLORREF cr );
COLORREF WINAPI CntRecColorGet( HWND hWnd, LPRECORDCORE lpRec, UINT iColor );
BOOL     WINAPI CntTtlSet( HWND hWnd, LPSTR lpszTitle );
LPSTR    WINAPI CntTtlGet( HWND hWnd );
VOID     WINAPI CntTtlAlignSet( HWND hWnd, DWORD dwAlign );
VOID     WINAPI CntFldTtlAlnSet( HWND hWnd, LPFIELDINFO lpFld, DWORD dwAlign );
VOID     WINAPI CntFldDataAlnSet( HWND hWnd, LPFIELDINFO lpFld, DWORD dwAlign );
VOID     WINAPI CntTtlSepSet( HWND hWndCnt );
VOID     WINAPI CntFldTtlSepSet( HWND hWndCnt );
BOOL     WINAPI CntFldTtlSet( HWND hWnd, LPFIELDINFO lpFld, LPSTR lpszColTitle, UINT wTitleLen );
LPSTR    WINAPI CntFldTtlGet( HWND hWnd, LPFIELDINFO lpFld );
BOOL     WINAPI CntCursorSet( HWND hWnd, HCURSOR hCursor, UINT iArea );
BOOL     WINAPI CntFontSet( HWND hWnd, HFONT hFont, UINT iFont );
HFONT    WINAPI CntFontGet( HWND hWnd, UINT iFont );
UINT     WINAPI CntViewSet( HWND hWnd, UINT iView );
UINT     WINAPI CntViewGet( HWND hWnd );
VOID     WINAPI CntDeferPaint( HWND hWnd );
VOID     WINAPI CntEndDeferPaint( HWND hWnd, BOOL bUpdate );
int      WINAPI CntDeferPaintEx( HWND hWnd );
int      WINAPI CntEndDeferPaintEx( HWND hWnd, BOOL bUpdate );
HWND     WINAPI CntFrameWndGet( HWND hWnd );
HWND     WINAPI CntSpltChildWndGet( HWND hWnd, UINT uPane );
VOID     WINAPI CntSpltBarCreate( HWND hCntWnd, UINT wMode, int xCoord );
VOID     WINAPI CntSpltBarDelete( HWND hCntWnd, UINT wMode, int xCoord );
VOID     WINAPI CntAttribSet( HWND hWnd, DWORD dwAttrib );
VOID     WINAPI CntAttribClear( HWND hWnd, DWORD dwAttrib );
VOID     WINAPI CntFldAttrSet( LPFIELDINFO lpFI, DWORD dwAttrib );
VOID     WINAPI CntFldAttrClear( LPFIELDINFO lpFI, DWORD dwAttrib );
DWORD    WINAPI CntFldAttrGet( HWND hCntWnd, LPFIELDINFO lpFld );
VOID     WINAPI CntRecAttrSet( LPRECORDCORE lpRec, DWORD dwAttrib );
DWORD    WINAPI CntRecAttrGet( LPRECORDCORE lpRec );
VOID     WINAPI CntRecAttrClear( LPRECORDCORE lpRec, DWORD dwAttrib );
DWORD    WINAPI CntTotalRecsGet( HWND hCntWnd );
UINT     WINAPI CntLineSpaceGet( HWND hCntWnd );
VOID     WINAPI CntStyleSet( HWND hWnd, DWORD dwStyle );
DWORD    WINAPI CntStyleGet( HWND hWnd );
VOID     WINAPI CntStyleClear( HWND hWnd, DWORD dwStyle );
VOID     WINAPI CntSizeCheck( HWND hWndCnt, int cxWidth, int cyHeight, int cxMaxWidth,
                              LPINT lpcxUnused, LPINT lpcyPartial,
                              LPINT lpbHscroll, LPINT lpbVscroll );
VOID     WINAPI CntRetainBaseHt( HWND hWndCnt, BOOL bRetain );
LPVOID   WINAPI CntUserDataSet( HWND hCntWnd, LPVOID lpUserData, UINT wUserBytes );
LPVOID   WINAPI CntUserDataGet( HWND hCntWnd );
UINT     WINAPI CntNotifyMsgGet( HWND hWnd, LPARAM lParam );
VOID     WINAPI CntNotifyMsgDone( HWND hWnd, LPARAM lParam );
VOID     WINAPI CntNotifyAssocEx( HWND hWnd, UINT wEvent, UINT wOemCharVal,
                                  LPRECORDCORE lpRec, LPFIELDINFO lpFld, LONG lInc, 
                                  BOOL bShiftKey, BOOL bCtrlKey, LPVOID lpUserData );
VOID     WINAPI CntNotifyAssoc( HWND hWnd, UINT wEvent, UINT wOemCharVal,
                                LPRECORDCORE lpRec, LPFIELDINFO lpFld, int nInc, 
                                BOOL bShiftKey, BOOL bCtrlKey, LPVOID lpUserData );
HWND     WINAPI CntCNChildWndGet( HWND hCntWnd, LPARAM lParam );
UINT     WINAPI CntCNCharGet( HWND hCntWnd, LPARAM lParam );
LONG     WINAPI CntCNIncExGet( HWND hCntWnd, LPARAM lParam );
int      WINAPI CntCNIncGet( HWND hCntWnd, LPARAM lParam );
int      WINAPI CntCNThumbTrkGet( HWND hCntWnd, LPARAM lParam );
LPRECORDCORE WINAPI CntCNRecGet( HWND hCntWnd, LPARAM lParam );
LPFIELDINFO  WINAPI CntCNFldGet( HWND hCntWnd, LPARAM lParam );
LPVOID   WINAPI CntCNUserDataGet( HWND hCntWnd, LPARAM lParam );
BOOL     WINAPI CntCNShiftKeyGet( HWND hCntWnd, LPARAM lParam );
BOOL     WINAPI CntCNCtrlKeyGet( HWND hCntWnd, LPARAM lParam );
int      WINAPI CntCNSplitBarGet( HWND hCntWnd, LPARAM lParam );
BOOL     WINAPI CntFldDefine( LPFIELDINFO lpFI, UINT wFldType, DWORD flColAttr,
                              UINT wFTitleLines, DWORD flFTitleAlign,
                              DWORD flFDataAlign, UINT xWidth,
                              UINT xEditWidth, UINT yEditLines,
                              UINT wOffStruct, UINT wDataBytes );
BOOL     WINAPI CntFldUserSet( LPFIELDINFO lpFI, LPVOID lpUserData, UINT wUserBytes );
LPVOID   WINAPI CntFldUserGet( LPFIELDINFO lpFld );
BOOL     WINAPI CntFldDataSet( LPRECORDCORE lpRec, LPFIELDINFO lpFld, UINT wBufSize, LPVOID lpBuf );
BOOL     WINAPI CntFldDataGet( LPRECORDCORE lpRec, LPFIELDINFO lpFld, UINT wBufSize, LPVOID lpBuf );
BOOL     WINAPI CntTtlBmpSet( HWND hCntWnd, HBITMAP hBmpTtl,
            DWORD flBmpAlign, int xOffBmp, int yOffBmp,
            int xTransPxl, int yTransPxl, BOOL bTransparent );
BOOL     WINAPI CntFldTtlBmpSet( LPFIELDINFO lpFI, HBITMAP hBmpFldTtl,
                                 DWORD flFTBmpAlign, int xOffBmp, int yOffBmp,
                                 int xTransPxl, int yTransPxl, BOOL bTransparent );
BOOL     WINAPI CntTtlBtnSet( HWND hCntWnd, int nBtnWidth, int nBtnPlacement,
                              LPSTR lpszText, DWORD flTxtAlign, HBITMAP hBitmap,
                              DWORD flBmpAlign, int xOffBmp, int yOffBmp,
                              int xTransPxl, int yTransPxl, BOOL bTransparent );
BOOL     WINAPI CntFldTtlBtnSet( LPFIELDINFO lpFI, int nBtnWidth, int nBtnPlacement,
                                 LPSTR lpszText, DWORD flTxtAlign, HBITMAP hBitmap,
                                 DWORD flBmpAlign, int xOffBmp, int yOffBmp,
                                 int xTransPxl, int yTransPxl, BOOL bTransparent );
BOOL     WINAPI CntRecDataSet( LPRECORDCORE lpRec, LPVOID lpData );
LPVOID   WINAPI CntRecDataGet( HWND hCntWnd, LPRECORDCORE lpRec, LPVOID lpBuf );
BOOL     WINAPI CntRecUserSet( LPRECORDCORE lpRec, LPVOID lpUserData, UINT wUserBytes );
LPVOID   WINAPI CntRecUserGet( LPRECORDCORE lpRecord );
BOOL     WINAPI CntFreeFldInfo( LPFIELDINFO lpField );
BOOL     WINAPI CntFreeRecCore( LPRECORDCORE lpRecord );
BOOL     WINAPI CntAddFldTail( HWND hCntWnd, LPFIELDINFO lpNew );
BOOL     WINAPI CntAddRecTail( HWND hCntWnd, LPRECORDCORE lpNew );
BOOL     WINAPI CntAddFldHead( HWND hCntWnd, LPFIELDINFO lpNew );
BOOL     WINAPI CntAddRecHead( HWND hCntWnd, LPRECORDCORE lpNew );
BOOL     WINAPI CntInsFldAfter( HWND hCntWnd, LPFIELDINFO lpField, LPFIELDINFO lpNew );
BOOL     WINAPI CntInsRecAfter( HWND hCntWnd, LPRECORDCORE lpRecord, LPRECORDCORE lpNew );
BOOL     WINAPI CntInsFldBefore( HWND hCntWnd, LPFIELDINFO lpField, LPFIELDINFO lpNew );
BOOL     WINAPI CntInsRecBefore( HWND hCntWnd, LPRECORDCORE lpRecord, LPRECORDCORE lpNew );
BOOL     WINAPI CntMoveRecAfter( HWND hCntWnd, LPRECORDCORE lpRecord, LPRECORDCORE lpRecMov );
BOOL     WINAPI CntKillFldList( HWND hCntWnd );
BOOL     WINAPI CntKillRecList( HWND hCntWnd );
LPFIELDINFO   WINAPI CntFldHeadGet(HWND hCntWnd);
LPRECORDCORE  WINAPI CntRecHeadGet(HWND hCntWnd);
LPFIELDINFO   WINAPI CntFldTailGet(HWND hCntWnd);
LPRECORDCORE  WINAPI CntRecTailGet(HWND hCntWnd);
LPFIELDINFO   WINAPI CntNewFldInfo( void );
LPRECORDCORE  WINAPI CntNewRecCore( DWORD dwRecSize );
LPFIELDINFO   WINAPI CntRemoveFldHead( HWND hCntWnd );
LPRECORDCORE  WINAPI CntRemoveRecHead( HWND hCntWnd );
LPFIELDINFO   WINAPI CntRemoveFldTail( HWND hCntWnd );
LPRECORDCORE  WINAPI CntRemoveRecTail( HWND hCntWnd );
LPFIELDINFO   WINAPI CntRemoveFld( HWND hCntWnd, LPFIELDINFO lpField );
LPRECORDCORE  WINAPI CntRemoveRec( HWND hCntWnd, LPRECORDCORE lpRecord );
LPFIELDINFO   WINAPI CntScrollFldList( HWND hCntWnd, LPFIELDINFO lpField, LONG lDelta );
LPRECORDCORE  WINAPI CntScrollRecList( HWND hCntWnd, LPRECORDCORE lpRecord, LONG lDelta );
LPFIELDINFO   WINAPI CntNextFld( LPFIELDINFO lpField );
LPRECORDCORE  WINAPI CntNextRec( LPRECORDCORE lpRecord );
LPFIELDINFO   WINAPI CntPrevFld( LPFIELDINFO lpField );
LPRECORDCORE  WINAPI CntPrevRec( LPRECORDCORE lpRecord );

LPRECORDCORE  WINAPI CntTopRecGet( HWND hCntWnd );
BOOL          WINAPI CntTopRecSet( HWND hCntWnd, LPRECORDCORE lpRec );
LPRECORDCORE  WINAPI CntBotRecGet( HWND hCntWnd );
int           WINAPI CntRecsDispGet( HWND hCntWnd );
LPRECORDCORE  WINAPI CntScrollRecAreaEx( HWND hCntWnd, LONG lIncrement );
LPRECORDCORE  WINAPI CntScrollRecArea( HWND hCntWnd, int nIncrement );
VOID          WINAPI CntVScrollPosExSet( HWND hCntWnd, LONG lPosition );
VOID          WINAPI CntVScrollPosSet( HWND hCntWnd, int nPosition );
VOID          WINAPI CntScrollFldArea( HWND hCntWnd, int nIncrement );
LPRECORDCORE  WINAPI CntSelRecGet( HWND hCntWnd );
BOOL          WINAPI CntSelectRec( HWND hCntWnd, LPRECORDCORE lpRec );
BOOL          WINAPI CntUnSelectRec( HWND hCntWnd, LPRECORDCORE lpRec );
BOOL          WINAPI CntIsRecSelected( HWND hCntWnd, LPRECORDCORE lpRec );
BOOL          WINAPI CntSelectFld( HWND hCntWnd, LPRECORDCORE lpRec, LPFIELDINFO lpFld );
BOOL          WINAPI CntUnSelectFld( HWND hCntWnd, LPRECORDCORE lpRec, LPFIELDINFO lpFld );
BOOL          WINAPI CntIsFldSelected( HWND hCntWnd, LPRECORDCORE lpRec, LPFIELDINFO lpFld );
LPFIELDINFO   WINAPI CntFldAtPtGet( HWND hCntWnd, LPPOINT lpPt );
LPRECORDCORE  WINAPI CntRecAtPtGet( HWND hCntWnd, LPPOINT lpPt );
VOID          WINAPI CntKeyBdEnable( HWND hCntWnd, BOOL bEnable );
VOID          WINAPI CntMouseEnable( HWND hCntWnd, BOOL bEnable );
BOOL          WINAPI CntFocusMove( HWND hCntWnd, UINT wDir );
BOOL          WINAPI CntIsFocusCellRO( HWND hCntWnd );
BOOL          WINAPI CntFocusSet( HWND hCntWnd, LPRECORDCORE lpRecord, LPFIELDINFO lpFld  );
BOOL          WINAPI CntTopFocusSet( HWND hCntWnd );
BOOL          WINAPI CntBotFocusSet( HWND hCntWnd );
VOID          WINAPI CntFocRectEnable( HWND hCntWnd, BOOL bEnable );
VOID          WINAPI CntFocMsgEnable( HWND hCntWnd, BOOL bEnable );
VOID          WINAPI CntFocusScrollEnable( HWND hCntWnd, BOOL bEnable );
LPRECORDCORE  WINAPI CntFocusRecGet( HWND hCntWnd );
LPFIELDINFO   WINAPI CntFocusFldGet( HWND hCntWnd );
int           WINAPI CntFocusFldLock( HWND hCntWnd );
int           WINAPI CntFocusRecLock( HWND hCntWnd );
int           WINAPI CntFocusFldUnlck( HWND hCntWnd );
int           WINAPI CntFocusRecUnlck( HWND hCntWnd );

BOOL          WINAPI CntFocusOrgExGet( HWND hCntWnd, LPINT lpnXorg, LPINT lpnYorg, BOOL bScreen );
DWORD         WINAPI CntFocusOrgGet( HWND hCntWnd, BOOL bScreen );
BOOL          WINAPI CntFocusExtExGet( HWND hCntWnd, LPINT lpnXext, LPINT lpnYext );
DWORD         WINAPI CntFocusExtGet( HWND hCntWnd );
BOOL          WINAPI CntFldTtlOrgExGet( HWND hCntWnd, LPFIELDINFO lpFld, LPINT lpnXorg, 
                                        LPINT lpnYorg, BOOL bScreen );
DWORD         WINAPI CntFldTtlOrgGet( HWND hCntWnd, LPFIELDINFO lpFld, BOOL bScreen );
BOOL          WINAPI CntFldEditExtExGet( HWND hCntWnd, LPFIELDINFO lpFld, LPINT lpnXext, LPINT lpnYext );
DWORD         WINAPI CntFldEditExtGet( HWND hCntWnd, LPFIELDINFO lpFld );
BOOL          WINAPI CntFldBytesExGet( HWND hCntWnd, LPFIELDINFO lpFld, UINT FAR *lpuBytes, UINT FAR *lpuOffset );
DWORD         WINAPI CntFldBytesGet( HWND hCntWnd, LPFIELDINFO lpFld );
BOOL          WINAPI CntEndRecEdit( HWND hCntWnd, LPRECORDCORE lpRecord, LPFIELDINFO lpFld );
VOID          WINAPI CntUpdateRecArea( HWND hCntWnd, LPRECORDCORE lpRecord, LPFIELDINFO lpFld );
VOID          WINAPI CntDeltaExSet( HWND hCntWnd, LONG lDelta );
LONG          WINAPI CntDeltaExGet( HWND hCntWnd );
VOID          WINAPI CntDeltaSet( HWND hCntWnd, int nDelta );
int           WINAPI CntDeltaGet( HWND hCntWnd );
VOID          WINAPI CntDeltaPosExSet( HWND hCntWnd, LONG lDeltaPos );
LONG          WINAPI CntDeltaPosExGet( HWND hCntWnd );
VOID          WINAPI CntDeltaPosSet( HWND hCntWnd, int nDeltaPos );
int           WINAPI CntDeltaPosGet( HWND hCntWnd );
UINT          WINAPI CntFldTypeGet( HWND hCntWnd, LPFIELDINFO lpFld );
BOOL          WINAPI CntFldCvtProcSet( LPFIELDINFO lpFI, CVTPROC lpfnCvtProc );
BOOL          WINAPI CntFldDescSet( LPFIELDINFO lpFI, LPVOID lpDesc, UINT wDescBytes );
VOID          WINAPI CntFldDescEnable( LPFIELDINFO lpFI, BOOL bEnable );
BOOL          WINAPI CntFldDrwProcSet( LPFIELDINFO lpFI, DRAWPROC lpfnDrawProc );
int           WINAPI CntFldWidthGet( HWND hCntWnd, LPFIELDINFO lpFld, BOOL bPixels );
int           WINAPI CntFldWidthSet( HWND hCntWnd, LPFIELDINFO lpFld, UINT nWidth );
UINT          WINAPI CntLastFldExpand( HWND hCntWnd, int nExtra );
int           WINAPI CntFldAutoSize( HWND hCntWnd, LPFIELDINFO lpFld, UINT uMethod,
                                     int nExtra, int nMinFldWidth, int nMaxFldWidth );
int           WINAPI CntRowHtSet( HWND hCntWnd, int nHeight, UINT wLineSpace );
int           WINAPI CntRowHtGet( HWND hCntWnd );
int           WINAPI CntTtlHtSet( HWND hCntWnd, int nHeight );
int           WINAPI CntTtlHtGet( HWND hCntWnd );
int           WINAPI CntFldTtlHtSet( HWND hCntWnd, int nHeight );
int           WINAPI CntFldTtlHtGet( HWND hCntWnd );
LPFIELDINFO   WINAPI CntFldByIndexGet( HWND hCntWnd, UINT wIndex );
VOID          WINAPI CntFldIndexSet( LPFIELDINFO lpFI, UINT wIndex );
UINT          WINAPI CntFldIndexGet( LPFIELDINFO lpFI );
BOOL          WINAPI CntPaintProcSet( HWND hCntWnd, PAINTPROC lpfnPaintProc );

LPSELECTEDFLD WINAPI CntNewSelFld( void );
BOOL          WINAPI CntFreeSelFld( LPSELECTEDFLD lpSelField );
LPSELECTEDFLD WINAPI CntSelFldHeadGet( LPRECORDCORE lpRecord );
LPSELECTEDFLD WINAPI CntSelFldTailGet( LPRECORDCORE lpRecord );
BOOL          WINAPI CntAddSelFldHead( LPRECORDCORE lpRecord, LPSELECTEDFLD lpNew );
BOOL          WINAPI CntAddSelFldTail( LPRECORDCORE lpRecord, LPSELECTEDFLD lpNew );
LPSELECTEDFLD WINAPI CntRemSelFldHead( LPRECORDCORE lpRecord );
LPSELECTEDFLD WINAPI CntRemSelFldTail( LPRECORDCORE lpRecord );
LPSELECTEDFLD WINAPI CntRemSelFld( LPRECORDCORE lpRecord, LPSELECTEDFLD lpSelField );
BOOL          WINAPI CntInsSelFldBfor( LPRECORDCORE lpRecord, LPSELECTEDFLD lpSelField, LPSELECTEDFLD lpNew );
BOOL          WINAPI CntInsSelFldAftr( LPRECORDCORE lpRecord, LPSELECTEDFLD lpSelField, LPSELECTEDFLD lpNew );
BOOL          WINAPI CntKillSelFldLst( LPRECORDCORE lpRecord );
LPSELECTEDFLD WINAPI CntNextSelFld( LPSELECTEDFLD lpSelField );
LPSELECTEDFLD WINAPI CntPrevSelFld( LPSELECTEDFLD lpSelField );
LPSELECTEDFLD WINAPI CntFindSelFld( LPRECORDCORE lpRecord, LPFIELDINFO lpField );
BOOL          WINAPI CntSelFldUserSet( LPSELECTEDFLD lpSF, LPVOID lpUserData, UINT wUserBytes );
LPVOID        WINAPI CntSelFldUserGet( LPSELECTEDFLD lpSelFld );

BOOL          WINAPI CntNameSetEx( LPRECORDCORE lpRC, LPSTR lpszText, LPSTR lpszName, LPSTR lpszIcon );
BOOL          WINAPI CntRecIconSetEx(LPRECORDCORE lpRC, HANDLE hImage, int fImageType, 
                                     BOOL fUseTransp, int xTrans, int yTrans);
BOOL          WINAPI CntSetFlowedHorSpacing( HWND hCntWnd, int nWidth );
BOOL          WINAPI CntIconArrangeAttrSet( HWND hCntWnd, int aiType, int nWidth, int nHeight,
              int nHorSpace, int nVerSpace, int xIndent, int yIndent );
BOOL          WINAPI CntIconArrangeAttrGet( HWND hCntWnd, int *aiType, int *nWidth, int *nHeight,
              int *nHorSpace, int *nVerSpace, int *xIndent, int *yIndent );
BOOL          WINAPI CntArrangeIcons( HWND hCntWnd );

LPCNTDRAGINIT WINAPI CntDragInitInfoGet( HWND hCntWnd);
VOID WINAPI CntSetDragCursors(HWND hCntWnd, HCURSOR hDropNotAllowed, HCURSOR hCopySingle, 
                              HCURSOR hCopyMulti, HCURSOR hMoveSingle, HCURSOR hMoveMulti);
int           WINAPI CntDragInfoSet(HWND hCntWnd, LPCDRAGINFO lpDragInfo);
LPCDRAGINFO   WINAPI CntDragInfoGet(HWND hCntWnd,int nIndex);
int           WINAPI CntDragIndexGet(HWND hCntWnd);
LPCDRAGINFO   WINAPI CntDragTargetInfoGet(HWND hCntWnd);
LPRECORDCORE  WINAPI CntDragOverRecGet(HWND hCntWnd);
LPRECORDCORE  WINAPI CntDropOverRecGet(HWND hCntWnd);
LPCNTDRAGTRANSFER WINAPI CntDragTransferInfoGet(HWND hCntWnd);
LPCNTDRAGTRANSFER WINAPI CntRenderCompleteInfoGet(HWND hCntWnd);
DWORD         WINAPI CntSelRecCountGet(HWND hCntWnd);
LPVOID        WINAPI CntAllocDragInfo(HWND hCntWnd, DWORD dwItems );
LPDRAGITEM    WINAPI CntDragItemGet(HWND hCntWnd,LPCDRAGINFO lpDragInfo, DWORD dwItem);
VOID          WINAPI CntSetMsgValue( HWND hCntWnd, LRESULT lVal );
int           WINAPI CntFldPxlWidthSet( HWND hCntWnd, LPFIELDINFO lpFld, UINT nWidth );
int           WINAPI CntCNInputIDGet( HWND hCntWnd );
UINT          WINAPI CntGetRecordData( HWND hWndCnt, LPRECORDCORE lpRec, LPFIELDINFO lpField, 
                                       UINT wMaxLen, LPSTR lpStrData );

#ifdef  __cplusplus
}
#endif

#endif   /* end wrapper */
