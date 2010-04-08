/********************************************************************
//
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
//
//    Project  : CA/OpenIngres Visual DBA
//
//    Source   : locsel01.c
//
//    Dialog box for selecting locations (used by Modify structure. 
//             Souce Modifstr.c
//
********************************************************************/

#include "dll.h"
#include "subedit.h"
#include "dbadlg1.h"
#include "dlgres.h"
#include "catolist.h"
#include "getdata.h"
#include "msghandl.h"
#include "lexical.h"
#include "domdata.h"
#include "dbaginfo.h"

static void OnDestroy        (HWND hwnd);
static void OnCommand        (HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
static BOOL OnInitDialog     (HWND hwnd, HWND hwndFocus, LPARAM lParam);
static void OnSysColorChange (HWND hwnd);
static BOOL OccupyLocationControl (HWND hwnd);


BOOL CALLBACK __export DlgLocationSelectionDlgProc
                       (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

BOOL WINAPI __export DlgLocationSelection (HWND hwndOwner, LPSTORAGEPARAMS lpstructure)
/* ____________________________________________________________________________
// Function:
// Shows the Create/Alter profile dialog.
//
// Paramters:
//     1) hwndOwner:    Handle of the parent window for the dialog.
//     2) lpstructure:    Points to structure containing information used to 
//                      initialise the dialog. The dialog uses the same
//                      structure to return the result of the dialog.
//
// Returns: TRUE if Successful.
// ____________________________________________________________________________
*/
{
   FARPROC lpProc;
   int     retVal;
   
   if (!IsWindow (hwndOwner) || !lpstructure)
   {
       ASSERT(NULL);
       return FALSE;
   }

   lpProc = MakeProcInstance ((FARPROC) DlgLocationSelectionDlgProc, hInst);
   retVal = VdbaDialogBoxParam
       (hResource,
        MAKEINTRESOURCE (IDD_LOCSEL01),
        hwndOwner,
        (DLGPROC) lpProc,
        (LPARAM)  lpstructure
       );

   FreeProcInstance (lpProc);
   return (retVal);
}


BOOL CALLBACK __export DlgLocationSelectionDlgProc
                       (HWND hwnd, UINT message, WPARAM wParam, LPARAM  lParam)
{
   switch (message)
   {
       HANDLE_MSG (hwnd, WM_COMMAND, OnCommand);
       HANDLE_MSG (hwnd, WM_DESTROY, OnDestroy);
       
       case WM_INITDIALOG:
           return HANDLE_WM_INITDIALOG (hwnd, wParam, lParam, OnInitDialog);
               
       default:
           return FALSE;

   }
   return TRUE;
}


static BOOL OnInitDialog (HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
   LPSTORAGEPARAMS lpstructure = (LPSTORAGEPARAMS)lParam;
   HWND hwndLocation = GetDlgItem (hwnd, IDC_LOCSEL01_LOC);
   char szFormat [100];
   char szTitle  [MAX_TITLEBAR_LEN];
   char buffer   [MAXOBJECTNAME];
   LPUCHAR parentstrings [MAXPLEVEL];

   if (!AllocDlgProp (hwnd, lpstructure))
       return FALSE;

   parentstrings [0] = lpstructure->DBName;
   if (lpstructure->nObjectType == OT_TABLE)
   {
       LoadString (hResource, (UINT)IDS_T_CHANGELOC_OF_TABLE, szFormat, sizeof (szFormat));
       GetExactDisplayString (
           lpstructure->objectname,
           lpstructure->objectowner,
           OT_TABLE,
           parentstrings,
           buffer);
       lpHelpStack = StackObject_PUSH (lpHelpStack, StackObject_INIT ((UINT)IDD_LOCSEL01));
   }
   else
   {
       LoadString (hResource, (UINT)IDS_T_CHANGELOC_OF_INDEX, szFormat, sizeof (szFormat));
       GetExactDisplayString (
           lpstructure->objectname,
           lpstructure->objectowner,
           OT_INDEX,
           parentstrings,
           buffer);
       lpHelpStack = StackObject_PUSH (lpHelpStack, StackObject_INIT ((UINT)IDD_CHANGLOC_IDX));
   }
   wsprintf (
       szTitle,
       szFormat,
       GetVirtNodeName (GetCurMdiNodeHandle ()),
       lpstructure->DBName,
       buffer);
   SetWindowText (hwnd, szTitle);

   //CAListBoxFillLocations (hwndLocation);
   OccupyLocationControl(hwnd);
   //
   // Select Locations
   //
   {
       LPOBJECTLIST ls = lpstructure->lpNewLocations;
       char* item;

       while (ls)
       {
           item = (char* )ls->lpObject;
           CAListBox_SelectString (hwndLocation, -1, item);
           ls = ls->lpNext;
       }
   }
   richCenterDialog (hwnd);
   return TRUE;
}


static void OnDestroy (HWND hwnd)
{
   DeallocDlgProp (hwnd);
   lpHelpStack = StackObject_POP (lpHelpStack);
}


static void OnCommand (HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
   LPSTORAGEPARAMS lpstructure = GetDlgProp (hwnd);
   BOOL Success = FALSE;
   HWND hwndLoc = GetDlgItem (hwnd, IDC_LOCSEL01_LOC);

   switch (id)
   {
       case IDOK:
           FreeObjectList (lpstructure->lpNewLocations);
           lpstructure->lpNewLocations = NULL;
           lpstructure->lpNewLocations = AddItemToList (hwndLoc);
           EndDialog (hwnd, TRUE);
           break;

       case IDCANCEL:
           EndDialog (hwnd, FALSE);
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
{
	HWND hwndCtl = GetDlgItem (hwnd, IDC_LOCSEL01_LOC);
	int hNode;
	int  err;
	BOOL bSystem;
	char szObject[MAXOBJECTNAME];
   char szFilter[MAXOBJECTNAME];

	ZEROINIT(szObject);
	ZEROINIT(szFilter);

	hNode   = GetCurMdiNodeHandle();
	bSystem = GetSystemFlag ();

	err = DOMGetFirstObject(hNode,
									OT_LOCATION,
									0,
									NULL,
									TRUE,
									NULL,
									szObject,
									NULL,
									NULL);

	while (err == RES_SUCCESS)
	{
       BOOL bOK;
       if (DOMLocationUsageAccepted(hNode,szObject,LOCATIONDATABASE,&bOK)== RES_SUCCESS && bOK)
       {
		    CAListBox_AddString(hwndCtl, szObject);
       }
       err = DOMGetNextObject (szObject, szFilter, NULL);
	}
	return TRUE;
}

