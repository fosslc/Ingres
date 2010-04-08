/********************************************************************
//
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
//
//    Project  : CA/OpenIngres Visual DBA
//
//    Source   : alterdb.c
//
//    Alter database.
//
**  26-Apr-2001 (noifr01)
**    (rmcmd counterpart of SIR 103299) max lenght of rmcmd command lines
**    has been increased to 800 against 2.6. now use a new constant for
**    remote command buffer sizes, because the other "constant", in fact a
**    #define, has been redefined and depends on the result of a function 
**    call (something that doesn't compile if you put it as the size of a char[]
**    array. The new constant, used for buffer sizes only, is the biggest of 
**    the possible values)
**  23-Nov-2005 (fanra01)
**      Bug 115560
**      Yet another sticky plaster.
**      Rearrange the alterdb dialog to reflect the current command
**      processing.  Alterdb only accepts single command line
**      parameters at present.
********************************************************************/

#include <stdlib.h>
#include "dll.h"
#include "subedit.h"
#include "dbadlg1.h"
#include "dlgres.h"
#include "getdata.h"
#include "catospin.h"
#include "msghandl.h"
#include "domdata.h"

#define BLOCK_SIZE_NUMBER 5
#define BLOCK_MIN_JOURNAL 32
static char* sizeTypes[] =
{
   " 4096", " 8192", "16384", "32768", "65536",
};

static EDITMINMAX limits[] =
{
   IDC_ALTERDB_BLOCK,IDC_ALTERDB_BLOCKSPIN,ALTERDB_MIN_BLOCK,ALTERDB_MAX_BLOCK,
   IDC_ALTERDB_INITIAL,IDC_ALTERDB_INITIALSPIN,ALTERDB_MIN_INITIAL,ALTERDB_MAX_INITIAL,
   -1// terminator
};

BOOL CALLBACK __export DlgAlterDBDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

static void OnDestroy(HWND hwnd);
static void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
static BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);
static void OnSysColorChange(HWND hwnd);

static BOOL OccupySizeControl (HWND hwnd);
static void InitialiseEditControls (HWND hwnd);
static void InitialiseSpinControls (HWND hwnd);

static BOOL SendCommand(HWND hwnd);

BOOL WINAPI __export DlgAlterDB (HWND hwndOwner, LPALTERDB lpalterdb)
/*
// Function:
// Shows the Alter Database dialog.
//
// Paramters:
//     hwndOwner:  - Handle of the parent window for the dialog.
//     lpalterdb:  - Points to structure containing information used to 
//                   initialise the dialog. The dialog uses the same
//                   structure to return the result of the dialog.
//
// Returns:
// TRUE if successful.
*/
{
   int     RetVal;

   if (!IsWindow(hwndOwner) || !lpalterdb)
   {
     ASSERT (NULL);
     return FALSE;
   }

   // Ensure the defaults passed in are valid

   if (lpalterdb->nBlock > ALTERDB_MAX_BLOCK || lpalterdb->nBlock < ALTERDB_MIN_BLOCK)
   lpalterdb->nBlock = ALTERDB_MIN_BLOCK;

   if (lpalterdb->nInitial > ALTERDB_MAX_INITIAL || lpalterdb->nInitial < ALTERDB_MIN_INITIAL)
   lpalterdb->nInitial = ALTERDB_MIN_INITIAL;

#if 0
   if (lpalterdb->nBlockSize > ALTERDB_SIZE_??????)
       lpalterdb->nBlockSize = 0;
#endif

   RetVal = VdbaDialogBoxParam(hResource,
                           MAKEINTRESOURCE(IDD_ALTERDB),
                           hwndOwner,
                           DlgAlterDBDlgProc,
                           (LPARAM)lpalterdb);

   return (RetVal);
}


BOOL CALLBACK __export DlgAlterDBDlgProc
                       (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
   switch (message)
   {
     HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
     HANDLE_MSG(hwnd, WM_DESTROY, OnDestroy);

     case WM_INITDIALOG:
       return HANDLE_WM_INITDIALOG(hwnd, wParam, lParam, OnInitDialog);

     case LM_GETEDITMINMAX:
     {
       LPEDITMINMAX lpdesc = (LPEDITMINMAX) lParam;
       HWND hwndCtl = (HWND) wParam;

       ASSERT(IsWindow(hwndCtl));
       ASSERT(lpdesc);

       return GetEditCtrlMinMaxValue(hwnd, hwndCtl, limits, &lpdesc->dwMin, &lpdesc->dwMax);
     }

     case LM_GETEDITSPINCTRL:
     {
       HWND hwndEdit = (HWND) wParam;
       HWND FAR * lphwnd = (HWND FAR *)lParam;
       *lphwnd = GetEditSpinControl(hwndEdit, limits);
       break;
     }

     default:
       return FALSE;
   }
   return TRUE;
}


static BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
   LPALTERDB lpdb = (LPALTERDB)lParam;
   char Title[200];
   char Number[20];
   /*
   ** add handles for edit controls to set state.
   */
   HWND hwndSize    = GetDlgItem(hwnd, IDC_ALTERDB_BLOCKSIZE);
   HWND hwndBlock   = GetDlgItem(hwnd, IDC_ALTERDB_BLOCK);
   HWND hwndInitial = GetDlgItem(hwnd, IDC_ALTERDB_INITIAL);
   
   ZEROINIT (Title);
   
   //Fill the windows title bar
   GetWindowText(hwnd,Title,GetWindowTextLength(hwnd)+1);
   x_strcat(Title, " ");
   x_strcat(Title, GetVirtNodeName ( GetCurMdiNodeHandle ()));
   x_strcat(Title, "::");
   x_strcat(Title,lpdb->DBName);

   SetWindowText(hwnd,Title);

   if (!AllocDlgProp(hwnd, lpdb))
   return FALSE;

   if (!OccupySizeControl(hwnd))
   {
     ASSERT(NULL);
     EndDialog (hwnd, FALSE);
     return TRUE;
   }
   // The Minimun value for Block is 32
   lpdb->nBlock=BLOCK_MIN_JOURNAL;
   x_strcpy(Number,"32");
   Edit_SetText(hwndBlock, Number);


   InitialiseEditControls(hwnd);

   /*
   ** Set the radio buttons according to flags.
   ** Add new flags for target journal block count, journal block size
   ** and initial block count.
   ** Enable edit control according to radio button status.
   */
   CheckDlgButton( hwnd, IDC_TARGET_JNL_BLOCKS, lpdb->bTgtBlock );
   EnableWindow( hwndBlock, (lpdb->bTgtBlock) ? TRUE : FALSE );
   
   CheckDlgButton(hwnd, IDC_JOURNAL_BLOCK_SIZE, lpdb->bJnlBlockSize);
   EnableWindow( hwndSize, (lpdb->bJnlBlockSize) ? TRUE : FALSE );
   
   CheckDlgButton(hwnd, IDC_INITIAL_JNL_BLOCKS, lpdb->bInitBlock);
   EnableWindow( hwndInitial, (lpdb->bInitBlock) ? TRUE : FALSE );

   CheckDlgButton(hwnd, IDC_ALTERDB_DJOURNALING, lpdb->bDisableJrnl);
   CheckDlgButton(hwnd, IDC_ALTERDB_DELETEOLDCKP, lpdb->bDelOldestCkPt);
   CheckDlgButton(hwnd, IDC_ALTERDB_VERBOSE, lpdb->bVerbose);
   lpHelpStack = StackObject_PUSH (lpHelpStack, StackObject_INIT ((UINT)IDD_ALTERDB));

   richCenterDialog(hwnd);
   return TRUE;
}


static void OnDestroy(HWND hwnd)
{
   SubclassAllNumericEditControls(hwnd, EC_RESETSUBCLASS);
   DeallocDlgProp(hwnd);
   lpHelpStack = StackObject_POP (lpHelpStack);
}


static void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    /*
    ** Move dialog control handles to function scope for reuse.
    */
    HWND hwndSize    = GetDlgItem(hwnd, IDC_ALTERDB_BLOCKSIZE);
    HWND hwndBlock   = GetDlgItem(hwnd, IDC_ALTERDB_BLOCK);
    HWND hwndInitial = GetDlgItem(hwnd, IDC_ALTERDB_INITIAL);
    LPALTERDB lpdb   = GetDlgProp(hwnd);

    switch (id)
    {
        case IDOK:
        {
            int nIdx;
            char szNumber[20];

            if (!VerifyAllNumericEditControls (hwnd, TRUE, TRUE))
                break;

            /*
            ** Process target journal blocks only if flag is enabled.
            */
            if ((lpdb->bTgtBlock =
                IsDlgButtonChecked(hwnd, IDC_TARGET_JNL_BLOCKS)))
            {
                ZEROINIT (szNumber);
                Edit_GetText(hwndBlock, szNumber, sizeof(szNumber));
                lpdb->nBlock = atol(szNumber);
       
                if (lpdb->nBlock < BLOCK_MIN_JOURNAL ||
                    lpdb->nBlock > ALTERDB_MAX_BLOCK )
                {
                    ErrorMessage ((UINT)IDS_E_NUMBER_RANGE, RES_ERR);  
                    SetFocus(hwndBlock);
                    break;
                }
            }

            /*
            ** Process initial journal blocks only if flag is enabled.
            */
            if ((lpdb->bInitBlock =
                IsDlgButtonChecked(hwnd, IDC_INITIAL_JNL_BLOCKS)))
            {
                ZEROINIT (szNumber);
                Edit_GetText(hwndInitial, szNumber, sizeof(szNumber));
                lpdb->nInitial = atol(szNumber);
   
                if (lpdb->nInitial < 0 || lpdb->nInitial > lpdb->nBlock )
                {
                    ErrorMessage ((UINT)IDS_E_INITIAL_NUMBER, RES_ERR);  
                    SetFocus(hwndInitial);
                    break;
                }
            }

            /*
            ** Process journal block size only if flag is enabled.
            */
            if ((lpdb->bJnlBlockSize =
                IsDlgButtonChecked(hwnd, IDC_JOURNAL_BLOCK_SIZE)))
            {
                ZEROINIT (szNumber);
                nIdx = ComboBox_GetCurSel(hwndSize);
                ComboBox_GetLBText(hwndSize, nIdx, szNumber);
                lpdb->nBlockSize = atol(szNumber);
            }
   
            lpdb->bDisableJrnl = IsDlgButtonChecked(hwnd, IDC_ALTERDB_DJOURNALING);
            lpdb->bDelOldestCkPt = IsDlgButtonChecked(hwnd, IDC_ALTERDB_DELETEOLDCKP);
            lpdb->bVerbose = IsDlgButtonChecked(hwnd, IDC_ALTERDB_VERBOSE);

            if ( SendCommand( hwnd) == TRUE)
                EndDialog(hwnd, 1);

            break;
        }

        case IDCANCEL:
        {
            EndDialog(hwnd, FALSE);
            break;
        }

        case IDC_ALTERDB_BLOCK:
        case IDC_ALTERDB_INITIAL:
        {
            switch (codeNotify)
            {
                case EN_CHANGE:
                {
                    int nCount;

                    if (!IsWindowVisible(hwnd))
                        break;

                    // Test to see if the control is empty. If so then
                    // break out.  It becomes tiresome to edit
                    // if you delete all characters and an error box pops up.

                    nCount = Edit_GetTextLength(hwndCtl);
                    if (nCount == 0)
                        break;

                    VerifyAllNumericEditControls(hwnd, TRUE, TRUE);
                        break;
                }

                case EN_KILLFOCUS:
                {
                    HWND hwndNew = GetFocus();
                    int nNewCtl = GetDlgCtrlID(hwndNew);

                    if (nNewCtl == IDCANCEL || nNewCtl == IDOK
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
        }

        case IDC_ALTERDB_BLOCKSPIN:
        case IDC_ALTERDB_INITIALSPIN:
        {
            ProcessSpinControl(hwndCtl, codeNotify, limits);
            break;
        }
        /*
        ** Add event handling for the radio buttons.  Radio buttons for the
        ** edit control determine if the control is enabled.
        */
        case IDC_TARGET_JNL_BLOCKS:
        case IDC_JOURNAL_BLOCK_SIZE:
        case IDC_INITIAL_JNL_BLOCKS:
        case IDC_ALTERDB_DJOURNALING:
        case IDC_ALTERDB_DELETEOLDCKP:
            /*
            ** Get button status
            */
            lpdb->bTgtBlock = IsDlgButtonChecked(hwnd, IDC_TARGET_JNL_BLOCKS);
            lpdb->bJnlBlockSize = IsDlgButtonChecked(hwnd, IDC_JOURNAL_BLOCK_SIZE);
            lpdb->bInitBlock = IsDlgButtonChecked(hwnd, IDC_INITIAL_JNL_BLOCKS);
            lpdb->bDisableJrnl = IsDlgButtonChecked(hwnd, IDC_ALTERDB_DJOURNALING);
            lpdb->bDelOldestCkPt = IsDlgButtonChecked(hwnd, IDC_ALTERDB_DELETEOLDCKP);
            /*
            ** Enable edit controls according to radio button status.
            */
            EnableWindow( hwndBlock, (lpdb->bTgtBlock) ? TRUE : FALSE );
            EnableWindow( hwndSize, (lpdb->bJnlBlockSize) ? TRUE : FALSE );
            EnableWindow( hwndInitial, (lpdb->bInitBlock) ? TRUE : FALSE );
            break;
    }
}


static void OnSysColorChange(HWND hwnd)
{
#ifdef _USE_CTL3D
   Ctl3dColorChange();
#endif
}


static BOOL OccupySizeControl (HWND hwnd)
/*
// Function:
// Fills the size combobox box with the block sizes.
//
// Parameters:
//     hwnd- Handle to the dialog window.
//
// Returns:
// TRUE if successful.
*/
{
   int  i;
   HWND hwndCtl = GetDlgItem (hwnd, IDC_ALTERDB_BLOCKSIZE);

   for (i = 0; i < BLOCK_SIZE_NUMBER; i++)
       ComboBox_AddString (hwndCtl, sizeTypes [i]);
   return TRUE;
}



static void InitialiseEditControls (HWND hwnd)
{
   char szNumber[20];
   LPALTERDB lpdb = GetDlgProp(hwnd);

   SubclassAllNumericEditControls(hwnd, EC_SUBCLASS);
   LimitNumericEditControls(hwnd);

   //my_itoa(lpdb->nBlock, szNumber, 10);
   my_dwtoa(lpdb->nBlock, szNumber, 10);
   SetDlgItemText(hwnd, IDC_ALTERDB_BLOCK, szNumber);

   //my_itoa(lpdb->nInitial, szNumber, 10);
   my_dwtoa(lpdb->nInitial, szNumber, 10);
   SetDlgItemText(hwnd, IDC_ALTERDB_INITIAL, szNumber);
}


static void InitialiseSpinControls (HWND hwnd)
{
   DWORD dwMin, dwMax;
   LPALTERDB lpdb = GetDlgProp(hwnd);

   GetEditCtrlMinMaxValue(hwnd, GetDlgItem(hwnd, IDC_ALTERDB_BLOCK), limits, &dwMin, &dwMax);
   SpinRangeSet(GetDlgItem(hwnd, IDC_ALTERDB_BLOCKSPIN), dwMin, dwMax);

   GetEditCtrlMinMaxValue(hwnd, GetDlgItem(hwnd, IDC_ALTERDB_INITIAL), limits, &dwMin, &dwMax);
   SpinRangeSet(GetDlgItem(hwnd, IDC_ALTERDB_INITIALSPIN), dwMin, dwMax);

   SpinPositionSet(GetDlgItem(hwnd, IDC_ALTERDB_BLOCKSPIN), lpdb->nBlock);
   SpinPositionSet(GetDlgItem(hwnd, IDC_ALTERDB_INITIALSPIN), lpdb->nInitial);
}



// create and execute remote command
static BOOL SendCommand(HWND hwnd)
{
    LPALTERDB alterdb = GetDlgProp(hwnd);
    UCHAR buf2[MAXOBJECTNAME*3];
    UCHAR buf[MAX_RMCMD_BUFSIZE];
    LPUCHAR   vnodeName=GetVirtNodeName(GetCurMdiNodeHandle ());   


    x_strcpy(buf,"alterdb ");
    x_strcat(buf,alterdb->DBName);
    x_strcat(buf," ");

    /*
    ** Add command line parameters only if the flags indicate that they
    ** are enabled.
    */
    if (alterdb->bTgtBlock)
    {
        memset(buf2, 0, sizeof(buf2));
        wsprintf(buf2,"-target_jnl_blocks=%d ",alterdb->nBlock);
        x_strcat(buf,buf2);
    }
    
    if ((alterdb->bJnlBlockSize) && (alterdb->nBlockSize != 0))
    {
        memset(buf2, 0, sizeof(buf2));
        wsprintf(buf2,"-jnl_block_size=%ld ",alterdb->nBlockSize);
        x_strcat(buf,buf2);
    }

    if (alterdb->bInitBlock)
    {
        memset(buf2, 0, sizeof(buf2));
        wsprintf(buf2,"-init_jnl_blocks=%d ",alterdb->nInitial);
        x_strcat(buf,buf2);
    }
    
    if (alterdb->bDisableJrnl) {
        x_strcat(buf,"-disable_journaling ");
    }

    if (alterdb->bDelOldestCkPt) {
        x_strcat(buf,"-delete_oldest_ckp ");
    }

    if (alterdb->bVerbose) {
        x_strcat(buf,"-verbose");
    }

    //create title 
    wsprintf(buf2,
       ResourceString ((UINT)IDS_T_RMCMD_ALTERDB),
       vnodeName,
       alterdb->DBName);

    //execute command
    execrmcmd(vnodeName,buf,buf2);
   
    return (TRUE);
}
