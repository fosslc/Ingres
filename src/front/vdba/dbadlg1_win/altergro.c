/********************************************************************
//
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
//
//    Project  : CA/OpenIngres Visual DBA
//
//    Source   : altergro.c
//
//    Alter group
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


static void OnDestroy       (HWND hwnd);
static void OnCommand       (HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
static BOOL CreateObject    (HWND hwnd, LPGROUPPARAMS   lpgroup);
static BOOL AlterObject     (HWND hwnd);
static BOOL OnInitDialog    (HWND hwnd, HWND hwndFocus, LPARAM lParam);
static void OnSysColorChange(HWND hwnd);
static BOOL FillControlsFromStructure (HWND hwnd, LPGROUPPARAMS lpgroupparams);
static BOOL FillStructureFromControls (HWND hwnd, LPGROUPPARAMS lpgroupparams);
static void EnableDisableOKButton     (HWND hwnd);

BOOL CALLBACK __export DlgAlterGroupDlgProc
                       (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);


                   
int WINAPI __export DlgAlterGroup (HWND hwndOwner, LPGROUPPARAMS lpgroupparams)

/*
// Function:
// Shows the Alter group dialog.
//
// Paramters:
//     1) hwndOwner    :   Handle of the parent window for the dialog.
//     2) lpgroupparams:   Points to structure containing information used to 
//                         initialise the dialog. The dialog uses the same
//                         structure to return the result of the dialog.
//
// Returns:
// TRUE if successful.
*/
{
   FARPROC lpProc;
   int     retVal;

   if (!IsWindow (hwndOwner) || !lpgroupparams)
   {
       ASSERT(NULL);
       return FALSE;
   }

   lpProc = MakeProcInstance ((FARPROC) DlgAlterGroupDlgProc, hInst);
   retVal = VdbaDialogBoxParam
       (hResource,
        MAKEINTRESOURCE (IDD_ALTER_GROUP),
        hwndOwner,
        (DLGPROC) lpProc,
        (LPARAM)  lpgroupparams
       );

   FreeProcInstance (lpProc);
   return (retVal);
}



BOOL CALLBACK __export DlgAlterGroupDlgProc
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
   LPGROUPPARAMS lpgroup   = (LPGROUPPARAMS)lParam;
   char szTitle  [MAX_TITLEBAR_LEN];
   char szFormat [100];

   if (!AllocDlgProp (hwnd, lpgroup))
       return FALSE;
   //
   // force catolist.dll to load
   //
   CATOListDummy();

   LoadString (hResource, (UINT)IDS_T_ALTER_GROUP, szFormat, sizeof (szFormat));
   wsprintf (szTitle,
       szFormat,
       GetVirtNodeName (GetCurMdiNodeHandle ()));
   SetWindowText (hwnd, szTitle);
   FillControlsFromStructure (hwnd, lpgroup);
   EnableWindow (GetDlgItem (hwnd, IDC_ALTER_GROUP_GROUPNAME), FALSE);

   EnableDisableOKButton (hwnd);
   lpHelpStack = StackObject_PUSH (lpHelpStack, StackObject_INIT ((UINT)IDD_ALTER_GROUP));

   richCenterDialog (hwnd);
   return TRUE;
}


static void OnDestroy (HWND hwnd)
{
   DeallocDlgProp(hwnd);
   lpHelpStack = StackObject_POP (lpHelpStack);
}


static void OnCommand (HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
   LPGROUPPARAMS lpgroup = GetDlgProp (hwnd);

   switch (id)
   {
       case IDOK:
       {
           BOOL Success = AlterObject (hwnd);

           if (!Success)
               break;
           else
               EndDialog (hwnd, TRUE);
       }
       break;
           

       case IDCANCEL:
           EndDialog (hwnd, FALSE);
           break;
   }
}


static void OnSysColorChange (HWND hwnd)
{
#ifdef _USE_CTL3D
   Ctl3dColorChange();
#endif
}


static BOOL CreateObject (HWND hwnd, LPGROUPPARAMS   lpgroup)
{
   int     ires;
   LPUCHAR vnodeName = GetVirtNodeName (GetCurMdiNodeHandle ());

   ires = DBAAddObject (vnodeName, OT_GROUP, (void *) lpgroup );

   if (ires != RES_SUCCESS)
   {
       ErrorMessage ((UINT) IDS_E_CREATE_GROUP_FAILED, ires);
       return FALSE;
   }

   return TRUE;
}

// ____________________________________________________________________________%
//

static BOOL AlterObject  (HWND hwnd)
{
   GROUPPARAMS g2;
   GROUPPARAMS g3;
   BOOL Success, bOverwrite;
   int  ires;
   HWND myFocus;
   int  hdl           = GetCurMdiNodeHandle ();
   LPUCHAR vnodeName  = GetVirtNodeName (hdl);
   LPGROUPPARAMS lpg1 = GetDlgProp (hwnd);
   BOOL bChangedbyOtherMessage = FALSE;
   int irestmp;
   ZEROINIT (g2);
   ZEROINIT (g3);

   x_strcpy (g2.ObjectName, lpg1->ObjectName);
   x_strcpy (g3.ObjectName, lpg1->ObjectName);
   Success = TRUE;
   myFocus = GetFocus();
   ires    = GetDetailInfo (vnodeName, OT_GROUP, &g2, TRUE, &hdl);
   if (ires != RES_SUCCESS)
   {
       ReleaseSession(hdl,RELEASE_ROLLBACK);
       switch (ires) {
          case RES_ENDOFDATA:
             ires = ConfirmedMessage ((UINT)IDS_C_ALTER_CONFIRM_CREATE);
             SetFocus (myFocus);
             if (ires == IDYES) {
                Success = FillStructureFromControls (hwnd, &g3);
                if (Success) {
                  ires = DBAAddObject (vnodeName, OT_GROUP, (void *) &g3 );
                  if (ires != RES_SUCCESS) {
                    ErrorMessage ((UINT) IDS_E_CREATE_GROUP_FAILED, ires);
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
       FreeAttachedPointers (&g2, OT_GROUP);
       FreeAttachedPointers (&g3, OT_GROUP);
       return Success;
   }

   if (!AreObjectsIdentical (lpg1, &g2, OT_GROUP))
   {
       ReleaseSession(hdl,RELEASE_ROLLBACK);
       ires   = ConfirmedMessage ((UINT)IDS_C_ALTER_RESTART_LASTCHANGES);
       bChangedbyOtherMessage = TRUE;
       irestmp=ires;
       if (ires == IDYES) {
           ires = GetDetailInfo (vnodeName, OT_GROUP, lpg1, FALSE, &hdl);
           if (ires != RES_SUCCESS) {
               Success =FALSE;
               ErrorMessage ((UINT)IDS_E_ALTER_ACCESS_PROBLEM, RES_ERR);
           }
           else {
              FillControlsFromStructure (hwnd, lpg1);
              FreeAttachedPointers (&g2, OT_GROUP);
              return FALSE;
           }
       }
       else
       {
           // Double Confirmation ?
           ires   = ConfirmedMessage ((UINT)IDS_C_ALTER_CONFIRM_OVERWRITE);
           if (ires != IDYES)
               Success=FALSE;
           else
           {
               char buf [2*MAXOBJECTNAME+5], buf2 [MAXOBJECTNAME];
               if (GetDBNameFromObjType (OT_GROUP,  buf2, NULL))
               {
                   wsprintf (buf,"%s::%s", vnodeName, buf2);
                   ires = Getsession (buf, SESSION_TYPE_CACHEREADLOCK, &hdl);
                   if (ires != RES_SUCCESS)
                       Success = FALSE;
               }
               else
                   Success = FALSE;
           }
       }
       if (!Success)
       {
           FreeAttachedPointers (&g2, OT_GROUP);
           return Success;
       }
   }

   Success = FillStructureFromControls (hwnd, &g3);
   if (Success) {
      ires = DBAAlterObject (lpg1, &g3, OT_GROUP, hdl);
      if (ires != RES_SUCCESS)  {
         Success=FALSE;
         ErrorMessage ((UINT) IDS_E_ALTER_GROUP_FAILED, ires);
         ires = GetDetailInfo (vnodeName, OT_GROUP, &g2, FALSE, &hdl);
         if (ires != RES_SUCCESS)
            ErrorMessage ((UINT)IDS_E_ALTER_ACCESS_PROBLEM, RES_ERR);
         else {
            if (!AreObjectsIdentical (lpg1, &g2, OT_GROUP)) {
               if (!bChangedbyOtherMessage) {
                   ires  = ConfirmedMessage ((UINT)IDS_C_ALTER_RESTART_LASTCHANGES);
               }
               else
                  ires=irestmp;

               if (ires == IDYES) {
                  ires = GetDetailInfo (vnodeName, OT_GROUP, lpg1, FALSE, &hdl);
                  if (ires == RES_SUCCESS)
                     FillControlsFromStructure (hwnd, lpg1);
               }
               else
                   bOverwrite = TRUE;
            }
         }
      }
   }

   FreeAttachedPointers (&g2, OT_GROUP);
   FreeAttachedPointers (&g3, OT_GROUP);
   return Success;
}


static BOOL FillControlsFromStructure (HWND hwnd, LPGROUPPARAMS lpgroupparams)
{
   HWND hwndGroup     = GetDlgItem (hwnd, IDC_ALTER_GROUP_GROUPNAME);
   HWND hwndUsers     = GetDlgItem (hwnd, IDC_ALTER_GROUP_USERLIST );
   int  index, max_database_number ;
   LPCHECKEDOBJECTS lsuser;

   Edit_LimitText (hwndGroup, MAXOBJECTNAME -1);
   Edit_SetText   (hwndGroup, lpgroupparams->ObjectName);
   
   CAListBoxFillUsers (hwndUsers);
   
   //
   // Checked the user name if user is belong to the group 'ObjectName' 
   //
   max_database_number = CAListBox_GetCount (hwndUsers);
   lsuser = (LPCHECKEDOBJECTS) lpgroupparams->lpfirstuser;
   while (lsuser)
   {
       index = CAListBox_FindString (hwndUsers, -1, lsuser->dbname);
       {
           if ((index >= 0) && (index <max_database_number))
               CAListBox_SetSel (hwndUsers, TRUE, index);
       }
       lsuser = lsuser->pnext;
   }

   return TRUE;
}


static BOOL FillStructureFromControls (HWND hwnd, LPGROUPPARAMS lpgroupparams)
{
   HWND hwndGroup     = GetDlgItem (hwnd, IDC_ALTER_GROUP_GROUPNAME);
   HWND hwndUsers     = GetDlgItem (hwnd, IDC_ALTER_GROUP_USERLIST );
   int  i, max_database_number ;
   LPCHECKEDOBJECTS list = NULL;
   LPCHECKEDOBJECTS obj;
   char szGroup  [MAXOBJECTNAME], buf  [MAXOBJECTNAME];
   BOOL checked;
   
   Edit_GetText (hwndGroup, szGroup, sizeof (szGroup));
   if (!IsObjectNameValid (szGroup, OT_UNKNOWN))
   {
       MsgInvalidObject ((UINT)IDS_E_NOT_A_VALIDE_OBJECT);
       Edit_SetSel (hwndGroup, 0, -1);
       SetFocus (hwndGroup);
       return FALSE;
   }
   x_strcpy (lpgroupparams->ObjectName, szGroup);

   //
   // Get the names of users that have been checked and
   // insert into the list
   //
   max_database_number = CAListBox_GetCount (hwndUsers);
   list = NULL;
   for (i = 0; i < max_database_number; i++)
   {
       checked = CAListBox_GetSel (hwndUsers, i);
       if (checked == 1)
       {
           CAListBox_GetText (hwndUsers, i, buf);
           obj = ESL_AllocMem (sizeof (CHECKEDOBJECTS));
           if (!obj)
           {
               lpgroupparams->lpfirstdb = FreeCheckedObjects (lpgroupparams->lpfirstdb);
               list = FreeCheckedObjects (list);
               ErrorMessage ((UINT) IDS_E_CANNOT_ALLOCATE_MEMORY, RES_ERR);
               return FALSE;
           }
           else
           {
               x_strcpy (obj->dbname, buf);
               obj->bchecked = TRUE;
               list = AddCheckedObject (list, obj);
           }
       }
   }
   lpgroupparams->lpfirstdb   = NULL;
   lpgroupparams->lpfirstuser = list;

   return TRUE;
}

static void EnableDisableOKButton (HWND hwnd)
{
   HWND hwndGroup = GetDlgItem (hwnd, IDC_ALTER_GROUP_GROUPNAME);

   if (Edit_GetTextLength (hwndGroup) == 0)
       EnableWindow (GetDlgItem (hwnd, IDOK), FALSE);
   else
       EnableWindow (GetDlgItem (hwnd, IDOK), TRUE);
}
