/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : wmusrmsg.h, Header file.
**    Project  : Ingres II / Visual Manager
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Owner Message
**
** History:
**
** 16-Dec-1998 (uk$so01)
**    created
** 29-Feb-2000 (uk$so01)
**    SIR # 100634. The "Add" button of Parameter Tab can be used to set 
**    Parameters for System and User Tab. 
** 01-Mar-2000 (uk$so01)
**    SIR #100635, Add the Functionality of Find operation.
** 02-Mar-2000 (uk$so01)
**    SIR #100690, If user runs the second Ivm than activate the first one.
** 20-Apr-2000 (uk$so01)
**    SIR #101252
**    When user start the VCBF, and if IVM is already running
**    then request IVM the start VCBF
** 23-Oct-2001 (uk$so01)
**    SIR #106057 (sqltest as ActiveX & Sql Assistant as In-Process COM Server)
** 19-Dec-2001 (uk$so01)
**    SIR #106648, Split vdba into the small component ActiveX/COM
** 19-Dec-2003 (uk$so01)
**    SIR #111475, Coorperative shutdown between the DBMS Client & Server.
**/

#if !defined (WMUSRMSG_HEADER)
#define WMUSRMSG_HEADER
#define WM_BASE (WM_USER + 1000)

#define WM_LAYOUTEDITDLG_CANCEL             (WM_BASE+ 1)    // WPARAM = 0, LPARAM = 0.
#define WM_LAYOUTEDITDLG_OK                 (WM_BASE+ 2)    // WPARAM = 0, LPARAM = 0.
#define WM_LAYOUTCOMBODLG_CANCEL            (WM_BASE+ 3)    // WPARAM = 0, LPARAM = 0.
#define WM_LAYOUTCOMBODLG_OK                (WM_BASE+ 4)    // WPARAM = 0, LPARAM = 0.
#define WM_LAYOUTEDITNUMBERDLG_CANCEL       (WM_BASE+ 5)
#define WM_LAYOUTEDITNUMBERDLG_OK           (WM_BASE+ 6)
#define WM_LAYOUTEDITSPINDLG_CANCEL         (WM_BASE+ 7)
#define WM_LAYOUTEDITSPINDLG_OK             (WM_BASE+ 8)
#define WM_LAYOUTCHECKBOX_CHECKCHANGE       (WM_BASE+ 9)
#define WM_LAYOUTEDITINTEGERDLG_CANCEL      (WM_BASE+10)
#define WM_LAYOUTEDITINTEGERDLG_OK          (WM_BASE+11)
#define WM_LAYOUTEDITINTEGERSPINDLG_CANCEL  (WM_BASE+12)
#define WM_LAYOUTEDITINTEGERSPINDLG_OK      (WM_BASE+13)
#define WM_LAYOUTEDIT_HIDE_PROPERTY         (WM_BASE+14) 
#define WM_LAYOUTEDITDLG_EXPR_ASSISTANT     (WM_BASE+15) // WPARAM = 0, LPARAM = (LPCTSTR)[Current Text in the Edit Ctrl].

#define WMUSRMSG_UPDATEDATA                 (WM_BASE+16) 

#define WMUSRMSG_PAGE_VALIDATE              (WM_BASE +41)
#define WMUSRMSG_SHELL_NOTIFY_ICON          (WM_BASE +42)
#define WMUSRMSG_GETITEMDATA                (WM_BASE +43)
#define WMUSRMSG_MATCHINSTANCE              (WM_BASE +44)
#define WMUSRMSG_REMOVE_EVENT               (WM_BASE +45)
#define WMUSRMSG_NEW_EVENT                  (WM_BASE +46)
#define WMUSRMSG_GETHELPID                  (WM_BASE +47)
#define WMUSRMSG_READ_EVENT                 (WM_BASE +48)
#define WMUSRMSG_WORKERTHREADUPDATE         (WM_BASE +49)
#define WMUSRMSG_COMPONENT_CHANGED          (WM_BASE +50)
#define WMUSRMSG_MESSAGEBOX                 (WM_BASE +51)
#define WMUSRMSG_START_STOP                 (WM_BASE +52)
#define WMUSRMSG_QUERYDROPTARGET            (WM_BASE +53)
#define WMUSRMSG_DROPOVER                   (WM_BASE +54)
#define WMUSRMSG_DRAGDROP_DONE              (WM_BASE +55)
#define WMUSRMSG_UPDATE_COMPONENT           (WM_BASE +56)
#define WMUSRMSG_UPDATE_COMPONENT_ICON      (WM_BASE +57)
#define WMUSRMSG_UPDATE_EVENT               (WM_BASE +58)
#define WMUSRMSG_UPDATE_ALERT               (WM_BASE +59)
#define WMUSRMSG_UPDATE_UNALERT             (WM_BASE +60)
#define WMUSRMSG_UPDATE_SHELLICON           (WM_BASE +61)
#define WMUSRMSG_REPAINT                    (WM_BASE +62)
#define WMUSRMSG_LISTCTRL_EDITING           (WM_BASE +63)
#define WMUSRMSG_VARIABLE_UNSET             (WM_BASE +64)
#define WMUSRMSG_ENABLE_CONTROL             (WM_BASE +65)
#define WMUSRMSG_REINITIALIZE_EVENT         (WM_BASE +66)
#define WMUSRMSG_REACTIVATE_SELECTION       (WM_BASE +67)
#define WMUSRMSG_NAMESERVER_STARTED         (WM_BASE +68)
#define WMUSRMSG_PANE_CREATED               (WM_BASE +69)
#define WMUSRMSG_APP_SHUTDOWN               (WM_BASE +70)
#define WMUSRMSG_GET_STATE                  (WM_BASE +71)
#define WMUSRMSG_ERROR                      (WM_BASE +72)
#define WMUSRMSG_SAVE_DATA                  (WM_BASE +73)
#define WMUSRMSG_ADD_OBJECT                 (WM_BASE +74)
#define WMUSRMSG_FIND                       (WM_BASE +75)
#define WMUSRMSG_FIND_ACCEPTED              (WM_BASE +76)
#define WMUSRMSG_ACTIVATE                   (WM_BASE +77)
#define WMUSRMSG_RUNEXTERNALPROGRAM         (WM_BASE +78)
#define WMUSRMSG_CTRLxSHIFT_UP              (WM_BASE +79)
#define WMUSRMSG_CLEANDATA                  (WM_BASE +80)
#define WMUSRMSG_SQL_FETCH                  (WM_BASE +81)
#define WMUSRMSG_CHANGE_SETTING             (WM_BASE +82)
#define WMUSRMSG_GETFONT                    (WM_BASE +83)
#define WMUSRMSG_LISTCTRL_CHECKCHANGE       (WM_BASE +84)
#define WMUSRMSG_COORPERATIVE_SHUTDOWN      (WM_BASE +85)

#define DBBROWS_TYPE_INTEGER     1 // Type of column is mapped as integer
#define DBBROWS_TYPE_CHARACTER   2 // Type of column is mapped as character
#define DBBROWS_TYPE_DECIMAL     3 // Type of column is mapped as floating point
#define DBBROWS_TYPE_FUNCTION    4 // Type of column is mapped as function. Ex: Date ('11-12-1997')
#define DBBROWS_TYPE_DATE        5 // Type of column is a Date expression
#define DBBROWS_TYPE_MONEY       6 // Type of column is a Money expression
#define DBBROWS_TYPE_HANDLER     7 // Type of column is a Data Handler
#define DBBROWS_TYPE_UNKNOWN     8 // Type of column is unknown


//
// The WMUSRMSG_FIND handler must return the following constants:
#define FIND_RESULT_OK           101 // Something has been found
#define FIND_RESULT_NOTFOUND     102 // Find sth failed
#define FIND_RESULT_END          103 // Reach the end of document (top or bottom)


#endif
