/*****************************************************************************
**
** Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Project  : Visual DBA / Replicator.
**
**    Source   : propagat.c
**
**    Dialog box for Propagating data (use by replicator)
**
**  History after 04-May-1999:
**
**  08-Sep-1999 (schph01)
**    bug #98653
**    VDBA / DOM / Replication : Launched the remote command "execrmcmd()"
**    with the same vnode name used in OpenNodeStruct().
**  14-Feb-2000 (somsa01)
**	  Properly include resource.h .
**  26-Mar-2001 (noifr01)
**   (sir 104270) removal of code for managing Ingres/Desktop
**  10-Dec-2001 (noifr01)
**   (sir 99596) removal of obsolete code and resources
**  02-Sep-2002 (schph01)
**    bug 108645 Replace the call of CGhostname() by the GetLocalVnodeName()
**    function.( XX.XXXXX.gcn.local_vnode parameter in config.dat)
******************************************************************************/

#include "compat.h"
#include "gc.h"
#include "dll.h"
#include "subedit.h"

#include "domdata.h"
#include "dbaginfo.h"
#include "dom.h"
#include "error.h"

#include "getdata.h"
#include "dbadlg1.h"
#include "dlgres.h"
#include "catolist.h"
#include "msghandl.h"
#include "lexical.h"
#include "replutil.h"
#include "dgerrh.h"
#include "winutils.h" // ShowHourGlass(),RemoveHourGlass()
#include "extccall.h"

// New as of May 12, 98 for right pane change on replicator item
#include "tree.h"   // LPTREERECORD
extern void MfcClearReplicRightPane(HWND hWnd, LPTREERECORD lpRecord);   // from childfrm.cpp

BOOL CALLBACK __export DlgPropagateDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

static void OnDestroy(HWND hwnd);
static void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
static BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);
static void OnSysColorChange(HWND hwnd);
static void EnableDisableOKButton (HWND hwnd);
static void GetVnodeandDbnameTarget(HWND hwnd, UCHAR * szName);

static BOOL OccupyDbnumberControl (HWND hwnd);
static void RefreshBranchesOnTargetNode(LPPROPAGATE lppropadb);
static void RefreshReplicRightPaneOnTargetNode(LPPROPAGATE lppropadb);
static BOOL HasParentStaticReplicator(LPDOMDATA lpDomData, DWORD dwCurSel, char pReplicDbName[]);

BOOL WINAPI __export DlgPropagate (HWND hwndOwner ,LPPROPAGATE lpPropagate) 
/*
   Function:
      Shows the system modification dialog.

   Paramters:
      hwndOwner   - Handle of the parent window for the dialog.
      lpPropagate    - Points to structure containing information used to 
                  - initialise the dialog. The dialog uses the same
                  - structure to return the result of the dialog.

   Returns:
      TRUE if successful.
*/
{
   FARPROC lpProc;
   int     RetVal;

   if (!IsWindow(hwndOwner) || !lpPropagate)
   {
     ASSERT(NULL);
     return FALSE;
   }
                                        
   lpProc = MakeProcInstance ((FARPROC) DlgPropagateDlgProc, hInst);

   RetVal = VdbaDialogBoxParam (hResource,
                            MAKEINTRESOURCE(IDD_PROPAGATE),
                            hwndOwner,
                            (DLGPROC) lpProc,
                            (LPARAM)lpPropagate );

   FreeProcInstance (lpProc);

   return (RetVal);
}


BOOL CALLBACK __export DlgPropagateDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  switch (message)
  {
   HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
   HANDLE_MSG(hwnd, WM_DESTROY, OnDestroy);

   case WM_INITDIALOG:
     return HANDLE_WM_INITDIALOG(hwnd, wParam, lParam, OnInitDialog);

   default:
     return FALSE;
  }
  return TRUE;
}


static BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
   int nProd = 0;
   char Title[200];
   // HWND hwndproductall = GetDlgItem(hwnd, IDC_PRODUCT_ALL);
   LPPROPAGATE lpPropagate = (LPPROPAGATE)lParam;
   
   ZEROINIT (Title);
   
   if (!AllocDlgProp(hwnd, lpPropagate))
      return FALSE;
   lpHelpStack = StackObject_PUSH (lpHelpStack, StackObject_INIT ((UINT)IDD_PROPAGATE));

   //Fill the windows title bar
   GetWindowText(hwnd,Title,GetWindowTextLength(hwnd)+1);
   x_strcat(Title, " ");
   x_strcat(Title, GetVirtNodeName ( GetCurMdiNodeHandle ()));
   x_strcat(Title, "::");
   x_strcat(Title,lpPropagate->DBName);

   SetWindowText(hwnd,Title);

   if (!OccupyDbnumberControl(hwnd))
   {
     ASSERT(NULL);
     EndDialog(hwnd, FALSE);
     return TRUE;
   }

   richCenterDialog(hwnd);

   return TRUE;
}


static void OnDestroy(HWND hwnd)
{
   DeallocDlgProp(hwnd);
   lpHelpStack = StackObject_POP (lpHelpStack);
}

// moved from ..\winmain to hdr directory
#include "domdisp.h"

// from main.c
extern HWND GetVdbaDocumentHandle(HWND hwndDoc);
#include "treelb.e"             // LM_GETxyz
#include "monitor.h"

#include <resource.h>

static void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
  int  max,i,ires;
  char szMsg[1024];
  char szName[MAXOBJECTNAME*2+2];

  // identification of the node number starting from the node name
  int max_server,cpt,iNode;
  char *nodeName;

  // dom updates
  int ichildobjecttype,level,iret;
  LPUCHAR aparentsResult[MAXPLEVEL];

  // focus
  HWND currentFocus;

   switch (id)
   {
     case IDOK:
     {
       HWND    hwndTargetData  = GetDlgItem (hwnd, IDC_TARGETDATABASE);
       LPPROPAGATE lpdb  = GetDlgProp(hwnd);
       CHAR bufowner[MAXOBJECTNAME];
       CHAR extradata[EXTRADATASIZE];
       UCHAR DBAUsernameOntarget[MAXOBJECTNAME];
       UCHAR szTargetNodeName[MAXOBJECTNAME];
       LPUCHAR parentstrings [MAXPLEVEL];
       int irestype;

       //nCount = CAListBox_GetSelCount(hwndTargetData);
       max    = CAListBox_GetCount(hwndTargetData);

       for (i=0;i<max;i++) {
         if (CAListBox_GetSel (hwndTargetData, i)) {
           CAListBox_GetText(hwndTargetData, i, szName);
           GetVnodeandDbnameTarget(hwnd,szName);

           if (lpdb->iReplicType==REPLIC_V11) {
              UCHAR username[100];
              UCHAR bufcmd[200];
              UCHAR buftitle[120];
              UCHAR szConnect[120];
              UCHAR szTxt[400];
              REPLCONNECTPARAMS lpadd2;
              char buf[MAXOBJECTNAME*2+2];
              int iReplicInstalledOnTarget,hnode,SessNum,repObjType,iResult,dummySesHndl;
              int Dbnumber = CAListBox_GetItemData(hwndTargetData, i);
              BOOL bExistddserverspecial=FALSE;
              ShowHourGlass ();
              // Get DBMS_TYPE for special management on the Gateway.
              ZEROINIT (lpadd2);
              // Database name and Number
              x_strcpy ((char *)lpadd2.DBName, (const char *)lpdb->DBName);
              lpadd2.nNumber = Dbnumber;
              lpadd2.nReplicVersion = lpdb->iReplicType;
              DBAGetUserName(GetVirtNodeName(GetCurMdiNodeHandle()),username);
              repObjType = GetRepObjectType4ll(OT_REPLIC_CONNECTION, lpadd2.nReplicVersion);
              iResult = GetDetailInfo (GetVirtNodeName(GetCurMdiNodeHandle()),
                                       repObjType,
                                       &lpadd2,
                                       FALSE,
                                       &dummySesHndl);
              if (iResult != RES_SUCCESS) {
                assert (FALSE);
                goto refresh;
              }
              if (x_stricmp(lpadd2.szDBMStype,"dcom") == 0) // move config on an gateway DATACOM
              {
                // the version of "ddmovcfg" must support the datacom specific stuff
                // ( available with DATACOM version > 9.0 , but a patch may be required )
                // launch remote command and refresh.
                wsprintf(bufcmd,"ddmovcfg %s %d -u%s",
                         lpdb->DBName,
                         Dbnumber,
                         username);
                wsprintf(buftitle,ResourceString(IDS_REPLIC_PROP_DLGTITLE),
                         GetVirtNodeName(GetCurMdiNodeHandle()),
                         lpdb->DBName,
                         lpdb->TargetNode,
                         lpdb->TargetDB);
                execrmcmd(GetVirtNodeName(GetCurMdiNodeHandle()),
                          bufcmd,
                          buftitle);
                goto refresh;
              }
              else
              {
                if (lpadd2.szDBMStype[0] != 0 &&
                    x_stricmp(lpadd2.szDBMStype,"ingres") != 0) // move config not avalable on an Gataeway.
                {
                  UCHAR bufmsg[250];
                  //"The Propagate functionality is not available for this Server Type (%s) ."
                  //" - Cannot propagate."
                  wsprintf(bufmsg,ResourceString(IDS_F_PROPAGATE_SVR_TYPE),szName);
                  MessageBox(GetFocus(),bufmsg,
                             NULL, MB_ICONHAND | MB_OK | MB_TASKMODAL);
                  goto refresh;
                }
                assert(lpadd2.szDBMStype[0] != 0);
              }
              // verify if dd_server_special exist in current selected database
              //DBAGetUserName(GetVirtNodeName(GetCurMdiNodeHandle()),username);
              parentstrings[0]=lpdb->DBName;
              parentstrings[1]=NULL;
              ires= DOMGetObjectLimitedInfo(GetCurMdiNodeHandle(),
                                            "dd_server_special",
                                            username,
                                            OT_TABLE,
                                            1,
                                            parentstrings,
                                            TRUE,
                                            &irestype,
                                            buf,
                                            bufowner,
                                            extradata);
              if (ires==RES_SUCCESS)
                  bExistddserverspecial=TRUE;
              ires= DOMGetObjectLimitedInfo(GetCurMdiNodeHandle(),
                                            "DD_SERVER_SPECIAL",
                                            username,
                                            OT_TABLE,
                                            1,
                                            parentstrings,
                                            TRUE,
                                            &irestype,
                                            buf,
                                            bufowner,
                                            extradata);
              if (ires==RES_SUCCESS)
                  bExistddserverspecial=TRUE;

              parentstrings [0] = lpdb->TargetDB;
              parentstrings [1] = NULL;
              hnode = OpenNodeStruct  (lpdb->TargetNode);
              if (hnode<0) {
                MessageBox(GetFocus(),ResourceString(IDS_ERR_NB_CONNECT_REACHED),
                                    NULL, MB_ICONHAND | MB_OK | MB_TASKMODAL);
                goto refresh;
              }
              // Temporary for activate a session
              DOMGetFirstObject (hnode, OT_DATABASE, 0, NULL, FALSE, NULL, buf, NULL, NULL);
              //Get DBA user name for this database
              iret = DOMGetObjectLimitedInfo(hnode,
                                      parentstrings [0],
                                      "",
                                      OT_DATABASE,
                                      0,
                                      parentstrings,
                                      TRUE,
                                      &irestype,
                                      buf,
                                      DBAUsernameOntarget,
                                      extradata
                                      );
              CloseNodeStruct(hnode,FALSE);
              if (iret != RES_SUCCESS) {
                wsprintf(buf,ResourceString(IDS_REPLIC_DB_NOTFOUND_PROPSTP));
                MessageBox(GetFocus(), buf, NULL,
                           MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL);
                goto refresh;
              }
              // Format string with vnode name and dba user name.
              lstrcpy( szTargetNodeName ,lpdb->TargetNode);
              lstrcat(szTargetNodeName,LPUSERPREFIXINNODENAME);
              lstrcat(szTargetNodeName,DBAUsernameOntarget);
              lstrcat(szTargetNodeName,LPUSERSUFFIXINNODENAME);
              // verify if the catalog for replicator exist
              hnode = OpenNodeStruct  (szTargetNodeName);
              if (hnode<0) {
                MessageBox(GetFocus(),ResourceString(IDS_ERR_NB_CONNECT_REACHED),
                                    NULL, MB_ICONHAND | MB_OK | MB_TASKMODAL);
                goto refresh;
              }
              // Temporary for activate a session
              ires = DOMGetFirstObject (hnode, OT_DATABASE, 0, NULL, FALSE, NULL, buf, NULL, NULL);
              iReplicInstalledOnTarget = GetReplicInstallStatus(hnode,lpdb->TargetDB,DBAUsernameOntarget);
              CloseNodeStruct(hnode,FALSE);
              if (iReplicInstalledOnTarget == REPLIC_NOINSTALL) {
                  char szTitle[200];
                  wsprintf(szTitle,ResourceString (IDS_REPLIC_INSTALL_REPL));
                  wsprintf(szTxt, ResourceString ((UINT)IDS_W_REPL_NOT_INSTALL), lpdb->TargetNode, lpdb->TargetDB);

                  if (MessageBox(NULL, szTxt , szTitle, MB_ICONEXCLAMATION | MB_YESNO| MB_TASKMODAL) == IDYES)  {
                    hnode = OpenNodeStruct  (szTargetNodeName);
                    if (hnode<0) {
                      MessageBox(GetFocus(),ResourceString(IDS_ERR_NB_CONNECT_REACHED),
                                            NULL, MB_ICONHAND | MB_OK | MB_TASKMODAL);
                      goto refresh;
                    }
                    // Temporary for activate a session
                    ires = DOMGetFirstObject (hnode, OT_DATABASE, 0, NULL, FALSE, NULL, buf, NULL, NULL);
                    // Launch remote command replicat
                    // Create internal table for replicator 
                    wsprintf(bufcmd,"repcat %s -u%s",lpdb->TargetDB,DBAUsernameOntarget);
                    wsprintf( buftitle, ResourceString ((UINT)IDS_I_INSTALL_REPLOBJ), // Installing Replication objects for %s::%s
                                lpdb->TargetNode,
                                lpdb->TargetDB);
                    execrmcmd(szTargetNodeName,bufcmd,buftitle);
                    CloseNodeStruct(hnode,FALSE);
                    RefreshBranchesOnTargetNode(lpdb);
                  }
                  else
                    continue; 
              }

              hnode = OpenNodeStruct  (szTargetNodeName);
              if (hnode<0) {
                //"Maximum number of connections has been reached - Cannot propagate."
                MessageBox(GetFocus(),ResourceString(IDS_ERR_NB_CONNECT_REACHED) ,
                                    NULL, MB_ICONHAND | MB_OK | MB_TASKMODAL);
                goto refresh;
              }
              iret= DBACloseNodeSessions(lpdb->TargetNode,TRUE,TRUE);
              if (iret!=RES_SUCCESS) {//"Cannot Close sessions on Target Node."
                  //MessageBox(GetFocus(),ResourceString(IDS_ERR_CLOSE_SESSION), NULL, MB_ICONHAND | MB_OK | MB_TASKMODAL);
                  MessageBox(GetFocus(),ResourceString(IDS_E_CLOSING_SESSION2), NULL, MB_ICONHAND | MB_OK | MB_TASKMODAL);
                  CloseNodeStruct(hnode,FALSE);
                  goto refresh; 
              }
              // Temporary for activate a session
              DOMGetFirstObject (hnode, OT_DATABASE, 0, NULL, FALSE, NULL, buf, NULL, NULL);
              wsprintf(bufcmd,"ddmovcfg %s %d -u%s",
                     lpdb->DBName,
                     Dbnumber,
                     username);
              wsprintf(buftitle,ResourceString(IDS_REPLIC_PROP_DLGTITLE),
                     GetVirtNodeName(GetCurMdiNodeHandle()),
                     lpdb->DBName,
                     lpdb->TargetNode,
                     lpdb->TargetDB);
              execrmcmd(GetVirtNodeName(GetCurMdiNodeHandle()),
                        bufcmd,
                        buftitle);
              wsprintf(szConnect,"%s::%s",szTargetNodeName,lpdb->TargetDB);
              iret = Getsession(szConnect, SESSION_TYPE_INDEPENDENT, &SessNum);
              if (iret != RES_SUCCESS)  {
                  char tmp[MAXOBJECTNAME*2];
                  wsprintf(tmp,ResourceString(IDS_REPLIC_ERR_OPN_SESS),szConnect);
                  MessageWithHistoryButton(GetFocus(), tmp);
                  CloseNodeStruct(hnode,FALSE);
                  goto refresh; 
              }
              iret = CreateDD_SERVER_SPECIALIfNotExist ();
              if (iret != RES_SUCCESS) {
                  MessageWithHistoryButton(GetFocus(), ResourceString(IDS_ERR_CREATE_DD_SERVER_SPECIAL));
                  ReleaseSession(SessNum, RELEASE_COMMIT);
                  CloseNodeStruct(hnode,FALSE);
                  goto refresh; 
              }
              iret = DeleteRowsInDD_SERVER_SPECIAL();
              if (iret != RES_SUCCESS) {
                  //"Failure while updating the 'dd_server_special' catalog."
                  MessageWithHistoryButton(GetFocus(), ResourceString(IDS_ERR_DELETE_DD_SERVER_SPECIAL));
                  ReleaseSession(SessNum, RELEASE_COMMIT);
                  CloseNodeStruct(hnode,FALSE);
                  goto refresh;
              }
              ReleaseSession(SessNum, RELEASE_COMMIT);
              CloseNodeStruct(hnode,FALSE);
              if ( bExistddserverspecial && iret == RES_SUCCESS ) {
                  iret = CopyDataAcrossTables(GetVirtNodeName(GetCurMdiNodeHandle()), lpdb->DBName,
                                              username, "dd_server_special",
                                              lpdb->TargetNode, lpdb->TargetDB,
                                              DBAUsernameOntarget, "dd_server_special",
                                              FALSE,NULL);
                  if (iret != RES_SUCCESS && iret!=RES_ENDOFDATA)
                      // "Failure while populating the 'dd_server_special' catalog."
                      MessageWithHistoryButton(GetFocus(), ResourceString(IDS_ERR_COPY_DD_SERVER_SPECIAL));
              }
refresh:
              ires=RES_SUCCESS;
           }
           else {
              ires=DbaPropagateReplicatorCfg(
                     GetVirtNodeName(GetCurMdiNodeHandle()), lpdb->DBName,
                     lpdb->TargetNode,lpdb->TargetDB);
           }
           if (ires!=RES_SUCCESS) {
             currentFocus = GetFocus();
             wsprintf (szMsg,
                       ResourceString ((UINT)IDS_E_PROPAGATE_FAILED), 
                       //"Propagate failed for %s::%s",
                       lpdb->TargetNode,
                       lpdb->TargetDB); 
             MessageBox(NULL, szMsg, NULL, MB_ICONSTOP|MB_OK|MB_TASKMODAL);
             SetFocus (currentFocus);
             //break; 
           }
           else {
             // Refresh destination nodes - can be several if (local) concerned with
             iNode = -1;
             max_server = GetMaxVirtNodes();
             for (cpt=0; cpt<max_server; cpt++) {
               if (isVirtNodeEntryBusy(cpt)) {
                 char *pDest;
                 BOOL bMustRefresh = FALSE;
                 nodeName = GetVirtNodeName(cpt);
                 pDest=x_strstr(nodeName,ResourceString((UINT)IDS_I_LOCALNODE));
                 //if (x_stricmp(nodeName,ResourceString((UINT)IDS_I_LOCALNODE))) {
                 if (pDest != nodeName) {
                   // Not Local node: compare Target node name with node name
                   char * pDest1 = x_strstr(nodeName, lpdb->TargetNode);
                   if ( pDest1 == nodeName) {
                     iNode = cpt;
                     bMustRefresh = TRUE;
                   }
                 }
                 else {
                   // local node : compare Target node name with gcn.local_vnode in config.dat
                   UCHAR convertedNodeName[MAXOBJECTNAME];
                   memset (convertedNodeName,EOS,MAXOBJECTNAME);
                   GetLocalVnodeName (convertedNodeName,MAXOBJECTNAME);
                   if (convertedNodeName[0] == EOS)
                     x_strcpy(convertedNodeName, nodeName);
                   if (x_stricmp(convertedNodeName, lpdb->TargetNode) == 0) {
                     iNode = cpt;
                     bMustRefresh = TRUE;
                   }
                 }

                 // Refresh branches on node if eligible
                 if (bMustRefresh) {
                   int iloc;
                   int itype[]={OT_TABLE,
                                OT_REPLIC_CONNECTION,
                                OT_REPLIC_REGTABLE,
                                OT_REPLIC_MAILUSER,
                                OT_REPLIC_CDDS };
                   for (iloc=0;iloc<sizeof(itype)/sizeof(int);iloc++) {
                     ichildobjecttype = itype[iloc];
                     level = 1;
                     aparentsResult[0] = lpdb->TargetDB; // only dest. database
                     aparentsResult[1] = NULL;
                     aparentsResult[2] = NULL;
                     iret = UpdateDOMDataDetail(iNode,
                                               ichildobjecttype,
                                               level,
                                               aparentsResult,
                                               FALSE,  // bwithsystem
                                               FALSE,
                                               TRUE,
                                               FALSE,
                                               FALSE);
                     if (iret != RES_SUCCESS) {
                       wsprintf (szMsg,
                                 ResourceString ((UINT)IDS_E_PROPAGATE_FAILED), 
                                 //"Propagate failed for %s::%s",
                                 lpdb->TargetNode,
                                 lpdb->TargetDB); 
                       currentFocus = GetFocus();
                       MessageBox(NULL, szMsg, NULL, MB_ICONSTOP|MB_OK|MB_TASKMODAL);
                       SetFocus (currentFocus);
                     }
                     else {
                       iret = DOMUpdateDisplayData2 (
                                 iNode,
                                 ichildobjecttype,
                                 level,
                                 aparentsResult,
                                 FALSE,          // bWithChildren
                                 ACT_ADD_OBJECT, // iAction so that only already expanded
                                                 // branches will be updated
                                 0L,             // idItem: not meaningful in that case
                                 NULL,           // hwndDOMDoc
                                 FORCE_REFRESH_NONE);
                       if (iret != RES_SUCCESS) {
                         wsprintf (szMsg,
                                   ResourceString ((UINT)IDS_E_PROPAGATE_FAILED), 
                                   //"Propagate failed for %s::%s",
                                   lpdb->TargetNode,
                                   lpdb->TargetDB); 
                         currentFocus = GetFocus();
                         MessageBox(NULL, szMsg, NULL, MB_ICONSTOP|MB_OK|MB_TASKMODAL);
                         SetFocus (currentFocus);
                       }
                     }
                   }
                 }
               }
             }
           }
           RefreshReplicRightPaneOnTargetNode(lpdb);
         }
       }
        RemoveHourGlass ();

       EndDialog(hwnd, TRUE);
       break;
     
       case IDC_TARGETDATABASE:
       if ( codeNotify == CALBN_CHECKCHANGE || codeNotify == CALBN_SELCHANGE ||
                           codeNotify == CALBN_KILLFOCUS )
         EnableDisableOKButton (hwnd);
       break;
     }

     case IDCANCEL:
       EndDialog(hwnd, FALSE);
       break;
   }
}


static void OnSysColorChange(HWND hwnd)
{
#ifdef _USE_CTL3D
   Ctl3dColorChange();
#endif
}

static BOOL OccupyDbnumberControl (HWND hwnd)
/*
   Function:
      Fills the Database Number .

   Parameters:
      hwnd   - Handle to the dialog window.

   Returns:
      TRUE if successful.
*/
{
   int hNode;
   UCHAR buffilter[MAXOBJECTNAME];
   UCHAR bufextra[MAXOBJECTNAME];
   UCHAR szObject[MAXOBJECTNAME*2+2];
   LPUCHAR parentstrings [MAXPLEVEL];
   UCHAR convertedNodeName[MAXOBJECTNAME];
   int ListErr;
   int err;
   BOOL bSystem = TRUE;
   BOOL bRetVal = TRUE;
   HWND hwndCtl      = GetDlgItem (hwnd, IDC_TARGETDATABASE);
   LPPROPAGATE lpdb  = GetDlgProp(hwnd);

   hNode = GetCurMdiNodeHandle();
   CAListBox_ResetContent(hwndCtl);

   if ( lpdb->iReplicType == REPLIC_V11 )   {
      GetVnodeDatabaseAndFillCtrl(hNode, lpdb->DBName,hwndCtl);
   }
   else
   {
       parentstrings [0] = lpdb->DBName;
       if (x_stricmp(GetVirtNodeName(hNode),ResourceString((UINT)IDS_I_LOCALNODE)))
          x_strcpy(convertedNodeName, GetVirtNodeName(hNode));
       else {
          // Get the gcn.local_vnode in config.dat if vnode name is "(local)"
          memset (convertedNodeName,EOS,MAXOBJECTNAME);
          GetLocalVnodeName (convertedNodeName,MAXOBJECTNAME);
          if (convertedNodeName[0] == EOS)
             x_strcpy(convertedNodeName, GetVirtNodeName(hNode));
       }
       err = DOMGetFirstObject(hNode,
                               OT_REPLIC_CONNECTION,
                               1,
                               parentstrings,
                               bSystem,
                               NULL,
                               szObject,
                               buffilter,
                               bufextra);
   
       while (err == RES_SUCCESS)
       {
         char NodeName[MAXOBJECTNAME];
         char DBName[MAXOBJECTNAME];
         int TargetType = getint(buffilter+STEPSMALLOBJ);
         int Dbnumber   = getint(buffilter);

         FillVnodeandDbtargetFromString(NodeName,DBName,szObject);


         if ( (lpdb->iReplicType==REPLIC_V11 || TargetType==REPLIC_FULLPEER ||
               TargetType==REPLIC_PROT_RO) &&
              (x_stricmp(convertedNodeName,NodeName)||
               x_strcmp(lpdb->DBName,DBName)) &&
              x_strcmp(NodeName,"mobile") 
            ) {
            ListErr=CAListBox_AddString(hwndCtl, szObject );
            CAListBox_SetItemData(hwndCtl,ListErr,Dbnumber);
            if (ListErr == CB_ERR || ListErr == CB_ERRSPACE)
              bRetVal=FALSE;
         }
         err = DOMGetNextObject (szObject, buffilter, bufextra);
       }
   }
   return bRetVal;
}

static void EnableDisableOKButton (HWND hwnd)
{
   if (CAListBox_GetSelCount(GetDlgItem (hwnd, IDC_TARGETDATABASE))  == 0 )
       EnableWindow (GetDlgItem (hwnd, IDOK), FALSE);
   else
       EnableWindow (GetDlgItem (hwnd, IDOK), TRUE);
}

static void GetVnodeandDbnameTarget(HWND hwnd, UCHAR * szName)
{
   LPPROPAGATE lpdb  = GetDlgProp(hwnd);
   FillVnodeandDbtargetFromString(lpdb->TargetNode,lpdb->TargetDB,szName);

}
static void RefreshBranchesOnTargetNode( LPPROPAGATE lppropadb)
{
   HWND hwndCurDoc;
   int iret;
   // Update/Rebuild replicator/other branches on target node/database,
   // can be several if (local) concerned with
   hwndCurDoc = GetWindow (hwndMDIClient, GW_CHILD);
   while (hwndCurDoc && GetWindow (hwndCurDoc, GW_OWNER)) {
       hwndCurDoc = GetWindow (hwndCurDoc, GW_HWNDNEXT);
   }
   while (hwndCurDoc) {
       char *pDest;
       BOOL bMustRefresh = FALSE;
       char* nodeName = GetVirtNodeName(GetMDINodeHandle (hwndCurDoc));

       // Skip Nodesel in mdi mode
       int docType = QueryDocType(hwndCurDoc);
       if (docType == TYPEDOC_NODESEL /* && docType != TYPEDOC_UNKNOWN */ ) {
         hwndCurDoc = GetWindow (hwndCurDoc, GW_HWNDNEXT);
         while (hwndCurDoc && GetWindow (hwndCurDoc, GW_OWNER)) {
           hwndCurDoc = GetWindow (hwndCurDoc, GW_HWNDNEXT);
         }
         continue;
       }

       pDest=x_strstr(nodeName,ResourceString((UINT)IDS_I_LOCALNODE));
       if (pDest != nodeName) {
         // Not Local node: compare Target node name with node name
         char *pDest1 = x_strstr(nodeName, lppropadb->TargetNode);
         if (pDest1 == nodeName)
           bMustRefresh = TRUE;
       }
       else {
         // local node : compare Target node name with gcn.local_vnode in config.dat
         UCHAR convertedNodeName[MAXOBJECTNAME];
         memset (convertedNodeName,EOS,MAXOBJECTNAME);
         GetLocalVnodeName (convertedNodeName,MAXOBJECTNAME);
         if (convertedNodeName[0] == EOS)
           bMustRefresh = TRUE; // gcn.local_vnode not define, force refresh
         else {
           if (x_stricmp(convertedNodeName, lppropadb->TargetNode) == 0)
             bMustRefresh = TRUE;
         }
       }

       // Refresh branches on node if eligible
       if (bMustRefresh) {
         if (QueryDocType(hwndCurDoc) == TYPEDOC_DOM) {
           LPDOMDATA lpDomData = (LPDOMDATA)GetWindowLong(GetVdbaDocumentHandle(hwndCurDoc), 0);
           LPUCHAR       aparents[MAXPLEVEL];
           DWORD         id;
           LPTREERECORD  lpRecord;

           //
           // Call UpdateDOMData so that the internal tables will be up-to-date
           //
           aparents[0] = lppropadb->TargetDB;
           aparents[1] = aparents[2] = NULL;
           iret = UpdateDOMDataDetail(lpDomData->psDomNode->nodeHandle,
                               OT_TABLE,
                               1,               // level
                               aparents,
                               lpDomData->bwithsystem,
                               FALSE,TRUE,FALSE,FALSE);
           iret = UpdateDOMDataDetail(lpDomData->psDomNode->nodeHandle,
                                OT_VIEW,
                                1,               // level
                                aparents,
                                lpDomData->bwithsystem,
                                FALSE,TRUE,FALSE,FALSE);
           iret = UpdateDOMDataDetail(lpDomData->psDomNode->nodeHandle,
                                OT_DBEVENT,
                                1,               // level
                                aparents,
                                lpDomData->bwithsystem,
                                FALSE,TRUE,FALSE,FALSE);
           iret = UpdateDOMDataDetail(lpDomData->psDomNode->nodeHandle,
                               OT_REPLIC_CONNECTION,
                               1,               // level
                               aparents,
                               lpDomData->bwithsystem,
                               FALSE,TRUE,FALSE,FALSE);
           iret = UpdateDOMDataDetail(lpDomData->psDomNode->nodeHandle,
                               OT_REPLIC_CDDS,
                               1,               // level
                               aparents,
                               lpDomData->bwithsystem,
                               FALSE,TRUE,FALSE,FALSE);
           iret = UpdateDOMDataDetail(lpDomData->psDomNode->nodeHandle,
                               OT_REPLIC_MAILUSER,
                               1,               // level
                               aparents,
                               lpDomData->bwithsystem,
                               FALSE,TRUE,FALSE,FALSE);
           iret = UpdateDOMDataDetail(lpDomData->psDomNode->nodeHandle,
                               OT_REPLIC_REGTABLE,
                               1,               // level
                               aparents,
                               lpDomData->bwithsystem,
                               FALSE,TRUE,FALSE,FALSE);

           //
           // Update the lines in the tree :
           //

           gMemErrFlag = MEMERR_NOMESSAGE;

           // Make display fit to new contents of the cache
           DOMDisableDisplay   (lpDomData->psDomNode->nodeHandle, 0);
           DOMUpdateDisplayData(lpDomData->psDomNode->nodeHandle, OT_TABLE,1,aparents,
                               FALSE,ACT_BKTASK,0L,0);
           DOMUpdateDisplayData(lpDomData->psDomNode->nodeHandle, OT_VIEW, 1,aparents,
                               FALSE,ACT_BKTASK,0L,0);
           DOMUpdateDisplayData(lpDomData->psDomNode->nodeHandle, OT_DBEVENT, 1,aparents,
                               FALSE,ACT_BKTASK,0L,0);
           DOMUpdateDisplayData(lpDomData->psDomNode->nodeHandle, OT_REPLIC_CONNECTION, 1,aparents,
                               FALSE,ACT_BKTASK,0L,0);
           DOMUpdateDisplayData(lpDomData->psDomNode->nodeHandle, OT_REPLIC_CDDS, 1,aparents,
                               FALSE,ACT_BKTASK,0L,0);
           DOMUpdateDisplayData(lpDomData->psDomNode->nodeHandle, OT_REPLIC_REGTABLE, 1,aparents,
                               FALSE,ACT_BKTASK,0L,0);
           DOMUpdateDisplayData(lpDomData->psDomNode->nodeHandle, OT_REPLIC_MAILUSER, 1,aparents,
                               FALSE,ACT_BKTASK,0L,0);
           UpdateDBEDisplay(lpDomData->psDomNode->nodeHandle, aparents[0]);
           DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, 0);

           //
           // Regenerate the replicator branch
           //

           id = SendMessage(lpDomData->hwndTreeLb, LM_GETFIRST, 0, 0L);
           while (id) {
             lpRecord = (LPTREERECORD) SendMessage(lpDomData->hwndTreeLb,
                                                   LM_GETITEMDATA,
                                                   0, (LPARAM)id);
             if (lpRecord->recType == OT_STATIC_REPLICATOR) {
               if (x_stricmp (lpRecord->extra, lppropadb->TargetDB) == 0) {

                 // Reset right pane info for change in property pages
                 MfcClearReplicRightPane(GetVdbaDocumentHandle(hwndCurDoc), lpRecord);

                 // Rebuild sub branches if already expanded
                 if (lpRecord->bSubValid) {
                   DWORD idChildObj;
                   TreeDeleteAllChildren(lpDomData, id);
                   // rebuild dummy sub-item and collapse it
                   idChildObj = AddDummySubItem(lpDomData, id);
                   SendMessage(lpDomData->hwndTreeLb, LM_COLLAPSE,
                               0, (LPARAM)id);
                   lpRecord->bSubValid = FALSE;
                 }
               }
             }
             id = SendMessage(lpDomData->hwndTreeLb, LM_GETNEXT, 0, (LPARAM)id);
           }
         }
       }

       // Get next document handle (with loop to skip the icon title windows)
       hwndCurDoc = GetWindow (hwndCurDoc, GW_HWNDNEXT);
       while (hwndCurDoc && GetWindow (hwndCurDoc, GW_OWNER)) {
         hwndCurDoc = GetWindow (hwndCurDoc, GW_HWNDNEXT);
       }
   }
}

static void RefreshReplicRightPaneOnTargetNode(LPPROPAGATE lppropadb)
{
   HWND hwndCurDoc;
   // for each dom, if current selection is "static replicator" on targetDB, refresh right pane
   // can be several
   hwndCurDoc = GetWindow (hwndMDIClient, GW_CHILD);
   while (hwndCurDoc && GetWindow (hwndCurDoc, GW_OWNER)) {
       hwndCurDoc = GetWindow (hwndCurDoc, GW_HWNDNEXT);
   }
   while (hwndCurDoc) {
       char* nodeName = GetVirtNodeName(GetMDINodeHandle (hwndCurDoc));

       int docType = QueryDocType(hwndCurDoc);
       if (docType == TYPEDOC_DOM) {
           HWND hwndDomDoc = GetVdbaDocumentHandle(hwndCurDoc);
           LPDOMDATA lpDomData = (LPDOMDATA)GetWindowLong(hwndDomDoc, 0);
           DWORD dwCurSel = GetCurSel(lpDomData);
           if (dwCurSel > 0) {
               char replicDbName[MAXOBJECTNAME];
               if (HasParentStaticReplicator(lpDomData, dwCurSel, replicDbName)) {
                   if (x_stricmp (replicDbName, lppropadb->TargetDB) == 0) {
                     // Must frame UpdateRightPane() call with push/pop oivers, since might not be current doc!
                     int oldOiVers = SetOIVers (lpDomData->ingresVer);
                     UpdateRightPane(hwndDomDoc, lpDomData, TRUE, 0);
                     SetOIVers(oldOiVers);
                   }
               }
           }
       }

       // Get next document handle (with loop to skip the icon title windows)
       hwndCurDoc = GetWindow (hwndCurDoc, GW_HWNDNEXT);
       while (hwndCurDoc && GetWindow (hwndCurDoc, GW_OWNER)) {
         hwndCurDoc = GetWindow (hwndCurDoc, GW_HWNDNEXT);
       }
   }
}

static BOOL HasParentStaticReplicator(LPDOMDATA lpDomData, DWORD dwCurrentSel, char pReplicDbName[])
{
    int level;
    DWORD dwSel;

    // Initialization
    dwSel = dwCurrentSel;
    if (dwSel)
        level = SendMessage(lpDomData->hwndTreeLb, LM_GETLEVEL, 0, dwSel);
    else
        level = -1;

    // Loop from current sel up to root item
    while (dwSel > 0 && level >= 0) {
      int itemObjType = GetItemObjType(lpDomData, dwSel);
      if (itemObjType == OT_STATIC_REPLICATOR) {
          LPTREERECORD lpRecord = (LPTREERECORD) SendMessage(lpDomData->hwndTreeLb,
                                                             LM_GETITEMDATA, 0, (LPARAM)dwSel);
          x_strcpy(pReplicDbName, lpRecord->extra);
          return TRUE;
      }

      // get the parent id and it's level
      dwSel = SendMessage(lpDomData->hwndTreeLb, LM_GETPARENT, 0, (LPARAM)dwSel);
      if (dwSel > 0)
        level = SendMessage(lpDomData->hwndTreeLb, LM_GETLEVEL, 0, dwSel);
      else
          level = -1;
    }

    // Not found
    *pReplicDbName = '\0';
    return FALSE;
}
