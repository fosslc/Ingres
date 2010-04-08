/********************************************************************
//
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
//
//    Project : graphical tool for ingres database administrator
//
//    Source : cntutil.c
//
//        contains the code that manages a generic container window
//
//    Author : Emmanuel Blattes
//
//
********************************************************************/

//
// Includes
//
#define OEMRESOURCE   // For OBM_ bitmaps
#include <windows.h>
#include <windowsx.h>   // GetWindowFont, ...
#include <stddef.h>     // ofssetof()
#include <stdio.h>      // sprintf (for floats)

#include "main.h"
#include "dba.h"
#include "winutils.h"
#include "catocntr.h"
#include "cntutil.h"

// static container management functions
static VOID           NEAR DefineContainerColumns(HWND hwndCnt, LPGENCNTRDATA lpGenCntrData, int colnum);
static BOOL           NEAR ContainerOldSel(HWND hwndCnt, LPGENCNTRDATA lpGenCntrData);
static BOOL           NEAR SetGenericCntrData(HWND hwndCnt, LPGENCNTRDATA lpGenCntrData);
static LPGENCNTRDATA  NEAR GetGenericCntrData(HWND hwndCnt);
static VOID           NEAR UnlockGenericCntrData(HWND hwndCnt);
static VOID           NEAR RemoveGenericCntrData(HWND hwndCnt);


//---------- resources
//
//    CONTROL         "",IDC_CONTAINER,"CA_Container",
//                    WS_BORDER | WS_GROUP | WS_TABSTOP |
//                    CTS_SINGLESEL | CTS_HORZSCROLL | CTS_VERTSCROLL,
//                    10,51,249,121
//
//                    
//---------- dialog proc
//
//    case WM_INITDIALOG:
//    InitContainer(GetDlgItem(hwnd, IDC_CONTAINER), hwnd, colnum);
//    
//    
//
//    case WM_DESTROY:
//      TerminateContainer(GetDlgItem(hwnd, IDC_CONTAINER));
//
//    case WM_COMMAND:
//    if (wParam == 0)  // or id dans boite de dialogue (notification)
//      if ((HWND)(LOWORD(lParam)) == lpGenCntrData->hwndCnt) {
//        hwndCnt = LOWORD(lParam);
//        switch (HIWORD(lParam)) {
//          case CN_RECSELECTED:
//            // Selection has changed
//            ContainerNewSel(hwndCnt);
//            return TRUE;      // Processed
//
//          case CN_FLDSIZED:
//          case CN_HSCROLL_PAGEUP:
//          case CN_HSCROLL_PAGEDOWN:
//          case CN_HSCROLL_LINEUP:
//          case CN_HSCROLL_LINEDOWN:
//          case CN_HSCROLL_THUMBPOS:
//            ContainerUpdateCombo(hwndCnt);
//            return TRUE;      // Processed
//        }
//      }
//      return FALSE;   // default is not processed
//      break;

//----------------------------------------------------------------------
//
//               Generic Container management functions
//
//----------------------------------------------------------------------

//
//  Local constants
//

// horizontal and vertical margins for bottom corner of the container
#define HMARGIN 0
#define VMARGIN 0
// Special value for first line (caption line)
#define CONT_FIRSTLINE_HANDLE ((LPRECORDCORE)(-1))
// For GenericContainerCellDraw() : combo bitmap dimensions
#define BITMAPS_WIDTH   17
#define BITMAPS_HEIGHT  19

//
// To be called at dialog box creation time, on WM_INITDIALOG
// stores the data to be associated with the control in a property
//
static BOOL NEAR SetGenericCntrData(HWND hwndCnt, LPGENCNTRDATA lpGenCntrData)
{
  return HDLGSetProp(hwndCnt, (LPVOID)lpGenCntrData);
}

//
//  Retrieves the data stored by SetGenericCntrData
//
static LPGENCNTRDATA NEAR GetGenericCntrData(HWND hwndCnt)
{
  return (LPGENCNTRDATA)HDLGGetProp(hwndCnt);
}

// To be called if we have made a GetGenericCntrData and we stay in the box
// for a while...
static VOID NEAR UnlockGenericCntrData(HWND hwndCnt)
{
  HDLGUnlockProp(hwndCnt);
}

//
// To be called at dialog box exit time
// frees the memory and the property allocated by SetGenericCntrData
//
static VOID NEAR RemoveGenericCntrData(HWND hwndCnt)
{
  HDLGRemoveProp(hwndCnt);
}

//
//  Utility : Says whether the column number is the last column
//
static BOOL NEAR IsLastColumn(HWND hwndCnt, int colnum)
{
  LPGENCNTRDATA lpGenCntrData;
  BOOL          bRet;

  lpGenCntrData = GetGenericCntrData(hwndCnt);
  if (!lpGenCntrData)
    return FALSE;
  if (colnum < lpGenCntrData->columns - 1)
    bRet = FALSE;
  else
    bRet = TRUE;
  UnlockGenericCntrData(hwndCnt);
  return bRet;
}

//
// Utility: returns the LPFIELDINFO for a cell, having received
//  the hLine and the column number
//
static LPFIELDINFO NEAR QueryLpFld(HWND hwndCnt, LPRECORDCORE hLine, int colnum)
{
  LPGENCNTRDATA   lpGenCntrData;
  LPFIELDINFO     lpFld;
  GENCNTR_COLUMN  column;
  int             cpt;

  lpGenCntrData = GetGenericCntrData(hwndCnt);
  if (!lpGenCntrData)
    return 0;

  // Special management for first line
  if (hLine == CONT_FIRSTLINE_HANDLE) {
    // get a lpFld on the searched column : assume ordered
    // according to column numbers
    lpFld = CntFldHeadGet(hwndCnt);
    for (cpt=1; cpt<=colnum; cpt++)
      lpFld = CntNextFld(lpFld);
  }
  else {
    lpFld = CntFldHeadGet(hwndCnt);
    while (lpFld) {
      CntFldDataGet(hLine, lpFld, 0, (LPVOID)&column);
      if (column.colnum == colnum && column.hLine == hLine)
        break;
      lpFld = CntNextFld(lpFld);
    }
  }

  UnlockGenericCntrData(hwndCnt);
  return lpFld;
}

//
//  BOF...
//
// Set caption to the container
//
void ContainerSetCaption(HWND hwndCnt, char *szCaption)
{
  SetWindowText(hwndCnt, szCaption);
}

//
// Disable display function, for flash avoid purposes
//
void ContainerDisableDisplay(HWND hwndCnt)
{
  CntDeferPaint(hwndCnt);
}

//
// Enables display function, for flash avoid purposes
// if bUpdate is TRUE, container is repainted immediately
//
void ContainerEnableDisplay(HWND hwndCnt, BOOL bUpdate)
{
  CntEndDeferPaint(hwndCnt, bUpdate);
}

//
// Creates a line and returns a line number
// creates the default cells, all alpha with a dummy text
//
int ContainerAddCntLine(HWND hwndCnt)
{
  LPGENCNTRDATA     lpGenCntrData;
  int               columns;
  LPGENCNTR_RECORD  lpCr;   // Dynamic alloc
  LPRECORDCORE      lpRC;
  int               unit;
  int               cpt;
  LPGENCNTR_COLUMN  lpCol;  // advances in lpCr

  lpGenCntrData = GetGenericCntrData(hwndCnt);
  if (!lpGenCntrData)
    return -1;

  if (!lpGenCntrData->lines) {
    (lpGenCntrData->lines)++;
    UnlockGenericCntrData(hwndCnt);
    return 0;   //  special value ---> caption line
  }

  columns = lpGenCntrData->columns;
  unit = offsetof(GENCNTR_RECORD, cont_col2);
  lpCr = (LPGENCNTR_RECORD)ESL_AllocMem( unit * columns );
  if (!lpCr) {
    UnlockGenericCntrData(hwndCnt);
    return -1;
  }

  CntDeferPaint( hwndCnt );   // Disable container paint

  lpRC = CntNewRecCore(unit * columns); // Don't be wrong in giving the size!

  // Fill the reccore with default values
  for (cpt = 0, lpCol = (LPGENCNTR_COLUMN)lpCr;
       cpt < columns;
       cpt++, lpCol++) {      // Note pointer arithmetic on lpCol!
    lpCol->dummy[0]  = 'A';
    lpCol->dummy[1]  = '\0';
    lpCol->type       = CONT_COLTYPE_EMPTY;
    lpCol->align      = CONT_ALIGN_LEFT;
    lpCol->precision  = 0;
    lpCol->ucol.s[0]  = '\0';

    // Store data for future calls to QueryLpFld()
    lpCol->colnum = cpt;    // Column number
    lpCol->hLine  = lpRC;   // LPRECORDCORE identifying the line
  }

  // Load data into the record.
  CntRecDataSet(lpRC, (LPVOID)lpCr);

  // Add this record to the list.
  CntAddRecTail(hwndCnt, lpRC);

  CntSelectRec(hwndCnt,lpRC);           // ???

  // MASQUED BECAUSE OF SIDE EFFECT ON SIBLINGS EDITS: 
  //CntFocusMove(hwndCnt, CFM_LASTFLD);

  // Free the allocated buffer
  ESL_FreeMem(lpCr);

  // Increment the vertical scroll range
  CntRangeInc(hwndCnt);

  CntEndDeferPaint( hwndCnt, FALSE);   // Enables container paint

  // make the recordcore array bigger, and add the new recordcore
  // using old number of lines
  if (lpGenCntrData->pLines == NULL)
    lpGenCntrData->pLines = ESL_AllocMem(sizeof(LPRECORDCORE));
  else
    lpGenCntrData->pLines = ESL_ReAllocMem(lpGenCntrData->pLines,
                            sizeof(LPRECORDCORE) * lpGenCntrData->lines,
                            0);
  if (lpGenCntrData->pLines == NULL) {
    UnlockGenericCntrData(hwndCnt);
    return -1;     // ERROR!
  }
  lpGenCntrData->pLines[lpGenCntrData->lines - 1] = lpRC;

  // update the number of lines
  (lpGenCntrData->lines)++;

  UnlockGenericCntrData(hwndCnt);

  // return the line number
  return (lpGenCntrData->lines - 1);
}

// Columns functions
// Convention : column number start from 0
// set a column width in number of characters
// returns TRUE if operation successful
BOOL ContainerSetColumnWidth(HWND hwndCnt, int colnum, int width)
{
  LPRECORDCORE    hLine;
  LPFIELDINFO     lpFld;
  GENCNTR_COLUMN  column;

  // don't change if last column
  if (IsLastColumn(hwndCnt, colnum))
    return TRUE;

  // Scan the list of Fields, and stop when requested column is found
  hLine = CntFocusRecGet(hwndCnt);    // No matter which line
  lpFld = CntFldHeadGet(hwndCnt);
  while (lpFld) {
    CntFldDataGet(hLine, lpFld, 0, (LPVOID)&column);
    if (column.colnum == colnum)
      break;
    lpFld = CntNextFld(lpFld);
  }
  if (!lpFld)
    return FALSE;

  CntFldWidthSet(hwndCnt, lpFld, (UINT)width);
  return TRUE;
}

// Cell functions
// a cell is the intersection of a column and a line.
// Line id should have been previously obtained with ContainerAddCntLine()

// utility: makes the HLine starting from the line number
static LPRECORDCORE NEAR MakeHLineFromLineNum(HWND hwndCnt, int linenum)
{
  LPGENCNTRDATA   lpGenCntrData;

  lpGenCntrData = GetGenericCntrData(hwndCnt);
  if (!lpGenCntrData)
    return 0;
  if (linenum < 0 || linenum > lpGenCntrData->lines)
    return 0;
  if (linenum > 0)
    return lpGenCntrData->pLines[linenum-1];
  else
    return CONT_FIRSTLINE_HANDLE;
}


// Fill a cell with text - coltype can be title, text or combo
// returns TRUE if operation successful
BOOL ContainerFillTextCell(HWND hwndCnt, int linenum, int colnum, int type, int align, char *txt)
{
  LPFIELDINFO     lpFld;
  GENCNTR_COLUMN  column;
  LPRECORDCORE    hLine;

  //First column management
  DWORD           dwAlign;

  hLine = MakeHLineFromLineNum(hwndCnt, linenum);

  // check parameters
  if (type != CONT_COLTYPE_TEXT && type != CONT_COLTYPE_TITLE)
    return FALSE;

  // query lpFld from hline and colnum
  lpFld = QueryLpFld(hwndCnt, hLine, colnum);
  if (!lpFld)
    return FALSE;

  // special management for caption line
  if (hLine == CONT_FIRSTLINE_HANDLE) {

    // prepare align parameter
    switch (align) {
      case CONT_ALIGN_LEFT:
        dwAlign = CA_TA_LEFT;
        break;
      case CONT_ALIGN_RIGHT:
        dwAlign = CA_TA_RIGHT;
        break;
      default:
        dwAlign = CA_TA_HCENTER;
        break;
    }
    if (IsLastColumn(hwndCnt, colnum))
      dwAlign = CA_TA_LEFT;   //Force to left if last column
    CntFldTtlAlnSet(hwndCnt, lpFld, dwAlign);
    CntFldTtlSet(hwndCnt, lpFld, txt, x_strlen(txt)+1);
    return TRUE;
  }

  // extract data from cell
  if (!CntFldDataGet(hLine, lpFld, 0, (LPVOID)&column))
    return FALSE;

  // Update the data
  column.type = type;
  if (IsLastColumn(hwndCnt, colnum))
    column.align = CONT_ALIGN_LEFT;   //Force to left if last column
  else
    column.align = align;

  fstrncpy(column.ucol.s, txt, sizeof(column.ucol.s));

  // Load back data into the record.
  if (!CntFldDataSet(hLine, lpFld, 0, (LPVOID)&column))
    return FALSE;

  // Make the record being dispayed again
  // TO BE FINISHED

  return TRUE;
}

// --- A FINIR ...

// Fill a cell with a combobox
// returns TRUE if operation successful
BOOL ContainerFillComboCell(HWND hwndCnt, int linenum, int colnum, int selection, int align, char *txt)
{
  LPFIELDINFO     lpFld;
  GENCNTR_COLUMN  column;
  LPRECORDCORE    hLine;

  //First column management
  DWORD           dwAlign;

  // Combo fill management
  char   *pItems;
  char   *p;
  UINT    len;
  int     count;

  hLine = MakeHLineFromLineNum(hwndCnt, linenum);

  // query lpFld from hline and colnum
  lpFld = QueryLpFld(hwndCnt, hLine, colnum);
  if (!lpFld)
    return FALSE;

  // special management for caption line
  if (hLine == CONT_FIRSTLINE_HANDLE) {

    // prepare align parameter
    switch (align) {
      case CONT_ALIGN_LEFT:
        dwAlign = CA_TA_LEFT;
        break;
      case CONT_ALIGN_RIGHT:
        dwAlign = CA_TA_RIGHT;
        break;
      default:
        dwAlign = CA_TA_HCENTER;
        break;
    }
    if (IsLastColumn(hwndCnt, colnum))
      dwAlign = CA_TA_LEFT;   //Force to left if last column
    CntFldTtlAlnSet(hwndCnt, lpFld, dwAlign);
    CntFldTtlSet(hwndCnt, lpFld, txt, x_strlen(txt)+1);
    return TRUE;
  }

  // extract data from cell
  if (!CntFldDataGet(hLine, lpFld, 0, (LPVOID)&column))
    return FALSE;

  // Update the data
  column.type = CONT_COLTYPE_COMBO;
  if (IsLastColumn(hwndCnt, colnum))
    column.align = CONT_ALIGN_LEFT;   //Force to left if last column
  else
    column.align = align;

  // First loop to get the number of items and the length to allocate
  p = txt;
  count = 0;
  while (*p) {
    count++;
    len = x_strlen(p) + 1;
    p += len;
  }
  len = p-txt + 1;

  // allocate and fill using previous data
  pItems = ESL_AllocMem(len);
  if (!pItems)
    return FALSE;
  memcpy(pItems, txt, len);

  // store info in the column structure
  column.ucol.sCombo.itemCount  = count;
  column.ucol.sCombo.charCount  = len;
  column.ucol.sCombo.pItems     = pItems;
  column.ucol.sCombo.selection  = selection;

  // Load back data into the record.
  if (!CntFldDataSet(hLine, lpFld, 0, (LPVOID)&column))
    return FALSE;

  // Make the record being dispayed again
  // TO BE FINISHED

  return TRUE;
}

// Sets the current selection in a combobox type cell
// Does not change the selection if item not found
BOOL ContainerSetComboSelection(HWND hwndCnt, int linenum, int  colnum, char *selection)
{
  LPFIELDINFO     lpFld;
  GENCNTR_COLUMN  column;
  int             cpt;
  char           *pItem;
  LPRECORDCORE    hLine;
  LPGENCNTRDATA   lpGenCntrData;

  lpGenCntrData = GetGenericCntrData(hwndCnt);
  if (!lpGenCntrData)
    return FALSE;

  hLine = MakeHLineFromLineNum(hwndCnt, linenum);
  if (!hLine)
    return FALSE;

  // query lpFld from hline and colnum
  lpFld = QueryLpFld(hwndCnt, hLine, colnum);
  if (!lpFld)
    return FALSE;

  // extract data from cell
  if (!CntFldDataGet(hLine, lpFld, 0, (LPVOID)&column))
    return FALSE;

  // check the cell is a combo
  if (column.type != CONT_COLTYPE_COMBO)
    return FALSE;

  // search the string in the list
  pItem = column.ucol.sCombo.pItems;  // first item
  for (cpt=0; *pItem; cpt++) {
    if (lstrcmp(pItem, selection) == 0) {
      // item found : update the numeric selection
      column.ucol.sCombo.selection = cpt;

      // Store back data into the record.
      if (!CntFldDataSet(hLine, lpFld, 0, (LPVOID)&column))
          return FALSE;

      // item found : update selection in combo if currently displayed
      // on the item
      if (lpGenCntrData->lastLpRec == hLine)
        if (GetFocus() == lpGenCntrData->hWndCombo)
            ComboBox_SetCurSel(lpGenCntrData->hWndCombo, column.ucol.sCombo.selection);

      // item found : return TRUE
      return TRUE;
    }
    pItem += x_strlen(pItem) + 1;
  }
  return FALSE;
}



// Query the current selection in a combobox type cell
// the received buffer must be of size MAXOBJECTNAME+1
BOOL ContainerGetComboCellSelection(HWND hwndCnt, int linenum, int colnum, char *buf)
{
  LPFIELDINFO     lpFld;
  GENCNTR_COLUMN  column;
  int             cpt;
  char           *pItem;
  LPRECORDCORE    hLine;

  hLine = MakeHLineFromLineNum(hwndCnt, linenum);

  // query lpFld from hline and colnum
  lpFld = QueryLpFld(hwndCnt, hLine, colnum);
  if (!lpFld)
    return FALSE;

  // extract data from cell
  if (!CntFldDataGet(hLine, lpFld, 0, (LPVOID)&column))
    return FALSE;

  // check the cell is a combo
  if (column.type != CONT_COLTYPE_COMBO)
    return FALSE;

  // return the nth string
  pItem = column.ucol.sCombo.pItems;  // first item
  for (cpt=0; cpt<column.ucol.sCombo.selection; cpt++)
    pItem += x_strlen(pItem) + 1;
  x_strcpy(buf, pItem);
  return TRUE;
}

// Fill a cell with a numeric value
// returns TRUE if operation successful
BOOL ContainerFillNumCell(HWND hwndCnt, int linenum, int colnum, int align, long ival)
{
  LPFIELDINFO     lpFld;
  GENCNTR_COLUMN  column;
  LPRECORDCORE    hLine;

  hLine = MakeHLineFromLineNum(hwndCnt, linenum);

  // special management for caption line
  if (hLine == CONT_FIRSTLINE_HANDLE)
    return FALSE;   // Not accepted at this time

  // query lpFld from hline and colnum
  lpFld = QueryLpFld(hwndCnt, hLine, colnum);
  if (!lpFld)
    return FALSE;

  // extract data from cell
  if (!CntFldDataGet(hLine, lpFld, 0, (LPVOID)&column))
    return FALSE;

  // Update the data
  column.type = CONT_COLTYPE_INT;
  if (IsLastColumn(hwndCnt, colnum))
    column.align = CONT_ALIGN_LEFT;   //Force to left if last column
  else
    column.align = align;
  column.ucol.i = ival;

  // Load back data into the record.
  if (!CntFldDataSet(hLine, lpFld, 0, (LPVOID)&column))
    return FALSE;

  // Make the record being dispayed again
  // TO BE FINISHED

  return TRUE;
}

// Fill a cell with a float value - default precision will be 2
// returns TRUE if operation successful
BOOL ContainerFillFloatCell(HWND hwndCnt, int linenum, int colnum, int align, double fval)
{
  LPFIELDINFO     lpFld;
  GENCNTR_COLUMN  column;
  LPRECORDCORE    hLine;

  hLine = MakeHLineFromLineNum(hwndCnt, linenum);

  // special management for caption line
  if (hLine == CONT_FIRSTLINE_HANDLE)
    return FALSE;   // Not accepted at this time

  // query lpFld from hline and colnum
  lpFld = QueryLpFld(hwndCnt, hLine, colnum);
  if (!lpFld)
    return FALSE;

  // extract data from cell
  if (!CntFldDataGet(hLine, lpFld, 0, (LPVOID)&column))
    return FALSE;

  // Update the data
  column.type = CONT_COLTYPE_FLOAT;
  if (IsLastColumn(hwndCnt, colnum))
    column.align = CONT_ALIGN_LEFT;   //Force to left if last column
  else
    column.align = align;
  column.ucol.f = fval;

  // Load back data into the record.
  if (!CntFldDataSet(hLine, lpFld, 0, (LPVOID)&column))
    return FALSE;

  // Make the record being dispayed again
  // TO BE FINISHED

  return TRUE;
}

// set the float value display precision for a cell
// returns TRUE if operation successful
BOOL ContainerSetFloatCellPrec(HWND hwndCnt, int linenum, int colnum, int precision)
{
  LPFIELDINFO     lpFld;
  GENCNTR_COLUMN  column;
  LPRECORDCORE    hLine;

  hLine = MakeHLineFromLineNum(hwndCnt, linenum);

  // special management for caption line
  if (hLine == CONT_FIRSTLINE_HANDLE)
    return FALSE;   // Not accepted at this time

  // query lpFld from hline and colnum
  lpFld = QueryLpFld(hwndCnt, hLine, colnum);
  if (!lpFld)
    return FALSE;

  // extract data from cell
  if (!CntFldDataGet(hLine, lpFld, 0, (LPVOID)&column))
    return FALSE;

  // Update the data
  if (column.type != CONT_COLTYPE_FLOAT)
    return FALSE;
  column.precision = precision;

  // Load back data into the record.
  if (!CntFldDataSet(hLine, lpFld, 0, (LPVOID)&column))
    return FALSE;

  // Make the record being dispayed again
  // TO BE FINISHED

  return TRUE;
}


//------------------------------------------------------------------------
//
//    Windows container specific functions
//
//------------------------------------------------------------------------

int CALLBACK GenericContainerCellDraw( HWND hCntWnd, LPFIELDINFO lpFld,
                                       LPRECORDCORE lpRec, LPVOID lpData,
                                       HDC hDC, int nXstart, int nYstart,
                                       UINT fuOptions, LPRECT lpRect,
                                       LPSTR lpszOut, UINT uLenOut);

/****************************************************************************
 *  drawing function for CFA_OWNERDRAW fields.
 *
 *  This function is called when the container has converted the field
 *  data to string format and is ready to draw it to the display screen.
 *  The application would probably do some kind of range check here
 *  and change the text color and/or background color accordingly.
 *  ExtTextOut should be used to do the drawing. The data in its internal
 *  format is also passed here to allow checking of its value and possibly
 *  change the text or background color accordingly. The application must
 *  take care to always restore the original text or background color.
 *
 *  This function MUST be exported in the DEF file!
 ****************************************************************************/

int CALLBACK GenericContainerCellDraw( HWND hCntWnd, LPFIELDINFO lpFld, LPRECORDCORE lpRec, LPVOID lpData, HDC hDC, int nXstart, int nYstart, UINT fuOptions, LPRECT lpRect, LPSTR lpszOut, UINT uLenOut)
{
  COLORREF  crOldText = 0;
  COLORREF  crOldBk   = 0;
  BOOL      bChgedTxtColor = FALSE;
  BOOL      bChgedBkColor  = FALSE;

  // Custom management
  GENCNTR_COLUMN  cont_col;
  char            buf[BUFSIZE];

  // combo downarrow bitmap paint
  HDC     hMemDC;
  HBITMAP hBitmap, hOldBitmap;
  RECT    rcClient;
  int     x, y;
  HBRUSH  hBrush;

  // text alignment
  SIZE  size;
  int   remainder;
  int   availwidth;
  int   margin;

  // Decorations for title type cell
  HPEN  hOldPen, hWhitePen, hDkGrayPen;

  // Combo selection paint
  int   cpt;
  char *pItem;

  CntFldDataGet(lpRec, lpFld, 0, &cont_col);

  // Clear whole rectangle (old decorations cleanup) for combo type cell
  if (cont_col.type == CONT_COLTYPE_COMBO) {
    rcClient.left   = lpRect->left;
    rcClient.top    = lpRect->top;
    rcClient.right  = lpRect->right;
    rcClient.bottom = lpRect->bottom;
    hBrush = GetStockObject(WHITE_BRUSH);
    FillRect(hDC, &rcClient, hBrush);
  }

  switch (cont_col.type) {
    case CONT_COLTYPE_TITLE:
      fuOptions |= ETO_OPAQUE;
      crOldBk = SetBkColor(hDC, RGB(192,192,192));  // Light gray background
      bChgedBkColor = TRUE;
      crOldText = SetTextColor(hDC, RGB(255, 0, 0) ); // Red text
      bChgedTxtColor = TRUE;
      x_strcpy(buf, cont_col.ucol.s);
      //wsprintf(buf, "Title : %s", cont_col.ucol.s);
      break;
    case CONT_COLTYPE_TEXT:
      x_strcpy(buf, cont_col.ucol.s);
      //wsprintf(buf, "Text : %s", cont_col.ucol.s);
      break;
    case CONT_COLTYPE_COMBO:
      pItem = cont_col.ucol.sCombo.pItems;  // first item
      for (cpt=0; cpt<cont_col.ucol.sCombo.selection; cpt++)
        pItem += x_strlen(pItem) + 1;
      x_strcpy(buf, pItem);           // Show the 'nth' item
      //wsprintf(buf, "Combo : %s", cont_col.ucol.s);
      break;
    case CONT_COLTYPE_INT:
      wsprintf(buf, "%ld", cont_col.ucol.i);
      //wsprintf(buf, "Int : %ld", cont_col.ucol.i);
      break;
    case CONT_COLTYPE_FLOAT:
      // NOTE : wsprintf is not aware of the floating format...
      sprintf(buf, "%.*f", cont_col.precision, cont_col.ucol.f);
      //sprintf(buf, "Float : %.2f", cont_col.ucol.f);
      break;
    case CONT_COLTYPE_EMPTY:
      buf[0] = '\0';    // empty cell
      break;
    default:
      x_strcpy(buf, "Unknown type!");
      break;
  }

  // manage the horizontal positioning of the text
  if (cont_col.align != CONT_ALIGN_LEFT) {
    availwidth = lpRect->right - lpRect->left;
    margin = nXstart - lpRect->left;
#ifdef WIN32
    GetTextExtentPoint32(hDC, buf, x_strlen(buf), &size);
#else
    GetTextExtentPoint(hDC, buf, x_strlen(buf), &size);
#endif

    remainder = availwidth - 2*margin - size.cx;

    if (remainder > 0)
      switch (cont_col.align) {
        case CONT_ALIGN_CENTER:
          nXstart += remainder/2;
          break;
        case CONT_ALIGN_RIGHT:
          nXstart += remainder;
          break;
      }
  }

  // Draw the field data.
  ExtTextOut( hDC, nXstart, nYstart, fuOptions, lpRect,
              buf, x_strlen(buf), (LPINT)NULL);

  // Restore colors
  if (bChgedTxtColor)
    SetTextColor(hDC, crOldText);
  if (bChgedBkColor)
    SetBkColor(hDC, crOldBk);

  // Draw decorations for title type cell
  if (cont_col.type == CONT_COLTYPE_TITLE) {
    hWhitePen  = GetStockObject(WHITE_PEN);
    hDkGrayPen = CreatePen(PS_SOLID, 2, RGB(128, 128, 128));  // Dark gray

    hOldPen = SelectObject(hDC, hWhitePen);
    MoveToEx(hDC, lpRect->left,     lpRect->bottom, NULL);
    LineTo(hDC, lpRect->left,     lpRect->top);
    LineTo(hDC, lpRect->right-1,  lpRect->top);

    SelectObject(hDC, hDkGrayPen);
    MoveToEx(hDC, lpRect->left+2,  lpRect->bottom-2, NULL);
    LineTo(hDC, lpRect->right-2, lpRect->bottom-2);
    LineTo(hDC, lpRect->right-2, lpRect->top+1);

    SelectObject(hDC, hOldPen);
    DeleteObject(hDkGrayPen);
  }

  // Draw decorations for combo type cell
  if (cont_col.type == CONT_COLTYPE_COMBO) {
    hMemDC = CreateCompatibleDC(hDC);
    if (hMemDC) {
      hBitmap = LoadBitmap(0, (LPCSTR)OBM_DNARROW);
      if (hBitmap) {
        hOldBitmap = (HBITMAP)SelectObject(hMemDC,hBitmap);

        y = lpRect->top + 1;
        // 1 parent needed when in the CellDraw function!
        if (IsLastColumn(GetParent(hCntWnd), cont_col.colnum)) {
          GetClientRect(hCntWnd, &rcClient);
          x = rcClient.right;
          //x -= GetSystemMetrics(SM_CXVSCROLL);
          x -= 1;
        }
        else
          x = lpRect->right - 1;
        x -= GetSystemMetrics(SM_CXVSCROLL);
        BitBlt(hDC, x, y, BITMAPS_WIDTH, BITMAPS_HEIGHT,
               hMemDC, 0, 0, SRCCOPY);
        SelectObject(hMemDC,hOldBitmap);
        DeleteObject(hBitmap);
      }
      DeleteDC(hMemDC);
    }
  }

  // Finished
  return 0;

}

//
//  Initializes the container
//
VOID InitContainer(HWND hwndCnt, HWND hDlg, int colnum)
{
  int           charHeight;     // height of a cell before combo adjust
  int           comboHeight;    // height of a combo for cell height adjust
  HFONT         hFont;
  LPGENCNTRDATA lpGenCntrData;

  lpGenCntrData = ESL_AllocMem(sizeof(GENCNTRDATA));
  if (!lpGenCntrData)
    return;
  if (!SetGenericCntrData(hwndCnt, lpGenCntrData))
    return;

  // use the dialog box font
  hFont = GetWindowFont(hDlg);
  CntFontSet( hwndCnt, hFont, CF_GENERAL);

  // store number of columns
  lpGenCntrData->columns = colnum;

  // instance the Container cell draw function
  lpGenCntrData->lpfnDrawProc =
              (DRAWPROC) MakeProcInstance(GenericContainerCellDraw, hInst);

  // Defer painting until initialization is done
  CntDeferPaint( hwndCnt );

  // Init the vertical scrolling range to 0 since we have no records yet.
  CntRangeSet( hwndCnt, 0, 0 );
  CntVScrollPosSet(hwndCnt, 0);

  // Set the container view.
  CntViewSet( hwndCnt, CV_DETAIL );

  // Set colors for various portions of the container
  //CntColorSet( hwndCnt, CNTCOLOR_CNTBKGD,    RGB(255, 255, 255) );  // White
  CntColorSet( hwndCnt, CNTCOLOR_CNTBKGD,    GetSysColor(COLOR_WINDOW));
  CntColorSet( hwndCnt, CNTCOLOR_TITLE,      RGB(  0,   0,   0) );  // Black
  CntColorSet( hwndCnt, CNTCOLOR_FLDTITLES,  RGB(255,   0,   0) );  // Red
  CntColorSet( hwndCnt, CNTCOLOR_TEXT,       RGB(  0,   0, 255) );  // Blue
  CntColorSet( hwndCnt, CNTCOLOR_GRID,       RGB(  0,   0,   0) );  // Black

  // Set some general container attributes.
  CntAttribSet( hwndCnt, CA_SIZEABLEFLDS |
                         // Masqued due to wide last fld: CA_MOVEABLEFLDS |
                         CA_RECSEPARATOR |
                         // No container title CA_TITLE3D |
                         CA_FLDTTL3D | 
                         CA_VERTFLDSEP );

  // Set row character height and line spacing for container records,
  // and get back height in pixels
  charHeight = CntRowHtSet( hwndCnt, 1, CA_LS_NARROW);

  // Calculate combobox height
  comboHeight = CalculateComboHeight(hwndCnt);

  // Adjust if necessary
  if (comboHeight > charHeight)
    CntRowHtSet( hwndCnt, -comboHeight, 0);

  // title to same height
  if (comboHeight > charHeight)
    CntFldTtlHtSet( hwndCnt, -comboHeight);
  else 
    CntFldTtlHtSet( hwndCnt, -charHeight);

  // Define space for container field titles
  CntFldTtlHtSet( hwndCnt, 1 );
  CntFldTtlSepSet( hwndCnt );

  // Set up some cursors for the field title so we can swap columns.
  CntCursorSet( hwndCnt, LoadCursor(NULL, IDC_UPARROW ), CC_FLDTITLE );

  // MASQUED :
  //// Set up some cursors for the different areas.
  //  //    CntCursorSet( hwndCnt, LoadCursor(NULL, IDC_ICON ), CC_GENERAL );
  //CntCursorSet( hwndCnt, LoadCursor(NULL, IDC_CROSS ), CC_TITLE );
  //CntCursorSet( hwndCnt, LoadCursor(NULL, IDC_UPARROW ), CC_FLDTITLE );

  // MASQUED :
  //// Set some fonts for the different container areas.
  //CntFontSet( hwndCnt, hFontTtl,    CF_TITLE );
  //CntFontSet( hwndCnt, hFontFldTtl, CF_FLDTITLE );
  //CntFontSet( hwndCnt, hFontData,   CF_GENERAL );

  // Define the container fields(columns) and their attributes.
  DefineContainerColumns(hwndCnt, lpGenCntrData, colnum);

  CntAssociateSet( hwndCnt, hDlg);   // ??? Notification messages?

  // Paint the container
  CntEndDeferPaint( hwndCnt, FALSE);
  ShowWindow(hwndCnt,SW_NORMAL);

}

//
//  Define the fields (columns) of the container
//
static VOID NEAR DefineContainerColumns(HWND hwndCnt, LPGENCNTRDATA lpGenCntrData, int colnum)
{
  LPFIELDINFO lpFI;
  int         cpt;
  char        buf[BUFSIZE];

  for (cpt = 0; cpt < colnum; cpt++) {

    // Alloc a new field node
    lpFI = CntNewFldInfo( );

    // Set the default visual field width in characters units
    if (cpt < colnum-1)
      lpFI->cxWidth = MAXOBJECTNAME;
    else
      lpFI->cxWidth = 150;   // we won't want a white zone on the right

    // Set the field data alignment, type, length, and offset
    // to default values which are "Good for us"
    lpFI->flFDataAlign = CA_TA_LEFT | CA_TA_VCENTER;
    lpFI->wColType     = CFT_STRING;
    lpFI->wDataBytes   = sizeof(GENCNTR_COLUMN);
    lpFI->wOffStruct   = offsetof(GENCNTR_RECORD, cont_col2) * cpt;

    // Ownerdraw attribute - Mandatory for us!
    lpFI->flColAttr    = CFA_FLDREADONLY | CFA_OWNERDRAW;
    CntFldDrwProcSet(lpFI, lpGenCntrData->lpfnDrawProc);

    // Set field title and alignment
    //wsprintf(buf, "Column %d Title", cpt);
    buf[0] = '\0';
    CntFldTtlSet( hwndCnt, lpFI, buf, x_strlen(buf) + 1);
    lpFI->flFTitleAlign = CA_TA_LEFT | CA_TA_VCENTER;

    // Set magenta text - masqued
    //CntFldColorSet( hwndCnt, lpFI, CNTCOLOR_TEXT,    RGB(127,0,127) );

    // Set cyan background - masqued
    //CntFldColorSet( hwndCnt, lpFI, CNTCOLOR_FLDBKGD, RGB(0,127,127) );

    // Add this field to the container field list.
    CntAddFldTail( hwndCnt, lpFI );

    // Set the draw proc for the field ???
    // ??? CntFldDrwProcSet(lpFI,(DRAWPROC)FieldDrawProc);
  }
}

//
//  Frees objects associated to the container
//
VOID TerminateContainer(HWND hwndCnt)
{
  LPRECORDCORE    hLine;
  LPFIELDINFO     lpFld;
  GENCNTR_COLUMN  column;
  LPGENCNTRDATA   lpGenCntrData;

  lpGenCntrData = GetGenericCntrData(hwndCnt);
  if (!lpGenCntrData)
    return;

  // Free draw proc instance
  FreeProcInstance( lpGenCntrData->lpfnDrawProc);

  // Free the memory blocks allocated for combo boxes
  lpFld = CntFldHeadGet(hwndCnt);
  while (lpFld) {
    hLine = CntRecHeadGet(hwndCnt);
    while (hLine) {
      CntFldDataGet(hLine, lpFld, 0, (LPVOID)&column);
      if (column.type == CONT_COLTYPE_COMBO)
        ESL_FreeMem(column.ucol.sCombo.pItems);
      hLine = CntNextRec(hLine);
    }
    lpFld = CntNextFld(lpFld);
  }

  // We would free here the gdi objects created
  // for the container in InitContainer(),
  // such as fonts, bitmaps, icons, cursors

  // free the data associated with the container
  ESL_FreeMem(lpGenCntrData->pLines);
  ESL_FreeMem(lpGenCntrData);
  RemoveGenericCntrData(hwndCnt);
}

//
//  Update the combobox when a field has been sized,
//  or when there is a scroll in the container
//
VOID ContainerUpdateCombo(HWND hwndCnt)
{
  LPGENCNTRDATA   lpGenCntrData;

  lpGenCntrData = GetGenericCntrData(hwndCnt);
  if (!lpGenCntrData)
    return;

  if (lpGenCntrData->hWndCombo) {
    //SubClassEnd();
    DestroyWindow(lpGenCntrData->hWndCombo);
    lpGenCntrData->hWndCombo = 0;
    lpGenCntrData->lastSel = 0;
    // Simulate Selection change to recreate the combo
    ContainerNewSel(hwndCnt);
  }
}

//
//  Manage the previously selected cell
//  Updates the combobox selection if the cell was a combo
//
static BOOL NEAR ContainerOldSel(HWND hwndCnt, LPGENCNTRDATA lpGenCntrData)
{
  LPRECORDCORE    lpRec;
  GENCNTR_COLUMN  column;
  int             newsel;

  if (lpGenCntrData->lastSel) {
    if (lpGenCntrData->hWndCombo) {
      //SubClassEnd();
      lpRec = lpGenCntrData->lastLpRec;
      if (!CntFldDataGet(lpRec, lpGenCntrData->lastSel, 0, (LPVOID)&column))
        return FALSE;
      if (column.type == CONT_COLTYPE_COMBO) {
        newsel = ComboBox_GetCurSel(lpGenCntrData->hWndCombo);
        if (newsel == CB_ERR)
          newsel = 0;   // first item in case of error
        column.ucol.sCombo.selection = newsel;

        // Load back data into the record.
        CntFldDataSet(lpRec, lpGenCntrData->lastSel, 0, (LPVOID)&column);
      }
      DestroyWindow(lpGenCntrData->hWndCombo);
      lpGenCntrData->hWndCombo = 0;
    }
  }
  return TRUE;
}

//
//  Manage the newly selected cell
//
BOOL ContainerNewSel(HWND hwndCnt)
{
  GENCNTR_COLUMN  column;
  LPRECORDCORE    lpRec;
  RECT            rc;
  DWORD           dwOrg;
  DWORD           dwExt;
  char           *p;
  int             cpt;
  LPGENCNTRDATA   lpGenCntrData;

  lpGenCntrData = GetGenericCntrData(hwndCnt);
  if (!lpGenCntrData)
    return FALSE;

  if (lpGenCntrData->lastSel)
    ContainerOldSel(hwndCnt, lpGenCntrData);

  lpGenCntrData->lastSel = CntFocusFldGet(hwndCnt);
  if (lpGenCntrData->lastSel) {
    // Get Record
    lpRec = CntFocusRecGet(hwndCnt);
    lpGenCntrData->lastLpRec = lpRec;

    // extract data from cell
    if (!CntFldDataGet(lpRec, lpGenCntrData->lastSel, 0, (LPVOID)&column))
      return FALSE;

    // Manage combo type item
    if (column.type == CONT_COLTYPE_COMBO) {

      // Get rectangle
      dwOrg     = CntFocusOrgGet(hwndCnt, FALSE);   // Top left corner
      dwExt     = CntFocusExtGet(hwndCnt);          // width/height

      rc.left   = LOWORD(dwOrg) - 1;
      if (IsLastColumn(hwndCnt, column.colnum)) {
        RECT rcClient;
        GetClientRect(hwndCnt, &rcClient);
        rc.right = rcClient.right;
        // TO BE FINISHED : ONLY IF SCROLLBAR IS CURRENTLY VISIBLE
        //rc.right -= GetSystemMetrics(SM_CXVSCROLL);
        rc.right +=2;
      }
      else
        rc.right  = rc.left + LOWORD(dwExt) + 1;

      rc.top    = HIWORD(dwOrg);
      rc.bottom = rc.top + HIWORD(dwExt);

      // create combobox
      lpGenCntrData->hWndCombo =
                   CreateWindow("combobox",
                                0,
                                CBS_DROPDOWNLIST | WS_VISIBLE |
                                WS_CLIPSIBLINGS | WS_CHILD |WS_VSCROLL,
                                rc.left,
                                rc.top,
                                rc.right-rc.left,
                                (rc.bottom-rc.top)*5,
                                hwndCnt,
                                0,
                                hInst,
                                0);
      if (lpGenCntrData->hWndCombo) {
        int iSel =0;

        //SubClassBegin();
        //SendMessage(lpGenCntrData->hWndCombo, WM_SETFONT,
        //            (WPARAM) GetStockObject(ANSI_VAR_FONT), TRUE );
        SetWindowFont(lpGenCntrData->hWndCombo, CntFontGet(hwndCnt, CF_GENERAL), TRUE);

        // Fill the combo
        p = column.ucol.sCombo.pItems;
        for (cpt=0; cpt<column.ucol.sCombo.itemCount; cpt++) {
            ComboBox_AddString(lpGenCntrData->hWndCombo, p);
            p += x_strlen(p)+1; // advance to next item
        }
        ComboBox_SetCurSel(lpGenCntrData->hWndCombo, column.ucol.sCombo.selection);
        ComboBox_ShowDropdown(lpGenCntrData->hWndCombo, TRUE);
        BringWindowToTop(lpGenCntrData->hWndCombo);
        SetFocus(lpGenCntrData->hWndCombo); // Emb June 8, 95
      }
    }
  }
  return TRUE;
}


