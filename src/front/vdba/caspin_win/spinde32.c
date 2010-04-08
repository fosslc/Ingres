/****************************************************************************
 * Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
 ****************************************************************************/

/****************************************************************************
 * SPINDE32.C
 *
 * Routines to interface the DLL custom control to the Win32 Dialog Editor.
 ****************************************************************************/

#include <windows.h>
#include "spindll.h"
#include "spindlg.h"

LPCCSTYLE gpccs;   // global pointer to a CCSTYLE structure

CCSTYLEFLAGA aSpinStyleFlags[]= { { WS_TABSTOP,        0, "WS_TABSTOP"        },
                                  { WS_GROUP,          0, "WS_GROUP"          },
                                  { SPS_VERTICAL,      0, "SPS_VERTICAL"      },  
                                  { SPS_HORIZONTAL,    0, "SPS_HORIZONTAL"    },  
                                  { SPS_NOPEGNOTIFY,   0, "SPS_NOPEGNOTIFY"   },  
                                  { SPS_WRAPMINMAX,    0, "SPS_WRAPMINMAX"    },  
                                  { SPS_TEXTHASRANGE,  0, "SPS_TEXTHASRANGE"  },  
                                  { SPS_INVERTCHGBTN,  0, "SPS_INVERTCHGBTN"  },
                                  { SPS_HASSTRINGS,    0, "SPS_HASSTRINGS"    },
                                  { SPS_HASSORTEDSTRS, 0, "SPS_HASSORTEDSTRS" } };

#define NUM_SPIN_STYLES    (sizeof(aSpinStyleFlags) / sizeof(aSpinStyleFlags[0]))

BOOL CALLBACK ShowSpinStyleDlg( HWND hWndParent, LPCCSTYLE pccs );
int  CALLBACK SpinSizeToText( DWORD flStyle, DWORD flExtStyle, HFONT hFont, LPSTR pszText );

/****************************************************************************
 * CustomControlInfoA
 *
 * Purpose:
 *  Initializes an LPCCINFOA structure for this DLL.
 *  See CUSTCNTL.H for more info on these functions.
 *
 * Parameters:
 *  LPCCINFOA     acci        - pointer to an array of CCINFOA structures
 *
 * Return Value:
 *  UINT          Number of controls supported by this DLL
 ****************************************************************************/

UINT CALLBACK CustomControlInfoA( LPCCINFOA acci )
    {
    // Dlgedit is querying the number of controls this DLL supports,
    // so return 1. Then we'll get called again with a valid "acci"
    if( !acci )
      return 1;

    // Fill in the constant values.

    acci[0].flOptions         = 0;
    acci[0].cxDefault         = 11;      // default width  (dialog units)
    acci[0].cyDefault         = 20;      // default height (dialog units)
    acci[0].flStyleDefault    = WS_CHILD | WS_VISIBLE | SPS_VERTICAL;
    acci[0].flExtStyleDefault = 0;
    acci[0].flCtrlTypeMask    = 0;
    acci[0].cStyleFlags       = NUM_SPIN_STYLES;
    acci[0].aStyleFlags       = aSpinStyleFlags;
    acci[0].lpfnStyle         = ShowSpinStyleDlg;
    acci[0].lpfnSizeToText    = SpinSizeToText;
    acci[0].dwReserved1       = 0;
    acci[0].dwReserved2       = 0;
    
    // Copy the strings
    //
    // NOTE: MAKE SURE THE STRINGS COPIED DO NOT EXCEED THE LENGTH OF
    //       THE BUFFERS IN THE CCINFO STRUCTURE!
    
    lstrcpy( acci[0].szClass, CASPIN_CLASS );
    LoadString( hInstSpin, IDS_SHORTNAME, acci[0].szDesc,        CCHCCDESC );
    LoadString( hInstSpin, IDS_FULLNAME,  acci[0].szTextDefault, CCHCCTEXT );
    
    // Return the number of controls that the DLL supports
    
    return 1;
    }

/****************************************************************************
 * ShowSpinStyleDlg
 *
 * Purpose:
 *  Requests the control library to display a dialog box that allows
 *  editing of the control's styles.  The fields in a CCSTYLE structure
 *  must be set according to what the window is and what it contains.
 *
 * Parameters:
 *  HWND          hWndParent  - handle of parent window (dialog editor)
 *  LPCCSTYLE     pccs        - pointer to a CCSTYLE structure
 *
 * Return Value:
 *  BOOL          TRUE if successful else FALSE
 ****************************************************************************/

BOOL CALLBACK ShowSpinStyleDlg( HWND hWndParent, LPCCSTYLE pccs )
    {
    int  rc;
    BOOL bRet=TRUE;

    // Set the global styles pointer.
    gpccs = pccs;

    // Display the styles dialog.
    rc = DialogBox( hInstSpin, MAKEINTRESOURCE(IDD_STYLEDIALOG), hWndParent, SpinStyleDlgProc );

    if( rc == -1 )
      {
      char szText[LEN_ERRMSG], szTitle[LEN_ERRMSG];

      LoadString( hInstSpin, IDS_STYLEDLG_ERR, szText, LEN_ERRMSG );
      LoadString( hInstSpin, IDS_DLL_ERR, szTitle, LEN_ERRMSG );
      MessageBox( hWndParent, szText, szTitle, MB_OK | MB_ICONEXCLAMATION | MB_APPLMODAL );

      bRet = FALSE;
      }

    return bRet;
    }

/****************************************************************************
 * SpinSizeToText
 *
 * Purpose:
 *  Just no-op here (since we never actually display text in
 *  the control it doesn't need to be resized).
 *
 * Parameters:
 *  DWORD         flStyle     - control style
 *  DWORD         flExtStyle  - control extended style
 *  HFONT         hFont       - handle of font used to draw text
 *  LPSTR         pszText     - control text
 *
 * Return Value:
 *  int           pixel width control to accomodate text or -1 for error
 ****************************************************************************/

int CALLBACK SpinSizeToText( DWORD flStyle, DWORD flExtStyle, HFONT hFont, LPSTR pszText )
    {
    return -1;
    }

/****************************************************************************
 * GetSpinStyles
 * SetSpinStyles
 *
 * Purpose:
 *  Get or set the spin style flag in the CCSTYLE struct.
 *
 * Parameters:
 *  DWORD         dwStyle     - new control styles
 *
 * Return Value:
 *  DWORD         control styles
 ****************************************************************************/

DWORD WINAPI GetSpinStyles( void )
    {
    return gpccs->flStyle;
    }

VOID WINAPI SetSpinStyles( DWORD dwStyle )
    {
    gpccs->flStyle = dwStyle;
    }
