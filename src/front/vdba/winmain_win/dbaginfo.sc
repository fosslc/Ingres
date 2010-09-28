//#define NOUSEICEAPI
//#define NOCONNECT   // for demo version : any connect attempt fails immediately
/********************************************************************
**
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**  Project : Visual DBA
**
**  Source : dbaginfo.sc
**
**  includes:
**     -session cache management
**     -low level queries for getting the list of objects appearing in DOM and Monitor trees
**     -misc DBEvent tracking functions (related to the cache) allowing
**      background refresh of what is DBEvent driven(replicator monitoring, 
**      and DBEvent trace windows)
**
**  Author : Francois Noirot-Nerin
**
**  History after 04-May-1999:
**
**	05-May-1999 (noifr01)
**      fixed a bug resulting, when a replicator DBEvent was gotten 
**      (resulting in some refresh in the display), in getting an 
**      error message in the GUI, and having CTRL+E show the error : 
**      " E_LQ002E The 'get dbevent' query has been issued outside of
**      a DBMS session"
**  06-May-1999 (noifr01)
**      resetted sqlca.sqlcode to 0 at sereral places immediatly 
**      before invoking set_sql(session=...) since this specific
**      statement deosn't seem to reset it to 0 if it works 
**      succcessfuly. At the corresponding places, invoking 
**      ManageSQLError(&sqlca) resulted in duplicating the previous
**      SQL error (if any) in the history of errors (and errvdba.log)
**  31-May-1999 (schph01)
**      Add the field "database_owner" in the SQL statement in
**      OT_REPLIC_CDDSDBINFO_V11 type. Dba owner is required for send a
**      remote command.see Bugs #97168
**  05-Jul-1999 (noifr01)
**      (bug #97805) DBACloseNodeSessions() (close connections in the cache)
**      was not working properly when the input parameter was a node connection
**      uncluding a user impersonation, and the request was done with
**      bAllUsers == TRUE (close sessions on the same node regardless of the
**      connected user). A side effect was that even if all connections were
**      done under the same impersonated user, connections done against a "node"
**      equal to the "local hostname" were not considered in this case as the
**      "(local)" node, resulting in additional problems for some replicator
**      features where the connection for some queries is done under the local 
**      host name
**  04-Oct-1999 (noifr01)
**      (bug #99017, fixed through an alternate fix to bug #97490)): 
**      ensure that SESSION_TYPE_CACHENOREADLOCK sessions can be
**      used in the general case (not only for an immediate query and
**      session release). The (sufficient) change is just to Activate the
**      session upon session release, regardless of the session type.
**  22-Dec-1999 (schph01)
**       Bug #97680 Remove the quotes on User/Group/Role name, for the database
**       security alarm.
**  29-Dec-1999 (noifr01)
**       (bug #99867) added fetching of ICE Business Unit Facet roles attributes
**       (for storage in the cache) when fetching the corresponding list
**  10-Jan-2000 (noifr01)
**       cross-integrated following change (443743) from kitch01
**		25-Aug-1999 (kitch01)
**		   Bug 96991. Amend sessionid member of structure def_sessions
**		   to be char[33]. session_id's defined in IMA catalogs are
**		   varchar(32). Amended everywhere sessionid is used accordingly.
**		   Removed cast of session_id to int4 that causes this bug.
**  19-Jan-2000 (noifr01)
**      (bug 100063) (remove build warnings). An assignement from an 
**      uninitialized variable was removed: it corresponded to a variable that
**      is passed back to the caller, but is not used by the caller in this case
**      (the server number for a replicator connection, that doesn't apply at this
**      level for versions of replicator >= 1.1. The assignement is done at
**      another place  when connected to a replicator version of 1.05 or earlier)
**  24-Jan-2000 (noifr01)
**     (SIR 100117) set a description to VDBA sessions, when connecting to a
**     non gateway, non Ingres/Desktop node
**   26-Jan-2000 (noifr01)
**    (bug 100155) 
**    - get the server pid in the queries for getting the monitor
**     servers and transactions
**   27-Jan-2000 (noifr01)
**    (additional fix for bug 100155) 
**     thanks to dbmsinfo('ima_server'), fixed the remaining issue
**     of "the impact of the measure on the result", i.e. when excluding
**     the transaction or locklist used by the query, when querying the list
**     of transactions or locklists through imadb.  dbmsinfo('ima_server') allows 
**     indirectly to know the server pid of the connected session and 
**     take it into account in this context
**   03-Mar-2000 (noifr01)
**    (bug 100632) invoke ICE_C_Terminate() in DBACLoseNodeSessions()
**    (called when disconnecting from a node), and DBAginfoFree() (called
**    when exiting VDBA)
**   13-Mar-2000 (noifr01)
**   (bug 100867) emergency fix for EDBC: don't "set sessions with
**   description" under EDBC, where it is normally not accepted by gateways.
**   later fix is to be done for non EDBC builds, in the init sequence
**   (when called from the "CheckConnection() function)
**   21-Mar-2000 (noifr01)
**   (bug 100951) added isolation level management in the sessions cache:
**   GetSession() now accepts the (new) SESSION_KEEP_DEFAULTISOLATION bit 
**   in the sessiontype parameter: if not set, the isolation level will be 
**   forced to serializable (except for gateways and Ingres versions older
**   than OpenIngres 2.x) , except for ima sessions where if this bit is not
**   set, the session will be enforced to read_uncommitted (the less
**   restrictive), since ima queries are anyhow temporary snapshots and a
**   stricter isolation level does not seem to be needed
**   5-Apr-2000 (noifr01)
**   (bug 101173) (side effect of previous fix for bug 100951): don't invoke 
**   the isolation level statement in the case of a STAR database, where it
**   results in an SQL error. As a result, no DOM Star Database sub-branch
**   can be expanded
**   26-Apr-2000 (schph01)
**   (bug 101329) In the raw, cached list used both for getting the CDDS's
**   and the propagation paths, put path values of -1,-1 and -1 for the rows
**   representing a CDDS with no propogation path, instead of 0,0,0 that was
**   leading to a confusion
**   05-May-2000 (schph01)
**   bug 101445 Remove trim() in the select statement for retrieve the OT_RULE
**   because the column name in "order by" clause is not recognize with 
**   INGRES 2.0 or lower.
**   14-Sep-2000 (schph01)
**   Bug 102597 the buffer where the username is stored was filled only for
**   the first element in the linked list when there was no security alarm
**   name (in which case there is a "system" name, starting with the $ sign,
**   which we don't display in VDBA ), due to an un-needed logic.
**   Now fill the buffer in all cases.
**   19-Dec-2000 (noifr01)
**   (bug 103515) in GetSession(), manage a new flag indicating that the
**   session is used for "checking the connection". Moved the code that
**   checks the level of Ingres from the CheckConnection() function to
**   that GetSession() function (and call SetOIVers() accordingly), so 
**   that the code that worked only against certain versions now gets called
**   only depending on the version that has been detected
**   The code is now generic, which allows to remove some #ifdef EDBC's
**   04-Jan-2000 (noifr01)
**   fixed side effect of previous change for bug 103515: the change was
**   resulting in an error -40500 to appear in the history of errors (although
**   the rest was working). This was because the new select statement which is
**   now done within GetSession() when checking a connection was not committed,
**   resulting in an error in a statement such as "set lockmode" that followed.
**   12-Mar-2001 (schph01)
**   SIR 97068 retrieve the language used to create the view: QUEL or SQL.
**   21-Mar-2001 (noifr01)
**   (sir 104270) removal of code for managing Ingres/Desktop
**  30-Mar-2001 (noifr01)
**    (sir 104378) differentiation of II 2.6.
**    also added a test that the OIVERS_ definitions are in ascending order,
**    which now allows to simplify the tests against the version of ingres
**    all over the code
**  09-May-2001 (noifr01)
**    (bug 104674) return an error immediatly after a connection failure in
**    GetSession(), rather than letting the next statement fail and the
**    additional failure being reported. Also removed the retry of a connection
**    if it failed (obsolete code that anyhow isn't called any more)
**  10-May-2001 (schph01)
**     SIR 102509 manage raw data location.temporary disabled because of
**     concurrent changes in catalog.
**  15-Jun-2001(schph01)
**     SIR 104991 add function GetInfGrantUser4Rmcmd() to verify the rmcmd
**     grants availables for the current user.
**  25-Jul-2001 (noifr01)
**    (bug 105079) don't execute the "set lockmode session" statement for STAR
**    databases if SESSION_TYPE_CACHENOREADLOCK has been requested, since appa
**    rently the statement doesn't work always for STAR databases
**  27-Jul-2001 (noifr01)
**    (bug 102895) when getting the list of running servers, get also the
**     "capabilities" column in ima_dbms_server_parameters. 
**  19-Oct-2001 (noifr01)
**    (bug 106099) add an error in the history of SQL errors if the max number 
**    of sessions has been reached
**  01-Nov-2001 (hanje04)
**	Cast 'parentstrings' to (char *) before subtracting from 'pctemp' to
**	stop compiler errors on Linux
**  10-Dec-2001 (noifr01)
**   (sir 99596) removal of obsolete code and resources
**   also rearranged slightly change of 01-Nov-2001
**  28-Mar-2002 (noifr01)
**    (sir 99596) removed additional unused resources/sources
**  30-Apr-2002 (noifr01)
**  (bug 107688)
**  previous change was incorrect and resulted (among others) in sub-branches
**  of a table to remain empty. Applied corrective cast
**  02-Sep-2002 (schph01)
**    bug 108645 Add variable static tcLocalVnode initialized by InitLocalVnode()
**    function,
**    InitLocalVnode() retrieve the parameter XX.XXXXX.gcn.local_vnode
**    parameter in config.dat
**  07-Oct-2002 (schph01)
**    bug 108883 Add function GetInfOwner4RmcmdObject() to verify
**    the owner of the rmcmd objects (tables, views, dbevents and procedures)
**  25-Mar-2003 (noifr01)
**   (sir 107523) management of sequences
**  17-Sep-2003 (schph01)
**    Sir 110673 Add a new buffer (bufnew8[1001]) with the same size that the
**    column session_query define in structure SESSIONDATAMIN.
**  24-Nov-2003 (schph01)
**    (SIR 107122)Update test for determine the 2.65 ingres version.
**  23-Jan-2004 (schph01)
**     (sir 104378) detect version 3 of Ingres, for managing
**     new features provided in this version. replaced references
**     to 2.65 with refereces to 3  in #define definitions for
**     better readability in the future
**  29-Nov-2004 (schph01)
**     (bug 113540) update GetInfGrantUser4Rmcmd function.
**  09-Mar-2005 (nansa02)
**     Bug # 114041 Alter, infodb and Duplicatedb options disabled when connecting
**     to a HP-UX machine from vdba due to incorrect ingres version.
**  28-Jul-2005 (lazro01)
**     Bug # 114942 Added ORDER BY clause that will have the DBMS server  
**     return the data related to "Grantees on Table for Select" in sorted 
**     order.
**  02-Feb-2006 (drivi01)
**    BUG 115691
**    Keep VDBA tools at the same version as DBMS server.  Update VDBA
**    to 9.0 level versioning. Make sure that 9.0 version string is
**    correctly interpreted by visual tools and VDBA internal version
**    is set properly upon that interpretation.
**  12-Oct-2006 (wridu01)
**    BUG 116835
**    Keep VDBA tools at the same version as DBMS server.  Update VDBA
**    to 9.1 level versioning. Make sure that 9.1 version string is
**    correctly interpreted by visual tools and VDBA internal version
**    is set properly upon that interpretation.
**
**    Restructured the test to determine Ingres version.  Ingres 2007 has
**    a version string that begins with "II 9.1.0".  This was incorrectly 
**    being interpreted as OI 1.2.
**  10-Oct-2007 (drivi01)
**    Added a commit statement after list of databases is queried on startup
**    or refresh to make sure the lock on iidbdb is released.
** 14-Dec-2007 (kiria01) b119671
**    Tightened up the version parsing to ensure 9.1.0 and 9.2.0 did not
**    get recognised as 1.2 and 2.0 respectively and allow for future 9.x
**    to at least have 9.1 capabilities.
** 12-May-2009 (smeke01) b121598
**    Added sort on procedure_owner so that the text segments for database 
**    procedures with the same name but different owners are sorted 
**    correctly for the concatenation function.
** 11-Jan-2010 (drivi01)
**    Add support for new Ingres version 10.0.
** 22-Mar-2010 (drivi01)
**    Add a check for DBMS_TYPE to avoid VDBA running in limitted mode
**    when prefix for version string in Ingres installation changes.
**    Add routines for detecting VectorWise installations to enable
**    special features for VectorWise.
**    VectorWise Ingres installation version will always be set to 
**    OIVERS_100 to take advantage of all Ingres 10.0 functionality in 
**    VDBA.
** 08-Apr-2010 (drivi01)
**    Ingres VectorWise product has been updated to return INGRES_VECTORWISE
**    DBMS_TYPE.  Updated the code to recognize VectorWise installation using
**    DBMS_TYPE for VectorWise instead of version string.
** 11-May-2010 (drivi01)
**    Update DBAOrgGetFirstObject, retrieve table storage structure 
**    when querying the information about table.  Set the storage structure
**    in the Table parameters structure so it's always available 
**    when table parameters sturcture is available.
** 09-Jun-2010 (drivi01)
**    Initialize ExtraData in pcur structure prior to storing data
**    in the structure to avoid left over garbage in the memory 
**    be picked up by tables. Primarily this is done to zero out
**    Ingres Vectorwise storage structure type.
** 27-Jul-2010 (drivi01)
**    Add #ifndef NOUSEICEAPI to remove ice code that can be enabled
**    at any time by enable NOUSEICEAPI from a solution.
******************************************************************************/

#include <time.h>
#include <stdio.h>
#include <tchar.h>
#include "dba.h"
#include "domdata.h"
#include "dbaginfo.h"
#include "dbaparse.h"
#include "error.h"
#include "winutils.h"
#include "dbaset.h"
#include "settings.h"
#include "msghandl.h"
#include "dll.h"
#include "domdata.h"     
#include "resource.h"       // "out of memory" error management
#include "monitor.h"
#include "extccall.h"
#include "compat.h"
#include <er.h>
#include <lo.h>
#include "pm.h"
#include "gl.h"

#include "ice.h"

int  i,j,nbItems;
int  imaxsessions    = 6;
long llastgetsession = 0;
static BOOL bsuppspace;
static int iDBATypemem;
static BOOL bDisplayMessage = TRUE;

#define MAXREGSVR 40
struct def_sessions {
  UCHAR SessionName[2*MAXOBJECTNAME+2];
  BOOL  bLocked;
  int   sessiontype;
  BOOL  bKeepDefaultIsolation;
  BOOL  bIsStarDatabase;
  int   OIvers;
  BOOL  bIsVW;  //Is this a VectorWise installation;
  HWND  hwnd;
  BOOL  bIsDBESession;
  BOOL  bIsDBE4Replic;
  int   nbDBEEntries;
  BOOL  nbDBERegSvr;
  int   DBERegSvr[MAXREGSVR];
  BOOL  bMainDBERegistered;
  int   hnode;      // only if bIsDBE4Replic it set
  char   sessionid[33];
  int   session_serverpid;
  unsigned long lgetsession;
};
struct def_sessions session[MAXSESSIONS];

struct lldbadata {
   UCHAR Name[MAXOBJECTNAME];
   UCHAR OwnerName[MAXOBJECTNAME];
   UCHAR ExtraData[EXTRADATASIZE];
   union {
     SERVERDATAMIN      SvrData;
     SESSIONDATAMIN     SessionData;
     LOCKLISTDATAMIN    LockListData;
     LOCKDATAMIN        LockData;
     RESOURCEDATAMIN    ResourceData;
     LOGPROCESSDATAMIN  LogProcessData;
     LOGDBDATAMIN       LogDBdata;
     LOGTRANSACTDATAMIN LogTransacData;
     DD_REGISTERED_TABLES Registred_Tables;
     REPLICSERVERDATAMIN ReplicSrvDta;
     REPLICCDDSASSIGNDATAMIN ReplicCddsDta;
     STARDBCOMPONENTPARAMS   StarDbCompDta;
     ICE_CONNECTED_USERDATAMIN IceConnectedUserDta;
     ICE_ACTIVE_USERDATAMIN    IceActiveUserDta;
     ICE_TRANSACTIONDATAMIN    IceTransactionDta;
     ICE_CURSORDATAMIN         IceCursorDta;
     ICE_CACHEINFODATAMIN      IceCacheInfoDta;
     ICE_DB_CONNECTIONDATAMIN  IceDbConnectionDta;

   }uData;
   struct lldbadata *pprev;
   struct lldbadata *pnext;

};
struct lldbadata * staticplldbadata;
struct lldbadata * pcur;


struct SecuritySets SecuritySet[]={ 
   {OT_S_ALARM_SELSUCCESS_USER,  FALSE},
   {OT_S_ALARM_SELFAILURE_USER,  FALSE},
   {OT_S_ALARM_DELSUCCESS_USER,  FALSE},
   {OT_S_ALARM_DELFAILURE_USER,  FALSE},
   {OT_S_ALARM_INSSUCCESS_USER,  FALSE},
   {OT_S_ALARM_INSFAILURE_USER,  FALSE},
   {OT_S_ALARM_UPDSUCCESS_USER,  FALSE},
   {OT_S_ALARM_UPDFAILURE_USER,  FALSE},
   {OT_S_ALARM_CO_SUCCESS_USER,  FALSE},
   {OT_S_ALARM_CO_FAILURE_USER,  FALSE},
   {OT_S_ALARM_DI_SUCCESS_USER,  FALSE},
   {OT_S_ALARM_DI_FAILURE_USER,  FALSE}};
struct SecuritySets *pSecuritySet=&SecuritySet[0];
// includes SQL
exec sql include sqlca;
exec sql include sqlda;

struct lldbadata *GetNextLLData(pcurdata)
struct lldbadata *pcurdata;
{
  if (!pcurdata->pnext) {
    pcurdata->pnext= (struct lldbadata *)
                     ESL_AllocMem(sizeof(struct lldbadata));
    if (pcurdata->pnext)    // test added 17/10/95 Emb
      (pcurdata->pnext)->pprev = pcurdata;
  }
  return pcurdata->pnext;
}

static BOOL ICEIsChecked(LPUCHAR buf)
{
	if (!x_stricmp(buf,"checked"))
		return TRUE;
	else
		return FALSE;
}

static int iThreadCurSess = -1;
int DBAThreadDisableCurrentSession()
{
	/* the following statement will fail with old preprocessors/ingres.lib (single threaded LIBQ), but */
	/* it doesn't matter because in this case (single-threaded libq), there is no need to execute this */
	/* statement */
	sqlca.sqlcode =0L; /* since set_sql doesn't seem to reset this value*/
	exec sql set_sql(session=none);

	ManageSqlError(&sqlca); // Keep track of the SQL error if any
	if (sqlca.sqlcode < 0) 
		return RES_ERR;
	return RES_SUCCESS;
}
int DBAThreadEnableCurrentSession()
{
	if (iThreadCurSess<0)
		return RES_SUCCESS;
	return ActivateSession(iThreadCurSess);
}

BOOL HasSelectLoopInterruptBeenRequested() 
{
	if (HasLoopInterruptBeenRequested()) { // add here possible test on the query interrupt when available
		AddSqlError("VDBA Interrupt", "The user has requested an interrupt",-1);
		sqlca.sqlcode = -1;
		return TRUE;
	}
	return FALSE;


}

static TCHAR tcLocalVnode[MAXOBJECTNAME];
BOOL InitLocalVnode ( void )
{
	int iret = FALSE;
	memset (&tcLocalVnode,0,MAXOBJECTNAME);
	PMinit();
	if (PMload( NULL, (PM_ERR_FUNC *)NULL ) == OK)
	{
		char config_string[256];
		char *host,*value = NULL;

		/*
		** Get the host name.
		*/
		host = PMhost();

		/*
		** Build the string we will search for in the config.dat file.
		*/
		STprintf( config_string,
		          ERx("%s.%s.gcn.local_vnode"),
		          SystemCfgPrefix, host );
		if (PMget(config_string, &value) == OK && value != NULL)
			if ((value[0] != '\0'))
			{
				STcopy(value, tcLocalVnode);
				iret = TRUE;
			}
		PMfree();
	}
	return iret;
}


void GetLocalVnodeName (TCHAR *tclocnode, int iLenBuffer)
{
    fstrncpy(tclocnode, tcLocalVnode, iLenBuffer );
}

static int CmpItems(int iobjecttype,struct lldbadata * pcur1, struct lldbadata * pcur2)
{
   switch(iobjecttype) {
      case OT_MON_SERVER :
         return CompareMonInfo1(OT_MON_SERVER, &(pcur1->uData.SvrData),&(pcur2->uData.SvrData),TRUE);
         break;
      case OT_MON_LOCKLIST :
         return CompareMonInfo1(OT_MON_LOCKLIST, &(pcur1->uData.LockListData),&(pcur2->uData.LockListData),TRUE);
         break;
      case OT_MON_LOCK           :
      case OT_MON_LOCKLIST_LOCK  :
      case OT_MON_RES_LOCK   :
         return CompareMonInfo1(OT_MON_LOCK,   &(pcur1->uData.LockData),&(pcur2->uData.LockData),TRUE);
         break;
      case OT_MON_SESSION        :
         return CompareMonInfo1(OT_MON_SESSION,&(pcur1->uData.SessionData),&(pcur2->uData.SessionData),TRUE);
         break;
      case OTLL_MON_RESOURCE       :
         return CompareMonInfo1(OTLL_MON_RESOURCE,&(pcur1->uData.ResourceData),&(pcur2->uData.ResourceData),TRUE);
         break;
      case OT_MON_LOGPROCESS     :
         return CompareMonInfo1(OT_MON_LOGPROCESS,&(pcur1->uData.LogProcessData),&(pcur2->uData.LogProcessData),TRUE);
         break;
      case OT_MON_LOGDATABASE    :
         return CompareMonInfo1(OT_MON_LOGDATABASE,&(pcur1->uData.LogDBdata),&(pcur2->uData.LogDBdata),TRUE);
         break;
      case OT_MON_REPLIC_SERVER    :
         return CompareMonInfo1(OT_MON_REPLIC_SERVER,&(pcur1->uData.ReplicSrvDta),&(pcur2->uData.ReplicSrvDta),TRUE);
         break;
      case OT_MON_TRANSACTION :
         return CompareMonInfo1(OT_MON_TRANSACTION,&(pcur1->uData.LogTransacData),&(pcur2->uData.LogTransacData),TRUE);
         break;
      case OT_REPLIC_REGTBLS_V11:
         return CompareMonInfo1(OT_REPLIC_REGTBLS_V11,&(pcur1->uData.Registred_Tables),&(pcur2->uData.Registred_Tables),TRUE);
         break;
      case OT_MON_REPLIC_CDDS_ASSIGN:
         return CompareMonInfo1(OT_MON_REPLIC_CDDS_ASSIGN,&(pcur1->uData.ReplicCddsDta),&(pcur2->uData.ReplicCddsDta),TRUE);
         break;
      case OT_STARDB_COMPONENT:
         return CompareMonInfo1(OT_STARDB_COMPONENT,&(pcur1->uData.StarDbCompDta),&(pcur2->uData.StarDbCompDta),TRUE);
         break;
      case OT_MON_ICE_CONNECTED_USER:
         return CompareMonInfo1(OT_MON_ICE_CONNECTED_USER,&(pcur1->uData.IceConnectedUserDta),&(pcur2->uData.IceConnectedUserDta),TRUE);
         break;
      case OT_MON_ICE_ACTIVE_USER:
         return CompareMonInfo1(OT_MON_ICE_ACTIVE_USER,&(pcur1->uData.IceActiveUserDta),&(pcur2->uData.IceActiveUserDta),TRUE);
         break;
      case OT_MON_ICE_TRANSACTION:
         return CompareMonInfo1(OT_MON_ICE_TRANSACTION,&(pcur1->uData.IceTransactionDta),&(pcur2->uData.IceTransactionDta),TRUE);
         break;
      case OT_MON_ICE_CURSOR:
         return CompareMonInfo1(OT_MON_ICE_CURSOR,&(pcur1->uData.IceCursorDta),&(pcur2->uData.IceCursorDta),TRUE);
         break;
      case OT_MON_ICE_FILEINFO:
         return CompareMonInfo1(OT_MON_ICE_FILEINFO,&(pcur1->uData.IceCacheInfoDta),&(pcur2->uData.IceCacheInfoDta),TRUE);
         break;
      case OT_MON_ICE_DB_CONNECTION:
         return CompareMonInfo1(OT_MON_ICE_DB_CONNECTION,&(pcur1->uData.IceDbConnectionDta),&(pcur2->uData.IceDbConnectionDta),TRUE);
         break;

      default:
         myerror(ERR_MONITORTYPE);
         break;
   }
   return 0;
}

static void  SortItems(iobjecttype)
{
  int i1,icmp;
  struct lldbadata * pcur0;
  struct lldbadata * pcur1;
  struct lldbadata * pcur1p;
  struct lldbadata * pcurtempmem;
  struct lldbadata * pcurtemp0;
  struct lldbadata * pcurtemp1;
  switch (iobjecttype) {
    case OTLL_SECURITYALARM:
    case OTLL_DBSECALARM:
      for (pcur0=staticplldbadata,i1=0;i1<nbItems;pcur0=pcurtempmem,i1++) {
        pcurtempmem=pcur0->pnext;
        for (pcur1=pcur0;pcur1->pprev;) {
          pcur1p=pcur1->pprev;
          if (DOMTreeCmpllSecAlarms(
                     pcur1 ->Name,
                     pcur1 ->OwnerName,
                     pcur1 ->ExtraData,
                     (long)getint(pcur1 ->ExtraData+MAXOBJECTNAME),
                     pcur1p->Name,
                     pcur1p->OwnerName,
                     pcur1p->ExtraData,
                     (long)getint(pcur1p->ExtraData+MAXOBJECTNAME))>=0)
            break;
          pcurtemp0=pcur1p->pprev;
          pcurtemp1=pcur1 ->pnext;
          if (pcurtemp0)
            pcurtemp0->pnext=pcur1;
          else
            staticplldbadata=pcur1; /* start of list */
          if (pcurtemp1)
            pcurtemp1->pprev=pcur1p;
          pcur1p->pprev=pcur1;
          pcur1p->pnext=pcurtemp1;
          pcur1 ->pprev=pcurtemp0;
          pcur1 ->pnext=pcur1p;
        }
      }
      break;
    case OTLL_GRANTEE:
      for (pcur0=staticplldbadata,i1=0;i1<nbItems;pcur0=pcurtempmem,i1++) {
        pcurtempmem=pcur0->pnext;
        for (pcur1=pcur0;pcur1->pprev;) {
          pcur1p=pcur1->pprev;
          if (DOMTreeCmpllGrants(
                     pcur1 ->Name, pcur1 ->OwnerName, pcur1 ->ExtraData,
                     pcur1p->Name, pcur1p->OwnerName, pcur1p->ExtraData)>=0)
            break;
          pcurtemp0=pcur1p->pprev;
          pcurtemp1=pcur1 ->pnext;
          if (pcurtemp0)
            pcurtemp0->pnext=pcur1;
          else
            staticplldbadata=pcur1; /* start of list */
          if (pcurtemp1)
            pcurtemp1->pprev=pcur1p;
          pcur1p->pprev=pcur1;
          pcur1p->pnext=pcurtemp1;
          pcur1 ->pprev=pcurtemp0;
          pcur1 ->pnext=pcur1p;
        }
      }
      break;
    case OTLL_DBGRANTEE:
    case OTLL_OIDTDBGRANTEE:
      for (pcur0=staticplldbadata,i1=0;i1<nbItems;pcur0=pcurtempmem,i1++) {
        pcurtempmem=pcur0->pnext;
        for (pcur1=pcur0;pcur1->pprev;) {
          pcur1p=pcur1->pprev;
          if (DOMTreeCmpDoubleStr(
                     pcur1->OwnerName,
                     pcur1->Name,
                     pcur1p->OwnerName,
                     pcur1p->Name)>=0)
            break;
          pcurtemp0=pcur1p->pprev;
          pcurtemp1=pcur1 ->pnext;
          if (pcurtemp0)
            pcurtemp0->pnext=pcur1;
          else
            staticplldbadata=pcur1; /* start of list */
          if (pcurtemp1)
            pcurtemp1->pprev=pcur1p;
          pcur1p->pprev=pcur1;
          pcur1p->pnext=pcurtemp1;
          pcur1 ->pprev=pcurtemp0;
          pcur1 ->pnext=pcur1p;
        }
      }
      break;
    case OT_REPLIC_DISTRIBUTION:
    case OT_REPLIC_DISTRIBUTION_V11:
    case OT_REPLIC_CONNECTION_V11:
    case OT_REPLIC_CONNECTION:
//    case OT_REPLIC_CDDSDBINFO_V11:
      for (pcur0=staticplldbadata,i1=0;i1<nbItems;pcur0=pcurtempmem,i1++) {
        pcurtempmem=pcur0->pnext;
        for (pcur1=pcur0;pcur1->pprev;) {
          pcur1p=pcur1->pprev;
          if (getint(pcur1->ExtraData)>=getint(pcur1p->ExtraData))
            break;
          pcurtemp0=pcur1p->pprev;
          pcurtemp1=pcur1 ->pnext;
          if (pcurtemp0)
            pcurtemp0->pnext=pcur1;
          else
            staticplldbadata=pcur1; /* start of list */
          if (pcurtemp1)
            pcurtemp1->pprev=pcur1p;
          pcur1p->pprev=pcur1;
          pcur1p->pnext=pcurtemp1;
          pcur1 ->pprev=pcurtemp0;
          pcur1 ->pnext=pcur1p;
        }
      }
      break;
   case OT_REPLIC_CDDSDBINFO_V11:
      for (pcur0=staticplldbadata,i1=0;i1<nbItems;pcur0=pcurtempmem,i1++) {
        pcurtempmem=pcur0->pnext;
        for (pcur1=pcur0;pcur1->pprev;) {
          pcur1p=pcur1->pprev;
          if (getint(pcur1->ExtraData)>getint(pcur1p->ExtraData))
            break;
          if (getint(pcur1->ExtraData+3*STEPSMALLOBJ)>=getint(pcur1p->ExtraData))
            break;
          pcurtemp0=pcur1p->pprev;
          pcurtemp1=pcur1 ->pnext;
          if (pcurtemp0)
            pcurtemp0->pnext=pcur1;
          else
            staticplldbadata=pcur1; /* start of list */
          if (pcurtemp1)
            pcurtemp1->pprev=pcur1p;
          pcur1p->pprev=pcur1;
          pcur1p->pnext=pcurtemp1;
          pcur1 ->pprev=pcurtemp0;
          pcur1 ->pnext=pcur1p;
        }
      }
      break;
    case OT_REPLIC_REGTBLS:
    //case OT_REPLIC_REGTBLS_V11:
    case OT_REPLIC_REGTABLE:
    //case OT_REPLIC_REGTABLE_V11:
    case OT_REPLIC_REGCOLS:
    case OT_REPLIC_REGCOLS_V11:
    case OT_REPLIC_DBMS_TYPE_V11:
      for (pcur0=staticplldbadata,i1=0;i1<nbItems;pcur0=pcurtempmem,i1++) {
        pcurtempmem=pcur0->pnext;
        for (pcur1=pcur0;pcur1->pprev;) {
          pcur1p=pcur1->pprev;
          if (x_stricmp(pcur1->Name,pcur1p->Name)>=0)
            break;
          pcurtemp0=pcur1p->pprev;
          pcurtemp1=pcur1 ->pnext;
          if (pcurtemp0)
            pcurtemp0->pnext=pcur1;
          else
            staticplldbadata=pcur1; /* start of list */
          if (pcurtemp1)
            pcurtemp1->pprev=pcur1p;
          pcur1p->pprev=pcur1;
          pcur1p->pnext=pcurtemp1;
          pcur1 ->pprev=pcurtemp0;
          pcur1 ->pnext=pcur1p;
        }
      }
      break;
    case OT_REPLIC_CDDS :
    case OTLL_REPLIC_CDDS:
    case OT_REPLIC_CDDS_V11:
    case OTLL_REPLIC_CDDS_V11:
      for (pcur0=staticplldbadata,i1=0;i1<nbItems;pcur0=pcurtempmem,i1++) {
        pcurtempmem=pcur0->pnext;
        for (pcur1=pcur0;pcur1->pprev;) {
          pcur1p=pcur1->pprev;
          if (DOMTreeCmp4ints(
                     getint(pcur1-> ExtraData),
                     getint(pcur1-> ExtraData+STEPSMALLOBJ),
                     getint(pcur1-> ExtraData+(2*STEPSMALLOBJ)),
                     getint(pcur1-> ExtraData+(3*STEPSMALLOBJ)),
                     getint(pcur1p->ExtraData),
                     getint(pcur1p->ExtraData+STEPSMALLOBJ),
                     getint(pcur1p->ExtraData+(2*STEPSMALLOBJ)),
                     getint(pcur1p->ExtraData+(3*STEPSMALLOBJ)))>=0)
            break;
          pcurtemp0=pcur1p->pprev;
          pcurtemp1=pcur1 ->pnext;
          if (pcurtemp0)
            pcurtemp0->pnext=pcur1;
          else
            staticplldbadata=pcur1; /* start of list */
          if (pcurtemp1)
            pcurtemp1->pprev=pcur1p;
          pcur1p->pprev=pcur1;
          pcur1p->pnext=pcurtemp1;
          pcur1 ->pprev=pcurtemp0;
          pcur1 ->pnext=pcur1p;
        }
      }
      break;
    case OT_MON_SERVER         :
    case OT_MON_LOCKLIST       :
    case OT_MON_LOCK           :
    case OT_MON_LOCKLIST_LOCK  :
    case OT_MON_RES_LOCK       :
    case OT_MON_SESSION        :
    case OTLL_MON_RESOURCE     :
    case OT_MON_LOGPROCESS     :
    case OT_MON_LOGDATABASE    :
    case OT_MON_TRANSACTION    :
    case OT_MON_REPLIC_SERVER  :
    case OT_MON_REPLIC_CDDS_ASSIGN:
    case OT_REPLIC_REGTBLS_V11 :
    case OT_STARDB_COMPONENT   :
    case OT_MON_ICE_CONNECTED_USER:
    case OT_MON_ICE_TRANSACTION   :
    case OT_MON_ICE_CURSOR        :
    case OT_MON_ICE_ACTIVE_USER   :
    case OT_MON_ICE_FILEINFO      :
    case OT_MON_ICE_DB_CONNECTION :
      for (pcur0=staticplldbadata,i1=0;i1<nbItems;pcur0=pcurtempmem,i1++) {
        pcurtempmem=pcur0->pnext;
        for (pcur1=pcur0;pcur1->pprev;) {
          pcur1p=pcur1->pprev;
          if (CmpItems(iobjecttype,pcur1,pcur1p)>=0)
             break;
          pcurtemp0=pcur1p->pprev;
          pcurtemp1=pcur1 ->pnext;
          if (pcurtemp0)
            pcurtemp0->pnext=pcur1;
          else
            staticplldbadata=pcur1; /* start of list */
          if (pcurtemp1)
            pcurtemp1->pprev=pcur1p;
          pcur1p->pprev=pcur1;
          pcur1p->pnext=pcurtemp1;
          pcur1 ->pprev=pcurtemp0;
          pcur1 ->pnext=pcur1p;
        }
      }
      break;
    case OT_VIEWCOLUMN:
    case OT_COLUMN:
      for (pcur0=staticplldbadata,i1=0;i1<nbItems;pcur0=pcurtempmem,i1++) {
        pcurtempmem=pcur0->pnext;
        for (pcur1=pcur0;pcur1->pprev;) {
          pcur1p=pcur1->pprev;
          if (getint(pcur1->ExtraData+4*STEPSMALLOBJ)>=getint(pcur1p->ExtraData+4*STEPSMALLOBJ))
            break;
          pcurtemp0=pcur1p->pprev;
          pcurtemp1=pcur1 ->pnext;
          if (pcurtemp0)
            pcurtemp0->pnext=pcur1;
          else
            staticplldbadata=pcur1; /* start of list */
          if (pcurtemp1)
            pcurtemp1->pprev=pcur1p;
          pcur1p->pprev=pcur1;
          pcur1p->pnext=pcurtemp1;
          pcur1 ->pprev=pcurtemp0;
          pcur1 ->pnext=pcur1p;
        }
      }
      break;

    case OT_TABLE: 
    case OT_VIEW:
    case OT_DATABASE:
    case OT_PROFILE:
    case OT_ROLEGRANT_USER:
    case OT_LOCATION:
    case OT_USER:
    case OT_GROUP:
    case OT_ROLE:
    case OT_PROCEDURE:
    case OT_SEQUENCE:
    case OT_SCHEMAUSER:
    case OT_DBEVENT:
    case OT_SYNONYM:
    case OT_INTEGRITY:
    //case OT_REPLIC_REGTABLE:
    case OT_REPLIC_REGTABLE_V11:
    //case OT_REPLIC_DISTRIBUTION:
    //case OT_REPLIC_DISTRIBUTION_V11:
    //case OT_REPLIC_REGTBLS:
    //case OT_REPLIC_REGTBLS_V11:
    case OT_REPLIC_MAILUSER:
    case OT_REPLIC_MAILUSER_V11:
    case OT_VIEWTABLE:
    case OT_RULE:
    case OT_INDEX:
    case OT_TABLELOCATION:
    case OT_GROUPUSER:
    case OT_ICE_ROLE                 :
    case OT_ICE_DBUSER               :
    case OT_ICE_DBCONNECTION         :
    case OT_ICE_WEBUSER              :
    case OT_ICE_WEBUSER_ROLE         :
    case OT_ICE_WEBUSER_CONNECTION   :
    case OT_ICE_PROFILE              :
    case OT_ICE_PROFILE_ROLE         :
    case OT_ICE_PROFILE_CONNECTION   :
    case OT_ICE_BUNIT                :
    case OT_ICE_BUNIT_SEC_ROLE       :
    case OT_ICE_BUNIT_SEC_USER       :
    case OT_ICE_BUNIT_FACET          :
    case OT_ICE_BUNIT_PAGE           :
    case OT_ICE_BUNIT_LOCATION       :
    case OT_ICE_BUNIT_FACET_ROLE     :
    case OT_ICE_BUNIT_FACET_USER     :
    case OT_ICE_BUNIT_PAGE_ROLE      :
    case OT_ICE_BUNIT_PAGE_USER      :
    case OT_ICE_SERVER_APPLICATION   :
    case OT_ICE_SERVER_LOCATION      :
    case OT_ICE_SERVER_VARIABLE      :
      for (pcur0=staticplldbadata,i1=0;i1<nbItems;pcur0=pcurtempmem,i1++) {
        pcurtempmem=pcur0->pnext;
        if (CanObjectHaveSchema(iobjecttype)) {
          suppspacebutleavefirst(pcur0->Name);
          suppspace(pcur0->OwnerName);
        }
        for (pcur1=pcur0;pcur1->pprev;) {
          pcur1p=pcur1->pprev;
          icmp = DOMTreeCmpStr(pcur1 ->Name,pcur1 ->OwnerName,
                               pcur1p->Name,pcur1p->OwnerName,
                               iobjecttype,TRUE);
          if (!icmp && CanObjectHaveSchema(iobjecttype)) {
             char * pchar;
             char bufloc[100];
             pchar=GetSchemaDot(pcur1->Name);//pchar=strchr(pcur1->Name,'.');
             if (!pchar) {
                sprintf(bufloc,"%s.%s", Quote4DisplayIfNeeded(pcur1->OwnerName),pcur1->Name);
                fstrncpy(pcur1->Name,bufloc,sizeof(pcur1->Name));
             }
             pchar=GetSchemaDot(pcur1p->Name);//pchar=strchr(pcur1p->Name,'.');
             if (!pchar) {
                sprintf(bufloc,"%s.%s",Quote4DisplayIfNeeded(pcur1p->OwnerName),pcur1p->Name);
                fstrncpy(pcur1p->Name,bufloc,sizeof(pcur1p->Name));
             }
             icmp = DOMTreeCmpStr(pcur1 ->Name,pcur1 ->OwnerName,
                                  pcur1p->Name,pcur1p->OwnerName,
                                  iobjecttype,TRUE);
          }
          if (icmp>=0)
            break;
          pcurtemp0=pcur1p->pprev;
          pcurtemp1=pcur1 ->pnext;
          if (pcurtemp0)
            pcurtemp0->pnext=pcur1;
          else
            staticplldbadata=pcur1; /* start of list */
          if (pcurtemp1)
            pcurtemp1->pprev=pcur1p;
          pcur1p->pprev=pcur1;
          pcur1p->pnext=pcurtemp1;
          pcur1 ->pprev=pcurtemp0;
          pcur1 ->pnext=pcur1p;
        }
      }
      break;
    default:
      myerror(ERR_LIST);
      break;
  }
}

static char SessionUserIdentifier[100]; /* TCHAR not accepted in declare section - TO BE FINISHED */


void DBAginfoInit()
{
  int i;
  for (i=0;i<MAXSESSIONS;i++) {
    session[i].SessionName[0]='\0';
    session[i].bLocked       = FALSE;
    session[i].bIsDBESession = FALSE;
  }
  SessionUserIdentifier[0]='\0';
  iThreadCurSess = -1;
}

long sessionnumber(hdl)
int hdl;
{
  return (hdl+1);
}

static int GetType(lpstring)
LPUCHAR lpstring;
{
  switch (*lpstring) {
    case 'T':
    case 't':
      return OT_TABLE;
      break;
    case 'V':
    case 'v':
      return OT_VIEW;
      break;
    case 'P':
    case 'p':
      return OT_PROCEDURE;
      break;
    case 'E':
    case 'e':
      return OT_DBEVENT;
      break;
    case 'S':
    case 's':
      return OT_SEQUENCE;
      break;
    default:
      return OT_ERROR;
  }
}

int GetNodeAndDB(LPUCHAR NodeName,LPUCHAR DBName,LPUCHAR SessionName)
{
    LPUCHAR pc;
    x_strcpy(NodeName,SessionName);
    pc=x_strstr(NodeName,"::");
    if (!pc) {
      DBName[0]='\0';
      return RES_ERR;
    }
    *pc='\0';
    x_strcpy(DBName,pc+2);
    return RES_SUCCESS;
}

static int GetMainCacheHdl( int hdl) 
{
  UCHAR NodeName[100];
  LPUCHAR pc;

  if (hdl<0 || hdl>imaxsessions) {
    myerror(ERR_SESSION);
    return FALSE;
  }

  x_strcpy(NodeName,session[hdl].SessionName);
  pc=x_stristr(NodeName,"::");
  if (pc){
     *pc='\0';
     return GetVirtNodeHandle(NodeName);
  }
  return -1;
}

static BOOL IsImaActiveSession(int hdl) 
{
  UCHAR NodeName[100];
  LPUCHAR pc;

  if (hdl<0 || hdl>imaxsessions) {
    myerror(ERR_SESSION);
    return FALSE;
  }

  x_strcpy(NodeName,session[hdl].SessionName);
  pc=x_stristr(NodeName,"::imadb");
  if (pc){
     int hnode;
     *pc='\0';
     if (session[hdl].bLocked)
       return TRUE;
     hnode=GetVirtNodeHandle(NodeName);
     if (hnode<0)
       return FALSE;
     if (MonOpenWindowsCount(hnode))
       return TRUE;
  }
  return FALSE;
}

static BOOL ClearImaCache(int hdl) {
  UCHAR NodeName[100];
  LPUCHAR pc;

  if (hdl<0 || hdl>imaxsessions) {
    myerror(ERR_SESSION);
    return FALSE;
  }

  x_strcpy(NodeName,session[hdl].SessionName);
  pc=x_stristr(NodeName,"::imadb");
  if (pc){
     *pc='\0';
     return ClearCacheMonData(NodeName);
  }
  return FALSE;
}

static char GetYNState(lpstring)
LPUCHAR lpstring;
{
  switch (*lpstring) {
    case 'Y':
    case 'y':
      return ATTACHED_YES;
      break;
    case 'N':
    case 'n':
      return ATTACHED_NO ;
      break;
    default:
      return ATTACHED_UNKNOWN;
  }
}

int ReleaseSession(hdl, releasetype)
int hdl;
int releasetype;
{
  int ires;

  if (hdl<0 || hdl>imaxsessions)
    return myerror(ERR_SESSION);
  if (!session[hdl].bLocked)
    return myerror(ERR_SESSION);

  ires = ActivateSession(hdl);
  if (ires!=RES_SUCCESS)
    return ires;

  ires=RES_SUCCESS;
  sqlca.sqlcode = 0; /* for cases with no sql command */ 
  switch (releasetype) {
    case RELEASE_NONE     :
      break;
    case RELEASE_COMMIT   :
      exec sql commit;
      ManageSqlError(&sqlca); // Keep track of the SQL error if any
      break;
    case RELEASE_ROLLBACK :
      exec sql rollback;
      ManageSqlError(&sqlca); // Keep track of the SQL error if any
      break;
    default:
      return myerror(ERR_SESSION);
      break;
  }
  if (sqlca.sqlcode < 0)  
     ires=RES_ERR;

  if (session[hdl].sessiontype==SESSION_TYPE_INDEPENDENT ) {
    exec sql disconnect;
    ManageSqlError(&sqlca); // Keep track of the SQL error if any
    session[hdl].SessionName[0]='\0';
    if (sqlca.sqlcode < 0)  
      ires=RES_ERR;
  }
  session[hdl].bLocked = FALSE;
  session[hdl].bIsDBESession = FALSE;
  iThreadCurSess = -1;
  return ires;
}

int CommitSession(hdl)
int hdl;
{
  int ires;

  if (hdl<0 || hdl>imaxsessions)
    return myerror(ERR_SESSION);
  if (!session[hdl].bLocked)
    return myerror(ERR_SESSION);

  ires = ActivateSession(hdl);
  if (ires!=RES_SUCCESS)
    return ires;

  ires=RES_SUCCESS;
  sqlca.sqlcode = 0; /* for cases with no sql command */ 
  exec sql commit;
  ManageSqlError(&sqlca); // Keep track of the SQL error if any
  if (sqlca.sqlcode < 0)  
     ires=RES_ERR;

  return ires;
}

int ActivateSession(hdl)
int hdl;
{
  exec sql begin declare section;
    long isession;
  exec sql end declare section;

  if (hdl<0 || hdl>imaxsessions)
    return myerror(ERR_SESSION);
  if (!session[hdl].bLocked)
    return myerror(ERR_SESSION);

  isession=sessionnumber(hdl);
  sqlca.sqlcode = 0;
  exec sql set_sql(session=:isession);
  ManageSqlError(&sqlca); // Keep track of the SQL error if any
  if (sqlca.sqlcode < 0) 
     return RES_ERR;
  iThreadCurSess = hdl;
  return RES_SUCCESS;
}
int GetImaDBsession(lpnodename, sessiontype, phdl)
UCHAR *lpnodename;
int sessiontype;
int * phdl;
{
  UCHAR buf[100];
  wsprintf(buf,"%s::imadb",lpnodename);
  return Getsession(buf, sessiontype, phdl);
}

BOOL SetSessionUserIdentifier(LPUCHAR username)
{
     lstrcpy(SessionUserIdentifier,username);
     return TRUE;
}
BOOL ResetSessionUserIdentifier()
{
     lstrcpy(SessionUserIdentifier,_T(""));
     return TRUE;
}

static UCHAR bufCAPINode[100];
LPUCHAR CAPINodeName(LPUCHAR lpVirtNode)
{
     UCHAR szLocal[100];
	 x_strcpy(bufCAPINode,lpVirtNode);
     RemoveGWNameFromString(bufCAPINode);
	 RemoveConnectUserFromString(bufCAPINode);

	 x_strcpy(szLocal, ResourceString ((UINT)IDS_I_LOCALNODE) );
     if (!x_strcmp(bufCAPINode, szLocal)) 
		return NULL;
     return bufCAPINode;
}
    

void RemoveGWNameFromString(LPUCHAR lpstring)
{
    char * pc= x_strstr(lpstring, LPCLASSPREFIXINNODENAME);
    if (pc) {
        char *pc2=x_strstr(pc,LPCLASSSUFFIXINNODENAME);
        if (pc2) {
           char buf[200];
           x_strcpy(buf,pc2+x_strlen(LPCLASSSUFFIXINNODENAME));
           x_strcpy(pc,buf);
        }
    }
    return;
}

void RemoveConnectUserFromString(LPUCHAR lpstring)
{
    char * pc= x_strstr(lpstring, LPUSERPREFIXINNODENAME);
    if (pc) {
        char *pc2=x_strstr(pc,LPUSERSUFFIXINNODENAME);
        if (pc2) {
           char buf[200];
           x_strcpy(buf,pc2+x_strlen(LPUSERSUFFIXINNODENAME));
           x_strcpy(pc,buf);
        }
    }
    return;
}
BOOL GetGWClassNameFromString(LPUCHAR lpstring, LPUCHAR lpdestbuffer)
{
    char * pc =x_strstr(lpstring,LPCLASSPREFIXINNODENAME);
    if (!pc) {
        lpdestbuffer[0]='\0';
        return FALSE;
    }
    x_strcpy(lpdestbuffer,pc+x_strlen(LPCLASSPREFIXINNODENAME));
    pc=x_strstr(lpdestbuffer,LPCLASSSUFFIXINNODENAME);
    if (!pc) {
        lpdestbuffer[0]='\0';
        return FALSE;
    }
    *pc='\0';
    return TRUE;
}

BOOL GetConnectUserFromString(LPUCHAR lpstring, LPUCHAR lpdestbuffer)
{
    char * pc =x_strstr(lpstring,LPUSERPREFIXINNODENAME);
    if (!pc) {
        lpdestbuffer[0]='\0';
        return FALSE;
    }
    x_strcpy(lpdestbuffer,pc+x_strlen(LPUSERPREFIXINNODENAME));
    pc=x_strstr(lpdestbuffer,LPUSERSUFFIXINNODENAME);
    if (!pc) {
        lpdestbuffer[0]='\0';
        return FALSE;
    }
    *pc='\0';
    return TRUE;
}
 
int Getsession(lpsessionname, sessiontype, phdl)
UCHAR *lpsessionname;
int sessiontype;
int * phdl;

{
  int           i,ichoice;
  unsigned long lchoice;
  int           nberr;
  char          szLocal[MAXOBJECTNAME];   // string for local connection
  char         *pColon;                   // colon search in connect name
  BOOL          bDoInitIma;
  BOOL          bHasGWSuffix;
  BOOL          bHasUserSuffix;
  char          szgateway[200];
  char          szTempconnectuser[200];
  BOOL          bKeepDefaultIsolation;
  BOOL          bCheckingConnection;
  BOOL          bIsStarDatabase;

  exec sql begin declare section;
    char connectname[2*MAXOBJECTNAME+2];
    char        szconnectuser[200];

    /* Emb Sep.14, 95: duplicate connectname for local connection purpose */
    char connectnameConn[2*MAXOBJECTNAME+2+10];

    long isession;
    long ltimeout;
    char buf[200];
    char dbms_type[MAXOBJECTNAME];
    char sessionid[33];
	int session_serverpid;
  exec sql end declare section;

   if (sessiontype & SESSION_CHECKING_CONNECTION) {
		bCheckingConnection = TRUE;
		sessiontype &= ~SESSION_CHECKING_CONNECTION;
	}
	else
		bCheckingConnection = FALSE;

  if (! bCheckingConnection && GetOIVers() != OIVERS_NOTOI && GetOIVers() != OIVERS_12 )
      bKeepDefaultIsolation = (sessiontype & SESSION_KEEP_DEFAULTISOLATION) ? TRUE:FALSE;
   else
      bKeepDefaultIsolation = TRUE;

  sessiontype &=(SESSION_KEEP_DEFAULTISOLATION-1);

  bDoInitIma     = FALSE;
  bIsStarDatabase= FALSE;
  bHasGWSuffix   = GetGWClassNameFromString(lpsessionname, szgateway);
  bHasUserSuffix = GetConnectUserFromString(lpsessionname, szconnectuser);
  if (SessionUserIdentifier[0]) {
     bHasUserSuffix = TRUE;
     lstrcpy(szconnectuser,SessionUserIdentifier);
  }
  if (szconnectuser[0]) {
    lstrcpy(szTempconnectuser,szconnectuser);
    lstrcpy(szconnectuser,(LPUCHAR)QuoteIfNeeded(szTempconnectuser));
  }

  llastgetsession ++;
  if (llastgetsession==(unsigned long) (-1)) {
    for (i=0;i<imaxsessions;i++)
      session[i].lgetsession = (unsigned long) 0; 
  }

  ichoice=-1;
  lchoice=(unsigned long) (-1);
  for (i=0;i<imaxsessions;i++) {
    if (session[i].bLocked)
      continue;
    if (sessiontype!=SESSION_TYPE_INDEPENDENT         &&
        !x_strcmp(lpsessionname,session[i].SessionName) &&
        /* for only checking a connection, the Isolation level doesn't matter */
        (bCheckingConnection ||   session[i].bKeepDefaultIsolation == bKeepDefaultIsolation ) 
       ) {
      isession=sessionnumber(i);
      StartSqlCriticalSection();
      sqlca.sqlcode =0L; /* since set_sql doesn't seem to reset this value*/
      exec sql set_sql(session=:isession);
      ManageSqlError(&sqlca); // Keep track of the SQL error if any
      exec sql commit;
      ManageSqlError(&sqlca); // Keep track of the SQL error if any
      StopSqlCriticalSection();
      ichoice=i;
      if (bCheckingConnection)
      {
          SetOIVers(session[ichoice].OIvers);
	  SetVW(session[ichoice].bIsVW);
      }
      bIsStarDatabase = session[ichoice].bIsStarDatabase;
      goto endgetsession;
    }
    if (ichoice>=0 && !lchoice) /* free entry already found */
      continue;
	if (x_stristr(session[i].SessionName,"::imadb"))  {
		if (GetMainCacheHdl(i) >=0)
			continue;
	}
    if (!session[i].SessionName[0]) { /*free entry */
      ichoice=i;
      lchoice = (unsigned long) 0;
    }
    else {          /* looks for oldest entry */
      if (ichoice <0 ) {
        ichoice=i;
        lchoice = session[i].lgetsession;
      }
      else {
        if (session[i].lgetsession< lchoice) {
          ichoice=i;
          lchoice = session[i].lgetsession;
        }
      }
    }
  }
  if (ichoice <0) {
    AddSqlError("VDBA connection", "You have reached the maximum number of sessions defined in the VDBA preferences",-1);
    return RES_ERR;
  }

  if (session[ichoice].SessionName[0]) {
    isession=sessionnumber(ichoice);
    StartSqlCriticalSection();
    sqlca.sqlcode =0L; /* since set_sql doesn't seem to reset this value*/
    exec sql set_sql(session=:isession);
    ManageSqlError(&sqlca); // Keep track of the SQL error if any
    exec sql disconnect;
    ManageSqlError(&sqlca); // Keep track of the SQL error if any
    StopSqlCriticalSection();
    ClearImaCache(ichoice);
    session[ichoice].SessionName[0]='\0';
  }
  fstrncpy(connectname,lpsessionname, sizeof(connectname)); 
  isession=sessionnumber(ichoice);

#ifdef NOCONNECT
  // Do not try to connect for a demo version
  return RES_ERR;
#endif

  if (sessiontype!=SESSION_TYPE_INDEPENDENT) {
    if (x_stristr(connectname,"::imadb"))
       bDoInitIma= TRUE;
  }

  {
    char NodeName[100];
    char *pc;
    x_strcpy(NodeName,connectname);
    pc=x_stristr(NodeName,"::");
    if (pc){
      int hnode;
      *pc='\0';
      hnode=GetVirtNodeHandle(NodeName);
      if (hnode>=0) {
        if (IsStarDatabase(hnode,pc+2))
          bIsStarDatabase = TRUE;
      }
    }
    x_strcpy(connectnameConn, connectname);
    if (bHasGWSuffix) {
        RemoveGWNameFromString(connectnameConn);
        if (!bIsStarDatabase) {
            x_strcat(connectnameConn,"/");
            x_strcat(connectnameConn,szgateway);
        }
    }
    if (bIsStarDatabase)
       x_strcat(connectnameConn, "/STAR");
    
    if (bHasUserSuffix) 
        RemoveConnectUserFromString(connectnameConn);

    // prepare string for local connection if necessary
    x_strcpy(szLocal, ResourceString ((UINT)IDS_I_LOCALNODE) );
    if (memcmp(connectnameConn, szLocal, x_strlen(szLocal)) == 0) {
      pColon = x_strstr(connectnameConn, "::");
      if (pColon) {
        x_strcpy(buf, pColon+x_strlen("::"));
        x_strcpy(connectnameConn, buf);
      }
    }

    StartSqlCriticalSection();
    StartCriticalConnectSection();

    if (/* sessiontype==SESSION_TYPE_INDEPENDENT  && */ szconnectuser[0]) {
       exec sql connect :connectnameConn session :isession identified by :szconnectuser;
    }
    else {
       exec sql connect :connectnameConn session :isession;
    }

    ManageSqlError(&sqlca); // Keep track of the SQL error if any

    StopCriticalConnectSection();
    StopSqlCriticalSection();

    if (sqlca.sqlcode == -34000) 
      return RES_NOGRANT;

    if (sqlca.sqlcode <0) 
      return RES_ERR;

    if (bCheckingConnection) {
         int  ioiversion = OIVERS_NOTOI;
         BOOL bIsIngres=FALSE;
         BOOL bIsVectorWise=FALSE;
         int i;
         static char * ingprefixes[]= {"OI","Oping","Ingres","II","ING"};

         if (OIVERS_12<= OIVERS_NOTOI || 
             OIVERS_20<= OIVERS_12    || 
             OIVERS_25<= OIVERS_20    ||
             OIVERS_26<= OIVERS_25)     {
            myerror(ERR_GW);
            exec sql disconnect;
            return RES_ERR;
         }

         exec sql select dbmsinfo('_version') into :buf;
         ManageSqlError(&sqlca); // Keep track of the SQL error if any
         if (sqlca.sqlcode<0) {
             exec sql disconnect;
            return RES_ERR;
         }
         exec sql commit;
         ManageSqlError(&sqlca); // Keep track of the SQL error if any
         if (sqlca.sqlcode<0) {
             exec sql disconnect;
            return RES_ERR;
         }
          buf[sizeof(buf)-1]='\0';
         for (i=0;i<sizeof(ingprefixes)/sizeof(char *);i++) {
             if (!x_strnicmp(buf,ingprefixes[i],x_strlen(ingprefixes[i])))
                 bIsIngres=TRUE;
         }
         
         /* Add additional check for Ingres in case if the version prefix changes 
         ** or it's VectorWise release, this time check for DBMS_TYPE.  
         ** Potentially the check above with ingprefixes array can be taken out.
         */
         if (!bIsIngres)
         {
             exec sql select cap_value into :dbms_type from iidbcapabilities where cap_capability='DBMS_TYPE';
             ManageSqlError(&sqlca);
             if (sqlca.sqlcode<0) 
             {
                 exec sql disconnect;
                 return RES_ERR;
             }
             exec sql commit;
             ManageSqlError(&sqlca); // Keep track of the SQL error if any
             if (sqlca.sqlcode<0) 
             {
                 exec sql disconnect;
                 return RES_ERR;
             }
             if (strstr(dbms_type, _T("INGRES")) > 0)
                 bIsIngres=TRUE;
	     if (strstr(dbms_type, _T("INGRES_VECTORWISE")) > 0)
		bIsVectorWise=TRUE;

             
          }
         
         // Change logic to better parse version string
         // Allow that the version might have been at the beginning of the version
         // but must have been preceded by a space in later versions and non-digit
         // for earlier.

         // Assume not openIngres 1.x nor 2.x : restricted mode
         ioiversion = OIVERS_NOTOI;

         // Check if this is VectorWise, as we want to have the same functionality for
         // VW as for Ingres, we need to set OIVERS high enough to open all of the functionality.
         if (bIsIngres && bIsVectorWise)
               ioiversion = OIVERS_100;
         else if (bIsIngres)
         {
			char *v;
		 	if ((v = strstr(buf, "10.")) && (v == buf || v[-1] == ' '))
			{
			 	ioiversion = OIVERS_100;
		 	}
			else if ((v = strstr(buf, "9.")) && (v == buf || v[-1] == ' '))
			{
				switch (v[2])
				{
				default:
// Insert new 9.x ... in get so that unrecognised 9.x at least gets the highest
// capability known about as we know it is a 9. something and isn't 9.0.x
				case '1': ioiversion = OIVERS_91;
					break;
				case '0': ioiversion = OIVERS_90;
				}
			}					
			else if ((v = strstr(buf, "3.")) && (v == buf || v[-1] < '0' || v[-1] > '9'))
			{
				ioiversion = OIVERS_30;
			}
			else if ((v = strstr(buf, "2.")) && (v == buf || v[-1] < '0' || v[-1] > '9'))
			{
				switch (v[2])
				{
				case '6': ioiversion = OIVERS_26;
					break;
				case '5': ioiversion = OIVERS_25;
					break;
				default:  ioiversion = OIVERS_20;
				}
			}					
			else if ((v = strstr(buf, "1.")) && (v == buf || v[-1] < '0' || v[-1] > '9'))
			{
                ioiversion = OIVERS_12;
			}
		}

      SetOIVers (ioiversion);
      SetVW(bIsVectorWise);
      if (GetOIVers() != OIVERS_NOTOI && GetOIVers() != OIVERS_12 )
         bKeepDefaultIsolation = FALSE; /* session used for checking the connection should be available for further usage when possible */
    }
    if (!bKeepDefaultIsolation && !bIsStarDatabase) {
      exec sql set session isolation level serializable, read write; /* for IMA, the session will be set to read_uncommitted after the init sequence */
       ManageSqlError(&sqlca); // Keep track of the SQL error if any
       if (sqlca.sqlcode<0) {
          exec sql disconnect;
         return RES_ERR;
       }
    }
    session[ichoice].bKeepDefaultIsolation = bKeepDefaultIsolation;
    session[ichoice].OIvers = GetOIVers();
    session[ichoice].bIsVW = IsVW();

  }
  if (sqlca.sqlcode < 0) 
    return RES_ERR;
  fstrncpy(session[ichoice].SessionName,connectname,
           sizeof(session[ichoice].SessionName));
endgetsession:
  /* set the lockmodes and time out  */
   ltimeout=GlobalSessionParams.time_out;
   if (ltimeout<0L)
      ltimeout=0L;
  sqlca.sqlcode = 0; /* for cases with no sql command */ 
  StartSqlCriticalSection();

  for (nberr=0;nberr<2;nberr++) {
    switch (sessiontype) {
      case SESSION_TYPE_INDEPENDENT    :
        exec sql set lockmode session where timeout=:ltimeout;
        ManageSqlError(&sqlca); // Keep track of the SQL error if any
        break;
      case SESSION_TYPE_CACHEREADLOCK  :
        exec sql set lockmode session where readlock=shared, timeout=:ltimeout;
        ManageSqlError(&sqlca); // Keep track of the SQL error if any
        break;
      case SESSION_TYPE_CACHENOREADLOCK:
        if (!bIsStarDatabase) { /* bug 105079 */ 
          exec sql set lockmode session where readlock=nolock, timeout=:ltimeout;
          ManageSqlError(&sqlca); // Keep track of the SQL error if any
        }
        break;
      default:
        StopSqlCriticalSection();
        return RES_ERR;
    }
    if (sqlca.sqlcode==-40500 && !nberr) {
      /* unstable state due normally to uncommited requests in SQL Test */
      exec sql disconnect;
      ManageSqlError(&sqlca); // Keep track of the SQL error if any
      x_strcpy(connectname,session[ichoice].SessionName);

      // prepare string for local connection if necessary
      x_strcpy(connectnameConn, connectname);

      if (bHasGWSuffix) {
        RemoveGWNameFromString(connectnameConn);
        x_strcat(connectnameConn,"/");
        x_strcat(connectnameConn,szgateway);
      }
      if (bHasUserSuffix) 
        RemoveConnectUserFromString(connectnameConn);

      x_strcpy(szLocal, ResourceString ((UINT)IDS_I_LOCALNODE) );
      if (memcmp(connectnameConn, szLocal, x_strlen(szLocal)) == 0) {
        pColon = x_strstr(connectname, "::");
        if (pColon) {
          x_strcpy(connectnameConn, pColon+x_strlen("::"));
        }
      }

      isession=sessionnumber(ichoice);
      StartCriticalConnectSection();
      if (/* sessiontype==SESSION_TYPE_INDEPENDENT  && */ szconnectuser[0]) {
         exec sql connect :connectnameConn session :isession identified by :szconnectuser;
      }
      else {
         exec sql connect :connectnameConn session :isession;
      }
      ManageSqlError(&sqlca); // Keep track of the SQL error if any
      StopCriticalConnectSection();

      if (sqlca.sqlcode == -34000)  {
         StopSqlCriticalSection();
         return RES_NOGRANT;
      }
      if (sqlca.sqlcode < 0)  {
         StopSqlCriticalSection();
         return RES_ERR;
      }
      if (!bKeepDefaultIsolation && !x_stristr(connectnameConn,"::imadb")) {
         exec sql set session isolation level serializable, read write; /* for IMA, the session will be set to read_uncommitted after the init sequence */
         ManageSqlError(&sqlca); // Keep track of the SQL error if any
         if (sqlca.sqlcode<0) {
            exec sql disconnect;
           return RES_ERR;
         }
      }
    }
    else 
      break;
  }
  StopSqlCriticalSection();
  if (sqlca.sqlcode < 0) { 
     exec sql disconnect;
     ManageSqlError(&sqlca); // Keep track of the SQL error if any
     session[ichoice].SessionName[0]='\0';
     return RES_ERR;
  }

   /* GetOIVers() can now be used here, after change for bug 103515 */
   if (GetOIVers() != OIVERS_NOTOI) {
      exec sql set session with description='Visual DBA';
      ManageSqlError(&sqlca); // Keep track of the SQL error if any
   }

  if (bDoInitIma) {
     exec sql execute procedure ima_set_server_domain;
     ManageSqlError(&sqlca); // Keep track of the SQL error if any

     exec sql execute procedure ima_set_vnode_domain;
     ManageSqlError(&sqlca); // Keep track of the SQL error if any

     exec sql commit;
     ManageSqlError(&sqlca); // Keep track of the SQL error if any

    if (!bKeepDefaultIsolation) {
       exec sql set session isolation level read uncommitted;
       ManageSqlError(&sqlca); // Keep track of the SQL error if any
    }
     exec sql set lockmode session where readlock=nolock, timeout=:ltimeout;
     ManageSqlError(&sqlca); // Keep track of the SQL error if any

	 /* Bug 96991. Use ima_session as this returns a char(32) value
	 **	that is platform independent. session_id returns incorrect values
	 **	for 64-bit platforms (see fix for bug 78975 in scuisvc.c)
	 */
	
/* noifr01 10-jan-2000: added the "repeated" attribute upon the "propagate"
** operation, consistently with the VDBA 2.6 code line
*/
     exec sql repeated select dbmsinfo('ima_session') into :sessionid;
     ManageSqlError(&sqlca); // Keep track of the SQL error if any

	 STcopy(sessionid, session[ichoice].sessionid);

     exec sql repeated select server_pid 
                       into  :session_serverpid
                       from ima_dbms_server_parameters
					   where server=dbmsinfo('ima_server');
     ManageSqlError(&sqlca); // Keep track of the SQL error if any
    if (sqlca.sqlcode != 0) { /*even +100 is not acceptable*/
      exec sql disconnect;
      ManageSqlError(&sqlca); // Keep track of the SQL error if any
      session[ichoice].SessionName[0]='\0';
      return RES_ERR;
    }

     session[ichoice].session_serverpid = session_serverpid;

     exec sql commit;
     ManageSqlError(&sqlca); // Keep track of the SQL error if any
  }

  session[ichoice].bIsStarDatabase = bIsStarDatabase;
  session[ichoice].sessiontype   = sessiontype;
  session[ichoice].lgetsession   = llastgetsession;
  session[ichoice].bLocked       = TRUE;
  session[ichoice].bIsDBESession = FALSE;
  *phdl = ichoice;
  iThreadCurSess = ichoice;
  return RES_SUCCESS;
}

LPUCHAR GetSessionName(hdl)
int hdl;
{
  if (hdl<0 || hdl>imaxsessions) {
    myerror(ERR_SESSION);
    return "";
  }
  if (!session[hdl].bLocked) 
    myerror(ERR_SESSION);

  return  session[hdl].SessionName;
}

static BOOL addgranteeobject(lpi,lpsinglename,lpsingleownername,type,value)
int *lpi;
LPUCHAR lpsinglename;
LPUCHAR lpsingleownername;
int type;
int value;
{
  fstrncpy(pcur->Name,lpsinglename,MAXOBJECTNAME);
  fstrncpy(pcur->OwnerName,lpsingleownername,MAXOBJECTNAME);
  storeint(pcur->ExtraData,type);
  storeint(pcur->ExtraData+STEPSMALLOBJ,value);
  pcur=GetNextLLData(pcur);
  if (!pcur) 
    return FALSE;
  (*lpi)++;
  return TRUE;
}

static ICE_C_CLIENT    ICEclient = NULL;

// name modified Emb 26/4/95 for critical section test
static int  DBAOrgGetFirstObject(lpVirtNode,iobjecttype,level,parentstrings,
                       bWithSystem,lpobjectname,lpownername,lpextradata)
LPUCHAR lpVirtNode;
int     iobjecttype;
int     level;
LPUCHAR *parentstrings;
BOOL    bWithSystem;
LPUCHAR lpobjectname;
LPUCHAR lpownername;
LPUCHAR lpextradata;
{
  BOOL bGenericGateway = FALSE;
  char connectname[2*MAXOBJECTNAME+2];
  char * pctemp;
  int iret,itmp;
  UCHAR OLDname[MAXOBJECTNAME], OLDowner[MAXOBJECTNAME];
  BOOL IsFound;
  int iresselect, ilocsession ;
  BOOL bHasDot;
  UCHAR ParentWhenDot[MAXOBJECTNAME],ParentOwnerWhenDot[MAXOBJECTNAME];
  exec sql begin declare section;
    char singlename[MAXOBJECTNAME];
    char singleownername[MAXOBJECTNAME];
    char storage_struct[MAXOBJECTNAME];
    char name3[MAXOBJECTNAME];
    char bufev1[MAXOBJECTNAME];
    char bufev2[MAXOBJECTNAME];
    char bufnew1[MAXOBJECTNAME];
    char bufnew2[MAXOBJECTNAME];
    char bufnew3[MAXOBJECTNAME];
    char bufnew4[MAXOBJECTNAME];
    char bufnew5[MAXOBJECTNAME];
    char bufnew6[MAXOBJECTNAME];
    char bufnew7[MAXOBJECTNAME];
    char bufnew8[1001];
    char extradata[300]; 
    char selectstatement[150]; 
    char rt1[9], rt2[9], rt3[9], rt4[9], rt5[9],rt6[9],rt7[9],rt8[9],rt9[9];
    char rt10[9],rt11[9],rt12[9],rt13[9],rt14[9],rt15[9],rt16[9],rt17[9],rt18[9];
    int intno,  intno2, intno3, intno4, intno5 ,intno6;
    int intno7, intno8, intno9, intno10;
    char icursess[33];
    short null1,null2,null3,null4,null5;
    short snullind1,snullind2;
    char * pTextSegment;
    int session_serverpid;
  exec sql end declare section;
    BOOL bIceRequest=FALSE;
    ICE_STATUS      status = 0;
    LPUCHAR pret[20];
    ICEBUSUNITDOCDATA busdocdata;
    struct VWStorageStruct {
	LPUCHAR lpVal;
	int nVal;
    } VWStorageStruct[] = {
	{"vectorwise", IDX_VW},
	{"VECTORWISE", IDX_VW},
	{"vectorwise_ix", IDX_VWIX},
	{"VECTORWISE_IX", IDX_VWIX},
	{"\0", 0}};

  bsuppspace= FALSE;

  if (!staticplldbadata) {
    staticplldbadata=(struct lldbadata *)
                     ESL_AllocMem(sizeof(struct lldbadata));
    if (!staticplldbadata) {
      myerror(ERR_ALLOCMEM);
      return RES_ERR;
    }
  }
  if (level>0) {
    if (!parentstrings)
      return RES_ERR;
    for (itmp=0;itmp<level;itmp++) {
      if (!parentstrings[itmp])
        return RES_ERR;
    }
  }
  bHasDot=FALSE;
  if (level>1) {
    pctemp = GetSchemaDot(parentstrings[1]);
    //pctemp =strchr(parentstrings[1],'.');
    if (pctemp) {
      fstrncpy(ParentWhenDot,pctemp+1,sizeof(ParentWhenDot));
      fstrncpy(ParentOwnerWhenDot,parentstrings[1],1+(pctemp-(char *)(parentstrings[1])));
      bHasDot=TRUE;
    }
    else 
      bHasDot=FALSE;
  }
  bIceRequest=FALSE;

  switch (iobjecttype) {
    case OT_MON_SERVER         :
    case OT_MON_SESSION        :
    case OT_MON_LOCKLIST       :
    case OT_MON_LOCKLIST_LOCK  :
    case OT_MON_LOCK           :
    case OTLL_MON_RESOURCE     :
    case OT_MON_RES_LOCK       :
    case OT_MON_LOGPROCESS     :
    case OT_MON_LOGDATABASE    :
    case OT_MON_TRANSACTION    :
    case OT_MON_ICE_CONNECTED_USER:
    case OT_MON_ICE_TRANSACTION   :
    case OT_MON_ICE_CURSOR        :
    case OT_MON_ICE_ACTIVE_USER   :
    case OT_MON_ICE_FILEINFO      :
    case OT_MON_ICE_DB_CONNECTION :
         sprintf(connectname,"%s::%s",lpVirtNode,"imadb");
         break;
    case OT_MON_REPLIC_SERVER  :
         {
            int hnodetmp = GetVirtNodeHandle(lpVirtNode);
            sprintf(connectname,
                    "%s::%s",
                    lpVirtNode,
                    GetMonInfoName(hnodetmp, OT_DATABASE, parentstrings, singlename,sizeof(singlename)-1)
                   );
         }
         break;
    case OT_MON_REPLIC_CDDS_ASSIGN:
        {
            LPREPLICSERVERDATAMIN lp = (LPREPLICSERVERDATAMIN)parentstrings;
            sprintf(connectname,"%s::%s",lp->LocalDBNode, lp->LocalDBName);
            break;
        }

    case OT_ICE_BUNIT_FACET_ROLE     :
    case OT_ICE_BUNIT_FACET_USER     :
    case OT_ICE_BUNIT_PAGE_ROLE      :
    case OT_ICE_BUNIT_PAGE_USER      :
		{
			char bufx[100];
			char * pc;
			int itype1;
			switch (iobjecttype) {
				case OT_ICE_BUNIT_FACET_ROLE :
				case OT_ICE_BUNIT_FACET_USER :
					itype1 = OT_ICE_BUNIT_FACET;
					break;
				case OT_ICE_BUNIT_PAGE_ROLE  :
				case OT_ICE_BUNIT_PAGE_USER  :
					itype1 = OT_ICE_BUNIT_PAGE;
					break;
				default:
					return RES_ERR;
					break;
			}
			x_strcpy(bufx,parentstrings[1]);
			pc = x_strchr(bufx,'.');
			if (pc) {
				*pc='\0';
				x_strcpy(busdocdata.suffix,pc+1);
			}
			else 
				x_strcpy(busdocdata.suffix,"");
			x_strcpy(busdocdata.name,bufx);
			x_strcpy(busdocdata.icebusunit.Name,parentstrings[0]);
			iret=GetICEInfo(lpVirtNode,itype1, &busdocdata);
			if(iret!=RES_SUCCESS)
				return iret;
			bIceRequest=TRUE;
		}
		break;

    case OT_ICE_ROLE                 :
    case OT_ICE_DBUSER               :
    case OT_ICE_DBCONNECTION         :
    case OT_ICE_WEBUSER              :
    case OT_ICE_WEBUSER_ROLE         :
    case OT_ICE_WEBUSER_CONNECTION   :
    case OT_ICE_PROFILE              :
    case OT_ICE_PROFILE_ROLE         :
    case OT_ICE_PROFILE_CONNECTION   :
    case OT_ICE_BUNIT                :
    case OT_ICE_BUNIT_SEC_ROLE       :
    case OT_ICE_BUNIT_SEC_USER       :
    case OT_ICE_BUNIT_FACET          :
    case OT_ICE_BUNIT_PAGE           :
    case OT_ICE_BUNIT_LOCATION       :
    case OT_ICE_SERVER_APPLICATION   :
    case OT_ICE_SERVER_LOCATION      :
    case OT_ICE_SERVER_VARIABLE      :
#ifdef NOUSEICEAPI
         sprintf(connectname,"%s::%s",lpVirtNode,"icesvr");
#else
	     bIceRequest=TRUE;
#endif
         break;
    default:
      if (level>0 && iobjecttype!=OT_GROUPUSER && iobjecttype!=OT_ROLEGRANT_USER){
         fstrncpy(singlename,parentstrings[0],sizeof(singlename));
         suppspace(singlename);
         sprintf(connectname,"%s::%s",lpVirtNode,singlename);
      }
      else 
         sprintf(connectname,"%s::%s",lpVirtNode,GlobalInfoDBName());
      break;
  }
  if (bIceRequest) {
#ifndef NOUSEICEAPI
	if (GetICeUserAndPassWord(GetVirtNodeHandle(lpVirtNode),bufnew1,bufnew2)!=RES_SUCCESS)
		return RES_ERR;
    if ((status =ICE_C_Initialize ()) != OK) {
		if ((status =ICE_C_Initialize ()) != OK) {
			ManageIceError(&ICEclient, &status);
			return RES_ERR;
		}
	}
    if ((status = ICE_C_Connect (CAPINodeName(lpVirtNode), bufnew1,bufnew2, &ICEclient)) != OK) {
		ManageIceError(&ICEclient, &status);
		return RES_ERR;
	}
#endif
  }
  else {
     iret = Getsession(connectname,SESSION_TYPE_CACHENOREADLOCK, &ilocsession);
     if (iret !=RES_SUCCESS) {
       return RES_ERR;
     //  return iret; some values were not managed at higher level
     }
  }

  iresselect = RES_SUCCESS;
  i=0;
  pcur=staticplldbadata;

  iDBATypemem = iobjecttype;

  switch (iobjecttype) {
    case OTLL_GRANTEE:
        if (bWithSystem)
          x_strcpy(selectstatement,"1=1");
        else
          x_strcpy(selectstatement," not object_owner = '$ingres'");
        exec sql select object_name, object_owner,object_type,permit_user,
                         text_segment
                  into   :singlename,:singleownername,:rt1,:name3,
                         :extradata
                  from iipermits
                  where :selectstatement
                  order by object_name, object_owner, permit_user;
        exec sql begin;
          ManageSqlError(&sqlca); // Keep track of the SQL error if any
			if (HasSelectLoopInterruptBeenRequested()) {
				iresselect = RES_ERR;
				exec sql endselect;
			}
          if (isempty(name3))
            fstrncpy(name3,lppublicdispstring(),MAXOBJECTNAME);
          suppspace(singlename);
          suppspace(singleownername);
          fstrncpy(pcur->Name,(LPTSTR)Quote4DisplayIfNeeded(singlename),MAXOBJECTNAME);
          fstrncpy(pcur->OwnerName,singleownername,MAXOBJECTNAME);
          fstrncpy(pcur->ExtraData,name3,MAXOBJECTNAME);
          suppspace(pcur->ExtraData);
          storeint(pcur->ExtraData+MAXOBJECTNAME,GetType(rt1));
          storeint(pcur->ExtraData+MAXOBJECTNAME+STEPSMALLOBJ,
                   ParsePermitText(GetType(rt1), extradata));
          pcur=GetNextLLData(pcur);
          if (!pcur) {
            iresselect = RES_ERR;
            exec sql endselect;
            ManageSqlError(&sqlca); // Keep track of the SQL error if any
          }
          i++;
        exec sql end;
        ManageSqlError(&sqlca); // Keep track of the SQL error if any
      break;
    case OT_MON_ICE_CONNECTED_USER:
      {
        LPSERVERDATAMIN lp = (LPSERVERDATAMIN) parentstrings;

        exec sql repeated select dbmsinfo('ima_vnode') into :extradata;
        ManageSqlError(&sqlca); // Keep track of the SQL error if any
      
        wsprintf(singleownername,"%s::/@%s",extradata,lp->listen_address);
        suppspace(singleownername);


          exec sql repeated select server, name   ,ice_user,req_count,timeout
                            into   :name3,:bufnew1,:bufnew2,:intno   ,:intno2

                            from ice_users;
          exec sql begin;
          ManageSqlError(&sqlca); // Keep track of the SQL error if any
			if (HasSelectLoopInterruptBeenRequested()) {
				iresselect = RES_ERR;
				exec sql endselect;
			}
          pcur->uData.IceConnectedUserDta.svrdata= *lp; 
          suppspace(name3);
          suppspace(bufnew1);
          suppspace(bufnew2);
          fstrncpy(pcur->uData.IceConnectedUserDta.name, bufnew1,
                                    sizeof(pcur->uData.IceConnectedUserDta.name));
          fstrncpy(pcur->uData.IceConnectedUserDta.inguser, bufnew2,
                                    sizeof(pcur->uData.IceConnectedUserDta.inguser));
          pcur->uData.IceConnectedUserDta.req_count = intno;
          pcur->uData.IceConnectedUserDta.timeout   = intno2;

          pcur=GetNextLLData(pcur);
          if (!pcur) {
            iresselect = RES_ERR;
            exec sql endselect;
            ManageSqlError(&sqlca); // Keep track of the SQL error if any
          }
          i++;
          exec sql end;
        ManageSqlError(&sqlca); // Keep track of the SQL error if any
      }
      break;
    case OT_MON_ICE_ACTIVE_USER   :
      {
        LPSERVERDATAMIN lp = (LPSERVERDATAMIN) parentstrings;

        exec sql repeated select dbmsinfo('ima_vnode') into :extradata;
        ManageSqlError(&sqlca); // Keep track of the SQL error if any
      
        wsprintf(singleownername,"%s::/@%s",extradata,lp->listen_address);
        suppspace(singleownername);

          exec sql repeated select server, name   ,host    ,query   ,err_count
                            into   :name3,:bufnew1,:bufnew2,:bufnew3,:intno

                            from ice_active_users;
          exec sql begin;
          ManageSqlError(&sqlca); // Keep track of the SQL error if any
			if (HasSelectLoopInterruptBeenRequested()) {
				iresselect = RES_ERR;
				exec sql endselect;
			}
/*          pcur->uData.IceConnectedUserDta.sesssvrdata= *lp; */
          suppspace(name3);
          suppspace(bufnew1);
          suppspace(bufnew2);
          suppspace(bufnew3);
          fstrncpy(pcur->uData.IceActiveUserDta.name, bufnew1,
                                    sizeof(pcur->uData.IceActiveUserDta.name));
          fstrncpy(pcur->uData.IceActiveUserDta.host, bufnew2,
                                    sizeof(pcur->uData.IceActiveUserDta.host));
          fstrncpy(pcur->uData.IceActiveUserDta.query, bufnew3,
                                    sizeof(pcur->uData.IceActiveUserDta.query));
          pcur->uData.IceActiveUserDta.count = intno;

          pcur=GetNextLLData(pcur);
          if (!pcur) {
            iresselect = RES_ERR;
            exec sql endselect;
            ManageSqlError(&sqlca); // Keep track of the SQL error if any
          }
          i++;
          exec sql end;
        ManageSqlError(&sqlca); // Keep track of the SQL error if any
      }
      break;
    case OT_MON_ICE_TRANSACTION   :
      {
        LPSERVERDATAMIN lp = (LPSERVERDATAMIN) parentstrings;
 
        exec sql repeated select dbmsinfo('ima_vnode') into :extradata;
        ManageSqlError(&sqlca); // Keep track of the SQL error if any
      
        wsprintf(singleownername,"%s::/@%s",extradata,lp->listen_address);
        suppspace(singleownername);

          exec sql repeated select server,trans_key,name    ,trn_session ,owner
                            into   :name3,:bufnew1 ,:bufnew2,:bufnew3,:bufnew4
 
                            from ice_user_transactions;
          exec sql begin;
          ManageSqlError(&sqlca); // Keep track of the SQL error if any
			if (HasSelectLoopInterruptBeenRequested()) {
				iresselect = RES_ERR;
				exec sql endselect;
			}
          pcur->uData.IceTransactionDta.svrdata= *lp; 
          suppspace(name3);
          suppspace(bufnew1);
          suppspace(bufnew2);
          suppspace(bufnew3);
          suppspace(bufnew4);
          fstrncpy(pcur->uData.IceTransactionDta.trans_key, bufnew1,
                                    sizeof(pcur->uData.IceTransactionDta.trans_key));
          fstrncpy(pcur->uData.IceTransactionDta.name, bufnew2,
                                    sizeof(pcur->uData.IceTransactionDta.name));
          fstrncpy(pcur->uData.IceTransactionDta.connect, bufnew3,
                                    sizeof(pcur->uData.IceTransactionDta.connect));
          fstrncpy(pcur->uData.IceTransactionDta.owner, bufnew4,
                                    sizeof(pcur->uData.IceTransactionDta.owner));

          pcur=GetNextLLData(pcur);
          if (!pcur) {
            iresselect = RES_ERR;
            exec sql endselect;
            ManageSqlError(&sqlca); // Keep track of the SQL error if any
          }
          i++;
          exec sql end;
        ManageSqlError(&sqlca); // Keep track of the SQL error if any
      }
      break;
    case OT_MON_ICE_CURSOR        :
      {
        LPSERVERDATAMIN lp = (LPSERVERDATAMIN) parentstrings;
 
        exec sql repeated select dbmsinfo('ima_vnode') into :extradata;
        ManageSqlError(&sqlca); // Keep track of the SQL error if any
      
        wsprintf(singleownername,"%s::/@%s",extradata,lp->listen_address);
        suppspace(singleownername);

          exec sql repeated select server,curs_key ,name    ,query   ,owner
                            into   :name3,:bufnew1 ,:bufnew2,:bufnew3,:bufnew4
 
                            from ice_user_cursors;
          exec sql begin;
          ManageSqlError(&sqlca); // Keep track of the SQL error if any
			if (HasSelectLoopInterruptBeenRequested()) {
				iresselect = RES_ERR;
				exec sql endselect;
			}
/*          pcur->uData.IceConnectedUserDta.sesssvrdata= *lp; */
          suppspace(name3);
          suppspace(bufnew1);
          suppspace(bufnew2);
          suppspace(bufnew3);
          suppspace(bufnew4);
          fstrncpy(pcur->uData.IceCursorDta.curs_key, bufnew1,
                                    sizeof(pcur->uData.IceCursorDta.curs_key));
          fstrncpy(pcur->uData.IceCursorDta.name, bufnew2,
                                    sizeof(pcur->uData.IceCursorDta.name));
          fstrncpy(pcur->uData.IceCursorDta.query, bufnew3,
                                    sizeof(pcur->uData.IceCursorDta.query));
          fstrncpy(pcur->uData.IceCursorDta.owner, bufnew4,
                                    sizeof(pcur->uData.IceCursorDta.owner));

          pcur=GetNextLLData(pcur);
          if (!pcur) {
            iresselect = RES_ERR;
            exec sql endselect;
            ManageSqlError(&sqlca); // Keep track of the SQL error if any
          }
          i++;
          exec sql end;
        ManageSqlError(&sqlca); // Keep track of the SQL error if any
      }
      break;
    case OT_MON_ICE_FILEINFO      :
      {
        LPSERVERDATAMIN lp = (LPSERVERDATAMIN) parentstrings;
 
        exec sql repeated select dbmsinfo('ima_vnode') into :extradata;
        ManageSqlError(&sqlca); // Keep track of the SQL error if any
      
        wsprintf(singleownername,"%s::/@%s",extradata,lp->listen_address);
        suppspace(singleownername);

          exec sql repeated select server ,cache_index ,name    ,loc_name   ,status,
                                   counter,exist       ,owner   ,timeout    ,in_use,
                                   req_count
                            into   :name3 ,:bufnew1    ,:bufnew2,:bufnew3   ,:bufnew4,
                                   :intno ,:intno2     ,:bufnew5,:intno3    ,:intno4,
                                   :intno5  
                            from ice_cache;
          exec sql begin;
          ManageSqlError(&sqlca); // Keep track of the SQL error if any
			if (HasSelectLoopInterruptBeenRequested()) {
				iresselect = RES_ERR;
				exec sql endselect;
			}
          pcur->uData.IceCacheInfoDta.svrdata= *lp; 
          suppspace(name3);
          suppspace(bufnew1);
          suppspace(bufnew2);
          suppspace(bufnew3);
          suppspace(bufnew4);
          suppspace(bufnew5);
          fstrncpy(pcur->uData.IceCacheInfoDta.cache_index, bufnew1,
                                    sizeof(pcur->uData.IceCacheInfoDta.cache_index));
          fstrncpy(pcur->uData.IceCacheInfoDta.name, bufnew2,
                                    sizeof(pcur->uData.IceCacheInfoDta.name));
          fstrncpy(pcur->uData.IceCacheInfoDta.loc_name, bufnew3,
                                    sizeof(pcur->uData.IceCacheInfoDta.loc_name));
          fstrncpy(pcur->uData.IceCacheInfoDta.status, bufnew4,
                                    sizeof(pcur->uData.IceCacheInfoDta.status));
          fstrncpy(pcur->uData.IceCacheInfoDta.owner, bufnew5,
                                    sizeof(pcur->uData.IceCacheInfoDta.owner));
          pcur->uData.IceCacheInfoDta.counter   = intno;
          pcur->uData.IceCacheInfoDta.iexist    = intno2;
          pcur->uData.IceCacheInfoDta.itimeout  = intno3;
          pcur->uData.IceCacheInfoDta.in_use    = intno4;
          pcur->uData.IceCacheInfoDta.req_count = intno5;
          pcur=GetNextLLData(pcur);
          if (!pcur) {
            iresselect = RES_ERR;
            exec sql endselect;
            ManageSqlError(&sqlca); // Keep track of the SQL error if any
          }
          i++;
          exec sql end;
        ManageSqlError(&sqlca); // Keep track of the SQL error if any
      }
      break;
    case OT_MON_ICE_DB_CONNECTION :
      {
        LPSERVERDATAMIN lp = (LPSERVERDATAMIN) parentstrings;
 
        exec sql repeated select dbmsinfo('ima_vnode') into :extradata;
        ManageSqlError(&sqlca); // Keep track of the SQL error if any
      
        wsprintf(singleownername,"%s::/@%s",extradata,lp->listen_address);
        suppspace(singleownername);

          exec sql repeated select server,db_key  ,driver,name    ,use_count, timeout
                            into   :name3,:bufnew1,:intno,:bufnew2,:intno2  ,:intno3
                            from ice_db_connections;
          exec sql begin;
          ManageSqlError(&sqlca); // Keep track of the SQL error if any
			if (HasSelectLoopInterruptBeenRequested()) {
				iresselect = RES_ERR;
				exec sql endselect;
			}
/*          pcur->uData.IceConnectedUserDta.sesssvrdata= *lp; */
          suppspace(name3);
          suppspace(bufnew1);
          suppspace(bufnew2);
          fstrncpy(pcur->uData.IceDbConnectionDta.sessionkey, bufnew1,
                                    sizeof(pcur->uData.IceDbConnectionDta.sessionkey));
          fstrncpy(pcur->uData.IceDbConnectionDta.dbname, bufnew2,
                                    sizeof(pcur->uData.IceDbConnectionDta.dbname));
          pcur->uData.IceDbConnectionDta.driver   = intno;
          pcur->uData.IceDbConnectionDta.iused    = intno2;
          pcur->uData.IceDbConnectionDta.itimeout = intno3;
          pcur=GetNextLLData(pcur);
          if (!pcur) {
            iresselect = RES_ERR;
            exec sql endselect;
            ManageSqlError(&sqlca); // Keep track of the SQL error if any
          }
          i++;
          exec sql end;
        ManageSqlError(&sqlca); // Keep track of the SQL error if any
      }
      break;
    case OT_MON_SERVER :
      {

//        exec sql execute procedure ima_set_server_domain;
//        ManageSqlError(&sqlca); // Keep track of the SQL error if any

//        exec sql execute procedure ima_set_vnode_domain;
//        ManageSqlError(&sqlca); // Keep track of the SQL error if any

        exec sql repeated select dbmsinfo('ima_vnode') into :extradata;
        ManageSqlError(&sqlca); // Keep track of the SQL error if any

		suppspace(extradata);
		x_strcat(extradata,"::/@");

        exec sql select a.name_server  , a.listen_address  , a.server_class,
                        a.server_dblist,b.listen_state, c.server_pid, c.capabilities
                 into   :singlename  , :singleownername, :name3      ,
                        :bufnew1  ,    :bufnew2:null1  , :intno:null2, :bufnew3:null3
                 from ima_gcn_registrations a left join 
                      (ima_dbms_server_status b left join ima_dbms_server_parameters c on trim(b.server) = trim(c.server))
                      on trim(b.server) = trim(concat(:extradata,a.listen_address));
				     
        exec sql begin;
          ManageSqlError(&sqlca); // Keep track of the SQL error if any
			if (HasSelectLoopInterruptBeenRequested()) {
				iresselect = RES_ERR;
				exec sql endselect;
			}
          fstrncpy(pcur->uData.SvrData.name_server   ,singlename     ,
                                          sizeof(pcur->uData.SvrData.name_server));
          fstrncpy(pcur->uData.SvrData.listen_address,singleownername,
                                          sizeof(pcur->uData.SvrData.listen_address));
          fstrncpy(pcur->uData.SvrData.server_class  ,name3          ,
                                          sizeof(pcur->uData.SvrData.server_class));
          fstrncpy(pcur->uData.SvrData.server_dblist,bufnew1        ,
                                          sizeof(pcur->uData.SvrData.server_dblist));
          if (null1==-1)
             x_strcpy(bufnew2,"");
          if (null2 == -1) {
             pcur->uData.SvrData.bHasServerPid = FALSE;
             pcur->uData.SvrData.serverPid = -1L;
          }
          else {
             pcur->uData.SvrData.bHasServerPid = TRUE;
             pcur->uData.SvrData.serverPid =intno;
          }
          fstrncpy(pcur->uData.SvrData.listen_state,bufnew2        ,
                                          sizeof(pcur->uData.SvrData.listen_state));
          if (null3==-1)
             x_strcpy(bufnew3,"");



          pcur->uData.SvrData.servertype = FindClassServerType(name3,bufnew3);

          pcur->uData.SvrData.bMultipleWithSameName = FALSE;

          pcur=GetNextLLData(pcur);
          if (!pcur) {
            iresselect = RES_ERR;
            exec sql endselect;
            ManageSqlError(&sqlca); // Keep track of the SQL error if any
          }
          i++;
        exec sql end;
        ManageSqlError(&sqlca); // Keep track of the SQL error if any
      }
      break;
    case OT_MON_SESSION:
      {
        LPSERVERDATAMIN lp = (LPSERVERDATAMIN) parentstrings;

        exec sql repeated select dbmsinfo('ima_vnode') into :extradata;
        ManageSqlError(&sqlca); // Keep track of the SQL error if any
      
        wsprintf(singleownername,"%s::/@%s",extradata,lp->listen_address);
        suppspace(singleownername);

//        exec sql execute procedure ima_set_server_domain;
//        ManageSqlError(&sqlca); // Keep track of the SQL error if any
      
//        exec sql commit;
//        ManageSqlError(&sqlca); // Keep track of the SQL error if any
      
//        exec sql execute procedure ima_set_vnode_domain;
//        ManageSqlError(&sqlca); // Keep track of the SQL error if any
      
//        exec sql commit;
//        ManageSqlError(&sqlca); // Keep track of the SQL error if any

          exec sql repeated select a.session_id      , a.db_lock           , a.db_name         ,
                        a.server_facility , a.session_query     , a.session_terminal,
                        a.client_pid      , a.server_pid        , a.effective_user  ,
                        a.real_user       , b.session_state_num , b.session_state   ,
                        a.application_code, a.session_name      , b.session_wait_reason
                   into
                        :name3            , :bufev1             , :bufev2     ,
                        :rt1              , :bufnew8            , :bufnew1    ,
                        :bufnew2          , :intno              , :bufnew4    ,
                        :bufnew5          , :intno2             , :selectstatement,
                        :intno3           , :bufnew6            , :bufnew7
                 from
                            ima_server_sessions a,
                            ima_server_sessions_extra b
                   where
                            trim(a.server) = :singleownername and
                            trim(b.server) = :singleownername and
                          a.session_id = b.session_id;
          exec sql begin;
          ManageSqlError(&sqlca); // Keep track of the SQL error if any
			if (HasSelectLoopInterruptBeenRequested()) {
				iresselect = RES_ERR;
				exec sql endselect;
			}
          pcur->uData.SessionData.sesssvrdata= *lp;

          if (x_strncmp(bufnew7,"00",2) == 0)
            x_strcpy(pcur->uData.SessionData.session_wait_reason,"COM");
          else
            fstrncpy(pcur->uData.SessionData.session_wait_reason, bufnew7,
                                    sizeof(pcur->uData.SessionData.session_wait_reason));

          fstrncpy(pcur->uData.SessionData.session_name, bufnew6,
                                    sizeof(pcur->uData.SessionData.session_name));
        
          fstrncpy(pcur->uData.SessionData.session_id, name3,
                                    sizeof(pcur->uData.SessionData.session_id));

          suppspace(bufev2);
                if (x_strncmp(bufev2,"<no_",4) == 0)
            wsprintf(pcur->uData.SessionData.db_name,ResourceString(IDS_NONE_));
                else 
            fstrncpy(pcur->uData.SessionData.db_name, bufev2,
                                        sizeof(pcur->uData.SessionData.db_name));
                    
          fstrncpy(pcur->uData.SessionData.server_facility, rt1,
                                        sizeof(pcur->uData.SessionData.server_facility));

          fstrncpy(pcur->uData.SessionData.session_query, bufnew8,
                                      sizeof(pcur->uData.SessionData.session_query));

          suppspace(bufnew1);
          if (bufnew1[0] == '\0' || x_stricmp(bufnew1,"NONE") == 0)
            x_strcpy(pcur->uData.SessionData.session_terminal,ResourceString(IDS_NONE_));
          else
            fstrncpy(pcur->uData.SessionData.session_terminal, bufnew1,
                                        sizeof(pcur->uData.SessionData.session_terminal));

          pcur->uData.SessionData.server_pid=intno;
        
          fstrncpy(pcur->uData.SessionData.client_pid, bufnew2,
                                        sizeof(pcur->uData.SessionData.client_pid));
          suppspace(bufnew4);
          suppspace(bufnew5);
          fstrncpy(pcur->uData.SessionData.effective_user, bufnew4,
                                        sizeof(pcur->uData.SessionData.effective_user));
          fstrncpy(pcur->uData.SessionData.real_user, bufnew5,
                                        sizeof(pcur->uData.SessionData.real_user));
          pcur->uData.SessionData.session_state_num=intno2;
          fstrncpy(pcur->uData.SessionData.session_state, selectstatement,
                                        sizeof(pcur->uData.SessionData.session_state));
          pcur->uData.SessionData.application_code=intno3;
          pcur->uData.SessionData.bMultipleWithSameName = FALSE;

          pcur=GetNextLLData(pcur);
          if (!pcur) {
            iresselect = RES_ERR;
            exec sql endselect;
            ManageSqlError(&sqlca); // Keep track of the SQL error if any
          }
          i++;
          exec sql end;
        ManageSqlError(&sqlca); // Keep track of the SQL error if any
      }
      break;
    case OT_MON_LOCKLIST:
      {
//        exec sql execute procedure ima_set_vnode_domain;
//        ManageSqlError(&sqlca); // Keep track of the SQL error if any
      
//        exec sql commit;
//        ManageSqlError(&sqlca); // Keep track of the SQL error if any

        /* Bug 96991, use STcopy to move value to structure member,
		** and remove cast to int4 from where clause in following select
		*/
		STcopy(session[ilocsession].sessionid, icursess);
		session_serverpid = session[ilocsession].session_serverpid;
        exec sql repeated select locklist_id         ,locklist_logical_count ,
                        locklist_status     ,locklist_lock_count,
                        locklist_max_locks  ,locklist_name0     ,
                        locklist_name1      ,locklist_server_pid,
                        locklist_session_id ,
                        locklist_wait_id    ,locklist_related_llb
                  into  :intno              ,:intno2            ,
                        :bufev1             ,:intno3            ,
                        :intno4             ,:intno5            ,
                        :intno6             ,:intno7            ,
                        :bufev2             ,
                        :intno9             ,:intno10
                  from
                        ima_locklists
                  where not (locklist_session_id = :icursess AND locklist_server_pid=:session_serverpid);
        exec sql begin;
          ManageSqlError(&sqlca); // Keep track of the SQL error if any
			if (HasSelectLoopInterruptBeenRequested()) {
				iresselect = RES_ERR;
				exec sql endselect;
			}
          pcur->uData.LockListData.locklist_id            = intno;
          pcur->uData.LockListData.locklist_logical_count = intno2;  // logical count
          pcur->uData.LockListData.locklist_lock_count    = intno3;  // lock count
          pcur->uData.LockListData.locklist_max_locks     = intno4;
          pcur->uData.LockListData.locklist_tx_high       = intno5;
          pcur->uData.LockListData.locklist_tx_low        = intno6;
          pcur->uData.LockListData.locklist_server_pid    = intno7;
          pcur->uData.LockListData.locklist_wait_id       = intno9;
          pcur->uData.LockListData.locklist_related_llb   = intno10;
          pcur->uData.LockListData.bIs4AllLockLists       = FALSE;

          suppspace(bufev1);
          if (bufev1[0] == '\0' || x_stricmp(bufev1,"NONE") == 0)
            x_strcpy(bufev1,ResourceString(IDS_NONE_));
          fstrncpy(pcur->uData.LockListData.locklist_status, bufev1,
                            sizeof(pcur->uData.LockListData.locklist_status));
          fstrncpy(pcur->uData.LockListData.locklist_session_id, bufev2,
                            sizeof(pcur->uData.LockListData.locklist_session_id));
        
          pcur=GetNextLLData(pcur);
          if (!pcur) {
            iresselect = RES_ERR;
            exec sql endselect;
            ManageSqlError(&sqlca); // Keep track of the SQL error if any
          }
          i++;
          exec sql end;
      }
      ManageSqlError(&sqlca); // Keep track of the SQL error if any
      break;
   case OT_MON_LOCKLIST_LOCK:
      {
        LPLOCKLISTDATAMIN lp = (LPLOCKLISTDATAMIN) parentstrings;
        int hnodetmp;

        if (lp->bIs4AllLockLists) 
           x_strcpy(selectstatement,"l.resource_id = r.resource_id");
        else 
          wsprintf(selectstatement,"l.resource_id = r.resource_id and l.locklist_id = %ld",
                   lp->locklist_id);

cont_getlocks:
//        exec sql execute procedure ima_set_vnode_domain;
//        ManageSqlError(&sqlca); // Keep track of the SQL error if any
      
//        exec sql commit;
//        ManageSqlError(&sqlca); // Keep track of the SQL error if any
      
//        exec sql execute procedure ima_set_server_domain;
//        ManageSqlError(&sqlca); // Keep track of the SQL error if any
      
//        exec sql commit;
//        ManageSqlError(&sqlca); // Keep track of the SQL error if any

        hnodetmp = GetVirtNodeHandle(lpVirtNode);
        exec sql select l.lock_id              ,l.lock_request_mode, l.lock_state,
                        r.resource_key         ,r.resource_index_id,
                        r.resource_type        ,r.resource_table_id, r.resource_database_id,
                        r.resource_page_number ,l.resource_id      ,l.locklist_id
                 into   :intno                 ,:rt1               , :name3 ,
                        :singlename            ,:intno8            ,
                        :intno2                ,:intno3            , :intno4,
                        :intno5                ,:intno6            , :intno7
                 from ima_locks l ,ima_resources r 
                 where :selectstatement;
            exec sql begin;
            ManageSqlError(&sqlca); // Keep track of the SQL error if any
			if (HasSelectLoopInterruptBeenRequested()) {
				iresselect = RES_ERR;
				exec sql endselect;
			}
          pcur->uData.LockData.lock_id= intno;
		  suppspace(rt1);
		  suppspace(name3);
          fstrncpy(pcur->uData.LockData.lock_request_mode   , rt1    ,
                                    sizeof(pcur->uData.LockData.lock_request_mode));
          fstrncpy(pcur->uData.LockData.lock_state   , name3    ,
                                    sizeof(pcur->uData.LockData.lock_state));
          fstrncpy(pcur->uData.LockData.resource_key  , singlename   ,
                                    sizeof(pcur->uData.LockData.resource_key));
          pcur->uData.LockData.resource_type          = intno2;
          switch (pcur->uData.LockData.resource_type)  {
              case 1:
                pcur->uData.LockData.locktype = LOCK_TYPE_DB;
                break;
              case 2:
                pcur->uData.LockData.locktype = LOCK_TYPE_TABLE;
                break;
              case 3:
                pcur->uData.LockData.locktype = LOCK_TYPE_PAGE;
                break;
              default :
                pcur->uData.LockData.locktype = LOCK_TYPE_OTHER;
          }

           
          pcur->uData.LockData.resource_table_id      = intno3;
          pcur->uData.LockData.resource_database_id   = intno4;
          pcur->uData.LockData.resource_page_number   = intno5;
          pcur->uData.LockData.resource_id            = intno6;
          pcur->uData.LockData.locklist_id            = intno7;
          pcur->uData.LockData.resource_index_id      = intno8;
          pcur->uData.LockData.hdl                    = hnodetmp;
          FillMonLockDBName(&(pcur->uData.LockData));
          pcur=GetNextLLData(pcur);

          if (!pcur) {
              iresselect = RES_ERR;
              exec sql endselect;
              ManageSqlError(&sqlca); // Keep track of the SQL error if any
          }
          i++;
            exec sql end;
        ManageSqlError(&sqlca); // Keep track of the SQL error if any
      }
    break;
    case OT_MON_LOGPROCESS:
      {
//        exec sql execute procedure ima_set_vnode_domain;
//        ManageSqlError(&sqlca); // Keep track of the SQL error if any
      
//        exec sql commit;
//        ManageSqlError(&sqlca); // Keep track of the SQL error if any
      
//        exec sql execute procedure ima_set_server_domain;
//        ManageSqlError(&sqlca); // Keep track of the SQL error if any
      
//        exec sql commit;
//        ManageSqlError(&sqlca); // Keep track of the SQL error if any
          
        exec sql repeated select log_process_id  ,process_pid      ,process_status_num,
                        process_count   ,process_writes   ,process_log_forces,
                        process_waits   ,process_tx_begins, process_tx_ends,
                        process_status  ,log_id_instance
                 into   :intno          ,:intno2          ,:intno3,
                        :intno4         ,:intno5          ,:intno6,
                        :intno7         ,:intno8          ,:intno9,
                        :extradata      ,:intno10
                   from
                            ima_log_processes;
          exec sql begin;
          ManageSqlError(&sqlca); // Keep track of the SQL error if any
			if (HasSelectLoopInterruptBeenRequested()) {
				iresselect = RES_ERR;
				exec sql endselect;
			}
          pcur->uData.LogProcessData.log_process_id     = (65536 *intno10)+intno;
          pcur->uData.LogProcessData.process_pid        = intno2;
          pcur->uData.LogProcessData.process_status_num = intno3;
          pcur->uData.LogProcessData.process_count      = intno4;
          pcur->uData.LogProcessData.process_writes     = intno5;
          pcur->uData.LogProcessData.process_log_forces = intno6;
          pcur->uData.LogProcessData.process_waits      = intno7;
          pcur->uData.LogProcessData.process_tx_begins  = intno8;
          pcur->uData.LogProcessData.process_tx_ends    = intno9;

          wsprintf(pcur->uData.LogProcessData.LogProcessName,"%8lx",
                                         pcur->uData.LogProcessData.log_process_id);
          decodeStatusVal(pcur->uData.LogProcessData.process_status_num,
                          pcur->uData.LogProcessData.process_status);

          //fstrncpy(pcur->uData.LogProcessData.process_status   , extradata    ,
          //                                sizeof(pcur->uData.LogProcessData.process_status));

          pcur=GetNextLLData(pcur);
          if (!pcur) {
            iresselect = RES_ERR;
            exec sql endselect;
            ManageSqlError(&sqlca); // Keep track of the SQL error if any
          }
          i++;
          exec sql end;
        ManageSqlError(&sqlca); // Keep track of the SQL error if any
      }
      break;
    case OT_MON_LOGDATABASE    :
      {
//        exec sql execute procedure ima_set_vnode_domain;
//        ManageSqlError(&sqlca); // Keep track of the SQL error if any
      
//        exec sql commit;
//        ManageSqlError(&sqlca); // Keep track of the SQL error if any
      
//        exec sql execute procedure ima_set_server_domain;
//        ManageSqlError(&sqlca); // Keep track of the SQL error if any
      
//        exec sql commit;
//        ManageSqlError(&sqlca); // Keep track of the SQL error if any
          exec sql repeated select db_name     , db_status      , db_tx_count,
                        db_tx_begins, db_tx_ends     , db_reads,
                        db_writes
                   into   :singlename ,:selectstatement, :intno,
                            :intno2     ,:intno3         , :intno4,
                            :intno5
                 from
                    ima_log_databases;
          exec sql begin;
          ManageSqlError(&sqlca); // Keep track of the SQL error if any
			if (HasSelectLoopInterruptBeenRequested()) {
				iresselect = RES_ERR;
				exec sql endselect;
			}

          suppspace(singlename);
          fstrncpy(pcur->uData.LogDBdata.db_name, singlename ,
                                      sizeof(pcur->uData.LogDBdata.db_name));
          fstrncpy(pcur->uData.LogDBdata.db_status, selectstatement ,
                                      sizeof(pcur->uData.LogDBdata.db_status));

          pcur->uData.LogDBdata.db_tx_count  = intno;
          pcur->uData.LogDBdata.db_tx_begins = intno2;
          pcur->uData.LogDBdata.db_tx_ends   = intno3;
          pcur->uData.LogDBdata.db_reads     = intno4;
          pcur->uData.LogDBdata.db_writes    = intno5;

          pcur=GetNextLLData(pcur);
          if (!pcur) {
            iresselect = RES_ERR;
            exec sql endselect;
            ManageSqlError(&sqlca); // Keep track of the SQL error if any
          }
          i++;
          exec sql end;
        ManageSqlError(&sqlca); // Keep track of the SQL error if any
      }
      break;
    case OT_MON_TRANSACTION:
      {
//        exec sql execute procedure ima_set_server_domain;
//        ManageSqlError(&sqlca); // Keep track of the SQL error if any
      
//        exec sql commit;
//        ManageSqlError(&sqlca); // Keep track of the SQL error if any
      
//        exec sql execute procedure ima_set_vnode_domain;
//        ManageSqlError(&sqlca); // Keep track of the SQL error if any
         
//        exec sql commit;
//        ManageSqlError(&sqlca); // Keep track of the SQL error if any

        /* Bug 96991, use STcopy to move value to structure member,
		** and remove cast to int4 from where clause in following select
		*/
		STcopy(session[ilocsession].sessionid, icursess);
		session_serverpid = session[ilocsession].session_serverpid;

         exec sql repeated select tx_id_id          , tx_id_instance      ,
                        tx_db_name        , tx_user_name        ,
                        tx_status         , tx_session_id       ,
                        tx_transaction_id , tx_transaction_high ,
                        tx_transaction_low, tx_state_split      ,
                        tx_state_write    , tx_server_pid
                   into   :intno            , :intno2             ,
                        :bufnew1          , :bufnew2            ,
                        :extradata        , :bufnew3            ,
                        :bufnew4          , :intno4             ,
                        :intno5           , :intno6             ,
                        :intno7           , :intno8
                   from
                        ima_log_transactions
                 where not ( tx_session_id = :icursess AND tx_server_pid=:session_serverpid );

          exec sql begin;
            ManageSqlError(&sqlca); // Keep track of the SQL error if any
			if (HasSelectLoopInterruptBeenRequested()) {
				iresselect = RES_ERR;
				exec sql endselect;
			}

            pcur->uData.LogTransacData.tx_id_id       = intno;
            pcur->uData.LogTransacData.tx_id_instance = intno2;

            pcur->uData.LogTransacData.tx_transaction_high = intno4;
            pcur->uData.LogTransacData.tx_transaction_low  = intno5;
            
            pcur->uData.LogTransacData.tx_state_split = intno6;
            pcur->uData.LogTransacData.tx_state_write = intno7;
			pcur->uData.LogTransacData.tx_server_pid  = intno8;

            fstrncpy(pcur->uData.LogTransacData.tx_transaction_id, bufnew4 ,
                                        sizeof(pcur->uData.LogTransacData.tx_transaction_id));
            suppspace(bufnew1);
            suppspace(bufnew2);
            fstrncpy(pcur->uData.LogTransacData.tx_db_name, bufnew1 ,
                                        sizeof(pcur->uData.LogTransacData.tx_db_name));
            fstrncpy(pcur->uData.LogTransacData.tx_user_name, bufnew2 ,
                                        sizeof(pcur->uData.LogTransacData.tx_user_name));
            fstrncpy(pcur->uData.LogTransacData.tx_status, extradata ,
                                        sizeof(pcur->uData.LogTransacData.tx_status));
            fstrncpy(pcur->uData.LogTransacData.tx_session_id, bufnew3 ,
                                        sizeof(pcur->uData.LogTransacData.tx_session_id));
            // Not in the Table TO BE FINISHED  
                  //pcur->uData.LogTransacData.stat_write = intno;
                  //pcur->uData.LogTransacData.stat_split = intno2;
            pcur=GetNextLLData(pcur);
            if (!pcur) {
              iresselect = RES_ERR;
              exec sql endselect;
              ManageSqlError(&sqlca); // Keep track of the SQL error if any
            }
            i++;
          exec sql end;
        ManageSqlError(&sqlca); // Keep track of the SQL error if any
      }
      break;
    case OTLL_MON_RESOURCE:
      {
        int hnodetmp = GetVirtNodeHandle(lpVirtNode);
        exec sql repeated select resource_id       , resource_grant_mode , resource_convert_mode,
                        resource_type     , resource_database_id,   resource_table_id,
                        resource_index_id , resource_page_number, resource_row_id,
                        resource_key
                 into   :intno            , :bufev1             , :bufev2 ,
                        :intno2           , :intno3             , :intno4 ,
                        :intno5           , :intno6             , :intno7,
                        :singlename
                 from ima_resources;
            exec sql begin;
          ManageSqlError(&sqlca); // Keep track of the SQL error if any
			if (HasSelectLoopInterruptBeenRequested()) {
				iresselect = RES_ERR;
				exec sql endselect;
			}
          fstrncpy(pcur->uData.ResourceData.resource_grant_mode   , bufev1    ,
                                          sizeof(pcur->uData.ResourceData.resource_grant_mode));
          fstrncpy(pcur->uData.ResourceData.resource_convert_mode, bufev2    ,
                                          sizeof(pcur->uData.ResourceData.resource_convert_mode));
          fstrncpy(pcur->uData.ResourceData.resource_key, singlename  ,
                                          sizeof(pcur->uData.ResourceData.resource_key));
          pcur->uData.ResourceData.resource_id           = intno;
          pcur->uData.ResourceData.resource_type         = intno2;
          pcur->uData.ResourceData.resource_database_id  = intno3;
          pcur->uData.ResourceData.resource_table_id     = intno4;
          pcur->uData.ResourceData.resource_index_id     = intno5;
          pcur->uData.ResourceData.resource_page_number  = intno6;
          pcur->uData.ResourceData.resource_row_id       = intno7;
          //pcur->uData.ResourceData.resource_key6         = intno8;

          pcur->uData.ResourceData.hdl                   = hnodetmp;
          FillMonResourceDBName(&(pcur->uData.ResourceData));

          pcur=GetNextLLData(pcur);
          if (!pcur) {
            iresselect = RES_ERR;
            exec sql endselect;
            ManageSqlError(&sqlca); // Keep track of the SQL error if any
          }
          i++;
            exec sql end;

      }
      break;
   case OT_MON_RES_LOCK:
      {
        LPRESOURCEDATAMIN lp = (LPRESOURCEDATAMIN) parentstrings;

        wsprintf(selectstatement,"l.resource_id = r.resource_id and resource_id = %ld",
                   lp->resource_id);
        goto cont_getlocks;
      }
    break;
    case OT_MON_REPLIC_SERVER  :
      {
        LPRESOURCEDATAMIN lpdb = (LPRESOURCEDATAMIN) parentstrings;
        int hnodetmp = GetVirtNodeHandle(lpVirtNode);

        if ( CreateDD_SERVER_SPECIALIfNotExist () == RES_ERR)
            break;

        //exec sql select count(*) into :intno from iitables where table_name = 'dd_server_special';
        //if (intno == 0) {
        //    exec sql create table dd_server_special (localdb smallint not null , server_no smallint not null ,vnode_name char (32) not null);
        //    if (sqlca.sqlcode < 0)  { 
        //        ManageSqlError(&sqlca); // Keep track of the SQL error if any
        //        break;
        //    }
        //}
        exec sql commit;
        exec sql repeated select  d.vnode_name, d.database_name, c.vnode_name,   b.server_no, a.localdb
                 into    :bufnew1,     :bufnew2,        :bufnew3:null1, :intno,      :intno2
                 from    ( (dd_paths a left join dd_db_cdds b on a.targetdb=b.database_no and a.cdds_no=b.cdds_no)
                          left join dd_server_special c 
                          on b.server_no= c.server_no and a.localdb=c.localdb
                         ),dd_databases d
                 where   a.localdb = d.database_no
                 group by d.vnode_name, d.database_name, c.vnode_name,   b.server_no, a.localdb;
    
        exec sql begin;
          ManageSqlError(&sqlca); // Keep track of the SQL error if any
			if (HasSelectLoopInterruptBeenRequested()) {
				iresselect = RES_ERR;
				exec sql endselect;
			}
          fstrncpy(pcur->uData.ReplicSrvDta.ParentDataBaseName,
                   GetMonInfoName(hnodetmp, OT_DATABASE, parentstrings, singlename,sizeof(singlename)-1),
                   sizeof(pcur->uData.ReplicSrvDta.ParentDataBaseName));
          if (null1==-1)
             x_strcpy(bufnew3,bufnew1); 
          suppspace(bufnew1);
          suppspace(bufnew2);
          suppspace(bufnew3);
          fstrncpy(pcur->uData.ReplicSrvDta.LocalDBNode,bufnew1,
                                      sizeof(pcur->uData.ReplicSrvDta.LocalDBNode));
          fstrncpy(pcur->uData.ReplicSrvDta.LocalDBName,bufnew2,
                                      sizeof(pcur->uData.ReplicSrvDta.LocalDBName));
          fstrncpy(pcur->uData.ReplicSrvDta.RunNode,bufnew3,
                                      sizeof(pcur->uData.ReplicSrvDta.RunNode));
          pcur->uData.ReplicSrvDta.serverno     = intno;
          pcur->uData.ReplicSrvDta.localdb      = intno2;
          pcur->uData.ReplicSrvDta.startstatus  = REPSVR_UNKNOWNSTATUS;
          pcur->uData.ReplicSrvDta.DBEhdl       = DBEHDL_NOTASSIGNED;  // managed by background task
          pcur->uData.ReplicSrvDta.FullStatus[0]='\0'; // managed by background task

          pcur=GetNextLLData(pcur);
          if (!pcur) {
            iresselect = RES_ERR;
            exec sql endselect;
            ManageSqlError(&sqlca); // Keep track of the SQL error if any
          }
          i++;
        exec sql end;
        ManageSqlError(&sqlca); // Keep track of the SQL error if any
      }
      break;

    case OT_STARDB_COMPONENT:
        exec sql repeated select ldb_node       , ldb_dbmstype    , ldb_database,
						ldb_object_type, ldb_object_name , ldb_object_owner
                 into   :singlename    , :singleownername, :name3,
                        :bufnew1       , :bufnew2        , :bufnew3   
                 from iiregistered_objects;
        exec sql begin;
          ManageSqlError(&sqlca); // Keep track of the SQL error if any
			if (HasSelectLoopInterruptBeenRequested()) {
				iresselect = RES_ERR;
				exec sql endselect;
			}
		  suppspace(singlename);
		  suppspace(singleownername);
		  suppspace(name3);
		  suppspace(bufnew1);
		  suppspace(bufnew2);
		  suppspace(bufnew3);
          lstrcpy(pcur->uData.StarDbCompDta.NodeName     ,singlename);
          lstrcpy(pcur->uData.StarDbCompDta.ServerClass  ,singleownername);
          lstrcpy(pcur->uData.StarDbCompDta.DBName       ,name3);
          lstrcpy(pcur->uData.StarDbCompDta.ObjectName   ,bufnew2);
          lstrcpy(pcur->uData.StarDbCompDta.ObjectOwner  ,bufnew3);
		  switch (bufnew1[0]) {
			case 'T':case 't': pcur->uData.StarDbCompDta.itype =OT_TABLE    ;break;
			case 'V':case 'v': pcur->uData.StarDbCompDta.itype =OT_VIEW     ;break;
			case 'P':case 'p': pcur->uData.StarDbCompDta.itype =OT_PROCEDURE;break;
			case 'I':case 'i': pcur->uData.StarDbCompDta.itype =OT_INDEX    ;break;
	        default:pcur->uData.StarDbCompDta.itype =OT_UNKNOWN    ;break;
		  }
          pcur=GetNextLLData(pcur);
          if (!pcur) {
            iresselect = RES_ERR;
            exec sql endselect;
            ManageSqlError(&sqlca); // Keep track of the SQL error if any
          }
          i++;
        exec sql end;
        ManageSqlError(&sqlca); // Keep track of the SQL error if any
      
      break;

    case OT_MON_REPLIC_CDDS_ASSIGN:
    {
    LPREPLICSERVERDATAMIN lp = (LPREPLICSERVERDATAMIN)parentstrings;
    intno4 = lp->serverno;
    EXEC SQL repeated SELECT d.database_no,TRIM(db.vnode_name) ,TRIM(db.database_name), 
                    c.cdds_no, TRIM(c.cdds_name), d.target_type
        INTO        :intno       , :bufnew1           , :bufnew2             ,
                    :intno2   , :bufnew3   , :intno3
        FROM  dd_db_cdds d, dd_cdds c, dd_databases db
        WHERE   d.server_no = :intno4
        AND d.database_no = db.database_no
        AND d.cdds_no = c.cdds_no
        AND db.local_db != 1
        ORDER   BY 1, 3;
    EXEC SQL BEGIN;
        ManageSqlError(&sqlca); // Keep track of the SQL error if any
			if (HasSelectLoopInterruptBeenRequested()) {
				iresselect = RES_ERR;
				exec sql endselect;
			}
        pcur->uData.ReplicCddsDta.Database_No = intno;
        fstrncpy(pcur->uData.ReplicCddsDta.szVnodeName,bufnew1, 
                    sizeof(pcur->uData.ReplicCddsDta.szVnodeName));
        fstrncpy(pcur->uData.ReplicCddsDta.szDBName,bufnew2,
                    sizeof(pcur->uData.ReplicCddsDta.szDBName));
        fstrncpy(pcur->uData.ReplicCddsDta.szCddsName,bufnew3,
                    sizeof(pcur->uData.ReplicCddsDta.szCddsName));

        pcur->uData.ReplicCddsDta.Cdds_No = intno2;
        switch (intno3)   {
            case REPLIC_FULLPEER:
                fstrncpy(pcur->uData.ReplicCddsDta.szTargetType,"Full Peer",
                         sizeof(pcur->uData.ReplicCddsDta.szTargetType));
            break ;
            case REPLIC_PROT_RO:
                fstrncpy(pcur->uData.ReplicCddsDta.szTargetType,"Protected Read-only",
                         sizeof(pcur->uData.ReplicCddsDta.szTargetType));
            break;
            case REPLIC_UNPROT_RO:
                fstrncpy(pcur->uData.ReplicCddsDta.szTargetType,"Unprotected Read-only",
                         sizeof(pcur->uData.ReplicCddsDta.szTargetType));
            break;
            default:
                fstrncpy(pcur->uData.ReplicCddsDta.szTargetType,"Unknown type",
                         sizeof(pcur->uData.ReplicCddsDta.szTargetType));
        }
      pcur=GetNextLLData(pcur);
      if (!pcur) {
        iresselect = RES_ERR;
        exec sql endselect;
        ManageSqlError(&sqlca); // Keep track of the SQL error if any
      }
      i++;
    EXEC SQL END;
    ManageSqlError(&sqlca); // Keep track of the SQL error if any
    }
    break;
    case OT_TABLE:
      {
          /* STAR  */
          int iStarType;
          if (IsStarDatabase(GetMainCacheHdl(ilocsession), parentstrings[0])) {
            exec sql repeated select a.table_name,a.table_owner,a.table_indexes,
                            a.table_integrities,a.view_base,a.location_name, a.multi_locations,a.table_subtype,
                            b.ldb_node,b.ldb_dbmstype, b.ldb_object_name
                     into   :singlename,:singleownername,:rt1,
                            :rt2, :rt3, :name3, :rt4, :rt5,
                            :bufnew5:null1, :bufnew6:null2,:bufnew7:null3
                     from   iitables a , iiregistered_objects b 
                     where  a.table_name=b.ddb_object_name
                            and a.table_owner=b.ddb_object_owner
                            and table_type='T' 
                     union 
                     select a.table_name,a.table_owner,a.table_indexes,
                            a.table_integrities,a.view_base,a.location_name, a.multi_locations,a.table_subtype,
                            '','',''
                     from   iitables a  
                     where  table_type='T' and NOT EXISTS (select * from iiregistered_objects b where
                                                           a.table_name=b.ddb_object_name
                                                           and a.table_owner=b.ddb_object_owner
                                                          );
                               
            
            exec sql begin;
              ManageSqlError(&sqlca); // Keep track of the SQL error if any
			if (HasSelectLoopInterruptBeenRequested()) {
				iresselect = RES_ERR;
				exec sql endselect;
			}
              suppspacebutleavefirst(singlename);
              fstrncpy(pcur->Name,(LPTSTR)Quote4DisplayIfNeeded(singlename),MAXOBJECTNAME);
              fstrncpy(pcur->OwnerName,singleownername,MAXOBJECTNAME);
              fstrncpy(pcur->ExtraData,name3,MAXOBJECTNAME);
              suppspace(pcur->ExtraData);
              pcur->ExtraData[MAXOBJECTNAME]  =GetYNState(rt1); // indexes
              pcur->ExtraData[MAXOBJECTNAME+1]=GetYNState(rt2); // integr.
              pcur->ExtraData[MAXOBJECTNAME+2]=GetYNState(rt3); // base_v
              pcur->ExtraData[MAXOBJECTNAME+3]=GetYNState(rt4); // multiloc
              storeint(pcur->ExtraData+MAXOBJECTNAME+4, -1);    // DUMMY ID FOR MONITORING
              iStarType=OBJTYPE_UNKNOWN;
              if (rt5[0]=='N' || rt5[0]=='n')
                iStarType=OBJTYPE_STARNATIVE;
              if (rt5[0]=='L' || rt5[0]=='l')
                iStarType=OBJTYPE_STARLINK;
              storeint(pcur->ExtraData+MAXOBJECTNAME+4+STEPSMALLOBJ, iStarType);  
              pcur=GetNextLLData(pcur);
              if (!pcur) {
                iresselect = RES_ERR;
                exec sql endselect;
                ManageSqlError(&sqlca); // Keep track of the SQL error if any
              }
              i++;
            exec sql end;
            ManageSqlError(&sqlca); // Keep track of the SQL error if any
            break;
          }
          if (GetOIVers() == OIVERS_NOTOI) {
             intno=-1;   // reltid = -1 (dummy if gateway or <Oping 1.x )
             exec sql repeated select table_name,table_owner,table_indexes,
                             table_integrities,view_base,location_name, multi_locations,
			     storage_structure
                        
                    into :singlename,:singleownername,:rt1,
                       :rt2,:rt3,:name3,:rt4, :storage_struct
                    from iitables t
                    where table_type='T';

             exec sql begin;
               ManageSqlError(&sqlca); // Keep track of the SQL error if any
				if (HasSelectLoopInterruptBeenRequested()) {
					iresselect = RES_ERR;
					exec sql endselect;
				}

               ZEROINIT(pcur->ExtraData);
               fstrncpy(pcur->Name,singlename,MAXOBJECTNAME);
               fstrncpy(pcur->OwnerName,singleownername,MAXOBJECTNAME);
               fstrncpy(pcur->ExtraData,name3,MAXOBJECTNAME);
               suppspace(pcur->ExtraData);
               pcur->ExtraData[MAXOBJECTNAME]  =GetYNState(rt1); // indexes
               pcur->ExtraData[MAXOBJECTNAME+1]=GetYNState(rt2); // integr.
               pcur->ExtraData[MAXOBJECTNAME+2]=GetYNState(rt3); // base_v
               pcur->ExtraData[MAXOBJECTNAME+3]=GetYNState(rt4); // multiloc
               storeint(pcur->ExtraData+MAXOBJECTNAME+4,intno); // table id for monitoring
               suppspace(storage_struct);
               for (j=0; VWStorageStruct[j].nVal!=0;j++)
               {
               		if (x_strnicmp(VWStorageStruct[j].lpVal, storage_struct, x_strlen(storage_struct)) == 0)
               		{
               			storeint(pcur->ExtraData+MAXOBJECTNAME+4+STEPSMALLOBJ, VWStorageStruct[j].nVal);
               			break;
               		}
               } 
               storeint(pcur->ExtraData+MAXOBJECTNAME+4+STEPSMALLOBJ+STEPSMALLOBJ, OBJTYPE_NOTSTAR); 
               pcur=GetNextLLData(pcur);
               if (!pcur) {
                 iresselect = RES_ERR;
                 exec sql endselect;
                 ManageSqlError(&sqlca); // Keep track of the SQL error if any
               }
               i++;
             exec sql end;
             ManageSqlError(&sqlca); // Keep track of the SQL error if any
             break;
           }
           exec sql repeated select table_name,table_owner,table_indexes,
                       table_integrities,view_base,location_name, multi_locations,
                       reltid, storage_structure
                    into :singlename,:singleownername,:rt1,
                       :rt2,:rt3,:name3,:rt4,:intno, :storage_struct
                    from iitables t,iirelation r
                    where table_type='T' and t.table_name=r.relid and t.table_owner=r.relowner;

           exec sql begin;
             ManageSqlError(&sqlca); // Keep track of the SQL error if any
             if (HasSelectLoopInterruptBeenRequested()) {
               iresselect = RES_ERR;
               exec sql endselect;
             }
             ZEROINIT(pcur->ExtraData);
             suppspacebutleavefirst(singlename);
             suppspace(singleownername);
             fstrncpy(pcur->Name,(LPTSTR)Quote4DisplayIfNeeded(singlename),MAXOBJECTNAME);
             fstrncpy(pcur->OwnerName,singleownername,MAXOBJECTNAME);
             fstrncpy(pcur->ExtraData,name3,MAXOBJECTNAME);
             suppspace(pcur->ExtraData);
             pcur->ExtraData[MAXOBJECTNAME]  =GetYNState(rt1); // indexes
             pcur->ExtraData[MAXOBJECTNAME+1]=GetYNState(rt2); // integr.
             pcur->ExtraData[MAXOBJECTNAME+2]=GetYNState(rt3); // base_v
             pcur->ExtraData[MAXOBJECTNAME+3]=GetYNState(rt4); // multiloc
             storeint(pcur->ExtraData+MAXOBJECTNAME+4,intno); // table id for monitoring
             suppspace(storage_struct);
             for (j=0; VWStorageStruct[j].nVal!=0;j++)
             {
             	if (x_strnicmp(VWStorageStruct[j].lpVal, storage_struct, x_strlen(storage_struct)) == 0)
             	{
             		storeint(pcur->ExtraData+MAXOBJECTNAME+4+STEPSMALLOBJ, VWStorageStruct[j].nVal);
             		break;
             	}
             } 
             storeint(pcur->ExtraData+MAXOBJECTNAME+4+STEPSMALLOBJ+STEPSMALLOBJ, OBJTYPE_NOTSTAR); 
             pcur=GetNextLLData(pcur);
             if (!pcur) {
               iresselect = RES_ERR;
               exec sql endselect;
               ManageSqlError(&sqlca); // Keep track of the SQL error if any
             }
             i++;
           exec sql end;
           ManageSqlError(&sqlca); // Keep track of the SQL error if any
      }
      break;
    case OT_VIEW:
      {
      if (GetOIVers() == OIVERS_NOTOI)
          bGenericGateway = TRUE;
      if (bGenericGateway) {
          exec sql repeated select table_name,table_owner, view_dml
                    into :singlename, :singleownername,:extradata
                    from iiviews
                    where text_sequence=1; 

           exec sql begin;
             ManageSqlError(&sqlca); // Keep track of the SQL error if any
			if (HasSelectLoopInterruptBeenRequested()) {
				iresselect = RES_ERR;
				exec sql endselect;
			}
             fstrncpy(pcur->Name,singlename,MAXOBJECTNAME);
             fstrncpy(pcur->OwnerName,singleownername,MAXOBJECTNAME);
             //storeint(pcur->ExtraData+4,intno); // table id for monitoring
             pcur->ExtraData[3]=extradata[0];
             storeint(pcur->ExtraData+4+STEPSMALLOBJ, OBJTYPE_NOTSTAR); 
             pcur=GetNextLLData(pcur);
             if (!pcur) {
               iresselect = RES_ERR;
               exec sql endselect;
               ManageSqlError(&sqlca); // Keep track of the SQL error if any
             }
             i++;
           exec sql end;
           ManageSqlError(&sqlca); // Keep track of the SQL error if any

      }
      else {
             /* STAR */
             int iStarType;
             if (IsStarDatabase(GetMainCacheHdl(ilocsession), parentstrings[0])) {
               exec sql repeated select a.table_name,a.table_owner, a.view_dml,b.table_subtype
                        into :singlename,:singleownername, :extradata,:rt5
                        from iiviews a, iitables b
                        where view_dml = 'S' and text_sequence=1  
                              and a.table_name=b.table_name and a.table_owner=b.table_owner; 
           
               exec sql begin;
                 ManageSqlError(&sqlca); // Keep track of the SQL error if any
					if (HasSelectLoopInterruptBeenRequested()) {
						iresselect = RES_ERR;
						exec sql endselect;
					}
                 suppspacebutleavefirst(singlename);
                 fstrncpy(pcur->Name,(LPTSTR)Quote4DisplayIfNeeded(singlename),MAXOBJECTNAME);
                 fstrncpy(pcur->OwnerName,singleownername,MAXOBJECTNAME);
                 storeint(pcur->ExtraData+4, -1);    // DUMMY ID FOR MONITORING
                 pcur->ExtraData[3]=extradata[0];
                 iStarType=OBJTYPE_UNKNOWN;
                 if (rt5[0]=='N' || rt5[0]=='n')
                   iStarType=OBJTYPE_STARNATIVE;
                 if (rt5[0]=='L' || rt5[0]=='l')
                   iStarType=OBJTYPE_STARLINK;
                 storeint(pcur->ExtraData+4+STEPSMALLOBJ, iStarType); 
                 pcur=GetNextLLData(pcur);
                 if (!pcur) {
                   iresselect = RES_ERR;
                   exec sql endselect;
                   ManageSqlError(&sqlca); // Keep track of the SQL error if any
                 }
                 i++;
               exec sql end;
               ManageSqlError(&sqlca); // Keep track of the SQL error if any
               break;
             }
          
              exec sql repeated select table_name,table_owner,     reltid, view_dml
                       into :singlename, :singleownername,:intno, :extradata
                       from iiviews t, iirelation r
                       where text_sequence=1 and t.table_name=r.relid and t.table_owner=r.relowner; 

              exec sql begin;
                ManageSqlError(&sqlca); // Keep track of the SQL error if any
				if (HasSelectLoopInterruptBeenRequested()) {
					iresselect = RES_ERR;
					exec sql endselect;
				}
                suppspace(singlename);
                fstrncpy(pcur->Name,(LPTSTR)Quote4DisplayIfNeeded(singlename),MAXOBJECTNAME);
                fstrncpy(pcur->OwnerName,singleownername,MAXOBJECTNAME);
                storeint(pcur->ExtraData+4,intno); // table id for monitoring
                pcur->ExtraData[3]=extradata[0];
                storeint(pcur->ExtraData+4+STEPSMALLOBJ, OBJTYPE_NOTSTAR); 
                pcur=GetNextLLData(pcur);
                if (!pcur) {
                  iresselect = RES_ERR;
                  exec sql endselect;
                  ManageSqlError(&sqlca); // Keep track of the SQL error if any
                }
                i++;
              exec sql end;
              ManageSqlError(&sqlca); // Keep track of the SQL error if any
           }
      }
      break;
    case OT_DATABASE:
      if (GetOIVers() == OIVERS_NOTOI)
         bGenericGateway = TRUE;

      if (bGenericGateway) {
        singlename[0]='\0';
        exec sql repeated select name
                 into :singlename
                 from iidatabase ;

        exec sql begin;
          ManageSqlError(&sqlca); // Keep track of the SQL error if any
			if (HasSelectLoopInterruptBeenRequested()) {
				iresselect = RES_ERR;
				exec sql endselect;
			}
          fstrncpy(pcur->Name,singlename,MAXOBJECTNAME);
          ZEROINIT (pcur->OwnerName);
          //fstrncpy(pcur->OwnerName,singleownername,MAXOBJECTNAME);
          storeint(pcur->ExtraData,0);
          storeint(pcur->ExtraData+STEPSMALLOBJ,DBTYPE_REGULAR);
          pcur=GetNextLLData(pcur);
          if (!pcur) {
            iresselect = RES_ERR;
            exec sql endselect;
            ManageSqlError(&sqlca); // Keep track of the SQL error if any
          }
          i++;
        exec sql end;
        ManageSqlError(&sqlca); // Keep track of the SQL error if any
        break;
      }
      else {
       exec sql repeated select a.name,   a.own,           a.db_id,a.dbservice,
                        b.cdb_name
                 into   :singlename,:singleownername,:intno,:intno2,
                        :bufnew2:null1
                 from iidatabase a left join iistar_cdbs b on
                        a.name=b.ddb_name ;

        exec sql begin;
          ManageSqlError(&sqlca); // Keep track of the SQL error if any
			if (HasSelectLoopInterruptBeenRequested()) {
				iresselect = RES_ERR;
				exec sql endselect;
			}
		  intno2 &= 3L; // distrib db info in 2 first bits
		  if (intno2!=OBJTYPE_NOTSTAR && intno2!=OBJTYPE_STARNATIVE && intno2!=OBJTYPE_STARLINK)
			intno2=OBJTYPE_NOTSTAR;
          fstrncpy(pcur->Name,singlename,MAXOBJECTNAME);
          fstrncpy(pcur->OwnerName,singleownername,MAXOBJECTNAME);
          storeint(pcur->ExtraData,intno);
          storeint(pcur->ExtraData+STEPSMALLOBJ,intno2);
          if (null1==-1)
             bufnew2[0]='\0';
          suppspace(bufnew2);
          x_strcpy(pcur->ExtraData+2*STEPSMALLOBJ,bufnew2);
          pcur=GetNextLLData(pcur);
          if (!pcur) {
            iresselect = RES_ERR;
            exec sql endselect;
            ManageSqlError(&sqlca); // Keep track of the SQL error if any
          }
          i++;
        exec sql end;
        exec sql commit;
        ManageSqlError(&sqlca); // Keep track of the SQL error if any
        break;
      }
      break;
    case OT_PROFILE:
      {
        exec sql repeated select profile_name
                  into   :singlename
                  from iiprofiles;
        exec sql begin;
          ManageSqlError(&sqlca); // Keep track of the SQL error if any
			if (HasSelectLoopInterruptBeenRequested()) {
				iresselect = RES_ERR;
				exec sql endselect;
			}
          fstrncpy(pcur->Name,singlename,MAXOBJECTNAME);
          pcur=GetNextLLData(pcur);
          if (!pcur) {
            iresselect = RES_ERR;
            exec sql endselect;
            ManageSqlError(&sqlca); // Keep track of the SQL error if any
          }
          i++;
        exec sql end;
        ManageSqlError(&sqlca); // Keep track of the SQL error if any
      }
      fstrncpy(pcur->Name,lpdefprofiledispstring(),MAXOBJECTNAME);
      pcur=GetNextLLData(pcur);
      if (!pcur) 
         iresselect = RES_ERR;
      i++;
      break;
    case OT_ICE_ROLE                 :
#ifdef NOUSEICEAPI
      {
        exec sql repeated select name
                  into   :singlename
                  from ice_roles;
        exec sql begin;
          ManageSqlError(&sqlca); // Keep track of the SQL error if any
			if (HasSelectLoopInterruptBeenRequested()) {
				iresselect = RES_ERR;
				exec sql endselect;
			}
          fstrncpy(pcur->Name,singlename,MAXOBJECTNAME);
          pcur=GetNextLLData(pcur);
          if (!pcur) {
            iresselect = RES_ERR;
            exec sql endselect;
            ManageSqlError(&sqlca); // Keep track of the SQL error if any
          }
          i++;
        exec sql end;
        ManageSqlError(&sqlca); // Keep track of the SQL error if any
      }
#else
	  {
		ICE_C_PARAMS select[] =
		{
			{ICE_IN, "action",       "select" },
			{ICE_OUT,"role_name",    NULL     },
			{0,      NULL,           NULL     }
		};
        if ((status = ICE_C_Execute (ICEclient, "role", select)) == OK)
        {
            int end;
            do
            {
                if (((status = ICE_C_Fetch (ICEclient, &end)) == OK) && (!end))
                {
                   fstrncpy(pcur->Name,ICE_C_GetAttribute (ICEclient, 1),MAXOBJECTNAME);
                   pcur=GetNextLLData(pcur);
                   if (!pcur) {
                      iresselect = RES_ERR;
                      break;
                   }
                   i++;
                }
            }
            while ((status == OK) && (!end));
            ManageIceError(&ICEclient, &status);
            ICE_C_Close (ICEclient);
			if (status!=OK)
				iresselect = RES_ERR;
        }
		else
				iresselect = RES_ERR;
	  }
#endif
      break;
    case OT_ICE_DBUSER               :
#ifdef NOUSEICEAPI
      {
        exec sql repeated select name
                  into   :singlename
                  from ice_dbusers;
        exec sql begin;
          ManageSqlError(&sqlca); // Keep track of the SQL error if any
			if (HasSelectLoopInterruptBeenRequested()) {
				iresselect = RES_ERR;
				exec sql endselect;
			}
          fstrncpy(pcur->Name,singlename,MAXOBJECTNAME);
          pcur=GetNextLLData(pcur);
          if (!pcur) {
            iresselect = RES_ERR;
            exec sql endselect;
            ManageSqlError(&sqlca); // Keep track of the SQL error if any
          }
          i++;
        exec sql end;
        ManageSqlError(&sqlca); // Keep track of the SQL error if any
      }
#else
	  {
		ICE_C_PARAMS select[] =
		{
			{ICE_IN, "action",       "select" },
			{ICE_OUT,"dbuser_alias",    NULL     },
			{0,      NULL,           NULL     }
		};
        if ((status = ICE_C_Execute (ICEclient, "dbuser", select)) == OK)
        {
            int end;
            do
            {
                if (((status = ICE_C_Fetch (ICEclient, &end)) == OK) && (!end))
                {
                   fstrncpy(pcur->Name,ICE_C_GetAttribute (ICEclient, 1),MAXOBJECTNAME);
                   pcur=GetNextLLData(pcur);
                   if (!pcur) {
                      iresselect = RES_ERR;
                      break;
                   }
                   i++;
                }
            }
            while ((status == OK) && (!end));
            ManageIceError(&ICEclient, &status);
            ICE_C_Close (ICEclient);
			if (status!=OK)
				iresselect = RES_ERR;
        }
		else
				iresselect = RES_ERR;
	  }
#endif
      break;
    case OT_ICE_DBCONNECTION         :
#ifdef NOUSEICEAPI
      {
        exec sql repeated select name
                  into   :singlename
                  from ice_db;
        exec sql begin;
          ManageSqlError(&sqlca); // Keep track of the SQL error if any
			if (HasSelectLoopInterruptBeenRequested()) {
				iresselect = RES_ERR;
				exec sql endselect;
			}
          fstrncpy(pcur->Name,singlename,MAXOBJECTNAME);
          pcur=GetNextLLData(pcur);
          if (!pcur) {
            iresselect = RES_ERR;
            exec sql endselect;
            ManageSqlError(&sqlca); // Keep track of the SQL error if any
          }
          i++;
        exec sql end;
        ManageSqlError(&sqlca); // Keep track of the SQL error if any
      }
#else
	  {
		ICE_C_PARAMS select[] =
		{
			{ICE_IN, "action",       "select" },
			{ICE_OUT,"db_name",      NULL     },
			{0,      NULL,           NULL     }
		};
        if ((status = ICE_C_Execute (ICEclient, "database", select)) == OK)
        {
            int end;
            do
            {
                if (((status = ICE_C_Fetch (ICEclient, &end)) == OK) && (!end))
                {
                   fstrncpy(pcur->Name,ICE_C_GetAttribute (ICEclient, 1),MAXOBJECTNAME);
                   pcur=GetNextLLData(pcur);
                   if (!pcur) {
                      iresselect = RES_ERR;
                      break;
                   }
                   i++;
                }
            }
            while ((status == OK) && (!end));
            ManageIceError(&ICEclient, &status);
            ICE_C_Close (ICEclient);
			if (status!=OK)
				iresselect = RES_ERR;
        }
		else
				iresselect = RES_ERR;
	  }
#endif
      break;
    case OT_ICE_WEBUSER              :
#ifdef NOUSEICEAPI
      {
        exec sql repeated select name
                  into   :singlename
                  from ice_users;
        exec sql begin;
          ManageSqlError(&sqlca); // Keep track of the SQL error if any
			if (HasSelectLoopInterruptBeenRequested()) {
				iresselect = RES_ERR;
				exec sql endselect;
			}
          fstrncpy(pcur->Name,singlename,MAXOBJECTNAME);
          pcur=GetNextLLData(pcur);
          if (!pcur) {
            iresselect = RES_ERR;
            exec sql endselect;
            ManageSqlError(&sqlca); // Keep track of the SQL error if any
          }
          i++;
        exec sql end;
        ManageSqlError(&sqlca); // Keep track of the SQL error if any
      }
#else
	  {
		ICE_C_PARAMS select[] =
		{
			{ICE_IN, "action",       "select" },
			{ICE_OUT,"user_name"  ,       NULL     },
			{0,      NULL,           NULL     }
		};
        if ((status = ICE_C_Execute (ICEclient, "user", select)) == OK)
        {
            int end;
            do
            {
                if (((status = ICE_C_Fetch (ICEclient, &end)) == OK) && (!end))
                {
                   fstrncpy(pcur->Name,ICE_C_GetAttribute (ICEclient, 1),MAXOBJECTNAME);
                   pcur=GetNextLLData(pcur);
                   if (!pcur) {
                      iresselect = RES_ERR;
                      break;
                   }
                   i++;
                }
            }
            while ((status == OK) && (!end));
            ManageIceError(&ICEclient, &status);
            ICE_C_Close (ICEclient);
			if (status!=OK)
				iresselect = RES_ERR;
        }
		else
				iresselect = RES_ERR;
	  }
#endif
      break;
    case OT_ICE_BUNIT_FACET_USER     :
    case OT_ICE_BUNIT_PAGE_USER      :
#ifdef NOUSEICEAPI
#else
	  {
		ICE_C_PARAMS retrieve[] =
		{
			{ICE_IN        ,"action"      , "retrieve"        },
			{ICE_IN|ICE_OUT,"du_doc_id"   , NULL              },
			{ICE_OUT       ,"du_user_name", NULL              },
			{ICE_OUT       , "du_execute" , NULL              },
			{ICE_OUT       , "du_read"    , NULL              },
			{ICE_OUT       , "du_update"  , NULL              },
			{ICE_OUT       , "du_delete"  , NULL              },
			{0             ,NULL          , NULL              }
		};
		retrieve[1].value = busdocdata.doc_ID;
        if ((status = ICE_C_Execute (ICEclient, "document_user", retrieve)) == OK)
        {
            int end;
            do
            {
                if (((status = ICE_C_Fetch (ICEclient, &end)) == OK) && (!end))
                {
					int j1;
					for (j1=0;j1<6;j1++) {
						pret[j1]=ICE_C_GetAttribute (ICEclient, j1+1);
						if (!pret[j1])
							pret[j1]="";
					}
                   if (!x_strcmp(ICE_C_GetAttribute (ICEclient, 1),busdocdata.doc_ID)) {// redundant, but who knows...
                      fstrncpy(pcur->Name,ICE_C_GetAttribute (ICEclient, 2),MAXOBJECTNAME);
						storeint(pcur->ExtraData                ,ICEIsChecked(pret[2]));
						storeint(pcur->ExtraData+STEPSMALLOBJ   ,ICEIsChecked(pret[3]));
						storeint(pcur->ExtraData+2*STEPSMALLOBJ ,ICEIsChecked(pret[4]));
						storeint(pcur->ExtraData+3*STEPSMALLOBJ ,ICEIsChecked(pret[5]));
                      pcur=GetNextLLData(pcur);
                      if (!pcur) {
                         iresselect = RES_ERR;
                         break;
                      }
                      i++;
				   }
                }
            }
            while ((status == OK) && (!end));
            ManageIceError(&ICEclient, &status);
            ICE_C_Close (ICEclient);
			if (status!=OK) {
				iresselect = RES_ERR;
				break;
			}
        }
		else {
				iresselect = RES_ERR;
				break;
		}
	  }
#endif
		break;


    case OT_ICE_BUNIT_FACET_ROLE     :
    case OT_ICE_BUNIT_PAGE_ROLE      :
#ifdef NOUSEICEAPI
#else
	  {
		ICE_C_PARAMS retrieve[] =
		{
			{ICE_IN        ,"action"      , "retrieve"        },
			{ICE_IN|ICE_OUT,"dr_doc_id"   , NULL              },
			{ICE_OUT       ,"dr_role_name", NULL              },
			{ICE_OUT       ,"dr_execute" , NULL               },
			{ICE_OUT       ,"dr_read"    , NULL               },
			{ICE_OUT       ,"dr_update"  , NULL               },
			{ICE_OUT       ,"dr_delete"  , NULL               },
			{0             ,NULL          , NULL              }
		};
		retrieve[1].value=busdocdata.doc_ID;
        if ((status = ICE_C_Execute (ICEclient, "document_role", retrieve)) == OK)
        {
            int end;
            do
            {
                if (((status = ICE_C_Fetch (ICEclient, &end)) == OK) && (!end))
                {
					int j1;
					for (j1=0;j1<6;j1++) {
						pret[j1]=ICE_C_GetAttribute (ICEclient, j1+1);
						if (!pret[j1])
							pret[j1]="";
					}
                   if (!x_strcmp(ICE_C_GetAttribute (ICEclient, 1),busdocdata.doc_ID)) {// redundant, but who knows...
                      fstrncpy(pcur->Name,ICE_C_GetAttribute (ICEclient, 2),MAXOBJECTNAME);
						storeint(pcur->ExtraData                ,ICEIsChecked(pret[2]));
						storeint(pcur->ExtraData+STEPSMALLOBJ   ,ICEIsChecked(pret[3]));
						storeint(pcur->ExtraData+2*STEPSMALLOBJ ,ICEIsChecked(pret[4]));
						storeint(pcur->ExtraData+3*STEPSMALLOBJ ,ICEIsChecked(pret[5]));
 
                      pcur=GetNextLLData(pcur);
                      if (!pcur) {
                         iresselect = RES_ERR;
                         break;
                      }
                      i++;
				   }
                }
            }
            while ((status == OK) && (!end));
            ManageIceError(&ICEclient, &status);
            ICE_C_Close (ICEclient);
			if (status!=OK) {
				iresselect = RES_ERR;
				break;
			}
        }
		else {
				iresselect = RES_ERR;
				break;
		}
	  }
#endif
		break;

    case OT_ICE_WEBUSER_ROLE         :
#ifdef NOUSEICEAPI
      {

        fstrncpy(singlename,parentstrings[0],sizeof(singlename));

        exec sql repeated select c.name
                 into   :singlename
                 from ice_users a, ice_user_role b, ice_roles c
                 where a.name=:singlename and a.id = b.user_id and b.role_id = c.id;

        exec sql begin;
          ManageSqlError(&sqlca); // Keep track of the SQL error if any
			if (HasSelectLoopInterruptBeenRequested()) {
				iresselect = RES_ERR;
				exec sql endselect;
			}
          fstrncpy(pcur->Name,singlename, MAXOBJECTNAME);
          pcur=GetNextLLData(pcur);
          if (!pcur) {
            iresselect = RES_ERR;
            exec sql endselect;
            ManageSqlError(&sqlca); // Keep track of the SQL error if any
          }
          i++;
        exec sql end;
        ManageSqlError(&sqlca); // Keep track of the SQL error if any
      }
#else
	  {
		char bufid[100];
		ICE_C_PARAMS select1[] =
		{
			{ICE_IN, "action",       "select" },
			{ICE_OUT,"user_name" ,    NULL     },
			{ICE_OUT,"user_id"  ,     NULL     },
			{0,      NULL,           NULL     }
		};
		ICE_C_PARAMS select2[] =
		{
			{ICE_IN        , "action"      , "select" },
			{ICE_IN|ICE_OUT,"ur_user_id"   , NULL     },
			{ICE_OUT       ,"ur_role_name" , NULL     },
			{0             , NULL          , NULL     }
		};
		select2[1].value = bufid;
        if ((status = ICE_C_Execute (ICEclient, "user", select1)) == OK)
        {
            int end;
			x_strcpy(bufid,"");
            do
            {
                if (((status = ICE_C_Fetch (ICEclient, &end)) == OK) && (!end))
                {
                   if (!x_strcmp(ICE_C_GetAttribute (ICEclient, 1),parentstrings[0])) {
				      x_strcpy(bufid,ICE_C_GetAttribute (ICEclient, 2));
					  break;
				   }
                }
            }
            while ((status == OK) && (!end));
            ManageIceError(&ICEclient, &status);
            ICE_C_Close (ICEclient);
			if (status!=OK || x_strlen(bufid)==0) {
				iresselect = RES_ERR;
				break;
			}
        }
		else {
				iresselect = RES_ERR;
				break;
		}
        if ((status = ICE_C_Execute (ICEclient, "user_role", select2)) == OK)
        {
            int end;
            do
            {
                if (((status = ICE_C_Fetch (ICEclient, &end)) == OK) && (!end))
                {
                   if (!x_strcmp(ICE_C_GetAttribute (ICEclient, 1),bufid)) {
                      fstrncpy(pcur->Name,ICE_C_GetAttribute (ICEclient, 2),MAXOBJECTNAME);
                      pcur=GetNextLLData(pcur);
                      if (!pcur) {
                         iresselect = RES_ERR;
                         break;
                      }
                      i++;
				   }
                }
            }
            while ((status == OK) && (!end));
            ManageIceError(&ICEclient, &status);
            ICE_C_Close (ICEclient);
			if (status!=OK || x_strlen(bufid)==0) {
				iresselect = RES_ERR;
				break;
			}
        }
		else {
				iresselect = RES_ERR;
				break;
		}
	  }
#endif
      break;
    case OT_ICE_WEBUSER_CONNECTION   :
#ifdef NOUSEICEAPI
      {
        fstrncpy(singlename,parentstrings[0],sizeof(singlename));

        exec sql repeated select c.name
                 into   :singlename
                 from ice_users a, ice_user_db b, ice_db c
                 where a.name=:singlename and a.id = b.user_id and b.db_id = c.id;

        exec sql begin;
          ManageSqlError(&sqlca); // Keep track of the SQL error if any
			if (HasSelectLoopInterruptBeenRequested()) {
				iresselect = RES_ERR;
				exec sql endselect;
			}
          fstrncpy(pcur->Name,singlename, MAXOBJECTNAME);
          pcur=GetNextLLData(pcur);
          if (!pcur) {
            iresselect = RES_ERR;
            exec sql endselect;
            ManageSqlError(&sqlca); // Keep track of the SQL error if any
          }
          i++;
        exec sql end;
        ManageSqlError(&sqlca); // Keep track of the SQL error if any

      }
#else
	  {
		char bufid[100];
		ICE_C_PARAMS select1[] =
		{
			{ICE_IN, "action",       "select" },
			{ICE_OUT,"user_name" ,    NULL     },
			{ICE_OUT,"user_id"  ,     NULL     },
			{0,      NULL,           NULL     }
		};
		ICE_C_PARAMS select2[] =
		{
			{ICE_IN         , "action"     ,    "select" },
			{ICE_IN |ICE_OUT,"ud_user_id"  ,    NULL     },
			{ICE_OUT        ,"ud_db_name"  ,    NULL     },
			{0              , NULL         ,    NULL     }
		};
		select2[1].value = bufid;
        if ((status = ICE_C_Execute (ICEclient, "user", select1)) == OK)
        {
            int end;
			x_strcpy(bufid,"");
            do
            {
                if (((status = ICE_C_Fetch (ICEclient, &end)) == OK) && (!end))
                {
                   if (!x_strcmp(ICE_C_GetAttribute (ICEclient, 1),parentstrings[0])) {
				      x_strcpy(bufid,ICE_C_GetAttribute (ICEclient, 2));
					  break;
				   }
                }
            }
            while ((status == OK) && (!end));
            ManageIceError(&ICEclient, &status);
            ICE_C_Close (ICEclient);
			if (status!=OK || x_strlen(bufid)==0) {
				iresselect = RES_ERR;
				break;
			}
        }
		else {
				iresselect = RES_ERR;
				break;
		}
        if ((status = ICE_C_Execute (ICEclient, "user_database", select2)) == OK)
        {
            int end;
            do
            {
                if (((status = ICE_C_Fetch (ICEclient, &end)) == OK) && (!end))
                {
                   if (!x_strcmp(ICE_C_GetAttribute (ICEclient, 1),bufid)) {
                      fstrncpy(pcur->Name,ICE_C_GetAttribute (ICEclient, 2),MAXOBJECTNAME);
                      pcur=GetNextLLData(pcur);
                      if (!pcur) {
                         iresselect = RES_ERR;
                         break;
                      }
                      i++;
				   }
                }
            }
            while ((status == OK) && (!end));
            ManageIceError(&ICEclient, &status);
            ICE_C_Close (ICEclient);
			if (status!=OK || x_strlen(bufid)==0) {
				iresselect = RES_ERR;
				break;
			}
        }
		else {
				iresselect = RES_ERR;
				break;
		}
	  }
#endif
      break;
    case OT_ICE_PROFILE              :
#ifdef NOUSEICEAPI
      {
        exec sql repeated select name
                  into   :singlename
                  from ice_profiles;
        exec sql begin;
          ManageSqlError(&sqlca); // Keep track of the SQL error if any
			if (HasSelectLoopInterruptBeenRequested()) {
				iresselect = RES_ERR;
				exec sql endselect;
			}
          fstrncpy(pcur->Name,singlename,MAXOBJECTNAME);
          pcur=GetNextLLData(pcur);
          if (!pcur) {
            iresselect = RES_ERR;
            exec sql endselect;
            ManageSqlError(&sqlca); // Keep track of the SQL error if any
          }
          i++;
        exec sql end;
        ManageSqlError(&sqlca); // Keep track of the SQL error if any
      }
#else
	  {
		ICE_C_PARAMS select[] =
		{
			{ICE_IN, "action",       "select" },
			{ICE_OUT,"profile_name", NULL     },
			{0,      NULL,           NULL     }
		};
        if ((status = ICE_C_Execute (ICEclient, "profile", select)) == OK)
        {
            int end;
            do
            {
                if (((status = ICE_C_Fetch (ICEclient, &end)) == OK) && (!end))
                {
                   fstrncpy(pcur->Name,ICE_C_GetAttribute (ICEclient, 1),MAXOBJECTNAME);
                   pcur=GetNextLLData(pcur);
                   if (!pcur) {
                      iresselect = RES_ERR;
                      break;
                   }
                   i++;
                }
            }
            while ((status == OK) && (!end));
            ManageIceError(&ICEclient, &status);
            ICE_C_Close (ICEclient);
			if (status!=OK)
				iresselect = RES_ERR;
        }
		else
				iresselect = RES_ERR;
	  }
#endif
      break;
    case OT_ICE_PROFILE_ROLE         :
#ifdef NOUSEICEAPI
      {

        fstrncpy(singlename,parentstrings[0],sizeof(singlename));

        exec sql repeated select c.name
                 into   :singlename
                 from ice_profiles a, ice_profiles_roles b, ice_roles c
                 where a.name=:singlename and a.id = b.profile and b.role = c.id;

        exec sql begin;
          ManageSqlError(&sqlca); // Keep track of the SQL error if any
			if (HasSelectLoopInterruptBeenRequested()) {
				iresselect = RES_ERR;
				exec sql endselect;
			}
          fstrncpy(pcur->Name,singlename, MAXOBJECTNAME);
          pcur=GetNextLLData(pcur);
          if (!pcur) {
            iresselect = RES_ERR;
            exec sql endselect;
            ManageSqlError(&sqlca); // Keep track of the SQL error if any
          }
          i++;
        exec sql end;
        ManageSqlError(&sqlca); // Keep track of the SQL error if any
      }
#else
	  {
		char bufid[100];
		ICE_C_PARAMS select1[] =
		{
			{ICE_IN ,"action"      , "select" },
			{ICE_OUT,"profile_name",  NULL    },
			{ICE_OUT,"profile_id"  ,  NULL    },
			{0      ,NULL          ,  NULL    }
		};
		ICE_C_PARAMS select2[] =
		{
			{ICE_IN , "action"      ,    "select" },
			{ICE_IN|ICE_OUT,"pr_profile_id"   ,    NULL     },
			{ICE_OUT,"pr_role_name" ,    NULL     },
			{0      , NULL          ,    NULL     }
		};
		select2[1].value = bufid;
		
        if ((status = ICE_C_Execute (ICEclient, "profile", select1)) == OK)
        {
            int end;
			x_strcpy(bufid,"");
            do
            {
                if (((status = ICE_C_Fetch (ICEclient, &end)) == OK) && (!end))
                {
                   if (!x_strcmp(ICE_C_GetAttribute (ICEclient, 1),parentstrings[0])) {
				      x_strcpy(bufid,ICE_C_GetAttribute (ICEclient, 2));
					  break;
				   }
                }
            }
            while ((status == OK) && (!end));
            ManageIceError(&ICEclient, &status);
            ICE_C_Close (ICEclient);
			if (status!=OK || x_strlen(bufid)==0) {
				iresselect = RES_ERR;
				break;
			}
        }
		else {
				iresselect = RES_ERR;
				break;
		}
        if ((status = ICE_C_Execute (ICEclient, "profile_role", select2)) == OK)
        {
            int end;
            do
            {
                if (((status = ICE_C_Fetch (ICEclient, &end)) == OK) && (!end))
                {
                   if (!x_strcmp(ICE_C_GetAttribute (ICEclient, 1),bufid)) {
                      fstrncpy(pcur->Name,ICE_C_GetAttribute (ICEclient, 2),MAXOBJECTNAME);
                      pcur=GetNextLLData(pcur);
                      if (!pcur) {
                         iresselect = RES_ERR;
                         break;
                      }
                      i++;
				   }
                }
            }
            while ((status == OK) && (!end));
            ManageIceError(&ICEclient, &status);
            ICE_C_Close (ICEclient);
			if (status!=OK || x_strlen(bufid)==0) {
				iresselect = RES_ERR;
				break;
			}
        }
		else {
				iresselect = RES_ERR;
				break;
		}
	  }
#endif
      break;
    case OT_ICE_PROFILE_CONNECTION   :
#ifdef NOUSEICEAPI
      {
        fstrncpy(singlename,parentstrings[0],sizeof(singlename));

        exec sql repeated select c.name
                 into   :singlename
                 from ice_profiles a, ice_profiles_dbs b, ice_db c
                 where a.name=:singlename and a.id = b.profile and b.db = c.id;

        exec sql begin;
          ManageSqlError(&sqlca); // Keep track of the SQL error if any
			if (HasSelectLoopInterruptBeenRequested()) {
				iresselect = RES_ERR;
				exec sql endselect;
			}
          fstrncpy(pcur->Name,singlename, MAXOBJECTNAME);
          pcur=GetNextLLData(pcur);
          if (!pcur) {
            iresselect = RES_ERR;
            exec sql endselect;
            ManageSqlError(&sqlca); // Keep track of the SQL error if any
          }
          i++;
        exec sql end;
        ManageSqlError(&sqlca); // Keep track of the SQL error if any

      }
#else
	  {
		char bufid[100];
		ICE_C_PARAMS select1[] =
		{
			{ICE_IN, "action",       "select" },
			{ICE_OUT,"profile_name" ,    NULL     },
			{ICE_OUT,"profile_id"  ,     NULL     },
			{0,      NULL,           NULL     }
		};
		ICE_C_PARAMS select2[] =
		{
			{ICE_IN        , "action"       , "select" },
			{ICE_IN|ICE_OUT,"pd_profile_id" ,    NULL  },
			{ICE_OUT       ,"pd_db_name"    ,    NULL  },
			{0             , NULL           ,    NULL  }
		};
		select2[1].value=bufid;
        if ((status = ICE_C_Execute (ICEclient, "profile", select1)) == OK)
        {
            int end;
			x_strcpy(bufid,"");
            do
            {
                if (((status = ICE_C_Fetch (ICEclient, &end)) == OK) && (!end))
                {
                   if (!x_strcmp(ICE_C_GetAttribute (ICEclient, 1),parentstrings[0])) {
				      x_strcpy(bufid,ICE_C_GetAttribute (ICEclient, 2));
					  break;
				   }
                }
            }
            while ((status == OK) && (!end));
            ManageIceError(&ICEclient, &status);
            ICE_C_Close (ICEclient);
			if (status!=OK || x_strlen(bufid)==0) {
				iresselect = RES_ERR;
				break;
			}
        }
		else {
				iresselect = RES_ERR;
				break;
		}
        if ((status = ICE_C_Execute (ICEclient, "profile_database", select2)) == OK)
        {
            int end;
            do
            {
                if (((status = ICE_C_Fetch (ICEclient, &end)) == OK) && (!end))
                {
                   if (!x_strcmp(ICE_C_GetAttribute (ICEclient, 1),bufid)) {
                      fstrncpy(pcur->Name,ICE_C_GetAttribute (ICEclient, 2),MAXOBJECTNAME);
                      pcur=GetNextLLData(pcur);
                      if (!pcur) {
                         iresselect = RES_ERR;
                         break;
                      }
                      i++;
				   }
                }
            }
            while ((status == OK) && (!end));
            ManageIceError(&ICEclient, &status);
            ICE_C_Close (ICEclient);
			if (status!=OK || x_strlen(bufid)==0) {
				iresselect = RES_ERR;
				break;
			}
        }
		else {
				iresselect = RES_ERR;
				break;
		}
	  }
#endif
      break;
    case OT_ICE_BUNIT                :
#ifdef NOUSEICEAPI
      {
        exec sql repeated select name
                  into   :singlename
                  from ice_units;
        exec sql begin;
          ManageSqlError(&sqlca); // Keep track of the SQL error if any
			if (HasSelectLoopInterruptBeenRequested()) {
				iresselect = RES_ERR;
				exec sql endselect;
			}
          fstrncpy(pcur->Name,singlename,MAXOBJECTNAME);
          pcur=GetNextLLData(pcur);
          if (!pcur) {
            iresselect = RES_ERR;
            exec sql endselect;
            ManageSqlError(&sqlca); // Keep track of the SQL error if any
          }
          i++;
        exec sql end;
        ManageSqlError(&sqlca); // Keep track of the SQL error if any
      }
#else
	  {
		ICE_C_PARAMS select[] =
		{
			{ICE_IN, "action",       "select" },
			{ICE_OUT,"unit_name",    NULL     },
			{0,      NULL,           NULL     }
		};
        if ((status = ICE_C_Execute (ICEclient, "unit", select)) == OK)
        {
            int end;
            do
            {
                if (((status = ICE_C_Fetch (ICEclient, &end)) == OK) && (!end))
                {
                   fstrncpy(pcur->Name,ICE_C_GetAttribute (ICEclient, 1),MAXOBJECTNAME);
                   pcur=GetNextLLData(pcur);
                   if (!pcur) {
                      iresselect = RES_ERR;
                      break;
                   }
                   i++;
                }
            }
            while ((status == OK) && (!end));
            ManageIceError(&ICEclient, &status);
            ICE_C_Close (ICEclient);
			if (status!=OK)
				iresselect = RES_ERR;
        }
		else
				iresselect = RES_ERR;
	  }
#endif
      break;
    case OT_ICE_BUNIT_SEC_ROLE       :
#ifdef NOUSEICEAPI
#else
	  {
		char bufid[100];
		ICE_C_PARAMS select1[] =
		{
			{ICE_IN, "action",       "select" },
			{ICE_OUT,"unit_name" ,    NULL     },
			{ICE_OUT,"unit_id"  ,     NULL     },
			{0,      NULL,           NULL     }
		};
		ICE_C_PARAMS select2[] =
		{
			{ICE_IN        , "action"       , "retrieve" },
			{ICE_IN|ICE_OUT, "ur_unit_id"   , NULL     },
			{ICE_OUT       , "ur_role_name" , NULL     },
			{ICE_OUT       , "ur_execute"   , NULL     },
			{ICE_OUT       , "ur_read"      , NULL     },
			{ICE_OUT       , "ur_insert"    , NULL     },
			{0             , NULL           , NULL     }
		};
		select2[1].value=bufid;
        if ((status = ICE_C_Execute (ICEclient, "unit", select1)) == OK)
        {
			int end;
			x_strcpy(bufid,"");
			do
			{
				if (((status = ICE_C_Fetch (ICEclient, &end)) == OK) && (!end))
				{
					if (!x_strcmp(ICE_C_GetAttribute (ICEclient, 1),parentstrings[0])) {
						x_strcpy(bufid,ICE_C_GetAttribute (ICEclient, 2));
						break;
					}
				}
			}
            while ((status == OK) && (!end));
            ManageIceError(&ICEclient, &status);
            ICE_C_Close (ICEclient);
			if (status!=OK || x_strlen(bufid)==0) {
				iresselect = RES_ERR;
				break;
			}
        }
		else {
				iresselect = RES_ERR;
				break;
		}
		if ((status = ICE_C_Execute (ICEclient, "unit_role", select2)) == OK)
		{
			int end;
			do
			{
				if (((status = ICE_C_Fetch (ICEclient, &end)) == OK) && (!end))
				{
					int j1;
					for (j1=0;j1<5;j1++) {
						pret[j1]=ICE_C_GetAttribute (ICEclient, j1+1);
						if (!pret[j1])
							pret[j1]="";
					}
					if (!x_strcmp(ICE_C_GetAttribute (ICEclient, 1),bufid)) {
						fstrncpy(pcur->Name,ICE_C_GetAttribute (ICEclient, 2),MAXOBJECTNAME);
						// order is execute read insert
						storeint(pcur->ExtraData                ,ICEIsChecked(pret[2]));
						storeint(pcur->ExtraData+STEPSMALLOBJ   ,ICEIsChecked(pret[3]));
						storeint(pcur->ExtraData+2*STEPSMALLOBJ ,ICEIsChecked(pret[4]));
						pcur=GetNextLLData(pcur);
						if (!pcur) {
							iresselect = RES_ERR;
							break;
						}
						i++;
					}
				}
			}
            while ((status == OK) && (!end));
            ManageIceError(&ICEclient, &status);
            ICE_C_Close (ICEclient);
			if (status!=OK || x_strlen(bufid)==0) {
				iresselect = RES_ERR;
				break;
			}
        }
		else {
				iresselect = RES_ERR;
				break;
		}
	  }
#endif
		break;
    case OT_ICE_BUNIT_SEC_USER       :
#ifdef NOUSEICEAPI
#else
	  {
		char bufid[100];
		ICE_C_PARAMS select1[] =
		{
			{ICE_IN, "action",       "select" },
			{ICE_OUT,"unit_name" ,    NULL     },
			{ICE_OUT,"unit_id"  ,     NULL     },
			{0,      NULL,           NULL     }
		};
		ICE_C_PARAMS select2[] =
		{
			{ICE_IN        , "action"      , "retrieve" },
			{ICE_IN|ICE_OUT,"uu_unit_id"   , NULL       },
			{ICE_OUT       ,"uu_user_name" , NULL       },
			{ICE_OUT       ,"uu_execute"   , NULL       },
			{ICE_OUT       ,"uu_read"      , NULL       },
			{ICE_OUT       ,"uu_insert"    , NULL       },
			{0             ,NULL           , NULL       }
		};
		select2[1].value=bufid;
        if ((status = ICE_C_Execute (ICEclient, "unit", select1)) == OK)
        {
            int end;
			x_strcpy(bufid,"");
            do
            {
                if (((status = ICE_C_Fetch (ICEclient, &end)) == OK) && (!end))
                {
                   if (!x_strcmp(ICE_C_GetAttribute (ICEclient, 1),parentstrings[0])) {
				      x_strcpy(bufid,ICE_C_GetAttribute (ICEclient, 2));
					  break;
				   }
                }
            }
            while ((status == OK) && (!end));
            ManageIceError(&ICEclient, &status);
            ICE_C_Close (ICEclient);
			if (status!=OK || x_strlen(bufid)==0) {
				iresselect = RES_ERR;
				break;
			}
        }
		else {
				iresselect = RES_ERR;
				break;
		}
        if ((status = ICE_C_Execute (ICEclient, "unit_user", select2)) == OK)
        {
            int end;
            do
            {
                if (((status = ICE_C_Fetch (ICEclient, &end)) == OK) && (!end))
                {
					int j1;
					for (j1=0;j1<5;j1++) {
						pret[j1]=ICE_C_GetAttribute (ICEclient, j1+1);
						if (!pret[j1])
							pret[j1]="";
					}
                   if (!x_strcmp(ICE_C_GetAttribute (ICEclient, 1),bufid)) {
                      fstrncpy(pcur->Name,ICE_C_GetAttribute (ICEclient, 2),MAXOBJECTNAME);
						// order is UnitRead ReadDoc CreateDoc
						storeint(pcur->ExtraData                ,ICEIsChecked(pret[2]));
						storeint(pcur->ExtraData+STEPSMALLOBJ   ,ICEIsChecked(pret[3]));
						storeint(pcur->ExtraData+2*STEPSMALLOBJ ,ICEIsChecked(pret[4]));
                      pcur=GetNextLLData(pcur);
                      if (!pcur) {
                         iresselect = RES_ERR;
                         break;
                      }
                      i++;
				   }
                }
            }
            while ((status == OK) && (!end));
            ManageIceError(&ICEclient, &status);
            ICE_C_Close (ICEclient);
			if (status!=OK || x_strlen(bufid)==0) {
				iresselect = RES_ERR;
				break;
			}
        }
		else {
				iresselect = RES_ERR;
				break;
		}
	  }
#endif
		break;
    case OT_ICE_BUNIT_FACET          :
    case OT_ICE_BUNIT_PAGE           :
#ifdef NOUSEICEAPI
#else
	  {
		char bufid[100];
		ICE_C_PARAMS select1[] =
		{
			{ICE_IN, "action",       "select" },
			{ICE_OUT,"unit_name" ,    NULL     },
			{ICE_OUT,"unit_id"  ,     NULL     },
			{0,      NULL,           NULL     }
		};
		ICE_C_PARAMS select2[] =
		{
			{ICE_IN        , "action"      , "select" },
			{ICE_OUT       ,"doc_unit_id"  , NULL    },
			{ICE_OUT       ,"doc_name"     , NULL     },
			{ICE_OUT       ,"doc_suffix"   , NULL     },
			{ICE_OUT       ,"doc_type"     , NULL     },
			{0             , NULL          , NULL     }
		};
        if ((status = ICE_C_Execute (ICEclient, "unit", select1)) == OK)
        {
            int end;
			x_strcpy(bufid,"");
            do
            {
                if (((status = ICE_C_Fetch (ICEclient, &end)) == OK) && (!end))
                {
                   if (!x_strcmp(ICE_C_GetAttribute (ICEclient, 1),parentstrings[0])) {
				      x_strcpy(bufid,ICE_C_GetAttribute (ICEclient, 2));
					  break;
				   }
                }
            }
            while ((status == OK) && (!end));
            ManageIceError(&ICEclient, &status);
            ICE_C_Close (ICEclient);
			if (status!=OK || x_strlen(bufid)==0) {
				iresselect = RES_ERR;
				break;
			}
        }
		else {
				iresselect = RES_ERR;
				break;
		}
        if ((status = ICE_C_Execute (ICEclient, "document", select2)) == OK)
		{
			int end;
			do
			{
				if (((status = ICE_C_Fetch (ICEclient, &end)) == OK) && (!end))
				{
					int j1;
					for (j1=0;j1<4;j1++) {
						pret[j1]=ICE_C_GetAttribute (ICEclient, j1+1);
						if (!pret[j1])
							pret[j1]="";
					}

					if (!x_strcmp(pret[0],bufid)) {
						UCHAR buftype[100];
						x_strcpy(buftype,pret[3]);
						if(  (iobjecttype==OT_ICE_BUNIT_FACET && !x_strcmp(buftype,"facet"))
						   ||(iobjecttype==OT_ICE_BUNIT_PAGE  && !x_strcmp(buftype,"page" ))
						  ) {
							wsprintf(pcur->Name,"%s.%s",pret[1],pret[2]);
							pcur=GetNextLLData(pcur);
							if (!pcur) {
								iresselect = RES_ERR;
								break;
							}
							i++;
						}
					}
				}
			}
			while ((status == OK) && (!end));
			ManageIceError(&ICEclient, &status);
			ICE_C_Close (ICEclient);
			if (status!=OK || x_strlen(bufid)==0) {
				iresselect = RES_ERR;
				break;
			}
		}
		else {
				iresselect = RES_ERR;
				break;
		}
	  }
#endif
		break;
    case OT_ICE_BUNIT_LOCATION       :
#ifdef NOUSEICEAPI
      {
        fstrncpy(singlename,parentstrings[0],sizeof(singlename));

        exec sql repeated select c.name
                 into   :singlename
                 from ice_units a, ice_units_locs b, ice_locations c
                 where a.name=:singlename and a.id = b.unit and b.loc = c.id;

        exec sql begin;
          ManageSqlError(&sqlca); // Keep track of the SQL error if any
			if (HasSelectLoopInterruptBeenRequested()) {
				iresselect = RES_ERR;
				exec sql endselect;
			}
          fstrncpy(pcur->Name,singlename, MAXOBJECTNAME);
          pcur=GetNextLLData(pcur);
          if (!pcur) {
            iresselect = RES_ERR;
            exec sql endselect;
            ManageSqlError(&sqlca); // Keep track of the SQL error if any
          }
          i++;
        exec sql end;
        ManageSqlError(&sqlca); // Keep track of the SQL error if any

      }
#else
	  {
		char bufid[100];
		ICE_C_PARAMS select1[] =
		{
			{ICE_IN, "action",       "select" },
			{ICE_OUT,"unit_name" ,    NULL     },
			{ICE_OUT,"unit_id"  ,     NULL     },
			{0,      NULL,           NULL     }
		};
		ICE_C_PARAMS select2[] =
		{
			{ICE_IN        , "action"          , "select" },
			{ICE_IN|ICE_OUT,"ul_unit_id"       , NULL     },
			{ICE_OUT       ,"ul_location_name" , NULL     },
			{0             , NULL              , NULL     }
		};
		select2[1].value=bufid;
        if ((status = ICE_C_Execute (ICEclient, "unit", select1)) == OK)
        {
            int end;
			x_strcpy(bufid,"");
            do
            {
                if (((status = ICE_C_Fetch (ICEclient, &end)) == OK) && (!end))
                {
                   if (!x_strcmp(ICE_C_GetAttribute (ICEclient, 1),parentstrings[0])) {
				      x_strcpy(bufid,ICE_C_GetAttribute (ICEclient, 2));
					  break;
				   }
                }
            }
            while ((status == OK) && (!end));
            ManageIceError(&ICEclient, &status);
            ICE_C_Close (ICEclient);
			if (status!=OK || x_strlen(bufid)==0) {
				iresselect = RES_ERR;
				break;
			}
        }
		else {
				iresselect = RES_ERR;
				break;
		}
        if ((status = ICE_C_Execute (ICEclient, "unit_location", select2)) == OK)
        {
            int end;
            do
            {
                if (((status = ICE_C_Fetch (ICEclient, &end)) == OK) && (!end))
                {
                   if (!x_strcmp(ICE_C_GetAttribute (ICEclient, 1),bufid)) {
                      fstrncpy(pcur->Name,ICE_C_GetAttribute (ICEclient, 2),MAXOBJECTNAME);
                      pcur=GetNextLLData(pcur);
                      if (!pcur) {
                         iresselect = RES_ERR;
                         break;
                      }
                      i++;
				   }
                }
            }
            while ((status == OK) && (!end));
            ManageIceError(&ICEclient, &status);
            ICE_C_Close (ICEclient);
			if (status!=OK || x_strlen(bufid)==0) {
				iresselect = RES_ERR;
				break;
			}
        }
		else {
				iresselect = RES_ERR;
				break;
		}
	  }
#endif
      break;
    case OT_ICE_SERVER_APPLICATION   :
#ifdef NOUSEICEAPI
      {
        exec sql repeated select name
                  into   :singlename
                  from ice_applications;
        exec sql begin;
          ManageSqlError(&sqlca); // Keep track of the SQL error if any
			if (HasSelectLoopInterruptBeenRequested()) {
				iresselect = RES_ERR;
				exec sql endselect;
			}
          fstrncpy(pcur->Name,singlename,MAXOBJECTNAME);
          pcur=GetNextLLData(pcur);
          if (!pcur) {
            iresselect = RES_ERR;
            exec sql endselect;
            ManageSqlError(&sqlca); // Keep track of the SQL error if any
          }
          i++;
        exec sql end;
        ManageSqlError(&sqlca); // Keep track of the SQL error if any
      }
#else
	  {
		ICE_C_PARAMS select[] =
		{
			{ICE_IN, "action",       "select" },
			{ICE_OUT,"sess_name",    NULL     },
			{0,      NULL,           NULL     }
		};
        if ((status = ICE_C_Execute (ICEclient, "session_grp", select)) == OK)
        {
            int end;
            do
            {
                if (((status = ICE_C_Fetch (ICEclient, &end)) == OK) && (!end))
                {
                   fstrncpy(pcur->Name,ICE_C_GetAttribute (ICEclient, 1),MAXOBJECTNAME);
                   pcur=GetNextLLData(pcur);
                   if (!pcur) {
                      iresselect = RES_ERR;
                      break;
                   }
                   i++;
                }
            }
            while ((status == OK) && (!end));
            ManageIceError(&ICEclient, &status);
            ICE_C_Close (ICEclient);
			if (status!=OK)
				iresselect = RES_ERR;
        }
		else
				iresselect = RES_ERR;
	  }
#endif
      break;
    case OT_ICE_SERVER_LOCATION      :
#ifdef NOUSEICEAPI
      {
        exec sql repeated select name
                  into   :singlename
                  from ice_locations;
        exec sql begin;
          ManageSqlError(&sqlca); // Keep track of the SQL error if any
			if (HasSelectLoopInterruptBeenRequested()) {
				iresselect = RES_ERR;
				exec sql endselect;
			}
          fstrncpy(pcur->Name,singlename,MAXOBJECTNAME);
          pcur=GetNextLLData(pcur);
          if (!pcur) {
            iresselect = RES_ERR;
            exec sql endselect;
            ManageSqlError(&sqlca); // Keep track of the SQL error if any
          }
          i++;
        exec sql end;
        ManageSqlError(&sqlca); // Keep track of the SQL error if any
      }
#else
	  {
		ICE_C_PARAMS select[] =
		{
			{ICE_IN, "action",       "select" },
			{ICE_OUT,"loc_name",    NULL     },
			{0,      NULL,           NULL     }
		};
        if ((status = ICE_C_Execute (ICEclient, "ice_locations", select)) == OK)
        {
            int end;
            do
            {
                if (((status = ICE_C_Fetch (ICEclient, &end)) == OK) && (!end))
                {
                   fstrncpy(pcur->Name,ICE_C_GetAttribute (ICEclient, 1),MAXOBJECTNAME);
                   pcur=GetNextLLData(pcur);
                   if (!pcur) {
                      iresselect = RES_ERR;
                      break;
                   }
                   i++;
                }
            }
            while ((status == OK) && (!end));
            ManageIceError(&ICEclient, &status);
            ICE_C_Close (ICEclient);
			if (status!=OK)
				iresselect = RES_ERR;
        }
		else
				iresselect = RES_ERR;
	  }
#endif
      break;
    case OT_ICE_SERVER_VARIABLE      :
#ifdef NOUSEICEAPI
      {
        exec sql repeated select name
                  into   :singlename
                  from ice_server_vars;
        exec sql begin;
          ManageSqlError(&sqlca); // Keep track of the SQL error if any
			if (HasSelectLoopInterruptBeenRequested()) {
				iresselect = RES_ERR;
				exec sql endselect;
			}
          fstrncpy(pcur->Name,singlename,MAXOBJECTNAME);
          pcur=GetNextLLData(pcur);
          if (!pcur) {
            iresselect = RES_ERR;
            exec sql endselect;
            ManageSqlError(&sqlca); // Keep track of the SQL error if any
          }
          i++;
        exec sql end;
        ManageSqlError(&sqlca); // Keep track of the SQL error if any
      }
#else
	  {
		ICE_C_PARAMS select[] =
		{
			{ICE_OUT|ICE_IN,"page"  ,  ""     },
			{ICE_OUT|ICE_IN,"session"  ,  ""     },
			{ICE_OUT|ICE_IN,"server"  ,  "checked"     },
			{ICE_OUT,"name"  ,  NULL     },
			{ICE_OUT,"value"  ,  NULL     },
			{0,      NULL,      NULL     }
		};
        if ((status = ICE_C_Execute (ICEclient, "getVariables", select)) == OK)
        {
            int end;
            do
            {
                if (((status = ICE_C_Fetch (ICEclient, &end)) == OK) && (!end))
                {
                   fstrncpy(pcur->Name,     ICE_C_GetAttribute (ICEclient, 3),MAXOBJECTNAME);
                   pcur=GetNextLLData(pcur);
                   if (!pcur) {
                      iresselect = RES_ERR;
                      break;
                   }
                   i++;
                }
            }
            while ((status == OK) && (!end));
            ManageIceError(&ICEclient, &status);
            ICE_C_Close (ICEclient);
			if (status!=OK)
				iresselect = RES_ERR;
        }
		else {
			ManageIceError(&ICEclient, &status);
			iresselect = RES_ERR;
		}
	  }
#endif
      break;
    case OT_ROLEGRANT_USER:
      {
        fstrncpy(name3,parentstrings[0],sizeof(name3));
        exec sql repeated select grantee_name,gr_type
                  into   :singlename,:extradata
                  from   iirolegrants
                  where  role_name=:name3 ;
        exec sql begin;
          ManageSqlError(&sqlca); // Keep track of the SQL error if any
			if (HasSelectLoopInterruptBeenRequested()) {
				iresselect = RES_ERR;
				exec sql endselect;
			}
          if (extradata[0]=='P')
            fstrncpy(singlename,lppublicdispstring(),MAXOBJECTNAME);
          fstrncpy(pcur->Name,singlename,MAXOBJECTNAME);
          pcur=GetNextLLData(pcur);
          if (!pcur) {
            iresselect = RES_ERR;
            exec sql endselect;
            ManageSqlError(&sqlca); // Keep track of the SQL error if any
          }
          i++;
        exec sql end;
        ManageSqlError(&sqlca); // Keep track of the SQL error if any
      }
      break;
    case OT_LOCATION:
      {
        if (GetOIVers() >= OIVERS_26)
        {
          EXEC SQL REPEATED SELECT location_name,location_area,
                    data_usage, work_usage, jrnl_usage, ckpt_usage, dump_usage , raw_pct
                    INTO   :singlename,  :extradata,
                    :rt1,       :rt2,       :rt3,       :rt4,       :rt5,     :intno
                    FROM iilocation_info ;
            EXEC SQL BEGIN;
              ManageSqlError(&sqlca); // Keep track of the SQL error if any
                if (HasSelectLoopInterruptBeenRequested()) {
                    iresselect = RES_ERR;
                    exec sql endselect;
                }
              fstrncpy(pcur->Name,singlename,MAXOBJECTNAME);
              suppspace(extradata);
              fstrncpy(pcur->ExtraData,extradata,MAXOBJECTNAME);
              pctemp=pcur->ExtraData+MAXOBJECTNAME;
              // DBCS management: DO NOT use CMnext here, the code below is just a mapping
              // of some info into a few 1 bytes characters
              *(pctemp++)=GetYNState(rt1);
              *(pctemp++)=GetYNState(rt2);
              *(pctemp++)=GetYNState(rt3);
              *(pctemp++)=GetYNState(rt4);
              *(pctemp++)=GetYNState(rt5);
              storeint(pcur->OwnerName,intno);
              pcur=GetNextLLData(pcur);
              if (!pcur) {
                iresselect = RES_ERR;
                exec sql endselect;
                ManageSqlError(&sqlca); // Keep track of the SQL error if any
              }
              i++;
            EXEC SQL END;
        }
        else
        {
          intno = 0;
          EXEC SQL REPEATED SELECT location_name,location_area,
                    data_usage, work_usage, jrnl_usage, ckpt_usage, dump_usage
                    INTO   :singlename,  :extradata,
                    :rt1,       :rt2,       :rt3,       :rt4,       :rt5
                    FROM iilocation_info ;
            EXEC SQL BEGIN;
              ManageSqlError(&sqlca); // Keep track of the SQL error if any
                if (HasSelectLoopInterruptBeenRequested()) {
                    iresselect = RES_ERR;
                    exec sql endselect;
                }
              fstrncpy(pcur->Name,singlename,MAXOBJECTNAME);
              suppspace(extradata);
              fstrncpy(pcur->ExtraData,extradata,MAXOBJECTNAME);
              pctemp=pcur->ExtraData+MAXOBJECTNAME;
              // DBCS management: DO NOT use CMnext here, the code below is just a mapping
              // of some info into a few 1 bytes characters
              *(pctemp++)=GetYNState(rt1);
              *(pctemp++)=GetYNState(rt2);
              *(pctemp++)=GetYNState(rt3);
              *(pctemp++)=GetYNState(rt4);
              *(pctemp++)=GetYNState(rt5);
              storeint(pcur->OwnerName,intno);
              pcur=GetNextLLData(pcur);
              if (!pcur) {
                iresselect = RES_ERR;
                exec sql endselect;
                ManageSqlError(&sqlca); // Keep track of the SQL error if any
              }
              i++;
            EXEC SQL END;
        }

        ManageSqlError(&sqlca); // Keep track of the SQL error if any
      }
      break;
    case OT_USER:
      {
        exec sql repeated select user_name       ,audit_all
                 into   :singlename,:extradata
                 from iiusers ;

        exec sql begin;
          ManageSqlError(&sqlca); // Keep track of the SQL error if any
          if (HasSelectLoopInterruptBeenRequested()) {
            iresselect = RES_ERR;
            exec sql endselect;
          }
          suppspace(singlename);
          fstrncpy(pcur->Name,singlename,MAXOBJECTNAME);
          pcur->ExtraData[0]=GetYNState(extradata);
                  pcur=GetNextLLData(pcur);
          if (!pcur) {
            iresselect = RES_ERR;
            exec sql endselect;
            ManageSqlError(&sqlca); // Keep track of the SQL error if any
          }
          i++;
        exec sql end;
        ManageSqlError(&sqlca); // Keep track of the SQL error if any

                        /* add user "(public)" */
        if (pcur) {
          /* Emb 17/10/95 : don't add if there was a memory error in the select */
          fstrncpy(pcur->Name,lppublicdispstring(),MAXOBJECTNAME);
          pcur=GetNextLLData(pcur);
        }
        if (!pcur) {
          iresselect = RES_ERR;
        }
        i++;
      }
      break;
    case OT_GROUP:
        exec sql repeated select groupid
                 into :singlename
                 from iiusergroup
                 group by groupid ;

        exec sql begin;
          ManageSqlError(&sqlca); // Keep track of the SQL error if any
			if (HasSelectLoopInterruptBeenRequested()) {
				iresselect = RES_ERR;
				exec sql endselect;
			}
          fstrncpy(pcur->Name,singlename,MAXOBJECTNAME);
          pcur=GetNextLLData(pcur);
          if (!pcur) {
            iresselect = RES_ERR;
            exec sql endselect;
            ManageSqlError(&sqlca); // Keep track of the SQL error if any
          }
          i++;
        exec sql end;
        ManageSqlError(&sqlca); // Keep track of the SQL error if any
      break;
    case OT_ROLE:
        exec sql repeated select roleid
                 into   :singlename
                 from iirole ;

        exec sql begin;
          ManageSqlError(&sqlca); // Keep track of the SQL error if any
			if (HasSelectLoopInterruptBeenRequested()) {
				iresselect = RES_ERR;
				exec sql endselect;
			}
          fstrncpy(pcur->Name,singlename,MAXOBJECTNAME);
          pcur=GetNextLLData(pcur);
          if (!pcur) {
            iresselect = RES_ERR;
            exec sql endselect;
            ManageSqlError(&sqlca); // Keep track of the SQL error if any
          }
          i++;
        exec sql end;
        ManageSqlError(&sqlca); // Keep track of the SQL error if any
      break;
    case OT_PROCEDURE:
        IsFound=FALSE;
        exec sql repeated select procedure_name,procedure_owner,text_sequence,text_segment
                 into   :singlename,:singleownername,  :intno,       :extradata
                 from iiprocedures
                 order by procedure_name, procedure_owner, text_sequence;

        exec sql begin;
           ManageSqlError(&sqlca); // Keep track of the SQL error if any
			if (HasSelectLoopInterruptBeenRequested()) {
				iresselect = RES_ERR;
				exec sql endselect;
			}
           suppspace(singlename);
           suppspace(singleownername);
           if (!IsFound) {
              IsFound=TRUE;
              fstrncpy(OLDname,singlename,MAXOBJECTNAME);
              fstrncpy(OLDowner,singleownername,MAXOBJECTNAME);
           }            
           if (x_strcmp(OLDname,singlename)==0 &&
              x_strcmp(OLDowner,singleownername)==0) {   
              if (!ConcatText(extradata)) {          // concatenate
                 iresselect = RES_ERR;
                 exec sql endselect;
                 ManageSqlError(&sqlca); // Keep track of the SQL error if any
              }
           }
           else {
              fstrncpy(pcur->Name,(LPTSTR)Quote4DisplayIfNeeded(OLDname),MAXOBJECTNAME);
              fstrncpy(pcur->OwnerName,OLDowner,MAXOBJECTNAME);
              ParseProcText(pcur->ExtraData);
              FreeText();
              if (!ConcatText(extradata))            // concatenate
                 return RES_ERR;
              fstrncpy(OLDname,singlename,MAXOBJECTNAME);
              fstrncpy(OLDowner,singleownername,MAXOBJECTNAME);
              pcur=GetNextLLData(pcur);
              if (!pcur) {
                iresselect = RES_ERR;
                exec sql endselect;
                ManageSqlError(&sqlca); // Keep track of the SQL error if any
              }
              i++;
           }
        exec sql end;
        ManageSqlError(&sqlca); // Keep track of the SQL error if any
        if (IsFound && iresselect!=RES_ERR) {        // process the last one
           ParseProcText(pcur->ExtraData);
           fstrncpy(pcur->Name,(LPTSTR)Quote4DisplayIfNeeded(OLDname),MAXOBJECTNAME);
           fstrncpy(pcur->OwnerName,OLDowner,MAXOBJECTNAME);
           FreeText();
           pcur=GetNextLLData(pcur);
           if (!pcur) 
             iresselect = RES_ERR;
           i++;
        }
      break;
    case OT_SCHEMAUSER:
      {
        if (GetOIVers() == OIVERS_NOTOI)
            bGenericGateway = TRUE;

        if (bGenericGateway) {
            EXEC SQL repeated SELECT table_owner
            INTO :singlename
            FROM iitables
                UNION SELECT table_owner 
                FROM iiviews;
            EXEC SQL BEGIN;
              ManageSqlError(&sqlca); // Keep track of the SQL error if any
			if (HasSelectLoopInterruptBeenRequested()) {
				iresselect = RES_ERR;
				exec sql endselect;
			}
              suppspace(singlename);
              fstrncpy(pcur->Name,singlename,MAXOBJECTNAME);
              ZEROINIT (pcur->OwnerName); //fstrncpy(pcur->OwnerName,_T(""),MAXOBJECTNAME);
              pcur=GetNextLLData(pcur);
              if (!pcur) {
                iresselect = RES_ERR;
                EXEC SQL ENDSELECT;
                ManageSqlError(&sqlca); // Keep track of the SQL error if any
              }
              i++;
            EXEC SQL END;
            ManageSqlError(&sqlca); // Keep track of the SQL error if any
            break;
        }
        else{
            exec sql repeated select schema_name, schema_owner
                     into   :singlename, :singleownername
                     from iischema  ;

            exec sql begin;
              ManageSqlError(&sqlca); // Keep track of the SQL error if any
			if (HasSelectLoopInterruptBeenRequested()) {
				iresselect = RES_ERR;
				exec sql endselect;
			}
              fstrncpy(pcur->Name,singlename,MAXOBJECTNAME);
              fstrncpy(pcur->OwnerName,singleownername,MAXOBJECTNAME);
              pcur=GetNextLLData(pcur);
              if (!pcur) {
                iresselect = RES_ERR;
                exec sql endselect;
                ManageSqlError(&sqlca); // Keep track of the SQL error if any
              }
              i++;
            exec sql end;
            ManageSqlError(&sqlca); // Keep track of the SQL error if any
          }
          break;
      }
    case OT_DBEVENT:
      {
        exec sql repeated select event_name, event_owner
                 into   :singlename,:singleownername
                 from iievents
                 where text_sequence=1 ;

        exec sql begin;
          ManageSqlError(&sqlca); // Keep track of the SQL error if any
		  if (HasSelectLoopInterruptBeenRequested()) {
				iresselect = RES_ERR;
			    exec sql endselect;
		  }
          suppspace(singlename);
          fstrncpy(pcur->Name,(LPTSTR)Quote4DisplayIfNeeded(singlename),MAXOBJECTNAME);
          fstrncpy(pcur->OwnerName,singleownername,MAXOBJECTNAME);
          pcur=GetNextLLData(pcur);
          if (!pcur) {
            iresselect = RES_ERR;
            exec sql endselect;
            ManageSqlError(&sqlca); // Keep track of the SQL error if any
          }
          i++;
        exec sql end;
        ManageSqlError(&sqlca); // Keep track of the SQL error if any
      }
      break;
    case OT_SEQUENCE:
      {
        exec sql repeated select seq_name, seq_owner
                 into   :singlename,:singleownername
                 from iisequences;

        exec sql begin;
          ManageSqlError(&sqlca); // Keep track of the SQL error if any
		  if (HasSelectLoopInterruptBeenRequested()) {
				iresselect = RES_ERR;
			    exec sql endselect;
		  }
          suppspace(singlename);
          fstrncpy(pcur->Name,(LPTSTR)Quote4DisplayIfNeeded(singlename),MAXOBJECTNAME);
          fstrncpy(pcur->OwnerName,singleownername,MAXOBJECTNAME);
          pcur=GetNextLLData(pcur);
          if (!pcur) {
            iresselect = RES_ERR;
            exec sql endselect;
            ManageSqlError(&sqlca); // Keep track of the SQL error if any
          }
          i++;
        exec sql end;
        ManageSqlError(&sqlca); // Keep track of the SQL error if any
      }
      break;
    case OT_SYNONYM:
      {
           exec sql repeated select synonym_name,synonym_owner,   table_name, table_owner
                    into   :singlename, :singleownername,:extradata, :name3
                    from iisynonyms;

           exec sql begin;
             ManageSqlError(&sqlca); // Keep track of the SQL error if any
             if (HasSelectLoopInterruptBeenRequested()) {
               iresselect = RES_ERR;
               exec sql endselect;
             }
             suppspace(name3);
             suppspace(extradata);
             suppspace(singlename);
             fstrncpy(pcur->Name,(LPTSTR)Quote4DisplayIfNeeded(singlename),MAXOBJECTNAME);
             fstrncpy(pcur->OwnerName,singleownername,MAXOBJECTNAME);
             sprintf(pcur->ExtraData,"%s.%s",(LPTSTR)Quote4DisplayIfNeeded(name3),
                                             (LPTSTR)Quote4DisplayIfNeeded(extradata));
             bsuppspace=TRUE;
             pcur=GetNextLLData(pcur);
             if (!pcur) {
               iresselect = RES_ERR;
               exec sql endselect;
               ManageSqlError(&sqlca); // Keep track of the SQL error if any
             }
             i++;
           exec sql end;
           ManageSqlError(&sqlca); // Keep track of the SQL error if any
      }
      break;
    case OT_INTEGRITY:
      {
        if (bHasDot) {
           sprintf(selectstatement,"table_name='%s' and table_owner='%s'",
                   RemoveDisplayQuotesIfAny(ParentWhenDot),RemoveDisplayQuotesIfAny(ParentOwnerWhenDot));
        }
        else { 
           sprintf(selectstatement,"table_name='%s'",
                   RemoveDisplayQuotesIfAny(parentstrings[1]));
        }
        x_strcat(selectstatement," and text_sequence=1");

        exec sql select integrity_number,text_segment
                 into   :intno,          :extradata
                 from iiintegrities
                 where :selectstatement; 

        exec sql begin;
          ManageSqlError(&sqlca); // Keep track of the SQL error if any
          if (HasSelectLoopInterruptBeenRequested()) {
            iresselect = RES_ERR;
            exec sql endselect;
          }
          wsprintf(pcur->Name,"%d ... %s",intno,
                   getsubstring(extradata," is ",24));
          pcur->OwnerName[0]='\0'; // no Owner for integrity.
          storeint(pcur->ExtraData,intno);
          pcur=GetNextLLData(pcur);
          if (!pcur) {
            iresselect = RES_ERR;
            exec sql endselect;
            ManageSqlError(&sqlca); // Keep track of the SQL error if any
          }
          i++;
        exec sql end;
        ManageSqlError(&sqlca); // Keep track of the SQL error if any
      }
      break;
    case OT_VIEWCOLUMN:
    case OT_COLUMN:
      {
           if (bHasDot) {
              sprintf(selectstatement,"table_name='%s' and table_owner='%s'",
                    RemoveDisplayQuotesIfAny(ParentWhenDot),RemoveDisplayQuotesIfAny(ParentOwnerWhenDot));
           }
           else {
              sprintf(selectstatement,"table_name='%s'",
                      RemoveDisplayQuotesIfAny(parentstrings[1]));
           }

           exec sql select column_name, column_datatype, column_length,
                            column_nulls, column_defaults, column_sequence
                    into   :singlename, :singleownername,:intno,
                            :rt1,         :rt2,            :intno2
                    from iicolumns
                    where :selectstatement
                    order by column_sequence;

           exec sql begin;
             ManageSqlError(&sqlca); // Keep track of the SQL error if any
			if (HasSelectLoopInterruptBeenRequested()) {
				iresselect = RES_ERR;
				exec sql endselect;
			}
             fstrncpy(pcur->Name,singlename,MAXOBJECTNAME);
             storeint(pcur->ExtraData,
                      IngGetStringType(singleownername, intno));
             storeint(pcur->ExtraData+STEPSMALLOBJ,intno);
             pcur->ExtraData[2*STEPSMALLOBJ  ]=GetYNState(rt1);
             pcur->ExtraData[2*STEPSMALLOBJ+1]=GetYNState(rt2);
             storeint(pcur->ExtraData+4*STEPSMALLOBJ ,intno2);
             pcur=GetNextLLData(pcur);
             if (!pcur) {
               iresselect = RES_ERR;
               exec sql endselect;
               ManageSqlError(&sqlca); // Keep track of the SQL error if any
             }
             i++;
           exec sql end;
           ManageSqlError(&sqlca); // Keep track of the SQL error if any
      }
      break;
    case OT_REPLIC_CONNECTION:
      {
        exec sql repeated select database_no, node_name,  database_name,
                         server_role, target_type
                  into   :intno,      :singlename,:singleownername,
                         :intno2,     :intno3
                  from dd_connections;
        exec sql begin;
          ManageSqlError(&sqlca); // Keep track of the SQL error if any
			if (HasSelectLoopInterruptBeenRequested()) {
				iresselect = RES_ERR;
				exec sql endselect;
			}
          fstrncpy(pcur->Name,singlename,MAXOBJECTNAME);
          fstrncpy(pcur->OwnerName,singleownername,MAXOBJECTNAME);
          storeint(pcur->ExtraData,                  intno );
          storeint(pcur->ExtraData+STEPSMALLOBJ,     intno2);
          storeint(pcur->ExtraData+(2*STEPSMALLOBJ), intno3);
          pcur=GetNextLLData(pcur);
          if (!pcur) {
            iresselect = RES_ERR;
            exec sql endselect;
            ManageSqlError(&sqlca); // Keep track of the SQL error if any
          }
          i++;
        exec sql end;
        ManageSqlError(&sqlca); // Keep track of the SQL error if any
      }
      break;
    case OT_REPLIC_REGTABLE:
      {
        exec sql repeated select tablename,   dd_routing, tables_created,
                         columns_registered, rules_created
                  into   :singlename, :intno,     :rt1:null1,
                         :rt2:null2,  :rt3:null3
                  from dd_registered_tables;
        exec sql begin;
          ManageSqlError(&sqlca); // Keep track of the SQL error if any
			if (HasSelectLoopInterruptBeenRequested()) {
				iresselect = RES_ERR;
				exec sql endselect;
			}
          if (null1==-1)
            rt1[0]='N';
          if (null2==-1)
            rt2[0]='N';
          if (null3==-1)
            rt3[0]='N';
          fstrncpy(pcur->Name,singlename,MAXOBJECTNAME);
          storeint(pcur->ExtraData,                  intno );
          storeint(pcur->ExtraData+STEPSMALLOBJ,    (BOOL)(rt1[0]=='Y'));
          storeint(pcur->ExtraData+(2*STEPSMALLOBJ),(BOOL)(rt2[0]=='Y'));
          storeint(pcur->ExtraData+(3*STEPSMALLOBJ),(BOOL)(rt3[0]=='Y'));
          pcur=GetNextLLData(pcur);
          if (!pcur) {
            iresselect = RES_ERR;
            exec sql endselect;
            ManageSqlError(&sqlca); // Keep track of the SQL error if any
          }
          i++;
        exec sql end;
        ManageSqlError(&sqlca); // Keep track of the SQL error if any
      }
      break;
    case OT_REPLIC_REGTABLE_V11:
      {
        exec sql repeated select table_name   , cdds_no, columns_registered,
                         rules_created, prio_lookup_table ,table_no,
                         table_owner
                  into   :singlename  , :intno   , :name3, 
                         :bufnew1     , :bufnew2 , :intno2,
                         :bufnew3
                  from dd_regist_tables;
        exec sql begin;
          ManageSqlError(&sqlca); // Keep track of the SQL error if any
			if (HasSelectLoopInterruptBeenRequested()) {
				iresselect = RES_ERR;
				exec sql endselect;
			}
          suppspacebutleavefirst(singlename);
          fstrncpy(pcur->Name,(LPTSTR)Quote4DisplayIfNeeded(singlename),MAXOBJECTNAME);
          fstrncpy(pcur->OwnerName,bufnew3,MAXOBJECTNAME);
          storeint(pcur->ExtraData,                  intno );
          // to be finished 
          //fstrncpy(pcur->ExtraData+STEPSMALLOBJ, name3 ,MAXOBJECTNAME);
          //fstrncpy(pcur->ExtraData+STEPSMALLOBJ+MAXOBJECTNAME, bufnew1 ,MAXOBJECTNAME);
          //fstrncpy(pcur->ExtraData+STEPSMALLOBJ+(2*MAXOBJECTNAME), bufnew2 ,MAXOBJECTNAME);
          //storeint(pcur->ExtraData+STEPSMALLOBJ+(3*MAXOBJECTNAME), intno2  );

          pcur=GetNextLLData(pcur);
          if (!pcur) {
            iresselect = RES_ERR;
            exec sql endselect;
            ManageSqlError(&sqlca); // Keep track of the SQL error if any
          }
          i++;
        exec sql end;
        ManageSqlError(&sqlca); // Keep track of the SQL error if any
      }
      break;
    case OT_REPLIC_CDDS :
    case OTLL_REPLIC_CDDS:
      {
        exec sql repeated select dd_routing, localdb,   source,  target
                  into   :intno,     :intno2, :intno3, :intno4
                  from dd_distribution;
        exec sql begin;
          ManageSqlError(&sqlca); // Keep track of the SQL error if any
			if (HasSelectLoopInterruptBeenRequested()) {
				iresselect = RES_ERR;
				exec sql endselect;
			}
          storeint(pcur->ExtraData,                  intno );
          storeint(pcur->ExtraData+STEPSMALLOBJ,     intno2);
          storeint(pcur->ExtraData+(2*STEPSMALLOBJ), intno3);
          storeint(pcur->ExtraData+(3*STEPSMALLOBJ), intno4);
          pcur=GetNextLLData(pcur);
          if (!pcur) {
            iresselect = RES_ERR;
            exec sql endselect;
            ManageSqlError(&sqlca); // Keep track of the SQL error if any
          }
          i++;
        exec sql end;
        ManageSqlError(&sqlca); // Keep track of the SQL error if any
      }
      break;
      case OT_REPLIC_CDDS_V11:
      case OTLL_REPLIC_CDDS_V11:
      {
        exec sql repeated select a.cdds_no ,b.localdb , b.sourcedb,b.targetdb
              into   :intno,     :intno2, :intno3, :intno4
              from dd_cdds a,dd_paths b
              where a.cdds_no = b.cdds_no
        union select cdds_no,-1,-1,-1
                from dd_cdds
                where cdds_no not in (select cdds_no from dd_paths);
        exec sql begin;
          ManageSqlError(&sqlca); // Keep track of the SQL error if any
			if (HasSelectLoopInterruptBeenRequested()) {
				iresselect = RES_ERR;
				exec sql endselect;
			}
          storeint(pcur->ExtraData,                  intno );
          storeint(pcur->ExtraData+STEPSMALLOBJ,     intno2);
          storeint(pcur->ExtraData+(2*STEPSMALLOBJ), intno3);
          storeint(pcur->ExtraData+(3*STEPSMALLOBJ), intno4);
          pcur=GetNextLLData(pcur);
          if (!pcur) {
            iresselect = RES_ERR;
            exec sql endselect;
            ManageSqlError(&sqlca); // Keep track of the SQL error if any
          }
          i++;
        exec sql end;
        ManageSqlError(&sqlca); // Keep track of the SQL error if any
      }
      break;
// JFS Begin
    case OT_REPLIC_DISTRIBUTION:
      {
        exec sql repeated select dd_routing, localdb,   source,  target
                  into   :intno,     :intno2, :intno3, :intno4
                  from dd_distribution;
        exec sql begin;
          ManageSqlError(&sqlca); // Keep track of the SQL error if any
			if (HasSelectLoopInterruptBeenRequested()) {
				iresselect = RES_ERR;
				exec sql endselect;
			}
          storeint(pcur->ExtraData,                  intno );
          storeint(pcur->ExtraData+STEPSMALLOBJ,     intno2);
          storeint(pcur->ExtraData+(2*STEPSMALLOBJ), intno3);
          storeint(pcur->ExtraData+(3*STEPSMALLOBJ), intno4);
          pcur=GetNextLLData(pcur);
          if (!pcur) {
            iresselect = RES_ERR;
            exec sql endselect;
            ManageSqlError(&sqlca); // Keep track of the SQL error if any
          }
          i++;
        exec sql end;
        ManageSqlError(&sqlca); // Keep track of the SQL error if any
      }
      break;
   case OT_REPLIC_DISTRIBUTION_V11:
      {
        if ( CreateDD_SERVER_SPECIALIfNotExist () == RES_ERR)
            break;

        //exec sql select count(*) into :intno from iitables where table_name = 'dd_server_special';
        //if (intno == 0) {
        //    exec sql create table dd_server_special (cdds_no smallint ,localdb smallint , targetdb smallint ,vnode_name char (32));
        //    if (sqlca.sqlcode < 0)  { 
        //        ManageSqlError(&sqlca); // Keep track of the SQL error if any
        //        break;
        //    }
        //}
        //exec sql commit;

        exec sql repeated select a.cdds_no, a.localdb,  a.sourcedb,  a.targetdb, c.vnode_name
                  into   :intno,    :intno2,    :intno3,     :intno4,     :bufnew2:null1
                  from   ( (dd_paths a left join dd_db_cdds b on a.targetdb=b.database_no and a.cdds_no=b.cdds_no)
                          left join dd_server_special c 
                          on b.server_no= c.server_no and a.localdb=c.localdb
                         );
        exec sql begin;
          ManageSqlError(&sqlca); // Keep track of the SQL error if any
			if (HasSelectLoopInterruptBeenRequested()) {
				iresselect = RES_ERR;
				exec sql endselect;
			}
          if (null1==-1)
             bufnew2[0]='\0';
          suppspace(bufnew2);
          x_strcpy(pcur->Name,bufnew2);
          storeint(pcur->ExtraData,                  intno );
          storeint(pcur->ExtraData+STEPSMALLOBJ,     intno2);
          storeint(pcur->ExtraData+(2*STEPSMALLOBJ), intno3);
          storeint(pcur->ExtraData+(3*STEPSMALLOBJ), intno4);
          pcur=GetNextLLData(pcur);
          if (!pcur) {
            iresselect = RES_ERR;
            exec sql endselect;
            ManageSqlError(&sqlca); // Keep track of the SQL error if any
          }
          i++;
        exec sql end;
        ManageSqlError(&sqlca); // Keep track of the SQL error if any
      }
      break;

   case OT_REPLIC_REGCOLS:
       {
        x_strcpy(singleownername,lpownername);

        exec sql repeated select column_name, key_column ,replicated_column
                  into   :singlename,:rt1,:rt2 
                  from dd_registered_columns
                  where tablename =:singleownername;

        exec sql begin;
          ManageSqlError(&sqlca); // Keep track of the SQL error if any
          if (HasSelectLoopInterruptBeenRequested()) {
            iresselect = RES_ERR;
            exec sql endselect;
          }
          fstrncpy(pcur->Name,singlename,MAXOBJECTNAME);
          fstrncpy(pcur->ExtraData,rt1,2);
          fstrncpy(pcur->ExtraData+2,rt2,2);
          pcur=GetNextLLData(pcur);
          if (!pcur) {
            iresselect = RES_ERR;
            exec sql endselect;
            ManageSqlError(&sqlca); // Keep track of the SQL error if any
          }
          i++;
        exec sql end;
        ManageSqlError(&sqlca); // Keep track of the SQL error if any
       }
       break;
   case OT_REPLIC_REGCOLS_V11:
       {
        if (bHasDot) {
           fstrncpy(bufnew1,(LPUCHAR)RemoveDisplayQuotesIfAny(ParentWhenDot),     MAXOBJECTNAME);
           fstrncpy(bufnew2,(LPUCHAR)RemoveDisplayQuotesIfAny(ParentOwnerWhenDot),MAXOBJECTNAME);
        }
        else {
          myerror(ERR_PARENTNOEXIST);
          iresselect = RES_ERR;
          break;
        }

        exec sql repeated select c.column_name, c.key_sequence,c.column_sequence
                  into   :singlename,:intno,:intno3 
                  from dd_regist_columns c , dd_regist_tables t 
                  where t.table_name = :bufnew1 and t.table_owner = :bufnew2 and
                  c.table_no = t.table_no ;

        exec sql begin;
          ManageSqlError(&sqlca); // Keep track of the SQL error if any
			if (HasSelectLoopInterruptBeenRequested()) {
				iresselect = RES_ERR;
				exec sql endselect;
			}
          fstrncpy(pcur->Name,singlename,MAXOBJECTNAME);
          storeint(pcur->ExtraData,    intno );
          storeint(pcur->ExtraData+STEPSMALLOBJ, intno3);
          
          pcur=GetNextLLData(pcur);
          if (!pcur) {
            iresselect = RES_ERR;
            exec sql endselect;
            ManageSqlError(&sqlca); // Keep track of the SQL error if any
          }
          i++;
        exec sql end;
        ManageSqlError(&sqlca); // Keep track of the SQL error if any
       }
       break;
      
   case OT_REPLIC_REGTBLS:
       {
        i=0;
        exec sql repeated select tablename, tables_created,columns_registered ,
                  rules_created,rule_lookup_table,priority_lookup_table,
                  dd_routing

                  into   :singlename,:rt1:null1,:rt2:null2,
                  :rt3:null3,:singleownername:null4,:name3:null5,
                  :intno
                  from dd_registered_tables;

        exec sql begin;
          ManageSqlError(&sqlca); // Keep track of the SQL error if any
			if (HasSelectLoopInterruptBeenRequested()) {
				iresselect = RES_ERR;
				exec sql endselect;
			}

          fstrncpy(pcur->Name,singlename,MAXOBJECTNAME);

          if (null4 == -1)
             *pcur->OwnerName=0;
          else
             fstrncpy(pcur->OwnerName,singleownername,MAXOBJECTNAME);

          if (null5 == -1)
             *pcur->ExtraData=0;
          else
             fstrncpy(pcur->ExtraData,name3,MAXOBJECTNAME);

          suppspace(pcur->ExtraData);

          storeint(pcur->ExtraData+MAXOBJECTNAME,    intno );

          storeint(pcur->ExtraData+MAXOBJECTNAME+(STEPSMALLOBJ),   (null1 == -1) ? 0 : rt1[0]);
          storeint(pcur->ExtraData+MAXOBJECTNAME+(2*STEPSMALLOBJ), (null2 == -1) ? 0 : rt2[0]);
          storeint(pcur->ExtraData+MAXOBJECTNAME+(3*STEPSMALLOBJ), (null3 == -1) ? 0 : rt3[0]);
          pcur=GetNextLLData(pcur);
          if (!pcur) {
            iresselect = RES_ERR;
            exec sql endselect;
            ManageSqlError(&sqlca); // Keep track of the SQL error if any
          }
          i++;
        exec sql end;
        ManageSqlError(&sqlca); // Keep track of the SQL error if any
       }
       break;
       case OT_REPLIC_REGTBLS_V11:
       {
        i=0;
        exec sql  repeated select table_no   , table_name         , table_owner,
                  columns_registered, supp_objs_created    , rules_created,
                  cdds_no           , cdds_lookup_table  , prio_lookup_table,
                  index_used
                  into   :intno     , :singlename        , :bufnew1,
                         :bufev1    , :bufev2            , :bufnew2, 
                         :intno2    , :singleownername   , :name3  ,
                         :bufnew3
                  from dd_regist_tables;

        exec sql begin;
          ManageSqlError(&sqlca); // Keep track of the SQL error if any
			if (HasSelectLoopInterruptBeenRequested()) {
				iresselect = RES_ERR;
				exec sql endselect;
			}
          // table_no
          pcur->uData.Registred_Tables.table_no = intno;

          // table_name
          suppspace(singlename);
          x_strcpy(pcur->uData.Registred_Tables.tablename,singlename);

          
          // columns_registred
          suppspace(bufev1);
          pcur->uData.Registred_Tables.colums_registered[0] = ( bufev1[0] == 0 ) ? 0 : 0x0043; // 'C'
          pcur->uData.Registred_Tables.colums_registered[1] = 0;

          // supp_objs_created new for replicator 1.1
          suppspace(bufev2);
          pcur->uData.Registred_Tables.table_created[0]     = ( bufev2[0] == 0 ) ? 0 : 0x0054; // 'T';
          pcur->uData.Registred_Tables.table_created_ini[0] = pcur->uData.Registred_Tables.table_created[0];
          pcur->uData.Registred_Tables.table_created[1]     = 0;
          pcur->uData.Registred_Tables.table_created_ini[1] = 0;
          x_strcpy(pcur->uData.Registred_Tables.szDate_table_created,bufev2);              

          //rules_created 
          suppspace(bufnew2);
          pcur->uData.Registred_Tables.rules_created[0]     = ( bufnew2[0] == 0 )? 0 : 0x0052; // 'R'
          pcur->uData.Registred_Tables.rules_created_ini[0] = pcur->uData.Registred_Tables.rules_created[0];
          pcur->uData.Registred_Tables.rules_created[1]     = 0;
          pcur->uData.Registred_Tables.rules_created_ini[1] = 0;
          x_strcpy(pcur->uData.Registred_Tables.szDate_rules_created,bufnew2);
          
          // cdds_no
          pcur->uData.Registred_Tables.cdds = intno2;

          suppspace(bufnew1);
          x_strcpy(pcur->uData.Registred_Tables.tableowner,bufnew1);
          
          suppspace(singleownername);
          x_strcpy(pcur->uData.Registred_Tables.cdds_lookup_table_v11,singleownername);

          // prio_lookup_table
          suppspace(name3);
          x_strcpy(pcur->uData.Registred_Tables.priority_lookup_table_v11,name3);
          
          // index_used
          suppspace(bufnew3);
          x_strcpy(pcur->uData.Registred_Tables.IndexName,bufnew3);

          pcur=GetNextLLData(pcur);
          if (!pcur) {
            iresselect = RES_ERR;
            exec sql endselect;
            ManageSqlError(&sqlca); // Keep track of the SQL error if any
          }
          i++;
        exec sql end;
        ManageSqlError(&sqlca); // Keep track of the SQL error if any
       }
       break;
// JFS End

    case OT_REPLIC_MAILUSER:
    case OT_REPLIC_MAILUSER_V11:
      {
        exec sql repeated select mail_username
                  into   :extradata
                  from dd_mail_alert;
        exec sql begin;
          ManageSqlError(&sqlca); // Keep track of the SQL error if any
			if (HasSelectLoopInterruptBeenRequested()) {
				iresselect = RES_ERR;
				exec sql endselect;
			}
          /* extradata is used because it's length is > 80. Truncated then */
          /* to dispaly length. Use GetDetailInfos for the complete name   */
          fstrncpy(pcur->Name,extradata,MAXOBJECTNAME);
          pcur=GetNextLLData(pcur);
          if (!pcur) {
            iresselect = RES_ERR;
            exec sql endselect;
            ManageSqlError(&sqlca); // Keep track of the SQL error if any
          }
          i++;
        exec sql end;
        ManageSqlError(&sqlca); // Keep track of the SQL error if any
      }
      break;
    case OT_REPLIC_DBMS_TYPE_V11:
      {
        exec sql repeated select dbms_type, short_description , gateway
                  into   :singlename, :extradata ,:intno
                  from dd_dbms_types;
        exec sql begin;
          ManageSqlError(&sqlca); // Keep track of the SQL error if any
			if (HasSelectLoopInterruptBeenRequested()) {
				iresselect = RES_ERR;
				exec sql endselect;
			}
          fstrncpy(pcur->Name,singlename,MAXOBJECTNAME);
          fstrncpy(pcur->ExtraData,extradata,MAXOBJECTNAME);
          //storeint(pcur->ExtraData,intno);
          pcur=GetNextLLData(pcur);
          if (!pcur) {
            iresselect = RES_ERR;
            exec sql endselect;
            ManageSqlError(&sqlca); // Keep track of the SQL error if any
          }
          i++;
        exec sql end;
        ManageSqlError(&sqlca); // Keep track of the SQL error if any
      }
      break;
    case OT_REPLIC_CONNECTION_V11:
    {
        exec sql repeated select database_no, vnode_name, database_name, dbms_type,
                         local_db , remark
                  into   :intno   , :singlename , :singleownername,:bufev1,
                         :intno3  , :selectstatement
                  from dd_databases;

        exec sql begin;
        ManageSqlError(&sqlca); // Keep track of the SQL error if any
			if (HasSelectLoopInterruptBeenRequested()) {
				iresselect = RES_ERR;
				exec sql endselect;
			}
          fstrncpy(pcur->Name,singlename,MAXOBJECTNAME);
          fstrncpy(pcur->OwnerName,singleownername,MAXOBJECTNAME);
          storeint(pcur->ExtraData,                  intno );
//          storeint(pcur->ExtraData+STEPSMALLOBJ,     intno2);  // removed 20-Jan-2000: server no nore attached at that level for replicator 1.1 and later
          storeint(pcur->ExtraData+(2*STEPSMALLOBJ), intno3);
          fstrncpy(pcur->ExtraData+(3*STEPSMALLOBJ), bufev1,(MAXOBJECTNAME/2));
          //fstrncpy(pcur->ExtraData+(3*STEPSMALLOBJ)+(MAXOBJECTNAME/2), selectstatement,80); // 80 Len of field Remark 
          // to be finished
        pcur=GetNextLLData(pcur);
        if (!pcur) {
          iresselect = RES_ERR;
          exec sql endselect;
          ManageSqlError(&sqlca); // Keep track of the SQL error if any
        }
        i++;
        exec sql end;
        ManageSqlError(&sqlca); // Keep track of the SQL error if any

    }
    break;
    case OT_REPLIC_CDDSDBINFO_V11:
    {
        intno6 = atoi(parentstrings[1]);
        // value when replic type is not define.
        intno5=REPLIC_TYPE_UNDEFINED;
        // value when replic server number is not define.
        intno4=-1; 
        exec sql repeated select a.database_no, a.vnode_name,  a.database_name,
                        b.target_type, b.server_no ,  b.cdds_no,a.database_owner
                 into   :intno       , :singlename, :singleownername    ,
                        :intno2      , :intno3,     :intno6,   :bufev1
                 from dd_db_cdds b ,dd_databases a
                 where a.database_no=b.database_no
                 union
                  select database_no,vnode_name , database_name , :intno5, :intno4,(-1),database_owner
                  from  dd_databases
                  where database_no not in (select database_no from dd_db_cdds where cdds_no = :intno6);

        exec sql begin;
        ManageSqlError(&sqlca); // Keep track of the SQL error if any
          if (HasSelectLoopInterruptBeenRequested()) {
            iresselect = RES_ERR;
            exec sql endselect;
          }
          suppspace(bufev1);
          fstrncpy(pcur->Name,singlename,MAXOBJECTNAME);
          fstrncpy(pcur->OwnerName,singleownername,MAXOBJECTNAME);
          storeint(pcur->ExtraData,                  intno );
          storeint(pcur->ExtraData+STEPSMALLOBJ,     intno2);
          storeint(pcur->ExtraData+(2*STEPSMALLOBJ), intno3);
          storeint(pcur->ExtraData+(3*STEPSMALLOBJ), intno6);
          fstrncpy(pcur->ExtraData+(4*STEPSMALLOBJ),bufev1,MAXOBJECTNAME);

        pcur=GetNextLLData(pcur);
        if (!pcur) {
          iresselect = RES_ERR;
          exec sql endselect;
          ManageSqlError(&sqlca); // Keep track of the SQL error if any
        }
        i++;
        exec sql end;
        ManageSqlError(&sqlca); // Keep track of the SQL error if any

    }
    break;


    case OTLL_DBGRANTEE:
      {
/*        exec sql select grantee_name,database_name,gr_type,db_access,cr_tab,cr_proc,   */
/*                     db_admin, lk_mode, qry_io, qry_row, up_syscat, qry_io_lim,        */
/*                     qry_row_lim,                                                      */
/*                     sel_syscat, tbl_stats, idle_time,conn_time, sess_prio,            */
/*                     idle_time_lim, conn_time_lim, sess_prio_lim                       */
/*                 into   :singlename,:singleownername,:rt1, :rt2,     :rt3,  :rt4,      */
/*                     :rt5,     :rt6,    :rt7,   :rt8,    :rt9,      :intno,            */
/*                     :intno2,                                                          */
/*                     :rt10,     :rt11,    :rt12,   :rt13,    :rt14,                    */
/*                     :intno3,   :intno4,  :intno5                                      */
/*                 from iidbprivileges ;                                                 */
         exec sql repeated select grantee,dbname,char(charextract(' UGRP', (gtype+2))),
                         char(charextract('UNY',(mod((control/2048),(2))+ mod((flags  /2048),(2))+1))), /*dbaccess */
                         char(charextract('UNY',(mod((control/256 ),(2))+ mod((flags  /256 ),(2))+1))), /*cr_tab   */
                         char(charextract('UNY',(mod((control/512 ),(2))+ mod((flags  /512 ),(2))+1))), /*cr_proc  */
                         char(charextract('UNY',(mod((control/8192),(2))+ mod((flags  /8192),(2))+1))), /*db_admin */
                         char(charextract('UNY',(mod((control/1024),(2))+ mod((flags  /1024),(2))+1))), /*lk_mode  */
                         char(charextract('UNY',(mod((control/2   ),(2))+ mod((flags  /2   ),(2))+1))), /*qry_io   */
                         char(charextract('UNY',(mod((control     ),(2))+ mod((flags       ),(2))+1))), /*qry_row  */
                         char(charextract('UNY',(mod((control/4096),(2))+ mod((flags  /4096),(2))+1))), /*up_syscat*/
                         qdio_limit,
                         qrow_limit,
                         char(charextract('UNY',(mod((control/32768) ,(2))+mod((flags/32768 ),(2))+1))),/*sel_syscat*/
                         char(charextract('UNY',(mod((control/65536) ,(2))+mod((flags/65536 ),(2))+1))),/*tbl_stats */
                         char(charextract('UNY',(mod((control/131072),(2))+mod((flags/131072),(2))+1))),/*idle_time */
                         char(charextract('UNY',(mod((control/262144),(2))+mod((flags/262144),(2))+1))),/*conn_time */
                         char(charextract('UNY',(mod((control/524288),(2))+mod((flags/524288),(2))+1))),/*sess_prio */
                         idle_time_limit,
                         connect_time_limit,
                         priority_limit,
                         char(charextract('UNY',(mod((control/4     ),(2))+mod((flags/4     ),(2))+1))),/*qry_cpu   */
                         char(charextract('UNY',(mod((control/8     ),(2))+mod((flags/8     ),(2))+1))),/*qry_page  */
                         char(charextract('UNY',(mod((control/16    ),(2))+mod((flags/16    ),(2))+1))),/*qry_cost  */
                         qcpu_limit,
                         qpage_limit,
                         qcost_limit,
                         char(charextract('UNY',(mod((control/1048576),(2))+mod((flags/1048576    ),(2))+1)))/*cr_sequ   */

              into   :singlename,:singleownername,:rt1, :rt2,     :rt3,  :rt4,      
                     :rt5,     :rt6,    :rt7,   :rt8,    :rt9,      :intno,
                     :intno2,
                     :rt10,     :rt11,    :rt12,   :rt13,    :rt14,
                     :intno3,   :intno4,  :intno5 ,
                     :rt15,:rt16,:rt17,:intno6,:intno7,:intno8, :rt18
                 from iidbpriv ;

        exec sql begin;
          ManageSqlError(&sqlca); // Keep track of the SQL error if any
			if (HasSelectLoopInterruptBeenRequested()) {
				iresselect = RES_ERR;
				exec sql endselect;
			}
          suppspace(singlename);
          if (!x_stricmp(singlename,lppublicsysstring()))
            fstrncpy(singlename,lppublicdispstring(),MAXOBJECTNAME);
          if (GetYNState(rt2)==ATTACHED_YES) 
            if (!addgranteeobject(&i,singlename,singleownername,
               OT_DBGRANT_ACCESY_USER,intno)) return RES_ERR;
          if (GetYNState(rt2)==ATTACHED_NO) 
            if (!addgranteeobject(&i,singlename,singleownername,
               OT_DBGRANT_ACCESN_USER,intno)) return RES_ERR;
          if (GetYNState(rt3)==ATTACHED_YES) 
            if (!addgranteeobject(&i,singlename,singleownername,
               OT_DBGRANT_CRETBY_USER,intno)) return RES_ERR;
          if (GetYNState(rt3)==ATTACHED_NO) 
            if (!addgranteeobject(&i,singlename,singleownername,
               OT_DBGRANT_CRETBN_USER,intno)) return RES_ERR;
          if (GetYNState(rt4)==ATTACHED_YES) 
            if (!addgranteeobject(&i,singlename,singleownername,
               OT_DBGRANT_CREPRY_USER,intno)) return RES_ERR;
          if (GetYNState(rt4)==ATTACHED_NO) 
            if (!addgranteeobject(&i,singlename,singleownername,
               OT_DBGRANT_CREPRN_USER,intno)) return RES_ERR;
          if (GetYNState(rt5)==ATTACHED_YES) 
            if (!addgranteeobject(&i,singlename,singleownername,
               OT_DBGRANT_DBADMY_USER,intno)) return RES_ERR;
          if (GetYNState(rt5)==ATTACHED_NO) 
            if (!addgranteeobject(&i,singlename,singleownername,
               OT_DBGRANT_DBADMN_USER,intno)) return RES_ERR;
          if (GetYNState(rt6)==ATTACHED_YES) 
            if (!addgranteeobject(&i,singlename,singleownername,
               OT_DBGRANT_LKMODY_USER,intno)) return RES_ERR;
          if (GetYNState(rt6)==ATTACHED_NO) 
            if (!addgranteeobject(&i,singlename,singleownername,
               OT_DBGRANT_LKMODN_USER,intno)) return RES_ERR;
          if (GetYNState(rt7)==ATTACHED_YES) 
            if (!addgranteeobject(&i,singlename,singleownername,
               OT_DBGRANT_QRYIOY_USER,intno)) return RES_ERR;
          if (GetYNState(rt7)==ATTACHED_NO) 
            if (!addgranteeobject(&i,singlename,singleownername,
               OT_DBGRANT_QRYION_USER,intno)) return RES_ERR;
          if (GetYNState(rt8)==ATTACHED_YES) 
            if (!addgranteeobject(&i,singlename,singleownername,
               OT_DBGRANT_QRYRWY_USER,intno2)) return RES_ERR;
          if (GetYNState(rt8)==ATTACHED_NO) 
            if (!addgranteeobject(&i,singlename,singleownername,
               OT_DBGRANT_QRYRWN_USER,intno2)) return RES_ERR;
          if (GetYNState(rt9)==ATTACHED_YES) 
            if (!addgranteeobject(&i,singlename,singleownername,
               OT_DBGRANT_UPDSCY_USER,intno2)) return RES_ERR;
          if (GetYNState(rt9)==ATTACHED_NO) 
            if (!addgranteeobject(&i,singlename,singleownername,
               OT_DBGRANT_UPDSCN_USER,intno2)) return RES_ERR;
          if (GetYNState(rt10)==ATTACHED_YES) 
            if (!addgranteeobject(&i,singlename,singleownername,
               OT_DBGRANT_SELSCY_USER,intno2)) return RES_ERR;
          if (GetYNState(rt10)==ATTACHED_NO) 
            if (!addgranteeobject(&i,singlename,singleownername,
               OT_DBGRANT_SELSCN_USER,intno2)) return RES_ERR;
          if (GetYNState(rt11)==ATTACHED_YES) 
            if (!addgranteeobject(&i,singlename,singleownername,
               OT_DBGRANT_TBLSTY_USER,intno2)) return RES_ERR;
          if (GetYNState(rt11)==ATTACHED_NO) 
            if (!addgranteeobject(&i,singlename,singleownername,
               OT_DBGRANT_TBLSTN_USER,intno2)) return RES_ERR;
          if (GetYNState(rt12)==ATTACHED_YES) 
            if (!addgranteeobject(&i,singlename,singleownername,
               OT_DBGRANT_IDLTLY_USER,intno3)) return RES_ERR;
          if (GetYNState(rt12)==ATTACHED_NO) 
            if (!addgranteeobject(&i,singlename,singleownername,
               OT_DBGRANT_IDLTLN_USER,intno3)) return RES_ERR;
          if (GetYNState(rt13)==ATTACHED_YES) 
            if (!addgranteeobject(&i,singlename,singleownername,
               OT_DBGRANT_CNCTLY_USER,intno4)) return RES_ERR;
          if (GetYNState(rt13)==ATTACHED_NO) 
            if (!addgranteeobject(&i,singlename,singleownername,
               OT_DBGRANT_CNCTLN_USER,intno4)) return RES_ERR;
          if (GetYNState(rt14)==ATTACHED_YES) 
            if (!addgranteeobject(&i,singlename,singleownername,
               OT_DBGRANT_SESPRY_USER,intno5)) return RES_ERR;
          if (GetYNState(rt14)==ATTACHED_NO) 
            if (!addgranteeobject(&i,singlename,singleownername,
               OT_DBGRANT_SESPRN_USER,intno5)) return RES_ERR;
          if (GetYNState(rt15)==ATTACHED_YES)
            if (!addgranteeobject(&i,singlename,singleownername,
               OT_DBGRANT_QRYCPY_USER,intno6)) return RES_ERR;
          if (GetYNState(rt15)==ATTACHED_NO) 
            if (!addgranteeobject(&i,singlename,singleownername,
               OT_DBGRANT_QRYCPN_USER,intno6)) return RES_ERR;
          if (GetYNState(rt16)==ATTACHED_YES) 
            if (!addgranteeobject(&i,singlename,singleownername,
               OT_DBGRANT_QRYPGY_USER,intno7)) return RES_ERR;
          if (GetYNState(rt16)==ATTACHED_NO) 
            if (!addgranteeobject(&i,singlename,singleownername,
               OT_DBGRANT_QRYPGN_USER,intno7)) return RES_ERR;
          if (GetYNState(rt17)==ATTACHED_YES) 
            if (!addgranteeobject(&i,singlename,singleownername,
               OT_DBGRANT_QRYCOY_USER,intno8)) return RES_ERR;
          if (GetYNState(rt17)==ATTACHED_NO) 
            if (!addgranteeobject(&i,singlename,singleownername,
               OT_DBGRANT_QRYCON_USER,intno8)) return RES_ERR;
          if (GetYNState(rt18)==ATTACHED_YES) 
            if (!addgranteeobject(&i,singlename,singleownername,
               OT_DBGRANT_SEQCRY_USER,intno)) return RES_ERR;
          if (GetYNState(rt18)==ATTACHED_NO) 
            if (!addgranteeobject(&i,singlename,singleownername,
               OT_DBGRANT_SEQCRN_USER,intno)) return RES_ERR;
        exec sql end;
        ManageSqlError(&sqlca); // Keep track of the SQL error if any
      }
      break;
    case OTLL_OIDTDBGRANTEE:
      {
        exec sql repeated select NAME,       RESOURCEAUTH,    DBAAUTH
                 into   :singlename,:singleownername,:rt1
                 from  SYSADM.SYSUSERAUTH;

        exec sql begin;
          ManageSqlError(&sqlca); // Keep track of the SQL error if any
			if (HasSelectLoopInterruptBeenRequested()) {
				iresselect = RES_ERR;
				exec sql endselect;
			}
          suppspace(singlename);
          if (!addgranteeobject(&i,singlename,parentstrings[0],
             OT_DBGRANT_ACCESY_USER,0))   return RES_ERR;
          if (GetYNState(singleownername)==ATTACHED_YES) 
            if (!addgranteeobject(&i,singlename,parentstrings[0],
               OT_DBGRANT_CRETBY_USER,0)) return RES_ERR;
          if (GetYNState(rt1)==ATTACHED_YES) 
            if (!addgranteeobject(&i,singlename,parentstrings[0],
               OT_DBGRANT_DBADMY_USER,0)) return RES_ERR;
        exec sql end;
        ManageSqlError(&sqlca); // Keep track of the SQL error if any
      }
      break;
    case OT_VIEWTABLE:
      {
        BOOL bSqlView;
        if (GetOIVers() == OIVERS_NOTOI)
           bGenericGateway = TRUE;
        if (bHasDot) {
           sprintf(selectstatement,"table_name='%s' and table_owner='%s'",
                 RemoveDisplayQuotesIfAny(ParentWhenDot),RemoveDisplayQuotesIfAny(ParentOwnerWhenDot));
        }
        else {
           sprintf(selectstatement,"table_name='%s'",
                   RemoveDisplayQuotesIfAny(parentstrings[1]));
        }

        if (bGenericGateway) {
           // the length of the column "text_segment" will vary from gateway to
           // gateway.
           pTextSegment = NULL;
           intno2 = 0;
           // get length of column text_segment in iicolumns
           EXEC SQL repeated SELECT column_length 
                    INTO :intno2
                    FROM iicolumns 
                    WHERE (table_name = 'IIVIEWS'and column_name = 'TEXT_SEGMENT')
					      OR (table_name = 'iiviews'and column_name = 'text_segment');
           // get all text segment an concatenate them
           if ( sqlca.sqlcode != 0) {
              ManageSqlError(&sqlca); // Keep track of the SQL error if any
              break;
           }
           pTextSegment = ESL_AllocMem(intno2+1);
           if (!pTextSegment) {
                iret = RES_ERR; // error allocated memory
                break;
           }
           EXEC SQL SELECT text_segment, text_sequence,view_dml
                    INTO   :pTextSegment,   :intno,:name3
                    FROM iiviews
                    WHERE :selectstatement
                    ORDER BY text_sequence; 
           EXEC SQL BEGIN;
             ManageSqlError(&sqlca); // Keep track of the SQL error if any
			if (HasSelectLoopInterruptBeenRequested()) {
				iresselect = RES_ERR;
				exec sql endselect;
			}
             ourString2Lower(pTextSegment);
             if (!ConcatText(pTextSegment)) {
               iresselect = RES_ERR;
               EXEC SQL ENDSELECT;
             }
           EXEC SQL END;
           ManageSqlError(&sqlca); // Keep track of the SQL error if any
           if ( iresselect == RES_ERR ) { // error in ConcatText(pTextSegment)
              iret = RES_ERR;
              if (pTextSegment) { 
                  ESL_FreeMem(pTextSegment);
                  pTextSegment = NULL;
              }
              break;
           }
           singleownername[0]='\0';
           bSqlView = (name3[0] == _T('S') || name3[0] == _T('s')) ? TRUE : FALSE;
           if (bHasDot && !bSqlView)
             fstrncpy(singleownername,(LPUCHAR)RemoveDisplayQuotesIfAny(ParentOwnerWhenDot),MAXOBJECTNAME);
           while ((iret=GetViewTables(singlename,singleownername,bSqlView))==RES_SUCCESS){
             ourString2Upper(singlename);
             ourString2Upper(singleownername);
             if (!singleownername[0] && bHasDot)
                  fstrncpy(singleownername,(LPUCHAR)RemoveDisplayQuotesIfAny(ParentOwnerWhenDot),MAXOBJECTNAME);
             fstrncpy(pcur->Name,singlename,MAXOBJECTNAME);
             fstrncpy(pcur->OwnerName,singleownername,MAXOBJECTNAME);
             pcur=GetNextLLData(pcur);
             if (!pcur) {
               iresselect = RES_ERR;
               break;
             }
             i++;
           }
           FreeText();
           if (pTextSegment) { 
              ESL_FreeMem(pTextSegment);
              pTextSegment = NULL;
           }
           if (iret==RES_ERR)
               iresselect = RES_ERR;
        } // end bGenericGateway == TRUE 
        else {
            exec sql select text_segment, text_sequence,view_dml
                     into   :extradata,   :intno,:name3
                     from iiviews
                     where :selectstatement
                     order by text_sequence; 

            exec sql begin;
              ManageSqlError(&sqlca); // Keep track of the SQL error if any
			if (HasSelectLoopInterruptBeenRequested()) {
				iresselect = RES_ERR;
				exec sql endselect;
			}
              if (!ConcatText(extradata)) {
                iresselect = RES_ERR;
                exec sql endselect;
                ManageSqlError(&sqlca); // Keep track of the SQL error if any
              }
            exec sql end;
            ManageSqlError(&sqlca); // Keep track of the SQL error if any
            if ( iresselect == RES_ERR ) { // error in ConcatText(pTextSegment)
              iret = RES_ERR; 
              break;
           }

            singleownername[0]='\0';
            bSqlView = (name3[0] == _T('S') || name3[0] == _T('s')) ? TRUE : FALSE;
            if (bHasDot && !bSqlView)
               fstrncpy(singleownername,(LPUCHAR)RemoveDisplayQuotesIfAny(ParentOwnerWhenDot),MAXOBJECTNAME);
            while ((iret=GetViewTables(singlename,singleownername,bSqlView))==RES_SUCCESS){
              fstrncpy(pcur->Name,(LPTSTR)Quote4DisplayIfNeeded(singlename),MAXOBJECTNAME);
              fstrncpy(pcur->OwnerName,singleownername,MAXOBJECTNAME);
              pcur=GetNextLLData(pcur);
              if (!pcur) {
                iresselect = RES_ERR;
                break;
              }
              i++;
            }
            FreeText();
            if (iret==RES_ERR)
                iresselect = RES_ERR;
        }
      }
      break;
    case OTLL_DBSECALARM:
      {
        int j, Jmax;
        Jmax=sizeof(SecuritySet)/sizeof(SecuritySet[0]);

        exec sql repeated select object_name,    object_owner,    security_user,
                         security_number,text_segment,    
                         alarm_name,     dbevent_name,    dbevent_owner
                  into   :singlename,    :singleownername,:name3,
                         :intno,         :extradata,      
                         :selectstatement,:bufev1:snullind1,:bufev2:snullind2
                  from iisecurity_alarms ;

        exec sql begin;
           ManageSqlError(&sqlca); // Keep track of the SQL error if any
			if (HasSelectLoopInterruptBeenRequested()) {
				iresselect = RES_ERR;
				exec sql endselect;
			}
           ParseSecurityText(extradata, pSecuritySet, Jmax); /* Set security flags */   
           for (j=0;j<Jmax;j++) {
              if (SecuritySet[j].TypeOn) {
                 suppspace(singlename);
                 suppspace(singleownername);
                 StringWithOwner(singlename,singleownername,pcur->Name);
                 if (!snullind1 && !snullind2) {
                   suppspace(bufev1);
                   suppspace(bufev2);
                   StringWithOwner(bufev1,bufev2,pcur->OwnerName);
                 }
                 else
                   pcur->OwnerName[0]='\0';
                 if (isempty(name3))
                    fstrncpy(name3,lppublicdispstring(),MAXOBJECTNAME);
                 suppspace(name3);
                 fstrncpy(pcur->ExtraData,(LPTSTR)RemoveSQLQuotes(name3),
                     MAXOBJECTNAME);       /* User/Group/Role on which security applies */
                 suppspace(pcur->ExtraData);
                 storeint(pcur->ExtraData+MAXOBJECTNAME,
                     intno);               /* security number */
                 storeint(pcur->ExtraData+MAXOBJECTNAME+STEPSMALLOBJ,
                     SecuritySet[j].Value);
                 if (selectstatement[0]=='$')
                    selectstatement[0]='\0';
                 suppspace(selectstatement);
                 fstrncpy(pcur->ExtraData+(MAXOBJECTNAME+2*STEPSMALLOBJ),
                     selectstatement, MAXOBJECTNAMENOSCHEMA); /* name of sec.alarm*/
                 pcur=GetNextLLData(pcur);
                 if (!pcur) {
                   iresselect = RES_ERR;
                   exec sql endselect;
                   ManageSqlError(&sqlca); // Keep track of the SQL error if any
                 }
                 i++;
              }
           }
        exec sql end;
        ManageSqlError(&sqlca); // Keep track of the SQL error if any
      }
      break;
    case OTLL_SECURITYALARM:
      {
        int j, Jmax;
        Jmax=sizeof(SecuritySet)/sizeof(SecuritySet[0]);

        exec sql repeated select object_name,    object_owner,    security_user,
                         security_number,text_segment,
                         alarm_name,     dbevent_name,    dbevent_owner
                  into   :singlename,    :singleownername,:name3,
                         :intno,         :extradata,
                         :selectstatement,:bufev1:snullind1,:bufev2:snullind2
                  from iisecurity_alarms;

        exec sql begin;
           ManageSqlError(&sqlca); // Keep track of the SQL error if any
           if (HasSelectLoopInterruptBeenRequested()) {
              iresselect = RES_ERR;
              exec sql endselect;
           }
           ParseSecurityText(extradata, pSecuritySet, Jmax); /* Set security flags */   
           for (j=0;j<Jmax;j++) {
              if (SecuritySet[j].TypeOn) {
                 suppspace(singlename);
                 suppspace(singleownername);
                 assert((x_strlen(Quote4DisplayIfNeeded(singlename))+x_strlen(Quote4DisplayIfNeeded(singleownername))) < sizeof(pcur->Name));
                 StringWithOwner((LPUCHAR)Quote4DisplayIfNeeded(singlename),
                                  singleownername,pcur->Name);
                 if (!snullind1 && !snullind2) {
                   suppspace(bufev1);
                   suppspace(bufev2);
                   StringWithOwner((LPUCHAR)Quote4DisplayIfNeeded(bufev1),bufev2,pcur->OwnerName);
                 }
                 else
                   pcur->OwnerName[0]='\0';
                 if (isempty(name3))
                    fstrncpy(name3,lppublicdispstring(),MAXOBJECTNAME);
                 suppspace(name3);
                 fstrncpy(pcur->ExtraData, (LPTSTR)RemoveSQLQuotes(name3), MAXOBJECTNAME);       /* user on which security applies */
                 suppspace(pcur->ExtraData);
                 storeint(pcur->ExtraData+MAXOBJECTNAME,
                     intno);               /* security number */
                 storeint(pcur->ExtraData+MAXOBJECTNAME+STEPSMALLOBJ,
                     SecuritySet[j].Value);
                 if (selectstatement[0]=='$')
                    selectstatement[0]='\0';
                 suppspace(selectstatement);
                 fstrncpy(pcur->ExtraData+(MAXOBJECTNAME+2*STEPSMALLOBJ),
                     selectstatement, MAXOBJECTNAMENOSCHEMA); /* name of sec.alarm*/
                 pcur=GetNextLLData(pcur);
                 if (!pcur) {
                   iresselect = RES_ERR;
                   exec sql endselect;
                   ManageSqlError(&sqlca); // Keep track of the SQL error if any
                 }
                 i++;
              }
           }
        exec sql end;
        ManageSqlError(&sqlca); // Keep track of the SQL error if any
      }
      break;
    case OT_RULE:
      {
        IsFound=FALSE;
        if (bHasDot) {
           sprintf(selectstatement,"table_name='%s' and rule_owner='%s'",
                 RemoveDisplayQuotesIfAny(ParentWhenDot),RemoveDisplayQuotesIfAny(ParentOwnerWhenDot));
             /* rule_owner is the same as table_owner*/
        }
        else { 
           sprintf(selectstatement,"table_name='%s'",
                   RemoveDisplayQuotesIfAny(parentstrings[1]));
        }

        exec sql select rule_name,  rule_owner,   text_segment, text_sequence
                 into   :singlename,:singleownername,:extradata,:intno
                 from iirules
                 where :selectstatement
                 order by rule_name,rule_owner,text_sequence;

        exec sql begin;
           suppspace(singlename);
           suppspace(singleownername);
           ManageSqlError(&sqlca); // Keep track of the SQL error if any
			if (HasSelectLoopInterruptBeenRequested()) {
				iresselect = RES_ERR;
				exec sql endselect;
			}
           if (!IsFound) {      // first time in the loop : init breakpoints
              IsFound=TRUE;
              fstrncpy(OLDname,singlename,MAXOBJECTNAME);
              fstrncpy(OLDowner,singleownername,MAXOBJECTNAME);
           }            
           if (x_strcmp(OLDname,singlename)==0 &&          // still same...
              x_strcmp(OLDowner,singleownername)==0) {     // .. rule
              if (!ConcatText(extradata)) {               // concatenate 
                 iresselect = RES_ERR;
                 exec sql endselect;
                 ManageSqlError(&sqlca); // Keep track of the SQL error if any
              }
           }
           else {                                        // rule changes.
              fstrncpy(pcur->Name,(LPTSTR)Quote4DisplayIfNeeded(OLDname),MAXOBJECTNAME);
              fstrncpy(pcur->OwnerName,OLDowner,MAXOBJECTNAME);
              ParseRuleText(pcur->ExtraData);
              FreeText();
              if (!ConcatText(extradata))                // concatenate
                 return RES_ERR;
              pcur=GetNextLLData(pcur);
              if (!pcur) {
                iresselect = RES_ERR;
                exec sql endselect;
                ManageSqlError(&sqlca); // Keep track of the SQL error if any
              }
              i++;
              fstrncpy(OLDname,singlename,MAXOBJECTNAME);
              fstrncpy(OLDowner,singleownername,MAXOBJECTNAME);
           }
        exec sql end;
        ManageSqlError(&sqlca); // Keep track of the SQL error if any

        if (IsFound && iresselect!=RES_ERR) { // process the last one, if any.
           ParseRuleText(pcur->ExtraData);
           fstrncpy(pcur->Name,(LPTSTR)Quote4DisplayIfNeeded(OLDname),MAXOBJECTNAME);
           fstrncpy(pcur->OwnerName,OLDowner,MAXOBJECTNAME);
           FreeText();
           pcur=GetNextLLData(pcur);
           if (!pcur) 
             iresselect = RES_ERR;
           i++;
        }
      }
      break;
    case OT_INDEX:
      {
          if (IsStarDatabase(GetMainCacheHdl(ilocsession), parentstrings[0])) {
            if (bHasDot) {
               sprintf(selectstatement,"base_name='%s' and base_owner='%s'",
                       RemoveDisplayQuotesIfAny(ParentWhenDot),RemoveDisplayQuotesIfAny(ParentOwnerWhenDot));
            }
            else { 
               sprintf(selectstatement,"base_name='%s'",
                       RemoveDisplayQuotesIfAny(parentstrings[1]));
            }
            
            exec sql select index_name, index_owner,     storage_structure
                     into   :singlename,:singleownername,:extradata
                     from   iiindexes
                     where :selectstatement; 
            
            exec sql begin;
             ManageSqlError(&sqlca); // Keep track of the SQL error if any
			if (HasSelectLoopInterruptBeenRequested()) {
				iresselect = RES_ERR;
				exec sql endselect;
			}
             fstrncpy(pcur->Name,singlename,MAXOBJECTNAME);
             fstrncpy(pcur->OwnerName,singleownername,MAXOBJECTNAME);
             suppspace(extradata);
             x_strcpy(extradata+20,"..");

             fstrncpy(pcur->ExtraData,extradata,MAXOBJECTNAME);
             storeint(pcur->ExtraData+24, -1);  // DUMMY ID FOR MONITORING
             bsuppspace=TRUE;
             pcur=GetNextLLData(pcur);
             if (!pcur) {
               iresselect = RES_ERR;
               exec sql endselect;
               ManageSqlError(&sqlca); // Keep track of the SQL error if any
             }
             i++;
            exec sql end;
            ManageSqlError(&sqlca); // Keep track of the SQL error if any
            break;
          }

           if (bHasDot) {
              sprintf(selectstatement,"t.index_name= r.relid and t.index_owner=r.relowner and base_name='%s' and base_owner='%s'",
                      RemoveDisplayQuotesIfAny(ParentWhenDot),RemoveDisplayQuotesIfAny(ParentOwnerWhenDot));
           }
           else { 
              sprintf(selectstatement,"t.index_name= r.relid and t.index_owner=r.relowner and base_name='%s'",
                      RemoveDisplayQuotesIfAny(parentstrings[1]));
           }
           if (GetOIVers() == OIVERS_NOTOI) {
              if (bHasDot) {
                 sprintf(selectstatement,"base_name='%s' and base_owner='%s'",
                         RemoveDisplayQuotesIfAny(ParentWhenDot),RemoveDisplayQuotesIfAny(ParentOwnerWhenDot));
              }
              else 
                 sprintf(selectstatement,"base_name='%s'",RemoveDisplayQuotesIfAny(parentstrings[1]));

              exec sql select index_name, index_owner,     storage_structure
                       into   :singlename,:singleownername,:extradata
                       from   iiindexes 
                       where :selectstatement; 

              exec sql begin;
                ManageSqlError(&sqlca); // Keep track of the SQL error if any
				if (HasSelectLoopInterruptBeenRequested()) {
					iresselect = RES_ERR;
					exec sql endselect;
				}
                fstrncpy(pcur->Name,singlename,MAXOBJECTNAME);
                fstrncpy(pcur->OwnerName,singleownername,MAXOBJECTNAME);
                suppspace(extradata);
                x_strcpy(extradata+20,"..");

                fstrncpy(pcur->ExtraData,extradata,MAXOBJECTNAME);
                storeint(pcur->ExtraData+24,(-1)); /* table id for monitoring : dummy (no monitoring for gateways or Ingres <Oping 1.x */
                bsuppspace=TRUE;
                pcur=GetNextLLData(pcur);
                if (!pcur) {
                  iresselect = RES_ERR;
                  exec sql endselect;
                  ManageSqlError(&sqlca); // Keep track of the SQL error if any
                }
                i++;
              exec sql end;
              ManageSqlError(&sqlca); // Keep track of the SQL error if any
              break;
           }
           exec sql select index_name, index_owner,     storage_structure,
                    reltidx
                    into   :singlename,:singleownername,:extradata,
                    :intno
                    from   iiindexes t,iirelation r
                    where :selectstatement; 

           exec sql begin;
             ManageSqlError(&sqlca); // Keep track of the SQL error if any
			if (HasSelectLoopInterruptBeenRequested()) {
				iresselect = RES_ERR;
				exec sql endselect;
			}
             suppspace(singlename);
             fstrncpy(pcur->Name,(LPTSTR)Quote4DisplayIfNeeded(singlename),MAXOBJECTNAME);
             fstrncpy(pcur->OwnerName,singleownername,MAXOBJECTNAME);
             suppspace(extradata);
             x_strcpy(extradata+20,"..");

             fstrncpy(pcur->ExtraData,extradata,MAXOBJECTNAME);
             storeint(pcur->ExtraData+24,intno); // table id for monitoring
             bsuppspace=TRUE;
             pcur=GetNextLLData(pcur);
             if (!pcur) {
               iresselect = RES_ERR;
               exec sql endselect;
               ManageSqlError(&sqlca); // Keep track of the SQL error if any
             }
             i++;
           exec sql end;
           ManageSqlError(&sqlca); // Keep track of the SQL error if any
      }
      break;
    case OT_TABLELOCATION:
      {
        if (bHasDot) {
           sprintf(selectstatement,"table_name='%s' and table_owner='%s'",
                   RemoveDisplayQuotesIfAny(ParentWhenDot),RemoveDisplayQuotesIfAny(ParentOwnerWhenDot));
        }
        else { 
           sprintf(selectstatement,"table_name='%s'",
                   RemoveDisplayQuotesIfAny(parentstrings[1]));
        }

        exec sql select location_name,multi_locations
                 into :extradata,:rt1
                 from iitables
                 where :selectstatement; 

        ManageSqlError(&sqlca); // Keep track of the SQL error if any
        if (sqlca.sqlcode==100) {
          iresselect = RES_ERR;
          break;
        }
        fstrncpy(pcur->Name,extradata,MAXOBJECTNAME);
        pcur=GetNextLLData(pcur);
        if (!pcur) {
          iresselect = RES_ERR;
          break;
        }
        i++;

        if (GetYNState(rt1)==ATTACHED_YES) {
            exec sql select location_name 
                     into   :extradata
                     from  iimulti_locations 
                     where :selectstatement;

            exec sql begin;
               ManageSqlError(&sqlca); // Keep track of the SQL error if any
				if (HasSelectLoopInterruptBeenRequested()) {
					iresselect = RES_ERR;
					exec sql endselect;
				}
               fstrncpy(pcur->Name,extradata,MAXOBJECTNAME);
               suppspace(pcur->Name);
               if (pcur->Name[0] != '\0') {
                 pcur=GetNextLLData(pcur);
                 if (!pcur) {
                   iresselect = RES_ERR;
                   exec sql endselect;
                   ManageSqlError(&sqlca); // Keep track of the SQL error if any
                 }
                 i++;
               }
            exec sql end;
            ManageSqlError(&sqlca); // Keep track of the SQL error if any
        }
        break;
      }
      break;
    case OT_GROUPUSER:
      {

        fstrncpy(singlename,parentstrings[0],sizeof(singlename));

        exec sql repeated select groupmem
                 into   :singlename
                 from iiusergroup
                 where groupid=:singlename and (NOT groupmem='');

        exec sql begin;
          ManageSqlError(&sqlca); // Keep track of the SQL error if any
			if (HasSelectLoopInterruptBeenRequested()) {
				iresselect = RES_ERR;
				exec sql endselect;
			}
          fstrncpy(pcur->Name,singlename, MAXOBJECTNAME);
          pcur=GetNextLLData(pcur);
          if (!pcur) {
            iresselect = RES_ERR;
            exec sql endselect;
            ManageSqlError(&sqlca); // Keep track of the SQL error if any
          }
          i++;
        exec sql end;
        ManageSqlError(&sqlca); // Keep track of the SQL error if any
      }
      break;

    default:
      return RES_ENDOFDATA;
      break;
  }

  if (iresselect != RES_SUCCESS)
    goto err;

  nbItems=i;
  if (bIceRequest) {
    ICE_STATUS      status = 0;
#ifndef NOUSEICEAPI
    if (ICE_C_Disconnect (&ICEclient)!=OK)
       return RES_ERR;
#endif
  }
  else  {
     long lerrcode = sqlca.sqlcode;
     if (ReleaseSession(ilocsession, RELEASE_COMMIT)!=RES_SUCCESS)
        return RES_ERR;
     switch (lerrcode) {
       case   0L:
       case 100L:
         break;
       case -34000L:
         return RES_NOGRANT;
         break;
       default:
         return RES_ERR;
     }
  }
  i=0;
  SortItems(iobjecttype);
  pcur=staticplldbadata;
  return DBAGetNextObject(lpobjectname,lpownername,lpextradata);
err:
  if (bIceRequest) {
    ICE_STATUS      status = 0;
#ifndef NOUSEICEAPI
    if (ICE_C_Disconnect (&ICEclient)!=OK)
	   return RES_ERR;
#endif
  }
  else  {
	  ReleaseSession(ilocsession, RELEASE_COMMIT);
  }
  nbItems=0;
  i=0;
  return RES_ERR;
}

int  DBAGetFirstObject(lpVirtNode,iobjecttype,level,parentstrings,
                       bWithSystem,lpobjectname,lpownername,lpextradata)
LPUCHAR lpVirtNode;
int     iobjecttype;
int     level;
LPUCHAR *parentstrings;
BOOL    bWithSystem;
LPUCHAR lpobjectname;
LPUCHAR lpownername;
LPUCHAR lpextradata;
{
	return Task_DBAGetFirstObjectInterruptible (lpVirtNode,iobjecttype,level,parentstrings,
                       bWithSystem,lpobjectname,lpownername,lpextradata);
	return DBAGetFirstObject(lpVirtNode,iobjecttype,level,parentstrings,
                       bWithSystem,lpobjectname,lpownername,lpextradata);


}
// added Emb 26/4/95 for critical section test
int  DBAGetFirstObjectLL(lpVirtNode,iobjecttype,level,parentstrings,
                       bWithSystem,lpobjectname,lpownername,lpextradata)
LPUCHAR lpVirtNode;
int     iobjecttype;
int     level;
LPUCHAR *parentstrings;
BOOL    bWithSystem;
LPUCHAR lpobjectname;
LPUCHAR lpownername;
LPUCHAR lpextradata;
{
  int retval,hdl,ires;
  BOOL bMemErrAlreadySet;
  if (iobjecttype == OT_MON_REPLIC_CDDS_ASSIGN) {
    LPREPLICSERVERDATAMIN lp = (LPREPLICSERVERDATAMIN)parentstrings;
    UCHAR buf[MAXOBJECTNAME]; 
    hdl  = OpenNodeStruct (lp->LocalDBNode);
    if (hdl != -1)
        ires = DOMGetFirstObject (hdl, OT_DATABASE, 0, NULL, FALSE, NULL, buf, NULL, NULL);
    else
        return RES_ERR;
  }
    
  if (gMemErrFlag == MEMERR_STANDARD) {
    bMemErrAlreadySet = FALSE;
    gMemErrFlag = MEMERR_NOMESSAGE;
  }
  else
    bMemErrAlreadySet = TRUE;
  StartSqlCriticalSection();
  EnableSqlSaveText();
  retval = DBAOrgGetFirstObject(lpVirtNode,iobjecttype,level,parentstrings,
                          bWithSystem,lpobjectname,lpownername,lpextradata);
  DisableSqlSaveText();
  StopSqlCriticalSection();
  if (!bMemErrAlreadySet) {
    if (gMemErrFlag == MEMERR_HAPPENED)
      TS_MessageBox(TS_GetFocus(),
                 ResourceString(IDS_E_MEMERR_DBAGETFIRSTOBJECT),
                 NULL, MB_OK | MB_ICONSTOP | MB_TASKMODAL);
    gMemErrFlag = MEMERR_STANDARD;
  }
  if (iobjecttype == OT_MON_REPLIC_CDDS_ASSIGN) {
      CloseNodeStruct (hdl, TRUE);
  }
  return retval;
}


int  DBAGetNextObject(lpobjectname,lpownername,lpextradata)
LPUCHAR lpobjectname;
LPUCHAR lpownername;
LPUCHAR lpextradata;
{
  if (i<nbItems) {
    switch (iDBATypemem) {
      case OT_MON_SERVER:
         *((LPSERVERDATAMIN)lpobjectname)     = pcur->uData.SvrData;
         break;
      case OT_MON_SESSION:
         *((LPSESSIONDATAMIN)lpobjectname)    = pcur->uData.SessionData;
         break;
      case OT_MON_LOCKLIST:
         *((LPLOCKLISTDATAMIN)lpobjectname)   = pcur->uData.LockListData;
         break;
      case OT_MON_LOCK:
      case OT_MON_RES_LOCK:
      case OT_MON_LOCKLIST_LOCK:
         *((LPLOCKDATAMIN)lpobjectname)       = pcur->uData.LockData;
         break;
      case OTLL_MON_RESOURCE:
         *((LPRESOURCEDATAMIN)lpobjectname)   = pcur->uData.ResourceData;
         break;
      case OT_MON_LOGPROCESS:
         *((LPLOGPROCESSDATAMIN)lpobjectname) = pcur->uData.LogProcessData;
         break;
      case OT_MON_LOGDATABASE    :
         *((LPLOGDBDATAMIN)lpobjectname)      = pcur->uData.LogDBdata;
         break;
      case OT_MON_TRANSACTION:
         *((LPLOGTRANSACTDATAMIN)lpobjectname)= pcur->uData.LogTransacData;
         break;
      case OT_REPLIC_REGTBLS_V11:
         *((LPDD_REGISTERED_TABLES)lpobjectname) = pcur->uData.Registred_Tables;
         break;
      case OT_MON_REPLIC_SERVER:
         *((LPREPLICSERVERDATAMIN)lpobjectname) = pcur->uData.ReplicSrvDta;
         break;
      case OT_MON_REPLIC_CDDS_ASSIGN:
         *((LPREPLICCDDSASSIGNDATAMIN)lpobjectname) = pcur->uData.ReplicCddsDta;
         break;
      case OT_STARDB_COMPONENT:
         *((LPSTARDBCOMPONENTPARAMS)lpobjectname) = pcur->uData.StarDbCompDta;
         break;
      case OT_MON_ICE_CONNECTED_USER:
         *((LPICE_CONNECTED_USERDATAMIN)lpobjectname) = pcur->uData.IceConnectedUserDta;
         break;
      case OT_MON_ICE_ACTIVE_USER   :
         *((LPICE_ACTIVE_USERDATAMIN)lpobjectname) = pcur->uData.IceActiveUserDta;
         break;
      case OT_MON_ICE_TRANSACTION   :
         *((LPICE_TRANSACTIONDATAMIN)lpobjectname) = pcur->uData.IceTransactionDta;
         break;
      case OT_MON_ICE_CURSOR        :
         *((LPICE_CURSORDATAMIN)lpobjectname) = pcur->uData.IceCursorDta;
         break;
      case OT_MON_ICE_FILEINFO      :
         *((LPICE_CACHEINFODATAMIN)lpobjectname) = pcur->uData.IceCacheInfoDta;
         break;
      case OT_MON_ICE_DB_CONNECTION :
         *((LPICE_DB_CONNECTIONDATAMIN)lpobjectname) = pcur->uData.IceDbConnectionDta;
         break;

      default:
        fstrncpy(lpobjectname,pcur->Name,MAXOBJECTNAME);
        suppspacebutleavefirst(lpobjectname);
        if (lpownername) {
          fstrncpy(lpownername,pcur->OwnerName,MAXOBJECTNAME);
          suppspace(lpownername);
        }
        if (lpextradata) {
          memcpy(lpextradata,pcur->ExtraData,EXTRADATASIZE);
          if (bsuppspace)
            suppspace(lpextradata);
        }
        break;
    }
    pcur=pcur->pnext;
    i++;
    return RES_SUCCESS;
  }   
  return RES_ENDOFDATA;
}

void DBAginfoFree()
{
  int i;
  exec sql begin declare section;
    long isession;
  exec sql end declare section;
#ifndef NOUSEICEAPI
	ICE_C_Terminate();
#endif
  while (staticplldbadata){
    pcur=staticplldbadata->pnext;
    ESL_FreeMem(staticplldbadata);
    staticplldbadata=pcur;
  }

  exec sql whenever not found continue;
  for (i=0;i<MAXSESSIONS;i++) {
    if (session[i].SessionName[0]) {
      isession=sessionnumber(i);
      exec sql set_sql(session=:isession);
      exec sql disconnect;
      session[i].SessionName[0]='\0';
    }
  }

  //exec sql disconnect all;

  ManageSqlError(&sqlca); // Keep track of the SQL error if any
}

BOOL IsConnectionActive()
{
  int i;
  char * pc;
  for (i=0;i<imaxsessions;i++) {
    pc = session[i].SessionName;
    if (pc[0])
      return TRUE;
  }
  return FALSE;
}

int ActiveConnections()
{
  int i;
  int count;
  char * pc;
  count=0;
  for (i=0;i<imaxsessions;i++) {
    pc = session[i].SessionName;
    if (pc[0])
      count++;
  }
  return count;
}


int DBACloseNodeSessions(UCHAR * lpNodeName,BOOL bAllGWNodes, BOOL bAllUsers) // empty lpNodeName closes all
                                                              // sessions in the cache
{
  int i;
  char buf[100];
  char SessName[100];
  char bufNodeWithoutGWandUsr[200];
  exec sql begin declare section;
    long isession;
  exec sql end declare section;
  char localstring[MAXOBJECTNAME];
  char localhost[MAXOBJECTNAME];
  BOOL bWantToCloseLocal;
#ifndef NOUSEICEAPI
	ICE_C_Terminate();
#endif
	if (bAllGWNodes || bAllUsers ) {
		lstrcpy(bufNodeWithoutGWandUsr,lpNodeName);
		if (bAllGWNodes)
			RemoveGWNameFromString(bufNodeWithoutGWandUsr);
		if (bAllUsers)
			RemoveConnectUserFromString(bufNodeWithoutGWandUsr);
		lpNodeName = bufNodeWithoutGWandUsr;
	}


  x_strcpy(localstring, ResourceString ((UINT)IDS_I_LOCALNODE) );
  GetLocalVnodeName(localhost, MAXOBJECTNAME);

  if (lpNodeName[0] && (!x_stricmp(lpNodeName,localstring) || !x_stricmp(lpNodeName,localhost)))
	bWantToCloseLocal = TRUE;
  else
	bWantToCloseLocal = FALSE;


  wsprintf(buf,"%s::",lpNodeName);
  for (i=0;i<imaxsessions;i++) {
    lstrcpy(SessName,session[i].SessionName);
    if (!SessName[0]) 
      continue;
    if (bAllGWNodes) 
        RemoveGWNameFromString(SessName);
    if (bAllUsers)
        RemoveConnectUserFromString(SessName);
    if (bWantToCloseLocal) {
		char buf1[100],buf2[100];
		wsprintf(buf1, "%s::",localstring);
		wsprintf(buf2, "%s::",localhost);
		if (x_strnicmp(SessName,buf1,x_strlen(buf1)) && x_strnicmp(SessName,buf2,x_strlen(buf2)))
            continue;
	}
	else {
	    if (lpNodeName[0] && x_strnicmp(SessName,buf,x_strlen(buf)))
            continue;
    }
    if (session[i].bLocked)
      return RES_ERR;
    if (IsImaActiveSession(i))
       return RES_ERR;
    isession=sessionnumber(i);
    sqlca.sqlcode =0L; /* since set_sql doesn't seem to reset this value*/
    exec sql set_sql(session=:isession);
    ManageSqlError(&sqlca); // Keep track of the SQL error if any
    /* commit or rollback normally already has been done */
    exec sql disconnect;
    ClearImaCache(i);
    ManageSqlError(&sqlca); // Keep track of the SQL error if any
    session[i].SessionName[0]='\0';
  }
  return RES_SUCCESS;
}


// Added Emb 6/11/95 for local connection

// Emb Added bCheck 15/11/95 for ii_promptall remote commands deadloop

BOOL  DBAGetSessionUser(lpVirtNode,lpresultusername)
LPUCHAR lpVirtNode;
LPUCHAR lpresultusername;
{
  return DBAGetSessionUserWithCheck(lpVirtNode,lpresultusername, FALSE);
}

BOOL DBAGetUserName (const unsigned char* VirtNodeName, unsigned char* UserName)
{
  return DBAGetSessionUserWithCheck((char *)VirtNodeName,UserName, FALSE);
}


BOOL  DBAGetSessionUserWithCheck(lpVirtNode,lpresultusername, bCheck)
LPUCHAR lpVirtNode;
LPUCHAR lpresultusername;
BOOL    bCheck;
{
  char connectname[MAXOBJECTNAME];
//  char buf2[200];
  char szLocal[MAXOBJECTNAME];
  BOOL bResult;
  int  iret, ilocsession;
  BOOL bGenericGateway = FALSE;
  exec sql begin declare section;
    char buf[200];
  exec sql end declare section;
  
  if (GetOIVers() == OIVERS_NOTOI)
    bGenericGateway = TRUE;

  sprintf(connectname,"%s::%s",lpVirtNode,GlobalInfoDBName());

  iret = Getsession(connectname,SESSION_TYPE_CACHENOREADLOCK, &ilocsession);
  if (iret !=RES_SUCCESS)
    return FALSE;

  bResult = FALSE;

  if (bGenericGateway) {
     exec sql select dbmsinfo('username') into :buf;
  }
  else {
     exec sql repeated select dbmsinfo('session_user') into :buf;
  }
  ManageSqlError(&sqlca); // Keep track of the SQL error if any
  if (sqlca.sqlcode < 0) 
     goto end;
  buf[sizeof(buf)-1]='\0';
  suppspace(buf);

  // compare to username in node definition, except if local connection
  // and only if bCheck is set
  x_strcpy(szLocal, ResourceString ((UINT)IDS_I_LOCALNODE) );
#ifdef OLDIES // no more checking against user defined in the node
              // because of global entries
//  if (bCheck && memcmp(lpVirtNode, szLocal, x_strlen(szLocal)) != 0) {
//     if (!DBAGetUserName(lpVirtNode,buf2))
//       goto end;
//     if (x_stricmp(buf,buf2))
//       goto end;
//     if (buf[0] == '\0')
//       goto end;
//  }
#endif

  x_strcpy( lpresultusername, buf);
  bResult=TRUE;
end:
  ReleaseSession(ilocsession, RELEASE_COMMIT);
  return bResult;
}


BOOL CheckConnection(LPUCHAR lpVirtNode,BOOL bForceSession,BOOL bDisplayErrorMessage)
{
  char connectname[MAXOBJECTNAME];
  char szLocal[MAXOBJECTNAME];   // string for local connection
  BOOL bResult;
  int  iret, ilocsession;
  int ioldOIVers;

  x_strcpy(szLocal, ResourceString ((UINT)IDS_I_LOCALNODE) );

  sprintf(connectname,"%s::%s",lpVirtNode,GlobalInfoDBName());

  ShowHourGlass();
  StartSqlCriticalSection();
  ioldOIVers = GetOIVers();
  iret = Getsession(connectname,SESSION_TYPE_CACHENOREADLOCK | SESSION_CHECKING_CONNECTION , &ilocsession);
#ifdef WIN32
  if (iret !=RES_SUCCESS)
    iret = Getsession(connectname,SESSION_TYPE_CACHENOREADLOCK | SESSION_CHECKING_CONNECTION, &ilocsession);
#endif
  if (iret !=RES_SUCCESS) {
    StopSqlCriticalSection();
    RemoveHourGlass();
    if (bDisplayErrorMessage)
      ErrorMessage ((UINT)IDS_E_CONNECTION_FAILED , RES_ERR);   // Connection failure
    SetOIVers(ioldOIVers); /* restore DBMS level flag because GetSession failed, and may have changed it given the SESSION_CHECKING_CONNECTION flag */
    return FALSE;
  }

  bResult=TRUE;

  ReleaseSession(ilocsession, RELEASE_COMMIT);
  StopSqlCriticalSection();
  RemoveHourGlass();
  return bResult;
}

BOOL IsIngresIISession()
{
  exec sql begin declare section;
    char buf[200];
  exec sql end declare section;

  BOOL bIsIngresII=FALSE;
  int i;
  static char * ingprefixes[]= {"II"};

  if (GetOIVers() == OIVERS_NOTOI)
	return FALSE;


  exec sql repeated select dbmsinfo('_version') into :buf;
  ManageSqlError(&sqlca); // Keep track of the SQL error if any
  if (sqlca.sqlcode < 0) 
     buf[0]='\0';
  exec sql commit;
  buf[sizeof(buf)-1]='\0';
  for (i=0;i<sizeof(ingprefixes)/sizeof(char *);i++) {
      if (!x_strnicmp(buf,ingprefixes[i],x_strlen(ingprefixes[i])))
          bIsIngresII=TRUE;
  }
  return bIsIngresII;
}

BOOL GetDBNameFromObjType (int ObjType,  char * buffResult, char* dbName)
{
   switch (ObjType)
   {
       case OT_USER:
       case OT_GROUP:
       case OT_ROLE:
       case OT_PROFILE:
       case OT_LOCATION:
       case OT_DATABASE:
           x_strcpy (buffResult, GlobalInfoDBName());
           return TRUE;
       default:
           if (dbName)
           {
               x_strcpy (buffResult, dbName);
               return TRUE;
           }      
           else
               return FALSE;
   }
}

static LPUCHAR GDBName_NONE= "iidbdb";

LPUCHAR GlobalInfoDBName()
{
  return GDBName_NONE;
}

static int iOIver = OIVERS_12;
static int bIsVW = 0;

int GetOIVers ()
{
    return iOIver;
}

int SetOIVers (int oivers)
{
   int imem=iOIver;
   iOIver  =oivers;
   return imem;
}

int IsVW()
{
   return bIsVW;
}

void SetVW(int b)
{
   bIsVW = b;
}

static UCHAR NODETOBECLOSED[MAXOBJECTNAME]= {'\0'};
static BOOL bMultiple2beClosed = FALSE;

int PostNodeClose(LPUCHAR nodename)
{
  if (NODETOBECLOSED[0])
     bMultiple2beClosed = TRUE;
  else 
     x_strcpy(NODETOBECLOSED,nodename);
  return 0;
}
int PostCloseIfNeeded()
{
  if (NODETOBECLOSED[0]) {
     if (bMultiple2beClosed)
        DBACloseNodeSessions("",TRUE,TRUE);
     else 
        DBACloseNodeSessions(NODETOBECLOSED,TRUE,TRUE);
  }
  NODETOBECLOSED[0]='\0';
  bMultiple2beClosed = FALSE;
  return 0;
}

static int NotifyDBEError(BOOL bRepDbe)
{
   if (!bRepDbe) {
        TS_MessageBox(TS_GetFocus(),
                  ResourceString(IDS_ERR_CANNOT_SCAN_DBEV_CLOSEW),
                   NULL, MB_OK | MB_ICONSTOP | MB_TASKMODAL);
   }
   else {
        TS_MessageBox(TS_GetFocus(),
                  ResourceString(IDS_ERR_CANNOT_SCAN_DBEV_MON_NOTUTD),
                   NULL, MB_OK | MB_ICONSTOP | MB_TASKMODAL);
   }
  
   return RES_SUCCESS;
}
struct lcllist {char *evt;int itype;} mydbelist[] = {
          {"dd_insert"      ,REPLIC_DBE_OUT_INSERT        },
          {"dd_update"      ,REPLIC_DBE_OUT_UPDATE        },
          {"dd_delete"      ,REPLIC_DBE_OUT_DELETE        },
          {"dd_transaction" ,REPLIC_DBE_OUT_TRANSACTION   },
          {"dd_insert2"     ,REPLIC_DBE_IN_INSERT         },
          {"dd_update2"     ,REPLIC_DBE_IN_UPDATE         },
          {"dd_delete2"     ,REPLIC_DBE_IN_DELETE         },
          {"dd_transaction2",REPLIC_DBE_IN_TRANSACTION    },
          {NULL             ,REPLIC_DBE_NONE              }
        };

#define MAX_DBE_SCAN_TIME_SECOND 8
#define SCANTRIES 2
int DBEScanDBEvents()  // function to be called every n seconds (n = 5 ?)
{
  time_t ltimeStart;
  int i,ires,iret, itries;
  RAISEDDBE StructRaisedDbe; 
  exec sql begin declare section;
    char v_name [MAXOBJECTNAME];
    char v_text [256];
    char v_time[60];
    char v_owner[MAXOBJECTNAME];
  exec sql end declare section;

  ShowHourGlass();
  for (i=0;i<imaxsessions;i++) {
    BOOL bPurged=FALSE;
    if ( !(session[i].bLocked && session[i].bIsDBESession) )
      continue;
    if (!session[i].SessionName[0]) {
      NotifyDBEError(session[i].bIsDBE4Replic);
      continue;
    }
    ires = ActivateSession(i);
    if (ires!=RES_SUCCESS)  {
      NotifyDBEError(session[i].bIsDBE4Replic);
      RemoveHourGlass();
      return ires;
    }
    exec sql commit;
    ManageSqlError(&sqlca);
    if (sqlca.sqlcode != 0 && sqlca.sqlcode != 710) {
       NotifyDBEError(session[i].bIsDBE4Replic);
       RemoveHourGlass();
       return RES_ERR;
    }

    ltimeStart = time( NULL );
    itries=0;
    while((time(NULL)-ltimeStart)<MAX_DBE_SCAN_TIME_SECOND || itries<SCANTRIES) { 
      exec sql get dbevent;
      ManageSqlError(&sqlca);
      if (sqlca.sqlcode != 0 && sqlca.sqlcode != 710) {
         NotifyDBEError(session[i].bIsDBE4Replic);
         RemoveHourGlass();
         return RES_ERR;
      }
      exec sql commit;
      ManageSqlError(&sqlca);
      if (sqlca.sqlcode != 0 && sqlca.sqlcode != 710) {
        NotifyDBEError(session[i].bIsDBE4Replic);
        RemoveHourGlass();
        return RES_ERR;
      }
     exec sql inquire_sql(:v_name=dbeventname,:v_owner=dbeventowner,
                           :v_time=dbeventtime,:v_text =dbeventtext);
      ManageSqlError(&sqlca);
      if (sqlca.sqlcode != 0 && sqlca.sqlcode != 710) {
         NotifyDBEError(session[i].bIsDBE4Replic);
         RemoveHourGlass();
         return RES_ERR;
      }
      
      exec sql commit;
      ManageSqlError(&sqlca);
      if (sqlca.sqlcode != 0 && sqlca.sqlcode != 710) {
        NotifyDBEError(session[i].bIsDBE4Replic);
        RemoveHourGlass();
        return RES_ERR;
      }

      if (v_name[0]=='\0') {
         bPurged = TRUE;
         bDisplayMessage = TRUE;
         break;
      }

      // DBEvent found

      suppspace(v_time );
      suppspace(v_name );
      suppspace(v_owner);
      suppspace(v_text );
      if (!session[i].bIsDBE4Replic) {
        fstrncpy(StructRaisedDbe.StrTimeStamp,v_time ,sizeof(StructRaisedDbe.StrTimeStamp));
        fstrncpy(StructRaisedDbe.DBEventName ,v_name ,sizeof(StructRaisedDbe.DBEventName));
        fstrncpy(StructRaisedDbe.DBEventOwner,v_owner,sizeof(StructRaisedDbe.DBEventOwner));
        fstrncpy(StructRaisedDbe.DBEventText ,v_text ,sizeof(StructRaisedDbe.DBEventText));
        DBETraceDisplayRaisedDBE(session[i].hwnd, &StructRaisedDbe);
      }
      else {
        struct lcllist * plist=mydbelist;
        while (plist->evt) {
          if (!x_stricmp(plist->evt,v_name)) {
            RAISEDREPLICDBE raiseddbe;
            char * pc1;
            memset(&raiseddbe,'\0',sizeof(raiseddbe));
            raiseddbe.EventType=plist->itype;
            suppspace(v_text);
            pc1=x_strrchr(v_text,' ');
            if ( pc1) {
               *pc1='\0';
               pc1++;
               if (plist->itype!=REPLIC_DBE_OUT_TRANSACTION &&
                   plist->itype!=REPLIC_DBE_IN_TRANSACTION)
                   fstrncpy(raiseddbe.TableName,pc1,sizeof(raiseddbe.TableName));
            }
            fstrncpy(raiseddbe.DBName,v_text,sizeof(raiseddbe.DBName));
            DBEReplMonDisplayRaisedDBE(i,&raiseddbe);
            break;

          }
          plist++;
        }
        if (!plist->evt) {
          char * evtstart="dd_server";
          if (!x_strnicmp(v_name,evtstart,x_strlen(evtstart))){
              int svrno=atoi(v_name+x_strlen(evtstart));
              UCHAR NodeName[MAXOBJECTNAME];
              UCHAR DBName[MAXOBJECTNAME];
              int ires=GetNodeAndDB(NodeName,DBName,session[i].SessionName);
              if (ires!=RES_SUCCESS)
                 myerror(ERR_REGEVENTS);
              else {
	              RemoveConnectUserFromString(NodeName);
                  UpdateReplStatusinCache(NodeName,DBName,svrno,v_text,TRUE);
			  }
          }
          else
              myerror(ERR_REGEVENTS);
        }
      }
      /* 05-May-1999 (noifr01): reactivate the session within the loop, since if */
      /* a replicator monitoring dbevent has been gotten, the notification to    */
      /* other VDBA layers may result in a session switch                        */
      ires = ActivateSession(i);
      if (ires!=RES_SUCCESS)  {
        NotifyDBEError(session[i].bIsDBE4Replic);
        RemoveHourGlass();
        return ires;
      }
      itries++;
    }
    if (!bPurged && bDisplayMessage) {
      //"WARNING: Registered Database Events are raised at higher frequency than "
      //"they can be managed by this application.  Continue displaying this message?"
      iret = TS_MessageBox(TS_GetFocus(),ResourceString(IDS_ERR_REGISTERED_DB_EVENTS) ,
                                         NULL, MB_YESNO | MB_ICONSTOP | MB_TASKMODAL);
      if (iret == IDYES )
         bDisplayMessage = TRUE;
      if (iret == IDNO )
         bDisplayMessage = FALSE;
    }
  }
  RemoveHourGlass();
  return RES_SUCCESS;
}

int DBETraceInit(int hnode,LPUCHAR lpDBName,HWND hwnd,int *pires)
{
  int iret,isess;
  UCHAR buf[120];
  LPUCHAR lpVNodeName=GetVirtNodeName(hnode);
  if (!lpVNodeName) {
    myerror(ERR_DOMGETDATA);
    return RES_ERR;
  }
  wsprintf(buf,"%s::%s",lpVNodeName,lpDBName);

  iret = Getsession(buf, SESSION_TYPE_INDEPENDENT,&isess);
  if (iret!=RES_SUCCESS)
    return iret;

  *pires=isess;
  session[isess].hwnd=hwnd;
  session[isess].bIsDBESession = TRUE;
  session[isess].bIsDBE4Replic = FALSE;
  return RES_SUCCESS;
}

int DBETraceRegister(int hdl, LPUCHAR DBEventName, LPUCHAR DBEventOwner)
{
   char dbevent[120];
   char szSqlStm[100+MAXOBJECTNAME];
   int  ires;

   ires = ActivateSession(hdl);
   if (ires!=RES_SUCCESS)
     return ires;

   StringWithOwner(DBEventName,DBEventOwner,dbevent);

   wsprintf(szSqlStm,"register dbevent %s",dbevent);

   ires=ExecSQLImm(szSqlStm,TRUE, NULL, NULL, NULL);
   return ires;
}
int DBETraceUnRegister(int hdl, LPUCHAR DBEventName, LPUCHAR DBEventOwner)
{
   char dbevent[120];
   char szSqlStm[100+MAXOBJECTNAME];
   int  ires;

   DBEScanDBEvents()  ;
   ires = ActivateSession(hdl);
   if (ires!=RES_SUCCESS)
     return ires;

   StringWithOwner(DBEventName,DBEventOwner,dbevent);

   wsprintf(szSqlStm,"remove dbevent %s",dbevent);

   ires=ExecSQLImm(szSqlStm,TRUE, NULL, NULL, NULL);
   return ires;
}
int DBETraceTerminate(int hdl)
{
   session[hdl].bIsDBESession = FALSE;
   return ReleaseSession(hdl, RELEASE_COMMIT);
}

int DBEReplMonInit(int hnode,LPUCHAR lpDBName,int *pires)
{
  LPUCHAR lpVNodeName=GetVirtNodeName(hnode);
  if (!lpVNodeName) {
    myerror(ERR_DOMGETDATA);
    return RES_ERR;
  }
  return DBEReplGetEntry(lpVNodeName,lpDBName,pires, (-1));
}
int DBEReplMonTerminate(int hdl)
{
   return DBEReplReplReleaseEntry(hdl);
}

int DBEReplReplReleaseEntry(int hdl)
{
    if (!session[hdl].bIsDBESession ||
        !session[hdl].bIsDBE4Replic ||
        session[hdl].nbDBEEntries<1)  {
         return myerror(ERR_SESSION);
    }
    session[hdl].nbDBEEntries--;
    if (!session[hdl].nbDBEEntries) {
      CloseNodeStruct(session[hdl].hnode,FALSE);
      session[hdl].bIsDBESession = FALSE;
      push4mong();
      UpdateNodeDisplay(); // to show "connected state" eventual change
      pop4mong();
	  return ReleaseSession(hdl, RELEASE_COMMIT);
    }
    return RES_SUCCESS;
}

int DBECacheEntriesUsedByReplMon(int hnode)
{
   int isess;
   int ires=0;
   for (isess=0;isess<imaxsessions;isess++){
      if ( session[isess].nbDBEEntries>0  && 
           session[isess].hnode == hnode  &&
           session[isess].bIsDBESession   &&
           session[isess].bIsDBE4Replic    )
          ires++;
   }
   return ires;
}

BOOL isNodeWithUserUsedByReplMon(LPUCHAR lpNodeName)
{
  int isess;
  char buf[100];

  wsprintf(buf,"%s::",lpNodeName);

  for (isess=0;isess<imaxsessions;isess++){
      if (!x_strnicmp(session[isess].SessionName,buf,x_strlen(buf)) && 
           session[isess].bIsDBESession &&
           session[isess].bIsDBE4Replic    )
		   return TRUE;
		   
  }
  return FALSE;
}

int DBEReplGetEntry(LPUCHAR lpNodeName,LPUCHAR lpDBName,int *pires, int serverno)
{
    int hnode,ires,isess;
    UCHAR buf        [MAXOBJECTNAME];
    UCHAR bufowner   [MAXOBJECTNAME];
    UCHAR extradata  [EXTRADATASIZE];
    UCHAR SessionName[200];
	UCHAR DBOwner    [MAXOBJECTNAME];
	LPUCHAR parentstrings [MAXPLEVEL];
	UCHAR NodeName   [100];
	int irestype;

	// get DBOwner
	hnode=OpenNodeStruct(lpNodeName);
    if (hnode<0)
       return RES_ERR;
    ires= DOMGetFirstObject  ( hnode, OT_DATABASE, 0, NULL, TRUE,NULL,buf,bufowner,extradata);
    if (ires!=RES_SUCCESS && ires!=RES_ENDOFDATA) {
        CloseNodeStruct(hnode,FALSE);
        return ires;
    }
	parentstrings[0]=lpDBName;
	parentstrings[1]=NULL;
	ires = DOMGetObjectLimitedInfo(	hnode,
									lpDBName,
									"",
									OT_DATABASE,
									0,
									parentstrings,
									TRUE,
									&irestype,
									buf,
									DBOwner,
									extradata
								   );
    CloseNodeStruct(hnode,FALSE);
 	if (ires != RES_SUCCESS) 
       return ires;

	x_strcpy(NodeName,lpNodeName);
	RemoveConnectUserFromString(NodeName);
	x_strcat(NodeName,LPUSERPREFIXINNODENAME);
	x_strcat(NodeName,DBOwner);
	x_strcat(NodeName,LPUSERSUFFIXINNODENAME);

    wsprintf(SessionName,"%s::%s",NodeName,lpDBName);
  
    for (isess=0;isess<imaxsessions;isess++){
      if (!x_stricmp(session[isess].SessionName,SessionName) && 
           session[isess].bIsDBESession &&
           session[isess].bIsDBE4Replic    ) { 
         *pires=isess;
         ires=ActivateSession(isess);
         if (ires!=RES_SUCCESS)
            return RES_ERR;
         goto getentryend;
      }
    }
    hnode=OpenNodeStruct(NodeName);
    if (hnode<0)
       return RES_ERR;
    ires= DOMGetFirstObject  ( hnode, OT_DATABASE, 0, NULL, TRUE,NULL,buf,bufowner,extradata);
    if (ires!=RES_SUCCESS && ires!=RES_ENDOFDATA) {
        CloseNodeStruct(hnode,FALSE);
        return ires;
    }

    ires = Getsession(SessionName, SESSION_TYPE_INDEPENDENT,&isess);
    if (ires!=RES_SUCCESS) {
       CloseNodeStruct(hnode,FALSE);
        return ires;
    }

    push4mong();
    UpdateNodeDisplay(); // to show "connected state" eventual change
    pop4mong();
    ActivateSession(isess);    
    *pires=isess;
    session[isess].bIsDBESession = TRUE;
    session[isess].bIsDBE4Replic = TRUE;
	session[isess].bMainDBERegistered = FALSE;
	session[isess].nbDBEEntries=0;
    session[isess].nbDBERegSvr=0;
    session[isess].hnode=hnode;

getentryend:
    session[isess].nbDBEEntries++;
    if (serverno>-1) {
       int i;
       BOOL bDone=FALSE;
       for (i=0;i<session[isess].nbDBERegSvr;i++) {
          if (session[isess].DBERegSvr[i]==serverno) {
             bDone=TRUE;
             break;
          }
       }
       if (!bDone) {
          sprintf(buf,"register dbevent dd_server%d",serverno);
          ires=ExecSQLImm(buf,TRUE, NULL, NULL, NULL);
          if (ires!=RES_SUCCESS && ires!=RES_MOREDBEVENT) {
             DBEReplReplReleaseEntry(isess);
             return ires;
          } 
          if (session[isess].nbDBERegSvr>=MAXREGSVR) {
            //"Too many active Replication Servers on the same node"
            TS_MessageBox(TS_GetFocus(),ResourceString(IDS_ERR_ACTIVE_REPLICATION_SERV) ,
                       NULL,MB_ICONHAND | MB_OK | MB_TASKMODAL);
            DBEReplReplReleaseEntry(isess);
            return RES_ERR;
          }
          session[isess].DBERegSvr[session[isess].nbDBERegSvr]=serverno;
          session[isess].nbDBERegSvr++;
       }
       ires=ExecSQLImm("raise dbevent dd_ping",TRUE, NULL, NULL, NULL);
       if (ires!=RES_SUCCESS && ires!=RES_MOREDBEVENT) {
           //"Cannot ping server %d on %s::%s"
           sprintf(buf,ResourceString(IDS_F_PING_SVR),serverno,lpNodeName,lpDBName);
            TS_MessageBox(TS_GetFocus(),buf,NULL,MB_ICONHAND | MB_OK | MB_TASKMODAL);
            DBEReplReplReleaseEntry(isess);
            return RES_ERR;
       }
         
    }
    else { /* main dbevents */
       if (!session[isess].bMainDBERegistered) {
          struct lcllist * plist=mydbelist;
          while (plist->evt) {
             sprintf(buf,"REGISTER DBEVENT %s",plist->evt);
             ires=ExecSQLImm(buf,TRUE, NULL, NULL, NULL);
             if (ires!=RES_SUCCESS) {
                //"Cannot Register Database Event %s on %s::%s"
                sprintf(buf,ResourceString(IDS_F_REGISTER_DB_EVENTS),plist->evt,lpNodeName,lpDBName);
                TS_MessageBox(TS_GetFocus(),buf,NULL,MB_ICONHAND | MB_OK | MB_TASKMODAL);
                DBEReplReplReleaseEntry(isess);
                return ires;
             }
             plist++;
           }
           session[isess].bMainDBERegistered=TRUE;
       }
    }
    return RES_SUCCESS;
}

int CreateDD_SERVER_SPECIALIfNotExist ()
{
    EXEC SQL BEGIN DECLARE SECTION;
        int intno;
    EXEC SQL END DECLARE SECTION;
    EXEC SQL repeated SELECT COUNT(*) INTO :intno 
    FROM iitables 
    WHERE LOWERCASE(table_name) = 'dd_server_special';
    if (intno == 0) {
        EXEC SQL CREATE TABLE dd_server_special (localdb smallint not null ,
                                                 server_no smallint not null ,
                                                 vnode_name char (32) not null);
        if (sqlca.sqlcode < 0)  { 
            ManageSqlError(&sqlca); // Keep track of the SQL error if any
            EXEC SQL ROLLBACK;
            return RES_ERR;
        }
    }
    EXEC SQL COMMIT;
    return RES_SUCCESS;
}
int DeleteRowsInDD_SERVER_SPECIAL()
{
    EXEC SQL BEGIN DECLARE SECTION;
        int TblExist;
        int NumRows;
    EXEC SQL END DECLARE SECTION;

    EXEC SQL repeated SELECT COUNT(*) INTO :TblExist
    FROM iitables
    WHERE LOWERCASE(table_name) = 'dd_server_special';

    EXEC SQL repeated SELECT COUNT(*) INTO :NumRows
    FROM  dd_server_special;

    if (TblExist && NumRows) {
        EXEC SQL DELETE FROM dd_server_special;
        if (sqlca.sqlcode < 0)  { 
            ManageSqlError(&sqlca); // Keep track of the SQL error if any
            EXEC SQL ROLLBACK;
            return RES_ERR;
        }
    }
    EXEC SQL COMMIT;
    return RES_SUCCESS;
}

int DropInDD_SERVER_SPECIAL()
{
    EXEC SQL BEGIN DECLARE SECTION;
        int intno;
    EXEC SQL END DECLARE SECTION;
    EXEC SQL repeated SELECT COUNT(*) INTO :intno 
    FROM iitables 
    WHERE LOWERCASE(table_name) = 'dd_server_special';
    if (intno) {
        EXEC SQL DROP TABLE dd_server_special;
        if (sqlca.sqlcode < 0)  { 
            ManageSqlError(&sqlca); // Keep track of the SQL error if any
            EXEC SQL ROLLBACK;
            return RES_ERR;
        }
    }
    EXEC SQL COMMIT;
    return RES_SUCCESS;
}

static struct rmcmdgrant
   {
       UCHAR ObjectName[MAXOBJECTNAME];
       int   ObjectType;
       int   GrantType;
       BOOL  bGranted;
   } IngRmcmdGrantNeed[] = {
     {"rmcmdcmdend"        , OT_DBEVENT   , OT_DBEGRANT_RAISE_USER, FALSE},
     {"rmcmdcmdend"        , OT_DBEVENT   , OT_DBEGRANT_REGTR_USER, FALSE},
     {"rmcmdnewcmd"        , OT_DBEVENT   , OT_DBEGRANT_RAISE_USER, FALSE},
     {"rmcmdnewcmd"        , OT_DBEVENT   , OT_DBEGRANT_REGTR_USER, FALSE},
     {"rmcmdnewinputline"  , OT_DBEVENT   , OT_DBEGRANT_RAISE_USER, FALSE},
     {"rmcmdnewinputline"  , OT_DBEVENT   , OT_DBEGRANT_REGTR_USER, FALSE},
     {"rmcmdnewoutputline" , OT_DBEVENT   , OT_DBEGRANT_RAISE_USER, FALSE},
     {"rmcmdnewoutputline" , OT_DBEVENT   , OT_DBEGRANT_REGTR_USER, FALSE},
     {"rmcmdstp"           , OT_DBEVENT   , OT_DBEGRANT_RAISE_USER, FALSE},
     {"rmcmdstp"           , OT_DBEVENT   , OT_DBEGRANT_REGTR_USER, FALSE},
     {"launchremotecmd"    , OT_PROCEDURE , OT_PROCGRANT_EXEC_USER, FALSE},
     {"sendrmcmdinput"     , OT_PROCEDURE , OT_PROCGRANT_EXEC_USER, FALSE},
     {"remotecmdinview"    , OT_VIEW      , OT_VIEWGRANT_SEL_USER , FALSE},
     {"remotecmdinview"    , OT_VIEW      , OT_VIEWGRANT_INS_USER , FALSE},
     {"remotecmdinview"    , OT_VIEW      , OT_VIEWGRANT_UPD_USER , FALSE},
     {"remotecmdinview"    , OT_VIEW      , OT_VIEWGRANT_DEL_USER , FALSE},
     {"remotecmdoutview"   , OT_VIEW      , OT_VIEWGRANT_SEL_USER , FALSE},
     {"remotecmdoutview"   , OT_VIEW      , OT_VIEWGRANT_INS_USER , FALSE},
     {"remotecmdoutview"   , OT_VIEW      , OT_VIEWGRANT_UPD_USER , FALSE},
     {"remotecmdoutview"   , OT_VIEW      , OT_VIEWGRANT_DEL_USER , FALSE},
     {"remotecmdview"      , OT_VIEW      , OT_VIEWGRANT_SEL_USER , FALSE},
     {"remotecmdview"      , OT_VIEW      , OT_VIEWGRANT_INS_USER , FALSE},
     {"remotecmdview"      , OT_VIEW      , OT_VIEWGRANT_UPD_USER , FALSE},
     {"remotecmdview"      , OT_VIEW      , OT_VIEWGRANT_DEL_USER , FALSE}
    };
static struct rmcmdowner
   {
       TCHAR FromObject[MAXOBJECTNAME];
       TCHAR ColumnName[MAXOBJECTNAME];
       TCHAR WhereColumnName[MAXOBJECTNAME];
       TCHAR ObjectName[MAXOBJECTNAME];
   } IngRmcmdOwner[] = { "iitables","table_owner","table_name", "remotecmd",
                         "iitables","table_owner","table_name", "remotecmdin",
                         "iitables","table_owner","table_name", "remotecmdout",
                         "iiviews" ,"table_owner","table_name", "remotecmdinview",
                         "iiviews" ,"table_owner","table_name", "remotecmdoutview",
                         "iiviews" ,"table_owner","table_name", "remotecmdview",
                         "iiprocedures","procedure_owner","procedure_name","launchremotecmd",
                         "iiprocedures","procedure_owner","procedure_name","sendrmcmdinput",
                         "iievents","event_owner","event_name","rmcmdnewcmd",
                         "iievents","event_owner","event_name","rmcmdnewoutputline",
                         "iievents","event_owner","event_name","rmcmdnewinputline",
                         "iievents","event_owner","event_name","rmcmdcmdend",
                         "iievents","event_owner","event_name","rmcmdstp",
                         "","","",""
                       };

int GetInfOwner4RmcmdObject(LPUSERPARAMS lpUsrPar)
{
   EXEC SQL BEGIN DECLARE SECTION;
      char ObjOwner[MAXOBJECTNAME];
      char stmt[MAXOBJECTNAME*5];
   EXEC SQL END DECLARE SECTION;
   int iOwner,iNotOwner;
   struct rmcmdowner * lpRmcmdOwner = IngRmcmdOwner;
   lpUsrPar->iOwnerRmcmdObjects  = RMCMDOBJETS_NO_OWNER;
   iOwner    = 0;
   iNotOwner = 0;

   while(lpRmcmdOwner->FromObject[0] != _T('\0'))
   {
       ObjOwner[0] = _T('\0');
       wsprintf( stmt,
                "SELECT DISTINCT %s "
                "FROM %s "
                "WHERE lowercase (%s) = '%s'",lpRmcmdOwner->ColumnName,
                                              lpRmcmdOwner->FromObject,
                                              lpRmcmdOwner->WhereColumnName,
                                              lpRmcmdOwner->ObjectName );
       EXEC SQL EXECUTE IMMEDIATE :stmt INTO :ObjOwner;
       ManageSqlError(&sqlca);
       if (sqlca.sqlcode < 0)
         break;
       if (sqlca.sqlcode == 100)
       {
         iNotOwner++;
         lpRmcmdOwner++;
         continue;
       }
       suppspace(ObjOwner);
       if (x_stricmp(ObjOwner,lpUsrPar->ObjectName)==0)
         iOwner++;
       else
         iNotOwner++;
       lpRmcmdOwner++;
   }


   if ( sqlca.sqlcode !=0 && sqlca.sqlcode != 100)
       return RES_ERR;
   else
   {
       if (iOwner > 0)
       {
           if (iNotOwner == 0)
               lpUsrPar->iOwnerRmcmdObjects = RMCMDOBJETS_OWNER;
           else
               lpUsrPar->iOwnerRmcmdObjects = RMCMDOBJETS_PARTIAL_OWNER;
       }
       return RES_SUCCESS;
   }
}

int GetInfGrantUser4Rmcmd(LPUSERPARAMS lpUsrParam)
{
   EXEC SQL BEGIN DECLARE SECTION;
   char ObjName [MAXOBJECTNAME];
   char ObjType [9];
   char TextSeg [241];
   char *lpUserName = lpUsrParam->ObjectName;
   EXEC SQL END DECLARE SECTION;

   struct rmcmdgrant * lpRmcmdGrant = IngRmcmdGrantNeed;
   int iNum,iNbGrant = 0,iObjType,iGrantType;
   int iMax = sizeof(IngRmcmdGrantNeed)/sizeof (IngRmcmdGrantNeed[0]);

   lpUsrParam->bGrantUser4Rmcmd    = FALSE;
   lpUsrParam->bPartialGrant       = FALSE;

   GetInfOwner4RmcmdObject( lpUsrParam );

for (iNum=0; iNum < iMax ; iNum++)
      lpRmcmdGrant[iNum].bGranted =FALSE;

   EXEC SQL REPEAT SELECT object_name, object_type, text_segment
             INTO  :ObjName   ,:ObjType    , :TextSeg
             FROM iipermits
             WHERE permit_user = :lpUserName order by object_type;
   EXEC SQL BEGIN;
      ManageSqlError(&sqlca); // Keep track of the SQL error if any
      suppspace(ObjName);
      iObjType   = GetType(ObjType);
      iGrantType = ParsePermitText(iObjType, TextSeg);

      for (iNum=0; iNum < iMax ; iNum++)
      {
         if (x_stricmp(ObjName,lpRmcmdGrant[iNum].ObjectName)==0 &&
             lpRmcmdGrant[iNum].ObjectType == iObjType && lpRmcmdGrant[iNum].GrantType == iGrantType)
         {
            lpRmcmdGrant[iNum].bGranted=TRUE;
            break;
         }
      }
   EXEC SQL END;
   ManageSqlError(&sqlca); // Keep track of the SQL error if any

   for (iNum=0,iNbGrant=0; iNum < iMax ; iNum++)
   {
      if ( lpRmcmdGrant[iNum].bGranted == TRUE )
         iNbGrant++;
   }

   if (iNbGrant == iMax || lpUsrParam->iOwnerRmcmdObjects == RMCMDOBJETS_OWNER)
      lpUsrParam->bGrantUser4Rmcmd = TRUE;
   else 
   {
      if (iNbGrant > 0 || lpUsrParam->iOwnerRmcmdObjects == RMCMDOBJETS_PARTIAL_OWNER)
         lpUsrParam->bPartialGrant = TRUE;
   }
    if ( sqlca.sqlcode !=0 && sqlca.sqlcode != 100)
      return RES_ERR;
   else
      return RES_SUCCESS;
}
