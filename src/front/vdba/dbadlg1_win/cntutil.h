/********************************************************************
//
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
//
//    Project  : CA/OpenIngres Visual DBA
//
//    Source   : cntutil.h
//
//    
//
********************************************************************/
//
//    Author : Emmanuel Blattes
//
//

#ifndef __CNTUTIL_INCLUDED__
#define __CNTUTIL_INCLUDED__

#define CONT_COLTYPE_TITLE  1
#define CONT_COLTYPE_TEXT   2
#define CONT_COLTYPE_COMBO  3
#define CONT_COLTYPE_INT    4       // RESERVED for internal use
#define CONT_COLTYPE_FLOAT  5       // RESERVED for internal use
#define CONT_COLTYPE_EMPTY  0       // RESERVED for internal use

#define CONT_ALIGN_LEFT     1
#define CONT_ALIGN_CENTER   2
#define CONT_ALIGN_RIGHT    3

#define MAXCELLNAME 128   // maximum text length in a cell, excluding \0

// generic container data structure
typedef struct tagGENCNTRDATA {
   int          columns;      // number of columns in the container
   int          lines;        // number of lines in the container
   DRAWPROC     lpfnDrawProc; // Owner-draw items proc
   LPFIELDINFO  lastSel;      // last cell selected
   LPRECORDCORE lastLpRec;    // last record selected (for the last cell)
   HWND         hWndCombo;    // last combo created on the fly
   LPRECORDCORE *pLines;      // ptr on array of lines recordcores
} GENCNTRDATA, FAR *LPGENCNTRDATA;

//description of the anchor structure for a combo item
typedef struct tagGENCNTR_SCOMBO
{
  int   itemCount;    // Number of items in the combo
  int   charCount;    // total size, including double null terminator
  char *pItems;       // Pointer on the items, each behind the other
  int   selection;    // current selection (starting from 0)
} GENCNTR_SCOMBO, FAR *LPGENCNTR_SCOMBO;

// full description of a column (field)
typedef union tagGENCNTR_UCONT_COL
{
  char            s[MAXCELLNAME+1]; // Cont_coltype_title or text
  long            i;                // Cont_coltype_int -   take "long" precision
  double          f;                // Cont_coltype_float - take "double" precision
  GENCNTR_SCOMBO  sCombo;           // Cont_coltype_combo
} GENCNTR_U_COL, FAR *LPGENCNTR_U_COL;

// full description of a column (field)
typedef struct tagGENCNTR_COLUMN
{
  char              dummy[2];   // dummy text not to disturb the default display
  int               type;       // title, text, integer, float, combo
  int               align;      // left, center, right
  int               precision;  // for float only : digits after the decimal point
  GENCNTR_U_COL ucol;       // Column data (union)
  int               colnum;     // Column number, starting from 0 - used for search
  LPRECORDCORE      hLine;      // handle on the line (lpRec)     - used for search
} GENCNTR_COLUMN, FAR *LPGENCNTR_COLUMN;

//
// Describes the 2 first element of a record (row - line)
// we put 2 so that, using offsetof(second element), we get rid of the
// align problems which could happend if we simply used sizeof(GENCNTR_COLUMN)
//
// if the records contains more than 2 elements, should allocate more,
// taking care of the previously discussed align issue.
//
typedef struct tagGENCNTR_CONT_RECORD
{
  GENCNTR_COLUMN cont_col1;
  GENCNTR_COLUMN cont_col2;
} GENCNTR_RECORD,FAR *LPGENCNTR_RECORD;


// inits the container control hwndCnt of the dialog box hDlg
// with 'colnum' columns, and updates the received lpGenCntrData
VOID InitContainer(HWND hwndCnt, HWND hDlg, int colnum);

// Terminates a container window
VOID TerminateContainer(HWND hwndCnt);

// Sets the caption for the container
void ContainerSetCaption(HWND hwndCnt, char *szCaption);

// Disables the display, for flash avoid purposes
void ContainerDisableDisplay(HWND hwndCnt);

// Enables the display, and optionally immediately updates if flag is TRUE
void ContainerEnableDisplay(HWND hwndCnt, BOOL bUpdate);

// Creates a line and returns a line number (starting from 0)
// creates the default cells, all alpha with a dummy text
// first line will be a dummy line for caption
// returns -1 in case of error
int ContainerAddCntLine(HWND hwndCnt);

// set a column width in number of characters
// Convention : column number start from 0
// returns TRUE if operation successful
BOOL ContainerSetColumnWidth(HWND hwndCnt, int colnum, int width);

// Cell functions 
// a cell is the intersection of a column and a line.
// Line id should have been previously obtained with ContainerAddCntLine()

// Fill a cell with text - coltype can be TITLE or TEXT 
// returns TRUE if operation successful
BOOL ContainerFillTextCell(HWND         hCnt,
                           int          linenum,
                           int          colnum,
                           int          type,   // CONT_COLTYPE_XXX
                           int          align,  // CONT_ALIGN_XXX
                           char        *txt);   // Text to be displayed

// Fill a cell with a combobox
BOOL  ContainerFillComboCell(HWND         hCnt,
                             int          linenum,
                             int          colnum,
                             int          selection,  // selected item
                             int          align,      // CONT_ALIGN_XXX
                             char        *txt);       // combo items

// Sets the current selection in a combobox type cell
// Does not change the selection if item not found
BOOL  ContainerSetComboSelection(HWND    hwndCnt,
                                 int     linenum,
                                 int     colnum,
                                 char   *selection);  // item to select

// Query the current selection in a combobox type cell
// the parameter buffer must be of size MAXOBJECTNAME+1
BOOL ContainerGetComboCellSelection(HWND         hCnt,
                                    int          lineNum,
                                    int          colnum,
                                    char        *buf);

// Fill a cell with a numeric value
// returns TRUE if operation successful
BOOL ContainerFillNumCell(HWND        hCnt,
                         int          linenum,
                         int          colnum,
                         int          align,  // CONT_ALIGN_XXX
                         long         ival);  // int to be displayed

// Fill a cell with a float value - default precision will be 2
// returns TRUE if operation successful
BOOL ContainerFillFloatCell(HWND      hCnt,
                            int       linenum,
                            int       colnum,
                            int       align,  // CONT_ALIGN_XXX
                            double    fval);  // float to be displayed

// set the float value display precision for a cell
// returns TRUE if operation successful
BOOL ContainerSetFloatCellPrec(HWND      hCnt,
                               int       linenum,
                               int       colnum,
                               int       precision);

//
//  Manage the newly selected cell - in case of combobox
//
//  MUST BE CALLED ON IDOK IN THE DIALOG BOX BEFORE CALLING GetCellXxx()
//
BOOL ContainerNewSel(HWND hwndCnt);

//
//  Update the possible combobox when a field has been sized,
//  or when there is a scroll in the container
//
VOID ContainerUpdateCombo(HWND hwndCnt);

#endif  // __CNTUTIL_INCLUDED__

