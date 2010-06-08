/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Project  : Visual DBA
**
**    Source   : otmizedb.c
**
**    Generate the optimizedb command string 
** History:
** 20-Jan-2000 (uk$so01)
**    Bug #100063
**    Eliminate the undesired compiler's warning
** 06-Mar_2000 (schph01)
**    Bug #100708
**    updated the new bRefuseTblWithDupName variable in the structure
**    for the new sub-dialog for selecting a table
** 04-Oct-2000 (schph01)
**    Bug 102814
**    move '-i' parameter before DatabaseName parameter, otherwise '-i'
**    is unrecognized, and add a space character between the '-i' and the
**    file name.
** 05-Oct-2000 (schph01)
**    Bug 102813
**    Add control in dialog box and modify the generated syntax for
**    the '-o' parameter.
** 26-Apr-2001 (uk$so01)
**    SIR #102678
**    Support the composite histogram of base table and secondary index.
** 15-Oct-2001 (schph01)
**    additional change for SIR #102678 Add control in dialog box to generate
**    syntax for -zlr and -zdn parameters.
** 22-Nov-2005 (fanra01)
**    Bug 114856
**    Once checked, the generate statistics option for a table cannot be
**    unchecked without first removing all specified tables.
**    When the specified table option is cleared remove all the tables from
**    the specified table list.
**  09-Mar-2010 (drivi01) 
**    SIR 123397
**    Update the dialog to be more user friendly.  Minimize the amount
**    of controls exposed to the user when dialog is initially displayed.
**    Add "Show/Hide Advanced" button to see more/less options in the 
**    dialog.  Updated some default values for several controls.
**  02-Jun-2010 (drivi01)
**    Remove hard coded buffer sizes.
*/

#include "dll.h"
#include "subedit.h"
#include "dbadlg1.h"
#include "dlgres.h"
#include "catospin.h"
#include "lexical.h"
#include "getdata.h"
#include "msghandl.h"
#include "domdata.h"
#include "dbaginfo.h"
#include "extccall.h"


typedef struct tagTWOCHAR
{
   char* char1;
   char* char2;
} TWOCHAR;

#define MAXF4x8   4 
static TWOCHAR f4x8data [MAXF4x8] = {

   {"Exponential", "E"},
   {"Float",       "F"},
   {"Dec align",   "G"},
   {"Float N",     "N"}
};

static int IdList [] =
{
   IDC_OPTIMIZEDB_STAT          ,
   IDC_OPTIMIZEDB_WAIT          ,
   IDC_OPTIMIZEDB_SYSCAT        ,
   IDC_OPTIMIZEDB_INFOCOL       ,
   IDC_OPTIMIZEDB_HISTOGRAM     ,
   IDC_OPTIMIZEDB_GETSTAT       ,
   IDC_OPTIMIZEDB_COMPLETEFLAG  ,
   IDC_OPTIMIZEDB_MINMAXVALUE   ,
   IDC_OPTIMIZEDB_SPECIFYTABLES ,
   IDC_OPTIMIZEDB_MCI_NUMBER    ,
   IDC_OPTIMIZEDB_MCI_SPIN      ,
   IDC_OPTIMIZEDB_MCE_NUMBER    ,
   IDC_OPTIMIZEDB_MCE_SPIN      ,
   IDC_OPTIMIZEDB_STATSAMPLEDATA,
   IDC_OPTIMIZEDB_SORTTUPLE     ,
   IDC_OPTIMIZEDB_PERCENT_NUMBER,
   IDC_OPTIMIZEDB_PERCENT_SPIN  ,
   IDC_OPTIMIZEDB_IDDISPLAY     ,
   IDC_OPTIMIZEDB_MCI_CHECK       ,
   IDC_OPTIMIZEDB_MCE_CHECK       ,
   IDC_OPTIMIZEDB_PRECILEVEL_TXT,
   IDC_OPTIMIZEDB_PERCENT_TXT   ,
   IDC_OPTIMIZEDB_IDTABLES      ,
   IDC_OPTIMIZEDB_SPECIFYCOLUMS ,
   IDC_OPTIMIZEDB_IDCOLUMS      ,
   IDC_CHKBX_OUTPUT_FILE        ,
   IDC_EDIT_OUTPUT_FILE         ,
   IDC_OPTIMIZEDB_ZLR           ,
   IDC_OPTIMIZEDB_ZDN           ,
   -1
};



//static AUDITDBTPARAMS table;
static LPTABLExCOLS   lpcolumns;
static DISPLAYPARAMS  display;
static int nUsageLimited;
static LPCHECKEDOBJECTS pSpecifiedIndex;
static BOOL bExpanded;

#define OPTIMIZEDB_PRECILEVEL_MIN         0
#define OPTIMIZEDB_PRECILEVEL_MAX        30
#define OPTIMIZEDB_MCI_MIN                1
#define OPTIMIZEDB_MCI_MAX             5000
#define OPTIMIZEDB_MCE_MIN                1
#define OPTIMIZEDB_MCE_MAX             5000
#define OPTIMIZEDB_PERCENT_MIN            1
#define OPTIMIZEDB_PERCENT_MAX           99

#define DefPreciLevel "10"
#define DefMCI        "100" 
#define DefMCE        "100"
#define DefPercent    "10"

#define PRESELECTED_TABLE_AND_COLUMN     1
#define PRESELECTED_TABLE_ONLY           2
#define NO_PRESELECTED_TABLE_AND_COLUMN  3
#define PRESELECTED_INDEX_ONLY           4


// Structure describing the numeric edit controls
static EDITMINMAX limits[] =
{
   IDC_OPTIMIZEDB_PRECILEVELNUM,   IDC_OPTIMIZEDB_PRECILEVELSPIN,
       OPTIMIZEDB_PRECILEVEL_MIN,
       OPTIMIZEDB_PRECILEVEL_MAX,
   IDC_OPTIMIZEDB_MCI_NUMBER,      IDC_OPTIMIZEDB_MCI_SPIN,
       OPTIMIZEDB_MCI_MIN,
       OPTIMIZEDB_MCI_MAX,
   IDC_OPTIMIZEDB_MCE_NUMBER,      IDC_OPTIMIZEDB_MCE_SPIN,
       OPTIMIZEDB_MCE_MIN,
       OPTIMIZEDB_MCE_MAX,
   IDC_OPTIMIZEDB_PERCENT_NUMBER,  IDC_OPTIMIZEDB_PERCENT_SPIN,
       OPTIMIZEDB_PERCENT_MIN,
       OPTIMIZEDB_PERCENT_MAX,

   -1  // terminator
};



BOOL CALLBACK __export DlgOptimizeDBDlgProc
                       (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

static void    OnDestroy (HWND hwnd);
static void    OnCommand (HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
static BOOL    OnInitDialog (HWND hwnd, HWND hwndFocus, LPARAM lParam);
static void    OnSysColorChange (HWND hwnd);
static void    FillFloat4x8Control (HWND hwnd);

static void    EnableDisableControls (HWND hwnd);
static void    InitializeControls (HWND hwnd);
static void    InitialiseSpinControls (HWND hwnd);
static void    EnableDisableOKButton (HWND hwnd);
static BOOL    FillStructureFromControls (HWND hwnd);
static BOOL    SpecifiedColumns (LPTABLExCOLS lpTablexCol);
static void    SetDefaultMessageOption (HWND hwnd);
static void    EnableControls (HWND hwnd, BOOL bEnable);


LPOBJECTLIST   FindStringInListObject (LPOBJECTLIST lpList, char* Element);
LPTABLExCOLS   FindStringInListTableAndColumns (LPTABLExCOLS lpTablexCol, char* aString);
LPTABLExCOLS   RmoveElement (LPTABLExCOLS lpTablexCol, char* aTable);
LPTABLExCOLS   AddElement   (LPTABLExCOLS lpTablexCol, char* aTable);
LPTABLExCOLS   FreeTableAndColumns (LPTABLExCOLS lpTablexCol);

LPTABLExCOLS TABLExCOLUMNS_Copy (LPTABLExCOLS first)
{
	LPOBJECTLIST lc, pObj, lNewCol;
	LPTABLExCOLS lpTemp;
	LPTABLExCOLS lpList = NULL, ls = first;

	lpTemp = NULL;
	while (ls)
	{
		lpTemp = AddElement (lpTemp, ls->TableName);
		//
		// Copy the list of columns:
		lNewCol = NULL;
		lc = ls->lpCol;
		while (lc)
		{
			LPTSTR lpszCol = (LPTSTR)lc->lpObject;
			pObj = AddListObjectTail (&lNewCol, lstrlen(lpszCol) +1);
			lstrcpy(pObj->lpObject, lpszCol);

			lc = lc->lpNext;
		}
		lpTemp->lpCol = lNewCol;

		ls = ls->next;
	}
	return lpTemp;
}



int WINAPI __export DlgOptimizeDB (HWND hwndOwner, LPOPTIMIZEDBPARAMS lpoptimizedb)
/*
// Function:
// Shows the Refresh dialog.
//
// Paramters:
//     1) hwndOwner:   Handle of the parent window for the dialog.
//     1) lpoptimizedb:   Points to structure containing information used to 
//                     initialise the dialog. 
//
// Returns:
//     TRUE if successful.
//
*/
{
   FARPROC lpProc;
   int     retVal;
   
   if (!IsWindow (hwndOwner) || !lpoptimizedb)
   {
       ASSERT(NULL);
       return FALSE;
   }

   lpProc = MakeProcInstance ((FARPROC) DlgOptimizeDBDlgProc, hInst);
   retVal = VdbaDialogBoxParam
       (hResource,
        MAKEINTRESOURCE (IDD_OPTIMIZEDB),
        hwndOwner,
        (DLGPROC) lpProc,
        (LPARAM)  lpoptimizedb
       );

   FreeProcInstance (lpProc);
   return (retVal);
}


BOOL CALLBACK __export DlgOptimizeDBDlgProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
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

           ASSERT (IsWindow(hwndCtl));
           ASSERT (lpdesc);

           return GetEditCtrlMinMaxValue (hwnd, hwndCtl, limits, &lpdesc->dwMin, &lpdesc->dwMax);
       }

       case LM_GETEDITSPINCTRL:
       {
           HWND hwndEdit       = (HWND) wParam;
           HWND FAR * lphwnd   = (HWND FAR *)lParam;
           *lphwnd             = GetEditSpinControl (hwndEdit, limits);
           break;
       }

       default:
           return HandleUserMessages (hwnd, message, wParam, lParam);
   }
   return TRUE;
}


static BOOL OnInitDialog (HWND hwnd, HWND hwndFocus, LPARAM lParam)
{

   LPOPTIMIZEDBPARAMS lpoptimizedb  = (LPOPTIMIZEDBPARAMS)lParam;
   char szFormat [100];
   char szTemp   [100];
   char szTitle  [MAX_TITLEBAR_LEN];
   RECT rect;

   pSpecifiedIndex = NULL;
   if (!AllocDlgProp (hwnd, lpoptimizedb))
      return FALSE;
   SpinGetVersion();
   LoadString (hResource, (UINT)IDS_T_OPTIMIZEDB, szFormat, sizeof (szFormat));
   if(lpoptimizedb->bSelectTable&&lpoptimizedb->bSelectColumn)
      nUsageLimited = PRESELECTED_TABLE_AND_COLUMN;
   else if(lpoptimizedb->bSelectTable&&lpoptimizedb->bSelectColumn == FALSE)
      nUsageLimited = PRESELECTED_TABLE_ONLY;
   else
      nUsageLimited = NO_PRESELECTED_TABLE_AND_COLUMN;

   if (lpoptimizedb->m_nObjectType == OT_INDEX)
   {
      nUsageLimited = PRESELECTED_INDEX_ONLY;
   }
   if (lpoptimizedb->bComposite)
      Button_SetCheck (GetDlgItem (hwnd, IDC_OPTIMIZEDB_COMPOSITE_ON_BASE_TABLE), TRUE);

   wsprintf (szTitle, szFormat,
      GetVirtNodeName ( GetCurMdiNodeHandle ()),
      lpoptimizedb->DBName);
   
   switch(nUsageLimited)
   {
   case PRESELECTED_TABLE_AND_COLUMN:
     wsprintf(szTemp,"   (%s.%s %s)",lpoptimizedb->TableOwner, lpoptimizedb->TableName, lpoptimizedb->ColumnName);
     lstrcat(szTitle,szTemp);
    break;
   case PRESELECTED_TABLE_ONLY:
     wsprintf(szTemp," , Table %s",lpoptimizedb->szDisplayTableName);
     lstrcat(szTitle,szTemp);
     break;
   case PRESELECTED_INDEX_ONLY:
     wsprintf(szTemp," , Index %s",lpoptimizedb->tchszDisplayIndexName);
     lstrcat(szTitle, szTemp);
     break;
   default:
     break;
   }

   SetWindowText (hwnd, szTitle);
   lpHelpStack = StackObject_PUSH (lpHelpStack, StackObject_INIT ((UINT)IDD_OPTIMIZEDB));

//   ZEROINIT (table);
//   table.lpTable   = NULL;
   lpcolumns       = NULL;
   if(nUsageLimited == PRESELECTED_TABLE_AND_COLUMN) {
       LPOBJECTLIST obj;
       lpcolumns = AddElement (lpcolumns, lpoptimizedb->TableName);
       obj = AddListObject (lpcolumns->lpCol, x_strlen (lpoptimizedb->ColumnName) +1);
       lstrcpy(obj->lpObject,lpoptimizedb->ColumnName);
       lpcolumns->lpCol = obj;
   }
   if(nUsageLimited == PRESELECTED_TABLE_ONLY)
   {
       lpcolumns = AddElement (lpcolumns, lpoptimizedb->TableName);
   }
   InitialiseSpinControls (hwnd);
   InitializeControls     (hwnd);
   EnableDisableControls  (hwnd);

   Button_SetCheck(GetDlgItem( hwnd, IDC_OPTIMIZEDB_GETSTAT), TRUE);
   
   GetWindowRect(hwnd, &rect);
   MoveWindow(hwnd, rect.left, rect.top, rect.right-rect.left, (int)((rect.bottom-rect.top)/2.2), TRUE);
   bExpanded = FALSE;

   richCenterDialog(hwnd);
   return TRUE;
}


static void OnDestroy(HWND hwnd)
{
   pSpecifiedIndex= FreeCheckedObjects (pSpecifiedIndex);

   SubclassAllNumericEditControls (hwnd, EC_RESETSUBCLASS);
   DeallocDlgProp(hwnd);
   lpHelpStack = StackObject_POP (lpHelpStack);
}


static void OnCommand (HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
   RECT rect;
   LPOPTIMIZEDBPARAMS lpoptimizedb = GetDlgProp (hwnd);

   switch (id)
   {
       case IDOK:
           if (nUsageLimited == PRESELECTED_TABLE_ONLY)
           {
             char szTblOwner[MAXOBJECTNAME];
             OwnerFromString(lpcolumns->TableName, szTblOwner);
             if ( IsTheSameOwner(GetCurMdiNodeHandle(),szTblOwner) == FALSE )
               break;
           }
           FillStructureFromControls (hwnd);
//           FreeObjectList (table.lpTable);
//           table.lpTable = NULL;
           lpcolumns = FreeTableAndColumns (lpcolumns);
			
           EndDialog (hwnd, TRUE);
           break;

       case IDCANCEL:
//           FreeObjectList (table.lpTable);
           lpcolumns     = FreeTableAndColumns (lpcolumns);
//           table.lpTable = NULL;

           EndDialog (hwnd, FALSE);
           break;

       case IDC_OPTIMIZEDB_IDDISPLAY:
           ZEROINIT (display);
           DlgDisplay (hwnd, &display);
           break;

       case IDC_SHOW_ADVANCED:
	   if (!bExpanded)
	   {
		GetWindowRect(hwnd, &rect);
		MoveWindow(hwnd, rect.left, rect.top, rect.right-rect.left, (int)((rect.bottom-rect.top)*2.2), TRUE);
		bExpanded = TRUE;
		Button_SetText(GetDlgItem(hwnd, IDC_SHOW_ADVANCED), "<< Hide &Advanced");
	   }
	   else
	   {
		GetWindowRect(hwnd, &rect);
		MoveWindow(hwnd, rect.left, rect.top, rect.right-rect.left, (int)((rect.bottom-rect.top)/2.2), TRUE);
		bExpanded = FALSE;
		Button_SetText(GetDlgItem(hwnd, IDC_SHOW_ADVANCED), "Show &Advanced >>");
	   }
	   break;

       case IDC_OPTIMIZEDB_IDTABLES:
           {
               int   iret;
               char* aString;
               char  szTable [MAXOBJECTNAME];
               LPOBJECTLIST ls, pObj;
               LPTABLExCOLS lc, ltb;
                AUDITDBTPARAMS tempTable;
                memset (&tempTable, 0, sizeof(tempTable));
               lstrcpy (tempTable.DBName,  lpoptimizedb->DBName);
               tempTable.bRefuseTblWithDupName = FALSE;
                tempTable.lpTable = NULL;
                ltb =  lpcolumns;
                while (ltb)
                {
                    LPTSTR lpszTable = (LPTSTR)ltb->TableName;
                    pObj = AddListObjectTail (&tempTable.lpTable, lstrlen(lpszTable) +1);
                    lstrcpy(pObj->lpObject, lpszTable);

                    ltb = ltb->next;
                }

                iret = DlgAuditDBTable (hwnd, &tempTable);
                if (iret == TRUE)
                {
                    ls = tempTable.lpTable;
                    while (ls)
                    {
                        aString = (char *)ls->lpObject;
                        if (!FindStringInListTableAndColumns (lpcolumns, aString))
                        {
                            //
                            // Add table  into TABLExCOLS
                            //
                            lpcolumns = AddElement (lpcolumns, aString);
                        }
                        ls = ls->lpNext;
                    }

                    lc = lpcolumns;
                    while (lc)
                    {
                        if (!FindStringInListObject (tempTable.lpTable, lc->TableName))
                        {
                            //
                            // Delete table from TABLExCOLS
                            //
                            x_strcpy (szTable, lc->TableName);
                            lc = lc->next;
                            lpcolumns = RmoveElement (lpcolumns, szTable);
                        }
                        else lc = lc->next;
                    }
                }

                FreeObjectList(tempTable.lpTable);
                tempTable.lpTable = NULL;
                EnableDisableOKButton  (hwnd);
                if (lpcolumns)
                    Button_SetCheck (GetDlgItem (hwnd, IDC_OPTIMIZEDB_SPECIFYTABLES), TRUE);
                else
                {
                    Button_SetCheck (GetDlgItem (hwnd, IDC_OPTIMIZEDB_SPECIFYTABLES), FALSE);
                    EnableWindow    (GetDlgItem (hwnd, IDC_OPTIMIZEDB_IDTABLES), FALSE);
                }
                EnableDisableControls(hwnd);
           }
           break;


       case IDC_OPTIMIZEDB_IDCOLUMS:
           {
               int  iret;
               //
               // Show the colums of a table
               // Check the colums that have been checked
               //
               SPECIFYCOLPARAMS specifycolumns;
               ZEROINIT (specifycolumns);
               x_strcpy (specifycolumns.DBName, lpoptimizedb->DBName);
               specifycolumns.lpTableAndColumns = TABLExCOLUMNS_Copy(lpcolumns);
               iret = DlgSpecifyColumns (hwnd, &specifycolumns);
               if(iret == TRUE)
               {
                    lpcolumns = FreeTableAndColumns (lpcolumns);
                    lpcolumns = specifycolumns.lpTableAndColumns;
               }
               else
                    specifycolumns.lpTableAndColumns = FreeTableAndColumns(specifycolumns.lpTableAndColumns);

               if (SpecifiedColumns (lpcolumns))
                   Button_SetCheck (GetDlgItem (hwnd, IDC_OPTIMIZEDB_SPECIFYCOLUMS),  TRUE);
               else
                   Button_SetCheck (GetDlgItem (hwnd, IDC_OPTIMIZEDB_SPECIFYCOLUMS),  FALSE);
               EnableDisableControls (hwnd);
               EnableDisableOKButton (hwnd);
           }
           break;

       case IDC_OPTIMIZEDB_BUTTON_INDEX:
           {
               VDBA_ChooseIndex(hwnd, lpoptimizedb->DBName, &pSpecifiedIndex);
               if (pSpecifiedIndex)
                   Button_SetCheck (GetDlgItem (hwnd, IDC_OPTIMIZEDB_CHECK_INDEX), TRUE);
               else
                   Button_SetCheck (GetDlgItem (hwnd, IDC_OPTIMIZEDB_CHECK_INDEX), FALSE);
           }
           break;

       case IDC_OPTIMIZEDB_PRECILEVELNUM :
       case IDC_OPTIMIZEDB_MCI_NUMBER    :
       case IDC_OPTIMIZEDB_MCE_NUMBER    :
       case IDC_OPTIMIZEDB_PERCENT_NUMBER:
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

                   nCount = Edit_GetTextLength (hwndCtl);
                   if (nCount == 0)
                       break;

                   if (!VerifyAllNumericEditControls (hwnd, TRUE, TRUE))
                       break;
                   UpdateSpinButtons (hwnd);
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

                   if (!VerifyAllNumericEditControls (hwnd, TRUE, TRUE))
                       break;

                   UpdateSpinButtons (hwnd);
                   break;
               }
           }
           break;
           
       case IDC_OPTIMIZEDB_PRECILEVELSPIN:
       case IDC_OPTIMIZEDB_MCI_SPIN      :
       case IDC_OPTIMIZEDB_MCE_SPIN      :
       case IDC_OPTIMIZEDB_PERCENT_SPIN  :
           ProcessSpinControl (hwndCtl, codeNotify, limits);
           break;

       case IDC_OPTIMIZEDB_STAT  :
       case IDC_OPTIMIZEDB_SPECIFYTABLES:
       case IDC_OPTIMIZEDB_SPECIFYCOLUMS:
       case IDC_OPTIMIZEDB_CHECK_INDEX:
           EnableDisableControls (hwnd);
           break;

       case IDC_OPTIMIZEDB_PARAM :
           if (Button_GetCheck   (hwndCtl))
               EnableControls    (hwnd, FALSE);
           else
               EnableControls    (hwnd, TRUE);
           EnableDisableControls (hwnd);
           break;

       case IDC_OPTIMIZEDB_PARAMFILE:
           if (codeNotify == EN_CHANGE)
           {
               if (Edit_GetTextLength (hwndCtl) == 0)
                   EnableWindow (GetDlgItem (hwnd, IDOK), FALSE);
               else
                   EnableWindow (GetDlgItem (hwnd, IDOK), TRUE);
           }
           break;
       case IDC_OPTIMIZEDB_STATSAMPLEDATA:
           EnableDisableControls (hwnd);
           break;
       case IDC_CHKBX_OUTPUT_FILE:
		   EnableDisableControls (hwnd);
           break;
       case IDC_OPTIMIZEDB_MCI_CHECK:   //the new MCI checkbox
		   if (Button_GetCheck (GetDlgItem(hwnd, IDC_OPTIMIZEDB_MCI_CHECK)) == BST_CHECKED)
		   {
			   EnableWindow (GetDlgItem (hwnd, IDC_OPTIMIZEDB_MCI_NUMBER), TRUE);
			   EnableWindow (GetDlgItem (hwnd, IDC_OPTIMIZEDB_MCI_SPIN), TRUE);
		   }
		   else
		   {
			   EnableWindow (GetDlgItem (hwnd, IDC_OPTIMIZEDB_MCI_NUMBER), FALSE);
			   EnableWindow (GetDlgItem (hwnd, IDC_OPTIMIZEDB_MCI_SPIN), FALSE);
		   }
		   break;
	   case IDC_OPTIMIZEDB_MCE_CHECK:   //the new MCE checkbox
		   if (Button_GetCheck (GetDlgItem(hwnd, IDC_OPTIMIZEDB_MCE_CHECK)) == BST_CHECKED)
		   {
			   EnableWindow (GetDlgItem (hwnd, IDC_OPTIMIZEDB_MCE_NUMBER), TRUE);
			   EnableWindow (GetDlgItem (hwnd, IDC_OPTIMIZEDB_MCE_SPIN), TRUE);
		   }
		   else
		   {
			   EnableWindow (GetDlgItem (hwnd, IDC_OPTIMIZEDB_MCE_NUMBER), FALSE);
			   EnableWindow (GetDlgItem (hwnd, IDC_OPTIMIZEDB_MCE_SPIN), FALSE);
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


static void InitializeControls (HWND hwnd)
{
   char *szMin = "1";

   HWND hwndPreciLevel = GetDlgItem (hwnd, IDC_OPTIMIZEDB_PRECILEVELNUM ); 
   HWND hwndMCI        = GetDlgItem (hwnd, IDC_OPTIMIZEDB_MCI_NUMBER    );
   HWND hwndMCE        = GetDlgItem (hwnd, IDC_OPTIMIZEDB_MCE_NUMBER    );
   HWND hwndPercent    = GetDlgItem (hwnd, IDC_OPTIMIZEDB_PERCENT_NUMBER);

   SubclassAllNumericEditControls (hwnd, EC_SUBCLASS);
   
   Edit_LimitText (hwndPreciLevel, 10);
   Edit_LimitText (hwndMCI       , 10);
   Edit_LimitText (hwndMCE       , 10);
   Edit_LimitText (hwndPercent   , 10);

   Edit_SetText   (hwndPreciLevel, DefPreciLevel);
   Edit_SetText   (hwndMCI       , DefMCI       );
   Edit_SetText   (hwndMCE       , DefMCE       );
   Edit_SetText   (hwndPercent   , DefPercent   );
   LimitNumericEditControls (hwnd);
}



static void InitialiseSpinControls (HWND hwnd)
{
   DWORD dwMin, dwMax;

   GetEditCtrlMinMaxValue (hwnd, GetDlgItem (hwnd, IDC_OPTIMIZEDB_PRECILEVELNUM), limits, &dwMin, &dwMax);
   SpinRangeSet (GetDlgItem (hwnd, IDC_OPTIMIZEDB_PRECILEVELSPIN), dwMin, dwMax);

   GetEditCtrlMinMaxValue (hwnd, GetDlgItem (hwnd, IDC_OPTIMIZEDB_MCI_NUMBER    ), limits, &dwMin, &dwMax);
   SpinRangeSet (GetDlgItem (hwnd, IDC_OPTIMIZEDB_MCI_SPIN      ), dwMin, dwMax);

   GetEditCtrlMinMaxValue (hwnd, GetDlgItem (hwnd, IDC_OPTIMIZEDB_MCE_NUMBER    ), limits, &dwMin, &dwMax);
   SpinRangeSet (GetDlgItem (hwnd, IDC_OPTIMIZEDB_MCE_SPIN      ), dwMin, dwMax);

   GetEditCtrlMinMaxValue (hwnd, GetDlgItem (hwnd, IDC_OPTIMIZEDB_PERCENT_NUMBER), limits, &dwMin, &dwMax);
   SpinRangeSet (GetDlgItem (hwnd, IDC_OPTIMIZEDB_PERCENT_SPIN  ), dwMin, dwMax);
}


static void EnableDisableOKButton (HWND hwnd)
{

}

static BOOL FillStructureFromControls (HWND hwnd)
{
   char szGreatBuffer   [MAXOBJECTNAME*10];
   char szBuffer        [_MAX_PATH];
   char buf1            [MAXOBJECTNAME];
   char buftemp         [MAXOBJECTNAME*4];

   HWND hwnd_zfParamFile = GetDlgItem (hwnd, IDC_OPTIMIZEDB_PARAMFILE);
   HWND hwnd_iFileName   = GetDlgItem (hwnd, IDC_OPTIMIZEDB_STATFILE);
   HWND hwnd_oFileName   = GetDlgItem (hwnd, IDC_EDIT_OUTPUT_FILE);
   HWND hwnd_Percent     = GetDlgItem (hwnd, IDC_OPTIMIZEDB_PERCENT_NUMBER);
   HWND hwnd_MCI         = GetDlgItem (hwnd, IDC_OPTIMIZEDB_MCI_NUMBER);
   HWND hwnd_MCE         = GetDlgItem (hwnd, IDC_OPTIMIZEDB_MCE_NUMBER);
   HWND hwnd_level       = GetDlgItem (hwnd, IDC_OPTIMIZEDB_PRECILEVELNUM);
   
   LPUCHAR vnodeName = GetVirtNodeName (GetCurMdiNodeHandle ());
   LPOPTIMIZEDBPARAMS  lpoptimizedb  = GetDlgProp (hwnd);
   LPTABLExCOLS columnslist = lpcolumns;
   char szgateway[200];
   BOOL bHasGWSuffix = GetGWClassNameFromString(vnodeName, szgateway);

   ZEROINIT (szGreatBuffer);
   ZEROINIT (szBuffer);

   x_strcpy (szGreatBuffer, "optimizedb ");

   //
   // Ingres 2.6 only:
   // Generate -zcpk 
   //
   if (Button_GetCheck (GetDlgItem (hwnd, IDC_OPTIMIZEDB_COMPOSITE_ON_BASE_TABLE)))
       x_strcat (szGreatBuffer, " -zcpk");

   //
   // Generate the -uusername
   //
   if (DBAGetSessionUser(vnodeName, buftemp)) {
      x_strcat (szGreatBuffer, " -u");
      suppspace(buftemp);
      x_strcat (szGreatBuffer, buftemp);
      x_strcat (szGreatBuffer, " ");

   }


   //
   // Generate -zffilename
   //
   if (Button_GetCheck (GetDlgItem (hwnd, IDC_OPTIMIZEDB_PARAM)))
   {
       if (Edit_GetTextLength (hwnd_zfParamFile) > 0)
       {
           ZEROINIT (szBuffer);
           Edit_GetText (hwnd_zfParamFile, szBuffer, sizeof (szBuffer));
           NoLeadingEndingSpace (szBuffer);
           x_strcat (szGreatBuffer, " -zf ");
           x_strcat (szGreatBuffer, szBuffer);
       }
   }
   else
   {
       if (Button_GetCheck (GetDlgItem (hwnd, IDC_OPTIMIZEDB_SYSCAT)))
           x_strcat (szGreatBuffer, " +U");

       if (Button_GetCheck (GetDlgItem (hwnd, IDC_OPTIMIZEDB_WAIT)))
           x_strcat (szGreatBuffer, " +w");
       //
       // I don't kdow how to generate -xk for SQL option flags
       // TO BE FINISHED

       if (x_strlen (display.szString) > 0)
           x_strcat (szGreatBuffer, display.szString);

       //
       // Generate -Z flags
       // 

       //
       // Generate -zc (confuse with -U syst cat ?)
       //

       //
       // Generate -zh
       //
       if (Button_GetCheck (GetDlgItem (hwnd, IDC_OPTIMIZEDB_HISTOGRAM)))
           x_strcat (szGreatBuffer, " -zh");


       //
       // Generate -zk
       //
       if (Button_GetCheck (GetDlgItem (hwnd, IDC_OPTIMIZEDB_GETSTAT)))
           x_strcat (szGreatBuffer, " -zk");

       //
       // Generate -zn#, -zp
       //

       if (Button_GetCheck (GetDlgItem (hwnd, IDC_OPTIMIZEDB_STAT)))
       {
           if (Edit_GetTextLength (hwnd_level) > 0)
           {
               ZEROINIT (szBuffer);
               Edit_GetText (hwnd_level, szBuffer, sizeof (szBuffer));
               x_strcat (szGreatBuffer, " -zn");
               x_strcat (szGreatBuffer, szBuffer);
           }
           if (Button_GetCheck (GetDlgItem (hwnd, IDC_OPTIMIZEDB_ROW_PAGE)))
               x_strcat (szGreatBuffer, " -zp");
       }

       //
       // Generate -zr#
       //
       ZEROINIT (buf1);

       Edit_GetText (hwnd_MCI, buf1, sizeof (buf1));
       if ((Edit_GetTextLength (hwnd_MCI) > 0) && x_strcmp (buf1, DefMCI) != 0)
       {
           ZEROINIT (szBuffer);
           Edit_GetText (hwnd_MCI, szBuffer, sizeof (szBuffer));
           x_strcat (szGreatBuffer, " -zr");
           x_strcat (szGreatBuffer, szBuffer);
       }
       //
       // Generate -zu#
       //
       ZEROINIT (buf1);

       Edit_GetText (hwnd_MCE, buf1, sizeof (buf1));
       if ((Edit_GetTextLength (hwnd_MCE) > 0) && x_strcmp (buf1, DefMCE) != 0)
       {
           ZEROINIT (szBuffer);
           Edit_GetText (hwnd_MCE, szBuffer, sizeof (szBuffer));
           x_strcat (szGreatBuffer, " -zu");
           x_strcat (szGreatBuffer, szBuffer);
       }

       //
       // Generate -zs[s]#
       //
       if (Button_GetCheck (GetDlgItem (hwnd, IDC_OPTIMIZEDB_STATSAMPLEDATA)))
       {
           x_strcat (szGreatBuffer, " -zs");
           if (Button_GetCheck (GetDlgItem (hwnd, IDC_OPTIMIZEDB_SORTTUPLE)))
               x_strcat (szGreatBuffer, "s");

           if (Edit_GetTextLength (hwnd_Percent) > 0)
           {
               ZEROINIT (szBuffer);
               Edit_GetText (hwnd_Percent, szBuffer, sizeof (szBuffer));
               x_strcat (szGreatBuffer, szBuffer);
           }
           //
           // Generate -zdn
           //
           if (Button_GetCheck (GetDlgItem (hwnd, IDC_OPTIMIZEDB_ZDN)))
               x_strcat (szGreatBuffer, " -zdn");
       }

       //
       // Generate -zv
       //
       if (Button_GetCheck (GetDlgItem (hwnd, IDC_OPTIMIZEDB_INFOCOL)))
           x_strcat (szGreatBuffer, " -zv");

       //
       // Generate -zw
       //
       if (Button_GetCheck (GetDlgItem (hwnd, IDC_OPTIMIZEDB_COMPLETEFLAG)))
           x_strcat (szGreatBuffer, " -zw");

       //
       // Generate -zx
       //
       if (Button_GetCheck (GetDlgItem (hwnd, IDC_OPTIMIZEDB_MINMAXVALUE)))
           x_strcat (szGreatBuffer, " -zx");

       //
       // Generate -zlr
       //
       if (Button_GetCheck (GetDlgItem (hwnd, IDC_OPTIMIZEDB_ZLR)))
           x_strcat (szGreatBuffer, " -zlr");

       //
       // Generate -i filename
       //
       if (Button_GetCheck (GetDlgItem (hwnd, IDC_OPTIMIZEDB_STAT)))
       {
           if (Edit_GetTextLength (hwnd_iFileName) > 0)
           {
               ZEROINIT (szBuffer);
               Edit_GetText (hwnd_iFileName, szBuffer, sizeof (szBuffer));
               NoLeadingEndingSpace (szBuffer);
               x_strcat (szGreatBuffer, " -i ");
               x_strcat (szGreatBuffer, szBuffer);
           }
       }

       //
       // Generate -o filename
       //
       if (Button_GetCheck (GetDlgItem (hwnd, IDC_CHKBX_OUTPUT_FILE)))
       {
           if (Edit_GetTextLength (hwnd_oFileName) > 0)
           {
               ZEROINIT (szBuffer);
               Edit_GetText (hwnd_oFileName, szBuffer, sizeof (szBuffer));
               NoLeadingEndingSpace (szBuffer);
               x_strcat (szGreatBuffer, " -o ");
               x_strcat (szGreatBuffer, szBuffer);
           }
       }

       x_strcat (szGreatBuffer, " ");
       x_strcat (szGreatBuffer, lpoptimizedb->DBName);      // DBName
       if (bHasGWSuffix){
           x_strcat(szGreatBuffer,"/");
           x_strcat(szGreatBuffer,szgateway);
       }


       //
       // Generate Table and columns (-rtablename {-acolumnname}}
       //
       if (Button_GetCheck (GetDlgItem (hwnd, IDC_OPTIMIZEDB_SPECIFYTABLES)))
       {
           while (columnslist)
           {   
               LPOBJECTLIST ls = columnslist->lpCol;
               char* col;
               x_strcat (szGreatBuffer, " -r");
               x_strcat (szGreatBuffer, StringWithoutOwner(columnslist->TableName));
               if (Button_GetCheck (GetDlgItem (hwnd, IDC_OPTIMIZEDB_SPECIFYCOLUMS)) && !lpoptimizedb->bComposite)
               {
                   while (ls)
                   {
                       x_strcat (szGreatBuffer, " -a");
                       col = (char *)ls->lpObject;
                       x_strcat (szGreatBuffer, col);
                       ls = ls->lpNext;
                   }
               }
               columnslist = columnslist->next;
           }
       }

       //
       // Ingres 2.6 only:
       // Generate Secondary Indexes -r<Index name>
       if (lpoptimizedb->m_nObjectType == OT_INDEX)
       {
           x_strcat (szGreatBuffer, " -r");
           x_strcat (szGreatBuffer, StringWithoutOwner(lpoptimizedb->tchszIndexName));
       }
       else
       if (lpoptimizedb->m_nObjectType == OT_DATABASE && 
           Button_GetCheck (GetDlgItem (hwnd, IDC_OPTIMIZEDB_CHECK_INDEX)) &&
           pSpecifiedIndex)
       {
            LPCHECKEDOBJECTS ls = pSpecifiedIndex;
            while (ls)
            {
                x_strcat (szGreatBuffer, " -r");
                x_strcat (szGreatBuffer, ls->dbname); // ls->dbname noes not have owner prefixed !

                ls = ls->pnext;
            }
       }
   }


   wsprintf(buftemp,
       ResourceString ((UINT)IDS_T_RMCMD_OPTIMIZEDB), //"generate statistics on database %s::%s",
       vnodeName,
       lpoptimizedb->DBName);
   execrmcmd(vnodeName,szGreatBuffer,buftemp);
   return TRUE;
}



LPOBJECTLIST FindStringInListObject (LPOBJECTLIST lpList, char* Element)
{
   LPOBJECTLIST ls = lpList;
   char* aString;

   while (ls)
   {
       aString = (char *) ls->lpObject;

       if (x_strcmp (aString, Element) == 0)
           return(ls);
       ls = ls->lpNext;
   }
   return NULL;
}

LPTABLExCOLS FindStringInListTableAndColumns (LPTABLExCOLS lpTablexCol, char* aString)
{
   LPTABLExCOLS ls = lpTablexCol;


   while (ls)
   {
       if (x_strcmp (ls->TableName, aString) == 0)
           return (ls);

       ls = ls->next;
   }
   return NULL;
}

LPTABLExCOLS RmoveElement (LPTABLExCOLS lpTablexCol, char* aTable)
{
   LPTABLExCOLS tm, prec;
   LPTABLExCOLS ls = lpTablexCol;

   prec = ls;
   while (ls)
   {
       if (x_strcmp (ls->TableName, aTable) == 0)
       {
           tm = ls;
           if (ls == lpTablexCol)
           {
               ls = ls->next;

               FreeObjectList (tm->lpCol);
               tm->lpCol = NULL;
               ESL_FreeMem (tm);
               tm = NULL;
               return (ls);
           }
           else
           {
               prec->next = ls->next;
               FreeObjectList (tm->lpCol);
               tm->lpCol = NULL;
               ESL_FreeMem (tm);
               tm = NULL;
               return (lpTablexCol);
           }
       }
       prec = ls;
       ls = ls->next;
   }
   return lpTablexCol;
}

LPTABLExCOLS AddElement (LPTABLExCOLS lpTablexCol, char* aTable)
{
   LPTABLExCOLS obj;

   if (aTable)
   {
       obj = ESL_AllocMem (sizeof (TABLExCOLS));
       if (obj)
       {
           x_strcpy (obj->TableName, aTable);
           obj->lpCol = NULL;
           obj->next  = lpTablexCol;
           lpTablexCol= obj;
           return (lpTablexCol);
       }
       else
       {
           FreeTableAndColumns (lpTablexCol);
           ErrorMessage ((UINT) IDS_E_CANNOT_ALLOCATE_MEMORY, RES_ERR);
           return NULL;
       }
   }
   return NULL;
}

LPTABLExCOLS FreeTableAndColumns (LPTABLExCOLS lpTablexCol)
{
   LPTABLExCOLS ls = lpTablexCol, tmp;

   while (ls)
   {
       tmp = ls;
       ls  = ls->next;
       FreeObjectList (tmp->lpCol);
       tmp->lpCol = NULL;
       ESL_FreeMem (tmp);
   }
   return (ls);
}


static void EnableDisableControls (HWND hwnd)
{
   LPOPTIMIZEDBPARAMS  lpoptimizedb  = GetDlgProp (hwnd);

   if (Button_GetCheck (GetDlgItem (hwnd, IDC_OPTIMIZEDB_STATSAMPLEDATA)) == BST_CHECKED &&
       IsWindowEnabled( GetDlgItem (hwnd, IDC_OPTIMIZEDB_STATSAMPLEDATA)) )
   {
      EnableWindow (GetDlgItem (hwnd, IDC_OPTIMIZEDB_PERCENT_TXT)   ,TRUE);
      EnableWindow (GetDlgItem (hwnd, IDC_OPTIMIZEDB_PERCENT_SPIN)  ,TRUE);
      EnableWindow (GetDlgItem (hwnd, IDC_OPTIMIZEDB_SORTTUPLE)     ,TRUE);
      EnableWindow (GetDlgItem (hwnd, IDC_OPTIMIZEDB_PERCENT_NUMBER),TRUE);
      EnableWindow (GetDlgItem (hwnd, IDC_OPTIMIZEDB_ZDN),TRUE);
	  Button_SetCheck ( GetDlgItem(hwnd, IDC_OPTIMIZEDB_SORTTUPLE), TRUE);
   }
   else
   {
      EnableWindow (GetDlgItem (hwnd, IDC_OPTIMIZEDB_PERCENT_TXT)   ,FALSE);
      EnableWindow (GetDlgItem (hwnd, IDC_OPTIMIZEDB_PERCENT_SPIN)  ,FALSE);
      EnableWindow (GetDlgItem (hwnd, IDC_OPTIMIZEDB_SORTTUPLE)     ,FALSE);
      EnableWindow (GetDlgItem (hwnd, IDC_OPTIMIZEDB_PERCENT_NUMBER),FALSE);
      EnableWindow (GetDlgItem (hwnd, IDC_OPTIMIZEDB_ZDN),FALSE);
	  Button_SetCheck ( GetDlgItem(hwnd, IDC_OPTIMIZEDB_SORTTUPLE), FALSE);
   }
   if (Button_GetCheck (GetDlgItem (hwnd, IDC_CHKBX_OUTPUT_FILE)) == BST_CHECKED)
   {
      EnableWindow (GetDlgItem (hwnd, IDC_EDIT_OUTPUT_FILE) ,TRUE);
      EnableWindow (GetDlgItem (hwnd, IDC_OPTIMIZEDB_STAT),FALSE);
   }
   else
   {
      EnableWindow (GetDlgItem (hwnd, IDC_EDIT_OUTPUT_FILE),FALSE);
      if (Button_GetCheck (GetDlgItem (hwnd, IDC_OPTIMIZEDB_PARAM)) == BST_UNCHECKED)
         EnableWindow (GetDlgItem (hwnd, IDC_OPTIMIZEDB_STAT),TRUE);
   }
   if (Button_GetCheck (GetDlgItem (hwnd, IDC_OPTIMIZEDB_STAT)) == BST_CHECKED)
   {
      EnableWindow (GetDlgItem (hwnd, IDC_OPTIMIZEDB_PRECILEVELNUM) ,TRUE);
      EnableWindow (GetDlgItem (hwnd, IDC_OPTIMIZEDB_PRECILEVELSPIN),TRUE);
      EnableWindow (GetDlgItem (hwnd, IDC_OPTIMIZEDB_ROW_PAGE)      ,TRUE);
      EnableWindow (GetDlgItem (hwnd, IDC_OPTIMIZEDB_STATFILE)      ,TRUE);
      EnableWindow (GetDlgItem (hwnd, IDC_CHKBX_OUTPUT_FILE),FALSE);
   }
   else
   {
      EnableWindow (GetDlgItem (hwnd, IDC_OPTIMIZEDB_PRECILEVELNUM) ,FALSE);
      EnableWindow (GetDlgItem (hwnd, IDC_OPTIMIZEDB_PRECILEVELSPIN),FALSE);
      EnableWindow (GetDlgItem (hwnd, IDC_OPTIMIZEDB_ROW_PAGE)      ,FALSE);
      EnableWindow (GetDlgItem (hwnd, IDC_OPTIMIZEDB_STATFILE)      ,FALSE);
      if (Button_GetCheck (GetDlgItem (hwnd, IDC_OPTIMIZEDB_PARAM)) == BST_UNCHECKED)
         EnableWindow (GetDlgItem (hwnd, IDC_CHKBX_OUTPUT_FILE),TRUE);
   }
   if (Button_GetCheck (GetDlgItem (hwnd, IDC_OPTIMIZEDB_PARAM)) == BST_CHECKED)
      EnableWindow (GetDlgItem (hwnd, IDC_OPTIMIZEDB_PARAMFILE), TRUE);
   else
      EnableWindow (GetDlgItem (hwnd, IDC_OPTIMIZEDB_PARAMFILE), FALSE);

   if (nUsageLimited == NO_PRESELECTED_TABLE_AND_COLUMN) {
       if (Button_GetCheck (GetDlgItem (hwnd, IDC_OPTIMIZEDB_SPECIFYTABLES)) &&
           IsWindowEnabled( GetDlgItem (hwnd, IDC_OPTIMIZEDB_SPECIFYTABLES)) )
       {
           EnableWindow (GetDlgItem (hwnd, IDC_OPTIMIZEDB_IDTABLES), TRUE);
       }
       else
       {
            /*
            ** The specify table option has been unchecked.  Disable the
            ** button.
            */
            EnableWindow (GetDlgItem (hwnd, IDC_OPTIMIZEDB_IDTABLES), FALSE);
            /*
            ** Remove all previously selected tables.  If lpcolumns is not
            ** empty the specify table option is reset below.
            */
            lpcolumns     = FreeTableAndColumns (lpcolumns);
       }
   }
   else if (nUsageLimited == PRESELECTED_TABLE_AND_COLUMN)
   {
       //disable Button
       EnableWindow (GetDlgItem (hwnd, IDC_OPTIMIZEDB_PARAM)    ,FALSE);
       EnableWindow (GetDlgItem (hwnd, IDC_OPTIMIZEDB_PARAMFILE),FALSE);
       // Table
       Button_SetCheck (GetDlgItem (hwnd, IDC_OPTIMIZEDB_SPECIFYTABLES), TRUE);
       EnableWindow    (GetDlgItem (hwnd, IDC_OPTIMIZEDB_SPECIFYTABLES), FALSE);
       ShowWindow      (GetDlgItem (hwnd, IDC_OPTIMIZEDB_IDTABLES)     , SW_HIDE);
       ShowWindow      (GetDlgItem (hwnd, IDC_STATIC_TABLE_NAME)       , SW_SHOW);
       ShowWindow      (GetDlgItem (hwnd, IDC_EDIT_TABLE_NAME)         , SW_SHOW);
       Edit_SetText    (GetDlgItem (hwnd, IDC_EDIT_TABLE_NAME),
                        lpcolumns->TableName);
       EnableWindow    (GetDlgItem (hwnd, IDC_EDIT_TABLE_NAME)         , FALSE);
       // Column
       Button_SetCheck (GetDlgItem (hwnd, IDC_OPTIMIZEDB_SPECIFYCOLUMS), TRUE);
       EnableWindow (GetDlgItem (hwnd, IDC_OPTIMIZEDB_SPECIFYCOLUMS)   , FALSE);
       ShowWindow   (GetDlgItem (hwnd, IDC_OPTIMIZEDB_IDCOLUMS)        , SW_HIDE);
       ShowWindow   (GetDlgItem (hwnd, IDC_STATIC_COLUMN_NAME)         ,SW_SHOW);
       ShowWindow   (GetDlgItem (hwnd, IDC_EDIT_COLUMN_NAME)           ,SW_SHOW);
       Edit_SetText (GetDlgItem (hwnd, IDC_EDIT_COLUMN_NAME),lpcolumns->lpCol->lpObject);
       EnableWindow (GetDlgItem (hwnd, IDC_EDIT_COLUMN_NAME)           ,FALSE);

       EnableWindow (GetDlgItem (hwnd, IDC_OPTIMIZEDB_SYSCAT)          , FALSE);
       EnableWindow (GetDlgItem (hwnd, IDC_OPTIMIZEDB_INFOCOL)         , FALSE);
       EnableWindow (GetDlgItem (hwnd, IDC_OPTIMIZEDB_HISTOGRAM)       , FALSE);
   }
   else // PRESELECTED_TABLE_ONLY
   {
       EnableWindow    (GetDlgItem (hwnd, IDC_OPTIMIZEDB_PARAM)        ,FALSE);
       EnableWindow    (GetDlgItem (hwnd, IDC_OPTIMIZEDB_PARAMFILE)    ,FALSE);
       Button_SetCheck (GetDlgItem (hwnd, IDC_OPTIMIZEDB_SPECIFYTABLES),TRUE);
       EnableWindow    (GetDlgItem (hwnd, IDC_OPTIMIZEDB_SPECIFYTABLES),FALSE);
       ShowWindow      (GetDlgItem (hwnd, IDC_OPTIMIZEDB_IDTABLES)     ,SW_HIDE);
       ShowWindow      (GetDlgItem (hwnd, IDC_STATIC_TABLE_NAME)       ,SW_SHOW);
       ShowWindow      (GetDlgItem (hwnd, IDC_EDIT_TABLE_NAME)         ,SW_SHOW);
       Edit_SetText    (GetDlgItem (hwnd, IDC_EDIT_TABLE_NAME),
                        lpoptimizedb->szDisplayTableName);
       EnableWindow    (GetDlgItem (hwnd, IDC_EDIT_TABLE_NAME)         ,FALSE);

       if (Button_GetCheck (GetDlgItem (hwnd, IDC_OPTIMIZEDB_SPECIFYCOLUMS)) == BST_CHECKED)
         EnableWindow (GetDlgItem (hwnd, IDC_OPTIMIZEDB_IDCOLUMS), TRUE);
       else
         EnableWindow (GetDlgItem (hwnd, IDC_OPTIMIZEDB_IDCOLUMS), FALSE);

       EnableWindow (GetDlgItem (hwnd, IDC_OPTIMIZEDB_SYSCAT)   , FALSE);
       EnableWindow (GetDlgItem (hwnd, IDC_OPTIMIZEDB_INFOCOL)  , FALSE);
       EnableWindow (GetDlgItem (hwnd, IDC_OPTIMIZEDB_HISTOGRAM), FALSE);
   }

   if (lpcolumns) // List of tables and their columns: 
   {
       //
       // Exist Table:
       if (nUsageLimited != PRESELECTED_TABLE_AND_COLUMN)
           EnableWindow (GetDlgItem (hwnd, IDC_OPTIMIZEDB_SPECIFYCOLUMS),  TRUE);
       Button_SetCheck (GetDlgItem (hwnd, IDC_OPTIMIZEDB_SPECIFYTABLES), TRUE);
       if (Button_GetCheck (GetDlgItem (hwnd, IDC_OPTIMIZEDB_SPECIFYCOLUMS)))
           EnableWindow (GetDlgItem (hwnd, IDC_OPTIMIZEDB_IDCOLUMS), TRUE);
       else
           EnableWindow (GetDlgItem (hwnd, IDC_OPTIMIZEDB_IDCOLUMS), FALSE);
   }
   else
   {
       Button_SetCheck (GetDlgItem (hwnd, IDC_OPTIMIZEDB_SPECIFYCOLUMS), FALSE);
       EnableWindow (GetDlgItem (hwnd, IDC_OPTIMIZEDB_SPECIFYCOLUMS),  FALSE);
       EnableWindow (GetDlgItem (hwnd, IDC_OPTIMIZEDB_IDCOLUMS), FALSE);
   }

   if (Button_GetCheck (GetDlgItem (hwnd, IDC_OPTIMIZEDB_CHECK_INDEX)) &&
       IsWindowEnabled( GetDlgItem (hwnd, IDC_OPTIMIZEDB_CHECK_INDEX)) )
   {
       EnableWindow (GetDlgItem (hwnd, IDC_OPTIMIZEDB_BUTTON_INDEX), TRUE);
   }
   else
   {
       EnableWindow (GetDlgItem (hwnd, IDC_OPTIMIZEDB_BUTTON_INDEX), FALSE);
       //FreeObjectList (table.lpTable);
       //table.lpTable = NULL;
       //lpcolumns     = FreeTableAndColumns (lpcolumns);
   }

    if (lpoptimizedb->m_nObjectType == OT_INDEX)
    {
        EnableWindow    (GetDlgItem (hwnd, IDC_STATIC_TABLE_NAME),  FALSE);
        EnableWindow    (GetDlgItem (hwnd, IDC_OPTIMIZEDB_SPECIFYTABLES),  FALSE);
        EnableWindow    (GetDlgItem (hwnd, IDC_OPTIMIZEDB_SPECIFYCOLUMS),  FALSE);
        EnableWindow    (GetDlgItem (hwnd, IDC_OPTIMIZEDB_COMPOSITE_ON_BASE_TABLE), FALSE);
        ShowWindow      (GetDlgItem (hwnd, IDC_OPTIMIZEDB_IDCOLUMS),       SW_HIDE);
        ShowWindow      (GetDlgItem (hwnd, IDC_OPTIMIZEDB_BUTTON_INDEX),   SW_HIDE);
        Button_SetCheck (GetDlgItem (hwnd, IDC_OPTIMIZEDB_COMPOSITE_ON_BASE_TABLE), FALSE);
        Button_SetCheck (GetDlgItem (hwnd, IDC_OPTIMIZEDB_CHECK_INDEX), TRUE);
        Button_SetCheck (GetDlgItem (hwnd, IDC_OPTIMIZEDB_SPECIFYTABLES), FALSE);
        EnableWindow    (GetDlgItem (hwnd, IDC_OPTIMIZEDB_CHECK_INDEX), FALSE);
        ShowWindow      (GetDlgItem (hwnd, IDC_OPTIMIZEDB_CHECK_INDEX),  SW_SHOW);
        ShowWindow      (GetDlgItem (hwnd, IDC_OPTIMIZEDB_STATIC_INDEX), SW_SHOW);
        ShowWindow      (GetDlgItem (hwnd, IDC_OPTIMIZEDB_EDIT_INDEX),   SW_SHOW);
        Edit_SetText    (GetDlgItem (hwnd, IDC_OPTIMIZEDB_EDIT_INDEX), lpoptimizedb->tchszDisplayIndexName);
    }
    else
    if (lpcolumns || lpoptimizedb->m_nObjectType == OT_TABLE)
    {
        if (lpoptimizedb->bComposite)
            EnableWindow (GetDlgItem (hwnd, IDC_OPTIMIZEDB_COMPOSITE_ON_BASE_TABLE),  FALSE);
        else
        {
            if (nUsageLimited != PRESELECTED_TABLE_AND_COLUMN)
            {
                if (SpecifiedColumns (lpcolumns))
                {
                    Button_SetCheck (GetDlgItem (hwnd, IDC_OPTIMIZEDB_COMPOSITE_ON_BASE_TABLE), FALSE);
                    EnableWindow (GetDlgItem (hwnd, IDC_OPTIMIZEDB_COMPOSITE_ON_BASE_TABLE),  FALSE);
                }
                else
                    EnableWindow (GetDlgItem (hwnd, IDC_OPTIMIZEDB_COMPOSITE_ON_BASE_TABLE),  TRUE);
            }
            else
                EnableWindow (GetDlgItem (hwnd, IDC_OPTIMIZEDB_COMPOSITE_ON_BASE_TABLE),  FALSE);
        }

        if (lpoptimizedb->m_nObjectType == OT_TABLE)
        {
            EnableWindow (GetDlgItem (hwnd, IDC_OPTIMIZEDB_CHECK_INDEX), FALSE);
            ShowWindow   (GetDlgItem (hwnd, IDC_OPTIMIZEDB_CHECK_INDEX),  SW_SHOW);
            ShowWindow   (GetDlgItem (hwnd, IDC_OPTIMIZEDB_STATIC_INDEX), SW_HIDE);
            ShowWindow   (GetDlgItem (hwnd, IDC_OPTIMIZEDB_EDIT_INDEX),   SW_HIDE);
            ShowWindow   (GetDlgItem (hwnd, IDC_OPTIMIZEDB_BUTTON_INDEX), SW_HIDE);
        }
    }
    else
    {
        EnableWindow (GetDlgItem (hwnd, IDC_OPTIMIZEDB_COMPOSITE_ON_BASE_TABLE),  FALSE);
    }
}


static BOOL SpecifiedColumns (LPTABLExCOLS lpTablexCol)
{
   LPTABLExCOLS ls = lpTablexCol;

   while (ls)
   {
       if (ls->lpCol)
           return TRUE;

       ls = ls->next;
   }
   return FALSE;
}

static void EnableControls (HWND hwnd, BOOL bEnable)
{
   int i = 0;

   while (IdList [i])
   {
       EnableWindow (GetDlgItem (hwnd, IdList [i]), bEnable);
       i++;
   }
}


