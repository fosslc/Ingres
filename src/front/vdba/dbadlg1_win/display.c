/********************************************************************
//
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
//
//    Project  : CA/OpenIngres Visual DBA
//
//    Source   : display.c
//
//    Sub Dialog box for Display SQL option for the Dlg box Optimize db & statdump 
//
********************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <limits.h>
#include "dll.h"
#include "subedit.h"
#include "dbadlg1.h"
#include "dlgres.h"
#include "catolist.h"
#include "catospin.h"
#include "lexical.h"
#include "getdata.h"
#include "msghandl.h"
#include "domdata.h"
#include "esltools.h"
#include "rmcmd.h"


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



#define DISPLAY_CHARCOLWIDTH_MIN       1
#define DISPLAY_CHARCOLWIDTH_MAX   65535
#define DISPLAY_TEXTCOLWIDTH_MIN       1
#define DISPLAY_TEXTCOLWIDTH_MAX   65535
#define DISPLAY_I1WIDTH_MIN            1
#define DISPLAY_I1WIDTH_MAX        65535
#define DISPLAY_I2WIDTH_MIN            1
#define DISPLAY_I2WIDTH_MAX        65535
#define DISPLAY_I4WIDTH_MIN            1
#define DISPLAY_I4WIDTH_MAX        65535
#define DISPLAY_F4WIDTH_MIN            1
#define DISPLAY_F4WIDTH_MAX        65535
#define DISPLAY_F4DPLACE_MIN           1
#define DISPLAY_F4DPLACE_MAX       65535
#define DISPLAY_F8WIDTH_MIN            1
#define DISPLAY_F8WIDTH_MAX        65535
#define DISPLAY_F8DPLACE_MIN           1
#define DISPLAY_F8DPLACE_MAX       65535

#define DefCharColW   "6"
#define DefTextColW   "6"
#define DefI1W        "6"
#define DefI2W        "6"
#define DefI4W        "13" 
#define DefF4W        "10" 
#define DefF4D        "3"  
#define DefF8W        "11" 
#define DefF8D        "3"  

#define DefF4         f4x8data [MAXF4x8 -1]  
#define DefF8         f4x8data [MAXF4x8 -1]   

static  BOOL Float4Modif = FALSE;
static  BOOL Float8Modif = FALSE;

// Structure describing the numeric edit controls
static EDITMINMAX limits[] =
{
   IDC_DISPLAY_CHARCOLWIDTH_N,  IDC_DISPLAY_CHARCOLWIDTH_S,
       DISPLAY_CHARCOLWIDTH_MIN,
       DISPLAY_CHARCOLWIDTH_MAX,
   IDC_DISPLAY_TEXTCOLWIDTH_N,  IDC_DISPLAY_TEXTCOLWIDTH_S,
       DISPLAY_TEXTCOLWIDTH_MIN,
       DISPLAY_TEXTCOLWIDTH_MAX,
   IDC_DISPLAY_I1WIDTH_NUMBER,  IDC_DISPLAY_I1WIDTH_SPIN,
       DISPLAY_I1WIDTH_MIN,
       DISPLAY_I1WIDTH_MAX,
   IDC_DISPLAY_I2WIDTH_NUMBER,  IDC_DISPLAY_I2WIDTH_SPIN,
       DISPLAY_I2WIDTH_MIN,
       DISPLAY_I2WIDTH_MAX,
   IDC_DISPLAY_I4WIDTH_NUMBER,  IDC_DISPLAY_I4WIDTH_SPIN,
       DISPLAY_I4WIDTH_MIN,
       DISPLAY_I4WIDTH_MAX,
   IDC_DISPLAY_FLOAT4_WNUMBER,  IDC_DISPLAY_FLOAT4_WSPIN,
       DISPLAY_F8WIDTH_MIN,
       DISPLAY_F8WIDTH_MAX,
   IDC_DISPLAY_FLOAT4_DNUMBER,  IDC_DISPLAY_FLOAT4_DSPIN,
       DISPLAY_F4DPLACE_MIN,
       DISPLAY_F4DPLACE_MAX,
   IDC_DISPLAY_FLOAT8_WNUMBER,  IDC_DISPLAY_FLOAT8_WSPIN,
       DISPLAY_F8WIDTH_MIN,
       DISPLAY_F8WIDTH_MAX,
   IDC_DISPLAY_FLOAT8_DNUMBER,  IDC_DISPLAY_FLOAT8_DSPIN,
       DISPLAY_F8DPLACE_MIN,
       DISPLAY_F8DPLACE_MAX,

   -1  // terminator
};


BOOL CALLBACK __export DlgDisplayDlgProc
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
static void    SetDefaultMessageOption (HWND hwnd);



int WINAPI __export DlgDisplay (HWND hwndOwner, LPDISPLAYPARAMS lpdisplay)
/*
// Function:
// Shows the Refresh dialog.
//
// Paramters:
//     1) hwndOwner:   Handle of the parent window for the dialog.
//     1) lpdisplay:   Points to structure containing information used to 
//                     initialise the dialog. 
//
// Returns:
//     TRUE if successful.
//
*/
{
   FARPROC lpProc;
   int     retVal;
   
   if (!IsWindow (hwndOwner) || !lpdisplay)
   {
       ASSERT(NULL);
       return FALSE;
   }

   lpProc = MakeProcInstance ((FARPROC) DlgDisplayDlgProc, hInst);
   retVal = VdbaDialogBoxParam
       (hResource,
        MAKEINTRESOURCE (IDD_DISPLAY),
        hwndOwner,
        (DLGPROC) lpProc,
        (LPARAM)  lpdisplay
       );

   FreeProcInstance (lpProc);
   return (retVal);
}


BOOL CALLBACK __export DlgDisplayDlgProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
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

   LPDISPLAYPARAMS lpdisplay  = (LPDISPLAYPARAMS)lParam;

   if (!AllocDlgProp (hwnd, lpdisplay))
       return FALSE;
   SpinGetVersion();

   InitialiseSpinControls (hwnd);
   InitializeControls     (hwnd);
   FillFloat4x8Control    (hwnd);
   EnableDisableControls  (hwnd);
   lpHelpStack = StackObject_PUSH (lpHelpStack, StackObject_INIT ((UINT)IDD_DISPLAY));
   richCenterDialog(hwnd);
   return TRUE;
}


static void OnDestroy(HWND hwnd)
{
   SubclassAllNumericEditControls (hwnd, EC_RESETSUBCLASS);
   DeallocDlgProp(hwnd);
   lpHelpStack = StackObject_POP (lpHelpStack);
}


static void OnCommand (HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
   LPDISPLAYPARAMS lpdisplay = GetDlgProp (hwnd);

   switch (id)                                  
   {
       case IDOK:
           FillStructureFromControls (hwnd);

           EndDialog (hwnd, TRUE);
           break;

       case IDCANCEL:

           EndDialog (hwnd, FALSE);
           break;

       case IDC_DISPLAY_CHARCOLWIDTH_N:  
       case IDC_DISPLAY_TEXTCOLWIDTH_N:  
       case IDC_DISPLAY_I1WIDTH_NUMBER:  
       case IDC_DISPLAY_I2WIDTH_NUMBER:  
       case IDC_DISPLAY_I4WIDTH_NUMBER:  
       case IDC_DISPLAY_FLOAT4_WNUMBER:  
       case IDC_DISPLAY_FLOAT4_DNUMBER:  
       case IDC_DISPLAY_FLOAT8_WNUMBER:  
       case IDC_DISPLAY_FLOAT8_DNUMBER:  
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
           
       case IDC_DISPLAY_CHARCOLWIDTH_S:  
       case IDC_DISPLAY_TEXTCOLWIDTH_S:  
       case IDC_DISPLAY_I1WIDTH_SPIN  :  
       case IDC_DISPLAY_I2WIDTH_SPIN  :  
       case IDC_DISPLAY_I4WIDTH_SPIN  :  
       case IDC_DISPLAY_FLOAT4_WSPIN  :  
       case IDC_DISPLAY_FLOAT4_DSPIN  :  
       case IDC_DISPLAY_FLOAT8_WSPIN  :  
       case IDC_DISPLAY_FLOAT8_DSPIN  :  
           ProcessSpinControl (hwndCtl, codeNotify, limits);
           break;

       case IDC_DISPLAY_FLOAT4:
           if (codeNotify == CBN_SELCHANGE)
               Float4Modif = TRUE;
           break;

       case IDC_DISPLAY_FLOAT8:
           if (codeNotify == CBN_SELCHANGE)
               Float8Modif = TRUE;
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
   HWND hwndCharColW   = GetDlgItem (hwnd, IDC_DISPLAY_CHARCOLWIDTH_N);
   HWND hwndTextColW   = GetDlgItem (hwnd, IDC_DISPLAY_TEXTCOLWIDTH_N);
   HWND hwndI1W        = GetDlgItem (hwnd, IDC_DISPLAY_I1WIDTH_NUMBER);
   HWND hwndI2W        = GetDlgItem (hwnd, IDC_DISPLAY_I2WIDTH_NUMBER);
   HWND hwndI4W        = GetDlgItem (hwnd, IDC_DISPLAY_I4WIDTH_NUMBER);
   HWND hwndF4W        = GetDlgItem (hwnd, IDC_DISPLAY_FLOAT4_WNUMBER);
   HWND hwndF4D        = GetDlgItem (hwnd, IDC_DISPLAY_FLOAT4_DNUMBER);
   HWND hwndF8W        = GetDlgItem (hwnd, IDC_DISPLAY_FLOAT8_WNUMBER);
   HWND hwndF8D        = GetDlgItem (hwnd, IDC_DISPLAY_FLOAT8_DNUMBER);

   SubclassAllNumericEditControls (hwnd, EC_SUBCLASS);
   
   Edit_LimitText (hwndCharColW  , 10);
   Edit_LimitText (hwndTextColW  , 10);
   Edit_LimitText (hwndI1W       , 10);
   Edit_LimitText (hwndI2W       , 10);
   Edit_LimitText (hwndI4W       , 10);
   Edit_LimitText (hwndF4W       , 10);
   Edit_LimitText (hwndF4D       , 10);
   Edit_LimitText (hwndF8W       , 10);
   Edit_LimitText (hwndF8D       , 10);

   Edit_SetText   (hwndCharColW  , DefCharColW  );
   Edit_SetText   (hwndTextColW  , DefTextColW  );
   Edit_SetText   (hwndI1W       , DefI1W       );
   Edit_SetText   (hwndI2W       , DefI2W       );
   Edit_SetText   (hwndI4W       , DefI4W       );
   Edit_SetText   (hwndF4W       , DefF4W       );
   Edit_SetText   (hwndF4D       , DefF4D       );
   Edit_SetText   (hwndF8W       , DefF8W       );
   Edit_SetText   (hwndF8D       , DefF8D       );

   LimitNumericEditControls (hwnd);
}



static void InitialiseSpinControls (HWND hwnd)
{
   DWORD dwMin, dwMax;

   GetEditCtrlMinMaxValue (hwnd, GetDlgItem (hwnd, IDC_DISPLAY_CHARCOLWIDTH_N), limits, &dwMin, &dwMax); 
   SpinRangeSet (GetDlgItem (hwnd, IDC_DISPLAY_CHARCOLWIDTH_S), dwMin, dwMax);

   GetEditCtrlMinMaxValue (hwnd, GetDlgItem (hwnd, IDC_DISPLAY_TEXTCOLWIDTH_N), limits, &dwMin, &dwMax); 
   SpinRangeSet (GetDlgItem (hwnd, IDC_DISPLAY_TEXTCOLWIDTH_S), dwMin, dwMax);

   GetEditCtrlMinMaxValue (hwnd, GetDlgItem (hwnd, IDC_DISPLAY_I1WIDTH_NUMBER), limits, &dwMin, &dwMax); 
   SpinRangeSet (GetDlgItem (hwnd, IDC_DISPLAY_I1WIDTH_SPIN  ), dwMin, dwMax);

   GetEditCtrlMinMaxValue (hwnd, GetDlgItem (hwnd, IDC_DISPLAY_I2WIDTH_NUMBER), limits, &dwMin, &dwMax); 
   SpinRangeSet (GetDlgItem (hwnd, IDC_DISPLAY_I2WIDTH_SPIN  ), dwMin, dwMax);

   GetEditCtrlMinMaxValue (hwnd, GetDlgItem (hwnd, IDC_DISPLAY_I4WIDTH_NUMBER), limits, &dwMin, &dwMax); 
   SpinRangeSet (GetDlgItem (hwnd, IDC_DISPLAY_I4WIDTH_SPIN  ), dwMin, dwMax);

   GetEditCtrlMinMaxValue (hwnd, GetDlgItem (hwnd, IDC_DISPLAY_FLOAT4_WNUMBER), limits, &dwMin, &dwMax); 
   SpinRangeSet (GetDlgItem (hwnd, IDC_DISPLAY_FLOAT4_WSPIN  ), dwMin, dwMax);

   GetEditCtrlMinMaxValue (hwnd, GetDlgItem (hwnd, IDC_DISPLAY_FLOAT4_DNUMBER), limits, &dwMin, &dwMax); 
   SpinRangeSet (GetDlgItem (hwnd, IDC_DISPLAY_FLOAT4_DSPIN  ), dwMin, dwMax);

   GetEditCtrlMinMaxValue (hwnd, GetDlgItem (hwnd, IDC_DISPLAY_FLOAT8_WNUMBER), limits, &dwMin, &dwMax); 
   SpinRangeSet (GetDlgItem (hwnd, IDC_DISPLAY_FLOAT8_WSPIN  ), dwMin, dwMax);

   GetEditCtrlMinMaxValue (hwnd, GetDlgItem (hwnd, IDC_DISPLAY_FLOAT8_DNUMBER), limits, &dwMin, &dwMax); 
   SpinRangeSet (GetDlgItem (hwnd, IDC_DISPLAY_FLOAT8_DSPIN  ), dwMin, dwMax);
}


static void EnableDisableOKButton (HWND hwnd)
{
   /*
   HWND hwndSpecifiedTables = GetDlgItem (hwnd, IDC_DISPLAY_TABLES);


   if (table.lpTable)
       Button_SetCheck (hwndSpecifiedTables, TRUE);
   else
       Button_SetCheck (hwndSpecifiedTables, FALSE);
   EnableWindow (hwndSpecifiedTables, FALSE);
   */

}

static BOOL FillStructureFromControls (HWND hwnd)
{

   char szGreatBuffer   [3000];
   char szSQLOptionFlag [200];
   char szZFlag         [200];
   char szBuffer        [100];
   char szTxC           [2000];
   char szBufferFile    [64];
   char buf1            [64];
   char buf2            [64];
//   char buftemp         [200];

   HWND hwnd_cN  = GetDlgItem (hwnd, IDC_DISPLAY_CHARCOLWIDTH_N);
   HWND hwnd_F4  = GetDlgItem (hwnd, IDC_DISPLAY_FLOAT4);
   HWND hwnd_F4W = GetDlgItem (hwnd, IDC_DISPLAY_FLOAT4_WNUMBER);
   HWND hwnd_F4D = GetDlgItem (hwnd, IDC_DISPLAY_FLOAT4_DNUMBER);
   HWND hwnd_F8  = GetDlgItem (hwnd, IDC_DISPLAY_FLOAT8);
   HWND hwnd_F8W = GetDlgItem (hwnd, IDC_DISPLAY_FLOAT8_WNUMBER);
   HWND hwnd_F8D = GetDlgItem (hwnd, IDC_DISPLAY_FLOAT8_DNUMBER);
   HWND hwnd_I1  = GetDlgItem (hwnd, IDC_DISPLAY_I1WIDTH_NUMBER);
   HWND hwnd_I2  = GetDlgItem (hwnd, IDC_DISPLAY_I2WIDTH_NUMBER);
   HWND hwnd_I4  = GetDlgItem (hwnd, IDC_DISPLAY_I4WIDTH_NUMBER);
   HWND hwnd_tN  = GetDlgItem (hwnd, IDC_DISPLAY_TEXTCOLWIDTH_N);
   
   LPDISPLAYPARAMS  lpdisplay  = GetDlgProp (hwnd);
   LPUCHAR vnodeName = GetVirtNodeName (GetCurMdiNodeHandle ());

   ZEROINIT (szGreatBuffer);
   ZEROINIT (szBufferFile);
   ZEROINIT (szBuffer);
   ZEROINIT (szTxC);
   ZEROINIT (szSQLOptionFlag);
   ZEROINIT (szZFlag);

   x_strcpy (szGreatBuffer, "");
   
   //
   // Generate SQL flag
   //

   //
   // Generate -cN
   //
   ZEROINIT (szBuffer);
   Edit_GetText (hwnd_cN, szBuffer, sizeof (szBuffer));

   if ((x_strlen (szBuffer) > 0) && (x_strcmp (szBuffer, DefCharColW) != 0))
   {
       x_strcat (szGreatBuffer, " -c");
       x_strcat (szGreatBuffer, szBuffer);
   }
   //
   // Generate f4xM.N
   //
   ZEROINIT (buf1);
   ZEROINIT (buf2);

   Edit_GetText (hwnd_F4W, buf1, sizeof (buf1));
   Edit_GetText (hwnd_F4D, buf2, sizeof (buf2));

   if (Float4Modif || (x_strcmp (buf1, DefF4W) !=0) || (x_strcmp (buf2, DefF4D) !=0))
   {
       int i;
    
       ZEROINIT (szBuffer);
       ComboBox_GetText (hwnd_F4, szBuffer, sizeof (szBuffer));
    
       for (i=0; i<MAXF4x8; i++)
       {
           if (x_strcmp (f4x8data [i].char1, szBuffer) == 0)
           {
               x_strcat (szGreatBuffer, " -f4");
               x_strcat (szGreatBuffer, f4x8data [i].char2);
               break;
           }
       }

       if (Edit_GetTextLength (hwnd_F4W) > 0)
       {
           ZEROINIT (szBuffer);
           Edit_GetText (hwnd_F4W, szBuffer, sizeof (szBuffer));
           x_strcat (szGreatBuffer, szBuffer);
       }

       if (Edit_GetTextLength (hwnd_F4D) > 0)
       {
           ZEROINIT (szBuffer);
           Edit_GetText (hwnd_F4D, szBuffer, sizeof (szBuffer));
           x_strcat (szGreatBuffer, ".");
           x_strcat (szGreatBuffer, szBuffer);
       }
   }

   //
   // Generate f8xM.N
   //
   ZEROINIT (buf1);
   ZEROINIT (buf2);

   Edit_GetText (hwnd_F8W, buf1, sizeof (buf1));
   Edit_GetText (hwnd_F8D, buf2, sizeof (buf2));

   if (Float8Modif || x_strcmp (buf1, DefF8W) !=0 || x_strcmp (buf2, DefF8D) !=0)
   {
       int i;
       
       ZEROINIT (szBuffer);
       ComboBox_GetText (hwnd_F8, szBuffer, sizeof (szBuffer));
    
       for (i=0; i<MAXF4x8; i++)
       {
           if (x_strcmp (f4x8data [i].char1, szBuffer) == 0)
           {
               x_strcat (szGreatBuffer, " -f8");
               x_strcat (szGreatBuffer, f4x8data [i].char2);
               break;
           }
       }

       if (Edit_GetTextLength (hwnd_F8W) > 0)
       {
           ZEROINIT (szBuffer);
           Edit_GetText (hwnd_F8W, szBuffer, sizeof (szBuffer));
           x_strcat (szGreatBuffer, szBuffer);
       }

       if (Edit_GetTextLength (hwnd_F8D) > 0)
       {
           ZEROINIT (szBuffer);
           Edit_GetText (hwnd_F8D, szBuffer, sizeof (szBuffer));
           x_strcat (szGreatBuffer, ".");
           x_strcat (szGreatBuffer, szBuffer);
       }
   }
   //
   // Generate -ikN (k = 1)
   //
   ZEROINIT (buf1);

   Edit_GetText (hwnd_I1, buf1, sizeof (buf1));
   if ((Edit_GetTextLength (hwnd_I1) > 0) && x_strcmp (buf1, DefI1W) != 0)
   {
       ZEROINIT (szBuffer);
       Edit_GetText (hwnd_I1, szBuffer, sizeof (szBuffer));
       x_strcat (szGreatBuffer, " -i1");
       x_strcat (szGreatBuffer, szBuffer);
   }
   //
   // Generate -ikN (k = 2)
   //
   ZEROINIT (buf1);

   Edit_GetText (hwnd_I2, buf1, sizeof (buf1));
   if ((Edit_GetTextLength (hwnd_I2) > 0) && x_strcmp (buf1, DefI2W) != 0)
   {
       ZEROINIT (szBuffer);
       Edit_GetText (hwnd_I2, szBuffer, sizeof (szBuffer));
       x_strcat (szGreatBuffer, " -i2");
       x_strcat (szGreatBuffer, szBuffer);
   }
   //
   // Generate -ikN (k = 4)
   //
   ZEROINIT (buf1);

   Edit_GetText (hwnd_I4, buf1, sizeof (buf1));
   if ((Edit_GetTextLength (hwnd_I4) > 0) && x_strcmp (buf1, DefI4W) != 0)
   {
       ZEROINIT (szBuffer);
       Edit_GetText (hwnd_I4, szBuffer, sizeof (szBuffer));
       x_strcat (szGreatBuffer, " -i4");
       x_strcat (szGreatBuffer, szBuffer);
   }
   //
   // Generate -tN
   //
   ZEROINIT (buf1);

   Edit_GetText (hwnd_tN, buf1, sizeof (buf1));
   if ((Edit_GetTextLength (hwnd_tN) > 0) && x_strcmp (buf1, DefTextColW) != 0)
   {
       ZEROINIT (szBuffer);
       Edit_GetText (hwnd_tN, szBuffer, sizeof (szBuffer));
       x_strcat (szGreatBuffer, " -t");
       x_strcat (szGreatBuffer, szBuffer);
   }


   //   if (Button_GetCheck (GetDlgItem (hwnd, IDC_DISPLAY_WAIT)))
   //       x_strcat (szGreatBuffer, " +w");

   //
   // Generate the -uusername
   //
   //if (DBAGetSessionUser(vnodeName, buftemp)) {
   //   x_strcat (szGreatBuffer, " -u");
   //   suppspace(buftemp);
   //   x_strcat (szGreatBuffer, buftemp);
   //}

   //
   // I don't know how to generate -xk for SQL option flags
   // TO BE FINISHED

   x_strcpy (lpdisplay->szString, szGreatBuffer);
   //MessageBox (GetFocus(), szGreatBuffer, "Message", MB_OK);

   return TRUE;
}

static void FillFloat4x8Control (HWND hwnd)
{
   int  i;
   HWND hwndF4 = GetDlgItem (hwnd, IDC_DISPLAY_FLOAT4);
   HWND hwndF8 = GetDlgItem (hwnd, IDC_DISPLAY_FLOAT8);

   for (i=0; i<MAXF4x8; i++)
   {
       ComboBox_AddString (hwndF4, f4x8data [i].char1);
       ComboBox_AddString (hwndF8, f4x8data [i].char1);
   }
   ComboBox_SelectString (hwndF4, -1, f4x8data [MAXF4x8 -1].char1);
   ComboBox_SelectString (hwndF8, -1, f4x8data [MAXF4x8 -1].char1);
}

static void EnableDisableControls (HWND hwnd)
{

}

