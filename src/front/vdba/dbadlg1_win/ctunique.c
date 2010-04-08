/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : ctunique.c
**    Project  : CA-Visual DBA,OpenIngres V2.0
**    Author   : UK Sotheavut
**    Purpose  : Dialog Box for Creating a Table Level Unique Constraint
**
** History:
**
** xx-xx-1996 (uk$so01)
**    Created
** 24-Feb-2000 (uk$so01)
**    BUG #100560. Disable the Parallel index creation and index enhancement
**    if not running against the DBMS 2.5 or later.
** 26-Mar-2001 (noifr01)
**    (sir 104270) removal of code for managing Ingres/Desktop
**  30-Mar-2001 (noifr01)
**    (sir 104378) differentiation of II 2.6.
**/

#include "dll.h"
#include "catolist.h"
#include "dlgres.h"
#include "getdata.h"
#include "domdata.h"
#include "msghandl.h"
#include "extccall.h"
#include "resource.h"
#include "dbaginfo.h"

typedef struct tagUNIQUESTRUCT
{
    //
    // bAlter: TRUE if the data is from the DBMS.
    //         ie from the GetDetailInfo.
    BOOL  bAlter;

    int    nOld;        // Non Zero if old constraint name.
    INDEXOPTION constraintWithClause;        // Constarint Enhancement (2.6)
    LPSTRINGLIST l1;    // List of Unique Columns
} UNIQUESTRUCT, FAR* LPUNIQUESTRUCT;
static LPUNIQUESTRUCT UNIQUESTRUCT_INIT (LPTLCUNIQUE    lpUnique);
static LPUNIQUESTRUCT UNIQUESTRUCT_DONE (LPUNIQUESTRUCT lpObj);
static LPTLCUNIQUE    VDBA20xUniqueKey_Init (LPUNIQUESTRUCT lpStruct);

//static TCHAR tchszMsg1[] = "The Unique Constraint must have at least one column.";
//static TCHAR tchszMsg2[] = "The constraint name already exists.\nPlease choose another one.";
//static TCHAR tchszMsg3[] = "Duplicate definition:\n"
//                           "The constraint has already been defined.";

static void OnDestroy               (HWND hwnd);
static void OnCommand               (HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
static BOOL OnInitDialog            (HWND hwnd, HWND hwndFocus, LPARAM lParam);
static void OnSysColorChange        (HWND hwnd);
static void OnOK                    (HWND hwnd);
static void OnAddConstraint         (HWND hwnd);
static void OnDropConstraint        (HWND hwnd, BOOL bCascade);

static void OnUniqueColumnDown      (HWND hwnd);
static void OnUniqueColumnUp        (HWND hwnd);
static void OnConstraintSelChange   (HWND hwnd);
static void GetConstraintName       (HWND hwndCtrl, LPSTR lptchBuffer);
static void EnableButton            (HWND hwnd);
static void OnButtonPut             (HWND hwnd);
static void OnButtonRemove          (HWND hwnd);
static void DestroyConstraintList   (HWND hwndCtrl);
static void UpdateUnique            (HWND hwnd, LPUNIQUESTRUCT lpUn);
static BOOL OnUpdateConstraintName  (HWND hwnd);
static void OnConstraintIndex       (HWND hwnd);
static void SetDisplayMode          (HWND hwnd, BOOL bAlter);

static LPUNIQUESTRUCT UNIQUESTRUCT_INIT (LPTLCUNIQUE    lpUnique)
{
    LPUNIQUESTRUCT    lpObj;
    if (!lpUnique)
        return NULL;
    lpObj = (LPUNIQUESTRUCT)ESL_AllocMem (sizeof (UNIQUESTRUCT));
    if (!lpObj)
    {
        ErrorMessage ((UINT)IDS_E_CANNOT_ALLOCATE_MEMORY, RES_ERR);
        return NULL;
    }
    memset (lpObj, 0, sizeof (UNIQUESTRUCT));
    lpObj->bAlter = lpUnique->bAlter;
    lpObj->l1 = StringList_Copy (lpUnique->lpcolumnlist);
    INDEXOPTION_COPY (&(lpObj->constraintWithClause), &(lpUnique->constraintWithClause));

    return lpObj;
}

static LPUNIQUESTRUCT UNIQUESTRUCT_DONE (LPUNIQUESTRUCT lpObj)
{
    if (!lpObj)
        return NULL;
    lpObj->l1 = StringList_Done (lpObj->l1);
    lpObj->constraintWithClause.pListLocation = StringList_Done (lpObj->constraintWithClause.pListLocation);
    ESL_FreeMem (lpObj);
    return NULL;
}

static LPTLCUNIQUE VDBA20xUniqueKey_Init (LPUNIQUESTRUCT lpStruct)
{
    LPTLCUNIQUE  lpObj;

    if (!lpStruct)
        return NULL;
    lpObj = (LPTLCUNIQUE)ESL_AllocMem (sizeof (TLCUNIQUE));
    if (!lpObj)
    {
        ErrorMessage ((UINT)IDS_E_CANNOT_ALLOCATE_MEMORY, RES_ERR);
        return NULL;
    }
    memset (lpObj, 0, sizeof (TLCUNIQUE));
    lpObj->bAlter = lpStruct->bAlter;
    lpObj->lpcolumnlist = StringList_Copy (lpStruct->l1);
    INDEXOPTION_COPY (&(lpObj->constraintWithClause), &(lpStruct->constraintWithClause));
    return lpObj;
}



BOOL CALLBACK EXPORT CTUniqueDlgProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
int WINAPI EXPORT CTUnique (HWND hwndOwner, LPTABLEPARAMS lpTable)
{
    FARPROC lpProc;
    int     retVal;

    if (!IsWindow (hwndOwner) || !lpTable)
    {
        ASSERT(NULL);
        return FALSE;
    }
    lpProc = MakeProcInstance ((FARPROC) CTUniqueDlgProc, hInst);
    retVal = VdbaDialogBoxParam
        (hResource,
        MAKEINTRESOURCE (IDD_UNIQUECONSTRAINT),
        hwndOwner,
        (DLGPROC) lpProc,
        (LPARAM)  lpTable);
    FreeProcInstance (lpProc);
    return (retVal);
}


BOOL CALLBACK EXPORT CTUniqueDlgProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
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
    TCHAR tchszConstraintName [MAXOBJECTNAME];
    LPCOLUMNPARAMS lpCol          = NULL;
    LPSTRINGLIST   lx             = NULL;
    LPOBJECTLIST   ls             = NULL;
    LPUNIQUESTRUCT lpUniqueStruct = NULL;
    LPTLCUNIQUE    lpUniqueList   = NULL;
    LPTABLEPARAMS  lpTable    = (LPTABLEPARAMS)lParam;
    HWND hwndConstraintName   = GetDlgItem (hwnd, IDC_LIST2);       // List of Constraints Name
    HWND hwndListCols         = GetDlgItem (hwnd, IDC_TLCLIST1);    // List of Columns
    HWND hwndUniqueCols       = GetDlgItem (hwnd, IDC_TLCLIST2);    // List of Columns to make the unique Constraint

    if (!AllocDlgProp (hwnd, lpTable))
        return FALSE;
    if (!lpTable->bCreate)
        ShowWindow (GetDlgItem (hwnd, IDC_REMOVE_CASCADE), SW_SHOW);
    lpHelpStack = StackObject_PUSH (lpHelpStack, StackObject_INIT ((UINT)IDD_UNIQUECONSTRAINT));
    //
    // Fill the Unique Constraint Name in the listbox.
    //
    lpUniqueList = lpTable->lpUnique;
    while (lpUniqueList)
    {
        index = ListBox_AddString (hwndConstraintName, lpUniqueList->tchszConstraint);
        if (index == CB_ERR)
            return FALSE;
        lpUniqueStruct = UNIQUESTRUCT_INIT (lpUniqueList);
        if (!lpUniqueStruct)
        {
            DestroyConstraintList (hwndConstraintName);
            return FALSE;
        }
        if (VDBA20xTableUniqueKey_Search (lpTable->lpOldUnique, lpUniqueList->tchszConstraint))
            lpUniqueStruct->nOld = 1;
        ListBox_SetItemData (hwndConstraintName, index, (DWORD)lpUniqueStruct);
        lpUniqueList = lpUniqueList->next;
    }
    ListBox_SetCurSel (hwndConstraintName, 0);
    //
    // Initialize the Table's columns
    //
    ls = lpTable->lpColumns;
    while (ls)
    {
        lpCol = (LPCOLUMNPARAMS)ls->lpObject;
        if (!lpCol->bNullable)
            index = ListBox_AddString (hwndListCols, lpCol->szColumn);
        ls = ls->lpNext;
    }
    EnableButton     (hwnd);
    richCenterDialog (hwnd);
    //
    // Hide the button index enhancement if not Ingres >= 2.5
    if ( GetOIVers() < OIVERS_25) 
    {
        HWND hwndButtonIndex = GetDlgItem (hwnd, IDC_CONSTRAINT_INDEX);
        if (hwndButtonIndex && IsWindow (hwndButtonIndex))
            ShowWindow (hwndButtonIndex, SW_HIDE);
    }

    //
    // Preselect the unique constraint name if exists.
    // ---------------------------------------------------
    index = ListBox_GetCurSel (hwndConstraintName);
    if (index == LB_ERR)
        return TRUE;
    ListBox_GetText (hwndConstraintName, index, tchszConstraintName);
    if (!tchszConstraintName[0])
        return TRUE;
    lpUniqueStruct = (LPUNIQUESTRUCT)ListBox_GetItemData (hwndConstraintName, index);
    if (!lpUniqueStruct)
        return FALSE;
    //
    // Constraint name exists, then select the first one.
    //
    Edit_SetText (GetDlgItem (hwnd, IDC_EDIT1), tchszConstraintName);
    if (lpUniqueStruct && lpUniqueStruct->nOld > 0)
        EnableWindow (GetDlgItem (hwnd, IDC_EDIT1), FALSE);
    else
        EnableWindow (GetDlgItem (hwnd, IDC_EDIT1), TRUE);
    //
    // Select the unique columns
    lx = lpUniqueStruct->l1;
    while (lx)
    {
       ListBox_AddString (hwndUniqueCols, lx->lpszString);
       lx = lx->next;
    }
    EnableButton  (hwnd);

    return TRUE;
}


static void OnDestroy (HWND hwnd)
{
    DestroyConstraintList   (GetDlgItem (hwnd, IDC_LIST2));
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
    case IDC_TLCBUTTON_ADD:     // Button New
        OnAddConstraint (hwnd);
        EnableButton (hwnd);
        break;
    case IDC_TLCBUTTON_DELETE:  // Button Delete
        OnDropConstraint (hwnd, FALSE);
        EnableButton (hwnd);
        break;
    case IDC_REMOVE_CASCADE:    // Button Del Cascade
        OnDropConstraint (hwnd, TRUE);
        EnableButton (hwnd);
        break;
    case IDC_TLCBUTTON_UP:      // Button Up
        OnUniqueColumnUp  (hwnd);
        EnableButton (hwnd);
        break;
    case IDC_TLCBUTTON_DOWN:    // Button Down
        OnUniqueColumnDown(hwnd);
        EnableButton (hwnd);
        break;
    case IDC_LIST2:             // List of Constraints Names
        if (codeNotify == LBN_SELCHANGE)
        {
            OnConstraintSelChange  (hwnd);
            EnableButton (hwnd);
        }
        break;
    case IDC_TLCLIST1:          // List of Table columnns
        if (codeNotify == LBN_DBLCLK)
            OnButtonPut (hwnd);
        EnableButton (hwnd);
        break;
    case IDC_TLCLIST2:          // List of columns to form a unique constraint
        if (codeNotify == LBN_DBLCLK)
            OnButtonRemove (hwnd);
        EnableButton (hwnd);
        break;
    case IDC_BUTTON_PUT:        // Button >>
        OnButtonPut (hwnd);
        EnableButton (hwnd);
        break;
    case IDC_BUTTON_REMOVE:     // Button <<
        OnButtonRemove (hwnd);
        EnableButton (hwnd);
        break;
    case IDC_EDIT1:             // Edit Box of Constraint Name
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
    case IDC_CONSTRAINT_INDEX:
        OnConstraintIndex(hwnd);
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
    int i, j, nCount;
    int C1, C2;
    LPUNIQUESTRUCT    lpStruct = NULL, lpExist;
    LPTABLEPARAMS     lpTable  = (LPTABLEPARAMS)GetDlgProp (hwnd);
    LPTLCUNIQUE       ls = lpTable->lpUnique, lpUn = NULL, lpUniqueList = NULL;
    HWND hwndConstraintName   = GetDlgItem (hwnd, IDC_LIST2);   // Constraint Name
    nCount = ListBox_GetCount (hwndConstraintName); 
    //
    // Validate the data.
    for (i=0; i<nCount; i++)
    {
        lpStruct = (LPUNIQUESTRUCT)ListBox_GetItemData (hwndConstraintName, i);
        for (j=i+1; j<nCount; j++)
        {
            lpExist = (LPUNIQUESTRUCT)ListBox_GetItemData (hwndConstraintName, j);
            C1 = StringList_Count (lpStruct->l1);
            C2 = StringList_Count (lpExist->l1);
            if (C1 == C2)
            {
                BOOL isEqual = TRUE;
                LPSTRINGLIST l1 = lpStruct->l1;
                LPSTRINGLIST l2 = lpExist->l1;
                while (l1 && l2)
                {
                    if (!StringList_Search (lpStruct->l1, l2->lpszString))
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
                if (isEqual)
                {
                    // tchszMsg3
                    MessageBox (hwnd, ResourceString ((UINT)IDS_E_DUPLICATE_CONSTRAINT2), "Error", MB_ICONEXCLAMATION|MB_OK);
                    ListBox_SetCurSel (hwndConstraintName, j);
                    OnConstraintSelChange (hwnd);
                    return;
                }
            }
        }
        if (lpStruct && StringList_Count (lpStruct->l1) == 0)
        {
            // tchszMsg1
            MessageBox (hwnd, ResourceString ((UINT)IDS_E_MUST_HAVE_COLUMNS), NULL, MB_ICONEXCLAMATION|MB_OK);
            ListBox_SetCurSel (hwndConstraintName, i);
            OnConstraintSelChange (hwnd);
            return;
        }
    }

    //
    // Construct the list of Unique Constraints.
    for (i=0; i<nCount; i++)
    {
        lpStruct = (LPUNIQUESTRUCT)ListBox_GetItemData (hwndConstraintName, i);
        lpUn     =  VDBA20xUniqueKey_Init (lpStruct);
        if (!lpUn)
        {
            ErrorMessage ((UINT)IDS_E_CANNOT_ALLOCATE_MEMORY, RES_ERR);
            VDBA20xTableUniqueKey_Done (lpUniqueList);
            EndDialog (hwnd, IDCANCEL);
            return;
        }
        ListBox_GetText (hwndConstraintName, i, lpUn->tchszConstraint);
        lpUniqueList = VDBA20xTableUniqueKey_Add (lpUniqueList, lpUn);
    }
    lpTable->lpUnique = VDBA20xTableUniqueKey_Done (lpTable->lpUnique);
    lpTable->lpUnique = lpUniqueList;
    EndDialog (hwnd, IDOK);
}

//
// Add Unique Key
// When user clicks on the button: New
// ----------------------------------
static void OnAddConstraint (HWND hwnd)
{
    int   index;
    TCHAR tchszCName [MAXOBJECTNAME];
    LPUNIQUESTRUCT    lpStruct;
    LPTABLEPARAMS     lpTable = (LPTABLEPARAMS)GetDlgProp (hwnd);
    HWND hwndConstraintName   = GetDlgItem (hwnd, IDC_LIST2);       // Liqst of Constraint Names

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
    lpStruct = (LPUNIQUESTRUCT)ESL_AllocMem (sizeof (UNIQUESTRUCT));
    if (!lpStruct)
    {
        EndDialog (hwnd, IDCANCEL);
        ErrorMessage ((UINT)IDS_E_CANNOT_ALLOCATE_MEMORY, RES_ERR);
        return;
    }
    memset (lpStruct, 0, sizeof (UNIQUESTRUCT));
    ListBox_SetItemData (hwndConstraintName, index, (DWORD)lpStruct);
    //
    // Initialize the characteristics.
    ListBox_ResetContent    (GetDlgItem (hwnd, IDC_TLCLIST2));      // Unique key cols
    ListBox_SetCurSel       (hwndConstraintName, index);
    OnConstraintSelChange   (hwnd);
}


static void OnDropConstraint (HWND hwnd, BOOL bCascade)
{
    int   nCount, index;
    TCHAR tchszConstraint[MAXOBJECTNAME];
    LPUNIQUESTRUCT lpUn       = NULL;
    LPTABLEPARAMS     lpTable = (LPTABLEPARAMS)GetDlgProp (hwnd);
    HWND hwndConstraintName   = GetDlgItem (hwnd, IDC_LIST2);       // List of Constraint Names

    memset (tchszConstraint, 0, sizeof (tchszConstraint));
    index = ListBox_GetCurSel (hwndConstraintName);
    if (index == LB_ERR)
        return;     
    ListBox_GetText (hwndConstraintName, index, tchszConstraint);
    if (!tchszConstraint[0])
        return;
    lpUn = (LPUNIQUESTRUCT)ListBox_GetItemData (hwndConstraintName, index);
    if (!lpUn)
        return;
    if (lpUn->nOld && bCascade)
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
    lpTable->lpUnique = VDBA20xTableUniqueKey_Remove(lpTable->lpUnique, tchszConstraint);
    UNIQUESTRUCT_DONE (lpUn);
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




static void OnUniqueColumnUp (HWND hwnd)
{
    int   index;
    LPUNIQUESTRUCT lpUn         = NULL;
    LPTABLEPARAMS  lpTable      = (LPTABLEPARAMS)GetDlgProp (hwnd);
    HWND  hwndConstraintName    = GetDlgItem (hwnd, IDC_LIST2);     // List of Constraint Names
    HWND  hwndUniqueCols        = GetDlgItem (hwnd, IDC_TLCLIST2);  // List of Columns to make the unique Constraint
    TCHAR szItem    [MAXOBJECTNAME+1];
    TCHAR szItemCur [MAXOBJECTNAME+1];
    int   itemData, itemDataCur;

    index = (int)ListBox_GetCurSel (hwndUniqueCols);
    if (index == 0 || index == LB_ERR)
        return;
    ListBox_GetText     (hwndUniqueCols, index -1, szItem);
    ListBox_GetText     (hwndUniqueCols, index   , szItemCur);
    itemData    = (int) ListBox_GetItemData (hwndUniqueCols, index -1);
    itemDataCur = (int) ListBox_GetItemData (hwndUniqueCols, index);

    ListBox_DeleteString(hwndUniqueCols, index);
    ListBox_InsertString(hwndUniqueCols, index-1,  szItemCur);
    ListBox_SetItemData (hwndUniqueCols, index-1,  itemDataCur);
    ListBox_SetCurSel   (hwndUniqueCols, index-1);

    index = ListBox_GetCurSel (hwndConstraintName);
    if (index == LB_ERR)
        return;
    //
    // Update Data
    lpUn = (LPUNIQUESTRUCT)ListBox_GetItemData (hwndConstraintName, index);
    if (!lpUn)
        return;
    UpdateUnique (hwnd, lpUn);
}


static void OnUniqueColumnDown (HWND hwnd)
{
    int   index, nCount = 0;
    LPUNIQUESTRUCT    lpUn      = NULL;
    LPTABLEPARAMS     lpTable   = (LPTABLEPARAMS)GetDlgProp (hwnd);
    HWND  hwndConstraintName    = GetDlgItem (hwnd, IDC_LIST2);     // List of Constraint Names
    HWND  hwndUniqueCols        = GetDlgItem (hwnd, IDC_TLCLIST2);  // List of Columns to make the unique Constraint
    TCHAR szItem    [MAXOBJECTNAME+1];
    TCHAR szItemCur [MAXOBJECTNAME+1];
    int   itemData, itemDataCur;

    nCount= ListBox_GetCount (hwndUniqueCols);
    index = (int)ListBox_GetCurSel (hwndUniqueCols);
    if (index == (nCount-1)|| index == LB_ERR)
        return;
    ListBox_GetText     (hwndUniqueCols, index +1, szItem);
    ListBox_GetText     (hwndUniqueCols, index   , szItemCur);
    itemData    = (int) ListBox_GetItemData (hwndUniqueCols, index +1);
    itemDataCur = (int) ListBox_GetItemData (hwndUniqueCols, index);

    ListBox_DeleteString(hwndUniqueCols, index);
    ListBox_InsertString(hwndUniqueCols, index+1,  szItemCur);
    ListBox_SetItemData (hwndUniqueCols, index+1,  itemDataCur);
    ListBox_SetCurSel   (hwndUniqueCols, index+1);

    index = ListBox_GetCurSel (hwndConstraintName);
    if (index == LB_ERR)
        return;
    //
    // Update Data
    lpUn = (LPUNIQUESTRUCT)ListBox_GetItemData (hwndConstraintName, index);
    if (!lpUn)
        return;
    UpdateUnique (hwnd, lpUn);
}


static void OnConstraintSelChange   (HWND hwnd)
{
    int   index;
    TCHAR tchszConstraintName [MAXOBJECTNAME];
    LPSTRINGLIST   lpList = NULL;
    LPUNIQUESTRUCT lpUn   = NULL;
    HWND  hwndConstraintName   = GetDlgItem (hwnd, IDC_LIST2);      // List of Constraint Names
    HWND  hwndListCols         = GetDlgItem (hwnd, IDC_TLCLIST1);   // List of Columns
    HWND  hwndUniqueCols       = GetDlgItem (hwnd, IDC_TLCLIST2);   // List of Columns to make the unique Constraint
    LPTABLEPARAMS     lpTable  = (LPTABLEPARAMS)GetDlgProp (hwnd);
   
    memset (tchszConstraintName, 0, sizeof (tchszConstraintName));
    index = ListBox_GetCurSel (hwndConstraintName);
    if (index != LB_ERR)
        ListBox_GetText (hwndConstraintName, index, tchszConstraintName);
    if (!tchszConstraintName[0])
        return;
    lpUn = (LPUNIQUESTRUCT)ListBox_GetItemData (hwndConstraintName, index);
    if (!lpUn)
        return;
    Edit_SetText (GetDlgItem (hwnd, IDC_EDIT1), tchszConstraintName);
    if (VDBA20xTableUniqueKey_Search (lpTable->lpOldUnique, tchszConstraintName))
        EnableWindow (GetDlgItem (hwnd, IDC_EDIT1), FALSE);
    else
        EnableWindow (GetDlgItem (hwnd, IDC_EDIT1), TRUE);
    //
    // Display the Unique columns
    ListBox_ResetContent (hwndUniqueCols);
    lpList = lpUn->l1;
    while (lpList)
    {
        ListBox_AddString (hwndUniqueCols, lpList->lpszString);
        lpList = lpList->next;
    }
}


static void EnableButton (HWND hwnd)
{
    int index;
    LPTABLEPARAMS     lpTable  = (LPTABLEPARAMS)GetDlgProp (hwnd);
    HWND  hwndConstraintName   = GetDlgItem (hwnd, IDC_LIST2);       // List of Constraint Names
    HWND  hwndListCols         = GetDlgItem (hwnd, IDC_TLCLIST1);    // List of Columns
    HWND  hwndUniqueCols       = GetDlgItem (hwnd, IDC_TLCLIST2);    // List of Columns to make the unique Constraint
    // 
    // Button >>
    if (ListBox_GetCurSel (hwndListCols) == LB_ERR)
        EnableWindow (GetDlgItem (hwnd, IDC_BUTTON_PUT), FALSE);
    else
        EnableWindow (GetDlgItem (hwnd, IDC_BUTTON_PUT), TRUE);
    //
    // Button <<
    if (ListBox_GetCurSel (hwndUniqueCols) == LB_ERR)
        EnableWindow (GetDlgItem (hwnd, IDC_BUTTON_REMOVE), FALSE);
    else
        EnableWindow (GetDlgItem (hwnd, IDC_BUTTON_REMOVE), TRUE);
    //
    // Button Delete
    index = ListBox_GetCurSel (hwndConstraintName);
    if (index == LB_ERR)
    {
        EnableWindow (GetDlgItem (hwnd, IDC_TLCBUTTON_DELETE), FALSE);
        EnableWindow (GetDlgItem (hwnd, IDC_REMOVE_CASCADE), FALSE);
    }
    else
        EnableWindow (GetDlgItem (hwnd, IDC_TLCBUTTON_DELETE), TRUE);
    //
    // Button Del Cascade
    if (index != LB_ERR)
    {
        TCHAR tchszItem [MAXOBJECTNAME];

        memset (tchszItem, 0, sizeof (tchszItem));
        ListBox_GetText (hwndConstraintName, index, tchszItem);
        if (!tchszItem [0])
            return;
        if (VDBA20xTableUniqueKey_Search (lpTable->lpOldUnique, tchszItem))
            EnableWindow (GetDlgItem (hwnd, IDC_REMOVE_CASCADE), TRUE);
        else
            EnableWindow (GetDlgItem (hwnd, IDC_REMOVE_CASCADE), FALSE);
    }
    index = ListBox_GetCurSel (hwndConstraintName);
    if (index == LB_ERR || ListBox_GetCount (hwndConstraintName) == 0)
    {
        // 
        // Grey out the controls
        EnableWindow (GetDlgItem (hwnd, IDC_TLCLIST1),      FALSE);
        EnableWindow (GetDlgItem (hwnd, IDC_TLCLIST2),      FALSE);
        EnableWindow (GetDlgItem (hwnd, IDC_BUTTON_PUT),    FALSE);
        EnableWindow (GetDlgItem (hwnd, IDC_BUTTON_REMOVE), FALSE);
        EnableWindow (GetDlgItem (hwnd, IDC_EDIT1),         FALSE);
    }
    else
    if (index >= 0)
    {
        // 
        // Enable the controls
        LPUNIQUESTRUCT lpUniqueStruct = NULL;
        lpUniqueStruct = (LPUNIQUESTRUCT)ListBox_GetItemData (hwndConstraintName, index);
        EnableWindow (GetDlgItem (hwnd, IDC_TLCLIST1) ,     TRUE);
        EnableWindow (GetDlgItem (hwnd, IDC_TLCLIST2),      TRUE);
        EnableWindow (GetDlgItem (hwnd, IDC_BUTTON_PUT),    TRUE);
        EnableWindow (GetDlgItem (hwnd, IDC_BUTTON_REMOVE), TRUE);
        if (lpUniqueStruct && (lpUniqueStruct->nOld > 0 || lpUniqueStruct->bAlter))
        {
            EnableWindow (GetDlgItem (hwnd, IDC_EDIT1), FALSE);
            SetDisplayMode (hwnd, TRUE);
        }
        else
        {
            EnableWindow (GetDlgItem (hwnd, IDC_EDIT1), TRUE);
            SetDisplayMode (hwnd, FALSE);
        }
    }
}


static void OnButtonPut (HWND hwnd)
{
    int   cursel;
    LPUNIQUESTRUCT    lpUn     = NULL;
    LPTABLEPARAMS     lpTable  = (LPTABLEPARAMS)GetDlgProp (hwnd);
    HWND  hwndConstraintName   = GetDlgItem (hwnd, IDC_LIST2);       // Constraint Name
    HWND  hwndListCols         = GetDlgItem (hwnd, IDC_TLCLIST1);    // List of Columns
    HWND  hwndUniqueCols       = GetDlgItem (hwnd, IDC_TLCLIST2);    // List of Columns to make the unique Constraint
    TCHAR item [MAXOBJECTNAME];

    cursel = ListBox_GetCurSel (hwndListCols);
    if (cursel == LB_ERR)
        return;
    ListBox_GetText (hwndListCols, cursel, item);
    if (ListBox_FindStringExact (hwndUniqueCols, -1, item) != LB_ERR)
        return;
    ListBox_AddString    (hwndUniqueCols, item); 
    cursel = ListBox_GetCurSel (hwndConstraintName);
    //
    // Update data
    if (cursel == LB_ERR)
        return;
    lpUn = (LPUNIQUESTRUCT)ListBox_GetItemData (hwndConstraintName, cursel);
    if (!lpUn)
        return;
    UpdateUnique (hwnd, lpUn);
}


static void OnButtonRemove (HWND hwnd)
{
    int   cursel;
    LPUNIQUESTRUCT    lpUn     = NULL;
    LPTABLEPARAMS     lpTable  = (LPTABLEPARAMS)GetDlgProp (hwnd);
    HWND  hwndConstraintName   = GetDlgItem (hwnd, IDC_LIST2);       // Constraint Name
    HWND  hwndListCols         = GetDlgItem (hwnd, IDC_TLCLIST1);    // List of Columns
    HWND  hwndUniqueCols       = GetDlgItem (hwnd, IDC_TLCLIST2);    // List of Columns to make the unique Constraint

    cursel = ListBox_GetCurSel (hwndUniqueCols);
    if (cursel == LB_ERR)
        return;
    ListBox_DeleteString (hwndUniqueCols, cursel);
    cursel = ListBox_GetCurSel (hwndConstraintName);
    if (cursel == LB_ERR)
        return;
    //
    // Update data
    if (cursel == LB_ERR)
        return;
    lpUn = (LPUNIQUESTRUCT)ListBox_GetItemData (hwndConstraintName, cursel);
    if (!lpUn)
        return;
    UpdateUnique (hwnd, lpUn);
}

static void DestroyConstraintList (HWND hwndCtrl)
{
    LPUNIQUESTRUCT lpUn;
    int i, nCount = ListBox_GetCount (hwndCtrl);

    for (i=0; i<nCount; i++)
    {
        lpUn = (LPUNIQUESTRUCT)ListBox_GetItemData (hwndCtrl, i);
        UNIQUESTRUCT_DONE (lpUn);
    }
    ListBox_ResetContent (hwndCtrl);
}

static void UpdateUnique  (HWND hwnd, LPUNIQUESTRUCT lpUn)
{
    int i, nCount;
    HWND  hwndUniqueCols       = GetDlgItem (hwnd, IDC_TLCLIST2);    // List of Columns to make the unique Constraint
    HWND  hwndConstraintName   = GetDlgItem (hwnd, IDC_LIST2);       // Constraint Name
    LPSTRINGLIST lpList = NULL;
    TCHAR tchszItem [MAXOBJECTNAME];

    memset (tchszItem, 0, sizeof (tchszItem));
    if (!lpUn)
        return;
    nCount = ListBox_GetCount (hwndUniqueCols);
    for (i=0; i<nCount; i++)
    {
        ListBox_GetText (hwndUniqueCols, i, tchszItem);
        lpList = StringList_Add (lpList, tchszItem);
        if (!lpList)
        {
            ErrorMessage ((UINT)IDS_E_CANNOT_ALLOCATE_MEMORY, RES_ERR);
            return;
        }
    }
    lpUn->l1 = StringList_Done (lpUn->l1);
    lpUn->l1 = lpList;
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

static BOOL OnUpdateConstraintName  (HWND hwnd)
{
    int   i, sx, index;
    TCHAR tchszCName [MAXOBJECTNAME];
    LPUNIQUESTRUCT lpUn;
    HWND  hwndConstraintName  = GetDlgItem (hwnd, IDC_LIST2);

    memset (tchszCName, 0, sizeof (tchszCName));
    index = ListBox_GetCurSel (hwndConstraintName);
    if (index == LB_ERR)
        return TRUE;
    lpUn = (LPUNIQUESTRUCT)ListBox_GetItemData (hwndConstraintName, index);
    Edit_GetText (GetDlgItem (hwnd, IDC_EDIT1), tchszCName, sizeof (tchszCName)); 
    if (tchszCName [0] == '<')
        return TRUE;
    if (!tchszCName [0])
        GetConstraintName (hwndConstraintName, tchszCName);
    sx = ListBox_FindStringExact (hwndConstraintName, -1, tchszCName);
    if (sx >= 0 && sx != index)
    {   
        // tchszMsg2
        MessageBox (hwnd, ResourceString ((UINT)IDS_E_CONSTRAINT_NAME_EXIST), NULL, MB_ICONEXCLAMATION|MB_OK);
        SetFocus (GetDlgItem (hwnd, IDC_EDIT1));
        return FALSE;
    }
    ListBox_DeleteString (hwndConstraintName, index);
    i = ListBox_InsertString (hwndConstraintName, index, tchszCName);
    if (i != LB_ERR)
    {
        ListBox_SetItemData (hwndConstraintName, i, (DWORD)lpUn);
        ListBox_SetCurSel   (hwndConstraintName, i);
    }
    return TRUE;
}

static void OnConstraintIndex(HWND hwnd)
{
    int   cursel;

    LPUNIQUESTRUCT    lpUn     = NULL;
    LPTABLEPARAMS     lpTable  = (LPTABLEPARAMS)GetDlgProp (hwnd);
    HWND  hwndConstraintName   = GetDlgItem (hwnd, IDC_LIST2);
    cursel = ListBox_GetCurSel (hwndConstraintName);
    if (cursel == LB_ERR)
        return;
    lpUn = (LPUNIQUESTRUCT)ListBox_GetItemData (hwndConstraintName, cursel);
    if (!lpUn)
        return;
    VDBA_IndexOption (hwnd, lpTable->DBName, 1, &(lpUn->constraintWithClause), lpTable);
}

static void SetDisplayMode (HWND hwnd, BOOL bAlter)
{
    EnableWindow (GetDlgItem (hwnd, IDC_TLCLIST1), !bAlter);
    EnableWindow (GetDlgItem (hwnd, IDC_BUTTON_PUT), !bAlter);
    EnableWindow (GetDlgItem (hwnd, IDC_BUTTON_REMOVE), !bAlter);
    EnableWindow (GetDlgItem (hwnd, IDC_TLCBUTTON_UP), !bAlter);
    EnableWindow (GetDlgItem (hwnd, IDC_TLCBUTTON_DOWN), !bAlter);
    EnableWindow (GetDlgItem (hwnd, IDC_TLCLIST2), !bAlter);
}
