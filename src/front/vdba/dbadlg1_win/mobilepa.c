/********************************************************************
//
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
//
//    Project  : CA/OpenIngres Visual DBA
//
//    Source   : mobilpa.c
//
//    select Mobile parameters for replicator 1.05
//
********************************************************************/


#include "dll.h"     
#include "dlgres.h"
#include "msghandl.h"
#include "dbaset.h"
#include "domdata.h"
#include "getdata.h"


#define REPLIC_NOCOLLISISION    0
#define REPLIC_QUEUECLEANING    1
#define REPLIC_LASTWRITECOLL    2
#define REPLIC_FIRSTWRITECOLL   3

CTLDATA typeMobileCollisions[] =
{
   REPLIC_NOCOLLISISION,    IDS_NOCOLLISISION,
   REPLIC_QUEUECLEANING,    IDS_QUEUECLEANING,
   REPLIC_LASTWRITECOLL,    IDS_LASTWRITECOLL,
   REPLIC_FIRSTWRITECOLL,   IDS_FIRSTWRITECOLL,
   -1                  // terminator
};

#define REPLIC_ROLLBACKRETRY         1
#define REPLIC_ROLLBACKQUIET         2
#define REPLIC_ROLLBACKRECONF        3
#define REPLIC_ROLLBACKSKIP          4

CTLDATA typeErrorMode[] =
{
   REPLIC_ROLLBACKRETRY ,   IDS_ROLLBACKRETRY,
   REPLIC_ROLLBACKQUIET ,   IDS_ROLLBACKQUIET,
   REPLIC_ROLLBACKRECONF,   IDS_ROLLBACKRECONF,
   REPLIC_ROLLBACKSKIP  ,   IDS_ROLLBACKSKIP ,
   -1                  // terminator
};

static DWORD FindItemData( HWND hwndCtl, DWORD ItemValue)
{
  DWORD ItemMax ,value;
  DWORD i;
  ItemMax=ComboBox_GetCount(hwndCtl);
  for (i=0;i<ItemMax;i++) {
    value = ComboBox_GetItemData(hwndCtl, i);
    if (value == ItemValue)
      return i;
  }
  return (DWORD)CB_ERR;
}

BOOL CALLBACK __export DlgMobileParamDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

static void OnDestroy                  (HWND hwnd);
static void OnCommand                  (HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
static BOOL OnInitDialog               (HWND hwnd, HWND hwndFocus, LPARAM lParam);
static void OnSysColorChange           (HWND hwnd);

static BOOL OccupyTypesCollisions      (HWND hwnd);
static BOOL OccupyTypesErrors          (HWND hwnd);
static BOOL FillStructureFromControls  (HWND hwnd, LPREPMOBILE lpreplmob);
static BOOL FillControlFromStructure   (HWND hwnd, LPREPMOBILE lpreplmob);


int  WINAPI __export DlgMobileParam (HWND hwndOwner, LPREPMOBILE lpmobileparam)
/*
//   Function:
//   configure Mobile parameters for Replication 1.05 .
//
//   Paramters:
//       hwndOwner      - Handle of the parent window for the dialog.
//       lpmobileparam  - Points to structure containing information used to 
//                        initialise the dialog. The dialog uses the same
//                        structure to return the result of the dialog.
//
//   Returns:
//       TRUE if successful.
//
*/
{
   FARPROC lpProc;
   int     retVal;

   if (!IsWindow(hwndOwner) || !lpmobileparam)
   {
       ASSERT(NULL);
       return FALSE;
   }

   lpProc = MakeProcInstance ((FARPROC) DlgMobileParamDlgProc, hInst);
   retVal = VdbaDialogBoxParam(hResource,
                               MAKEINTRESOURCE (IDD_MOBILE_PARAM),
                               hwndOwner,
                               (DLGPROC) lpProc,
                               (LPARAM)  lpmobileparam
                               );

   FreeProcInstance (lpProc);
   return (retVal);
}


BOOL CALLBACK __export DlgMobileParamDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
   switch (message)
   {
       HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
       HANDLE_MSG(hwnd, WM_DESTROY, OnDestroy);

       case WM_INITDIALOG:
           return HANDLE_WM_INITDIALOG (hwnd, wParam, lParam, OnInitDialog);

       default:
           return HandleUserMessages(hwnd, message, wParam, lParam);
   }
   return TRUE;
}


static BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
   LPREPMOBILE lprepmob = (LPREPMOBILE) lParam;

   if (!AllocDlgProp(hwnd, lprepmob))
       return FALSE;

   lpHelpStack = StackObject_PUSH (lpHelpStack,
                                   StackObject_INIT ((UINT)IDD_MOBILE_PARAM));

   if (!OccupyTypesCollisions(hwnd)|| !OccupyTypesErrors(hwnd))
   {
       ASSERT(NULL);
       return FALSE;
   }
   
   FillControlFromStructure(hwnd, lprepmob);

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
  LPREPMOBILE lprepmob = GetDlgProp(hwnd);
  BOOL Success;
  int  ires;
   
  switch (id)
  {
    case IDOK:
      Success = FillStructureFromControls (hwnd, lprepmob);
      if (!Success)
        break;

      ires = DBAUpdMobile(GetVirtNodeName ( GetCurMdiNodeHandle ()),
                          lprepmob);

      if (ires != RES_SUCCESS)  {
        ErrorMessage ((UINT) IDS_E_REPL_MOBIL_FAILED, ires);
        break;
      }
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

static BOOL OccupyTypesCollisions (HWND hwnd)
/*
// Function:
//     Fills the Collision drop down box with the Collisions names.
//     Replicator version 1.05
//
// Parameters:
//     hwnd - Handle to the dialog window.
// 
// Returns:
//     TRUE if successful.
*/
{
   HWND hwndCtl = GetDlgItem (hwnd, IDC_MOBIL_COLLISION);
   return OccupyComboBoxControl(hwndCtl, typeMobileCollisions);
}

static BOOL OccupyTypesErrors (HWND hwnd)
/*
// Function:
//     Fills the Error drop down box with the errors names.version 1.05
//
// Parameters:
//     hwnd - Handle to the dialog window.
// 
// Returns:
//     TRUE if successful.
*/
{
   HWND hwndCtl = GetDlgItem (hwnd, IDC_MOBIL_ERROR);
   return OccupyComboBoxControl(hwndCtl, typeErrorMode);

}

static BOOL FillStructureFromControls  (HWND hwnd, LPREPMOBILE lpreplmob)
{
  DWORD indexCollision,indexError;
  HWND hwndCollision = GetDlgItem(hwnd, IDC_MOBIL_COLLISION);
  HWND hwndError     = GetDlgItem(hwnd, IDC_MOBIL_ERROR);

  indexCollision            = ComboBox_GetCurSel(hwndCollision);

  if (indexCollision == CB_ERR)
    return(FALSE);
  lpreplmob->collision_mode = ComboBox_GetItemData(hwndCollision, indexCollision);

  indexError                = ComboBox_GetCurSel(hwndError);

  if (indexError == CB_ERR)
    return(FALSE);
  lpreplmob->error_mode     = ComboBox_GetItemData(hwndError, indexError);

  return TRUE;
}

static BOOL FillControlFromStructure (HWND hwnd, LPREPMOBILE lpreplmob)
{
  DWORD nbIndex; 
  HWND hwndCollision = GetDlgItem(hwnd, IDC_MOBIL_COLLISION);
  HWND hwndError     = GetDlgItem(hwnd, IDC_MOBIL_ERROR);
  
  nbIndex = FindItemData(hwndCollision, lpreplmob->collision_mode );
  if (nbIndex != CB_ERR)
      ComboBox_SetCurSel(hwndCollision, nbIndex );
  nbIndex = FindItemData(hwndError ,lpreplmob->error_mode);
  if (nbIndex != CB_ERR)
      ComboBox_SetCurSel(hwndError, nbIndex );  
  return TRUE;
}
