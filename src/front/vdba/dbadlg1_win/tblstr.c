/********************************************************************
//
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
//
//    Project  : CA/OpenIngres Visual DBA
//
//    Source   : tblstr.c
//
//    Sub dialog box of Creating a table
//
********************************************************************/
/*
** History >= 10-Sep-2004
**
** 10-Sep-2004 (uk$so01)
**    BUG #113002 / ISSUE #13671785 & 13671608, Removal of B1 Security
**    from the back-end implies the removal of some equivalent functionalities
**    from VDBA.
*/

#include "dll.h"
#include <stddef.h>
#include "dbadlg1.h"
#include "dlgres.h"
#include "getdata.h"
#include "domdata.h"
#include "dbaset.h"
#include "dbaginfo.h"
#include "dom.h"
#include "catolist.h"

static void ComboBox_InitPageSize (HWND hwnd, LONG Page_size);

// Structure describing the label granularity types available
static CTLDATA labelTypes[] =
{
   LABEL_GRAN_TABLE,        IDS_TABLE,
   LABEL_GRAN_ROW,          IDS_ROW,
   LABEL_GRAN_SYS_DEFAULTS, IDS_SYS_DEF,
   -1    // terminator
};

// Structure describing the security audit types available.
static CTLDATA auditTypes[] =
{
   SECURITY_AUDIT_TABLEROW,   IDS_TABLEROW,
   SECURITY_AUDIT_TABLENOROW, IDS_TABLENOROW,
   SECURITY_AUDIT_ROW,        IDS_ROW,
   SECURITY_AUDIT_NOROW,      IDS_NOROW,
   SECURITY_AUDIT_TABLE,      IDS_TABLE,
   -1    // terminator
};

BOOL CALLBACK __export DlgTableStructureDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

static void OnDestroy(HWND hwnd);
static void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
static BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);
static void OnSysColorChange(HWND hwnd);

static BOOL OccupyLocationControl(HWND hwnd);
static BOOL OccupyLabelControl(HWND hwnd);
static BOOL OccupyAuditOptionControl(HWND hwnd);
static BOOL OccupyAuditKeyControl(HWND hwnd);

int WINAPI __export DlgTableStructure (HWND hwndOwner, LPTABLEPARAMS lptable)
/*
	Function:
		Shows the Table Structure dialog

	Paramters:
		hwndOwner	- Handle of the parent window for the dialog.
		lptable 	   - Points to structure containing information used to 
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

	if (!IsWindow(hwndOwner) || !lptable)
	{
		ASSERT(NULL);
		return -1;
	}

	if (!(*lptable->DBName))
	{
		ASSERT(NULL);
		return -1;
	}

   lpfnDlgProc = MakeProcInstance((FARPROC)DlgTableStructureDlgProc, hInst);

	iRetVal = VdbaDialogBoxParam (hResource, MAKEINTRESOURCE(IDD_TABLESTRUCT), hwndOwner, (DLGPROC)lpfnDlgProc, (LPARAM)lptable);

   FreeProcInstance (lpfnDlgProc);
	return iRetVal;
}


BOOL CALLBACK __export DlgTableStructureDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
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
	return TRUE;
}


static BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
	LPTABLEPARAMS lptbl = (LPTABLEPARAMS)lParam;

	if (!AllocDlgProp(hwnd, lptbl))
		return FALSE;
    //
    // OpenIngres Version 2.0 only
    // Initialize the Combobox of page_size.
    ComboBox_InitPageSize (hwnd, lptbl->uPage_size);

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

	if (GetOIVers() >= OIVERS_30)
	{
		HWND hwndC = GetDlgItem(hwnd, IDC_STATIC1);
		if (hwndC)
			ShowWindow (hwndC, SW_HIDE);
		hwndC = GetDlgItem(hwnd, IDC_LABEL_GRAN);
		if (hwndC)
			ShowWindow (hwndC, SW_HIDE);
	}
	if (!OccupyLabelControl(hwnd)
	 || !OccupyAuditOptionControl(hwnd)
	 || !OccupyAuditKeyControl(hwnd)
	 || !OccupyLocationControl(hwnd))
	{
		ASSERT(NULL);
		return FALSE;
	}

   CheckDlgButton(hwnd, IDC_JOURNALING, lptbl->bJournaling);
   CheckDlgButton(hwnd, IDC_DUPLICATES, lptbl->bDuplicates);

   if (lptbl->nSecAudit != USE_DEFAULT)
   {
      SelectComboBoxItem(GetDlgItem(hwnd, IDC_SEC_AUDIT_OPTION),
                         auditTypes,
                         lptbl->nSecAudit);
   }

   if (lptbl->nLabelGran != USE_DEFAULT)
   {
      SelectComboBoxItem(GetDlgItem(hwnd, IDC_LABEL_GRAN),
                         labelTypes,
                         lptbl->nLabelGran);
   }

   if (*lptbl->szSecAuditKey)
   {
      ComboBox_SelectString(GetDlgItem(hwnd, IDC_SEC_AUDIT_KEY),
                            -1,
                            lptbl->szSecAuditKey);
   }

   if (lptbl->lpLocations)
   {
      LPOBJECTLIST lplist = lptbl->lpLocations;

      while (lplist)
      {
         LPCHECKEDOBJECTS lpObj = (LPCHECKEDOBJECTS)lplist->lpObject;

         CAListBox_SelectString(GetDlgItem(hwnd, IDC_LOCATIONS),
                                -1,
                                lpObj->dbname);

         lplist = lplist->lpNext;
      }
   }
   if (GetOIVers() != OIVERS_12)
     lpHelpStack = StackObject_PUSH (lpHelpStack, StackObject_INIT ((UINT)IDD_OIV2_TABLESTRUCT));
   else
     lpHelpStack = StackObject_PUSH (lpHelpStack, StackObject_INIT ((UINT)IDD_TABLESTRUCT));
	richCenterDialog(hwnd);

	return TRUE;
}


static void OnDestroy(HWND hwnd)
{
	DeallocDlgProp(hwnd);
   lpHelpStack = StackObject_POP (lpHelpStack);
}


static void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
	switch (id)
	{
		case IDOK:
		{
            int index;
            HWND hwndCombo = GetDlgItem (hwnd, IDC_COMBOPAGESIZE);

			LPTABLEPARAMS lptbl = GetDlgProp(hwnd);
         HWND hwndLocations = GetDlgItem(hwnd, IDC_LOCATIONS);
         HWND hwndSecOption = GetDlgItem(hwnd, IDC_SEC_AUDIT_OPTION);
         HWND hwndLabel = GetDlgItem(hwnd, IDC_LABEL_GRAN);
         HWND hwndKey = GetDlgItem(hwnd, IDC_SEC_AUDIT_KEY);
         int nIdx;

         FreeObjectList (lptbl->lpLocations);  // Vut adds
         lptbl->lpLocations = NULL;            // Vut adds
         lptbl->lpLocations = CreateList4CheckedObjects (hwndLocations);
         lptbl->bJournaling = IsDlgButtonChecked(hwnd, IDC_JOURNALING);
         lptbl->bDuplicates = IsDlgButtonChecked(hwnd, IDC_DUPLICATES);
         index = ComboBox_GetCurSel (hwndCombo);
         if (index != -1)
         {
                lptbl->uPage_size = (LONG)ComboBox_GetItemData (hwndCombo, index);
                if (lptbl->uPage_size == 2048L)
                    lptbl->uPage_size = 0L;
         }
         ComboBox_GetText(hwndKey, lptbl->szSecAuditKey, sizeof(lptbl->szSecAuditKey));

			nIdx = ComboBox_GetCurSel(hwndSecOption);
			lptbl->nSecAudit = (int)ComboBox_GetItemData(hwndSecOption, nIdx);

			nIdx = ComboBox_GetCurSel(hwndLabel);
			lptbl->nLabelGran = (int)ComboBox_GetItemData(hwndLabel, nIdx);

			EndDialog(hwnd, 1);
			break;
		}

		case IDCANCEL:
			EndDialog(hwnd, 0);
			break;
	}
}


static void OnSysColorChange(HWND hwnd)
{
#ifdef _USE_CTL3D
	Ctl3dColorChange();
#endif
}


static BOOL OccupyLocationControl (HWND hwnd)
/*
	Function:
		Fills the location drop down box with the location names.

	Parameters:
		hwnd	- Handle to the dialog window.

	Returns:
		TRUE if successful.
*/
{
	HWND hwndCtl = GetDlgItem (hwnd, IDC_LOCATIONS);
	int hNode;
	int err;
	BOOL bSystem;
	char szObject[MAXOBJECTNAME];
   char szFilter[MAXOBJECTNAME];
   LPUCHAR aparents[MAXPLEVEL];
	LPTABLEPARAMS lptbl = GetDlgProp(hwnd);

	ZEROINIT(aparents);
	ZEROINIT(szObject);
	ZEROINIT(szFilter);

	aparents[0] = lptbl->DBName;

	hNode = GetCurMdiNodeHandle();
	bSystem = GetSystemFlag ();

	err = DOMGetFirstObject(hNode,
									OT_LOCATION,
									0,
									aparents,
									TRUE,
									NULL,
									szObject,
									NULL,
									NULL);

	while (err == RES_SUCCESS)
	{
      BOOL bOK;
      if (DOMLocationUsageAccepted(hNode,szObject,LOCATIONDATABASE,&bOK)==
          RES_SUCCESS && bOK) {
		   CAListBox_AddString(hwndCtl, szObject);
      }
		err = DOMGetNextObject (szObject, szFilter, NULL);
	}

	return TRUE;
}


static BOOL OccupyLabelControl(HWND hwnd)
{
	HWND hwndCtl = GetDlgItem (hwnd, IDC_LABEL_GRAN);
   return OccupyComboBoxControl(hwndCtl, labelTypes);
}


static BOOL OccupyAuditOptionControl(HWND hwnd)
{
	HWND hwndCtl = GetDlgItem (hwnd, IDC_SEC_AUDIT_OPTION);
   return OccupyComboBoxControl(hwndCtl, auditTypes);
}


static BOOL OccupyAuditKeyControl(HWND hwnd)
{
	LPTABLEPARAMS lptbl = GetDlgProp(hwnd);
	HWND hwndCtl = GetDlgItem (hwnd, IDC_SEC_AUDIT_KEY);
   BOOL bRetVal = TRUE;
   LPOBJECTLIST lplist = lptbl->lpColumns;

   while (lplist)
   {
      LPCOLUMNPARAMS lpCols = (LPCOLUMNPARAMS)lplist->lpObject;

      if (lpCols)
      {
         int nIdx = ComboBox_AddString(hwndCtl, lpCols->szColumn);

         if (nIdx == CB_ERR)
         {
	         bRetVal = FALSE;
            break;
         }
      }

      lplist = lplist->lpNext;
   }

   return bRetVal;
}


static void ComboBox_InitPageSize (HWND hwnd, LONG Page_size)
{
    int i, index;
    typedef struct tagCxU
    {
        char* cx;
        LONG  ux;
    } CxU;
    CxU pageSize [] =
    {
        {" 2K", 2048L},
        {" 4K", 4096L},
        {" 8K", 8192L},
        {"16K",16384L},
        {"32K",32768L},
        {"64K",65536L}
    };
    HWND hwndCombo = GetDlgItem (hwnd, IDC_COMBOPAGESIZE);
    for (i=0; i<6; i++)
    {
        index = ComboBox_AddString (hwndCombo, pageSize [i].cx);
        if (index != -1)
            ComboBox_SetItemData (hwndCombo, index, pageSize [i].ux);
    }
    ComboBox_SetCurSel(hwndCombo, 0);
    for (i=0; i<6; i++)
    {
        if (pageSize [i].ux == Page_size)
        {
            ComboBox_SetCurSel(hwndCombo, i);
            break;
        }
    }
}
