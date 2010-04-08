/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/
/*
**
**  Project  : CA/OpenIngres Visual DBA
**  Source   : crintegr.c
**  Dialog box for Creating an Integrity
**
**  History after 14-Feb-2002
**
** 14-Fev-2002 (uk$so01)
**    SIR #106648, Integrate SQL Assistant In-Process Server
** 09-Mar-2009 (smeke01) b121764
**    Pass parameters to CreateSearchCondWizard as UCHAR so that we don't 
**    need to use the T2A macro.
*/


#include "dll.h"
#include "subedit.h"
#include "dbadlg1.h"
#include "dlgres.h"
#include "catolist.h"
#include "getdata.h"
#include "lexical.h"
#include "msghandl.h"
#include "domdata.h"
#include "dbaginfo.h"
#include "tools.h"
#include "sqlasinf.h"
#include "extccall.h"


//#define MAX_TEXT_EDIT 2000

static BOOL CreateObject              (HWND hwnd, LPINTEGRITYPARAMS lpintegrity);
static BOOL AlterObject               (HWND hwnd);
static void OnDestroy                 (HWND hwnd);
static void OnCommand                 (HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
static BOOL OnInitDialog              (HWND hwnd, HWND hwndFocus, LPARAM lParam);
static void OnSysColorChange          (HWND hwnd);
static void InitialiseEditControls    (HWND hwnd);
static BOOL FillControlsFromStructure (HWND hwnd, LPINTEGRITYPARAMS lpintegrityparams);
static BOOL FillStructureFromControls (HWND hwnd, LPINTEGRITYPARAMS lpintegrityparams);
static void EnableDisableOKButton     (HWND hwnd);



BOOL CALLBACK __export DlgCreateIntegrityDlgProc
                       (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

int WINAPI __export DlgCreateIntegrity (HWND hwndOwner, LPINTEGRITYPARAMS lpintegrityparams)

/*
//	Function:
//		Shows the Create/Alter integrity dialog.
//
// Paramters:
//     1) hwndOwner:           Handle of the parent window for the dialog.
//     2) lpintegrityparams:   Points to structure containing information used to 
//                             initialise the dialog. The dialog uses the same
//                             structure to return the result of the dialog.
//
// Returns:
//     TRUE if successful.
*/
{
   FARPROC lpProc;
   int     retVal;
       
   if (!IsWindow (hwndOwner) || !lpintegrityparams)
   {
       ASSERT(NULL);
   return FALSE;
   }
   
   lpProc = MakeProcInstance ((FARPROC) DlgCreateIntegrityDlgProc, hInst);
   retVal = VdbaDialogBoxParam
       (hResource,
       MAKEINTRESOURCE (IDD_INTEGRITY),
       hwndOwner,
       (DLGPROC) lpProc,
       (LPARAM)  lpintegrityparams
       );

   FreeProcInstance (lpProc);
   return (retVal);
}

BOOL CALLBACK __export DlgCreateIntegrityDlgProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
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
   LPINTEGRITYPARAMS lpintegrity = (LPINTEGRITYPARAMS)lParam;
   char szFormat [100];
   char szTitle  [MAX_TITLEBAR_LEN];
   char buffer   [MAXOBJECTNAME];
   LPUCHAR parentstrings [MAXPLEVEL];

   if (!AllocDlgProp (hwnd, lpintegrity))
       return FALSE;
   //
   // force catolist.dll to load
   //
   CATOListDummy();
   if (lpintegrity->bCreate)
   {
       LoadString (hResource, (UINT)IDS_T_CREATE_INTEGRITY, szFormat, sizeof (szFormat));
       lpHelpStack = StackObject_PUSH (lpHelpStack, StackObject_INIT ((UINT)IDD_INTEGRITY));
   }
   //else
   //    LoadString (hResource, (UINT)IDS_T_ALTER_INTEGRITY,  szFormat, sizeof (szFormat));
   parentstrings [0] = lpintegrity->DBName;
   GetExactDisplayString (
       lpintegrity->TableName,
       lpintegrity->TableOwner,
       OT_TABLE,
       parentstrings,
       buffer);

   wsprintf (szTitle, szFormat,
       GetVirtNodeName ( GetCurMdiNodeHandle ()),
       lpintegrity->DBName,
       buffer);
   SetWindowText (hwnd, szTitle);

   FillControlsFromStructure (hwnd, lpintegrity);
   EnableDisableOKButton (hwnd);
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
   HWND hwndSearchCondition = GetDlgItem (hwnd, IDC_INTEGRITY_SEARCHCONDITION);
   LPINTEGRITYPARAMS lpintegrity = GetDlgProp (hwnd);

   switch (id)
   {
       case IDOK:
       {
           BOOL Success = FALSE;

           if (lpintegrity->bCreate)
           {
               Success = FillStructureFromControls (hwnd, lpintegrity);
               if (!Success)
               {
                   FreeAttachedPointers (lpintegrity, OT_INTEGRITY);
                   break;
               }

               Success = CreateObject (hwnd, lpintegrity);
               FreeAttachedPointers (lpintegrity, OT_INTEGRITY);
           }
           /*
           else
           {
               Success = AlterObject (hwnd);
           }
           */

           if (!Success)
               break;
           else
               EndDialog (hwnd, TRUE);
       }
       break;


       case IDCANCEL:
           EndDialog (hwnd, FALSE);
           break;

       case IDC_INTEGRITY_SEARCHCONDITION:
           switch (codeNotify)
           {
               case EN_CHANGE:
               {
                   BOOL bEnable = TRUE;

                   if (Edit_GetTextLength (hwndCtl) == 0)
                       bEnable = FALSE;
                   EnableWindow (GetDlgItem (hwnd, IDOK), bEnable);
               }
               break;
           }
           break;

       case IDC_INTEGRITY_ASSISTANCE:
			{
				TOBJECTLIST obj;
				LPTSTR lpszSQL = NULL;
				LPUCHAR lpVirtNodeName = GetVirtNodeName(GetCurMdiNodeHandle());
				//
				// Construct the list of tables:
				TOBJECTLIST* pTableList = NULL;
				int nLen1 = MultiByteToWideChar(CP_ACP, 0, (LPSTR)StringWithoutOwner (lpintegrity->TableName), -1, NULL, 0);
				int nLen2 = MultiByteToWideChar(CP_ACP, 0, (LPSTR)lpintegrity->TableOwner, -1, NULL, 0);

				LPWSTR lpszW1 = malloc (sizeof(WCHAR)*(nLen1 +1));
				LPWSTR lpszW2 = malloc (sizeof(WCHAR)*(nLen2 +1));
				MultiByteToWideChar(CP_ACP, 0, (LPSTR)StringWithoutOwner (lpintegrity->TableName), -1, lpszW1, nLen1);
				MultiByteToWideChar(CP_ACP, 0, (LPSTR)lpintegrity->TableOwner, -1, lpszW2, nLen2);

				wcscpy (obj.wczObject, lpszW1);
				wcscpy (obj.wczObjectOwner, lpszW2);
				obj.nObjType = OBT_TABLE;
				free (lpszW1);
				free (lpszW2);
				pTableList = TOBJECTLIST_Add (pTableList, &obj);

				lpszSQL = CreateSearchCondWizard(hwnd, lpVirtNodeName, lpintegrity->DBName, pTableList, NULL);
				if (lpszSQL)
				{
					Edit_ReplaceSel(hwndSearchCondition, lpszSQL);
					free(lpszSQL);
				}
				pTableList = TOBJECTLIST_Done (pTableList);
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


static void InitialiseEditControls (HWND hwnd)
{
   HWND hwndTableName      = GetDlgItem (hwnd, IDC_INTEGRITY_TABLENAME);
   HWND hwndSearchCondition= GetDlgItem (hwnd, IDC_INTEGRITY_SEARCHCONDITION);
   LPINTEGRITYPARAMS lpintegrity = GetDlgProp (hwnd);

   Edit_LimitText (hwndTableName , MAX_TEXT_EDIT-1);
}


static BOOL CreateObject (HWND hwnd, LPINTEGRITYPARAMS lpintegrity)
{
   int     ires;
   LPUCHAR vnodeName = GetVirtNodeName (GetCurMdiNodeHandle ());

   ires = DBAAddObject (vnodeName, OT_INTEGRITY, (void *) lpintegrity );

   if (ires != RES_SUCCESS)
   {
       ErrorMessage ((UINT) IDS_E_CREATE_INTEGRITY_FAILED, ires);
       return FALSE;
   }

   return  TRUE;
}


static BOOL AlterObject  (HWND hwnd)
{
   return FALSE;
}


static BOOL FillControlsFromStructure (HWND hwnd, LPINTEGRITYPARAMS lpintegrityparams)
{
   HWND hwndSearchCondition = GetDlgItem (hwnd, IDC_INTEGRITY_SEARCHCONDITION);

   Edit_LimitText (hwndSearchCondition, MAX_TEXT_EDIT-1);
   Edit_SetText   (hwndSearchCondition, lpintegrityparams->lpSearchCondition);

   return  TRUE;
}


static BOOL FillStructureFromControls (HWND hwnd, LPINTEGRITYPARAMS lpintegrityparams)
{
   HWND hwndSearchCondition = GetDlgItem (hwnd, IDC_INTEGRITY_SEARCHCONDITION);
   char szSearch_condition  [MAX_TEXT_EDIT];
   
   Edit_GetText (hwndSearchCondition, szSearch_condition, MAX_TEXT_EDIT);

   if (x_strlen (szSearch_condition) > 0)
   {
       lpintegrityparams->lpSearchCondition = ESL_AllocMem (x_strlen (szSearch_condition) +1);
       if (!lpintegrityparams->lpSearchCondition)
       {
           ErrorMessage   ((UINT)IDS_E_CANNOT_ALLOCATE_MEMORY, RES_ERR);
           return FALSE;
       }
       x_strcpy (lpintegrityparams->lpSearchCondition, szSearch_condition);
   }

   return  TRUE;
}

static void EnableDisableOKButton (HWND hwnd)
{
   HWND hwndSearchCondition = GetDlgItem (hwnd, IDC_INTEGRITY_SEARCHCONDITION);

   if (Edit_GetTextLength (hwndSearchCondition) == 0)
       EnableWindow (GetDlgItem (hwnd, IDOK), FALSE);
   else
       EnableWindow (GetDlgItem (hwnd, IDOK), TRUE);
}

