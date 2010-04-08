/*****************************************************************************
**
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Project : CA/OpenIngres Visual DBA
**
**    Source : domdmisc.c
**    Miscellaneous for cache
**
**    Author : Francois Noirot-Nerin
**
** History:
** uk$so01 (20-Jan-2000, Bug #100063)
**         Eliminate the undesired compiler's warning
** 28-Feb-2000(schph01)
**    Sir #100559 add a parameter for the function MfcDlgIcePassword()
** 26-Mar-2001 (noifr01)
**   (sir 104270) removal of code for managing Ingres/Desktop
** 09-May-2001 (noifr01)
**   (bug 104675)  updated the IsStarDataBase() function so that it considers
**   imadb as a non-star database without the need to do a query. This avoids 
**   some loopback problems in GetSession(), in particular when rmcmd is 
**   invoked, which now invokes GetSession() against the imadb database instead
**   if iidbdb
** 10-Dec-2001 (noifr01)
**   (sir 99596) removal of obsolete code and resources
** 02-Sep-2002 (schph01)
**   bug 108645 replace the call to GCHostname by GetLocalVnodeName ().
** 01-Oct-2003 (schph01)
**   bug 111019 Add DOMUpdateDisplayAllDataAllUsers() function to refresh the
**   branches when OT_ICE_WEBUSER is dropped
*****************************************************************************/

#include "dba.h"
#include "msghandl.h"
#include "monitor.h"
#include "dll.h"
#include "resource.h"

#include "domdata.h"
#include "domdloca.h"
#include "dbaginfo.h"
#include "error.h"
#include "domdisp.h"
#include "extccall.h"
struct def_virtnode  virtnode[MAXNODES];

DOMREFRESHPARAMS MonNodesRefreshParms;
BOOL HasSystemMonNodes;
int nbMonNodes;
int monnodeserrcode;
struct MonNodeData   * lpMonNodeData;

int imaxnodes = 10;

BOOL initNodeStruct()
{
   DBAginfoInit();
   lpMonNodeData = (struct MonNodeData * ) 0;
   ActivateBkTask();
   return TRUE;   /* statics are initialized to 0;  */
}

BOOL EndNodeStruct()
{
   int i;

   StopBkTask();
   for (i=0;i<imaxnodes;i++) {
      if (virtnode[i].pdata) 
         FreeNodeStruct(i,FALSE);   
   }
   DBAginfoFree();
   freeRevInfoData();
   freeMonNodeData(lpMonNodeData);
   if (getMonStack())
      myerror(ERR_LEVELNUMBER);
   return TRUE;
}
BOOL isMonSpecialState(int hnode)
{
   if (hnode<0 || hnode >= imaxnodes) {
      myerror(ERR_UPDATEDOMDATA);
      return FALSE;
   }
   if (!virtnode[hnode].pdata) {
      myerror(ERR_UPDATEDOMDATA);
      return FALSE;
   }
   return virtnode[hnode].bIsSpecialMonState;

}
BOOL SetMonNormalState(int hnode)
{
   if (hnode<0 || hnode >= imaxnodes) {
      myerror(ERR_UPDATEDOMDATA);
      return FALSE;
   }
   if (!virtnode[hnode].pdata) {
      myerror(ERR_UPDATEDOMDATA);
      return FALSE;
   }
   virtnode[hnode].bIsSpecialMonState = FALSE;
   return TRUE;
}

int  OpenNodeStruct  (lpVirtNode)
LPUCHAR lpVirtNode;
{
   int i;

   for (i=0;i<imaxnodes;i++) {
      if (virtnode[i].pdata) {
         if (!x_stricmp(virtnode[i].pdata->VirtNodeName,lpVirtNode)) {
            virtnode[i].nUsers++;
            return i;
         }
      }
   }

   for (i=0;i<imaxnodes;i++) {
      if (!virtnode[i].pdata) {
opn:
         virtnode[i].pdata = (struct ServConnectData *)
                 ESL_AllocMem( (UINT) sizeof(struct ServConnectData));
         if (!virtnode[i].pdata)
            return -1;
         memset(virtnode[i].pdata,0x00,sizeof(struct ServConnectData));
         fstrncpy(virtnode[i].pdata->VirtNodeName,
                  lpVirtNode, sizeof(virtnode[i].pdata->VirtNodeName));
         virtnode[i].nUsers++;
         virtnode[i].bIsSpecialMonState = FALSE;
         x_strcpy(virtnode[i].pdata->ICEUserName    ,"");
         x_strcpy(virtnode[i].pdata->ICEUserPassword,"");
         if (virtnode[i].nUsers !=1)
            myerror(ERR_OPEN_NODE);
         return i;
      }
   }

   for (i=0;i<imaxnodes;i++) { 
      if (!virtnode[i].nUsers) { 
         FreeNodeStruct(i,FALSE);
         goto opn;
      }
   }


   return -1;  /* no free entry */
}

BOOL CloseNodeStruct(hnodestruct, bkeep)
int hnodestruct;
BOOL bkeep;
{
   int newnUsers;

   if (hnodestruct<0 || hnodestruct >= imaxnodes) {
      myerror(ERR_UPDATEDOMDATA);
      return FALSE;
   }
   virtnode[hnodestruct].nUsers --;
   newnUsers = virtnode[hnodestruct].nUsers ;

   if (newnUsers <0) {
     myerror(ERR_CLOSE_NODE);
     return FALSE;
   }

   if (newnUsers >0 || bkeep)
      return TRUE;

  return FreeNodeStruct(hnodestruct,FALSE);
}

BOOL FreeNodeStructsForName(LPUCHAR NodeName,BOOL bAllGW, BOOL bAllUsers)
{
   int i;
   for (i=0;i<imaxnodes;i++) {
      if (virtnode[i].pdata) {
         UCHAR NodeName1[100];
         x_strcpy(NodeName1,virtnode[i].pdata->VirtNodeName);
         if (bAllGW)
            RemoveGWNameFromString(NodeName1);
         if (bAllUsers)
            RemoveConnectUserFromString(NodeName1);
         if (!x_stricmp(NodeName1,NodeName)) 
            FreeNodeStruct(i,FALSE);
      }
   }
   return TRUE;
 

}

BOOL FreeNodeStruct(hnodestruct,bAllGWNodes)
int hnodestruct;
BOOL bAllGWNodes;
{
   struct ServConnectData * pdata1;

   if (hnodestruct<0 || hnodestruct >= imaxnodes) {
      myerror(ERR_UPDATEDOMDATA);
      return FALSE;
   }

   pdata1 = virtnode[hnodestruct].pdata;

   if (!pdata1) {
      myerror(ERR_UPDATEDOMDATA);
      return FALSE;
   }
   
   if (bAllGWNodes) {
      int i;
      UCHAR NodeNameIni[100];
      x_strcpy(NodeNameIni,virtnode[hnodestruct].pdata->VirtNodeName);
      for (i=0;i<imaxnodes;i++) {
         if (virtnode[i].pdata) {
            UCHAR NodeName[100];
            x_strcpy(NodeName,virtnode[i].pdata->VirtNodeName);
            RemoveGWNameFromString(NodeName);
            if (!x_stricmp(NodeName,NodeNameIni))
               FreeNodeStruct(i,FALSE);
         }
      }
      return TRUE;
   }  

   if (virtnode[hnodestruct].nUsers) {
     myerror(ERR_UPDATEDOMDATA);
     return FALSE;

   }
 
   freeDBData          (pdata1->lpDBData          );
   freeProfilesData    (pdata1->lpProfilesData    );
   freeUsersData       (pdata1->lpUsersData       );
   freeGroupsData      (pdata1->lpGroupsData      );
   freeRolesData       (pdata1->lpRolesData       );
   freeLocationsData   (pdata1->lpLocationsData   );
   freeRawSecuritiesData(pdata1->lpRawDBSecurityData);
   freeRawDBGranteesData(pdata1->lpRawDBGranteeData);
   freeServerData       (pdata1->lpServerData       );
   freeLockListData     (pdata1->lpLockListData     );
   freeResourceData     (pdata1->lpResourceData     );
   freeLogProcessData   (pdata1->lpLogProcessData   );
   freeLogDBData        (pdata1->lpLogDBData        );
   freeLogTransactData  (pdata1->lpLogTransactData  );
   freeSimpleLeafObjectData  (pdata1->lpIceRolesData);
   freeSimpleLeafObjectData  (pdata1->lpIceDbUsersData);
   freeSimpleLeafObjectData  (pdata1->lpIceDbConnectionsData);
   freeIceObjectWithRolesAndDBData(pdata1->lpIceWebUsersData);
   freeIceObjectWithRolesAndDBData(pdata1->lpIceProfilesData);
   freeIceBusinessUnitsData(pdata1->lpIceBusinessUnitsData);
   freeSimpleLeafObjectData  (pdata1->lpIceApplicationsData);
   freeSimpleLeafObjectData  (pdata1->lpIceLocationsData);
   freeSimpleLeafObjectData  (pdata1->lpIceVarsData);
   ESL_FreeMem (pdata1);
   virtnode[hnodestruct].pdata = NULL;

   return TRUE;
} 

BOOL ClearCacheMonData(LPUCHAR lpNodeName)
{
   struct ServConnectData * pdata1;
   int hnodestruct=GetVirtNodeHandle(lpNodeName);
   if (hnodestruct<0)
      return FALSE;

   pdata1 = virtnode[hnodestruct].pdata;

   freeServerData       (pdata1->lpServerData       );
   freeLockListData     (pdata1->lpLockListData     );
   freeResourceData     (pdata1->lpResourceData     );
   freeLogProcessData   (pdata1->lpLogProcessData   );
   freeLogDBData        (pdata1->lpLogDBData        );
   freeLogTransactData  (pdata1->lpLogTransactData  );
   pdata1->lpServerData      = NULL;
   pdata1->lpLockListData    = NULL;
   pdata1->lpResourceData    = NULL;
   pdata1->lpLogProcessData  = NULL;
   pdata1->lpLogDBData       = NULL;
   pdata1->lpLogTransactData = NULL;
   
   return TRUE;

}


int GetVirtNodeHandle(lpNodeName)
LPUCHAR lpNodeName;
{
   int i;
   for (i=0;i<imaxnodes;i++) {
      if (virtnode[i].pdata) {
         if (!x_stricmp(lpNodeName,virtnode[i].pdata->VirtNodeName))
            return i;
      }
   }
   return (-1);
}

BOOL ReplaceLocalWithRealNodeName(LPUCHAR lpNodeName)
{
	char BaseHostName[MAXOBJECTNAME];
    char LocalNode[100];
	char buf[200];
	int lenlocalnode;

    x_strcpy(LocalNode,ResourceString((UINT)IDS_I_LOCALNODE));
	lenlocalnode = x_strlen(LocalNode);
	if (!lenlocalnode)
		return FALSE;

	// test if "(local..)..."
    if (x_strnicmp(lpNodeName,LocalNode,lenlocalnode)) 
	    return FALSE;
 
	// Get local vnode name
	GetLocalVnodeName (BaseHostName,MAXOBJECTNAME);
	if (!isLocalWithRealNodeName(BaseHostName))
		return FALSE;

	wsprintf(buf,"%s%s",BaseHostName,lpNodeName+lenlocalnode);
	x_strcpy(lpNodeName,buf);
	return TRUE;
}

BOOL isLocalWithRealNodeName(LPUCHAR lpNodeName)
{
	NODEDATAMIN nodedata;
	char BaseHostName[MAXOBJECTNAME];
	int  ires;
	BOOL bResult=TRUE;

	// Get local vnode name
	GetLocalVnodeName (BaseHostName,MAXOBJECTNAME);

	if (x_stricmp(BaseHostName,lpNodeName))
		return FALSE;
	
	push4mong();

	ires=GetFirstMonInfo(0,0,NULL,OT_NODE,(void * )&nodedata,NULL);
	while (ires==RES_SUCCESS) {
		if (!x_stricmp(nodedata.NodeName,BaseHostName))
			bResult=FALSE;
		ires=GetNextMonInfo((void * )&nodedata);
	}
	pop4mong();
	return bResult;
}


int NodeConnections(LPUCHAR lpNodeName)
{
   int i;
   int iresult= 0;
   char LocalNode[100];
   BOOL bIsLocal=FALSE;
   x_strcpy(LocalNode,ResourceString((UINT)IDS_I_LOCALNODE));
   if (!x_stricmp(lpNodeName,LocalNode))
	   bIsLocal=TRUE;
   for (i=0;i<imaxnodes;i++) {
      if (virtnode[i].pdata) {
         char buf[100];
         x_strcpy(buf,virtnode[i].pdata->VirtNodeName);
         RemoveGWNameFromString(buf);
         RemoveConnectUserFromString(buf);
         if (!x_stricmp(lpNodeName,buf))
            iresult++;
		 else {
			 if (bIsLocal && isLocalWithRealNodeName(buf))
				 iresult++;
		 }
      }
   }
   return iresult;
}

int NodeServerConnections(LPUCHAR lpNodeName)
{
   int i;
   int iresult= 0;
   for (i=0;i<imaxnodes;i++) {
      if (virtnode[i].pdata) {
         char buf[100];
         x_strcpy(buf,virtnode[i].pdata->VirtNodeName);
         RemoveConnectUserFromString(buf);
         if (!x_stricmp(lpNodeName,buf))
            iresult++;

      }
   }
   return iresult;
}

static void RemoveIngresGWSuffix(char * buffer)
{
	CHAR buf1[100];
	BOOL bHasGWSuffix = GetGWClassNameFromString(buffer, buf1);
	if (bHasGWSuffix) {
		if (!x_stricmp(buf1,"INGRES"))
			RemoveGWNameFromString(buffer);
	}
	return;
}

// iobjecttype used only for users, for special management of users in
// the Nodes window. Otherwise, the general case is to consider that the
// default server class may not be Ingres
static BOOL AreHandlesOnSameServer(int hdl1,int hdl2,int iobjecttype)
{
   char buf1[100];
   char buf2[100];


   if (!virtnode[hdl1].pdata || !virtnode[hdl2].pdata)
	return FALSE;
   
   x_strcpy(buf1,virtnode[hdl1].pdata->VirtNodeName);
   RemoveConnectUserFromString(buf1);
   if (iobjecttype== OT_USER)
      RemoveIngresGWSuffix(buf1);  

   x_strcpy(buf2,virtnode[hdl2].pdata->VirtNodeName);
   RemoveConnectUserFromString(buf2);
   if (iobjecttype== OT_USER)
	   RemoveIngresGWSuffix(buf2);  

   
   if (!x_stricmp(buf1,buf2))
	   return TRUE;
   return FALSE;
}
int  UpdateDOMDataAllUsers(int hnodestruct,     int iobjecttype,  int level,
                        LPUCHAR *parentstrings, BOOL bWithSystem, BOOL bWithChildren)
{
   int i,ires1,ires=RES_SUCCESS;
   for (i=0;i<imaxnodes;i++) {
      if (virtnode[i].pdata) {
		  if (AreHandlesOnSameServer(hnodestruct,i,iobjecttype)) {
			  ires1=UpdateDOMDataDetail(i,iobjecttype,level,parentstrings,bWithSystem,bWithChildren,TRUE,FALSE,FALSE);
			  if (ires1!=RES_SUCCESS)
				  ires=ires1;
		  }
      }
   }
   return ires;
}

int  UpdateDOMDataDetailAllUsers  (int hnodestruct,  int iobjecttype,   int level,          LPUCHAR *parentstrings,
								   BOOL bWithSystem, BOOL bWithChildren,BOOL bOnlyIfExist,  BOOL bUnique,
								   BOOL bWithDisplay)
{
   int i,ires1,ires=RES_SUCCESS;
   for (i=0;i<imaxnodes;i++) {
      if (virtnode[i].pdata) {
		  if (AreHandlesOnSameServer(hnodestruct,i,iobjecttype)) {
			  ires1=UpdateDOMDataDetail  (i,iobjecttype, level, parentstrings,
								   bWithSystem, bWithChildren,bOnlyIfExist, bUnique, bWithDisplay);
			  if (ires1!=RES_SUCCESS)
				  ires=ires1;
		  }
      }
   }
   return ires;
}

int  DOMUpdateDisplayDataAllUsers (int      hnodestruct,    // handle on node struct
								   int      iobjecttype,    // object type
								   int      level,          // parenthood level
								   LPUCHAR *parentstrings,  // parenthood names
								   BOOL     bWithChildren,  // should we expand children?
								   int      iAction,        // why is this function called ?
								   DWORD    idItem,         // if expansion: item being expanded
								   HWND     hwndDOMDoc)    // handle on DOM MDI document
{
   int i,ires1,ires=RES_SUCCESS;
   for (i=0;i<imaxnodes;i++) {
      if (virtnode[i].pdata) {
		  if (AreHandlesOnSameServer(hnodestruct,i,iobjecttype)) {
			  ires1=DOMUpdateDisplayData (i,iobjecttype, level, parentstrings, 
								           bWithChildren,  iAction,  idItem, hwndDOMDoc) ;
			  if (ires1!=RES_SUCCESS)
				  ires=ires1;
		  }
      }
   }
   return ires;
}

int  DOMUpdateDisplayAllDataAllUsers (int hnodestruct)    // handle on node struct
{
   int i,ires1,ires=RES_SUCCESS;
   for (i=0;i<imaxnodes;i++) {
      if (virtnode[i].pdata) {
          if (AreHandlesOnSameServer(hnodestruct,i,OT_VIRTNODE)) {
              ires1=DOMUpdateDisplayData (i,OT_VIRTNODE, 0, NULL, 
                                           FALSE,  ACT_BKTASK,  0L, 0) ;
              if (ires1!=RES_SUCCESS)
                  ires=ires1;
          }
      }
   }
   return ires;
}

char * GetVirtNodeName(hnodestruct)
int     hnodestruct;
{
   struct ServConnectData * pdata1;

   if (hnodestruct<0 || hnodestruct >= imaxnodes) 
      return (LPUCHAR)0;

   pdata1 = virtnode[hnodestruct].pdata;

   if (!pdata1) 
      return (LPUCHAR)0;

   return pdata1->VirtNodeName;
}


int GetMaxVirtNodes()
{
   return imaxnodes;
}

BOOL isVirtNodeEntryBusy(hnodestruct)
int     hnodestruct;
{
   if (virtnode[hnodestruct].pdata)
      return TRUE;
   else
      return FALSE;
}



int GetReplicInstallStatus(hnodestruct, lpDBName, lpDBOwner)
int     hnodestruct;
LPUCHAR lpDBName;
LPUCHAR lpDBOwner;
{
   LPUCHAR parentstrings[1];
   int     resulttype;
   int     resultlevel;
   LPUCHAR resultparentstrings[2];
   UCHAR buf1[MAXOBJECTNAME];
   UCHAR buf2[MAXOBJECTNAME];
   UCHAR resultobjectname[MAXOBJECTNAME];  
   UCHAR resultowner[MAXOBJECTNAME];      
   UCHAR resultextradata[MAXOBJECTNAME];
   UCHAR buftablename[MAXOBJECTNAME];
   int res;

   parentstrings[0]= lpDBName;
   resultparentstrings[0]=buf1;
   resultparentstrings[1]=buf2;
   res = DOMGetObjectLimitedInfo(hnodestruct,
                                 lpDBName,                         // object name,
                                 lpDBOwner,                        // object owner
                                 OT_DATABASE,                      // iobjecttype
                                 0,                                // level
                                 parentstrings,                    // parenthood
                                 TRUE,                             // bwithsystem
                                 &resulttype,
                                 resultobjectname,
                                 resultowner,
                                 resultextradata
                                 );
   if ( res != RES_SUCCESS){
      char szMessTemp[MAXOBJECTNAME];
      //"Cannot get replicator status on database %s"
      wsprintf (szMessTemp,ResourceString(IDS_F_REPLICATOR_STATUS),lpDBName);
      TS_MessageBox(TS_GetFocus(), szMessTemp , NULL, MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL);
      return REPLIC_NOINSTALL;
   }
   if(DOMGetObject(hnodestruct,
                   StringWithOwner("dd_mobile_opt",lpDBOwner,buftablename),
                   "",
                   OT_TABLE,1,parentstrings,
              TRUE,&resulttype, &resultlevel, resultparentstrings,
              resultobjectname, resultowner, resultextradata)
      ==RES_SUCCESS)
      return REPLIC_V105;

   if(DOMGetObject(hnodestruct,
                   StringWithOwner("DD_MOBILE_OPT",lpDBOwner,buftablename),
                   "",
                   OT_TABLE,1,parentstrings,
              TRUE,&resulttype, &resultlevel, resultparentstrings,
              resultobjectname, resultowner, resultextradata)
      ==RES_SUCCESS)
      return REPLIC_V105;

   if(DOMGetObject(hnodestruct,
                   StringWithOwner("dd_all_tables",lpDBOwner,buftablename),
                   "",
                   OT_VIEW,1,parentstrings,
              TRUE,&resulttype, &resultlevel, resultparentstrings,
              resultobjectname, resultowner, resultextradata)
      ==RES_SUCCESS)
      return REPLIC_V10;
   if(DOMGetObject(hnodestruct,
                   StringWithOwner("DD_ALL_TABLES",lpDBOwner,buftablename),
                   "",
                   OT_VIEW,1,parentstrings,
              TRUE,&resulttype, &resultlevel, resultparentstrings,
              resultobjectname, resultowner, resultextradata)
      ==RES_SUCCESS)
      return REPLIC_V10;



   if(DOMGetObject(hnodestruct,
                   StringWithOwner("dd_cdds",lpDBOwner,buftablename),
                   "",
                   OT_TABLE,1,parentstrings,
              TRUE,&resulttype, &resultlevel, resultparentstrings,
              resultobjectname, resultowner, resultextradata)
      ==RES_SUCCESS)
      return REPLIC_V11;
   if(DOMGetObject(hnodestruct,
                   StringWithOwner("DD_CDDS",lpDBOwner,buftablename),
                   "",
                   OT_TABLE,1,parentstrings,
              TRUE,&resulttype, &resultlevel, resultparentstrings,
              resultobjectname, resultowner, resultextradata)
      ==RES_SUCCESS)
      return REPLIC_V11;

   return REPLIC_NOINSTALL;
}

// Star management
int  GetDatabaseStarType(int hnodestruct,LPUCHAR lpDBName)
{
  int irestype,ires;
  CHAR buf     [MAXOBJECTNAME];
  CHAR bufowner[MAXOBJECTNAME];
  CHAR extradata[EXTRADATASIZE];
  LPUCHAR parents[MAXPLEVEL];
  parents[0]=NULL;

  ires= DOMGetObjectLimitedInfo(hnodestruct,
								lpDBName,
                                NULL,
                                OT_DATABASE,
                                0,
                                parents,
                                TRUE,
                                &irestype,
                                buf,
                                bufowner,
                                extradata);
   if (ires==RES_SUCCESS)
      return getint(extradata+STEPSMALLOBJ);
   return -1;
}

BOOL IsStarDatabase(int hnodestruct,LPUCHAR lpDBName)
{
  if (!x_stricmp(lpDBName,"imadb") || !x_stricmp(lpDBName,"iidbdb"))
    return FALSE;
  if (hnodestruct<0)
    return FALSE;
  if (GetDatabaseStarType(hnodestruct, lpDBName) == DBTYPE_DISTRIBUTED)
    return TRUE;
  else
    return FALSE;
}

#include "ice.h"
extern BOOL MfcDlgIcePassword( LPICEPASSWORDDATA lpPassworddta ,LPUCHAR lpVnode);

int GetICeUserAndPassWord(int hnodestruct, LPUCHAR bufuser, LPUCHAR bufpasswd)
{
   struct ServConnectData * pdata1;

   if (hnodestruct<0 || hnodestruct >= imaxnodes) 
      return RES_ERR;

   pdata1 = virtnode[hnodestruct].pdata;

   if (!pdata1) 
      return RES_ERR;
   if (!x_stricmp(pdata1->ICEUserName,"") || !x_stricmp(pdata1->ICEUserPassword,"")) {
	   ICEPASSWORDDATA PwdData;
	   if (! MfcDlgIcePassword( &PwdData, pdata1->VirtNodeName))
	  	  return RES_ERR;
       SetICeUserAndPassWord(hnodestruct, PwdData.tchUserName,  PwdData.tchPassword);
   }
	x_strcpy(bufuser, pdata1->ICEUserName);
	x_strcpy(bufpasswd,pdata1->ICEUserPassword);
	return RES_SUCCESS;

}

int SetICeUserAndPassWord(int hnodestruct, LPUCHAR bufuser, LPUCHAR bufpasswd)
{
   struct ServConnectData * pdata1;

   if (hnodestruct<0 || hnodestruct >= imaxnodes) 
      return RES_ERR;

   pdata1 = virtnode[hnodestruct].pdata;

   if (!pdata1) 
      return RES_ERR;
   x_strcpy( pdata1->ICEUserName,bufuser);
   x_strcpy(pdata1->ICEUserPassword,bufpasswd);
   return RES_SUCCESS;

}


