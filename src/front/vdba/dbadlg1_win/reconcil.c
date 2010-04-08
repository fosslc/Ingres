/****************************************************************************
**
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Project  : CA/OpenIngres Visual DBA
**
**    Source   : reconcil.c
**
**    Recover the replicated object on which the problem occured.
**
**    Bug #100389 (schph01)
**    Inverse the parameters order for the reconcil command because
**    the -u option must be at the left of any command line options that may
**    include quotes (in this case, the "start time" parameter may be entered
**    in quotes)
**  26-Apr-2001 (noifr01)
**    (rmcmd counterpart of SIR 103299) max lenght of rmcmd command lines
**    has been increased to 800 against 2.6. now use a new constant for
**    remote command buffer sizes, because the other "constant", in fact a
**    #define, has been redefined and depends on the result of a function 
**    call (something that doesn't compile if you put it as the size of a char[]
**    array. The new constant, used for buffer sizes only, is the biggest of 
**    the possible values)
*****************************************************************************/

#include <stdio.h>
#include "dll.h"
#include "subedit.h"
#include "tools.h"
#include "domdata.h"
#include "dbaginfo.h"
#include "getdata.h"
#include "dbadlg1.h"
#include "dlgres.h"
#include "catolist.h"
#include "msghandl.h"
#include "resource.h"


BOOL CALLBACK __export DlgReconcilerDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

static void OnDestroy(HWND hwnd);
static void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
static BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);
static void OnSysColorChange(HWND hwnd);

static BOOL OccupyDbnumberControl (HWND hwnd);
static BOOL OccupyCddsControl (HWND hwnd);
static void InitialiseEditControls (HWND hwnd);
static void EnableDisableOKButton (HWND hwnd);
static BOOL SendCommand(HWND hwnd);

BOOL WINAPI __export DlgReconciler (HWND hwndOwner, LPRECONCILER lpReconciler)
/*
   Function:
      Shows the system modification dialog.

   Paramters:
      hwndOwner   - Handle of the parent window for the dialog.
      lpReconciler    - Points to structure containing information used to 
                  - initialise the dialog. The dialog uses the same
                  - structure to return the result of the dialog.

   Returns:
      TRUE if successful.
*/
{
   FARPROC lpProc;
   int     RetVal;

   if (!IsWindow(hwndOwner) || !lpReconciler)
   {
     ASSERT(NULL);
     return FALSE;
   }

   lpProc = MakeProcInstance ((FARPROC) DlgReconcilerDlgProc, hInst);

   RetVal = VdbaDialogBoxParam (hResource,
                            MAKEINTRESOURCE(IDD_RECONCILER),
                            hwndOwner,
                            (DLGPROC) lpProc,
                            (LPARAM)lpReconciler);

   FreeProcInstance (lpProc);

   return (RetVal);
}


BOOL CALLBACK __export DlgReconcilerDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  switch (message)
  {
   HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
   HANDLE_MSG(hwnd, WM_DESTROY, OnDestroy);

   case WM_INITDIALOG:
     return HANDLE_WM_INITDIALOG(hwnd, wParam, lParam, OnInitDialog);

   default:
     return FALSE;
  }
  return TRUE;
}


static BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
   int  nProd = 0;
   char Title[200];
   HWND hwndProducts   = GetDlgItem(hwnd, IDC_PRODUCTS);
   HWND hwndTables     = GetDlgItem(hwnd, IDC_TABLES);
   HWND hwndtableall   = GetDlgItem(hwnd, IDC_TABLE_ALL);
   HWND hwndproductall = GetDlgItem(hwnd, IDC_PRODUCT_ALL);
   
   LPRECONCILER lpmod = (LPRECONCILER)lParam;
   
   ZEROINIT (Title);

   if (!AllocDlgProp(hwnd, lpmod))
      return FALSE;
   lpHelpStack = StackObject_PUSH (lpHelpStack, StackObject_INIT ((UINT)IDD_RECONCILER));
   
   //Fill the windows title bar
   GetWindowText(hwnd,Title,GetWindowTextLength(hwnd)+1);
   x_strcat(Title, " ");
   x_strcat(Title, GetVirtNodeName ( GetCurMdiNodeHandle ()));
   x_strcat(Title, "::");
   x_strcat(Title,lpmod->DBName);

   SetWindowText(hwnd,Title);

   if (!OccupyDbnumberControl(hwnd)
    || !OccupyCddsControl(hwnd))
   {
     ASSERT(NULL);
     EndDialog(hwnd, FALSE);
     return TRUE;
   }
	
   InitialiseEditControls(hwnd);
   EnableDisableOKButton (hwnd);
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
       int nIndex;
       int nCount;
       int nLast=0;
       int max;
       int i;
       int first=0;
       UCHAR buf[MAXOBJECTNAME];
       HWND    hwndCdds     = GetDlgItem (hwnd, IDC_CDDSNUMBER);
       HWND    hwndDbNumber = GetDlgItem (hwnd, IDC_DBNUMBER);
       HWND    hwndStartTime= GetDlgItem (hwnd, IDC_STARTTIME);
       LPRECONCILER lpreconciler = GetDlgProp(hwnd);


       // fill structure with CDDS values
       nCount = 0;
       ZEROINIT (lpreconciler->CddsNo);

       nCount = CAListBox_GetSelCount(hwndCdds);
       max    = CAListBox_GetCount(hwndCdds);
       
       if (nCount != max)
         {
         for (i=0;i<max;i++)
           {
           if (CAListBox_GetSel (hwndCdds, i))
             {
             char szName[MAX_DBNAME_LEN];
             
             CAListBox_GetText(hwndCdds, i, szName);
             
             if (first==0 && nCount>1)
               {
               x_strcat(lpreconciler->CddsNo,"'(");
               first=1;
               }
             
             x_strcat(lpreconciler->CddsNo,szName);
             
             nLast++;

             if ( nLast == nCount && nCount>1 )
               x_strcat(lpreconciler->CddsNo,")'");
             else
               {
               if (nCount>1)
                 x_strcat(lpreconciler->CddsNo,",");
               }
             }
           }
         }
       else
         x_strcat(lpreconciler->CddsNo,"all ");

       // Fill structure with Targetdbnumber
       nIndex=ComboBox_GetCurSel(hwndDbNumber);
       ComboBox_GetLBText(hwndDbNumber, nIndex, buf);
       
       GetIntFromStartofString( buf, lpreconciler->TarGetDbNumber);

       //sscanf (buf,"%s",lpreconciler->TarGetDbNumber );

       //Fill structure with the Start time
       Edit_GetText(hwndStartTime, buf, MAXOBJECTNAME );
       x_strcpy (lpreconciler->StarTime,buf);
       
       if (SendCommand(hwnd)==TRUE)
         EndDialog(hwnd, TRUE);

       break;
     }

     case IDCANCEL:
       EndDialog(hwnd, FALSE);
       break;

     case IDC_DBNUMBER:
       if ( codeNotify == CBN_SELCHANGE)
         EnableDisableOKButton (hwnd);
       break;

     case IDC_CDDSNUMBER:
       if ( codeNotify == CALBN_CHECKCHANGE || codeNotify == CALBN_SELCHANGE ||
                           codeNotify == CALBN_KILLFOCUS )
         EnableDisableOKButton (hwnd);
       break;

     case IDC_STARTTIME:
       if (codeNotify == EN_CHANGE)
         EnableDisableOKButton (hwnd);
       break;

   }
}


static void OnSysColorChange(HWND hwnd)
{
#ifdef _USE_CTL3D
   Ctl3dColorChange();
#endif
}


static BOOL OccupyDbnumberControl (HWND hwnd)
/*
   Function:
      Fills the Database Number .

   Parameters:
      hwnd   - Handle to the dialog window.

   Returns:
      TRUE if successful.
*/
{
   int hNode;
   UCHAR buffilter[MAXOBJECTNAME];
   UCHAR szObject[MAXOBJECTNAME];
   UCHAR szLocalDatabase[MAXOBJECTNAME];
   char * lpTemp;
   LPUCHAR parentstrings [MAXPLEVEL];
   int ListErr;
   int err,iReplicVersion;
   BOOL bSystem = TRUE;
   BOOL bRetVal = TRUE;
   LPUCHAR vnodeName  = GetVirtNodeName(GetCurMdiNodeHandle ());
   HWND hwndCtl       = GetDlgItem (hwnd, IDC_DBNUMBER);
   LPRECONCILER lpdb  = GetDlgProp(hwnd);

   hNode = GetCurMdiNodeHandle();
   parentstrings [0] = lpdb->DBName;

   wsprintf(szLocalDatabase," %s::%s",vnodeName,lpdb->DBName);

   ComboBox_ResetContent(hwndCtl);
   // Get everything from dd_connections 
   iReplicVersion=GetReplicInstallStatus(GetCurMdiNodeHandle (),lpdb->DBName,
                                                                lpdb->DbaName);
   // Get additionnal information on current CDDS: name,error ,collision

   err = DOMGetFirstObject(hNode,
                           OT_REPLIC_CONNECTION,
                           1,
                           parentstrings,
                           bSystem,
                           NULL,
                           szObject,
                           NULL,
                           NULL);
   
   while (err == RES_SUCCESS)
   {
     if (iReplicVersion == REPLIC_V11)  {
       ListErr=ComboBox_AddString(hwndCtl, szObject );
       if (ListErr == CB_ERR || ListErr == CB_ERRSPACE)
         bRetVal=FALSE;
     }
     else {
       lpTemp=x_stristr(szObject,szLocalDatabase);
       if (lpTemp) {
         if (x_stricmp(lpTemp,szLocalDatabase) != 0) {
           ListErr=ComboBox_AddString(hwndCtl, szObject );
           if (ListErr == CB_ERR || ListErr == CB_ERRSPACE)
             bRetVal=FALSE;
         }
       }
     }
     err = DOMGetNextObject (szObject, buffilter, NULL);
   }
   ComboBox_SetCurSel(hwndCtl,0);
   return bRetVal;
}


static BOOL OccupyCddsControl (HWND hwnd)
/*
   Function:
      Fills the CDDS number in list box.

   Parameters:
      hwnd   - Handle to the dialog window.

   Returns:
      TRUE if successful.
*/
{
   int hNode;
   UCHAR buffilter[MAXOBJECTNAME];
   UCHAR CddsName[MAXOBJECTNAME];
   LPUCHAR parentstrings [MAXPLEVEL];
   BOOL bSystem=FALSE;
   int ListErr;
   int err;
   BOOL bRetVal = TRUE;
   HWND hwndCtl = GetDlgItem (hwnd, IDC_CDDSNUMBER);
   LPRECONCILER lpdb  = GetDlgProp(hwnd);

   ZEROINIT (CddsName);
   hNode = GetCurMdiNodeHandle();
   parentstrings [0] = lpdb->DBName;
   
   CAListBox_ResetContent(hwndCtl);

   err = DOMGetFirstObject(hNode,
                           OT_REPLIC_CDDS,
                           1,
                           parentstrings,
                           bSystem,
                           NULL,
                           CddsName,
                           NULL,
                           NULL );
   
   while (err == RES_SUCCESS)
   {
     ListErr=CAListBox_AddString(hwndCtl, CddsName );
     if (ListErr == LB_ERR || ListErr == LB_ERRSPACE)
       bRetVal=FALSE;
     err = DOMGetNextObject (CddsName, buffilter, NULL);
   }

   return bRetVal;
}

// SendCommand

// create command line and execute remote command
static BOOL SendCommand(HWND hwnd)
{
   //create the command for remote 
   UCHAR buf2[MAXOBJECTNAME*3];
   UCHAR buf[MAX_RMCMD_BUFSIZE];
 
   int iReplicVersion;
   LPRECONCILER  lpreconciler  = GetDlgProp(hwnd);
   LPUCHAR vnodeName = GetVirtNodeName(GetCurMdiNodeHandle ());

   // Get everything from dd_connections 
   iReplicVersion=GetReplicInstallStatus(GetCurMdiNodeHandle (),lpreconciler->DBName,
                                                                lpreconciler->DbaName);
   // Get additionnal information on current CDDS: name,error ,collision
   if (iReplicVersion == REPLIC_V11)  {
	   UCHAR tmpusername[MAXOBJECTNAME];
	   DBAGetUserName(vnodeName,tmpusername);
 
       wsprintf(buf,"reconcil %s %s %s -u%s %s",lpreconciler->DBName,
                                           lpreconciler->TarGetDbNumber,
                                           lpreconciler->CddsNo,
                                           tmpusername,
										   lpreconciler->StarTime);
       wsprintf(buf2,ResourceString(IDS_REPLIC_REC_1PARM_TITLE), lpreconciler->DBName);
   }
   else {
       wsprintf(buf,"reconcil %s::%s %s %s %s %s", vnodeName,
                                                   lpreconciler->DBName,
                                                   lpreconciler->DbaName,
                                                   lpreconciler->TarGetDbNumber,
                                                   lpreconciler->CddsNo,
                                                   lpreconciler->StarTime);
       wsprintf(buf2,ResourceString(IDS_REPLIC_REC_2PARMS_TITLE),
                  vnodeName,
                  lpreconciler->DBName);
   }

   execrmcmd(vnodeName,buf,buf2);

   return(TRUE);
}

static void InitialiseEditControls (HWND hwnd)
{
	HWND hwndStartTime = GetDlgItem(hwnd, IDC_STARTTIME);

	Edit_LimitText(hwndStartTime, MAXOBJECTNAME-1);

}
static void EnableDisableOKButton (HWND hwnd)
{
   if (Edit_GetTextLength (GetDlgItem (hwnd, IDC_STARTTIME)) == 0    ||
       CAListBox_GetSelCount(GetDlgItem (hwnd, IDC_CDDSNUMBER))  == 0 ||
       ComboBox_GetCurSel(GetDlgItem (hwnd, IDC_DBNUMBER)) == CB_ERR  )
       EnableWindow (GetDlgItem (hwnd, IDOK), FALSE);
   else
       EnableWindow (GetDlgItem (hwnd, IDOK), TRUE);
}
