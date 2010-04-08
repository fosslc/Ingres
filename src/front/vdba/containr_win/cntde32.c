/****************************************************************************
 * Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
 ****************************************************************************/

/****************************************************************************
 * CNTDE32.C
 *
 * Routines to interface the DLL custom control to the Win32 Dialog Editor.
 ****************************************************************************/

#include <windows.h>
#include "cntdll.h"
#include "cntdlg.h"

LPCCSTYLE gpccs;   // global pointer to a CCSTYLE structure

CCSTYLEFLAGA aCntStyleFlags[] = { { CTS_READONLY,    0,  "CTS_READONLY"    },
                                  { CTS_SINGLESEL,   0,  "CTS_SINGLESEL"   },
                                  { CTS_MULTIPLESEL, 0,  "CTS_MULTIPLESEL" },
                                  { CTS_EXTENDEDSEL, 0,  "CTS_EXTENDEDSEL" },
                                  { CTS_BLOCKSEL,    0,  "CTS_BLOCKSEL"    },
                                  { CTS_SPLITBAR,    0,  "CTS_SPLITBAR"    },
                                  { CTS_VERTSCROLL,  0,  "CTS_VERTSCROLL"  },
                                  { CTS_HORZSCROLL,  0,  "CTS_HORZSCROLL"  },
                                  { CTS_INTEGRALWIDTH,  0,  "CTS_INTEGRALWIDTH"  },
                                  { CTS_INTEGRALHEIGHT, 0,  "CTS_INTEGRALHEIGHT" },
                                  { CTS_EXPANDLASTFLD,  0,  "CTS_EXPANDLASTFLD"  },
                                  { WS_TABSTOP,      0,  "WS_TABSTOP"      },
                                  { WS_GROUP,        0,  "WS_GROUP"        },
                                  { WS_VISIBLE,      0,  "WS_VISIBLE"      },
                                  { WS_BORDER,       0,  "WS_BORDER"       },
                                  { WS_DLGFRAME,     0,  "WS_DLGFRAME"     },
                                  { WS_CAPTION,      0,  "WS_CAPTION"      },
                                  { WS_SYSMENU,      0,  "WS_SYSMENU"      } };


#define NUM_CNT_STYLES    (sizeof(aCntStyleFlags) / sizeof(aCntStyleFlags[0]))

BOOL CALLBACK ShowCntStyleDlg( HWND hWndParent, LPCCSTYLE pccs );
int  CALLBACK CntSizeToText( DWORD flStyle, DWORD flExtStyle, HFONT hFont, LPSTR pszText );

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
    acci[0].cxDefault         = 20;      // default width  (dialog units)
    acci[0].cyDefault         = 20;      // default height (dialog units)
    acci[0].flStyleDefault    = WS_CHILD | WS_VISIBLE | WS_BORDER;
    acci[0].flExtStyleDefault = 0;
    acci[0].flCtrlTypeMask    = 0;
    acci[0].cStyleFlags       = NUM_CNT_STYLES;
    acci[0].aStyleFlags       = aCntStyleFlags;
    acci[0].lpfnStyle         = ShowCntStyleDlg;
    acci[0].lpfnSizeToText    = CntSizeToText;
    acci[0].dwReserved1       = 0;
    acci[0].dwReserved2       = 0;
    
    // Copy the strings
    //
    // NOTE: MAKE SURE THE STRINGS COPIED DO NOT EXCEED THE LENGTH OF
    //       THE BUFFERS IN THE CCINFO STRUCTURE!
    
    lstrcpy( acci[0].szClass, CONTAINER_CLASS );
    LoadString( hInstCnt, IDS_SHORTNAME, acci[0].szDesc,        CCHCCDESC );
    LoadString( hInstCnt, IDS_FULLNAME,  acci[0].szTextDefault, CCHCCTEXT );
    
    // Return the number of controls that the DLL supports
    
    return 1;
    }

/****************************************************************************
 * ShowCntStyleDlg
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

BOOL CALLBACK ShowCntStyleDlg( HWND hWndParent, LPCCSTYLE pccs )
    {
    int  rc;
    BOOL bRet=TRUE;

    // Set the global styles pointer.
    gpccs = pccs;

    // Display the styles dialog.
    rc = DialogBox( hInstCnt, MAKEINTRESOURCE(IDD_STYLEDIALOG), hWndParent, CntStyleDlgProc );

    if( rc == -1 )
      {
      char szText[LEN_ERRMSG], szTitle[LEN_ERRMSG];

      LoadString( hInstCnt, IDS_STYLEDLG_ERR, szText, LEN_ERRMSG );
      LoadString( hInstCnt, IDS_DLL_ERR, szTitle, LEN_ERRMSG );
      MessageBox( hWndParent, szText, szTitle, MB_OK | MB_ICONEXCLAMATION | MB_APPLMODAL );

      bRet = FALSE;
      }

    return bRet;
    }

/****************************************************************************
 * CntSizeToText
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
 *  int           pixel width control must be to accomodate text or -1 for error
 ****************************************************************************/

int CALLBACK CntSizeToText( DWORD flStyle, DWORD flExtStyle, HFONT hFont, LPSTR pszText )
    {
    return -1;
    }

/****************************************************************************
 * GetCntStyles
 * SetCntStyles
 *
 * Purpose:
 *  Get or set the container style flag in the CCSTYLE struct.
 *
 * Parameters:
 *  DWORD         dwStyle     - new control styles
 *
 * Return Value:
 *  DWORD         control styles
 ****************************************************************************/

DWORD WINAPI GetCntStyles( void )
    {
    return gpccs->flStyle;
    }

VOID WINAPI SetCntStyles( DWORD dwStyle )
    {
    gpccs->flStyle = dwStyle;
    }

/****************************************************************************
 * ListClasses
 *
 * Purpose:
 *  Called by Borland's Resource Workshop retrieve the information
 *  necessary to edit the custom controls contain in this DLL.
 *  This is an alternative to the Microsoft xxxStyle convention.
 *
 * Return Value:
 *  HANDLE        Handle to a Global memory object or NULL if it cannot
 *                be allocated.  The caller must free the memory.
 *                NOTE: This memory is shared with other applications
 *                      so GMEM_SHARE is set when allocating.
 ****************************************************************************/

HANDLE WINAPI ListClasses( LPSTR szAppName, UINT wVersion, LPVOID fnLoad, LPVOID fnEdit )
    {
    return NULL;
    }
