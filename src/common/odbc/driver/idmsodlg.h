/*
**  IDMSODLG.H
**
**  Header file for IDMS ODBC driver dialogs
**
**  (C) Copyright 1993,1998 Ingres Corporation
**
**
**  Change History
**  --------------
**
**  date        programmer          description
**
**  04/29/1994  Dave Ross          Initial coding
**  03/04/1997  Jean Teng          Added IDC_SERVERTYPE
**  03/07/1997  Jean Teng          Changed DLG_IDMS_xxx to DLG_INGRES_xxx
**  11/10/1997  Jean Teng          Added IDC_ROLENAME and IDC_ROLEPWD
**  04/13/1998  Dave Thole         Added GROUP support.
**  12/01/2004  Ralph Loen         Added DBMS password support.
*/

#ifndef _INC_IDMSODLG
#define _INC_IDMSODLG

/*
**  Resource defines for SQLDriverConnect dialog boxes:
*/
#define DLG_INGRES_DSN              100
#define DLG_INGRES_DRIVER           101
#define DLG_UNIX_DSN                102
#define DLG_UNIX_DRIVER             103

#define IDC_DSN                     120
#define IDC_DICT                    121
#define IDC_NODE                    122
#define IDC_TIME                    123
#define IDC_TASK                    124
#define IDC_UID                     125
#define IDC_PWD                     126
#define IDC_ACCT                    127
#undef  IDC_HELP
#define IDC_HELP                    128
#define IDC_SERVERTYPE              129
#define IDC_ROLENAME                130
#define IDC_ROLEPWD                 131
#define IDC_GROUP                   132
#define IDC_ROLENAME_TEXT           133
#define IDC_ROLEPWD_TEXT            134
#define IDC_GROUP_TEXT              135
#define IDC_DBMS_PWD                136
#endif  /* _INC_IDMSODLG */
