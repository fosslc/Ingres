/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : CTCheck.c 
**               Dialog Box for Creating a Table Level Check  Constraint.
**    Project  : CA-Visual DBA,OpenIngres V2.0
**    Author   : UK Sotheavut
**
** History:
**
** xx-xx-1996 (uk$so01)
**    Created.
** 25-May-2000 (uk$so01)
**    bug 99242 (DBCS) modify the functions: GenDisplayExpr()
** 21-Mar-2001 (noifr01)
**     (bugs 99242 (DBCS) and 104250). cleanup for DBCS compliance, and fixed 
**     the side effect of a previous change for DBCS, that was resulting in bug
**     104250
** 27-Mar-2001 (noifr01)
**    (sir 104270) removal of code for managing Ingres/Desktop
** 14-Fev-2002 (uk$so01)
**    SIR #106648, Integrate SQL Assistant In-Process Server
** 09-Mar-2009 (smeke01) b121764
**    Pass parameters to CreateSearchCondWizard as UCHAR so that we don't 
**    need to use the T2A macro. Also fix missing MultiByteToWideChar() call
**    in OnAssistant function.
**/

#include <stdio.h>
#include "dll.h"
#include "catolist.h"
#include "dlgres.h"
#include "getdata.h"
#include "domdata.h"
#include "msghandl.h"
#include "resource.h"
#include "tchar.h"
#include "dbaginfo.h"
#include "sqlasinf.h"
#include "extccall.h"

typedef struct tagCHECKSTRUCT
{
    //
    // bAlter: TRUE if the data is from the DBMS.
    //         ie from the GetDetailInfo.
    BOOL  bAlter;

    int    nOld;        // Non Zero if old constraint name.
    LPTSTR expr;        // Expression string
} CHECKSTRUCT, FAR* LPCHECKSTRUCT;
static LPCHECKSTRUCT CHECKSTRUCT_INIT (LPTLCCHECK    lpCheck);
static LPCHECKSTRUCT CHECKSTRUCT_DONE (LPCHECKSTRUCT lpObj);
static LPTLCCHECK    VDBA20xCheck_Init(LPCHECKSTRUCT lpStruct);

//static TCHAR tchszMsg1[] = "The Definition of Check Constraint must have an expression.";
//static TCHAR tchszMsg2[] = "The Constraint Name already exists.\nPlease choose another one.";
//static TCHAR tchszMsg3[] = "Duplicate definition:\n"
//                           "The constraint has already been defined.";


static void OnDestroy               (HWND hwnd);
static void OnCommand               (HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
static BOOL OnInitDialog            (HWND hwnd, HWND hwndFocus, LPARAM lParam);
static void OnSysColorChange        (HWND hwnd);
static void OnOK                    (HWND hwnd);
static void OnAddConstraint         (HWND hwnd);
static void OnDropConstraint        (HWND hwnd, BOOL bCascade);

static void OnConstraintSelChange   (HWND hwnd);
static void OnConstraintEditChange  (HWND hwnd);
static void GetConstraintName       (HWND hwndCtrl, LPSTR lptchBuffer);
static void EnableButton            (HWND hwnd);
static void OnEditExprKillFocus     (HWND hwnd);
static void OnAssistant (HWND hwnd);
static LPTSTR GenEnternalExpr (LPTSTR lpDisplayExpr);
static LPTSTR GenDisplayExpr  (LPTSTR lpInternalExpr);
static void DestroyConstraintList (HWND hwndCtrl);
static BOOL OnUpdateConstraintName (HWND hwnd);
static void SetDisplayMode (HWND hwnd, BOOL bAlter);

static LPCHECKSTRUCT CHECKSTRUCT_INIT (LPTLCCHECK    lpCheck)
{
    LPCHECKSTRUCT    lpObj;
    if (!lpCheck)
        return NULL;
    lpObj = (LPCHECKSTRUCT)ESL_AllocMem (sizeof (CHECKSTRUCT));
    if (!lpObj)
    {
        ErrorMessage ((UINT)IDS_E_CANNOT_ALLOCATE_MEMORY, RES_ERR);
        return NULL;
    }
    memset (lpObj, 0, sizeof (CHECKSTRUCT));
    lpObj->bAlter = lpCheck->bAlter;
    if (lpCheck->lpexpression)
    {
        lpObj->expr = (LPTSTR)ESL_AllocMem (lstrlen (lpCheck->lpexpression)+1);
        if (!lpObj->expr)
        {
            ErrorMessage ((UINT)IDS_E_CANNOT_ALLOCATE_MEMORY, RES_ERR);
            return NULL;
        }
        lstrcpy (lpObj->expr, lpCheck->lpexpression);
    }
    return lpObj;
}

static LPCHECKSTRUCT CHECKSTRUCT_DONE (LPCHECKSTRUCT lpObj)
{
    if (!lpObj)
        return NULL;
    if (lpObj->expr) ESL_FreeMem (lpObj->expr);
    ESL_FreeMem (lpObj);
    return NULL;
}

static LPTLCCHECK    VDBA20xCheck_Init (LPCHECKSTRUCT lpStruct)
{
    LPTLCCHECK  lpObj;

    if (!lpStruct)
        return NULL;
    lpObj = (LPTLCCHECK)ESL_AllocMem (sizeof (TLCCHECK));
    if (!lpObj)
    {
        ErrorMessage ((UINT)IDS_E_CANNOT_ALLOCATE_MEMORY, RES_ERR);
        return NULL;
    }
    memset (lpObj, 0, sizeof (TLCCHECK));
    lpObj->bAlter = lpStruct->bAlter;
    if (lpStruct->expr)
    {
        LPTSTR lpInternal;
        lpObj->lpexpression = ESL_AllocMem (lstrlen (lpStruct->expr) +12);
        if (!lpObj->lpexpression)
        {
            ErrorMessage ((UINT)IDS_E_CANNOT_ALLOCATE_MEMORY, RES_ERR);
            return NULL;
        }
        lpInternal = GenEnternalExpr (lpStruct->expr);
        if (lpInternal)
        {
            lstrcpy (lpObj->lpexpression, lpInternal);
            ESL_FreeMem (lpInternal);
        }
        else
            lstrcpy (lpObj->lpexpression, lpStruct->expr);
    }
    return lpObj;
}


BOOL CALLBACK EXPORT CTCheckDlgProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
BOOL WINAPI EXPORT CTCheck (HWND hwndOwner, LPTABLEPARAMS lpTable)
{
    FARPROC lpProc;
    int     retVal;

    if (!IsWindow (hwndOwner) || !lpTable)
    {
        ASSERT(NULL);
        return FALSE;
    }
    lpProc = MakeProcInstance ((FARPROC) CTCheckDlgProc, hInst);
    retVal = VdbaDialogBoxParam
        (hResource,
        MAKEINTRESOURCE (IDD_CHECKCONSTRAINT),
        hwndOwner,
        (DLGPROC) lpProc,
        (LPARAM)  lpTable);
    FreeProcInstance (lpProc);
    return (retVal);
}


BOOL CALLBACK EXPORT CTCheckDlgProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
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
    LPCHECKSTRUCT lpCheckStruct;
    LPTLCCHECK    lpCheckList = NULL;
    LPTABLEPARAMS lpTable     = (LPTABLEPARAMS)lParam;
    
    HWND hwndConstraintName   = GetDlgItem (hwnd, IDC_LIST3);       // Constraint Name
    HWND hwndExpression       = GetDlgItem (hwnd, IDC_TLCEDIT1);    // Check Expression.
    if (!AllocDlgProp (hwnd, lpTable))
        return FALSE;
    if (!lpTable->bCreate)
    {
        ShowWindow (GetDlgItem (hwnd, IDC_REMOVE_CASCADE), SW_SHOW);
    }

    lpHelpStack = StackObject_PUSH (lpHelpStack, StackObject_INIT ((UINT)IDD_CHECKCONSTRAINT));
    //
    // Fill the Check Constraint Name in the listbox.
    //
    lpCheckList = lpTable->lpCheck;
    while (lpCheckList)
    {
        index = ListBox_AddString (hwndConstraintName, lpCheckList->tchszConstraint);
        if (index == CB_ERR)
            return FALSE;
                
        lpCheckStruct = CHECKSTRUCT_INIT (lpCheckList);
        if (!lpCheckStruct)
        {
            DestroyConstraintList (hwndConstraintName);
            return FALSE;
        }
        if (VDBA20xTableCheck_Search (lpTable->lpOldCheck, lpCheckList->tchszConstraint))
            lpCheckStruct->nOld = 1;
        ListBox_SetItemData (hwndConstraintName, index, (DWORD)lpCheckStruct);
        lpCheckList = lpCheckList->next;
    }
    ListBox_SetCurSel (hwndConstraintName, 0);
    EnableButton     (hwnd);
    richCenterDialog (hwnd);
    //
    // Preselect the Check constraint name if exists.
    // ---------------------------------------------------
    index = ListBox_GetCurSel (hwndConstraintName);
    if (index == LB_ERR)
        return TRUE;
    ListBox_GetText (hwndConstraintName, index, tchszConstraintName);
    if (!tchszConstraintName[0])
        return TRUE;
    lpCheckStruct = (LPCHECKSTRUCT)ListBox_GetItemData (hwndConstraintName, index);
    if (!lpCheckStruct)
        return FALSE;
    //
    // Constraint name exists, then select the first one.
    //
    Edit_SetText (GetDlgItem (hwnd, IDC_EDIT1), tchszConstraintName);
    if (lpCheckStruct && lpCheckStruct->nOld > 0)
        EnableWindow (GetDlgItem (hwnd, IDC_EDIT1), FALSE);
    else
        EnableWindow (GetDlgItem (hwnd, IDC_EDIT1), TRUE);
    //
    // Select the check expression.
    if (lpCheckStruct->expr)
    {
        LPTSTR lpDisplay = GenDisplayExpr  (lpCheckStruct->expr);
        if (lpDisplay)
        {
            Edit_SetText (hwndExpression, lpDisplay);
            ESL_FreeMem (lpDisplay);
        }
        else
            Edit_SetText (hwndExpression, "");
    }
    EnableButton  (hwnd);
    return TRUE;
}


static void OnDestroy (HWND hwnd)
{
    DestroyConstraintList   (GetDlgItem (hwnd, IDC_LIST3));
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
    case IDC_LIST3:             // List of Check Constraint Names
        if (codeNotify == LBN_SELCHANGE)
        {
            OnConstraintSelChange  (hwnd);
            EnableButton (hwnd);
        }
        break;
    case IDC_TLCEDIT1:          // Check Expression Edit Box
        if (codeNotify == EN_KILLFOCUS)
            OnEditExprKillFocus (hwnd);
        break;
    case IDC_TLCBUTTON_ASSISTANT:
        OnAssistant (hwnd);
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
    int i, j, nCount = 0;
    LPCHECKSTRUCT lpStruct, lpExist;
    LPTABLEPARAMS lpTable  = (LPTABLEPARAMS)GetDlgProp (hwnd);
    LPTLCCHECK ls  = lpTable->lpCheck, lpCheckList = NULL, lpCh = NULL;
    HWND hwndConstraintName   = GetDlgItem (hwnd, IDC_LIST3);   // Constraint Name
    
    //
    // Validate the data.
    nCount = ListBox_GetCount (hwndConstraintName); 
    for (i=0; i<nCount; i++)
    {
        lpStruct = (LPCHECKSTRUCT)ListBox_GetItemData (hwndConstraintName, i);
        for (j=i+1; j<nCount; j++)
        {
            lpExist = (LPCHECKSTRUCT)ListBox_GetItemData (hwndConstraintName, j);
            if (lpStruct && lpExist && lpExist->expr && lstrcmpi (lpStruct->expr, lpExist->expr) == 0)
            {
                // tchszMsg3
                    MessageBox (hwnd, ResourceString ((UINT)IDS_E_DUPLICATE_CONSTRAINT2), "Error", MB_ICONEXCLAMATION|MB_OK);
                ListBox_SetCurSel (hwndConstraintName, j);
                OnConstraintSelChange (hwnd);
                return;
            }
        }
        if (lpStruct && (!lpStruct->expr || (lpStruct->expr && lstrlen (lpStruct->expr) == 0)))
        {
            // tchszMsg1
            MessageBox (hwnd, ResourceString ((UINT)IDS_E_MUST_HAVE_EXPRESSION), NULL, MB_ICONEXCLAMATION|MB_OK);
            ListBox_SetCurSel (hwndConstraintName, i);
            OnConstraintSelChange (hwnd);
            return;
        }
    }
    //
    // Construct the list of Check Constraints.
    for (i=0; i<nCount; i++)
    {
        lpStruct = (LPCHECKSTRUCT)ListBox_GetItemData (hwndConstraintName, i);
        lpCh     =  VDBA20xCheck_Init (lpStruct);
        if (!lpCh)
        {
            ErrorMessage ((UINT)IDS_E_CANNOT_ALLOCATE_MEMORY, RES_ERR);
            VDBA20xTableCheck_Done (lpCheckList);
            EndDialog (hwnd, IDCANCEL);
            return;
        }
        ListBox_GetText (hwndConstraintName, i, lpCh->tchszConstraint);
        lpCheckList = VDBA20xTableCheck_Add (lpCheckList, lpCh);
    }
    lpTable->lpCheck = VDBA20xTableCheck_Done (lpTable->lpCheck);
    lpTable->lpCheck = lpCheckList;
    EndDialog (hwnd, IDOK);
}


static void OnAddConstraint (HWND hwnd)
{
    int   index;
    TCHAR tchszCName [MAXOBJECTNAME];
    LPCHECKSTRUCT lpStruct;
    LPTABLEPARAMS lpTable  = (LPTABLEPARAMS)GetDlgProp (hwnd);
    HWND hwndConstraintName   = GetDlgItem (hwnd, IDC_LIST3);       // Constraint Name
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
    lpStruct = (LPCHECKSTRUCT)ESL_AllocMem (sizeof (CHECKSTRUCT));
    if (!lpStruct)
    {
        EndDialog (hwnd, IDCANCEL);
        ErrorMessage ((UINT)IDS_E_CANNOT_ALLOCATE_MEMORY, RES_ERR);
        return;
    }
    memset (lpStruct, 0, sizeof (CHECKSTRUCT));
    ListBox_SetItemData (hwndConstraintName, index, (DWORD)lpStruct);
    //
    // Initialize the characteristics.
    ListBox_SetCurSel       (hwndConstraintName, index);
    Edit_SetText (GetDlgItem (hwnd, IDC_TLCEDIT1), "");
    OnConstraintSelChange   (hwnd);
}


static void OnDropConstraint (HWND hwnd, BOOL bCascade)
{
    int   index, nCount;
    TCHAR tchszConstraint[MAXOBJECTNAME];
    /*
    TCHAR szMessage[256];
    TCHAR szTitle[256];
    TCHAR szOutput[256];
    */
    LPCHECKSTRUCT lpCh     = NULL;
    LPTABLEPARAMS lpTable  = (LPTABLEPARAMS)GetDlgProp (hwnd);
    HWND hwndConstraintName= GetDlgItem (hwnd, IDC_LIST3);       // Constraint Name

    memset (tchszConstraint, 0, sizeof (tchszConstraint));
    index = ListBox_GetCurSel (hwndConstraintName);
    if (index == LB_ERR)
        return;     
    ListBox_GetText (hwndConstraintName, index, tchszConstraint);
    if (!tchszConstraint[0])
        return;
    lpCh = (LPCHECKSTRUCT)ListBox_GetItemData (hwndConstraintName, index);
    if (!lpCh)
        return;
    if (lpCh->nOld && bCascade)
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
    lpTable->lpCheck = VDBA20xTableCheck_Remove(lpTable->lpCheck, tchszConstraint);
    CHECKSTRUCT_DONE (lpCh);
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


static void OnConstraintSelChange   (HWND hwnd)
{
    int   index;
    TCHAR tchszConstraintName [MAXOBJECTNAME];
    LPCHECKSTRUCT      lpCh   = NULL;
    HWND hwndConstraintName   = GetDlgItem (hwnd, IDC_LIST3);       // Constraint Name
    HWND hwndExpression       = GetDlgItem (hwnd, IDC_TLCEDIT1);    // Check Expression
    LPTABLEPARAMS lpTable  = (LPTABLEPARAMS)GetDlgProp (hwnd);
   
    memset (tchszConstraintName, 0, sizeof (tchszConstraintName));
    index = ListBox_GetCurSel (hwndConstraintName);
    if (index != LB_ERR)
        ListBox_GetText (hwndConstraintName, index, tchszConstraintName);
    if (!tchszConstraintName[0])
        return;
    lpCh = (LPCHECKSTRUCT)ListBox_GetItemData (hwndConstraintName, index);
    if (!lpCh)
        return;
    Edit_SetText (GetDlgItem (hwnd, IDC_EDIT1), tchszConstraintName);
    if (VDBA20xTableCheck_Search (lpTable->lpOldCheck, tchszConstraintName))
        EnableWindow (GetDlgItem (hwnd, IDC_EDIT1), FALSE);
    else
        EnableWindow (GetDlgItem (hwnd, IDC_EDIT1), TRUE);
    //
    // Display the Check Expression columns.
    if (lpCh->expr)
    {
        LPTSTR lpDisplay = GenDisplayExpr  (lpCh->expr);
        if (lpDisplay)
        {
            Edit_SetText (hwndExpression, lpDisplay);
            ESL_FreeMem (lpDisplay);
        }
        else
            Edit_SetText (hwndExpression, "");
    }
    else
        Edit_SetText (hwndExpression, "");
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
    int index;
    HWND hwndConstraintName   = GetDlgItem (hwnd, IDC_LIST3);       // Constraint Name
    LPTABLEPARAMS     lpTable = (LPTABLEPARAMS)GetDlgProp (hwnd);
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
        if (VDBA20xTableCheck_Search (lpTable->lpOldCheck, tchszItem))
            EnableWindow (GetDlgItem (hwnd, IDC_REMOVE_CASCADE), TRUE);
        else
            EnableWindow (GetDlgItem (hwnd, IDC_REMOVE_CASCADE), FALSE);
    }
    index = ListBox_GetCurSel (hwndConstraintName);
    if (index == LB_ERR || ListBox_GetCount (hwndConstraintName) == 0)
    {
        // 
        // Grey out the controls
        EnableWindow (GetDlgItem (hwnd, IDC_TLCBUTTON_ASSISTANT) , FALSE);
        EnableWindow (GetDlgItem (hwnd, IDC_TLCEDIT1), FALSE);
        EnableWindow (GetDlgItem (hwnd, IDC_EDIT1), FALSE);
    }
    else
    if (index >= 0)
    {
        // 
        // Enable the controls
        LPCHECKSTRUCT lpCheckStruct = NULL;
        lpCheckStruct = (LPCHECKSTRUCT)ListBox_GetItemData (hwndConstraintName, index);
        EnableWindow (GetDlgItem (hwnd, IDC_TLCBUTTON_ASSISTANT) , TRUE);
        EnableWindow (GetDlgItem (hwnd, IDC_TLCEDIT1), TRUE);
        if (lpCheckStruct && (lpCheckStruct->nOld > 0 || lpCheckStruct->bAlter))
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


static void OnEditExprKillFocus (HWND hwnd)
{
    int   index;
    TCHAR tchszConstraintName [MAXOBJECTNAME];
    LPCHECKSTRUCT  lpCh   = NULL;
    HWND hwndConstraintName   = GetDlgItem (hwnd, IDC_LIST3);       // Constraint Name
    HWND hwndExpression       = GetDlgItem (hwnd, IDC_TLCEDIT1);    // Check Expression
    LPTABLEPARAMS lpTable  = (LPTABLEPARAMS)GetDlgProp (hwnd);
   
    index = ListBox_GetCurSel (hwndConstraintName);
    if (index == LB_ERR)
        return;
    ListBox_GetText (hwndConstraintName, index, tchszConstraintName);
    if (!tchszConstraintName[0])
        return;
    lpCh = (LPCHECKSTRUCT)ListBox_GetItemData (hwndConstraintName, index);
    if (lpCh)
    {
        LPTSTR lpInternalExpr = NULL;
        LPTSTR lpExpr = NULL;
        int len = GetWindowTextLength (hwndExpression) +2;
        lpExpr = ESL_AllocMem (len + 10);
        if (!lpExpr)
        {
            ErrorMessage ((UINT)IDS_E_CANNOT_ALLOCATE_MEMORY, RES_ERR);
            return;
        }
        Edit_GetText (hwndExpression, lpExpr, len+1);
        if (lstrlen (lpExpr) > 0)
            lpInternalExpr = GenEnternalExpr (lpExpr);
        else
            lpInternalExpr = NULL;
        if (lpCh->expr)
            ESL_FreeMem (lpCh->expr);
        if (lpExpr)
            ESL_FreeMem (lpExpr);
        lpCh->expr = lpInternalExpr;
    }  
}

static void OnAssistant (HWND hwnd)
{
	TOBJECTLIST obj;
	LPTSTR lpszSQL;
	HWND hwndExpression    = GetDlgItem (hwnd, IDC_TLCEDIT1); // Check Expression
	LPTABLEPARAMS lpTable  = (LPTABLEPARAMS)GetDlgProp (hwnd);
	LPUCHAR lpVirtNodeName        = GetVirtNodeName(GetCurMdiNodeHandle());

	LPOBJECTLIST  ls;
	LPCOLUMNPARAMS lpC;
	int nLen1;
	LPWSTR lpszW1;

	//
	// Construct the list of tables:
	TOBJECTLIST* lpCol = NULL;
	memset (&obj, 0, sizeof(obj));
	ls = lpTable->lpColumns;
	while (ls)
	{
		lpC = (LPCOLUMNPARAMS)ls->lpObject;
		nLen1 = MultiByteToWideChar(CP_ACP, 0, (LPSTR)lpC->szColumn, -1, NULL, 0);
		lpszW1 = malloc (sizeof(WCHAR)*(nLen1 +1));
		MultiByteToWideChar(CP_ACP, 0, (LPSTR)lpC->szColumn, -1, lpszW1, nLen1);
		wcscpy (obj.wczObject, lpszW1);
		free (lpszW1);

		lpCol = TOBJECTLIST_Add (lpCol, &obj);
		ls = ls->lpNext;
	}

	lpszSQL = CreateSearchCondWizard(hwnd, lpVirtNodeName, lpTable->DBName, NULL, lpCol);
	if (lpszSQL)
	{
		Edit_ReplaceSel(hwndExpression, lpszSQL);
		Edit_SetModify (hwndExpression, TRUE);
		SetFocus (hwndExpression);
		free(lpszSQL);
	}
	lpCol = TOBJECTLIST_Done (lpCol);
}

static LPTSTR GenEnternalExpr (LPTSTR lpDisplayExpr)
{
    int m;
    TCHAR tchszCheck [16];
    LPTSTR lpPos = lpDisplayExpr;
    LPTSTR lpOut = NULL;

    while (lpPos && lpPos[0] == ' ')
        lpPos++;
    if (lpPos)
    {
        if (lstrlen (lpPos) > 5)
        {
            fstrncpy (tchszCheck, lpPos, 6);
            if (lstrcmpi (tchszCheck, "CHECK") == 0)
                m = ConcateStrings (&lpOut, lpPos, (LPTSTR)0);
            else
                m = ConcateStrings (&lpOut, "CHECK (", lpPos, ")", (LPTSTR)0);
        }
        else
            m = ConcateStrings (&lpOut, "CHECK (", lpPos, ")", (LPTSTR)0);
        if (!m) return NULL;
        return lpOut;
    }
    return NULL;
}

static LPTSTR GenDisplayExpr  (LPTSTR lpInternalExpr)
{
    int m;
    TCHAR tchszCheck [16];
    LPTSTR lpPos = lpInternalExpr;
    LPTSTR lpOut = NULL;
    TCHAR* ptchOut = NULL;
    while (lpPos && *lpPos == _T(' '))
        lpPos = _tcsinc (lpPos);
    if (lpPos)
    {
        if (_tcslen (lpPos) > 5)
        {
            fstrncpy (tchszCheck, lpPos, 6);
            if (_tcsicmp (tchszCheck, _T("CHECK")) == 0)
            {
                while (lpPos && *lpPos != _T('('))
                    lpPos = _tcsinc (lpPos);
                lpPos = _tcsinc (lpPos);
                if (lpPos)
                    m = ConcateStrings (&lpOut, lpPos, (LPTSTR)0);
            }
        }
        else
            m = ConcateStrings (&lpOut, lpPos, (LPTSTR)0);
        if (!m) return NULL;
        
        ptchOut = _tcsrchr(lpOut,_T(')'));
		if (ptchOut)
                *ptchOut = _T('\0');
        return lpOut;
    }
    else
        return NULL;
}

static void DestroyConstraintList (HWND hwndCtrl)
{
    LPCHECKSTRUCT lpCh;
    int i, nCount = ListBox_GetCount (hwndCtrl);

    for (i=0; i<nCount; i++)
    {
        lpCh = (LPCHECKSTRUCT)ListBox_GetItemData (hwndCtrl, i);
        CHECKSTRUCT_DONE (lpCh);
    }
    ListBox_ResetContent (hwndCtrl);
}

static BOOL OnUpdateConstraintName  (HWND hwnd)
{
    int   i, sx, index;
    TCHAR tchszCName [MAXOBJECTNAME];
    LPCHECKSTRUCT lpCh;
    HWND  hwndConstraintName  = GetDlgItem (hwnd, IDC_LIST3);

    memset (tchszCName, 0, sizeof (tchszCName));
    index = ListBox_GetCurSel (hwndConstraintName);
    if (index == LB_ERR)
        return TRUE;
    lpCh = (LPCHECKSTRUCT)ListBox_GetItemData (hwndConstraintName, index);
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
        ListBox_SetItemData (hwndConstraintName, i, (DWORD)lpCh);
        ListBox_SetCurSel   (hwndConstraintName, i);
    }
    return TRUE;
}

static void SetDisplayMode (HWND hwnd, BOOL bAlter)
{
    EnableWindow (GetDlgItem (hwnd, IDC_TLCEDIT1), !bAlter);
}
