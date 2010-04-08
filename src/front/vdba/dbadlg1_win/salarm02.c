/********************************************************************
//
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
//
//    Project  : CA/OpenIngres Visual DBA
//
//    Source   : salarm02.c
//
//    Dialog box for Creating a Security alarm dialog
// History:
// uk$so01 (20-Jan-2000, Bug #100063)
//         Eliminate the undesired compiler's warning
//
// 21-Mar-2001 (schph01)
//        bug 104269: Fill the dbevent combobox with the dbevent existing in
//        iidbdb database instead of the current database.
// 21-Mars-2001 (schph01)
//    bug 104091 Add EscapedSimpleQuote() function : escaped all the
//    simple quotes find in dbevent text.
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
#include "resource.h"

char szSecurityDBEvent [MAXOBJECTNAME];
BOOL bNoDisplayMessageDB = FALSE;
BOOL CALLBACK __export DlgCreateSecurityAlarm2DlgProc
                       (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

static void OnDestroy                  (HWND hwnd);
static void OnCommand                  (HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
static BOOL OnInitDialog               (HWND hwnd, HWND hwndFocus, LPARAM lParam);
static void OnSysColorChange           (HWND hwnd);
static void PreCheckItem               (HWND hwnd);
static void EnableDisableOKButton      (HWND hwnd);
static BOOL FillStructureFromControls  (HWND hwnd, LPSECURITYALARMPARAMS lpsecurity);
static void EnableControl              (HWND hwnd, BOOL bEnable);
//static LPCHECKOBJECTS AddItemToList2   (HWND hwndCtl);

int WINAPI __export DlgCreateSecurityAlarm2
                    (HWND hwndOwner, LPSECURITYALARMPARAMS lpsecurityalarmparams)
/*
// Function:
// Shows the Create Security alarm dialog.
//
// Paramters:
//     1) hwndOwner:               Handle of the parent window for the dialog.
//     2) lpsecurityalarmparams:   Points to structure containing information used to 
//                                 initialise the dialog. The dialog uses the same
//                                 structure to return the result of the dialog.
//     
// Returns:
//    TRUE if successful.
*/
{
   FARPROC lpProc;
   int     retVal;

   if (!IsWindow (hwndOwner) || !lpsecurityalarmparams)
   {
       ASSERT(NULL);
       return FALSE;
   }
   
   lpProc = MakeProcInstance ((FARPROC) DlgCreateSecurityAlarm2DlgProc, hInst);
   retVal = VdbaDialogBoxParam
       (hResource,
       MAKEINTRESOURCE (IDD_SECURITY_ALARM2),
       hwndOwner,
       (WNDPROC) lpProc,
       (LPARAM)  lpsecurityalarmparams );

   FreeProcInstance (lpProc);
   return (retVal);
}

BOOL CALLBACK __export DlgCreateSecurityAlarm2DlgProc
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

   LPSECURITYALARMPARAMS lpsecurity   = (LPSECURITYALARMPARAMS)lParam;
   HWND hwndUsers    = GetDlgItem (hwnd, IDC_SALARM_BYUSER );
   HWND hwndDB       = GetDlgItem (hwnd, IDC_SALARM_ONTABLE);
   HWND hwndDBE      = GetDlgItem (hwnd, IDC_SALARM_DBEVENT);
   HWND hwndCaptionDB= GetDlgItem (hwnd, IDC_SALARM_STATIC_DB);

   char szFormat [100];
   char szTitle  [180];

   if (!AllocDlgProp (hwnd, lpsecurity))
       return FALSE;

   bNoDisplayMessageDB = FALSE;
   //
   // force catolist.dll to load
   //
   CATOListDummy();

   if (lpsecurity->bInstallLevel) {
      SetWindowText (hwndCaptionDB, "On:");
      //"Create Security Alarm on Current Installation on %s"
      wsprintf (szTitle, ResourceString(IDS_F_CREATE_SECURITY),
                GetVirtNodeName ( GetCurMdiNodeHandle ()));
   }
   else {
      LoadString (hResource, (UINT)IDS_T_CREATE_SECURITY, szFormat, sizeof (szFormat));
      wsprintf (szTitle, szFormat,
                GetVirtNodeName ( GetCurMdiNodeHandle ()),
                lpsecurity->DBName);

   }


   
   SetWindowText (hwnd, szTitle);

   Edit_LimitText (GetDlgItem (hwnd, IDC_SALARM_DBEVENT_TEXT), MAXOBJECTNAME-1);
   ZEROINIT (szSecurityDBEvent);
   LoadString (hResource, (UINT)IDS_I_NODBEVENT, szSecurityDBEvent, sizeof (szSecurityDBEvent));

   //
   // Set the default to user
   //
   Button_SetCheck (GetDlgItem (hwnd, IDC_SALARM_USER), TRUE);

   //
   // Get the available users names and insert them into the CA list box 
   // 

   CAListBoxFillUsers (hwndUsers);

   //
   // Get the available DB names and insert them into the table list 
   // 
   if (lpsecurity->bInstallLevel) {
       char * pcurinst = ResourceString(IDS_CURRENT_INSTALLATION);
       CAListBox_AddString   (hwndDB,pcurinst);
       CAListBox_SelectString(hwndDB,-1,pcurinst);
       lpsecurity->iObjectType = OT_VIRTNODE;
       EnableWindow(hwndDB,FALSE);
       EnableWindow(hwndDBE,FALSE);
   }
   else  {
       CAListBoxFillDatabases (hwndDB);
       lpsecurity->iObjectType = OT_DATABASE;
   }

   if (!lpsecurity->bInstallLevel &&
       !ComboBoxFillDBevents (hwndDBE, "iidbdb"))
   {
       ComboBoxDestroyItemData (hwndDBE);
       return FALSE;
   }

   {
       int   k;
       char* buffowner;

       k = ComboBox_AddString (hwndDBE, szSecurityDBEvent);
       buffowner = ESL_AllocMem (x_strlen (szSecurityDBEvent) +1);
       x_strcpy (buffowner, szSecurityDBEvent);
       ComboBox_SetItemData (hwndDBE, k, buffowner);
       ComboBox_SelectString(hwndDBE, -1, szSecurityDBEvent);
       EnableControl (hwnd, FALSE);
   }

   PreCheckItem (hwnd);
   EnableDisableOKButton (hwnd);
   //
   // The value 9039 is defined in MAINMFC.H but is not accessible from this file:
   lpHelpStack = StackObject_PUSH (lpHelpStack, StackObject_INIT (lpsecurity->bInstallLevel? 9039: (UINT)IDD_SECURITY_ALARM2));

   richCenterDialog (hwnd);
   return TRUE;
}



static void OnDestroy (HWND hwnd)
{
   HWND hwndDBE      = GetDlgItem (hwnd, IDC_SALARM_DBEVENT);
   
   ComboBoxDestroyItemData (hwndDBE);
   DeallocDlgProp (hwnd);
   lpHelpStack = StackObject_POP (lpHelpStack);
}


static void OnCommand (HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
   LPSECURITYALARMPARAMS lpsecurity = GetDlgProp (hwnd);
   HWND hwndUsers    = GetDlgItem (hwnd, IDC_SALARM_BYUSER );
   HWND hwndTables = GetDlgItem (hwnd, IDC_SALARM_ONTABLE );
   HWND hwndName   = GetDlgItem (hwnd, IDC_SALARM_NAME );        

   int idu;    
   int i, n, idt;   
   int ires;
   char szName [MAXOBJECTNAME];

   switch (id)
   {
       case IDOK:
           if (!FillStructureFromControls (hwnd, lpsecurity))
               break;
           //
           // Call the low level function to write data on disk
           //

           ires = DBAAddObject
               (GetVirtNodeName (GetCurMdiNodeHandle ()),
               OTLL_SECURITYALARM,
               (void *) lpsecurity );

           if (ires != RES_SUCCESS)
           {
               FreeAttachedPointers (lpsecurity, OTLL_SECURITYALARM);
               ErrorMessage   ((UINT)IDS_E_CREATE_SECURITY_FAILED, ires);
               break;
           }
           else
               EndDialog (hwnd, TRUE);
           
           FreeAttachedPointers (lpsecurity, OTLL_SECURITYALARM);
           break;

       case IDCANCEL:
           EndDialog (hwnd, FALSE);
           FreeAttachedPointers (lpsecurity, OTLL_SECURITYALARM);
           break;

       case IDC_SALARM_BYUSER:
           if (Edit_GetText (hwndName, szName, sizeof (szName)) != 0 &&
              CAListBox_GetSelCount (hwndUsers) > 1)   {
                idu=CAListBox_GetCurSel (hwndUsers);
                n = CAListBox_GetCount (hwndUsers);
                for (i=0; i<n; i++)
                       CAListBox_SetSel (hwndUsers, FALSE, i);
               CAListBox_SetCurSel (hwndUsers, idu);
               CAListBox_SetSel (hwndUsers, TRUE, idu);
               //"Multiple auth-id is not allowed if the security alarm has a name."
               MessageBox(hwnd,
                   ResourceString(IDS_ERR_USING_CHECKPOINT),
                   NULL, MB_OK | MB_ICONEXCLAMATION | MB_TASKMODAL);
                   EnableDisableOKButton (hwnd);
             }
           else if (CAListBox_GetSelCount (hwndUsers) > 1)
                  Edit_Enable(hwndName,FALSE);
                else
                  Edit_Enable(hwndName,TRUE);
               
           EnableDisableOKButton (hwnd);
           break;

       case IDC_SALARM_ONTABLE:
           if (codeNotify == CALBN_CHECKCHANGE) {
               idt = CAListBox_GetCurSel (hwndTables);              
                n = CAListBox_GetCount (hwndTables);                
       
                for (i=0; i<n; i++)
                  CAListBox_SetSel (hwndTables, FALSE, i);      
           
               CAListBox_SetCurSel (hwndTables, idt);              
                CAListBox_SetSel (hwndTables, TRUE, idt);
               if (bNoDisplayMessageDB == FALSE) {
                  int iret;
                  //"Only one database can be checked. Previous database, if any, was unchecked.\nDon't display this message any more?"
                  iret = MessageBox(hwnd, ResourceString(IDS_ERR_DATABASE_BE_CHECKED),
                         NULL, MB_OKCANCEL | MB_ICONEXCLAMATION | MB_TASKMODAL);
                  if (iret == IDOK)
                     bNoDisplayMessageDB=TRUE;
               }
           }
           EnableDisableOKButton (hwnd);
           break;

       case IDC_SALARM_SUCCESS:
       case IDC_SALARM_FAILURE:
       case IDC_SALARM_SUCCESSFAILURE:
       case IDC_SALARM_SELECT:
       case IDC_SALARM_DELETE:
       case IDC_SALARM_INSERT:
       case IDC_SALARM_UPDATE:
       case IDC_SALARM_CONNECT:
       case IDC_SALARM_DISCONNECT:
           EnableDisableOKButton (hwnd);
           break;

       case IDC_SALARM_USER:
           if (Button_GetCheck (hwndCtl))
           {
               CAListBox_ResetContent (hwndUsers);
               CAListBoxFillUsers     (hwndUsers);
               EnableDisableOKButton (hwnd);
           }
           break;

       case IDC_SALARM_GROUP:
           if (Button_GetCheck (hwndCtl))
           {
               CAListBox_ResetContent (hwndUsers);
               CAListBoxFillGroups    (hwndUsers);
               EnableDisableOKButton (hwnd);
           }
           break;

       case IDC_SALARM_ROLE:
           if (Button_GetCheck (hwndCtl))
           {
               CAListBox_ResetContent (hwndUsers);
               CAListBoxFillRoles     (hwndUsers);
               EnableDisableOKButton  (hwnd);
           }
           break;

       case IDC_SALARM_PUBLIC:
           if (Button_GetCheck (hwndCtl))
           {
               CAListBox_ResetContent (hwndUsers);
               EnableDisableOKButton  (hwnd);
           }
           break;

       case IDC_SALARM_DBEVENT:
           if (codeNotify == CBN_SELCHANGE)
           {
               char szItem [MAXOBJECTNAME];

               ZEROINIT (szItem);
               ComboBox_GetText (hwndCtl, szItem, sizeof (szItem));

               if (x_strcmp (szItem, szSecurityDBEvent) == 0)
               {
                   lpsecurity->bDBEvent = FALSE;
                   EnableControl (hwnd, FALSE);
               }
               else
                   EnableControl (hwnd, TRUE);
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


static void EnableDisableOKButton  (HWND hwnd)
{
   HWND hwndUsers   = GetDlgItem (hwnd, IDC_SALARM_BYUSER );
   HWND hwndDB  = GetDlgItem (hwnd, IDC_SALARM_ONTABLE);
   HWND hwndSuccess = GetDlgItem (hwnd, IDC_SALARM_SUCCESS);
   HWND hwndFailure = GetDlgItem (hwnd, IDC_SALARM_FAILURE);
   HWND hwndSucFail = GetDlgItem (hwnd, IDC_SALARM_SUCCESSFAILURE);
   HWND hwndSelect  = GetDlgItem (hwnd, IDC_SALARM_SELECT);
   HWND hwndDelete  = GetDlgItem (hwnd, IDC_SALARM_DELETE);
   HWND hwndInsert  = GetDlgItem (hwnd, IDC_SALARM_INSERT);
   HWND hwndUpdate  = GetDlgItem (hwnd, IDC_SALARM_UPDATE);
   HWND hwndConnect    = GetDlgItem (hwnd, IDC_SALARM_CONNECT);
   HWND hwndDisconnect = GetDlgItem (hwnd, IDC_SALARM_DISCONNECT);

   int  table_sel_count = CAListBox_GetSelCount (hwndDB);
   int  user_sel_count  = CAListBox_GetSelCount (hwndUsers);
   BOOL rb = Button_GetCheck (hwndSuccess) || Button_GetCheck (hwndFailure) \
          || Button_GetCheck (hwndSucFail);
   BOOL tb = Button_GetCheck (hwndSelect)  || Button_GetCheck (hwndDelete) \
          || Button_GetCheck (hwndInsert)  || Button_GetCheck (hwndUpdate) \
          || Button_GetCheck (hwndConnect) || Button_GetCheck (hwndDisconnect);

   if (!Button_GetCheck (GetDlgItem (hwnd, IDC_SALARM_PUBLIC)))
   {
       if ((table_sel_count == 0) || (user_sel_count == 0) || (!tb) || (!rb))
           EnableWindow (GetDlgItem (hwnd, IDOK), FALSE);
       else
           EnableWindow (GetDlgItem (hwnd, IDOK), TRUE);
   }
   else
   {
       if ((table_sel_count == 0) || (!tb) || (!rb))
           EnableWindow (GetDlgItem (hwnd, IDOK), FALSE);
       else
           EnableWindow (GetDlgItem (hwnd, IDOK), TRUE);

   }
}

static void PreCheckItem (HWND hwnd)
{
   LPSECURITYALARMPARAMS lpsecurity = GetDlgProp (hwnd);
   HWND hwndUsers   = GetDlgItem (hwnd, IDC_SALARM_BYUSER );
   HWND hwndDB  = GetDlgItem (hwnd, IDC_SALARM_ONTABLE);
   HWND hwndSuccess = GetDlgItem (hwnd, IDC_SALARM_SUCCESS);
   HWND hwndFailure = GetDlgItem (hwnd, IDC_SALARM_FAILURE);
   HWND hwndSelect  = GetDlgItem (hwnd, IDC_SALARM_SELECT);
   HWND hwndDelete  = GetDlgItem (hwnd, IDC_SALARM_DELETE);
   HWND hwndInsert  = GetDlgItem (hwnd, IDC_SALARM_INSERT);
   HWND hwndUpdate  = GetDlgItem (hwnd, IDC_SALARM_UPDATE);
   HWND hwndConnect    = GetDlgItem (hwnd, IDC_SALARM_CONNECT);
   HWND hwndDisconnect = GetDlgItem (hwnd, IDC_SALARM_DISCONNECT);

   Button_SetCheck (hwndSuccess, lpsecurity->bsuccfail  [SECALARMSUCCESS]);
   Button_SetCheck (hwndFailure, lpsecurity->bsuccfail  [SECALARMFAILURE]);

   Button_SetCheck (hwndSelect,    lpsecurity->baccesstype [SECALARMSEL]);
   Button_SetCheck (hwndDelete,    lpsecurity->baccesstype [SECALARMDEL]);
   Button_SetCheck (hwndInsert,    lpsecurity->baccesstype [SECALARMINS]);
   Button_SetCheck (hwndUpdate,    lpsecurity->baccesstype [SECALARMUPD]);
   Button_SetCheck (hwndConnect   ,lpsecurity->baccesstype [SECALARMCONNECT]);
   Button_SetCheck (hwndDisconnect,lpsecurity->baccesstype [SECALARMDISCONN]);

   if (x_strlen (lpsecurity->PreselectTable) > 0) // DB Name
       CAListBox_SelectString (hwndDB, -1, lpsecurity->PreselectTable);
}

static BOOL FillStructureFromControls (HWND hwnd, LPSECURITYALARMPARAMS lpsecurity)
{
   char buf        [MAXOBJECTNAME];
//   char buffowner  [MAXOBJECTNAME];
//   char bufftable  [MAXOBJECTNAME];
   char szDBE      [MAXOBJECTNAME];
   char szName     [MAXOBJECTNAME];

   LPCHECKEDOBJECTS  obj;
   LPCHECKEDOBJECTS  list = NULL;
   int i, max_database_number, checked;

   HWND hwndDB         = GetDlgItem (hwnd, IDC_SALARM_ONTABLE);
   HWND hwndUsers      = GetDlgItem (hwnd, IDC_SALARM_BYUSER );
   HWND hwndSuccess    = GetDlgItem (hwnd, IDC_SALARM_SUCCESS);
   HWND hwndFailure    = GetDlgItem (hwnd, IDC_SALARM_FAILURE);
   HWND hwndSelect     = GetDlgItem (hwnd, IDC_SALARM_SELECT);
   HWND hwndDelete     = GetDlgItem (hwnd, IDC_SALARM_DELETE);
   HWND hwndInsert     = GetDlgItem (hwnd, IDC_SALARM_INSERT);
   HWND hwndUpdate     = GetDlgItem (hwnd, IDC_SALARM_UPDATE);
   HWND hwndConnect    = GetDlgItem (hwnd, IDC_SALARM_CONNECT);
   HWND hwndDisconnect = GetDlgItem (hwnd, IDC_SALARM_DISCONNECT);
   HWND hwndDBE        = GetDlgItem (hwnd, IDC_SALARM_DBEVENT);
   HWND hwndDBEText    = GetDlgItem (hwnd, IDC_SALARM_DBEVENT_TEXT);
   HWND hwndName       = GetDlgItem (hwnd, IDC_SALARM_NAME);

   x_strcpy (lpsecurity->DBName, "iidbdb");
   ZEROINIT (szName);
   Edit_GetText (hwndName, szName, sizeof (szName));
   x_strcpy (lpsecurity->ObjectName, szName);

   lpsecurity->bsuccfail [SECALARMSUCCESS] = Button_GetCheck (hwndSuccess);
   lpsecurity->bsuccfail [SECALARMFAILURE] = Button_GetCheck (hwndFailure);
   //
   // Get selection from
   // toggle buttons (Select, Delete, Insert, Update)
   //
   lpsecurity->baccesstype [SECALARMSEL]       = Button_GetCheck (hwndSelect);
   lpsecurity->baccesstype [SECALARMDEL]       = Button_GetCheck (hwndDelete);
   lpsecurity->baccesstype [SECALARMINS]       = Button_GetCheck (hwndInsert);
   lpsecurity->baccesstype [SECALARMUPD]       = Button_GetCheck (hwndUpdate);
   lpsecurity->baccesstype [SECALARMCONNECT]   = Button_GetCheck (hwndConnect   );
   lpsecurity->baccesstype [SECALARMDISCONN]   = Button_GetCheck (hwndDisconnect);
   
   //
   // Get the names of users that have been checked and
   // insert into the list
   //
  
   if (Button_GetCheck (GetDlgItem (hwnd, IDC_SALARM_USER)))
       lpsecurity->iAuthIdType = OT_USER;
   else
   if (Button_GetCheck (GetDlgItem (hwnd, IDC_SALARM_GROUP)))
       lpsecurity->iAuthIdType = OT_GROUP;
   else
   if (Button_GetCheck (GetDlgItem (hwnd, IDC_SALARM_ROLE)))
       lpsecurity->iAuthIdType = OT_ROLE;
   else
   if (Button_GetCheck (GetDlgItem (hwnd, IDC_SALARM_PUBLIC)))
       lpsecurity->iAuthIdType = OT_PUBLIC;
   else
       lpsecurity->iAuthIdType = OT_USER;

   max_database_number = CAListBox_GetCount (hwndUsers);
   
   
   if (Button_GetCheck (GetDlgItem (hwnd, IDC_SALARM_PUBLIC)))
   {
       obj = ESL_AllocMem (sizeof (CHECKEDOBJECTS));
       if (!obj)
       {
           FreeAttachedPointers (lpsecurity, OTLL_SECURITYALARM);
           ErrorMessage   ((UINT)IDS_E_CANNOT_ALLOCATE_MEMORY, RES_ERR);
           return FALSE;
       }
       else
       {
           x_strcpy (obj->dbname, lppublicsysstring());
           obj->bchecked = TRUE;
           list = AddCheckedObject (list, obj);
       }
   }

   for (i = 0; i < max_database_number; i++)
   {
       checked = CAListBox_GetSel (hwndUsers, i);
       if (checked == 1)
       {
           CAListBox_GetText (hwndUsers, i, buf);
           obj = ESL_AllocMem (sizeof (CHECKEDOBJECTS));
           if (!obj)
           {
               FreeAttachedPointers (lpsecurity, OTLL_SECURITYALARM);
               ErrorMessage   ((UINT)IDS_E_CANNOT_ALLOCATE_MEMORY, RES_ERR);
               return FALSE;
           }
           else
           {
               if (x_strcmp (buf, lppublicdispstring()) == 0)
                   x_strcpy (obj->dbname, lppublicsysstring());
               else
                   x_strcpy (obj->dbname, QuoteIfNeeded(buf));
               obj->bchecked = TRUE;
               list = AddCheckedObject (list, obj);
           }
       }
   }
   lpsecurity->lpfirstUser = list;

   //
   // Get the names of Database that have been checked and
   // insert into the list
   //
   
   max_database_number = CAListBox_GetCount (hwndDB);
   list = NULL;
   
   for (i=0; i<max_database_number; i++)
   {
       checked = CAListBox_GetSel (hwndDB, i);
       if (checked)
       {
           CAListBox_GetText (hwndDB, i, buf);

           obj = ESL_AllocMem (sizeof (CHECKEDOBJECTS));
           if (!obj)
           {
               FreeAttachedPointers (lpsecurity, OTLL_SECURITYALARM);
               ErrorMessage   ((UINT)IDS_E_CANNOT_ALLOCATE_MEMORY, RES_ERR);
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
   lpsecurity->lpfirstTable = list;

   ComboBox_GetText (hwndDBE, szDBE, sizeof (szDBE));
   if ((x_strlen (szDBE) > 0 ) && (x_strcmp (szDBE, szSecurityDBEvent) != 0))
   {
       char szOwner [MAXOBJECTNAME];
       char szAll   [MAXOBJECTNAME];
       int  len, nSel;

       nSel = ComboBox_GetCurSel (hwndDBE);
       x_strcpy (szOwner, (char *) ComboBox_GetItemData (hwndDBE, nSel));
       StringWithOwner  (szDBE, szOwner, szAll);
       x_strcpy (lpsecurity->DBEvent, szAll);
       len = Edit_GetTextLength (hwndDBEText);
       if (len > 0)
       {
           lpsecurity->lpDBEventText = ESL_AllocMem ((len*sizeof(TCHAR)) + sizeof(TCHAR));
           if (lpsecurity->lpDBEventText)
           {
             Edit_GetText (hwndDBEText, lpsecurity->lpDBEventText, len+1); // Emb 26/06/97: +1 was missing --> lost character
             if (EscapedSimpleQuote(&lpsecurity->lpDBEventText) == RES_ERR) {
               FreeAttachedPointers (lpsecurity, OTLL_SECURITYALARM);
               ErrorMessage   ((UINT)IDS_E_CANNOT_ALLOCATE_MEMORY, RES_ERR);
               return FALSE;
             }
           }
           else
           {
               FreeAttachedPointers (lpsecurity, OTLL_SECURITYALARM);
               ErrorMessage   ((UINT)IDS_E_CANNOT_ALLOCATE_MEMORY, RES_ERR);
               return FALSE;
           }
       }
       lpsecurity->bDBEvent = TRUE;
   }
   else if (lpsecurity->lpDBEventText)
   {
       ESL_FreeMem (lpsecurity->lpDBEventText);
       lpsecurity->lpDBEventText = NULL;
   }

   return TRUE;
}


static void EnableControl (HWND hwnd, BOOL bEnable)
{
   int i,hwnd_id [2] =  {IDC_SALARM_DBEVENT_TEXT, IDC_SALARM_DBEVENT_LTEXT};

   for (i=0; i<2; i++)
   {
       HWND hwndCtl = GetDlgItem (hwnd, hwnd_id[i]);
       EnableWindow (hwndCtl, bEnable);
   }
}
