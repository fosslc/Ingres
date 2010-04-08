/********************************************************************
//
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
//
//    Project : CA/OpenIngres Visual DBA
//
//    Source : domdata.h
//    public functions for cache
**
**   26-Mar-2001 (noifr01)
**   (sir 104270) removal of code for managing Ingres/Desktop
**   23-Nov-2001 (noifr01)
**   added #if[n]def's for the libmon project
**   01-Oct-2003 (schph01)
**   (bug 111019) Add prototype for DOMUpdateDisplayAllDataAllUsers()
**   function.
**   24-Mar-2004 (shaha03)
**   (sir 112040) added LIBMON_DOMGetFirstObject() to suuport the multi-threaded context with the same purpose of DOMGetFirstObject()
**   (sir 112040) added LIBMON_DOMGetNextObject() to suuport the multi-threaded context with the same purpose of DOMGetNextObject()
**   (sir 112040) added LIBMON_UpdateDOMData() to suuport the multi-threaded context with the same purpose of UpdateDOMData()
**   (sir 112040) added "int thdl" parameter to the functions push4domg() & pop4domg() to make them thread eanbled
**   16-Apr-2004 (noifr01)
**   (sir 112040) #ifdef'ed modified prototypes of push4domg() and pop4domg()
**   so that winmain project still builds (using old version of these functions)
********************************************************************/

BOOL initNodeStruct(void);
BOOL EndNodeStruct(void);

#ifndef LIBMON_HEADER
int  OpenNodeStruct  (LPUCHAR lpVirtNode); 
BOOL CloseNodeStruct (int hnodestruct, BOOL bkeep);
#endif

BOOL FreeNodeStructsForName(LPUCHAR NodeName,BOOL bAllGW, BOOL bAllUsers);
BOOL FreeNodeStruct( int hnodestruct,BOOL bAllGWNodes);

int  DOMGetFirstObject(int hnodestruct,
                        int iobjecttype,
                        int level,
                        LPUCHAR *parentstrings,
                        BOOL bwithsystem,
                        LPUCHAR lpfilterowner,
                        LPUCHAR lpresultobjectname,
                        LPUCHAR lpresultowner,      
                        LPUCHAR lpresultextrastring);

int  LIBMON_DOMGetFirstObject(int hnodestruct,
                        int iobjecttype,
                        int level,
                        LPUCHAR *parentstrings,
                        BOOL bwithsystem,
                        LPUCHAR lpfilterowner,
                        LPUCHAR lpresultobjectname,
                        LPUCHAR lpresultowner,      
                        LPUCHAR lpresultextrastring,
						int thdl); /* thdl--Thread handle*/

int  DOMGetNextObject(LPUCHAR lpobjectname, 
                        LPUCHAR lpresultowner,      
                        LPUCHAR lpresultextrastring);

int  LIBMON_DOMGetNextObject(LPUCHAR lpobjectname, 
                        LPUCHAR lpresultowner,      
                        LPUCHAR lpresultextrastring,
						int thdl);/* thdl--Thread handle*/

BOOL DOMGetObjListVer(int hnodestruct,
                        int iobjecttype,
                        int level,
                        LPUCHAR *parentstrings,
                        long *lpver);

int  UpdateDOMData		(int hnodestruct,
                        int iobjecttype,
                        int level,
                        LPUCHAR *parentstrings,
                        BOOL bWithSystem,
                        BOOL bWithChildren);

int	LIBMON_UpdateDOMData(int hnodestruct,
                        int iobjecttype,
                        int level,
                        LPUCHAR *parentstrings,
                        BOOL bWithSystem,
                        BOOL bWithChildren,
						int		thdl);/* thdl--Thread handle*/
int  UpdateDOMDataAllUsers(int hnodestruct,
                        int iobjecttype,
                        int level,
                        LPUCHAR *parentstrings,
                        BOOL bWithSystem,
                        BOOL bWithChildren);

#ifndef LIBMON_HEADER
int  DOMUpdateDisplayDataAllUsers (int      hnodestruct,    // handle on node struct
								   int      iobjecttype,    // object type
								   int      level,          // parenthood level
								   LPUCHAR *parentstrings,  // parenthood names
								   BOOL     bWithChildren,  // should we expand children?
								   int      iAction,        // why is this function called ?
								   DWORD    idItem,         // if expansion: item being expanded
								   HWND     hwndDOMDoc);    // handle on DOM MDI document

int  DOMUpdateDisplayAllDataAllUsers (int hnodestruct);     // handle on node struct

int  UpdateDOMDataDetail (int hnodestruct,
                          int iobjecttype,
                          int level,
                          LPUCHAR *parentstrings,
                          BOOL bWithSystem,
                          BOOL bWithChildren,
                          BOOL bOnlyIfExist,
                          BOOL bUnique,
                          BOOL bWithDisplay);

int  UpdateDOMDataDetailAllUsers  (int hnodestruct,
 								   int iobjecttype,
								   int level,
								   LPUCHAR *parentstrings,
								   BOOL bWithSystem,
								   BOOL bWithChildren,
								   BOOL bOnlyIfExist,
								   BOOL bUnique,
								   BOOL bWithDisplay);
#endif /* #ifndef LIBMON_HEADER */


#ifndef BLD_MULTI /*To make available in Non multi-thread context only */
BOOL CleanNodeReplicInfo(int hdl);
#endif
BOOL IsNodeUsed4ExtReplMon(LPUCHAR NodeName);

#ifdef LIBMON_HEADER
BOOL UpdateReplStatusinCacheNeedRefresh (LPUCHAR NodeName,
							 LPUCHAR DBName,
							 int svrno,
							 LPUCHAR evttext);
	#ifndef BLD_MULTI	/*To make available in Non multi-thread context only */
		BOOL CloseCachesExtReplicInfoNeedRefresh(LPUCHAR NodeName);
	#endif
#else
int UpdateReplStatusinCache (LPUCHAR NodeName,
							 LPUCHAR DBName,
							 int svrno,
							 LPUCHAR evttext,
							 BOOL bDisplay);
int CloseCachesExtReplicInfo(LPUCHAR NodeName);
#endif

#if defined(LIBMON_HEADER) || defined(BLD_MULTI)
void push4domg(int thdl); /* thdl--Thread handle*/
void pop4domg(int thdl); /* thdl--Thread handle*/
#else
void push4domg();
void pop4domg();
#endif

#define REPLIC_VER_ERR   (-1)
#define REPLIC_NOINSTALL 0
#define REPLIC_V10       1
#define REPLIC_V105      2
#define REPLIC_V11       3

#ifndef BLD_MULTI /*To make available in Non multi-thread context only */
int GetReplicInstallStatus(int hnodestruct, LPUCHAR lpDBName, LPUCHAR lpDBOwner);
#endif

#ifndef LIBMON_HEADER
long PrepareGlobalDOMUpd(BOOL bReset); /* see comment in source if 0L is returned */ 
int    GetICeUserAndPassWord(int hnodestruct, LPUCHAR bufuser, LPUCHAR bufpasswd);
int    SetICeUserAndPassWord(int hnodestruct, LPUCHAR bufuser, LPUCHAR bufpasswd);
#endif

int    GetMaxVirtNodes      (void);
BOOL   isVirtNodeEntryBusy  (int hnodestruct);
char * GetVirtNodeName      (int hnodestruct);
int    GetVirtNodeHandle    (LPUCHAR lpNodeName);
int    NodeConnections      (LPUCHAR lpNodeName);
int    NodeServerConnections(LPUCHAR lpNodeName);
BOOL   isLocalWithRealNodeName(LPUCHAR lpNodeName);
BOOL   ReplaceLocalWithRealNodeName(LPUCHAR lpNodeName);

BOOL   ClearCacheMonData  (LPUCHAR lpNodeName);


int    DOMBkRefresh(time_t refreshtime,BOOL bFromAnimation, HWND hwndAnimation);
void   RefreshOnLoad(void);
BOOL   RefreshPropWindows_MT(BOOL bOnload, time_t refreshtime);
BOOL   RefreshPropWindows(BOOL bOnload, time_t refreshtime);

int    DOMGetObject(int     hnodestruct,
                  LPUCHAR lpobjectname,
                  LPUCHAR lpobjectowner,

                  int     iobjecttype,
                  int     level,
                  LPUCHAR *parentstrings,
                  BOOL    bwithsystem,

                  int     *presulttype,
                  int     *presultlevel,
                  LPUCHAR *resultparentstrings,/* the pointers must point to
                                                 existing memory areas before
                                                 the call. these areas will
                                                 be filled */

                  LPUCHAR lpresultobjectname,  /* will normally be filled with
                                                  the same string as in
                                                  lpobjectname */
                  LPUCHAR lpresultowner,      
                  LPUCHAR lpresultextradata);


int DOMGetObjectLimitedInfo(int     hnodestruct,
                            LPUCHAR lpobjectname,
                            LPUCHAR lpobjectowner,
                            int     iobjecttype,
                            int     level,
                            LPUCHAR *parentstrings,
                            BOOL    bwithsystem,
                            int     *presulttype,
                            LPUCHAR lpresultobjectname,  
                            LPUCHAR lpresultowner, 
                            LPUCHAR lpresultextradata);



int  DOMGetFirstRelObject(int hnodestruct,
                          int iobjecttype,
                          int level,
                          LPUCHAR *parentstrings, /*if last level(s) empty,
                                                   no filter on these levels*/    
                          BOOL    bwithsystem,
                          LPUCHAR lpDBfilter,
                          LPUCHAR lpobjectfilter,
                          LPUCHAR *resultparentstrings,/* the pointers must
                                                 point to existing memory
                                                 areas before the call.
                                                 these areas will be filled */

                          LPUCHAR lpresultobjectname,
                          LPUCHAR lpresultowner,      
                          LPUCHAR lpresultextradata);

int  DOMGetNextRelObject (LPUCHAR *resultparentstrings,/* the pointers must
                                                 point to existing memory
                                                 areas before the call.
                                                 these areas will be filled */
                          LPUCHAR lpresultobjectname, 
                          LPUCHAR lpresultowner,      
                          LPUCHAR lpresultextrastring);

int DOMGetDBOwner(int hnodestruct,LPUCHAR dbname, LPUCHAR resultdbowner);

int DOMLocationUsageAccepted(int       hnodestruct,
                             LPUCHAR   lpobjectname,
                             int       usagetype,
                             BOOL    * pReturn);

int GetRepObjectType4ll(int iobjecttype,int ireplicver);
int getMonStack(void);
#ifdef LIBMON_HEADER
int LIBMON_ExistRemoveSessionProc(int hnode);
int LIBMON_ExistShutDownSvr( int hnode);
int LIBMON_ExistProcedure4OpenCloseSvr( int hnode);
#else
BOOL ExistRemoveSessionProc(int hnode);
BOOL ExistShutDownSvr( int hnode);
BOOL ExistProcedure4OpenCloseSvr( int hnode);
#endif


