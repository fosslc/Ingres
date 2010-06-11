/*****************************************************************************
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Project  : Visual DBA
**
**    Source   : creattbl.c
**
**    Main Dialog box for Creating a Table
**
**
**  History after 04-May-1999:
**
**    28-May-1999 (schph01)
**      Fixed bug 97166 :
**      Create table / new column : if the "default" checkbox  is checked, 
**      and a default value is entered and then removed , create table was 
**      failing (with an error code of -31000). 
**    30-Jun-1999 (schph01)
**      bug #97072
**      Dom Window / Create table : When creating a new column, the default for
**      NULLABILITY was not consistent with that of the SQL syntax.
**      Since the "with null" SQL syntax  (which is the default ) implies in
**      fact that there is a default value of NULL, the VDBA interface is
**      modified as follows:
**      When creating a new column, both the "Nullable" and the "Default"
**      checkboxes are checked. The "def.spec" control is pre-filled with
**      "NULL".
**      In addition, when unchecking "Nullable", a warning reminds that the
**      default value of NULL (in the "Def spec" control) is not compatible
**      with the fact that the column is no more nullable, and proposes both
**      to uncheck the "Default" checkbox, and to remove the "NULL" string in
**      the "Def.Spec" control.
**      For gateways and STAR databases, only "Nullable" is checked when a
**      column is created, since the "default" controls are not present.
**    01-Jul-1999 (schph01)
**      bug #97686:
**      (the lenght of a column could be copied into the column name)
**      Update lpEditRC and lpEditFld variables in the management of the
**      following messages:
**          CN_SETFOCUS,CN_NEWFOCUS,CN_NEWFOCUSFLD,CN_NEWFOCUSREC and
**          CN_BEGFLDTTLEDIT
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
**   08-Jul-1999 (schph01)
**      SIR #97838
**      VDBA / Create table: having both "nullable" and "unique" checked on the
**      same column should not be allowed.
**      Currently, a warning appearing when pressing OK will just specify in this
**      case that the column will not be nullable because it has asked to be
**      unique.
**   08-Jul-1999 (schph01)
**      Additional change for SIR #97838
**      Don't manage the incompatibility between "unique" and "nullable" on
**      create table for STAR or GATEWAY tables, where "unique" doesn't apply.
**   26-May-2000 (noifr01)
**    bug #99242  cleanup for DBCS compliance
** 29-May-2000 (uk$so01)
**    bug 99242 (DBCS), replace the functions
**    IsCharAlphaNumeric by _istalnum
**    IsCharAlpha by _istalpha
** 08-Dec-2000 (schph01)
**    SIR 102823 Add two columns type: Object Key and Table Key
** 15-Dec-2000 (schph01)
**    SIR 102819 leave the button "Options" always available even with
**    "Create Table As Select" checkbox is checked.If the "select statement"
**    is filled the "Security Audit Key" combobox is filled with the result
**    of this statement 
** 26-Mar-2001 (noifr01)
**   (sir 104270) removal of code for managing Ingres/Desktop
** 04-Apr-2001 (schph01)
**   (sir 103820) Increasing the maximum size of character data types to 32K
** 21-Jun-2001 (schph01)
**    (SIR 103694) STEP 2 support of UNICODE datatypes
** 18-Sep-2001 (schph01)
**    (BUG 105386) hide "Default" checkbox and "Def Spec" edit and 
**    "System Maintained" checkbox when column type long byte, long varchar or
**    long nvarchar are selected.
** 14-Fev-2002 (uk$so01)
**    SIR #106648, Integrate SQL Assistant In-Process Server
** 28-Mar-2002 (noifr01)
**    (sir 99596) removed additional unused resources/sources
** 05-Jun-2002 (noifr01)
**     (bug 107773) removed unused VerifyColumns() function, and adapted 
**     buffer sizes
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
** 30-Oct-2003 (noifr01)
**    fixed cross integration error in change 465572 for SIR 99596
** 12-May-2004 (schph01)
**    (SIR 111507) Add management for new column type bigint
** 12-Oct-2006 (wridu01)
**    (Sir 116835) Add support for Ansi Date/Time datatypes
** 25-Oct-2006 (wridu01)
**    (Sir 116837) Add MONEY, LONG VARCHAR and LONG BYTE back into dropdown
**    for Gateway servers
** 09-Mar-2009 (smeke01) b121764
**    Pass parameters to CreateSelectStmWizard as UCHAR so that we don't need  
**    to use the T2A macro.
** 09-Mar-2010 (drivi01) SIR 123397
**    Increase the row height to space out rows in "Create Table" dialog
**    and prevent overlapping of controls.
** 23-Mar-2010 (drivi01)
**    Add option to create VectorWise table.  Disable option if it
**    is not a VectorWise installation.
**    Disable "Options" button if VectorWise is checked, as options
**    are not supported with VectorWise table.
** 11-May-2010 (drivi01)
**    Disable "Check" constraint button b/c it isn't supported.
**    Add OnCreateVectorWiseTable function which will be
**    executed every time "Create VectorWise table" is checked.
**    The function will enable/disable supported options and
**    set Table parameter structure bCreateVectorWise option
**    appropriately.
** 02-Jun-2010 (drivi01)
**    Remove hard coded buffer sizes.
*****************************************************************************/


#include "dll.h"
#include "dbadlg1.h"
#include "dlgres.h"
#include "getdata.h"
#include "msghandl.h"
#include "domdata.h"
#include "dbaginfo.h"
#include "catolist.h"
#include "catocntr.h"
#include "winutils.h"
#include "sqlasinf.h"
#include "starutil.h"
#include "extccall.h"

#include "prodname.h"
#include "resource.h"
#include "tchar.h"

static BOOL bPassedByExtraButton=FALSE;
static BOOL bDDB = FALSE;       // Distributed database ?
static BOOL IsDDB() { return bDDB; }
static BOOL bGenericGateway = FALSE;
static const char szNull[] = _T("NULL");

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

// Structure describing the data types available where
static CTLDATA typeTypesUnicode[] =
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
   INGTYPE_UNICODE_NCHR,   IDS_INGTYPE_UNICODE_NCHR,     // unicode nchar
   INGTYPE_UNICODE_NVCHR,  IDS_INGTYPE_UNICODE_NVCHR,   // unicode nvarchar
   INGTYPE_UNICODE_LNVCHR, IDS_INGTYPE_UNICODE_LNVCHR, // unicode long nvarchar
   -1    // terminator
};

static CTLDATA typeTypesBigInt[] =
{
   INGTYPE_C,            IDS_INGTYPE_C,
   INGTYPE_CHAR,         IDS_INGTYPE_CHAR,
   INGTYPE_TEXT,         IDS_INGTYPE_TEXT,
   INGTYPE_VARCHAR,      IDS_INGTYPE_VARCHAR,
   INGTYPE_LONGVARCHAR,  IDS_INGTYPE_LONGVARCHAR,
   INGTYPE_BIGINT,       IDS_INGTYPE_BIGINT,
   INGTYPE_INT8,         IDS_INGTYPE_INT8,
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


// Structure describing the data types available for GateWay
static CTLDATA typeGateWayTypes[] =
{
   //INGTYPE_C,            IDS_INGTYPE_C,
   INGTYPE_CHAR,         IDS_INGTYPE_CHAR,
   //INGTYPE_TEXT,         IDS_INGTYPE_TEXT,
   INGTYPE_VARCHAR,      IDS_INGTYPE_VARCHAR,
   INGTYPE_LONGVARCHAR,  IDS_INGTYPE_LONGVARCHAR,
   INGTYPE_INT4,         IDS_INGTYPE_INT4,
   INGTYPE_INT2,         IDS_INGTYPE_INT2,
   //INGTYPE_INT1,         IDS_INGTYPE_INT1,
   INGTYPE_DECIMAL,      IDS_INGTYPE_DECIMAL,
   INGTYPE_FLOAT,        IDS_INGTYPE_FLOAT,
   INGTYPE_FLOAT8,       IDS_INGTYPE_FLOAT8,
   INGTYPE_FLOAT4,       IDS_INGTYPE_FLOAT4,
   INGTYPE_DATE,         IDS_INGTYPE_DATE,
   INGTYPE_MONEY,        IDS_INGTYPE_MONEY,
   //INGTYPE_BYTE,         IDS_INGTYPE_BYTE,
   //INGTYPE_BYTEVAR,      IDS_INGTYPE_BYTEVAR,
   INGTYPE_LONGBYTE,     IDS_INGTYPE_LONGBYTE,
   INGTYPE_INTEGER,      IDS_INGTYPE_INTEGER,   // equivalent to INT4
   -1    // terminator
};

// Structure describing the min/max length for a given data type
typedef struct tagREGLENGTH
{
   int nId;             // Data type id
   int nMinLength;      // Minimum length of data type
   DWORD dwMaxLength;   // Maximum length of data type
} REGLENGTH, FAR * LPREGLENGTH;

// If a data type exists in the following table, the length edit box
// is enabled when the data type is selected.
static REGLENGTH nReqLength[] =
{
   INGTYPE_DECIMAL,     11111,   99999,     // special case 5 characters 31,31
                                            // where 31 = max decimal precision
   INGTYPE_CHAR,        0,       2000,
   INGTYPE_VARCHAR,     0,       2000,
//   INGTYPE_LONGVARCHAR, 0,       1024 * 2000000,
   INGTYPE_BYTE,        1,       2000,
   INGTYPE_BYTEVAR,     1,       2000,
//   INGTYPE_LONGBYTE,    1,       1024 * 2000000,
   INGTYPE_FLOAT,       0,       53,
   INGTYPE_TEXT,        1,       2000,
   INGTYPE_C,           1,       2000,
   -1    // terminator
};

static REGLENGTH nReqLength26[] =
{
   INGTYPE_DECIMAL,     11111,   99999,     // special case 5 characters 31,31
                                            // where 31 = max decimal precision
   INGTYPE_CHAR,        0,       32000,
   INGTYPE_VARCHAR,     0,       32000,
   INGTYPE_BYTE,        1,       32000,
   INGTYPE_BYTEVAR,     1,       32000,
   INGTYPE_TEXT,        1,       32000,
   INGTYPE_C,           1,       32000,
   INGTYPE_FLOAT,       0,       53,
   INGTYPE_UNICODE_NCHR, 1,      16000,
   INGTYPE_UNICODE_NVCHR,1,      16000,
   -1    // terminator
};

// Union describing the container column data
typedef union tagCNTDATA
{
   UINT uDataType;      // Data type
   LENINFO len;         // Data type sizes
   int nPrimary;        // Primary key (0 = not part of primary key)
   BOOL bNullable;      // TRUE if nullable
   BOOL bUnique;        // TRUE if unique data
   BOOL bDefault;       // TRUE if default
   LPSTR lpszDefSpec;   // if bDefault, default value to use
   LPSTR lpszCheck;     // check constraint at the column level
   BOOL bSystemMaintained;// TRUE if System Maintained for an Objectkey
} CNTDATA, FAR * LPCNTDATA;

// Pointers for each record in the container
static LPCNTDATA lprecType = NULL;
static LPCNTDATA lprecLen = NULL;
static LPCNTDATA lprecPrimary = NULL;
static LPCNTDATA lprecNullable = NULL;
static LPCNTDATA lprecUnique = NULL;
static LPCNTDATA lprecDefault = NULL;
static LPCNTDATA lprecDefSpec = NULL;
// static LPCNTDATA lprecCheck = NULL;
static LPCNTDATA lprecSysMaintained = NULL;

// Defines for each record in the container
// For star nodes, we only use DT_TYPE, DT_LEN, DT_NULLABLE and DT_DEFAULT
// All Correspondence is made by GetRecordDataType() which calls ConvertRecnumIntoDataType(),
// and a reverse function also exists
#define DT_TYPE      10
#define DT_LEN       11
#define DT_PRIMARY   12
#define DT_NULLABLE  13
#define DT_UNIQUE    14
#define DT_DEFAULT   15
#define DT_DEFSPEC   16
// #define DT_CHECK     17
#define DT_SYSMAINTAINED 17
#define DT_TITLE     -1    // special case for the field title

static LPCNTDATA FAR * cntRecords[] =
{
   &lprecType,
   &lprecLen,
   //&lprecPrimary,
   &lprecNullable,
   &lprecUnique,
   &lprecDefault,
   &lprecDefSpec,
//   &lprecCheck,
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

// Pointer to the field drawing procedure for the container
static DRAWPROC lpfnDrawProc = NULL;
// Size of the container records structure
static UINT uDataSize;
// Total number of fields that have been created in the container
static UINT uFieldCounter = 0;
// Pointer to the original container child window procedure
static WNDPROC wndprocCntChild = NULL;
// Pointer to the original edit window procedure
static WNDPROC wndprocEdit = NULL;
// Pointer to the original table name edit window procedure
static WNDPROC wndprocName = NULL;
// Pointer to the original type combo box window procedure
static WNDPROC wndprocType = NULL;
// Pointer to the original column name edit box
static WNDPROC wndprocColName = NULL;
// Handle to the container child.
static HWND hwndCntChild = NULL;
// Handle to the edit control used in the container for editing.
static HWND hwndCntEdit = NULL;
// Handle to the data type drop down combo used in the container
static HWND hwndType = NULL;
// Height of the check box.  Since its square the width is the same.
static int nCBHeight = 0;
// Storage for the container edit box to use
static LPRECORDCORE lpEditRC;
static LPFIELDINFO lpEditFI;

// Storage for the container type window to use.
static LPRECORDCORE lpTypeRC;
static LPFIELDINFO lpTypeFI;

static HWND hwndThisDialog = NULL;


// Margins to use for the check box in the container
#define BOX_LEFT_MARGIN     2
#define BOX_VERT_MARGIN     2
// Margin to use for the text for the check box in the container
#define TEXT_LEFT_MARGIN    3
// Window identifier for the container edit window
#define ID_CNTEDIT         20000
// Window identifier for the container drop down data type list
#define ID_CNTCOMBO        20001
// Width of the edit control border.
#define EDIT_BORDER_WIDTH  2
// Width of the cursor in the edit control
#define EDIT_CURSOR_WIDTH  1
// Width of the focus border
#define FOCUS_BORDER_WIDTH 2
// Width of the container separator line
#define SEPARATOR_WIDTH    1
// Height of type combobox window
#define TYPE_WINDOW_HEIGHT 120
// Maximum length of a varchar
#define MAX_VARCHAR        999
// Maximum length of a check or default spec string
#define MAX_DEFAULTSTRING  1500
// Gap between the text and the edit box
#define EDIT_LEFT_MARGIN      2
// Width of the combo box control border.
#define CB_BORDER_WIDTH  2
// Width of the focus field in the combobox control
#define CB_FOCUS_WIDTH  1
// Max precision for decimal data type
#define MAX_DECIMAL_PRECISION 31

#define DEFAULT_PRECISION  5
#define DEFAULT_SCALE      0
#define DEFAULT_FLOAT      8

BOOL CALLBACK __export DlgCreateTableDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI __export CntTblChildSubclassProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI __export TblCntChildDefWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK __export EnumTblCntChildProc(HWND hwnd, LPARAM lParam);
LRESULT WINAPI __export TblEditSubclassProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI __export TblEditDefWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI __export TblNameDefWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI __export TblTypeSubclassProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI __export TblTypeDefWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI __export ColNameSubclassProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI __export ColNameDefWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

static void OnDestroy(HWND hwnd);
static void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
static BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);
static void OnSysColorChange(HWND hwnd);

static void OccupyTypeControl(HWND hwnd);
static void InitialiseEditControls (HWND hwnd);
static void InitContainer(HWND hwndCtnr);
int CALLBACK __export DrawTblFldData( HWND   hwndCnt,
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
static void DrawCheckBox(HDC hdc, LPRECT lprect, BOOL bSelected, int nStringId, BOOL bGrey);
static void CntOnLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags);
static void CntOnLButtonUp(HWND hwnd, int x, int y, UINT keyFlags);
static void CntOnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
static void TblName_OnChar(HWND hWnd, UINT ch, int cRepeat);
static void Edit_OnChar(HWND hWnd, UINT ch, int cRepeat);
static void Edit_OnKey(HWND hWnd, UINT vk, BOOL fDown, int cRepeat, UINT flags);
static UINT Edit_OnGetDlgCode(HWND hWnd, MSG FAR* lpmsg);
static void Edit_OnKillFocus(HWND hwnd, HWND hwndNewFocus);
static void Type_OnChar(HWND hWnd, UINT ch, int cRepeat);
static void Type_OnKey(HWND hWnd, UINT vk, BOOL fDown, int cRepeat, UINT flags);
static UINT Type_OnGetDlgCode(HWND hWnd, MSG FAR* lpmsg);
static void Type_OnSetFocus(HWND hwnd, HWND hwndOldFocus);
static void Type_OnKillFocus(HWND hwnd, HWND hwndNewFocus);
static BOOL CntFindField(HWND hwnd, LPSTR lpszTitle);
static void DrawStaticText(HDC hDC, LPRECT lprect, LPCSTR lpszText, BOOL bAlignLeft, BOOL bGrey, UINT uStringId);
static BOOL AllocContainerRecords(UINT uSize);
static BOOL ReAllocContainerRecords(UINT uNewSize);
static void FreeContainerRecords(void);
static void CopyContainerRecords(HWND hwndCnt);
static void InitialiseFieldData(UINT uField);
static void AddRecordsToContainer(HWND hwndCnt);
static int GetRecordDataType(HWND hwndCnt, LPRECORDCORE lpRCIn);
static void ShowEditWindow(HWND hwndCnt, LPRECT lprect, int nType, LPCNTDATA lpdata);
static void ShowTypeWindow(HWND hwndCnt, LPRECT lprect);
static void SaveEditData(HWND hwndCnt);
static void MakeColumnsList(HWND hwnd, LPTABLEPARAMS lptbl);
static void ReleaseField(HWND hwndCnt, LPFIELDINFO lpField);
static void ReleaseAllFields(HWND hwndCnt);
static void ColName_OnKey(HWND hWnd, UINT vk, BOOL fDown, int cRepeat, UINT flags);
static UINT ColName_OnGetDlgCode(HWND hWnd, MSG FAR* lpmsg);
static void AddContainerColumn(HWND hwnd);
static LPRECORDCORE GetDataTypeRecord(HWND hwndCnt, int nDataType);
static int GetMinReqLength(int nId);
static long GetMaxReqLength(int nId);
static int GetDataTypeStringId(int nId);
static void MakeTypeLengthString(UINT uType, LPCNTDATA lpdata, LPSTR lpszNumber);
static void GetTypeLength(UINT uType, LPCNTDATA lpdata, LPSTR lpszNumber);
static void VerifyDecimalString(HWND hwndEdit);
static void GetTitleRect(HWND hwndCnt, LPRECT lprect);
static void ResetCntEditCtl(HWND hwndCnt);
static BOOL FindReferencesColumnName(HWND hwnd, LPSTR lpszCol, LPOBJECTLIST FAR * FAR * lplpRefList, LPOBJECTLIST FAR * lplpElement);
static void EnableOKButton(HWND hwnd);
static void CreateDlgTitle(HWND hwnd);
static LPOBJECTLIST DropEmptyReferences(LPOBJECTLIST lplist);
static LPOBJECTLIST DropDuplicateReferences(LPOBJECTLIST lplistRef);
static int GetLastPrimaryKey(HWND hwndCnt);
static void VerifyPrimaryKeySequence(HWND hwndCnt, LPFIELDINFO lpFIKey);

static BOOL IsLengthZero (HWND hwnd, char* buffOut);
static void OnAsSelectClick         (HWND hwnd);
static BOOL OnAssistant (HWND hwnd);
static void OnUniqueConstraint      (HWND hwnd);
static void OnCheckConstraint       (HWND hwnd);
static void OnCreateVectorWiseTable	(HWND hwnd);
static BOOL  VDBA20xTableDataValidate (HWND hwnd, LPTABLEPARAMS lpTS);

static void OnOK (HWND hwnd);
static void OnColumnEditChange (HWND hwnd);
static void OnTableEditChange  (HWND hwnd);
static void OnAddColumn    (HWND hwnd);
static void OnDropColumn   (HWND hwnd);

static void OnCntCharHit   (HWND hwnd);
static void OnOption       (HWND hwnd);
static void OnReference    (HWND hwnd);
static void CntUpdateUnique(HWND hwnd, LPTLCUNIQUE lpunique, LPTLCUNIQUE lpNewUnique);
static int PrimaryKeyColumn_Count (HWND hwndCnt);
static void OnPrimaryKey(HWND hwnd);

static void OnStarLocalTable(HWND hwnd);
static int  ConvertRecnumIntoDataType(int nOffset);
static int  ConvertDataTypeIntoRecnum(int nDataType);
static void InitializeFieldDataWithDefault(HWND hwnd);
static void VerifyDefaultAndSpecDef(HWND hwndCtl, LPFIELDINFO lpFICur);
static void OnChangeFocus(HWND hwnd);
static void OnBeginTitleEdit(HWND hwnd);
static void InitializeDataLenWithDefault(HWND hwndCnt, LPFIELDINFO lpFI);
static void SetDataTypeWithCurSelCombobox();
static BOOL AddDummyWhereClause(LPTSTR *lpNewString, LPTSTR lpSelectStatement,
                                LPTSTR lpTblName,    LPTSTR lpHeader);

int WINAPI __export DlgCreateTable (HWND hwndOwner, LPTABLEPARAMS lpcreate)
/*
    Function:
        Shows the Create Table dialog

    Paramters:
        hwndOwner   - Handle of the parent window for the dialog.
        lpcreate    - Points to structure containing information used to 
                        - initialise the dialog. The dialog uses the same
                        - structure to return the result of the dialog.
    Returns:
        TRUE if successful.

    Note:
        If invalid parameters are passed in they are reset to a default
        so the dialog can be safely displayed.
*/
{
    int iRetVal;
    FARPROC lpfnDlgProc;
    if (!IsWindow(hwndOwner) || !lpcreate)
    {
        ASSERT(NULL);
        return -1;
    }
    if (!(*lpcreate->DBName))
    {
        ASSERT(NULL);
        return -1;
    }
    lpfnDlgProc = MakeProcInstance((FARPROC)DlgCreateTableDlgProc, hInst);
    iRetVal = VdbaDialogBoxParam (hResource, MAKEINTRESOURCE(IDD_CREATETABLE), hwndOwner, (DLGPROC)lpfnDlgProc, (LPARAM)lpcreate);
    FreeProcInstance (lpfnDlgProc);
    return iRetVal;
}


BOOL CALLBACK __export DlgCreateTableDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
        HANDLE_MSG(hwnd, WM_DESTROY, OnDestroy);
        case WM_INITDIALOG:
            return HANDLE_WM_INITDIALOG(hwnd, wParam, lParam, OnInitDialog);
        default:
            return HandleUserMessages(hwnd, message, wParam, lParam);
    }
    return FALSE;
}


static BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    LPCNTINFO lpcntInfo;
    BOOL bEnabled = FALSE;
    LPTABLEPARAMS lptbl = (LPTABLEPARAMS)lParam;
    if (!AllocDlgProp(hwnd, lptbl))
        return FALSE;
    // Initialise our globals.

    hwndThisDialog = hwnd;
    bPassedByExtraButton=FALSE;
    lpfnDrawProc    = NULL;
    uDataSize       = 0;
    uFieldCounter   = 0;
    wndprocCntChild = NULL;
    hwndCntChild    = NULL;
    hwndCntEdit     = NULL;
    hwndType        = NULL;
    nCBHeight       = 0;
    lpEditFI        = NULL;
    lpEditRC        = NULL;

    bDDB            = FALSE;
    bGenericGateway = FALSE;
    //
    // OpenIngres Version 2.0 only
    // Initialize the Combobox of page_size.
    // ComboBox_InitPageSize (hwnd, lpidx->uPage_size);
    if (GetOIVers() == OIVERS_12)
    {
        ShowWindow (GetDlgItem (hwnd, IDC_STATIC_PS), SW_HIDE);
        ShowWindow (GetDlgItem (hwnd, IDC_COMBOPAGESIZE), SW_HIDE);
    }
    else
    {
        ShowWindow (GetDlgItem (hwnd, IDC_STATIC_PS), SW_SHOW);
        ShowWindow (GetDlgItem (hwnd, IDC_COMBOPAGESIZE), SW_SHOW);
    }

    if (IsVW())
	  EnableWindow (GetDlgItem (hwnd, IDC_INGCHECKVW), TRUE);
    else
	  EnableWindow (GetDlgItem (hwnd, IDC_INGCHECKVW), FALSE);

    // Star management
    bDDB = lptbl->bDDB;   // global variable update
    lptbl->bCoordinator = TRUE;
    lptbl->bDefaultName = TRUE;
    if (IsDDB()) {
      ShowWindow(GetDlgItem(hwnd, IDC_BUTTON_STAR_LOCALTABLE), SW_SHOW);
      ShowWindow(GetDlgItem(hwnd, IDC_REFERENCES), SW_HIDE);
      ShowWindow(GetDlgItem(hwnd, IDC_TLCBUTTON_UNIQUE), SW_HIDE);
      ShowWindow(GetDlgItem(hwnd, IDC_TLCBUTTON_CHECK), SW_HIDE);
    }

    // generic gateway
    if (GetOIVers() == OIVERS_NOTOI)
      bGenericGateway = TRUE;
    if (bGenericGateway) {
      ShowWindow(GetDlgItem(hwnd, IDC_BUTTON_STAR_LOCALTABLE), SW_HIDE);
      ShowWindow(GetDlgItem(hwnd, IDC_REFERENCES), SW_HIDE);
      ShowWindow(GetDlgItem(hwnd, IDC_TLCBUTTON_UNIQUE), SW_HIDE);
      ShowWindow(GetDlgItem(hwnd, IDC_TLCBUTTON_CHECK), SW_HIDE);
      ShowWindow(GetDlgItem(hwnd, IDC_STRUCTURE ), SW_HIDE);
    }
/*
** Don't switch procedure for MAINWIN builds, causes bug 108564
** FIXME!!!!
*/

#ifndef MAINWIN
    wndprocColName = SubclassWindow(GetDlgItem(hwnd, IDC_COLNAME), ColNameSubclassProc);
#endif

    InitialiseEditControls(hwnd);
    InitContainer(GetDlgItem(hwnd, IDC_CONTAINER));
    // Disable the OK button if there is no text in the table name control
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
    lptbl->bJournaling = TRUE;
    lptbl->bDuplicates = TRUE;
    lptbl->nSecAudit   = SECURITY_AUDIT_TABLE;
    lptbl->nLabelGran  = LABEL_GRAN_SYS_DEFAULTS;
    lptbl->lpLocations = NULL;
    lptbl->uPage_size  = 2048L;
    lptbl->szSecAuditKey[0] = '\0';
    
    CreateDlgTitle(hwnd);
    if (IsDDB()) 
        lpHelpStack = StackObject_PUSH (lpHelpStack, StackObject_INIT ((UINT)8022));
    else
        lpHelpStack = StackObject_PUSH (lpHelpStack, StackObject_INIT ((UINT)IDD_CREATETABLE));
    richCenterDialog(hwnd);
    return TRUE;
}


static void OnDestroy(HWND hwnd)
{
    HWND hwndCnt = GetDlgItem(hwnd, IDC_CONTAINER);
    LPTABLEPARAMS lptbl = GetDlgProp (hwnd);
   
    ASSERT(IsWindow(hwndCnt));
    ReleaseAllFields(hwndCnt);
    SubclassWindow(hwndCntChild, wndprocCntChild);
    if (lpfnDrawProc)
        FreeProcInstance(lpfnDrawProc);
    FreeContainerRecords();
   
    FreeAttachedPointers (lptbl,  OT_TABLE);        // Vut adds
    DeallocDlgProp(hwnd);
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
        EndDialog (hwnd, FALSE);
        break;
    case IDC_COLNAME:
        if (codeNotify == EN_CHANGE)
            OnColumnEditChange (hwnd);
        break;
    case IDC_TABLE:
        if (codeNotify == EN_CHANGE)
            OnTableEditChange (hwnd);
        break;
    case IDC_ADD:
        OnAddColumn (hwnd);
        break;
    case IDC_DROP:
        OnDropColumn (hwnd);
        break;

    case IDC_CONTAINER:
        switch (codeNotify)
        {
        case CN_CHARHIT:
            OnCntCharHit (hwnd);
            break;
        case CN_SETFOCUS:
        case CN_NEWFOCUS:
        case CN_NEWFOCUSFLD:
        case CN_NEWFOCUSREC:
            OnChangeFocus(hwnd);
            break;
        case CN_BEGFLDTTLEDIT:
            OnBeginTitleEdit(hwnd);
            break;
        }
        break;
    case IDC_STRUCTURE:
        OnOption (hwnd);
        break;
    case IDC_REFERENCES:
        OnReference (hwnd);
        break;
    case IDC_INGCHECKASSELECT:
        OnAsSelectClick (hwnd);
        break;
    case IDC_INGEDITSQL:
        EnableOKButton(hwnd);
        break;
    case IDC_BUTTON_INGASSISTANT:
        OnAssistant (hwnd);
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

    case IDC_BUTTON_STAR_LOCALTABLE:
      OnStarLocalTable(hwnd);
      break;
    case IDC_INGCHECKVW:
      OnCreateVectorWiseTable(hwnd);
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

    // Limit the text in the edit controls
    Edit_LimitText(hwndTable, MAXOBJECTNAME-1);
    // Initialise edit controls with strings
    Edit_SetText(hwndTable, lptbl->objectname);
}


static void InitContainer(HWND hwnd)
/*
    Function:
        Performs the necessary initialisation for the container window.

    Parameters:
        hwnd    - Handle to the container window.

    Returns:
        None.
*/
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
    CntAttribSet(hwnd, CA_MOVEABLEFLDS | CA_FLDTTL3D | CA_VERTFLDSEP);
    // Height of check box plus margins
    nCBHeight = tm.tmHeight - 1;
    
    // Set the height of the container control to that of an edit control
    GetClientRect(GetDlgItem(hdlg, IDC_COLNAME), &rect);
    nRowHeight = rect.bottom - rect.top;
    CntRowHtSet(hwnd, -nRowHeight+(-4), 0);
    // Define space for container field titles.
    CntFldTtlHtSet(hwnd, 1);
    CntFldTtlSepSet(hwnd);
    // Set up some cursors for the different areas.
    CntCursorSet( hwnd, LoadCursor(NULL, IDC_UPARROW ), CC_FLDTITLE );
    // Set some fonts for the different container areas.
    CntFontSet( hwnd, hFont, CF_GENERAL );
    // Initialise the drawing procedure for our owner draw fields.
    lpfnDrawProc = (DRAWPROC)MakeProcInstance((FARPROC)DrawTblFldData, hInst);
    AllocContainerRecords(sizeof(CNTDATA));
    // Now paint the container.
    CntEndDeferPaint( hwnd, TRUE );

    // Subclass the container child window so we can operate the check boxes.
    EnumChildWindows(hwnd, EnumTblCntChildProc, (LPARAM)(HWND FAR *)&hwndCntChild);
    wndprocCntChild = SubclassWindow(hwndCntChild, CntTblChildSubclassProc);
    
    // Create an edit window for the container.
    hwndCntEdit = CreateWindow("edit",
                               "",
                               WS_CHILD | WS_BORDER | ES_LEFT | ES_AUTOHSCROLL | WS_CLIPSIBLINGS,
                               0, 0, 0, 0,
                               hwndCntChild,
                               (HMENU)ID_CNTEDIT,
                               hInst,
                               NULL);
    wndprocEdit = SubclassWindow(hwndCntEdit, TblEditSubclassProc);
    SetWindowFont(hwndCntEdit, hFont, FALSE);
    // Create a combobox for the container column data types
    hwndType = CreateWindow("COMBOBOX",
                            "",
                            WS_CHILD | CBS_DROPDOWNLIST | CBS_HASSTRINGS |
                            CBS_SORT | WS_VSCROLL,
                            0, 0, 0, 0,
                            hwndCntChild,
                            (HMENU)ID_CNTCOMBO,
                            hInst,
                            NULL);
    wndprocType = SubclassWindow(hwndType, TblTypeSubclassProc);
    OccupyTypeControl(hdlg);
    SetWindowFont(hwndType, hFont, FALSE);
    CntFocRectEnable(hwnd, TRUE);
    lpcntInfo = CntInfoGet(hwnd);
    if (lpcntInfo->nFieldNbr < MAXIMUM_COLUMNS)
       bEnabled = TRUE;
    EnableWindow(GetDlgItem(hdlg, IDC_ADD), bEnabled);
    SelectObject(hdc, hOldFont);
    ReleaseDC(hwnd, hdc);
}


BOOL CALLBACK __export EnumTblCntChildProc(HWND hwnd, LPARAM lParam)
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


LRESULT WINAPI __export CntTblChildSubclassProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
       HANDLE_MSG(hwnd, WM_LBUTTONDOWN, CntOnLButtonDown);
       HANDLE_MSG(hwnd, WM_LBUTTONUP, CntOnLButtonUp);
       HANDLE_MSG(hwnd, WM_LBUTTONDBLCLK, CntOnLButtonDown);
       HANDLE_MSG(hwnd, WM_COMMAND, CntOnCommand);
    
       default:
          return TblCntChildDefWndProc (hwnd, message, wParam, lParam);
    }
    return 0L;
}


LRESULT WINAPI __export TblCntChildDefWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    return CallWindowProc(wndprocCntChild, hwnd, message, wParam, lParam);
}


int CALLBACK __export DrawTblFldData( HWND   hwndCnt,     // container window handle
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
    LPCNTDATA lpdata = (LPCNTDATA)lpData;
    COLORREF crText = CntColorGet(hwndCnt, CNTCOLOR_TEXT);
    COLORREF crBkgnd = CntColorGet(hwndCnt, CNTCOLOR_FLDBKGD);
    int nDataType;
    BOOL bGrey = FALSE;
    
    // Ensure our text comes out the color we expect
    
    crText = SetTextColor(hDC, crText);
    crBkgnd = SetBkColor(hDC, crBkgnd);
    
    nDataType = GetRecordDataType(hwndCnt, lpRec);
    
    // star: all records not used
    if (nDataType == -1)
      return 1;
    
    switch (nDataType)
    {
        case DT_TYPE:
        {
            int nStringId = GetDataTypeStringId(lpdata->uDataType);
            DrawStaticText(hDC, lpRect, NULL, TRUE, bGrey, IDS_TYPE);
            if (nStringId != -1)
            {
               lpRect->left += CB_BORDER_WIDTH + CB_FOCUS_WIDTH;
               DrawStaticText(hDC, lpRect, NULL, TRUE, bGrey, nStringId);
            }
            break;
        }
        
        case DT_LEN:
        {
            char szNumber[20];
            CNTDATA data;
            LPRECORDCORE lpRC = GetDataTypeRecord(hwndCnt, DT_TYPE);
            int nStringId = IDS_LENGTH;
            long lMax;
            
            CntFldDataGet(lpRC, lpFld, sizeof(data), &data);
            lMax = GetMaxReqLength(data.uDataType);
            
            if (lMax == -1)
               bGrey = TRUE;
            else
            {
               switch (data.uDataType)
               {
                  case INGTYPE_DECIMAL:
                     nStringId = IDS_PRESEL;
                     break;
            
                  case INGTYPE_FLOAT:
                     nStringId = IDS_SIGDIG;
                     break;
               }
            }

            DrawStaticText(hDC, lpRect, NULL, TRUE, bGrey, nStringId);

            if (bGrey)
               *szNumber = (char)NULL;
            else
            {
               MakeTypeLengthString(data.uDataType, lpdata, szNumber);
               lpRect->left += EDIT_BORDER_WIDTH + EDIT_CURSOR_WIDTH + EDIT_LEFT_MARGIN;
            }
            
            DrawStaticText(hDC, lpRect, szNumber, FALSE, bGrey, 0);
            break;
        }
        case DT_NULLABLE:
        {
            LPTABLEPARAMS lpTS = NULL;
            ASSERT (hwndThisDialog);
            if (!hwndThisDialog)
                break;
            lpTS  = (LPTABLEPARAMS)GetDlgProp (hwndThisDialog);
            ASSERT (lpTS);
            if (!lpTS)
                break;
            //
            // The column is part of primary key:
            if (StringList_Search (lpTS->primaryKey.pListKey, lpFld->lpFTitle))
            {
                lpdata->bNullable = FALSE;
                bGrey = TRUE;
            }
            DrawCheckBox(hDC, lpRect, lpdata->bNullable, IDS_NULLABLE, bGrey);
        break;
        }

        case DT_UNIQUE:
        {
           DrawCheckBox(hDC, lpRect, lpdata->bUnique, IDS_UNIQUE, FALSE);
           break;
        }
        
        case DT_DEFAULT:
        {
           CNTDATA data;
           LPRECORDCORE lpRC = GetDataTypeRecord(hwndCnt, DT_TYPE);
           CntFldDataGet(lpRC, lpFld, sizeof(data), &data);

           if (data.uDataType == INGTYPE_LONGVARCHAR ||
               data.uDataType == INGTYPE_LONGBYTE    ||
               data.uDataType == INGTYPE_UNICODE_LNVCHR)
              break; // not to display the checkbox for this data type
  
           DrawCheckBox(hDC, lpRect, lpdata->bDefault, IDS_DEFAULT, bGrey);
           break;
        }
        
        case DT_DEFSPEC:
        {
           CNTDATA data,datatype;
           LPRECORDCORE lpRC, lpRCType;

           lpRCType = GetDataTypeRecord(hwndCnt, DT_TYPE);
           CntFldDataGet(lpRCType, lpFld, sizeof(datatype), &datatype);
           if (datatype.uDataType == INGTYPE_LONGVARCHAR ||
               datatype.uDataType == INGTYPE_LONGBYTE    ||
               datatype.uDataType == INGTYPE_UNICODE_LNVCHR)
              break;// not to draw the static text for this data type

           lpRC = GetDataTypeRecord(hwndCnt, DT_DEFAULT);
           CntFldDataGet(lpRC, lpFld, sizeof(data), &data);
           if (!data.bDefault)
              bGrey = TRUE;

           DrawStaticText(hDC, lpRect, NULL, TRUE, bGrey, IDS_DEFSPEC);
           lpRect->left += EDIT_BORDER_WIDTH + EDIT_CURSOR_WIDTH + EDIT_LEFT_MARGIN;
           if (lpdata->lpszDefSpec)
              DrawStaticText(hDC, lpRect, lpdata->lpszDefSpec, FALSE, bGrey, 0);
           break;
        }
        case DT_SYSMAINTAINED:
        {
           CNTDATA data;
           LPRECORDCORE lpRC = GetDataTypeRecord(hwndCnt, DT_TYPE);
           CntFldDataGet(lpRC, lpFld, sizeof(data), &data);

           if (data.uDataType == INGTYPE_OBJKEY|| data.uDataType == INGTYPE_TABLEKEY)
              bGrey = FALSE;
           else
              break; // hide the checkbox "System Maintained"
           DrawCheckBox(hDC, lpRect, lpdata->bSystemMaintained, IDS_SYST_MAINTAINED, bGrey);
           break;
        }

        default:
           ASSERT(NULL);
    }

    SetTextColor(hDC, crText);
    SetBkColor(hDC, crBkgnd);
    
    // Repaint any child windows
    
    if (IsWindowVisible(hwndCntEdit))
       InvalidateRect(hwndCntEdit, NULL, TRUE);
    
    if (IsWindowVisible(hwndType))
       InvalidateRect(hwndType, NULL, TRUE);
    
    return 1;
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
    ExtTextOut (hdc, rect.left, rect.top, ETO_CLIPPED, &rect, szText, lstrlen(szText), (LPINT)NULL);
    if (bGrey)
        SetTextColor(hdc, crText);
}


static void CntOnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
   HWND hwndCombo = hwndCtl;
   HWND hwndCnt = GetParent(hwnd);

   // UKS Nov 3' 1995 <<
   // 
   //if (hwndCombo == hwndType && codeNotify == CBN_SELCHANGE)
   //      EnableOKButton (hwnd);
   // UKS Nov 3' 1995 >>

   switch (id)
   {
      case ID_CNTEDIT:
      {
         switch (codeNotify)
         {
            case EN_CHANGE:
            {
               if (!lpEditRC || !lpEditFI)
                  break;
               switch (GetRecordDataType(hwndCnt, lpEditRC))
               {
                  case DT_LEN:
                  {
                     LPRECORDCORE lpRC;
                     CNTDATA data;
                     long lMax;
                     int nMin;

                     lpRC = GetDataTypeRecord(hwndCnt, DT_TYPE);
                     CntFldDataGet(lpRC, lpEditFI, sizeof(data), &data);
                     nMin = GetMinReqLength(data.uDataType);
                     lMax = GetMaxReqLength(data.uDataType);
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
                  }
/*
                  case DT_PRIMARY:
                  {
                     // Only allow numeric characters less than or equal
                     // to the last highest primary key + 1.

                     int nMax = GetLastPrimaryKey(hwndCnt) + 1;
                     int nMin = 0;

                     VerifyNumericEditControl(hwnd,
                                              hwndCtl,
                                              FALSE,
                                              TRUE,
                                              (DWORD)nMin,
                                              (DWORD)nMax);

                     break;
                  }
*/
               }
               break;
            }
         }

         break;
      }

      case ID_CNTCOMBO:
      {
         switch (codeNotify)
         {
            case CBN_SELCHANGE:
            {
               LPRECORDCORE lpSystemRC;
               // Set data with curent selection in combobox
               SetDataTypeWithCurSelCombobox();
               // Reset the length to zero
               InitializeDataLenWithDefault(hwndCnt, lpTypeFI);
               // Enable or disable System maintained check box
               lpSystemRC = GetDataTypeRecord(hwndCnt, DT_SYSMAINTAINED);
               CntUpdateRecArea(hwndCnt, lpSystemRC, lpTypeFI);

               // Enable or disable Default check box
               lpSystemRC = GetDataTypeRecord(hwndCnt, DT_DEFAULT);
               CntUpdateRecArea(hwndCnt, lpSystemRC, lpTypeFI);
               // Enable or disable "Default specification" edit control
               lpSystemRC = GetDataTypeRecord(hwndCnt, DT_DEFSPEC);
               CntUpdateRecArea(hwndCnt, lpSystemRC, lpTypeFI);

               break;
            }
         }

         break;
      }
      }
}


static void CntOnLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags)
{
    POINT ptMouse;
    LPFIELDINFO   lpFI;
    LPRECORDCORE  lpRC;
    LPRECORDCORE  lpRec = NULL;
    LPRECORDCORE  lpRecType = NULL;
    HWND hwndCnt = GetParent(hwnd);
    HWND hwndDlg = GetParent(hwndCnt);
    LPTABLEPARAMS lpTS  = (LPTABLEPARAMS)GetDlgProp (hwndDlg);
    CNTDATA dataAttr;
    //
    // Forward message to take care of the focus rect.
    FORWARD_WM_LBUTTONDOWN(hwnd, fDoubleClick, x, y, keyFlags, TblCntChildDefWndProc);
    ptMouse.x = x;
    ptMouse.y = y;
    lpFI = CntFldAtPtGet(hwndCnt, &ptMouse);
    if (!lpFI)
        return;
    if (lpRC = CntRecAtPtGet(hwndCnt, &ptMouse))
    {
        CNTDATA data;
        LPCNTINFO lpcntInfo = CntInfoGet(hwndCnt);
        int  nId;
        RECT rect;
        BOOL bGreyOut  = FALSE, bDefined = FALSE;
        LPTLCUNIQUE ls = lpTS->lpUnique;

        // Get the data for this cell
        memset (&dataAttr, 0, sizeof (dataAttr));
        CntFldDataGet(lpRC, lpFI, sizeof(data), &data);
        GetFocusCellRect(hwndCnt, &rect);
        nId = GetRecordDataType(hwndCnt, lpRC);
        switch (nId)
        {
        case DT_TYPE:
            ShowTypeWindow(hwndCnt, &rect);
            break;
        case DT_DEFSPEC:
            lpRecType = GetDataTypeRecord(hwndCnt, DT_TYPE);
            CntFldDataGet(lpRecType, lpFI, sizeof(dataAttr), &dataAttr);
            if (dataAttr.uDataType == INGTYPE_LONGVARCHAR ||
                dataAttr.uDataType == INGTYPE_LONGBYTE    ||
                dataAttr.uDataType == INGTYPE_UNICODE_LNVCHR)
              break;

            lpRec = GetDataTypeRecord(hwndCnt, DT_DEFAULT);
            CntFldDataGet(lpRec, lpFI, sizeof(dataAttr), &dataAttr);
            if (!dataAttr.bDefault)
                break;
            ShowEditWindow(hwndCnt, &rect, nId, &data);
            break;
        case DT_LEN:
            lpRec = GetDataTypeRecord(hwndCnt, DT_TYPE);
            CntFldDataGet(lpRec, lpFI, sizeof(dataAttr), &dataAttr);
            if (GetMaxReqLength(dataAttr.uDataType) != -1) 
                ShowEditWindow(hwndCnt, &rect, nId, &data);
            break;

        case DT_NULLABLE:
            if (!data.bNullable && !IsDDB() && !bGenericGateway)
            {
                CNTDATA DataUnique;
                lpRec = GetDataTypeRecord(hwndCnt, DT_UNIQUE);
                CntFldDataGet(lpRec, lpFI, sizeof(DataUnique), &DataUnique);
                if (DataUnique.bUnique)
                {
                    // tchszMsg3
                    MessageBox (hwnd, ResourceString ((UINT)IDS_E_NULLABLEANDUNIQUE),
                                      ResourceString ((UINT)IDS_TITLE_WARNING), MB_ICONEXCLAMATION|MB_OK);
                    break;
                }
            }
            data.bNullable = !data.bNullable;
            CntFldDataSet(lpRC, lpFI, sizeof(data), &data);
            CntUpdateRecArea(hwndCnt, lpRC, lpFI);
            if (!data.bNullable)
                VerifyDefaultAndSpecDef(hwndCnt,lpFI);

            break;
        case DT_UNIQUE:
            bDefined = FALSE;
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
                // tchszMsg3
                MessageBox (hwnd, ResourceString ((UINT)IDS_E_USESUBDLG_2_DELETECONSTRAINT), ResourceString ((UINT)IDS_TITLE_WARNING), MB_ICONEXCLAMATION|MB_OK);
                break;
            }
            if (!data.bUnique && !IsDDB() && !bGenericGateway)
            {
                CNTDATA DataNullable;
                lpRec = GetDataTypeRecord(hwndCnt, DT_NULLABLE);
                CntFldDataGet(lpRec, lpFI, sizeof(DataNullable), &DataNullable);
                if (DataNullable.bNullable)
                {
                    // tchszMsg3
                    MessageBox (hwnd, ResourceString ((UINT)IDS_E_NULLABLEANDUNIQUE),
                                      ResourceString ((UINT)IDS_TITLE_WARNING), MB_ICONEXCLAMATION|MB_OK);
                    break;
                }
            }
            data.bUnique = !data.bUnique;
            CntFldDataSet(lpRC, lpFI, sizeof(data), &data);
            CntUpdateRecArea(hwndCnt, lpRC, lpFI);
            break;
        case DT_DEFAULT:
            lpRec = GetDataTypeRecord(hwndCnt, DT_TYPE);
            CntFldDataGet(lpRec, lpFI, sizeof(dataAttr), &dataAttr);
            if (dataAttr.uDataType == INGTYPE_LONGVARCHAR ||
                dataAttr.uDataType == INGTYPE_LONGBYTE    ||
                dataAttr.uDataType == INGTYPE_UNICODE_LNVCHR)
              break; // No update necessary when this data type are selected

            data.bDefault = !data.bDefault;
            CntFldDataSet(lpRC, lpFI, sizeof(data), &data);
            CntUpdateRecArea(hwndCnt, lpRC, lpFI);
            if (!IsDDB()) {
              lpRec = GetDataTypeRecord(hwndCnt, DT_DEFSPEC);
              CntUpdateRecArea(hwndCnt, lpRec, lpFI);
            }
            break;
        case DT_SYSMAINTAINED:
            lpRec = GetDataTypeRecord(hwndCnt, DT_TYPE);
            CntFldDataGet(lpRec, lpFI, sizeof(dataAttr), &dataAttr);
            if (dataAttr.uDataType == INGTYPE_TABLEKEY || dataAttr.uDataType == INGTYPE_OBJKEY) {
              if (!data.bSystemMaintained ) {
                CNTDATA DataSpec,DataDefault,DataNullable;
                LPRECORDCORE lpRecNullable,lpRecDefDefault,lpRecDefSpec;
                lpRecNullable   = GetDataTypeRecord(hwndCnt, DT_NULLABLE);
                lpRecDefDefault = GetDataTypeRecord(hwndCnt, DT_DEFAULT);
                lpRecDefSpec    = GetDataTypeRecord(hwndCnt, DT_DEFSPEC);
                CntFldDataGet(lpRecNullable  , lpFI , sizeof(DataNullable), &DataNullable);
                CntFldDataGet(lpRecDefSpec   , lpFI , sizeof(DataSpec)    , &DataSpec);
                CntFldDataGet(lpRecDefDefault, lpFI , sizeof(DataDefault) , &DataDefault);
                if (DataNullable.bNullable || !DataDefault.bDefault || DataSpec.lpszDefSpec) {
                  int iret;
                  iret = MessageBox (hwnd, ResourceString ((UINT)IDS_W_SYSTEM_MAINTAINED),
                                    ResourceString ((UINT)IDS_TITLE_WARNING), MB_ICONEXCLAMATION|MB_OKCANCEL);
                  if (iret == IDOK)
                  {
                    DataNullable.bNullable = FALSE;
                    DataDefault.bDefault   = TRUE;
                    if (DataSpec.lpszDefSpec) {
                        ESL_FreeMem(DataSpec.lpszDefSpec);
                        DataSpec.lpszDefSpec = NULL;
                    }
                    CntFldDataSet(lpRecNullable  , lpFI, sizeof(DataNullable), &DataNullable);
                    CntFldDataSet(lpRecDefSpec   , lpFI, sizeof(DataSpec)    , &DataSpec);
                    CntFldDataSet(lpRecDefDefault, lpFI, sizeof(DataDefault) , &DataDefault);
                    CntUpdateRecArea(hwndCnt, lpRecNullable  , lpFI);
                    CntUpdateRecArea(hwndCnt, lpRecDefSpec   , lpFI);
                    CntUpdateRecArea(hwndCnt, lpRecDefDefault, lpFI);
                  }
                }
              }
              data.bSystemMaintained = !data.bSystemMaintained;
              CntFldDataSet(lpRC, lpFI, sizeof(data), &data);
              CntUpdateRecArea(hwndCnt, lpRC, lpFI);
          }
            break;
        default:
            // Needs to be masqued out for star
            // ASSERT(NULL);
          ;
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
        {   // In the title area.
            rect.bottom--;
            ShowEditWindow(hwndCnt, &rect, DT_TITLE, lpFI->lpFTitle);
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
    
    FORWARD_WM_LBUTTONUP(hwnd, x, y, keyFlags, TblCntChildDefWndProc);
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
            Edit_SetSel(GetDlgItem(hdlg, IDC_COLNAME), 0, -1);
        }
    }
}


LRESULT WINAPI __export TblEditSubclassProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        HANDLE_MSG(hwnd, WM_KILLFOCUS, Edit_OnKillFocus);
        HANDLE_MSG(hwnd, WM_KEYDOWN, Edit_OnKey);
        HANDLE_MSG(hwnd, WM_CHAR, Edit_OnChar);
        HANDLE_MSG(hwnd, WM_GETDLGCODE, Edit_OnGetDlgCode);
        default:
            return TblEditDefWndProc(hwnd, message, wParam, lParam);
    }
    return 0L;
}


LRESULT WINAPI __export TblEditDefWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
   return CallWindowProc(wndprocEdit, hwnd, message, wParam, lParam);
}


static void Edit_OnKillFocus(HWND hwnd, HWND hwndNewFocus)
{
    HWND hwndCnt = GetParent(GetParent(hwnd));
    int nType;

    nType = GetRecordDataType(hwndCnt, lpEditRC);
    switch (nType)
    {
    case DT_TITLE:
        // Ensure we dont get into a situation where the column name
        // is blank. If we are editing the column name and it is blank,
        // reset the name to what it was before the edit.

        if (Edit_GetTextLength(hwnd) == 0)
           ResetCntEditCtl(hwndCnt);
        break;
    default:
        break;
    }

    SaveEditData (hwndCnt);
    SetWindowPos (hwndCntEdit, HWND_BOTTOM, 0, 0, 0, 0, SWP_HIDEWINDOW);
    CntUpdateRecArea (hwndCnt, lpEditRC, lpEditFI);
    FORWARD_WM_KILLFOCUS(hwnd, hwndNewFocus, TblEditDefWndProc);
}


static void Edit_OnChar(HWND hwnd, UINT ch, int cRepeat)
{
    HWND hwndCnt = GetParent(GetParent(hwnd));
    int nId;
    BOOL bForward = FALSE;
    
    // If the edit box has already lost focus don't let the char thru.
    // Since we process the ESCAPE, TAB, and RETURN on the keydown we
    // still get the WM_CHAR msg after the keyup. By the time we get it,
    // though, focus has shifted back to the container and the result
    // will be an annoying beep if the char msg goes thru. 
    
    if (ch == VK_ESCAPE || ch == VK_RETURN || ch == VK_TAB)
    {
        if(GetFocus() != hwnd)
            return;
    }
    nId = GetRecordDataType(hwndCnt, lpEditRC);
    // Handle the keys globally valid to all edit boxes
    if (ch == VK_BACK)
        bForward = TRUE;
    // Handle specific characters for each edit box type
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
           // If there is no comma and the first number is present then
           // allow the comma
           if (!(_fstrchr(szNumber, ',')) && *szNumber)
           {
                CntFldDataGet(lpRec, lpFI, sizeof(dataType), &dataType);
                if (dataType.uDataType == INGTYPE_DECIMAL  && ch == ',')
                    bForward = TRUE;
           }
           if ((_istalnum(ch) && !_istalpha(ch)) || bForward)
                FORWARD_WM_CHAR(hwnd, ch, cRepeat, TblEditDefWndProc);
           break;
        }
        case DT_DEFSPEC:
            FORWARD_WM_CHAR(hwnd, ch, cRepeat, TblEditDefWndProc);
            break;
        case DT_TITLE:
            if (IsValidObjectChar(ch) || bForward)
                FORWARD_WM_CHAR(hwnd, ch, cRepeat, TblEditDefWndProc);
            break;
/*
        case DT_PRIMARY:
            if ((IsCharAlphaNumeric((char)ch) && !IsCharAlpha((char)ch)) || bForward)
               FORWARD_WM_CHAR(hwnd, ch, cRepeat, TblEditDefWndProc);
            break;
*/
        default:
            ASSERT(NULL);
    }
}


static void Edit_OnKey(HWND hwnd, UINT vk, BOOL fDown, int cRepeat, UINT flags)
{
   HWND hwndCnt = GetParent(GetParent(hwnd));

   // We don't process key-up messages.
   if( !fDown )
   {
      // Forward this to the default proc for processing.
      FORWARD_WM_KEYUP(hwnd, vk, cRepeat, flags, TblEditDefWndProc);
      return;
   }

   switch(vk)
   {
      case VK_ESCAPE:
      {
         ResetCntEditCtl(hwndCnt);
         SetFocus(hwndCnt);
         break;
      }

      case VK_UP:
      case VK_DOWN:
      case VK_RETURN:
      case VK_TAB:
      {
         SetFocus(hwndCnt);

         // Send the RETURN or TAB to the container so it will notify
         // the app. The app will then move the focus appropriately.
         FORWARD_WM_KEYDOWN(hwndCnt, vk, cRepeat, flags, SendMessage);

         break;
      }
   }

   // Forward this to the default proc for processing.
   FORWARD_WM_KEYDOWN(hwnd, vk, cRepeat, flags, TblEditDefWndProc);
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
   uRet = (UINT)FORWARD_WM_GETDLGCODE(hwnd, lpmsg, TblEditDefWndProc);

   // Now specify that we want all keyboard input.
   uRet |= DLGC_WANTMESSAGE;

   return uRet;
}


LRESULT WINAPI __export TblTypeSubclassProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
   switch (message)
   {
      HANDLE_MSG(hwnd, WM_SETFOCUS, Type_OnSetFocus);
      HANDLE_MSG(hwnd, WM_KILLFOCUS, Type_OnKillFocus);
      HANDLE_MSG(hwnd, WM_KEYDOWN, Type_OnKey);
      HANDLE_MSG(hwnd, WM_CHAR, Type_OnChar);
      HANDLE_MSG(hwnd, WM_GETDLGCODE, Type_OnGetDlgCode);

      default:
        return TblTypeDefWndProc(hwnd, message, wParam, lParam);
   }

   return 0L;
}


LRESULT WINAPI __export TblTypeDefWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
   return CallWindowProc(wndprocType, hwnd, message, wParam, lParam);
}


static void Type_OnSetFocus(HWND hwnd, HWND hwndOldFocus)
{
   HWND hwndCnt = GetParent(GetParent(hwnd));

   lpTypeRC = CntFocusRecGet(hwndCnt);
   lpTypeFI = CntFocusFldGet(hwndCnt);
   FORWARD_WM_SETFOCUS(hwnd, hwndOldFocus, TblTypeDefWndProc);
}


static void Type_OnKillFocus(HWND hwnd, HWND hwndNewFocus)
{
    HWND hwndCnt = GetParent(GetParent(hwnd));
    int nIdx;
    CNTDATA data;
    LPRECORDCORE lpRC;
    LPFIELDINFO lpFI;
    int nMin;
    
    nIdx = ComboBox_GetCurSel(hwndType);
    data.uDataType = (int)ComboBox_GetItemData(hwndType, nIdx);
    CntFldDataSet(lpTypeRC, lpTypeFI, sizeof(data), &data);
    SetWindowPos(hwndType, HWND_BOTTOM, 0, 0, 0, 0, SWP_HIDEWINDOW);
    CntUpdateRecArea(hwndCnt, lpTypeRC, lpTypeFI);
    lpRC = GetDataTypeRecord(hwndCnt, DT_LEN);
    lpFI = CntFocusFldGet(hwndCnt);
    CntFldDataGet(lpRC, lpFI, sizeof(data), &data);
    // Ensure the data type length gets initialised with a valid value.
    nMin = GetMinReqLength(data.uDataType);
    if (nMin != -1)
    {
        if (data.uDataType != INGTYPE_DECIMAL)
        {
            if (data.len.dwDataLen <= 0)
                 data.len.dwDataLen = (DWORD)nMin;
        }
        else
        {
            data.len.decInfo.nPrecision = DEFAULT_PRECISION;
            data.len.decInfo.nScale = DEFAULT_SCALE;
        }
        CntFldDataSet(lpRC, lpTypeFI, sizeof(data), &data);
    }
    CntUpdateRecArea(hwndCnt, lpRC, lpTypeFI);
    lpRC = GetDataTypeRecord(hwndCnt, DT_TYPE);
    CntFldDataGet(lpRC, lpTypeFI, sizeof(data), &data);
    lpTypeRC = NULL;
    lpTypeFI = NULL;
    FORWARD_WM_KILLFOCUS(hwnd, hwndNewFocus, TblTypeDefWndProc);
}


static void Type_OnChar(HWND hwnd, UINT ch, int cRepeat)
{
    HWND hwndCnt = GetParent(GetParent(hwnd));
    FORWARD_WM_CHAR(hwnd, ch, cRepeat, TblTypeDefWndProc);
}


static void Type_OnKey(HWND hwnd, UINT vk, BOOL fDown, int cRepeat, UINT flags)
{
    HWND hwndCnt = GetParent(GetParent(hwnd));
    // We don't process key-up messages.
    if( !fDown )
    {
        // Forward this to the default proc for processing.
        FORWARD_WM_KEYUP(hwnd, vk, cRepeat, flags, TblTypeDefWndProc);
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
    FORWARD_WM_KEYDOWN(hwnd, vk, cRepeat, flags, TblTypeDefWndProc);
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
   uRet = (UINT)FORWARD_WM_GETDLGCODE(hwnd, lpmsg, TblTypeDefWndProc);
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


static void OccupyTypeControl(HWND hwnd)
/*
   Function:
      Occupy the type combo box with all the available data types.

   Parameters:
      hwnd     - Handle to the dialog window.

   Returns:
      None.
*/
{
   if (bGenericGateway)
      OccupyComboBoxControl(hwndType, typeGateWayTypes);
   else
   {

     /* Put explicit Checks for all version strings */

     if (GetOIVers() >= OIVERS_91)                             // II 9.1.0 or higher
        OccupyComboBoxControl(hwndType, typeTypesAnsiDate);
     else if (GetOIVers() == OIVERS_90)                        // II 9.0.x
            OccupyComboBoxControl(hwndType, typeTypesBigInt);
     else if (GetOIVers() == OIVERS_30)                        // II 3.0
            OccupyComboBoxControl(hwndType, typeTypesBigInt);
     else if (GetOIVers() == OIVERS_26)                        // II 2.6
            OccupyComboBoxControl(hwndType, typeTypesUnicode);
     else OccupyComboBoxControl(hwndType, typeTypes);          // Releases prior to II 2.6
   }
}


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
    UCHAR szText[50];
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
        fstrncpy(szText, (LPUCHAR)lpszText, min(lstrlen(lpszText) + 1, sizeof(szText)));
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
           return ConvertRecnumIntoDataType(nOffset);
        nOffset++;
        lpRC = CntNextRec(lpRC);
    }
    // Data type not found.  This should never happen.
    // ASSERT(NULL);
    return -1;
}


void GetFocusCellRect(HWND hwndCnt, LPRECT lprect)
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
    int nLen, nStringId;
    HDC hdc;
    HFONT hfont;
    DWORD dwLen;
    char szText[50];
    LPSTR lpszString;
    BOOL bUseCurrent = (HIWORD(lpdata) == 0 ? FALSE : TRUE);
    char szNumber[20];
    UINT uChar = LOWORD(lpdata);
    int nLeftMargin = EDIT_LEFT_MARGIN;
    
    hdc = GetDC(hwndCnt);
    hfont = SelectObject(hdc, CntFontGet(hwndCnt, CF_GENERAL));
    
    Edit_SetText(hwndCntEdit, "");
    switch (nType)
    {
    case DT_LEN:
        {
            // Get the maximum length of the string
            LPFIELDINFO lpFI = CntFocusFldGet(hwndCnt);
            LPRECORDCORE lpRC;
            CNTDATA data;
            long lMax;
            
            lpRC = GetDataTypeRecord(hwndCnt, DT_TYPE);
            CntFldDataGet(lpRC, lpFI, sizeof(data), &data);
            lMax = GetMaxReqLength(data.uDataType);
            if (lMax == -1)
                nLen = 0;
            else
            {
                my_dwtoa(lMax, szNumber, 10);
                nLen = lstrlen(szNumber);
            }
            // Set the header string
            switch (data.uDataType)
            {
            case INGTYPE_DECIMAL:
                nStringId = IDS_PRESEL;
                break;
            case INGTYPE_FLOAT:
                nStringId = IDS_SIGDIG;
                break;
            default:
                nStringId = IDS_LENGTH;
            }
            if (bUseCurrent)
            {
                MakeTypeLengthString(data.uDataType, lpdata, szNumber);
                lpszString = szNumber;
            }
        }
        break;
    
    case DT_DEFSPEC:
        nLen = MAX_DEFAULTSTRING;
        nStringId = IDS_DEFSPEC;
        if (bUseCurrent)
            lpszString = lpdata->lpszDefSpec;
        break;
    case DT_TITLE:
        nLen = MAXOBJECTNAME - 1;
        nStringId = 0;
        if (bUseCurrent)
            lpszString = (LPSTR)lpdata;
        nLeftMargin = 0;
        break;
/*
    case DT_PRIMARY:
        {
            LPCNTINFO lpcntInfo = CntInfoGet(hwndCnt);
            int nNext;
            my_itoa(lpcntInfo->nFieldNbr, szNumber, 10);
            nLen = lstrlen(szNumber);
            nStringId = IDS_PRIMARY;
            if (bUseCurrent)
            {
                nNext = lpdata->nPrimary;
                if (nNext == 0)
                    nNext = GetLastPrimaryKey(hwndCnt) + 1;
                my_itoa(nNext, szNumber, 10);
                lpszString = szNumber;
            }
        }
*/
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
    SetWindowPos (hwndCntEdit,
        HWND_TOP,
        lprect->left + (int)LOWORD(dwLen) + nLeftMargin,
        lprect->top,
        lprect->right - lprect->left - (int)LOWORD(dwLen) - nLeftMargin,
        lprect->bottom - lprect->top,
        SWP_SHOWWINDOW);
    SetFocus (hwndCntEdit);
    SelectObject(hdc, hfont);
    ReleaseDC(hwndCnt, hdc);
    // Either show the character that started the edit or the current data
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
   HDC hdc;
   HFONT hfont;
   DWORD dwLen;
   char szText[50];
   LPFIELDINFO lpFI;
   LPRECORDCORE lpRC;
   CNTDATA data;
   int nStringId;

   hdc = GetDC(hwndCnt);
   hfont = SelectObject(hdc, CntFontGet(hwndCnt, CF_GENERAL));
   LoadString(hResource, IDS_TYPE, szText, sizeof(szText));
   dwLen = GetTextExtent(hdc, szText, lstrlen(szText));

   // Select the current data type into the combo box

   lpFI = CntFocusFldGet(hwndCnt);
   lpRC = GetDataTypeRecord(hwndCnt, DT_TYPE);
   CntFldDataGet(lpRC, lpFI, sizeof(data), &data);
   if ((nStringId = GetDataTypeStringId(data.uDataType)) != -1)
   {
      LoadString(hResource, nStringId, szText, sizeof(szText));
      ComboBox_SelectString(hwndType, -1, szText);
   }
   SetWindowPos(hwndType,
                HWND_TOP,
                lprect->left + (int)LOWORD(dwLen) + EDIT_LEFT_MARGIN,
                lprect->top,
                lprect->right - lprect->left -
                              (int)LOWORD(dwLen) - EDIT_LEFT_MARGIN,
                TYPE_WINDOW_HEIGHT,
                SWP_SHOWWINDOW);
   SetFocus(hwndType);
   SelectObject(hdc, hfont);
   ReleaseDC(hwndCnt, hdc);
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
   CNTDATA data;

   ZEROINIT(data);

   if (lpEditRC && lpEditFI)
      CntFldDataGet(lpEditRC, lpEditFI, sizeof(data), &data);

   switch (GetRecordDataType(hwndCnt, lpEditRC))
   {
      case DT_LEN:
      {
         char szNumber[20];
         LPRECORDCORE lpRec = GetDataTypeRecord(hwndCnt, DT_TYPE);
         CNTDATA dataType;

         CntFldDataGet(lpRec, lpEditFI, sizeof(dataType), &dataType);
         Edit_GetText(hwndCntEdit, szNumber, sizeof(szNumber));
         GetTypeLength(dataType.uDataType, &data, szNumber);
         break;
      }

      case DT_DEFSPEC:
      {
         int nLen = Edit_GetTextLength(hwndCntEdit);
         if (nLen >0) 
         {
            if (data.lpszDefSpec)
            {
               data.lpszDefSpec = (LPSTR)ESL_ReAllocMem(data.lpszDefSpec,
                                                        nLen + 1,
                                                        lstrlen(data.lpszDefSpec) + 1);
            }
            else
            {
               data.lpszDefSpec = (LPSTR)ESL_AllocMem(nLen + 1);
            }
            Edit_GetText(hwndCntEdit, data.lpszDefSpec, nLen + 1);
         }
         else
         {
            if (data.lpszDefSpec)
            {
               ESL_FreeMem (data.lpszDefSpec);
               data.lpszDefSpec = NULL;
            }
         }
         break;
      }

      case DT_TITLE:
      {
         char szTitle[MAXOBJECTNAME + 1];

         Edit_GetText(hwndCntEdit, szTitle, sizeof(szTitle));
         CntFldTtlSet(hwndCnt, lpEditFI, szTitle, lstrlen(szTitle) + 1);
         break;
      }
/*
      case DT_PRIMARY:
      {
         char szNumber[20];

         Edit_GetText(hwndCntEdit, szNumber, sizeof(szNumber));
         data.nPrimary = my_atoi(szNumber);
         break;
      }
*/

      default:
         ASSERT(NULL);
   }

   if (lpEditRC && lpEditFI)
      CntFldDataSet(lpEditRC, lpEditFI, sizeof(data), &data);
}


static void MakeColumnsList(HWND hwnd, LPTABLEPARAMS lptbl)
/*
   Function:
      Create a linked list of COLUMNPARAMS objects for all the columns
      in the container.  Set the object with data based on the cells
      in that column.

   Parameters:
      lptbl    - Pointer to TABLEPARAMS object to receive the pointer
                 to the created linked list.

   Returns:
      None.
*/
{
   HWND hwndCnt = GetDlgItem(hwnd, IDC_CONTAINER);
   LPOBJECTLIST lpColList = NULL;
   LPFIELDINFO lpFld;
   int nbKey = 0;

   // Work through the container control and create a linked list
   // of the columns and their attributes.

   lpFld = CntFldTailGet(hwndCnt);

   while (lpFld)
   {
      LPCOLUMNPARAMS lpObj;
      LPOBJECTLIST lpColTemp;
      LPRECORDCORE lpRC;

      lpColTemp = AddListObject(lpColList, sizeof(COLUMNPARAMS));
      if (!lpColTemp)
      {
         // Need to free previously allocated objects.
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

         // Retrieve the data from the container for this column
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
/*
            case DT_PRIMARY:
               lpObj->nPrimary = data.nPrimary;
               nbKey++;
               break;
*/
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
              // Needs to be masqued out for star
              // ASSERT(NULL);
              ;
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
               // masqued type change to float 25/11/96
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
   //FreeObjectList (lptbl->lpColumns); // Vut 
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
   LPRECORDCORE lpRC = CntRecHeadGet(hwndCnt);

   while (lpRC)
   {
      CNTDATA data;

      // Get the data for this cell
      CntFldDataGet(lpRC, lpField, sizeof(data), &data);
      switch (GetRecordDataType(hwndCnt, lpRC))
      {
         case DT_DEFSPEC:
         {
            if (data.lpszDefSpec)
               ESL_FreeMem(data.lpszDefSpec);
            data.lpszDefSpec = NULL;
            break;
         }
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


LRESULT WINAPI __export ColNameSubclassProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        HANDLE_MSG(hwnd, WM_KEYDOWN, ColName_OnKey);
        HANDLE_MSG(hwnd, WM_GETDLGCODE, ColName_OnGetDlgCode);
        default:
            return ColNameDefWndProc( hwnd, message, wParam, lParam);
    }
    return 0L;
}


LRESULT WINAPI __export ColNameDefWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
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
      FORWARD_WM_KEYUP(hwnd, vk, cRepeat, flags, ColNameDefWndProc);
      return;
   }
   switch(vk)
   {
      case VK_RETURN:
      {
         AddContainerColumn(hdlg);
         break;
      }
      default:
         FORWARD_WM_KEYDOWN(hwnd, vk, cRepeat, flags, ColNameDefWndProc);
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

   Edit_GetText(GetDlgItem(hwnd, IDC_COLNAME), szColumn, sizeof(szColumn));
   suppspace(szColumn);
   if (!szColumn[0])
      return;
   if (CntFindField(hwndCnt, szColumn))
   {
      // Field already exists in container.
      char szError[MAXOBJECTNAME*4+256];
      char szTitle[BUFSIZE];
      HWND currentFocus = GetFocus();

      LoadString(hResource, IDS_COLUMNEXISTS, szError, sizeof(szError));
//      LoadString(hResource, IDS_PRODUCT, szTitle, sizeof(szTitle));
      VDBA_GetProductCaption(szTitle, sizeof(szTitle));
      MessageBox(NULL, szError, szTitle, MB_OK | MB_ICONEXCLAMATION | MB_TASKMODAL);

      SetFocus(GetDlgItem(hwnd, IDC_COLNAME));
      Edit_SetSel(GetDlgItem(hwnd, IDC_COLNAME), 0, -1);

      return;
   }
   // Ensure the object name is valid
   if (!VerifyObjectName(hwnd, szColumn, TRUE, TRUE, TRUE))
      return;
   CntDeferPaint(hwndCnt);
   // Add the column to the container
   uFieldCounter++;
   // Allocate enough memory for this fields data. Since each
   // field is given a specific offset into the allocated
   // memory (see wOffStruct), the memory for the new field
   // is always tagged onto the end of the current block,
   // regardless of any deleted fields.
   uNewDataSize = uFieldCounter * sizeof(CNTDATA);
   if (uNewDataSize > uDataSize)
      ReAllocContainerRecords(uNewDataSize);
   // Copy the record data into our buffer
   CopyContainerRecords(hwndCnt);
   // Initialise the data for this field.
   InitialiseFieldData(uFieldCounter - 1);
   // Initialise the field descriptor.
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
   // Add the field
   CntAddFldTail(hwndCnt, lpFI);
   // Add the new records to the container
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
   {
      // Display the type control
      LPRECORDCORE lpRC;
      LPFIELDINFO lpFirstField;
      RECT rect;
      lpRC = GetDataTypeRecord(hwndCnt, DT_TYPE);
      // set focus on first of the list,
      // then on newly created one (scroll problem solve)
      lpFirstField = CntFldHeadGet (hwndCnt);
      CntFocusSet(hwndCnt, lpRC, lpFirstField);
      CntFocusSet(hwndCnt, lpRC, lpFI);
      GetFocusCellRect(hwndCnt, &rect);
      ShowTypeWindow(hwndCnt, &rect);
      // Set data with curent selection in combobox
      SetDataTypeWithCurSelCombobox();
      InitializeDataLenWithDefault(hwndCnt,lpFI);
   }
   // Initilize with default value for Nullable,Default and "Def Spec".
   InitializeFieldDataWithDefault(hwndCnt);

   CntEndDeferPaint(hwndCnt, TRUE);
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
   uRet = (UINT)FORWARD_WM_GETDLGCODE(hwnd, lpmsg, ColNameDefWndProc);
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
   int recnum = ConvertDataTypeIntoRecnum(nDataType);

   lpRC = CntRecHeadGet(hwndCnt);

   for (nOffset = 0; nOffset < recnum; nOffset++)
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


static int GetDataTypeStringId(int nId)
/*
   Function:
      Retrieves the resource string id for the given data type.

   Parameters:
      nId      - Data type

   Returns:
      Resource string id for given data type, -1 if data type is invalid.
*/
{
   int nOffset = 0;
   CTLDATA *lpDataType;

   // Use Explicit logic to select the Ingres Release

   if (GetOIVers() >= OIVERS_91)         // II 9.1.0 or higher
      lpDataType = typeTypesAnsiDate;
   else if (GetOIVers() == OIVERS_90)    // II 9.0.x 
          lpDataType = typeTypesBigInt;
   else if (GetOIVers() == OIVERS_30)    // II 3.0
          lpDataType = typeTypesBigInt;
   else if (GetOIVers() == OIVERS_26)    // II 2.6
          lpDataType = typeTypesUnicode;
   else lpDataType = typeTypes;          // Releases prior to II 2.5


   while (lpDataType[nOffset].nId != -1)
   {
      if (lpDataType[nOffset].nId == nId)
         return lpDataType[nOffset].nStringId;
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
          my_itoa(lpdata->len.decInfo.nPrecision, lpszNumber, 10);
          lstrcat(lpszNumber, ",");
          my_itoa(lpdata->len.decInfo.nScale, lpszNumber + lstrlen(lpszNumber), 10);
      }
      else
          *lpszNumber = (char)NULL;
   }
   else
      my_dwtoa(lpdata->len.dwDataLen, lpszNumber, 10);
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
         lpdata->len.decInfo.nPrecision = my_atoi(lpszNumber);
         lpdata->len.decInfo.nScale = my_atoi(lpszComma + 1);
      }
   }
   else
   {
      if( *lpszNumber == (char)NULL && uType == INGTYPE_FLOAT )
         lpdata->len.dwDataLen = DEFAULT_FLOAT;
      else
         lpdata->len.dwDataLen = my_atodw(lpszNumber);
   }
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
   GetFocusCellRect(hwndCnt, lprect);
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
      case DT_DEFSPEC:
      {
         lpszString = data.lpszDefSpec;
         break;
      }

//       case DT_CHECK:
//       {
//          lpszString = data.lpszCheck;
//          break;
//       }
      case DT_TITLE:
      {
         lpszString = lpEditFI->lpFTitle;
         break;
      }
/*
      case DT_PRIMARY:
        return;
*/
      default:
         ASSERT(NULL);  
         return;      // don't set text
   }
   Edit_SetText(hwndCntEdit, lpszString);
}




static BOOL FindReferencesColumnName( HWND hwnd, LPSTR lpszCol, LPOBJECTLIST FAR * FAR * lplplpRefList, LPOBJECTLIST FAR * lplpElement)
/*
   Function:
      Determines if the given column name exists in the referential
      column name list.

   Parameters:
      lpszCol     - Column name to search for.
      lplpRefList - Address for return of column list
      lplpElement - Address for return of element in column list

   Returns:
      TRUE if an element was found
*/
{
   LPTABLEPARAMS lptbl = GetDlgProp(hwnd);
   LPOBJECTLIST lplist;

   if (lplplpRefList)
      *lplplpRefList = NULL;
   if (lplpElement)
      *lplpElement = NULL;
   if (!lptbl || !lptbl->lpReferences)
   {
      return FALSE;
   }
   lplist = lptbl->lpReferences;
   while (lplist)
   {
      LPREFERENCEPARAMS lpObj = (LPREFERENCEPARAMS)lplist->lpObject;
      LPOBJECTLIST lplistCols = lpObj->lpColumns;

      while (lplistCols)
      {
         LPREFERENCECOLS lpCols = (LPREFERENCECOLS)lplistCols->lpObject;

         if (lstrcmpi(lpCols->szRefingColumn, lpszCol) == 0)
         {
            if (lplplpRefList)
               *lplplpRefList = &lpObj->lpColumns;
            if (lplpElement)
               *lplpElement = lplistCols;
            return TRUE;
         }

         if (*lpObj->szTable == (char)NULL)
         {
            // If we are referencing the same table, check the referenced
            // columns as well.
            if (lstrcmpi(lpCols->szRefedColumn, lpszCol) == 0)
            {
               if (lplplpRefList)
                  *lplplpRefList = &lpObj->lpColumns;
               if (lplpElement)
                  *lplpElement = lplistCols;
               return TRUE;
            }
         }
         lplistCols = lplistCols->lpNext;
      }
      lplist = lplist->lpNext;
   }
   return FALSE;   
}


static void EnableOKButton(HWND hwnd)
{
    BOOL bEnable = FALSE;
    HWND hwndCnt = GetDlgItem(hwnd, IDC_CONTAINER);
    LPCNTINFO lpcntInfo = CntInfoGet(hwndCnt);
    
    BOOL b = Button_GetCheck (GetDlgItem (hwnd, IDC_INGCHECKASSELECT));
    if (b)
    {
        if (Edit_GetTextLength(GetDlgItem(hwnd, IDC_TABLE)) > 0 && Edit_GetTextLength(GetDlgItem(hwnd, IDC_INGEDITSQL)) > 0)
            EnableWindow(GetDlgItem(hwnd, IDOK), TRUE);
        else
            EnableWindow(GetDlgItem(hwnd, IDOK), FALSE);
        return;
    }
    // We must have a table name and some columns in the table for the OK
    // button to be enabled.
    if ((Edit_GetTextLength(GetDlgItem(hwnd, IDC_TABLE)) > 0) && lpcntInfo->nFieldNbr > 0)
        bEnable = TRUE;
    EnableWindow(GetDlgItem(hwnd, IDOK), bEnable);
}


static void CreateDlgTitle(HWND hwnd)
{
    char szTitle[MAXOBJECTNAME*4+256];
    
    LPTABLEPARAMS lptbl = GetDlgProp(hwnd);
    LPSTR lpszNode = GetVirtNodeName(GetCurMdiNodeHandle());
    GetWindowText(hwnd, szTitle, sizeof(szTitle));
    lstrcat(szTitle, lpszNode);
    lstrcat(szTitle, "::");
    lstrcat(szTitle, lptbl->DBName);
    SetWindowText(hwnd, szTitle);
}


static LPOBJECTLIST DropEmptyReferences(LPOBJECTLIST lplistRef)
/*
   Function:
      Looks through the references list and drops any references that
      no longer reference any columns.  This situation is usually the
      result of a column being deleted from the table definition that is
      also used as a reference column.

   Parameters:
      lplist      - Address of list of REFERENCEPARAMS

   Returns:
      None.
*/
{
   LPOBJECTLIST lplist = lplistRef;

   while (lplist)
   {
      LPREFERENCEPARAMS lpRef = (LPREFERENCEPARAMS)lplist->lpObject;
      LPOBJECTLIST lplistCols = lpRef->lpColumns;
      LPOBJECTLIST lpTemp;
      BOOL bRefing = FALSE;

      while (lplistCols)
      {
         LPREFERENCECOLS lpCols = (LPREFERENCECOLS)lplistCols->lpObject;
         if (*lpCols->szRefedColumn)
         {
            bRefing = TRUE;
            break;
         }
         lplistCols = lplistCols->lpNext;
      }
      lpTemp = lplist->lpNext;
      if (!bRefing)
      {
         FreeObjectList(lpRef->lpColumns);
         lpRef->lpColumns = NULL;
         lplistRef = DeleteListObject(lplistRef, lplist);
      }
      lplist = lpTemp;
   }
   return lplistRef;
}


static LPOBJECTLIST DropDuplicateReferences(LPOBJECTLIST lplistRef)
{
   LPOBJECTLIST lplist = lplistRef;

   while (lplist)
   {
      LPREFERENCEPARAMS lpRef = (LPREFERENCEPARAMS)lplist->lpObject;
//      LPOBJECTLIST lpDupObj;
      char cTemp;
      REFERENCEPARAMS refParams;

      ZEROINIT(refParams);
      refParams = *lpRef;
      // Temporarily set the table name in the list with an invalid
      // character so it will not be found in the list comparison.
      cTemp = *lpRef->szTable;
      *lpRef->szTable = '!';
/*        
      if (lpDupObj = refcmp(lplistRef, &refParams))
      {
         // Delete the duplicate object.

         LPREFERENCEPARAMS lpRefDel = (LPREFERENCEPARAMS)lpDupObj->lpObject;

         FreeObjectList(lpRefDel->lpColumns);
         lpRefDel->lpColumns = NULL;
         lplistRef = DeleteListObject(lplistRef, lpDupObj);
      }
      else
         *lpRef->szTable = cTemp;
*/
      lplist = lplist->lpNext;
   }
   return lplistRef;
}


static int GetLastPrimaryKey(HWND hwndCnt)
{
  return 0;
}


static void VerifyPrimaryKeySequence(HWND hwndCnt, LPFIELDINFO lpFIKey)
/*
   Function:
      Looks through the primary keys and ensures they are sequential
      and there are no duplicates.

   Parameters:
      hwndCnt     - Handle to the container window.
      lpFIKey     - Address of last field to have the primary key modified.
                    This is used to resolve the precedence when duplicate
                    keys are found and is refered to as the 'fixed' key.

   Returns:
      None.
*/
{
  return;
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
                case INGTYPE_DECIMAL:
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


static void OnAsSelectClick (HWND hwnd)
{
    LPTABLEPARAMS lptbl = GetDlgProp(hwnd);

    BOOL b = Button_GetCheck (GetDlgItem (hwnd, IDC_INGCHECKASSELECT));
    if (b)
    {
        ShowWindow (GetDlgItem (hwnd, IDC_STATIC_INGCOLUMNS),       SW_HIDE);
        ShowWindow (GetDlgItem (hwnd, IDC_STATIC_INGNAME),          SW_HIDE);
        ShowWindow (GetDlgItem (hwnd, IDC_COLNAME),                 SW_HIDE);
        ShowWindow (GetDlgItem (hwnd, IDC_ADD),                     SW_HIDE);
        ShowWindow (GetDlgItem (hwnd, IDC_DROP),                    SW_HIDE);
        ShowWindow (GetDlgItem (hwnd, IDC_CONTAINER),               SW_HIDE);
        ShowWindow (GetDlgItem (hwnd, IDC_STRUCTURE),               SW_SHOW);
        ShowWindow (GetDlgItem (hwnd, IDC_REFERENCES),              SW_HIDE);
        ShowWindow (GetDlgItem (hwnd, IDC_TLCBUTTON_UNIQUE),        SW_HIDE);
        ShowWindow (GetDlgItem (hwnd, IDC_TLCBUTTON_CHECK),         SW_HIDE);
        ShowWindow (GetDlgItem (hwnd, IDC_CONSTRAINT_INDEX),        SW_HIDE);

        ShowWindow (GetDlgItem (hwnd, IDC_STATIC_INGCOLUMNHEADER),  SW_SHOW);
        ShowWindow (GetDlgItem (hwnd, IDC_INGEDITCOLUMNHEADER),     SW_SHOW);
        ShowWindow (GetDlgItem (hwnd, IDC_STATIC_INGSELECT),        SW_SHOW);
        ShowWindow (GetDlgItem (hwnd, IDC_INGEDITSQL),              SW_SHOW);
        ShowWindow (GetDlgItem (hwnd, IDC_BUTTON_INGASSISTANT),     SW_SHOW);

        if (!lptbl->bDDB) {
          if (bPassedByExtraButton)
                    //"If you have specified options in the \"Options\", \"References\", \"Unique\" or \"Check\" sub-dialogs, "
                    //"they will not be taken into account if you use the \"Create Table As Select\" option"
                    MessageBox (NULL, ResourceString(IDS_ERR_CREATE_TBL_AS_SELECT),
                                      ResourceString(IDS_TITLE_WARNING), MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL);
        }
        if (lstrlen(lptbl->szSecAuditKey)) {
            int iRet;
            iRet = MessageBox (NULL, ResourceString(IDS_W_CHOICE_SECURITY_AUDIT),
                                     ResourceString(IDS_TITLE_WARNING), MB_ICONEXCLAMATION | MB_OKCANCEL | MB_TASKMODAL);
            if (iRet == IDOK)
                memset (lptbl->szSecAuditKey,0,sizeof(lptbl->szSecAuditKey));
        }
    }
    else
    {
        ShowWindow (GetDlgItem (hwnd, IDC_STATIC_INGCOLUMNS),       SW_SHOW);
        ShowWindow (GetDlgItem (hwnd, IDC_STATIC_INGNAME),          SW_SHOW);
        ShowWindow (GetDlgItem (hwnd, IDC_COLNAME),                 SW_SHOW);
        ShowWindow (GetDlgItem (hwnd, IDC_ADD),                     SW_SHOW);
        ShowWindow (GetDlgItem (hwnd, IDC_DROP),                    SW_SHOW);
        ShowWindow (GetDlgItem (hwnd, IDC_CONTAINER),               SW_SHOW);
        ShowWindow (GetDlgItem (hwnd, IDC_STRUCTURE),               SW_SHOW);
        ShowWindow (GetDlgItem (hwnd, IDC_REFERENCES),              SW_SHOW);
        ShowWindow (GetDlgItem (hwnd, IDC_TLCBUTTON_UNIQUE),        SW_SHOW);
        ShowWindow (GetDlgItem (hwnd, IDC_TLCBUTTON_CHECK),         SW_SHOW);
        ShowWindow (GetDlgItem (hwnd, IDC_CONSTRAINT_INDEX),        SW_SHOW);

        ShowWindow (GetDlgItem (hwnd, IDC_STATIC_INGCOLUMNHEADER),  SW_HIDE);
        ShowWindow (GetDlgItem (hwnd, IDC_INGEDITCOLUMNHEADER),     SW_HIDE);
        ShowWindow (GetDlgItem (hwnd, IDC_STATIC_INGSELECT),        SW_HIDE);
        ShowWindow (GetDlgItem (hwnd, IDC_INGEDITSQL),              SW_HIDE);
        ShowWindow (GetDlgItem (hwnd, IDC_BUTTON_INGASSISTANT),     SW_HIDE);
    }

    // Star management
    if (IsDDB()) {
      // always visible buttons:
      ShowWindow(GetDlgItem(hwnd, IDC_STRUCTURE), SW_SHOW);
      ShowWindow(GetDlgItem(hwnd, IDC_BUTTON_STAR_LOCALTABLE), SW_SHOW);

      // always invisible buttons
      ShowWindow(GetDlgItem(hwnd, IDC_REFERENCES), SW_HIDE);
      ShowWindow(GetDlgItem(hwnd, IDC_TLCBUTTON_UNIQUE), SW_HIDE);
      ShowWindow(GetDlgItem(hwnd, IDC_TLCBUTTON_CHECK), SW_HIDE);
    }
    // Gateway management
    if (bGenericGateway) {
      // always invisible buttons
      ShowWindow(GetDlgItem(hwnd, IDC_BUTTON_STAR_LOCALTABLE), SW_HIDE);
      ShowWindow(GetDlgItem(hwnd, IDC_STRUCTURE), SW_HIDE);
      ShowWindow(GetDlgItem(hwnd, IDC_REFERENCES), SW_HIDE);
      ShowWindow(GetDlgItem(hwnd, IDC_TLCBUTTON_UNIQUE), SW_HIDE);
      ShowWindow(GetDlgItem(hwnd, IDC_TLCBUTTON_CHECK), SW_HIDE);
    }

    EnableOKButton(hwnd);
}

static BOOL OnAssistant (HWND hwnd)
{
	LPTABLEPARAMS lptbl = (LPTABLEPARAMS)GetDlgProp(hwnd);
	LPTSTR lpszSQL = NULL;
	LPUCHAR lpVirtNodeName = GetVirtNodeName(GetCurMdiNodeHandle());

	lpszSQL = CreateSelectStmWizard(hwnd, lpVirtNodeName, lptbl->DBName);
	if (lpszSQL)  
	{
		Edit_ReplaceSel(GetDlgItem(hwnd, IDC_INGEDITSQL), lpszSQL);
		free(lpszSQL);
		return TRUE;
	}

	return FALSE;
}

static void OnUniqueConstraint (HWND hwnd)
{
    TABLEPARAMS ts;
    LPTABLEPARAMS lptbl = GetDlgProp (hwnd);
    bPassedByExtraButton=TRUE;

    memset (&ts, 0, sizeof (ts));
    lstrcpy (ts.DBName,     lptbl->DBName);
    lstrcpy (ts.objectname, lptbl->objectname);
    ts.bCreate      = lptbl->bCreate;
    ts.uPage_size   = (lptbl->uPage_size == 0L)? 2048L: lptbl->uPage_size;
    ts.lpUnique     = VDBA20xTableUniqueKey_CopyList (lptbl->lpUnique);
    ts.lpReferences = lptbl->lpReferences;
    ts.primaryKey   = lptbl->primaryKey;
    ts.bCreateVectorWise = lptbl->bCreateVectorWise;
    MakeColumnsList   (hwnd, &ts);
    if (CTUnique(hwnd, &ts) == IDOK)
    {
        CntUpdateUnique (hwnd, lptbl->lpUnique, ts.lpUnique);
        lptbl->lpUnique = VDBA20xTableUniqueKey_Done (lptbl->lpUnique);
        lptbl->lpUnique = ts.lpUnique;
        ts.lpUnique     = NULL;
    }
    ts.lpUnique = VDBA20xTableUniqueKey_Done (ts.lpUnique);
    ts.lpReferences = NULL;
    ts.lpColumns= VDBA20xColumnList_Done (ts.lpColumns);
}


static void OnCheckConstraint (HWND hwnd)
{
    TABLEPARAMS ts;
    LPTABLEPARAMS lptbl = GetDlgProp (hwnd);
    bPassedByExtraButton=TRUE;

    memset (&ts, 0, sizeof (ts));
    lstrcpy (ts.DBName,     lptbl->DBName);
    lstrcpy (ts.objectname, lptbl->objectname);
    ts.bCreate      = lptbl->bCreate;
    ts.uPage_size   = (lptbl->uPage_size == 0L)? 2048L: lptbl->uPage_size;
    ts.lpCheck      = VDBA20xTableCheck_CopyList (lptbl->lpCheck);
    MakeColumnsList   (hwnd, &ts);
    if (CTCheck (hwnd, &ts) == IDOK)
    {
        lptbl->lpCheck = VDBA20xTableCheck_Done (lptbl->lpCheck);
        lptbl->lpCheck = ts.lpCheck;
        ts.lpCheck     = NULL;
    }
    ts.lpCheck = VDBA20xTableCheck_Done (ts.lpCheck);
    ts.lpColumns= VDBA20xColumnList_Done (ts.lpColumns);
}

static void OnCreateVectorWiseTable(HWND hwnd)
{
    LPTABLEPARAMS lptbl = (LPTABLEPARAMS)GetDlgProp (hwnd);

    if (Button_GetCheck(GetDlgItem(hwnd, IDC_INGCHECKVW)))
    {
      EnableWindow(GetDlgItem(hwnd, IDC_STRUCTURE), FALSE);
      EnableWindow(GetDlgItem(hwnd, IDC_TLCBUTTON_CHECK), FALSE);
      lptbl->bCreateVectorWise=TRUE;
    }
    else
    {
      EnableWindow(GetDlgItem(hwnd, IDC_STRUCTURE), TRUE);
      EnableWindow(GetDlgItem(hwnd, IDC_TLCBUTTON_CHECK), TRUE);
      lptbl->bCreateVectorWise=FALSE;
    }
}

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
        MessageBox (NULL, Msg, ResourceString(IDS_TITLE_WARNING), MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL);
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
            MessageBox (hwnd, ResourceString ((UINT)IDS_E_UNIQUE_MUSTBE_NOTNUL), ResourceString ((UINT)IDS_TITLE_WARNING), MB_ICONEXCLAMATION|MB_OK);
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
                TCHAR msg [MAXOBJECTNAME*4+256];//"Duplicate Keys :\nThe primary key and unique key (%s)"
                wsprintf (msg, ResourceString(IDS_F_DUPLICATE_KEY), lpCol->szColumn);
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
                TCHAR msg [MAXOBJECTNAME*4+256];//"Duplicate Keys :\nThe primary key and unique key <%s>,"
                wsprintf (msg, ResourceString(IDS_F_DUPLICATE_KEY), lx->tchszConstraint);
                MessageBox (NULL, msg, NULL, MB_ICONSTOP|MB_OK);
                return FALSE;
            }
        }
        lx = lx->next;
    }
    return TRUE;
}



static void OnOK (HWND hwnd)
{
    int   ires;
    BOOL  Success;
    TCHAR szTable     [MAXOBJECTNAME]; /* MAXOBJECTNAME includes the trailing \0 */

    LPTABLEPARAMS lptbl = GetDlgProp (hwnd);
    HWND hwndTable      = GetDlgItem (hwnd, IDC_TABLE);
    HWND hwndCnt        = GetDlgItem (hwnd, IDC_CONTAINER);
    HWND hwndHeader     = GetDlgItem (hwnd, IDC_INGEDITCOLUMNHEADER);
    HWND hwndSQLText    = GetDlgItem (hwnd, IDC_INGEDITSQL);

    Edit_GetText (hwndTable, szTable, sizeof(szTable));

    //
    // Create Table as VectorWise
    lptbl->bCreateVectorWise = Button_GetCheck(GetDlgItem(hwnd, IDC_INGCHECKVW));
    //erase all the data under options and check dialogs as it isn't supported
    if (lptbl->bCreateVectorWise)
    {
	lptbl->lpCheck = NULL;
    }

    //
    // Create Table as Select ...
    lptbl->bCreateAsSelect = Button_GetCheck (GetDlgItem(hwnd, IDC_INGCHECKASSELECT));
    if (lptbl->lpSelectStatement)
    {
        ESL_FreeMem (lptbl->lpSelectStatement);
        lptbl->lpSelectStatement = NULL;
    }
    if (lptbl->lpColumnHeader)
    {
        ESL_FreeMem (lptbl->lpColumnHeader);
        lptbl->lpColumnHeader = NULL;
    }
    if (lptbl->bCreateAsSelect)
    {
        UINT nHeader = GetWindowTextLength (hwndHeader);
        UINT nSql    = GetWindowTextLength (hwndSQLText);
        if (nHeader > 0)
        {
            lptbl->lpColumnHeader = (LPSTR)ESL_AllocMem (nHeader +2);
            if (!lptbl->lpColumnHeader)
                return;
            GetWindowText (hwndHeader, lptbl->lpColumnHeader, nHeader+1);
        }
        if (nSql > 0)
        {
            lptbl->lpSelectStatement = (LPSTR)ESL_AllocMem (nSql +2);
            if (!lptbl->lpSelectStatement)
                return;
            GetWindowText (hwndSQLText, lptbl->lpSelectStatement, nSql+1);
        }
    }
    if (!VerifyObjectName(hwnd, szTable, TRUE, FALSE, TRUE))
    {
        Edit_SetSel (hwndTable, 0, -1);
        SetFocus (hwndTable);
        return;
    }
    lstrcpy(lptbl->objectname, szTable);
    lstrcpy(lptbl->szSchema, "");
    MakeColumnsList (hwnd, lptbl);
    if (lptbl->uPage_size == 2048L)
        lptbl->uPage_size = 0L;
    //
    // Data Validation
    Success = VDBA20xTableDataValidate (hwnd, lptbl);
    if (!Success)
    {
        lptbl->lpColumns = VDBA20xColumnList_Done (lptbl->lpColumns);
        lptbl->lpColumns = NULL;
        return;
    }
    if (!lptbl->bCreate)
    {
        EndDialog (hwnd, FALSE);
        return;
    }

    ires = DBAAddObject (GetVirtNodeName (GetCurMdiNodeHandle ()), OT_TABLE,(void *)lptbl);
    if (ires != RES_SUCCESS)
    {
        ErrorMessage ((UINT) IDS_E_CREATE_TABLE_FAILED, ires);
        lptbl->lpColumns = VDBA20xColumnList_Done (lptbl->lpColumns);
        SetFocus  (hwnd);
        return;
    }
    EndDialog (hwnd, TRUE);
}

static void OnColumnEditChange (HWND hwnd)
{
    HWND hwndCnt = GetDlgItem(hwnd, IDC_CONTAINER);
    LPOBJECTLIST lpColList = NULL;
    LPFIELDINFO  lpFld;
    TCHAR tchszColumn [MAXOBJECTNAME -1];
    //
    // Disable ADD & Delete button if no column name
    if (Edit_GetTextLength(GetDlgItem (hwnd, IDC_COLNAME)) == 0)
    {
        EnableWindow (GetDlgItem(hwnd, IDC_ADD),  FALSE);
        EnableWindow (GetDlgItem(hwnd, IDC_DROP), FALSE);
    }
    else
        EnableWindow (GetDlgItem(hwnd, IDC_ADD), TRUE);
    
    Edit_GetText (GetDlgItem (hwnd, IDC_COLNAME), tchszColumn, sizeof (tchszColumn));
    lpFld = CntFldTailGet (hwndCnt);
    while (lpFld)
    {
        if (lstrcmpi (tchszColumn, lpFld->lpFTitle) == 0)
        {
            EnableWindow (GetDlgItem(hwnd, IDC_DROP), TRUE);
            return;
        }
        lpFld = CntPrevFld (lpFld);
   }
}

static void OnTableEditChange  (HWND hwnd)
{
    EnableOKButton (hwnd);
}

static void OnAddColumn   (HWND hwnd)
{
    AddContainerColumn (hwnd);
    EnableOKButton (hwnd);
}

static void OnDropColumn  (HWND hwnd)
{
    //     
    // Remove the field from the container
    TCHAR szColumn [MAXOBJECTNAME-1];
    LPTABLEPARAMS lptbl = (LPTABLEPARAMS)GetDlgProp (hwnd);
    HWND  hwndCnt       = GetDlgItem(hwnd, IDC_CONTAINER);
    LPFIELDINFO lpField = CntFldHeadGet (hwndCnt);
    BOOL bEnabled = FALSE;
    LPCNTINFO lpcntInfo;
    
    Edit_GetText (GetDlgItem(hwnd, IDC_COLNAME), szColumn, sizeof(szColumn));
    //
    // Check the Constraints (Uniques, References).  If they use the column to be dropped
    // then remove these constraints.
    if (FindReferencesColumnName(hwnd, szColumn, NULL, NULL))
    {
        // The column name selected for dropping exists in the referential
        // integrity column list.
        LPOBJECTLIST FAR * lplpColList;
        LPOBJECTLIST lpElement;
        while (FindReferencesColumnName(hwnd, szColumn, &lplpColList, &lpElement))
        {
            //
            // Remove the column from the references list and continue with the drop.
            *lplpColList = DeleteListObject(*lplpColList, lpElement);
        }
    }
    if (lstrcmpi(szColumn, lptbl->szSecAuditKey))
        lptbl->szSecAuditKey [0] = '\0';
    lptbl->lpReferences = DropEmptyReferences(lptbl->lpReferences);
    lptbl->lpReferences = DropDuplicateReferences(lptbl->lpReferences);
    CntDeferPaint(hwndCnt);
    while (lpField)
    {
        if (!lstrcmpi(lpField->lpFTitle, szColumn))
        {
            // Work through each record releasing any memory
            // allocated to store the records data.
            ReleaseField(hwndCnt, lpField);
            CntRemoveFld(hwndCnt, lpField);
            break;
        }
        lpField = CntNextFld(lpField);
    }
    VerifyPrimaryKeySequence(hwndCnt, NULL);
    lpcntInfo = CntInfoGet(hwndCnt);
    if (lpcntInfo->nFieldNbr < MAXIMUM_COLUMNS)
        bEnabled = TRUE;
    EnableWindow(GetDlgItem(hwnd, IDC_ADD), bEnabled);
    bEnabled = FALSE;
    if (lpcntInfo->nFieldNbr > 0)
        bEnabled = TRUE;
    EnableWindow(GetDlgItem(hwnd, IDC_REFERENCES), bEnabled);
    EnableOKButton(hwnd);
    CntEndDeferPaint(hwndCnt, TRUE);
}

static void OnCntCharHit  (HWND hwnd)
{
    HWND hwndCtl        = GetDlgItem (hwnd, IDC_CONTAINER); 
    LPRECORDCORE lpRC   = CntFocusRecGet(hwndCtl);
    LPRECORDCORE lpRec  = NULL;
    LPRECORDCORE lpRecType  = NULL;
    LPFIELDINFO  lpFI   = CntFocusFldGet(hwndCtl);
    RECT    rect;
    UINT    uChar;
    LPARAM  lParam;
    CNTDATA data, dataAttr;
    int     nType;
    if (!lpFI)
        return;

#ifdef WIN32
    lParam = (LPARAM)hwndCtl;
#else
    lParam = MAKELPARAM(hwndCtl, CN_CHARHIT);   // codeNotify = CN_CHARHIT
#endif
  
    uChar = CntCNCharGet(hwndCtl, lParam);
    // Get the data for this cell
    CntFldDataGet(lpRC, lpFI, sizeof(data), &data);
    GetFocusCellRect(hwndCtl, &rect);
    nType = GetRecordDataType(hwndCtl, lpRC);
    switch (nType)
    {
    case DT_TYPE:
        EnableOKButton(hwnd);
        break;
    case DT_DEFSPEC:
        {
        CNTDATA dataDef;
        lpRecType = GetDataTypeRecord(hwndCtl, DT_TYPE);
        CntFldDataGet(lpRecType, lpFI, sizeof(dataAttr), &dataAttr);
        if (dataAttr.uDataType == INGTYPE_LONGVARCHAR ||
            dataAttr.uDataType == INGTYPE_LONGBYTE    ||
            dataAttr.uDataType == INGTYPE_UNICODE_LNVCHR)
          break;
        //
        // Only allow edit if bDefault is checked.
        lpRec = GetDataTypeRecord(hwndCtl, DT_DEFAULT);
        CntFldDataGet(lpRec, lpFI, sizeof(dataDef), &dataDef);
        if (!dataDef.bDefault)
            break;
        // GPF corrected Emb 9/8/95 : swap parameters of MAKELONG
        ShowEditWindow(hwndCtl, &rect, nType, (LPCNTDATA)MAKELONG(uChar,0));
        }
        break;
    case DT_LEN:
        {
        CNTDATA dataType;
        //
        // Only allow edit if data type requires it
        lpRec = GetDataTypeRecord(hwndCtl, DT_TYPE);
        CntFldDataGet(lpRec, lpFI, sizeof(dataType), &dataType);
        if (GetMaxReqLength(dataType.uDataType) != -1)
        {
            // GPF corrected Emb 9/8/95 : swap parameters of MAKELONG
            ShowEditWindow(hwndCtl, &rect, nType, (LPCNTDATA)MAKELONG(uChar, 0));
            EnableOKButton(hwnd);
        }
        }
        break;
    case DT_NULLABLE:
        {
        if (uChar != VK_SPACE)
            break;
        data.bNullable = !data.bNullable;
        CntFldDataSet(lpRC, lpFI, sizeof(data), &data);
        CntUpdateRecArea(hwndCtl, lpRC, lpFI);
        if (!data.bNullable)
            VerifyDefaultAndSpecDef(hwndCtl,lpFI);
        }
        break;

    case DT_UNIQUE:
        {
        data.bUnique = !data.bUnique;
        CntFldDataSet(lpRC, lpFI, sizeof(data), &data);
        CntUpdateRecArea(hwndCtl, lpRC, lpFI);
        }
        break;
     
    case DT_DEFAULT:
        if (uChar == VK_SPACE)
        {
            CNTDATA dataAttr;
            lpRecType = GetDataTypeRecord(hwndCtl, DT_TYPE);
            CntFldDataGet(lpRecType, lpFI, sizeof(dataAttr), &dataAttr);
            if (dataAttr.uDataType == INGTYPE_LONGVARCHAR ||
                dataAttr.uDataType == INGTYPE_LONGBYTE    ||
                dataAttr.uDataType == INGTYPE_UNICODE_LNVCHR)
              break;

            data.bDefault = !data.bDefault;
            CntFldDataSet(lpRC, lpFI, sizeof(data), &data);
            CntUpdateRecArea(hwndCtl, lpRC, lpFI);
        }
        if (!IsDDB()) {
          lpRec = GetDataTypeRecord(hwndCtl, DT_DEFSPEC);
          CntUpdateRecArea(hwndCtl, lpRec, lpFI);
        }
        break;
    case DT_SYSMAINTAINED:
        lpRecType = GetDataTypeRecord(hwndCtl, DT_TYPE);
        CntFldDataGet(lpRecType, lpFI, sizeof(dataAttr), &dataAttr);
        if (dataAttr.uDataType != INGTYPE_OBJKEY  ||
            dataAttr.uDataType != INGTYPE_TABLEKEY)
              break;
        data.bSystemMaintained = !data.bSystemMaintained;
        CntFldDataSet(lpRC, lpFI, sizeof(data), &data);
        CntUpdateRecArea(hwndCtl, lpRC, lpFI);
        break;
    default:
          ASSERT(NULL);
          break;
    }
}

static void CleanUp(LPTABLEPARAMS lpTbl , LPTSTR lpColHead,LPTSTR lpSelStat,LPTSTR lpStr)
{
    lstrcpy(lpTbl->szSchema, "");
    lstrcpy(lpTbl->szNodeName,"");
    if (lpColHead)
        ESL_FreeMem (lpColHead);
    if (lpSelStat)
        ESL_FreeMem (lpSelStat);
    if(lpStr)
        ESL_FreeMem (lpStr);
}


static void SimulateColumnList(HWND hwnd, LPTABLEPARAMS lptbl)
{
    HWND hwndTable      = GetDlgItem (hwnd, IDC_TABLE);
    HWND hwndHeader     = GetDlgItem (hwnd, IDC_INGEDITCOLUMNHEADER);
    HWND hwndSQLText    = GetDlgItem (hwnd, IDC_INGEDITSQL);
    LPTSTR lpString, lpColHeader, lpSelStatement,lpCurSelStat;
    TCHAR tchszConnection [(MAXOBJECTNAME*2)+2];
    UINT nHeader, nSql;
    int ires,hdl;

    lpString       = NULL;
    lpColHeader    = NULL;
    lpSelStatement = NULL;

    nHeader = GetWindowTextLength (hwndHeader);
    nSql    = GetWindowTextLength (hwndSQLText);

    Edit_GetText (hwndTable, lptbl->objectname, sizeof(lptbl->objectname));

    DBAGetUserName(GetVirtNodeName(GetCurMdiNodeHandle()),lptbl->szSchema);
    lstrcpy(lptbl->szNodeName,GetVirtNodeName(GetCurMdiNodeHandle()));

    if (nHeader > 0)
    {
        lpColHeader = (LPSTR)ESL_AllocMem (nHeader + 1);
        if (!lpColHeader) {
            ErrorMessage ((UINT) IDS_E_CANNOT_ALLOCATE_MEMORY, RES_ERR);
            CleanUp(lptbl , lpColHeader, lpSelStatement, lpString);
            return;
        }
        GetWindowText (hwndHeader, lpColHeader, nHeader + 1);
    }

    if (nSql > 0)
    {
        lpSelStatement = (LPSTR)ESL_AllocMem (nSql + 1);
        if (!lpSelStatement) {
            ErrorMessage ((UINT) IDS_E_CANNOT_ALLOCATE_MEMORY, RES_ERR);
            CleanUp(lptbl, lpColHeader, lpSelStatement, lpString);
            return;
        }
        GetWindowText (hwndSQLText, lpSelStatement, nSql + 1);
    }
    else
    {
        CleanUp(lptbl, lpColHeader, lpSelStatement, lpString);
        return; // nothing to do the SQL statement is empty
    }

    lpCurSelStat = lpSelStatement;

    if (!AddDummyWhereClause(&lpString, lpCurSelStat, lptbl->objectname, lpColHeader))
    {
        MessageBox (hwnd, ResourceString ((UINT)IDS_E_TABLE_EVALUATE_COLUMNS),
                          ResourceString ((UINT)IDS_TITLE_WARNING), MB_ICONEXCLAMATION|MB_OK);
        CleanUp(lptbl, lpColHeader, lpSelStatement, lpString);
        return;
    }

    wsprintf (tchszConnection,"%s::%s", lptbl->szNodeName, lptbl->DBName);
    ires = Getsession (tchszConnection, SESSION_TYPE_CACHENOREADLOCK, &hdl);
    if (ires != RES_SUCCESS)
    {
        ErrorMessage ((UINT) IDS_E_TABLE_EVALUATE_COLUMNS, ires);
        CleanUp(lptbl, lpColHeader, lpSelStatement, lpString);
        return;
    }
    ires = SQLGetColumnList(lptbl, lpString);

    ReleaseSession(hdl, RELEASE_ROLLBACK);
    if (ires != RES_SUCCESS)
        ErrorMessage ((UINT) IDS_E_TABLE_EVALUATE_COLUMNS, ires);

    CleanUp(lptbl, lpColHeader, lpSelStatement, lpString);

}

static void OnOption (HWND hwnd)
{
    LPTABLEPARAMS lptbl = GetDlgProp (hwnd);

    if (Button_GetCheck (GetDlgItem (hwnd, IDC_INGCHECKASSELECT)))
    {
        TCHAR szTable[MAXOBJECTNAME];
        HWND hwndTable      = GetDlgItem (hwnd, IDC_TABLE);
        HWND hwndSQLText    = GetDlgItem (hwnd, IDC_INGEDITSQL);
        int nSql            = GetWindowTextLength (hwndSQLText);
        // with STAR the statement "create" is committed immediately. No columns list available.
        if (!IsDDB())
        {
            Edit_GetText (hwndTable, szTable, sizeof(szTable));
            if (lstrlen(szTable) == 0 ) {
                if (nSql > 0)
                {
                //"Please enter the table name before accessing the 'Options' Dialog"
                    MessageBox (hwnd, ResourceString ((UINT)IDS_W_ENTER_TABLE_NAME),
                                      ResourceString ((UINT)IDS_TITLE_WARNING), MB_ICONEXCLAMATION|MB_OK);
                    return;
                }
            }
            else if (!VerifyObjectName(hwnd, szTable, TRUE, FALSE, TRUE))
            {
                Edit_SetSel (hwndTable, 0, -1);
                SetFocus (hwndTable);
                return;
            }

            SimulateColumnList (hwnd, lptbl);
        }
    }
    else
      MakeColumnsList   (hwnd, lptbl);
    DlgTableStructure (hwnd, lptbl);
    lptbl->lpColumns = VDBA20xColumnList_Done (lptbl->lpColumns);
}

static void OnReference   (HWND hwnd)
{
    TABLEPARAMS ts;
    LPTABLEPARAMS lptbl = GetDlgProp (hwnd);
    bPassedByExtraButton=TRUE;

    memset (&ts, 0, sizeof (ts));
    lstrcpy (ts.DBName,     lptbl->DBName);
    lstrcpy (ts.objectname, lptbl->objectname);
    ts.bCreate      = lptbl->bCreate;
    ts.uPage_size   = (lptbl->uPage_size == 0L)? 2048L: lptbl->uPage_size;
    ts.lpReferences = VDBA20xTableReferences_CopyList (lptbl->lpReferences);
    ts.lpUnique     = lptbl->lpUnique;
    ts.primaryKey   = lptbl->primaryKey;
    ts.bCreateVectorWise = lptbl->bCreateVectorWise;
    MakeColumnsList   (hwnd, &ts);
    if (DlgReferences (hwnd, &ts) == IDOK)
    {
        lptbl->lpReferences = VDBA20xTableReferences_Done (lptbl->lpReferences);
        lptbl->lpReferences = ts.lpReferences;
        ts.lpReferences     = NULL;
    }
    ts.lpUnique     = NULL;
    ts.lpReferences = VDBA20xTableReferences_Done (ts.lpReferences);
    ts.lpColumns    = VDBA20xColumnList_Done (ts.lpColumns);
}

static void CntUpdateUnique(HWND hwnd, LPTLCUNIQUE lpunique, LPTLCUNIQUE lpNewUnique)
{
    BOOL    bFound = FALSE;
    CNTDATA dataUnique;
    HWND hwndCnt   = GetDlgItem(hwnd, IDC_CONTAINER);
    LPTLCUNIQUE ls = NULL;
    LPFIELDINFO   lpFld;
    LPRECORDCORE  lpRec = NULL;

    if (IsDDB())
      return;

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

static int PrimaryKeyColumn_Count (HWND hwndCnt)
{
  return 0;
}


static void OnStarLocalTable(HWND hwnd)
{
  LPTABLEPARAMS lptbl = GetDlgProp (hwnd);

  // extern "C" function - return value not used yet
  HWND hwndTable      = GetDlgItem (hwnd, IDC_TABLE);
  Edit_GetText (hwndTable, lptbl->szCurrentName, sizeof(lptbl->szCurrentName));
  DlgStarLocalTable(hwnd, lptbl);
}

//
// Create Primary Key
static void OnPrimaryKey(HWND hwnd)
{
    HWND hwndContainer = NULL;
    LPCOLUMNPARAMS lpCol = NULL;
    LPOBJECTLIST ls = NULL;
    LPSTRINGLIST lpObj = NULL;
    LPTABLEPARAMS lptbl = GetDlgProp (hwnd);
    if (!lptbl)
        return;

    lptbl->primaryKey.pListTableColumn = StringList_Done (lptbl->primaryKey.pListTableColumn);
    //
    // Construct the list of columns to use for creating primary key:
    MakeColumnsList   (hwnd, lptbl);
    ls = lptbl->lpColumns;
    while (ls)
    {
        lpCol = (LPCOLUMNPARAMS)ls->lpObject;
        if (lpCol->uDataType != INGTYPE_LONGVARCHAR && lpCol->uDataType != INGTYPE_LONGBYTE)
        {
            lptbl->primaryKey.pListTableColumn = StringList_AddObject (lptbl->primaryKey.pListTableColumn, lpCol->szColumn, &lpObj);
            if (lpObj)
                lpObj->extraInfo1 = lpCol->bNullable? 0: 1;
        }
        ls = ls->lpNext;
    }
    VDBA_TablePrimaryKey(hwnd, lptbl->DBName, &(lptbl->primaryKey), lptbl);
    lptbl->lpColumns = VDBA20xColumnList_Done (lptbl->lpColumns);
    hwndContainer = GetDlgItem(hwnd, IDC_CONTAINER);
    if (hwndContainer)
    {
        RECT r;
        GetClientRect(hwndContainer, &r);
        InvalidateRect(hwndContainer, &r, FALSE);
    }
}


//
// Added since star management taken in account:
// 2 functions to convert
// record number (line in ownerdraw column of container) <---> data type
// according to whether the current database is distributed or not.
//
// BOTH FUNCTIONS MUST BE MAINTAINED IN PARALLEL !!!
//
static int  ConvertRecnumIntoDataType(int nOffset)
{
  if (IsDDB()) {
    switch (nOffset) {
      case 0: return DT_TYPE;
      case 1: return DT_LEN;
      case 2: return DT_NULLABLE;
      case 3: return DT_DEFAULT;
      case 4: return DT_SYSMAINTAINED;

      // must manage default, since less lines than in non-distributed databases
      default: return -1;
    }
  }
  else {
    if (bGenericGateway) {
        switch (nOffset) {
          case 0: return DT_TYPE;
          case 1: return DT_LEN;
          case 2: return DT_NULLABLE;
          //case 3: return DT_DEFAULT;

          default: return -1;
        }
    }
    else
    {
        switch (nOffset) {
          case 0: return DT_TYPE;
          case 1: return DT_LEN;
          //case 2: return DT_PRIMARY;
          case 2: return DT_NULLABLE;
          case 3: return DT_UNIQUE;
          case 4: return DT_DEFAULT;
          case 5: return DT_DEFSPEC;
          //case 7: return DT_CHECK;
          case 6: return DT_SYSMAINTAINED;

          // no default, since all lines used
        }
    }
  }

  // Case not managed !
  ASSERT(NULL);
  return -1;

}

static int ConvertDataTypeIntoRecnum(int nDataType)
{
  if (IsDDB()) {
    switch (nDataType) {
      case DT_TYPE    : return 0;
      case DT_LEN     : return 1;
      case DT_NULLABLE: return 2;
      case DT_DEFAULT : return 3;
      case DT_SYSMAINTAINED: return 4;
    }
  }
  else {
    if (bGenericGateway) {
        switch (nDataType) {
          case DT_TYPE    : return 0;
          case DT_LEN     : return 1;
          case DT_NULLABLE: return 2;
          default         : return -1;
          //case DT_DEFAULT : return 3;
        }
    }
    else {
        switch (nDataType) {
          case DT_TYPE    : return 0;
          case DT_LEN     : return 1;
          //case DT_PRIMARY : return 2;
          case DT_NULLABLE: return 2;
          case DT_UNIQUE  : return 3;
          case DT_DEFAULT : return 4;
          case DT_DEFSPEC : return 5;
          //case DT_CHECK   : return 7;
          case DT_SYSMAINTAINED: return 6;
        }
    }
  }

  // Case not managed ! probably parallel maintenance not complete
  ASSERT(NULL);
  return -1;
}

static void InitializeFieldDataWithDefault(HWND hwndCnt)
/*
    Function:
        Initialize with default value for Nullable,Default and "Def Spec" options.

    Paramters:
        hwnd    - Handle to the container window.

    Returns:
        None.
*/
{
  CNTDATA data;
  LPRECORDCORE lpRC;
  LPFIELDINFO lpLastField;
  int nLen;

  lpLastField = CntFldTailGet (hwndCnt);
  if(!lpLastField)
    return;
  // check Nullable.
  lpRC = GetDataTypeRecord(hwndCnt, DT_NULLABLE);
  CntFldDataGet(lpRC, lpLastField, sizeof(data), &data);
  data.bNullable=TRUE;
  CntFldDataSet(lpRC, lpLastField, sizeof(data), &data);

  if (bGenericGateway)
    return;

  // check Default.
  lpRC = GetDataTypeRecord(hwndCnt, DT_DEFAULT);
  CntFldDataGet(lpRC, lpLastField, sizeof(data), &data);
  if (IsDDB())
    data.bDefault=FALSE;
  else
    data.bDefault=TRUE;
  CntFldDataSet(lpRC, lpLastField, sizeof(data), &data);

  if (IsDDB())
    return;
  // fill "Def Spec" with NULL like default value.
  lpRC = GetDataTypeRecord(hwndCnt, DT_DEFSPEC);
  CntFldDataGet(lpRC, lpLastField, sizeof(data), &data);
  nLen = lstrlen(szNull);
  data.lpszDefSpec = (LPSTR)ESL_AllocMem(nLen + 1);
  lstrcpy(data.lpszDefSpec,szNull);
  CntFldDataSet(lpRC, lpLastField, sizeof(data), &data);
}


static void VerifyDefaultAndSpecDef(HWND hwndCtl, LPFIELDINFO lpFICur)
/*
    Function:
        Verify "Default" and "Def. Spec" when unchecking "Nullable",
        a warning reminds that the default value of NULL 
        (in the "Def spec" control) is not compatible with the fact that the
        column is no more nullable, and proposes both to uncheck the "Default"
        checkbox, and to remove the "NULL" string in the "Def.Spec" control.

        Not available with a connection on a Generic Gateway and if the current
        database is distributed.

    Paramters:
        hwndCtl - Handle to the container window.
        lpFICur - Address of the current field.

    Returns:
        None.
*/
{
  LPRECORDCORE lpRecDef,lpRecSpecTmp,lpRecType;
  CNTDATA dataTmp,dataDef,DtaType;

  if (IsDDB() || bGenericGateway)
    return;

  lpRecType = GetDataTypeRecord(hwndCtl, DT_TYPE);
  CntFldDataGet(lpRecType, lpFICur, sizeof(DtaType), &DtaType);
  if (DtaType.uDataType == INGTYPE_LONGVARCHAR ||
      DtaType.uDataType == INGTYPE_LONGBYTE    ||
      DtaType.uDataType == INGTYPE_UNICODE_LNVCHR)
    return; // Not Display message for this data type

  lpRecDef = GetDataTypeRecord(hwndCtl, DT_DEFAULT);
  CntFldDataGet(lpRecDef, lpFICur, sizeof(dataDef), &dataDef);
  lpRecSpecTmp = GetDataTypeRecord(hwndCtl, DT_DEFSPEC);
  CntFldDataGet(lpRecSpecTmp, lpFICur, sizeof(dataTmp), &dataTmp);
  if ( dataDef.bDefault == TRUE &&
      (dataTmp.lpszDefSpec && _tcsicmp(dataTmp.lpszDefSpec,szNull) == 0) )
  {
      int nRes;
      //"The column currently has a default value of 'NULL'.\n"
      //"Uncheck 'Default' and remove 'NULL' from the 'Def.Spec' control ?"
      nRes = MessageBox(GetFocus(),ResourceString ((UINT)IDS_I_NON_NULLABLE_DATATYPE),
                                   ResourceString(IDS_TITLE_WARNING),
                                   MB_ICONQUESTION | MB_YESNO );
      if (nRes == IDYES)
      {
          dataDef.bDefault = FALSE;
          CntFldDataSet(lpRecDef, lpFICur, sizeof(dataDef), &dataDef);
          if (dataTmp.lpszDefSpec)
          {
               ESL_FreeMem(dataTmp.lpszDefSpec);
               dataTmp.lpszDefSpec = NULL;
          }
          CntFldDataSet(lpRecSpecTmp, lpFICur, sizeof(dataTmp), &dataTmp);
          CntUpdateRecArea(hwndCtl, lpRecDef, lpFICur);
          CntUpdateRecArea(hwndCtl, lpRecSpecTmp, lpFICur);
      }
  }
}

static void OnChangeFocus(HWND hwnd)
{
   HWND hwndCnt = GetDlgItem (hwnd, IDC_CONTAINER);

   lpEditRC = CntFocusRecGet(hwndCnt);
   lpEditFI = CntFocusFldGet(hwndCnt);
}

static void OnBeginTitleEdit(HWND hwnd)
{
   HWND hwndCnt = GetDlgItem (hwnd, IDC_CONTAINER);

   lpEditRC = NULL;
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
      data.len.dwDataLen = GetMinReqLength(dataType.uDataType);
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


/*
**
**   Function:
**        reallocated the memory in lpDestination and insert from lpCurrent the string lpStrAdd.
**
**   Paramters:
**     - lpDestination    : Current buffer need to be reallocated
**     - lpCurrent        : Address where the string lpStrAdd must be added
**
**     - lpStrAdd         : Character strings having to be added to lpDestination
**
**   Output :
**     - Return the address right after the length of lpStrAdd
**     - Return NULL when allocated memory failled.
*/
static LPTSTR InsertString (LPTSTR *lpDestination, LPTSTR lpCurrent,LPTSTR lpStrAdd)
{
    LPTSTR lpTemp,lpEndString;
    unsigned int nbAlloc,iStartInsertPosition,nbLenAddStr,nbLenOldStr;

    if ( lpCurrent <= *lpDestination )
    {
        ASSERT(FALSE);
        return *lpDestination;
    }

    iStartInsertPosition = lpCurrent - *lpDestination;
    nbLenAddStr = _tcslen(lpStrAdd);
    nbLenOldStr = _tcslen(*lpDestination);

    // Reallocate buffer , caculate size of new buffer
    nbAlloc = nbLenAddStr * sizeof(TCHAR) + sizeof(TCHAR) + nbLenOldStr;

    lpTemp = (LPTSTR)ESL_ReAllocMem(*lpDestination, nbAlloc, nbLenOldStr);
    if (!lpTemp)
    {
        ErrorMessage ((UINT) IDS_E_CANNOT_ALLOCATE_MEMORY, RES_ERR);
        return NULL;
    }
    else
        *lpDestination = lpTemp;

    if (iStartInsertPosition == nbLenOldStr)
    {
        lstrcat (*lpDestination,lpStrAdd);
    }
    else
    {
        // Allocate new buffer to concatenate string
        lpEndString = (LPSTR)ESL_AllocMem (nbAlloc + sizeof(TCHAR));
        if (!lpEndString) {
            ErrorMessage ((UINT) IDS_E_CANNOT_ALLOCATE_MEMORY, RES_ERR);
            return NULL;
        }

        // save the end of string into new buffer
        x_strcpy(lpEndString, *lpDestination + iStartInsertPosition);
        //insert lpStrAdd into the new reallocated string lpDestination
        fstrncpy(*lpDestination+iStartInsertPosition, lpStrAdd, nbLenAddStr+sizeof(TCHAR));
        //add the end of string
        lstrcat (*lpDestination,lpEndString);
        ESL_FreeMem (lpEndString);
    }
    // return address just after lpAddStr
    return *lpDestination + iStartInsertPosition + nbLenAddStr;
}

static LPTSTR GoEndOfQuotes( LPTSTR lpStrTemp , TCHAR tcType)
{

    if (*lpStrTemp == tcType)
        lpStrTemp = _tcsinc(lpStrTemp); // Go to the next character
    else
        return NULL;

    while (*lpStrTemp)
    {
        if (*lpStrTemp == tcType)
        {
            lpStrTemp = _tcsinc(lpStrTemp);
            if (*lpStrTemp == tcType )
            {
                lpStrTemp = _tcsinc(lpStrTemp);
                continue; // this quote is escaped
            }
            else
                return lpStrTemp;    // end of string in quotes.
        }
        else
            lpStrTemp = _tcsinc(lpStrTemp);
    }
    return NULL;
}

static LPTSTR GoEndOfParenthesis( LPTSTR lpStrTemp )
{
    int iNbParenthesis = 0;

    if(*lpStrTemp != _T('('))
        return NULL;

    // go to a corresponding parenthesis
    while (*lpStrTemp )
    {
        if(*lpStrTemp == _T('('))
            iNbParenthesis++;
        else if ( *lpStrTemp == _T(')') )
            iNbParenthesis--;
        else if ( *lpStrTemp == _T('\'') )
        {
            lpStrTemp = GoEndOfQuotes (lpStrTemp,_T('\''));
            if (lpStrTemp == NULL)
                return NULL;
            continue;
        }
        else if (*lpStrTemp == _T('"') )
        {
            lpStrTemp = GoEndOfQuotes(lpStrTemp,_T('"'));
            if (lpStrTemp == NULL)
                return NULL;
            continue;
        }
        if (iNbParenthesis == 0)
        {
            lpStrTemp = _tcsinc(lpStrTemp);
            return lpStrTemp;
        }
        lpStrTemp = _tcsinc(lpStrTemp);
    }
    return NULL;
}

/*
**
**  Function: 
**      Add into the "select" statement an "where" clause never true.
**      Example:
**           - "select * from iitables"                        becomes "select * from iitables where 1 = 0"
**
**           - "select table_name from iitables"               becomes "select table_name from iitables where 1 = 0"
**             "     union (select table_name from iicolumns"          "     union (select table_name from iicolumns where 1 = 0"
**             "     union (select table_name from iiviews))"          "     union (select table_name from iiviews where 1 = 0))"
**
**  Parameters:
**    - *lpNewString      : buffer for the "create table as select" statement.
**    - lpSelectStatement : statement analyzed.
**    - lpTblName         : Table name.
**    - lpHeader          : List of new columns name.
**
**
**  Output:
**    - TRUE   : if the statement is genetated without error.
**    - FALSE  : - error with allocation or reallocation of the memory.
**               - SQL word "select" not in first position in the statement,
**               - no space juste after "select"
**               - Error in number of parenthesis
**               - Error in number of simple quote
**               - Error in number of double quotes
**
*/
static BOOL AddDummyWhereClause(LPTSTR *lpNewString, LPTSTR lpSelectStatement,
                                LPTSTR lpTblName, LPTSTR lpHeader)
{
    LPTSTR lpStrWithWhereClause,lpStrBeginWithWhereClause, lptemp1;
    int nMemoryAllocated, nMemAlloc,iParenthesisLevel;
    BOOL bWhereInserted,bCloseParenthesisAtEndOfWhere,bContinueLevel1,bContinueLevel2;
    TCHAR STR_CREATE_TABLE[]             = "create table ";
    TCHAR STR_AS[]                       = " as ";
    TCHAR STR_DUMMY_WHERE_CLAUSE[]       = " 1 = 0 and (";
    TCHAR STR_FIRST_DUMMY_WHERE_CLAUSE[] = " where 1 = 0";
    TCHAR STR_RIGTH_PARENTHESIS[]        = ")";
    TCHAR STR_LEFT_PARENTHESIS[]         = "(";

    TCHAR STR_SELECT[] = "select"  ;
    TCHAR STR_GROUP [] = "group by";
    TCHAR STR_HAVING[] = "having"  ;
    TCHAR STR_ORDER [] = "order"   ;
    TCHAR STR_UNION [] = "union"   ;
    TCHAR STR_WHERE [] = "where"   ;

    // insert into the SQL statement the dummy where clause
    nMemAlloc = _tcslen(lpSelectStatement) + sizeof(TCHAR);
    lpStrWithWhereClause = (LPSTR)ESL_AllocMem (nMemAlloc);
    if (!lpStrWithWhereClause) {
        ErrorMessage ((UINT) IDS_E_CANNOT_ALLOCATE_MEMORY, RES_ERR);
        return FALSE;
    }
    _tcscpy(lpStrWithWhereClause, lpSelectStatement);
    lpStrBeginWithWhereClause = lpStrWithWhereClause;
    bWhereInserted                = FALSE;
    bCloseParenthesisAtEndOfWhere = FALSE;
    iParenthesisLevel = 0;

    bContinueLevel1 = TRUE;
    bContinueLevel2 = TRUE;

    while (*lpStrWithWhereClause)
    {
        if (isspace(*lpStrWithWhereClause))
        {
            lpStrWithWhereClause = _tcsinc(lpStrWithWhereClause);
            continue;
        }

        if (*lpStrWithWhereClause == _T('('))
        {
            iParenthesisLevel++;
            lpStrWithWhereClause = _tcsinc(lpStrWithWhereClause);
            continue;
        }

        if (strnicmp (lpStrWithWhereClause,STR_SELECT,_tcslen(STR_SELECT)) != 0)
        {
            ESL_FreeMem (lpStrBeginWithWhereClause);
            return FALSE; // error select is not the first SQL word.
        }
        else
            lpStrWithWhereClause = _tcsninc(lpStrWithWhereClause, _tcslen(STR_SELECT));

        if (!isspace(*lpStrWithWhereClause))
        {
            ESL_FreeMem (lpStrBeginWithWhereClause);
            return FALSE; // after select the space is required
        }
        lpStrWithWhereClause = _tcsinc(lpStrWithWhereClause);

        bWhereInserted                = FALSE;
        bCloseParenthesisAtEndOfWhere = FALSE;
        bContinueLevel2 = TRUE;
        while (bContinueLevel2)
        {
            if (isspace(*lpStrWithWhereClause))
            {
                LPSTR lpNextChar = lpStrWithWhereClause;
                int iNbSpaces = 0;
                while ( isspace(*lpNextChar) )
                {
                    iNbSpaces++;
                    lpNextChar = _tcsinc(lpNextChar);
                }
                if (strnicmp (lpNextChar,STR_GROUP ,_tcslen(STR_GROUP))  == 0 ||
                    strnicmp (lpNextChar,STR_HAVING,_tcslen(STR_HAVING)) == 0 ||
                    strnicmp (lpNextChar,STR_ORDER ,_tcslen(STR_ORDER))  == 0 ||
                    strnicmp (lpNextChar,STR_UNION ,_tcslen(STR_UNION))  == 0 )
                {
                    if (bCloseParenthesisAtEndOfWhere)
                    {
                        bCloseParenthesisAtEndOfWhere = FALSE;
                        lptemp1 = InsertString (    &lpStrBeginWithWhereClause,
                                                    lpStrWithWhereClause,
                                                    STR_RIGTH_PARENTHESIS);
                        if (lptemp1)
                            lpStrWithWhereClause = lptemp1;
                        else
                        {
                            ESL_FreeMem (lpStrBeginWithWhereClause);
                            return FALSE;
                        }
                    }
                    if (!bWhereInserted)
                    {
                        lptemp1 = InsertString ( &lpStrBeginWithWhereClause,
                                                 lpStrWithWhereClause,
                                                 STR_FIRST_DUMMY_WHERE_CLAUSE);
                        if (lptemp1)
                            lpStrWithWhereClause = lptemp1;
                        else
                        {
                            ESL_FreeMem (lpStrBeginWithWhereClause);
                            return FALSE;
                        }
                        bWhereInserted = TRUE;
                    }
                    if ( strnicmp (lpNextChar, STR_UNION, _tcslen(STR_UNION)) == 0 )
                    {
                        lpStrWithWhereClause = _tcsninc(lpStrWithWhereClause, _tcslen(STR_UNION)+iNbSpaces);
                        bContinueLevel1 = TRUE;
                        break;
                    }
                }
                else if (strnicmp (lpNextChar, STR_WHERE, _tcslen(STR_WHERE)) == 0 )
                {
                    lpStrWithWhereClause = _tcsninc(lpStrWithWhereClause, _tcslen(STR_WHERE)+iNbSpaces);
                    lptemp1 = InsertString (&lpStrBeginWithWhereClause,
                                             lpStrWithWhereClause,
                                             STR_DUMMY_WHERE_CLAUSE);
                    bWhereInserted = TRUE;
                    bCloseParenthesisAtEndOfWhere = TRUE;
                    if (lptemp1)
                        lpStrWithWhereClause = lptemp1;
                    else
                    {
                        ESL_FreeMem (lpStrBeginWithWhereClause);
                        return FALSE;
                    }
                }
                else
                   lpStrWithWhereClause += iNbSpaces; // no SQL word, skip the space characters
            }
            if ( *lpStrWithWhereClause == _T(')') || *lpStrWithWhereClause == _T('\0') )
            {
                if ( bCloseParenthesisAtEndOfWhere )
                {
                    bCloseParenthesisAtEndOfWhere = FALSE;
                    lptemp1 = InsertString ( &lpStrBeginWithWhereClause,
                                              lpStrWithWhereClause,
                                              STR_RIGTH_PARENTHESIS);
                    if (lptemp1)
                        lpStrWithWhereClause = lptemp1;
                    else
                    {
                        ESL_FreeMem (lpStrBeginWithWhereClause);
                        return FALSE;
                    }
                }
                if ( !bWhereInserted )
                {
                    lptemp1 = InsertString (&lpStrBeginWithWhereClause,
                                            lpStrWithWhereClause,
                                            STR_FIRST_DUMMY_WHERE_CLAUSE);
                    if (lptemp1)
                        lpStrWithWhereClause = lptemp1;
                    else
                    {
                        ESL_FreeMem (lpStrBeginWithWhereClause);
                        return FALSE;
                    }
                    bWhereInserted = TRUE;
                }

                if ( *lpStrWithWhereClause == _T('\0') && iParenthesisLevel == 0)
                {
                    bContinueLevel1 = FALSE;
                    break;
                }

                iParenthesisLevel--;
                if ( iParenthesisLevel < 0 ) // error number of parenthesis
                {
                    ESL_FreeMem (lpStrBeginWithWhereClause);
                    return FALSE;
                }
                if (*lpStrWithWhereClause != _T('\0'))
                    lpStrWithWhereClause = _tcsinc(lpStrWithWhereClause);
                while(TRUE)
                {
                    if ( strnicmp (lpStrWithWhereClause,STR_UNION,_tcslen(STR_UNION)) == 0 )
                    {
                        lpStrWithWhereClause = _tcsninc(lpStrWithWhereClause, _tcslen(STR_UNION));
                        bContinueLevel1 = TRUE;
                        bContinueLevel2 = FALSE;
                        break;
                    }

                    if ( *lpStrWithWhereClause == _T(')') )
                    {
                        iParenthesisLevel--;
                        lpStrWithWhereClause = _tcsinc(lpStrWithWhereClause);
                        continue;
                    }

                    if (isspace(*lpStrWithWhereClause))
                    {
                        lpStrWithWhereClause = _tcsinc(lpStrWithWhereClause);
                        continue;
                    }

                    if (*lpStrWithWhereClause != _T('\0'))
                        lpStrWithWhereClause = _tcsinc(lpStrWithWhereClause);
                    else
                    {
                        bContinueLevel1 = FALSE;
                        bContinueLevel2 = FALSE;

                        if (iParenthesisLevel != 0)
                        {
                            ESL_FreeMem (lpStrBeginWithWhereClause);
                            return FALSE; // error number of parenthesis
                        }
                        break;
                    }
                } // end while level 3
            }
            if(!bContinueLevel2)
                break;
            if (*lpStrWithWhereClause == _T('('))
                lpStrWithWhereClause = GoEndOfParenthesis(lpStrWithWhereClause);
            else if (*lpStrWithWhereClause == _T('"'))
                lpStrWithWhereClause = GoEndOfQuotes(lpStrWithWhereClause,_T('"'));
            else if (*lpStrWithWhereClause == _T('\''))
                lpStrWithWhereClause = GoEndOfQuotes(lpStrWithWhereClause,_T('\''));
            else
                lpStrWithWhereClause = _tcsinc(lpStrWithWhereClause);

            if (lpStrWithWhereClause == NULL) // error in GoEndOfParenthesis()  or GoEndOfQuotes()
            {
                ESL_FreeMem (lpStrBeginWithWhereClause);
                return FALSE;
            }

        } // End While Level 2
        if (!bContinueLevel1)
            break;
        if (*lpStrWithWhereClause != _T('\0'))
            lpStrWithWhereClause = _tcsinc(lpStrWithWhereClause);
    }
    //
    // Create Table as Select 
    if (lpHeader && _tcslen(lpHeader) > 0)
        nMemoryAllocated = _tcslen(STR_CREATE_TABLE) + _tcslen(lpTblName) +
                           _tcslen(STR_LEFT_PARENTHESIS)+ _tcslen(lpHeader) +
                           _tcslen(STR_RIGTH_PARENTHESIS) +
                           _tcslen(STR_AS) +  _tcslen(lpStrBeginWithWhereClause) + sizeof(TCHAR);
    else
        nMemoryAllocated = _tcslen(STR_CREATE_TABLE)+ _tcslen(lpTblName) +
                           _tcslen(STR_AS) + _tcslen(lpStrBeginWithWhereClause) + sizeof(TCHAR);

    *lpNewString = (LPSTR)ESL_AllocMem (nMemoryAllocated);
    if (!*lpNewString) {
        ErrorMessage ((UINT) IDS_E_CANNOT_ALLOCATE_MEMORY, RES_ERR);
        ESL_FreeMem (lpStrBeginWithWhereClause);
        return FALSE;
    }

    lstrcpy (*lpNewString,STR_CREATE_TABLE);
    lstrcat (*lpNewString,lpTblName);

    if (lpHeader && _tcslen(lpHeader) > 0){
        lstrcat (*lpNewString,STR_LEFT_PARENTHESIS);
        lstrcat (*lpNewString,lpHeader);
        lstrcat (*lpNewString,STR_RIGTH_PARENTHESIS);
    }
    lstrcat (*lpNewString,STR_AS);
    lstrcat (*lpNewString,lpStrBeginWithWhereClause);

    if (lpStrBeginWithWhereClause)
        ESL_FreeMem (lpStrBeginWithWhereClause);

    return TRUE;
}


