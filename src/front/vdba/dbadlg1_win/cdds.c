/********************************************************************
**
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Project  : Visual DBA
**
**    Source   : cdds.c
**
**    Add / modify replication CDDS 
** History:
** uk$so01 (20-Jan-2000, Bug #100063)
**         Eliminate the undesired compiler's warning
**  26-May-2000
**    bug #99242 . cleanup for DBCS compliance
**  02-Jun-2010 (drivi01)
**    Remove hard coded buffer sizes.
********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
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
#include <tchar.h>


static void PathCntOnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
static void ColCntOnLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags);
static void ColCntOnChar(HWND hwnd, TCHAR ch, int cRepeat);
static void TableCheckChange(HWND hwnd,HWND hwndCtl,int id, BOOL bStatusChanging);

// Prototyped Emb 15/11/95 (force columns data in linked list)
static void  GetColumnsParams(HWND hwnd,LPDD_REGISTERED_TABLES lptable);

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
    FARPROC fp;                       /* for subclassing combo selection changes */
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
    int  ikey;
} COLS, FAR *LPCOLS;


static void ChangePath(HWND hCntPath,LPCNTPATHDATA lpcntdata)
{
    LPRECORDCORE  lprec = CntFocusRecGet(hCntPath);
    LPDD_DISTRIBUTION lpdis;
    PATHS path;
    int   local,source,target;

    local  = (int)ComboBox_GetItemData(lpcntdata->hcomboorig,ComboBox_GetCurSel(lpcntdata->hcomboorig));
    source = (int)ComboBox_GetItemData(lpcntdata->hcombolocal ,ComboBox_GetCurSel(lpcntdata->hcombolocal ));
    target = (int)ComboBox_GetItemData(lpcntdata->hcombotarget,ComboBox_GetCurSel(lpcntdata->hcombotarget));

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

LRESULT WINAPI __export PathCntWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    HWND hCnt =GetParent(hwnd);
    LPCNTPATHDATA lpcntdata=(LPCNTPATHDATA)CntUserDataGet(hCnt);
    if (lpcntdata)
    {
        LRESULT lret = CallWindowProc((WNDPROC)lpcntdata->fp,
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
        ChangePath(hCnt,lpcntdata);
    }
}


LRESULT WINAPI __export ColCntWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    HWND hCnt =GetParent(hwnd);
    LPCNTCOLDATA lpcntcoldata=(LPCNTCOLDATA)CntUserDataGet(hCnt);
    if (lpcntcoldata)

    {
        LRESULT lret = CallWindowProc((WNDPROC)lpcntcoldata->fp,
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
    EnableWindow(GetDlgItem(hwnd,ID_COLADDALL ),bEnabled);
    EnableWindow(GetDlgItem(hwnd,ID_COLNONE   ),bEnabled);



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


      if (badd)
       {
       if (cols.ikey)
         bOneKey = TRUE;
       *cols.repkey= cols.ikey ? 'K' : 0;
       *cols.repcol= 'R';
       }
      else
       {
       *cols.repkey=0;
       *cols.repcol=0;
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


static void FillColumns(LPREPCDDS lpcdds ,LPDD_REGISTERED_TABLES lptable,HWND hwndCnt, HWND hDlg)
{
    int          err;
    TABLEPARAMS  tableparams;
    int          SesHndl;          // current session handle
    LPOBJECTLIST lpOC;


    ZEROINIT (tableparams);

    x_strcpy (tableparams.DBName,     lpcdds->DBName);
    x_strcpy (tableparams.objectname, lptable->tablename);
    x_strcpy (tableparams.szSchema,   lpcdds->DBOwnerName);

    err = GetDetailInfo (GetVirtNodeName (GetCurMdiNodeHandle ()) , OT_TABLE, &tableparams, FALSE,
                         &SesHndl);

    if (err != RES_SUCCESS)
        return;

    CntDeferPaint( hwndCnt  );

    CntKillRecList(hwndCnt);
    CntRangeSet( hwndCnt, 0, 0 );


    for (lpOC=tableparams.lpColumns;lpOC;lpOC=lpOC->lpNext)
       {
       LPRECORDCORE lpRC = CntNewRecCore(sizeof(COLS));
       LPCOLUMNPARAMS lpCP=(LPCOLUMNPARAMS)lpOC->lpObject;

       LPOBJECTLIST  lpO;
       COLS          col;

       ZEROINIT(col);
       x_strcpy(col.name,lpCP->szColumn);
       col.ikey=lpCP->nPrimary;

       *col.repkey= col.ikey ? 'K' : 0;
       *col.repcol= 'R';

       
       /* See if column already registered */
       for (lpO=lptable->lpCols;lpO;lpO=lpO->lpNext)

       {
           LPDD_REGISTERED_COLUMNS lpcol = (LPDD_REGISTERED_COLUMNS)lpO->lpObject;
           if (lstrcmp (lpcol->columnname,lpCP->szColumn) == 0)

           {
               lstrcpy(col.repkey,lpcol->key_colums);
               lstrcpy(col.repcol,lpcol->rep_colums);
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


    // Ps + Emb 05/03/96 : FreeAttachedPointers taken out of the 'for' loop
    FreeAttachedPointers (&tableparams,  OT_TABLE);

    CntSelectRec(hwndCnt,CntTopRecGet(hwndCnt));

    // Emb 14/11/95 to force at least one indexed column
    AllCols(hDlg, TRUE);

    // Emb 15/11/95 to force columns data in linked list
    GetColumnsParams(hDlg, lptable);

    CntEndDeferPaint( hwndCnt, TRUE );
}


static void TableRefresh(HWND hwnd,HWND hwndCtl)
{
    int id = CAListBox_GetCurSel(hwndCtl);

    if (id == LB_ERR)
        return;

    if (CAListBox_GetSel(hwndCtl, id))

    {
        LPOBJECTLIST lpOL = (LPOBJECTLIST) CAListBox_GetItemData(hwndCtl, id);

        if (lpOL)

        {
            LPDD_REGISTERED_TABLES lptable=lpOL->lpObject;

            EnableTableParams(hwnd,TRUE);

            if (lptable)

            {
                LPREPCDDS lpcdds = (LPREPCDDS)GetDlgProp (hwnd);

                lpcdds->flag=TRUE;

                Edit_SetText(GetDlgItem(hwnd,ID_LOOKUPTABLE   ),
                  lptable->rule_lookup_table    );

                Edit_SetText(GetDlgItem(hwnd,ID_PRIORITYLOOKUP),
                  lptable->priority_lookup_table);

                Button_SetCheck(GetDlgItem(hwnd,ID_SUPPORTTABLE),
                  *lptable->table_created ==  'T');

                Button_SetCheck(GetDlgItem(hwnd,ID_RULES),
                  *lptable->rules_created ==  'R');

                lpcdds->flag=FALSE;
                FillColumns(lpcdds,lptable,GetDlgItem(hwnd,ID_COLSCONTAINER), hwnd);
            }
        }
      else
        EnableTableParams(hwnd,FALSE);

    }
    else
        EnableTableParams(hwnd,FALSE);
}


static void TableCheckChange(HWND hwnd,HWND hwndCtl,int id, BOOL bStatusChanging)
{
    LPDD_REGISTERED_TABLES lptable  = NULL;
    char ach[MAXOBJECTNAME*2];
    LPREPCDDS lpcdds                = (LPREPCDDS)GetDlgProp (hwnd);


    //if (id==-1)
    //    id = CAListBox_GetCurSel(hwndCtl);
    if (id==-1)   {
        id = CAListBox_GetCurSel(hwndCtl);
        if (id == CALB_ERR )
           return;
    }


    if (CAListBox_GetSel(hwndCtl, id))

    {
        LPOBJECTLIST lpOL= (LPOBJECTLIST)CAListBox_GetItemData(hwndCtl, id);
        if (!lpOL)
           {
           lpOL = (LPOBJECTLIST) AddListObjectTail(&lpcdds->registered_tables, sizeof(DD_REGISTERED_TABLES));
           if (lpOL)
       
           {
               char *p;
               LPDD_REGISTERED_TABLES lptable=(LPDD_REGISTERED_TABLES)lpOL->lpObject;

               memset((char*) lptable,0,sizeof(DD_REGISTERED_TABLES));

               CAListBox_GetText(hwndCtl, id, ach);
               p=x_strchr(ach,' ');  if (p) *p=0;

               x_strcpy(lptable->tablename,ach);
               lptable->cdds=lpcdds->cdds;
               lptable->table_no=++lpcdds->LastNumber;

               CAListBox_SetItemData(hwndCtl, id , lpOL);
               lptable->bStillInCDDS= TRUE;
           }
        }
        else { // table has already been checked in the past (extradata already there)
            LPDD_REGISTERED_TABLES lptable=(LPDD_REGISTERED_TABLES)lpOL->lpObject;
            char *p3;
            CAListBox_GetText(hwndCtl, id, ach);
            p3=x_strchr(ach,'(');
            if (p3 != NULL){
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
            LPDD_REGISTERED_TABLES lptable = (LPDD_REGISTERED_TABLES)lpOL->lpObject;
            lptable->bStillInCDDS= FALSE;
#ifdef OLDIES
            // 02/05/96  PS for deleted columns 
            FreeObjectList(lptable->lpCols);
            lptable->lpCols=NULL;
            lpcdds->registered_tables = DeleteListObject(lpcdds->registered_tables, lpOL);
            CAListBox_SetItemData(hwndCtl, id ,NULL);
#endif
        }
    }
    TableRefresh(hwnd,hwndCtl);
}



static BOOL FillStructureFromControls (HWND hwnd)
{
    LPREPCDDS lpcdds = (LPREPCDDS) GetDlgProp (hwnd);

    if (!lpcdds->bAlter)
    {
        char ach[8];
        Edit_GetText(GetDlgItem(hwnd,ID_CDDSNUM),ach,sizeof(ach));
        lpcdds->cdds=my_atoi(ach);

    }

    return TRUE;
}

static BOOL CreateObject (HWND hwnd, LPREPCDDS lpcdds )
{
    

    if (DBAAddObject ( GetVirtNodeName (GetCurMdiNodeHandle ()) ,
                       OT_REPLIC_CDDS, (void *) lpcdds ) != RES_SUCCESS)

    {
/*       ErrorMessage ((UINT) IDS_E_CREATE_CDDS_FAILED, ires); */
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
   ires    = GetDetailInfo (vnodeName, OT_REPLIC_CDDS, &r2, TRUE, &hdl);

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
                  ires = DBAAddObject (vnodeName, OT_REPLIC_CDDS, (void *) &r3 );
                  if (ires != RES_SUCCESS) {
                    ErrorMessage ((UINT) IDS_E_CREATE_CDDS_FAILED, ires);
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

   ires = DBAAlterObject (lpold, lpr1, OT_REPLIC_CDDS, hdl);
   if (ires != RES_SUCCESS)  {
      Success=FALSE;
      ErrorMessage ((UINT) IDS_E_ALTER_CDDS_FAILED, ires);
      ires    = GetDetailInfo (vnodeName, OT_REPLIC_CDDS, &r2, FALSE, &hdl);
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

static void CreateTableString(LPSTR lpDest,LPDD_REGISTERED_TABLES lptable)
{
    wsprintf(lpDest,"%s (%d)",lptable->tablename,lptable->cdds);
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
    cntColData.fp = (WNDPROC) SetWindowLong(GetWindow(hCntCol,GW_CHILD),GWL_WNDPROC,(LONG) ColCntWndProc);
    CntUserDataSet( hCntCol, (LPVOID) &cntColData,sizeof(cntColData));

    cntPathData.hcomboorig=CreatePathCombo(hwnd,hCntPath);
    cntPathData.hcombolocal=CreatePathCombo(hwnd,hCntPath);
    cntPathData.hcombotarget=CreatePathCombo(hwnd,hCntPath);
    cntPathData.fp = (FARPROC) SetWindowLong(GetWindow(hCntPath,GW_CHILD),GWL_WNDPROC,(LONG) PathCntWndProc);


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
    lpFI->cxWidth      = 8;
    lpFI->cxPxlWidth   = 0;
    lpFI->flFDataAlign = CA_TA_LEFT | CA_TA_VCENTER;
    lpFI->wColType     = CFT_STRING;
    lpFI->flColAttr    = CFA_FLDREADONLY | CFA_FLDTTLREADONLY | CFA_OWNERDRAW;
    lpFI->wDataBytes   = 2;
    lpFI->wOffStruct   = offsetof(COLS, repkey);
    lstrcpy(szTitle, ResourceString ((UINT) IDS_I_REP_KEY));
    CntFldTtlSet(hCntCol, lpFI, szTitle, lstrlen(szTitle) + 1);
    CntFldDrwProcSet(lpFI, (DRAWPROC) lpfnDrawProc);
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

int CALLBACK __export DrawCtnCol( HWND   hwndCnt,     /* container window handle */
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
            *cols.repcol= *cols.repcol ? 0 : 'R';
            if (cols.ikey>0)
               *cols.repkey= *cols.repcol ? 'K' :0 ;
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

                if (!(*cols.repkey) &&  !(*cols.repcol))

                {
                    lptable->lpCols=DeleteListObject(lptable->lpCols, lpCols);
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
                  lptable->rule_lookup_table,
                  sizeof(lptable->rule_lookup_table));

                Edit_GetText(GetDlgItem(hwnd,ID_PRIORITYLOOKUP),
                  lptable->priority_lookup_table,
                  sizeof(lptable->priority_lookup_table));

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

        ComboSetSelFromItemData(lpcntpathdata->hcomboorig , lpdis->localdb);
        ComboSetSelFromItemData(lpcntpathdata->hcombolocal, lpdis->source );
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


//static void GetFirstAvailableCDDS(LPREPCDDS lpcdds,LPSTR lpBuf)
//{
//   int     hdl, ires;
//   char    buf [MAXOBJECTNAME*2];
//   char    buffilter [MAXOBJECTNAME];
//   LPUCHAR parentstrings [MAXPLEVEL];
//   long i;
//
//   ZEROINIT (buf);
//   ZEROINIT (buffilter);
//   parentstrings [0] = lpcdds->DBName;
//   parentstrings [1] = NULL;
//
//   hdl      = GetCurMdiNodeHandle ();
//
//   for (i=0;i<0xffffL;i++)
//       {
//       ires = DOMGetFirstObject (hdl, OT_REPLIC_CDDS, 1,
//              parentstrings, FALSE, NULL, buf, NULL, NULL);
//
//       while (ires == RES_SUCCESS)
//          {
//          long l=my_atodw(buf);
//
//          if (i<l)
//             {
//             my_dwtoa (i, lpBuf,10);
//             return;
//             }
//
//          if (i==l)
//              break;
//
//          ires    = DOMGetNextObject (buf, buffilter, NULL);
//          }
//       }
//   *lpBuf=0;
//}
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

       for (lpO2=lpO->lpNext;lpO2;lpO2=lpO2->lpNext)
          {
          if (memcmp(lpO->lpObject,lpO2->lpObject,sizeof(DD_DISTRIBUTION))==0)
             {
             ErrorMessage((UINT)IDS_E_DUPLICATE_PATH, RES_ERR);
             return FALSE;
             }    
          }

       if (lpdis->localdb==lpdis->target)
           {
             //
             // "Local and target Database must be different."
             //
             ErrorMessage ((UINT) IDS_E_LOCAL_TARGET, RES_ERR);
             return FALSE;
           }

       if (lpdis->source==lpdis->target)
           {
             //
             // "Source and target Database must be different."
             //
             ErrorMessage ((UINT) IDS_E_SOURCE_TARGET, RES_ERR);
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

        lpdis->localdb = (int)ComboBox_GetItemData(lpcntdata->hcomboorig,0);
        lpdis->source  = (int)ComboBox_GetItemData(lpcntdata->hcombolocal ,0);
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
    int i;

    CntDeferPaint( hCntCol  );
    for (i=0;i<CAListBox_GetCount(hwndCtl);i++)

    {
        if (CAListBox_GetSel(hwndCtl, i)!=badd)

        {
            CAListBox_SetSel(hwndCtl,badd,i);  
            TableCheckChange(hwnd,hwndCtl,i,TRUE);
                      
        }
    }
    CntEndDeferPaint( hCntCol,TRUE);
}


static BOOL IsSystemReplicationTable(LPSTR lpBuf)
{
    TCHAR *ptcTemp;
    // goto the last "logical character"
    ptcTemp = _tcsdec(lpBuf,lpBuf + _tcslen(lpBuf));
    if (!ptcTemp)
       return FALSE;
    ptcTemp = _tcsdec(lpBuf,ptcTemp);

    if (!ptcTemp || ptcTemp == lpBuf)
       return FALSE; /* the length of lpBuf is <= 2 "logical character" */

    if (x_stricmp( ptcTemp, "_a" ) == 0 || x_stricmp( ptcTemp, "_s" ) == 0)
        return TRUE;
    else
        return FALSE;
}

static void FillCAListTable (HWND hdlg,HWND hwndCtl, LPREPCDDS lpcdds)
{
    int     hdl, ires;
    char    buf [MAXOBJECTNAME*2];
    char    buffilter [MAXOBJECTNAME];
    LPUCHAR parentstrings [MAXPLEVEL];
    LPOBJECTLIST  lpO;
    BOOL    benabled;

    ZEROINIT (buf);
    ZEROINIT (buffilter);
    parentstrings [0] = lpcdds->DBName;
    parentstrings [1] = NULL;

    hdl      = GetCurMdiNodeHandle ();

    CAListBox_ResetContent(hwndCtl);

    /* add all registered tables */
    for (lpO=lpcdds->registered_tables;lpO;lpO=lpO->lpNext)

    {
        int iIndex;
        LPDD_REGISTERED_TABLES lptable=(LPDD_REGISTERED_TABLES)lpO->lpObject;

        if (lptable->cdds == lpcdds->cdds)

        {
            iIndex=CAListBox_AddString (hwndCtl, lptable->tablename);
            CAListBox_SetSel(hwndCtl, TRUE,iIndex);
            lptable->bStillInCDDS=TRUE;
        }
        else

        {
            CreateTableString(buf,lptable);
            iIndex = CAListBox_AddString (hwndCtl, buf);
            lptable->bStillInCDDS=FALSE;
        }

        CAListBox_SetItemData(hwndCtl, iIndex, lpO);
        CAListBox_SetCurSel(hwndCtl, iIndex);            //PS
        TableRefresh(hdlg,hwndCtl);
        TableGetParams(hdlg,hwndCtl,-1);
    }


    ires = DOMGetFirstObject (hdl, OT_TABLE, 1,
      parentstrings, FALSE, NULL, buf, buffilter, NULL);

    while (ires == RES_SUCCESS)

    {
        BOOL bFnd=FALSE;
        for (lpO=lpcdds->registered_tables;lpO;lpO=lpO->lpNext)
        {
            if (lstrcmp (((LPDD_REGISTERED_TABLES)lpO->lpObject)->tablename,
              buf) ==0)

            {
                bFnd=TRUE;
                break;
                //ires   = DOMGetNextObject (buf, buffilter, NULL);
                //continue;
            }
        }
        if (!bFnd) {
           /* some of the tables are reserved for replication */

           if (!x_strcmp(buffilter,lpcdds->DBOwnerName) &&
               !IsSystemReplicationTable(buf))
               CAListBox_AddString (hwndCtl, buf);
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

static void CreatePathString(LPSTR lpDest,LPDD_CONNECTION lpconn)
{
    wsprintf(lpDest,"%s::%s",lpconn->szVNode,lpconn->szDBName);
}

static void FillPropagationPath(HWND hwnd,HWND hwndCtl, LPREPCDDS lpcdds)
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
        lpRC = CntNewRecCore(sizeof(PATHS));

        ZEROINIT(path);

        while (lpOConnect)

        {
            LPDD_CONNECTION lpconn=(LPDD_CONNECTION) lpOConnect->lpObject;

            if (lpconn->dbNo == lpdis->localdb)
                CreatePathString(path.original,lpconn);
            if (lpconn->dbNo == lpdis->source)
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
    char    szWork[MAXOBJECTNAME*4];
    int     hdl = GetCurMdiNodeHandle ();

    /* Fill CDDS number */

    if (lpcdds->bAlter)
       my_itoa (lpcdds->cdds, szWork ,10);
    else {  
       GetFirstAvailableCDDS(lpcdds,szWork);
       lpcdds->cdds = my_atoi ( szWork );
    }

    Edit_SetText(GetDlgItem(hdlg,ID_CDDSNUM),szWork);

    EnableWindow(GetDlgItem(hdlg,ID_CDDSNUM),!lpcdds->bAlter);

    /* for this CDDS add propagation path */
    FillPropagationPath(hdlg,GetDlgItem(hdlg,ID_PATHCONTAINER),lpcdds);

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
        for (lpOL = lpcdds->connections ; lpOL ; lpOL = lpOL->lpNext)

        {
            char buf[PATHS_LENGTH];
            int index;
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

static BOOL OnInitDialog (HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    LPREPCDDS lpcdds = (LPREPCDDS) lParam;
    LPREPCDDS lpnew  = (LPREPCDDS) ESL_AllocMem(sizeof(REPCDDS));
    UCHAR   szTitle[MAXOBJECTNAME*3+100];

    SetWindowLong(hwnd,DWL_USER,(LONG) lpnew);

    /* Make a copy of the original structure, the current will change */

    if (!lpnew ||  (CopyStructure (hwnd, lpcdds,lpnew)!=RES_SUCCESS))

    {
        myerror(ERR_ALLOCMEM);
        ErrorMessage ( (UINT)IDS_E_CANNOT_ALLOCATE_MEMORY, RES_ERR);
        EndDialog(hwnd,FALSE);
    }

    /* Initialise the drawing procedure for our owner draw fields. */
    lpfnDrawProc = (DRAWPROC)MakeProcInstance((FARPROC)DrawCtnCol, hInst);

    if (!AllocDlgProp (hwnd, lpcdds))
        return FALSE;
    /* */
    /* force catolist.dll to load */
    /* */
    CATOListDummy();

    /*LoadString (hResource, (UINT)IDS_T_CDDS, szTitle, sizeof (szTitle)); */
    /*SetWindowText (hwnd, szTitle); */

    Edit_LimitText(GetDlgItem(hwnd,ID_CDDSNUM),8);
    Edit_LimitText(GetDlgItem(hwnd,ID_LOOKUPTABLE),MAXOBJECTNAME);
    Edit_LimitText(GetDlgItem(hwnd,ID_PRIORITYLOOKUP),MAXOBJECTNAME);

    /* Hide controls used to add path */


    InitializeContainers(hwnd);

    /* Add all connections to combos */
    AddCombosPaths(hwnd,lpcdds);

    FillControlsFromStructure (hwnd, lpcdds);

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
    lpHelpStack = StackObject_PUSH (lpHelpStack, StackObject_INIT ((UINT)IDD_REPL_CDDS));

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

static void OnCommand (HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    switch (id)
    {
    case IDOK:

       if (!CheckIndexForEachTable(hwnd) )
         {
            MessageBox(hwnd,ResourceString ((UINT)IDS_E_CDDS_NOINDEX),//"At least one table has no indexed column replicated",
                       NULL, MB_OK | MB_ICONEXCLAMATION | MB_TASKMODAL);
            //ErrorMessage ((UINT)IDS_E_CDDS_NOINDEX, RES_ERR);
            break;
         }
       FillStructureFromControls (hwnd);
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
        break;

    case ID_REMOVEPATH:
        RemovePath(hwnd);
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
            //EnableDisableOKbutton(hwnd);
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
            //EnableDisableOKbutton(hwnd);
#ifndef WIN32
            StopSqlCriticalSection();
#endif
            break;

        }
        break;

 //   case ID_TABLES:
 //       switch (codeNotify)

 //       {
 //       case CALBN_SELCHANGE:
 //           TableRefresh(hwnd,hwndCtl);
 //           break;

 //       case CALBN_CHECKCHANGE:
 //           TableCheckChange(hwnd,hwndCtl,-1,TRUE);
 //           break;

 //       }
 //       break;

    case ID_SUPPORTTABLE: 
    case ID_RULES:     
        if (codeNotify == BN_CLICKED)
            TableGetParams(hwnd,GetDlgItem(hwnd,ID_TABLES),-1);
        break;


    case ID_LOOKUPTABLE:
    case ID_PRIORITYLOOKUP:
        if (codeNotify == EN_CHANGE)
            TableGetParams(hwnd,GetDlgItem(hwnd,ID_TABLES),-1);
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

    FreeProcInstance(lpfnDrawProc);
    DeallocDlgProp(hwnd);
    lpHelpStack = StackObject_POP (lpHelpStack);
}


BOOL CALLBACK __export DlgCDDSProc
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


int WINAPI __export DlgCDDS (HWND hwndOwner, LPREPCDDS lpcdds)

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

    lpProc = MakeProcInstance ((FARPROC) DlgCDDSProc, hInst);
    retVal = VdbaDialogBoxParam
      (hResource,
      MAKEINTRESOURCE (IDD_REPL_CDDS),
      hwndOwner,
      (DLGPROC) lpProc,
      (LPARAM)  lpcdds
      );

    FreeProcInstance (lpProc);
    return (retVal);
}


