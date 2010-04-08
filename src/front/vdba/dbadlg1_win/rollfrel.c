/********************************************************************
//
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
//
//    Project  : CA/OpenIngres Visual DBA
//
//    Source   : rollfRel.c
//
//    "relocate" sub-dialog box for roll forward
//
//
********************************************************************/

#include <stdlib.h>
#include "dll.h"                                  
#include "subedit.h"
#include "dbadlg1.h"
#include "dlgres.h"
#include "getdata.h"
#include "msghandl.h"
#include "domdata.h"
#include "esltools.h"
#include "rmcmd.h"
#include "msghandl.h"
#include "resource.h"

static LPSTRINGLIST prevloclist;
static LPSTRINGLIST replloclist;

// complimentary utility for stringlists
static LPCTSTR StringList_GetAt(LPSTRINGLIST list, int pos);
static BOOL    StringList_SetAt(LPSTRINGLIST list, int pos, LPCSTR string);
static LPSTRINGLIST StringList_RemoveAt(LPSTRINGLIST oldList, int pos);

// controls management
static LPSTRINGLIST dupList(LPSTRINGLIST pSrcList, LPSTRINGLIST pDstList);
static void fillRelocListBox(HWND hwnd, LPSTRINGLIST pPrevLocList, LPSTRINGLIST pReplLocList, int sel);
static void fillLocationsListBoxes(HWND hwnd, LPROLLFWDPARAMS lprollfwd);
static void ScatterRelocSelection(HWND hwnd);

// dialog proc
BOOL CALLBACK __export DlgRollForwardRelocateDlgProc
                   (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

// message handlers
static void OnDestroy     (HWND hwnd);
static void OnCommand     (HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
static BOOL OnInitDialog  (HWND hwnd, HWND hwndFocus, LPARAM lParam);

BOOL WINAPI __export DlgRollForwardRelocate (HWND hwndOwner, LPROLLFWDPARAMS lprollfwd)
{
   FARPROC lpProc;
   int     retVal;

   if (!IsWindow(hwndOwner) || !lprollfwd)
   {
       ASSERT (NULL);
       return FALSE;
   }

   lpProc = MakeProcInstance ((FARPROC) DlgRollForwardRelocateDlgProc, hInst);
   retVal = VdbaDialogBoxParam
       (hResource,
        MAKEINTRESOURCE (IDD_ROLLFWD_RELOCATE),
        hwndOwner,
        (DLGPROC) lpProc,
        (LPARAM)  lprollfwd
       );

   FreeProcInstance (lpProc);
   return (retVal);
}


BOOL CALLBACK __export DlgRollForwardRelocateDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
   switch (message)
   {
       HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
       HANDLE_MSG(hwnd, WM_DESTROY, OnDestroy);

       case WM_INITDIALOG:
           return HANDLE_WM_INITDIALOG(hwnd, wParam, lParam, OnInitDialog);

       default:
           return HandleUserMessages (hwnd, message, wParam, lParam);

   }
   return TRUE;
}


static BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
   LPROLLFWDPARAMS lprollfwd = (LPROLLFWDPARAMS)lParam;

   if (!AllocDlgProp(hwnd, lprollfwd))
       return FALSE;
   
   // initialize locations lists
   ZEROINIT (prevloclist);
   ZEROINIT (replloclist);

   // copy received lists into local lists
   prevloclist = dupList(lprollfwd->lpOldLocations, prevloclist);
   replloclist = dupList(lprollfwd->lpNewLocations, replloclist);

   // fill existing locations lists
   fillLocationsListBoxes(hwnd, lprollfwd);

   // fill relocation list (includes selection scatter if applies)
   fillRelocListBox(hwnd, prevloclist, replloclist, 0);

   // context help
   lpHelpStack = StackObject_PUSH (lpHelpStack, StackObject_INIT ((UINT)IDD_ROLLFWD_RELOCATE));

   // center
   richCenterDialog(hwnd);

   // focus
   if (!prevloclist || !replloclist) {
     SetFocus(GetDlgItem(hwnd, IDC_ROLLFWD_RELOC_NEW));
     return FALSE;    // hand-managed focus
   }
   else
     return TRUE;     // default focus managed by windows
}


static void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
  LPROLLFWDPARAMS lprollfwd = GetDlgProp(hwnd);
  HWND            hList = GetDlgItem(hwnd, IDC_ROLLFWD_RELOC_LIST);
  int             index;
  int             prevcount, replcount;
  
   switch (id)
   {
       case IDOK:

         // check for <unnamed> in local lists
         if (StringList_Search(prevloclist, "<unnamed>")) {
           //"Invalid Location Name <unnamed> in Locations List"
           MessageBox (hwnd, ResourceString(IDS_ERR_LOCATION_LIST) , NULL, MB_ICONEXCLAMATION|MB_OK);
           break;
         }
         if (StringList_Search(replloclist, "<unnamed>")) {
           //"Invalid Location Name <unnamed> in New Locations List"
           MessageBox (hwnd, ResourceString(IDS_ERR_NEW_LOCATION_LIST) , NULL, MB_ICONEXCLAMATION|MB_OK);
           break;
         }

         // copy local lists into received lists
         lprollfwd->lpOldLocations = dupList(prevloclist, lprollfwd->lpOldLocations);
         lprollfwd->lpNewLocations = dupList(replloclist, lprollfwd->lpNewLocations);

         // free local lists
         StringList_Done (prevloclist);
         StringList_Done (replloclist);

         EndDialog(hwnd, TRUE);
         break;

       case IDCANCEL:
         // free local lists
         StringList_Done (prevloclist);
         StringList_Done (replloclist);

         EndDialog(hwnd, FALSE);
         break;

       case IDC_ROLLFWD_RELOC_NEW:
         prevloclist = StringList_Add(prevloclist, "<unnamed>");
         replloclist = StringList_Add(replloclist, "<unnamed>");
         prevcount = StringList_Count(prevloclist);
         replcount = StringList_Count(replloclist);
         ASSERT (prevcount == replcount);
         SetFocus(hList);
         fillRelocListBox(hwnd, prevloclist, replloclist, prevcount-1);
         break;

       case IDC_ROLLFWD_RELOC_DEL:
         index = ListBox_GetCurSel(hList);
         if (index != LB_ERR) {
           int count = ListBox_GetCount(hList);
           int pos = (int)ListBox_GetItemData(hList, index);
           prevloclist = StringList_RemoveAt(prevloclist, pos);
           replloclist = StringList_RemoveAt(replloclist, pos);
           if (index > count-2)
             index = count-2;   // to have a selection after deleting the tail item
           fillRelocListBox(hwnd, prevloclist, replloclist, index);
         }
         break;

       case IDC_ROLLFWD_RELOC_PREVLOC:
       case IDC_ROLLFWD_RELOC_REPLLOC:
         //if (codeNotify == LBN_KILLFOCUS) {
         if (codeNotify == LBN_SELCHANGE) {
           index = ListBox_GetCurSel(hList);
           if (index != LB_ERR) {
             int pos = (int)ListBox_GetItemData(hList, index);
             int index2 = ListBox_GetCurSel(hwndCtl);
             char buf[MAXOBJECTNAME];
             if (index2 != LB_ERR) {
               ListBox_GetText(hwndCtl, index2, buf);
               if (id == IDC_ROLLFWD_RELOC_PREVLOC)
                 StringList_SetAt(prevloclist, pos, buf);
               else
                 StringList_SetAt(replloclist, pos, buf);
             }
             fillRelocListBox(hwnd, prevloclist, replloclist, index);
           }
         }
         break;

       case IDC_ROLLFWD_RELOC_LIST:
         if (codeNotify == LBN_SELCHANGE)
           ScatterRelocSelection(hwnd);
         break;

   }
}


static void OnDestroy(HWND hwnd)
{
   DeallocDlgProp(hwnd);
   lpHelpStack = StackObject_POP (lpHelpStack);
}

static void ScatterRelocSelection(HWND hwnd)
{
  HWND hPrevList = GetDlgItem(hwnd, IDC_ROLLFWD_RELOC_PREVLOC);
  HWND hReplList = GetDlgItem(hwnd, IDC_ROLLFWD_RELOC_REPLLOC);
  HWND hList = GetDlgItem(hwnd, IDC_ROLLFWD_RELOC_LIST);
  int index = ListBox_GetCurSel(hList);
  if (index != LB_ERR) {
    int pos = (int)ListBox_GetItemData(hList, index);
    LPCTSTR prevString = StringList_GetAt(prevloclist, pos);
    LPCTSTR replString = StringList_GetAt(replloclist, pos);
    int     prevIndex;
    int     replIndex;

    ASSERT(prevString);
    ASSERT(replString);
    prevIndex = ListBox_FindStringExact(hPrevList, -1, prevString);
    ListBox_SetCurSel(hPrevList, prevIndex);  // No selection if no match
    replIndex = ListBox_FindStringExact(hReplList, -1, replString);
    ListBox_SetCurSel(hReplList, replIndex);  // No selection if no match
  }
  else {
    ListBox_SetCurSel(hPrevList, -1);     // No selection
    ListBox_SetCurSel(hReplList, -1);     // No selection
  }
}

// duplicate a locations list
static LPSTRINGLIST dupList(LPSTRINGLIST pSrcList, LPSTRINGLIST pDstList)
{
  LPSTRINGLIST lpCur;

  // empty destination list
  StringList_Done (pDstList);
  pDstList = 0;

  lpCur = pSrcList;
  while (lpCur) {
    pDstList = StringList_Add(pDstList, lpCur->lpszString);
    lpCur = lpCur->next;
  }
  return pDstList;
}

// fill relocations lists
static void fillRelocListBox(HWND hwnd, LPSTRINGLIST pPrevLocList, LPSTRINGLIST pReplLocList, int sel)
{
  HWND hList = GetDlgItem(hwnd, IDC_ROLLFWD_RELOC_LIST);
  char buf[MAXOBJECTNAME];
  int  index;
  int  pos;

  ListBox_ResetContent(hList);
  pos = 0;
  while (pPrevLocList && pReplLocList) {
    wsprintf(buf, "%s  --->  %s", pPrevLocList->lpszString, pReplLocList->lpszString);
    index = ListBox_AddString(hList, buf);
    ListBox_SetItemData(hList, index, (LPVOID)pos);

    pos++;
    pPrevLocList = pPrevLocList->next;
    pReplLocList = pReplLocList->next;
  }
  ListBox_SetCurSel(hList, sel);
  ScatterRelocSelection(hwnd);    // force in case selection has not changed
}

// fill locations lists
static void fillLocationsListBoxes(HWND hwnd, LPROLLFWDPARAMS lprollfwd)
{
  HWND hPrevList = GetDlgItem(hwnd, IDC_ROLLFWD_RELOC_PREVLOC);
  HWND hReplList = GetDlgItem(hwnd, IDC_ROLLFWD_RELOC_REPLLOC);
	int hNode;
	int i = 0;
  int nIndex=0;
	int err;
  BOOL bSystem;
	UCHAR szObject[MAXOBJECTNAME];
  UCHAR szFilter[MAXOBJECTNAME];
  BOOL bOK;

	ZEROINIT(szObject);
	ZEROINIT(szFilter);
   
  ListBox_ResetContent(hPrevList);
  ListBox_ResetContent(hReplList);
  bSystem = TRUE;
  hNode = lprollfwd->node;

	err = DOMGetFirstObject(hNode,
									OT_LOCATION,
									0,
									NULL,
									bSystem,
									NULL,
									szObject,
									NULL,
									NULL);
	while (err == RES_SUCCESS){
    // if (1 || DOMLocationUsageAccepted(hNode,szObject,LOCATIONDATABASE,&bOK)==RES_SUCCESS && bOK) {
    if (DOMLocationUsageAccepted(hNode,szObject,LOCATIONDATABASE,&bOK)==RES_SUCCESS && bOK) {
      ListBox_AddString(hPrevList, szObject);
      ListBox_AddString(hReplList, szObject);
    }
		err = DOMGetNextObject (szObject, szFilter, NULL);
	}
}

// complimentary utility for stringlists
static LPCTSTR StringList_GetAt(LPSTRINGLIST list, int pos)
{
  int cpt;

  if (!list) {
    ASSERT (FALSE);
    return 0;     // Abnormal!
  }

  for (cpt = 0; cpt<pos; cpt++) {
    if (!list) {
      ASSERT (FALSE);
      return 0;   // Abnormal!
    }
    list = list->next;
  }

  if (!list) {
    ASSERT (FALSE);
    return 0;     // Abnormal!
  }
  return list->lpszString;
}

static BOOL StringList_SetAt(LPSTRINGLIST list, int pos, LPCSTR string)
{
  int cpt;

  if (!list) {
    ASSERT (FALSE);
    return FALSE;     // Abnormal!
  }

  for (cpt = 0; cpt<pos; cpt++) {
    if (!list) {
      ASSERT (FALSE);
      return FALSE;     // Abnormal!
    }
    list = list->next;
  }
  if (!list) {
    ASSERT (FALSE);
    return FALSE;     // Abnormal!
  }
  ESL_FreeMem(list->lpszString);
  list->lpszString = (LPTSTR)ESL_AllocMem (lstrlen(string) +1);
  x_strcpy(list->lpszString, string);
  return TRUE;
}

static LPSTRINGLIST StringList_RemoveAt(LPSTRINGLIST oldList, int pos)
{
    int cpt = 0;
    LPSTRINGLIST ls     = oldList;
    LPSTRINGLIST lpPrec = NULL, tmp;

    while (ls)
    {
        if (cpt == pos)
        {
            tmp = ls;
            if (lpPrec == NULL)             // Remove the head of the list 
                oldList = oldList->next;
            else
                lpPrec->next = ls->next;    // Remove somewhere else
            
            ESL_FreeMem (tmp->lpszString);
            ESL_FreeMem (tmp);
            return oldList;
        }
        lpPrec = ls;
        ls = ls->next;
        cpt++;
    }
    return oldList;

}
