/********************************************************************
//
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
//
//    Project  : CA/OpenIngres Visual DBA
//
//    Source   : modifstr.c
//
//    Dialog box for modifying structure of Table/Index
//
********************************************************************/

#include <ctype.h>
#include <stdlib.h>
#include "dll.h"
#include "subedit.h"
#include "dbadlg1.h"
#include "dlgres.h"
#include "catolist.h"
#include "catospin.h"
#include "getdata.h"
#include "msghandl.h"
#include "lexical.h"
#include "tools.h"
#include "domdata.h"
#include "dbaginfo.h"
#include "dgerrh.h"
#include "winutils.h" // ShowHourGlass(),RemoveHourGlass()
#include "resource.h"

#define  NBPAGE_MIN 1
#define  NBPAGE_MAX 8388607

// Structure describing the numeric edit controls
static EDITMINMAX limits[] =
{
   IDC_MODIFSTR_NBPAGE, IDC_MODIFSTR_SPIN_NBPAGE, NBPAGE_MIN, NBPAGE_MAX,
   -1// terminator
};
//static LPIDXCOLS lpIdxCol = NULL;

LPOBJECTLIST m_loc1 = NULL;
LPOBJECTLIST m_loc2 = NULL;




static void OnDestroy        (HWND hwnd);
static void OnCommand        (HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
static BOOL OnInitDialog     (HWND hwnd, HWND hwndFocus, LPARAM lParam);
static void OnSysColorChange (HWND hwnd);

static void ActivateControls          (HWND hwnd);
static void InitialiseEditControls    (HWND hwnd);
static void InitialiseSpinControls    (HWND hwnd);
static void EnableDisableOKButton     (HWND hwnd);
static BOOL FillControlsFromStructure (HWND hwnd, LPSTORAGEPARAMS lpstructure);
static BOOL FillStructureFromControls (HWND hwnd, LPSTORAGEPARAMS lpstructure);
static BOOL ModifyObject_table (HWND hwnd);
static BOOL ModifyObject_index (HWND hwnd);
static LPOBJECTLIST CopyList   (const LPOBJECTLIST lpInList);
static LPOBJECTLIST CopyList2  (const LPOBJECTLIST lpInList);


BOOL CALLBACK __export DlgModifyStructureDlgProc
                       (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

BOOL WINAPI __export DlgModifyStructure (HWND hwndOwner, LPSTORAGEPARAMS lpstructure)
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

   lpProc = MakeProcInstance ((FARPROC) DlgModifyStructureDlgProc, hInst);
   retVal = VdbaDialogBoxParam
       (hResource,
        MAKEINTRESOURCE (IDD_MODIFSTR),
        hwndOwner,
        (DLGPROC) lpProc,
        (LPARAM)  lpstructure
       );

   FreeProcInstance (lpProc);
   return (retVal);
}


BOOL CALLBACK __export DlgModifyStructureDlgProc
                       (HWND hwnd, UINT message, WPARAM wParam, LPARAM  lParam)
{
   switch (message)
   {
       HANDLE_MSG (hwnd, WM_COMMAND, OnCommand);
       HANDLE_MSG (hwnd, WM_DESTROY, OnDestroy);

       case WM_INITDIALOG:
           return HANDLE_WM_INITDIALOG (hwnd, wParam, lParam, OnInitDialog);

       case LM_GETEDITMINMAX:
       {
           LPEDITMINMAX lpdesc = (LPEDITMINMAX) lParam;
           HWND hwndCtl = (HWND) wParam;

           ASSERT(IsWindow(hwndCtl));
           ASSERT(lpdesc);

           return GetEditCtrlMinMaxValue (hwnd, hwndCtl, limits, &lpdesc->dwMin, &lpdesc->dwMax);
       }

       case LM_GETEDITSPINCTRL:
       {
           HWND hwndEdit = (HWND) wParam;
           HWND FAR * lphwnd = (HWND FAR *)lParam;
           *lphwnd = GetEditSpinControl(hwndEdit, limits);
           break;
       }

       default:
           return HandleUserMessages(hwnd, message, wParam, lParam);
   }
   return TRUE;
}


static BOOL OnInitDialog (HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
   LPSTORAGEPARAMS lpstructure = (LPSTORAGEPARAMS)lParam;
   char szFormat [100];
   char szTitle  [MAX_TITLEBAR_LEN];
   char buffer   [MAXOBJECTNAME];
   LPUCHAR parentstrings [MAXPLEVEL];

   if (!AllocDlgProp (hwnd, lpstructure))
       return FALSE;

   parentstrings [0] = lpstructure->DBName;
   if (lpstructure->nObjectType == OT_TABLE)
   {
       LoadString (hResource, (UINT)IDS_T_MODIFY_TABLE_STRUCT, szFormat, sizeof (szFormat));
       GetExactDisplayString (
           lpstructure->objectname,
           lpstructure->objectowner,
           OT_TABLE,
           parentstrings,
           buffer);
       lpHelpStack = StackObject_PUSH (lpHelpStack, StackObject_INIT ((UINT)IDD_MODIFSTR));
       EnableWindow (GetDlgItem (hwnd, IDC_MODIFSTR_SHRINKBTREEIDX), FALSE);
   }
   else
   {
       LoadString (hResource, (UINT)IDS_T_MODIFY_INDEX_STRUCT, szFormat, sizeof (szFormat));
       GetExactDisplayString (
           lpstructure->objectname,
           lpstructure->objectowner,
           OT_INDEX,
           parentstrings,
           buffer);
       lpHelpStack = StackObject_PUSH (lpHelpStack, StackObject_INIT ((UINT)IDD_MODIF_IDX));
       //EnableWindow (GetDlgItem (hwnd, IDC_MODIFSTR_DELETEALLDATA), FALSE);
       //EnableWindow (GetDlgItem (hwnd, IDC_MODIFSTR_READONLY), FALSE);
       //EnableWindow (GetDlgItem (hwnd, IDC_MODIFSTR_NOREADONLY), FALSE);
   }

   wsprintf (
       szTitle,
       szFormat,
       GetVirtNodeName (GetCurMdiNodeHandle ()),
       lpstructure->DBName,
       buffer);
   SetWindowText (hwnd, szTitle);

   Button_SetCheck (GetDlgItem (hwnd, IDC_MODIFSTR_RELOC), TRUE);
   lpstructure->nModifyAction = MODIFYACTION_RELOC;

   //
   // Duplicate column list needed by Changes storage structure dlg box
   //
   FreeObjectList (lpstructure->lpMaintainCols);
   lpstructure->lpMaintainCols = NULL;
   lpstructure->lpMaintainCols = CopyList (lpstructure->lpidxCols);

   FreeObjectList (lpstructure->lpNewLocations);
   lpstructure->lpNewLocations = CopyList2(lpstructure->lpLocations);

   m_loc1 = CopyList2(lpstructure->lpLocations);
   m_loc2 = CopyList2(lpstructure->lpLocations);


   InitialiseSpinControls    (hwnd);
   InitialiseEditControls    (hwnd);
   FillControlsFromStructure (hwnd, lpstructure);
   ActivateControls          (hwnd);
   EnableDisableOKButton     (hwnd);

   richCenterDialog (hwnd);
   return TRUE;
}


static void OnDestroy (HWND hwnd)
{
   LPSTORAGEPARAMS lpstructure = GetDlgProp (hwnd);

   FreeObjectList (m_loc1);
   FreeObjectList (m_loc2);
   FreeObjectList (lpstructure->lpNewLocations);
   m_loc1 = NULL;
   m_loc2 = NULL;
   lpstructure->lpNewLocations = NULL;
   DeallocDlgProp (hwnd);
   lpHelpStack = StackObject_POP (lpHelpStack);
}


static void OnCommand (HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
   LPSTORAGEPARAMS lpstructure = GetDlgProp (hwnd);
   BOOL Success = FALSE;
   HWND hwndIdRelocate = GetDlgItem (hwnd, IDC_MODIFSTR_IDLOC01);
   HWND hwndIdLocation = GetDlgItem (hwnd, IDC_MODIFSTR_IDLOC02);
   HWND hwndIdChangeSt = GetDlgItem (hwnd, IDC_MODIFSTR_IDCHANGESTRUCT);

   switch (id)
   {
       case IDOK:
       {
           BOOL Success = FALSE;
           FillStructureFromControls (hwnd, lpstructure);

           if (lpstructure->nModifyAction == MODIFYACTION_RELOC)
           {
               FreeObjectList (lpstructure->lpNewLocations);
               lpstructure->lpNewLocations = CopyList2 (m_loc1);
           }
           else
           if (lpstructure->nModifyAction == MODIFYACTION_REORG)
           {
               FreeObjectList (lpstructure->lpNewLocations);
               lpstructure->lpNewLocations = CopyList2 (m_loc2);
           }

           if (lpstructure->nModifyAction == MODIFYACTION_CHGSTORAGE)
           {
               LPOBJECTLIST temp = lpstructure->lpidxCols;

               lpstructure->lpidxCols      =  lpstructure->lpMaintainCols;
               lpstructure->lpMaintainCols =  temp;
           }
			ShowHourGlass();
           if (lpstructure->nObjectType == OT_TABLE)
               Success  = ModifyObject_table (hwnd);
           else
               Success  = ModifyObject_index (hwnd);
			RemoveHourGlass();
           if (!Success)
           {
               if (lpstructure->nModifyAction == MODIFYACTION_CHGSTORAGE)
               {
                   LPOBJECTLIST temp = lpstructure->lpidxCols;

                   lpstructure->lpidxCols      =  lpstructure->lpMaintainCols;
                   lpstructure->lpMaintainCols =  temp;
               }
               break;
           }
           else
           {
               EndDialog (hwnd, TRUE);
           }

           break;
       }

       case IDCANCEL:
           EndDialog (hwnd, FALSE);
           break;

       case IDC_MODIFSTR_RELOC:
       case IDC_MODIFSTR_CHANGELOC:
       case IDC_MODIFSTR_CHANGESTORAGE:
       case IDC_MODIFSTR_ADDPAGE:
       case IDC_MODIFSTR_SHRINKBTREEIDX:
       case IDC_MODIFSTR_DELETEALLDATA:
       case IDC_MODIFSTR_READONLY:
       case IDC_MODIFSTR_NOREADONLY:
       case IDC_MARKAS_PHYSINCONSIST:
       case IDC_MARKAS_PHYSCONSIST:
       case IDC_MARKAS_LOGIINCONSIST:
       case IDC_MARKAS_LOGICONSIST:
       case IDC_MARKAS_DISALLOWED:
       case IDC_MARKAS_ALLOWED:
           ActivateControls (hwnd);
           break;

       case IDC_MODIFSTR_IDLOC01:
           {
           int ires;

           lpstructure->nModifyAction  = MODIFYACTION_RELOC;
           FreeObjectList (lpstructure->lpNewLocations);

           lpstructure->lpNewLocations = CopyList2 (m_loc1);
           ires = DlgRelocate (hwnd, lpstructure);
           if (ires)
           {
               FreeObjectList (m_loc1); m_loc1 = NULL;
               m_loc1 = CopyList2 (lpstructure->lpNewLocations);
           }
           else
           {
               FreeObjectList (lpstructure->lpNewLocations);
               lpstructure->lpNewLocations = CopyList2 (m_loc1);
           }

           break;
           }

       case IDC_MODIFSTR_IDLOC02:
           {
           int ires ;

           lpstructure->nModifyAction  = MODIFYACTION_REORG;
           FreeObjectList (lpstructure->lpNewLocations);
           lpstructure->lpNewLocations = CopyList2 (m_loc2);

           ires = DlgLocationSelection (hwnd, lpstructure);
           if (ires)
           {
               FreeObjectList (m_loc2); m_loc2 = NULL;
               m_loc2 = CopyList2 (lpstructure->lpNewLocations);
           }
           else
           {
               FreeObjectList (lpstructure->lpNewLocations);
               lpstructure->lpNewLocations = CopyList2 (m_loc2);
           }

           break;
           }
       case IDC_MODIFSTR_IDCHANGESTRUCT:
           lpstructure->nModifyAction = MODIFYACTION_CHGSTORAGE;
           DlgChangeStorageStructure (hwnd, lpstructure);
           break;

       case IDC_MODIFSTR_NBPAGE:
           switch (codeNotify)
           {
               case EN_CHANGE:
               {
                   int  nCount;

                   if (!IsWindowVisible(hwnd))
                       break;

                   // Test to see if the control is empty. If so then
                   // break out.  It becomes tiresome to edit
                   // if you delete all characters and an error box pops up.

                   EnableDisableOKButton (hwnd);
                   nCount = Edit_GetTextLength (hwndCtl);
                   if (nCount == 0)
                       break;

                   VerifyAllNumericEditControls (hwnd, TRUE, TRUE);
                   break;
               }

               case EN_KILLFOCUS:
               {
                   HWND hwndNew = GetFocus();
                   int nNewCtl  = GetDlgCtrlID (hwndNew);

                   if (nNewCtl == IDCANCEL
                       || nNewCtl == IDOK
                       || !IsChild(hwnd, hwndNew))
                       // Dont verify on any button hits
                       break;

                   if (!VerifyAllNumericEditControls(hwnd, TRUE, TRUE))
                       break;
                   UpdateSpinButtons(hwnd);
                   break;
               }
           }
           break;

       case IDC_MODIFSTR_SPIN_NBPAGE:
           ProcessSpinControl (hwndCtl, codeNotify, limits);
           break;


   }
}


static void OnSysColorChange(HWND hwnd)
{
#ifdef _USE_CTL3D
   Ctl3dColorChange();
#endif
}



static BOOL FillControlsFromStructure (HWND hwnd, LPSTORAGEPARAMS lpstructure)
{

   if ( lpstructure->bPhysInconsistent )
   {
      EnableWindow    (GetDlgItem (hwnd, IDC_MARKAS_PHYSINCONSIST), FALSE);
      EnableWindow    (GetDlgItem (hwnd, IDC_MARKAS_PHYSCONSIST)  , TRUE );
   }
   else
   {
      EnableWindow    (GetDlgItem (hwnd, IDC_MARKAS_PHYSCONSIST)   , FALSE);
      EnableWindow    (GetDlgItem (hwnd, IDC_MARKAS_PHYSINCONSIST) , TRUE );
   }

   if ( lpstructure->bLogInconsistent )
   {
      EnableWindow    (GetDlgItem (hwnd, IDC_MARKAS_LOGIINCONSIST), FALSE);
      EnableWindow    (GetDlgItem (hwnd, IDC_MARKAS_LOGICONSIST)  , TRUE );
   }
   else
   {
      EnableWindow    (GetDlgItem (hwnd, IDC_MARKAS_LOGICONSIST)  , FALSE);
      EnableWindow    (GetDlgItem (hwnd, IDC_MARKAS_LOGIINCONSIST), TRUE );
   }


   if ( lpstructure->bRecoveryDisallowed )
   {
      EnableWindow    (GetDlgItem (hwnd, IDC_MARKAS_DISALLOWED), FALSE);
      EnableWindow    (GetDlgItem (hwnd, IDC_MARKAS_ALLOWED)   , TRUE );
   }
   else
   {
      EnableWindow    (GetDlgItem (hwnd, IDC_MARKAS_ALLOWED)   , FALSE);
      EnableWindow    (GetDlgItem (hwnd, IDC_MARKAS_DISALLOWED), TRUE );
   }

   if (lpstructure->nObjectType == OT_TABLE)
   {
      if (lpstructure->bReadOnly)
      {
         EnableWindow (GetDlgItem (hwnd, IDC_MODIFSTR_READONLY)   , FALSE);
         EnableWindow (GetDlgItem (hwnd, IDC_MODIFSTR_NOREADONLY) , TRUE );
      }
      else
      {
         EnableWindow (GetDlgItem (hwnd, IDC_MODIFSTR_NOREADONLY) , FALSE);
         EnableWindow (GetDlgItem (hwnd, IDC_MODIFSTR_READONLY)   , TRUE);
      }
   }
   else
   {
      EnableWindow (GetDlgItem (hwnd, IDC_MODIFSTR_DELETEALLDATA), FALSE);
      EnableWindow (GetDlgItem (hwnd, IDC_MODIFSTR_NOREADONLY)   , FALSE);
      EnableWindow (GetDlgItem (hwnd, IDC_MODIFSTR_READONLY)     , FALSE);
   }
   return TRUE;
}


static BOOL FillStructureFromControls (HWND hwnd, LPSTORAGEPARAMS lpstructure)
{
   char szNumber [MAXOBJECTNAME];
   HWND hwndPages      = GetDlgItem (hwnd, IDC_MODIFSTR_NBPAGE);

   if (lpstructure->nModifyAction == MODIFYACTION_ADDPAGE)
   {
       ZEROINIT (szNumber);
       Edit_GetText (hwndPages, szNumber, sizeof (szNumber));
       lpstructure->lExtend  = atoi (szNumber);
   }
   return TRUE;
}


static void EnableDisableOKButton (HWND hwnd)
{
}


static void ActivateControls (HWND hwnd)
{
   LPSTORAGEPARAMS lpstructure = GetDlgProp (hwnd);
   HWND hwndIdRelocate = GetDlgItem (hwnd, IDC_MODIFSTR_IDLOC01);
   HWND hwndIdLocation = GetDlgItem (hwnd, IDC_MODIFSTR_IDLOC02);
   HWND hwndIdChangeSt = GetDlgItem (hwnd, IDC_MODIFSTR_IDCHANGESTRUCT);
   HWND hwndPages      = GetDlgItem (hwnd, IDC_MODIFSTR_NBPAGE     );
   HWND hwndLPages     = GetDlgItem (hwnd, IDC_MODIFSTR_LNBPAGE    );
   HWND hwndSPages     = GetDlgItem (hwnd, IDC_MODIFSTR_SPIN_NBPAGE);

   if (Button_GetCheck (GetDlgItem (hwnd, IDC_MODIFSTR_RELOC)))
   {
       lpstructure->nModifyAction = MODIFYACTION_RELOC;
       EnableWindow (hwndIdRelocate, TRUE);
   }
   else
       EnableWindow (hwndIdRelocate, FALSE);

   if (Button_GetCheck (GetDlgItem (hwnd, IDC_MODIFSTR_CHANGELOC)))
   {
       lpstructure->nModifyAction = MODIFYACTION_REORG;
       EnableWindow (hwndIdLocation, TRUE);
   }
   else
       EnableWindow (hwndIdLocation, FALSE);

   if (Button_GetCheck (GetDlgItem (hwnd, IDC_MODIFSTR_CHANGESTORAGE)))
   {
       lpstructure->nModifyAction = MODIFYACTION_CHGSTORAGE;
       EnableWindow (hwndIdChangeSt, TRUE);
   }
   else
       EnableWindow (hwndIdChangeSt, FALSE);

   if (Button_GetCheck (GetDlgItem (hwnd, IDC_MODIFSTR_ADDPAGE)))
   {
       lpstructure->nModifyAction = MODIFYACTION_ADDPAGE;
       EnableWindow (hwndLPages, TRUE);
       EnableWindow (hwndPages,  TRUE);
       EnableWindow (hwndSPages, TRUE);
   }
   else
   {
       EnableWindow (hwndLPages, FALSE);
       EnableWindow (hwndPages,  FALSE);
       EnableWindow (hwndSPages, FALSE);
       Edit_SetText (hwndPages,  "");
   }

   if (Button_GetCheck (GetDlgItem (hwnd, IDC_MODIFSTR_SHRINKBTREEIDX)))
   {
       lpstructure->nModifyAction = MODIFYACTION_MERGE;
   }

   if (Button_GetCheck (GetDlgItem (hwnd, IDC_MODIFSTR_DELETEALLDATA)))
   {
       lpstructure->nModifyAction = MODIFYACTION_TRUNC;
   }

   if (Button_GetCheck (GetDlgItem (hwnd, IDC_MODIFSTR_READONLY)))
   {
       lpstructure->nModifyAction = MODIFYACTION_READONLY;
   }

   if (Button_GetCheck (GetDlgItem (hwnd, IDC_MODIFSTR_NOREADONLY)))
   {
       lpstructure->nModifyAction = MODIFYACTION_NOREADONLY;
   }
   if (Button_GetCheck (GetDlgItem (hwnd, IDC_MODIFSTR_READONLY)))
   {
       lpstructure->nModifyAction = MODIFYACTION_READONLY;
   }

   if (Button_GetCheck (GetDlgItem (hwnd,IDC_MARKAS_PHYSINCONSIST )))
   {
       lpstructure->nModifyAction = MODIFYACTION_PHYSINCONSIST;
   }
   if (Button_GetCheck (GetDlgItem (hwnd,IDC_MARKAS_PHYSCONSIST )))
   {
       lpstructure->nModifyAction = MODIFYACTION_PHYSCONSIST;
   }

   if (Button_GetCheck (GetDlgItem (hwnd,IDC_MARKAS_LOGIINCONSIST )))
   {
       lpstructure->nModifyAction = MODIFYACTION_LOGINCONSIST;
   }
   if (Button_GetCheck (GetDlgItem (hwnd,IDC_MARKAS_LOGICONSIST )))
   {
       lpstructure->nModifyAction = MODIFYACTION_LOGCONSIST;
   }

   if (Button_GetCheck (GetDlgItem (hwnd, IDC_MARKAS_DISALLOWED)))
   {
       lpstructure->nModifyAction = MODIFYACTION_RECOVERYDISALLOWED;
   }
   if (Button_GetCheck (GetDlgItem (hwnd, IDC_MARKAS_ALLOWED)))
   {
       lpstructure->nModifyAction = MODIFYACTION_RECOVERYALLOWED;
   }
}

static void InitialiseSpinControls (HWND hwnd)
{
   DWORD dwMin, dwMax;

   GetEditCtrlMinMaxValue   (hwnd, GetDlgItem (hwnd, IDC_MODIFSTR_NBPAGE), limits, &dwMin, &dwMax);
   SpinRangeSet (GetDlgItem (hwnd, IDC_MODIFSTR_SPIN_NBPAGE), dwMin, dwMax);
}

static void InitialiseEditControls (HWND hwnd)
{
   HWND hwndPageNumber    = GetDlgItem (hwnd, IDC_MODIFSTR_NBPAGE);
   SubclassAllNumericEditControls (hwnd, EC_SUBCLASS);

   Edit_LimitText (hwndPageNumber, 7);
   LimitNumericEditControls (hwnd);
}



static BOOL ModifyObject_index (HWND hwnd)
{
   int             ires;
   int             SesHndl;
   INDEXPARAMS     indexparams;
   LPSTORAGEPARAMS lpstructure = GetDlgProp (hwnd);
   int             hdl         = GetCurMdiNodeHandle ();
   LPUCHAR         vnodeName   = GetVirtNodeName (hdl);

   ZEROINIT (indexparams);

   x_strcpy (indexparams.DBName,     lpstructure->DBName);
   x_strcpy (indexparams.objectname, lpstructure->objectname);
   x_strcpy (indexparams.objectowner,lpstructure->objectowner);
   ires = GetDetailInfo (vnodeName, OT_INDEX, &indexparams, TRUE, &SesHndl);

   if (ires != RES_SUCCESS)
   {
      FreeAttachedPointers (&indexparams,  OT_INDEX);
      return FALSE;
   }


   ires = DBAModifyObject (lpstructure, SesHndl);
   FreeAttachedPointers (&indexparams,  OT_INDEX);
   if (ires != RES_SUCCESS)
   {
       switch (lpstructure->nModifyAction)
       {
           case MODIFYACTION_RELOC     :
               ErrorMessage ((UINT) IDS_E_RELOC_INDEX_FAILED, ires);
               break;
           case MODIFYACTION_REORG     :
               ErrorMessage ((UINT) IDS_E_CHANGELOC_INDEX_FAILED, ires);
               break;
           case MODIFYACTION_TRUNC     :
               ErrorMessage ((UINT) IDS_E_DELETE_ALLDATA_FAILED, ires);
               break;
           case MODIFYACTION_MERGE     :
               ErrorMessage ((UINT) IDS_E_SHRINKBTREE_FAILED, ires);
               break;
           case MODIFYACTION_ADDPAGE   :
               ErrorMessage ((UINT) IDS_E_ADDPAGES_FAILED, ires);
               break;
           case MODIFYACTION_CHGSTORAGE:
               ErrorMessage ((UINT) IDS_E_MODIFY_INDEX_STRUCT_FAILED, ires);
               break;
           case MODIFYACTION_READONLY:
           case MODIFYACTION_NOREADONLY:
               //"Error in modify table to [no]readonly"
               MessageWithHistoryButton(GetFocus(), ResourceString(IDS_ERR_MODIF_TBL));
               break;
           case IDC_MARKAS_PHYSINCONSIST:
           case IDC_MARKAS_PHYSCONSIST:
               //"Error in modify table to phys_[in]consistent"
               MessageWithHistoryButton(GetFocus(), ResourceString(IDS_ERR_MODIF_PHYS));
               break;
           case IDC_MARKAS_LOGIINCONSIST:
           case IDC_MARKAS_LOGICONSIST:
               //"Error in modify table to log_[in]consistent"
               MessageWithHistoryButton(GetFocus(), ResourceString(IDS_ERR_MODIF_LOG));
               break;
           case IDC_MARKAS_DISALLOWED:
           case IDC_MARKAS_ALLOWED:
               //"Error in modify table to table_recovery_[dis]allowed."
               MessageWithHistoryButton(GetFocus(), ResourceString(IDS_ERR_MODIF_RECOVERY));
               break;
       }

       return FALSE;
   }

   return(TRUE);
}


static BOOL ModifyObject_table (HWND hwnd)
{
   int             ires, answer;
   int             SesHndl;
   TABLEPARAMS     tableparams;
   LPSTORAGEPARAMS lpstructure = GetDlgProp (hwnd);
   int  hdl                    = GetCurMdiNodeHandle ();
   LPUCHAR vnodeName           = GetVirtNodeName (hdl);

   ZEROINIT (tableparams);

   x_strcpy (tableparams.DBName,     lpstructure->DBName);
   x_strcpy (tableparams.objectname, lpstructure->objectname);
   x_strcpy (tableparams.szSchema,   lpstructure->objectowner);

   ires = GetDetailInfo (vnodeName, OT_TABLE, &tableparams, TRUE, &SesHndl);
   if (ires != RES_SUCCESS)
   {
      FreeAttachedPointers (&tableparams,  OT_TABLE);
      return FALSE;
   }
   answer = IDYES;
   if (lpstructure->nModifyAction == MODIFYACTION_TRUNC)
       answer = ConfirmedMessage ((UINT)IDS_C_DELETE_ALL_DATA);
   if (answer != IDYES)
       return FALSE;

   ires = DBAModifyObject (lpstructure, SesHndl);
   FreeAttachedPointers (&tableparams,  OT_TABLE);
   if (ires != RES_SUCCESS)
   {
       switch (lpstructure->nModifyAction)
       {
           case MODIFYACTION_RELOC     :
               ErrorMessage ((UINT) IDS_E_RELOC_TABLE_FAILED, ires);
               break;
           case MODIFYACTION_REORG     :
               ErrorMessage ((UINT) IDS_E_CHANGELOC_TABLE_FAILED, ires);
               break;
           case MODIFYACTION_TRUNC     :
               ErrorMessage ((UINT) IDS_E_DELETE_ALLDATA_FAILED, ires);
               break;
           case MODIFYACTION_MERGE     :
               ErrorMessage ((UINT) IDS_E_SHRINKBTREE_FAILED, ires);
               break;
           case MODIFYACTION_ADDPAGE   :
               ErrorMessage ((UINT) IDS_E_ADDPAGES_FAILED, ires);
               break;
           case MODIFYACTION_CHGSTORAGE:
               ErrorMessage ((UINT) IDS_E_MODIFY_TABLE_STRUCT_FAILED, ires);
               break;
           case MODIFYACTION_READONLY:
           case MODIFYACTION_NOREADONLY:
               //"Error in modify table to [no]readonly"
               MessageWithHistoryButton(GetFocus(), ResourceString(IDS_ERR_MODIF_TBL));
               break;
           case IDC_MARKAS_PHYSINCONSIST:
           case IDC_MARKAS_PHYSCONSIST:
               //"Error in modify table to phys_[in]consistent"
               MessageWithHistoryButton(GetFocus(), ResourceString(IDS_ERR_MODIF_PHYS));
               break;
           case IDC_MARKAS_LOGIINCONSIST:
           case IDC_MARKAS_LOGICONSIST:
               //"Error in modify table to log_[in]consistent"
               MessageWithHistoryButton(GetFocus(), ResourceString(IDS_ERR_MODIF_LOG));
               break;
           case IDC_MARKAS_DISALLOWED:
           case IDC_MARKAS_ALLOWED:
               //"Error in modify table to table_recovery_[dis]allowed."
               MessageWithHistoryButton(GetFocus(), ResourceString(IDS_ERR_MODIF_RECOVERY));
               break;
       }
       return FALSE;
   }

   return(TRUE);
}


static LPOBJECTLIST CopyList (const LPOBJECTLIST lpInList)
{
   LPOBJECTLIST
       lpOut  = NULL,
       lpList = lpInList;

   while (lpList)
   {
       LPIDXCOLS    lpObj, obj;
       LPOBJECTLIST lpTemp;

       lpTemp = AddListObjectTail (&lpOut, sizeof (IDXCOLS));
       if (!lpTemp)
       {
           // Need to free previously allocated objects.
           FreeObjectList (lpOut);
           return NULL;
       }

       obj   = (LPIDXCOLS) lpList->lpObject;
       lpObj = (LPIDXCOLS) lpTemp->lpObject;
       lpObj->attr.bKey        = obj->attr.bKey;
       lpObj->attr.bDescending = obj->attr.bDescending;
       lpObj->attr.nPrimary    = obj->attr.nPrimary;
       x_strcpy (lpObj->szColName, obj->szColName);

       lpList = lpList->lpNext;
   }
   return lpOut;
}

static LPOBJECTLIST CopyList2 (const LPOBJECTLIST lpInList)
{
   LPOBJECTLIST
       lpOut  = NULL,
       lpList = lpInList;

   while (lpList)
   {
       char*        item;
       LPOBJECTLIST lpTemp;
       item   =(char *)lpList->lpObject;
       suppspace (item);

       lpTemp = AddListObjectTail (&lpOut, x_strlen (item) +1);
       if (!lpTemp)
       {
           // Need to free previously allocated objects.
           FreeObjectList (lpOut);
           return NULL;
       }
       x_strcpy ((char *)lpTemp->lpObject, item);
       //lpOut  = lpTemp;

       lpList = lpList->lpNext;
   }
   return lpOut;
}



