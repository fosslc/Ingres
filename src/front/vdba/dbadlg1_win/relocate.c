/********************************************************************
//
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
//
//    Project  : CA/OpenIngres Visual DBA
//
//    Source   : relocate.c
//
//    Sub Dialog box of Modify structur (src: modifstr.c)
//             Reoganize  the table or index location
//
********************************************************************/

#include "dll.h"
#include "subedit.h"
#include "dbadlg1.h"
#include "dlgres.h"
#include "catocntr.h"   
#include "cntutil.h"  
#include "getdata.h"  
#include "msghandl.h"
#include "domdata.h"
#include "esltools.h"  

static int nLine = 0;


static void OnDestroy        (HWND hwnd);
static void OnCommand        (HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
static BOOL OnInitDialog     (HWND hwnd, HWND hwndFocus, LPARAM lParam);
static void OnSysColorChange (HWND hwnd);
static void FillAllComboCells(HWND hwnd, int maxLine);
static void FillAllData      (HWND hwnd);
static void SelectItemInComboCell     (HWND hwnd, int nLine);
//static void DuplicateLocationList     (HWND hwnd);
static BOOL FillStructureFromControls (HWND hwnd);
static LPCHECKEDOBJECTS GetLocations  ();


BOOL CALLBACK __export DlgRelocateDlgProc
                       (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

BOOL WINAPI __export DlgRelocate (HWND hwndOwner, LPSTORAGEPARAMS lpstructure)
/* ____________________________________________________________________________
// Function:
// Shows the Create/Alter profile dialog.
//
// Paramters:
//     1) hwndOwner:    Handle of the parent window for the dialog.
//     2) lpstructure:  Points to structure containing information used to 
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

   lpProc = MakeProcInstance ((FARPROC) DlgRelocateDlgProc, hInst);
   retVal = VdbaDialogBoxParam
       (hResource,
        MAKEINTRESOURCE (IDD_RELOCATE),
        hwndOwner,
        (DLGPROC) lpProc,
        (LPARAM)  lpstructure
       );

   FreeProcInstance (lpProc);
   return (retVal);
}


BOOL CALLBACK __export DlgRelocateDlgProc
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
   char szFormat [100];
   char szTitle  [MAX_TITLEBAR_LEN];
   char buffer   [MAXOBJECTNAME];
   LPUCHAR parentstrings [MAXPLEVEL];

   HWND hwndContainer = GetDlgItem (hwnd, IDC_RELOCATE_CONTAINER);

   if (!AllocDlgProp (hwnd, lpstructure))
       return FALSE;
   parentstrings [0] = lpstructure->DBName;
   if (lpstructure->nObjectType == OT_TABLE)
   {
       LoadString (hResource, (UINT)IDS_T_RELOCATE_TABLE, szFormat, sizeof (szFormat));
       GetExactDisplayString (
           lpstructure->objectname,
           lpstructure->objectowner,
           OT_TABLE,
           parentstrings,
           buffer);
       lpHelpStack = StackObject_PUSH (lpHelpStack, StackObject_INIT ((UINT)IDD_RELOCATE));
   }
   else
   {
       LoadString (hResource, (UINT)IDS_T_RELOCATE_INDEX, szFormat, sizeof (szFormat));
       GetExactDisplayString (
           lpstructure->objectname,
           lpstructure->objectowner,
           OT_INDEX,
           parentstrings,
           buffer);
       lpHelpStack = StackObject_PUSH (lpHelpStack, StackObject_INIT ((UINT)IDD_RELOC_IDX));
   }

   wsprintf (
       szTitle,
       szFormat,
       GetVirtNodeName (GetCurMdiNodeHandle ()),
       lpstructure->DBName,
       buffer);
   SetWindowText (hwnd, szTitle);
   
   //DuplicateLocationList   (hwnd);
   InitContainer           (hwndContainer, hwnd, 2);
   ContainerDisableDisplay (hwndContainer);
   FillAllData             (hwnd);
   ContainerEnableDisplay  (hwndContainer, TRUE);
   richCenterDialog        (hwnd);

   return TRUE;
}


static void OnDestroy (HWND hwnd)
{
   DeallocDlgProp     (hwnd);
   TerminateContainer (GetDlgItem (hwnd, IDC_RELOCATE_CONTAINER));
   lpHelpStack = StackObject_POP (lpHelpStack);
}


static void OnCommand (HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
   // Emb July 01, 97: found Useless and even gpfs with new container
   // since it sends notification messages BEFORE initdialog had a chance
   // to allocate dialog property
   //LPSTORAGEPARAMS lpstructure = GetDlgProp (hwnd);

   switch (id)
   {
       case IDOK:
           if (FillStructureFromControls (hwnd))
               EndDialog (hwnd, TRUE);
           else
               break;
           break;

       case IDCANCEL:
           EndDialog (hwnd, FALSE);
           break;

       case IDC_RELOCATE_CONTAINER:
           switch (codeNotify)
           {
               case CN_RECSELECTED:
                   //
                   // Selection has changed
                   //
                   ContainerNewSel (hwndCtl);
                   break;

               case CN_FLDSIZED:
               case CN_HSCROLL_PAGEUP:
               case CN_HSCROLL_PAGEDOWN:
               case CN_HSCROLL_LINEUP:
               case CN_HSCROLL_LINEDOWN:
               case CN_HSCROLL_THUMBPOS:
                   ContainerUpdateCombo (hwndCtl);
                   break;

               default:
                   break;
           }
           break;
   }
}


static void OnSysColorChange(HWND hwnd)
{
#ifdef _USE_CTL3D
   Ctl3dColorChange();
#endif
}

static void FillAllComboCells(HWND hwnd, int maxLine)
{
   HWND hwndContainer = GetDlgItem (hwnd, IDC_RELOCATE_CONTAINER);
   LPSTORAGEPARAMS  lpstorage = GetDlgProp (hwnd);
   LPCHECKEDOBJECTS ls, lploc = GetLocations ();
   int     i, maxlen = 0;
   char*   item;
   char*   buffer;
   char*   first;

   ls = lploc;
   while (ls)
   {
       item   = (char*) ls->dbname;
       maxlen = maxlen + x_strlen (item) +1;

       ls = ls->pnext;
   }
   if (maxlen > 0)
   {
       buffer = ESL_AllocMem (maxlen +2);
       first  = buffer;

       ls = lploc;
       x_strcpy (buffer, (char*) ls->dbname);
       ls = ls->pnext;

       while (ls)
       {
           item   = (char*)  ls->dbname;
           buffer = buffer + x_strlen (buffer) +1;
           x_strcpy (buffer, item);

           ls = ls->pnext;
       }
       buffer = buffer + x_strlen (buffer) +1;
       x_strcpy (buffer, "");

       //
       // Fill all combos
       //

       for (i=1; i<maxLine; i++)
       {
           ContainerFillComboCell(
               hwndContainer,
               i,
               1,
               0,
               CONT_ALIGN_LEFT,
               first);
       }
       ESL_FreeMem (first);
       first = NULL;
   }
   lploc = FreeCheckedObjects (lploc);
}


static LPCHECKEDOBJECTS GetLocations ()
{
   int  hdl, ires;
   BOOL bwsystem;
   char buf [MAXOBJECTNAME];
   char buffilter [MAXOBJECTNAME];
   LPCHECKEDOBJECTS obj, list = NULL;  
   ZEROINIT (buf);
   ZEROINIT (buffilter);

   hdl      = GetCurMdiNodeHandle ();
   // Force to TRUE so as II_DATABASE is picked up.
   bwsystem = TRUE;
       
   ires     = DOMGetFirstObject (hdl, OT_LOCATION, 0, NULL, bwsystem, NULL, buf, NULL, NULL);
   while (ires == RES_SUCCESS)
   {
      BOOL bOK;
      if (DOMLocationUsageAccepted(hdl,buf,LOCATIONDATABASE,&bOK)==
          RES_SUCCESS && bOK) {
          obj = ESL_AllocMem (sizeof (CHECKEDOBJECTS));
          if (obj)
          {
             x_strcpy (obj->dbname, buf);
             obj->pnext = NULL;
             list = AddCheckedObject (list, obj);
          }
      }
      ires  = DOMGetNextObject (buf, buffilter, NULL);
   }
   return list;
}





static void FillAllData (HWND hwnd)
{
   HWND  hwndContainer = GetDlgItem (hwnd, IDC_RELOCATE_CONTAINER);
   LPSTORAGEPARAMS  lpstorage = GetDlgProp (hwnd);
   LPCHECKEDOBJECTS lploc = GetLocations ();
   LPOBJECTLIST lx;
   int   i = 1;
   char* item;

   nLine = ContainerAddCntLine  (hwndContainer);
   ContainerFillTextCell (hwndContainer, 0, 0, CONT_COLTYPE_TITLE, CONT_ALIGN_LEFT, ResourceString ((UINT)IDS_I_OLD_LOCATIONS));
   ContainerFillTextCell (hwndContainer, 0, 1, CONT_COLTYPE_TITLE, CONT_ALIGN_LEFT, ResourceString ((UINT)IDS_I_NEW_LOCATIONS));

   lx    = lpstorage->lpLocations;
   while (lx)
   {
       nLine = ContainerAddCntLine  (hwndContainer);
       lx = lx->lpNext;
   }

   ContainerSetColumnWidth (hwndContainer, 0, 26);
   ContainerSetColumnWidth (hwndContainer, 1, 26);

   lx    = lpstorage->lpLocations;
   while (lx)
   {
       item = (char *)lx->lpObject;
       ContainerFillTextCell(
           hwndContainer,
           i,
           0,
           CONT_COLTYPE_TEXT,
           CONT_ALIGN_LEFT,
           item);

       lx = lx->lpNext;
       i++;
   }

   lploc = FreeCheckedObjects (lploc);
   FillAllComboCells          (hwnd, nLine+1);
   SelectItemInComboCell      (hwnd, nLine);
}


static BOOL FillStructureFromControls (HWND hwnd)
{
   int  i;
   char buffer [MAXOBJECTNAME];
   LPOBJECTLIST
       list = NULL,
       obj;
   LPSTORAGEPARAMS  lpstorage = GetDlgProp (hwnd);
   HWND hwndContainer         = GetDlgItem (hwnd, IDC_RELOCATE_CONTAINER);


   // Mandatory in case current cell is a combo
   ContainerNewSel(hwndContainer);

   for (i=1; i<= nLine; i++)
   {
       ZEROINIT (buffer);
       ContainerGetComboCellSelection (hwndContainer, i, 1, buffer);
     
       obj = AddListObjectTail (&list, x_strlen (buffer) +1);
       if (obj)
       {
           x_strcpy ((UCHAR *)obj->lpObject, buffer);
           //list = obj;
       }
       else
       {
           FreeObjectList (list);
           list = NULL;
           return FALSE;
       }
   }
   FreeObjectList (lpstorage->lpNewLocations);
   lpstorage->lpNewLocations = NULL;
   lpstorage->lpNewLocations = list;

   return TRUE;
}

static void SelectItemInComboCell (HWND hwnd, int nLine)
{
   int             i;
   LPOBJECTLIST    lx;
   char*           item;
   LPSTORAGEPARAMS lpstorage = GetDlgProp (hwnd);
   HWND hwndContainer        = GetDlgItem (hwnd, IDC_RELOCATE_CONTAINER);

   lx    = lpstorage->lpNewLocations;
   i     = 1;
   while (lx)
   {
       item = (char *)lx->lpObject;
       ContainerSetComboSelection (
           hwndContainer,
           i,
           1,
           item);
           
       lx = lx->lpNext;
       i++;
       if (i > nLine) break;
   }
}
/*
static void DuplicateLocationList (HWND hwnd)
{
   LPOBJECTLIST
       lx,
       obj,
       lpList = NULL;
   char*
       item;
   LPSTORAGEPARAMS
       lpstorage = GetDlgProp (hwnd);

   lx    = lpstorage->lpLocations;
   while (lx)
   {
       item = (char *)lx->lpObject;
       
       obj = AddListObject (lpList, x_strlen (item) +1);
       if (obj)
       {
           x_strcpy ((UCHAR *)obj->lpObject, item);
           lpList = obj;
       }
       else
       {
           //
           // Cannot allocate memory
           //
           FreeObjectList (lpList);
           lpList = NULL;
       }
           
       lx = lx->lpNext;
   }
   lpstorage->lpNewLocations = lpList;
}
*/
