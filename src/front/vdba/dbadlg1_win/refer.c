/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : refer.c
**    Project  : CA-Visual DBA,OpenIngres V2.0
**    Author   : UK Sotheavut
**    Purpose  : Dialog Box for specifying the Foreign Keys of the Table
**
** History:
**
** xx-12-1996 (uk$so01)
**    Created
** 24-Feb-2000 (uk$so01)
**    BUG #100560. Disable the Parallel index creation and index enhancement
**    if not running against the DBMS 2.5 or later.
** 26-Mar-2001 (noifr01)
**    (sir 104270) removal of code for managing Ingres/Desktop
** 30-Mar-2001 (noifr01)
**    (sir 104378) differentiation of II 2.6.
** 21-Jun-2001 (schph01)
**    (SIR 103694) STEP 2 support of UNICODE datatypes
** 12-May-2004 (schph01)
**    (SIR 111507) Add management for new column type bigint
** 05-Oct-2006 (gupsh01)
**    Modify date to ingresdate.
** 12-Oct-2006 (wridu01)
**    Add support for Ansi Date/Time
**/

#include "dll.h"
#include "getdata.h"
#include "domdata.h"
#include "dbadlg1.h"
#include "dlgres.h"
#include "msghandl.h"
#include "winutils.h"
#include "extccall.h"
#include "prodname.h"
#include "resource.h"
#include "dbaginfo.h"

typedef struct tagFKEYSTRUCT
{
    //
    // bAlter: TRUE if the data is from the DBMS.
    //         ie from the GetDetailInfo.
    BOOL  bAlter;

    int    nOld;        // Non Zero if old constraint name.
    TCHAR  tchszOwner         [MAXOBJECTNAME];
    TCHAR  tchszTable         [MAXOBJECTNAME];
    TCHAR  tchszRefConstraint [MAXOBJECTNAME];
    LPSTRINGLIST l1;    // List of Referencing Columns
    LPSTRINGLIST l2;    // List of Referenced Columns;

    BOOL  bUseDeleteAction;                  // If TRUE, tchszDeleteAction is available
    BOOL  bUseUpdateAction;                  // If TRUE, tchszUpdateAction is available
    TCHAR tchszDeleteAction [16];            // "cascade" | "restrict" | "set null"
    TCHAR tchszUpdateAction [16];            // "cascade" | "restrict" | "set null"
    INDEXOPTION constraintWithClause;        // Constarint Enhancement (2.6)
} FKEYSTRUCT, FAR* LPFKEYSTRUCT;
static LPFKEYSTRUCT FKEYSTRUCT_INIT (LPREFERENCEPARAMS lpRef);
static LPFKEYSTRUCT FKEYSTRUCT_DONE (LPFKEYSTRUCT lpObj);
static LPREFERENCEPARAMS VDBA20xForeignKey_Init (LPFKEYSTRUCT lpStruct);

static TCHAR tchszSame[MAXOBJECTNAME];

static void OnDestroy               (HWND hwnd);
static void OnCommand               (HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
static BOOL OnInitDialog            (HWND hwnd, HWND hwndFocus, LPARAM lParam);
static void OnSysColorChange        (HWND hwnd);
static void EnableDisableOKButton   (HWND hwnd);
static void OnOK                    (HWND hwnd);
static void OnAddConstraint         (HWND hwnd);
static void OnDropConstraint        (HWND hwnd, BOOL bCascade);
static void OnTableColumnsSelChange (HWND hwnd);
static void OnReferencedTbSelChange (HWND hwnd);
static void OnUniqueKeySelChange    (HWND hwnd);

static void OnForeignKeyColumnDown  (HWND hwnd);
static void OnForeignKeyColumnUp    (HWND hwnd);
static void OnConstraintSelChange   (HWND hwnd);
static void GetConstraintName       (HWND hwnd, LPSTR lptchBuffer);
static void EnableButton            (HWND hwnd);
static void ShowPrimaryKey          (HWND hwnd, LPTABLEPARAMS lpTS, LPTSTR lpVnode, BOOL bSameTable, LPTSTR lpKey);
static LPSTRINGLIST MakePrimaryKey  (LPOBJECTLIST lpColList);
static LPSTRINGLIST MakeUniqueKey   (LPTLCUNIQUE  lpUnique, LPTSTR tchszUniqueKey);
static void OnPrimaryKeyRadioClick  (HWND hwnd);
static void OnUniqueKeyRadioClick   (HWND hwnd);
static void VDBA20xComboKey_CleanUp (HWND hwndCtrl);
static BOOL UpdateReference         (HWND hwnd, LPFKEYSTRUCT lpFk);
static BOOL OnUpdateConstraintName  (HWND hwnd);
static BOOL DataValidation          (HWND hwnd);
static void DestroyConstraintList   (HWND hwndCtrl);
static void OnButtonPut             (HWND hwnd);
static void OnButtonRemove          (HWND hwnd);
static void OnConstraintIndex       (HWND hwnd);
static void InitComboActions        (HWND hwndCombo);
static void SetDisplayMode          (HWND hwnd, BOOL bAlter);

static LPFKEYSTRUCT FKEYSTRUCT_INIT (LPREFERENCEPARAMS lpRef)
{
    LPFKEYSTRUCT    lpObj;
    LPOBJECTLIST    ls;
    LPREFERENCECOLS lpRefCol;
    if (!lpRef)
        return NULL;
    lpObj = (LPFKEYSTRUCT)ESL_AllocMem (sizeof (FKEYSTRUCT));
    if (!lpObj)
    {
        ErrorMessage ((UINT)IDS_E_CANNOT_ALLOCATE_MEMORY, RES_ERR);
        return NULL;
    }
    memset (lpObj, 0, sizeof (FKEYSTRUCT));
    lpObj->bAlter = lpRef->bAlter;
    lpObj->bUseDeleteAction = lpRef->bUseDeleteAction;
    lpObj->bUseUpdateAction = lpRef->bUseUpdateAction;
    lstrcpy (lpObj->tchszOwner,         lpRef->szSchema);
    lstrcpy (lpObj->tchszTable,         lpRef->szTable);
    lstrcpy (lpObj->tchszRefConstraint, lpRef->szRefConstraint);
    lstrcpy (lpObj->tchszDeleteAction, lpRef->tchszDeleteAction);
    lstrcpy (lpObj->tchszUpdateAction, lpRef->tchszUpdateAction);
    INDEXOPTION_COPY (&(lpObj->constraintWithClause), &(lpRef->constraintWithClause));

    ls = lpRef->lpColumns;
    while (ls)
    {
        lpRefCol = (LPREFERENCECOLS)ls->lpObject;
        lpObj->l1 = StringList_Add (lpObj->l1, lpRefCol->szRefingColumn);
        lpObj->l2 = StringList_Add (lpObj->l2, lpRefCol->szRefedColumn);
        if (!(lpObj->l1 && lpObj->l2))
        {
            ErrorMessage ((UINT)IDS_E_CANNOT_ALLOCATE_MEMORY, RES_ERR);
            FKEYSTRUCT_DONE (lpObj);
            return NULL;
        }
        ls = ls->lpNext;
    }
    return lpObj;
}

static LPFKEYSTRUCT FKEYSTRUCT_DONE (LPFKEYSTRUCT lpObj)
{
    if (!lpObj)
        return NULL;
    lpObj->l1 = StringList_Done (lpObj->l1);
    lpObj->l2 = StringList_Done (lpObj->l2);
    lpObj->constraintWithClause.pListLocation = StringList_Done (lpObj->constraintWithClause.pListLocation);
    ESL_FreeMem (lpObj);
    return NULL;
}

static LPREFERENCEPARAMS VDBA20xForeignKey_Init (LPFKEYSTRUCT lpStruct)
{
    LPSTRINGLIST l1, l2;
    LPREFERENCEPARAMS lpObj;
    LPOBJECTLIST     O, listCol = NULL;
    LPREFERENCECOLS lpRefCol;
    if (!lpStruct)
        return NULL;
    lpObj = (LPREFERENCEPARAMS)ESL_AllocMem (sizeof (REFERENCEPARAMS));
    if (!lpObj)
    {
        ErrorMessage ((UINT)IDS_E_CANNOT_ALLOCATE_MEMORY, RES_ERR);
        return NULL;
    }
    memset (lpObj, 0, sizeof (REFERENCEPARAMS));
    lpObj->bAlter = lpStruct->bAlter;
    lpObj->bUseDeleteAction = lpStruct->bUseDeleteAction;
    lpObj->bUseUpdateAction = lpStruct->bUseUpdateAction;
    lstrcpy (lpObj->szSchema,       lpStruct->tchszOwner);
    lstrcpy (lpObj->szTable,        lpStruct->tchszTable);
    lstrcpy (lpObj->szRefConstraint,lpStruct->tchszRefConstraint);
    lstrcpy (lpObj->tchszDeleteAction, lpStruct->tchszDeleteAction);
    lstrcpy (lpObj->tchszUpdateAction, lpStruct->tchszUpdateAction);
    INDEXOPTION_COPY (&(lpObj->constraintWithClause), &(lpStruct->constraintWithClause));
    //
    // Create a list of foreign key columns, ie list of referencing columns
    // and list of referenced columns.
    //
    l1 = lpStruct->l1;
    l2 = lpStruct->l2;
    while (l1 && l2)
    {
        O = AddListObjectTail (&listCol, sizeof(REFERENCECOLS));
        if (!O)
        {
            FreeObjectList (listCol);
            listCol = NULL;
            ErrorMessage ((UINT)IDS_E_CANNOT_ALLOCATE_MEMORY, RES_ERR);
            return NULL;
        }
        lpRefCol = (LPREFERENCECOLS)O->lpObject;
        lstrcpy (lpRefCol->szRefingColumn, l1->lpszString);    
        lstrcpy (lpRefCol->szRefedColumn,  l2->lpszString);    
        l1 = l1->next;
        l2 = l2->next;
    }
    lpObj->lpColumns = listCol;
    return lpObj;
}


BOOL CALLBACK EXPORT DlgReferencesDlgProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
int WINAPI EXPORT DlgReferences (HWND hwndOwner, LPTABLEPARAMS lpTable)
{
    FARPROC lpProc;
    int     retVal;

    if (!IsWindow (hwndOwner) || !lpTable)
    {
        ASSERT(NULL);
        return FALSE;
    }
    lpProc = MakeProcInstance ((FARPROC) DlgReferencesDlgProc, hInst);
    retVal = VdbaDialogBoxParam
        (hResource,
        MAKEINTRESOURCE (IDD_REFERENCES),
        hwndOwner,
        (DLGPROC) lpProc,
        (LPARAM)  lpTable);
    FreeProcInstance (lpProc);
    return (int)retVal;
}


BOOL CALLBACK EXPORT DlgReferencesDlgProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
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
        return HANDLE_WM_INITDIALOG (hwnd, wParam, lParam, OnInitDialog);
    default:
        return FALSE;
    }
    return TRUE;
}

static BOOL OnInitDialog (HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    int   index;
    LPSTR lpString = NULL;
    TCHAR szTab    [] = {' ', ' ', ' ', ' ', '\0'};
    TCHAR szReturn [] = {0x0D, 0x0A, '\0'}; 
    TCHAR szConstraintName [MAXOBJECTNAME];
    TCHAR tchszParentTbName[MAXOBJECTNAME];
    LPFKEYSTRUCT      lpFkStruct;
    LPSTRINGLIST      lpStringList = NULL;
    LPREFERENCEPARAMS lpRef;
    LPOBJECTLIST      ls;
    LPCOLUMNPARAMS    lpCol    = NULL;
    LPTABLEPARAMS     lpTable  = (LPTABLEPARAMS)lParam;
    TABLEPARAMS       TS;
    HWND hwndConstraintName   = GetDlgItem (hwnd, IDC_LIST3);
    HWND hwndReferencingCols  = GetDlgItem (hwnd, IDC_LIST0);
    HWND hwndPkeyCols         = GetDlgItem (hwnd, IDC_LIST2);
    HWND hwndReferencedTable  = GetDlgItem (hwnd, IDC_COMBO2);
    HWND hwndReferencedPKey   = GetDlgItem (hwnd, IDC_EDIT1);
    HWND hwndComboPKey        = GetDlgItem (hwnd, IDC_REFERENCES);
    HWND hwndComboUKey        = GetDlgItem (hwnd, IDC_COMBO3);
    LPTSTR lpVnode = (LPTSTR)GetVirtNodeName(GetCurMdiNodeHandle ());

    if (!AllocDlgProp (hwnd, lpTable))
        return FALSE;
    lpHelpStack = StackObject_PUSH (lpHelpStack, StackObject_INIT ((UINT)IDD_REFERENCES));
    if (!lpTable->bCreate)
        ShowWindow (GetDlgItem (hwnd, IDC_REMOVE_CASCADE), SW_SHOW);
    else
        ShowWindow (GetDlgItem (hwnd, IDC_REMOVE_CASCADE), SW_HIDE);

    InitComboActions(GetDlgItem (hwnd, IDC_COMBO_DELETE_RULE));
    InitComboActions(GetDlgItem (hwnd, IDC_COMBO_UPDATE_RULE));

    Button_SetCheck (GetDlgItem (hwnd, IDC_CHECK_DELETE_RULE), FALSE);
    Button_SetCheck (GetDlgItem (hwnd, IDC_CHECK_UPDATE_RULE), FALSE);
    ComboBox_SetCurSel (GetDlgItem (hwnd, IDC_COMBO_DELETE_RULE), 1);
    ComboBox_SetCurSel (GetDlgItem (hwnd, IDC_COMBO_UPDATE_RULE), 1);

    //
    // Fill the Referential Constraint Name ListBox.
    //
    ls = lpTable->lpReferences;
    while (ls)
    {
        lpRef = (LPREFERENCEPARAMS)ls->lpObject;
        index = ListBox_AddString (hwndConstraintName, lpRef->szConstraintName);
        if (index == LB_ERR)
            return FALSE;
        lpFkStruct = FKEYSTRUCT_INIT (lpRef);
        if (!lpFkStruct)
        {
            DestroyConstraintList (hwndConstraintName);
            return FALSE;
        }
        if (VDBA20xForeignKey_Search (lpTable->lpOldReferences, lpRef->szConstraintName))
            lpFkStruct->nOld = 1;
        ListBox_SetItemData (hwndConstraintName, index, (DWORD)lpFkStruct);
        ls = ls->lpNext;
    }
    ListBox_SetCurSel (hwndConstraintName, 0);
   
    //
    // Initialize the Referencing Table's columns
    //
    ls = lpTable->lpColumns;
    while (ls)
    {
        lpCol = (LPCOLUMNPARAMS)ls->lpObject;
        if (lpCol->nPrimary > 0)
            index = ListBox_AddString (hwndReferencingCols, lpCol->szColumn);
        else
            index = ListBox_AddString (hwndReferencingCols, lpCol->szColumn);
        if (index == LB_ERR)
            return FALSE;
        ls = ls->lpNext;
    }
    //
    // Fill Table in the Referenced Combobox.
    if (LoadString (hResource, (UINT)IDS_SAMETABLE, tchszSame, sizeof (tchszSame)) > 0)
        index = ComboBox_AddString (hwndReferencedTable, tchszSame);
    else
    {
        index = ComboBox_AddString (hwndReferencedTable, "<Same Table>");
        lstrcpy (tchszSame, "<Same Table>");
    }
    if (index != CB_ERR)
    {
        LPSTR dummyCreator = (LPSTR)ESL_AllocMem (MAXOBJECTNAME);
        if (dummyCreator)
        {
            lstrcpy (dummyCreator, "<DummyCreator>");
            ComboBox_SetItemData (hwndReferencedTable, index, dummyCreator);
        }
    }
    ComboBoxFillTables (hwndReferencedTable, (LPUCHAR)(lpTable->DBName));
    if (!lpTable->bCreate)
    {
        TCHAR tchszItem [MAXOBJECTNAME];
        StringWithOwner ((LPTSTR)Quote4DisplayIfNeeded(lpTable->objectname),  lpTable->szSchema, tchszItem);
        index = ComboBox_FindStringExact (hwndReferencedTable, -1, Quote4DisplayIfNeeded(lpTable->objectname));
        if (index == CB_ERR)
            index = ComboBox_FindStringExact (hwndReferencedTable, -1, tchszItem);
        if (index >= 0)
        {
            LPTSTR lpOwner = (LPTSTR)ComboBox_GetItemData (hwndReferencedTable, index);
            if (lpOwner) ESL_FreeMem (lpOwner);
            ComboBox_DeleteString (hwndReferencedTable, index);
        }
    }
    richCenterDialog (hwnd);
    EnableButton (hwnd);
    //
    // Hide the button index enhancement if not Ingres >= 2.5
    if ( GetOIVers() < OIVERS_25) 
    {
        HWND hwndButtonIndex = GetDlgItem (hwnd, IDC_CONSTRAINT_INDEX);
        if (hwndButtonIndex && IsWindow (hwndButtonIndex))
            ShowWindow (hwndButtonIndex, SW_HIDE);
    }


    //
    // Preselect the referential constraint name if exists.
    // ---------------------------------------------------
    //
    // Check the primary Key radio by default. And Disable the ComboBox of Unique Keys.
    CheckRadioButton (hwnd, IDC_RADIO1, IDC_RADIO2, IDC_RADIO1);
    EnableWindow (hwndComboUKey , FALSE);   // Disable Combo of Unique Keys

    index = ListBox_GetCurSel (hwndConstraintName);
    if (index == LB_ERR)
        return TRUE;
    ListBox_GetText (hwndConstraintName, index, szConstraintName);
    if (!szConstraintName[0])
        return TRUE;
    //
    // Constraint name exists, then select the first one.
    //
    lpFkStruct = (LPFKEYSTRUCT)ListBox_GetItemData (hwndConstraintName, index);
    //
    // For the existing constraint name, ie from the GetDetailInfo, then grey out
    // the constrol.
    Edit_SetText (GetDlgItem (hwnd, IDC_EDIT2), szConstraintName);
    if (lpFkStruct && lpFkStruct->nOld > 0)
        EnableWindow (GetDlgItem (hwnd, IDC_EDIT2), FALSE);
    else
        EnableWindow (GetDlgItem (hwnd, IDC_EDIT2), TRUE);
    //
    // Selected the Referenced Table (Parent Table)
    if (lstrcmp (lpRef->szTable, tchszSame) == 0 || 
        (lstrcmpi (lpTable->objectname, lpRef->szTable) == 0 && lstrcmpi (lpTable->szSchema, lpRef->szSchema) == 0))
        ComboBox_SelectString (hwndReferencedTable, -1, tchszSame);    
    else
        ComboBoxSelectStringWithOwner (hwndReferencedTable, (LPCTSTR)lpFkStruct->tchszTable, (LPTSTR)lpFkStruct->tchszOwner);
    //
    // Fill the the Foreign Key columns.
    lpStringList = lpFkStruct->l1;
    while (lpStringList)
    {   
        index = ListBox_AddString (hwndPkeyCols, lpStringList->lpszString);
        if (index == LB_ERR)
            return FALSE;
         lpStringList = lpStringList->next;
    }
    // 
    // Check the Referenced constraint name, ie show the primary key or unique key
    // where the foreign key columns are referencing to.
    memset (&TS, 0, sizeof (TABLEPARAMS));
    index = ComboBox_GetCurSel (hwndReferencedTable);
    if (index != CB_ERR)
        GetWindowText (hwndReferencedTable, tchszParentTbName, sizeof (tchszParentTbName));
    if (lstrcmpi (lpFkStruct->tchszTable, tchszSame) == 0 ||
        (lstrcmpi (lpFkStruct->tchszTable, lpTable->objectname) == 0 && lstrcmpi (lpFkStruct->tchszOwner, lpTable->szSchema) == 0))
    {
        if (lpFkStruct->tchszRefConstraint[0])
            ShowPrimaryKey (hwnd, (LPTABLEPARAMS)&TS, lpVnode, TRUE, lpFkStruct->tchszRefConstraint);
        else
            ShowPrimaryKey (hwnd, (LPTABLEPARAMS)&TS, lpVnode, TRUE, NULL);
    }
    else
    {
        LPSTR lpCreator = NULL;;
        lstrcpy (TS.DBName,         lpTable->DBName);
        lstrcpy (TS.objectname,     RemoveDisplayQuotesIfAny((LPTSTR)StringWithoutOwner (tchszParentTbName)));
        lpCreator = (LPSTR)ComboBox_GetItemData (hwndReferencedTable, index);
        if (!lpCreator)
        {
            FreeAttachedPointers (&TS, OT_TABLE);
            return FALSE;
        }
        lstrcpy (TS.szSchema,   lpCreator);
        if (lpFkStruct->tchszRefConstraint[0])
            ShowPrimaryKey (hwnd, (LPTABLEPARAMS)&TS, lpVnode, FALSE, lpFkStruct->tchszRefConstraint);
        else
            ShowPrimaryKey (hwnd, (LPTABLEPARAMS)&TS, lpVnode, FALSE, NULL);
    }
    //
    // Preselect the Referential Action:
    Button_SetCheck (GetDlgItem (hwnd, IDC_CHECK_DELETE_RULE), lpFkStruct->bUseDeleteAction);
    Button_SetCheck (GetDlgItem (hwnd, IDC_CHECK_UPDATE_RULE), lpFkStruct->bUseUpdateAction);
    ComboBox_SelectString (GetDlgItem (hwnd, IDC_COMBO_DELETE_RULE), -1, lpFkStruct->tchszDeleteAction);
    ComboBox_SelectString (GetDlgItem (hwnd, IDC_COMBO_UPDATE_RULE), -1, lpFkStruct->tchszUpdateAction);

    FreeAttachedPointers (&TS, OT_TABLE);
    EnableButton (hwnd);
    return TRUE;
}


static void OnDestroy (HWND hwnd)
{
    HWND hwndReferencedTable  = GetDlgItem (hwnd, IDC_COMBO2);
    HWND hwndReferencingCols  = GetDlgItem (hwnd, IDC_LIST0);

    VDBA20xComboKey_CleanUp (GetDlgItem (hwnd, IDC_REFERENCES));
    VDBA20xComboKey_CleanUp (GetDlgItem (hwnd, IDC_COMBO3));
    DestroyConstraintList   (GetDlgItem (hwnd, IDC_LIST3));
    ComboBoxDestroyItemData (hwndReferencedTable);
    DeallocDlgProp (hwnd);
    lpHelpStack = StackObject_POP (lpHelpStack);
}


static void OnCommand (HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    switch (id)
    {
    case IDOK:
        OnOK (hwnd);
        break;
    case IDCANCEL:
        EndDialog (hwnd, IDCANCEL);
        break;
    case IDC_ADD:           // Button New
        OnAddConstraint (hwnd);
        EnableButton (hwnd);
        break;
    case IDC_DELETE:        // Button Delete
        OnDropConstraint (hwnd, FALSE);
        EnableButton (hwnd);
        break;
    case IDC_REMOVE_CASCADE:// Button Del Cascade
        OnDropConstraint (hwnd, TRUE);
        EnableButton (hwnd);
        break;
    case IDC_EDIT2:         // Constraint Name Edit Box
        switch (codeNotify)
        {
        case EN_KILLFOCUS:
            if (Edit_GetModify (hwndCtl) && OnUpdateConstraintName (hwnd))
                Edit_SetModify (hwndCtl, FALSE);
            break;
        case EN_CHANGE:
            Edit_SetModify (hwndCtl, TRUE);
            break;
        case EN_SETFOCUS:
            break;
        }
        break;
    case IDC_LIST3:         // List of Referential Constraint Name
        if (codeNotify ==  LBN_SELCHANGE)
        {
            ShowHourGlass ();
            OnConstraintSelChange  (hwnd);
            EnableButton (hwnd);
            RemoveHourGlass ();
        }
        break;
    case IDC_COMBO2:        // ComboBox of Parent Table
        if (codeNotify == CBN_SELCHANGE)
        {
            ShowHourGlass ();
            OnReferencedTbSelChange (hwnd);
            EnableButton (hwnd);
            RemoveHourGlass ();
        }
        break;
    case IDC_COMBO3:        // Unique Key Cobo Box
        if (codeNotify == CBN_SELCHANGE)
        {
            ShowHourGlass ();
            OnUniqueKeySelChange (hwnd);
            EnableButton (hwnd);
            RemoveHourGlass ();
        }
        break;
    case IDC_LIST0:         // List of Table Columns
        if (codeNotify == LBN_DBLCLK)
            OnButtonPut (hwnd);
        EnableButton (hwnd);
        break;
    case IDC_LIST2:         // List Foreign Key Columns
        if (codeNotify == LBN_DBLCLK)
            OnButtonRemove (hwnd);
        EnableButton (hwnd);
        break;
    case IDC_BUTTON_PUT:    // The button (>>)    
        OnButtonPut (hwnd);
        EnableButton (hwnd);
        break;
    case IDC_BUTTON_REMOVE: // The button (<<)  
        OnButtonRemove (hwnd);
        EnableButton (hwnd);
        break;
    case IDC_BUTTON_UP:     // The Button UP
        OnForeignKeyColumnUp  (hwnd);
        break;
    case IDC_BUTTON_DOWN:   // The Button Down
        OnForeignKeyColumnDown(hwnd);
        break;
    case IDC_RADIO1:
        ShowHourGlass ();
        OnPrimaryKeyRadioClick(hwnd);        
        OnReferencedTbSelChange (hwnd);
        EnableButton (hwnd);
        RemoveHourGlass ();
        break;
    case IDC_RADIO2:
        ShowHourGlass ();
        OnUniqueKeyRadioClick (hwnd);        
        OnReferencedTbSelChange (hwnd);
        EnableButton (hwnd);
        RemoveHourGlass ();
        break;
    case IDC_CONSTRAINT_INDEX:
        OnConstraintIndex(hwnd);
        break;
    case IDC_CHECK_DELETE_RULE:
    case IDC_CHECK_UPDATE_RULE:
        {
            int index;
            LPFKEYSTRUCT lpFk = NULL;

            index = ListBox_GetCurSel (GetDlgItem (hwnd, IDC_LIST3));
            if (index != LB_ERR)
                lpFk = (LPFKEYSTRUCT)ListBox_GetItemData (GetDlgItem (hwnd, IDC_LIST3), index);
            UpdateReference (hwnd, lpFk);
        }
        break;
    case IDC_COMBO_DELETE_RULE:
    case IDC_COMBO_UPDATE_RULE:
        if (codeNotify == CBN_SELCHANGE)
        {
            int index;
            LPFKEYSTRUCT lpFk = NULL;

            index = ListBox_GetCurSel (GetDlgItem (hwnd, IDC_LIST3));
            if (index != LB_ERR)
                lpFk = (LPFKEYSTRUCT)ListBox_GetItemData (GetDlgItem (hwnd, IDC_LIST3), index);
            UpdateReference (hwnd, lpFk);
        }
        break;
    }
}


static void OnSysColorChange (HWND hwnd)
{
#ifdef _USE_CTL3D
   Ctl3dColorChange();
#endif
}


static void OnOK (HWND hwnd)
{
    int   i, nCount;
    HWND  hwndConstraintName  = GetDlgItem (hwnd, IDC_LIST3);
    HWND  hwndReferencedTable = GetDlgItem (hwnd, IDC_COMBO2);
    HWND  hwndReferencingCols = GetDlgItem (hwnd, IDC_LIST0);
    HWND  hwndPkeyCols        = GetDlgItem (hwnd, IDC_LIST2);
    LPOBJECTLIST        lpReferenceList = NULL;
    LPFKEYSTRUCT        lpStruct;
    LPTABLECOLUMNS      lpColList = NULL;
    LPREFERENCEPARAMS   lpFk = NULL;
    LPTABLEPARAMS   lpTable  = (LPTABLEPARAMS)GetDlgProp (hwnd);
    //
    // Try to validate the data
    //
    if (!DataValidation (hwnd))
        return ;
    //
    // Construct the list of References
    nCount = ListBox_GetCount (hwndConstraintName);
    for (i=0; i<nCount; i++)
    {
        lpStruct = (LPFKEYSTRUCT)ListBox_GetItemData (hwndConstraintName, i);
        lpFk     =  VDBA20xForeignKey_Init (lpStruct);
        if (!lpFk)
        {
            ErrorMessage ((UINT)IDS_E_CANNOT_ALLOCATE_MEMORY, RES_ERR);
            VDBA20xTableReferences_Done (lpReferenceList);
            EndDialog (hwnd, IDCANCEL);
            return;
        }
        ListBox_GetText (hwndConstraintName, i, lpFk->szConstraintName);
        lpReferenceList = VDBA20xForeignKey_Add (lpReferenceList, lpFk);
    }
    lpTable->lpReferences = VDBA20xTableReferences_Done (lpTable->lpReferences);
    lpTable->lpReferences = lpReferenceList;
    EndDialog (hwnd, IDOK);
}

//
// Add Foreign Key
// When user clicks on the button: New
// ----------------------------------
static void OnAddConstraint (HWND hwnd)
{
    int  index;
    HWND hwndConstraintName  = GetDlgItem (hwnd, IDC_LIST3);
    LPTABLEPARAMS   lpTable  = (LPTABLEPARAMS)GetDlgProp (hwnd);
    LPFKEYSTRUCT      lpFk;
    TCHAR tchszCName  [MAXOBJECTNAME];

    //
    // Add the <Unnamed n> into the list box.
    GetConstraintName (hwndConstraintName, tchszCName);
    index = ListBox_AddString (hwndConstraintName, tchszCName); 
    if (index == LB_ERR)
    {
        EndDialog (hwnd, IDCANCEL);
        ErrorMessage ((UINT)IDS_E_CANNOT_ALLOCATE_MEMORY, RES_ERR);
        return;
    }
    lpFk = (LPFKEYSTRUCT)ESL_AllocMem (sizeof (FKEYSTRUCT));
    if (!lpFk)
    {
        EndDialog (hwnd, IDCANCEL);
        ErrorMessage ((UINT)IDS_E_CANNOT_ALLOCATE_MEMORY, RES_ERR);
        return;
    }
    memset (lpFk, 0, sizeof (FKEYSTRUCT));
    ListBox_SetItemData (hwndConstraintName, index, (DWORD)lpFk);
    //
    // Initialize the characteristics.
    VDBA20xComboKey_CleanUp (GetDlgItem (hwnd, IDC_REFERENCES));
    VDBA20xComboKey_CleanUp (GetDlgItem (hwnd, IDC_COMBO3));
    ListBox_ResetContent    (GetDlgItem (hwnd, IDC_LIST2));         // Foreign key cols
    ComboBox_SetCurSel      (GetDlgItem (hwnd, IDC_COMBO2), -1);    // Table
    ComboBox_SetCurSel      (GetDlgItem (hwnd, IDC_COMBO3), -1);    // Unique Key
    ListBox_SetCurSel       (hwndConstraintName, index);
    Edit_SetText (GetDlgItem (hwnd, IDC_EDIT2), tchszCName);
    if (VDBA20xForeignKey_Search (lpTable->lpOldReferences, tchszCName))
        EnableWindow (GetDlgItem (hwnd, IDC_EDIT2), FALSE);
    else
        EnableWindow (GetDlgItem (hwnd, IDC_EDIT2), TRUE);
    Edit_SetText (GetDlgItem(hwnd, IDC_EDIT1), "");
}


static void OnDropConstraint (HWND hwnd, BOOL bCascade)
{
    int   nCount, index;
    TCHAR tchszConstraint [MAXOBJECTNAME];
    TCHAR szMessage[256];
    TCHAR szTitle[256];
    TCHAR szOutput[256];
    HWND  hwndConstraintName   = GetDlgItem (hwnd, IDC_LIST3);
    LPFKEYSTRUCT lpFk;
    LPTABLEPARAMS     lpTable  =  (LPTABLEPARAMS)GetDlgProp (hwnd);

    memset (tchszConstraint, 0, sizeof (tchszConstraint));
    index = ListBox_GetCurSel (hwndConstraintName);
    if (index == LB_ERR)
        return;     
    ListBox_GetText (hwndConstraintName, index, tchszConstraint);
    if (!tchszConstraint[0])
        return;
    LoadString(hResource, IDS_DELETEREF, szMessage, sizeof(szMessage));
//    LoadString(hResource, IDS_PRODUCT,   szTitle,   sizeof(szTitle));
    VDBA_GetProductCaption(szTitle, sizeof(szTitle));
    wsprintf(szOutput, szMessage, tchszConstraint);
    if (MessageBox(NULL, szOutput, szTitle, MB_ICONEXCLAMATION | MB_YESNO| MB_TASKMODAL) == IDNO)
        return;
    lpFk = (LPFKEYSTRUCT)ListBox_GetItemData (hwndConstraintName, index);
    if (!lpFk)
        return;
    if (lpFk->nOld && bCascade)
    {
        LPSTRINGLIST lpObj;
        lpTable->lpDropObjectList = StringList_AddObject (lpTable->lpDropObjectList, tchszConstraint, &lpObj);
        if (!lpTable->lpDropObjectList)
        {
            ErrorMessage ((UINT)IDS_E_CANNOT_ALLOCATE_MEMORY, RES_ERR);
            return;
        }
        lpObj->extraInfo1 = 1;
    }
    lpTable->lpReferences = VDBA20xTableReferences_Remove(lpTable->lpReferences, tchszConstraint);
    FKEYSTRUCT_DONE (lpFk);
    nCount = ListBox_GetCount (hwndConstraintName);
    ListBox_DeleteString (hwndConstraintName, index);
    if (index == (nCount-1))
    {
        //
        // We've deleted the last item in the list box. 
        ListBox_SetCurSel (hwndConstraintName, index -1);
        OnConstraintSelChange (hwnd);
    }
    else
    if (index < (nCount -1) && (index > 0))
    {
        //
        // We've deleted the item in the midle of the list box. 
        ListBox_SetCurSel (hwndConstraintName, index);
        OnConstraintSelChange (hwnd);
    }
    else
    if (index == 0)
    {
        //
        // We've deleted the first item in the list box. 
        ListBox_SetCurSel (hwndConstraintName, 0);
        OnConstraintSelChange (hwnd);
    }
}

//
// When this combo change its selection, we clean up the item data
// of primary key combo and unique key combo and then reinitialize
// them again.
// Display information about foreign key or unique key in the Edit Control.
// 
static void OnReferencedTbSelChange (HWND hwnd)
{
    int index;
    LPFKEYSTRUCT      lpFk;
    LPTABLEPARAMS     lpTS;
    LPTABLEPARAMS     lpTable  =  (LPTABLEPARAMS)GetDlgProp (hwnd);
    TCHAR tchszParentTbName  [128];
    TCHAR tchszConstraint    [MAXOBJECTNAME];
    HWND  hwndConstraintName   = GetDlgItem (hwnd, IDC_LIST3);
    HWND  hwndReferencedTable  = GetDlgItem (hwnd, IDC_COMBO2);
    HWND  hwndReferencedPKey   = GetDlgItem (hwnd, IDC_EDIT1);
    HWND  hwndComboPKey        = GetDlgItem (hwnd, IDC_REFERENCES);  // Hidden Control of Primary Key Constraint
    HWND  hwndComboUKey        = GetDlgItem (hwnd, IDC_COMBO3);      // Unique Key Constraint
    LPTSTR lpVnode = (LPTSTR)GetVirtNodeName(GetCurMdiNodeHandle ());

    index = ComboBox_GetCurSel (hwndReferencedTable);
    if (index == CB_ERR)
    {
        //MessageBox (hwnd, tchszMsg1, "Error", MB_ICONEXCLAMATION|MB_OK);
        return;
    }
    memset (tchszParentTbName, 0, sizeof (tchszParentTbName));
    GetWindowText (hwndReferencedTable, tchszParentTbName, sizeof (tchszParentTbName));
    Edit_SetText  (hwndReferencedPKey, "");
    //
    // Clean up the Primary Key and Unique Key Combo Box
    VDBA20xComboKey_CleanUp (hwndComboPKey);
    VDBA20xComboKey_CleanUp (hwndComboUKey);
    memset (tchszConstraint, 0, sizeof (tchszConstraint));
    index = ListBox_GetCurSel (hwndConstraintName);
    ListBox_GetText (hwndConstraintName, index, tchszConstraint);
    lpFk = (LPFKEYSTRUCT)ListBox_GetItemData (hwndConstraintName, index);
    //
    // Get the key of the current table.
    lpTS = (LPTABLEPARAMS)ESL_AllocMem (sizeof (TABLEPARAMS));
    if (!lpTS)
    {
        ErrorMessage ((UINT)IDS_E_CANNOT_ALLOCATE_MEMORY, RES_ERR);
        return;
    }
    memset (lpTS, 0, sizeof (TABLEPARAMS));
    if (lstrcmpi (tchszParentTbName, tchszSame) == 0)
    {   //
        // Show Primary Key of Referencing Table. The Current table
        //
        if (lpFk && lpFk->tchszRefConstraint[0])
            ShowPrimaryKey (hwnd, (LPTABLEPARAMS)lpTS, NULL, TRUE, lpFk->tchszRefConstraint);
        else
            ShowPrimaryKey (hwnd, (LPTABLEPARAMS)lpTS, NULL, TRUE, NULL);
    }
    else
    {   //
        // Show Primary Key or Unique Key of Referenced Table. (Parent table).
        //
        LPSTR lpCreator = NULL;;
        lstrcpy (lpTS->DBName,         lpTable->DBName);
        lstrcpy (lpTS->objectname,     RemoveDisplayQuotesIfAny((LPTSTR)StringWithoutOwner (tchszParentTbName)));
        index = ComboBox_GetCurSel (hwndReferencedTable);
        if (index == CB_ERR)
            return;
        lpCreator = (LPSTR)ComboBox_GetItemData (hwndReferencedTable, index);
        if (!lpCreator)
            return;
        lstrcpy (lpTS->szSchema,   lpCreator);
        if (lpFk && lpFk->tchszRefConstraint[0])
            ShowPrimaryKey (hwnd, (LPTABLEPARAMS)lpTS, lpVnode, FALSE, lpFk->tchszRefConstraint);
        else
            ShowPrimaryKey (hwnd, (LPTABLEPARAMS)lpTS, lpVnode, FALSE, NULL);
    }
    FreeAttachedPointers (lpTS, OT_TABLE);
    ESL_FreeMem (lpTS);
    UpdateReference (hwnd, lpFk);
}



static void OnTableColumnsSelChange (HWND hwnd)
{
    EnableButton (hwnd);
}


static void OnForeignKeyColumnUp (HWND hwnd)
{
    int   i, index, nCount = 0;
    TCHAR szItem     [MAXOBJECTNAME+1];
    TCHAR szItemCur  [MAXOBJECTNAME+1];
    TCHAR tchszColumn[MAXOBJECTNAME];
    LPFKEYSTRUCT lpFk;
    LPTABLEPARAMS lpTable      = (LPTABLEPARAMS)GetDlgProp (hwnd);
    HWND hwndConstraintName    = GetDlgItem (hwnd, IDC_LIST3);
    HWND hwndPkeyCols = GetDlgItem (hwnd, IDC_LIST2);
    index = (int)ListBox_GetCurSel (hwndPkeyCols);
    if (index == 0 || index == LB_ERR)
        return;
    ListBox_GetText     (hwndPkeyCols, index -1, szItem);
    ListBox_GetText     (hwndPkeyCols, index   , szItemCur);
    ListBox_DeleteString(hwndPkeyCols, index);
    ListBox_InsertString(hwndPkeyCols, index-1,  szItemCur);
    ListBox_SetCurSel   (hwndPkeyCols, index-1);
    index = ListBox_GetCurSel (hwndConstraintName);
    if (index == LB_ERR)
        return;
    //
    // Update the list of referencing columns
    lpFk = (LPFKEYSTRUCT)ListBox_GetItemData (hwndConstraintName, index);
    if (!lpFk)
        return;
    lpFk->l1 = StringList_Done (lpFk->l1);
    nCount = ListBox_GetCount (hwndPkeyCols);
    for (i=0; i<nCount; i++)
    {
        memset (tchszColumn, 0, sizeof (tchszColumn));
        ListBox_GetText (hwndPkeyCols, i, tchszColumn);
        lpFk->l1 = StringList_Add (lpFk->l1, tchszColumn);
    }
}

static void OnForeignKeyColumnDown (HWND hwnd)
{
    int   i, index, nCount = 0;
    TCHAR szItem    [MAXOBJECTNAME+1];
    TCHAR szItemCur [MAXOBJECTNAME+1];
    TCHAR tchszColumn[MAXOBJECTNAME];
    HWND hwndConstraintName   = GetDlgItem (hwnd, IDC_LIST3);
    HWND hwndPkeyCols         = GetDlgItem (hwnd, IDC_LIST2);
    LPFKEYSTRUCT lpFk;

    nCount= ListBox_GetCount (hwndPkeyCols);
    index = (int)ListBox_GetCurSel (hwndPkeyCols);
    if (index == (nCount-1) || index == LB_ERR)
        return;
    ListBox_GetText     (hwndPkeyCols, index +1, szItem);
    ListBox_GetText     (hwndPkeyCols, index   , szItemCur);
    ListBox_DeleteString(hwndPkeyCols, index);
    ListBox_InsertString(hwndPkeyCols, index+1,  szItemCur);
    ListBox_SetCurSel   (hwndPkeyCols, index+1);
    // Update the list of referencing columns
    index = ListBox_GetCurSel (hwndConstraintName);
    if (index == LB_ERR)
        return;
    lpFk = (LPFKEYSTRUCT)ListBox_GetItemData (hwndConstraintName, index);
    if (!lpFk)
        return;
    lpFk->l1 = StringList_Done (lpFk->l1);
    nCount = ListBox_GetCount (hwndPkeyCols);
    for (i=0; i<nCount; i++)
    {
        memset (tchszColumn, 0, sizeof (tchszColumn));
        ListBox_GetText (hwndPkeyCols, i, tchszColumn);
        lpFk->l1 = StringList_Add (lpFk->l1, tchszColumn);
    }
}


static void OnConstraintSelChange   (HWND hwnd)
{
    int   index;
    TCHAR szTab    [] = {' ', ' ', ' ', ' ', '\0'};
    TCHAR szReturn [] = {0x0D, 0x0A, '\0'}; 
    LPSTR lpString = NULL;
    TCHAR tchszConstraintName [MAXOBJECTNAME];
    TCHAR tchszFull1 [MAXOBJECTNAME];
    TCHAR tchszFull2 [MAXOBJECTNAME];
    HWND  hwndConstraintName  = GetDlgItem (hwnd, IDC_LIST3);
    HWND  hwndReferencedTable = GetDlgItem (hwnd, IDC_COMBO2);
    HWND  hwndReferencingCols = GetDlgItem (hwnd, IDC_LIST0);
    HWND  hwndPkeyCols        = GetDlgItem (hwnd, IDC_LIST2);
    HWND  hwndComboPKey       = GetDlgItem (hwnd, IDC_REFERENCES);  // Hidden Control of Primary Key Constraint
    HWND  hwndComboUKey       = GetDlgItem (hwnd, IDC_COMBO3);      // Unique Key Constraint

    LPSTRINGLIST      ls;
    LPOBJECTLIST      listCol = NULL;
    TABLEPARAMS       TS;
    LPFKEYSTRUCT      lpRef;
    LPTABLEPARAMS     lpTable  = (LPTABLEPARAMS)GetDlgProp (hwnd);
    LPTSTR            lpVnode  = (LPTSTR)GetVirtNodeName(GetCurMdiNodeHandle ());
    
    memset (&TS, 0, sizeof (TABLEPARAMS));
    index = ListBox_GetCurSel (hwndConstraintName);
    if (index == LB_ERR)
        return;
    if (index != LB_ERR)
        ListBox_GetText (hwndConstraintName, index, tchszConstraintName);
    if (!tchszConstraintName[0])
        return;
    lpRef = (LPFKEYSTRUCT)ListBox_GetItemData (hwndConstraintName, index);
    //
    // For the existing constraint name, ie from the GetDetailInfo, then grey out
    // the constrol.
    Edit_SetText (GetDlgItem (hwnd, IDC_EDIT2), tchszConstraintName);
    if (VDBA20xForeignKey_Search (lpTable->lpOldReferences, tchszConstraintName))
        EnableWindow (GetDlgItem (hwnd, IDC_EDIT2), FALSE);
    else
        EnableWindow (GetDlgItem (hwnd, IDC_EDIT2), TRUE);
    //
    // Reset the Referenced Table, and (Display its Primary Key or Unique Key)
    //
    StringWithOwner(lpRef->tchszTable,   lpRef->tchszOwner, tchszFull1);
    StringWithOwner(lpTable->objectname, lpTable->szSchema, tchszFull2);
    if (lstrcmpi (lpRef->tchszTable, tchszSame) == 0 || lstrcmpi (tchszFull1, tchszFull2) == 0)
        ComboBox_SelectString (hwndReferencedTable, -1, tchszSame);    
    else
        ComboBoxSelectStringWithOwner (hwndReferencedTable, (LPCTSTR)lpRef->tchszTable, (LPTSTR)lpRef->tchszOwner);
    //
    // Reset the Foreign Key Columns
    //
    ListBox_ResetContent (hwndPkeyCols);
    ls = lpRef->l1;
    while (ls)
    {
        index = ListBox_AddString (hwndPkeyCols, ls->lpszString);
        ls = ls->next;
    }
    //
    // Show the primary key or Unique Key of the parent table.
    //
    index = ComboBox_GetCurSel (hwndReferencedTable);
    StringWithOwner(lpRef->tchszTable,   lpRef->tchszOwner, tchszFull1);
    StringWithOwner(lpTable->objectname, lpTable->szSchema, tchszFull2);
    if (index != CB_ERR && lstrcmpi (lpRef->tchszTable, tchszSame) != 0 && lstrcmpi (tchszFull1, tchszFull2) != 0)
    {
        LPSTR tbCreator = (LPSTR)ComboBox_GetItemData (hwndReferencedTable, index);
        if (tbCreator) lstrcpy (lpRef->tchszOwner, tbCreator);
        memset (&TS, 0, sizeof (TABLEPARAMS));
        lstrcpy (TS.DBName,         lpTable->DBName);
        lstrcpy (TS.objectname,     RemoveDisplayQuotesIfAny((LPTSTR)StringWithoutOwner (lpRef->tchszTable)));
        if (!tbCreator)
            return;
        lstrcpy (TS.szSchema,    tbCreator);
        if (lpRef && lpRef->tchszRefConstraint[0])
            ShowPrimaryKey (hwnd, (LPTABLEPARAMS)&TS, lpVnode, FALSE, lpRef->tchszRefConstraint);
        else
            ShowPrimaryKey (hwnd, (LPTABLEPARAMS)&TS, lpVnode, FALSE, NULL);
    }
    else
    {
        lpRef->tchszOwner [0] = '\0';
        if (lpRef && lpRef->tchszRefConstraint[0])
            ShowPrimaryKey (hwnd, (LPTABLEPARAMS)&TS, NULL, TRUE, lpRef->tchszRefConstraint);
        else
            ShowPrimaryKey (hwnd, (LPTABLEPARAMS)&TS, NULL, TRUE, lpRef->tchszRefConstraint);
    }
    Button_SetCheck (GetDlgItem (hwnd, IDC_CHECK_DELETE_RULE), lpRef->bUseDeleteAction);
    Button_SetCheck (GetDlgItem (hwnd, IDC_CHECK_UPDATE_RULE), lpRef->bUseUpdateAction);
    if (lpRef->tchszDeleteAction[0])
        ComboBox_SelectString (GetDlgItem (hwnd, IDC_COMBO_DELETE_RULE), -1, lpRef->tchszDeleteAction);
    else
        ComboBox_SelectString (GetDlgItem (hwnd, IDC_COMBO_DELETE_RULE), -1, "restrint");

    if (lpRef->tchszUpdateAction[0])
        ComboBox_SelectString (GetDlgItem (hwnd, IDC_COMBO_UPDATE_RULE), -1, lpRef->tchszUpdateAction);
    else
        ComboBox_SelectString (GetDlgItem (hwnd, IDC_COMBO_UPDATE_RULE), -1, "restrint");
    FreeAttachedPointers (&TS, OT_TABLE);
}



static void GetConstraintName (HWND hwndCtrl, LPSTR lptchBuffer)
{
    int   i, nCount, iMax, counter = 0;
    char  tchBuffer [MAXOBJECTNAME+1];
    char  tchBuffer2[MAXOBJECTNAME+1];
    
    nCount = ListBox_GetCount (hwndCtrl);
    for (i=0; i<nCount; i++)
    {
        memset (tchBuffer, 0, sizeof (tchBuffer));
        ListBox_GetText (hwndCtrl, i, tchBuffer);
        if (tchBuffer [0] == '<')
        {
            sscanf (tchBuffer, "%s %d", tchBuffer2, &iMax);
            if (counter < iMax)
                counter = iMax;
        }
    }
    wsprintf (lptchBuffer, ResourceString(IDS_UNAMED_W_NB), counter+1);
}


static void EnableButton (HWND hwnd)
{
    int   index;
    HWND  hwndReferencedTable = GetDlgItem (hwnd, IDC_COMBO2);
    HWND  hwndTbCols          = GetDlgItem (hwnd, IDC_LIST0);
    HWND  hwndPkeyCols        = GetDlgItem (hwnd, IDC_LIST2);
    HWND  hwndComboPKey       = GetDlgItem (hwnd, IDC_REFERENCES);  // Hidden Control of Primary Key Constraint
    HWND  hwndComboUKey       = GetDlgItem (hwnd, IDC_COMBO3);      // Unique Key Constraint
    HWND  hwndConstraintName  = GetDlgItem (hwnd, IDC_LIST3);      // Constraint Name
    LPTABLEPARAMS lpTable     = (LPTABLEPARAMS)GetDlgProp (hwnd);

    // 
    // Button >>
    if (ListBox_GetCurSel (hwndTbCols) == LB_ERR)
        EnableWindow (GetDlgItem (hwnd, IDC_BUTTON_PUT), FALSE);
    else
        EnableWindow (GetDlgItem (hwnd, IDC_BUTTON_PUT), TRUE);
    //
    // Button <<
    if (ListBox_GetCurSel (hwndPkeyCols) == LB_ERR)
        EnableWindow (GetDlgItem (hwnd, IDC_BUTTON_REMOVE), FALSE);
    else
        EnableWindow (GetDlgItem (hwnd, IDC_BUTTON_REMOVE), TRUE);
    //
    // Button Delete
    index = ListBox_GetCurSel (hwndConstraintName);
    if (index == LB_ERR)
    {
        EnableWindow (GetDlgItem (hwnd, IDC_DELETE), FALSE);
        EnableWindow (GetDlgItem (hwnd, IDC_REMOVE_CASCADE), FALSE);
    }
    else
        EnableWindow (GetDlgItem (hwnd, IDC_DELETE), TRUE);
    //
    // Button Del Cascade
    if (index != LB_ERR)
    {
        TCHAR tchszItem [MAXOBJECTNAME];

        memset (tchszItem, 0, sizeof (tchszItem));
        ListBox_GetText (hwndConstraintName, index, tchszItem);
        if (!tchszItem [0])
            return;
        if (VDBA20xForeignKey_Search (lpTable->lpOldReferences, tchszItem))
            EnableWindow (GetDlgItem (hwnd, IDC_REMOVE_CASCADE), TRUE);
        else
            EnableWindow (GetDlgItem (hwnd, IDC_REMOVE_CASCADE), FALSE);
    }
    index = ListBox_GetCurSel (hwndConstraintName);
    if (index == LB_ERR || ListBox_GetCount (hwndConstraintName) == 0)
    {
        // 
        // Grey out the controls
        EnableWindow (GetDlgItem (hwnd, IDC_EDIT2) , FALSE);
        EnableWindow (GetDlgItem (hwnd, IDC_RADIO1), FALSE);
        EnableWindow (GetDlgItem (hwnd, IDC_RADIO2), FALSE);
        EnableWindow (hwndTbCols,          FALSE);
        EnableWindow (hwndPkeyCols,        FALSE);
        EnableWindow (hwndComboUKey,       FALSE);
        EnableWindow (hwndReferencedTable, FALSE);
        EnableWindow (GetDlgItem (hwnd, IDC_EDIT2),  FALSE);
    }
    else
    if (index >= 0)
    {
        // 
        // Enable the controls
        LPFKEYSTRUCT lpFkStruct = NULL;
        lpFkStruct = (LPFKEYSTRUCT)ListBox_GetItemData (hwndConstraintName, index);
        EnableWindow (GetDlgItem (hwnd, IDC_EDIT2), TRUE);
        EnableWindow (GetDlgItem (hwnd, IDC_RADIO1),TRUE);
        EnableWindow (GetDlgItem (hwnd, IDC_RADIO2),TRUE);
        EnableWindow (hwndTbCols,          TRUE);
        EnableWindow (hwndPkeyCols,        TRUE);
        index = ComboBox_GetCurSel (hwndReferencedTable);

        if (Button_GetCheck (GetDlgItem (hwnd, IDC_RADIO2)) && 
            index != CB_ERR && 
            ComboBox_GetLBTextLen (hwndReferencedTable, index) > 0)
            EnableWindow (hwndComboUKey,       TRUE);
        else
            EnableWindow (hwndComboUKey,       FALSE);
        EnableWindow (hwndReferencedTable, TRUE);
        if (lpFkStruct && (lpFkStruct->nOld > 0 || lpFkStruct->bAlter))
        {
            EnableWindow (GetDlgItem (hwnd, IDC_EDIT2), FALSE);
            SetDisplayMode (hwnd, TRUE);
        }
        else
        {
            EnableWindow (GetDlgItem (hwnd, IDC_EDIT2), TRUE);
            SetDisplayMode (hwnd, FALSE);
        }
    }
}


static void FormatColumn (LPSTRINGLIST lpCol, LPTSTR lpszBufferCol)
{
    switch (lpCol->extraInfo1)
    {
    case INGTYPE_C:
        wsprintf (lpszBufferCol, "c (%ld)", lpCol->extraInfo2);
        break;
    case INGTYPE_CHAR:
        wsprintf (lpszBufferCol, "char (%ld)", lpCol->extraInfo2);
        break;
    case INGTYPE_TEXT:
        wsprintf (lpszBufferCol, "text (%ld)", lpCol->extraInfo2);
        break;
    case INGTYPE_VARCHAR:
        wsprintf (lpszBufferCol, "varchar (%ld)", lpCol->extraInfo2);
        break;
    case INGTYPE_LONGVARCHAR:
        lstrcpy (lpszBufferCol, "long varchar");
        break;
    case INGTYPE_BIGINT:
        lstrcpy (lpszBufferCol, "bigint");
        break;
    case INGTYPE_INT8:
        lstrcpy (lpszBufferCol, "int8");
        break;
    case INGTYPE_INT4:
        lstrcpy (lpszBufferCol, "int");
        break;
    case INGTYPE_INT2:
        lstrcpy (lpszBufferCol, "smallint");
        break;
    case INGTYPE_INT1: 
        lstrcpy (lpszBufferCol, "integer1");
        break;
    case INGTYPE_DECIMAL: 
        wsprintf (lpszBufferCol, "decimal (%d, %d)", lpCol->extraInfo2, lpCol->extraInfo3);
        break;
    case INGTYPE_FLOAT8:  
        lstrcpy (lpszBufferCol, "float");
        break;
    case INGTYPE_FLOAT4: 
        lstrcpy (lpszBufferCol, "real");
        break;
    case INGTYPE_DATE:  
        lstrcpy (lpszBufferCol, "date");     // 116835: changed from "inresdate"
        break;
    case INGTYPE_ADTE:  
        lstrcpy (lpszBufferCol, "ansidate");     // 116835
        break;
    case INGTYPE_TMWO:  
        lstrcpy (lpszBufferCol, "time without time zone");     // 116835
        break;
    case INGTYPE_TMW:  
        lstrcpy (lpszBufferCol, "time with time zone");     // 116835
        break;
    case INGTYPE_TME:  
        lstrcpy (lpszBufferCol, "time with local time zone");     // 116835
        break;
    case INGTYPE_TSWO:  
        lstrcpy (lpszBufferCol, "timestamp without time zone");     // 116835
        break;
    case INGTYPE_TSW:  
        lstrcpy (lpszBufferCol, "timestamp with time zone");     // 116835
        break;
    case INGTYPE_TSTMP:  
        lstrcpy (lpszBufferCol, "timestamp with local time zone");     // 116835
        break;
    case INGTYPE_INYM:  
        lstrcpy (lpszBufferCol, "interval year to month");     // 116835
        break;
    case INGTYPE_INDS:  
        lstrcpy (lpszBufferCol, "interval day to second");     // 116835
        break;
    case INGTYPE_IDTE:  
        lstrcpy (lpszBufferCol, "ingresdate");     // 116835
        break;
    case INGTYPE_MONEY: 
        lstrcpy (lpszBufferCol, "money");
        break;
    case INGTYPE_BYTE:
        wsprintf (lpszBufferCol, "byte (%ld)", lpCol->extraInfo2);
        break;
    case INGTYPE_BYTEVAR:
        lstrcpy (lpszBufferCol, "byte varying");
        break;
    case INGTYPE_LONGBYTE: 
        lstrcpy (lpszBufferCol, "long byte");
        break;
    case INGTYPE_OBJKEY: 
        lstrcpy (lpszBufferCol, "object_key");
        break;
    case INGTYPE_TABLEKEY:
        lstrcpy (lpszBufferCol, "table_key");
        break;
    case INGTYPE_FLOAT: 
        lstrcpy (lpszBufferCol, "float");
        break;
    case INGTYPE_UNICODE_NCHR:
        wsprintf (lpszBufferCol, "nchar (%ld)", lpCol->extraInfo2);
        break;
    case INGTYPE_UNICODE_NVCHR:
        wsprintf (lpszBufferCol, "nvarchar (%ld)", lpCol->extraInfo2);
        break;
    case INGTYPE_UNICODE_LNVCHR:
        lstrcpy (lpszBufferCol, "long nvarchar");
        break;

    }
}

static void ShowPrimaryKey (HWND hwnd, LPTABLEPARAMS lpTS, LPTSTR lpVnode, BOOL bSameTable, LPTSTR lpKey)
{
    int   m, index;
    int   m_Radio     = -1;
    HWND  hwndParent  = GetParent (hwnd);
    BOOL  isOneColumn = TRUE;
    TCHAR szTab    [] = {' ', ' ', ' ', ' ', '\0'};
    TCHAR szReturn [] = {0x0D, 0x0A, '\0'}; 
    TCHAR szKeyName[MAXOBJECTNAME];
    TCHAR tchszColFormat [32];
    HWND  hwndReferencedPKey   = GetDlgItem (hwnd, IDC_EDIT1);
    HWND  hwndComboPKey        = GetDlgItem (hwnd, IDC_REFERENCES);  // Hidden Control of Primary Key Constraint
    HWND  hwndComboUKey        = GetDlgItem (hwnd, IDC_COMBO3);      // Unique Key Constraint

    LPSTR lpString    = NULL;
    LPSTRINGLIST   tmp, lx;
    LPTLCUNIQUE    ls = NULL;
    LPTABLEPARAMS  lpTable = (LPTABLEPARAMS)GetDlgProp (hwnd);

    lstrcpy (szKeyName, "Primary Key = ");
    if (!bSameTable)
    {
        if (!VDBA20xGetDetailPKeyInfo (lpTS, lpVnode))
        {
            // tchszMsg7
            MessageBox (hwnd, ResourceString ((UINT)IDS_E_FAILED_TOGETPKEY), NULL, MB_ICONEXCLAMATION|MB_OK);
            // tchszMsg1
            Edit_SetText (hwndReferencedPKey, ResourceString ((UINT)IDS_E_NO_PKEY_NOR_UKEYS));
            FreeAttachedPointers (lpTS, OT_TABLE);
            return;

        }
        if (!VDBA20xGetDetailUKeyInfo (lpTS, lpVnode))
        {
            // tchszMsg8
            MessageBox (hwnd, ResourceString ((UINT)IDS_E_FAILED_TOGETUKEYS), NULL, MB_ICONEXCLAMATION|MB_OK);
            // tchszMsg1
            Edit_SetText (hwndReferencedPKey, ResourceString ((UINT)IDS_E_NO_PKEY_NOR_UKEYS));
            FreeAttachedPointers (lpTS, OT_TABLE);
            return;
        }
        if (!(lpTS->lpUnique || lpTS->lpPrimaryColumns))
        {
            // tchszMsg1
            Edit_SetText (hwndReferencedPKey, ResourceString ((UINT)IDS_E_NO_PKEY_NOR_UKEYS));
            FreeAttachedPointers (lpTS, OT_TABLE);
        }
        VDBA20xComboKey_CleanUp (hwndComboPKey);
        VDBA20xComboKey_CleanUp (hwndComboUKey);
        //
        // Fill up the Primary Key ComboBox.
        ComboBox_SetCurSel (hwndComboPKey, -1);
        index = ComboBox_AddString (hwndComboPKey, lpTS->tchszPrimaryKeyName);
        if (index != CB_ERR)
        {
            ComboBox_SetItemData (hwndComboPKey, index, lpTS->lpPrimaryColumns);
            ComboBox_SetCurSel   (hwndComboPKey, index);
        }
        lpTS->lpPrimaryColumns = NULL;
        //
        // Fill up the Unique Key ComboBox.
        ls = lpTS->lpUnique;
        while (ls)
        {
            index = ComboBox_AddString (hwndComboUKey, ls->tchszConstraint);
            if (index != CB_ERR)
            {
                ComboBox_SetItemData (hwndComboUKey, index, ls->lpcolumnlist);
                ls->lpcolumnlist = NULL;
            }
            ls = ls->next;
        }
        ComboBox_SetCurSel (hwndComboUKey, 0);
    }
    else // The same table.
    {
        int   i, nCount, nNumber = 0;
        BOOL  bFound = FALSE;
        TCHAR tchszItem  [MAXOBJECTNAME];
        TCHAR tchszUname [32];
        LPOBJECTLIST   lplist;
        LPCOLUMNPARAMS lpcp;
        LPSTRINGLIST   lpk = MakePrimaryKey (lpTable->lpColumns);
        LPTLCUNIQUE    l   = lpTable->lpUnique;   // Table Level Unique Constraints
        while (l)
        {
            index = ComboBox_AddString (hwndComboUKey, l->tchszConstraint);
            if (index != CB_ERR)
            {
                LPSTRINGLIST lpu;

                lpu = StringList_Copy (l->lpcolumnlist);
                ComboBox_SetItemData (hwndComboUKey, index, lpu);
            }
            l = l->next;
        }
        ComboBox_SetCurSel (hwndComboUKey, 0);
        //
        // Prepare the index for <Unnamed 1>, <Unamed 2>, ...
        nCount = ComboBox_GetCount (hwndComboUKey);
        for (i=0; i<nCount; i++)
        {
            memset (tchszItem, 0, sizeof (tchszItem));
            ComboBox_GetLBText (hwndComboUKey, i, tchszItem);
            if (tchszItem [0] && tchszItem[0] == '<')
                nNumber++;
        }
        lplist = lpTable->lpColumns;    
        //
        // Column Level Unique Constraints of the current table.
        // The column level unique constraints will be taken in consideration only if
        // they are not appeared in the table level unique constraint, ie are not in
        // the Unique Sub-Dialog Box.
        while (lplist)
        {
            lpcp  = (LPCOLUMNPARAMS)lplist->lpObject;
            l     = lpTable->lpUnique; 
            bFound= FALSE;
            while (l)
            {
                if (StringList_Count (l->lpcolumnlist) == 1 && lstrcmpi ((l->lpcolumnlist)->lpszString, lpcp->szColumn) == 0)
                {
                    bFound = TRUE;
                    break;
                }
                l = l->next;
            }
            if (lpcp->bUnique && !bFound)
            {
                LPSTRINGLIST lpstringlist = NULL, lpStr = NULL;
                nNumber++;
                wsprintf (tchszUname, ResourceString(IDS_UNAMED_W_NB), nNumber);
                lpstringlist = StringList_AddObject (lpstringlist, lpcp->szColumn, &lpStr);
                if (!lpstringlist)
                {
                    ErrorMessage ((UINT)IDS_E_CANNOT_ALLOCATE_MEMORY, RES_ERR);
                    return;
                }
                lpStr->extraInfo1 = (int)lpcp->uDataType;
                lpStr->extraInfo2 = (lpcp->uDataType == INGTYPE_DECIMAL)? lpcp->lenInfo.decInfo.nPrecision:lpcp->lenInfo.dwDataLen;
                lpStr->extraInfo3 = (lpcp->uDataType == INGTYPE_DECIMAL)? lpcp->lenInfo.decInfo.nScale: 0;
                index = ComboBox_AddString (hwndComboUKey, tchszUname);
                if (index != CB_ERR)
                {
                    ComboBox_SetItemData (hwndComboUKey, index, lpstringlist);
                }
            }
            lplist = lplist->lpNext;
        }
        if (lpk)
        {
            index = ComboBox_AddString (hwndComboPKey, "<Unnamed>");
            if (index >= 0)
                ComboBox_SetItemData (hwndComboPKey, index, lpk);
        }
    }
    if (lpKey)
    {
        index = ComboBox_FindStringExact (hwndComboPKey, -1, lpKey);
        if (index != CB_ERR)
        {
            CheckRadioButton (hwnd, IDC_RADIO1, IDC_RADIO2, IDC_RADIO1);
            ComboBox_SetCurSel (hwndComboPKey, index);
            EnableWindow (hwndComboUKey, FALSE);        // Disable Combo of Unique Keys
        }
        else
        {
            index = ComboBox_FindStringExact (hwndComboUKey, -1, lpKey);
            if (index != CB_ERR)
            {
                CheckRadioButton (hwnd, IDC_RADIO1, IDC_RADIO2, IDC_RADIO2);
                ComboBox_SetCurSel (hwndComboUKey, index);
                EnableWindow (hwndComboUKey, TRUE);     // Enable Combo of Unique Keys
            }
        }
    }
    else
    {
        //
        // In this case we do not select the referenced constraint name.
        ComboBox_SetCurSel (hwndComboPKey, 0);
        ComboBox_SetCurSel (hwndComboUKey, 0);
    }
    //
    // Display Information about the Primary Key or Unique Key.
    if (Button_GetCheck (GetDlgItem (hwnd, IDC_RADIO1)))
    {
        m_Radio = IDC_RADIO1;
        //if (bSameTable)
        //    lx = MakePrimaryKey (lpTable->lpColumns);
        //else
        {
            index = ComboBox_GetCurSel (hwndComboPKey);
            if (index == CB_ERR)
                lx = NULL;
            else
            if (bSameTable)
                lx = StringList_Copy ((LPSTRINGLIST)ComboBox_GetItemData (hwndComboPKey, index));
            else
                lx = (LPSTRINGLIST)ComboBox_GetItemData (hwndComboPKey, index);
        }
        lstrcpy (szKeyName, "Primary Key = ");
    }
    else
    if (Button_GetCheck (GetDlgItem (hwnd, IDC_RADIO2)))
    {
        m_Radio = IDC_RADIO2;
        //
        //if (bSameTable)
        //    lx = MakeUniqueKey (lpTable->lpUnique, lpKey);
        //else
        {
            lx = NULL;
            index = ComboBox_GetCurSel (hwndComboUKey);
            if (index == CB_ERR)
                lx = NULL;
            else
            if (bSameTable)
                lx = StringList_Copy ((LPSTRINGLIST)ComboBox_GetItemData (hwndComboUKey, index));
            else
                lx = (LPSTRINGLIST)ComboBox_GetItemData (hwndComboUKey, index);
        }
        lstrcpy (szKeyName, "Unique Key = ");
    }
    tmp = lx;
    while (tmp)
    {
        if (isOneColumn)
        {
            m = ConcateStrings (&lpString, szKeyName,  "(", (LPSTR)0);
            if (!m) 
            {
                if (bSameTable) lx = StringList_Done (lx);
                return;
            }
        }
        else
        {
            m = ConcateStrings (&lpString, ", ", (LPSTR)0);
            if (!m) 
            {
                if (bSameTable) lx = StringList_Done (lx);
                return;
            }
        }
        memset (tchszColFormat, 0, sizeof (tchszColFormat));
        FormatColumn (tmp, tchszColFormat);
        m = ConcateStrings (&lpString, tmp->lpszString, " ", tchszColFormat, (LPSTR)0);
        isOneColumn = FALSE;
        if (!m) return;
        tmp = tmp->next;
    }
    if (lx)
    {
        m = ConcateStrings (&lpString, ")", (LPSTR)0);
        if (!m)
        {       
            if (bSameTable)  lx = StringList_Done (lx);
            return;
        }
    }
    if (bSameTable)
        lx = StringList_Done (lx);
    if (lpString)
    {
        Edit_SetText (hwndReferencedPKey, lpString);
        ESL_FreeMem (lpString);
    }
    else
    {
        if (m_Radio == IDC_RADIO1)
            // tchszMsg2
            Edit_SetText (hwndReferencedPKey, ResourceString ((UINT)IDS_E_NO_PKEY));
        else
            // tchszMsg3
            Edit_SetText (hwndReferencedPKey, ResourceString ((UINT)IDS_E_NO_UKEYS));
    }
}

// TODO:
// We use the extra info field:
// SHORT extraInfo1 -> data type
// LONG  extraInfo2 -> data type length if the type requires it
static LPSTRINGLIST MakePrimaryKey (LPOBJECTLIST lpColList)
{
    LPCOLUMNPARAMS lpCol;
    LPSTRINGLIST list = NULL, lpStr = NULL;
    LPOBJECTLIST ls   = lpColList;
   
    while (ls)
    {
        lpCol = (LPCOLUMNPARAMS)ls->lpObject;
        if (lpCol->nPrimary)
        {
            list = StringList_AddObject (list, lpCol->szColumn, &lpStr);
            if (!list)
            {
                ErrorMessage ((UINT)IDS_E_CANNOT_ALLOCATE_MEMORY, RES_ERR);
                return NULL;
            }
            lpStr->extraInfo1 = (int)lpCol->uDataType;
            lpStr->extraInfo2 = (lpCol->uDataType == INGTYPE_DECIMAL)? lpCol->lenInfo.decInfo.nPrecision:lpCol->lenInfo.dwDataLen;
            lpStr->extraInfo3 = (lpCol->uDataType == INGTYPE_DECIMAL)? lpCol->lenInfo.decInfo.nScale: 0;
        }
        ls = ls->lpNext;
    }
    return list;
}


static void OnPrimaryKeyRadioClick  (HWND hwnd)
{
    int   m, index;
    TCHAR szTab    [] = {' ', ' ', ' ', ' ', '\0'};
    TCHAR szReturn [] = {0x0D, 0x0A, '\0'}; 
    TCHAR tchszTable[MAXOBJECTNAME];
    TCHAR tchszColFormat [32];
    BOOL  isOneColumn = TRUE, bSame = FALSE;
    HWND  hwndReferencedPKey   = GetDlgItem (hwnd, IDC_EDIT1);
    HWND  hwndComboPKey        = GetDlgItem (hwnd, IDC_REFERENCES);  // Hidden Control of Primary Key Constraint
    HWND  hwndReferencedTable  = GetDlgItem (hwnd, IDC_COMBO2);
    HWND  hwndConstraintName   = GetDlgItem (hwnd, IDC_LIST3);
    LPTABLEPARAMS  lpTable     = (LPTABLEPARAMS)GetDlgProp (hwnd);
    LPFKEYSTRUCT   lpFk;
    LPSTRINGLIST   ls, lx;
    LPTSTR lpString = NULL;

    EnableWindow (GetDlgItem (hwnd, IDC_COMBO3), FALSE);            // Disable Combo of Unique Keys
    ComboBox_SetCurSel (hwndComboPKey, 0);
    memset (tchszTable, 0, sizeof (tchszTable));
    ComboBox_GetText (hwndReferencedTable, tchszTable, sizeof (tchszTable));
    if (!tchszTable[0])
    {
        // tchszMsg2
        Edit_SetText (hwndReferencedPKey, ResourceString ((UINT)IDS_E_NO_PKEY));
        return;
    }
    bSame = lstrcmpi (tchszSame, tchszTable) == 0;
    //if (bSame)
    //    ls = MakePrimaryKey (lpTable->lpColumns);
    //else
    //    ls = (LPSTRINGLIST)ComboBox_GetItemData (hwndComboPKey, 0);

    index = ComboBox_GetCurSel (hwndComboPKey);
    if (index == CB_ERR || GetWindowTextLength (hwndComboPKey) <= 0)
    {
        // tchszMsg2
        Edit_SetText (hwndReferencedPKey, ResourceString ((UINT)IDS_E_NO_PKEY));
        return;
    }
    ls = (LPSTRINGLIST)ComboBox_GetItemData (hwndComboPKey, 0);
    if (!ls)
    {
        // tchszMsg2
        Edit_SetText (hwndReferencedPKey, ResourceString ((UINT)IDS_E_NO_PKEY));
        return;
    }
    //
    // Display the Primary Key Info.
    lx = ls;
    while (ls)
    {
        if (isOneColumn)
        {
            m = ConcateStrings (&lpString, "Primary Key = (", (LPSTR)0);
            if (!m) 
            {
                if (bSame) lx = StringList_Done (lx);
                return;
            }
        }
        else
        {
            m = ConcateStrings (&lpString, ", ", (LPSTR)0);
            if (!m) 
            {
                if (bSame) lx = StringList_Done (lx);
                return;
            }
        }
        memset (tchszColFormat, 0, sizeof (tchszColFormat));
        FormatColumn (ls, tchszColFormat);
        m = ConcateStrings (&lpString, ls->lpszString, " ", tchszColFormat, (LPSTR)0);
        isOneColumn = FALSE;
        if (!m) return;
        ls = ls->next;
    }
    if (lx)
    {
        m = ConcateStrings (&lpString, ")", (LPSTR)0);
        if (!m)
        {       
            if (bSame)  lx = StringList_Done (lx);
            return;
        }
    }
//    if (bSame)
//        lx = StringList_Done (lx);
    Edit_SetText (hwndReferencedPKey, lpString);
    if (lpString)
        ESL_FreeMem (lpString);
    //
    // Update Data 
    index = ListBox_GetCurSel (hwndConstraintName);
    if (index == LB_ERR)
        return;
    lpFk = (LPFKEYSTRUCT)ListBox_GetItemData (hwndConstraintName, index);
    if (!lpFk)
        return;
    UpdateReference (hwnd, lpFk);
}

static void OnUniqueKeyRadioClick   (HWND hwnd)
{
    int   m, index;
    TCHAR szTab    [] = {' ', ' ', ' ', ' ', '\0'};
    TCHAR szReturn [] = {0x0D, 0x0A, '\0'}; 
    TCHAR tchszTable[MAXOBJECTNAME];
    TCHAR tchszColFormat [32];
    BOOL  isOneColumn = TRUE, bSame = FALSE;
    HWND  hwndConstraintName   = GetDlgItem (hwnd, IDC_LIST3);
    HWND  hwndReferencedPKey   = GetDlgItem (hwnd, IDC_EDIT1);
    HWND  hwndReferencedTable  = GetDlgItem (hwnd, IDC_COMBO2);
    HWND  hwndComboUKey        = GetDlgItem (hwnd, IDC_COMBO3);
    LPTABLEPARAMS  lpTable     = (LPTABLEPARAMS)GetDlgProp (hwnd);
    LPFKEYSTRUCT   lpFk;
    LPSTRINGLIST   ls, lx;
    LPTSTR lpString = NULL;

    EnableWindow (hwndComboUKey, TRUE);     // Enable Combo of Unique Keys
    index = ComboBox_GetCurSel (hwndComboUKey);
    if (index == CB_ERR || GetWindowTextLength (hwndComboUKey) <= 0)
    {
        // tchszMsg3
        Edit_SetText (hwndReferencedPKey, ResourceString ((UINT)IDS_E_NO_UKEYS));
        return;
    }
    ComboBox_GetText (hwndReferencedTable, tchszTable, sizeof (tchszTable));
    ls = (LPSTRINGLIST)ComboBox_GetItemData (hwndComboUKey, index);
    //
    // Display the Unique Key Info.
    lx = ls;
    while (ls)
    {
        if (isOneColumn)
        {
            m = ConcateStrings (&lpString, "Unique Key = (",  (LPSTR)0);
            if (!m) 
            {
  //              if (bSame) lx = StringList_Done (lx);
                return;
            }
        }
        else
        {
            m = ConcateStrings (&lpString, ", ", (LPSTR)0);
            if (!m) 
            {
  //              if (bSame) lx = StringList_Done (lx);
                return;
            }
        }
        memset (tchszColFormat, 0, sizeof (tchszColFormat));
        FormatColumn (ls, tchszColFormat);
        m = ConcateStrings (&lpString, ls->lpszString, " ", tchszColFormat, (LPSTR)0);
        isOneColumn = FALSE;
        if (!m) 
        {
  //          if (bSame) lx = StringList_Done (lx);
            return;
        }
        ls = ls->next;
    }
    if (lx)
    {
        m = ConcateStrings (&lpString, ")", (LPSTR)0);
        if (!m)
        {       
//          if (bSame)  lx = StringList_Done (lx);
            return;
        }
    }
//    if (bSame)
//        lx = StringList_Done (lx);
    Edit_SetText (hwndReferencedPKey, lpString);
    ESL_FreeMem (lpString);
    //
    // Update Data 
    index = ListBox_GetCurSel (hwndConstraintName);
    if (index == LB_ERR)
        return;
    lpFk = (LPFKEYSTRUCT)ListBox_GetItemData (hwndConstraintName, index);
    if (!lpFk)
        return;
    UpdateReference (hwnd, lpFk);
}


static void VDBA20xComboKey_CleanUp (HWND hwndCtrl)
{
    LPSTRINGLIST lpList;
    int i, nCount = ComboBox_GetCount (hwndCtrl);
    for (i=0; i<nCount; i++)
    {
        lpList = (LPSTRINGLIST)ComboBox_GetItemData (hwndCtrl, i);
        lpList = StringList_Done (lpList);
    }
    ComboBox_ResetContent (hwndCtrl);
    ComboBox_SetCurSel (hwndCtrl, -1);
}

// TODO:
// We use the extra info field:
// SHORT extraInfo1 -> data type
// LONG  extraInfo2 -> data type length if the type requires it
static LPSTRINGLIST MakeUniqueKey (LPTLCUNIQUE  lpUnique, LPTSTR tchszUniqueKey)
{
    LPTLCUNIQUE ls = lpUnique;
    LPSTRINGLIST list = NULL, lx, lpStr = NULL;

    while (ls)
    {
        if (lstrcmpi (ls->tchszConstraint, tchszUniqueKey) == 0)
        {
            lx = ls->lpcolumnlist;
            while (lx)
            {
                list = StringList_AddObject (list, lx->lpszString, &lpStr);
                if (!list)
                {
                    ErrorMessage ((UINT)IDS_E_CANNOT_ALLOCATE_MEMORY, RES_ERR);
                    return NULL;
                }
                lpStr->extraInfo1 = lx->extraInfo1;
                lpStr->extraInfo2 = lx->extraInfo2;
                lpStr->extraInfo3 = lx->extraInfo3;
                lx   = lx->next;
            }
            return list;
        }
        ls = ls->next;
    }
    if (lpUnique) // Return the first unique.
    {
        LPSTRINGLIST list = NULL, lx;
        lx = lpUnique->lpcolumnlist;
        while (lx)
        {
            list = StringList_AddObject (list, lx->lpszString, &lpStr);
            if (!list)
            {
                ErrorMessage ((UINT)IDS_E_CANNOT_ALLOCATE_MEMORY, RES_ERR);
                return NULL;
            }
            lpStr->extraInfo1 = lx->extraInfo1;
            lpStr->extraInfo2 = lx->extraInfo2;
            lpStr->extraInfo3 = lx->extraInfo3;
            lx   = lx->next;
        }
        return list;
    }
    return NULL;
}

//
// Check the current defintion of reference.
// If it is different from the one in the list, then update the one in the list.
// 
static BOOL UpdateReference (HWND hwnd, LPFKEYSTRUCT lpFk)
{
    int   index, i, nCount, m_Radio;
    HWND  hwndConstraintName   = GetDlgItem (hwnd, IDC_LIST3);
    LPTABLEPARAMS     lpTable  = (LPTABLEPARAMS)GetDlgProp (hwnd);
    LPSTRINGLIST lx = NULL;
    TCHAR tchszTable  [128];
    TCHAR tchszColumn [MAXOBJECTNAME];
    HWND  hwndPkeyCols         = GetDlgItem (hwnd, IDC_LIST2);
    HWND  hwndReferencedTable  = GetDlgItem (hwnd, IDC_COMBO2);
    HWND  hwndReferencedPKey   = GetDlgItem (hwnd, IDC_EDIT1);
    HWND  hwndComboPKey        = GetDlgItem (hwnd, IDC_REFERENCES);  // Hidden Control of Primary Key Constraint
    HWND  hwndComboUKey        = GetDlgItem (hwnd, IDC_COMBO3);      // Unique Key Constraint

    if (!lpFk)
        return FALSE;
    
    memset (tchszTable, 0, sizeof (tchszTable));
    index = ComboBox_GetCurSel (hwndReferencedTable);
    if (index != CB_ERR)
        ComboBox_GetLBText (hwndReferencedTable, index, tchszTable);
    //
    // Update the list of referencing columns
    lpFk->l1 = StringList_Done (lpFk->l1);
    nCount = ListBox_GetCount (hwndPkeyCols);
    for (i=0; i<nCount; i++)
    {
        memset (tchszColumn, 0, sizeof (tchszColumn));
        ListBox_GetText (hwndPkeyCols, i, tchszColumn);
        lpFk->l1 = StringList_Add (lpFk->l1, tchszColumn);
    }
    //
    // Update the list of referenced columns
    if (!tchszTable [0])
        return FALSE;
    lpFk->l2 = StringList_Done (lpFk->l2);
    if (Button_GetCheck (GetDlgItem (hwnd, IDC_RADIO1)))    // Primary Key
    {
        m_Radio = IDC_RADIO1;
        //if (lstrcmpi (tchszTable, tchszSame) == 0)
        //{
        //    lx = MakePrimaryKey (lpTable->lpColumns);
        //    lpFk->l2 = StringList_Copy (lx);
        //    StringList_Done (lx);
        //}
        //else
        {
            index = ComboBox_GetCurSel (hwndComboPKey);
            if (index == CB_ERR)
                lx = NULL;
            else
                lx = (LPSTRINGLIST)ComboBox_GetItemData (hwndComboPKey, index);
            lpFk->l2 = StringList_Copy (lx);
        }
    }
    else
    if (Button_GetCheck (GetDlgItem (hwnd, IDC_RADIO2)))    // Unique Key
    {
        TCHAR tchszKey [MAXOBJECTNAME];
        m_Radio = IDC_RADIO2;
        memset (tchszKey, 0, sizeof(tchszKey));
        index = ComboBox_GetCurSel (hwndComboUKey);
        if (index != CB_ERR)
            ComboBox_GetLBText (hwndComboUKey, index, tchszKey);
        //if (lstrcmpi (tchszTable, tchszSame) == 0)
        //{
        //    lx = MakeUniqueKey (lpTable->lpUnique, tchszKey);
        //    lpFk->l2 = StringList_Copy (lx);
        //    StringList_Done (lx);
        //}
        //else
        {
            index = ComboBox_GetCurSel (hwndComboUKey);
            if (index == CB_ERR)
                lx = NULL;
            else
                lx = (LPSTRINGLIST)ComboBox_GetItemData (hwndComboUKey, index);
            lpFk->l2 = StringList_Copy (lx);
        }
    }
    //
    // Update the parent table name (Owner & Table)
    index =ComboBox_GetCurSel (hwndReferencedTable);
    if (index == CB_ERR)
        lstrcpy (lpFk->tchszTable, "");
    else
    {
        LPTSTR lpOwner;
        ComboBox_GetLBText (hwndReferencedTable, index, tchszTable);
        lstrcpy (lpFk->tchszTable, RemoveDisplayQuotesIfAny((LPTSTR)StringWithoutOwner (tchszTable)));
        lpOwner = (LPTSTR)ComboBox_GetItemData (hwndReferencedTable, index);
        if (lpOwner && lpOwner [0] != '<')
            lstrcpy (lpFk->tchszOwner, lpOwner);
    }

    //
    // 2.6 Reference key actions (on delete | update)
    lpFk->bUseDeleteAction = Button_GetCheck (GetDlgItem (hwnd, IDC_CHECK_DELETE_RULE));
    lpFk->bUseUpdateAction = Button_GetCheck (GetDlgItem (hwnd, IDC_CHECK_UPDATE_RULE));
    index = ComboBox_GetCurSel (GetDlgItem (hwnd, IDC_COMBO_DELETE_RULE));
    if (index != CB_ERR)
        ComboBox_GetLBText (GetDlgItem (hwnd, IDC_COMBO_DELETE_RULE), index, lpFk->tchszDeleteAction);
    index = ComboBox_GetCurSel (GetDlgItem (hwnd, IDC_COMBO_UPDATE_RULE));
    if (index != CB_ERR)
        ComboBox_GetLBText (GetDlgItem (hwnd, IDC_COMBO_UPDATE_RULE), index, lpFk->tchszUpdateAction);
    return TRUE;
}

static void OnUniqueKeySelChange (HWND hwnd)
{
    OnUniqueKeyRadioClick (hwnd);
}

static BOOL DataValidation (HWND hwnd)
{
    int   i, j, nCount = 0;
    int   c1, c2;
    HWND  hwndConstraintName    = GetDlgItem (hwnd, IDC_LIST3);
    LPTABLEPARAMS     lpTable   = (LPTABLEPARAMS)GetDlgProp (hwnd);
    LPFKEYSTRUCT lpFk, lpExist;

    nCount = ListBox_GetCount (hwndConstraintName);
    if (nCount <= 0)
        return TRUE;
    for (i=0; i<nCount; i++)
    {
        lpFk = (LPFKEYSTRUCT)ListBox_GetItemData (hwndConstraintName, i);        
        //
        // Check the duplicated definition.
        for (j=i+1; j<nCount; j++)
        {
            lpExist = (LPFKEYSTRUCT)ListBox_GetItemData (hwndConstraintName, j);        
            c1 = StringList_Count (lpFk->l1); 
            c2 = StringList_Count (lpExist->l1);
            /*
            if (c1 == 1)
            {
                LPCOLUMNPARAMS lpC;
                LPOBJECTLIST lplist = lpTable->lpColumns;
                while (lplist)
                {
                    lpC = (LPCOLUMNPARAMS)lplist->lpObject;
                    if (lpC->bUnique && lstrcmpi (lpC->szColumn, (lpFk->l1)->lpszString) == 0)
                    {
                        // tchszMsg10
                        MessageBox (hwnd, ResourceString ((UINT)IDS_E_DUPLICATE_CONSTRAINT2), "Error", MB_ICONEXCLAMATION|MB_OK);
                        ListBox_SetCurSel (hwndConstraintName, i);
                        OnConstraintSelChange (hwnd);
                        return FALSE;
                    }
                    lplist = lplist->lpNext;
                }
            }
            */
            if (c1 == c2)
            {
                BOOL isEqual = TRUE;
                LPSTRINGLIST l1 = lpFk->l1;
                LPSTRINGLIST l2 = lpExist->l1;
                while (l1 && l2)
                {
                    if (!StringList_Search (lpFk->l1, l2->lpszString))
                    {
                        isEqual = FALSE;
                        break;
                    }
                    if (!StringList_Search (lpExist->l1, l1->lpszString))
                    {
                        isEqual = FALSE;
                        break;
                    }
                    l1 = l1->next;
                    l2 = l2->next;
                }
                if (isEqual && lstrcmpi (lpFk->tchszRefConstraint, lpExist->tchszRefConstraint) == 0)
                {
                   // tchszMsg10
                    MessageBox (hwnd, ResourceString ((UINT)IDS_E_DUPLICATE_CONSTRAINT2), NULL, MB_ICONEXCLAMATION|MB_OK);
                    ListBox_SetCurSel (hwndConstraintName, j);
                    OnConstraintSelChange (hwnd);
                    return FALSE;
                }
            }
        }
        if (!(lpFk->l2 && lpFk->tchszTable[0]))
        {   //
            // Data Validation Failed.
            // tchszMsg9
            MessageBox (hwnd, ResourceString ((UINT)IDS_E_MUST_CHOOSE_PARENT_TABLE), NULL, MB_ICONEXCLAMATION|MB_OK);
            return FALSE;
        }
        if (StringList_Count (lpFk->l1) != StringList_Count (lpFk->l2))
        {   //
            // Data Validation Failed.
            // tchszMsg4
            MessageBox (hwnd, ResourceString ((UINT)IDS_E_FKEY_MUST_MATCH_PKEY_UKEYS), NULL, MB_ICONEXCLAMATION|MB_OK);
            return FALSE;
        }
    }
    return TRUE;
}

static void DestroyConstraintList (HWND hwndCtrl)
{
    LPFKEYSTRUCT lpFk;
    int i, nCount = ListBox_GetCount (hwndCtrl);

    for (i=0; i<nCount; i++)
    {
        lpFk = (LPFKEYSTRUCT)ListBox_GetItemData (hwndCtrl, i);
        FKEYSTRUCT_DONE (lpFk);
    }
    ListBox_ResetContent (hwndCtrl);
}

static BOOL OnUpdateConstraintName (HWND hwnd)
{
    int   i, sx, index;
    TCHAR tchszCName [MAXOBJECTNAME];
    LPFKEYSTRUCT lpFk;
    HWND  hwndConstraintName  = GetDlgItem (hwnd, IDC_LIST3);

    memset (tchszCName, 0, sizeof (tchszCName));
    index = ListBox_GetCurSel (hwndConstraintName);
    if (index == LB_ERR)
        return TRUE;
    lpFk = (LPFKEYSTRUCT)ListBox_GetItemData (hwndConstraintName, index);
    Edit_GetText (GetDlgItem (hwnd, IDC_EDIT2), tchszCName, sizeof (tchszCName)); 
    if (tchszCName [0] == '<')
        return TRUE;
    if (!tchszCName [0])
        GetConstraintName (hwndConstraintName, tchszCName);
    sx = ListBox_FindStringExact (hwndConstraintName, -1, tchszCName);
    if (sx >= 0 && sx != index)
    {
        // tchszMsg5
        MessageBox (hwnd, ResourceString ((UINT)IDS_E_CONSTRAINT_NAME_EXIST), NULL, MB_ICONEXCLAMATION|MB_OK);
        SetFocus (GetDlgItem (hwnd, IDC_EDIT1));
        return FALSE;
    }

    ListBox_DeleteString (hwndConstraintName, index);
    i = ListBox_InsertString (hwndConstraintName, index, tchszCName);
    if (i != LB_ERR)
    {
        ListBox_SetItemData (hwndConstraintName, i, (DWORD)lpFk);
        ListBox_SetCurSel   (hwndConstraintName, i);
    }
    return TRUE;
}


static void OnButtonPut (HWND hwnd)
{
    int   i, cursel, nCount;
    LPFKEYSTRUCT  lpFk;
    LPTABLEPARAMS lpTable     = (LPTABLEPARAMS)GetDlgProp (hwnd);
    HWND  hwndConstraintName   = GetDlgItem (hwnd, IDC_LIST3);      // Constraint Name
    HWND  hwndPkeyCols         = GetDlgItem (hwnd, IDC_LIST2);      // List of Pkey Cols
    HWND  hwndTbCols           = GetDlgItem (hwnd, IDC_LIST0);      // List of Table Columns.
    TCHAR item [MAXOBJECTNAME];
    TCHAR tchszColumn[MAXOBJECTNAME];
    cursel = ListBox_GetCurSel (hwndTbCols);
    if (cursel == LB_ERR)
        return;

    ListBox_GetText  (hwndTbCols, cursel, item);
    if (ListBox_FindStringExact (hwndPkeyCols, -1, item) != LB_ERR)
        return;
    ListBox_AddString (hwndPkeyCols, item); 
    cursel = ListBox_GetCurSel (hwndConstraintName);
    //
    // Update data
    if (cursel == LB_ERR)
        return;
    lpFk = (LPFKEYSTRUCT)ListBox_GetItemData (hwndConstraintName, cursel);
    //
    // Update the list of referencing columns
    if (!lpFk)
        return;
    lpFk->l1 = StringList_Done (lpFk->l1);
    nCount = ListBox_GetCount (hwndPkeyCols);
    for (i=0; i<nCount; i++)
    {
        memset (tchszColumn, 0, sizeof (tchszColumn));
        ListBox_GetText (hwndPkeyCols, i, tchszColumn);
        lpFk->l1 = StringList_Add (lpFk->l1, tchszColumn);
    }
}


static void OnButtonRemove (HWND hwnd)
{
    int   i, cursel, nCount;
    LPFKEYSTRUCT  lpFk;
    LPTABLEPARAMS lpTable     = (LPTABLEPARAMS)GetDlgProp (hwnd);
    HWND  hwndConstraintName   = GetDlgItem (hwnd, IDC_LIST3);      // Constraint Name
    HWND  hwndPkeyCols         = GetDlgItem (hwnd, IDC_LIST2);      // List of Pkey Cols
    HWND  hwndTbCols           = GetDlgItem (hwnd, IDC_LIST0);      // List of Table Columns.
    TCHAR tchszColumn [MAXOBJECTNAME];

    cursel = ListBox_GetCurSel (hwndPkeyCols);
    if (cursel == LB_ERR)
        return;
    ListBox_DeleteString (hwndPkeyCols, cursel); 
    cursel = ListBox_GetCurSel (hwndConstraintName);
    if (cursel == LB_ERR)
        return;
    //
    // Update the list of referencing columns
    lpFk = (LPFKEYSTRUCT)ListBox_GetItemData (hwndConstraintName, cursel);
    if (!lpFk)
        return;
    lpFk->l1 = StringList_Done (lpFk->l1);
    nCount = ListBox_GetCount (hwndPkeyCols);
    for (i=0; i<nCount; i++)
    {
        memset (tchszColumn, 0, sizeof (tchszColumn));
        ListBox_GetText (hwndPkeyCols, i, tchszColumn);
        lpFk->l1 = StringList_Add (lpFk->l1, tchszColumn);
    }
}

static void OnConstraintIndex(HWND hwnd)
{
    int cursel;
    LPFKEYSTRUCT  lpFk;
    LPTABLEPARAMS lpTable     = (LPTABLEPARAMS)GetDlgProp (hwnd);
    HWND  hwndConstraintName   = GetDlgItem (hwnd, IDC_LIST3);      // Constraint Name

    cursel = ListBox_GetCurSel (hwndConstraintName);
    if (cursel == LB_ERR)
        return;

    lpFk = (LPFKEYSTRUCT)ListBox_GetItemData (hwndConstraintName, cursel);
    if (!lpFk)
        return;
    VDBA_IndexOption (hwnd, lpTable->DBName, 2, &(lpFk->constraintWithClause), lpTable);
}

static void InitComboActions (HWND hwndCombo)
{
    ComboBox_AddString (hwndCombo, "cascade");
    ComboBox_AddString (hwndCombo, "restrict");
    ComboBox_AddString (hwndCombo, "set null");
    ComboBox_AddString (hwndCombo, "no action");
}

static void SetDisplayMode (HWND hwnd, BOOL bAlter)
{
    EnableWindow (GetDlgItem (hwnd, IDC_COMBO2), !bAlter);
    EnableWindow (GetDlgItem (hwnd, IDC_BUTTON_PUT), !bAlter);
    EnableWindow (GetDlgItem (hwnd, IDC_BUTTON_REMOVE), !bAlter);
    EnableWindow (GetDlgItem (hwnd, IDC_BUTTON_UP), !bAlter);
    EnableWindow (GetDlgItem (hwnd, IDC_BUTTON_DOWN), !bAlter);
    EnableWindow (GetDlgItem (hwnd, IDC_LIST0), !bAlter);

    EnableWindow (GetDlgItem (hwnd, IDC_LIST0), !bAlter);
    EnableWindow (GetDlgItem (hwnd, IDC_LIST2), !bAlter);
    EnableWindow (GetDlgItem (hwnd, IDC_REFERENCES), !bAlter);
    EnableWindow (GetDlgItem (hwnd, IDC_COMBO3), !bAlter);

    EnableWindow (GetDlgItem (hwnd, IDC_RADIO1), !bAlter);
    EnableWindow (GetDlgItem (hwnd, IDC_RADIO2), !bAlter);
    EnableWindow (GetDlgItem (hwnd, IDC_CHECK_DELETE_RULE), !bAlter);
    EnableWindow (GetDlgItem (hwnd, IDC_CHECK_UPDATE_RULE), !bAlter);
    EnableWindow (GetDlgItem (hwnd, IDC_COMBO_DELETE_RULE), !bAlter);
    EnableWindow (GetDlgItem (hwnd, IDC_COMBO_UPDATE_RULE), !bAlter);
}
