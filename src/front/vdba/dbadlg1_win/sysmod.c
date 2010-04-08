/********************************************************************
//
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
//
//    Project  : CA/OpenIngres Visual DBA
//
//    Source   : sysmod.c
//
//    System modification dialog box
//
**  26-Apr-2001 (noifr01)
**    (rmcmd counterpart of SIR 103299) max lenght of rmcmd command lines
**    has been increased to 800 against 2.6. now use a new constant for
**    remote command buffer sizes, because the other "constant", in fact a
**    #define, has been redefined and depends on the result of a function 
**    call (something that doesn't compile if you put it as the size of a char[]
**    array. The new constant, used for buffer sizes only, is the biggest of 
**    the possible values)
**  30-Oct-2001 (schph01)
**    bug 106209 manage the page size option of SYSMOD
********************************************************************/

#include "dll.h"
#include "subedit.h"
#include "domdata.h"
#include "getdata.h"
#include "dbadlg1.h"
#include "dlgres.h"
#include "catolist.h"
#include "msghandl.h"

static CTLDATA prodTypes[] =
{
   PROD_INGRES,      IDS_PROD_INGRES,
   PROD_VISION,      IDS_PROD_VISION,
   PROD_W4GL,        IDS_PROD_W4GL,
   PROD_INGRES_DBD,  IDS_PROD_INGRES_DBD,
   -1      // Terminator
};
//   PROD_NOFECLIENTS,   IDS_PROD_NOFECLIENTS, PS

BOOL CALLBACK __export DlgSysModDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

static void OnDestroy(HWND hwnd);
static void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
static BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);
static void OnSysColorChange(HWND hwnd);

static BOOL OccupyTableControl (HWND hwnd);
static BOOL OccupyProductsControl (HWND hwnd);
static void InitialiseEditControls (HWND hwnd);
static BOOL SendCommand(HWND hwnd);

BOOL WINAPI __export DlgSysMod (HWND hwndOwner, LPSYSMOD lpsysmod)
/*
   Function:
      Shows the system modification dialog.

   Paramters:
      hwndOwner   - Handle of the parent window for the dialog.
      lpsysmod    - Points to structure containing information used to 
                  - initialise the dialog. The dialog uses the same
                  - structure to return the result of the dialog.

   Returns:
      TRUE if successful.
*/
{
   FARPROC lpProc;
   int     RetVal;

   if (!IsWindow(hwndOwner) || !lpsysmod)
   {
     ASSERT(NULL);
     return FALSE;
   }
                                        
   lpProc = MakeProcInstance ((FARPROC) DlgSysModDlgProc, hInst);

   RetVal = VdbaDialogBoxParam (hResource,
                            MAKEINTRESOURCE(IDD_SYSMOD),
                            hwndOwner,
                            (DLGPROC) lpProc,
                            (LPARAM)lpsysmod);

   FreeProcInstance (lpProc);

   return (RetVal);
}


BOOL CALLBACK __export DlgSysModDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
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

static void ComboBox_InitPageSize (HWND hwnd, LONG Page_size)
{
	int i, index;
	typedef struct tagCxU
	{
		char* cx;
		LONG  ux;
	} CxU;
	CxU pageSize [] =
	{
		{" 2K", 2048L},
		{" 4K", 4096L},
		{" 8K", 8192L},
		{"16K",16384L},
		{"32K",32768L},
		{"64K",65536L}
	};
	HWND hwndCombo = GetDlgItem (hwnd, IDC_COMBO_PAGESIZE);
	for (i=0; i<6; i++)
	{
		index = ComboBox_AddString (hwndCombo, pageSize [i].cx);
		if (index != -1)
			ComboBox_SetItemData (hwndCombo, index, pageSize [i].ux);
	}
	ComboBox_SetCurSel(hwndCombo, 0);
	for (i=0; i<6; i++)
	{
		if (pageSize [i].ux == Page_size)
		{
			ComboBox_SetCurSel(hwndCombo, i);
			break;
		}
	}
}

static BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
   int nProd = 0;
   char Title[200];
   HWND hwndProducts   = GetDlgItem(hwnd, IDC_PRODUCTS);
   HWND hwndTables     = GetDlgItem(hwnd, IDC_TABLES);
   HWND hwndtableall   = GetDlgItem(hwnd, IDC_TABLE_ALL);
   HWND hwndproductall = GetDlgItem(hwnd, IDC_PRODUCT_ALL);
   
   LPSYSMOD lpmod = (LPSYSMOD)lParam;
   
   ZEROINIT (Title);

   if (!AllocDlgProp(hwnd, lpmod))
      return FALSE;
   
   //Fill the windows title bar
   GetWindowText(hwnd,Title,GetWindowTextLength(hwnd)+1);
   x_strcat(Title, " ");
   x_strcat(Title, GetVirtNodeName ( GetCurMdiNodeHandle ()));
   x_strcat(Title, "::");
   x_strcat(Title,lpmod->szDBName);

   SetWindowText(hwnd,Title);

   if (!OccupyTableControl(hwnd)
    || !OccupyProductsControl(hwnd))
   {
     ASSERT(NULL);
     EndDialog(hwnd, FALSE);
     return TRUE;
   }

   // button all product enable
   Button_SetCheck(hwndproductall, 1);

   // button all table enable 
   Button_SetCheck(hwndtableall, 1);

   // default Tables are disabled
   EnableWindow(hwndTables,FALSE);
   ShowWindow(hwndTables,SW_HIDE);
   
   // default Products are disabled 
   EnableWindow(hwndProducts,FALSE);
   ShowWindow(hwndProducts,SW_HIDE);

   ComboBox_InitPageSize(hwnd, 2048L);

   lpHelpStack = StackObject_PUSH (lpHelpStack, StackObject_INIT ((UINT)IDD_SYSMOD));
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
       int nCount;
       int max,nIdx;
       int i;
       HWND    hwndProd    = GetDlgItem (hwnd, IDC_PRODUCTS);
       HWND    hwndTables  = GetDlgItem (hwnd, IDC_TABLES);
       HWND    hwndComboPage = GetDlgItem (hwnd, IDC_COMBO_PAGESIZE);
       LPSYSMOD lpmod = GetDlgProp(hwnd);


       lpmod->bWait = IsDlgButtonChecked(hwnd, IDC_WAITDB);

       if (IsDlgButtonChecked(hwnd, IDC_CHECK_PAGE_SIZE))
       {
          nIdx = ComboBox_GetCurSel (hwndComboPage);
          if (nIdx != -1)
          {
              lpmod->lPageSize = (LONG)ComboBox_GetItemData (hwndComboPage, nIdx);
          }
          else
              lpmod->lPageSize = 0L;
       }
       else
       {
          lpmod->lPageSize = 0L;
       }

       if (IsDlgButtonChecked(hwnd, IDC_TABLE_SPECIFY) == TRUE)
       {
         nCount = CAListBox_GetSelCount(hwndTables);
         max    = CAListBox_GetCount(hwndTables);
         lpmod->szTables = AddItemToList (hwndTables);
           if (!lpmod->szTables)
             {
             FreeObjectList (lpmod->szTables);
             lpmod->szTables = NULL;
             ErrorMessage   ((UINT) IDS_E_CANNOT_ALLOCATE_MEMORY, RES_ERR);
             break;
             }
       }
       // fill structure with selected product
       nCount = 0;
       ZEROINIT (lpmod->szProducts);

       if (IsDlgButtonChecked(hwnd, IDC_PRODUCT_SPECIFY) == TRUE)
       {
         nCount = CAListBox_GetSelCount(hwndProd);
         max    = CAListBox_GetCount(hwndProd);
         for (i=0;i<max;i++)
           {
           if (CAListBox_GetSel (hwndProd, i))
             {
             char szName[MAX_DBNAME_LEN];
             //CAListBox_GetText(hwndProd, i, szName);
             DWORD dwtype = CAListBox_GetItemData(hwndProd, i);
             ProdNameFromProdType(dwtype, szName);
             x_strcat(lpmod->szProducts,szName);
             x_strcat(lpmod->szProducts," ");
             }
           }
       }
       if (SendCommand(hwnd)==TRUE)
         {
         FreeObjectList (lpmod->szTables);
         lpmod->szTables = NULL;
         EndDialog(hwnd, TRUE);
         }
       else
         {
         FreeObjectList (lpmod->szTables);
         lpmod->szTables = NULL;
         }

       break;
     }

     case IDCANCEL:
       EndDialog(hwnd, FALSE);
       break;

     case IDC_TABLE_ALL:
       switch (codeNotify)
       {
         case BN_CLICKED:
         {
         // select or deselect all TABLES 
         HWND hwndSpetable = GetDlgItem(hwnd, IDC_TABLE_SPECIFY);
         HWND hwndTable    = GetDlgItem(hwnd, IDC_TABLES);

         // disable the CAListbox table
         EnableWindow(hwndTable,FALSE);
         ShowWindow(hwndTable,SW_HIDE);

         // disable the button specify
         Button_SetState(hwndSpetable, FALSE);
         }
       }
       break;

     case IDC_TABLE_SPECIFY:
       switch (codeNotify)
       {
         case BN_CLICKED:
         {
           HWND hwndAlltable = GetDlgItem(hwnd, IDC_TABLE_ALL);
           HWND hwndTable    = GetDlgItem(hwnd, IDC_TABLES);

           // enable the CAListbox table
           EnableWindow(hwndTable,TRUE);
           ShowWindow(hwndTable,SW_SHOW);
           // disable the button all tables
           Button_SetState(hwndAlltable, FALSE);
         }
       }
       break;

     case IDC_PRODUCT_SPECIFY:
       switch (codeNotify)
       {
       case BN_CLICKED:
         {
           HWND hwndAllProducts    = GetDlgItem(hwnd, IDC_PRODUCT_ALL);
           HWND hwndProducts       = GetDlgItem(hwnd, IDC_PRODUCTS);

           // user products are enable
           EnableWindow (hwndProducts,TRUE);
           ShowWindow(hwndProducts,SW_SHOW);
           // Button All product is disable
           Button_SetState(hwndAllProducts, FALSE);
         }
       }
       break;

     case IDC_PRODUCT_ALL:
       switch (codeNotify)
       {
       case BN_CLICKED:
         {
           HWND hwndSpecifProducts = GetDlgItem(hwnd, IDC_PRODUCT_SPECIFY);
           HWND hwndProducts       = GetDlgItem(hwnd, IDC_PRODUCTS);

           // user product are disabled
           EnableWindow (hwndProducts,FALSE);
           ShowWindow(hwndProducts,SW_HIDE);
           // Button specify products is disable
           Button_SetState(hwndSpecifProducts, FALSE);
         }
       }
       break;
     case IDC_CHECK_PAGE_SIZE:
       switch (codeNotify)
       {
       case BN_CLICKED:
         {
           HWND    hwndComboPage = GetDlgItem (hwnd, IDC_COMBO_PAGESIZE);
           if (IsDlgButtonChecked(hwnd, IDC_CHECK_PAGE_SIZE))
           {
              EnableWindow (hwndComboPage,TRUE);
              //ShowWindow(hwndComboPage,SW_SHOW);
           }
           else
           {
             EnableWindow (hwndComboPage,FALSE);
             //ShowWindow(hwndComboPage,SW_HIDE);
           }
         }
       }
   }
}


static void OnSysColorChange(HWND hwnd)
{
#ifdef _USE_CTL3D
   Ctl3dColorChange();
#endif
}


static BOOL OccupyTableControl (HWND hwnd)
/*
// Function:
//    Fills the table list down box with the table names.
//
// Parameters:
//    hwnd   - Handle to the dialog window.
//
// Returns:
//    TRUE if successful.
*/
{
   int      ListErr,
            err,
            hNode;
   char     szObject [MAXOBJECTNAME],
            szOwner  [MAXOBJECTNAME];
   LPUCHAR  parentstrings [MAXPLEVEL];
   HWND     hwndCtl = GetDlgItem (hwnd, IDC_TABLES);
   LPSYSMOD lpdb    = GetDlgProp (hwnd);
   BOOL     bSystem = TRUE,
            bRetVal = TRUE;;

   hNode            = GetCurMdiNodeHandle();
   parentstrings [0]= lpdb->szDBName;

   err = DOMGetFirstObject
       (hNode,
        OT_TABLE,
        1,
        parentstrings,
        bSystem,
        NULL,
        szObject,
        szOwner,
        NULL);
   
   while (err == RES_SUCCESS)
   {
       if (IsSystemObject (OT_TABLE, szObject, szOwner))
       {
           ListErr = CAListBox_AddString (hwndCtl, szObject);
           if (ListErr == LB_ERR || ListErr == LB_ERRSPACE)
               bRetVal = FALSE;
       }
       err = DOMGetNextObject (szObject, szOwner, NULL);
   }
   return bRetVal;
}



static BOOL OccupyProductsControl (HWND hwnd)
/*
   Function:
      Fills the products list box with the product names.

   Parameters:
      hwnd   - Handle to the dialog window.

   Returns:
      TRUE if successful.
*/
{
   HWND hwndCtl = GetDlgItem (hwnd, IDC_PRODUCTS);
   return OccupyCAListBoxControl(hwndCtl, prodTypes);
}

// SendCommand

// create and execute remote command
static BOOL SendCommand(HWND hwnd)
{
   //create the command for remote 
   UCHAR buf2[MAXOBJECTNAME*3];
   int ObjectLen = 0;
   int BufLen    = 0;
   BOOL      bFirst  = TRUE;
   LPSYSMOD  lpmod   = GetDlgProp(hwnd);
   LPOBJECTLIST ls   = lpmod->szTables;
   UCHAR buf[MAX_RMCMD_BUFSIZE];
   LPUCHAR vnodeName = GetVirtNodeName(GetCurMdiNodeHandle ());   

       x_strcpy(buf,"sysmod ");
       x_strcat(buf,lpmod->szDBName);
       x_strcat(buf," ");

   //store the name of table 
   while (ls)
     {
     ObjectLen = x_strlen(ls->lpObject );
     BufLen    = x_strlen(buf);

     if (ObjectLen >= MAX_LINE_COMMAND-BufLen)
       {
         ErrorMessage ((UINT)IDS_E_TOO_MANY_TABLES_SELECTED, RES_ERR);
         return FALSE;
       }
     else
       {
       x_strcat(buf,(char *)ls->lpObject);
       x_strcat(buf," ");
       ls = ls->lpNext;
       }
     }

   // store the product in command line
   if ( lpmod->szProducts[0] != '\0')
     {
     ObjectLen = x_strlen(lpmod->szProducts);
     BufLen    = x_strlen(buf);
   
     if (ObjectLen >= MAX_LINE_COMMAND-BufLen)
       {
         ErrorMessage ((UINT)IDS_E_TOO_MANY_PRODUCTS_SELECTED, RES_ERR);
         return FALSE;
       }
       else
         {
         if (bFirst)
           {
             x_strcat(buf,"-f ");
             bFirst=FALSE;
           }
         x_strcat(buf,lpmod->szProducts);
         }
     }

   // 
   ObjectLen = 16;
   BufLen    = x_strlen(buf);
   if (ObjectLen >= MAX_LINE_COMMAND-BufLen)
   {
     ErrorMessage ((UINT)IDS_E_TOO_MANY_TABLES_SELECTED, RES_ERR);
     return FALSE;
   }
   else
   {
     TCHAR szPage[10];
     if (lpmod->lPageSize!=0)
     {
       x_strcat(buf,"-page_size=");
       wsprintf(szPage,"%u ",lpmod->lPageSize);
       x_strcat(buf,szPage);
     }
   }

   // Wait for database to quiesce
   ObjectLen = 3;
   BufLen    = x_strlen(buf);

   if (ObjectLen >= MAX_LINE_COMMAND-BufLen)
   {
     ErrorMessage ((UINT)IDS_E_TOO_MANY_TABLES_SELECTED, RES_ERR);
     return FALSE;
   }
   else
   {
     if (lpmod->bWait)
       x_strcat(buf,"+w");
     else
       x_strcat(buf,"-w");
   }

   wsprintf(buf2,"sysmod on database %s::%s",
                  vnodeName,
                  lpmod->szDBName);

   execrmcmd(vnodeName,buf,buf2);

   return(TRUE);
}
