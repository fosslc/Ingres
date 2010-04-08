/********************************************************************
**
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
**
**  Project : libmon library for the Ingres Monitor ActiveX control
**
**    Source : domdmisc.c
**    misc cache functions
**
** History
**
**	22-Nov-2001 (noifr01)
**      extracted and adapted from the winmain\domdmisc.c source
**  26-Mar-2001 (noifr01)
**      completed the DBAAppend4Save and DBAReadData functions
**  24-Mar-2004 (shaha03)
**		(sir 112040) Added the Multi-thread ability to the LibMon directory.
**      (sir 112040) Added LBMNMUTEX Mutexes for making the library thread safe
**      (sir 112040) Added code to initialize the global variables in initNodeStruct()
**			Added code to clean the mutexes in endNodeStruct()
**		(sir 112040) Modifed the Hard coded "int imaxnodes = 10" to "int imaxnodes = MAX_NODES"
**
********************************************************************/

#include "libmon.h"
#include "domdloca.h"
#include "error.h"
#include "lbmnmtx.h"
struct def_virtnode  virtnode[MAXNODES];

LBMNMUTEX lbmn_mtx_nodestruct; /*Mutex for 
									1. LIBMON_OpenNodeStruct()
									2. LIBMON_CloseNodeStruct() */
LBMNMUTEX lbmn_mtx_session; /*Mutex for 
									1. GetSession()
									2. ReleaseSession() */
LBMNMUTEX lbmn_mtx_actsession[MAXNODES]; /*Mutex for
											1. ActivateSession()*/

LBMNMUTEX lbmn_mtx_ndsummary[MAXNODES]; /*hnode level Mutex for 
											1. MonGetLockSummary()
											2. MonGetLogSummary()
											3. MonGetDetailInfo()*/
LBMNMUTEX lbmn_mtx_Info[MAXNODES];  /* 
									    vnode level mutex for 
										1. LIBMON_GetFirstMonInfo()
										2. LIBMON_GetNextMonInfo()
										3. LIBMON_ReleaseMonInfoHdl()
										4. UpdateMonInfo()
										5. LIBMON_NeedBkRefresh()
										6. GetTransactionString()*/

mon4struct	monInfo[MAX_THREADS];  /*Structure to maintain the handle and push4mong(), pop4mong() functions*/

LBMNMUTEX   lclhst_name_mtx ; /* Mutex for 
								1. LIBMON_getLocalHostname()*/



typedef struct _fileident
{
	void ** ppbufsave;
	int *   pbufsavelen;

	void * pbufload;
	int iloadbuflen;
	int icurpos;

	
} _fident, FAR * FILEIDENT;

int imaxnodes = MAXNODES;

BOOL initNodeStruct()
{
	int i;

	/*Initialize mutexes with null values*/
	/*Creation of Mutexes is differed till the first usage of Muttex */
	memset(&lbmn_mtx_nodestruct,0,sizeof(lbmn_mtx_nodestruct));
	memset(&lbmn_mtx_session,0,sizeof(lbmn_mtx_session));
	memset(&lclhst_name_mtx,0,sizeof(lclhst_name_mtx));
	
	for(i=0; i < MAXNODES; i++)
	{
		memset(&lbmn_mtx_ndsummary[i],0,sizeof(LBMNMUTEX));	
		memset(&lbmn_mtx_Info[i],0,sizeof(LBMNMUTEX));
		memset(&lbmn_mtx_actsession[i],0,sizeof(LBMNMUTEX));
	}

	for(i=0; i < MAX_THREADS; i++)
		memset(&monInfo[i], 0, sizeof(mon4struct));

	DBAginfoInit();
    return TRUE;   /* statics are initialized to 0;  */
}

BOOL EndNodeStruct()
{
   int i;

   for (i=0;i<imaxnodes;i++) { /*Returns false if any of the usage count is greater than 0, and doesn't clear the mutex*/
	   if(virtnode[i].nUsers)
		   return FALSE;
   }
   for (i=0;i<imaxnodes;i++) {
      if (virtnode[i].pdata) 
         FreeNodeStruct(i,FALSE);
   }
   DBAginfoFree();
   freeRevInfoData();
   if (getMonStack())
      myerror(ERR_LEVELNUMBER); 

   /*Clearing the created mutexes. Will ignore if the mutexes are not created*/
   for(i=0; i<MAXNODES; i++)
   {
	   CleanLBMNMutex(&lbmn_mtx_ndsummary[i]);
	   CleanLBMNMutex(&lbmn_mtx_Info[i]);
	   CleanLBMNMutex(&lbmn_mtx_actsession[i]);
   }
   CleanLBMNMutex(&lclhst_name_mtx);
   CleanLBMNMutex(&lbmn_mtx_nodestruct);
   CleanLBMNMutex(&lbmn_mtx_session);

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

int  LIBMON_OpenNodeStruct  (lpVirtNode)
LPUCHAR lpVirtNode;
{
   int i;

   if(!CreateLBMNMutex(&lbmn_mtx_nodestruct))/*Creating a Mutex: Will ignore if the mutex is already created */
		return RES_ERR;
   if(!OwnLBMNMutex(&lbmn_mtx_nodestruct, INFINITE)) /*Owning the Mutex*/
	return RES_ERR;

   for (i=0;i<imaxnodes;i++) {
      if (virtnode[i].pdata) {
         if (!x_stricmp(virtnode[i].pdata->VirtNodeName,lpVirtNode)) {
            virtnode[i].nUsers++;
			if(!UnownLBMNMutex(&lbmn_mtx_nodestruct)) /*Releasing the mutex before return*/
				return -1;
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
		 {
			(void)UnownLBMNMutex(&lbmn_mtx_nodestruct); /*Releasing the mutex before return*/
            return -1;
		 }
         memset(virtnode[i].pdata,0x00,sizeof(struct ServConnectData));
         fstrncpy(virtnode[i].pdata->VirtNodeName,
                  lpVirtNode, sizeof(virtnode[i].pdata->VirtNodeName));
         virtnode[i].nUsers++;
         virtnode[i].bIsSpecialMonState = FALSE;
         x_strcpy(virtnode[i].pdata->ICEUserName    ,"");
         x_strcpy(virtnode[i].pdata->ICEUserPassword,"");
         if (virtnode[i].nUsers !=1)
            myerror(ERR_OPEN_NODE);
		 if(!UnownLBMNMutex(&lbmn_mtx_nodestruct)) /*Releasing the mutex before return*/
			 return -1;
         return i;
      }
   }

   for (i=0;i<imaxnodes;i++) { 
      if (!virtnode[i].nUsers) { 
         FreeNodeStruct(i,FALSE);
         goto opn;
      }
   }

   (void)UnownLBMNMutex(&lbmn_mtx_nodestruct); /*Releasing the mutex before return*/
   return -1;  /* no free entry */
}

BOOL LIBMON_CloseNodeStruct(int hnodestruct)
{
   int newnUsers;
   int tempReturnVal;

   if(!CreateLBMNMutex(&lbmn_mtx_nodestruct)) /*Creating a Mutex: Will ignore if the mutex is already created */
	   return FALSE;
   if(!OwnLBMNMutex(&lbmn_mtx_nodestruct, INFINITE)) /*Owning the Mutex to be thread safe*/
		return FALSE;

   if (hnodestruct<0 || hnodestruct >= imaxnodes) {
      myerror(ERR_UPDATEDOMDATA);
	  (void)UnownLBMNMutex(&lbmn_mtx_nodestruct); /*Releasing the mutex before return*/
      return FALSE;
   }
   virtnode[hnodestruct].nUsers --;
   newnUsers = virtnode[hnodestruct].nUsers ;

   if (newnUsers <0) {
     myerror(ERR_CLOSE_NODE);
	 (void)UnownLBMNMutex(&lbmn_mtx_nodestruct); /*Releasing the mutex before return*/
     return FALSE;
   }

   if (newnUsers >0 )
   {
	  if(!UnownLBMNMutex(&lbmn_mtx_nodestruct)) /*Releasing the mutex before return*/
		  return FALSE;
      return TRUE;
   }

   tempReturnVal = FreeNodeStruct(hnodestruct,FALSE); /*Store the return in temp val to Unown the mutex before returning*/
   if(!UnownLBMNMutex(&lbmn_mtx_nodestruct)) /*Releasing the mutex before return*/
	   return FALSE;
   return tempReturnVal;
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

    x_strcpy(LocalNode,LIBMON_getLocalNodeString());
	lenlocalnode = x_strlen(LocalNode);
	if (!lenlocalnode)
		return FALSE;

	// test if "(local..)..."
    if (x_strnicmp(lpNodeName,LocalNode,lenlocalnode)) 
	    return FALSE;
 
	// Get local vnode name
	fstrncpy(BaseHostName,LIBMON_getLocalHostName(),MAXOBJECTNAME);
	if (!isLocalWithRealNodeName(BaseHostName))
		return FALSE;

	wsprintf(buf,"%s%s",BaseHostName,lpNodeName+lenlocalnode);
	x_strcpy(lpNodeName,buf);
	return TRUE;
}

BOOL isLocalWithRealNodeName(LPUCHAR lpNodeName)
{
	char BaseHostName[MAXOBJECTNAME];
	BOOL bResult=TRUE;

	// Get local vnode name
	fstrncpy(BaseHostName,LIBMON_getLocalHostName(),MAXOBJECTNAME);
	
	if (x_stricmp(BaseHostName,lpNodeName))
		return FALSE;

	// no more test whether there is a node with the same name as the localhost,
	// because this one is used anyhow. Normally, there has already been a warning
	// when expanding the nodes list if there is such a node. If is is the 
	// "installation password" node, there should be no problem
	
	return bResult;
}


int NodeConnections(LPUCHAR lpNodeName)
{
   int i;
   int iresult= 0;
   char LocalNode[100];
   BOOL bIsLocal=FALSE;
   x_strcpy(LocalNode,LIBMON_getLocalNodeString());
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

#ifndef BLD_MULTI /*To make available in Non multi-thread context only */
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

   if(!CreateLBMNMutex(&lbmn_mtx_Info[hnodestruct])) /*Creating a Mutex: Will ignore if the mutex is already created */
	   return RES_ERR;
   if(!OwnLBMNMutex(&lbmn_mtx_Info[hnodestruct],INFINITE)) /*Owning the Mutex */
		return RES_ERR;

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

   if ( res != RES_SUCCESS)
   {
	  (void)UnownLBMNMutex(&lbmn_mtx_Info[hnodestruct]); /*Releasing the Mutex before return */
      return REPLIC_VER_ERR;
   }
   
   if(DOMGetObject(hnodestruct,
                   StringWithOwner("dd_mobile_opt",lpDBOwner,buftablename),
                   "",
                   OT_TABLE,1,parentstrings,
              TRUE,&resulttype, &resultlevel, resultparentstrings,
              resultobjectname, resultowner, resultextradata)
      ==RES_SUCCESS)
   {
	  if(!UnownLBMNMutex(&lbmn_mtx_Info[hnodestruct])) /*Releasing the Mutex before return */
		  return RES_ERR;
      return REPLIC_V105;
   }

   if(DOMGetObject(hnodestruct,
                   StringWithOwner("DD_MOBILE_OPT",lpDBOwner,buftablename),
                   "",
                   OT_TABLE,1,parentstrings,
              TRUE,&resulttype, &resultlevel, resultparentstrings,
              resultobjectname, resultowner, resultextradata)
      ==RES_SUCCESS)
   {
      if(!UnownLBMNMutex(&lbmn_mtx_Info[hnodestruct])) /*Releasing the Mutex before return */
		  return RES_ERR;
	  return REPLIC_V105;
   }

   if(DOMGetObject(hnodestruct,
                   StringWithOwner("dd_all_tables",lpDBOwner,buftablename),
                   "",
                   OT_VIEW,1,parentstrings,
              TRUE,&resulttype, &resultlevel, resultparentstrings,
              resultobjectname, resultowner, resultextradata)
      ==RES_SUCCESS)
   {
	  if(!UnownLBMNMutex(&lbmn_mtx_Info[hnodestruct])) /*Releasing the Mutex before return */
		  return RES_ERR;
      return REPLIC_V10;
   }
   if(DOMGetObject(hnodestruct,
                   StringWithOwner("DD_ALL_TABLES",lpDBOwner,buftablename),
                   "",
                   OT_VIEW,1,parentstrings,
              TRUE,&resulttype, &resultlevel, resultparentstrings,
              resultobjectname, resultowner, resultextradata)
      ==RES_SUCCESS)
   {
	  if(!UnownLBMNMutex(&lbmn_mtx_Info[hnodestruct])) /*Releasing the Mutex before return */
		  return RES_ERR;
      return REPLIC_V10;
   }



   if(DOMGetObject(hnodestruct,
                   StringWithOwner("dd_cdds",lpDBOwner,buftablename),
                   "",
                   OT_TABLE,1,parentstrings,
              TRUE,&resulttype, &resultlevel, resultparentstrings,
              resultobjectname, resultowner, resultextradata)
      ==RES_SUCCESS)
   {
	  if(!UnownLBMNMutex(&lbmn_mtx_Info[hnodestruct])) /*Releasing the Mutex before return */
		  return RES_ERR;
      return REPLIC_V11;
   }
   if(DOMGetObject(hnodestruct,
                   StringWithOwner("DD_CDDS",lpDBOwner,buftablename),
                   "",
                   OT_TABLE,1,parentstrings,
              TRUE,&resulttype, &resultlevel, resultparentstrings,
              resultobjectname, resultowner, resultextradata)
      ==RES_SUCCESS)
   {
	  if(!UnownLBMNMutex(&lbmn_mtx_Info[hnodestruct])) /*Releasing the Mutex before return */
		  return RES_ERR;
      return REPLIC_V11;
   }

   (void)UnownLBMNMutex(&lbmn_mtx_Info[hnodestruct]); /*Releasing the Mutex before return */
   return REPLIC_NOINSTALL;
}
#endif

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




int DBAAppend4Save(FILEIDENT fident, void *buf, UINT cb)
{
	void * pbuf;
	if (*(fident->pbufsavelen) == 0) {
		pbuf = malloc(cb);
		if (!pbuf)
			return RES_ERR;
		*(fident->ppbufsave) = pbuf;
		memcpy(pbuf,buf,cb);
		* (fident->pbufsavelen) = cb;
		return RES_SUCCESS;
	}
	pbuf = ESL_ReAllocMem(*(fident->ppbufsave), *(fident->pbufsavelen)+cb, *(fident->pbufsavelen));
	if (!pbuf)
		return RES_ERR;
	*(fident->ppbufsave) = pbuf;
	memcpy((BYTE *)pbuf +*(fident->pbufsavelen),buf,cb);
	*(fident->pbufsavelen) += cb;
	return RES_SUCCESS;
}

int DBAReadData     (FILEIDENT fident, void *bufresu, UINT cb)
{
	if (fident->icurpos + (int)cb > fident->iloadbuflen)
		return RES_ERR;
	memcpy((char *) bufresu, (char *) fident->pbufload + fident->icurpos, cb);
	fident->icurpos += cb;
	return RES_SUCCESS;
}


int  DOMSaveList ( fident, hnodestruct, iobjecttype, pstartoflist)
FILEIDENT fident;
int hnodestruct;
int iobjecttype;
void * pstartoflist;
{
   int   i, ires;
   BOOL  btemp;
   void * ptemp;
   struct ServConnectData * pdata1 = virtnode[hnodestruct].pdata;
   char buftmp[MAXOBJECTNAME];

   ires = RES_SUCCESS;

   switch (iobjecttype) {

      case OT_VIRTNODE  :
         {
			int users;
            int itype[]={OT_DATABASE,
                         OT_USER
                        };
         
            btemp  = pdata1? TRUE:FALSE;

            ires = DBAAppend4Save(fident, &btemp, sizeof(btemp));
            if (!btemp || ires!=RES_SUCCESS)
               return ires;

            users=virtnode[hnodestruct].nUsers-DBECacheEntriesUsedByReplMon(hnodestruct);

            ires = DBAAppend4Save(fident, &users,sizeof(users));
            if (!btemp || ires!=RES_SUCCESS)
               return ires;
            
            
			x_strcpy(buftmp,pdata1->ICEUserPassword); // will be restored after the save operation
			x_strcpy(pdata1->ICEUserPassword,""); // don't save the password (will be re-prompted after loading

            ires = DBAAppend4Save(fident, pdata1, sizeof(struct ServConnectData));
			x_strcpy(pdata1->ICEUserPassword,buftmp); // restore the password for current VDBA session
            if (ires!=RES_SUCCESS)
               return ires;
            for (i=0;i<sizeof(itype)/sizeof(int);i++) {
               switch (itype[i]){
                  case OT_DATABASE:
                     ptemp = (void *) pdata1->lpDBData          ; break;
                  case OT_USER:
                     ptemp = (void *) pdata1->lpUsersData       ; break;
                    default:
                     return RES_ERR;
               }
               ires= DOMSaveList(fident, hnodestruct, itype[i], ptemp);
               if (ires!=RES_SUCCESS)
                  return ires;
            }
         }
         break;
      case OT_DATABASE  :
         {
            struct DBData * pDBData = (struct DBData * )pstartoflist;
            int itype[]={OT_TABLE,
                         OT_DBEVENT
						};

            while (pDBData) {
			   struct DBData dta = *pDBData;
			   dta.lpMonReplServerData=(struct MonReplServerData  *)0;
               ires = DBAAppend4Save(fident, &dta, sizeof(struct DBData));
               if (ires!=RES_SUCCESS)
                  return ires;
               for (i=0;i<sizeof(itype)/sizeof(int);i++) {
                   switch (itype[i]){
                      case OT_TABLE:
                         ptemp = (void *) pDBData -> lpTableData      ; break;
                      case OT_DBEVENT:
                         ptemp = (void *) pDBData -> lpDBEventData    ; break;
                      default:
                         return RES_ERR;
                  }
                  ires= DOMSaveList(fident, hnodestruct, itype[i], ptemp);
                  if (ires!=RES_SUCCESS)
                     return ires;
               }
               pDBData=pDBData->pnext;
            }
         }
         break;
      case OT_USER      :
         {
            struct UserData * puserdata = (struct UserData * )pstartoflist;

            while (puserdata) {
               ires = DBAAppend4Save(fident, puserdata,
                                             sizeof(struct UserData));
               if (ires!=RES_SUCCESS)
                  return ires;
               puserdata=puserdata->pnext;
            }
         }
         break;
      case OT_TABLE     :
         {
            struct TableData * pTableData = (struct TableData * )pstartoflist;
            int itype[]={OT_COLUMN
                         };

            while (pTableData) {
               ires = DBAAppend4Save(fident, pTableData, sizeof(struct TableData));
               if (ires!=RES_SUCCESS)
                  return ires;
               for (i=0;i<sizeof(itype)/sizeof(int);i++) {
                   switch (itype[i]){
                      case OT_COLUMN:
                         ptemp = (void *) pTableData->lpColumnData   ;break;
                      default:
                         return RES_ERR;
                  }
                  ires= DOMSaveList(fident, hnodestruct, itype[i], ptemp);
                  if (ires!=RES_SUCCESS)
                     return ires;
               }
               pTableData=pTableData->pnext;
            }
         }
         break;
      case OT_COLUMN :
         {
            struct ColumnData * pColumndata = (struct ColumnData * )pstartoflist;

            while (pColumndata) {
               ires = DBAAppend4Save(fident, pColumndata,
                                             sizeof(struct ColumnData));
               if (ires!=RES_SUCCESS)
                  return ires;
               pColumndata=pColumndata->pnext;
            }
         }
         break;
      case OT_DBEVENT                    :
         {
            struct DBEventData * pDBEventdata =
                                       (struct DBEventData * )pstartoflist;

            while (pDBEventdata) {
               ires = DBAAppend4Save(fident, pDBEventdata,
                                             sizeof(struct DBEventData));
               if (ires!=RES_SUCCESS)
                  return ires;
               pDBEventdata=pDBEventdata->pnext;
            }
         }
         break;
      default:
         return myerror(ERR_OBJECTNOEXISTS);
         break;
   }
   return ires;
}

int  DOMReadList ( fident, hnodestruct, iobjecttype, lplpstartoflist)
FILEIDENT fident;
int hnodestruct;
int iobjecttype;
void ** lplpstartoflist;
{
   int   i, ires, ires1;
   BOOL  btemp;
   void ** pptemp;
   struct ServConnectData * pdata1 = virtnode[hnodestruct].pdata;

   ires = RES_SUCCESS;

   switch (iobjecttype) {

      case OT_VIRTNODE  :
         {
            int itype[]={OT_DATABASE,
                         OT_USER
                        };
            virtnode[hnodestruct].pdata = (struct ServConnectData *)0;
            ires = DBAReadData(fident, &btemp, sizeof(btemp));
            if (ires!=RES_SUCCESS)
               return ires;
            if (!btemp) 
               return RES_SUCCESS;

            ires = DBAReadData(fident, &virtnode[hnodestruct].nUsers,
                                  sizeof(virtnode[hnodestruct].nUsers));
            if (ires!=RES_SUCCESS)
               return ires;

            pdata1 = (struct ServConnectData *)
                       ESL_AllocMem( (UINT) sizeof(struct ServConnectData));
            if (!pdata1)
               return RES_ERR;

            virtnode[hnodestruct].bIsSpecialMonState=TRUE; // special state
            virtnode[hnodestruct].pdata = pdata1;

            ires = DBAReadData(fident,pdata1,sizeof(struct ServConnectData));
            if (ires!=RES_SUCCESS)
               return ires;

            pdata1->lpServerData      = NULL;   // cache not saved for monitor info
            pdata1->lpLockListData    = NULL;   // (too dynamic data)
            pdata1->lpResourceData    = NULL;
            pdata1->lpLogProcessData  = NULL;
            pdata1->lpLogDBData       = NULL;
            pdata1->lpLogTransactData = NULL;

            for (i=0;i<sizeof(itype)/sizeof(int);i++) {
               switch (itype[i]){
                  case OT_DATABASE:
                     pptemp = (void **) &(pdata1->lpDBData)          ; break;
                  case OT_USER:
                     pptemp = (void **) &(pdata1->lpUsersData)       ; break;
                    default:
                     return RES_ERR;
               }
               ires1= DOMReadList(fident, hnodestruct, itype[i], pptemp);
               if (ires1!=RES_SUCCESS)
                  ires=ires1;
            }
            if (ires!=RES_SUCCESS) {
               virtnode[hnodestruct].pdata = (struct ServConnectData *)0;
               // TO BE FINISHED: free linked lists
            }
         }
         break;
      case OT_DATABASE  :
         {
            struct DBData ** ppDBData = (struct DBData ** )lplpstartoflist;
            int itype[]={OT_TABLE,
                         OT_DBEVENT};


            while (*ppDBData) {
               *ppDBData = ESL_AllocMem(sizeof(struct DBData));
               if (!*ppDBData)
                  return RES_ERR;
               ires = DBAReadData(fident, *ppDBData,
                                          sizeof(struct DBData));
               if (ires!=RES_SUCCESS)
                  return ires;
			   (*ppDBData)->lpMonReplServerData=NULL;
			   (*ppDBData)->nbmonreplservers=0;
			   for (i=0;i<sizeof(itype)/sizeof(int);i++) {
                   switch (itype[i]){
                      case OT_TABLE:
                         pptemp = (void **)&((*ppDBData)->lpTableData      );break;
                      case OT_DBEVENT:
                         pptemp = (void **)&((*ppDBData)->lpDBEventData    );break;
                      default:
                         return RES_ERR;
                  }
                  ires1= DOMReadList(fident, hnodestruct, itype[i], pptemp);
                  if (ires1!=RES_SUCCESS)
                     ires=ires1;
               }
               ppDBData=&((*ppDBData)->pnext);
            }
         }
         break;
      case OT_USER      :
         {
            struct UserData ** ppuserdata =
                                      (struct UserData ** )lplpstartoflist;

            while (*ppuserdata) {
               *ppuserdata = ESL_AllocMem(sizeof(struct UserData));
               if (!*ppuserdata)
                  return RES_ERR;
               ires = DBAReadData(fident, *ppuserdata,
                                             sizeof(struct UserData));
               if (ires!=RES_SUCCESS)
                  return ires;
               ppuserdata=&((*ppuserdata)->pnext);
            }
         }
         break;
      case OT_TABLE     :
         {
            struct TableData ** ppTabledata =
                                      (struct TableData ** )lplpstartoflist;
            int itype[]={OT_COLUMN};

            while (*ppTabledata) {
               *ppTabledata = ESL_AllocMem(sizeof(struct TableData));
               if (!*ppTabledata)
                  return RES_ERR;
               ires = DBAReadData(fident, *ppTabledata,
                                             sizeof(struct TableData));
               if (ires!=RES_SUCCESS)
                  return ires;
               for (i=0;i<sizeof(itype)/sizeof(int);i++) {
                   switch (itype[i]){
                      case OT_COLUMN:
                         pptemp = (void **)&((*ppTabledata)->lpColumnData)   ;break;
                      default:
                         return RES_ERR;
                  }
                  ires1= DOMReadList(fident, hnodestruct, itype[i], pptemp);
                  if (ires1!=RES_SUCCESS)
                     ires=ires1;
               }
               ppTabledata=&((*ppTabledata)->pnext);
            }
         }
         break;
      case OT_COLUMN :
      case OT_DBEVENT                    :
         {
            struct DBEventData ** ppDBEventdata =
                                   (struct DBEventData ** )lplpstartoflist;

            while (*ppDBEventdata) {
               *ppDBEventdata = ESL_AllocMem(sizeof(struct DBEventData));
               if (!*ppDBEventdata)
                  return RES_ERR;
               ires = DBAReadData(fident, *ppDBEventdata,
                                             sizeof(struct DBEventData));
               if (ires!=RES_SUCCESS)
                  return ires;
               ppDBEventdata=&((*ppDBEventdata)->pnext);
            }
         }
         break;
      default:
         return myerror(ERR_OBJECTNOEXISTS);
         break;
   }
   return ires;
}


int LIBMON_SaveCache(void ** ppbuf, int * plen)
{
   int i,ires;
   _fident fident1;
   FILEIDENT fident = &fident1;

   fident->ppbufsave = ppbuf;
   fident->pbufsavelen = plen;
   *plen = 0;

   ires = DBAAppend4Save(fident, &imaxnodes, sizeof(imaxnodes));
   if (ires!=RES_SUCCESS)
      return ires;

   ires = DBAAppend4Save(fident, &DOMUpdver, sizeof(DOMUpdver));
   if (ires!=RES_SUCCESS)
      return ires;

   for (i=0;i<imaxnodes;i++)  {
      ires = DOMSaveList(fident,i, OT_VIRTNODE, (void *) NULL);   
      if (ires!=RES_SUCCESS)
         return ires;
   }

   return RES_SUCCESS;
}

void LIBMON_FreeSaveCacheBuffer(void * pbuf)
{
   ESL_FreeMem (pbuf);
}

int LIBMON_LoadCache(void *pbuf, int ilen)
{
   int i,ires,itemp;

   _fident fident1;
   FILEIDENT fident = &fident1;

   fident->pbufload = pbuf;
   fident->iloadbuflen = ilen;
   fident->icurpos = 0;

   ires = DBAReadData(fident, &itemp, sizeof(itemp));
   if (ires!=RES_SUCCESS)
      return ires;

   ires = DBAReadData(fident, &DOMUpdver, sizeof(DOMUpdver));
   if (ires!=RES_SUCCESS)
      return ires;

   imaxnodes=itemp;

   for (i=0;i<imaxnodes;i++)  {
      ires = DOMReadList(fident,i, OT_VIRTNODE, (void **) NULL);   
      if (ires!=RES_SUCCESS)
         return ires;
   }
   return RES_SUCCESS;
}


