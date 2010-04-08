/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Project  : CA/OpenIngres Visual DBA
**
**    Source   : statdump.c
**
**    Dialog box for Generating the statdump command string
** History:
** 20-Jan-2000 (uk$so01)
**    Bug #100063
**    Eliminate the undesired compiler's warning
** 06-Mar_2000 (schph01)
**    Bug #100708
**    updated the new bRefuseTblWithDupName variable in the structure
**    for the new sub-dialog for selecting a table
** 04-Oct-2000 (schph01)
**    Bug 102815
**    move '-o' parameter before DatabaseName parameter, otherwise '-o'
**    is unrecognized, and add a space between the '-o' and the file name.
** 09-Oct-2000 (schph01)
**    bug #102868 the maximun length for the path and file name now is _MAX_PATH,
**    for options -zf and -o.
** 26-Apr-2001 (uk$so01)
**    SIR #102678
**    Support the composite histogram of base table and secondary index.
*/

#include "dll.h"
#include "dbadlg1.h"
#include "dlgres.h"
#include "catospin.h"
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

#define IDC_STATDUMP_PARAM           20420
#define IDC_STATDUMP_PARAMFILE       20421

static int IdList [] =
{
   IDC_STATDUMP_DIRECT        ,
   IDC_STATDUMP_DIRECTFILE    ,
   IDC_STATDUMP_WAIT          ,
   IDC_STATDUMP_DISPSYSCAT    ,
   IDC_STATDUMP_DELSYSCAT     ,
   IDC_STATDUMP_QUIETMODE     ,
   IDC_STATDUMP_PRECILEVEL    ,
   IDC_STATDUMP_PRECILEVELNUM ,
   IDC_STATDUMP_PRECILEVELSPIN,
   IDC_STATDUMP_SPECIFYTABLES ,
   IDC_STATDUMP_IDTABLES      ,
   IDC_STATDUMP_SPECIFYCOLUMS ,
   IDC_STATDUMP_IDCOLUMS      ,
   IDC_STATDUMP_IDDISPLAY     ,
   -1
};



//static AUDITDBTPARAMS table;
static LPTABLExCOLS   lpcolumns;
static DISPLAYPARAMS  display;
static LPCHECKEDOBJECTS pSpecifiedIndex;

#define STATDUMP_PRECILEVEL_MIN         1
#define STATDUMP_PRECILEVEL_MAX     65535

#define DefPreciLevel "10"



// Structure describing the numeric edit controls
static EDITMINMAX limits[] =
{
   IDC_STATDUMP_PRECILEVELNUM,   IDC_STATDUMP_PRECILEVELSPIN,
       STATDUMP_PRECILEVEL_MIN,
       STATDUMP_PRECILEVEL_MAX,

   -1  // terminator
};
extern int VDBA_ChooseIndex(HWND hwndParent, LPCTSTR lpszDatabase, LPCHECKEDOBJECTS* ppListObjects);





BOOL CALLBACK __export DlgDisplayStatisticsDlgProc
                       (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

static void    OnDestroy (HWND hwnd);
static void    OnCommand (HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
static BOOL    OnInitDialog (HWND hwnd, HWND hwndFocus, LPARAM lParam);
static void    OnSysColorChange (HWND hwnd);

static void    EnableDisableControls (HWND hwnd);
static void    InitializeControls (HWND hwnd);
static void    InitialiseSpinControls (HWND hwnd);
static void    EnableDisableOKButton (HWND hwnd);
static BOOL    FillStructureFromControls (HWND hwnd);
static BOOL    SpecifiedColumns (LPTABLExCOLS lpTablexCol);
static void    SetDefaultMessageOption (HWND hwnd);
static void    EnableControls (HWND hwnd, BOOL bEnable);

//
// These extern functions are implemented in the file: optimizedb.c
extern LPOBJECTLIST FindStringInListObject (LPOBJECTLIST lpList, char* Element);
extern LPTABLExCOLS FindStringInListTableAndColumns (LPTABLExCOLS lpTablexCol, char* aString);
extern LPTABLExCOLS RmoveElement (LPTABLExCOLS lpTablexCol, char* aTable);
extern LPTABLExCOLS AddElement   (LPTABLExCOLS lpTablexCol, char* aTable);
extern LPTABLExCOLS FreeTableAndColumns (LPTABLExCOLS lpTablexCol);
extern LPTABLExCOLS TABLExCOLUMNS_Copy (LPTABLExCOLS first);



int WINAPI __export DlgDisplayStatistics (HWND hwndOwner, LPSTATDUMPPARAMS lpstatdump)
/*
// Function:
// Shows the Refresh dialog.
//
// Paramters:
//     1) hwndOwner:   Handle of the parent window for the dialog.
//     1) lpstatdump:   Points to structure containing information used to 
//                     initialise the dialog. 
//
// Returns:
//     TRUE if successful.
//
*/
{
   FARPROC lpProc;
   int     retVal;
   
   if (!IsWindow (hwndOwner) || !lpstatdump)
   {
       ASSERT(NULL);
       return FALSE;
   }

   lpProc = MakeProcInstance ((FARPROC) DlgDisplayStatisticsDlgProc, hInst);
   retVal = VdbaDialogBoxParam
       (hResource,
        MAKEINTRESOURCE (IDD_STATDUMP),
        hwndOwner,
        (DLGPROC) lpProc,
        (LPARAM)  lpstatdump
       );

   FreeProcInstance (lpProc);
   return (retVal);
}


BOOL CALLBACK __export DlgDisplayStatisticsDlgProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
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

   LPSTATDUMPPARAMS lpstatdump  = (LPSTATDUMPPARAMS)lParam;
   char szFormat [100];
   char szTitle  [MAX_TITLEBAR_LEN];

   pSpecifiedIndex = NULL;
   if (!AllocDlgProp (hwnd, lpstatdump))
       return FALSE;
   SpinGetVersion();

//   ZEROINIT (table);
//   table.lpTable   = NULL;
   lpcolumns       = NULL;

   LoadString (hResource, (UINT)IDS_T_STATDUMP, szFormat, sizeof (szFormat));
   wsprintf (szTitle, szFormat,
       GetVirtNodeName ( GetCurMdiNodeHandle ()),
       lpstatdump->DBName);

   if (lpstatdump->bStartSinceTable)
   {
     char sztemp[MAXOBJECTNAME];
     // Title
     wsprintf( sztemp ," , table %s",lpstatdump->szDisplayTableName);
     lstrcat(szTitle,sztemp);
     // Enable Button
     Button_SetCheck (GetDlgItem (hwnd, IDC_STATDUMP_SPECIFYTABLES),BST_CHECKED);
     EnableWindow    (GetDlgItem (hwnd, IDC_STATDUMP_SPECIFYTABLES),FALSE);
     ShowWindow      (GetDlgItem (hwnd, IDC_STATDUMP_IDTABLES),SW_HIDE);
     ShowWindow      (GetDlgItem (hwnd, IDC_EDIT_TABLE_NAME)  ,SW_SHOW);
     ShowWindow      (GetDlgItem (hwnd, IDC_STATIC_TABLE_NAME),SW_SHOW);
     SetWindowText   (GetDlgItem (hwnd, IDC_EDIT_TABLE_NAME)  ,lpstatdump->szDisplayTableName );
     EnableWindow    (GetDlgItem (hwnd, IDC_EDIT_TABLE_NAME),FALSE);

     // Add table  into TABLExCOLS
     lpcolumns = AddElement (lpcolumns, lpstatdump->szCurrentTableName);
//     x_strcpy (table.DBName,  lpstatdump->DBName);
   }
   else
   if (lpstatdump->bStartSinceIndex)
   {
     char sztemp[MAXOBJECTNAME];
     // Title
     wsprintf( sztemp ," , Index %s",lpstatdump->tchszDisplayIndexName);
     lstrcat(szTitle, sztemp);
     SetWindowText   (GetDlgItem (hwnd, IDC_EDIT_TABLE_NAME)  ,lpstatdump->szDisplayTableName );
   }

   SetWindowText (hwnd, szTitle);

   lpHelpStack = StackObject_PUSH (lpHelpStack, StackObject_INIT ((UINT)IDD_STATDUMP));

   InitialiseSpinControls (hwnd);
   InitializeControls     (hwnd);
   EnableDisableControls  (hwnd);

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
   LPSTATDUMPPARAMS lpstatdump = GetDlgProp (hwnd);

   switch (id)
   {
       case IDOK:
           if (lpstatdump->bStartSinceTable)
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

       case IDC_STATDUMP_IDDISPLAY:
           ZEROINIT (display);
           DlgDisplay (hwnd, &display);
           break;

       case IDC_STATDUMP_IDTABLES:
           {
               int   iret;
               char* aString;
               char  szTable [MAXOBJECTNAME];
               LPOBJECTLIST ls, pObj;
               LPTABLExCOLS lc, ltb;

                AUDITDBTPARAMS tempTable;
                memset (&tempTable, 0, sizeof(tempTable));
               lstrcpy (tempTable.DBName,  lpstatdump->DBName);
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
                if (lpcolumns)
                    Button_SetCheck (GetDlgItem (hwnd, IDC_STATDUMP_SPECIFYTABLES), TRUE);
                else
                {
                    Button_SetCheck (GetDlgItem (hwnd, IDC_STATDUMP_SPECIFYTABLES), FALSE);
                    EnableWindow    (GetDlgItem (hwnd, IDC_STATDUMP_IDTABLES), FALSE);
                }

               EnableDisableOKButton  (hwnd);
               EnableDisableControls  (hwnd);
           }
           break;


       case IDC_STATDUMP_IDCOLUMS:
           {
               int  iret;
               //
               // Show the colums of a table
               // Check the colums that have been checked
               //
               SPECIFYCOLPARAMS specifycolumns;
               ZEROINIT (specifycolumns);
               x_strcpy (specifycolumns.DBName, lpstatdump->DBName);
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
                   Button_SetCheck (GetDlgItem (hwnd, IDC_STATDUMP_SPECIFYCOLUMS),  TRUE);
               else
                   Button_SetCheck (GetDlgItem (hwnd, IDC_STATDUMP_SPECIFYCOLUMS),  FALSE);
               EnableDisableControls (hwnd);
               EnableDisableOKButton (hwnd);
           }
           break;

       case IDC_STATDUMP_BUTTON_INDEX:
           {
               VDBA_ChooseIndex(hwnd, lpstatdump->DBName, &pSpecifiedIndex);
               if (pSpecifiedIndex)
                   Button_SetCheck (GetDlgItem (hwnd, IDC_STATDUMP_CHECK_INDEX), TRUE);
               else
                   Button_SetCheck (GetDlgItem (hwnd, IDC_STATDUMP_CHECK_INDEX), FALSE);
           }
           break;

       case IDC_STATDUMP_PRECILEVELNUM :
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
           
       case IDC_STATDUMP_PRECILEVELSPIN:  
           ProcessSpinControl (hwndCtl, codeNotify, limits);
           break;

       case IDC_STATDUMP_DIRECT:
       case IDC_STATDUMP_SPECIFYTABLES:
       case IDC_STATDUMP_SPECIFYCOLUMS:
       case IDC_STATDUMP_CHECK_INDEX:
           EnableDisableControls (hwnd);
           break;

       case IDC_STATDUMP_PARAM :
           EnableDisableControls (hwnd);
           if (Button_GetCheck   (hwndCtl))
               EnableControls    (hwnd, FALSE);
           else
               EnableControls    (hwnd, TRUE);
           break;

       case IDC_STATDUMP_PARAMFILE:
           if (codeNotify == EN_CHANGE)
           {
               if (Edit_GetTextLength (hwndCtl) == 0)
                   EnableWindow (GetDlgItem (hwnd, IDOK), FALSE);
               else
                   EnableWindow (GetDlgItem (hwnd, IDOK), TRUE);
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
   HWND hwndPreciLevel = GetDlgItem (hwnd, IDC_STATDUMP_PRECILEVELNUM ); 

   SubclassAllNumericEditControls (hwnd, EC_SUBCLASS);
   
   Edit_LimitText (hwndPreciLevel, 10);

   Edit_SetText   (hwndPreciLevel, DefPreciLevel);
   LimitNumericEditControls (hwnd);
}



static void InitialiseSpinControls (HWND hwnd)
{
   DWORD dwMin, dwMax;

   GetEditCtrlMinMaxValue (hwnd, GetDlgItem (hwnd, IDC_STATDUMP_PRECILEVELNUM), limits, &dwMin, &dwMax);
   SpinRangeSet (GetDlgItem (hwnd, IDC_STATDUMP_PRECILEVELSPIN), dwMin, dwMax);
}


static void EnableDisableOKButton (HWND hwnd)
{

}

static BOOL FillStructureFromControls (HWND hwnd)
{

   char szGreatBuffer   [3000];
   char szBuffer        [_MAX_PATH];
   char buftemp         [200];

   HWND hwnd_zfParamFile = GetDlgItem (hwnd, IDC_STATDUMP_PARAMFILE);
   HWND hwnd_oFileName   = GetDlgItem (hwnd, IDC_STATDUMP_DIRECTFILE);
   HWND hwnd_level       = GetDlgItem (hwnd, IDC_STATDUMP_PRECILEVELNUM);
   
   LPUCHAR vnodeName = GetVirtNodeName (GetCurMdiNodeHandle ());
   LPSTATDUMPPARAMS  lpstatdump  = GetDlgProp (hwnd);
   LPTABLExCOLS columnslist = lpcolumns;
	char szgateway[200];
	BOOL bHasGWSuffix = GetGWClassNameFromString(vnodeName, szgateway);


   ZEROINIT (szGreatBuffer);
   ZEROINIT (szBuffer);

   x_strcpy (szGreatBuffer, "statdump ");
   
   //
   // Ingres 2.6 only:
   // Generate -zcpk 
   //
   if (Button_GetCheck (GetDlgItem (hwnd, IDC_STATDUMP_COMPOSITE_ON_BASE_TABLE)))
       x_strcat (szGreatBuffer, " -zcpk");

   //
   // Generate -zffilename
   //
   if (Button_GetCheck (GetDlgItem (hwnd, IDC_STATDUMP_PARAM)))
   {
       if (Edit_GetTextLength (hwnd_zfParamFile) > 0)
       {
           ZEROINIT (szBuffer);
           Edit_GetText (hwnd_zfParamFile, szBuffer, sizeof (szBuffer));
           NoLeadingEndingSpace (szBuffer);
           x_strcat (szGreatBuffer, " -zf");
           x_strcat (szGreatBuffer, szBuffer);
       }
   }
   else
   {
       //
       // Generate SQL flag
       //
       if (x_strlen (display.szString) > 0)
           x_strcat (szGreatBuffer, display.szString);

       if (Button_GetCheck (GetDlgItem (hwnd, IDC_STATDUMP_WAIT)))
           x_strcat (szGreatBuffer, " +w");

       //
       // Generate the -uusername
       //
       if (DBAGetSessionUser(vnodeName, buftemp))
       {
           x_strcat (szGreatBuffer, " -u");
           suppspace(buftemp);
           x_strcat (szGreatBuffer, buftemp);
       }

       //
       // I don't know how to generate -xk for SQL option flags
       // TO BE FINISHED

       //
       // Generate -Z flags
       // 

       //
       // Generate -zc
       //
       if (Button_GetCheck (GetDlgItem (hwnd, IDC_STATDUMP_DISPSYSCAT)))
           x_strcat (szGreatBuffer, " -zc");

       //
       // Generate -zdl
       //
       if (Button_GetCheck (GetDlgItem (hwnd, IDC_STATDUMP_DELSYSCAT)))
       {
           x_strcat (szGreatBuffer, " -zdl");
           x_strcat (szGreatBuffer, " +U");
       }
       //
       // Generate -zn#
       //
       if (Button_GetCheck (GetDlgItem (hwnd, IDC_STATDUMP_PRECILEVEL)))
       {
           if (Edit_GetTextLength (hwnd_level) > 0)
           {
               ZEROINIT (szBuffer);
               Edit_GetText (hwnd_level, szBuffer, sizeof (szBuffer));
               x_strcat (szGreatBuffer, " -zn");
               x_strcat (szGreatBuffer, szBuffer);
           }
       }
       //
       // Generate -zq
       //

       if (Button_GetCheck (GetDlgItem (hwnd, IDC_STATDUMP_QUIETMODE)))
       {
       x_strcat (szGreatBuffer, " -zq");
       }

       //
       // Generate -ofilename
       //
       if (Button_GetCheck (GetDlgItem (hwnd, IDC_STATDUMP_DIRECT)))
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

       x_strcat (szGreatBuffer, " " );
       x_strcat (szGreatBuffer, lpstatdump->DBName );
       if (bHasGWSuffix){
           x_strcat(szGreatBuffer,"/");
           x_strcat(szGreatBuffer,szgateway);
       }

       //
       // Generate Table and columns (-rtablename {-acolumnname}}
       //
       if (Button_GetCheck (GetDlgItem (hwnd, IDC_STATDUMP_SPECIFYTABLES)))
       {
           while (columnslist)
           {   LPOBJECTLIST ls = columnslist->lpCol;
               char* col;
               x_strcat (szGreatBuffer, " -r");
               x_strcat (szGreatBuffer, StringWithoutOwner(columnslist->TableName));
               if (Button_GetCheck (GetDlgItem (hwnd, IDC_STATDUMP_SPECIFYCOLUMS)))
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
       if (lpstatdump->m_nObjectType == OT_INDEX)
       {
           x_strcat (szGreatBuffer, " -r");
           x_strcat (szGreatBuffer, StringWithoutOwner(lpstatdump->tchszCurrentIndexName));
       }
       else
       if (lpstatdump->m_nObjectType == OT_DATABASE && 
           Button_GetCheck (GetDlgItem (hwnd, IDC_STATDUMP_CHECK_INDEX)) &&
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

   //
   // Generate the -uusername
   //
   //if (DBAGetSessionUser(vnodeName, buftemp)) {
   //   x_strcat (szGreatBuffer, " -u");
   //   suppspace(buftemp);
   //   x_strcat (szGreatBuffer, buftemp);
   //}

   wsprintf(buftemp,
       ResourceString ((UINT)IDS_T_RMCMD_STATDUMP),
       // display statistics on database %s::%s,
       vnodeName,
       lpstatdump->DBName);
   execrmcmd(vnodeName,szGreatBuffer,buftemp);

   return TRUE;
}


static void EnableDisableControls (HWND hwnd)
{
   LPSTATDUMPPARAMS  lpstatdump  = GetDlgProp (hwnd);

   if (Button_GetCheck (GetDlgItem (hwnd, IDC_STATDUMP_DIRECT)))
   {
       EnableWindow (GetDlgItem (hwnd, IDC_STATDUMP_DIRECTFILE), TRUE);
   }
   else
   {
       EnableWindow (GetDlgItem (hwnd, IDC_STATDUMP_DIRECTFILE), FALSE);
   }

   if (Button_GetCheck (GetDlgItem (hwnd, IDC_STATDUMP_PARAM)))
   {
       EnableWindow (GetDlgItem (hwnd, IDC_STATDUMP_PARAMFILE), TRUE);
   }
   else
   {
       EnableWindow (GetDlgItem (hwnd, IDC_STATDUMP_PARAMFILE), FALSE);
   }

   if (Button_GetCheck (GetDlgItem (hwnd, IDC_STATDUMP_SPECIFYTABLES)))
   {
       EnableWindow (GetDlgItem (hwnd, IDC_STATDUMP_IDTABLES), TRUE);
   }
   else
   {
       EnableWindow (GetDlgItem (hwnd, IDC_STATDUMP_IDTABLES), FALSE);
//       FreeObjectList (table.lpTable);
//       table.lpTable = NULL;
       lpcolumns     = FreeTableAndColumns (lpcolumns);
   }

   if (lpcolumns)
   {
       EnableWindow (GetDlgItem (hwnd, IDC_STATDUMP_SPECIFYCOLUMS),  TRUE);
       EnableWindow (GetDlgItem (hwnd, IDC_STATDUMP_IDCOLUMS), TRUE);
   }
   else
   {
       Button_SetCheck (GetDlgItem (hwnd, IDC_STATDUMP_SPECIFYCOLUMS), FALSE);
       EnableWindow (GetDlgItem (hwnd, IDC_STATDUMP_SPECIFYCOLUMS),  FALSE);
       EnableWindow (GetDlgItem (hwnd, IDC_STATDUMP_IDCOLUMS), FALSE);
   }

   if (Button_GetCheck (GetDlgItem (hwnd, IDC_STATDUMP_SPECIFYCOLUMS)))
       EnableWindow (GetDlgItem (hwnd, IDC_STATDUMP_IDCOLUMS), TRUE);
   else
       EnableWindow (GetDlgItem (hwnd, IDC_STATDUMP_IDCOLUMS), FALSE);

   if (Button_GetCheck (GetDlgItem (hwnd, IDC_STATDUMP_CHECK_INDEX)) && 
      lpstatdump && 
      lpstatdump->m_nObjectType != OT_INDEX)
   {
      EnableWindow(GetDlgItem (hwnd, IDC_STATDUMP_BUTTON_INDEX), TRUE);
   }
   else
   {
      EnableWindow(GetDlgItem (hwnd, IDC_STATDUMP_BUTTON_INDEX), FALSE);
   }


    if (lpstatdump && lpstatdump->m_nObjectType == OT_INDEX)
    {
        EnableWindow    (GetDlgItem (hwnd, IDC_STATDUMP_COMPOSITE_ON_BASE_TABLE), FALSE);
        EnableWindow    (GetDlgItem (hwnd, IDC_STATDUMP_STATIC_INDEX),   TRUE);
        EnableWindow    (GetDlgItem (hwnd, IDC_STATDUMP_SPECIFYTABLES),  FALSE);
        EnableWindow    (GetDlgItem (hwnd, IDC_STATDUMP_SPECIFYCOLUMS),  FALSE);
        ShowWindow      (GetDlgItem (hwnd, IDC_STATDUMP_IDCOLUMS),   SW_HIDE);
        ShowWindow      (GetDlgItem (hwnd, IDC_STATDUMP_IDTABLES),   SW_HIDE);
        Button_SetCheck (GetDlgItem (hwnd, IDC_STATDUMP_CHECK_INDEX), TRUE);
        EnableWindow    (GetDlgItem (hwnd, IDC_STATDUMP_CHECK_INDEX), FALSE);
        ShowWindow      (GetDlgItem (hwnd, IDC_STATDUMP_BUTTON_INDEX), SW_HIDE);
        ShowWindow      (GetDlgItem (hwnd, IDC_STATDUMP_STATIC_INDEX), SW_SHOW);
        ShowWindow      (GetDlgItem (hwnd, IDC_STATDUMP_EDIT_INDEX),   SW_SHOW);
        Edit_SetText    (GetDlgItem (hwnd, IDC_STATDUMP_EDIT_INDEX), lpstatdump->tchszDisplayIndexName);
    }
    else
    if (lpcolumns || lpstatdump->m_nObjectType == OT_TABLE)
    {
        EnableWindow    (GetDlgItem (hwnd, IDC_STATDUMP_COMPOSITE_ON_BASE_TABLE),  TRUE);
        Button_SetCheck (GetDlgItem (hwnd, IDC_STATDUMP_COMPOSITE_ON_BASE_TABLE),  TRUE);
        if (lpstatdump->m_nObjectType == OT_TABLE)
        {
            EnableWindow (GetDlgItem (hwnd, IDC_STATDUMP_CHECK_INDEX), FALSE);
            ShowWindow   (GetDlgItem (hwnd, IDC_STATDUMP_CHECK_INDEX),  SW_SHOW);
            ShowWindow   (GetDlgItem (hwnd, IDC_STATDUMP_STATIC_INDEX), SW_HIDE);
            ShowWindow   (GetDlgItem (hwnd, IDC_STATDUMP_EDIT_INDEX),   SW_HIDE);
            ShowWindow   (GetDlgItem (hwnd, IDC_STATDUMP_BUTTON_INDEX), SW_HIDE);
        }
        else
        {
            EnableWindow    (GetDlgItem (hwnd, IDC_STATDUMP_COMPOSITE_ON_BASE_TABLE),  FALSE);
        }
    }
    else
    {
        EnableWindow (GetDlgItem (hwnd, IDC_STATDUMP_COMPOSITE_ON_BASE_TABLE),  FALSE);
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

