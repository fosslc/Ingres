/********************************************************************
//
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
//
//    Project  : CA/OpenIngres Visual DBA
//
//    Source   : synobj.c
//
//    Dialog box for creating a synonym with cross reference
//
********************************************************************/

#include "dll.h"
#include "subedit.h"
#include "dbadlg1.h"
#include "dlgres.h"
#include "catolist.h"
#include "getdata.h"
#include "lexical.h"
#include "msghandl.h"
#include "dba.h"
#include "domdata.h"
#include "dbaginfo.h"
#include "dom.h"
#include "esltools.h"
#include "winutils.h"



BOOL CALLBACK __export DlgCreateSynonymObjDlgProc
                       (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

static void OnDestroy              (HWND hwnd);
static void OnCommand              (HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
static BOOL OnInitDialog           (HWND hwnd, HWND hwndFocus, LPARAM lParam);
static void OnSysColorChange       (HWND hwnd);
static void InitialiseEditControls (HWND hwnd);
static void EnableDisableOKButton  (HWND hwnd);       


int WINAPI __export DlgCreateSynonymOnObject (HWND hwndOwner, LPSYNONYMPARAMS lpsynonymparams)

/*
// Function:
// Shows the Create synonym dialog.
//
// Paramters:
//     1) hwndOwner:               Handle of the parent window for the dialog.
//     2) lpsynonymparams:         Points to structure containing information used to 
//                                 initialise the dialog. The dialog uses the same
//                                 structure to return the result of the dialog.
//     
// Returns:
//    TRUE if successful.
*/
{
   FARPROC lpProc;
   int     retVal;
   

   if (!IsWindow (hwndOwner) || !lpsynonymparams)
   {
       ASSERT(NULL);
       return FALSE;
   }

   lpProc = MakeProcInstance ((FARPROC) DlgCreateSynonymObjDlgProc, hInst);
   retVal = VdbaDialogBoxParam
        (hResource,
         MAKEINTRESOURCE (IDD_SYNONYM_ONOBJECT),
         hwndOwner,
         (DLGPROC) lpProc,
         (LPARAM)  lpsynonymparams
        );

   FreeProcInstance (lpProc);
   return (retVal);
}

BOOL CALLBACK __export DlgCreateSynonymObjDlgProc
                       (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
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
   char szFormat [100];
   char szTitle  [MAX_TITLEBAR_LEN];
   char buffer   [MAXOBJECTNAME];
   LPUCHAR parentstrings [MAXPLEVEL];
   LPSYNONYMPARAMS lpsynonym   = (LPSYNONYMPARAMS)lParam;

   if (!AllocDlgProp (hwnd, lpsynonym))
       return FALSE;
   //
   // force catolist.dll to load
   //
   CATOListDummy();

   // DON'T PUSH CONTEXT HELP VALUE - THREE POSSIBLE VALUES ALREADY PUSHED
   LoadString (hResource, (UINT)IDS_T_CREATE_REFSYNONYM, szFormat, sizeof (szFormat));
   parentstrings [0] = lpsynonym->DBName;
   GetExactDisplayString (
       lpsynonym->RelatedObject,
       lpsynonym->RelatedObjectOwner,
       OT_SYNONYMOBJECT,
       parentstrings,
       buffer);

   wsprintf (szTitle, szFormat,
       GetVirtNodeName ( GetCurMdiNodeHandle ()),
       lpsynonym->DBName,
       buffer);
   SetWindowText (hwnd, szTitle);

   InitialiseEditControls (hwnd);

   //
   // Disable the OK button if there is neither text in the 
   // synonym name control nor object is selected
   //
   
   EnableDisableOKButton (hwnd);       

   richCenterDialog (hwnd);
   return TRUE;
}



static void OnDestroy (HWND hwnd)
{
   DeallocDlgProp(hwnd);
}


static void OnCommand (HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
   LPSYNONYMPARAMS lpsynonym = GetDlgProp (hwnd);
   int     ires;
   LPUCHAR parentstrings [MAXPLEVEL];

   parentstrings [0] = lpsynonym->DBName;              
   switch (id)
   {
       case IDOK:
       {
           char szSynonymName [MAXOBJECTNAME -1];
           HWND hwndSynonymName = GetDlgItem (hwnd, IDC_SYNONYM_ONOBJECT_NAME);

           Edit_GetText (hwndSynonymName, szSynonymName, MAXOBJECTNAME);
           if (!IsObjectNameValid (szSynonymName, OT_UNKNOWN))
           {
               MsgInvalidObject ((UINT)IDS_E_NOT_A_VALIDE_OBJECT);
               Edit_SetSel (hwndSynonymName, 0, -1);
               SetFocus (hwndSynonymName);
               break;
           }
           else
               x_strcpy (lpsynonym->ObjectName, szSynonymName);
           //
           // Create and add object
           //
           ires = DBAAddObject
               ( GetVirtNodeName ( GetCurMdiNodeHandle ()),
                 OT_SYNONYM,
                 (void *) lpsynonym );

           if (ires != RES_SUCCESS)
           {
               ErrorMessage ((UINT) IDS_E_CREATE_SYNONYM_FAILED, ires);
               break;
           }
           else
           {
               EndDialog (hwnd, TRUE);
           }
       }
       break;
           

       case IDCANCEL:
           EndDialog (hwnd, FALSE);
           break;

       case IDC_SYNONYM_ONOBJECT_NAME:
           EnableDisableOKButton  (hwnd);
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
   HWND hwndSynonymName  = GetDlgItem (hwnd, IDC_SYNONYM_ONOBJECT_NAME);

   Edit_LimitText (hwndSynonymName , MAXOBJECTNAME);
}



static void EnableDisableOKButton (HWND hwnd)
{
   HWND hwndSynonymName = GetDlgItem (hwnd, IDC_SYNONYM_ONOBJECT_NAME);

   if (Edit_GetTextLength (hwndSynonymName) == 0)
       EnableWindow (GetDlgItem (hwnd, IDOK), FALSE);
   else
       EnableWindow (GetDlgItem (hwnd, IDOK), TRUE);
}
            

