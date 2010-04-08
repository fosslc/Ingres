/********************************************************************
//
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
//
//    Project  : CA/OpenIngres Visual DBA
//
//    Source   : svrdis.c
//
//    Dialog box for disconnecting the virtual nodes from the server
//
********************************************************************/

#include "dll.h"
#include "subedit.h"
#include "dbadlg1.h"
#include "dlgres.h"
#include "getdata.h"
#include "dom.h"
#include "domdata.h"
#include "dbaginfo.h"
#include "winutils.h"

#include <string.h>

//#define  MAX_TEXT_EDIT 1000


BOOL CALLBACK __export DlgServerDisconnectDlgProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

static void OnDestroy(HWND hwnd);
static void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
static BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);

static BOOL OccupyServerControl (HWND hwnd);
static BOOL OccupyWindowsControl (HWND hwnd);
static void OnSysColorChange(HWND hwnd);
static void AddDocInWindowsControl (HWND hwndCtl, int VirtNodeHandle);
static void Disconnecting (LPUCHAR VirtNodeName);




BOOL WINAPI __export DlgServerDisconnect (HWND hwndOwner, LPSERVERDIS lpsvrdis)
/*
//	Function:
//		Shows the Disconnect Server dialog
//
//	Paramters:
//		hwndOwner		- Handle of the parent window for the dialog.
//		lpsvrdis			- Points to structure containing information used to 
//							- initialise the dialog. The dialog uses the same
//							- structure to return the result of the dialog.
//
//	Returns:
//		TRUE if successful.
*/
{
	if (!IsWindow(hwndOwner) || !lpsvrdis)
	{
		ASSERT(NULL);
		return FALSE;
	}

	if (VdbaDialogBoxParam(hResource, MAKEINTRESOURCE(IDD_DISCONNECT), hwndOwner, DlgServerDisconnectDlgProc, (LPARAM)lpsvrdis) == -1)
		return FALSE;

	return TRUE;
}




BOOL CALLBACK __export DlgServerDisconnectDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
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

static BOOL OnInitDialog (HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
	LPSERVERDIS lpsvr       = (LPSERVERDIS)lParam;
	HWND hwndServer  = GetDlgItem (hwnd, IDC_SVRDIS_SERVERS);
	HWND hwndWindows = GetDlgItem (hwnd, IDC_SVRDIS_WINDOWS);
   HWND  hwndCurDoc;       // The current mdi
   char* node_name;
   int   VirtNodeHandle;
   char szTitle [MAX_TITLEBAR_LEN];

   if (!AllocDlgProp(hwnd, lpsvr))
       return FALSE;

   if (!OccupyServerControl(hwnd))
   {
       EndDialog (hwnd, -1);
       return TRUE;
   }

   if (!OccupyWindowsControl(hwnd))
   {
       EndDialog(hwnd, -1);
       return TRUE;
   }
  
   LoadString (hResource, (UINT)IDS_T_SVR_DISCONNECT, szTitle, sizeof (szTitle));
   SetWindowText (hwnd, szTitle);
  
   //
   // Find the first activated Mdi Node child -> handle
   //
   hwndCurDoc = GetWindow (hwndMDIClient, GW_CHILD);
   while (hwndCurDoc && GetWindow (hwndCurDoc, GW_OWNER))
   {
       hwndCurDoc = GetWindow (hwndCurDoc, GW_HWNDNEXT);
   }

   //
   // Get the first activated Mdi Node child name
   // and select the string node_name in the Combo box 
   //

   node_name = GetVirtNodeName (GetMDINodeHandle (hwndCurDoc));
   if (node_name)
     ComboBox_SelectString (hwndServer,-1, node_name);
   else {
     if (ComboBox_GetCount (hwndServer) > 0)
        ComboBox_SetCurSel (hwndServer,0);
   }
 
   //
   // Show the Mdi in the windows
   //

   if (node_name)
     VirtNodeHandle = GetVirtNodeHandle ((LPUCHAR)node_name);
   else
     VirtNodeHandle = 0;

   if (VirtNodeHandle >= 0)
   {
       AddDocInWindowsControl (hwndWindows, VirtNodeHandle);
   }
   
   if (ComboBox_GetCount (hwndServer) == 0)
   {
       EnableWindow (GetDlgItem (hwnd, IDOK), FALSE);
   }
   lpHelpStack = StackObject_PUSH (lpHelpStack, StackObject_INIT ((UINT)IDD_DISCONNECT));
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
   HWND hwndServers = GetDlgItem(hwnd, IDC_SVRDIS_SERVERS);
   LPSERVERDIS lpsvr = GetDlgProp(hwnd);
   
   switch (id)
   {
       case IDOK:
       {
           ComboBox_GetText (hwndServers, lpsvr->szServerName, sizeof(lpsvr->szServerName));
              Disconnecting ((LPUCHAR)lpsvr->szServerName);
      
              EndDialog (hwnd, 1);
           break;
       }

       case IDCANCEL:
       {
           EndDialog(hwnd, 0);
           break;
       }

       case IDC_SVRDIS_SERVERS:
       {
              BOOL bEnable = TRUE;
           switch (codeNotify)
           {
               case CBN_EDITCHANGE:
               {
                   OccupyWindowsControl (hwnd);
                   break;
               }
               case CBN_DBLCLK:
               case CBN_SELCHANGE:
               {
                   OccupyWindowsControl (hwnd);
                   break;
               }
               break;

           }
           if (ComboBox_GetCount (hwndServers) == 0)
           {
               bEnable = FALSE;
           }
           EnableWindow (GetDlgItem (hwnd, IDOK), bEnable);
              
           break;
       }
   }
}

static void OnSysColorChange(HWND hwnd)
{
#ifdef _USE_CTL3D
    Ctl3dColorChange();
#endif
}




static BOOL OccupyServerControl (HWND hwnd)
/*
//    Function:
//        Fills the server drop down box with the server names.
//
//    Parameters:
//        hwnd    - Handle to the dialog window.
//
//    Returns:
//        TRUE if successful.
*/
{
   HWND hwndCtl = GetDlgItem (hwnd, IDC_SVRDIS_SERVERS);
   int     i;
   char*   buf;
   int     max_server = GetMaxVirtNodes ();

   for (i = 0; i < max_server; i++)
   {
       if (isVirtNodeEntryBusy (i))
       {
           buf = GetVirtNodeName (i);
           if (buf)
               SendDlgItemMessage (hwnd, IDC_SVRDIS_SERVERS, CB_ADDSTRING,
                                   0, (LPARAM)buf);
       }
   }
   return TRUE;
}





static BOOL OccupyWindowsControl (HWND hwnd)
/*
//    Function:
//        Fills the windows list box with the window names associated with
//        the currently selected server.
//
//    Parameters:
//        hwnd    - Handle to the dialog window.
//
//    Returns:
//        TRUE if successful.
*/
{  HWND    hwndCtl    = GetDlgItem (hwnd, IDC_SVRDIS_WINDOWS);
   HWND    hwndServer = GetDlgItem (hwnd, IDC_SVRDIS_SERVERS);
   char    szServer [MAXOBJECTNAME -1];
   int     VirtNodeHandle;

   ComboBox_GetText (hwndServer, szServer, MAXOBJECTNAME -1);
  
   VirtNodeHandle = GetVirtNodeHandle ((LPUCHAR)szServer);
   if (VirtNodeHandle >= 0)
   {
       AddDocInWindowsControl (hwndCtl, VirtNodeHandle);
   }
   return TRUE;
}

// Modified Emb 25/4/95 to accept any mdi document type
static void AddDocInWindowsControl (HWND hwndCtl, int VirtNodeHandle)
{
   HWND        hwndCurDoc;       // for loop on all documents
   char  cur_mdi_title [100];
   char  ch [3];
   char  a_string [MAX_TEXT_EDIT];
   
   ch [0] = 0x0D;
   ch [1] = 0x0A;
   ch [2] = '\0';

   hwndCurDoc = GetWindow (hwndMDIClient, GW_CHILD);
   lstrcpy (a_string,"");
   
   while (hwndCurDoc && GetWindow (hwndCurDoc, GW_OWNER))
   {
       hwndCurDoc = GetWindow (hwndCurDoc, GW_HWNDNEXT);
   }

   while (hwndCurDoc)
   {
       //
       // add window in the list if node 
       // manage update for this mdi document if concerned with
       //
       if (GetMDINodeHandle (hwndCurDoc) == VirtNodeHandle)
       {
           GetWindowText (hwndCurDoc, cur_mdi_title, 100);
           x_strcat (a_string, cur_mdi_title);
           x_strcat (a_string, ch);
           //AddDocInList(hwndCurDoc,...);
       }

       //
       // Get next document handle (with loop to skip the icon title windows)
       //

       hwndCurDoc = GetWindow (hwndCurDoc, GW_HWNDNEXT);
       while (hwndCurDoc && GetWindow (hwndCurDoc, GW_OWNER))
       {
           hwndCurDoc = GetWindow (hwndCurDoc, GW_HWNDNEXT);
       }
   }
   
   Edit_SetText  (hwndCtl, a_string);
}






// Modified Emb 25/4/95 to accept any mdi document type
static void Disconnecting (LPUCHAR VirtNodeName)
{      
   HWND        hwndCurDoc;       // for loop on all documents
   int         VirtNodeHandle;
   hwndCurDoc = GetWindow (hwndMDIClient, GW_CHILD);

   VirtNodeHandle = GetVirtNodeHandle ((LPUCHAR)VirtNodeName);
   while (hwndCurDoc && GetWindow (hwndCurDoc, GW_OWNER))
   {
       hwndCurDoc = GetWindow (hwndCurDoc, GW_HWNDNEXT);
   }

   while (hwndCurDoc)
   {
       //
       // Destroy Mdis that connected to the Virtual Node 'VirtNodeHandle' 
       //
       if (GetMDINodeHandle (hwndCurDoc) == VirtNodeHandle)
       {
           SendMessage (hwndMDIClient, WM_MDIDESTROY, (WPARAM)hwndCurDoc, 0L);
           hwndCurDoc = GetWindow (hwndMDIClient, GW_CHILD);
           while (hwndCurDoc && GetWindow (hwndCurDoc, GW_OWNER))
               hwndCurDoc = GetWindow (hwndCurDoc, GW_HWNDNEXT);
           continue;
       }
       //
       // Get next document handle (with loop to skip the icon title windows)
       //
       hwndCurDoc = GetWindow (hwndCurDoc, GW_HWNDNEXT);
       while (hwndCurDoc && GetWindow (hwndCurDoc, GW_OWNER))
       {
           hwndCurDoc = GetWindow (hwndCurDoc, GW_HWNDNEXT);
       }
   }
   ShowHourGlass();
   DBACloseNodeSessions(VirtNodeName,FALSE,FALSE);
   FreeNodeStruct (VirtNodeHandle,FALSE);
   RemoveHourGlass();
}

int GetNumberOfOpenedDocuments (LPUCHAR VirtNodeName)
{      
   HWND        hwndCurDoc;       // for loop on all documents
   int         VirtNodeHandle;
   int        docCount = 0;
   hwndCurDoc = GetWindow (hwndMDIClient, GW_CHILD);

   VirtNodeHandle = GetVirtNodeHandle ((LPUCHAR)VirtNodeName);
   while (hwndCurDoc && GetWindow (hwndCurDoc, GW_OWNER))
       hwndCurDoc = GetWindow (hwndCurDoc, GW_HWNDNEXT);

   while (hwndCurDoc)
   {
       if (GetMDINodeHandle (hwndCurDoc) == VirtNodeHandle)
         docCount++;

       hwndCurDoc = GetWindow (hwndCurDoc, GW_HWNDNEXT);
       while (hwndCurDoc && GetWindow (hwndCurDoc, GW_OWNER))
         hwndCurDoc = GetWindow (hwndCurDoc, GW_HWNDNEXT);
   }
   return docCount;
}

