/********************************************************************
**
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Project  : Visual DBA
**
**    Source   : cddsv11.c
**
**    Add / modify replication CDDS
**
**
**
** History:
**
**  04-May-99 (schph01)
**    when registering a table in a CDDS (thus registering directly the 
**    appropriate columns), without creating the support objects,  the
**    "column_registered" column in DD_REGIST_TABLE was not updated, resulting
**    in repmgr to display that the table was not registered.
**    The current change updates the corresponding element in the structure, in
**    order for the low level to work accordingly in this case
**  06-May-99 (schph01)
**    forces initialization of the default "Error Mode" and "Collision Mode".
**    to values consistent with those of repmgr ("Skip transaction" and 
**    "Passive Detection") (this second one already appeared correctly because
**    of the sort order, but could have become wrong after translation).
**  21-Jul-1999 (schph01)
**    bug #97685 :
**    VDBA should not accept negative values for a CDDS number when
**    creating a new replicator CDDS.
**  20-Jan-2000 (uk$so01)
**    bug #100063
**     Eliminate the undesired compiler's warning
**  06-Mar-2000 (schph01 and noifr01) 
**    bug #100750. PRovide a warning when registering a table with
**    nullable columns in the replication key
**  15-Jun-2000 (schph01)
**    bug #101846 change the buffer size for message
**    IDS_I_ADD_DB_CDDS_KEEP_DEF
**  23-May-2000
**    bug #99242 . cleanup for DBCS compliance
**  05-May-2003
**    bug #110176 Change the Help ID launch when the F1 key is pressed.
**  10-Jun-2005
**    Bug #114699/Issue#14115145 
**    Added a check, not do display messages if the columns of a 
**    table are already registered.
**  02-Apr-2009 (drivi01)
**    Clean up warnings in this file.
********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <tchar.h>
#include "dll.h"
#include "subedit.h"
#include "dbadlg1.h"
#include "dlgres.h"
#include "catolist.h"
#include "getdata.h"
#include "msghandl.h"
#include "lexical.h"

#include "catolist.h"
#include "catospin.h"
#include "catocntr.h"

#include "domdata.h"
#include "dbaginfo.h"
#include "error.h"
#include "winutils.h" // for hourglass fonction
#include "resource.h"

static BOOL OccupyTypesErrors(HWND hwnd);
static BOOL OccupyTypesCollisions(HWND hwnd);

static void PathCntOnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
static void ColCntOnLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags);
static void ColCntOnChar(HWND hwnd, TCHAR ch, int cRepeat);
static void TableCheckChange(HWND hwnd,HWND hwndCtl,int id, BOOL bStatusChanging);
static void CreateDatabaseString(LPSTR lpDest,LPDD_CONNECTION lpconn ,HWND hwnd);

extern BOOL MfcAddDatabaseCdds( LPREPCDDS lpcdds ,int nHandle );

// Prototyped Emb 15/11/95 (force columns data in linked list)
static void  GetColumnsParams(HWND hwnd,LPDD_REGISTERED_TABLES lptable);

CTLDATA typeTypesRepl[] =
{
   REPLIC_FULLPEER,    IDS_FULLPEER,
   REPLIC_PROT_RO,     IDS_PROTREADONLY,
   REPLIC_UNPROT_RO,   IDS_UNPROTREADONLY,
   REPLIC_EACCESS,     IDS_GATEWAY,
   -1                  // terminator
};
/*
#define REPLIC_COLLISION_PASSIVE    0
#define REPLIC_COLLISION_ACTIVE     1
#define REPLIC_COLLISION_BEGNIN     2
#define REPLIC_COLLISION_PRIORITY   3
#define REPLIC_COLLISION_LASTWRITE  4

*/
CTLDATA typeCollisions[] =
{
   REPLIC_COLLISION_PASSIVE,    IDS_COLLISION_PASSIVE,
   REPLIC_COLLISION_ACTIVE,     IDS_COLLISION_ACTIVE,
   REPLIC_COLLISION_BEGNIN,     IDS_COLLISION_BEGNIN,
   REPLIC_COLLISION_PRIORITY,   IDS_COLLISION_PRIORITY,
   REPLIC_COLLISION_LASTWRITE,  IDS_COLLISION_LASTWRITE,
   -1                  // terminator
};
/*
#define REPLIC_ERROR_SKIP_TRANSACTION 0
#define REPLIC_ERROR_SKIP_ROW         1
#define REPLIC_ERROR_QUIET_CDDS       2
#define REPLIC_ERROR_QUIET_DATABASE   3
#define REPLIC_ERROR_QUIET_SERVER     4
*/
CTLDATA typeErrors[] =
{
   REPLIC_ERROR_SKIP_TRANSACTION,    IDS_ERROR_SKIP_TRANSACTION,
   REPLIC_ERROR_SKIP_ROW,            IDS_ERROR_SKIP_ROW,
   REPLIC_ERROR_QUIET_CDDS,          IDS_ERROR_QUIET_CDDS,
   REPLIC_ERROR_QUIET_DATABASE,      IDS_ERROR_QUIET_DATABASE,
   REPLIC_ERROR_QUIET_SERVER,        IDS_ERROR_QUIET_SERVER,
   -1                  // terminator
};

/* Pointer to the field drawing procedure for the container */
static DRAWPROC lpfnDrawProc = NULL;

/* structures used for containers */

/* used by col container */
typedef struct tagCNTCOLDATA
{
    WNDPROC fp;                       /* for subclassing mouse clicks */
    LPDD_REGISTERED_TABLES lptable;   /* To add, remove cols */
} CNTCOLDATA,FAR *LPCNTCOLDATA;

/* used by path container */
typedef struct tagCNTPATHDATA
{
    HWND hcomboorig;
    HWND hcombolocal;
    HWND hcombotarget;
    WNDPROC fp;                       /* for subclassing combo selection changes */
} CNTPATHDATA,FAR *LPCNTPATHDATA;


#define PATHS_LENGTH ((MAXOBJECTNAME*2)+8)  /* (17) MyNode::MyDB */

typedef struct tagPATHS
{
    char original[PATHS_LENGTH];
    char local   [PATHS_LENGTH];
    char target  [PATHS_LENGTH];
    LPOBJECTLIST lpOL;
} PATHS, FAR *LPPATHS;

typedef struct tagCOLS
{
    char name[MAXOBJECTNAME];
    char repkey[2];
    char repcol[2];
    int  column_sequence; 
    int  ikey;
    BOOL bCanBeUnregistered;
} COLS, FAR *LPCOLS;

typedef struct tagDATABASESCDDS
{
    int   dbNo;                             // Server id (server role)
    UCHAR szVNode  [MAXOBJECTNAME];         // Virtual node name
    UCHAR szDBName [MAXOBJECTNAME];         // Database name on target machine
    int   nType;                            // Type of replication to be 
    int   nServer;                          // Server id (server role)
    LPOBJECTLIST lpOL;
} DATABASESCDDS, FAR *LPDATABASESCDDS;

static BOOL bInDlg = FALSE;
static BOOL bFirstCall = FALSE;
#define SPECIALDISPLAY "* "
#define MAXSPACES 50
#define MAX_LEN_EDIT  32
#define SPECIALSEPARATOR "                                                  "

#ifndef WIN32
static void StringAdjusted2Pixel(char * bufsrc, char * bufdest , WORD imaxlen, HDC hDC)
#else
static void StringAdjusted2Pixel(char * bufsrc, char * bufdest , long imaxlen, HDC hDC)
#endif
/*
//
// Function:  
//
// Parameters:
//    bufsrc  - Buffer source 
//    bufdest - Buffer destination - if this buffer is not empty 
//              concatenation with the source buffer.  
//    imaxlen - maximun length for the string   
//    hDC     - Handle HDC     
// Returns:
//    void
//
*/
{
    TCHAR  buf[MAXOBJECTNAME*4];
    BOOL  bFind=TRUE;
    DWORD dwtab,dwbufLen,dwspace;
    WORD  wlen,wtab,wspace,wmaxspacelen,nbspace;

    ZEROINIT(buf);

    if (x_strlen(bufdest) == 0) {
      x_strcpy(buf,bufsrc);
    }
    else  {
      x_strcpy(buf,bufdest);
      x_strncat(buf,bufsrc,x_strlen(bufsrc));
    }
    
    // width of one Tab
    dwtab = GetTextExtent(hDC, "\t", 1);
    wtab  = LOWORD(dwtab);
    
    // width of one space
    dwspace = GetTextExtent(hDC, " ", 1);
    wspace  = LOWORD(dwspace);

    // width of string global buffer
    dwbufLen = GetTextExtent(hDC, buf, x_strlen(buf));
    wlen     = LOWORD(dwbufLen);

    if ( (wlen+wtab) < imaxlen) { 
        // number of space in string destination
        wmaxspacelen = (WORD)imaxlen - (wlen+wtab);
        nbspace      = wmaxspacelen / wspace;

        if (x_strlen(bufdest) == 0)
          memset(buf+x_strlen(bufsrc),0x20,nbspace );
        else
          memset(buf+x_strlen(bufsrc)+x_strlen(bufdest),0x20,nbspace);
    }
    else {

        while ((wlen+wtab) > imaxlen) {
          TCHAR *tcTemp;
          // obtain a TCHAR pointer to the last "logical character" of a string
          tcTemp = _tcsdec(buf,buf+_tcslen(buf));
          // and remove this character
          if (tcTemp != NULL)
            *tcTemp = _T('\0');

          dwbufLen = GetTextExtent(hDC, buf, _tcslen(buf));
          wlen     = LOWORD(dwbufLen);
        }
    }

    x_strcat(buf,"\t");

    x_strcpy(bufdest,buf);
}

static void EnableDisableAddDropDBbutton(HWND hwnd)
{
    DWORD MaxCount,maxItem,i;
    char ach[MAXOBJECTNAME*4];
    char *Index;
    HWND hCntPath        = GetDlgItem(hwnd,ID_PATHCONTAINER);
    HWND hCntPathAdd     = GetDlgItem(hwnd,ID_ADDPATH);
    HWND hwndDBCdds      = GetDlgItem(hwnd,ID_DATABASES_CDDS);
    HWND hwndDBCddsAdd   = GetDlgItem(hwnd,ID_ADD_DATABASES_CDDS);
    HWND hwndDBCddsAlter = GetDlgItem(hwnd,ID_ALTER_DATABASES_CDDS);
    HWND hwndDBCddsDrop  = GetDlgItem(hwnd,ID_DROP_DATABASES_CDDS);

    maxItem = ListBox_GetCount(hwndDBCdds);
    MaxCount = CntTotalRecsGet(hCntPath);

    if (maxItem==0)
    {
        EnableWindow(hwndDBCddsDrop , FALSE);
        EnableWindow(hwndDBCddsAlter, FALSE);
        EnableWindow(hCntPathAdd    , TRUE);

        if (MaxCount)
            EnableWindow(hwndDBCddsAdd  , FALSE);
        else
        {
            EnableWindow(hwndDBCddsAdd  , TRUE);
            EnableWindow(hCntPathAdd    , TRUE);
        }
    }
    else
    {
        for (i=0;i<maxItem;i++)
        {
            ListBox_GetText(hwndDBCdds, i , ach);
            Index = x_strstr(ach,SPECIALDISPLAY);
            if (Index != NULL)
                break;
        }
        if (Index != NULL)
        {
            EnableWindow(hwndDBCddsAlter, FALSE);
            EnableWindow(hwndDBCddsAdd  , FALSE);
            EnableWindow(hwndDBCddsDrop , TRUE);
            EnableWindow(hCntPathAdd    , FALSE);
        }
        else
        {
            EnableWindow(hwndDBCddsAlter, TRUE);
            if (MaxCount)
                EnableWindow(hwndDBCddsAdd  , FALSE);
            else
                EnableWindow(hwndDBCddsAdd  , TRUE);
            EnableWindow(hwndDBCddsDrop , FALSE);
            EnableWindow(hCntPathAdd    , TRUE);
        }
    }
}

static void AddDatabasesCdds(HWND hwnd)
{
    int iret,id1;
    char lpDest[MAXOBJECTNAME*4];
    LPOBJECTLIST  lpOConnect;
    HWND hCntDatabase = GetDlgItem(hwnd,ID_DATABASES_CDDS);
    LPREPCDDS lpcdds = (LPREPCDDS)GetDlgProp (hwnd);
    LPDD_CONNECTION dbcdds;
    ZEROINIT (lpDest);
    iret = MessageBox(GetFocus(),ResourceString(IDS_I_ADD_DB_CDDS_SPECIAL_HOR_PAR),
                      NULL, MB_ICONQUESTION | MB_YESNO |MB_TASKMODAL);
        if (iret == IDNO)
           return;

    if ( MfcAddDatabaseCdds(lpcdds,GetCurMdiNodeHandle ()) )
    {
        lpOConnect  = lpcdds->connections;
        while(lpOConnect)
        {
            dbcdds = lpOConnect->lpObject;
            if (dbcdds->bConnectionWithoutPath)
                break;
            lpOConnect=lpOConnect->lpNext;
        }
        CreateDatabaseString(lpDest,dbcdds,hwnd);
        id1=ListBox_AddString(hCntDatabase, lpDest);
        if (id1 != LB_ERR && id1 != LB_ERRSPACE)
        {
            ListBox_SetItemData(hCntDatabase, id1, dbcdds->dbNo);
            ListBox_SetCurSel(hCntDatabase,id1);
        }
    }
}

static void DropDatabasesCdds(HWND hwnd)
{
    int id,nodb;
    LPREPCDDS lpcdds = (LPREPCDDS)GetDlgProp (hwnd);
    LPDD_CONNECTION lpconn;
    LPOBJECTLIST  lpOConnect;
    HWND hCntDatabase = GetDlgItem(hwnd,ID_DATABASES_CDDS);

    id = ListBox_GetCurSel(hCntDatabase);
    if (id==LB_ERR)
      return;
    nodb = ListBox_GetItemData(hCntDatabase, id);
    lpOConnect  = lpcdds->connections;
    while (lpOConnect)  {
      lpconn    = lpOConnect->lpObject;
      if (lpconn->dbNo == (int)nodb &&
          lpconn->CDDSNo == (int) (lpcdds->cdds)&& 
          lpconn->bConnectionWithoutPath)
      {
          if (lpconn->bElementCanDelete)
              lpcdds->connections=DeleteListObject(lpcdds->connections, lpOConnect);
          else
          {
              lpconn->bIsInCDDS              = FALSE;
              lpconn->bConnectionWithoutPath = FALSE;
          }
        ListBox_DeleteString(hCntDatabase, id);
        break;
      }
      lpOConnect=lpOConnect->lpNext;
    }
}

static void AlterDatabasesCdds(HWND hwnd)
{
    DWORD id,nodb,id1,id2;
    int iret=TRUE;
    char lpDest[MAXOBJECTNAME*4];
    DD_CONNECTION prevconndata;

    LPREPCDDS lpcdds          = (LPREPCDDS)GetDlgProp (hwnd);
    LPOBJECTLIST  lpOConnect  = lpcdds->connections;
    LPDD_CONNECTION dbcdds    = (LPDD_CONNECTION) lpOConnect->lpObject;
    LPDD_CONNECTION lpconn    = (LPDD_CONNECTION) lpOConnect->lpObject;
    HWND hCntDatabase = GetDlgItem(hwnd,ID_DATABASES_CDDS);
    ZEROINIT (lpDest);

    id = ListBox_GetCurSel(hCntDatabase);
    if (id==LB_ERR)
      return;
    nodb = ListBox_GetItemData(hCntDatabase, id);
    while (lpOConnect)  {
      lpconn    = lpOConnect->lpObject;
      if (lpconn->dbNo == (int)nodb && lpconn->CDDSNo == (int) (lpcdds->cdds))
        dbcdds = lpOConnect->lpObject;
      lpOConnect=lpOConnect->lpNext;
    }
    prevconndata=*dbcdds;
    iret=DlgReplServer(hwnd,dbcdds);
    if (!iret)
      return;
    if (prevconndata.nTypeRepl!=dbcdds->nTypeRepl)
        dbcdds->bTypeChosen=TRUE;
    if (prevconndata.nServerNo!=dbcdds->nServerNo)
        dbcdds->bServerChosen=TRUE;
    CreateDatabaseString(lpDest,dbcdds,hwnd);

    id2 = ListBox_DeleteString(hCntDatabase, id);
    if (id2 != LB_ERR) {
        id1=ListBox_InsertString(hCntDatabase, id, lpDest);
        if (id1 != LB_ERR && id1 != LB_ERRSPACE)  {
           ListBox_SetItemData(hCntDatabase, id1, nodb);
           ListBox_SetCurSel(hCntDatabase,id1);
        }
    }
}

static void EnableDisableOKbutton(HWND hwnd)
/*
//
// Function:
//     disable/enable OK button when cdds_no or cdds_name are empty
//     or the word "UNDEFINED" is in the listbox database 
//     information for cdds 
//
// Parameters:
//     hwnd - Handle to the dialog window.
//
// Returns:
//    void
//
*/
{
    DWORD edLen1,edLen2,maxItem,i;
    char *iIndex;
    char ach[MAXOBJECTNAME*4];
    HWND hwndDBCdds   = GetDlgItem(hwnd,ID_DATABASES_CDDS);
    HWND hwndCDDSNum  = GetDlgItem(hwnd,ID_CDDSNUM);
    HWND hwndNameCDDS = GetDlgItem(hwnd,ID_NAME_CDDS);
    HWND hCntPath     = GetDlgItem(hwnd,ID_PATHCONTAINER);
    HWND hwndOK       = GetDlgItem(hwnd,IDOK);


    edLen1=Edit_GetTextLength(hwndCDDSNum);

    edLen2=Edit_GetTextLength(hwndNameCDDS);

    if (!edLen1 || !edLen2 )
    {
        EnableWindow(hwndOK, FALSE);
        return;
    }

    maxItem = ListBox_GetCount(hwndDBCdds);

    for (i=0,iIndex=NULL;i<maxItem;i++) {
        ListBox_GetText(hwndDBCdds, i , ach);
        iIndex = x_strstr(ach,"Undefined");
        if (iIndex != NULL) {
          EnableWindow(hwndOK, FALSE);
          return;
        }
    }
    EnableWindow(hwndOK, TRUE);
}

static int GetIndexForInsertString(HWND hwndCtl,DWORD ItemValue)
/*
// Function:
//     Get the item data and compare with ItemValue
//
// Parameters:
//     hwnd      - Handle to the listbox control.
//     ItemValue - item data for the next item in the listbox.
// Returns:
//     return the index where you should insert this item in the listbox.
*/
{
  DWORD ItemMax,value,i;

  ItemMax = ListBox_GetCount(hwndCtl);
  if (ItemMax == LB_ERR)
     return LB_ERR;

  for (i=0;i<ItemMax;i++) {
    value = ListBox_GetItemData(hwndCtl, i);
    if (value > ItemValue)
      return i;
  }
  return i;
}


static DWORD FindItemDatabaseCdds(HWND hwndCtl,DWORD ItemValue)
/*
// Function:
//     Find the item data associated with the item in 
//     listbox Database information for CDDS .version 1.1
//
// Parameters:
//     hwnd      - Handle to the listbox control.
//     ItemValue - item data to be find in the listbox.
// Returns:
//     if found return the index else return LB_ERR.
*/
{
  DWORD ItemMax,value;
  DWORD i;
  ItemMax=ListBox_GetCount(hwndCtl);
  for (i=0;i<ItemMax;i++) {
    value = ListBox_GetItemData(hwndCtl, i);
    if (value == ItemValue)
      return i;
  }
  return (DWORD)LB_ERR;
}
/*
// Function:
//     Find the item data associated with the item in 
//     Combobox for Collision Mode and Error Mode.version 1.1
//
// Parameters:
//     hwnd      - Handle to the Combobox control.
//     ItemValue - item data to be find in the listbox.
// Returns:
//     if found return the index else return CB_ERR.
*/

static DWORD FindItemData( HWND hwndCtl, DWORD ItemValue)
{
  DWORD ItemMax ,value;
  DWORD i;
  ItemMax=ComboBox_GetCount(hwndCtl);
  for (i=0;i<ItemMax;i++) {
    value = ComboBox_GetItemData(hwndCtl, i);
    if (value == ItemValue)
      return i;
  }
  return (DWORD)CB_ERR;
}

static void CreateDatabaseString(LPSTR lpDest,LPDD_CONNECTION lpconn ,HWND hwnd)
/*
// Function:
//     Create a string for listbox databases information for CDDS .
//     replicatoir version 1.1
//
// Parameters:
//     lpDest          - destination string.
//     LPDD_CONNECTION - structure.
//
// Returns:
//     void.
*/
{
   char Descr[MAXOBJECTNAME];
   char szKey[MAXOBJECTNAME*2];
   char szBufDest[MAXOBJECTNAME*4];
#ifndef WIN32
   WORD BaseDBnum,BaseVnode,BaseType,BaseServer;
#else
   long BaseDBnum,BaseVnode,BaseType,BaseServer;
#endif
   HDC CurDC;
   RECT CurRect;

   if (!lpDest)
     {
        ASSERT(NULL);
        return;
     }

   ZEROINIT(szBufDest);

   CurDC = GetDC(GetDlgItem(hwnd,ID_DATABASES_CDDS));

   if ( CurDC == (HDC) NULL)
      return;

   GetWindowRect(GetDlgItem(hwnd,IDC_DB_STATIC)     , &CurRect);
   BaseDBnum=CurRect.left;

   GetWindowRect(GetDlgItem(hwnd,IDC_VNODE_STATIC)  , &CurRect);
   BaseVnode=CurRect.left;

   GetWindowRect(GetDlgItem(hwnd,IDC_TYPE_STATIC)   , &CurRect);
   BaseType =CurRect.left;

   GetWindowRect(GetDlgItem(hwnd,IDC_SERVER_STATIC) , &CurRect);
   BaseServer =CurRect.left;


   // database number width
   itoa(lpconn->dbNo,szKey,10);
   StringAdjusted2Pixel(szKey, szBufDest, BaseVnode-BaseDBnum, CurDC);


  // if (x_strcmp(lpconn->szVNode,"mobile")==0)
  //    lpconn->nServerNo=1;
   if (lpconn->bConnectionWithoutPath)
      wsprintf(szKey,"%s%s::%s",SPECIALDISPLAY,lpconn->szVNode, lpconn->szDBName);
   else
      wsprintf(szKey,"%s::%s",lpconn->szVNode, lpconn->szDBName);
   StringAdjusted2Pixel(szKey, szBufDest, BaseType-BaseDBnum, CurDC);


   switch (lpconn->nTypeRepl) {
      case REPLIC_FULLPEER :
         x_strcpy(Descr,ResourceString(IDS_REPLIC_FULL_PEER));
      break;
      case REPLIC_PROT_RO  :
         x_strcpy(Descr,ResourceString(IDS_REPLIC_PROT_RO));
      break;
      case REPLIC_UNPROT_RO:
         x_strcpy(Descr,ResourceString(IDS_REPLIC_UNPROT_RO));
      break;
      case REPLIC_TYPE_UNDEFINED:
         x_strcpy(Descr,ResourceString(IDS_REPLIC_UNDEFINED));
      break;
   }
   StringAdjusted2Pixel(Descr, szBufDest, BaseServer-BaseDBnum, CurDC);

   x_strcpy(lpDest,szBufDest);

   itoa(lpconn->nServerNo,szKey,10);
   x_strcat(lpDest,szKey);

   ReleaseDC(GetDlgItem(hwnd,ID_DATABASES_CDDS),CurDC);
}

static void CreatePathString(LPSTR lpDest,LPDD_CONNECTION lpconn)
{
    wsprintf(lpDest,"%s::%s",lpconn->szVNode,lpconn->szDBName);
}
static char * emptystring="";
static LPUCHAR GetRepDBName(LPREPCDDS lpcdds, int dbno, LPUCHAR bufresult)
{
   LPOBJECTLIST  lpOConnect= lpcdds->connections;
   while (lpOConnect) {
      LPDD_CONNECTION lpconn=(LPDD_CONNECTION) lpOConnect->lpObject;  // no need to test the CDDS (DB Nos are independent)
     if (lpconn->dbNo == dbno) {
          TCHAR buf[200];
          CreatePathString(buf,lpconn);
          wsprintf(bufresult,"'%s'",buf);
          return bufresult;
     }
      lpOConnect = lpOConnect->lpNext;
   }
   return emptystring;
}

static BOOL CheckPathsConsistency(HWND hwnd)
{
   DWORD MaxCount,i,j,k;
   int iret;
   LPREPCDDS lpcdds          = (LPREPCDDS)GetDlgProp (hwnd);
   HWND hCntPath             = GetDlgItem(hwnd,ID_PATHCONTAINER);
   LPDD_DISTRIBUTION lpdis,lpdis1,lpdis2;
   LPRECORDCORE  lpstartrec = CntRecHeadGet(hCntPath);
   LPRECORDCORE  lprec,lprec1,lprec2;
   TCHAR buf[800],abuf[8][200];
   PATHS path,path1,path2;
   BOOL bMessage1=TRUE;
   
   MaxCount = CntTotalRecsGet(hCntPath);

   for (i=0,lprec=lpstartrec;i<MaxCount;i++,lprec=CntNextRec(lprec))  {
      BOOL bNeedAB=FALSE;
      BOOL bNeedBC=FALSE; 
      CntRecDataGet(hCntPath,lprec,(LPVOID)&path);
      lpdis =(LPDD_DISTRIBUTION) path.lpOL->lpObject;
      if (lpdis->localdb==lpdis->target) {  // A B B type not allowed
          //"There is a path with %s both as local and target. "
          //"This is not allowed."
          wsprintf(buf, ResourceString(IDS_ERR_CDDS_PATH),GetRepDBName(lpcdds,lpdis->localdb,abuf[1]));
          MessageBox(GetFocus(), buf, NULL,
                        MB_OK | MB_ICONEXCLAMATION | MB_TASKMODAL);
          return FALSE;
     
      }
      if (lpdis->source!=lpdis->localdb) {
         bNeedAB=TRUE; // if A B C type, need for A A B and B B C
         bNeedBC=TRUE;
         }
      for (j=0,lprec1=lpstartrec;j<MaxCount;j++,lprec1=CntNextRec(lprec1))  {
         if (j==i)
           continue;
         CntRecDataGet(hCntPath,lprec1,(LPVOID)&path1);
         lpdis1 =(LPDD_DISTRIBUTION) path1.lpOL->lpObject;
         if (j<i &&     // j<i to avoid multiple identical messages
             lpdis->source==lpdis1->source && 
             lpdis->target==lpdis1->target)  { 
            if (lpdis->localdb==lpdis1->localdb) {
                //"There are several identical paths %s  %s  %s . Continue?"
                wsprintf (buf, ResourceString(IDS_ERR_IDENTICAL_PATH),
                          GetRepDBName(lpcdds,lpdis->source, abuf[0]),
                          GetRepDBName(lpcdds,lpdis->localdb,abuf[1]),
                          GetRepDBName(lpcdds,lpdis->target, abuf[2]));
            }
            else {
               //"There are several paths with the same source %s and "
               //"the same target %s, which may result in collisions. Continue?"
               wsprintf (buf,ResourceString(IDS_F_SAMESOURCE_SAMETARGET), GetRepDBName(lpcdds,lpdis->source,abuf[0]),
                               GetRepDBName(lpcdds,lpdis->target,abuf[1]));
            }      
            iret = MessageBox(GetFocus(),buf,NULL, MB_ICONQUESTION | MB_YESNO |MB_TASKMODAL);
            if (iret == IDNO)
               return FALSE;
         }
         if (lpdis->target == lpdis1->source && lpdis1->target !=lpdis->source) {
            BOOL bAC = FALSE;
            for (k=0,lprec2=lpstartrec;k<MaxCount;k++,lprec2=CntNextRec(lprec2))  {
               if (k==i || k==j)
                  continue;
               CntRecDataGet(hCntPath,lprec2,(LPVOID)&path2);
               lpdis2 =(LPDD_DISTRIBUTION) path2.lpOL->lpObject;
               if (lpdis2->source==lpdis->source && lpdis2->target==lpdis1->target) {
                  bAC=TRUE;
                  break;
               }
            }
            if (!bAC && bMessage1) {
                //"There is a path from %s to %s, and another one from %s to %s, "
                //"but none from %s to %s.\nData modified on %s will not be propagated to %s. "
                //"Continue displaying similar message for other occurences?"
                wsprintf (buf,ResourceString(IDS_F_MESSAGE_ONE) ,
                            GetRepDBName(lpcdds,lpdis->source , abuf[0]),
                            GetRepDBName(lpcdds,lpdis->target , abuf[1]),
                            GetRepDBName(lpcdds,lpdis1->source, abuf[2]),
                            GetRepDBName(lpcdds,lpdis1->target, abuf[3]),
                            GetRepDBName(lpcdds,lpdis->source , abuf[4]),
                            GetRepDBName(lpcdds,lpdis1->target, abuf[5]),
                            GetRepDBName(lpcdds,lpdis->source , abuf[6]),
                            GetRepDBName(lpcdds,lpdis1->target ,abuf[7]) );
               iret = MessageBox(GetFocus(),buf,NULL, MB_ICONQUESTION | MB_YESNOCANCEL |MB_TASKMODAL);
               if (iret == IDCANCEL)
                  return FALSE;
               if (iret == IDNO)
                  bMessage1=FALSE;
            } 
         }
         if (bNeedAB) {
            if (lpdis1->source==lpdis->source   && 
                lpdis1->target==lpdis->localdb     ) {
                bNeedAB=FALSE;
            }
         }
         if (bNeedBC) {
            if (lpdis1->source==lpdis->localdb  && 
                lpdis1->target==lpdis->target     ) {
                bNeedBC=FALSE;
            }
         }
      }
      if (bNeedAB) {
         //"There is a path %s %s %s, without a path from %s to %s.\n This will not work."
         wsprintf(buf, ResourceString(IDS_F_MESSAGE_TWO),
                       GetRepDBName(lpcdds,lpdis->source ,abuf[0]),
                       GetRepDBName(lpcdds,lpdis->localdb,abuf[1]),
                       GetRepDBName(lpcdds,lpdis->target ,abuf[2]),
                       GetRepDBName(lpcdds,lpdis->source ,abuf[3]),
                       GetRepDBName(lpcdds,lpdis->localdb,abuf[4]) );
         MessageBox(GetFocus(), buf, NULL,   MB_OK | MB_ICONEXCLAMATION | MB_TASKMODAL);
         return FALSE;
      }
      if (bNeedBC) {
          //"There is a path %s %s %s, without a path from %s to %s.\n Data will not be replicated from %s to %s.Continue?"
          wsprintf(buf, ResourceString(IDS_F_MESSAGE_THREE),
                       GetRepDBName(lpcdds,lpdis->source ,abuf[0]),
                       GetRepDBName(lpcdds,lpdis->localdb,abuf[1]),
                       GetRepDBName(lpcdds,lpdis->target ,abuf[2]),
                       GetRepDBName(lpcdds,lpdis->localdb,abuf[3]),
                       GetRepDBName(lpcdds,lpdis->target ,abuf[4]),
                       GetRepDBName(lpcdds,lpdis->localdb,abuf[5]),
                       GetRepDBName(lpcdds,lpdis->target ,abuf[6])  );
         iret=MessageBox(GetFocus(), buf, ResourceString(IDS_TITLE_WARNING),   MB_ICONQUESTION | MB_YESNO | MB_TASKMODAL);
         if (iret == IDNO)
             return FALSE;
         
      }
   }
   return TRUE;
}

static LPUCHAR getReplicDBNodeName(LPREPCDDS lpcdds,int dbno,LPUCHAR bufresult)
{
   LPOBJECTLIST  lpOConnect  = lpcdds->connections;
   LPDD_CONNECTION lpconn;
   while (lpOConnect)  {
     lpconn                      = lpOConnect->lpObject;
     if (lpconn->dbNo==dbno) {
        lstrcpy(bufresult,lpconn->szVNode);
        return bufresult;
     }
     lpOConnect  = lpOConnect->lpNext;
   }
   lstrcpy(bufresult,"");
   return bufresult;
}

static BOOL CheckDBServerAndType(HWND hwnd)
{
   int iret;
   char buf[1000];
   LPREPCDDS lpcdds = (LPREPCDDS)GetDlgProp (hwnd);
   LPDD_DISTRIBUTION lpdis0,lpdis1;
   TCHAR RunNode0[MAXOBJECTNAME],RunNode1[MAXOBJECTNAME];
   BOOL bMessageGT10Done=FALSE;
   LPOBJECTLIST  lpOConnect ,lpOConnect1, lpO,lp1;
   LPDD_CONNECTION lpconn,lpconn1;

   // check server types 
   for ( lpOConnect  = lpcdds->connections;lpOConnect; lpOConnect=lpOConnect->lpNext) {
      lpconn=lpOConnect->lpObject;
      lpconn->bIsTarget=FALSE;
      if (lpconn->CDDSNo==(int)lpcdds->cdds) {
         for (lpO=lpcdds->distribution;lpO;lpO=lpO->lpNext) {
            lpdis0 =(LPDD_DISTRIBUTION) lpO->lpObject;
            if (lpdis0->CDDSNo==(int)(lpcdds->cdds)) {
               if (lpdis0->source==lpconn->dbNo || lpdis0->localdb==lpconn->dbNo) {
                  if (lpconn->nTypeRepl!=REPLIC_FULLPEER) {
                     //"Database %d %s::%s should be FULL PEER, since it it either "
                     //"source or target in at least one propagation path."
                     wsprintf(buf, ResourceString(IDS_F_MESSAGE_FOUR) , lpconn->dbNo, lpconn->szVNode, lpconn->szDBName);
                     MessageBox(GetFocus(), buf, NULL,MB_OK | MB_ICONEXCLAMATION | MB_TASKMODAL);
                     return FALSE;
                  }
               }
            }
         }
      }
   }

   // check server numbers
   for (lpOConnect=lpcdds->connections;lpOConnect;lpOConnect=lpOConnect->lpNext) {
      BOOL bReached=FALSE;
      lpconn    = lpOConnect->lpObject;
      if (lpconn->CDDSNo==(int)lpcdds->cdds) {
          if (lpconn->nServerNo<1) {
            if (!lpconn->bConnectionWithoutPath && lpconn->bIsInCDDS)
            {
              //"Server Number of database %d %s::%s should be >=1."
              wsprintf(buf, ResourceString(IDS_F_MESSAGE_FIVE),
                             lpconn->dbNo, lpconn->szVNode, lpconn->szDBName);
              MessageBox(GetFocus(), buf, NULL,MB_OK | MB_ICONEXCLAMATION | MB_TASKMODAL);
              return FALSE;
            }
         }
         if (lpconn->nServerNo>10 && !bMessageGT10Done) {
            //"Some servers numbers are >10.\n"
            //" You may need to create additional replicator subdirectories.\n"
            //" Please refer to the help system or the documention for more information.\n"
            //"Continue?"
            lstrcpy(buf, ResourceString(IDS_ERR_MESSAGE_SIX));
            iret = MessageBox(GetFocus(),buf,NULL, MB_ICONQUESTION | MB_YESNO |MB_TASKMODAL);
            if (iret == IDNO)
               return FALSE;
            bMessageGT10Done=TRUE;
         }

          for (lpOConnect1=lpcdds->connections;lpOConnect1;lpOConnect1=lpOConnect1->lpNext) {
            if (lpOConnect1==lpOConnect) {
               bReached=TRUE;
               continue;
            }
            lpconn1 = lpOConnect1->lpObject;
            if (lpconn->nServerNo==lpconn1->nServerNo) { // 2 databases with same serverno
               for (lpO=lpcdds->distribution;lpO;lpO=lpO->lpNext) {
                  lpdis0 =(LPDD_DISTRIBUTION) lpO->lpObject;
                  if (lpconn->dbNo==lpdis0->target && lpconn->CDDSNo == lpdis0->CDDSNo) {  // if first one is target...
                     getReplicDBNodeName(lpcdds,lpdis0->localdb,RunNode0);// get Run Node
                     if (lpdis0->RunNode[0])
                        lstrcpy(RunNode0,lpdis0->RunNode);
                     for (lp1=lpcdds->distribution;lp1;lp1=lp1->lpNext) {
                        if (lp1==lpO)
                           continue;
                        lpdis1 =(LPDD_DISTRIBUTION) lp1->lpObject;
                        if (lpdis1->CDDSNo>=0) {
                           if (lpconn1->dbNo == lpdis1->target && lpconn1->CDDSNo==lpdis1->CDDSNo) {  // and second one also target
                              getReplicDBNodeName(lpcdds,lpdis1->localdb,RunNode1);// get Run Node
                              if (lpdis1->RunNode[0])
                                 lstrcpy(RunNode1,lpdis1->RunNode);
                              if (!x_stricmp(RunNode0,RunNode1)) {
                                 if (lpdis0->localdb!=lpdis1->localdb) {
                                    if (lpconn1->CDDSNo==(int)lpcdds->cdds)  {
                                       //"Databases %d %s::%s and %d %s::%s have both the same server number (%d),"
                                       //"but they appear in paths requiring on a same node to have a server "
                                       //" with the same number but with 2 different local databases.\n"
                                       //"Please change the server number for one of them"
                                       wsprintf(buf, ResourceString(IDS_F_MESSAGE_SEVEN),
                                                     lpconn->dbNo, lpconn->szVNode, lpconn->szDBName,
                                                     lpconn1->dbNo, lpconn1->szVNode, lpconn1->szDBName,
                                                     lpconn->nServerNo);
                                       MessageBox(GetFocus(), buf, NULL,MB_OK | MB_ICONEXCLAMATION | MB_TASKMODAL);
                                    }
                                    else {
                                       //"Database %d %s::%s in this CDDS has the same server number (%d) as database %d %s::%s in CDDS %d, "
                                       //"but they appear in paths requiring on a same node to have a server "
                                       //" with the same number but with 2 different local databases.\n"
                                       //"Please change the server number for this database."
                                       wsprintf(buf, ResourceString(IDS_F_MESSAGE_EIGHT),
                                                     lpconn->dbNo, lpconn->szVNode, lpconn->szDBName,
                                                     lpconn->nServerNo,
                                                     lpconn1->dbNo, lpconn1->szVNode, lpconn1->szDBName,
                                                     lpconn1->CDDSNo);
                                       MessageBox(GetFocus(), buf, NULL,MB_OK | MB_ICONEXCLAMATION | MB_TASKMODAL);
                                    }
                                    return FALSE;
                                 }
                              }
                           }
                        }
                     }
                  }
               }
            }
         }
      }
   }
   return TRUE;
}

static BOOL VerifyTblRegAndLookupTblExist(LPREPCDDS lpcdds)
{
    LPDD_REGISTERED_TABLES lpTblReg;
    LPOBJECTLIST lpOConnect;
    lpOConnect  = lpcdds->registered_tables;
    while (lpOConnect)  {
      lpTblReg    = lpOConnect->lpObject;
      if (lpTblReg->cdds == (int)lpcdds->cdds &&
          lstrlen( lpTblReg->cdds_lookup_table_v11 )>0)
            return TRUE;
      lpOConnect=lpOConnect->lpNext;
    }
    return FALSE;
}

static void UpdateDBListFromPaths(HWND hwnd)
{
    char  lpDest[MAXOBJECTNAME*4];
    DWORD MaxCount,i,j;
    int svrno;
    int   alllist[3],index,ret;
    LPREPCDDS lpcdds          = (LPREPCDDS)GetDlgProp (hwnd);
    HWND hCntPath             = GetDlgItem(hwnd,ID_PATHCONTAINER);
    HWND hLbDatabaseCdds      = GetDlgItem(hwnd,ID_DATABASES_CDDS);
    LPDD_DISTRIBUTION lpdis,lpdis0,lpdis1;
    LPRECORDCORE  lprec = CntRecHeadGet(hCntPath);

    LPOBJECTLIST  lpOConnect,lpOConnect1,lpO,lp1;
    LPDD_CONNECTION lpconn,lpconn1;
    PATHS path;
    BOOL bTargetTypeChangedFromManual  = FALSE;

    ZEROINIT (lpDest);

    ListBox_ResetContent(hLbDatabaseCdds);
    MaxCount = CntTotalRecsGet(hCntPath);

    // new section
    lpOConnect  = lpcdds->connections;
    while (lpOConnect)  {
      lpconn                      = lpOConnect->lpObject;
      lpconn->bMustRemainFullPeer = FALSE;
      lpconn->bIsInCDDS           = FALSE;
      if (bFirstCall && lpconn->CDDSNo==(int)(lpcdds->cdds) && MaxCount==0)
      {
        int iret;
        TCHAR buf[350];
        BOOL bDisplayMesg = TRUE;

        if (lpcdds->bAlter && VerifyTblRegAndLookupTblExist(lpcdds))
        {
            bDisplayMesg = FALSE;
            iret = IDYES;
        }
        if (bDisplayMesg)
        {
            wsprintf(buf, ResourceString(IDS_I_ADD_DB_CDDS_KEEP_DEF),
                          lpconn->dbNo,
                          lpconn->szVNode,
                          lpconn->szDBName);
            iret = MessageBox(GetFocus(), buf, NULL, MB_YESNO | MB_ICONQUESTION | MB_TASKMODAL);
        }
        if (iret == IDYES)
        {
            lpconn->bIsInCDDS = TRUE;
            lpconn->bConnectionWithoutPath = TRUE;
            CreateDatabaseString(lpDest , lpconn ,hwnd);
            ret = ListBox_AddString(hLbDatabaseCdds, lpDest);
            if (ret != LB_ERR  && ret != LB_ERRSPACE)
                ListBox_SetItemData(hLbDatabaseCdds, ret,lpconn->dbNo);
        }
      }

      lpOConnect = lpOConnect->lpNext;
    }


    for (i=0;i<MaxCount;i++)  {
      CntRecDataGet(hCntPath,lprec,(LPVOID)&path);
      lpdis =(LPDD_DISTRIBUTION) path.lpOL->lpObject;

      alllist[0] = lpdis->source;
      alllist[1] = lpdis->localdb;
      alllist[2] = lpdis->target;

      for (j=0;j<3;j++) {  // identify entries in connection list for that CDDS
          LPDD_CONNECTION lpfoundconn=NULL;
          lpOConnect  = lpcdds->connections;
          while (lpOConnect)  {
            lpconn    = lpOConnect->lpObject;
            if (lpconn->dbNo == alllist[j] && lpconn->CDDSNo==(int)(lpcdds->cdds)) {
                lpfoundconn=lpconn;
                lpconn->bConnectionWithoutPath = FALSE;
                break;
            }
            lpOConnect=lpOConnect->lpNext;
          }
          if (!lpfoundconn) {
            lpOConnect  = lpcdds->connections;
            while (lpOConnect)  {
              lpconn    = lpOConnect->lpObject;
              if (lpconn->dbNo == alllist[j] && lpconn->CDDSNo<0) {
                lpconn->CDDSNo=lpcdds->cdds;
                lpfoundconn=lpconn;
                break;
              }
              lpOConnect=lpOConnect->lpNext;
            }
          }
          if (!lpfoundconn) {
              myerror(ERR_REPLICCDDS);
              return;
          }
      }
      lprec = CntNextRec(lprec);
    }

    // update server types 
    for ( lpOConnect  = lpcdds->connections;lpOConnect; lpOConnect=lpOConnect->lpNext) {
       lpconn    = lpOConnect->lpObject;
       lpconn->bIsTarget=FALSE;
       lpconn->localdbIfTarget=-1;
       if (lpconn->CDDSNo==(int)lpcdds->cdds) {
          BOOL bNeedFullPeer = FALSE;
          lprec = CntRecHeadGet(hCntPath);
          for (i=0;i<MaxCount;i++)  {
             CntRecDataGet(hCntPath,lprec,(LPVOID)&path);
             lpdis =(LPDD_DISTRIBUTION) path.lpOL->lpObject;
             if (lpconn->dbNo==lpdis->source || lpconn->dbNo==lpdis->localdb)
               bNeedFullPeer = TRUE;
             if (lpconn->dbNo==lpdis->target) {
                 lpconn->bIsTarget=TRUE;
                 lpconn->localdbIfTarget=lpdis->localdb;
             }
             lprec = CntNextRec(lprec);
          }
          if (bNeedFullPeer) {
            if (lpconn->bTypeChosen && lpconn->nTypeRepl!=REPLIC_FULLPEER)
                   bTargetTypeChangedFromManual= TRUE;
            lpconn->nTypeRepl=REPLIC_FULLPEER;
          }
          else {
             if (!lpconn->bTypeChosen) {
                 lpconn->nTypeRepl=REPLIC_PROT_RO;
              }
          }
       }
    }


    // update server numbers
   for (lpOConnect=lpcdds->connections;lpOConnect;lpOConnect=lpOConnect->lpNext) {
      lpconn    = lpOConnect->lpObject;
      if (lpconn->CDDSNo==(int)lpcdds->cdds) {
          if (lpconn->nServerNo<1 || !lpconn-> bServerChosen) {
              for (svrno=1;svrno<1000;svrno++){
                  BOOL bReached=FALSE;
                  BOOL bCanUse = TRUE;
               for (lpOConnect1=lpcdds->connections;lpOConnect1;lpOConnect1=lpOConnect1->lpNext) {
                    if (lpOConnect1==lpOConnect) {
                        bReached=TRUE;
                      continue;
                  }
                  lpconn1 = lpOConnect1->lpObject;
                  if (lpconn1->CDDSNo==(int)lpcdds->cdds) {
                        if (lpconn1->nServerNo==svrno && ( !bReached || lpconn->bServerChosen)) {
                           if (lpconn1->nTypeRepl==REPLIC_FULLPEER ||
                               lpconn ->nTypeRepl==REPLIC_FULLPEER)  {
                                bCanUse=FALSE;
                                break; // full peer's need unique values
                        }
                             if (lpconn1->bIsTarget && lpconn->bIsTarget &&
                                 lpconn1->localdbIfTarget==lpconn->localdbIfTarget) {
                                break;   // OK
                        }
                     }
                  }
                  if ( (lpconn1->CDDSNo!=(int)lpcdds->cdds) || 
                       (!bReached && lpconn->bIsTarget)        ) { // check for same local db for given runnode/serverno
                        if (lpconn1->nServerNo==svrno) {
                        TCHAR RunNode0[MAXOBJECTNAME],RunNode1[MAXOBJECTNAME];
                        for (lpO=lpcdds->distribution;lpO;lpO=lpO->lpNext) {
                           lpdis0 =(LPDD_DISTRIBUTION) lpO->lpObject;
                           if (lpdis0->CDDSNo==(int)(lpcdds->cdds)) {
                              if (lpconn->dbNo==lpdis0->target) {
                                 getReplicDBNodeName(lpcdds,lpdis0->localdb,RunNode0);
                                 if (lpdis0->RunNode[0])
                                    lstrcpy(RunNode0,lpdis0->RunNode);
                                 for (lp1=lpcdds->distribution;lp1;lp1=lp1->lpNext) {
                                    if (lp1==lpO)
                                       continue;
                                    lpdis1 =(LPDD_DISTRIBUTION) lp1->lpObject;
                                    if (lpdis1->CDDSNo==(int)(lpconn1->CDDSNo) && lpdis1->CDDSNo>=0) {
                                       if (lpconn1->dbNo == lpdis1->target) {
                                          getReplicDBNodeName(lpcdds,lpdis1->localdb,RunNode1);
                                          if (lpdis1->RunNode[0])
                                             lstrcpy(RunNode1,lpdis1->RunNode);
                                          if (!x_stricmp(RunNode0,RunNode1)) {
                                             if (lpdis0->localdb!=lpdis1->localdb) {
                                                bCanUse=FALSE;
                                                break; // full peer's need unique values
                                             }
                                          }
                                       }
                                    }
                                 }
                              }
                           }
                        }
                        if (!bCanUse)
                           break;
                     }
                  }
               }
               if (bCanUse) {
                      lpconn->nServerNo=svrno;
                      break;
               }
            }  // end of loop on svrno
         }  // end of "if (lpconn->nServerNo<1 ... (not assigned yet)
       }     // end of "if (lpconn->CDDSNo==(int)lpcdds->cdds ...
    }

    lprec = CntRecHeadGet(hCntPath);

    for (i=0;i<MaxCount;i++)  {
      CntRecDataGet(hCntPath,lprec,(LPVOID)&path);
      lpdis =(LPDD_DISTRIBUTION) path.lpOL->lpObject;
      
      alllist[0] = lpdis->source;
      alllist[1] = lpdis->localdb;
      alllist[2] = lpdis->target;

      for (j=0;j<3;j++) {
        ret = FindItemDatabaseCdds(hLbDatabaseCdds,alllist[j]);
        if (ret == LB_ERR)  {
          lpOConnect  = lpcdds->connections;
          while (lpOConnect)  {
            lpconn    = lpOConnect->lpObject;
            if (lpconn->dbNo == alllist[j] && lpconn->CDDSNo==(int)(lpcdds->cdds)) {
              if (j<2) {
                lpconn->bMustRemainFullPeer = TRUE;
                lpconn->nTypeRepl = REPLIC_FULLPEER;
              }
              if (GetOIVers() != OIVERS_12) {
                if (lpconn->nServerNo ==0)
                   lpconn->nServerNo = 1;
              }
              CreateDatabaseString(lpDest , lpconn ,hwnd);
              lpconn->bIsInCDDS = TRUE;
              break;
            }
             lpOConnect=lpOConnect->lpNext;
          }
           ret = LB_ERR;
          index = LB_ERR;
          index = GetIndexForInsertString(hLbDatabaseCdds,alllist[j]);
          if (index != LB_ERR)
            ret = ListBox_InsertString(hLbDatabaseCdds, index, lpDest);
          if (ret != LB_ERR  && ret != LB_ERRSPACE && index != LB_ERR)
            ListBox_SetItemData(hLbDatabaseCdds, ret,alllist[j]);
          ZEROINIT (lpDest);
        }
      }
      lprec = CntNextRec(lprec);
    }
    if (MaxCount>0) {
        ListBox_SetCurSel(hLbDatabaseCdds, 0);
        EnableWindow(hLbDatabaseCdds, TRUE);
        EnableWindow(GetDlgItem(hwnd,ID_ALTER_DATABASES_CDDS),TRUE);
    }
}

static BOOL OccupyTypesErrors (HWND hwnd)
/*
// Function:
//     Fills the Error drop down box with the errors names.version 1.1
//
// Parameters:
//     hwnd - Handle to the dialog window.
// 
// Returns:
//     TRUE if successful.
*/
{
   HWND hwndCtl = GetDlgItem (hwnd, ID_ERROR_MODE);
   return OccupyComboBoxControl(hwndCtl, typeErrors);

}
static BOOL OccupyTypesCollisions (HWND hwnd)
/*
// Function:
//     Fills the Collision drop down box with the Collisions names.
//     Replicator version 1.1
//
// Parameters:
//     hwnd - Handle to the dialog window.
// 
// Returns:
//     TRUE if successful.
*/
{
   HWND hwndCtl = GetDlgItem (hwnd, ID_COLLISION_MODE);
   return OccupyComboBoxControl(hwndCtl, typeCollisions);
}
static void ComboSetSelFromItemData(HWND hCombo,int itemdata);

static void ChangePath(HWND hCntPath,LPCNTPATHDATA lpcntdata,BOOL bCopySourceToLocal)
{
    LPRECORDCORE  lprec = CntFocusRecGet(hCntPath);
    LPDD_DISTRIBUTION lpdis;
    PATHS path;
    int   local,source,target;

    source  = (int)ComboBox_GetItemData(lpcntdata->hcomboorig,ComboBox_GetCurSel(lpcntdata->hcomboorig));
    if (bCopySourceToLocal)
          ComboSetSelFromItemData(lpcntdata->hcombolocal, source);
    local   = (int)ComboBox_GetItemData(lpcntdata->hcombolocal ,ComboBox_GetCurSel(lpcntdata->hcombolocal ));
    target  = (int)ComboBox_GetItemData(lpcntdata->hcombotarget,ComboBox_GetCurSel(lpcntdata->hcombotarget));

    CntRecDataGet(hCntPath,lprec,(LPVOID)&path);

    lpdis =(LPDD_DISTRIBUTION) path.lpOL->lpObject;

    if (local  != lpdis->localdb ||
        source != lpdis->source  ||
        target != lpdis->target
      )

    {
        lpdis->localdb= local;
        lpdis->source = source;
        lpdis->target = target;

        ComboBox_GetText(lpcntdata->hcomboorig,path.original, sizeof(path.original));
        ComboBox_GetText(lpcntdata->hcombolocal,path.local, sizeof(path.local));
        ComboBox_GetText(lpcntdata->hcombotarget,path.target, sizeof(path.target));
        CntRecDataSet(lprec,(LPVOID)&path);
    }

}

LRESULT WINAPI __export PathCntWndProcNew(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    HWND hCnt =GetParent(hwnd);
    LPCNTPATHDATA lpcntdata=(LPCNTPATHDATA)CntUserDataGet(hCnt);
    if (lpcntdata)

    {
        LRESULT lret = CallWindowProc(lpcntdata->fp,
          hwnd,
          message,
          wParam,
          lParam);

          switch(message)
          {
            HANDLE_MSG(hwnd, WM_COMMAND, PathCntOnCommand);
          }
        return lret;
    }
    return 0L;
}


static void PathCntOnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    HWND hCnt =GetParent(hwnd);
    LPCNTPATHDATA lpcntdata=(LPCNTPATHDATA)CntUserDataGet(hCnt);

    if (codeNotify == CBN_SELCHANGE && 
      (hwndCtl==lpcntdata->hcomboorig  ||
      hwndCtl==lpcntdata->hcombolocal  ||
      hwndCtl==lpcntdata->hcombotarget))
    {
        BOOL bCopyOrigToLocal=FALSE;
        if (hwndCtl==lpcntdata->hcomboorig && bInDlg)
           bCopyOrigToLocal=TRUE;
        ChangePath(hCnt,lpcntdata,bCopyOrigToLocal);
        UpdateDBListFromPaths(GetParent(hCnt));
        EnableDisableOKbutton(GetParent(hCnt));
        EnableDisableAddDropDBbutton(GetParent(hCnt));
    }
}


LRESULT WINAPI __export ColCntWndProcNew(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    HWND hCnt =GetParent(hwnd);
    LPCNTCOLDATA lpcntcoldata=(LPCNTCOLDATA)CntUserDataGet(hCnt);
    if (lpcntcoldata)

    {
        LRESULT lret = CallWindowProc(lpcntcoldata->fp,
          hwnd,
          message,
          wParam,
          lParam);

        switch (message)
        {
            HANDLE_MSG(hwnd, WM_LBUTTONDOWN, ColCntOnLButtonDown);
            HANDLE_MSG(hwnd, WM_CHAR, ColCntOnChar);
        }

        return lret;
    }
    return 0L;
}


static void ColCntOnLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags)
{
    RECT rect;
    POINT pt;
    HWND hCnt =GetParent(hwnd);
    LPCNTCOLDATA lpcntcoldata=(LPCNTCOLDATA)CntUserDataGet(hCnt);
    GetFocusCellRect(hCnt,&rect);
    ClientToScreen(hCnt,(LPPOINT)&rect.left);
    ClientToScreen(hCnt,(LPPOINT)&rect.right);
    GetCursorPos(&pt);
    if (PtInRect(&rect,pt))
    {
        FORWARD_WM_COMMAND(GetParent(hCnt), ID_COLSCONTAINER, hwnd, LAST_CN_MSG, PostMessage);
    }
}

static void ColCntOnChar(HWND hwnd, TCHAR ch, int cRepeat)
{
    HWND hCnt =GetParent(hwnd);
    LPCNTCOLDATA lpcntcoldata=(LPCNTCOLDATA)CntUserDataGet(hCnt);

    if (ch == 0x20)
    {
        FORWARD_WM_COMMAND(GetParent(hCnt), ID_COLSCONTAINER, hwnd, LAST_CN_MSG, PostMessage);
    }
}
static BOOL CopyListObject (LPOBJECTLIST lpSrc,LPOBJECTLIST FAR *lplpdst,int isize)
{
    while (lpSrc)

    {
        LPOBJECTLIST lpNew=AddListObjectTail(lplpdst,isize);
        if (!lpNew)
            return FALSE;
        memcpy((char*) lpNew->lpObject,(char*)lpSrc->lpObject,isize);
        lpSrc=lpSrc->lpNext;
    }
    return TRUE;
}
static int CopyStructure (HWND hwnd, LPREPCDDS lpsrc,LPREPCDDS lpdst)
{
    LPOBJECTLIST lpO;
    memcpy((char*) lpdst,(char *) lpsrc,offsetof(REPCDDS, flag));

    if (!CopyListObject (lpsrc->distribution,&lpdst->distribution,sizeof(DD_DISTRIBUTION)))
        return RES_ERR;

    if (!CopyListObject (lpsrc->connections,&lpdst->connections,sizeof(DD_CONNECTION)))
        return RES_ERR;

    for (lpO=lpsrc->registered_tables;lpO;lpO=lpO->lpNext)
    {
        LPOBJECTLIST lpNew=AddListObjectTail(&lpdst->registered_tables,sizeof(DD_REGISTERED_TABLES));

        if (!lpNew)
            return RES_ERR;

        memcpy((char*) lpNew->lpObject,(char*)lpO->lpObject,sizeof(DD_REGISTERED_TABLES));
        ((LPDD_REGISTERED_TABLES)lpNew->lpObject)->lpCols=NULL;

        if (!CopyListObject ( ((LPDD_REGISTERED_TABLES)lpO->lpObject)->lpCols,
          &((LPDD_REGISTERED_TABLES)lpNew->lpObject)->lpCols,
          sizeof(DD_REGISTERED_COLUMNS)))
            return RES_ERR;
    }


   return RES_SUCCESS;
}

static void  EnableTableParams(HWND hwnd,BOOL bEnabled)
{
    if (!bEnabled)

    {
        HWND hwndCnt = GetDlgItem(hwnd,ID_COLSCONTAINER);
        LPREPCDDS lpcdds = (LPREPCDDS)GetDlgProp (hwnd);
        char cemptystring = 0;
        lpcdds->flag=TRUE;

        if (CntRecHeadGet(hwndCnt))

        {
            CntDeferPaint( hwndCnt  );
            CntKillRecList(hwndCnt);
            CntEndDeferPaint( hwndCnt, TRUE );
        }    
        Edit_SetText(GetDlgItem(hwnd,ID_LOOKUPTABLE   ),&cemptystring);
        Edit_SetText(GetDlgItem(hwnd,ID_PRIORITYLOOKUP),&cemptystring);
        Button_SetCheck(GetDlgItem(hwnd,ID_SUPPORTTABLE),FALSE);
        Button_SetCheck(GetDlgItem(hwnd,ID_RULES),FALSE);
//        Button_SetCheck(GetDlgItem(hwnd,ID_BOTH_QUEUESHADOW),FALSE);
//        Button_SetCheck(GetDlgItem(hwnd,ID_SHADOW_ONLY),FALSE);
        
          

        lpcdds->flag=FALSE;
    }

    EnableWindow(GetDlgItem(hwnd,ID_SUPPORTTABLE          ),bEnabled);
    EnableWindow(GetDlgItem(hwnd,ID_RULES                 ),bEnabled);
    EnableWindow(GetDlgItem(hwnd,ID_LOOKUPTABLE           ),bEnabled);
    EnableWindow(GetDlgItem(hwnd,ID_PRIORITYLOOKUP        ),bEnabled);
    EnableWindow(GetDlgItem(hwnd,ID_COLSCONTAINER         ),bEnabled);
    EnableWindow(GetDlgItem(hwnd,ID_LOOKUPTABLESTATIC     ),bEnabled);
    EnableWindow(GetDlgItem(hwnd,ID_PRIORITYLOOKUPSTATIC  ),bEnabled);
    EnableWindow(GetDlgItem(hwnd,ID_GROUPCOLUMNS          ),bEnabled);
    EnableWindow(GetDlgItem(hwnd,ID_COLADDALL             ),bEnabled);
    EnableWindow(GetDlgItem(hwnd,ID_COLNONE               ),bEnabled);
//    EnableWindow(GetDlgItem(hwnd,ID_BOTH_QUEUESHADOW      ),bEnabled);
//    EnableWindow(GetDlgItem(hwnd,ID_SHADOW_ONLY           ),bEnabled);



}

static void AllCols(HWND hwnd,BOOL badd)
{
   HWND hCntCol  = GetDlgItem(hwnd,ID_COLSCONTAINER);
   LPRECORDCORE  lprec= CntRecHeadGet(hCntCol);
   COLS cols;
   BOOL          bOneKey;     // Emb 14/11/95 : need at least one keyed col.

   CntDeferPaint(hCntCol);
   bOneKey = FALSE;

   while (lprec)
      {
      ZEROINIT(cols); 
      CntRecDataGet(hCntCol,lprec,(LPVOID)&cols);


      if (badd) {
	//This case comes in when you want to register all columns.
        if (cols.bCanBeUnregistered) {
          if (cols.ikey)
            bOneKey = TRUE;
          *cols.repkey= cols.ikey ? 'K' : 0;
          *cols.repcol= 'R';
        }
        else {
	  if(*cols.repcol==0) //show the message only if the column is not registered.
          	ErrorMessage ((UINT) IDS_E_COLUMN_NOT_UNREGISTERED, RES_ERR);
        }
      }
      else {
        if (cols.bCanBeUnregistered) {
          *cols.repkey=0;
          *cols.repcol=0;
        }
        else {
          ErrorMessage ((UINT) IDS_E_COLUMN_NOT_UNREGISTERED, RES_ERR);
        }

      }
      CntRecDataSet( lprec,(LPVOID)&cols);
      lprec=CntNextRec(lprec);
      }

   if (badd && !bOneKey)
     {
     lprec= CntRecHeadGet(hCntCol);
     ZEROINIT(cols); 
     CntRecDataGet(hCntCol,lprec,(LPVOID)&cols);
     *cols.repkey = 'K';
     CntRecDataSet( lprec,(LPVOID)&cols);
     }

   CntEndDeferPaint( hCntCol, TRUE );
}


static BOOL FillColumns(LPREPCDDS lpcdds ,LPDD_REGISTERED_TABLES lptable,HWND hwndCnt, HWND hDlg)
{
    int          err;
    TABLEPARAMS  tableparams;
    int          SesHndl;          // current session handle
    int          iIndex;
    HWND hCntTable;
    LPOBJECTLIST lpOC;
	BOOL bHasNullsInKey = FALSE;
//    LPOBJECTLIST lpCT;
//    LPCONSTRAINTPARAMS lpConstrainte;


    ZEROINIT (tableparams);

    x_strcpy (tableparams.DBName,     lpcdds->DBName);
    x_strcpy (tableparams.objectname, lptable->tablename);
    //x_strcpy (tableparams.szSchema,   lpcdds->DBOwnerName);
    x_strcpy (tableparams.szSchema,   lptable->tableowner);
    err = GetDetailInfo (GetVirtNodeName (GetCurMdiNodeHandle ()) , OT_TABLE, &tableparams, FALSE,
                         &SesHndl);

    if (err != RES_SUCCESS)
        return FALSE;

    CntDeferPaint( hwndCnt  );

    CntKillRecList(hwndCnt);
    CntRangeSet( hwndCnt, 0, 0 );
   // New code 06/25/97
    x_strcpy(lptable->IndexName,lptable->tablename);

    if (!tableparams.bUflag && tableparams.UniqueSecIndexName[0]=='\0') {
       char MessErr[200];
       hCntTable = GetDlgItem(hDlg,ID_TABLES);
       iIndex    = CAListBox_GetCurSel(hCntTable);
       CAListBox_SetSel(hCntTable, FALSE,iIndex);   // unchecks the table 
       //"Table %s has no primary key or unique index"
       wsprintf(MessErr,ResourceString(IDS_F_MESSAGE_NINE), tableparams.objectname);
       MessageBox(hDlg, MessErr,
                        NULL,
                        MB_OK | MB_ICONEXCLAMATION | MB_TASKMODAL);
       TableCheckChange(hDlg,hCntTable,iIndex,TRUE);
       EnableTableParams(hDlg,FALSE);
    }
    else  {
       if (!tableparams.bUflag && tableparams.UniqueSecIndexName[0]!='\0') 
          x_strcpy(lptable->IndexName,tableparams.UniqueSecIndexName);
       for (lpOC=tableparams.lpColumns;lpOC;lpOC=lpOC->lpNext) {
          LPRECORDCORE lpRC = CntNewRecCore(sizeof(COLS));
          LPCOLUMNPARAMS lpCP=(LPCOLUMNPARAMS)lpOC->lpObject;
          LPOBJECTLIST  lpO;
          COLS          col;    
          ZEROINIT(col);
          
          x_strcpy(col.name,lpCP->szColumn);
//          if (GetOIVers() == OIVERS_12)
//             col.ikey =lpCP->nPrimary; //placed in Secondary for 2.0
//          else 
             col.ikey =lpCP->nSecondary;
          if (tableparams.UniqueSecIndexName[0])
             col.ikey =lpCP->nSecondary;
          *col.repkey= col.ikey ? 'K' : 0;
          *col.repcol= 'R';

          if (col.ikey!=0 || (!lpCP->bNullable && !lpCP->bDefault))
             col.bCanBeUnregistered=FALSE;
          else 
             col.bCanBeUnregistered=TRUE;
          if (col.ikey!=0 && lpCP->bNullable)
				bHasNullsInKey = TRUE;

          col.column_sequence = 1;

          /* See if column already registered */
          for (lpO=lptable->lpCols;lpO;lpO=lpO->lpNext){
             LPDD_REGISTERED_COLUMNS lpcol = (LPDD_REGISTERED_COLUMNS)lpO->lpObject;
               if (lstrcmp (lpcol->columnname,lpCP->szColumn) == 0){
                  lstrcpy(col.repkey,lpcol->key_colums);
                  lstrcpy(col.repcol,lpcol->rep_colums);
                  col.column_sequence=lpcol->column_sequence;
                  break;
               }
          }
          /* Load data into the record. */
          CntRecDataSet( lpRC, (LPVOID)&col);
          /* Add this record to the list. */
          CntAddRecTail( hwndCnt, lpRC );
          /* Increment the vertical scroll range */
          CntRangeInc(hwndCnt);
          
       }
    }
    // Modif PS+EMB 05/03/96
    FreeAttachedPointers (&tableparams,  OT_TABLE);

    CntSelectRec(hwndCnt,CntTopRecGet(hwndCnt));

    // Emb 14/11/95 to force at least one indexed column
    //AllCols(hDlg, TRUE);

    // Emb 15/11/95 to force columns data in linked list
    GetColumnsParams(hDlg, lptable);
    *lptable->colums_registered = lptable->lpCols ? 'C' : 0;
    *(lptable->colums_registered+1)=0;

    CntEndDeferPaint( hwndCnt, TRUE );
	return bHasNullsInKey;
}


static BOOL TableRefresh(HWND hwnd,HWND hwndCtl)
{
    int id = CAListBox_GetCurSel(hwndCtl);
	BOOL bHasNullsInKey = FALSE;

    if (id == LB_ERR)
        return FALSE;

    if (CAListBox_GetSel(hwndCtl, id))  // is checked

    {
        LPOBJECTLIST lpOL = (LPOBJECTLIST) CAListBox_GetItemData(hwndCtl, id);

        if (lpOL)  // associated item data (has already been checked at least once

        {
            LPDD_REGISTERED_TABLES lptable=lpOL->lpObject;

            EnableTableParams(hwnd,TRUE);

            if (lptable)

            {
                LPREPCDDS lpcdds = (LPREPCDDS)GetDlgProp (hwnd);

                lpcdds->flag=TRUE;

                Edit_SetText(GetDlgItem(hwnd,ID_LOOKUPTABLE   ),
                  lptable->cdds_lookup_table_v11    );

                Edit_SetText(GetDlgItem(hwnd,ID_PRIORITYLOOKUP),
                  lptable->priority_lookup_table_v11);

                Button_SetCheck(GetDlgItem(hwnd,ID_SUPPORTTABLE),
                  *lptable->table_created ==  'T');

                Button_SetCheck(GetDlgItem(hwnd,ID_RULES),
                  *lptable->rules_created ==  'R');

                lpcdds->flag=FALSE;
                // hour glass to show intense system activity
                ShowHourGlass();
                bHasNullsInKey=FillColumns(lpcdds,lptable,GetDlgItem(hwnd,ID_COLSCONTAINER), hwnd);
                RemoveHourGlass();
            }
        }
      else
        EnableTableParams(hwnd,FALSE);   // no associated item data

    }
    else
        EnableTableParams(hwnd,FALSE);  // is not checked
    return bHasNullsInKey;
}

// ***   TableCheckChange():
//       sets the extradata for a table in the table list if table is
//       checked and extradata weren't there already
//       If check status moving from unchecked to checked, tests if the
//       table is in another CDDS (in this case, extradata were set when
//       entering the dialog

static void
TableCheckChange(HWND hwnd,HWND hwndCtl,int id, BOOL bStatusChanging)
{
    char ach[MAXOBJECTNAME*3+MAXSPACES];
    LPDD_REGISTERED_TABLES lptable  = NULL;
    LPREPCDDS lpcdds                = (LPREPCDDS)GetDlgProp (hwnd);


    if (id==-1)   {
        id = CAListBox_GetCurSel(hwndCtl);
        if (id == CALB_ERR )
           return;
    }


    if (CAListBox_GetSel(hwndCtl, id))   // table checked

    {
        LPOBJECTLIST lpOL= (LPOBJECTLIST)CAListBox_GetItemData(hwndCtl, id);
        if (!lpOL) {                     // first time table is checked
           lpOL = (LPOBJECTLIST) AddListObjectTail(&lpcdds->registered_tables, sizeof(DD_REGISTERED_TABLES));
           if (lpOL) {
               char *p;
               char *p2;

               LPDD_REGISTERED_TABLES lptable=(LPDD_REGISTERED_TABLES)lpOL->lpObject;
               

               memset((char*) lptable,0,sizeof(DD_REGISTERED_TABLES));

               CAListBox_GetText(hwndCtl, id, ach);
               
               // find at the end of string the owner name of this table
               p2=x_strstr(ach,SPECIALSEPARATOR);
               if (p2 != NULL)
                  x_strcpy(lptable->tableowner,p2+MAXSPACES);
               p=x_strstr(ach,SPECIALSEPARATOR);  if (p) *p=0;
//             x_strcpy(lptable->tablename,ach);
               x_strcpy(lptable->tablename,RemoveDisplayQuotesIfAny(StringWithoutOwner(ach)));
               lptable->cdds=lpcdds->cdds;
               lptable->table_no=++lpcdds->LastNumber;
               
               // see for retrieve indexe name
			   // SQL request not necessary to retrieve the index name.
			   // The index name if filled anyhow later in FillColumns()
               // ires = ReplicRetrieveIndexstring( lptable );

               CAListBox_SetItemData(hwndCtl, id , lpOL);
               lptable->bStillInCDDS= TRUE;
           }
        }
        else { // table has already been checked in the past (extradata already there)
            LPDD_REGISTERED_TABLES lptable=(LPDD_REGISTERED_TABLES)lpOL->lpObject;
            CAListBox_GetText(hwndCtl, id, ach);
            if (lptable->cdds != lpcdds->cdds)  {
               lptable->bStillInCDDS= FALSE;
               CAListBox_SetSel(hwndCtl, FALSE, id);
               //     "This table is in other CDDS",
               ErrorMessage ((UINT) IDS_E_TABLE_IN_OTHER_CDDS, RES_ERR);
               return;
            }


            if (lptable->bStillInCDDS && bStatusChanging)
               myerror(ERR_REPLICCDDS);
            lptable->bStillInCDDS= TRUE;

        }
    }
    else

    {
        LPOBJECTLIST lpOL= (LPOBJECTLIST)CAListBox_GetItemData(hwndCtl, id);
        if (lpOL)

        {
            LPDD_REGISTERED_TABLES lptable=(LPDD_REGISTERED_TABLES)lpOL->lpObject;
            lptable->bStillInCDDS= FALSE;
            *lptable->table_created = ' ';
            *lptable->rules_created = ' ';

#ifdef OLDIES
            // 02/05/96  PS for deleted columns 
            FreeObjectList(lptable->lpCols);
            lptable->lpCols=NULL;
            lpcdds->registered_tables=DeleteListObject(lpcdds->registered_tables, lpOL);
            CAListBox_SetItemData(hwndCtl, id ,NULL);
#endif
        }
    }
	if (TableRefresh(hwnd,hwndCtl))
                MessageBox(GetFocus(), ResourceString(IDS_W_TBL_NULL_KEYS),
                        ResourceString(IDS_TITLE_WARNING), MB_OK | MB_ICONEXCLAMATION | MB_TASKMODAL);
}



static BOOL FillStructureFromControls (HWND hwnd)
{
  char ach[8];
  DWORD index;
  HWND hwndCollision = GetDlgItem(hwnd,ID_COLLISION_MODE);
  HWND hwndError     = GetDlgItem(hwnd,ID_ERROR_MODE);
  LPREPCDDS lpcdds   = (LPREPCDDS) GetDlgProp (hwnd);

  Edit_GetText(GetDlgItem(hwnd,ID_CDDSNUM),ach,sizeof(ach));
  lpcdds->cdds=my_atoi(ach);

  Edit_GetText(GetDlgItem(hwnd,ID_NAME_CDDS),lpcdds->szCDDSName,
                          sizeof(lpcdds->szCDDSName));

  index = ComboBox_GetCurSel(hwndCollision);
  lpcdds->collision_mode=(int)ComboBox_GetItemData(hwndCollision, index);

  index = ComboBox_GetCurSel(hwndError);
  lpcdds->error_mode=(int)ComboBox_GetItemData(hwndError, index);

  return TRUE;
}

static BOOL CreateObject (HWND hwnd, LPREPCDDS lpcdds )
{
    

    if (DBAAddObject ( GetVirtNodeName (GetCurMdiNodeHandle ()) ,
                       OT_REPLIC_CDDS_V11, (void *) lpcdds ) != RES_SUCCESS)

    {
        ErrorMessage ((UINT) IDS_E_CREATE_CDDS_FAILED, RES_ERR); 
        return FALSE;
    }

    return TRUE;
}

// ____________________________________________________________________________%
//

static BOOL AlterObject  (HWND hwnd)
{
   LPREPCDDS lpold = (LPREPCDDS)GetWindowLong(hwnd,DWL_USER);
   REPCDDS r2;
   REPCDDS r3;
   BOOL Success, bOverwrite;
   int  ires;
   HWND myFocus;
   int  hdl           = GetCurMdiNodeHandle ();
   LPUCHAR vnodeName  = GetVirtNodeName (hdl);
   LPREPCDDS lpr1 = GetDlgProp (hwnd);


   ZEROINIT (r2);
   ZEROINIT (r3);

   memcpy((char*) &r2,(char *) lpr1,offsetof(REPCDDS, flag));
   memcpy((char*) &r3,(char *) lpr1,offsetof(REPCDDS, flag));

   Success = TRUE;
   myFocus = GetFocus();
   ires    = GetDetailInfo (vnodeName, OT_REPLIC_CDDS_V11, &r2, TRUE, &hdl);

   if (ires != RES_SUCCESS)
   {
       ReleaseSession(hdl,RELEASE_ROLLBACK);
       switch (ires) {
          case RES_ENDOFDATA:
             ires = ConfirmedMessage ((UINT)IDS_C_ALTER_CONFIRM_CREATE);
             SetFocus (myFocus);
             if (ires == IDYES) {
                Success = CopyStructure (hwnd, lpr1,&r3);
                
                if (Success) {
                  ires = DBAAddObject (vnodeName, OT_REPLIC_CDDS_V11, (void *) &r3 );
                  if (ires != RES_SUCCESS) {
                    ErrorMessage ((UINT) IDS_E_CREATE_CDDS_FAILED, RES_ERR);
                    Success=FALSE;
                  }
                }
             }
             break;
          default:
            Success=FALSE;
            ErrorMessage ((UINT)IDS_E_ALTER_ACCESS_PROBLEM, RES_ERR);
            break;
       }
       FreeAttachedPointers (&r2, OT_REPLIC_CDDS);
       FreeAttachedPointers (&r3, OT_REPLIC_CDDS);
       return Success;
   }

   if (!AreObjectsIdentical (lpold, &r2, OT_REPLIC_CDDS))
   {
       ReleaseSession(hdl,RELEASE_ROLLBACK);
       ires   = ConfirmedMessage ((UINT)IDS_C_ALTER_RESTART_LASTCHANGES);
       if (ires == IDYES)
       {
          FreeAttachedPointers (&r2, OT_REPLIC_CDDS);
          return TRUE;
       }
       else
       {
           // Double Confirmation ?
           ires   = ConfirmedMessage ((UINT)IDS_C_ALTER_CONFIRM_OVERWRITE);
           if (ires != IDYES)
               Success=FALSE;
           else
           {
               char buf [2*MAXOBJECTNAME+5], buf2 [MAXOBJECTNAME];
               if (GetDBNameFromObjType (OT_USER,  buf2, NULL))
               {
                   wsprintf (buf,"%s::%s", vnodeName, buf2);
                   ires = Getsession (buf, SESSION_TYPE_CACHEREADLOCK, &hdl);
                   if (ires != RES_SUCCESS)
                       Success = FALSE;
               }
               else
                   Success = FALSE;
           }
       }
       if (!Success)
       {
          FreeAttachedPointers (&r2, OT_REPLIC_CDDS);
           return Success;
       }    
   }

   ires = DBAAlterObject (lpold, lpr1, OT_REPLIC_CDDS_V11, hdl);
   if (ires != RES_SUCCESS)  {
      Success=FALSE;
      ErrorMessage ((UINT) IDS_E_ALTER_CDDS_FAILED, ires);
      ires    = GetDetailInfo (vnodeName, OT_REPLIC_CDDS_V11, &r2, FALSE, &hdl);
      if (ires != RES_SUCCESS)
         ErrorMessage ((UINT)IDS_E_ALTER_ACCESS_PROBLEM, RES_ERR);
      else {
            if (!AreObjectsIdentical (lpold, &r2, OT_REPLIC_CDDS)){
               ires   = ConfirmedMessage ((UINT)IDS_C_ALTER_RESTART_LASTCHANGES);
            if (ires != IDYES) 
                bOverwrite = TRUE;
         }
      }
   }
   FreeAttachedPointers (&r2, OT_REPLIC_CDDS);
   FreeAttachedPointers (&r3, OT_REPLIC_CDDS);

   return Success;
}

static void CreateTableString(LPSTR lpDest,LPDD_REGISTERED_TABLES lptable,LPSTR lpSrcTb)
{
   if (x_strcmp(lpSrcTb,lptable->tablename)==0)
      wsprintf(lpDest,"%s (%d)",lptable->tablename,lptable->cdds);
   else
      wsprintf(lpDest,"%s (%d)", lpSrcTb ,lptable->cdds);
}

static HWND CreatePathCombo(HWND hwnd,HWND hwndCnt)
{
    HWND hRet;
    LPREPCDDS lpcdds = (LPREPCDDS)GetDlgProp (hwnd);
    hRet = CreateWindow("combobox",NULL,
      WS_CHILD | CBS_DROPDOWNLIST | CBS_HASSTRINGS |
      CBS_SORT | WS_VSCROLL | WS_TABSTOP
      ,0, 0, 0, 200
      ,GetWindow(hwndCnt,GW_CHILD),(HMENU)0,hInst,NULL);

    return hRet;
}   

static void InitializeContainers(HWND hwnd)
{
    char szTitle[64];
    LPFIELDINFO lpFI;
    WORD wIndex   = 0;
    HFONT hFont   = GetWindowFont(hwnd);
    HWND hCntPath = GetDlgItem(hwnd,ID_PATHCONTAINER);
    HWND hCntCol  = GetDlgItem(hwnd,ID_COLSCONTAINER);
    int nRowHeight;
    RECT rc;
    CNTCOLDATA cntColData;
    CNTPATHDATA cntPathData;


    cntColData.lptable=NULL;
    cntColData.fp = (WNDPROC) SetWindowLong(GetWindow(hCntCol,GW_CHILD),GWL_WNDPROC,(LONG) ColCntWndProcNew);
    CntUserDataSet( hCntCol, (LPVOID) &cntColData,sizeof(cntColData));

    cntPathData.hcomboorig=CreatePathCombo(hwnd,hCntPath);
    cntPathData.hcombolocal=CreatePathCombo(hwnd,hCntPath);
    cntPathData.hcombotarget=CreatePathCombo(hwnd,hCntPath);
    cntPathData.fp = (WNDPROC) SetWindowLong(GetWindow(hCntPath,GW_CHILD),GWL_WNDPROC,(LONG) PathCntWndProcNew);


    SetWindowFont(cntPathData.hcomboorig, hFont, TRUE);
    SetWindowFont(cntPathData.hcombolocal, hFont, TRUE);
    SetWindowFont(cntPathData.hcombotarget, hFont, TRUE);

    CntUserDataSet( hCntPath, (LPVOID) &cntPathData,sizeof(cntPathData));

    GetClientRect(cntPathData.hcomboorig,&rc);
    nRowHeight = rc.bottom-1;


    /* Defer painting until done changing container attributes. */
    CntDeferPaint( hCntPath );
    CntDeferPaint( hCntCol  );

    /* Init the vertical scrolling range to 0 since we have no records. */
    CntRangeSet( hCntPath, 0, 0 );
    CntRangeSet( hCntCol , 0, 0 );
    CntViewSet( hCntPath, CV_DETAIL );
    CntViewSet( hCntCol , CV_DETAIL );


    /* Set the container colors. */
    CntColorSet( hCntPath, CNTCOLOR_CNTBKGD,    GetSysColor(COLOR_WINDOW)    );
    CntColorSet( hCntCol , CNTCOLOR_CNTBKGD,    GetSysColor(COLOR_WINDOW)    );
    CntColorSet( hCntPath, CNTCOLOR_FLDTITLES,  GetSysColor(COLOR_WINDOWTEXT));
    CntColorSet( hCntCol , CNTCOLOR_FLDTITLES,  GetSysColor(COLOR_WINDOWTEXT));
    CntColorSet( hCntPath, CNTCOLOR_TEXT,       GetSysColor(COLOR_WINDOWTEXT));
    CntColorSet( hCntCol , CNTCOLOR_TEXT,       GetSysColor(COLOR_WINDOWTEXT));
    CntColorSet( hCntPath, CNTCOLOR_GRID,       GetSysColor(COLOR_WINDOWTEXT));
    CntColorSet( hCntCol , CNTCOLOR_GRID,       GetSysColor(COLOR_WINDOWTEXT));

    /* Set the container view. */
    CntViewSet( hCntPath, CV_DETAIL );
    CntViewSet( hCntCol , CV_DETAIL );

    /* Set some general container attributes. */
    CntAttribSet(hCntPath, CA_SIZEABLEFLDS | CA_RECSEPARATOR | CA_VERTFLDSEP | CA_FLDTTL3D );
    CntAttribSet(hCntCol , CA_SIZEABLEFLDS | CA_RECSEPARATOR | CA_VERTFLDSEP | CA_FLDTTL3D );

    /* Define space for container field titles. */
    CntFldTtlHtSet(hCntPath, 1);
    CntFldTtlHtSet(hCntCol , 1);
    CntFldTtlSepSet(hCntPath);
    CntFldTtlSepSet(hCntCol );

    /* Set some fonts for the different container areas. */
    CntFontSet( hCntPath, hFont, CF_GENERAL );
    CntFontSet( hCntCol , hFont, CF_GENERAL );

    CntRowHtSet( hCntPath, -nRowHeight, CA_LS_NONE);
    CntRowHtSet( hCntCol , -nRowHeight, CA_LS_NONE);

    /* Initialise the field descriptor. */

    lpFI = CntNewFldInfo();
    lpFI->wIndex       = wIndex;
    lpFI->cxPxlWidth   = 0;
    lpFI->cxWidth      = 20;
    lpFI->flFDataAlign = CA_TA_LEFT | CA_TA_VCENTER;
    lpFI->wColType     = CFT_STRING;
    lpFI->flColAttr    = CFA_FLDREADONLY | CFA_FLDTTLREADONLY;
    lpFI->flFTitleAlign= CA_TA_LEFT | CA_TA_VCENTER;
    lpFI->wDataBytes   = PATHS_LENGTH;
    lpFI->wOffStruct   = offsetof(PATHS, original);
    lstrcpy(szTitle, ResourceString ((UINT) IDS_I_ORIGINAL));
    CntFldTtlSet(hCntPath, lpFI, szTitle, lstrlen(szTitle) + 1);
    CntFldAttrSet(lpFI, (DWORD) CFA_FLDREADONLY | CFA_FLDTTLREADONLY);
    /* Add this field to the container field list. */
    CntAddFldTail( hCntPath, lpFI );


    /* Initialise the field descriptor. */

    wIndex++;
    lpFI = CntNewFldInfo();
    lpFI->cxPxlWidth   = 0;
    lpFI->cxWidth      = 20;
    lpFI->wIndex       = wIndex;
    lpFI->flFDataAlign = CA_TA_LEFT | CA_TA_VCENTER;
    lpFI->wColType     = CFT_STRING;
    lpFI->flColAttr    = CFA_FLDREADONLY | CFA_FLDTTLREADONLY;
    lpFI->flFTitleAlign = CA_TA_LEFT | CA_TA_VCENTER;
    lpFI->wDataBytes   = PATHS_LENGTH;
    lpFI->wOffStruct   = offsetof(PATHS, local);
    lstrcpy(szTitle, ResourceString ((UINT) IDS_I_LOCAL));
    CntFldTtlSet(hCntPath, lpFI, szTitle, lstrlen(szTitle) + 1);
    CntFldAttrSet(lpFI, (DWORD) CFA_FLDREADONLY | CFA_FLDTTLREADONLY);
    /* Add this field to the container field list. */
    CntAddFldTail( hCntPath, lpFI );


    /* Initialise the field descriptor. */

    wIndex++;
    lpFI = CntNewFldInfo();
    lpFI->wIndex       = wIndex;
    lpFI->cxPxlWidth   = 0;
    lpFI->cxWidth      = 1200;
    lpFI->flFDataAlign = CA_TA_LEFT | CA_TA_VCENTER;
    lpFI->wColType     = CFT_STRING;
    lpFI->flColAttr    = CFA_FLDREADONLY | CFA_FLDTTLREADONLY;
    lpFI->flFTitleAlign = CA_TA_LEFT | CA_TA_VCENTER;
    lpFI->wDataBytes   = PATHS_LENGTH;
    lpFI->wOffStruct   = offsetof(PATHS, target);
    lstrcpy(szTitle, ResourceString ((UINT) IDS_I_TARGET));
    CntFldTtlSet(hCntPath, lpFI, szTitle, lstrlen(szTitle) + 1);
    CntFldAttrSet(lpFI, (DWORD) CFA_FLDREADONLY | CFA_FLDTTLREADONLY);
    /* Add this field to the container field list. */
    CntAddFldTail( hCntPath, lpFI );


    wIndex =0;


    /* Initialise the field descriptor. */

    lpFI = CntNewFldInfo();
    lpFI->wIndex       = wIndex ;
    lpFI->cxWidth      = 8;
    lpFI->cxPxlWidth   = 0;
    lpFI->flFDataAlign = CA_TA_LEFT | CA_TA_VCENTER;
    lpFI->wColType     = CFT_STRING;
    lpFI->flColAttr    = CFA_FLDREADONLY | CFA_FLDTTLREADONLY;;
    lpFI->wDataBytes   = MAXOBJECTNAME;
    lpFI->wOffStruct   = offsetof(COLS, name);
    lpFI->flFTitleAlign = CA_TA_LEFT | CA_TA_VCENTER;
    lstrcpy(szTitle, ResourceString ((UINT) IDS_I_NAME));
    CntFldTtlSet(hCntCol, lpFI, szTitle, lstrlen(szTitle) + 1);
    CntFldAttrSet(lpFI, (DWORD) CFA_FLDREADONLY | CFA_FLDTTLREADONLY);
    /* Add this field to the container field list. */
    CntAddFldTail( hCntCol, lpFI );


    wIndex++;
    lpFI = CntNewFldInfo();
    lpFI->wIndex       = wIndex;
    //lpFI->cxWidth      = 8;
    lpFI->cxWidth      = 9;
    lpFI->cxPxlWidth   = 0;
    lpFI->flFDataAlign = CA_TA_LEFT | CA_TA_VCENTER;
    //lpFI->wColType     = CFT_STRING;
    lpFI->wColType     = CFT_INT;
    //lpFI->flColAttr    = CFA_FLDREADONLY | CFA_FLDTTLREADONLY | CFA_OWNERDRAW;
    lpFI->flColAttr    = CFA_FLDREADONLY | CFA_FLDTTLREADONLY;
    lpFI->wDataBytes   = 2;
    //lpFI->wOffStruct   = offsetof(COLS, repkey);
    lpFI->wOffStruct   = offsetof(COLS, ikey);
    lstrcpy(szTitle, ResourceString ((UINT) IDS_I_REP_KEY));
    CntFldTtlSet(hCntCol, lpFI, szTitle, lstrlen(szTitle) + 1);
    //CntFldDrwProcSet(lpFI, (DRAWPROC) lpfnDrawProc);
    CntFldAttrSet(lpFI, (DWORD) CFA_FLDREADONLY | CFA_FLDTTLREADONLY);
    /* Add this field to the container field list. */
    CntAddFldTail( hCntCol, lpFI );


    wIndex++;
    lpFI = CntNewFldInfo();
    lpFI->wIndex       = wIndex++;
    lpFI->cxWidth      = 80;
    lpFI->cxPxlWidth   = 0;
    lpFI->flFDataAlign = CA_TA_LEFT | CA_TA_VCENTER;
    lpFI->wColType     = CFT_STRING;
    lpFI->flColAttr    = CFA_FLDREADONLY | CFA_FLDTTLREADONLY | CFA_OWNERDRAW;
    lpFI->wDataBytes   = 2;
    lpFI->wOffStruct   = offsetof(COLS, repcol);
    lstrcpy(szTitle, ResourceString ((UINT) IDS_I_REP));
    CntFldTtlSet(hCntCol, lpFI, szTitle, lstrlen(szTitle) + 1);
    CntFldDrwProcSet(lpFI, (DRAWPROC) lpfnDrawProc);
    CntFldAttrSet(lpFI, (DWORD) CFA_FLDREADONLY | CFA_FLDTTLREADONLY);
    /* Add this field to the container field list. */
    CntAddFldTail( hCntCol, lpFI );
    CntMouseEnable( hCntCol , TRUE );

    /* Now paint the container. */
    CntEndDeferPaint( hCntPath, TRUE );
    CntEndDeferPaint( hCntCol , TRUE );
}

int CALLBACK __export DrawCtnColNew( HWND   hwndCnt,     /* container window handle */
  LPFIELDINFO  lpFld, /* field being drawn */
  LPRECORDCORE lpRec, /* record being drawn */
  LPVOID lpData,      /* data in internal format */
  HDC    hDC,         /* container window DC */
  int    nXstart,     /* starting position Xcoord */
  int    nYstart,     /* starting position Ycoord */
  UINT   fuOptions,   /* rectangle type */
  LPRECT lpRect,      /* rectangle of data cell */
  LPSTR  lpszOut,     /* data in string format */
  UINT   uLenOut )    /* len of lpszOut buffer */
{
    char c=' ';
    /*COLORREF crText  = SetTextColor(hDC, CntColorGet(hwndCnt, CNTCOLOR_TEXT)); */
    /*COLORREF crBkgnd = SetBkColor(hDC, CntColorGet(hwndCnt, CNTCOLOR_FLDBKGD)); */


    /* First blank the field */
    ExtTextOut(hDC,lpRect->left,lpRect->top,ETO_OPAQUE|ETO_CLIPPED,lpRect,&c,1,NULL);

    /* put a checkmark  */

    if (*lpszOut)

    {
        HDC     hdcMemory;
        HBITMAP hbmpMyBitmap, hbmpOld;
        BITMAP bm;

        hbmpMyBitmap = LoadBitmap((HINSTANCE)0, MAKEINTRESOURCE(32760 /*OBM_CHECK*/));
        GetObject(hbmpMyBitmap, sizeof(BITMAP), &bm);

        hdcMemory = CreateCompatibleDC(hDC);
        hbmpOld = SelectObject(hdcMemory, hbmpMyBitmap);
        BitBlt(hDC, nXstart, nYstart, bm.bmWidth, bm.bmHeight, hdcMemory, 0, 0, SRCCOPY);
        SelectObject(hdcMemory, hbmpOld);
        DeleteDC(hdcMemory);
    }
    /*SetTextColor(hDC,crText);   */
    /*SetBkColor(hDC,crBkgnd); */
    return 1;
}



static void ModifyCols(HWND hwnd)
{
    HWND hCntCol        =   GetDlgItem(hwnd,ID_COLSCONTAINER);
    LPRECORDCORE  lprec =   CntSelRecGet(hCntCol);
    LPFIELDINFO   lpfi  =   CntFocusFldGet(hCntCol);


    if (lprec && lpfi && lpfi->wIndex>0)

    {
        LPFIELDINFO   lpfi2;
        COLS cols;
        CntRecDataGet(hCntCol,lprec,(LPVOID)&cols);
        switch (lpfi->wIndex)

        {
        case 1:
           // Emb 14/11/95 : we must ignore the ikey field
           if (1 || cols.ikey>0)
               {
               if (*cols.repkey)
                   {
                   *cols.repkey = 0;
                   // Emb 14/11/95 : don't change // old code: *cols.repcol=  0;
                   }
               else
                   {
                   *cols.repkey = 'K';
                   *cols.repcol = 'R';
                   }
               }
           else
               MessageBeep(0);
            lpfi2=CntNextFld(lpfi);
            break;

        case 2:
            if (cols.bCanBeUnregistered) {
               *cols.repcol= *cols.repcol ? 0 : 'R';
               if (cols.ikey>0)
                  *cols.repkey= *cols.repcol ? 'K' :0 ;
               lpfi2=CntPrevFld(lpfi);
            }
            else {
               *cols.repcol= 'R';
               CntRecDataSet(lprec,(LPVOID)&cols);
               CntUpdateRecArea( hCntCol, lprec,lpfi);

               //"Columns which are nullable, without default, or in the key must be registered",
               ErrorMessage ((UINT) IDS_E_COLUMN_NOT_UNREGISTERED, RES_ERR);
               return;
            }
            lpfi2=CntPrevFld(lpfi);
            break;
        }
        CntRecDataSet(lprec,(LPVOID)&cols);


        CntUpdateRecArea( hCntCol, lprec,lpfi);

        if (lpfi2)
           CntUpdateRecArea( hCntCol, lprec,lpfi2);

    }
}


static void  GetColumnsParams(HWND hwnd,LPDD_REGISTERED_TABLES lptable)
{
    HWND hCntCol  = GetDlgItem(hwnd,ID_COLSCONTAINER);
    LPRECORDCORE  lprec= CntRecHeadGet(hCntCol);
    while (lprec)

    {
        LPOBJECTLIST  lpCols=lptable->lpCols;
        COLS cols;
        BOOL bfind=FALSE;
        CntRecDataGet(hCntCol,lprec,(LPVOID)&cols);

        while (lpCols && !bfind)

        {
            if (x_strcmp(cols.name,
              ((LPDD_REGISTERED_COLUMNS)lpCols->lpObject)->columnname
              )==0)

            {
                bfind=TRUE;
                x_strcpy(((LPDD_REGISTERED_COLUMNS)lpCols->lpObject)->key_colums,
                  cols.repkey);

                x_strcpy(((LPDD_REGISTERED_COLUMNS)lpCols->lpObject)->rep_colums,
                  cols.repcol);

                ((LPDD_REGISTERED_COLUMNS)lpCols->lpObject)->key_sequence=cols.ikey;

                if (!(*cols.repkey) &&  !(*cols.repcol))

                {
                    //lptable->lpCols=DeleteListObject(lptable->lpCols, lpCols);
                   ((LPDD_REGISTERED_COLUMNS)lpCols->lpObject)->column_sequence=0; 
                }
                else 
                {
                   ((LPDD_REGISTERED_COLUMNS)lpCols->lpObject)->column_sequence=1;
                }

                break;   
            }
            lpCols=lpCols->lpNext;
        }

        if (!bfind && (*cols.repkey || *cols.repcol))

        {
            LPOBJECTLIST lpNew=AddListObjectTail(&lptable->lpCols,sizeof(DD_REGISTERED_COLUMNS));
            if (lpNew)

            {
                x_strcpy(((LPDD_REGISTERED_COLUMNS)lpNew->lpObject)->columnname,
                  cols.name);

                x_strcpy(((LPDD_REGISTERED_COLUMNS)lpNew->lpObject)->key_colums,
                  cols.repkey);

                x_strcpy(((LPDD_REGISTERED_COLUMNS)lpNew->lpObject)->rep_colums,
                  cols.repcol);
                
                ((LPDD_REGISTERED_COLUMNS)lpNew->lpObject)->key_sequence=cols.ikey;

                ((LPDD_REGISTERED_COLUMNS)lpNew->lpObject)->column_sequence=cols.column_sequence; 
            }
        }       
        lprec=CntNextRec(lprec);
    }
}


static void TableGetParams(HWND hwnd,HWND hwndCtl,int id)
{
    LPREPCDDS lpcdds = (LPREPCDDS)GetDlgProp (hwnd);

    if (id==-1)
       id = CAListBox_GetCurSel(hwndCtl);

    if (lpcdds->flag)
        return;

    if (CAListBox_GetSel(hwndCtl, id))

    {
        LPOBJECTLIST lpOL =(LPOBJECTLIST)CAListBox_GetItemData(hwndCtl, id);
        if (lpOL)

        {
            LPDD_REGISTERED_TABLES lptable= lpOL->lpObject;
            if (lptable)

            {
                Edit_GetText (GetDlgItem(hwnd,ID_LOOKUPTABLE   ),
                  lptable->cdds_lookup_table_v11,
                  sizeof(lptable->cdds_lookup_table_v11));

                Edit_GetText(GetDlgItem(hwnd,ID_PRIORITYLOOKUP),
                  lptable->priority_lookup_table_v11,
                  sizeof(lptable->priority_lookup_table_v11));

                *lptable->table_created = Button_GetCheck(
                  GetDlgItem(hwnd,ID_SUPPORTTABLE)) ?
                  'T' : ' ';

                *lptable->rules_created = Button_GetCheck(
                  GetDlgItem(hwnd,ID_RULES)) ?
                  'R' : ' ';

                GetColumnsParams(hwnd,lptable);

                *lptable->colums_registered = lptable->lpCols ? 'C' : 0;
                *(lptable->colums_registered+1)=0;

            }
        }
    }
}


/* Called each time the table selection changes */
/* Called each time a table param changes */

static void ComboSetSelFromItemData(HWND hCombo,int itemdata)
{
    int i;
    for (i=0;i<ComboBox_GetCount(hCombo);i++)

    {
        if (itemdata == (int) ComboBox_GetItemData(hCombo,i) )

        {
            ComboBox_SetCurSel(hCombo,i);
            break;
        }
    }
}



static void MovePathCombos(HWND hwnd)
{
    HWND hCntPath  = GetDlgItem(hwnd,ID_PATHCONTAINER);
    LPCNTPATHDATA lpcntpathdata=(LPCNTPATHDATA) CntUserDataGet(hCntPath);

    if (lpcntpathdata)

    {
        DWORD  dwOrg;
        DWORD  dwExt;
        POINT  ptOrg;
        POINT  ptExt;
        HDWP   hdwp;
        LPDD_DISTRIBUTION lpdis;
        LPFIELDINFO lpFI;
        RECT rc;
        LPRECORDCORE  lprec = CntFocusRecGet(hCntPath);

        PATHS path;

        if (!lprec)

        {
            ShowWindow(lpcntpathdata->hcomboorig,SW_HIDE);
            ShowWindow(lpcntpathdata->hcombolocal,SW_HIDE);
            ShowWindow(lpcntpathdata->hcombotarget,SW_HIDE);
            return;
        }



        CntRecDataGet(hCntPath,lprec,(LPVOID)&path);

        lpdis =(LPDD_DISTRIBUTION) path.lpOL->lpObject;

        ComboSetSelFromItemData(lpcntpathdata->hcomboorig , lpdis->source);
        ComboSetSelFromItemData(lpcntpathdata->hcombolocal, lpdis->localdb );
        ComboSetSelFromItemData(lpcntpathdata->hcombotarget,lpdis->target );


        GetClientRect(GetWindow(hCntPath,GW_CHILD), &rc);

        /* Get position to set the combos */
        dwOrg = CntFocusOrgGet(hCntPath,FALSE);
        dwExt = CntFocusExtGet(hCntPath);

        ptOrg.x = 0;
        ptOrg.y = HIWORD(dwOrg);

        lpFI = CntFldHeadGet(hCntPath);

        ptExt.x =  CntFldWidthGet( hCntPath, lpFI, TRUE);
        ptExt.y =  HIWORD(dwExt)*5;

        hdwp=BeginDeferWindowPos(3);
        hdwp=DeferWindowPos(hdwp, lpcntpathdata->hcomboorig, 0,
          ptOrg.x-1, ptOrg.y-1,
          ptExt.x+1, ptExt.y,
          SWP_NOZORDER|SWP_SHOWWINDOW);

        lpFI = CntNextFld(lpFI);
        ptOrg.x += ptExt.x;
        ptExt.x = CntFldWidthGet( hCntPath, lpFI, TRUE);

        hdwp=DeferWindowPos(hdwp, lpcntpathdata->hcombolocal, 0,
          ptOrg.x-1, ptOrg.y-1,
          ptExt.x+1, ptExt.y,
          SWP_NOZORDER|SWP_SHOWWINDOW);

        lpFI = CntNextFld(lpFI);
        ptOrg.x += ptExt.x;
        ptExt.x = rc.right-ptOrg.x;

        hdwp=DeferWindowPos(hdwp, lpcntpathdata->hcombotarget, 0,
          ptOrg.x-1, ptOrg.y-1,
          ptExt.x+2, ptExt.y,
          SWP_NOZORDER|SWP_SHOWWINDOW);
        EndDeferWindowPos(hdwp);
    }
}


static void GetFirstAvailableCDDS(LPREPCDDS lpcdds,LPSTR lpBuf)
{
   int     hdl, ires;
   char    buf [MAXOBJECTNAME*2];
   char    buffilter [MAXOBJECTNAME];
   LPUCHAR parentstrings [MAXPLEVEL];
   long i,imax;

   ZEROINIT (buf);
   ZEROINIT (buffilter);
   parentstrings [0] = lpcdds->DBName;
   parentstrings [1] = NULL;

   hdl      = GetCurMdiNodeHandle ();

   imax=0;
   ires = DOMGetFirstObject (hdl, OT_REPLIC_CDDS, 1,
          parentstrings, FALSE, NULL, buf, NULL, NULL);

   while (ires == RES_SUCCESS)  {
      long l=my_atodw(buf);
      if (imax<l)
        imax=l;
      ires    = DOMGetNextObject (buf, buffilter, NULL);
   }
   imax+=2L;

   for (i=0;i<imax;i++)
       {
       ires = DOMGetFirstObject (hdl, OT_REPLIC_CDDS, 1,
              parentstrings, FALSE, NULL, buf, NULL, NULL);

       while (ires == RES_SUCCESS)
          {
          long l=my_atodw(buf);

          if (i<l)
             {
             my_dwtoa (i, lpBuf,10);
             return;
             }

          if (i==l)
              break;

          ires    = DOMGetNextObject (buf, buffilter, NULL);
          }
       }
   my_dwtoa (imax-1, lpBuf,10);
//   *lpBuf=0;
}

static BOOL IsAvailableCDDSNo(LPREPCDDS lpcdds,int cddsno)
{
   int     hdl, ires;
   char    buf [MAXOBJECTNAME*2];
   LPUCHAR parentstrings [MAXPLEVEL];

   parentstrings [0] = lpcdds->DBName;
   parentstrings [1] = NULL;

   hdl      = GetCurMdiNodeHandle ();

   ires = DOMGetFirstObject (hdl, OT_REPLIC_CDDS, 1,
     parentstrings, FALSE, NULL, buf, NULL, NULL);

   while (ires == RES_SUCCESS)  {
       if (cddsno==my_atoi(buf))
          return FALSE;
       ires    = DOMGetNextObject (buf, NULL, NULL);
   }
   if (ires!=RES_ENDOFDATA)
      return FALSE;
   return TRUE;
}

static BOOL IsCddsCorrect(HWND hwnd)
{
   LPREPCDDS     lpcdds = (LPREPCDDS) GetDlgProp (hwnd);
   int     hdl, ires;
   char    buf [MAXOBJECTNAME*2];
   char    buffilter [MAXOBJECTNAME];
   LPUCHAR parentstrings [MAXPLEVEL];

   if (lpcdds->bAlter)
       return TRUE;

   ZEROINIT (buf);
   ZEROINIT (buffilter);
   parentstrings [0] = lpcdds->DBName;
   parentstrings [1] = NULL;

   hdl      = GetCurMdiNodeHandle ();

   ires = DOMGetFirstObject (hdl, OT_REPLIC_CDDS, 1,
     parentstrings, FALSE, NULL, buf, NULL, NULL);

   while (ires == RES_SUCCESS)
       {
       if (lpcdds->cdds==(UINT)my_atoi(buf))
           {
           ErrorMessage ((UINT) IDS_E_CDDS_EXISTS, RES_ERR);

            FORWARD_WM_NEXTDLGCTL(hwnd, GetDlgItem(hwnd,ID_CDDSNUM), GetDlgItem(hwnd,ID_CDDSNUM), PostMessage);
           return FALSE;
           }
       ires    = DOMGetNextObject (buf, buffilter, NULL);
       }
   return TRUE;
}

static BOOL IsPathCorrect(HWND hwnd)
{
   LPREPCDDS     lpcdds = (LPREPCDDS) GetDlgProp (hwnd);
   LPOBJECTLIST  lpO;

   for (lpO=lpcdds->distribution;lpO;lpO=lpO->lpNext)
       {
       LPOBJECTLIST  lpO2;
       LPDD_DISTRIBUTION lpdis =(LPDD_DISTRIBUTION) lpO->lpObject;
       if (lpdis->CDDSNo!=(int)(lpcdds->cdds))
           continue;
       for (lpO2=lpO->lpNext;lpO2;lpO2=lpO2->lpNext)
          {
           LPDD_DISTRIBUTION lpdis1 =(LPDD_DISTRIBUTION) lpO2->lpObject;
           if (lpdis1->CDDSNo!=(int)(lpcdds->cdds))
              continue;
          if (memcmp(lpO->lpObject,lpO2->lpObject,sizeof(DD_DISTRIBUTION))==0)
             {
             ErrorMessage((UINT)IDS_E_DUPLICATE_PATH, RES_ERR);
             return FALSE;
             }    
          }

       if (lpdis->source==lpdis->target)
           {
           //
           // "Source and target Database must be different."
           //
           ErrorMessage ((UINT) IDS_E_SOURCE_TARGET, RES_ERR);
           return FALSE;
           }

       if (lpdis->localdb==lpdis->target)
           {
           //
           // "Local and target Database must be different."
           //
           ErrorMessage ((UINT) IDS_E_LOCAL_TARGET, RES_ERR);
           return FALSE;
           }
       }
   return TRUE;
}

static void RemovePath(HWND hwnd)
{
    LPREPCDDS lpcdds = (LPREPCDDS) GetDlgProp (hwnd);
    HWND hCntPath  = GetDlgItem(hwnd,ID_PATHCONTAINER);
    LPRECORDCORE  lprec=CntSelRecGet(hCntPath);
    LPRECORDCORE  lpnewsel;
    PATHS path;

    lpnewsel = lprec->lpNext ? lprec->lpNext : lprec->lpPrev;

    CntDeferPaint( hCntPath );


    CntRecDataGet(hCntPath,lprec,(LPVOID)&path);
    lpcdds->distribution=DeleteListObject(lpcdds->distribution, path.lpOL);

    if (lpnewsel)
        CntSelectRec( hCntPath, lpnewsel);

    CntRemoveRec(hCntPath,lprec);
    CntRangeDec(hCntPath);
    CntEndDeferPaint( hCntPath, TRUE );
    MovePathCombos(hwnd);
    EnableWindow(GetDlgItem(hwnd,ID_REMOVEPATH),CntTopRecGet(hCntPath) ? TRUE : FALSE);
    
    if (CntTotalRecsGet(hCntPath)==0)
      {
      ListBox_ResetContent(GetDlgItem(hwnd,ID_DATABASES_CDDS));
      EnableWindow(GetDlgItem(hwnd,ID_ALTER_DATABASES_CDDS), FALSE);
      }
}


static void AddPath(HWND hwnd)
{
    LPOBJECTLIST  lpNew;
    LPREPCDDS     lpcdds = (LPREPCDDS) GetDlgProp (hwnd);
    HWND hCntPath  = GetDlgItem(hwnd,ID_PATHCONTAINER);
    LPCNTPATHDATA lpcntdata=(LPCNTPATHDATA) CntUserDataGet(hCntPath);

    lpNew = AddListObjectTail(&lpcdds->distribution, sizeof(DD_DISTRIBUTION));

    if (!lpNew)
        return;

    if (lpcntdata)

    {
        LPRECORDCORE lpRC = CntNewRecCore(sizeof(PATHS));
        PATHS path;
        LPDD_DISTRIBUTION lpdis;


        /* Add a blank container record  */
        ZEROINIT(path);

        path.lpOL = lpNew;

        lpdis =(LPDD_DISTRIBUTION) path.lpOL->lpObject;
        lpdis->CDDSNo  = lpcdds->cdds;
        lpdis->source  = (int)ComboBox_GetItemData(lpcntdata->hcomboorig,0);
        lpdis->localdb = (int)ComboBox_GetItemData(lpcntdata->hcombolocal ,0);
        lpdis->target  = (int)ComboBox_GetItemData(lpcntdata->hcombotarget,0);

        ComboBox_GetLBText(lpcntdata->hcomboorig  ,0,path.original);
        ComboBox_GetLBText(lpcntdata->hcombolocal ,0,path.local   );
        ComboBox_GetLBText(lpcntdata->hcombotarget,0,path.target  );

        CntDeferPaint( hCntPath );
        CntRangeInc(hCntPath);
        /* Load data into the record. */
        CntRecDataSet( lpRC, (LPVOID)&path);
        /* Add this record to the list. */
        CntAddRecTail( hCntPath, lpRC );
        /* Set the selection */
        CntSelectRec( hCntPath, lpRC);
        FORWARD_WM_VSCROLL(GetWindow(hCntPath,GW_CHILD), 0, SB_BOTTOM, 0, SendMessage);
        CntEndDeferPaint( hCntPath,TRUE);
        MovePathCombos(hwnd);
    }

    EnableWindow(GetDlgItem(hwnd,ID_REMOVEPATH),CntTopRecGet(hCntPath) ? TRUE : FALSE);

}

static void AllTables(HWND hwnd,BOOL badd)
{
    HWND hCntCol = GetDlgItem(hwnd,ID_COLSCONTAINER);
    HWND hwndCtl = GetDlgItem(hwnd,ID_TABLES);
    int i,iIndex;
    
    // hour glass to show intense system activity
    ShowHourGlass();
    // get item have the current selection   
    iIndex    = CAListBox_GetCurSel(hwndCtl);

    CAListBox_Enable(hwndCtl, FALSE );
    CntDeferPaint( hCntCol  );
    for (i=0;i<CAListBox_GetCount(hwndCtl);i++)

    {
        if (CAListBox_GetSel(hwndCtl, i)!=badd)

        {
            CAListBox_SetSel(hwndCtl,badd,i);  
            CAListBox_SetCurSel(hwndCtl, i);
            TableCheckChange(hwnd,hwndCtl,i,TRUE);
                      
        }
    }
    CntEndDeferPaint( hCntCol,TRUE);
    // set item have the current selection   
    CAListBox_SetCurSel(hwndCtl,iIndex);
    CAListBox_Enable(hwndCtl, TRUE );
    TableCheckChange(hwnd,hwndCtl,iIndex,FALSE);
    RemoveHourGlass();
}


static BOOL IsSystemReplicationTable(LPSTR lpBuf)
{
    TCHAR *ptcTemp;
    int i;
    // goto the last "logical character"
    ptcTemp = _tcsdec(lpBuf,lpBuf + _tcslen(lpBuf));

    for (i = 0; i<2 && ptcTemp;i++)
       ptcTemp = _tcsdec(lpBuf,ptcTemp);

    if (!ptcTemp || ptcTemp == lpBuf)
       return FALSE; /* the length of lpBuf is <= 3 "logical character" */

    // New Version replicator 1.1 .
    // name of shadow table XXXsha,name of archive table XXXarc 

    if (x_stricmp( ptcTemp, "arc" ) == 0 || x_stricmp( ptcTemp, "sha" ) == 0)
       return TRUE;
    else
       return FALSE;
}

static void FillCAListTable (HWND hdlg,HWND hwndCtl, LPREPCDDS lpcdds)
{
    int     hdl, ires,iIndex;
    char    buf [MAXOBJECTNAME*2];
    char    buffilter [MAXOBJECTNAME];
    char    resultString [MAXOBJECTNAME*3+MAXSPACES];
    LPUCHAR parentstrings [MAXPLEVEL];
    LPOBJECTLIST  lpO;
    BOOL    benabled;

    ZEROINIT (buf);
    ZEROINIT (buffilter);
    parentstrings [0] = lpcdds->DBName;
    parentstrings [1] = NULL;

    hdl      = GetCurMdiNodeHandle ();

    CAListBox_ResetContent(hwndCtl);


    ires = DOMGetFirstObject (hdl, OT_TABLE, 1,
                              parentstrings, FALSE, NULL, buf, buffilter, NULL);

    while (ires == RES_SUCCESS)  {
      iIndex = CB_ERR;
      /* some of the tables are reserved for replication */
      if (
       /*   !x_strcmp(buffilter,lpcdds->DBOwnerName) && */  // to be commented if server part OK
          !IsSystemReplicationTable(buf))  {
         // create the string with the name of table, 50 spaces
         // and the owner of this table.
         memset(resultString,0x20,sizeof(resultString));
                    
         wsprintf(resultString,"%s%s%s",buf,SPECIALSEPARATOR,buffilter);
         iIndex = CAListBox_AddString (hwndCtl, resultString);
         if ( iIndex != CALB_ERRSPACE && iIndex != CALB_ERR )  {
            for (lpO=lpcdds->registered_tables;lpO;lpO=lpO->lpNext) {
               LPDD_REGISTERED_TABLES lptable=(LPDD_REGISTERED_TABLES)lpO->lpObject;
              
               if (x_strcmp(RemoveDisplayQuotesIfAny(StringWithoutOwner(buf)),lptable->tablename) == 0 &&
                          x_strcmp(buffilter,lptable->tableowner) == 0)   {
                  if (lptable->cdds == lpcdds->cdds)  {
                     lptable->bStillInCDDS=TRUE;
                     CAListBox_SetSel(hwndCtl, TRUE,iIndex);
                     }
                  else  {
                     lptable->bStillInCDDS = FALSE;
                     CAListBox_DeleteString(hwndCtl,iIndex);
                     CreateTableString(buf,lptable,buf); // adds "(other cdds number)"
                     // create the string with the name of table, 50 spaces
                     // and the owner of this table.
                     memset(resultString,0x20,sizeof(resultString));
                       
                     wsprintf(resultString,"%s%s%s",buf,SPECIALSEPARATOR,buffilter);
                     iIndex = CAListBox_AddString (hwndCtl, resultString);
                  }
                  CAListBox_SetItemData(hwndCtl, iIndex, lpO);
                  CAListBox_SetCurSel(hwndCtl, iIndex);
                  TableRefresh(hdlg,hwndCtl);
                  TableGetParams(hdlg,hwndCtl,-1);
               }
            }
         }
      }
      ires    = DOMGetNextObject (buf, buffilter, NULL);
    }

    benabled=(CAListBox_GetCount(hwndCtl)>0);

    EnableWindow(GetDlgItem(hdlg,ID_TBLGROUP              ),benabled);
    EnableWindow(GetDlgItem(hdlg,ID_SUPPORTTABLE          ),benabled);
    EnableWindow(GetDlgItem(hdlg,ID_RULES                 ),benabled);
    EnableWindow(GetDlgItem(hdlg,ID_LOOKUPTABLE           ),benabled);
    EnableWindow(GetDlgItem(hdlg,ID_PRIORITYLOOKUP        ),benabled);
    EnableWindow(GetDlgItem(hdlg,ID_COLSCONTAINER         ),benabled);
    EnableWindow(GetDlgItem(hdlg,ID_LOOKUPTABLESTATIC     ),benabled);
    EnableWindow(GetDlgItem(hdlg,ID_PRIORITYLOOKUPSTATIC  ),benabled);
    EnableWindow(GetDlgItem(hdlg,ID_GROUPCOLUMNS          ),benabled);
    EnableWindow(GetDlgItem(hdlg,ID_COLADDALL             ),benabled);
    EnableWindow(GetDlgItem(hdlg,ID_COLNONE               ),benabled);
    EnableWindow(GetDlgItem(hdlg,ID_TABLES                ),benabled);
    EnableWindow(GetDlgItem(hdlg,ID_TBLADDALL             ),benabled);
    EnableWindow(GetDlgItem(hdlg,ID_TBLNONE               ),benabled);

    CAListBox_SetCurSel(hwndCtl, 0);
    TableCheckChange(hdlg,hwndCtl,-1,FALSE);
    TableRefresh(hdlg,hwndCtl);
}


static void FillPropagationPaths(HWND hwnd,HWND hwndCtl, LPREPCDDS lpcdds)
{
    LPOBJECTLIST  lpO;
    LPRECORDCORE  lpRC;

    CntDeferPaint( hwndCtl);

    CntKillRecList(hwndCtl);
    CntRangeSet( hwndCtl, 0, 0 );

    /* For all paths */
    for (lpO=lpcdds->distribution;lpO;lpO=lpO->lpNext)

    {
        PATHS path;
        LPOBJECTLIST  lpOConnect= lpcdds->connections;
        LPDD_DISTRIBUTION lpdis =(LPDD_DISTRIBUTION) lpO->lpObject;
        if (lpdis->CDDSNo!=(int)(lpcdds->cdds))
                continue;
        lpRC = CntNewRecCore(sizeof(PATHS));

        ZEROINIT(path);

        while (lpOConnect)

        {
            LPDD_CONNECTION lpconn=(LPDD_CONNECTION) lpOConnect->lpObject;
//          if (lpconn->CDDSNo!=lpcdds->cdds && lpconn->CDDSNo>=0)
//              continue;
//          (not needed because only node and db are used     

            if (lpconn->dbNo == lpdis->source) 
                CreatePathString(path.original,lpconn);
            if (lpconn->dbNo == lpdis->localdb)  
                CreatePathString(path.local,lpconn);
            if (lpconn->dbNo == lpdis->target)  
                CreatePathString(path.target,lpconn);
            lpOConnect=lpOConnect->lpNext;
        }
        path.lpOL = lpO;
        /* Load data into the record. */
        CntRecDataSet( lpRC, (LPVOID)&path);
        /* Add this record to the list. */
        CntAddRecTail( hwndCtl, lpRC );
        CntRangeInc(hwndCtl);
    }
    lpRC = CntTopRecGet(hwndCtl);
    if (lpRC)

    {
        CntSelectRec(hwndCtl,CntTopRecGet(hwndCtl));
    }
    else

    {
        EnableWindow(GetDlgItem(hwnd,ID_REMOVEPATH),FALSE);
    }
   CntEndDeferPaint( hwndCtl, TRUE );

}


static BOOL FillControlsFromStructure (HWND hdlg, LPREPCDDS lpcdds)
{
    char    szWork[256];
    int     hdl = GetCurMdiNodeHandle ();

    /* Fill CDDS number */

    if (lpcdds->bAlter)
       my_itoa (lpcdds->cdds, szWork ,10);
    else {  
       GetFirstAvailableCDDS(lpcdds,szWork);
       lpcdds->cdds = my_atoi ( szWork );
    }

    Edit_SetText(GetDlgItem(hdlg,ID_CDDSNUM),szWork);

    if (lpcdds->bAlter == TRUE) 
        Edit_SetReadOnly(GetDlgItem(hdlg,ID_CDDSNUM), TRUE);

    
    /* Fill CDDS name */
    if (lpcdds->bAlter)
      Edit_SetText(GetDlgItem(hdlg,ID_NAME_CDDS),lpcdds->szCDDSName);

    /* for this CDDS add propagation paths */
    FillPropagationPaths(hdlg,GetDlgItem(hdlg,ID_PATHCONTAINER),lpcdds);

    /*  */
    FillCAListTable (hdlg,GetDlgItem(hdlg,ID_TABLES),lpcdds);

    return TRUE;
}

static void AddCombosPaths(HWND hwnd,LPREPCDDS lpcdds)
{
    LPCNTPATHDATA lpcntpathdata=(LPCNTPATHDATA) CntUserDataGet(GetDlgItem(hwnd,ID_PATHCONTAINER));

    if (lpcntpathdata)

    {
        LPOBJECTLIST  lpOL;
        LPDD_CONNECTION lpconn  ;
        LPDD_CONNECTION lpconn1 ;
            
        for (lpOL = lpcdds->connections ; lpOL ; lpOL = lpOL->lpNext)

        {
            char buf[PATHS_LENGTH];
            int index;
             LPOBJECTLIST  lpOL1;
             BOOL bAlreadyInList=FALSE;
             for (lpOL1 = lpcdds->connections ; lpOL1!=lpOL ; lpOL1 = lpOL1->lpNext) {
                lpconn  =(LPDD_CONNECTION)lpOL ->lpObject;
                lpconn1 =(LPDD_CONNECTION)lpOL1->lpObject;
                if (!x_stricmp(lpconn->szVNode, lpconn1->szVNode) &&
                    !x_stricmp(lpconn->szDBName,lpconn1->szDBName)) {
                   bAlreadyInList=TRUE;
                   break;
                }
             }
             if (bAlreadyInList)
                 continue;

            CreatePathString(buf,(LPDD_CONNECTION) lpOL->lpObject);
            index=ComboBox_AddString(lpcntpathdata->hcomboorig, buf);
            ComboBox_SetItemData(lpcntpathdata->hcomboorig,index,((LPDD_CONNECTION) lpOL->lpObject)->dbNo);

            index=ComboBox_AddString(lpcntpathdata->hcombolocal, buf);
            ComboBox_SetItemData(lpcntpathdata->hcombolocal,index,((LPDD_CONNECTION) lpOL->lpObject)->dbNo);

            index=ComboBox_AddString(lpcntpathdata->hcombotarget, buf);
            ComboBox_SetItemData(lpcntpathdata->hcombotarget,index,((LPDD_CONNECTION) lpOL->lpObject)->dbNo);
        }
        ComboBox_SetCurSel(lpcntpathdata->hcomboorig,0);
        ComboBox_SetCurSel(lpcntpathdata->hcombolocal,0);
        ComboBox_SetCurSel(lpcntpathdata->hcombotarget,0);
    }
}

// domdloca.h moved from ..\winmain to hdr directory
#include "domdloca.h"

static BOOL OnInitDialog (HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    LPREPCDDS lpcdds = (LPREPCDDS) lParam;
    LPREPCDDS lpnew  = (LPREPCDDS) ESL_AllocMem(sizeof(REPCDDS));
    UCHAR   szTitle[100];
    DWORD     dwErrModeSelect,dwCollisionModeSelect,nbIndex;

    SetWindowLong(hwnd,DWL_USER,(LONG) lpnew);

    /* Make a copy of the original structure, the current will change */

    if (!lpnew ||  (CopyStructure (hwnd, lpcdds,lpnew)!=RES_SUCCESS))

    {
        myerror(ERR_ALLOCMEM);
        ErrorMessage ( (UINT)IDS_E_CANNOT_ALLOCATE_MEMORY, RES_ERR);
        EndDialog(hwnd,FALSE);
    }

    /* Initialise the drawing procedure for our owner draw fields. */
    lpfnDrawProc = (DRAWPROC)MakeProcInstance((FARPROC)DrawCtnColNew, hInst);

    if (!AllocDlgProp (hwnd, lpcdds))
        return FALSE;
    /* */
    /* force catolist.dll to load */
    /* */
    CATOListDummy();

    /*LoadString (hResource, (UINT)IDS_T_CDDS, szTitle, sizeof (szTitle)); */
    /*SetWindowText (hwnd, szTitle); */

    Edit_LimitText(GetDlgItem(hwnd,ID_CDDSNUM),8);
    Edit_LimitText(GetDlgItem(hwnd,ID_LOOKUPTABLE)   ,MAX_LEN_EDIT);
    Edit_LimitText(GetDlgItem(hwnd,ID_PRIORITYLOOKUP),MAX_LEN_EDIT);
    Edit_LimitText(GetDlgItem(hwnd,ID_NAME_CDDS)     ,MAX_LEN_EDIT);
    
    /* Fill the combobox Errors and Collisions */
    if (!OccupyTypesErrors(hwnd)||!OccupyTypesCollisions(hwnd))
    {
        ASSERT(NULL);
        return FALSE;
    }
    // Select default value for Error and collision Combobox
    if (lpcdds->bAlter) {
      dwErrModeSelect       = lpcdds->error_mode;
      dwCollisionModeSelect = lpcdds->collision_mode;
    }
    else {
      dwErrModeSelect       = REPLIC_ERROR_SKIP_TRANSACTION;
      dwCollisionModeSelect = REPLIC_COLLISION_PASSIVE;
    }

    nbIndex = FindItemData(GetDlgItem (hwnd, ID_ERROR_MODE), dwErrModeSelect);
    if (nbIndex != CB_ERR)
        ComboBox_SetCurSel(GetDlgItem (hwnd, ID_ERROR_MODE), nbIndex );
    else
        ComboBox_SetCurSel(GetDlgItem (hwnd, ID_ERROR_MODE), 0);

    nbIndex = FindItemData(GetDlgItem (hwnd, ID_COLLISION_MODE), dwCollisionModeSelect);
    if (nbIndex != CB_ERR)
        ComboBox_SetCurSel(GetDlgItem (hwnd, ID_COLLISION_MODE), nbIndex );
    else
        ComboBox_SetCurSel(GetDlgItem (hwnd, ID_COLLISION_MODE), 0);

    /* Hide controls used to add path */
    InitializeContainers(hwnd);

    /* Add all connections to combos */
    AddCombosPaths(hwnd,lpcdds);

    FillControlsFromStructure (hwnd, lpcdds);

    bFirstCall = TRUE;
    UpdateDBListFromPaths(hwnd);
    bFirstCall = FALSE;

    richCenterDialog (hwnd);
    /* if no connections are defined, exit */
    if (!lpcdds->connections)
    {
        ErrorMessage ((UINT)IDS_E_NO_CONNECTION_DEFINED, RES_ERR);
        EndDialog(hwnd,FALSE);
    }

    wsprintf(szTitle,
           ResourceString ((UINT)IDS_T_CDDS_DEFINITION_ON),
           GetVirtNodeName (GetCurMdiNodeHandle ()),
           lpcdds->DBName);
    SetWindowText(hwnd, szTitle);

    EnableDisableOKbutton(hwnd);
    EnableDisableAddDropDBbutton(hwnd);

    lpHelpStack = StackObject_PUSH (lpHelpStack, StackObject_INIT ((UINT)IDD_REPL_CDDS));

    bInDlg = TRUE;

    return TRUE;
}

// Emb 14/11/95 : check each table has at least one indexed column replicated
static BOOL CheckIndexForEachTable(HWND hwnd)
{
  BOOL                    bOk;  // each table has at least an index
  LPREPCDDS               lpcdds;
  LPOBJECTLIST            lpoTbl, lpoCol;
  LPDD_REGISTERED_COLUMNS lpCol;
  LPDD_REGISTERED_TABLES  lpTbl;

  lpcdds = (LPREPCDDS)GetDlgProp (hwnd);
  if (!lpcdds)
    return FALSE; //error!

  bOk = FALSE;  // assume no keyed column

  for (lpoTbl=lpcdds->registered_tables; lpoTbl; lpoTbl=lpoTbl->lpNext) {
    lpTbl = (LPDD_REGISTERED_TABLES)(lpoTbl->lpObject);
    if (lpTbl->bStillInCDDS) {
      for (lpoCol=lpTbl->lpCols; lpoCol; lpoCol=lpoCol->lpNext) {
        lpCol = (LPDD_REGISTERED_COLUMNS)(lpoCol->lpObject);
        if (lpCol->key_colums[0] == 'K') {
          bOk = TRUE;   // keyed column met
          break;
        }
      }
      if (!bOk)
        return FALSE;     // no keyed column for the table
    }
  }
  return TRUE;
}
static BOOL CheckAndUpdCDDSNo(HWND hwnd)
{
   char ach[256];
   int numno;
   LPREPCDDS lpcdds = (LPREPCDDS)GetDlgProp (hwnd);
   Edit_GetText(GetDlgItem(hwnd,ID_CDDSNUM),ach,sizeof(ach));
   numno=my_atoi(ach);
   if (numno < 0 || numno > 32767)
   {
      //"CDDS number must be between 0 and 32767."
      MessageBox(GetFocus(), ResourceString(IDS_E_INVALID_CDDS_NO), NULL,MB_OK | MB_ICONEXCLAMATION | MB_TASKMODAL);
      my_itoa (lpcdds->cdds, ach ,10);
      Edit_SetText(GetDlgItem(hwnd,ID_CDDSNUM),ach);
      return FALSE;
   }
   if (numno!=(int)lpcdds->cdds) {
      LPOBJECTLIST  lpO;
      if (!IsAvailableCDDSNo(lpcdds,numno)) {
         UCHAR buf[50];//"CDDS %d already in use."
         wsprintf(buf, ResourceString(IDS_F_MESSAGE_ELEVEN),numno);
         MessageBox(GetFocus(), buf, NULL,MB_OK | MB_ICONEXCLAMATION | MB_TASKMODAL);
         my_itoa (lpcdds->cdds, ach ,10);
         Edit_SetText(GetDlgItem(hwnd,ID_CDDSNUM),ach);
         return FALSE;
      }
      for (lpO=lpcdds->connections;lpO;lpO=lpO->lpNext) {
         LPDD_CONNECTION lpconn=lpO->lpObject;
         if (lpconn->CDDSNo==(int)lpcdds->cdds)
             lpconn->CDDSNo=numno;
      }
      for (lpO=lpcdds->distribution;lpO;lpO=lpO->lpNext) {
         LPDD_DISTRIBUTION lpdis =(LPDD_DISTRIBUTION) lpO->lpObject;
         if (lpdis->CDDSNo==(int)(lpcdds->cdds)) 
            lpdis->CDDSNo= numno;
      }
      lpcdds->cdds=numno;
   }
   return TRUE;
}

static BOOL VerifOnOk(HWND hwnd)
{
    DWORD i,maxItem,MaxCount;
    BOOL bFoundSvr0 = FALSE;
    char ach[MAXOBJECTNAME*4];
    char *Index;
    HWND hCntPath    = GetDlgItem(hwnd,ID_PATHCONTAINER);
    HWND hwndDBCdds  = GetDlgItem(hwnd,ID_DATABASES_CDDS);

    maxItem = ListBox_GetCount(hwndDBCdds);
    for (i=0;i<maxItem;i++)
    {
        ListBox_GetText(hwndDBCdds, i , ach);
        Index = x_strstr(ach,SPECIALDISPLAY);
        if (Index != NULL)
        {
            bFoundSvr0 = TRUE;
            break;
        }
    }

    MaxCount = CntTotalRecsGet(hCntPath);
    if (MaxCount == 0 && bFoundSvr0)
    {
    // If no path defined verify if the the lookup table is define.
        DWORD nId,edlen3;
        HWND hwndTable = GetDlgItem(hwnd,ID_TABLES);

        maxItem = CAListBox_GetCount(hwndTable);
        for (nId=0;nId < maxItem;nId++)
        {
            if (CAListBox_GetSel(hwndTable, nId))
            {
                LPOBJECTLIST lpOL =(LPOBJECTLIST)CAListBox_GetItemData(hwndTable, nId);
                if (lpOL)
                {
                    LPDD_REGISTERED_TABLES lptable= lpOL->lpObject;
                    if (lptable)
                    {
                        edlen3 = lstrlen( lptable->cdds_lookup_table_v11 );
                        if (edlen3)
                            return TRUE;
                    }
                }
            }
        }
    }
    else
        return TRUE;
    return FALSE;
}

static void OnCommand (HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    switch (id)
    {
    case IDOK:
       if (!VerifOnOk(hwnd))
       {
           int iret;
           iret = MessageBox(GetFocus(),ResourceString(IDS_I_ONOK_SPECIAL_HOR_PAR),
                             NULL, MB_ICONQUESTION | MB_YESNO |MB_TASKMODAL);
           if (iret == IDNO)
              break;
       }
       if (!CheckIndexForEachTable(hwnd) )
         {
            //"At least one table has no indexed column replicated"
            ErrorMessage ((UINT)IDS_E_CDDS_NOINDEX, RES_ERR);
            break;
         }
       if (!CheckAndUpdCDDSNo(hwnd))
          break;
       FillStructureFromControls (hwnd);
       if (!CheckPathsConsistency(hwnd))
           break;
       if (!CheckDBServerAndType(hwnd))
           break;
       if (IsPathCorrect(hwnd) && IsCddsCorrect(hwnd))
        {
            LPREPCDDS lpcdds = (LPREPCDDS) GetDlgProp (hwnd);
            BOOL Success = FALSE;

            if (lpcdds->bAlter)

            {
                Success = AlterObject (hwnd);
            }
            else

            {
                Success = CreateObject (hwnd, lpcdds);
            }

            if (!Success)
                break;
            else
                EndDialog (hwnd, TRUE);
        }
        break;

    case IDCANCEL:
        EndDialog (hwnd, FALSE);
        break;

    case ID_ADDPATH:
        AddPath(hwnd);
        UpdateDBListFromPaths(hwnd);
        EnableDisableOKbutton(hwnd);
        EnableDisableAddDropDBbutton(hwnd);
        break;
    case ID_ALTER_DATABASES_CDDS:
        {
        AlterDatabasesCdds(hwnd);
        EnableDisableOKbutton(hwnd);
        EnableDisableAddDropDBbutton(hwnd);
        }
        break;
    case ID_ADD_DATABASES_CDDS:
        {
        AddDatabasesCdds(hwnd);
        EnableDisableOKbutton(hwnd);
        EnableDisableAddDropDBbutton(hwnd);
        }
        break;
    case ID_DROP_DATABASES_CDDS:
        {
        DropDatabasesCdds(hwnd);
        EnableDisableOKbutton(hwnd);
        EnableDisableAddDropDBbutton(hwnd);
        }
        break;
    case ID_REMOVEPATH:
        RemovePath(hwnd);
        UpdateDBListFromPaths(hwnd);
        EnableDisableOKbutton(hwnd);
        EnableDisableAddDropDBbutton(hwnd);
        break;

    case ID_TBLADDALL:
        AllTables(hwnd,TRUE);
        break;

    case ID_TBLNONE:  
        AllTables(hwnd,FALSE);
        break;
    case ID_COLADDALL:
        AllCols(hwnd,TRUE);
        break;
    case ID_COLNONE:  
        AllCols(hwnd,FALSE);
        break;


    case ID_TABLES:
        // Prevent reentrance by SqlCriticalSection Use
        switch (codeNotify)

        {
        case CALBN_SELCHANGE:
#ifndef WIN32
            if (InSqlCriticalSection())
              break;
            StartSqlCriticalSection();
#endif
            TableRefresh(hwnd,hwndCtl);
            EnableDisableOKbutton(hwnd);
            EnableDisableAddDropDBbutton(hwnd);
#ifndef WIN32
            StopSqlCriticalSection();
#endif
            break;

        case CALBN_CHECKCHANGE:
#ifndef WIN32
            if (InSqlCriticalSection())
              break;
            StartSqlCriticalSection();
#endif
            TableCheckChange(hwnd,hwndCtl,-1,TRUE);
            EnableDisableOKbutton(hwnd);
            EnableDisableAddDropDBbutton(hwnd);
#ifndef WIN32
            StopSqlCriticalSection();
#endif
            break;

        }
        break;

    case ID_SUPPORTTABLE:
    case ID_RULES:
        if (codeNotify == BN_CLICKED)
            TableGetParams(hwnd,GetDlgItem(hwnd,ID_TABLES),-1);
        break;


    case ID_LOOKUPTABLE:
    case ID_PRIORITYLOOKUP:
        if (codeNotify == EN_CHANGE)
        {
            TableGetParams(hwnd,GetDlgItem(hwnd,ID_TABLES),-1);
            EnableDisableOKbutton(hwnd);
            EnableDisableAddDropDBbutton(hwnd);
        }
        break;

    case ID_PATHCONTAINER:
        switch (codeNotify)

        {
        case CN_TAB:
            FORWARD_WM_NEXTDLGCTL(hwnd, 0, HIBYTE(GetKeyState(VK_SHIFT)), PostMessage);
            break;

        case CN_RECSELECTED:
        case CN_FLDSIZED:
            MovePathCombos(hwnd);
            break;
        }
        break;

    case ID_COLSCONTAINER:
        switch (codeNotify)

        {
        case CN_TAB:
            FORWARD_WM_NEXTDLGCTL(hwnd, 0, HIBYTE(GetKeyState(VK_SHIFT)), PostMessage);
            break;

        case LAST_CN_MSG:
            ModifyCols(hwnd);
            TableGetParams(hwnd,GetDlgItem(hwnd,ID_TABLES),-1);
            break;
        }
        break;

    case ID_DATABASES_CDDS:
        if (codeNotify == LBN_SELCHANGE)
        {
            EnableDisableOKbutton(hwnd);
            EnableDisableAddDropDBbutton(hwnd);
        }
        break;

    case ID_CDDSNUM:
        switch (codeNotify) {
           case EN_KILLFOCUS:
              CheckAndUpdCDDSNo(hwnd);
              EnableDisableOKbutton(hwnd);
              break;
        }
        break;

    case ID_NAME_CDDS:
        if (codeNotify == EN_CHANGE)
        {
            EnableDisableOKbutton(hwnd);
            EnableDisableAddDropDBbutton(hwnd);
        }
        break;




    }
}


static void OnSysColorChange (HWND hwnd)
{
#ifdef _USE_CTL3D
    Ctl3dColorChange();
#endif
}


static void OnDestroy (HWND hwnd)
{
    HWND hCntCol   = GetDlgItem(hwnd,ID_COLSCONTAINER);
    HWND hCntPath  = GetDlgItem(hwnd,ID_PATHCONTAINER);
    LPCNTCOLDATA lpcntcoldata;
    LPCNTPATHDATA lpcntPathData;
    LPREPCDDS lp1 = (LPREPCDDS)GetWindowLong(hwnd,DWL_USER);
    LPREPCDDS lp2 = (LPREPCDDS)GetDlgProp (hwnd);

    if (lp1)

    {
        FreeAttachedPointers (lp1, OT_REPLIC_CDDS);
        ESL_FreeMem(lp1);
    }

    if (lp2)

    {
        FreeAttachedPointers (lp2, OT_REPLIC_CDDS);
    }

    lpcntcoldata=(LPCNTCOLDATA)CntUserDataGet(hCntCol);
    if (lpcntcoldata)
        SetWindowLong(GetWindow(hCntCol,GW_CHILD),GWL_WNDPROC,(LONG) lpcntcoldata->fp);

    lpcntPathData=(LPCNTPATHDATA)CntUserDataGet(hCntPath);
    if (lpcntPathData)
        SetWindowLong(GetWindow(hCntPath,GW_CHILD),GWL_WNDPROC,(LONG) lpcntPathData->fp);

    // kill all the lines in the containers
    CntKillRecList(hCntCol);
    CntKillRecList(hCntPath);

    bInDlg = FALSE;


    FreeProcInstance(lpfnDrawProc);
    DeallocDlgProp(hwnd);
    lpHelpStack = StackObject_POP (lpHelpStack);
}


BOOL CALLBACK __export DlgCDDSProcv11
(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)

    {
        HANDLE_MSG (hwnd, WM_COMMAND, OnCommand);
        HANDLE_MSG (hwnd, WM_DESTROY, OnDestroy);

    case WM_INITDIALOG:
        return HANDLE_WM_INITDIALOG (hwnd, wParam, lParam, OnInitDialog);

    default:
        return FALSE;
    }
    return TRUE;
}


int WINAPI __export DlgCDDSv11 (HWND hwndOwner, LPREPCDDS lpcdds)

/*
* Function: 
* Shows the Alter group dialog. 
* Paramters: 
*     1) hwndOwner    :   Handle of the parent window for the dialog. 
*     2) lpcdds:          Points to structure containing information used to  
*                         initialise the dialog. The dialog uses the same 
*                         structure to return the result of the dialog. 
* Returns: 
* TRUE if successful. */
{
    FARPROC lpProc;
    int     retVal;

    if (!IsWindow (hwndOwner) || !lpcdds)

    {
        ASSERT(NULL);
        return FALSE;
    }

    lpProc = MakeProcInstance ((FARPROC) DlgCDDSProcv11, hInst);
    retVal = VdbaDialogBoxParam
      (hResource,
      MAKEINTRESOURCE (IDD_REPL_CDDS_NEW),
      hwndOwner,
      (DLGPROC) lpProc,
      (LPARAM)  lpcdds
      );

    FreeProcInstance (lpProc);
    return (retVal);
}


