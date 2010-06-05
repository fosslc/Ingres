/******************************************************************************
**
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Project  : Visual DBA
**
**    Source   : AlterTbl.c
**
**    Main Dialog box for Altering a Table
**
**  Author     : UK Sotheavut.
**
**  History after 04-May-1999:
**
**  17-June-1999 (schph01) bug #97504:
**      disabled the "change column name" functionality
**  30-June-1999 (schph01) incidence of bug #97072 on "alter table":
**      in "alter table, only "with null (not default)" is allowed for added
**      columns. However, "with null" implies implicitly a default value of NULL.
**      For this reason, the "default" checkbox and the "def.spec" edit
**      controls are not shown for added columns (in "alter table" only).
**      This avoids any ambiguity or confusion, among others with the new interface
**      choice done in "create table", where "with default NULL" is acceptable and 
**      generates something consistent with the interface.
**  01-Jul-1999 (schph01)
**      bug #97686:
**      (the lenght of a column could be copied into the column name)
**      Update lpEditRC and lpEditFld variables in the management of the
**      following messages:
**          CN_SETFOCUS,CN_NEWFOCUS,CN_NEWFOCUSFLD,CN_NEWFOCUSREC
**   02-Jul-1999 (schph01)
**      bug #97690
**      Dom Window / Create/Alter table : when creating a "decimal" type 
**      column, VDBA used to require a mandatory precision and scale.
**      With the current change, an empty string is accepted, resulting in
**      generating a "decimal" instead of "decimal(x,y)" syntax.
**      The empty string becomes the default (instead of "0,0")
**   06-Jul-1999 (schph01)
**      bug #97687 and bug #97691
**      The current change changes the (editable) default  precision from 0 to
**      8, resulting by default in a float8 (since float4 is generated for a
**      precision <8, and float8 is generated for a precision >=8. Any value
**      >=8 could have been used with the same result. The final result is
**      consistent with what the user could expect if he is familiar with the
**      sql syntax if he doesn't specify a precision.
**      bug #97691
**      When changing a column type, the length control and its caption were
**      not refreshed until the control was selected.
**
** uk$so01 (20-Jan-2000, Bug #100063)
**         Eliminate the undesired compiler's warning
**   27-Jan-2000 (schph01)
**      bug #100156 "Nullable" and "Default" checkboxes are now available when a
**      column is added.
**      "Default" is always grayed, it is checked and un-checked automatically
**      when "Nullable" is changed.
**   27-Jan-2000 (schph01)
**      bug #100177 remove the check/uncheck for the key "SPACE" on the
**      checkbox "Nullable" and "Default"
** 29-May-2000 (uk$so01)
**    bug 99242 (DBCS), replace the functions
**    IsCharAlphaNumeric by _istalnum
**    IsCharAlpha by _istalpha
** 08-Dec-2000 (schph01)
**    SIR 102823 Add two columns type: Object Key and Table Key
** 21-Mar-2001 (noifr01)
**    (bug 99242) (DBCS) replaced usage of ambiguous function (in the
**    DBCS context) lstrcpyn with fstrncpy
** 26-Mar-2001 (noifr01)
**    (sir 104270) removal of code for managing Ingres/Desktop
**  30-Mar-2001 (noifr01)
**    (sir 104378) differentiation of II 2.6.
** 04-Apr-2001 (schph01)
**    (sir 103820) Increasing the maximum size of character data types to 32K
** 21-Jun-2001 (schph01)
**    (SIR 103694) STEP 2 support of UNICODE datatypes
** 28-Jun-2001 (schph01)
**    (SIR 105158) The dialog to change the page size is launch :
**        - When a column is added or removed  and the actuel page size is 2K.
**        - When the total width of the columns exceeds the actual page size.
** 18-Sep-2001 (schph01)
**    (BUG 105386) hide "Default" checkbox when column type long byte,
**    long varchar or long nvarchar are selected.
**    Display "System Maintained" checkbox only for column type "Object Key"
**    and "Table Key"
** 05-Jun-2002 (noifr01)
**    (bug 107773) removed unused VerifyColumns() function
**    22-Aug-2002 (hanje04)
**	BUG 108564
**	In OnInitDialog() don't switch procedure on IDC_TABLE and IDC_COLUMN to
**	check for valid characters becuase it is causing problems with
**	non-englsh characters (e.g. ccedilla) under MAINWIN.
**	Only temporary fix, problem needs address more completely.
** 12-Feb-2003 (uk$so01)
**    BUG #109810, strings shouldn't be truncated in VDBA Dialogs
** 11-Apr-2003 (uk$so01)
**    BUG #110045, Ctrl-V/Ctrl-C fail to work in the create/alter table dialog
**    boxes within the edit fields of table name and table column.
** 12-May-2004 (schph01)
**    (SIR 111507) Add management for new column type bigint
** 17-May-2004 (schph01)
**    (SIR 112514) Add management for Alter column syntax.
** 12-Oct-2006 (wridu01)
**    (Sir 116835) Add support for Ansi Date/Time datatypes
** 23-Mar-2010 (drivi01)
**    Disable "Create VectorWise Table" option for Alter table
**    dialog as it doesn't apply.
** 10-May-2010 (drivi01)
**    Update alter table dialog to not allow add/drop columns
**    functionality for Ingres VectorWise tables.
**    Disable column selection for Ingres VectorWise tables.
*******************************************************************************/

// NOTE:
// ----
// Due to the document.
// The primary key and uniqe key column require that the columns be defined as NOT NULL.
// When adding the new columns they cannot be specified as NOT NULL.
// Then the newly added columns could not be part of the primary keys or Unique keys.
// 
// 
#include <stddef.h>
#include "dll.h"
#include "dlgres.h"
#include "getdata.h"
#include "msghandl.h"
#include "domdata.h"
#include "catocntr.h"
#include "dbaginfo.h"
#include "winutils.h"
#include "extccall.h"
#include "prodname.h"
#include "resource.h"
#include "tchar.h"

static TABLEPARAMS statictableparams;

// Structure describing the data types available
static CTLDATA typeTypes[] =
{
   INGTYPE_C,            IDS_INGTYPE_C,
   INGTYPE_CHAR,         IDS_INGTYPE_CHAR,
   INGTYPE_TEXT,         IDS_INGTYPE_TEXT,
   INGTYPE_VARCHAR,      IDS_INGTYPE_VARCHAR,
   INGTYPE_LONGVARCHAR,  IDS_INGTYPE_LONGVARCHAR,
   INGTYPE_INT4,         IDS_INGTYPE_INT4,
   INGTYPE_INT2,         IDS_INGTYPE_INT2,
   INGTYPE_INT1,         IDS_INGTYPE_INT1,
   INGTYPE_DECIMAL,      IDS_INGTYPE_DECIMAL,
   INGTYPE_FLOAT,        IDS_INGTYPE_FLOAT,
   INGTYPE_FLOAT8,       IDS_INGTYPE_FLOAT8,
   INGTYPE_FLOAT4,       IDS_INGTYPE_FLOAT4,
   INGTYPE_DATE,         IDS_INGTYPE_DATE,
   INGTYPE_MONEY,        IDS_INGTYPE_MONEY,
   INGTYPE_BYTE,         IDS_INGTYPE_BYTE,
   INGTYPE_BYTEVAR,      IDS_INGTYPE_BYTEVAR,
   INGTYPE_LONGBYTE,     IDS_INGTYPE_LONGBYTE,
   INGTYPE_INTEGER,      IDS_INGTYPE_INTEGER,   // equivalent to INT4
   INGTYPE_OBJKEY,       IDS_INGTYPE_OBJECTKEY,
   INGTYPE_TABLEKEY,     IDS_INGTYPE_TABLEKEY,
   -1    // terminator
};

static CTLDATA typeTypesUnicode[] =
{
   INGTYPE_C,              IDS_INGTYPE_C,
   INGTYPE_CHAR,           IDS_INGTYPE_CHAR,
   INGTYPE_TEXT,           IDS_INGTYPE_TEXT,
   INGTYPE_VARCHAR,        IDS_INGTYPE_VARCHAR,
   INGTYPE_LONGVARCHAR,    IDS_INGTYPE_LONGVARCHAR,
   INGTYPE_INT4,           IDS_INGTYPE_INT4,
   INGTYPE_INT2,           IDS_INGTYPE_INT2,
   INGTYPE_INT1,           IDS_INGTYPE_INT1,
   INGTYPE_DECIMAL,        IDS_INGTYPE_DECIMAL,
   INGTYPE_FLOAT,          IDS_INGTYPE_FLOAT,
   INGTYPE_FLOAT8,         IDS_INGTYPE_FLOAT8,
   INGTYPE_FLOAT4,         IDS_INGTYPE_FLOAT4,
   INGTYPE_DATE,           IDS_INGTYPE_DATE,
   INGTYPE_MONEY,          IDS_INGTYPE_MONEY,
   INGTYPE_BYTE,           IDS_INGTYPE_BYTE,
   INGTYPE_BYTEVAR,        IDS_INGTYPE_BYTEVAR,
   INGTYPE_LONGBYTE,       IDS_INGTYPE_LONGBYTE,
   INGTYPE_INTEGER,        IDS_INGTYPE_INTEGER,   // equivalent to INT4
   INGTYPE_OBJKEY,         IDS_INGTYPE_OBJECTKEY,
   INGTYPE_TABLEKEY,       IDS_INGTYPE_TABLEKEY,
   INGTYPE_UNICODE_NCHR,   IDS_INGTYPE_UNICODE_NCHR,     // unicode nchar
   INGTYPE_UNICODE_NVCHR,  IDS_INGTYPE_UNICODE_NVCHR,   // unicode nvarchar
   INGTYPE_UNICODE_LNVCHR, IDS_INGTYPE_UNICODE_LNVCHR, // unicode long nvarchar
   -1    // terminator
};

static CTLDATA typeTypesBigInt[] =
{
   INGTYPE_C,              IDS_INGTYPE_C,
   INGTYPE_CHAR,           IDS_INGTYPE_CHAR,
   INGTYPE_TEXT,           IDS_INGTYPE_TEXT,
   INGTYPE_VARCHAR,        IDS_INGTYPE_VARCHAR,
   INGTYPE_LONGVARCHAR,    IDS_INGTYPE_LONGVARCHAR,
   INGTYPE_BIGINT,         IDS_INGTYPE_BIGINT,
   INGTYPE_INT8,           IDS_INGTYPE_INT8,
   INGTYPE_INT4,           IDS_INGTYPE_INT4,
   INGTYPE_INT2,           IDS_INGTYPE_INT2,
   INGTYPE_INT1,           IDS_INGTYPE_INT1,
   INGTYPE_DECIMAL,        IDS_INGTYPE_DECIMAL,
   INGTYPE_FLOAT,          IDS_INGTYPE_FLOAT,
   INGTYPE_FLOAT8,         IDS_INGTYPE_FLOAT8,
   INGTYPE_FLOAT4,         IDS_INGTYPE_FLOAT4,
   INGTYPE_DATE,           IDS_INGTYPE_DATE,
   INGTYPE_MONEY,          IDS_INGTYPE_MONEY,
   INGTYPE_BYTE,           IDS_INGTYPE_BYTE,
   INGTYPE_BYTEVAR,        IDS_INGTYPE_BYTEVAR,
   INGTYPE_LONGBYTE,       IDS_INGTYPE_LONGBYTE,
   INGTYPE_INTEGER,        IDS_INGTYPE_INTEGER,   // equivalent to INT4
   INGTYPE_OBJKEY,         IDS_INGTYPE_OBJECTKEY,
   INGTYPE_TABLEKEY,       IDS_INGTYPE_TABLEKEY,
   INGTYPE_UNICODE_NCHR,   IDS_INGTYPE_UNICODE_NCHR,     // unicode nchar
   INGTYPE_UNICODE_NVCHR,  IDS_INGTYPE_UNICODE_NVCHR,   // unicode nvarchar
   INGTYPE_UNICODE_LNVCHR, IDS_INGTYPE_UNICODE_LNVCHR, // unicode long nvarchar
   -1    // terminator
};

static CTLDATA typeTypesAnsiDate[] =
{
   INGTYPE_C,              IDS_INGTYPE_C,
   INGTYPE_CHAR,           IDS_INGTYPE_CHAR,
   INGTYPE_TEXT,           IDS_INGTYPE_TEXT,
   INGTYPE_VARCHAR,        IDS_INGTYPE_VARCHAR,
   INGTYPE_LONGVARCHAR,    IDS_INGTYPE_LONGVARCHAR,
   INGTYPE_BIGINT,         IDS_INGTYPE_BIGINT,
   INGTYPE_INT8,           IDS_INGTYPE_INT8,
   INGTYPE_INT4,           IDS_INGTYPE_INT4,
   INGTYPE_INT2,           IDS_INGTYPE_INT2,
   INGTYPE_INT1,           IDS_INGTYPE_INT1,
   INGTYPE_DECIMAL,        IDS_INGTYPE_DECIMAL,
   INGTYPE_FLOAT,          IDS_INGTYPE_FLOAT,
   INGTYPE_FLOAT8,         IDS_INGTYPE_FLOAT8,
   INGTYPE_FLOAT4,         IDS_INGTYPE_FLOAT4,
   INGTYPE_DATE,           IDS_INGTYPE_DATE,
   INGTYPE_ADTE,           IDS_INGTYPE_ADTE,             // ansidate
   INGTYPE_TMWO,           IDS_INGTYPE_TMWO,             // time without time zone
   INGTYPE_TMW,            IDS_INGTYPE_TMW,              // time with time zone
   INGTYPE_TME,            IDS_INGTYPE_TME,              // time with local time zone
   INGTYPE_TSWO,           IDS_INGTYPE_TSWO,             // timestamp without time zone
   INGTYPE_TSW,            IDS_INGTYPE_TSW,              // timestamp with time zone
   INGTYPE_TSTMP,          IDS_INGTYPE_TSTMP,            // timestamp with local time zone
   INGTYPE_INYM,           IDS_INGTYPE_INYM,             // interval year to month
   INGTYPE_INDS,           IDS_INGTYPE_INDS,             // interval day to second
   INGTYPE_IDTE,           IDS_INGTYPE_IDTE,             // ingresdate
   INGTYPE_MONEY,          IDS_INGTYPE_MONEY,
   INGTYPE_BYTE,           IDS_INGTYPE_BYTE,
   INGTYPE_BYTEVAR,        IDS_INGTYPE_BYTEVAR,
   INGTYPE_LONGBYTE,       IDS_INGTYPE_LONGBYTE,
   INGTYPE_INTEGER,        IDS_INGTYPE_INTEGER,          // equivalent to INT4
   INGTYPE_OBJKEY,         IDS_INGTYPE_OBJECTKEY,
   INGTYPE_TABLEKEY,       IDS_INGTYPE_TABLEKEY,
   INGTYPE_UNICODE_NCHR,   IDS_INGTYPE_UNICODE_NCHR,     // unicode nchar
   INGTYPE_UNICODE_NVCHR,  IDS_INGTYPE_UNICODE_NVCHR,    // unicode nvarchar
   INGTYPE_UNICODE_LNVCHR, IDS_INGTYPE_UNICODE_LNVCHR,   // unicode long nvarchar
   -1    // terminator
};

//
// Structure describing the min/max and default length for a given data type
//
typedef struct tagREGLENGTH
{
    int nId;                    // Data type id
    int nMinLength;             // Minimum length of data type
    int dwMaxLength;            // Maximum length of data type
    int dwDefaultLength;        // Default length of data type
} REGLENGTH, FAR * LPREGLENGTH;

//
// If a data type exists in the following table, the length edit box
// is enabled when the data type is selected.
//
#if 0 
static REGLENGTH nReqLength[] =
{
    INGTYPE_C,              1,  2000,    1,
    INGTYPE_CHAR,           1,  2000,    20,
    INGTYPE_VARCHAR,        1,  2000,    20,
    INGTYPE_TEXT,           1,  2000,    50,
    INGTYPE_BYTE,           1,  2000,    32,
    INGTYPE_BYTEVAR,        1,  2000,    32,
    -1    // terminator
};
#else
static REGLENGTH nReqLength[] =
{
    INGTYPE_C,              0,  2000,    0,
    INGTYPE_CHAR,           0,  2000,    0,
    INGTYPE_VARCHAR,        0,  2000,    0,
    INGTYPE_TEXT,           0,  2000,    0,
    INGTYPE_BYTE,           0,  2000,    0,
    INGTYPE_BYTEVAR,        0,  2000,    0,
    INGTYPE_FLOAT,          0,  53,      8,
    -1    // terminator
};
static REGLENGTH nReqLength26[] =
{
    INGTYPE_C,              0,  32000,    0,
    INGTYPE_CHAR,           0,  32000,    0,
    INGTYPE_VARCHAR,        0,  32000,    0,
    INGTYPE_TEXT,           0,  32000,    0,
    INGTYPE_BYTE,           0,  32000,    0,
    INGTYPE_BYTEVAR,        0,  32000,    0,
    INGTYPE_FLOAT,          0,  53,       8,
    INGTYPE_UNICODE_NCHR,   0,  16000,    0,
    INGTYPE_UNICODE_NVCHR,  0,  16000,    0,
    -1    // terminator
};
#endif

//
// Union describing the container column data
//
typedef union tagCNTDATA
{
    UINT  uDataType;                // A number identifying Data type
    int   nPrimary;                 // Primary key (0 = not part of primary key) sequence.
    BOOL  bNullable;                // TRUE if nullable
    BOOL  bDefault;                 // TRUE if default value allowed.
    BOOL  bUnique;                  // TRUE if unique constraint will be specified for the column.
    LPSTR lpszDefSpec;              // The default specification.
    LPSTR lpszCheck;                // The check constraint.
    LENINFO len;                    // The size of the column.
    BOOL bSystemMaintained;         // TRUE if System Maintained for an Objectkey
} CNTDATA, FAR * LPCNTDATA;


// Pointers for each record in the container
static LPCNTDATA lprecType      = NULL;
static LPCNTDATA lprecLen       = NULL;
static LPCNTDATA lprecPrimary   = NULL;
static LPCNTDATA lprecNullable  = NULL;
static LPCNTDATA lprecDefault   = NULL;
static LPCNTDATA lprecUnique    = NULL;
static LPCNTDATA lprecDefSpec   = NULL;
// static LPCNTDATA lprecCheck     = NULL;
static LPCNTDATA lprecSysMaintained = NULL;

//
// This variable holds list of columns that are part of Foreign Keys.
// It must be initialized at the WM_INITDIALOG
// and destroy at WM_DESTROY. 
static LPOIDTOBJECT lpFKeyCandidate = NULL;
//
// Store the table foreign key for supporting dialog box. 
// It must be initialized at the WM_INITDIALOG by copying the List of Foreign Keys
// into it, and destroy at WM_DESTROY
//
static  REFERENCES stReferences ;   
static BOOL bRelaunchAlter = FALSE;

// Defines for each record in the container
#define DT_TYPE      0
#define DT_LEN       1
//#define DT_PRIMARY   2
#define DT_NULLABLE  2
#define DT_UNIQUE    3
#define DT_DEFAULT   4
#define DT_DEFSPEC   5
// #define DT_CHECK     7
#define DT_SYSMAINTAINED 6
#define DT_TITLE    -1    // special case for the field title

static LPCNTDATA FAR * cntRecords[] =
{
    &lprecType,
    &lprecLen,
    //&lprecPrimary,
    &lprecNullable,
    &lprecUnique,
    &lprecDefault,
    &lprecDefSpec,
//    &lprecCheck,
    &lprecSysMaintained,
   (LPCNTDATA FAR *)NULL    // terminator
};

typedef struct tagPrimarySort
{
    int nKey;
    BOOL bFixed;
    BOOL bProcessed;
    LPFIELDINFO lpFI;
} PRIMARYSORT, FAR * LPPRIMARYSORT;


static DRAWPROC lpfnDrawProc    = NULL; // Pointer to the field drawing procedure for the container
static UINT     uDataSize;              // Size of the container records structure
static UINT     uFieldCounter   = 0;    // Total number of fields that have been created in the container
static WNDPROC  wndprocCntChild = NULL; // Pointer to the original container child window procedure
static WNDPROC  wndprocEdit     = NULL; // Pointer to the original edit window procedure
static WNDPROC  wndprocName     = NULL; // Pointer to the original table name edit window procedure
static WNDPROC  wndprocType     = NULL; // Pointer to the original type combo box window procedure
static WNDPROC  wndprocColName  = NULL; // Pointer to the original column name edit box
static HWND     hwndCntChild    = NULL; // Handle to the container child.
static HWND     hwndCntEdit     = NULL; // Handle to the edit control used in the container for editing.
static HWND     hwndType        = NULL; // Handle to the data type drop down combo used in the container
static int      nCBHeight       = 0;    // Height of the check box.  Since its square the width is the same.
static LPRECORDCORE lpEditRC;           // Storage for the container edit box to use
static LPFIELDINFO  lpEditFI;

static LPRECORDCORE lpTypeRC;           // Storage for the container type window to use.
static LPFIELDINFO  lpTypeFI;

static LPTABLEPARAMS lptblparams= NULL; // Store the table structure for supporting dialogs
static LPTABLEPARAMS lprefparams= NULL;
static HWND hwndThisDialog = NULL;

extern int MfcModifyPageSize(LPTABLEPARAMS lpTS);


#define BOX_LEFT_MARGIN         2       // Margins to use for the check box in the container
#define BOX_VERT_MARGIN         2       // Margins to use for the check box in the container
#define TEXT_LEFT_MARGIN        3       // Margin to use for the text for the check box in the container
#define ID_CNTEDIT          20000       // Window identifier for the container edit window
#define ID_CNTCOMBO         20001       // Window identifier for the container drop down data type list
#define EDIT_BORDER_WIDTH       2       // Width of the edit control border.
#define EDIT_CURSOR_WIDTH       1       // Width of the cursor in the edit control
#define FOCUS_BORDER_WIDTH      2       // Width of the focus border
#define SEPARATOR_WIDTH         1       // Width of the container separator line
#define TYPE_WINDOW_HEIGHT    120       // Height of type combobox window
#define MAX_VARCHAR           999       // Maximum length of a varchar
#define MAX_DEFAULTSTRING     500       // Maximum length of a check or default spec string
#define EDIT_LEFT_MARGIN        2       // Gap between the text and the edit box
#define CB_BORDER_WIDTH         2       // Width of the combo box control border.
#define CB_FOCUS_WIDTH          1       // Width of the focus field in the combobox control
#define MAX_DECIMAL_PRECISION  31       // Max precision for decimal data type
#define DEFAULT_PRECISION       5
#define DEFAULT_SCALE           0
#define DEFAULT_FLOAT           8

#ifdef  WIN32
    #define DlgExport  LRESULT WINAPI EXPORT
#else
    #define DlgExport  LRESULT WINAPI __export
#endif


BOOL CALLBACK EXPORT VDBA2xAlterTableDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK EXPORT EnumVDBA2xAlterTblCntChildProc(HWND hwnd, LPARAM lParam);

DlgExport CntVDBA2xAlterTblChildSubclassProc   (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
DlgExport VDBA2xAlterTblCntChildDefWndProc     (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
DlgExport VDBA2xAlterTblEditSubclassProc       (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
DlgExport VDBA2xAlterTblEditDefWndProc         (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
DlgExport VDBA2xAlterTblTypeSubclassProc       (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
DlgExport VDBA2xAlterTblTypeDefWndProc         (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
DlgExport VDBA2xAlterTblColNameSubclassProc    (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
DlgExport VDBA2xAlterTblColNameDefWndProc      (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

static void OnDestroy       (HWND hwnd);
static void OnCommand       (HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
static BOOL OnInitDialog    (HWND hwnd, HWND hwndFocus, LPARAM lParam);
static void OnSysColorChange(HWND hwnd);

static void ComboBoxFillVDBA2xDataType    (HWND hwndCtrl);
static void InitialiseEditControls      (HWND hwnd);
static void InitContainer               (HWND hwndCtnr);
static void DrawCheckBox    (HDC hdc, LPRECT lprect, BOOL bSelected, int nStringId, BOOL bGrey);
static void CntOnLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags);
static void CntOnLButtonUp  (HWND hwnd, int x, int y, UINT keyFlags);
static void CntOnCommand    (HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);

static void Edit_OnChar     (HWND hWnd, UINT ch, int cRepeat);
static void Edit_OnKey      (HWND hWnd, UINT vk, BOOL fDown, int cRepeat, UINT flags);
static UINT Edit_OnGetDlgCode   (HWND hWnd, MSG FAR* lpmsg);
static void Edit_OnKillFocus    (HWND hwnd, HWND hwndNewFocus);

static void Type_OnChar     (HWND hWnd, UINT ch, int cRepeat);
static void Type_OnKey      (HWND hWnd, UINT vk, BOOL fDown, int cRepeat, UINT flags);
static UINT Type_OnGetDlgCode   (HWND hWnd, MSG FAR* lpmsg);
static void Type_OnSetFocus     (HWND hwnd, HWND hwndOldFocus);
static void Type_OnKillFocus    (HWND hwnd, HWND hwndNewFocus);

static BOOL CntFindField        (HWND hwnd, LPSTR lpszTitle);
static void DrawStaticText      (HDC hDC, LPRECT lprect, LPCSTR lpszText, BOOL bAlignLeft, BOOL bGrey, UINT uStringId);
static BOOL AllocContainerRecords   (UINT uSize);
static BOOL ReAllocContainerRecords (UINT uNewSize);
static void FreeContainerRecords    (void);
static void CopyContainerRecords    (HWND hwndCnt);
static void InitialiseFieldData     (UINT uField);
static void AddRecordsToContainer   (HWND hwndCnt);
static int  GetRecordDataType       (HWND hwndCnt, LPRECORDCORE lpRCIn);

static void ShowEditWindow  (HWND hwndCnt, LPRECT lprect, int nType, LPCNTDATA lpdata);
static void ShowTypeWindow  (HWND hwndCnt, LPRECT lprect);

static void SaveEditData    (HWND hwndCnt);
static void MakeColumnsList (HWND hwnd, LPTABLEPARAMS lptbl);
static void ReleaseField    (HWND hwndCnt, LPFIELDINFO lpField);
static void ReleaseAllFields(HWND hwndCnt);
static void ColName_OnKey   (HWND hWnd, UINT vk, BOOL fDown, int cRepeat, UINT flags);
static UINT ColName_OnGetDlgCode    (HWND hWnd, MSG FAR* lpmsg);
static void AddContainerColumn      (HWND hwnd);
static LPRECORDCORE GetDataTypeRecord   (HWND hwndCnt, int nDataType);
static int  GetMinReqLength         (int nId);
static long GetMaxReqLength         (int nId);
static void MakeTypeLengthString    (UINT uType, LPCNTDATA lpdata, LPSTR lpszNumber);
static void GetTypeLength           (UINT uType, LPCNTDATA lpdata, LPSTR lpszNumber);
static void VerifyDecimalString     (HWND hwndEdit);
static void GetTitleRect            (HWND hwndCnt, LPRECT lprect);
static void ResetCntEditCtl         (HWND hwndCnt);
static BOOL FindReferencesColumnName(LPSTR lpszCol, LPOBJECTLIST FAR * FAR * lplpRefList, LPOBJECTLIST FAR * lplpElement);
static void EnableOKButton          (HWND hwnd);

static void LocalGetFocusCellRect        (HWND hwndCnt, LPRECT lprect);
static void OnOK                    (HWND hwnd);
static void OnColumnNameEditChange  (HWND hwnd);
static BOOL AllowTypeChanged        (HWND hwnd, LPFIELDINFO lpFI);
static int GetDataTypeStringId      (int nId);
static BOOL IsAlterColumn(LPTABLEPARAMS lpTemp, LPSTR lpColName, BOOL bDisplayMessage);

int CALLBACK EXPORT DrawVDBA2xAlterTblFldData( HWND   hwndCnt,
                                      LPFIELDINFO  lpFld,
                                      LPRECORDCORE lpRec,
                                      LPVOID lpData,
                                      HDC    hDC,
                                      int    nXstart,
                                      int    nYstart,
                                      UINT   fuOptions,
                                      LPRECT lpRect,
                                      LPSTR  lpszOut,
                                      UINT   uLenOut );





static BOOL FillVDBA2xDataType      (HWND hwndCtl, LPCTLDATA lpdata);
static void OnRemoveCntColumn       (HWND hwnd, BOOL bCascade);
static void OnTableNameEditChange   (HWND hwnd);
static void OnReferences            (HWND hwnd);
static void OnCancel                (HWND hwnd);
static void OnContainer             (HWND hwnd, HWND hwndCtl, UINT codeNotify);
static void OnCntEdit               (HWND hwnd, HWND hwndCtl, UINT codeNotify);
static void OnCntCombo              (HWND hwnd, HWND hwndCtl, UINT codeNotify);

static BOOL FillControlsFromStructure (HWND hwnd, LPTABLEPARAMS lpTS);
static BOOL FillStructureFromControls (HWND hwnd, LPTABLEPARAMS lpTS);
static void CntSetTableColumn         (HWND hwndCntr, LPCOLUMNPARAMS lpCol);
static BOOL AlterObject             (HWND hwnd);
static void OnUniqueConstraint      (HWND hwnd);
static void OnCheckConstraint       (HWND hwnd);

static LPOBJECTLIST RemoveForeignKeys (LPOBJECTLIST lpReferences, LPCTSTR lpszColumn);
static LPTLCUNIQUE  RemoveUniqueKeys  (LPTLCUNIQUE  lpUnique,     LPCTSTR lpszColumn);

static BOOL VDBA20xTableDataValidate (HWND hwnd, LPTABLEPARAMS lpTS);
static BOOL IsLengthZero (HWND hwnd, char* buffOut);
static void CntUpdateUnique(HWND hwnd, LPTLCUNIQUE lpunique, LPTLCUNIQUE lpNewUnique);
static void OnPrimaryKey(HWND hwnd);
static void OnChangeFocus(HWND hwnd);
static void InitializeDataLenWithDefault(HWND hwndCnt, LPFIELDINFO lpFI);
static void SetDataTypeWithCurSelCombobox();
static void UpdateNullableAndDefault(HWND hwndCnt, LPFIELDINFO lpFI);


BOOL WINAPI EXPORT VDBA2xAlterTable (HWND hwndOwner, LPTABLEPARAMS lpTableStructure)
{
    int iRetVal = -1;
    FARPROC lpfnDlgProc;

    if (!IsWindow(hwndOwner) || !lpTableStructure)
    {
        ASSERT(NULL);
        return -1;
    }
    if (!lpTableStructure->DBName[0])
    {
        ASSERT(NULL);
        return -1;
    }
    lpfnDlgProc = MakeProcInstance((FARPROC)VDBA2xAlterTableDlgProc, hInst);
    iRetVal     = VdbaDialogBoxParam (hResource, MAKEINTRESOURCE(IDD_CREATETABLE), hwndOwner, (DLGPROC)lpfnDlgProc, (LPARAM)lpTableStructure);
    FreeProcInstance (lpfnDlgProc);
    return (iRetVal == -1)? FALSE: TRUE;
}


BOOL CALLBACK EXPORT VDBA2xAlterTableDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        HANDLE_WM_COMMAND (hwnd, wParam, lParam, OnCommand);
        break;
    case WM_DESTROY:
        HANDLE_WM_DESTROY (hwnd, wParam, lParam, OnDestroy);
        break;
    case WM_INITDIALOG:
        return HANDLE_WM_INITDIALOG(hwnd, wParam, lParam, OnInitDialog);
    default:
        return HandleUserMessages(hwnd, message, wParam, lParam);
    }
    return FALSE;
}


static BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    TCHAR tchszFormat [128];
    TCHAR tchszTitle  [128];
    LPCNTINFO lpcntInfo;
    BOOL bEnabled = FALSE;
    LPTABLEPARAMS lptbl = (LPTABLEPARAMS)lParam;
    LPTABLECOLUMNS   ls    = NULL;
    LPOIDTOBJECT     lpObj = NULL;
    LONG lStyle = 0L;

    if (!AllocDlgProp (hwnd, lptbl))
        return FALSE;
    hwndThisDialog = hwnd;
    memset (&statictableparams, 0, sizeof (statictableparams));
    lStyle  = GetWindowLong (GetDlgItem (hwnd, IDC_TABLE), GWL_STYLE);
    lStyle |= ES_READONLY;
    SetWindowLong (GetDlgItem (hwnd, IDC_TABLE), GWL_STYLE, lStyle);
    //
    // Hide/Gray some controls
    ShowWindow   (GetDlgItem (hwnd, IDC_INGCHECKASSELECT), SW_HIDE);
    ShowWindow   (GetDlgItem (hwnd, IDC_INGCHECKVW), SW_HIDE);
    EnableWindow (GetDlgItem (hwnd, IDC_STRUCTURE), FALSE);
    EnableWindow (GetDlgItem (hwnd, IDC_TABLE), FALSE);
    ShowWindow   (GetDlgItem (hwnd, IDC_REMOVE_CASCADE), SW_SHOW);
    EnableWindow(GetDlgItem(hwnd, IDC_ADD),             FALSE);
    EnableWindow(GetDlgItem(hwnd, IDC_DROP),            FALSE);
    EnableWindow(GetDlgItem(hwnd, IDC_REMOVE_CASCADE),  FALSE);

    //
    // For VectorWise tables, hide the add/remove/remove cascade buttons 
    // as add/remove column functionality is not supported in 1.0 release
    //
    if (lptbl && (lptbl->StorageParams.nStructure == IDX_VW 
	|| lptbl->StorageParams.nStructure == IDX_VWIX))
    {
      ShowWindow(GetDlgItem(hwnd, IDC_ADD),             SW_HIDE);
      ShowWindow(GetDlgItem(hwnd, IDC_DROP),            SW_HIDE);
      ShowWindow(GetDlgItem(hwnd, IDC_REMOVE_CASCADE),  SW_HIDE);
      EnableWindow(GetDlgItem(hwnd, IDC_TLCBUTTON_CHECK), SW_HIDE);
    }

    //
    // Set the appropriate Dialog Box Title
    //
    LoadString (hResource, (UINT)IDS_ALTERTABLETITLE, tchszFormat, sizeof (tchszFormat));
    lpHelpStack = StackObject_PUSH (lpHelpStack, StackObject_INIT ((UINT)IDD_OIV2_ALTERTABLE));
    wsprintf (
        tchszTitle,
        tchszFormat,
        GetVirtNodeName (GetCurMdiNodeHandle ()),
        lptbl->DBName);
    SetWindowText (hwnd, (LPSTR)tchszTitle);
    //
    // Initialise our globals.
    //
    lpfnDrawProc    = NULL;
    uDataSize       = 0;
    uFieldCounter   = 0;
    wndprocCntChild = NULL;
    hwndCntChild    = NULL;
    hwndCntEdit     = NULL;
    hwndType    = NULL;
    nCBHeight   = 0;
    lpEditFI    = NULL;
    lpEditRC    = NULL;

/*
** Don't switch procedure for MAINWIN builds, causes bug 108564
** FIXME!!!!
*/

#ifndef MAINWIN
    wndprocColName  = SubclassWindow(GetDlgItem(hwnd, IDC_COLNAME), VDBA2xAlterTblColNameSubclassProc);
#endif

    InitialiseEditControls (hwnd);
    InitContainer (GetDlgItem(hwnd, IDC_CONTAINER));
    FillControlsFromStructure (hwnd, lptbl);
    //
    // Disable the OK button if there is no text in the table name control
    //
    EnableOKButton(hwnd);
    if (Edit_GetTextLength(GetDlgItem(hwnd, IDC_COLNAME)) == 0)
        EnableWindow(GetDlgItem(hwnd, IDC_ADD), FALSE);
    lpcntInfo = CntInfoGet(GetDlgItem(hwnd, IDC_CONTAINER));
    if (lpcntInfo->nFieldNbr > 0)
        bEnabled = TRUE;
    EnableWindow(GetDlgItem(hwnd, IDC_REFERENCES), bEnabled);
    //
    // Initialise the table structure structure
    //
    VDBA20xTableStruct_Copy (&statictableparams, lptbl, TABLEPARAMS_ALL);
    statictableparams.bCreate = FALSE;

    richCenterDialog(hwnd);
    return TRUE;
}


static void OnDestroy(HWND hwnd)
{
    HWND hwndCnt = GetDlgItem(hwnd, IDC_CONTAINER);
    LPTABLEPARAMS lptbl = GetDlgProp (hwnd);
    ASSERT(IsWindow (hwndCnt));
    ReleaseAllFields(hwndCnt);
    SubclassWindow(hwndCntChild, wndprocCntChild);
    if (lpfnDrawProc)
        FreeProcInstance(lpfnDrawProc);
    FreeContainerRecords();
    DeallocDlgProp(hwnd);
	VDBA20xTableStruct_Free (&statictableparams, TABLEPARAMS_ALL);
    lpHelpStack = StackObject_POP (lpHelpStack);
}


static void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    switch (id)
    {
    case IDOK:
        ShowHourGlass ();
        OnOK (hwnd);
        RemoveHourGlass ();
        break;
    case IDCANCEL:
        OnCancel (hwnd);
        break;
    case IDC_COLNAME:
        if (codeNotify == EN_CHANGE)
            OnColumnNameEditChange (hwnd);
        break;
    case IDC_TABLE:
        if (codeNotify == EN_CHANGE)
            OnTableNameEditChange (hwnd);
        break;
    case IDC_ADD:
        AddContainerColumn(hwnd);
        EnableOKButton(hwnd);
        break;
    case IDC_DROP:
        OnRemoveCntColumn (hwnd, FALSE);
        EnableOKButton(hwnd);
        break;
    case IDC_REMOVE_CASCADE:
        OnRemoveCntColumn (hwnd, TRUE);
        EnableOKButton(hwnd);
        break;
    case IDC_CONTAINER:
        OnContainer (hwnd, hwndCtl, codeNotify);
        break;
    case IDC_REFERENCES:
        OnReferences (hwnd);
        break;
    case IDC_TLCBUTTON_UNIQUE:
        OnUniqueConstraint (hwnd);
        break;
    case IDC_TLCBUTTON_CHECK:
        OnCheckConstraint (hwnd);
        break;
    case IDC_CONSTRAINT_INDEX:
        OnPrimaryKey(hwnd);
      break;
    default:
        break;
    }
}


static void OnSysColorChange(HWND hwnd)
{
#ifdef _USE_CTL3D
    Ctl3dColorChange();
#endif
}


static void InitialiseEditControls (HWND hwnd)
{
    LPTABLEPARAMS lptbl = GetDlgProp(hwnd);
    HWND hwndTable = GetDlgItem(hwnd, IDC_TABLE);
    Edit_LimitText(hwndTable, MAXOBJECTNAME-1);
    Edit_SetText  (hwndTable, lptbl->objectname);
}


static void InitContainer(HWND hwnd)
{
    HFONT hFont, hOldFont;
    HDC hdc = GetDC(hwnd);
    TEXTMETRIC tm;
    int nRowHeight;
    RECT rect;
    LPCNTINFO lpcntInfo;
    BOOL bEnabled = FALSE;
    HWND hdlg = GetParent(hwnd);
    // Defer painting until done changing container attributes.
    CntDeferPaint( hwnd );
    hFont = GetWindowFont(hdlg);
    hOldFont = SelectObject(hdc, hFont);
    GetTextMetrics(hdc, &tm);
    // Init the vertical scrolling range to 0 since we have no records.
    CntRangeExSet( hwnd, 0, 0 );
    // Set the container view.
    CntViewSet( hwnd, CV_DETAIL );
    // Set the container colors.
    CntColorSet( hwnd, CNTCOLOR_CNTBKGD,    RGB(255, 255, 255) );
    CntColorSet( hwnd, CNTCOLOR_FLDTITLES,  RGB(  0,   0,   0) );
    CntColorSet( hwnd, CNTCOLOR_TEXT,       RGB(  0,   0,   0) );
    CntColorSet( hwnd, CNTCOLOR_GRID,       RGB(  0,   0,   0) );
    // Set some general container attributes.
    CntAttribSet(hwnd, CA_MOVEABLEFLDS | CA_SIZEABLEFLDS | CA_FLDTTL3D | CA_VERTFLDSEP);
    // Height of check box plus margins
    nCBHeight = tm.tmHeight - 1;
    // Set the height of the container control to that of an edit control
    GetClientRect(GetDlgItem(hdlg, IDC_COLNAME), &rect);

    nRowHeight = rect.bottom - rect.top;
    CntRowHtSet(hwnd, -nRowHeight, 0);
    // Define space for container field titles.
    CntFldTtlHtSet(hwnd, 1);
    CntFldTtlSepSet(hwnd);
    // Set up some cursors for the different areas.
    CntCursorSet( hwnd, LoadCursor(NULL, IDC_UPARROW ), CC_FLDTITLE );
    // Set some fonts for the different container areas.
    CntFontSet( hwnd, hFont, CF_GENERAL );
    // Initialise the drawing procedure for our owner draw fields.
    lpfnDrawProc = (DRAWPROC)MakeProcInstance((FARPROC)DrawVDBA2xAlterTblFldData, hInst);
    AllocContainerRecords(sizeof(CNTDATA));

    // Now paint the container.
    CntEndDeferPaint( hwnd, TRUE );
    // Subclass the container child window so we can operate the check boxes.
    EnumChildWindows(hwnd, EnumVDBA2xAlterTblCntChildProc, (LPARAM)(HWND FAR *)&hwndCntChild);
    wndprocCntChild = SubclassWindow(hwndCntChild, CntVDBA2xAlterTblChildSubclassProc);
    //
    // Create an edit window for the container.
    //
    hwndCntEdit = CreateWindow(
        "edit",
        "",
        WS_CHILD | WS_BORDER | ES_LEFT | ES_AUTOHSCROLL | WS_CLIPSIBLINGS,
        0, 0, 0, 0,
        hwndCntChild,
        (HMENU)ID_CNTEDIT,
        hInst,
        NULL);
    wndprocEdit = SubclassWindow(hwndCntEdit, VDBA2xAlterTblEditSubclassProc);
    SetWindowFont(hwndCntEdit, hFont, FALSE);
    //
    // Create a combobox for the container column data types
    //
    hwndType = CreateWindow(
        "COMBOBOX",
        "",
        WS_CHILD | CBS_DROPDOWNLIST | CBS_HASSTRINGS | CBS_SORT | WS_VSCROLL,
        0, 0, 0, 0,
        hwndCntChild,
        (HMENU)ID_CNTCOMBO,
        hInst,
        NULL);
    wndprocType = SubclassWindow(hwndType, VDBA2xAlterTblTypeSubclassProc);
    ComboBoxFillVDBA2xDataType (hwndType);

    SetWindowFont(hwndType, hFont, FALSE);
    CntFocRectEnable(hwnd, TRUE);
    lpcntInfo = CntInfoGet(hwnd);
    if (lpcntInfo->nFieldNbr < MAXIMUM_COLUMNS)
        bEnabled = TRUE;
    EnableWindow(GetDlgItem(hdlg, IDC_ADD), bEnabled);
    SelectObject(hdc, hOldFont);
    ReleaseDC(hwnd, hdc);
}


BOOL CALLBACK EXPORT EnumVDBA2xAlterTblCntChildProc(HWND hwnd, LPARAM lParam)
{
    HWND FAR * lphwnd = (HWND FAR *)lParam;
    char szClassName[100];

    GetClassName(hwnd, szClassName, sizeof(szClassName));
    if (lstrcmpi(szClassName, CONTAINER_CHILD) == 0)
    {
        *lphwnd = hwnd;
        return FALSE;
    }
    return TRUE;
}


LRESULT WINAPI EXPORT CntVDBA2xAlterTblChildSubclassProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        HANDLE_MSG(hwnd, WM_LBUTTONDOWN,    CntOnLButtonDown);
        HANDLE_MSG(hwnd, WM_LBUTTONUP,      CntOnLButtonUp);
        HANDLE_MSG(hwnd, WM_LBUTTONDBLCLK,  CntOnLButtonDown);
        HANDLE_MSG(hwnd, WM_COMMAND,        CntOnCommand);
        default:
            return VDBA2xAlterTblCntChildDefWndProc (hwnd, message, wParam, lParam);
   }
   return 0L;
}


LRESULT WINAPI EXPORT VDBA2xAlterTblCntChildDefWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    return CallWindowProc(wndprocCntChild, hwnd, message, wParam, lParam);
}


BOOL CALLBACK EXPORT DrawVDBA2xAlterTblFldData( 
    HWND   hwndCnt,     // container window handle
    LPFIELDINFO  lpFld, // field being drawn
    LPRECORDCORE lpRec, // record being drawn
    LPVOID lpData,      // data in internal format
    HDC    hDC,         // container window DC
    int    nXstart,     // starting position Xcoord
    int    nYstart,     // starting position Ycoord
    UINT   fuOptions,   // rectangle type
    LPRECT lpRect,      // rectangle of data cell
    LPSTR  lpszOut,     // data in string format
    UINT   uLenOut )    // len of lpszOut buffer
{
    TCHAR     szNumber[20];
    CNTDATA   data;
    LPCNTDATA lpdata    = (LPCNTDATA)lpData;
    COLORREF  crText    = CntColorGet(hwndCnt, CNTCOLOR_TEXT);
    COLORREF  crBkgnd   = CntColorGet(hwndCnt, CNTCOLOR_FLDBKGD);
    int       nDataType, nStringId;
    BOOL      bGrey     = FALSE;
    HWND      hcontainer= GetParent (hwndCnt);
    HWND      hdlg      = GetParent (hcontainer);
    LPRECORDCORE   lpRC;
    LPCOLUMNPARAMS lpC;
    LPTABLEPARAMS  lpTS  = (LPTABLEPARAMS)GetDlgProp (hdlg);
    //
    // Ensure our text comes out the color we expect
    //
    crText      = SetTextColor (hDC, crText);
    crBkgnd     = SetBkColor   (hDC, crBkgnd);
    nDataType   = GetRecordDataType (hwndCnt, lpRec);
    switch (nDataType)
    {
    case DT_TYPE:
        nStringId = GetDataTypeStringId (lpdata->uDataType);
        DrawStaticText(hDC, lpRect, NULL, TRUE, bGrey, IDS_TYPE);
        if (nStringId != -1)
        {
            lpRect->left += CB_BORDER_WIDTH + CB_FOCUS_WIDTH;
            DrawStaticText(hDC, lpRect, NULL, TRUE, bGrey, nStringId);
        }
        break;
    case DT_LEN:
        nStringId = IDS_LENGTH;
        memset (&data, 0, sizeof (data));
        lpRC = GetDataTypeRecord(hwndCnt, DT_TYPE);
        CntFldDataGet (lpRC, lpFld, sizeof (CNTDATA), &data);
        
        switch (data.uDataType)
        {
        case INGTYPE_DECIMAL:
            nStringId = IDS_PRESEL;
            bGrey = FALSE;
            break;
        case INGTYPE_FLOAT:
            nStringId = IDS_SIGDIG;
            bGrey = FALSE;
            break;
        default:
            {
                if (GetOIVers() >= OIVERS_30)
                {
                    lpC = VDBA20xTableColumn_Search (lpTS->lpColumns, (LPCTSTR)lpFld->lpFTitle);
                    if (lpC && IsAlterColumn(lpTS, lpFld->lpFTitle,FALSE))
                        lpC = NULL;
                }
                else
                {
                    // 
                    // The LEN is always grey for the existing column.
                    lpC = VDBA20xTableColumn_Search (lpTS->lpColumns, (LPCTSTR)lpFld->lpFTitle);
                }
                if (lpC)
                    bGrey = TRUE;
                else
                {
                    //
                    // LEN is enable only if the data type requires it:
                    LONG lMax = GetMaxReqLength(data.uDataType);
                    if (lMax == -1)
                        bGrey = TRUE;
                    else
                        bGrey = FALSE;
                }
            }
            break;
        }
        DrawStaticText(hDC, lpRect, NULL, TRUE, bGrey, nStringId);
        MakeTypeLengthString(data.uDataType, lpdata, szNumber);
        lpRect->left += EDIT_BORDER_WIDTH + EDIT_CURSOR_WIDTH + EDIT_LEFT_MARGIN;
        DrawStaticText(hDC, lpRect, szNumber, FALSE, bGrey, 0);
        break;
    case DT_NULLABLE:
        /* version before Ingres 2.5 :Atrribute Nullable is always grey for alter table.*/
        /* version Ingres 2.5 :Attribute Nullable is available.*/
        if (GetOIVers() >= OIVERS_25)
        {
            if (GetOIVers() >= OIVERS_30)
            {
                lpC = VDBA20xTableColumn_Search (lpTS->lpColumns, (LPCTSTR)lpFld->lpFTitle);
                if (lpC && IsAlterColumn(lpTS, lpFld->lpFTitle,FALSE))
                {
                    CNTDATA   dataAttr;
                    lpRec = GetDataTypeRecord(hwndCnt, DT_UNIQUE);
                    CntFldDataGet(lpRec, lpFld, sizeof (CNTDATA), &dataAttr);
                    if (dataAttr.bUnique)
                    {
                        DrawCheckBox(hDC, lpRect, lpdata->bNullable, IDS_NULLABLE, TRUE);
                        break;
                    }
                    else
                        lpC = NULL;
                }
            }
            else
            {
                // 
                // The attribute Nullable is not grey for the newly added column.
                lpC = VDBA20xTableColumn_Search (lpTS->lpColumns, (LPCTSTR)lpFld->lpFTitle);
            }
            if (!lpC)
            {
                lpRC = GetDataTypeRecord(hwndCnt, DT_TYPE);
                CntFldDataGet (lpRC, lpFld, sizeof (CNTDATA), &data);
                if (data.uDataType == INGTYPE_LONGVARCHAR ||
                    data.uDataType == INGTYPE_LONGBYTE    ||
                    data.uDataType == INGTYPE_UNICODE_LNVCHR)
                    bGrey = TRUE; //Nullable is always greyed for long data type.
                else
                    bGrey = FALSE;
            }
            else
                bGrey = TRUE;
        }
        else
            bGrey = TRUE;
        DrawCheckBox(hDC, lpRect, lpdata->bNullable, IDS_NULLABLE, bGrey);
        break;
    case DT_UNIQUE:
        // 
        // The attribute Unique is always grey for the newly added column.
        lpC = VDBA20xTableColumn_Search (lpTS->lpColumns, (LPCTSTR)lpFld->lpFTitle);
        if (!lpC)
            bGrey = TRUE;
        if (lpC && lpC->bNullable)
            bGrey = TRUE;
        DrawCheckBox(hDC, lpRect, lpdata->bUnique, IDS_UNIQUE, bGrey);
        break;
    case DT_DEFAULT:
        // 
        // The attribute "Default" is hidden for the newly added column with long data type.
        lpC = VDBA20xTableColumn_Search (lpTS->lpColumns, (LPCTSTR)lpFld->lpFTitle);
        
        if (!lpC)
        {
            lpRC = GetDataTypeRecord(hwndCnt, DT_TYPE);
            CntFldDataGet (lpRC, lpFld, sizeof (CNTDATA), &data);
            if (data.uDataType == INGTYPE_LONGVARCHAR ||
                data.uDataType == INGTYPE_LONGBYTE    ||
                data.uDataType == INGTYPE_UNICODE_LNVCHR)
                break; //"Default" is hidden for long data type.
            else
                bGrey = FALSE;
        }
        else
            bGrey = TRUE;

        /* The attribute "Default" is always grey for alter table */
        DrawCheckBox (hDC, lpRect, lpdata->bDefault, IDS_DEFAULT, TRUE);
        break;
    case DT_DEFSPEC:
        // The attribute "Def. Spec." is always hide for the newly added column.
        lpC = VDBA20xTableColumn_Search (lpTS->lpColumns, (LPCTSTR)lpFld->lpFTitle);
        if (!lpC)
            break;
        bGrey = TRUE;
        DrawStaticText(hDC, lpRect, NULL, TRUE, bGrey, IDS_DEFSPEC);
        lpRect->left += EDIT_BORDER_WIDTH + EDIT_CURSOR_WIDTH + EDIT_LEFT_MARGIN;
        if (lpdata->lpszDefSpec)
            DrawStaticText(hDC, lpRect, lpdata->lpszDefSpec, FALSE, bGrey, 0);
        break;
    case DT_SYSMAINTAINED:
        lpRC = GetDataTypeRecord(hwndCnt, DT_TYPE);
        CntFldDataGet (lpRC, lpFld, sizeof (CNTDATA), &data);
        if (data.uDataType == INGTYPE_OBJKEY || data.uDataType == INGTYPE_TABLEKEY )
        {
            /* the attribute "System Maintained" is always greyed. No modification available */
            DrawCheckBox(hDC, lpRect, lpdata->bSystemMaintained, IDS_SYST_MAINTAINED, TRUE);
        }
        break;
    default:
        break;
    }
    SetTextColor(hDC, crText);
    SetBkColor  (hDC, crBkgnd);
    //
    // Repaint any child windows
    //
    if (IsWindowVisible(hwndCntEdit))
        InvalidateRect (hwndCntEdit, NULL, TRUE);
    if (IsWindowVisible(hwndType))
        InvalidateRect(hwndType, NULL, TRUE);
    return TRUE;
}


static void DrawCheckBox(HDC hdc, LPRECT lprect, BOOL bSelected, int nStringId, BOOL bGrey)
/*
   Function:
      Draws a check box in the container field.

   Parameters:
      hdc         - Device context for container field.
      lprect      - Drawing rectangle.
      bSelected   - If TRUE then check box is to be selected.
      nStringId   - ID of string to display with the check box.
      bGrey       - Draw text greyed out.

   Returns:
      None.
*/
{
    RECT rect;
    char szText[50];
    int nVertOffset;
    COLORREF crText;

    // Center the check box vertically in the drawing rectangle
    nVertOffset = ((lprect->bottom - lprect->top) - nCBHeight) / 2;
    rect.left   = lprect->left + BOX_LEFT_MARGIN;
    rect.top    = lprect->top + nVertOffset;
    rect.right  = rect.left + nCBHeight + 1;
    rect.bottom = rect.top + nCBHeight + 1;
    Rectangle3D (hdc, rect.left, rect.top, rect.right, rect.bottom);
    if (bSelected)
        DrawCheck3D(hdc, rect.left, rect.top, rect.right, rect.bottom, bGrey);
    // Draw the text associated with the check box.
    rect.left = rect.right + TEXT_LEFT_MARGIN;
    rect.right = lprect->right;
    LoadString(hResource, nStringId, szText, sizeof(szText));
    rect.top--;
    if (bGrey)
        crText = SetTextColor(hdc, DARK_GREY);
    ExtTextOut (
        hdc,
        rect.left,
        rect.top,
        ETO_CLIPPED,
        &rect,
        szText,
        lstrlen(szText),
        (LPINT)NULL);
    if (bGrey)
        SetTextColor(hdc, crText);
}


static void CntOnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    switch (id)
    {
    case ID_CNTEDIT:
        OnCntEdit (hwnd, hwndCtl, codeNotify);
        break;
    case ID_CNTCOMBO:
        OnCntCombo (hwnd, hwndCtl, codeNotify);
        break;
    default:
        break;
    }
}


static void CntOnLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags)
{
    LPCOLUMNPARAMS lpFound = NULL;
    POINT ptMouse;
    LPFIELDINFO lpFI;
    LPRECORDCORE lpRC = NULL, lpRec = NULL;
    HWND hwndCnt = GetParent (hwnd);
    HWND hDlg    = GetParent (hwndCnt);
    LPTABLEPARAMS lpTS = GetDlgProp (hDlg);

    //
    // Forward message to take care of the focus rect.
    //
    FORWARD_WM_LBUTTONDOWN(hwnd, fDoubleClick, x, y, keyFlags, VDBA2xAlterTblCntChildDefWndProc);
    ptMouse.x = x;
    ptMouse.y = y;
    lpFI = CntFldAtPtGet (hwndCnt, &ptMouse);
    if (!lpFI)
        return;
    if (lpTS && (lpTS->StorageParams.nStructure == IDX_VW || lpTS->StorageParams.nStructure == IDX_VWIX))
	return;
    if (lpRC = CntRecAtPtGet(hwndCnt, &ptMouse))
    {
        CNTDATA   data, dataAttr;
        LPCNTINFO lpcntInfo = CntInfoGet(hwndCnt);
        int  nId;
        RECT rect;
        //
        // Get the data for this cell
        //
        memset (&dataAttr, 0, sizeof (dataAttr));
        memset (&data,     0, sizeof (data));
        CntFldDataGet(lpRC, lpFI, sizeof(data), &data);
        LocalGetFocusCellRect(hwndCnt, &rect);
        nId = GetRecordDataType(hwndCnt, lpRC);

        switch (nId)
        {
        case DT_TYPE:
            if (!AllowTypeChanged (hDlg, lpFI))
                break;
            ShowTypeWindow(hwndCnt, &rect);
            break;
        case DT_LEN:
            //
            // Only allow edit if data type requires it
            //
            if (VDBA20xTableColumn_Search (lpTS->lpColumns, (LPCTSTR)lpFI->lpFTitle))
            {
                if (GetOIVers() >= OIVERS_30)
                {
                    if (!IsAlterColumn(lpTS, lpFI->lpFTitle,TRUE))
                        break;
                }
                else
                    break;
            }
            lpRec = GetDataTypeRecord(hwndCnt, DT_TYPE);
            CntFldDataGet(lpRec, lpFI, sizeof (CNTDATA), &dataAttr);
            switch (dataAttr.uDataType)
            {
            case INGTYPE_C          :
            case INGTYPE_TEXT       :
            case INGTYPE_CHAR       :
            case INGTYPE_VARCHAR    :
            case INGTYPE_BYTE       :
            case INGTYPE_BYTEVAR    :
            case INGTYPE_UNICODE_NCHR:
            case INGTYPE_UNICODE_NVCHR:
                ShowEditWindow(hwndCnt, &rect, nId, &data);
                break;
            case INGTYPE_FLOAT      :
            case INGTYPE_DECIMAL:
                if (VDBA20xTableColumn_Search (lpTS->lpColumns, lpFI->lpFTitle))
                    break; // Alter Table does not allow the change of numeric data type.
                ShowEditWindow(hwndCnt, &rect, nId, &data);
                break;
            default:
                break;
            }
            break;
        case DT_NULLABLE:
            /* with Ingres 2.5 you can add column with attribute NOT NULLABLE */
            /* and you can change the column attribute.*/
            lpFound = VDBA20xTableColumn_Search (lpTS->lpColumns, (LPCTSTR)lpFI->lpFTitle);
            if (lpFound)
            {
                if (GetOIVers() >= OIVERS_30)
                {
                    if (!IsAlterColumn(lpTS, lpFI->lpFTitle,TRUE))
                        break;
                    else
                    {
                        lpRec = GetDataTypeRecord(hwndCnt, DT_UNIQUE);
                        CntFldDataGet(lpRec, lpFI, sizeof (CNTDATA), &dataAttr);
                        if (dataAttr.bUnique)
                            break;
                    }
                }
                else
                    break;  // Existing Column
            }
            lpRec = GetDataTypeRecord(hwndCnt, DT_TYPE);
            CntFldDataGet(lpRec, lpFI, sizeof (CNTDATA), &dataAttr);
            if (dataAttr.uDataType == INGTYPE_LONGVARCHAR ||
                dataAttr.uDataType == INGTYPE_LONGBYTE    ||
                dataAttr.uDataType == INGTYPE_UNICODE_LNVCHR)
                break; // Ignore check/uncheck for lon data type
            else
            {
                lpRec = GetDataTypeRecord(hwndCnt, DT_NULLABLE);
                data.bNullable = !data.bNullable;
                CntFldDataSet(lpRec, lpFI, sizeof(data), &data);
                CntUpdateRecArea(hwndCnt, lpRec, lpFI);

                memset (&dataAttr, 0, sizeof (dataAttr));
                lpRec = GetDataTypeRecord(hwndCnt, DT_DEFAULT);
                CntFldDataGet(lpRec, lpFI, sizeof (CNTDATA), &dataAttr);
                dataAttr.bDefault = !data.bNullable;
                CntFldDataSet (lpRec, lpFI, sizeof(dataAttr), &dataAttr);
                CntUpdateRecArea(hwndCnt, NULL, lpFI);
            }
            break;
        case DT_DEFAULT:
            /* break; */
            if (VDBA20xTableColumn_Search (lpTS->lpColumns, (LPCTSTR)lpFI->lpFTitle))
            {
                if ( GetOIVers() >= OIVERS_30 )
                {
                    if (!IsAlterColumn(lpTS, lpFI->lpFTitle,TRUE) )
                        break;
                }
                else
                    break;
            }
            data.bDefault = !data.bDefault;
            CntFldDataSet(lpRec, lpFI, sizeof(data), &data);
            CntUpdateRecArea(hwndCnt, NULL, lpFI);
            break;
        case DT_UNIQUE:
            {
            BOOL bDefined = FALSE;
            LPTLCUNIQUE    ls;
            LPCOLUMNPARAMS lpcp;
            lpcp = VDBA20xTableColumn_Search (lpTS->lpColumns, (LPCTSTR)lpFI->lpFTitle);
            //
            // The newly added column cannot be declared as NOT NULL
            // So, it cannot be unique key.
            if (!VDBA20xTableColumn_Search (lpTS->lpColumns, lpFI->lpFTitle))
                break;

            if (lpcp && lpcp->bNullable)
                break;  // Nullable Column cannot be Unique.
            ls = lpTS->lpUnique;
            while (ls)
            {
                if (StringList_Count (ls->lpcolumnlist) == 1 && lstrcmpi ((ls->lpcolumnlist)->lpszString, lpFI->lpFTitle) == 0)
                {
                    bDefined = TRUE;
                    break;
                }
                ls = ls->next;
            }
            if (data.bUnique && bDefined)
            {
                // tchszMsg4
                MessageBox (hwnd, ResourceString ((UINT)IDS_E_USESUBDLG_2_DELETECONSTRAINT),
                                  ResourceString ((UINT)IDS_TITLE_WARNING), MB_ICONEXCLAMATION|MB_OK);
                break;
            }
            data.bUnique = !data.bUnique;
            CntFldDataSet(lpRC, lpFI, sizeof(data), &data);
            CntUpdateRecArea(hwndCnt, lpRC, lpFI);
            }
            break;
        case DT_DEFSPEC:
            if (VDBA20xTableColumn_Search (lpTS->lpColumns, (LPCTSTR)lpFI->lpFTitle))
            {
                if (GetOIVers() >= OIVERS_30)
                    IsAlterColumn(lpTS, lpFI->lpFTitle,TRUE);
                break;  // Cannot change column's attribute.
            }
            lpRec = GetDataTypeRecord (hwndCnt, DT_DEFAULT);
            CntFldDataGet (lpRec, lpFI, sizeof(dataAttr), &dataAttr);
            if (!dataAttr.bDefault)
                break;
            ShowEditWindow(hwndCnt, &rect, nId, &data);
            break;
        case DT_SYSMAINTAINED:
            /* no change available for "system maintained" attribut*/
            break;
        default:
            break;
        }
    }
    else
    if (fDoubleClick)
    {
        // Check to see if we are in the title area.
        RECT rect;
        LPRECORDCORE lpRCFocus = CntFocusRecGet(hwndCnt);
        // Set the focus so the title rectangle calculations work.
        CntFocusSet(hwndCnt, lpRCFocus, lpFI);
        GetTitleRect(hwndCnt, &rect);
        rect.bottom++;
        if (PtInRect(&rect, ptMouse))
        {
            // In the title area.
            rect.bottom--;
            // 17-June-1999 (schph01)bug #97504
            // disabled the "change column name" functionality
            //ShowEditWindow(hwndCnt, &rect, DT_TITLE, lpFI->lpFTitle);
        }
    }
}


static void CntOnLButtonUp(HWND hwnd, int x, int y, UINT keyFlags)
{
    // Check to see if we are in the title area.
    HWND hwndCnt = GetParent(hwnd);
    RECT rect;
    LPRECORDCORE lpRCFocus = CntFocusRecGet(hwndCnt);
    LPFIELDINFO lpFI;
    POINT ptMouse;
    HWND hdlg = GetParent(hwndCnt);

    FORWARD_WM_LBUTTONUP(hwnd, x, y, keyFlags, VDBA2xAlterTblCntChildDefWndProc);
    ptMouse.x = x;
    ptMouse.y = y;

    if ((lpFI = CntFldAtPtGet(hwndCnt, &ptMouse)) && !CntRecAtPtGet(hwndCnt, &ptMouse))
    {
        // Set the focus so the title rectangle calculations work.
        CntFocusSet(hwndCnt, lpRCFocus, lpFI);
        GetTitleRect(hwndCnt, &rect);
        rect.bottom++;
        if (PtInRect(&rect, ptMouse))
        {
            // In the title area.
            rect.bottom--;
            Edit_SetText(GetDlgItem(hdlg, IDC_COLNAME), lpFI->lpFTitle);
            Edit_SetSel(GetDlgItem(hdlg,  IDC_COLNAME), 0, -1);
        }
    }
}


LRESULT WINAPI EXPORT VDBA2xAlterTblEditSubclassProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        HANDLE_MSG(hwnd, WM_KILLFOCUS,  Edit_OnKillFocus);
        HANDLE_MSG(hwnd, WM_KEYDOWN,    Edit_OnKey);
        HANDLE_MSG(hwnd, WM_CHAR,       Edit_OnChar);
        HANDLE_MSG(hwnd, WM_GETDLGCODE, Edit_OnGetDlgCode);
        default:
            return VDBA2xAlterTblEditDefWndProc (hwnd, message, wParam, lParam);
   }
   return 0L;
}


LRESULT WINAPI EXPORT VDBA2xAlterTblEditDefWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    return CallWindowProc(wndprocEdit, hwnd, message, wParam, lParam);
}


static void Edit_OnKillFocus(HWND hwnd, HWND hwndNewFocus)
{
    HWND hwndCnt = GetParent(GetParent(hwnd));
    int  nType   = GetRecordDataType(hwndCnt, lpEditRC);
    LPTABLEPARAMS lpTS = GetDlgProp (GetParent (hwndCnt));
    LPCOLUMNPARAMS lpCol = NULL;
    LPFIELDINFO    lpFI;
    BOOL bAllowChange = TRUE;

    switch (nType)
    {
        case DT_TITLE:
        {
            // Ensure we dont get into a situation where the column name
            // is blank. If we are editing the column name and it is blank,
            // reset the name to what it was before the edit.
            if (Edit_GetTextLength(hwnd) == 0)
                ResetCntEditCtl(hwndCnt);
            break;
        }
    }
    //
    // Alter Table. You can increase the length of characters but you cannot
    // decrease it. You cannot change the length of numeric data type.
    //
    lpFI  = CntFocusFldGet(hwndCnt);
    if (GetOIVers() >= OIVERS_30)
    {
        lpCol=NULL;
    }
    else
        lpCol = VDBA20xTableColumn_Search (lpTS->lpColumns, lpFI->lpFTitle);
    if (lpFI && lpCol && nType == DT_LEN)
    {
        DWORD   length = 0L;
        TCHAR tchszText [16];
        
        switch (lpCol->uDataType)
        {
        case INGTYPE_C          :
        case INGTYPE_TEXT       :
        case INGTYPE_CHAR       :
        case INGTYPE_VARCHAR    :
        //case INGTYPE_LONGVARCHAR:
        case INGTYPE_BYTE       :         
        case INGTYPE_BYTEVAR    :      
        //case INGTYPE_LONGBYTE   :     
            GetWindowText (hwndCntEdit, tchszText, sizeof(tchszText)); 
            length = atoi (tchszText);
            if (length < lpCol->lenInfo.dwDataLen)
                bAllowChange = FALSE;
            else
                bAllowChange = TRUE;
            break;
        default:
            bAllowChange = FALSE;
            break;
        }
    }
    if (bAllowChange)
        SaveEditData (hwndCnt);
    SetWindowPos (hwndCntEdit, HWND_BOTTOM, 0, 0, 0, 0, SWP_HIDEWINDOW);
    CntUpdateRecArea(hwndCnt, lpEditRC, lpEditFI);
    FORWARD_WM_KILLFOCUS(hwnd, hwndNewFocus, VDBA2xAlterTblEditDefWndProc);
}


static void Edit_OnChar(HWND hwnd, UINT ch, int cRepeat)
{
    HWND hwndCnt = GetParent(GetParent(hwnd));
    int  nId;
    BOOL bForward = FALSE;
    //
    // If the edit box has already lost focus don't let the char thru.
    // Since we process the ESCAPE, TAB, and RETURN on the keydown we
    // still get the WM_CHAR msg after the keyup. By the time we get it,
    // though, focus has shifted back to the container and the result
    // will be an annoying beep if the char msg goes thru. 
    //
    if (ch == VK_ESCAPE || ch == VK_RETURN || ch == VK_TAB)
    {
        if(GetFocus() != hwnd)
            return;
    }
    nId = GetRecordDataType(hwndCnt, lpEditRC);
    //
    // Handle the keys globally valid to all edit boxes
    //
    switch (ch)
    {
        case VK_BACK:
            bForward = TRUE;
    }
    //
    // Handle specific characters for each edit box type
    //
    switch (nId)
    {
        case DT_LEN:
        {
            // Only allow numeric characters, except for the decimal type
            // where we allow the comma as a separator between the precision
            // and scale lengths
            LPRECORDCORE lpRec = GetDataTypeRecord(hwndCnt, DT_TYPE);
            LPFIELDINFO lpFI = CntFocusFldGet(hwndCnt);
            CNTDATA dataType;
            char szNumber[20];

            Edit_GetText(hwnd, szNumber, sizeof(szNumber));
            //
            // If there is no comma and the first number is present then allow the comma.
            //
            if (!(_fstrchr(szNumber, ',')) && *szNumber)
            {
                CntFldDataGet(lpRec, lpFI, sizeof(dataType), &dataType);
                if (dataType.uDataType == INGTYPE_DECIMAL && ch == ',')
                    bForward = TRUE;
            }
            if ((_istalnum(ch) && !_istalpha(ch)) || bForward)
                FORWARD_WM_CHAR(hwnd, ch, cRepeat, VDBA2xAlterTblEditDefWndProc);
            break;
        }
  
        case DT_TITLE:
        {
            if (IsValidObjectChar(ch) || bForward)
                FORWARD_WM_CHAR(hwnd, ch, cRepeat, VDBA2xAlterTblEditDefWndProc);
            break;
        }

        case DT_DEFSPEC:
            break;
        default:
            break;
    }
}


static void Edit_OnKey(HWND hwnd, UINT vk, BOOL fDown, int cRepeat, UINT flags)
{
    HWND hwndCnt = GetParent(GetParent(hwnd));

    // We don't process key-up messages.
    if( !fDown )
    {
        // Forward this to the default proc for processing.
        FORWARD_WM_KEYUP(hwnd, vk, cRepeat, flags, VDBA2xAlterTblEditDefWndProc);
        return;
    }
    switch(vk)
    {
    case VK_ESCAPE:
        ResetCntEditCtl(hwndCnt);
        SetFocus(hwndCnt);
        break;
    case VK_UP:
    case VK_DOWN:
    case VK_RETURN:
    case VK_TAB:
        SetFocus(hwndCnt);
        // Send the RETURN or TAB to the container so it will notify
        // the app. The app will then move the focus appropriately.
        FORWARD_WM_KEYDOWN(hwndCnt, vk, cRepeat, flags, SendMessage);
        break;
    }
    // Forward this to the default proc for processing.
    FORWARD_WM_KEYDOWN(hwnd, vk, cRepeat, flags, VDBA2xAlterTblEditDefWndProc);
}


static UINT Edit_OnGetDlgCode(HWND hwnd, MSG FAR* lpmsg)
{
    UINT uRet;
    // Processing of this message is only necessary for controls
    // within dialogs. The return value of DLGC_WANTMESSAGE tells the
    // dialog manager that we want ALL keystroke msgs to come to this
    // edit control and no further processing of the keystroke msg is
    // to take place. If this message is not processed in this manner
    // the control will not receive the keystroke messages that you
    // really need (like TAB, ENTER, and ESCAPE) while in a dialog.
    // This message is being processed the same way in the container.

    // Forward this to the default proc for processing.
    uRet = (UINT)FORWARD_WM_GETDLGCODE(hwnd, lpmsg, VDBA2xAlterTblEditDefWndProc);
    // Now specify that we want all keyboard input.
    uRet |= DLGC_WANTMESSAGE;
    return uRet;
}

LRESULT WINAPI EXPORT VDBA2xAlterTblTypeSubclassProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        HANDLE_MSG(hwnd, WM_SETFOCUS,     Type_OnSetFocus);
        HANDLE_MSG(hwnd, WM_KILLFOCUS,    Type_OnKillFocus);
        HANDLE_MSG(hwnd, WM_KEYDOWN,      Type_OnKey);
        HANDLE_MSG(hwnd, WM_CHAR,         Type_OnChar);
        HANDLE_MSG(hwnd, WM_GETDLGCODE,   Type_OnGetDlgCode);
        default:
            return VDBA2xAlterTblTypeDefWndProc(hwnd, message, wParam, lParam);
    }
    return 0L;
}


LRESULT WINAPI EXPORT VDBA2xAlterTblTypeDefWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    return CallWindowProc(wndprocType, hwnd, message, wParam, lParam);
}

// MESSAGE NOTIFICATION:
// ON COMBOBOX ... 
//
static void Type_OnSetFocus(HWND hwnd, HWND hwndOldFocus)
{
    HWND hwndCnt = GetParent(GetParent(hwnd));

    lpTypeRC = CntFocusRecGet(hwndCnt);
    lpTypeFI = CntFocusFldGet(hwndCnt);
    FORWARD_WM_SETFOCUS(hwnd, hwndOldFocus, VDBA2xAlterTblTypeDefWndProc);
}


static void Type_OnKillFocus(HWND hwnd, HWND hwndNewFocus)
{
    HWND hwndCnt = GetParent(GetParent(hwnd));
    int nIdx;
    CNTDATA data;
    LPRECORDCORE lpRC;

    nIdx = ComboBox_GetCurSel (hwndType);       
    data.uDataType =(int)ComboBox_GetItemData(hwndType, nIdx); // A numeric number identifying the OpenIngres/Desktop type.
    CntFldDataSet   (lpTypeRC, lpTypeFI, sizeof(data), &data);
    SetWindowPos    (hwndType, HWND_BOTTOM, 0, 0, 0, 0, SWP_HIDEWINDOW);
    CntUpdateRecArea(hwndCnt, lpTypeRC, lpTypeFI);
    lpRC = GetDataTypeRecord(hwndCnt, DT_LEN);
    //
    // Ensure the data type length gets initialised with a valid value.
    //
    /*
    nMin = GetMinReqLength(data.uDataType);
    if (nMin != -1)
    {
        if (data.uDataType != INGTYPE_DECIMAL)
           data.len.dwDataLen = (DWORD)nMin;
        else
        {
           data.len.decInfo.nPrecision = DEFAULT_PRECISION;
           data.len.decInfo.nScale = DEFAULT_SCALE;
        }
        CntFldDataSet(lpRC, lpTypeFI, sizeof(data), &data);
    }
    */
    CntUpdateRecArea(hwndCnt, lpRC, lpTypeFI);
    lpRC = GetDataTypeRecord(hwndCnt, DT_TYPE);
    CntFldDataGet(lpRC, lpTypeFI, sizeof(data), &data);
    lpTypeRC = NULL;
    lpTypeFI = NULL;
    FORWARD_WM_KILLFOCUS(hwnd, hwndNewFocus, VDBA2xAlterTblTypeDefWndProc);
}


static void Type_OnChar(HWND hwnd, UINT ch, int cRepeat)
{
   HWND hwndCnt = GetParent(GetParent(hwnd));
   FORWARD_WM_CHAR(hwnd, ch, cRepeat, VDBA2xAlterTblTypeDefWndProc);
}


static void Type_OnKey(HWND hwnd, UINT vk, BOOL fDown, int cRepeat, UINT flags)
{
    HWND hwndCnt = GetParent(GetParent(hwnd));
    // We don't process key-up messages.
    if( !fDown )
    {
        // Forward this to the default proc for processing.
        FORWARD_WM_KEYUP(hwnd, vk, cRepeat, flags, VDBA2xAlterTblTypeDefWndProc);
        return;
    }
    switch(vk)
    {
      case VK_ESCAPE:
      {
         CNTDATA data;
         int nOffset;
         int nCount = ComboBox_GetCount(hwndType);
         // Reset the combo box to the original selection
         CntFldDataGet(lpTypeRC, lpTypeFI, sizeof(data), &data);
         for (nOffset = 0; nOffset < nCount; nOffset++)
         {
            UINT uType = (UINT)ComboBox_GetItemData(hwndType, nOffset);
            if (uType == data.uDataType)
            {
               ComboBox_SetCurSel(hwndType, nOffset);
               break;
            }
         }

         SetFocus(hwndCnt);
         break;
      }
      case VK_RETURN:
      case VK_TAB:
      {
         SetFocus(hwndCnt);
         CntFocusMove(hwndCnt, CFM_DOWN);

         // Send the RETURN or TAB to the container so it will notify
         // the app. The app will then move the focus appropriately.
         FORWARD_WM_KEYDOWN(hwndCnt, vk, cRepeat, flags, SendMessage);
         break;
      }
   }
   // Forward this to the default proc for processing.
   FORWARD_WM_KEYDOWN(hwnd, vk, cRepeat, flags, VDBA2xAlterTblTypeDefWndProc);
}


static UINT Type_OnGetDlgCode(HWND hwnd, MSG FAR* lpmsg)
{
   UINT uRet;

   // Processing of this message is only necessary for controls
   // within dialogs. The return value of DLGC_WANTMESSAGE tells the
   // dialog manager that we want ALL keystroke msgs to come to this
   // combo box control and no further processing of the keystroke msg is
   // to take place. If this message is not processed in this manner
   // the control will not receive the keystroke messages that you
   // really need (like TAB, ENTER, and ESCAPE) while in a dialog.
   // This message is being processed the same way in the container.

   // Forward this to the default proc for processing.
   uRet = (UINT)FORWARD_WM_GETDLGCODE(hwnd, lpmsg, VDBA2xAlterTblTypeDefWndProc);

   // Now specify that we want all keyboard input.
   uRet |= DLGC_WANTMESSAGE;

   return uRet;
}


static BOOL CntFindField(HWND hwnd, LPSTR lpszTitle)
/*
   Function:
      Searches the container fields for a specified field title.

   Parameters:
      hwnd        - Handle to the container window
      lpszTitle   - Field title to search for.

   Returns:
      TRUE if title is found.
*/
{
   LPRECORDCORE lpRC;
   LPFIELDINFO lpFld;

   lpRC = CntRecHeadGet(hwnd);
   lpFld = CntFldHeadGet(hwnd);
   while (lpFld)
   {
      if (lstrcmpi(lpFld->lpFTitle, lpszTitle) == 0)
         return TRUE;

      lpFld = CntNextFld(lpFld);
   }
   return FALSE;
}


static void ComboBoxFillVDBA2xDataType (HWND hwndCtrl)
{
   if (GetOIVers() >= OIVERS_91)
      OccupyComboBoxControl(hwndCtrl, typeTypesAnsiDate);
   else if (GetOIVers() >= OIVERS_30)
      OccupyComboBoxControl(hwndCtrl, typeTypesBigInt);
   else if (GetOIVers() >= OIVERS_26)
      OccupyComboBoxControl(hwndCtrl, typeTypesUnicode);
   else
      OccupyComboBoxControl(hwndCtrl, typeTypes);
   ComboBox_SetCurSel (hwndCtrl, 0);
}



// duplicate code (oh!) from creattbl.c
// Modified Emb 27-28 Aug 97:
// cannot rely on HIWORD(lpszText) to distinguish between string and MAKEINTRESOURCE
// also, cannot use LOWORD to extract id from lpszText
// New code receives a complimentary string ID
// 2 cases:
//   - lpszText null and stringID not null  ---> use stringID
//   - lpszText not null and stringID null  ---> use lpszText
//
static void DrawStaticText(HDC hDC, LPRECT lprect, LPCSTR lpszText, BOOL bAlignLeft, BOOL bGrey, UINT uStringId)
{
    char szText[50];
    RECT rect;
    DWORD dwLen;
    int cbLen;
    int nVertOffset;
    TEXTMETRIC tm;
    COLORREF crText;
    
    rect = *lprect;

    // not-so-good code: if (HIWORD(lpszText) != 0)
    if (lpszText)
    {
        assert (uStringId == 0);
        fstrncpy(szText, (LPUCHAR) lpszText, min(lstrlen(lpszText) + 1, sizeof(szText)));
        // Ensure we are null terminated
        szText[sizeof(szText) - 1] = (char)NULL;
        cbLen = lstrlen(szText);
    }
    else
    {
      assert (uStringId != 0);
        // not-so-good code: used LOWORD(lpszText)
      cbLen = LoadString(hResource, uStringId, szText, sizeof(szText));
    }

    if (bAlignLeft)
    {
        // Line the text up with the check boxes
        rect.left += BOX_LEFT_MARGIN;
    }
    // Center the text vertically in the drawing rectangle
    GetTextMetrics(hDC, &tm);
    nVertOffset = ((rect.bottom - rect.top) - tm.tmHeight) / 2;
    if (bGrey)
        crText = SetTextColor(hDC, DARK_GREY);
    ExtTextOut (hDC, rect.left, rect.top + nVertOffset, ETO_CLIPPED, &rect, szText, lstrlen(szText), (LPINT)NULL);
    if (bGrey)
        SetTextColor(hDC, crText);
    // Adjust the rectangle to the end of the string
    dwLen = GetTextExtent(hDC, szText, cbLen);
    lprect->left += LOWORD(dwLen);
}

static BOOL AllocContainerRecords(UINT uSize)
{
   int nOffset = 0;

   while (cntRecords[nOffset])
   {
      LPCNTDATA FAR * lplpcntData = cntRecords[nOffset];
      *lplpcntData = (LPCNTDATA)ESL_AllocMem(uSize);
      if (!*lplpcntData)
      {
         FreeContainerRecords();
         return FALSE;
      }
      nOffset++;
   }
   uDataSize = uSize;
   return TRUE;
}


static BOOL ReAllocContainerRecords(UINT uNewSize)
{
   int nOffset = 0;

   while (cntRecords[nOffset])
   {
      LPCNTDATA FAR * lplpcntData = cntRecords[nOffset];
      if (*lplpcntData)
      {
         *lplpcntData = (LPCNTDATA)ESL_ReAllocMem(*lplpcntData, uNewSize, uDataSize);
         if (!*lplpcntData)
         {
            FreeContainerRecords();
            return FALSE;
         }
      }
      nOffset++;
   }
   uDataSize = uNewSize;
   return TRUE;
}


static void FreeContainerRecords(void)
{
   int nOffset = 0;

   while (cntRecords[nOffset])
   {
      LPCNTDATA FAR * lplpcntData = cntRecords[nOffset];
      if (*lplpcntData)
         ESL_FreeMem (*lplpcntData);
      *lplpcntData = NULL;
      nOffset++;
   }
   uDataSize = 0;
}


static void CopyContainerRecords(HWND hwndCnt)
/*
   Function:
      Copies all the records from the container into our own buffers.

   Parameters:
      hwndCnt     - Handle to the container window

   Returns:
      None.
*/
{
   LPRECORDCORE lpRC;
   int nOffset = 0;

   lpRC = CntRecHeadGet(hwndCnt);
   while (cntRecords[nOffset] && lpRC)
   {
      LPCNTDATA FAR * lplpcntData = cntRecords[nOffset];
      if (*lplpcntData)
         CntRecDataGet(hwndCnt, lpRC, (LPVOID)*lplpcntData);
      lpRC = CntNextRec(lpRC);
      nOffset++;
   }
}


static void InitialiseFieldData(UINT uField)
{
   int nOffset = 0;

   while (cntRecords[nOffset])
   {
      LPCNTDATA FAR * lplpcntData = cntRecords[nOffset];
      if (*lplpcntData)
      {
         LPCNTDATA lpThisData = *lplpcntData + uField;
         _fmemset(lpThisData, 0, sizeof(CNTDATA));
      }
      nOffset++;
   }
}


static void AddRecordsToContainer(HWND hwndCnt)
{
   int nOffset = 0;

   CntKillRecList(hwndCnt);
   while (cntRecords[nOffset])
   {
      LPCNTDATA FAR * lplpcntData = cntRecords[nOffset];
      if (*lplpcntData)
      {
         LPRECORDCORE lpRC = CntNewRecCore(uDataSize);
         CntRecDataSet(lpRC, (LPVOID)*lplpcntData);
         CntAddRecTail(hwndCnt, lpRC);
      }
      nOffset++;
   }
}

static int GetRecordDataType(HWND hwndCnt, LPRECORDCORE lpRCIn)
/*
   Function:
      Get the container data type for a given container record.

   Parameters:
      hwndCnt     - Handle to the container window.
      lpRCIn      - Pointer to the container record.

   Returns:
      Container data type.

   Note:
      See the list of DT_xxx defines for all valid container data types.
*/
{
   LPRECORDCORE lpRC;
   int nOffset = 0;

   if (!lpRCIn)
      return DT_TITLE;
   lpRC = CntRecHeadGet(hwndCnt);
   while (lpRC)
   {
      if (lpRCIn == lpRC)
         return nOffset;
      nOffset++;
      lpRC = CntNextRec(lpRC);
   }
   // Data type not found.  This should never happen.
   ASSERT(NULL);
   return -1;
}


static void LocalGetFocusCellRect(HWND hwndCnt, LPRECT lprect)
/*
   Function:
      Calculates the rectangle of the data area for the cell that
      currently has the focus.

   Parameters:
      hwndCnt     - Handle to the container window
      lprect      - Pointer to retangle for return value.
      
   Returns:
      None.
*/
{
   LPFIELDINFO lpFI;
   int nXOrg, nYOrg;
   LPCNTINFO lpcntInfo = CntInfoGet(hwndCnt);

   CntFocusOrgExGet(hwndCnt, &nXOrg, &nYOrg, FALSE);
   lpFI = CntFocusFldGet(hwndCnt);
   lprect->top = nYOrg;
   lprect->left = nXOrg;
   lprect->right = lprect->left + lpFI->cxPxlWidth;
   lprect->bottom = lprect->top + lpcntInfo->cyRow;
}


static void ShowEditWindow(HWND hwndCnt, LPRECT lprect, int nType, LPCNTDATA lpdata)
/*
   Function:
      Sets up and displays the edit window within the container for the
      given data type.

   Parameters:
      hwndCnt     - Handle to the container window
      lprect      - Drawing rectangle
      nType       - Type of data to edit
      lpdata      - If HIWORD(lpdata) = NULL then use the character passed
                    in the LOWORD.  Otherwise, lpdata points to the data
                    structure containing the string to be displayed, or for
                    DT_TITLE, lpdata is a pointer directly to the string.

   Returns:
      None.
*/
{
    int     nLen, nStringId, nLeftMargin = EDIT_LEFT_MARGIN;
    char    szText[50];
    char    szNumber[20];
    HDC     hdc;
    HFONT   hfont;
    DWORD   dwLen;
    LPSTR   lpszString;
    BOOL    bUseCurrent = (HIWORD(lpdata) == 0 ? FALSE : TRUE);
    UINT    uChar = LOWORD(lpdata);

    hdc     = GetDC(hwndCnt);
    hfont   = SelectObject(hdc, CntFontGet(hwndCnt, CF_GENERAL));
    Edit_SetText(hwndCntEdit, "");
    switch (nType)
    {
        case DT_LEN:
        {
            //
            // Get the current length and display it.
            //
            LPFIELDINFO     lpFI = CntFocusFldGet(hwndCnt);
            LPRECORDCORE    lpRC;
            CNTDATA data;

            lpRC = GetDataTypeRecord (hwndCnt, DT_TYPE);
            CntFldDataGet (lpRC, lpFI, sizeof (CNTDATA), &data);
            nStringId = IDS_LENGTH;
            switch (data.uDataType)
            {
            case INGTYPE_C          :
            case INGTYPE_TEXT       :
            case INGTYPE_CHAR       :
            case INGTYPE_VARCHAR    :
            case INGTYPE_BYTE       :
            case INGTYPE_BYTEVAR    :
            case INGTYPE_UNICODE_NCHR:
            case INGTYPE_UNICODE_NVCHR:
                nLen = 5;
                break;
            case INGTYPE_DECIMAL:
                nStringId = IDS_PRESEL;
                nLen = 32;
                break;
            case INGTYPE_FLOAT      :
                nStringId = IDS_SIGDIG;
                nLen = 2;
                break;
            default:
                break;
            }
        
            if (bUseCurrent)
            {
                MakeTypeLengthString(data.uDataType, lpdata, szNumber);
                lpszString = szNumber;
            }
            break;
        }
        
        case DT_TITLE:
        {
            nLen = MAXOBJECTNAME - 1;
            nStringId = 0;
            if (bUseCurrent)
                lpszString = (LPSTR)lpdata;
            nLeftMargin = 0;
            break;
        }
        case DT_DEFSPEC:
            nLen = MAX_DEFAULTSTRING;
            nStringId = IDS_DEFSPEC;
            if (bUseCurrent)
               lpszString = lpdata->lpszDefSpec;
            break;
        
        default:
            ASSERT(NULL);
    }
    if (nStringId)
    {
        LoadString(hResource, nStringId, szText, sizeof(szText));
        dwLen = GetTextExtent(hdc, szText, lstrlen(szText));
    }
    else
        dwLen = 0;
    Edit_LimitText(hwndCntEdit, nLen);
    SetWindowPos (
        hwndCntEdit,
        HWND_TOP,
        lprect->left + (int)LOWORD(dwLen) + nLeftMargin,
        lprect->top,
        lprect->right - lprect->left -
        (int)LOWORD(dwLen) - nLeftMargin - 2, // VUT OIDT
        lprect->bottom - lprect->top,
        SWP_SHOWWINDOW);
    SetFocus (hwndCntEdit);
    SelectObject (hdc, hfont);
    ReleaseDC (hwndCnt, hdc);
    //
    // Either show the character that started the edit or the current data
    //
    if (bUseCurrent)
    {
        Edit_SetText(hwndCntEdit, lpszString);
        Edit_SetSel(hwndCntEdit, 0, -1);
    }
    else
    {
        FORWARD_WM_CHAR(hwndCntEdit, uChar, 0, SendMessage);
    }
}


static void ShowTypeWindow(HWND hwndCnt, LPRECT lprect)
/*
   Function:
      Sets up and displays the type combo box window within the container.

   Parameters:
      hwndCnt     - Handle to the container window
      lprect      - Drawing rectangle

   Returns:
      None.
*/
{
    HDC     hdc;
    HFONT   hfont;
    SIZE    size;
    int     nStringId;
    char    szText[50];
    BOOL    isOK;
    CNTDATA data;
    LPFIELDINFO     lpFI;
    LPRECORDCORE    lpRC;

    hdc     = GetDC (hwndCnt);
    hfont   = SelectObject(hdc, CntFontGet(hwndCnt, CF_GENERAL));
    LoadString(hResource, IDS_TYPE, szText, sizeof(szText));
#ifdef WIN32
    isOK    = GetTextExtentPoint32 (hdc, (LPCTSTR)szText, lstrlen(szText), &size);
#else
    isOK    = GetTextExtentPoint   (hdc, (LPCSTR)szText, lstrlen(szText), &size);
#endif
    //
    // Select the current data type into the combo box
    //
    lpFI = CntFocusFldGet   (hwndCnt);
    lpRC = GetDataTypeRecord(hwndCnt, DT_TYPE);
    CntFldDataGet(lpRC, lpFI, sizeof(data), &data);
    nStringId = GetDataTypeStringId (data.uDataType);

    if (nStringId != -1)
    {
        LoadString(hResource, nStringId, szText, sizeof(szText));
        ComboBox_SelectString(hwndType, -1, szText);
    }
    SetWindowPos (
        hwndType,
        HWND_TOP,
        lprect->left + size.cx + EDIT_LEFT_MARGIN,
        lprect->top,
        lprect->right - lprect->left - size.cx - EDIT_LEFT_MARGIN -2, // VUT OIDT
        TYPE_WINDOW_HEIGHT,
        SWP_SHOWWINDOW);
   SetFocus (hwndType);
   SelectObject (hdc, hfont);
   ReleaseDC (hwndCnt, hdc);
}


static void SaveEditData(HWND hwndCnt)
/*
   Function:
      Gets the data from the container edit control and stores it in
      the place in the container data structure.

   Parameters:
      hwndCnt     - Handle to the container window.

   Returns:
      None.
*/
{
    CNTDATA  data;
    ZEROINIT(data);
    if (lpEditRC && lpEditFI)
        CntFldDataGet(lpEditRC, lpEditFI, sizeof(data), &data);

    switch (GetRecordDataType(hwndCnt, lpEditRC))
    {
        case DT_LEN:
        {
            char szNumber[20];
            LPRECORDCORE lpRec = GetDataTypeRecord  (hwndCnt, DT_TYPE);
            CNTDATA dataType;

            CntFldDataGet   (lpRec,         lpEditFI, sizeof (dataType), &dataType);
            Edit_GetText    (hwndCntEdit,   szNumber, sizeof (szNumber));
            GetTypeLength   (dataType.uDataType, &data, szNumber);
            break;
      }

      case DT_TITLE:
      {
         char szTitle[MAXOBJECTNAME + 1];

         Edit_GetText(hwndCntEdit, szTitle, sizeof(szTitle));
         CntFldTtlSet(hwndCnt, lpEditFI, szTitle, lstrlen(szTitle) + 1);
         break;
      }

    case DT_DEFSPEC:
        {
        int nLen = Edit_GetTextLength(hwndCntEdit);
        if (nLen == 0)
        {
            if (data.lpszDefSpec) ESL_FreeMem (data.lpszDefSpec);
            data.lpszDefSpec = NULL;
            break;
        }
        if (data.lpszDefSpec)
            data.lpszDefSpec = (LPTSTR)ESL_ReAllocMem (data.lpszDefSpec, nLen + 1, lstrlen(data.lpszDefSpec) +1);
        else
            data.lpszDefSpec = (LPSTR)ESL_AllocMem(nLen + 1);
        Edit_GetText(hwndCntEdit, data.lpszDefSpec, nLen + 1);
        }
        break;

    default:
         ASSERT(NULL);
   }

   if (lpEditRC && lpEditFI)
      CntFldDataSet(lpEditRC, lpEditFI, sizeof(data), &data);
}



static void MakeColumnsList (HWND hwnd, LPTABLEPARAMS lptbl)
{
    HWND hwndCnt = GetDlgItem(hwnd, IDC_CONTAINER);
    LPOBJECTLIST lpColList = NULL;
    LPFIELDINFO lpFld;
    int nbKey = 0;
    //
    // Work through the container control and create a linked list
    // of the columns and their attributes.
    //
    lpFld = CntFldTailGet(hwndCnt);

    while (lpFld)
    {
        LPCOLUMNPARAMS lpObj;
        LPOBJECTLIST lpColTemp;
        LPRECORDCORE lpRC;

        lpColTemp = AddListObject(lpColList, sizeof(COLUMNPARAMS));
        if (!lpColTemp)
        {   //
            // Need to free previously allocated objects.
            //
            FreeObjectList(lpColList);
            lpColList = NULL;
            break;
        }
        else
            lpColList = lpColTemp;
        lpObj = (LPCOLUMNPARAMS)lpColList->lpObject;
        lstrcpy(lpObj->szColumn, lpFld->lpFTitle);
        lpRC = CntRecHeadGet(hwndCnt);
        while (lpRC)
        {
            CNTDATA data;
            //
            // Retrieve the data (column attributes) from the container for this column
            //
            CntFldDataGet(lpRC, lpFld, sizeof(data), &data);

            switch (GetRecordDataType(hwndCnt, lpRC))
            {
            case DT_TYPE:
                if (data.uDataType == INGTYPE_INTEGER)
                    lpObj->uDataType = INGTYPE_INT4;
                else
                    lpObj->uDataType = data.uDataType;
                break;
            case DT_LEN:
                lpObj->lenInfo = data.len;
                break;
            case DT_NULLABLE:
                lpObj->bNullable = data.bNullable;
                break;
            case DT_UNIQUE:
                lpObj->bUnique = data.bUnique;
                break;
            case DT_DEFAULT:
                lpObj->bDefault = data.bDefault;
                break;
            case DT_DEFSPEC:
                if (lpObj->lpszDefSpec)
                    ESL_FreeMem (lpObj->lpszDefSpec);
                lpObj->lpszDefSpec = NULL;
                if (!data.lpszDefSpec)
                    break;
                lpObj->lpszDefSpec = (LPTSTR)ESL_AllocMem (lstrlen (data.lpszDefSpec) +1);
                if (!lpObj->lpszDefSpec)
                {
                    ErrorMessage ((UINT)IDS_E_CANNOT_ALLOCATE_MEMORY, RES_ERR);
                    return;
                }
                lstrcpy (lpObj->lpszDefSpec, data.lpszDefSpec);
                break;
            case DT_SYSMAINTAINED:
                lpObj->bSystemMaintained = data.bSystemMaintained;
                break;
            default:
                ASSERT(NULL);
            }
            lpRC = CntNextRec(lpRC);
        }
        //
        // Vut adds 09-August-95
        //
        switch (lpObj->uDataType)
        {
        case INGTYPE_FLOAT:
            if (lpObj->lenInfo.dwDataLen > 7)
                lpObj->uDataType = INGTYPE_FLOAT8;
            else
                lpObj->uDataType = INGTYPE_FLOAT4;
            break;
        case INGTYPE_DECIMAL:
            //if (lpObj->lenInfo.decInfo.nPrecision + lpObj->lenInfo.decInfo.nScale > 7)
            //    lpObj->uDataType = INGTYPE_FLOAT8;
            //else
            //    lpObj->uDataType = INGTYPE_FLOAT4;
            break;
        default:
            break;
        }
        lpFld = CntPrevFld(lpFld);
    }
    lptbl->lpColumns = lpColList;
    lptbl->nbKey     = nbKey;
}



static void ReleaseField(HWND hwndCnt, LPFIELDINFO lpField)
/*
   Function:
      Release any resources associated with a given column in the container.

   Parameters:
      hwndCnt  - Handle to the container window
      lpField  - Pointer to container field to release resources for.

   Returns:
      None.
*/
{
    CNTDATA data;
    LPRECORDCORE lpRC = CntRecHeadGet(hwndCnt);
    while (lpRC)
    {
        // Get the data for this cell
        CntFldDataGet(lpRC, lpField, sizeof(data), &data);
        switch (GetRecordDataType(hwndCnt, lpRC))
        {
        case DT_DEFSPEC:
            if (data.lpszDefSpec)
                ESL_FreeMem(data.lpszDefSpec);
            data.lpszDefSpec = NULL;
            break;
        default:
            break;
        }
        CntFldDataSet(lpRC, lpField, sizeof(data), &data);
        lpRC = CntNextRec(lpRC);
    }
}


static void ReleaseAllFields(HWND hwndCnt)
/*
   Function:
      Release resources associated with all columns in the container.

   Parameters:
      hwndCnt  - Handle to the container window

   Returns:
      None.
*/
{
   LPFIELDINFO lpFI = CntFldHeadGet(hwndCnt);

   while (lpFI)
   {
      ReleaseField(hwndCnt, lpFI);
      lpFI = CntNextFld(lpFI);
   }
}


LRESULT WINAPI EXPORT VDBA2xAlterTblColNameSubclassProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
   switch (message)
   {
      HANDLE_MSG(hwnd, WM_KEYDOWN, ColName_OnKey);
      HANDLE_MSG(hwnd, WM_GETDLGCODE, ColName_OnGetDlgCode);
      default:
         return VDBA2xAlterTblColNameDefWndProc( hwnd,
                                   message,
                                   wParam,
                                   lParam);
   }
   return 0L;
}


LRESULT WINAPI EXPORT VDBA2xAlterTblColNameDefWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    return CallWindowProc(wndprocColName, hwnd, message, wParam, lParam);
}


static void ColName_OnKey(HWND hwnd, UINT vk, BOOL fDown, int cRepeat, UINT flags)
{
    HWND hdlg = GetParent(hwnd);
    // We don't process key-up messages.
    if( !fDown )
    {
        // Forward this to the default proc for processing.
        FORWARD_WM_KEYUP(hwnd, vk, cRepeat, flags, VDBA2xAlterTblColNameDefWndProc);
        return;
    }
    switch(vk)
    {
    case VK_RETURN:
        AddContainerColumn(hdlg);
        break;
    default:
        FORWARD_WM_KEYDOWN(hwnd, vk, cRepeat, flags, VDBA2xAlterTblColNameDefWndProc);
    }
}


static void AddContainerColumn(HWND hwnd)
/*
   Function:
      Adds a new column to the container based on the value of the column
      name edit control.

   Parameters:
      hwnd     - Handle to the dialog window

   Returns:
      None.
*/
{
    char szColumn[MAXOBJECTNAME];
    HWND hwndCnt = GetDlgItem(hwnd, IDC_CONTAINER);
    LPFIELDINFO lpFI;
    UINT uNewDataSize;
    BOOL bEnabled = FALSE;
    LPCNTINFO lpcntInfo;
    LPCOLUMNPARAMS lpCol = NULL;
    LPTABLEPARAMS lpTS = GetDlgProp (hwnd);
    LPRECORDCORE lpRC;
    LPFIELDINFO lpFirstField;
    RECT rect;
    CNTDATA data;
    int  index, aType = 0;

    if (!lpTS)
        return;
    Edit_GetText(GetDlgItem(hwnd, IDC_COLNAME), szColumn, sizeof(szColumn));
   suppspace(szColumn);
   if (!szColumn[0])
      return;
    //
    // Ensure the object name is valid
    //
    if (!VerifyObjectName(hwnd, szColumn, TRUE, TRUE, TRUE))
        return;
    if (CntFindField(hwndCnt, szColumn))
    {
        //
        // Field already exists in container.
        //
        char szError[256];
        char szTitle[BUFSIZE];
        HWND currentFocus = GetFocus();
        
        LoadString(hResource, IDS_COLUMNEXISTS, szError, sizeof(szError));
      //LoadString(hResource, IDS_PRODUCT, szTitle, sizeof(szTitle));
        VDBA_GetProductCaption(szTitle, sizeof(szTitle));
        MessageBox(NULL, szError, szTitle, MB_OK | MB_ICONEXCLAMATION | MB_TASKMODAL);
        SetFocus (GetDlgItem(hwnd, IDC_COLNAME));
        Edit_SetSel (GetDlgItem(hwnd, IDC_COLNAME), 0, -1);
        return;
    } 
    //
    // Test to see if we need to reset the column (case Drop (A) and then Add (A))
    //
    lpCol = (LPCOLUMNPARAMS)VDBA20xTableColumn_Search (lpTS->lpColumns, (LPCTSTR)szColumn);
    if (lpCol)
    {
        CntDeferPaint     (hwndCnt);
        CntSetTableColumn (hwndCnt, lpCol);
        CntEndDeferPaint  (hwndCnt, TRUE);
        return;
    }
    CntDeferPaint(hwndCnt);
    //
    // Add the column to the container
    //
    uFieldCounter++;
    //
    // Allocate enough memory for this fields data. Since each
    // field is given a specific offset into the allocated
    // memory (see wOffStruct), the memory for the new field
    // is always tagged onto the end of the current block,
    // regardless of any deleted fields.
    //
    uNewDataSize = uFieldCounter * sizeof(CNTDATA);
    if (uNewDataSize > uDataSize)
        ReAllocContainerRecords(uNewDataSize);
    CopyContainerRecords(hwndCnt);              // Copy the record data into our buffer
    InitialiseFieldData(uFieldCounter - 1);     // Initialise the data for this field.
    //
    // Initialise the field descriptor.
    //
    lpFI = CntNewFldInfo();
    lpFI->wIndex       = uFieldCounter;
    lpFI->cxWidth      = 21;
    lpFI->flFDataAlign = CA_TA_LEFT | CA_TA_VCENTER;
    lpFI->wColType     = CFT_CUSTOM;
    lpFI->flColAttr    = CFA_OWNERDRAW;
    lpFI->wDataBytes   = sizeof(CNTDATA);
    lpFI->wOffStruct   = (uFieldCounter - 1) * sizeof(CNTDATA);
    CntFldTtlSet(hwndCnt, lpFI, szColumn, lstrlen(szColumn) + 1);
    CntFldColorSet(hwndCnt, lpFI, CNTCOLOR_FLDBKGD, CntFldColorGet(hwndCnt, lpFI, CNTCOLOR_FLDTTLBKGD));
    lpFI->flFTitleAlign = CA_TA_HCENTER | CA_TA_VCENTER;
    CntFldDrwProcSet(lpFI, lpfnDrawProc);
    //
    // Add the field
    //
    CntAddFldTail(hwndCnt, lpFI);
    //
    // Add the new records to the container
    //
    AddRecordsToContainer(hwndCnt);
    lpcntInfo = CntInfoGet(hwndCnt);
    if (lpcntInfo->nFieldNbr < MAXIMUM_COLUMNS)
        bEnabled = TRUE;
    EnableWindow(GetDlgItem(hwnd, IDC_ADD), bEnabled);
    bEnabled = FALSE;
    if (lpcntInfo->nFieldNbr > 0)
        bEnabled = TRUE;
    EnableWindow(GetDlgItem(hwnd, IDC_REFERENCES), bEnabled);
    EnableOKButton(hwnd);
    Edit_SetText(GetDlgItem(hwnd, IDC_COLNAME), "");
    
    //
    // Display the type control
    // 
    lpRC = GetDataTypeRecord(hwndCnt, DT_TYPE);
    //
    // Set focus on first of the list, then on newly created one (scroll problem solve)
    //
    lpFirstField = CntFldHeadGet (hwndCnt);
    CntFocusSet(hwndCnt, lpRC, lpFirstField);
    CntFocusSet(hwndCnt, lpRC, lpFI);
    
    LocalGetFocusCellRect(hwndCnt, &rect);
    ShowTypeWindow(hwndCnt, &rect);
    index = ComboBox_GetCurSel (hwndType);
    if (index != CB_ERR)
    {
        memset (&data, 0, sizeof (CNTDATA));
        aType = ComboBox_GetItemData (hwndType, index);
        lpRC  = GetDataTypeRecord(hwndCnt, DT_LEN);
        switch (aType)
        {
        case INGTYPE_BYTE: 
        case INGTYPE_BYTEVAR:
        case INGTYPE_C:
        case INGTYPE_TEXT:
        case INGTYPE_CHAR:
        case INGTYPE_VARCHAR:
        case INGTYPE_FLOAT4:
        case INGTYPE_FLOAT8:
            data.len.dwDataLen = 0;
            break;
        case INGTYPE_FLOAT:
            data.len.dwDataLen = DEFAULT_FLOAT;
            break;
        case INGTYPE_DECIMAL:
            data.len.decInfo.nPrecision = 0;
            data.len.decInfo.nScale     = 0;
            break;
        default:
            data.len.dwDataLen = 0;
            break;
        }
        CntFldDataSet (lpRC, lpFI, sizeof(CNTDATA), &data);
        //
        // Set up the Data type
        lpRC  = GetDataTypeRecord(hwndCnt, DT_TYPE);
        memset (&data, 0, sizeof (CNTDATA));
        data.uDataType = aType;
        CntFldDataSet (lpRC, lpFI, sizeof(CNTDATA), &data);
    }
    //
    // Set up the attribute Nullable to WITH NULL
    memset (&data, 0, sizeof (CNTDATA));
    lpRC  = GetDataTypeRecord(hwndCnt, DT_NULLABLE);
    data.bNullable = TRUE;
    CntFldDataSet (lpRC, lpFI, sizeof(CNTDATA), &data);
    //
    // Set up the attribute Default to NOT DEFAULT
    memset (&data, 0, sizeof (CNTDATA));
    lpRC  = GetDataTypeRecord(hwndCnt, DT_DEFAULT);
    data.bDefault = FALSE;
    CntFldDataSet (lpRC, lpFI, sizeof(CNTDATA), &data);
    CntEndDeferPaint(hwndCnt, TRUE);
    if (hwndCnt)
    {
        RECT r;
        GetClientRect(hwndCnt, &r);
        InvalidateRect(hwndCnt, &r, FALSE);
    }

}


static UINT ColName_OnGetDlgCode(HWND hwnd, MSG FAR* lpmsg)
{
   UINT uRet;

   // Processing of this message is only necessary for controls
   // within dialogs. The return value of DLGC_WANTMESSAGE tells the
   // dialog manager that we want ALL keystroke msgs to come to this
   // edit control and no further processing of the keystroke msg is
   // to take place. If this message is not processed in this manner
   // the control will not receive the keystroke messages that you
   // really need (like TAB, ENTER, and ESCAPE) while in a dialog.
   // This message is being processed the same way in the container.

   // Forward this to the default proc for processing.
   uRet = (UINT)FORWARD_WM_GETDLGCODE(hwnd, lpmsg, VDBA2xAlterTblColNameDefWndProc);
   // Now specify that we want all keyboard input.
   uRet |= DLGC_WANTMESSAGE;
   return uRet;
}

static LPRECORDCORE GetDataTypeRecord(HWND hwndCnt, int nDataType)
/*
   Function:
      Gets the container record associated with the given data type.

   Parameters:
      hwndCnt     - Handle to the container window
      nDataType   - Data type.

   Returns:
      Pointer to container record for data type if successful, otherwise NULL.
*/
{
   LPRECORDCORE lpRC;
   int nOffset;

   lpRC = CntRecHeadGet(hwndCnt);
   for (nOffset = 0; nOffset < nDataType; nOffset++)
   {
      lpRC = CntNextRec(lpRC);
   }
   return lpRC;
}


static int GetMinReqLength(int nId)
/*
   Function:
      Gets the minimum value for the given data type.

   Parameters:
      nId      - Data type

   Returns:
      Minimum value for data type, -1 if data type does not have
      a numeric minimum.
*/
{
   REGLENGTH *pCurrentReqLength;
   int nOffset = 0;

   if (GetOIVers() >= OIVERS_26)
      pCurrentReqLength = nReqLength26;
   else
      pCurrentReqLength = nReqLength;

   while (pCurrentReqLength[nOffset].nId != -1)
   {
      if (pCurrentReqLength[nOffset].nId == nId)
         return pCurrentReqLength[nOffset].nMinLength;
      nOffset++;
   }
   return -1;
}


static long GetMaxReqLength(int nId)
/*
   Function:
      Gets the maximum value for the given data type.

   Parameters:
      nId      - Data type

   Returns:
      Maximum value for data type, -1 if data type does not have
      a numeric maximum.
*/
{
   REGLENGTH *pCurrentReqLength;
   int nOffset = 0;

   if (GetOIVers() >= OIVERS_26)
      pCurrentReqLength = nReqLength26;
   else
      pCurrentReqLength = nReqLength;

   while (pCurrentReqLength[nOffset].nId != -1)
   {
      if (pCurrentReqLength[nOffset].nId == nId)
         return pCurrentReqLength[nOffset].dwMaxLength;
      nOffset++;
   }

   return -1;
}



static void MakeTypeLengthString(UINT uType, LPCNTDATA lpdata, LPSTR lpszNumber)
/*
   Function:
      Contructs a string representation of the given length for the given
      data type.

   Parameters:
      uType       - Data type
      lpdata      - Data structure containing the length
      lpszNumber  - Pointer to buffer to hold the string representation of
                    the length.

   Returns:
      None.
*/
{
    if (uType == INGTYPE_DECIMAL)
    {
        // Construct the decimal (precision,scale) string
       if ( lpdata->len.decInfo.nPrecision != 0 )
       {
           my_itoa (lpdata->len.decInfo.nPrecision, lpszNumber, 10);
           lstrcat (lpszNumber, ",");
           my_itoa (lpdata->len.decInfo.nScale, lpszNumber + lstrlen(lpszNumber), 10);
       }
       else
           *lpszNumber = (char)NULL;
    }
    else if (uType == INGTYPE_OBJKEY || uType == INGTYPE_TABLEKEY)
           *lpszNumber = (char)NULL;
    else
        wsprintf (lpszNumber, "%d", lpdata->len.dwDataLen);
}


static void GetTypeLength(UINT uType, LPCNTDATA lpdata, LPSTR lpszNumber)
/*
   Function:
      Fills out the data type length element of lpdata using the given string
      representation of the length.

   Parameters:
      uType       - Data type
      lpdata      - Data structure to receive the length
      lpszNumber  - String representation of the length

   Returns:
      None.
*/
{
    if (uType == INGTYPE_DECIMAL)
    {
        LPSTR lpszComma;
        if (lstrlen(lpszNumber) == 0)
        {
            lpdata->len.decInfo.nPrecision = 0;
            lpdata->len.decInfo.nScale = 0;
            return;
        }
        lpszComma = _fstrchr(lpszNumber, ',');
        if (lpszComma && (*(lpszComma + 1) != (char)NULL))
        {
            *lpszComma = (char)NULL;
            lpdata->len.decInfo.nPrecision   = my_atoi (lpszNumber);
            lpdata->len.decInfo.nScale       = my_atoi (lpszComma + 1);
        }
    }
    else
        lpdata->len.dwDataLen = my_atoi (lpszNumber);
}


static void VerifyDecimalString(HWND hwndEdit)
/*
   Function:
      Retrieves the decimal (precision,scale) string from a given edit
      control and verifies that the precision part is less than the maximum
      allowable precision, and that the scale is less than or equal to the
      precision.  If any part of the string is above its maximum it is reset
      to that maximum and the corrected string is written back to the edit
      control.

   Parameters:
      hwndEdit - Handle to the edit control

   Returns:
      None.  The corrected string is in the edit control.
*/
{
   char szNumber[20];
   LPSTR lpszComma;
   char szMaxPrecision[20];
   char szMaxScale[20];
   int nPrecision;
   char szVerified[20];
   BOOL bChanged = FALSE;

   Edit_GetText(hwndEdit, szNumber, sizeof(szNumber));
   if (!*szNumber)
      return;
   *szMaxPrecision = (char)NULL;
   *szMaxScale = (char)NULL;
   lpszComma = _fstrchr(szNumber, ',');
   if (lpszComma)
      *lpszComma = (char)NULL;
   // Verify the precision.
   if ((nPrecision = my_atoi(szNumber)) > MAX_DECIMAL_PRECISION)
   {
      // Replace the precision string with the maximum
      my_itoa(MAX_DECIMAL_PRECISION, szMaxPrecision, 10);
      bChanged = TRUE;
   }
   // Verify the scale, which cannot be greater than the precision.
   // The scale only exists if a comma is present in the string.
   if (lpszComma && (*(lpszComma + 1)))
   {
      if (my_atoi(lpszComma + 1) > nPrecision)
      {
         my_itoa(nPrecision, szMaxScale, 10);
         bChanged = TRUE;
      }
   }
   // Reconstruct decimal string using any maximum values that have
   // been exceeded.
   if (*szMaxPrecision)
      lstrcpy(szVerified, szMaxPrecision);
   else
      lstrcpy(szVerified, szNumber);
   if (lpszComma)
   {
      lstrcat(szVerified, ",");
      if (*szMaxScale)
         lstrcat(szVerified, szMaxScale);
      else
         lstrcat(szVerified, lpszComma + 1);
   }
   // Introduce the bChanged flag otherwise we get into an infinite
   // loop because Edit_SetText causes EN_CHANGE to get re-sent
   if (bChanged)
      Edit_SetText(hwndEdit, szVerified);
}


static void GetTitleRect(HWND hwndCnt, LPRECT lprect)
/*
   Function:
      Calculates the rectangle of the field title area of the container
      using the field whose cell currently has the focus.

   Parameters:
      hwndCnt  - Handle to the container window
      lprect   - Address to store returned title rectangle.

   Returns:
      None.  The title rectangle is in lprect.
*/
{
   LPCNTINFO lpcntInfo = CntInfoGet(hwndCnt);

   LocalGetFocusCellRect(hwndCnt, lprect);
   lprect->top = lpcntInfo->cyTitleArea;
   lprect->bottom = lprect->top + lpcntInfo->cyColTitleArea;
}


static void ResetCntEditCtl(HWND hwndCnt)
/*
   Function:
      Resets the text in the container edit control to show the
      original pre-edit data.

   Parameters:
      hwndCnt  - Handle to the container window

   Returns:
      None.
*/
{
   CNTDATA data;
   LPSTR lpszString;
   char szNumber[20];

   // Reset the text in the edit control to the original data
   ZEROINIT(data);
   if (lpEditRC && lpEditFI)
      CntFldDataGet(lpEditRC, lpEditFI, sizeof(data), &data);
   switch (GetRecordDataType(hwndCnt, lpEditRC))
   {
      case DT_LEN:
      {
         LPRECORDCORE lpRC = GetDataTypeRecord(hwndCnt, DT_TYPE);
         CNTDATA dataType;

         CntFldDataGet(lpRC, lpEditFI, sizeof(dataType), &dataType);
         MakeTypeLengthString(dataType.uDataType, &data, szNumber);
         lpszString = szNumber;
         break;
      }
      case DT_TITLE:
         lpszString = lpEditFI->lpFTitle;
         break;
      case DT_DEFSPEC:
         lpszString = data.lpszDefSpec;
         break;
      default:
         ASSERT(NULL);
   }

   Edit_SetText(hwndCntEdit, lpszString);
}

static void EnableOKButton(HWND hwnd)
{
    BOOL bEnable = FALSE;
    HWND hwndCnt = GetDlgItem(hwnd, IDC_CONTAINER);
    LPCNTINFO lpcntInfo = CntInfoGet(hwndCnt);

    if ((Edit_GetTextLength(GetDlgItem(hwnd, IDC_TABLE)) > 0) && lpcntInfo->nFieldNbr > 0)
        bEnable = TRUE;
    EnableWindow(GetDlgItem(hwnd, IDOK), bEnable);
}


static void OnOK (HWND hwnd)
{
    BOOL Success;
    TABLEPARAMS ts;

    memset  (&ts, 0, sizeof (TABLEPARAMS));
    Success = FillStructureFromControls (hwnd, &ts);
    if (!Success)
    {
        FreeAttachedPointers (&ts, OT_TABLE);
        return;
    }
    Success = VDBA20xTableDataValidate (hwnd, &ts);
    FreeAttachedPointers (&ts, OT_TABLE);
    if (!Success)
        return;

    Success = AlterObject (hwnd);
    if (!Success)
        return;
    else
    {
        LPTABLEPARAMS lpTS   = GetDlgProp(hwnd);
        HWND hwndTable  = GetDlgItem(hwnd, IDC_TABLE);
        TCHAR tchszTable [MAXOBJECTNAME];

        if (bRelaunchAlter)
        {
            Success = AlterObject (hwnd);
            if (!Success)
                return;
        }

        GetWindowText (hwndTable, tchszTable, sizeof(tchszTable));
        if (lstrcmpi (lpTS->objectname, tchszTable) != 0)
            lstrcpy (lpTS->objectname, tchszTable);
        EndDialog (hwnd, TRUE);
    }
}

static BOOL AlterObject (HWND hwnd)
{
    TABLEPARAMS   TS2, TS3;
    BOOL Success = TRUE, bOverwrite;
    int  ires, irestmp, hdl  = GetCurMdiNodeHandle ();
    LPUCHAR vnodeName        = GetVirtNodeName (hdl);
    LPTABLEPARAMS lpTS       = GetDlgProp(hwnd);
    BOOL bChangedbyOtherMessage = FALSE;
    bRelaunchAlter = FALSE;

    if (!lpTS)
        return FALSE;
    memset  (&TS3, 0, sizeof (TABLEPARAMS));
    memset  (&TS2, 0, sizeof (TABLEPARAMS));
    lstrcpy (TS2.DBName,    lpTS->DBName);
    lstrcpy (TS2.objectname,lpTS->objectname);
    lstrcpy (TS2.szSchema,  lpTS->szSchema);
    lstrcpy (TS3.DBName,    lpTS->DBName);
    lstrcpy (TS3.objectname,lpTS->objectname);
    lstrcpy (TS3.szSchema,  lpTS->szSchema);
    ires = GetDetailInfo (vnodeName, OT_TABLE, &TS2, TRUE, &hdl);
    if (ires != RES_SUCCESS)
    {
        ReleaseSession (hdl, RELEASE_ROLLBACK);
        switch (ires) 
        {
        case RES_ENDOFDATA:
            ires = ConfirmedMessage ((UINT)IDS_C_ALTER_CONFIRM_CREATE);
            if (ires == IDYES) 
            {
                Success = FillStructureFromControls (hwnd, &TS3);
                if (Success) 
                {
                    ires = DBAAddObject (vnodeName, OT_TABLE, (void *) &TS3 );
                    if (ires != RES_SUCCESS) 
                    {
                        ErrorMessage ((UINT)IDS_CREATETABLEFAILED, ires);
                        Success=FALSE;
                    }
                }
            }
            break;
        default:
            Success=FALSE;
            ErrorMessage ((UINT)IDS_E_ALTER_ACCESS_PROBLEM, RES_ERR);
            break;
        }
        FreeAttachedPointers (&TS2, OT_TABLE);
        FreeAttachedPointers (&TS3, OT_TABLE);
        return Success;
    }
    if (!VDBA20xTable_Equal (lpTS, &TS2))
    {
        ReleaseSession (hdl, RELEASE_ROLLBACK);
        ires   = ConfirmedMessage ((UINT)IDS_C_ALTER_RESTART_LASTCHANGES);
        bChangedbyOtherMessage = TRUE;
        irestmp = ires;
        if (ires == IDYES) 
        {
            FreeAttachedPointers (lpTS, OT_TABLE);
            ires = GetDetailInfo (vnodeName, OT_TABLE, lpTS, FALSE, &hdl);
            if (ires != RES_SUCCESS) 
            {
                Success = FALSE;
                ErrorMessage ((UINT)IDS_E_ALTER_ACCESS_PROBLEM, RES_ERR);
            }
            else 
            {
                FillControlsFromStructure (hwnd, lpTS);
                FreeAttachedPointers (&TS2, OT_TABLE);
                VDBA20xTableStruct_Free (&statictableparams, TABLEPARAMS_ALL);
                VDBA20xTableStruct_Copy (&statictableparams, lpTS, TABLEPARAMS_PRIMARYKEY|TABLEPARAMS_UNIQUEKEY|TABLEPARAMS_FOREIGNKEY|TABLEPARAMS_CHECK);

                return FALSE;
            }
        }
        else
        {
            // Double Confirmation ?
            ires   = ConfirmedMessage ((UINT)IDS_C_ALTER_CONFIRM_OVERWRITE);
            if (ires != IDYES)
                Success = FALSE;
            else
            {
                char buf [2*MAXOBJECTNAME+5];
                wsprintf (buf,"%s::%s", vnodeName, lpTS->DBName);
                ires = Getsession (buf, SESSION_TYPE_CACHEREADLOCK, &hdl);
                if (ires != RES_SUCCESS)
                    Success = FALSE;
            }
        }
        if (!Success)
        {
            FreeAttachedPointers (&TS2, OT_TABLE);
            return Success;
        }
    }

    Success = FillStructureFromControls (hwnd, &TS3);
    if (Success) 
    {
        ires = DBAAlterObject (lpTS, &TS3, OT_TABLE, hdl);
        if (ires != RES_SUCCESS)  
        {
            BOOL bLaunchModifyPage = FALSE;
            int iret;

            if (ires == RES_CHANGE_PAGESIZE_NEEDED)
            {
                // the Actual Page_size of the current table is 2k and for add or remove
                // column it's necessary to change the page size to 4K.
                bLaunchModifyPage = TRUE;
                iret = MessageBox (hwnd, ResourceString ((UINT)IDS_W_CHANGE_PAGESIZE_NEEDED),
                                         ResourceString ((UINT)IDS_TITLE_WARNING), MB_ICONEXCLAMATION|MB_YESNO);
            }
            else if (ires == RES_PAGE_SIZE_NOT_ENOUGH)
            {
                // actually isn't possible to know if this error it's because the
                // current page size is not enough or if the parameter MAX_TUPLES_LENGTH
                // it's necessary to change the page_size parameter for this table.
                bLaunchModifyPage = TRUE;
                iret = MessageBox (hwnd, ResourceString ((UINT)IDS_W_CHANGE_ROWSIZE_EXCEDED),
                                         ResourceString ((UINT)IDS_TITLE_WARNING), MB_ICONEXCLAMATION|MB_YESNO);
                // DBMS configuration Change MAX_TUPLE_LENGTH
                //ErrorMessage ((UINT)IDS_CHANGE_DBMS_CONFIG, ires);
                //Success=FALSE;
                //bRelaunchAlter = FALSE;
                //bLaunchModifyPage = FALSE;
            }
            else
            {
                bLaunchModifyPage = FALSE;
                Success=FALSE;
                ErrorMessage ((UINT)IDS_ALTERTABLEFAILED, ires);
                FreeAttachedPointers (&TS2, OT_TABLE);
                ires = GetDetailInfo (vnodeName, OT_TABLE, &TS2, FALSE, &hdl);
                if (ires != RES_SUCCESS)
                    ErrorMessage ((UINT)IDS_E_ALTER_ACCESS_PROBLEM, RES_ERR);
                else 
                {
                    if (!VDBA20xTable_Equal (lpTS, &TS2)) 
                    {
                        if (!bChangedbyOtherMessage) 
                            ires  = ConfirmedMessage ((UINT)IDS_C_ALTER_RESTART_LASTCHANGES);
                        else
                            ires = irestmp;
                        if (ires == IDYES) 
                        {
                            FreeAttachedPointers (lpTS, OT_TABLE);
                            ires = GetDetailInfo (vnodeName, OT_TABLE, lpTS, FALSE, &hdl);
                            if (ires == RES_SUCCESS)
                            {
                                VDBA20xTableStruct_Free (&statictableparams, TABLEPARAMS_ALL);
                                VDBA20xTableStruct_Copy (&statictableparams, lpTS, TABLEPARAMS_ALL);
                                FillControlsFromStructure (hwnd, lpTS);
                            }
                        }
                        else
                            bOverwrite = TRUE;
                    }
                }
            }
            if (bLaunchModifyPage && iret == IDYES)
            {
                long iOldPageSize = lpTS->uPage_size;
                iret = MfcModifyPageSize(lpTS);
                if (iret == IDCANCEL)
                {
                    Success=FALSE;
                    bRelaunchAlter = FALSE;
                }
                else
                {
                    if (iOldPageSize != lpTS->uPage_size)
                    {
                        iret = MessageBox (hwnd, ResourceString ((UINT)IDS_PAGESIZE_SUCESS), 
                                                 ResourceString ((UINT)IDS_TITLE_WARNING), MB_ICONQUESTION |MB_YESNO);
                        if (iret == IDYES)
                        {
                            Success=TRUE;
                            bRelaunchAlter = TRUE;
                        }
                        else
                        {
                            Success=FALSE;
                            bRelaunchAlter = FALSE;
                        }
                    }
                    else
                    {
                            Success=FALSE; // error in 'modify' statement
                            bRelaunchAlter = FALSE;
                    }
                }
            }
            else
            {
                Success=FALSE;
                bRelaunchAlter = FALSE;
            }
        }
    }
    FreeAttachedPointers (&TS2, OT_TABLE);
    FreeAttachedPointers (&TS3, OT_TABLE);
    return Success;
}


static void OnCancel (HWND hwnd)
{
    HWND hwndTable  = GetDlgItem(hwnd, IDC_TABLE);
    LPTABLEPARAMS lpTS       = GetDlgProp(hwnd);
    if (!lpTS)
        return;
    EndDialog (hwnd, FALSE);
}



static void OnRemoveCntColumn (HWND hwnd, BOOL bCascade)
{
    HWND  hwndCnt = GetDlgItem (hwnd, IDC_CONTAINER);
    TCHAR szColumn [32];
    LPFIELDINFO lpField  = CntFldHeadGet (hwndCnt);
    LPTABLEPARAMS  lpTS   = (LPTABLEPARAMS)GetDlgProp (hwnd);
    LPOIDTOBJECT   lpObjectList = NULL;
    LPCOLUMNPARAMS lpCol;
    Edit_GetText (GetDlgItem (hwnd, IDC_COLNAME), szColumn, sizeof(szColumn));
    if (!szColumn [0])
        return; // No column to remove, just ignore.
    lpCol = VDBA20xTableColumn_Search (lpTS->lpColumns, szColumn);
    if (bCascade)
    {
        if (lpCol && !StringList_Search (lpTS->lpDropObjectList, szColumn))
        {
            LPSTRINGLIST lpObj;
            lpTS->lpDropObjectList = StringList_AddObject (lpTS->lpDropObjectList, szColumn, &lpObj);
            if (!lpTS->lpDropObjectList)
            {
                ErrorMessage ((UINT)IDS_E_CANNOT_ALLOCATE_MEMORY, RES_ERR);
                return;
            }
            lpObj->extraInfo1 = 1;
        }
    }

    if (!lpCol)
    {
        // 
        // For the newly added columns, if we drop them we also drop the constraints that 
        // use these columns, except the Check Constraints. (Need to parse the expr to find the columns)
        statictableparams.lpReferences = RemoveForeignKeys (statictableparams.lpReferences, (LPTSTR)szColumn);
        statictableparams.lpUnique     = RemoveUniqueKeys  (statictableparams.lpUnique,     (LPTSTR)szColumn);
    }
    CntDeferPaint (hwndCnt);
    //
    // Remove column from the container
    //
    while (lpField)
    {
        if (lstrcmpi(lpField->lpFTitle, szColumn) == 0)
        {
            //
            // Work through each record releasing any memory
            // allocated to store the records data.
            //
            ReleaseField (hwndCnt, lpField);
            CntRemoveFld (hwndCnt, lpField);
            break;
        }
        lpField = CntNextFld(lpField);
    }
    CntEndDeferPaint(hwndCnt, TRUE);
}


static void OnTableNameEditChange (HWND hwnd)
{
    LPTABLEPARAMS lpTS = (LPTABLEPARAMS)GetDlgProp (hwnd);
    HWND hwndTable  = GetDlgItem(hwnd, IDC_TABLE);
    TCHAR tchszTable [MAXOBJECTNAMENOSCHEMA];

    GetWindowText (hwndTable, tchszTable, sizeof(tchszTable));
    if (lstrcmpi (lpTS->objectname, tchszTable) != 0)
    {
        EnableWindow (GetDlgItem (hwnd, IDC_ADD),       FALSE);
        EnableWindow (GetDlgItem (hwnd, IDC_DROP),      FALSE);
        EnableWindow (GetDlgItem (hwnd, IDC_REFERENCES),FALSE);
        EnableWindow (GetDlgItem (hwnd, IDC_COLNAME),   FALSE);
        EnableWindow (GetDlgItem (hwnd, IDC_CONTAINER), FALSE);
    }
    else
    {
        EnableWindow (GetDlgItem (hwnd, IDC_ADD),       TRUE);
        EnableWindow (GetDlgItem (hwnd, IDC_DROP),      TRUE);
        EnableWindow (GetDlgItem (hwnd, IDC_REFERENCES),TRUE);
        EnableWindow (GetDlgItem (hwnd, IDC_COLNAME),   TRUE);
        EnableWindow (GetDlgItem (hwnd, IDC_CONTAINER), TRUE);
    }
    EnableOKButton (hwnd);
}

//
// The structure statictableparams is declared 
// as STATIC GLOBAL for this source.
static void OnReferences (HWND hwnd)
{
    TABLEPARAMS ts;
    LPTABLEPARAMS lpTS   = (LPTABLEPARAMS)GetDlgProp (hwnd);

    memset (&ts, 0, sizeof (ts));
    lstrcpy (ts.DBName,     lpTS->DBName);
    lstrcpy (ts.objectname, lpTS->objectname);
    lstrcpy (ts.szSchema,   lpTS->szSchema);
    ts.bCreate          = lpTS->bCreate;
    ts.uPage_size       = (lpTS->uPage_size == 0L)? 2048L: lpTS->uPage_size;
    ts.lpUnique         = lpTS->lpUnique;
    ts.lpReferences     = VDBA20xTableReferences_CopyList (statictableparams.lpReferences);
    ts.lpDropObjectList = StringList_Copy (statictableparams.lpDropObjectList);
    ts.lpOldReferences  = lpTS->lpReferences;
    MakeColumnsList   (hwnd, &ts);
   
    if (DlgReferences (hwnd, &ts) == IDOK)
    {
        statictableparams.lpReferences     = VDBA20xTableReferences_Done (statictableparams.lpReferences);
        statictableparams.lpDropObjectList = StringList_Done (statictableparams.lpDropObjectList);
        statictableparams.lpReferences     = ts.lpReferences;
        statictableparams.lpDropObjectList = ts.lpDropObjectList;
        ts.lpDropObjectList = NULL;
        ts.lpReferences     = NULL;
    }
    ts.lpUnique         = NULL;
    ts.lpReferences     = VDBA20xTableReferences_Done (ts.lpReferences);
    ts.lpColumns        = VDBA20xColumnList_Done (ts.lpColumns);
    ts.lpDropObjectList = StringList_Done (ts.lpDropObjectList);
}


static void OnColumnNameEditChange (HWND hwnd)
{
    LPTABLEPARAMS lpTS   = (LPTABLEPARAMS)GetDlgProp (hwnd);
    HWND hwndCnt = GetDlgItem(hwnd, IDC_CONTAINER);
    LPOBJECTLIST lpColList = NULL;
    LPFIELDINFO  lpFld;
    TCHAR tchszColumn [MAXOBJECTNAME -1];

    if (Edit_GetTextLength(GetDlgItem (hwnd, IDC_COLNAME)) == 0)
    {
        EnableWindow(GetDlgItem(hwnd, IDC_ADD),             FALSE);
        EnableWindow(GetDlgItem(hwnd, IDC_DROP),            FALSE);
        EnableWindow(GetDlgItem(hwnd, IDC_REMOVE_CASCADE),  FALSE);
    }
    else
    {
        Edit_GetText (GetDlgItem (hwnd, IDC_COLNAME), tchszColumn, sizeof (tchszColumn));
        lpFld = CntFldTailGet (hwndCnt);
        while (lpFld)
        {
            if (lstrcmpi (tchszColumn, lpFld->lpFTitle) == 0)
            {
                EnableWindow (GetDlgItem(hwnd, IDC_DROP), TRUE);
                EnableWindow(GetDlgItem(hwnd, IDC_ADD),   FALSE);
                if (VDBA20xTableColumn_Search (lpTS->lpColumns, tchszColumn))
                    EnableWindow(GetDlgItem(hwnd, IDC_REMOVE_CASCADE),  TRUE);
                return;
            }
            lpFld = CntPrevFld (lpFld);
        }
        EnableWindow(GetDlgItem(hwnd, IDC_ADD),   TRUE);
        EnableWindow(GetDlgItem(hwnd, IDC_DROP),            FALSE);
        EnableWindow(GetDlgItem(hwnd, IDC_REMOVE_CASCADE),  FALSE);
   }    
}

static void OnContainer (HWND hwnd, HWND hwndCtl, UINT codeNotify)
{
    LPRECORDCORE lpRC    = CntFocusRecGet (hwndCtl);
    LPRECORDCORE lpRec   = NULL;
    LPFIELDINFO  lpFI    = CntFocusFldGet (hwndCtl);
    RECT    rect;
    UINT    uChar;
    LPARAM  lParam;
    CNTDATA data;
    CNTDATA dataType;
    //CNTDATA dataDefault;
    BOOL    bGreyOut = FALSE;
    int     nType;

    switch (codeNotify)
    {
    case CN_SETFOCUS:
    case CN_NEWFOCUS:
    case CN_NEWFOCUSFLD:
    case CN_NEWFOCUSREC:
        OnChangeFocus(hwnd);
        break;
    case CN_CHARHIT:
#ifdef WIN32    
        lParam = (LPARAM)hwndCtl;
#else
        lParam = MAKELPARAM (hwndCtl, codeNotify);
#endif
        uChar  = CntCNCharGet(hwndCtl, lParam);

        // Get the data for this cell
        CntFldDataGet    (lpRC, lpFI, sizeof(data), &data);
        LocalGetFocusCellRect (hwndCtl, &rect);
        nType = GetRecordDataType (hwndCtl, lpRC);

        switch (nType)
        {
        case DT_TYPE:
            EnableOKButton(hwnd);
            break;
        case DT_LEN:
            //
            // Only allow edit if data type requires it
            //
            lpRec = GetDataTypeRecord(hwndCtl, DT_TYPE);
            CntFldDataGet(lpRec, lpFI, sizeof(dataType), &dataType);
            if (GetMaxReqLength(dataType.uDataType) != -1)
            {
                // GPF corrected Emb 9/8/95 : swap parameters of MAKELONG
                ShowEditWindow (hwndCtl, &rect, nType, (LPCNTDATA)MAKELONG(uChar, 0));
                EnableOKButton (hwnd);
            }
            break;
        case DT_NULLABLE:
            /*if (uChar == VK_SPACE)
            {
                data.bNullable = !data.bNullable;
                CntFldDataSet(lpRC, lpFI, sizeof(data), &data);
                CntUpdateRecArea(hwndCtl, lpRC, lpFI);
            }*/
            break;

        case DT_DEFAULT:
            /*if (uChar == VK_SPACE)
            {
                data.bDefault = !data.bDefault;
                CntFldDataSet(lpRC, lpFI, sizeof(data), &data);
                CntUpdateRecArea(hwndCtl, lpRC, lpFI);
            }*/
            break;
        case DT_DEFSPEC:
        default:
            break;
        }
        break;
    }
}


static void OnCntEdit (HWND hwnd, HWND hwndCtl, UINT codeNotify)
{
    LPRECORDCORE lpRC;
    CNTDATA data;
    long    lMax;
    int     length = 0;
    int     nMin;
    HWND hwndCnt = GetParent (hwndCtl);

    switch (codeNotify)
    {
    case EN_CHANGE:
        if (!lpEditRC || !lpEditFI) // Global Static Variables
            break;
        switch (GetRecordDataType (hwndCnt, lpEditRC))
        {
        case DT_LEN:
            lpRC = GetDataTypeRecord (hwndCnt, DT_TYPE);
            CntFldDataGet (lpRC, lpEditFI, sizeof(data), &data);
            nMin = GetMinReqLength (data.uDataType);
            lMax = GetMaxReqLength (data.uDataType);
            if (lMax != -1)
            {
                if (data.uDataType != INGTYPE_DECIMAL)
                    VerifyNumericEditControl(hwnd,
                                    hwndCtl,
                                    FALSE,
                                    TRUE,
                                    (DWORD)nMin,
                                    (DWORD)lMax);
                else
                    VerifyDecimalString(hwndCtl);
            }
            break;
        
        default:
            break;
        }
        break;
    }
}


static void OnCntCombo(HWND hwnd, HWND hwndCtl, UINT codeNotify)
{
#if UKS_SPEC
    LPFIELDINFO  lpFI;
    LPRECORDCORE lpRec, lpRC;
    CNTDATA      data;
#endif
    int          index = -1, aType = 0;
    HWND hwndCnt = GetParent (hwndCtl);
    switch (codeNotify)
    {
    case CBN_SELCHANGE:
        {
            // Set data with curent selection in combobox
            SetDataTypeWithCurSelCombobox();
            // Reset the length to zero
            InitializeDataLenWithDefault(hwndCnt, lpTypeFI);
            // update Nullable checkbox
            UpdateNullableAndDefault(hwndCnt, lpTypeFI);
        }
        break;
#if UKS_SPEC
        //
        // Reset the length to zero or to the default
        //
        lpFI  = CntFocusFldGet     (hwndCnt);
        lpRC  = GetDataTypeRecord  (hwndCnt, DT_LEN);
        index = ComboBox_GetCurSel (hwndCtl);
        memset (&data, 0, sizeof (CNTDATA));
        if (index != CB_ERR)
        {
            aType = ComboBox_GetItemData (hwndCtl, index);
            switch (aType)
            {
            case INGTYPE_C:
                //data.len.dwDataLen = 1;
                data.len.dwDataLen = 0;
                break;
            case INGTYPE_TEXT:
                //data.len.dwDataLen = 50;
                data.len.dwDataLen = 0;
                break;
            case INGTYPE_BYTE:
            case INGTYPE_BYTEVAR:
                //data.len.dwDataLen = 32;
                data.len.dwDataLen = 0;
                break;
            case INGTYPE_CHAR:
            case INGTYPE_VARCHAR:
                //data.len.dwDataLen = 20;
                data.len.dwDataLen = 0;
                break;
            case INGTYPE_DECIMAL:
                //data.len.decInfo.nPrecision  = 5;
                data.len.decInfo.nPrecision  = 0;
                data.len.decInfo.nScale      = 0;
                break;
            default:
                break;
            }
        }
        CntFldDataSet (lpRC, lpFI, sizeof(data), &data);
        memset (&data, 0, sizeof (CNTDATA));
        data.uDataType = aType;
        lpRec = GetDataTypeRecord (hwndCnt, DT_TYPE);
        CntFldDataSet(lpRec, lpFI, sizeof(CNTDATA), &data);
        CntUpdateRecArea (hwndCnt, NULL, lpFI);
        break;
#endif
    default:
        break;
    }
}

/*------------------------------------------------ FillControlsFromStructure -*/
/* Fill up the container controls with data in TABLE STRUCTURE                */
/*                                                                            */
/* Parameter:                                                                 */
/*     1) hwnd  : Handle to the Dialog Box Window.                            */
/*     2) lpTS  : Address of TABLE PARAMS.                                    */
/* RETURN: TRUE if operation is done with success.                            */
/*----------------------------------------------------------------------------*/
static BOOL FillControlsFromStructure (HWND hwnd, LPTABLEPARAMS lpTS)
{
    HWND hwndTable  = GetDlgItem(hwnd, IDC_TABLE);
    HWND hwndCntr   = GetDlgItem(hwnd, IDC_CONTAINER);
    LPOBJECTLIST    ls = NULL, lpColList  = NULL;
    LPOBJECTLIST    lpObj = NULL;
    if (!lpTS)
        return FALSE;
    //
    // Set the Name of the table.
    //
    Edit_SetText (hwndTable, lpTS->objectname);
    //
    // Set the columns and their attributes into the container.
    //
    lpColList = lpTS->lpColumns;
    CntDeferPaint(hwndCntr);
    while (lpColList)
    {
        CntSetTableColumn (hwndCntr, (LPCOLUMNPARAMS)lpColList->lpObject);
        lpColList = lpColList->lpNext;
    }
    CntEndDeferPaint(hwndCntr, TRUE);
    return TRUE;
}


/*------------------------------------------------ FillStructureFromControls -*/
/* Fill up the the structure (TABLE STRUCTURE) from the container controls.   */
/*                                                                            */
/* Parameter:                                                                 */
/*     1) hwnd  : Handle to the Dialog Box Window.                            */
/*     2) lpTS  : Address of TABLE STRUCTRE.                                  */
/* RETURN: TRUE if operation is done with success.                            */
/*----------------------------------------------------------------------------*/
static BOOL FillStructureFromControls (HWND hwnd, LPTABLEPARAMS lpTS)
{
	UINT nMask;
    HWND hwndTable  = GetDlgItem(hwnd, IDC_TABLE);
    LPTABLECOLUMNS   lpColList  = NULL;
    LPTABLECOLUMNS   lpPkey     = NULL;
    if (!lpTS)
        return FALSE;
    //
    // Fill up general information about table.
    Edit_GetText (hwndTable, lpTS->objectname, sizeof (lpTS->objectname));
    //
	nMask = TABLEPARAMS_PRIMARYKEY|TABLEPARAMS_UNIQUEKEY|TABLEPARAMS_FOREIGNKEY|TABLEPARAMS_CHECK|TABLEPARAMS_LISTDROPOBJ;
    VDBA20xTableStruct_Copy (lpTS, &statictableparams, nMask);
    //
    // Fill up the table columns.
    MakeColumnsList (hwnd, lpTS);

    // TODO: Other informations

    return TRUE;
}


/*-------------------------------------------------------- CntSetTableColumn -*/
/* Put a column into the container control.                                   */
/*                                                                            */
/* Parameter:                                                                 */
/*     1) hwndCntr : Handle to the Container Control.                         */
/*     2) lpCol    : Address of COLUMNPARAMS.                                 */
/* REMARK: The column is not Moveable.                                        */
/*         Its data type cannot be changed.                                   */
/*         You cannot change the attribute default, nullable.                 */
/*----------------------------------------------------------------------------*/
static void CntSetTableColumn (HWND hwndCntr, LPCOLUMNPARAMS lpCol)
{
    LPCOLUMNPARAMS  lpColParams = NULL;
    LPFIELDINFO     lpFI;
    LPCNTINFO       lpcntInfo;
    LPRECORDCORE    lpRC;
    LPFIELDINFO     lpFirstField;
    UINT    uNewDataSize;
    BOOL    bEnabled = FALSE;
    RECT    rect;
    CNTDATA data;
    int     index   = -1, aType = 0;
    LPTLCUNIQUE ls  = NULL;
    HWND    hwndDlg = GetParent (hwndCntr);
    LPTABLEPARAMS lpTS   = (LPTABLEPARAMS)GetDlgProp (hwndDlg);
    
    lpColParams = (LPCOLUMNPARAMS)lpCol;
    if (CntFindField (hwndCntr, (LPSTR)lpColParams->szColumn))
        return;
    //
    // Add the column to the container
    //
    uFieldCounter++;
    //
    // Allocate enough memory for this fields data. Since each
    // field is given a specific offset into the allocated
    // memory (see wOffStruct), the memory for the new field
    // is always tagged onto the end of the current block,
    // regardless of any deleted fields.
    //
    uNewDataSize = uFieldCounter * sizeof (CNTDATA);
    if (uNewDataSize > uDataSize)
        ReAllocContainerRecords (uNewDataSize);
    CopyContainerRecords (hwndCntr);             // Copy the record data into our buffer
    InitialiseFieldData (uFieldCounter - 1);     // Initialise the data for this field.
    //
    // Initialise the field descriptor.
    //
    lpFI = CntNewFldInfo();
    lpFI->wIndex       = uFieldCounter;
    lpFI->cxWidth      = 21;
    lpFI->flFDataAlign = CA_TA_LEFT | CA_TA_VCENTER;
    lpFI->wColType     = CFT_CUSTOM;
    lpFI->flColAttr    = CFA_OWNERDRAW;
    lpFI->wDataBytes   = sizeof(CNTDATA);
    lpFI->wOffStruct   = (uFieldCounter - 1) * sizeof(CNTDATA);
    CntFldTtlSet    (hwndCntr, lpFI, lpColParams->szColumn, lstrlen  (lpColParams->szColumn) + 1);
    CntFldColorSet  (hwndCntr, lpFI, CNTCOLOR_FLDBKGD, CntFldColorGet (hwndCntr, lpFI, CNTCOLOR_FLDTTLBKGD));
    lpFI->flFTitleAlign = CA_TA_HCENTER | CA_TA_VCENTER;
    CntFldDrwProcSet(lpFI, lpfnDrawProc);
    //
    // Add the field
    //
    CntAddFldTail (hwndCntr, lpFI);
    //
    // Add the new records to the container
    //
    AddRecordsToContainer (hwndCntr);
    lpcntInfo = CntInfoGet(hwndCntr);
    if (lpcntInfo->nFieldNbr < MAXIMUM_COLUMNS)
        bEnabled = TRUE;
    EnableWindow(GetDlgItem(hwndDlg, IDC_ADD), bEnabled);
    bEnabled = FALSE;
    if (lpcntInfo->nFieldNbr > 0)
        bEnabled = TRUE;
    EnableWindow  (GetDlgItem(hwndDlg, IDC_REFERENCES), bEnabled);
    EnableOKButton(hwndDlg);
    Edit_SetText(GetDlgItem(hwndDlg, IDC_COLNAME), "");
    //
    // Display the type control
    // 
    lpRC = GetDataTypeRecord (hwndCntr, DT_TYPE);
    memset (&data, 0, sizeof (CNTDATA));
    if (lstrcmpi(lpColParams->tchszDataType,lpColParams->tchszInternalDataType) != 0 ) {
        TCHAR szObjKey[16],szTblKey[16];
        LoadString(hResource, IDS_OBJECT_KEY, szObjKey, sizeof(szObjKey));
        LoadString(hResource, IDS_TABLE_KEY , szTblKey, sizeof(szTblKey));

        if ( lstrcmpi(lpColParams->tchszInternalDataType,szObjKey) == 0)
            data.uDataType = INGTYPE_OBJKEY;
        else if ( lstrcmpi(lpColParams->tchszInternalDataType,szTblKey) == 0)
            data.uDataType = INGTYPE_TABLEKEY;
        else
            data.uDataType = lpColParams->uDataType;
    }
    else
        data.uDataType = lpColParams->uDataType;
    CntFldDataSet (lpRC, lpFI, sizeof(CNTDATA), &data);
    //
    // Set focus on first of the list, then on newly created one (scroll problem solve)
    //
    lpFirstField = CntFldHeadGet (hwndCntr);
    CntFocusSet (hwndCntr, lpRC, lpFirstField);
    CntFocusSet (hwndCntr, lpRC, lpFI);
    LocalGetFocusCellRect (hwndCntr, &rect);
    //
    // Set up the data type length
    //
    memset (&data, 0, sizeof (CNTDATA));
    lpRC  = GetDataTypeRecord(hwndCntr, DT_LEN);
    switch (lpColParams->uDataType)
    {
    case INGTYPE_C          :
    case INGTYPE_TEXT       :
    case INGTYPE_CHAR       :
    case INGTYPE_VARCHAR    :
    case INGTYPE_BYTE       :
    case INGTYPE_BYTEVAR    :
    case INGTYPE_UNICODE_NCHR:
    case INGTYPE_UNICODE_NVCHR:
        data.len.dwDataLen= (int)lpColParams->lenInfo.dwDataLen;
        break;
    case INGTYPE_DECIMAL:
        data.len.decInfo.nPrecision = lpColParams->lenInfo.decInfo.nPrecision;
        data.len.decInfo.nScale     = lpColParams->lenInfo.decInfo.nScale;
        break;
    default:
        data.len.dwDataLen = 0;
        break;
    }
    CntFldDataSet (lpRC, lpFI, sizeof(CNTDATA), &data);

    //
    // Set up the Unique Attribute.
    // lpCol->bUnique is never set to TRUE from the GetDetailInfo.
    // So, we must use the unique from the Unique Constraint List.
    memset (&data, 0, sizeof (CNTDATA));
    lpRC = GetDataTypeRecord(hwndCntr, DT_UNIQUE);
    ls = lpTS->lpUnique;
    while (ls)
    {
        if (StringList_Count (ls->lpcolumnlist) == 1 && lstrcmpi ((ls->lpcolumnlist)->lpszString, lpCol->szColumn) == 0)
        {
            data.bUnique = TRUE;
            CntFldDataSet (lpRC, lpFI, sizeof(CNTDATA), &data);
            break;
        }
        ls = ls->next;
    }
    //
    // Set up the Nullable.
    //
    memset (&data, 0, sizeof (CNTDATA));
    lpRC = GetDataTypeRecord(hwndCntr, DT_NULLABLE);
    data.bNullable = lpCol->bNullable;
    CntFldDataSet (lpRC, lpFI, sizeof(CNTDATA), &data);
    //
    // Set up the Default.
    //
    memset (&data, 0, sizeof (CNTDATA));
    lpRC = GetDataTypeRecord(hwndCntr, DT_DEFAULT);
    data.bDefault = lpCol->bDefault;
    CntFldDataSet (lpRC, lpFI, sizeof(CNTDATA), &data); 
    //
    // Set up the Default Specification.
    //
    memset (&data, 0, sizeof (CNTDATA));
    lpRC = GetDataTypeRecord(hwndCntr, DT_DEFSPEC);
    if (lpCol->lpszDefSpec && lstrlen (lpCol->lpszDefSpec) > 0)
    {
        data.lpszDefSpec = ESL_AllocMem (lstrlen (lpCol->lpszDefSpec) +1);
        if (!data.lpszDefSpec)
        {
            ErrorMessage ((UINT)IDS_E_CANNOT_ALLOCATE_MEMORY, RES_ERR);
            return;
        }
        lstrcpy (data.lpszDefSpec, lpCol->lpszDefSpec);
    }
    CntFldDataSet (lpRC, lpFI, sizeof(CNTDATA), &data);
    //
    // set up System maintained specification
    //
    memset (&data, 0, sizeof (CNTDATA));
    lpRC = GetDataTypeRecord(hwndCntr, DT_SYSMAINTAINED);
    data.bSystemMaintained = lpCol->bSystemMaintained;
    CntFldDataSet (lpRC, lpFI, sizeof(CNTDATA), &data);
}

static BOOL IsAlterColumn(LPTABLEPARAMS lpTemp, LPSTR lpColName, BOOL bDisplayMessage)
{
    LPIDXCOLS lpCol = NULL;
    if ( !lpTemp || !lpColName )
        return FALSE;

    if ( lpTemp->StorageParams.nStructure == IDX_BTREE )
    {
        LPOBJECTLIST ls = lpTemp->StorageParams.lpidxCols;
        while (ls)
        {
            lpCol = (LPIDXCOLS)ls->lpObject;
            if ( x_stricmp (lpCol->szColName, lpColName) == 0 &&
                 lpCol->attr.nPrimary > 0) /* This column is part of btree key*/
            {
                if (bDisplayMessage)
                {
                    MessageBox (NULL, ResourceString ((UINT)IDS_E_ALTER_TABLE_CANNOT_ALTER_COLUMN),
                                ResourceString ((UINT)IDS_TITLE_WARNING), MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL);
                }
                return FALSE;              /* no alter column available       */
            }
            ls = ls->lpNext;
        }
    }
    return TRUE;
}

//
// TRUE:  Data type can be changed
// FALSE: Data type cannot be changed
//
static BOOL AllowTypeChanged (HWND hwnd, LPFIELDINFO lpFI)
{
    LPTABLEPARAMS lpTS   = (LPTABLEPARAMS)GetDlgProp (hwnd);
    LPSTR lpColumnName;
    LPCOLUMNPARAMS lpCol = NULL;

    if (!(lpFI && lpTS))
        return TRUE;
    lpColumnName = (LPSTR)lpFI->lpFTitle;
    lpCol = VDBA20xTableColumn_Search (lpTS->lpColumns, lpColumnName);
    if (lpCol)
    {
        if (GetOIVers() >= OIVERS_30) /* to manage the new "alter column" */ 
            return IsAlterColumn(lpTS, lpColumnName, TRUE);
        return FALSE;
    }
    return TRUE;
}

static int GetDataTypeStringId(int nId)
{
   int nOffset = 0;
   CTLDATA *lpDataType;
   if (GetOIVers() >= OIVERS_91)
      lpDataType = typeTypesAnsiDate;
   else if (GetOIVers() >= OIVERS_30)
      lpDataType = typeTypesBigInt;
   else if (GetOIVers() >= OIVERS_26)
      lpDataType = typeTypesUnicode;
   else
      lpDataType = typeTypes;

   while (lpDataType[nOffset].nId != -1)
   {
      if (lpDataType[nOffset].nId == nId)
         return lpDataType[nOffset].nStringId;

      nOffset++;
   }

   return -1;
}

static void OnUniqueConstraint (HWND hwnd)
{
    TABLEPARAMS ts;
    LPTABLEPARAMS lpTS   = (LPTABLEPARAMS)GetDlgProp (hwnd);

    memset (&ts, 0, sizeof (ts));
    lstrcpy (ts.DBName,     lpTS->DBName);
    lstrcpy (ts.objectname, lpTS->objectname);
    lstrcpy (ts.szSchema,   lpTS->szSchema);
    ts.bCreate          = lpTS->bCreate;
    ts.uPage_size       = (lpTS->uPage_size == 0L)? 2048L: lpTS->uPage_size;
    ts.lpUnique         = VDBA20xTableUniqueKey_CopyList (statictableparams.lpUnique);
    ts.lpDropObjectList = StringList_Copy (statictableparams.lpDropObjectList);
    ts.lpOldUnique      = lpTS->lpUnique;
    MakeColumnsList   (hwnd, &ts);
   
    if (CTUnique (hwnd, &ts) == IDOK)
    {
        CntUpdateUnique (hwnd, statictableparams.lpUnique, ts.lpUnique);
        statictableparams.lpUnique         = VDBA20xTableUniqueKey_Done (statictableparams.lpUnique);
        statictableparams.lpDropObjectList = StringList_Done (statictableparams.lpDropObjectList);
        statictableparams.lpUnique         = ts.lpUnique;
        statictableparams.lpDropObjectList = ts.lpDropObjectList;
        ts.lpDropObjectList = NULL;
        ts.lpUnique         = NULL;
    }
    ts.lpUnique         = VDBA20xTableUniqueKey_Done (ts.lpUnique);
    ts.lpColumns        = VDBA20xColumnList_Done (ts.lpColumns);
    ts.lpDropObjectList = StringList_Done (ts.lpDropObjectList);
}


static void OnCheckConstraint (HWND hwnd)
{
    TABLEPARAMS ts;
    LPTABLEPARAMS lpTS   = (LPTABLEPARAMS)GetDlgProp (hwnd);

    memset (&ts, 0, sizeof (ts));
    lstrcpy (ts.DBName,     lpTS->DBName);
    lstrcpy (ts.objectname, lpTS->objectname);
    ts.bCreate          = lpTS->bCreate;
    ts.uPage_size       = (lpTS->uPage_size == 0L)? 2048L: lpTS->uPage_size;
    ts.lpCheck          = VDBA20xTableCheck_CopyList (statictableparams.lpCheck);
    ts.lpDropObjectList = StringList_Copy (statictableparams.lpDropObjectList);
    ts.lpOldCheck       = lpTS->lpCheck;
    MakeColumnsList   (hwnd, &ts);
   
    if (CTCheck (hwnd, &ts) == IDOK)
    {
        statictableparams.lpCheck          = VDBA20xTableCheck_Done (statictableparams.lpCheck);
        statictableparams.lpDropObjectList = StringList_Done (statictableparams.lpDropObjectList);
        statictableparams.lpCheck          = ts.lpCheck;
        statictableparams.lpDropObjectList = ts.lpDropObjectList;
        ts.lpDropObjectList = NULL;
        ts.lpCheck          = NULL;
    }
    ts.lpCheck          = VDBA20xTableCheck_Done (ts.lpCheck);
    ts.lpColumns        = VDBA20xColumnList_Done (ts.lpColumns);
    ts.lpDropObjectList = StringList_Done (ts.lpDropObjectList);
}

//
// Remove all the constraints that use the column name 'lpszColumn'
//
static LPOBJECTLIST RemoveForeignKeys (LPOBJECTLIST lpReferences, LPCTSTR lpszColumn)
{
    BOOL bActionDrop  = FALSE;
    LPOBJECTLIST ls   = lpReferences, lx;
    LPREFERENCEPARAMS lpRef;
    LPREFERENCECOLS   lpCol;
    while (ls)
    {
        lpRef = (LPREFERENCEPARAMS)ls->lpObject;
        lx    = lpRef->lpColumns;
        bActionDrop = FALSE;
        while (lx)
        {
            lpCol = (LPREFERENCECOLS)lx->lpObject;
            if (lstrcmpi (lpCol->szRefingColumn, lpszColumn) == 0)
            {
                bActionDrop = TRUE;
                break;
            }
            lx = lx->lpNext;
        }
        if (bActionDrop)
        {
            FreeObjectList (lpRef->lpColumns);
            lpRef->lpColumns = NULL;
            lpReferences = DeleteListObject (lpReferences, ls);
            ls = lpReferences;
        }
        else
            ls = ls->lpNext;
    }
    return lpReferences;
}

//
// Remove all the constraints that use the column name 'lpszColumn'
//
static LPTLCUNIQUE  RemoveUniqueKeys  (LPTLCUNIQUE  lpUnique, LPCTSTR lpszColumn)
{
    LPTLCUNIQUE ls = lpUnique;

    while (ls)
    {
        if (StringList_Search (ls->lpcolumnlist, lpszColumn))
        {
            lpUnique = VDBA20xTableUniqueKey_Remove (lpUnique, ls->tchszConstraint);
            ls = lpUnique;
        }
        else
           ls = ls->next;
    }

    return lpUnique;
}

// Example of 1 and 2 columns.
// DBMS considers the followings are confligs:
//
// Primary Key (a)   , Unique (a)
// Unique (a)        , Unique (a)
// Unique (a, b)     , Unique (b, a)
// Primary Key (a, b), Unique (a, b)
// Primary Key (a, b), Unique (b, a)
//
static BOOL  VDBA20xTableDataValidate (HWND hwnd, LPTABLEPARAMS lpTS)
{
    LPCOLUMNPARAMS lpCol;
    LPOBJECTLIST   ls = lpTS->lpColumns;
    LPSTRINGLIST   list1 = NULL, list2 = NULL;
    LPTLCUNIQUE    lx;
    TCHAR          tchszColumn [MAXOBJECTNAME];
    //
    // Must specify the length if data type requires it.
    if (IsLengthZero (hwnd, tchszColumn) && !lpTS->bCreateAsSelect)
    {
        TCHAR Msg [100];

        wsprintf (Msg, ResourceString ((UINT)IDS_E_SHOULD_SPECIFY_LEN), tchszColumn); 
        MessageBox (NULL, Msg, ResourceString ((UINT)IDS_TITLE_WARNING), MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL);
        return FALSE;
    }
    //
    // Warning: Define column as unique, the nullable attribute will be automatically set
    //          to NOT NULL.
    ls = lpTS->lpColumns;
    while (ls)
    {
        lpCol = (LPCOLUMNPARAMS)ls->lpObject;
        if (lpCol->bUnique && lpCol->bNullable)
        {
            // tchszMsg3
            MessageBox (hwnd, ResourceString ((UINT)IDS_E_UNIQUE_MUSTBE_NOTNUL), 
                              ResourceString ((UINT)IDS_TITLE_WARNING), MB_ICONEXCLAMATION|MB_OK);
            break;
        }
        ls = ls->lpNext;
    }
    //
    // Check for duplicate keys for single column.
    list1 = lpTS->primaryKey.pListKey;
    if (StringList_Count (list1) == 1)
    {
        ls = lpTS->lpColumns;
        while (ls)
        {
            lpCol = (LPCOLUMNPARAMS)ls->lpObject;
            if (lpCol->bUnique && lstrcmpi (lpCol->szColumn, list1->lpszString) == 0)
            {
                TCHAR msg [256];
                wsprintf (msg, ResourceString(IDS_ERR_DUPKEY), lpCol->szColumn);
                MessageBox (NULL, msg, NULL, MB_ICONSTOP|MB_OK);
                return FALSE;
            }
            ls = ls->lpNext;
        }
    }
    //
    // Check for duplicate keys for multiple columns.
    lx = lpTS->lpUnique;
    while (lx)
    {
        if (StringList_Count (list1) == StringList_Count (lx->lpcolumnlist))
        {
            BOOL Equal = TRUE;
            LPSTRINGLIST l1 = list1, l2 = lx->lpcolumnlist;
            while (l1 && l2)
            {
                if (!StringList_Search (list1, l2->lpszString) || !StringList_Search (lx->lpcolumnlist, l1->lpszString))
                {   
                    Equal = FALSE;
                    break;
                }
                l1 = l1->next;
                l2 = l2->next;
            }
            if (Equal)
            {
                TCHAR msg [256];
                wsprintf (msg, ResourceString(IDS_ERR_DUPKEY), lx->tchszConstraint);
                MessageBox (NULL, msg, NULL, MB_ICONSTOP|MB_OK);
                return FALSE;
            }
        }
        lx = lx->next;
    }
    return TRUE;
}

/*------------------------------------------------------- IsLengthZero -------*/
/*  Test if there is a table column whose data type is (c|char|text|varchar)  */
/*  And its data type length is zero.                                         */
/*                                                                            */
/*  1) hwnd    : Handle to the dialog box window.                             */
/*  2) buffOut : Buffer to receive the column's name that has length null.    */
/*                                                                            */
/*  Return TRUE or FALSE.                                                     */
/*----------------------------------------------------------------------------*/  
static BOOL IsLengthZero (HWND hwnd, char* buffOut)
{
    HWND         hwndCnt   = GetDlgItem(hwnd, IDC_CONTAINER);
    LPOBJECTLIST lpColList = NULL;
    LPFIELDINFO  lpFld;
    int          nbKey     = 0;
    BOOL         LengthZero= FALSE;

    //
    // Work through the container control and test each column.
    //
    lpFld = CntFldTailGet(hwndCnt);
    while (lpFld)
    {
        LPRECORDCORE   lpRC;
        int            typex = -1;
        lpRC = CntRecHeadGet (hwndCnt);
      
        while (lpRC)
        {
            CNTDATA data;
            //
            // Retrieve the data from the container for this column
            //
            CntFldDataGet (lpRC, lpFld, sizeof(data), &data);
            switch (GetRecordDataType (hwndCnt, lpRC))
            {
            case DT_TYPE:
                switch (data.uDataType)
                {
                case INGTYPE_C          :
                case INGTYPE_TEXT       :
                case INGTYPE_CHAR       :
                case INGTYPE_VARCHAR    :
                //case INGTYPE_LONGVARCHAR:
                case INGTYPE_BYTE       :         
                case INGTYPE_BYTEVAR    :      
                //case INGTYPE_LONGBYTE   :     
                    typex = data.uDataType;
                    break;
                }
                break;
            case DT_LEN:
                switch (typex)
                {
                case INGTYPE_C          :
                case INGTYPE_TEXT       :
                case INGTYPE_CHAR       :
                case INGTYPE_VARCHAR    :
                //case INGTYPE_LONGVARCHAR:
                case INGTYPE_BYTE       :         
                case INGTYPE_BYTEVAR    :      
                //case INGTYPE_LONGBYTE   :     
                    if (data.len.dwDataLen == 0)
                    {
                        lstrcpy(buffOut, lpFld->lpFTitle);
                        return TRUE;
                    }
                    break;
                case INGTYPE_DECIMAL:
                    //if (data.len.decInfo.nPrecision == 0)
                    //{
                    //    x_strcpy(buffOut, lpFld->lpFTitle);
                    //    return TRUE;
                    //}
                    break;
                default:
                    break;
                }
            }
            lpRC = CntNextRec (lpRC);
        }
        //
        // Get next column field
        //
        lpFld = CntPrevFld(lpFld);
    }
    return LengthZero;
}

static void CntUpdateUnique(HWND hwnd, LPTLCUNIQUE lpunique, LPTLCUNIQUE lpNewUnique)
{
    BOOL    bFound = FALSE;
    CNTDATA dataUnique;
    HWND hwndCnt   = GetDlgItem(hwnd, IDC_CONTAINER);
    LPTLCUNIQUE ls = NULL;
    LPFIELDINFO   lpFld;
    LPRECORDCORE  lpRec = NULL;
    lpFld = CntFldTailGet (hwndCnt);
    CntDeferPaint (hwndCnt);
    while (lpFld)
    {
        //
        // Check if the current column is part of unique key constraint.
        // If so, then set the Unique attribute to be TRUE.
        bFound = FALSE;
        ls = lpNewUnique;
        while (ls)
        {
            if (StringList_Count (ls->lpcolumnlist) == 1 && lstrcmpi ((ls->lpcolumnlist)->lpszString, lpFld->lpFTitle) == 0)
            {
                memset (&dataUnique, 0, sizeof (CNTDATA));
                lpRec = GetDataTypeRecord(hwndCnt, DT_UNIQUE);
                CntFldDataGet (lpRec, lpFld, sizeof (CNTDATA), &dataUnique);
                dataUnique.bUnique = TRUE;
                CntFldDataSet (lpRec, lpFld, sizeof(CNTDATA), &dataUnique);
                bFound = TRUE;
                break;
            }
            ls = ls->next;
        }
        if (!bFound)
        {
            //
            // The current column is not Unique. Check to see if it was unique before, 
            // ie previously in the Unique Sub-Dialog, if so, uncheck the Unique attribute.
            ls = lpunique;
            while (ls)
            {
                if (StringList_Count (ls->lpcolumnlist) == 1 && lstrcmpi ((ls->lpcolumnlist)->lpszString, lpFld->lpFTitle) == 0)
                {
                    memset (&dataUnique, 0, sizeof (CNTDATA));
                    lpRec = GetDataTypeRecord(hwndCnt, DT_UNIQUE);
                    CntFldDataGet (lpRec, lpFld, sizeof (CNTDATA), &dataUnique);
                    dataUnique.bUnique = FALSE;
                    CntFldDataSet (lpRec, lpFld, sizeof(CNTDATA), &dataUnique);
                    break;
                }
                ls = ls->next;
            }
        }
        lpFld = CntPrevFld(lpFld);
    }
    CntEndDeferPaint  (hwndCnt, TRUE);
}

//
// Alter Primary Key
static void OnPrimaryKey(HWND hwnd)
{
    TABLEPARAMS ts;
    HWND hwndContainer = NULL;
    LPCOLUMNPARAMS lpCol = NULL;
    LPOBJECTLIST ls = NULL;
    LPSTRINGLIST lpObj = NULL;
    LPTABLEPARAMS lptbl = GetDlgProp (hwnd);
    if (!lptbl)
        return;

    memset (&ts, 0, sizeof (ts));
    //
    // Construct the list of columns to use for creating primary key:
    MakeColumnsList   (hwnd, &ts);
    
    ls = ts.lpColumns;
    while (ls)
    {
        lpCol = (LPCOLUMNPARAMS)ls->lpObject;
        if (lpCol->uDataType != INGTYPE_LONGVARCHAR && lpCol->uDataType != INGTYPE_LONGBYTE)
        {
            statictableparams.primaryKey.pListTableColumn = StringList_AddObject (statictableparams.primaryKey.pListTableColumn, lpCol->szColumn, &lpObj);
            if (lpObj)
                lpObj->extraInfo1 = lpCol->bNullable? 0: 1;
        }
        ls = ls->lpNext;
    }
    //
    // Check to see if the primary key is marked for deleting:
    if (statictableparams.primaryKey.nMode != 1 && statictableparams.primaryKey.nMode != 2)
    {
        VDBA_TablePrimaryKey(hwnd, lptbl->DBName, &(statictableparams.primaryKey), &statictableparams);
    }
    ts.lpColumns = VDBA20xColumnList_Done (ts.lpColumns);
    hwndContainer = GetDlgItem(hwnd, IDC_CONTAINER);
    if (hwndContainer)
    {
        RECT r;
        GetClientRect(hwndContainer, &r);
        InvalidateRect(hwndContainer, &r, FALSE);
    }
    statictableparams.primaryKey.pListTableColumn = StringList_Done (statictableparams.primaryKey.pListTableColumn);
}

static void OnChangeFocus(HWND hwnd)
{
   HWND hwndCnt = GetDlgItem (hwnd, IDC_CONTAINER);

   lpEditRC = CntFocusRecGet(hwndCnt);
   lpEditFI = CntFocusFldGet(hwndCnt);
}

static void InitializeDataLenWithDefault(HWND hwndCnt, LPFIELDINFO lpFI)
{
   LPRECORDCORE lpLenRC;
   CNTDATA data,dataType;

   lpLenRC = GetDataTypeRecord(hwndCnt, DT_LEN);
   CntFldDataGet(lpLenRC, lpFI, sizeof(data) , &data);
   ASSERT(lpTypeRC);
   ASSERT(lpTypeFI);
   CntFldDataGet(lpTypeRC, lpTypeFI, sizeof(dataType) , &dataType);
   switch (dataType.uDataType)
   {
   case INGTYPE_FLOAT:
      data.len.dwDataLen = DEFAULT_FLOAT;
      break;
   case INGTYPE_DECIMAL:
      ZEROINIT(data.len);
      break;
   default:
      data.len.dwDataLen = 0;
      break;
   }
   CntFldDataSet(lpLenRC, lpFI, sizeof(data), &data);
   CntUpdateRecArea(hwndCnt, lpLenRC, lpFI);
}

static void SetDataTypeWithCurSelCombobox()
{
   CNTDATA data;
   int nIdx;

   ASSERT(lpTypeRC);
   ASSERT(lpTypeFI);
   ASSERT(hwndType);

   if (!lpTypeFI || !lpTypeRC || !hwndType)
      return;
   // Set data with curent selection in combobox
   nIdx = ComboBox_GetCurSel(hwndType);
   if ( nIdx != CB_ERR )
   {
       data.uDataType = (int)ComboBox_GetItemData(hwndType, nIdx);
       CntFldDataSet(lpTypeRC, lpTypeFI, sizeof(data), &data);
       return;
   }
   ASSERT(nIdx != CB_ERR);
}

static void UpdateNullableAndDefault(HWND hwndCnt, LPFIELDINFO lpFI)
{
   LPRECORDCORE lpRecNullable,lpRecType,lpRecDefault,lpRecSystMain;
   CNTDATA      data,dataType,dataDef;

   lpRecNullable = GetDataTypeRecord(hwndCnt, DT_NULLABLE);
   lpRecType     = GetDataTypeRecord(hwndCnt, DT_TYPE);
   lpRecDefault  = GetDataTypeRecord(hwndCnt, DT_DEFAULT);
   lpRecSystMain = GetDataTypeRecord(hwndCnt, DT_SYSMAINTAINED);

   CntFldDataGet(lpRecType, lpTypeFI, sizeof(CNTDATA) , &dataType);
   if (dataType.uDataType == INGTYPE_LONGBYTE ||
       dataType.uDataType == INGTYPE_UNICODE_LNVCHR ||
       dataType.uDataType == INGTYPE_LONGVARCHAR)
   {
       CntFldDataGet(lpRecNullable, lpTypeFI, sizeof(CNTDATA) , &data);
       if (!data.bNullable)
       {
           MessageBox (NULL, ResourceString ((UINT)IDS_E_NULLABLE_MANDATORY), 
                             ResourceString ((UINT)IDS_TITLE_WARNING), MB_ICONEXCLAMATION|MB_OK);
           data.bNullable = !data.bNullable ;
           CntFldDataSet (lpRecNullable, lpTypeFI, sizeof(CNTDATA), &data);

           CntFldDataGet(lpRecDefault, lpTypeFI, sizeof(CNTDATA) , &dataDef);
           dataDef.bDefault = !dataDef.bDefault;
           CntFldDataSet (lpRecDefault, lpTypeFI, sizeof(CNTDATA), &dataDef);

       }
   }
   CntUpdateRecArea(hwndCnt, lpRecDefault, lpTypeFI);  // Update Default (Grey/Ungrey/hide)
   CntUpdateRecArea(hwndCnt, lpRecNullable, lpTypeFI); // Update Nullable (Grey/Ungrey)
   CntUpdateRecArea(hwndCnt, lpRecSystMain, lpTypeFI); // Update Syystem Maintained (Grey/Ungrey/hide)
}
