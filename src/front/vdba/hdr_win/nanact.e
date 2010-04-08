/********************************************************************
//
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
//
//    Project  : CA/OpenIngres Visual DBA
//
//    Source   : nanact.e
//
//    
//
********************************************************************/
#ifndef NANACT_E_INCLUDED
#define NANACT_E_INCLUDED

#ifdef WIN32
    #ifdef NANACT_DEV
        #define NANACT_LIBAPI __declspec(dllexport)
    #else
        #define NANACT_LIBAPI __declspec(dllimport)
    #endif
#else
    #define NANACT_LIBAPI
#endif

//
//      NANBAR section
//
NANACT_LIBAPI BOOL    FAR PASCAL NanBarWndClassRegister(HANDLE hInstance);
NANACT_LIBAPI HWND    FAR PASCAL CreateNanBarWnd(HWND hwndParent, HANDLE hInstance, UINT * lpumsg);


typedef struct _NanBtnData 
{
    WORD            wButtonID;                              //      Button ID, used as wParam in the WM_COMMAND message
    WORD            wNotifyStringID;                //      StringID for a notification in a status window...
    HBITMAP         hBitMap;                                //      bitmap to show
    BOOL            fBreak;                                 //      if TRUE, causes NANBAR to make a separator to the next button
    BOOL            fCheckMark;                             //      if TRUE its a status button, released after second press
}       NANBTNDATA , FAR * LPNANBTNDATA;

#define         NANBTN_ID(s)            ((s).wButtonID)
#define         NANBTN_STRINGID(s)      ((s).wNotifyStringID)
#define         NANBTN_HBITMAP(s)       ((s).hBitMap)
#define         NANBTN_BREAK(s)         ((s).fBreak)
#define         NANBTN_CHECK(s)         ((s).fCheckMark)

#define NANBAR_CREATEBTN                WM_USER + 1
//
//      WM_CMD message, in lParam has to be a pointer to NANBTNCREATE, see below
//
#define NanBar_CreateBtn(hwndNan, lpcreate)\
            (void)SendMessage((HWND)(hwndNan), NANBAR_CREATEBTN, 0, (LPARAM)(LPNANBTNDATA)(lpcreate))

#define NANBAR_GETHEIGHT                WM_USER + 2
//
//      WM_CMD message, returns the height of the window
//
#define NanBar_GetHeight(hwndNan)\
            (int)SendMessage((HWND)(hwndNan), NANBAR_GETHEIGHT, 0, 0)

#define NANBAR_DISABLE                  WM_USER + 4
//
//      WM_CMD message, send, to disable a button, the button ID is in lParam
//              by default buttons are enabled
//              returns TRUE if BUTTON is successfull disabled
//      
#define NanBar_Disable(hwndNan, nButtonID)\
            (BOOL)SendMessage((HWND)(hwndNan), NANBAR_DISABLE, 0, MAKELPARAM(nButtonID, 0))

#define NANBAR_ISDISABLED                WM_USER + 5
#define NANBAR_BTNSTATUS                 WM_USER + 6
//
//      WM_CMD message, send, to see if a button is enabled or disabled , the button ID is in lParam
//              returns TRUE means enabled, FALSE means disabled
//      
#define NanBar_IsDisabled(hwndNan, nButtonID)\
            (BOOL)SendMessage((HWND)(hwndNan), NANBAR_ISDISABLED, 0, MAKELPARAM(nButtonID, 0))
#define NanBar_BtnStatus(hwndNan, nButtonID)\
            (BOOL)SendMessage((HWND)(hwndNan), NANBAR_BTNSTATUS, 0, MAKELPARAM(nButtonID, 0))

#define NANBAR_ENABLE                   WM_USER + 7
//
//      WM_CMD message, send, to enable a button, the button ID is in lParam
//              by default buttons are enabled
//              returns TRUE if BUTTON is successfull enabled
//      
#define NanBar_Enable(hwndNan, nButtonID)\
            (BOOL)SendMessage((HWND)(hwndNan), NANBAR_ENABLE, 0, MAKELPARAM(nButtonID, 0))

#define NANBAR_SETCHECK                 WM_USER + 8
//
//              WM_CMD message, send, to release the check of a button, the button ID is in lParam
//              if      a checkmark is not pressed, this message is going to press it 
#define NanBar_SetCheck(hwndNan, nButtonID)\
            (void)SendMessage((HWND)(hwndNan), NANBAR_SETCHECK, 0, MAKELPARAM(nButtonID, 0))

#define NANBAR_RELEASECHECK             WM_USER + 9
//
//              WM_CMD message, send, to release the check of a button, the button ID is in lParam
//              if      a checkmark is pressed, this message is going to release it 
#define NanBar_ReleaseCheck(hwndNan, nButtonID)\
            (void)SendMessage((HWND)(hwndNan), NANBAR_SETCHECK, 0, MAKELPARAM(nButtonID, 0))

// **** Notification messages sent to the owner window ****

#define NANBAR_NOTIFY_FREEBMP           1
#define NanBar_NotifyFreeBmp(hwndOwner, msg, hBitmap)\
            (void)SendMessage((HWND)(hwndOwner), (UINT)(msg), NANBAR_NOTIFY_FREEBMP, (LPARAM)(HBITMAP)(hBitmap))

#define NANBAR_NOTIFY_MSG               2
//
//      WM_CMD message, send, if mouse is over button, in lParam is wNotifyStringID
//      
#define NanBar_NotifyMsg(hwndOwner, msg, wNotifyStringID)\
            (void)SendMessage((HWND)(hwndOwner), (UINT)(msg), NANBAR_NOTIFY_MSG, MAKELPARAM(wNotifyStringID, 0))

// ****

//
//      NanStatus       section
//
NANACT_LIBAPI BOOL FAR PASCAL NanStatusWndClassRegister(HANDLE hInstance);
NANACT_LIBAPI HWND FAR PASCAL CreateNanStatusWnd(HWND hwndParent, HANDLE hInstance);


#define         STATUS_SETMSG                           WM_USER + 1             //      causes the status window to show a msg in lParam
#define         STATUS_GETHEIGHT                        WM_USER + 2             //      returns the height of the window
#define         STATUS_FONTCHG                          WM_USER + 3             //      changes the font for the status window, hFont is in lParam


//
//      NanPopUp section
//

typedef struct _NANPOPUPENTRY
{
    WORD            wIdentifyBy;
    WORD            wCMD;
    WORD            wState;
    WORD            wStringID;
    LPSTR           lpstrString;

}       NANPOPUPENTRY, FAR * LPNANPOPUPENTRY;

#define         POPUP_BY_ID                     ((WORD) 1)              //      the menu entry is defined in the stringtable resource
#define         POPUP_BY_LPSTR          ((WORD) 2)              //      the menu entry is defined by a LPSTR

#define         NPENTRY_IDBY(p)         ((p)->wIdentifyBy)
#define         NPENTRY_CMD(p)          ((p)->wCMD)
#define         NPENTRY_STATE(p)                ((p)->wState)
#define         NPENTRY_STRINGID(p)     ((p)->wStringID)
#define         NPENTRY_LPSTR(p)                ((p)->lpstrString)



typedef struct _NANPOPUPDATA
{
    HWND                                    hwndOwner;                              // who to send the wm_commands to
    HANDLE                                  hInstance;
    WORD                                    wNrOfEntries;
    LPNANPOPUPENTRY         pEntries;
}       NANPOPUPDATA, FAR * LPNANPOPUPDATA;

#define NPDATA_OWNER(s)         ((s).hwndOwner)
#define NPDATA_HINST(s)         ((s).hInstance)
#define NPDATA_NRENTRIES(s)     ((s).wNrOfEntries)
#define NPDATA_PENTRIES(s)      ((s).pEntries)


#define LPNPDATA_OWNER(p)               ((p)->hwndOwner)
#define LPNPDATA_HINST(p)               ((p)->hInstance)
#define LPNPDATA_NRENTRIES(p)   ((p)->wNrOfEntries)
#define LPNPDATA_PENTRIES(p)    ((p)->pEntries)



BOOL FAR PASCAL NanPopUp(LPNANPOPUPDATA lpNanPopUpData, LPPOINT lpPoint);


// Removed 02/05/95 Emb : BOOL FAR PASCAL NanGetWindowsDate(LPSTR lpstrDate);

#define Status_SetMsg(hwndStatus, lpmsg)\
            (void)SendMessage((HWND)(hwndStatus), STATUS_SETMSG, 0, (LPARAM)(LPSTR)(lpmsg))

#define Status_GetHeight(hwndStatus)\
            (unsigned short)SendMessage((HWND)(hwndStatus), STATUS_GETHEIGHT, 0, 0)

#define Status_FontChg(hwndStatus, hfont)\
            (void)SendMessage((HWND)(hwndStatus), STATUS_FONTCHG, 0, (LPARAM)(HFONT)(hfont))

#endif

