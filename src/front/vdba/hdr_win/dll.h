/********************************************************************
//
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
//
//    Project  : CA/OpenIngres Visual DBA
//
//    Source   : dll.h
//
//    
//
**  27-Mar-2001 (noifr01)
**   (sir 104270) removal of code for managing Ingres/Desktop
**  05-Jun-2002 (noifr01)
**   (bug 107773) removed prototype of no more used VerifyDBName()
**   function
**  18-Mar-2003 (schph01)
**   sir 107523 management of sequences
**  06-May-2010 (drivi01)
**   Add ComboBoxFillTablesFiltered which will load combo box
**   of tables with either Ingres or VW tables depending
**   on which are requested.
********************************************************************/

#ifndef _DLL_H
#define _DLL_H

// #define STRICT
#include <windows.h>
#include <windowsx.h>

#include <string.h>
#include <assert.h>

#include "dbadlg1.h"

#include "dba.h"
#include "dbaset.h"
#include "main.h"

#define MAX_TITLEBAR_LEN   180         // Max text length of Dialog box title
#define MAX_TEXT_EDIT      2000        // Max text length of Multi-line edit
//
// We define an array of 8 strings whose lenght is 580 characters
//
#define MAX_STRINGARRAY    8
#define MAX_STRINGLENGTH   580  // IDS_I_NO_ROWS_WITH_UNLOADDB uses something like 536


//  Maximum length of a string used in a check constraint
#define MAX_CHECKSTRING    1500

//#define _USE_CTL3D

#ifdef _USE_CTL3D
#include "ctl3d.h"
#endif

#ifndef __cplusplus
#ifdef NDEBUG
	#define ASSERT(exp) ((void)0)
#else
	#define ASSERT(exp) assert(exp)
#endif
#endif

#define DARK_GREY    RGB(127,127,127)

// extern HINSTANCE hInst;		// Instance of the application

// ************************************************************************

// Common structure used to form tables
// of strings to be loaded into comboboxes, listboxes etc...
typedef struct tagCTLDATA
{
	int nId;	       			// Item ID
	int nStringId;	      	// Resource string ID
} CTLDATA, FAR * LPCTLDATA;

typedef enum tagEDITACTION
{
	EC_SUBCLASS,
	EC_RESETSUBCLASS,
	EC_LIMITTEXT,
	EC_VERIFY,
	EC_UPDATESPIN
} EDITACTION;

typedef struct tagEDITACTION_VERIFY
{
	BOOL bShowError;
	BOOL bResetLimit;
} EDITACTION_VERIFY, FAR * LPEDITACTION_VERIFY;

typedef struct tagENUMEDIT
{
	EDITACTION editAction;
	union
	{
		EDITACTION_VERIFY verify;
	} action;
} ENUMEDIT, FAR * LPENUMEDIT;

typedef struct tagDLGPROP
{
	LPVOID lpv;			// Points to data structure for dialog
} DLGPROP, FAR * LPDLGPROP;
#define SZDLGPROP		"DLGPROP"

//********************************************************************
// Function Prototypes

void ErrorBox (HWND hwnd, int id, UINT fuStyle);
void ErrorBox1 (HWND hwnd, int id, UINT fuStyle, DWORD dw1);
void ErrorBox2 (HWND hwnd, int id, UINT fuStyle, DWORD dw1, DWORD dw2);
void ErrorBoxString(HWND hwnd, int id, UINT fuStyle, LPSTR lpszArg);
BOOL OccupyComboBoxControl (HWND hwndCtl, LPCTLDATA lpdata);
BOOL OccupyCAListBoxControl (HWND hwndCtl, LPCTLDATA lpdata);
BOOL SelectComboBoxItem(HWND hwndCtl, LPCTLDATA lpdata, int nId);

BOOL VerifyVNodeName(HWND hwnd, LPSTR lpszVNode, BOOL bShowError);
BOOL VerifyUserName(HWND hwnd, LPSTR lpszUser, BOOL bShowError);
BOOL VerifyGroupName(HWND hwnd, LPSTR lpszGroup, BOOL bShowError);
BOOL VerifyIndexName(HWND hwnd, LPSTR lpszIndex, BOOL bShowError);
BOOL VerifyListenAddress(HWND hwnd, LPSTR lpszListen, BOOL bShowError);
void EditIncrement(HWND hwndCtl, int nIncrement);

BOOL VerifyNumericEditControl (HWND hwnd, HWND hwndCtl, BOOL bShowError, BOOL bResetLimit, DWORD dwMin, DWORD dwMax);
BOOL IsEditControl(HWND hwnd, HWND hwndTest);
void my_ftoa(double dValue, char FAR * lpszBuffer, size_t nBufSize, UINT uCount);
BOOL CALLBACK __export EnumNumericEditCntls (HWND hwnd, LPARAM lParam);
BOOL VerifyAllNumericEditControls(HWND hwnd, BOOL bShowError, BOOL bResetLimit);
void SubclassAllNumericEditControls(HWND hwnd, EDITACTION ea);
void LimitNumericEditControls(HWND hwnd);
HWND GetSpinEditControl (HWND hwndSpin, LPEDITMINMAX lplimits);
BOOL HandleUserMessages(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
void UpdateSpinButtons(HWND hwnd);
void CAListBox_SelectItems(HWND hwndCtl, UINT uFlags, LPCTLDATA lpdata);
BOOL AllocDlgProp(HWND hwnd, LPVOID lpv);
void DeallocDlgProp(HWND hwnd);
LPVOID GetDlgProp(HWND hwnd);
BOOL VerifyDeviceName(HWND hwnd, LPSTR lpszDevice, BOOL bShowError);
HWND GetAppWindow(HWND hwnd);
void richCenterDialog(HWND hdlg);
LPOBJECTLIST CreateList4CheckedObjects(HWND hwndCtl);
void Rectangle3D(HDC hdc, int nLeft, int nTop, int nRight, int nBottom);
void DrawCheck3D(HDC hdc, int nLeft, int nTop, int nRight, int nBottom, BOOL bGrey);
BOOL VerifyObjectName (HWND hwnd,
                       LPSTR lpszObjectName,
                       BOOL bShowError,
                       BOOL bStartII,
                       BOOL bStartUnderscore);

void FillGrantees          (HWND hwndCtl, int GranteeType); 
void PreselectGrantee      (HWND hwndCtl, const char* selectString);
BOOL IsValidObjectChar(UINT ch);
LPOBJECTLIST refcmp(LPOBJECTLIST lplist, LPREFERENCEPARAMS lpRefParams);
LPOBJECTLIST FindPrevObject(LPOBJECTLIST lplist, LPOBJECTLIST lpcurrent);

/*
// src is the forme '[XXXXX] yyyyy'
// 
// This function copy the sub string 'yyyyy' to the buffer 'dest'
// if src does not contain [] then dest will be the same as src.
*/
void ExcludeBraceString    (LPUCHAR dest, const char* src);


LPOBJECTLIST AddItemToList         (HWND hwndCtl);
LPOBJECTLIST AddItemToListWithOwner(HWND hwndCtl);
LPOBJECTLIST AddItemToListQuoted(HWND hwndCtl);
LPOBJECTLIST APublicUser ();

LPCHECKEDOBJECTS DupList               (const LPCHECKEDOBJECTS a);
LPCHECKEDOBJECTS FindStringObject      (const LPCHECKEDOBJECTS a, const char* szFind);
LPCHECKEDOBJECTS Union                 (const LPCHECKEDOBJECTS a, const LPCHECKEDOBJECTS b);
LPCHECKEDOBJECTS Intersection          (const LPCHECKEDOBJECTS a, const LPCHECKEDOBJECTS b);
LPCHECKEDOBJECTS Difference            (const LPCHECKEDOBJECTS a, const LPCHECKEDOBJECTS b);
LPCHECKEDOBJECTS AddCheckedObjectTail  (LPCHECKEDOBJECTS       lc,LPCHECKEDOBJECTS obj);

void CAListBoxFillTableColumns         (HWND hwndCtl, LPUCHAR DBName, LPUCHAR TableName);
BOOL CAListBoxFillTableColumns_x_Types (HWND hwndCtl, LPUCHAR DBName, LPUCHAR TableName);
BOOL CAListBoxFillViewColumns_x_Types  (HWND hwndCtl, LPUCHAR DBName, LPUCHAR ViewName);

BOOL CAListBoxFillTables    (HWND hwndCtl, LPUCHAR DatabaseName);
void CAListBoxFillViewColumns  (HWND hwndCtl, LPUCHAR DBName, LPUCHAR ViewName);
void ComboBoxFillUsers      (HWND hwndCtl);
void CAListBoxFillUsers     (HWND hwndCtl);
void ComboBoxFillGroups     (HWND hwndCtl);
void ComboBoxFillProfiles   (HWND hwndCtl);
void CAListBoxFillGroups    (HWND hwndCtl);
void CAListBoxFillProfiles  (HWND hwndCtl);
void ComboBoxFillRoles      (HWND hwndCtl);
void CAListBoxFillRoles     (HWND hwndCtl);
void ComboBoxFillDatabases  (HWND hwndCtl);
void CAListBoxFillDatabases (HWND hwndCtl);
BOOL ComboBoxFillProcedures (HWND hwndCtl, LPUCHAR DatabaseName);
BOOL CAListBoxFillProcedures(HWND hwndCtl, LPUCHAR DatabaseName);
BOOL CAListBoxFillSequences (HWND hwndCtl, LPUCHAR DatabaseName);
BOOL ComboBoxFillDBevents   (HWND hwndCtl, LPUCHAR DatabaseName);
BOOL CAListBoxFillDBevents  (HWND hwndCtl, LPUCHAR DatabaseName);
BOOL ComboBoxFillTables     (HWND hwndCtl, LPUCHAR DatabaseName);
BOOL ComboBoxFillTablesFiltered	(HWND hwndCtl, LPUCHAR DatabaseName, BOOL bVW);
BOOL ComboBoxFillViews      (HWND hwndCtl, LPUCHAR DatabaseName);
BOOL CAListBoxFillViews     (HWND hwndCtl, LPUCHAR DatabaseName);
void ComboBoxSelectFirstStr (HWND hwndCtl);
void CAListBoxSelectFirstStr(HWND hwndCtl);
void ComboBoxFillLocations  (HWND hwndCtl);
void CAListBoxFillLocations (HWND hwndCtl);
void CAListBoxSelectStringWithOwner (HWND hwndCtl, const char* aString, char* aOwner);
void ComboBoxSelectStringWithOwner  (HWND hwndCtl, const char* aString, char* aOwner);

void ComboBoxDestroyItemData (HWND hwndCtl);
void CAListBoxDestroyItemData(HWND hwndCtl);

//
// The user call public is added into the list
//
void ComboBoxFillUsersP    (HWND hwndCtl);
void CAListBoxFillUsersP   (HWND hwndCtl);
char* ResourceString       (UINT idString);


// Database interface functions
BOOL ListBox_FillWindowNames(HWND hwndCtl, LPSTR lpszServer);
BOOL CAListBox_FillObjects(HWND hwndCtl);
BOOL GetObjectString(int id, LPSTR lpszBuffer, size_t cMax);
void ScreenToClientRect(HWND hwndClient, LPRECT lprect);
void SelectAllCALBItems(HWND hwnd, BOOL bSelect, int nIdx);

// CREATTBL.C
void GetFocusCellRect(HWND hwndCnt, LPRECT lprect);


/* ---------------------------------------------------- NoLeadingEndingSpace -*/
/*                                                                            */
/* Eliminate the leading and ending space of a string.                        */
/*                                                                            */
/*     1) aString      : The string you want to remove leading and ending     */
/*                       space out of it.                                     */
/*                                                                            */
/* ---------------------------------------------------------------------------*/
void
NoLeadingEndingSpace (char* aString);


//********************************************************************

//
// Star management
//
void ProdNameFromProdType(DWORD dwtype, char *lpSzName);

#endif		// _DLL_H
