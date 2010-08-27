/********************************************************************
**
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
**
**  Project : libmon library for the Ingres Monitor ActiveX control
**
**  Source : dbaginfo.sc
**
**  derived from the winmain\dbaginfo 
**
**  includes:
**     -session cache management
**     -low level queries for getting lists of objects
**     -misc DBEvent tracking functions
**
**  History
**
**  21-Nov-2001 (noifr01)
**      extracted and adapted from the mainmfc\monitor source
** 17-Jul-2003 (uk$so01)
**    SIR #106648, Vdba-Split, ensure that the ingres sessions 
**    have the descriptions.
** 17-Sep-2003 (schph01)
**    Sir 110673 Add a new buffer (bufnew8[1001]) with the same size that the
**    buffer session_query define in structure SESSIONDATAMIN.
** 24-Nov-2003 (schph01)
**    (SIR 107122)Update test for determine the 2.65 ingres version.
** 23-Jan-2004 (schph01)
**    (sir 104378) detect version 3 of Ingres, for managing
**    new features provided in this version. replaced references
**    to 2.65 with refereces to 3  in #define definitions for
**    better readability in the future
** 24-Mar-2004 (shaha03)
**	  (sir 112040) Global variables i,nbItems,bsuppspace and iDBATypemem are changed 
**		to i[MAX_THREADS],nbItems[MAX_THREADS],bsuppspace[MAX_THREADS] and iDBATypemem[MAX_THREADS] 
**		to support the threads. Accessing these varibales also is changed as that of arrays.
**	  (sir 112040) Global variables *staticplldbadata, *pcur changed to *staticplldbadata[MAX_THREADS],
**		pcur[MAX_THREADS] to suuport tyhe threads.Accessing these varibales also is changed as that 
**		of arrays.
**	  (sir 112040) "static int CmpItems()"  functions proto type is changed and 
**		"int thdl"(thread handle) parameter is added.
**	  (sir 112040) "static void  SortItems()" functions proto type is changed and 
**		"int thdl"(thread handle) parameter is added.
**    (sir 112040) added LIBMON_DBAGetFirstObject()to support the multi-threaded context for the 
**		same purpose of DBAGetFirstObject()
**    (sir 112040) added LIBMON_DBAGetNextObject()to support the multi-threaded context for the 
**		same purpose of DBAGetNextObject()
**    (sir 112040) Modified prototype of "static int  DBAOrgGetFirstObject()" and added
**		"int thdl" paramater (Thread handle) to support Multiple threading
** 12-May-2004 (schph01)
**    (SIR 111507) Add management for new column type bigint
** 19-Nov-2004 (noifr01)
**    BUG #113500 / ISSUE #13755297 (Vdba / monitor, references such as background 
**    refresh, or the number of sessions, are not taken into account in vdbamon nor 
**    in the VDBA Monitor functionality)
** 09-Mar-2005 (nansa02)
**    Bug # 114041 Fixed the incorrect ingres versioning for HP-UX.
** 02-Feb-2006 (drivi01)
**    BUG 115691
**    Keep VDBA tools at the same version as DBMS server.  Update VDBA
**    to 9.0 level versioning. Make sure that 9.0 version string is
**    correctly interpreted by visual tools and VDBA internal version
**    is set properly upon that interpretation.
** 12-Oct-2006 (wridu01)
**    (Sir 116835) Add support for Ansi Date/Time data types
**    Changed logic to parse the DBMSINFO('_version') string.  The 
**    previous logic interpreted II 9.1.0 as OpenIngres 1.2
** 14-Dec-2007 (kiria01) b119671
**    Tightened up the version parsing to ensure 9.1.0 and 9.2.0 did not
**    get recognised as 1.2 and 2.0 respectively and allow for future 9.x
**    to at least have 9.1 capabilities.
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
** 30-Jun-2010 (drivi01)
**    Bug #124006
**    Add new BOOLEAN datatype.
***********************************************************************/

#include <time.h>
#include <tchar.h>
#include "libmon.h"
#include "error.h"
#include "gc.h"
#include "er.h"
# include <lo.h>
#include "pm.h"

#include "domdloca.h"
#include "lbmnmtx.h" 

extern LBMNMUTEX lbmn_mtx_session; /* Mutex for thread support */

static LPOBJECTLIST LIBMON_AddListObjectTail(LPOBJECTLIST FAR * lplpList, int cbObjSize)
/*
   Function:
      Adds a new element to the tail of the list.

   Parameters:
      lplpList    - Address of root pointer to the list.
      cbObjSize   - Size of object to add to list

   Returns:
      Pointer to the new list object if successful, otherwise NULL.
*/
{
   LPOBJECTLIST lpNewElement = ESL_AllocMem(sizeof(OBJECTLIST));

   if (lpNewElement)
   {
      LPOBJECTLIST lpFind;

      lpNewElement->lpObject = ESL_AllocMem(cbObjSize);

      if (!lpNewElement->lpObject)
      {
         ESL_FreeMem(lpNewElement);
         return NULL;
      }
      /* // Find the current last object in the list */

      lpFind = *lplpList;
      while (lpFind)
      {
         if (!lpFind->lpNext)
            break;

         lpFind = lpFind->lpNext;
      }

      if (lpFind != NULL)
         lpFind->lpNext = lpNewElement;
      else
         *lplpList = lpNewElement;

      lpNewElement->lpNext = NULL;
   }

   return lpNewElement;
}

static LPOBJECTLIST LIBMON_FreeAllInObjectList(LPOBJECTLIST lpList)
/*
   Function:
      Deletes all objects from the list
   Parameters:
      lpList         - Head of list from which element is to be deleted.
   Return: NULL
*/
{
   LPOBJECTLIST lpObjectLst,lpObjLstNext;
   if (!lpList)
      return NULL;
   lpObjectLst = lpList;
   while (lpObjectLst)
   {
       lpObjLstNext = lpObjectLst->lpNext; // save the next Object list
       ESL_FreeMem(lpObjectLst->lpObject); // free the current object
       lpObjectLst->lpObject = NULL;
       ESL_FreeMem(lpObjectLst);           // free the current object list
       lpObjectLst = lpObjLstNext;
   }
   return NULL;
}

int  i[MAX_THREADS],nbItems[MAX_THREADS];
static int  imaxsessions    = 20;
long llastgetsession = 0;
static BOOL bsuppspace[MAX_THREADS];
static int iDBATypemem[MAX_THREADS];

typedef struct tagDOCLIST
{
   struct tagDOCLIST *lpNext;
   void   *pIpmDoc;
   LPOBJECTLIST pDBEventUntreated;
} DOCLIST , FAR *LPDOCLIST;

static LPDOCLIST LIBMON_FreeDocInDocList(LPDOCLIST lpBeginList, LPDOCLIST lpDelElement)
/*
   Function:
     Deletes an object from the list

   Parameters:
     lpBeginList     - Head of list from which element is to be deleted.
     lpDelElement    - Element to be deleted

   Returns:
      The start of the list.  This must be used in case the head of the list
      is deleted.
*/
{
    if( !lpDelElement)
        return lpBeginList;
    if (!lpBeginList)
        return NULL;

    if (lpBeginList == lpDelElement)
        lpBeginList = lpDelElement->lpNext;
    else
    {
        LPDOCLIST lpPrev = lpBeginList;
        while (lpPrev && lpPrev->lpNext != lpDelElement)
            lpPrev = lpPrev->lpNext;

        if (!lpPrev)
        // The element to be deleted was not found.
            return lpBeginList;

        // Point the previous element to the one after the element to be deleted.
        lpPrev->lpNext = lpDelElement->lpNext;
    }


    if (lpDelElement->pDBEventUntreated)
        lpDelElement->pDBEventUntreated = LIBMON_FreeAllInObjectList(lpDelElement->pDBEventUntreated);
    lpDelElement->pDBEventUntreated = NULL;
    ESL_FreeMem(lpDelElement);
    lpDelElement = NULL;
    return lpBeginList;

}

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
  LPDOCLIST lpDocLst;
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
struct lldbadata * staticplldbadata[MAX_THREADS];
struct lldbadata * pcur[MAX_THREADS];

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

static int iThreadCurSess = -1;

#ifndef BLD_MULTI /*To make available in Non multi-thread context only */
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
#endif

static BOOL bRequestInterrupt;
void LIBMON_Request_Query_Interrupt()
{
	bRequestInterrupt  = TRUE;
}

void LIBMON_Reset_Query_Interrupt_Flag()
{
	bRequestInterrupt  = FALSE;
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
  }
  return FALSE;
}

static void DBACloseUnusedSessions()
{
  int i;
  exec sql begin declare section;
    long isession;
  exec sql end declare section;

  for (i=0;i<imaxsessions;i++) {
    if (!session[i].SessionName[0]) 
      continue;
    if (session[i].bLocked)
      continue;
    if (IsImaActiveSession(i))
       continue;
    isession=sessionnumber(i);
    sqlca.sqlcode =0L; /* since set_sql doesn't seem to reset this value*/
    exec sql set_sql(session=:isession);
    ManageSqlError(&sqlca); // Keep track of the SQL error if any
    exec sql disconnect;
    ManageSqlError(&sqlca); // Keep track of the SQL error if any
    session[i].SessionName[0]='\0';
  }
}

void LIBMON_SetMaxSessions(int imaxsess)
{
	if (!LIBMON_CanUpdateMaxSessions())
		return;
	imaxsessions=imaxsess;
}

BOOL LIBMON_CanUpdateMaxSessions(void)
{
    int i;
    DBACloseUnusedSessions();
    for (i=0;i<imaxsessions;i++) {
        if (session[i].SessionName[0] || session[i].bLocked)
            return FALSE;
    }
    return TRUE;
}


static long lsessiontimeout = 0L;
void LIBMON_SetSessionTimeOut(long ltimeout)
{
	DBACloseUnusedSessions();
	lsessiontimeout = ltimeout;
}

extern LBMNMUTEX   lclhst_name_mtx;

BOOL bLocalVnodeGotten = FALSE;
static char szGCHostName[MAXOBJECTNAME] = ("");
char *LIBMON_getLocalHostName()
{
  if(!CreateLBMNMutex(&lclhst_name_mtx))/*Creating a Mutex: Will ignore if the mutex is already created */
		return szGCHostName;
  if(!OwnLBMNMutex(&lclhst_name_mtx, INFINITE)) /*Owning the Mutex */
		return szGCHostName;
  
  memset (szGCHostName, '\0', MAXOBJECTNAME);
  GChostname(szGCHostName, MAXOBJECTNAME);
  if (!szGCHostName[0])
    x_strcpy(szGCHostName,"UNKNOWN LOCAL HOST NAME");
  (void)UnownLBMNMutex(&lclhst_name_mtx); /*Releaseing the Mutex before return */
  return szGCHostName;

  if (!bLocalVnodeGotten) {
	  PMinit();
	  x_strcpy(szGCHostName,"UNKNOWN LOCAL HOST NAME");
	  if (PMload (NULL, NULL) == OK) {
  		 char*	host = PMhost();
		 char *value;
		 char buf[50];
		 wsprintf(buf,"ii.%s.gcn.local_vnode",host);
		 if (PMget(buf, &value ) == OK ) 
		    x_strcpy(szGCHostName,value);
		 PMfree();
	  }
	  bLocalVnodeGotten = TRUE;
  }
  if(!UnownLBMNMutex(&lclhst_name_mtx)) /*Releaseing the Mutex before return */
	return RES_ERR;
  return szGCHostName;

}

BOOL HasSelectLoopInterruptBeenRequested() 
{
	if (bRequestInterrupt) {
		AddSqlError("VDBA Interrupt", "The user has requested an interrupt",-1);
		sqlca.sqlcode = -1;
		return TRUE;
	}
	return FALSE;

}

static char localnodestring[100] = "(local)";

void LIBMON_SetLocalNodeString(char *pLocalNodeString)
{
	x_strcpy(localnodestring,pLocalNodeString);
}

char * LIBMON_getLocalNodeString()
{
	return localnodestring;
}

static char nonestring[100] = "<none>";
void LIBMON_SetNoneString(char *pnonestring)
{
	x_strcpy(nonestring,pnonestring);
}

char * LIBMON_getNoneString()
{
	return nonestring;
}


int IngGetStringType(lpstring, ilen)
LPUCHAR lpstring;
int ilen;
{
   UCHAR buf[MAXOBJECTNAME];
   struct int2string *typestring;
   struct int2string typestrings[]= {
      { INGTYPE_C           , "c"               },
      { INGTYPE_CHAR        , "char"            },
      { INGTYPE_CHAR        , "character"       },
      { INGTYPE_TEXT        , "text",           },
      { INGTYPE_VARCHAR     , "varchar"         },
      { INGTYPE_LONGVARCHAR , "long varchar"    },
      { INGTYPE_BIGINT      , "bigint"          },
      { INGTYPE_BOOLEAN     , "boolean"         },
      { INGTYPE_INT8        , "int8"            },
      { INGTYPE_INT4        , "integer"         },
      { INGTYPE_INT2        , "smallint"        },
      { INGTYPE_INT1        , "integer1"        },
      { INGTYPE_DECIMAL     , "decimal"         },
      { INGTYPE_FLOAT8      , "float8"          },
      { INGTYPE_FLOAT8      , "float"           },
      { INGTYPE_FLOAT4      , "float4"          },
      { INGTYPE_FLOAT4      , "real"            },
      { INGTYPE_DATE        , "date"            },
      { INGTYPE_ADTE        , "ansidate"                       },
      { INGTYPE_TMWO        , "time without time zone"         },
      { INGTYPE_TMW         , "time with time zone"            },
      { INGTYPE_TME         , "time with local time zone"      },
      { INGTYPE_TSWO        , "timestamp without time zone"    },
      { INGTYPE_TSW         , "timestamp with time zone"       },
      { INGTYPE_TSTMP       , "timestamp with local time zone" },
      { INGTYPE_INYM        , "interval year to month"         },
      { INGTYPE_INDS        , "interval day to second"         },
      { INGTYPE_IDTE        , "ingresdate"                     },
      { INGTYPE_MONEY       , "money"           },
      { INGTYPE_BYTE        , "byte"            },
      { INGTYPE_BYTEVAR     , "byte varying"    },
      { INGTYPE_LONGBYTE    , "long byte"       },
      { INGTYPE_UNICODE_NCHR, "nchar"           },
      { INGTYPE_UNICODE_NVCHR,"nvarchar"        },
      { INGTYPE_UNICODE_LNVCHR,"long nvarchar"  },
      { INGTYPE_OBJKEY      , "??"              }, /* see comment */
      { INGTYPE_TABLEKEY    , "??"              },
      { INGTYPE_SECURITYLBL , "??"              },
      { INGTYPE_SHORTSECLBL , "??"              },
      { 0                   ,(char *)0          }  };

      /* Last entries are unused at this time. Bytes seem to map to chars.*/
      /* To be filled if needed according to the strings found for that need*/

   fstrncpy(buf,lpstring,sizeof(buf));
   suppspace(buf);
   for (typestring=typestrings;typestring->pchar;typestring++) {
      if (!x_stricmp(typestring->pchar,buf)) {
         switch (typestring->item) {
            case INGTYPE_INT8:
            case INGTYPE_INT4:
            case INGTYPE_INT2:
            case INGTYPE_INT1:
               switch (ilen) {
                  case 8:   return INGTYPE_INT8; break;
                  case 4:   return INGTYPE_INT4; break;
                  case 2:   return INGTYPE_INT2; break;
                  case 1:   return INGTYPE_INT1; break;
               }
               break;
            case INGTYPE_FLOAT8:
            case INGTYPE_FLOAT4:
               switch (ilen) {
                  case 8:   return INGTYPE_FLOAT8; break;
                  case 4:   return INGTYPE_FLOAT4; break;
               }
               break;
            default:
               break;
         }
         return typestring->item;
      }
   }
   return INGTYPE_ERROR;
}

static int CmpItems(int iobjecttype,struct lldbadata * pcur1, struct lldbadata * pcur2, int thdl)
{
   switch(iobjecttype) {
      case OT_MON_SERVER :
         return LIBMON_CompareMonInfo1(OT_MON_SERVER, &(pcur1->uData.SvrData),&(pcur2->uData.SvrData),TRUE, thdl);
         break;
      case OT_MON_LOCKLIST :
         return LIBMON_CompareMonInfo1(OT_MON_LOCKLIST, &(pcur1->uData.LockListData),&(pcur2->uData.LockListData),TRUE, thdl);
         break;
      case OT_MON_LOCK           :
      case OT_MON_LOCKLIST_LOCK  :
      case OT_MON_RES_LOCK   :
         return LIBMON_CompareMonInfo1(OT_MON_LOCK,   &(pcur1->uData.LockData),&(pcur2->uData.LockData),TRUE, thdl);
         break;
      case OT_MON_SESSION        :
         return LIBMON_CompareMonInfo1(OT_MON_SESSION,&(pcur1->uData.SessionData),&(pcur2->uData.SessionData),TRUE, thdl);
         break;
      case OTLL_MON_RESOURCE       :
         return LIBMON_CompareMonInfo1(OTLL_MON_RESOURCE,&(pcur1->uData.ResourceData),&(pcur2->uData.ResourceData),TRUE, thdl);
         break;
      case OT_MON_LOGPROCESS     :
         return LIBMON_CompareMonInfo1(OT_MON_LOGPROCESS,&(pcur1->uData.LogProcessData),&(pcur2->uData.LogProcessData),TRUE, thdl);
         break;
      case OT_MON_LOGDATABASE    :
         return LIBMON_CompareMonInfo1(OT_MON_LOGDATABASE,&(pcur1->uData.LogDBdata),&(pcur2->uData.LogDBdata),TRUE, thdl);
         break;
      case OT_MON_REPLIC_SERVER    :
         return LIBMON_CompareMonInfo1(OT_MON_REPLIC_SERVER,&(pcur1->uData.ReplicSrvDta),&(pcur2->uData.ReplicSrvDta),TRUE, thdl);
         break;
      case OT_MON_TRANSACTION :
         return LIBMON_CompareMonInfo1(OT_MON_TRANSACTION,&(pcur1->uData.LogTransacData),&(pcur2->uData.LogTransacData),TRUE, thdl);
         break;
      case OT_REPLIC_REGTBLS_V11:
         return LIBMON_CompareMonInfo1(OT_REPLIC_REGTBLS_V11,&(pcur1->uData.Registred_Tables),&(pcur2->uData.Registred_Tables),TRUE, thdl);
         break;
      case OT_MON_REPLIC_CDDS_ASSIGN:
         return LIBMON_CompareMonInfo1(OT_MON_REPLIC_CDDS_ASSIGN,&(pcur1->uData.ReplicCddsDta),&(pcur2->uData.ReplicCddsDta),TRUE, thdl);
         break;
      case OT_STARDB_COMPONENT:
         return LIBMON_CompareMonInfo1(OT_STARDB_COMPONENT,&(pcur1->uData.StarDbCompDta),&(pcur2->uData.StarDbCompDta),TRUE, thdl);
         break;
      case OT_MON_ICE_CONNECTED_USER:
         return LIBMON_CompareMonInfo1(OT_MON_ICE_CONNECTED_USER,&(pcur1->uData.IceConnectedUserDta),&(pcur2->uData.IceConnectedUserDta),TRUE, thdl);
         break;
      case OT_MON_ICE_ACTIVE_USER:
         return LIBMON_CompareMonInfo1(OT_MON_ICE_ACTIVE_USER,&(pcur1->uData.IceActiveUserDta),&(pcur2->uData.IceActiveUserDta),TRUE, thdl);
         break;
      case OT_MON_ICE_TRANSACTION:
         return LIBMON_CompareMonInfo1(OT_MON_ICE_TRANSACTION,&(pcur1->uData.IceTransactionDta),&(pcur2->uData.IceTransactionDta),TRUE, thdl);
         break;
      case OT_MON_ICE_CURSOR:
         return LIBMON_CompareMonInfo1(OT_MON_ICE_CURSOR,&(pcur1->uData.IceCursorDta),&(pcur2->uData.IceCursorDta),TRUE, thdl);
         break;
      case OT_MON_ICE_FILEINFO:
         return LIBMON_CompareMonInfo1(OT_MON_ICE_FILEINFO,&(pcur1->uData.IceCacheInfoDta),&(pcur2->uData.IceCacheInfoDta),TRUE, thdl);
         break;
      case OT_MON_ICE_DB_CONNECTION:
         return LIBMON_CompareMonInfo1(OT_MON_ICE_DB_CONNECTION,&(pcur1->uData.IceDbConnectionDta),&(pcur2->uData.IceDbConnectionDta),TRUE, thdl);
         break;

      default:
         myerror(ERR_MONITORTYPE);
         break;
   }
   return 0;
}

static void  SortItems(iobjecttype,thdl)
int	thdl;
{
  int i1,icmp;
  struct lldbadata * pcur0;
  struct lldbadata * pcur1;
  struct lldbadata * pcur1p;
  struct lldbadata * pcurtempmem;
  struct lldbadata * pcurtemp0;
  struct lldbadata * pcurtemp1;
  switch (iobjecttype) {
    case OT_REPLIC_DISTRIBUTION:
    case OT_REPLIC_DISTRIBUTION_V11:
    case OT_REPLIC_CONNECTION_V11:
    case OT_REPLIC_CONNECTION:
//    case OT_REPLIC_CDDSDBINFO_V11:
      for (pcur0=staticplldbadata[thdl],i1=0;i1<nbItems[thdl];pcur0=pcurtempmem,i1++) {
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
            staticplldbadata[thdl]=pcur1; /* start of list */
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
      for (pcur0=staticplldbadata[thdl],i1=0;i1<nbItems[thdl];pcur0=pcurtempmem,i1++) {
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
            staticplldbadata[thdl]=pcur1; /* start of list */
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
      for (pcur0=staticplldbadata[thdl],i1=0;i1<nbItems[thdl];pcur0=pcurtempmem,i1++) {
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
            staticplldbadata[thdl]=pcur1; /* start of list */
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
      for (pcur0=staticplldbadata[thdl],i1=0;i1<nbItems[thdl];pcur0=pcurtempmem,i1++) {
        pcurtempmem=pcur0->pnext;
        for (pcur1=pcur0;pcur1->pprev;) {
          pcur1p=pcur1->pprev;
          if (CmpItems(iobjecttype,pcur1,pcur1p, thdl)>=0)
             break;
          pcurtemp0=pcur1p->pprev;
          pcurtemp1=pcur1 ->pnext;
          if (pcurtemp0)
            pcurtemp0->pnext=pcur1;
          else
            staticplldbadata[thdl]=pcur1; /* start of list */
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
      for (pcur0=staticplldbadata[thdl],i1=0;i1<nbItems[thdl];pcur0=pcurtempmem,i1++) {
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
            staticplldbadata[thdl]=pcur1; /* start of list */
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
    case OT_USER:
    case OT_DBEVENT:
    case OT_INTEGRITY:
    case OT_REPLIC_REGTABLE_V11:
    case OT_REPLIC_MAILUSER:
    case OT_REPLIC_MAILUSER_V11:
    case OT_INDEX:
      for (pcur0=staticplldbadata[thdl],i1=0;i1<nbItems[thdl];pcur0=pcurtempmem,i1++) {
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
            staticplldbadata[thdl]=pcur1; /* start of list */
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

static char SessionUserIdentifier[100]; /* char not accepted in declare section - TO BE FINISHED */


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

static int long nstart_session = 1;
void LIBMON_SetSessionStart(long nStart)
{
	if (nstart_session == 1)
		nstart_session = nStart;
}
static char tchLibmonSessionDescription[256] = ("Ingres Visual Performance Monitor Control (ipm.ocx)");
void LIBMON_SetSessionDescription(LPCTSTR lpszDescription)
{
	lstrcpy(tchLibmonSessionDescription, lpszDescription);
}

long sessionnumber(hdl)
int hdl;
{
  return (hdl+(int)nstart_session);
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
  
  if(!CreateLBMNMutex(&lbmn_mtx_session)) /*Creating a Mutex: Will ignore if the mutex is already created */
		return RES_ERR;
  if(!OwnLBMNMutex(&lbmn_mtx_session, INFINITE)) /*Owning the mutex*/
		return RES_ERR;
  
  if (hdl<0 || hdl>imaxsessions)
  {
	(void)UnownLBMNMutex(&lbmn_mtx_session); /*Releasing the mutex before return*/
    return myerror(ERR_SESSION);
  }
  if (!session[hdl].bLocked)
  {
    (void)UnownLBMNMutex(&lbmn_mtx_session); /*Releasing the mutex before return*/
    return myerror(ERR_SESSION);
  }

  ires = ActivateSession(hdl);
  if (ires!=RES_SUCCESS)
  {
    (void)UnownLBMNMutex(&lbmn_mtx_session); /*Releasing the mutex before return*/
    return ires;
  }

  ires=RES_SUCCESS;
  sqlca.sqlcode = 0; /* for cases with no sql command */ 
  switch (releasetype) {
    case RELEASE_NONE     :
      break;
    case RELEASE_COMMIT   :
      exec sql commit;
      ManageSqlError(&sqlca); /* Keep track of the SQL error if any*/
      break;
    case RELEASE_ROLLBACK :
      exec sql rollback;
      ManageSqlError(&sqlca); /* Keep track of the SQL error if any*/
      break;
    default:
      ires = myerror(ERR_SESSION); /*Store the return in temp and release mutex bfore return*/
      (void)UnownLBMNMutex(&lbmn_mtx_session); /*Releasing the mutex before return*/
      return ires;
  }
  if (sqlca.sqlcode < 0)
  {
     (void)UnownLBMNMutex(&lbmn_mtx_session); /*Releasing the mutex before return*/
     ires=RES_ERR;
  }

  if (session[hdl].sessiontype==SESSION_TYPE_INDEPENDENT ) {
    exec sql disconnect;
    ManageSqlError(&sqlca); /* Keep track of the SQL error if any*/
    session[hdl].SessionName[0]='\0';
    if (sqlca.sqlcode < 0)  
      ires=RES_ERR;
  }
  session[hdl].bLocked = FALSE;
  session[hdl].bIsDBESession = FALSE;
  iThreadCurSess = -1;
#ifdef BLD_MULTI /*To make available in Non multi-thread context only */
	exec sql set_sql(session=none);
#endif  
  if(!UnownLBMNMutex(&lbmn_mtx_session)) /*Releasing the mutex before return*/
	return RES_ERR;
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

extern LBMNMUTEX lbmn_mtx_actsession[MAXNODES];
int ActivateSession(hdl)
int hdl;
{
  exec sql begin declare section;
    long isession;
  exec sql end declare section;

  if(!CreateLBMNMutex(&lbmn_mtx_actsession[hdl])) /*Creating a Mutex: Will ignore if the mutex is already created */
		return RES_ERR;
  if(!OwnLBMNMutex(&lbmn_mtx_actsession[hdl], 0)) /*Owning the mutex and returns immediately if can't*/
	return RES_ERR ;
	
  if (hdl<0 || hdl>imaxsessions)
  {
    (void)UnownLBMNMutex(&lbmn_mtx_actsession[hdl]); /*Releasing the mutex before return*/
    return myerror(ERR_SESSION);
  }
  if (!session[hdl].bLocked)
  {
    (void)UnownLBMNMutex(&lbmn_mtx_actsession[hdl]); /*Releasing the mutex before return*/
    return myerror(ERR_SESSION);
  }

  isession=sessionnumber(hdl);
  sqlca.sqlcode = 0;
  exec sql set_sql(session=:isession);
  ManageSqlError(&sqlca); // Keep track of the SQL error if any
  if (sqlca.sqlcode < 0) 
  {
     (void)UnownLBMNMutex(&lbmn_mtx_actsession[hdl]); /*Releasing the mutex before return*/
     return RES_ERR;
  }
  iThreadCurSess = hdl;
  if(!UnownLBMNMutex(&lbmn_mtx_actsession[hdl])) /*Releasing the mutex before return*/
	return RES_ERR;
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
     lstrcpy(SessionUserIdentifier,(""));
     return TRUE;
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
    char* sessionDesc;
  exec sql end declare section;
  
   if(!CreateLBMNMutex(&lbmn_mtx_session))/*Creating a Mutex: Will ignore if the mutex is already created */
		return RES_ERR;
   if(!OwnLBMNMutex(&lbmn_mtx_session, INFINITE)) /*Owning the Mutex*/
		return RES_ERR;

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
      sqlca.sqlcode =0L; /* since set_sql doesn't seem to reset this value*/
      exec sql set_sql(session=:isession);
      ManageSqlError(&sqlca); // Keep track of the SQL error if any
      exec sql commit;
      ManageSqlError(&sqlca); // Keep track of the SQL error if any
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
    (void)UnownLBMNMutex(&lbmn_mtx_session); /*Releasing the mutex before return*/
    return RES_ERR;
  }

  if (session[ichoice].SessionName[0]) {
    isession=sessionnumber(ichoice);
    sqlca.sqlcode =0L; /* since set_sql doesn't seem to reset this value*/
    exec sql set_sql(session=:isession);
    ManageSqlError(&sqlca); // Keep track of the SQL error if any
    exec sql disconnect;
    ManageSqlError(&sqlca); // Keep track of the SQL error if any
    ClearImaCache(ichoice);
    session[ichoice].SessionName[0]='\0';
  }
  fstrncpy(connectname,lpsessionname, sizeof(connectname)); 
  isession=sessionnumber(ichoice);

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
    x_strcpy(szLocal, LIBMON_getLocalNodeString() );
    if (memcmp(connectnameConn, szLocal, x_strlen(szLocal)) == 0) {
      pColon = x_strstr(connectnameConn, "::");
      if (pColon) {
        x_strcpy(buf, pColon+x_strlen("::"));
        x_strcpy(connectnameConn, buf);
      }
    }


    if (/* sessiontype==SESSION_TYPE_INDEPENDENT  && */ szconnectuser[0]) {
       exec sql connect :connectnameConn session :isession identified by :szconnectuser;
    }
    else {
       exec sql connect :connectnameConn session :isession;
    }

    ManageSqlError(&sqlca); // Keep track of the SQL error if any

    if (sqlca.sqlcode == -34000) 
    {
      if(!UnownLBMNMutex(&lbmn_mtx_session)) /*Releasing the mutex before return*/
		return RES_ERR;
      return RES_NOGRANT;
    }

    if (sqlca.sqlcode <0) 
    {
      (void)UnownLBMNMutex(&lbmn_mtx_session); /*Releasing the mutex before return*/
      return RES_ERR;
    }

    if (bCheckingConnection) {
         int  ioiversion = OIVERS_NOTOI;
         BOOL bIsIngres=FALSE;
	 BOOL bIsVectorWise=FALSE;
         int i;
         static char * ingprefixes[]= {"OI","Oping","Ingres","II", "ING"};
         char *v;

         if (OIVERS_12<= OIVERS_NOTOI || 
             OIVERS_20<= OIVERS_12    || 
             OIVERS_25<= OIVERS_20    ||
             OIVERS_26<= OIVERS_25)     
         {
            myerror(ERR_GW);
            exec sql disconnect;
            (void)UnownLBMNMutex(&lbmn_mtx_session); /*Releasing the mutex before return*/
            return RES_ERR;
         }

         exec sql select dbmsinfo('_version') into :buf;
         ManageSqlError(&sqlca); // Keep track of the SQL error if any
         if (sqlca.sqlcode<0) 
         {
            exec sql disconnect;
            (void)UnownLBMNMutex(&lbmn_mtx_session); /*Releasing the mutex before return*/
            return RES_ERR;
         }
         exec sql commit;
         ManageSqlError(&sqlca); // Keep track of the SQL error if any
         if (sqlca.sqlcode<0) 
         {
            exec sql disconnect;
            (void)UnownLBMNMutex(&lbmn_mtx_session); /*Releasing the mutex before return*/
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
       if (sqlca.sqlcode<0) 
       {
          exec sql disconnect;
          (void)UnownLBMNMutex(&lbmn_mtx_session); /*Releasing the mutex before return*/
		  return RES_ERR;
       }
    }
    session[ichoice].bKeepDefaultIsolation = bKeepDefaultIsolation;
    session[ichoice].OIvers = GetOIVers();
    session[ichoice].bIsVW = IsVW();

  }
  if (sqlca.sqlcode < 0)
  {
	(void)UnownLBMNMutex(&lbmn_mtx_session); /*Releasing the mutex before return*/
    return RES_ERR;
  }
  fstrncpy(session[ichoice].SessionName,connectname,
           sizeof(session[ichoice].SessionName));
endgetsession:
  /* set the lockmodes and time out  */
   ltimeout=lsessiontimeout;
   if (ltimeout<0L)
      ltimeout=0L;
  sqlca.sqlcode = 0; /* for cases with no sql command */ 

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
        (void)UnownLBMNMutex(&lbmn_mtx_session); /*Releasing the mutex before return*/
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

      x_strcpy(szLocal, LIBMON_getLocalNodeString() );
      if (memcmp(connectnameConn, szLocal, x_strlen(szLocal)) == 0) {
        pColon = x_strstr(connectname, "::");
        if (pColon) {
          x_strcpy(connectnameConn, pColon+x_strlen("::"));
        }
      }

      isession=sessionnumber(ichoice);
      if (/* sessiontype==SESSION_TYPE_INDEPENDENT  && */ szconnectuser[0]) {
         exec sql connect :connectnameConn session :isession identified by :szconnectuser;
      }
      else {
         exec sql connect :connectnameConn session :isession;
      }
      ManageSqlError(&sqlca); // Keep track of the SQL error if any

      if (sqlca.sqlcode == -34000)  {
		 if(!UnownLBMNMutex(&lbmn_mtx_session)) /*Releasing the mutex before return*/
			return RES_ERR;
         return RES_NOGRANT;
      }
      if (sqlca.sqlcode < 0)  {
         (void)UnownLBMNMutex(&lbmn_mtx_session); /*Releasing the mutex before return*/
         return RES_ERR;
      }
      if (!bKeepDefaultIsolation && !x_stristr(connectnameConn,"::imadb")) {
         exec sql set session isolation level serializable, read write; /* for IMA, the session will be set to read_uncommitted after the init sequence */
         ManageSqlError(&sqlca); // Keep track of the SQL error if any
         if (sqlca.sqlcode<0) 
         {
            exec sql disconnect;
            (void)UnownLBMNMutex(&lbmn_mtx_session); /*Releasing the mutex before return*/
            return RES_ERR;
         }
      }
    }
    else 
      break;
  }
  if (sqlca.sqlcode < 0) { 
     exec sql disconnect;
     ManageSqlError(&sqlca); // Keep track of the SQL error if any
     session[ichoice].SessionName[0]='\0';
     (void)UnownLBMNMutex(&lbmn_mtx_session); /*Releasing the mutex before return*/
     return RES_ERR;
  }

   /* GetOIVers() can now be used here, after change for bug 103515 */
   if (GetOIVers() != OIVERS_NOTOI) {
      sessionDesc = tchLibmonSessionDescription;
      exec sql set session with description= :sessionDesc;
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
      (void)UnownLBMNMutex(&lbmn_mtx_session); /*Releasing the mutex before return*/
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
  if(!UnownLBMNMutex(&lbmn_mtx_session)) /*Releasing the mutex before return*/
	return RES_ERR;
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


// name modified Emb 26/4/95 for critical section test
static int  DBAOrgGetFirstObject(lpVirtNode,iobjecttype,level,parentstrings,
                       bWithSystem,lpobjectname,lpownername,lpextradata,thdl)
LPUCHAR lpVirtNode;
int     iobjecttype;
int     level;
LPUCHAR *parentstrings;
BOOL    bWithSystem;
LPUCHAR lpobjectname;
LPUCHAR lpownername;
LPUCHAR lpextradata;
int		thdl;
{
  char connectname[2*MAXOBJECTNAME+2];
  char * pctemp;
  int iret,itmp;
  int iresselect, ilocsession ;
  BOOL bHasDot;
  UCHAR ParentWhenDot[MAXOBJECTNAME],ParentOwnerWhenDot[MAXOBJECTNAME];
  exec sql begin declare section;
    char singlename[MAXOBJECTNAME];
    char singleownername[MAXOBJECTNAME];
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
    char rt1[9], rt2[9], rt3[9], rt4[9], rt5[9];
    int intno,  intno2, intno3, intno4, intno5 ,intno6;
    int intno7, intno8, intno9, intno10;
    char icursess[33];
    short null1,null2,null3;
    int session_serverpid;
  exec sql end declare section;

  LIBMON_Reset_Query_Interrupt_Flag();

  bsuppspace[thdl]= FALSE;

  if (!staticplldbadata[thdl]) {
    staticplldbadata[thdl]=(struct lldbadata *)
                     ESL_AllocMem(sizeof(struct lldbadata));
    if (!staticplldbadata[thdl]) {
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
                    LIBMON_GetMonInfoName(hnodetmp, OT_DATABASE, parentstrings, singlename,sizeof(singlename)-1, thdl)
                   );
         }
         break;
    case OT_MON_REPLIC_CDDS_ASSIGN:
        {
            LPREPLICSERVERDATAMIN lp = (LPREPLICSERVERDATAMIN)parentstrings;
            sprintf(connectname,"%s::%s",lp->LocalDBNode, lp->LocalDBName);
            break;
        }

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
  iret = Getsession(connectname,SESSION_TYPE_CACHENOREADLOCK, &ilocsession);
  if (iret !=RES_SUCCESS) {
    return RES_ERR;
  }

  iresselect = RES_SUCCESS;
  i[thdl]=0;
  pcur[thdl]=staticplldbadata[thdl];

  iDBATypemem[thdl] = iobjecttype;

  switch (iobjecttype) {
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
          pcur[thdl]->uData.IceConnectedUserDta.svrdata= *lp; 
          suppspace(name3);
          suppspace(bufnew1);
          suppspace(bufnew2);
          fstrncpy(pcur[thdl]->uData.IceConnectedUserDta.name, bufnew1,
                                    sizeof(pcur[thdl]->uData.IceConnectedUserDta.name));
          fstrncpy(pcur[thdl]->uData.IceConnectedUserDta.inguser, bufnew2,
                                    sizeof(pcur[thdl]->uData.IceConnectedUserDta.inguser));
          pcur[thdl]->uData.IceConnectedUserDta.req_count = intno;
          pcur[thdl]->uData.IceConnectedUserDta.timeout   = intno2;

          pcur[thdl]=GetNextLLData(pcur[thdl]);
          if (!pcur[thdl]) {
            iresselect = RES_ERR;
            exec sql endselect;
            ManageSqlError(&sqlca); // Keep track of the SQL error if any
          }
          i[thdl]++;
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
          fstrncpy(pcur[thdl]->uData.IceActiveUserDta.name, bufnew1,
                                    sizeof(pcur[thdl]->uData.IceActiveUserDta.name));
          fstrncpy(pcur[thdl]->uData.IceActiveUserDta.host, bufnew2,
                                    sizeof(pcur[thdl]->uData.IceActiveUserDta.host));
          fstrncpy(pcur[thdl]->uData.IceActiveUserDta.query, bufnew3,
                                    sizeof(pcur[thdl]->uData.IceActiveUserDta.query));
          pcur[thdl]->uData.IceActiveUserDta.count = intno;

          pcur[thdl]=GetNextLLData(pcur[thdl]);
          if (!pcur[thdl]) {
            iresselect = RES_ERR;
            exec sql endselect;
            ManageSqlError(&sqlca); // Keep track of the SQL error if any
          }
          i[thdl]++;
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
          pcur[thdl]->uData.IceTransactionDta.svrdata= *lp; 
          suppspace(name3);
          suppspace(bufnew1);
          suppspace(bufnew2);
          suppspace(bufnew3);
          suppspace(bufnew4);
          fstrncpy(pcur[thdl]->uData.IceTransactionDta.trans_key, bufnew1,
                                    sizeof(pcur[thdl]->uData.IceTransactionDta.trans_key));
          fstrncpy(pcur[thdl]->uData.IceTransactionDta.name, bufnew2,
                                    sizeof(pcur[thdl]->uData.IceTransactionDta.name));
          fstrncpy(pcur[thdl]->uData.IceTransactionDta.connect, bufnew3,
                                    sizeof(pcur[thdl]->uData.IceTransactionDta.connect));
          fstrncpy(pcur[thdl]->uData.IceTransactionDta.owner, bufnew4,
                                    sizeof(pcur[thdl]->uData.IceTransactionDta.owner));

          pcur[thdl]=GetNextLLData(pcur[thdl]);
          if (!pcur[thdl]) {
            iresselect = RES_ERR;
            exec sql endselect;
            ManageSqlError(&sqlca); // Keep track of the SQL error if any
          }
          i[thdl]++;
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
          fstrncpy(pcur[thdl]->uData.IceCursorDta.curs_key, bufnew1,
                                    sizeof(pcur[thdl]->uData.IceCursorDta.curs_key));
          fstrncpy(pcur[thdl]->uData.IceCursorDta.name, bufnew2,
                                    sizeof(pcur[thdl]->uData.IceCursorDta.name));
          fstrncpy(pcur[thdl]->uData.IceCursorDta.query, bufnew3,
                                    sizeof(pcur[thdl]->uData.IceCursorDta.query));
          fstrncpy(pcur[thdl]->uData.IceCursorDta.owner, bufnew4,
                                    sizeof(pcur[thdl]->uData.IceCursorDta.owner));

          pcur[thdl]=GetNextLLData(pcur[thdl]);
          if (!pcur[thdl]) {
            iresselect = RES_ERR;
            exec sql endselect;
            ManageSqlError(&sqlca); // Keep track of the SQL error if any
          }
          i[thdl]++;
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
          pcur[thdl]->uData.IceCacheInfoDta.svrdata= *lp; 
          suppspace(name3);
          suppspace(bufnew1);
          suppspace(bufnew2);
          suppspace(bufnew3);
          suppspace(bufnew4);
          suppspace(bufnew5);
          fstrncpy(pcur[thdl]->uData.IceCacheInfoDta.cache_index, bufnew1,
                                    sizeof(pcur[thdl]->uData.IceCacheInfoDta.cache_index));
          fstrncpy(pcur[thdl]->uData.IceCacheInfoDta.name, bufnew2,
                                    sizeof(pcur[thdl]->uData.IceCacheInfoDta.name));
          fstrncpy(pcur[thdl]->uData.IceCacheInfoDta.loc_name, bufnew3,
                                    sizeof(pcur[thdl]->uData.IceCacheInfoDta.loc_name));
          fstrncpy(pcur[thdl]->uData.IceCacheInfoDta.status, bufnew4,
                                    sizeof(pcur[thdl]->uData.IceCacheInfoDta.status));
          fstrncpy(pcur[thdl]->uData.IceCacheInfoDta.owner, bufnew5,
                                    sizeof(pcur[thdl]->uData.IceCacheInfoDta.owner));
          pcur[thdl]->uData.IceCacheInfoDta.counter   = intno;
          pcur[thdl]->uData.IceCacheInfoDta.iexist    = intno2;
          pcur[thdl]->uData.IceCacheInfoDta.itimeout  = intno3;
          pcur[thdl]->uData.IceCacheInfoDta.in_use    = intno4;
          pcur[thdl]->uData.IceCacheInfoDta.req_count = intno5;
          pcur[thdl]=GetNextLLData(pcur[thdl]);
          if (!pcur[thdl]) {
            iresselect = RES_ERR;
            exec sql endselect;
            ManageSqlError(&sqlca); // Keep track of the SQL error if any
          }
          i[thdl]++;
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
          fstrncpy(pcur[thdl]->uData.IceDbConnectionDta.sessionkey, bufnew1,
                                    sizeof(pcur[thdl]->uData.IceDbConnectionDta.sessionkey));
          fstrncpy(pcur[thdl]->uData.IceDbConnectionDta.dbname, bufnew2,
                                    sizeof(pcur[thdl]->uData.IceDbConnectionDta.dbname));
          pcur[thdl]->uData.IceDbConnectionDta.driver   = intno;
          pcur[thdl]->uData.IceDbConnectionDta.iused    = intno2;
          pcur[thdl]->uData.IceDbConnectionDta.itimeout = intno3;
          pcur[thdl]=GetNextLLData(pcur[thdl]);
          if (!pcur[thdl]) {
            iresselect = RES_ERR;
            exec sql endselect;
            ManageSqlError(&sqlca); // Keep track of the SQL error if any
          }
          i[thdl]++;
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
          fstrncpy(pcur[thdl]->uData.SvrData.name_server   ,singlename     ,
                                          sizeof(pcur[thdl]->uData.SvrData.name_server));
          fstrncpy(pcur[thdl]->uData.SvrData.listen_address,singleownername,
                                          sizeof(pcur[thdl]->uData.SvrData.listen_address));
          fstrncpy(pcur[thdl]->uData.SvrData.server_class  ,name3          ,
                                          sizeof(pcur[thdl]->uData.SvrData.server_class));
          fstrncpy(pcur[thdl]->uData.SvrData.server_dblist,bufnew1        ,
                                          sizeof(pcur[thdl]->uData.SvrData.server_dblist));
          if (null1==-1)
             x_strcpy(bufnew2,"");
          if (null2 == -1) {
             pcur[thdl]->uData.SvrData.bHasServerPid = FALSE;
             pcur[thdl]->uData.SvrData.serverPid = -1L;
          }
          else {
             pcur[thdl]->uData.SvrData.bHasServerPid = TRUE;
             pcur[thdl]->uData.SvrData.serverPid =intno;
          }
          fstrncpy(pcur[thdl]->uData.SvrData.listen_state,bufnew2        ,
                                          sizeof(pcur[thdl]->uData.SvrData.listen_state));
          if (null3==-1)
             x_strcpy(bufnew3,"");



          pcur[thdl]->uData.SvrData.servertype = FindClassServerType(name3,bufnew3);

          pcur[thdl]->uData.SvrData.bMultipleWithSameName = FALSE;

          pcur[thdl]=GetNextLLData(pcur[thdl]);
          if (!pcur[thdl]) {
            iresselect = RES_ERR;
            exec sql endselect;
            ManageSqlError(&sqlca); // Keep track of the SQL error if any
          }
          i[thdl]++;
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
          pcur[thdl]->uData.SessionData.sesssvrdata= *lp;

          if (x_strncmp(bufnew7,"00",2) == 0)
            x_strcpy(pcur[thdl]->uData.SessionData.session_wait_reason,"COM");
          else
            fstrncpy(pcur[thdl]->uData.SessionData.session_wait_reason, bufnew7,
                                    sizeof(pcur[thdl]->uData.SessionData.session_wait_reason));

          fstrncpy(pcur[thdl]->uData.SessionData.session_name, bufnew6,
                                    sizeof(pcur[thdl]->uData.SessionData.session_name));
        
          fstrncpy(pcur[thdl]->uData.SessionData.session_id, name3,
                                    sizeof(pcur[thdl]->uData.SessionData.session_id));

          suppspace(bufev2);
                if (x_strncmp(bufev2,"<no_",4) == 0)
            wsprintf(pcur[thdl]->uData.SessionData.db_name,LIBMON_getNoneString());
                else 
            fstrncpy(pcur[thdl]->uData.SessionData.db_name, bufev2,
                                        sizeof(pcur[thdl]->uData.SessionData.db_name));
                    
          fstrncpy(pcur[thdl]->uData.SessionData.server_facility, rt1,
                                        sizeof(pcur[thdl]->uData.SessionData.server_facility));

          fstrncpy(pcur[thdl]->uData.SessionData.session_query, bufnew8,
                                      sizeof(pcur[thdl]->uData.SessionData.session_query));

          suppspace(bufnew1);
          if (bufnew1[0] == '\0' || x_stricmp(bufnew1,"NONE") == 0)
            x_strcpy(pcur[thdl]->uData.SessionData.session_terminal,LIBMON_getNoneString());
          else
            fstrncpy(pcur[thdl]->uData.SessionData.session_terminal, bufnew1,
                                        sizeof(pcur[thdl]->uData.SessionData.session_terminal));

          pcur[thdl]->uData.SessionData.server_pid=intno;
        
          fstrncpy(pcur[thdl]->uData.SessionData.client_pid, bufnew2,
                                        sizeof(pcur[thdl]->uData.SessionData.client_pid));
          suppspace(bufnew4);
          suppspace(bufnew5);
          fstrncpy(pcur[thdl]->uData.SessionData.effective_user, bufnew4,
                                        sizeof(pcur[thdl]->uData.SessionData.effective_user));
          fstrncpy(pcur[thdl]->uData.SessionData.real_user, bufnew5,
                                        sizeof(pcur[thdl]->uData.SessionData.real_user));
          pcur[thdl]->uData.SessionData.session_state_num=intno2;
          fstrncpy(pcur[thdl]->uData.SessionData.session_state, selectstatement,
                                        sizeof(pcur[thdl]->uData.SessionData.session_state));
          pcur[thdl]->uData.SessionData.application_code=intno3;
          pcur[thdl]->uData.SessionData.bMultipleWithSameName = FALSE;

          pcur[thdl]=GetNextLLData(pcur[thdl]);
          if (!pcur[thdl]) {
            iresselect = RES_ERR;
            exec sql endselect;
            ManageSqlError(&sqlca); // Keep track of the SQL error if any
          }
          i[thdl]++;
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
          pcur[thdl]->uData.LockListData.locklist_id            = intno;
          pcur[thdl]->uData.LockListData.locklist_logical_count = intno2;  // logical count
          pcur[thdl]->uData.LockListData.locklist_lock_count    = intno3;  // lock count
          pcur[thdl]->uData.LockListData.locklist_max_locks     = intno4;
          pcur[thdl]->uData.LockListData.locklist_tx_high       = intno5;
          pcur[thdl]->uData.LockListData.locklist_tx_low        = intno6;
          pcur[thdl]->uData.LockListData.locklist_server_pid    = intno7;
          pcur[thdl]->uData.LockListData.locklist_wait_id       = intno9;
          pcur[thdl]->uData.LockListData.locklist_related_llb   = intno10;
          pcur[thdl]->uData.LockListData.bIs4AllLockLists       = FALSE;

          suppspace(bufev1);
          if (bufev1[0] == '\0' || x_stricmp(bufev1,"NONE") == 0)
            x_strcpy(bufev1,LIBMON_getNoneString());
          fstrncpy(pcur[thdl]->uData.LockListData.locklist_status, bufev1,
                            sizeof(pcur[thdl]->uData.LockListData.locklist_status));
          fstrncpy(pcur[thdl]->uData.LockListData.locklist_session_id, bufev2,
                            sizeof(pcur[thdl]->uData.LockListData.locklist_session_id));
        
          pcur[thdl]=GetNextLLData(pcur[thdl]);
          if (!pcur[thdl]) {
            iresselect = RES_ERR;
            exec sql endselect;
            ManageSqlError(&sqlca); // Keep track of the SQL error if any
          }
          i[thdl]++;
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
          pcur[thdl]->uData.LockData.lock_id= intno;
		  suppspace(rt1);
		  suppspace(name3);
          fstrncpy(pcur[thdl]->uData.LockData.lock_request_mode   , rt1    ,
                                    sizeof(pcur[thdl]->uData.LockData.lock_request_mode));
          fstrncpy(pcur[thdl]->uData.LockData.lock_state   , name3    ,
                                    sizeof(pcur[thdl]->uData.LockData.lock_state));
          fstrncpy(pcur[thdl]->uData.LockData.resource_key  , singlename   ,
                                    sizeof(pcur[thdl]->uData.LockData.resource_key));
          pcur[thdl]->uData.LockData.resource_type          = intno2;
          switch (pcur[thdl]->uData.LockData.resource_type)  {
              case 1:
                pcur[thdl]->uData.LockData.locktype = LOCK_TYPE_DB;
                break;
              case 2:
                pcur[thdl]->uData.LockData.locktype = LOCK_TYPE_TABLE;
                break;
              case 3:
                pcur[thdl]->uData.LockData.locktype = LOCK_TYPE_PAGE;
                break;
              default :
                pcur[thdl]->uData.LockData.locktype = LOCK_TYPE_OTHER;
          }

           
          pcur[thdl]->uData.LockData.resource_table_id      = intno3;
          pcur[thdl]->uData.LockData.resource_database_id   = intno4;
          pcur[thdl]->uData.LockData.resource_page_number   = intno5;
          pcur[thdl]->uData.LockData.resource_id            = intno6;
          pcur[thdl]->uData.LockData.locklist_id            = intno7;
          pcur[thdl]->uData.LockData.resource_index_id      = intno8;
          pcur[thdl]->uData.LockData.hdl                    = hnodetmp;
          LIBMON_FillMonLockDBName(&(pcur[thdl]->uData.LockData), thdl);
          pcur[thdl]=GetNextLLData(pcur[thdl]);

          if (!pcur[thdl]) {
              iresselect = RES_ERR;
              exec sql endselect;
              ManageSqlError(&sqlca); // Keep track of the SQL error if any
          }
          i[thdl]++;
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
          pcur[thdl]->uData.LogProcessData.log_process_id     = (65536 *intno10)+intno;
          pcur[thdl]->uData.LogProcessData.process_pid        = intno2;
          pcur[thdl]->uData.LogProcessData.process_status_num = intno3;
          pcur[thdl]->uData.LogProcessData.process_count      = intno4;
          pcur[thdl]->uData.LogProcessData.process_writes     = intno5;
          pcur[thdl]->uData.LogProcessData.process_log_forces = intno6;
          pcur[thdl]->uData.LogProcessData.process_waits      = intno7;
          pcur[thdl]->uData.LogProcessData.process_tx_begins  = intno8;
          pcur[thdl]->uData.LogProcessData.process_tx_ends    = intno9;

          wsprintf(pcur[thdl]->uData.LogProcessData.LogProcessName,"%8lx",
                                         pcur[thdl]->uData.LogProcessData.log_process_id);
          decodeStatusVal(pcur[thdl]->uData.LogProcessData.process_status_num,
                          pcur[thdl]->uData.LogProcessData.process_status);

          //fstrncpy(pcur->uData.LogProcessData.process_status   , extradata    ,
          //                                sizeof(pcur->uData.LogProcessData.process_status));

          pcur[thdl]=GetNextLLData(pcur[thdl]);
          if (!pcur[thdl]) {
            iresselect = RES_ERR;
            exec sql endselect;
            ManageSqlError(&sqlca); // Keep track of the SQL error if any
          }
          i[thdl]++;
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
          fstrncpy(pcur[thdl]->uData.LogDBdata.db_name, singlename ,
                                      sizeof(pcur[thdl]->uData.LogDBdata.db_name));
          fstrncpy(pcur[thdl]->uData.LogDBdata.db_status, selectstatement ,
                                      sizeof(pcur[thdl]->uData.LogDBdata.db_status));

          pcur[thdl]->uData.LogDBdata.db_tx_count  = intno;
          pcur[thdl]->uData.LogDBdata.db_tx_begins = intno2;
          pcur[thdl]->uData.LogDBdata.db_tx_ends   = intno3;
          pcur[thdl]->uData.LogDBdata.db_reads     = intno4;
          pcur[thdl]->uData.LogDBdata.db_writes    = intno5;

          pcur[thdl]=GetNextLLData(pcur[thdl]);
          if (!pcur[thdl]) {
            iresselect = RES_ERR;
            exec sql endselect;
            ManageSqlError(&sqlca); // Keep track of the SQL error if any
          }
          i[thdl]++;
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

            pcur[thdl]->uData.LogTransacData.tx_id_id       = intno;
            pcur[thdl]->uData.LogTransacData.tx_id_instance = intno2;

            pcur[thdl]->uData.LogTransacData.tx_transaction_high = intno4;
            pcur[thdl]->uData.LogTransacData.tx_transaction_low  = intno5;
            
            pcur[thdl]->uData.LogTransacData.tx_state_split = intno6;
            pcur[thdl]->uData.LogTransacData.tx_state_write = intno7;
			pcur[thdl]->uData.LogTransacData.tx_server_pid  = intno8;

            fstrncpy(pcur[thdl]->uData.LogTransacData.tx_transaction_id, bufnew4 ,
                                        sizeof(pcur[thdl]->uData.LogTransacData.tx_transaction_id));
            suppspace(bufnew1);
            suppspace(bufnew2);
            fstrncpy(pcur[thdl]->uData.LogTransacData.tx_db_name, bufnew1 ,
                                        sizeof(pcur[thdl]->uData.LogTransacData.tx_db_name));
            fstrncpy(pcur[thdl]->uData.LogTransacData.tx_user_name, bufnew2 ,
                                        sizeof(pcur[thdl]->uData.LogTransacData.tx_user_name));
            fstrncpy(pcur[thdl]->uData.LogTransacData.tx_status, extradata ,
                                        sizeof(pcur[thdl]->uData.LogTransacData.tx_status));
            fstrncpy(pcur[thdl]->uData.LogTransacData.tx_session_id, bufnew3 ,
                                        sizeof(pcur[thdl]->uData.LogTransacData.tx_session_id));
            // Not in the Table TO BE FINISHED  
                  //pcur->uData.LogTransacData.stat_write = intno;
                  //pcur->uData.LogTransacData.stat_split = intno2;
            pcur[thdl]=GetNextLLData(pcur[thdl]);
            if (!pcur[thdl]) {
              iresselect = RES_ERR;
              exec sql endselect;
              ManageSqlError(&sqlca); // Keep track of the SQL error if any
            }
            i[thdl]++;
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
          fstrncpy(pcur[thdl]->uData.ResourceData.resource_grant_mode   , bufev1    ,
                                          sizeof(pcur[thdl]->uData.ResourceData.resource_grant_mode));
          fstrncpy(pcur[thdl]->uData.ResourceData.resource_convert_mode, bufev2    ,
                                          sizeof(pcur[thdl]->uData.ResourceData.resource_convert_mode));
          fstrncpy(pcur[thdl]->uData.ResourceData.resource_key, singlename  ,
                                          sizeof(pcur[thdl]->uData.ResourceData.resource_key));
          pcur[thdl]->uData.ResourceData.resource_id           = intno;
          pcur[thdl]->uData.ResourceData.resource_type         = intno2;
          pcur[thdl]->uData.ResourceData.resource_database_id  = intno3;
          pcur[thdl]->uData.ResourceData.resource_table_id     = intno4;
          pcur[thdl]->uData.ResourceData.resource_index_id     = intno5;
          pcur[thdl]->uData.ResourceData.resource_page_number  = intno6;
          pcur[thdl]->uData.ResourceData.resource_row_id       = intno7;
          //pcur->uData.ResourceData.resource_key6         = intno8;

          pcur[thdl]->uData.ResourceData.hdl                   = hnodetmp;
          LIBMON_FillMonResourceDBName(&(pcur[thdl]->uData.ResourceData), thdl);

          pcur[thdl]=GetNextLLData(pcur[thdl]);
          if (!pcur[thdl]) {
            iresselect = RES_ERR;
            exec sql endselect;
            ManageSqlError(&sqlca); // Keep track of the SQL error if any
          }
          i[thdl]++;
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
          fstrncpy(pcur[thdl]->uData.ReplicSrvDta.ParentDataBaseName,
                   LIBMON_GetMonInfoName(hnodetmp, OT_DATABASE, parentstrings, singlename,sizeof(singlename)-1, thdl),
                   sizeof(pcur[thdl]->uData.ReplicSrvDta.ParentDataBaseName));
          if (null1==-1)
             x_strcpy(bufnew3,bufnew1); 
          suppspace(bufnew1);
          suppspace(bufnew2);
          suppspace(bufnew3);
          fstrncpy(pcur[thdl]->uData.ReplicSrvDta.LocalDBNode,bufnew1,
                                      sizeof(pcur[thdl]->uData.ReplicSrvDta.LocalDBNode));
          fstrncpy(pcur[thdl]->uData.ReplicSrvDta.LocalDBName,bufnew2,
                                      sizeof(pcur[thdl]->uData.ReplicSrvDta.LocalDBName));
          fstrncpy(pcur[thdl]->uData.ReplicSrvDta.RunNode,bufnew3,
                                      sizeof(pcur[thdl]->uData.ReplicSrvDta.RunNode));
          pcur[thdl]->uData.ReplicSrvDta.serverno     = intno;
          pcur[thdl]->uData.ReplicSrvDta.localdb      = intno2;
          pcur[thdl]->uData.ReplicSrvDta.startstatus  = REPSVR_UNKNOWNSTATUS;
          pcur[thdl]->uData.ReplicSrvDta.DBEhdl       = DBEHDL_NOTASSIGNED;  // managed by background task
          pcur[thdl]->uData.ReplicSrvDta.FullStatus[0]='\0'; // managed by background task
          pcur[thdl]->uData.ReplicSrvDta.bMultipleWithDiffLocalDB= FALSE;

          pcur[thdl]=GetNextLLData(pcur[thdl]);
          if (!pcur[thdl]) {
            iresselect = RES_ERR;
            exec sql endselect;
            ManageSqlError(&sqlca); // Keep track of the SQL error if any
          }
          i[thdl]++;
        exec sql end;
        ManageSqlError(&sqlca); // Keep track of the SQL error if any
      }
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
        pcur[thdl]->uData.ReplicCddsDta.Database_No = intno;
        fstrncpy(pcur[thdl]->uData.ReplicCddsDta.szVnodeName,bufnew1, 
                    sizeof(pcur[thdl]->uData.ReplicCddsDta.szVnodeName));
        fstrncpy(pcur[thdl]->uData.ReplicCddsDta.szDBName,bufnew2,
                    sizeof(pcur[thdl]->uData.ReplicCddsDta.szDBName));
        fstrncpy(pcur[thdl]->uData.ReplicCddsDta.szCddsName,bufnew3,
                    sizeof(pcur[thdl]->uData.ReplicCddsDta.szCddsName));

        pcur[thdl]->uData.ReplicCddsDta.Cdds_No = intno2;
        switch (intno3)   {
            case REPLIC_FULLPEER:
                fstrncpy(pcur[thdl]->uData.ReplicCddsDta.szTargetType,"Full Peer",
                         sizeof(pcur[thdl]->uData.ReplicCddsDta.szTargetType));
            break ;
            case REPLIC_PROT_RO:
                fstrncpy(pcur[thdl]->uData.ReplicCddsDta.szTargetType,"Protected Read-only",
                         sizeof(pcur[thdl]->uData.ReplicCddsDta.szTargetType));
            break;
            case REPLIC_UNPROT_RO:
                fstrncpy(pcur[thdl]->uData.ReplicCddsDta.szTargetType,"Unprotected Read-only",
                         sizeof(pcur[thdl]->uData.ReplicCddsDta.szTargetType));
            break;
            default:
                fstrncpy(pcur[thdl]->uData.ReplicCddsDta.szTargetType,"Unknown type",
                         sizeof(pcur[thdl]->uData.ReplicCddsDta.szTargetType));
        }
      pcur[thdl]=GetNextLLData(pcur[thdl]);
      if (!pcur[thdl]) {
        iresselect = RES_ERR;
        exec sql endselect;
        ManageSqlError(&sqlca); // Keep track of the SQL error if any
      }
      i[thdl]++;
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
              fstrncpy(pcur[thdl]->Name,(LPTSTR)Quote4DisplayIfNeeded(singlename),MAXOBJECTNAME);
              fstrncpy(pcur[thdl]->OwnerName,singleownername,MAXOBJECTNAME);
              fstrncpy(pcur[thdl]->ExtraData,name3,MAXOBJECTNAME);
              suppspace(pcur[thdl]->ExtraData);
              pcur[thdl]->ExtraData[MAXOBJECTNAME]  =GetYNState(rt1); // indexes
              pcur[thdl]->ExtraData[MAXOBJECTNAME+1]=GetYNState(rt2); // integr.
              pcur[thdl]->ExtraData[MAXOBJECTNAME+2]=GetYNState(rt3); // base_v
              pcur[thdl]->ExtraData[MAXOBJECTNAME+3]=GetYNState(rt4); // multiloc
              storeint(pcur[thdl]->ExtraData+MAXOBJECTNAME+4, -1);    // DUMMY ID FOR MONITORING
              iStarType=OBJTYPE_UNKNOWN;
              if (rt5[0]=='N' || rt5[0]=='n')
                iStarType=OBJTYPE_STARNATIVE;
              if (rt5[0]=='L' || rt5[0]=='l')
                iStarType=OBJTYPE_STARLINK;
              storeint(pcur[thdl]->ExtraData+MAXOBJECTNAME+4+STEPSMALLOBJ, iStarType);  
              pcur[thdl]=GetNextLLData(pcur[thdl]);
              if (!pcur[thdl]) {
                iresselect = RES_ERR;
                exec sql endselect;
                ManageSqlError(&sqlca); // Keep track of the SQL error if any
              }
              i[thdl]++;
            exec sql end;
            ManageSqlError(&sqlca); // Keep track of the SQL error if any
            break;
          }
          if (GetOIVers() == OIVERS_NOTOI) {
             intno=-1;   // reltid = -1 (dummy if gateway or <Oping 1.x )
             exec sql repeated select table_name,table_owner,table_indexes,
                             table_integrities,view_base,location_name, multi_locations
                        
                    into :singlename,:singleownername,:rt1,
                       :rt2,:rt3,:name3,:rt4
                    from iitables t
                    where table_type='T';

             exec sql begin;
               ManageSqlError(&sqlca); // Keep track of the SQL error if any
				if (HasSelectLoopInterruptBeenRequested()) {
					iresselect = RES_ERR;
					exec sql endselect;
				}

               fstrncpy(pcur[thdl]->Name,singlename,MAXOBJECTNAME);
               fstrncpy(pcur[thdl]->OwnerName,singleownername,MAXOBJECTNAME);
               fstrncpy(pcur[thdl]->ExtraData,name3,MAXOBJECTNAME);
               suppspace(pcur[thdl]->ExtraData);
               pcur[thdl]->ExtraData[MAXOBJECTNAME]  =GetYNState(rt1); // indexes
               pcur[thdl]->ExtraData[MAXOBJECTNAME+1]=GetYNState(rt2); // integr.
               pcur[thdl]->ExtraData[MAXOBJECTNAME+2]=GetYNState(rt3); // base_v
               pcur[thdl]->ExtraData[MAXOBJECTNAME+3]=GetYNState(rt4); // multiloc
               storeint(pcur[thdl]->ExtraData+MAXOBJECTNAME+4,intno); // table id for monitoring
               storeint(pcur[thdl]->ExtraData+MAXOBJECTNAME+4+STEPSMALLOBJ, OBJTYPE_NOTSTAR); 
               pcur[thdl]=GetNextLLData(pcur[thdl]);
               if (!pcur[thdl]) {
                 iresselect = RES_ERR;
                 exec sql endselect;
                 ManageSqlError(&sqlca); // Keep track of the SQL error if any
               }
               i[thdl]++;
             exec sql end;
             ManageSqlError(&sqlca); // Keep track of the SQL error if any
             break;
           }
           exec sql repeated select table_name,table_owner,table_indexes,
                       table_integrities,view_base,location_name, multi_locations,
                       reltid
                    into :singlename,:singleownername,:rt1,
                       :rt2,:rt3,:name3,:rt4,:intno
                    from iitables t,iirelation r
                    where table_type='T' and t.table_name=r.relid and t.table_owner=r.relowner;

           exec sql begin;
             ManageSqlError(&sqlca); // Keep track of the SQL error if any
             if (HasSelectLoopInterruptBeenRequested()) {
               iresselect = RES_ERR;
               exec sql endselect;
             }
             suppspacebutleavefirst(singlename);
             suppspace(singleownername);
             fstrncpy(pcur[thdl]->Name,(LPTSTR)Quote4DisplayIfNeeded(singlename),MAXOBJECTNAME);
             fstrncpy(pcur[thdl]->OwnerName,singleownername,MAXOBJECTNAME);
             fstrncpy(pcur[thdl]->ExtraData,name3,MAXOBJECTNAME);
             suppspace(pcur[thdl]->ExtraData);
             pcur[thdl]->ExtraData[MAXOBJECTNAME]  =GetYNState(rt1); // indexes
             pcur[thdl]->ExtraData[MAXOBJECTNAME+1]=GetYNState(rt2); // integr.
             pcur[thdl]->ExtraData[MAXOBJECTNAME+2]=GetYNState(rt3); // base_v
             pcur[thdl]->ExtraData[MAXOBJECTNAME+3]=GetYNState(rt4); // multiloc
             storeint(pcur[thdl]->ExtraData+MAXOBJECTNAME+4,intno); // table id for monitoring
             storeint(pcur[thdl]->ExtraData+MAXOBJECTNAME+4+STEPSMALLOBJ, OBJTYPE_NOTSTAR); 
             pcur[thdl]=GetNextLLData(pcur[thdl]);
             if (!pcur[thdl]) {
               iresselect = RES_ERR;
               exec sql endselect;
               ManageSqlError(&sqlca); // Keep track of the SQL error if any
             }
             i[thdl]++;
           exec sql end;
           ManageSqlError(&sqlca); // Keep track of the SQL error if any
      }
      break;
    case OT_DATABASE:
      {
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
          fstrncpy(pcur[thdl]->Name,singlename,MAXOBJECTNAME);
          fstrncpy(pcur[thdl]->OwnerName,singleownername,MAXOBJECTNAME);
          storeint(pcur[thdl]->ExtraData,intno);
          storeint(pcur[thdl]->ExtraData+STEPSMALLOBJ,intno2);
          if (null1==-1)
             bufnew2[0]='\0';
          suppspace(bufnew2);
          x_strcpy(pcur[thdl]->ExtraData+2*STEPSMALLOBJ,bufnew2);
          pcur[thdl]=GetNextLLData(pcur[thdl]);
          if (!pcur[thdl]) {
            iresselect = RES_ERR;
            exec sql endselect;
            ManageSqlError(&sqlca); // Keep track of the SQL error if any
          }
          i[thdl]++;
        exec sql end;
        ManageSqlError(&sqlca); // Keep track of the SQL error if any
        break;
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
          fstrncpy(pcur[thdl]->Name,singlename,MAXOBJECTNAME);
          pcur[thdl]->ExtraData[0]=GetYNState(extradata);
                  pcur[thdl]=GetNextLLData(pcur[thdl]);
          if (!pcur[thdl]) {
            iresselect = RES_ERR;
            exec sql endselect;
            ManageSqlError(&sqlca); // Keep track of the SQL error if any
          }
          i[thdl]++;
        exec sql end;
        ManageSqlError(&sqlca); // Keep track of the SQL error if any

                        /* add user "(public)" */
        if (pcur[thdl]) {
          /* Emb 17/10/95 : don't add if there was a memory error in the select */
          fstrncpy(pcur[thdl]->Name,lppublicdispstring(),MAXOBJECTNAME);
          pcur[thdl]=GetNextLLData(pcur[thdl]);
        }
        if (!pcur[thdl]) {
          iresselect = RES_ERR;
        }
        i[thdl]++;
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
             fstrncpy(pcur[thdl]->Name,singlename,MAXOBJECTNAME);
             storeint(pcur[thdl]->ExtraData,
                      IngGetStringType(singleownername, intno));
             storeint(pcur[thdl]->ExtraData+STEPSMALLOBJ,intno);
             pcur[thdl]->ExtraData[2*STEPSMALLOBJ  ]=GetYNState(rt1);
             pcur[thdl]->ExtraData[2*STEPSMALLOBJ+1]=GetYNState(rt2);
             storeint(pcur[thdl]->ExtraData+4*STEPSMALLOBJ ,intno2);
             pcur[thdl]=GetNextLLData(pcur[thdl]);
             if (!pcur[thdl]) {
               iresselect = RES_ERR;
               exec sql endselect;
               ManageSqlError(&sqlca); // Keep track of the SQL error if any
             }
             i[thdl]++;
           exec sql end;
           ManageSqlError(&sqlca); // Keep track of the SQL error if any
      }
      break;
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
          fstrncpy(pcur[thdl]->Name,(LPTSTR)Quote4DisplayIfNeeded(singlename),MAXOBJECTNAME);
          fstrncpy(pcur[thdl]->OwnerName,singleownername,MAXOBJECTNAME);
          pcur[thdl]=GetNextLLData(pcur[thdl]);
          if (!pcur[thdl]) {
            iresselect = RES_ERR;
            exec sql endselect;
            ManageSqlError(&sqlca); // Keep track of the SQL error if any
          }
          i[thdl]++;
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
          fstrncpy(pcur[thdl]->Name,singlename,MAXOBJECTNAME);
          fstrncpy(pcur[thdl]->OwnerName,singleownername,MAXOBJECTNAME);
          storeint(pcur[thdl]->ExtraData,                  intno );
          storeint(pcur[thdl]->ExtraData+STEPSMALLOBJ,     intno2);
          storeint(pcur[thdl]->ExtraData+(2*STEPSMALLOBJ), intno3);
          pcur[thdl]=GetNextLLData(pcur[thdl]);
          if (!pcur[thdl]) {
            iresselect = RES_ERR;
            exec sql endselect;
            ManageSqlError(&sqlca); // Keep track of the SQL error if any
          }
          i[thdl]++;
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
          fstrncpy(pcur[thdl]->Name,singlename,MAXOBJECTNAME);
          storeint(pcur[thdl]->ExtraData,                  intno );
          storeint(pcur[thdl]->ExtraData+STEPSMALLOBJ,    (BOOL)(rt1[0]=='Y'));
          storeint(pcur[thdl]->ExtraData+(2*STEPSMALLOBJ),(BOOL)(rt2[0]=='Y'));
          storeint(pcur[thdl]->ExtraData+(3*STEPSMALLOBJ),(BOOL)(rt3[0]=='Y'));
          pcur[thdl]=GetNextLLData(pcur[thdl]);
          if (!pcur[thdl]) {
            iresselect = RES_ERR;
            exec sql endselect;
            ManageSqlError(&sqlca); // Keep track of the SQL error if any
          }
          i[thdl]++;
        exec sql end;
        ManageSqlError(&sqlca); // Keep track of the SQL error if any
      }
      break;
    case OT_REPLIC_REGTBLS_V11:
       {
        i[thdl]=0;
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
          pcur[thdl]->uData.Registred_Tables.table_no = intno;

          // table_name
          suppspace(singlename);
          x_strcpy(pcur[thdl]->uData.Registred_Tables.tablename,singlename);

          
          // columns_registred
          suppspace(bufev1);
          pcur[thdl]->uData.Registred_Tables.colums_registered[0] = ( bufev1[0] == 0 ) ? 0 : 0x0043; // 'C'
          pcur[thdl]->uData.Registred_Tables.colums_registered[1] = 0;

          // supp_objs_created new for replicator 1.1
          suppspace(bufev2);
          pcur[thdl]->uData.Registred_Tables.table_created[0]     = ( bufev2[0] == 0 ) ? 0 : 0x0054; // 'T';
          pcur[thdl]->uData.Registred_Tables.table_created_ini[0] = pcur[thdl]->uData.Registred_Tables.table_created[0];
          pcur[thdl]->uData.Registred_Tables.table_created[1]     = 0;
          pcur[thdl]->uData.Registred_Tables.table_created_ini[1] = 0;
          x_strcpy(pcur[thdl]->uData.Registred_Tables.szDate_table_created,bufev2);              

          //rules_created 
          suppspace(bufnew2);
          pcur[thdl]->uData.Registred_Tables.rules_created[0]     = ( bufnew2[0] == 0 )? 0 : 0x0052; // 'R'
          pcur[thdl]->uData.Registred_Tables.rules_created_ini[0] = pcur[thdl]->uData.Registred_Tables.rules_created[0];
          pcur[thdl]->uData.Registred_Tables.rules_created[1]     = 0;
          pcur[thdl]->uData.Registred_Tables.rules_created_ini[1] = 0;
          x_strcpy(pcur[thdl]->uData.Registred_Tables.szDate_rules_created,bufnew2);
          
          // cdds_no
          pcur[thdl]->uData.Registred_Tables.cdds = intno2;

          suppspace(bufnew1);
          x_strcpy(pcur[thdl]->uData.Registred_Tables.tableowner,bufnew1);
          
          suppspace(singleownername);
          x_strcpy(pcur[thdl]->uData.Registred_Tables.cdds_lookup_table_v11,singleownername);

          // prio_lookup_table
          suppspace(name3);
          x_strcpy(pcur[thdl]->uData.Registred_Tables.priority_lookup_table_v11,name3);
          
          // index_used
          suppspace(bufnew3);
          x_strcpy(pcur[thdl]->uData.Registred_Tables.IndexName,bufnew3);

          pcur[thdl]=GetNextLLData(pcur[thdl]);
          if (!pcur[thdl]) {
            iresselect = RES_ERR;
            exec sql endselect;
            ManageSqlError(&sqlca); // Keep track of the SQL error if any
          }
          i[thdl]++;
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
          fstrncpy(pcur[thdl]->Name,singlename,MAXOBJECTNAME);
          fstrncpy(pcur[thdl]->OwnerName,singleownername,MAXOBJECTNAME);
          storeint(pcur[thdl]->ExtraData,                  intno );
//          storeint(pcur[thdl]->ExtraData+STEPSMALLOBJ,     intno2);  // removed 20-Jan-2000: server no nore attached at that level for replicator 1.1 and later
          storeint(pcur[thdl]->ExtraData+(2*STEPSMALLOBJ), intno3);
          fstrncpy(pcur[thdl]->ExtraData+(3*STEPSMALLOBJ), bufev1,(MAXOBJECTNAME/2));
          //fstrncpy(pcur->ExtraData+(3*STEPSMALLOBJ)+(MAXOBJECTNAME/2), selectstatement,80); // 80 Len of field Remark 
          // to be finished
        pcur[thdl]=GetNextLLData(pcur[thdl]);
        if (!pcur[thdl]) {
          iresselect = RES_ERR;
          exec sql endselect;
          ManageSqlError(&sqlca); // Keep track of the SQL error if any
        }
        i[thdl]++;
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
          fstrncpy(pcur[thdl]->Name,singlename,MAXOBJECTNAME);
          storeint(pcur[thdl]->ExtraData,    intno );
          storeint(pcur[thdl]->ExtraData+STEPSMALLOBJ, intno3);
          
          pcur[thdl]=GetNextLLData(pcur[thdl]);
          if (!pcur[thdl]) {
            iresselect = RES_ERR;
            exec sql endselect;
            ManageSqlError(&sqlca); // Keep track of the SQL error if any
          }
          i[thdl]++;
        exec sql end;
        ManageSqlError(&sqlca); // Keep track of the SQL error if any
       }
       break;
// JFS Begin
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
             fstrncpy(pcur[thdl]->Name,singlename,MAXOBJECTNAME);
             fstrncpy(pcur[thdl]->OwnerName,singleownername,MAXOBJECTNAME);
             suppspace(extradata);
             x_strcpy(extradata+20,"..");

             fstrncpy(pcur[thdl]->ExtraData,extradata,MAXOBJECTNAME);
             storeint(pcur[thdl]->ExtraData+24, -1);  // DUMMY ID FOR MONITORING
             bsuppspace[thdl]=TRUE;
             pcur[thdl]=GetNextLLData(pcur[thdl]);
             if (!pcur[thdl]) {
               iresselect = RES_ERR;
               exec sql endselect;
               ManageSqlError(&sqlca); // Keep track of the SQL error if any
             }
             i[thdl]++;
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
                fstrncpy(pcur[thdl]->Name,singlename,MAXOBJECTNAME);
                fstrncpy(pcur[thdl]->OwnerName,singleownername,MAXOBJECTNAME);
                suppspace(extradata);
                x_strcpy(extradata+20,"..");

                fstrncpy(pcur[thdl]->ExtraData,extradata,MAXOBJECTNAME);
                storeint(pcur[thdl]->ExtraData+24,(-1)); /* table id for monitoring : dummy (no monitoring for gateways or Ingres <Oping 1.x */
                bsuppspace[thdl]=TRUE;
                pcur[thdl]=GetNextLLData(pcur[thdl]);
                if (!pcur[thdl]) {
                  iresselect = RES_ERR;
                  exec sql endselect;
                  ManageSqlError(&sqlca); // Keep track of the SQL error if any
                }
                i[thdl]++;
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
             fstrncpy(pcur[thdl]->Name,(LPTSTR)Quote4DisplayIfNeeded(singlename),MAXOBJECTNAME);
             fstrncpy(pcur[thdl]->OwnerName,singleownername,MAXOBJECTNAME);
             suppspace(extradata);
             x_strcpy(extradata+20,"..");

             fstrncpy(pcur[thdl]->ExtraData,extradata,MAXOBJECTNAME);
             storeint(pcur[thdl]->ExtraData+24,intno); // table id for monitoring
             bsuppspace[thdl]=TRUE;
             pcur[thdl]=GetNextLLData(pcur[thdl]);
             if (!pcur[thdl]) {
               iresselect = RES_ERR;
               exec sql endselect;
               ManageSqlError(&sqlca); // Keep track of the SQL error if any
             }
             i[thdl]++;
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

  nbItems[thdl]=i[thdl];
  {
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
  i[thdl]=0;
  SortItems(iobjecttype, thdl);
  pcur[thdl]=staticplldbadata[thdl];
  return LIBMON_DBAGetNextObject(lpobjectname,lpownername,lpextradata, thdl);
err:
  ReleaseSession(ilocsession, RELEASE_COMMIT);
  nbItems[thdl]=0;
  i[thdl]=0;
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
	/* 0 is the default thread handle for single thread application */
	return LIBMON_DBAGetFirstObject(lpVirtNode,iobjecttype,level,parentstrings,
                       bWithSystem,lpobjectname,lpownername,lpextradata,0);
}

// added Emb 26/4/95 for critical section test
int  LIBMON_DBAGetFirstObject(lpVirtNode,iobjecttype,level,parentstrings,
                       bWithSystem,lpobjectname,lpownername,lpextradata, thdl)
LPUCHAR lpVirtNode;
int     iobjecttype;
int     level;
LPUCHAR *parentstrings;
BOOL    bWithSystem;
LPUCHAR lpobjectname;
LPUCHAR lpownername;
LPUCHAR lpextradata;
int		thdl;
{
  int retval,hdl;
  int ires;
  if (iobjecttype == OT_MON_REPLIC_CDDS_ASSIGN) {
    LPREPLICSERVERDATAMIN lp = (LPREPLICSERVERDATAMIN)parentstrings;
    UCHAR buf[MAXOBJECTNAME]; 
    hdl  = LIBMON_OpenNodeStruct (lp->LocalDBNode);
    if (hdl != -1)
        ires = LIBMON_DOMGetFirstObject(hdl, OT_DATABASE, 0, NULL, FALSE, NULL, buf, NULL, NULL, thdl);
    else
        return RES_ERR;
  }
 
  EnableSqlSaveText();
  retval = DBAOrgGetFirstObject(lpVirtNode,iobjecttype,level,parentstrings,
                          bWithSystem,lpobjectname,lpownername,lpextradata, thdl);
  DisableSqlSaveText();
  if (iobjecttype == OT_MON_REPLIC_CDDS_ASSIGN) {
      LIBMON_CloseNodeStruct (hdl);
  }
  return retval;
}


int  DBAGetNextObject(lpobjectname,lpownername,lpextradata)
LPUCHAR lpobjectname;
LPUCHAR lpownername;
LPUCHAR lpextradata;
{
	/* 0 is the default thread handle for single thread application */
	return LIBMON_DBAGetNextObject(lpobjectname,lpownername,lpextradata,0);
}

int  LIBMON_DBAGetNextObject(lpobjectname,lpownername,lpextradata,thdl)
LPUCHAR lpobjectname;
LPUCHAR lpownername;
LPUCHAR lpextradata;
int		thdl;
{
  if (i[thdl]<nbItems[thdl]) {
    switch (iDBATypemem[thdl]) {
      case OT_MON_SERVER:
         *((LPSERVERDATAMIN)lpobjectname)     = pcur[thdl]->uData.SvrData;
         break;
      case OT_MON_SESSION:
         *((LPSESSIONDATAMIN)lpobjectname)    = pcur[thdl]->uData.SessionData;
         break;
      case OT_MON_LOCKLIST:
         *((LPLOCKLISTDATAMIN)lpobjectname)   = pcur[thdl]->uData.LockListData;
         break;
      case OT_MON_LOCK:
      case OT_MON_RES_LOCK:
      case OT_MON_LOCKLIST_LOCK:
         *((LPLOCKDATAMIN)lpobjectname)       = pcur[thdl]->uData.LockData;
         break;
      case OTLL_MON_RESOURCE:
         *((LPRESOURCEDATAMIN)lpobjectname)   = pcur[thdl]->uData.ResourceData;
         break;
      case OT_MON_LOGPROCESS:
         *((LPLOGPROCESSDATAMIN)lpobjectname) = pcur[thdl]->uData.LogProcessData;
         break;
      case OT_MON_LOGDATABASE    :
         *((LPLOGDBDATAMIN)lpobjectname)      = pcur[thdl]->uData.LogDBdata;
         break;
      case OT_MON_TRANSACTION:
         *((LPLOGTRANSACTDATAMIN)lpobjectname)= pcur[thdl]->uData.LogTransacData;
         break;
      case OT_REPLIC_REGTBLS_V11:
         *((LPDD_REGISTERED_TABLES)lpobjectname) = pcur[thdl]->uData.Registred_Tables;
         break;
      case OT_MON_REPLIC_SERVER:
         *((LPREPLICSERVERDATAMIN)lpobjectname) = pcur[thdl]->uData.ReplicSrvDta;
         break;
      case OT_MON_REPLIC_CDDS_ASSIGN:
         *((LPREPLICCDDSASSIGNDATAMIN)lpobjectname) = pcur[thdl]->uData.ReplicCddsDta;
         break;
      case OT_STARDB_COMPONENT:
         *((LPSTARDBCOMPONENTPARAMS)lpobjectname) = pcur[thdl]->uData.StarDbCompDta;
         break;
      case OT_MON_ICE_CONNECTED_USER:
         *((LPICE_CONNECTED_USERDATAMIN)lpobjectname) = pcur[thdl]->uData.IceConnectedUserDta;
         break;
      case OT_MON_ICE_ACTIVE_USER   :
         *((LPICE_ACTIVE_USERDATAMIN)lpobjectname) = pcur[thdl]->uData.IceActiveUserDta;
         break;
      case OT_MON_ICE_TRANSACTION   :
         *((LPICE_TRANSACTIONDATAMIN)lpobjectname) = pcur[thdl]->uData.IceTransactionDta;
         break;
      case OT_MON_ICE_CURSOR        :
         *((LPICE_CURSORDATAMIN)lpobjectname) = pcur[thdl]->uData.IceCursorDta;
         break;
      case OT_MON_ICE_FILEINFO      :
         *((LPICE_CACHEINFODATAMIN)lpobjectname) = pcur[thdl]->uData.IceCacheInfoDta;
         break;
      case OT_MON_ICE_DB_CONNECTION :
         *((LPICE_DB_CONNECTIONDATAMIN)lpobjectname) = pcur[thdl]->uData.IceDbConnectionDta;
         break;

      default:
        fstrncpy(lpobjectname,pcur[thdl]->Name,MAXOBJECTNAME);
        suppspacebutleavefirst(lpobjectname);
        if (lpownername) {
          fstrncpy(lpownername,pcur[thdl]->OwnerName,MAXOBJECTNAME);
          suppspace(lpownername);
        }
        if (lpextradata) {
          memcpy(lpextradata,pcur[thdl]->ExtraData,EXTRADATASIZE);
          if (bsuppspace[thdl])
            suppspace(lpextradata);
        }
        break;
    }
    pcur[thdl]=pcur[thdl]->pnext;
    i[thdl]++;
    return RES_SUCCESS;
  }   
  return RES_ENDOFDATA;
}

void DBAginfoFree()
{
  int i;
  int j;
  exec sql begin declare section;
    long isession;
  exec sql end declare section;

  for(j=0; j<MAX_THREADS; j++) 
  while (staticplldbadata[j]){
    pcur[j]=staticplldbadata[j]->pnext;
    ESL_FreeMem(staticplldbadata[j]);
    staticplldbadata[j]=pcur[j];
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

	if (bAllGWNodes || bAllUsers ) {
		lstrcpy(bufNodeWithoutGWandUsr,lpNodeName);
		if (bAllGWNodes)
			RemoveGWNameFromString(bufNodeWithoutGWandUsr);
		if (bAllUsers)
			RemoveConnectUserFromString(bufNodeWithoutGWandUsr);
		lpNodeName = bufNodeWithoutGWandUsr;
	}


  x_strcpy(localstring, LIBMON_getLocalNodeString() );
  fstrncpy(localhost,LIBMON_getLocalHostName(),MAXOBJECTNAME);

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




static BOOL  DBAGetSessionUserWithCheck(LPUCHAR lpVirtNode,LPUCHAR lpresultusername)
{
  char connectname[MAXOBJECTNAME];
  BOOL bResult;
  int  iret, ilocsession;
  exec sql begin declare section;
    char buf[200];
  exec sql end declare section;
  
  sprintf(connectname,"%s::%s",lpVirtNode,GlobalInfoDBName());

  iret = Getsession(connectname,SESSION_TYPE_CACHENOREADLOCK, &ilocsession);
  if (iret !=RES_SUCCESS)
    return FALSE;

  bResult = FALSE;

  exec sql repeated select dbmsinfo('session_user') into :buf;
  
  ManageSqlError(&sqlca); // Keep track of the SQL error if any
  if (sqlca.sqlcode < 0) 
     goto end;
  buf[sizeof(buf)-1]='\0';
  suppspace(buf);

  x_strcpy( lpresultusername, buf);
  bResult=TRUE;
end:
  ReleaseSession(ilocsession, RELEASE_COMMIT);
  return bResult;
}

BOOL DBAGetUserName (const unsigned char* VirtNodeName, unsigned char* UserName)
{
  return DBAGetSessionUserWithCheck((char *)VirtNodeName,UserName);
}


int LIBMON_CheckConnection(LPUCHAR lpVirtNode)
{
  char connectname[MAXOBJECTNAME];
  char szLocal[MAXOBJECTNAME];   // string for local connection
  int iResult;
  int  iret, ilocsession;
  int ioldOIVers;
 
   x_strcpy(szLocal, LIBMON_getLocalNodeString() );

  sprintf(connectname,"%s::%s",lpVirtNode,GlobalInfoDBName());

  ioldOIVers = GetOIVers();
  iret = Getsession(connectname,SESSION_TYPE_CACHENOREADLOCK | SESSION_CHECKING_CONNECTION , &ilocsession);
  if (iret !=RES_SUCCESS) {
    SetOIVers(ioldOIVers); /* restore DBMS level flag because GetSession failed, and may have changed it given the SESSION_CHECKING_CONNECTION flag */
    return RES_CONNECTIONFAILURE;
  }

  iResult=RES_SUCCESS;

  ReleaseSession(ilocsession, RELEASE_COMMIT);  
  return iResult;
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

static BOOL AddDocument(void *pDocCur , int iSess )
{
    LPDOCLIST lpDocTmp;

    LPDOCLIST pTemp = (DOCLIST *)ESL_AllocMem(sizeof(DOCLIST));
    if (!pTemp)
    {
        myerror(ERR_ALLOCMEM);
        return FALSE;
    }
    pTemp->pIpmDoc = pDocCur;

    if (!session[iSess].lpDocLst)
    {
        session[iSess].lpDocLst = pTemp;
        return TRUE;
    }

    lpDocTmp = session[iSess].lpDocLst;
    while( lpDocTmp )
    {
        if (!lpDocTmp->lpNext) // the following is empty to be add
        {
            lpDocTmp->lpNext = pTemp;
            return TRUE;
        }
        lpDocTmp = lpDocTmp->lpNext;
    }
    return FALSE;
}

LPDOCLIST SearchDocument( void *pDocInfo , int iNumSession )
{
    LPDOCLIST lpDocTmp;
    lpDocTmp = session[iNumSession].lpDocLst;
    if (!lpDocTmp)
        return NULL;
    while( lpDocTmp )
    {
        if (lpDocTmp->pIpmDoc == pDocInfo)
            return lpDocTmp;
        lpDocTmp = lpDocTmp->lpNext;
    }
    return NULL;
}

static BOOL ExtractDbEventFromMemory(void *pDocInfo , int iNumSession , LPRAISEDREPLICDBE pRaisedDBE)
{
    LPDOCLIST lpDocTemp;
    LPRAISEDDBE lpMemRaise;

    lpDocTemp = SearchDocument( pDocInfo , iNumSession );
    if (lpDocTemp)
    {
      LPOBJECTLIST lpDBevent;
      struct lcllist * plist=mydbelist;

      lpDBevent = lpDocTemp->pDBEventUntreated;
      if (lpDBevent)
      {
        lpMemRaise = (LPRAISEDDBE)lpDBevent->lpObject;

        while (plist->evt) {
          if (!x_stricmp(plist->evt,lpMemRaise->DBEventName)) {
            char * pc1;
            memset(pRaisedDBE,'\0',sizeof(RAISEDREPLICDBE));
            pRaisedDBE->EventType=plist->itype;
            suppspace(lpMemRaise->DBEventText);
            /*pc1=_tcschr(lpMemRaise->DBEventText,(' '));*/
            pc1=STchr(lpMemRaise->DBEventText,(' '));
            if ( pc1) {
               *pc1='\0';
               pc1++;
               if (plist->itype!=REPLIC_DBE_OUT_TRANSACTION &&
                   plist->itype!=REPLIC_DBE_IN_TRANSACTION)
                 fstrncpy(pRaisedDBE->TableName,pc1,sizeof(pRaisedDBE->TableName));
            }
            fstrncpy(pRaisedDBE->DBName,lpMemRaise->DBEventText,sizeof(pRaisedDBE->DBName));
            pRaisedDBE->ihdl = iNumSession;

            // Delete the current event.
            lpDocTemp->pDBEventUntreated = lpDBevent->lpNext;
            ESL_FreeMem(lpDBevent->lpObject);
            lpDBevent->lpObject = NULL;
            ESL_FreeMem(lpDBevent);

            return TRUE;
          }
          plist++;
        }
      }
    }

    return FALSE;
}

static void StoreEvent4AllDocs(int iSess , LPRAISEDDBE pMemRaise)
{
    BOOL bFound = FALSE;
    LPDOCLIST lpDocTmp;
    lpDocTmp = session[iSess].lpDocLst;

    while( lpDocTmp )
    {
        LPOBJECTLIST lpNew=LIBMON_AddListObjectTail(&lpDocTmp->pDBEventUntreated,sizeof(RAISEDDBE));

        if ( !lpNew)
        {
            myerror(ERR_ALLOCMEM);
            return;
        }
        else
        {
            LPRAISEDDBE lpRaiseDb;
            lpRaiseDb = (LPRAISEDDBE)lpNew->lpObject;
            memcpy(lpRaiseDb,pMemRaise,sizeof(RAISEDDBE));
        }
        lpDocTmp = lpDocTmp->lpNext;
    }
}

#define MAX_DBE_SCAN_TIME_SECOND 8
#define SCANTRIES 2
#ifndef BLD_MULTI
int DBEScanDBEvents(void *pDocInfo,LPRAISEDREPLICDBE praiseddbe)  // function to be called every n seconds (n = 5 ?)
{
  int i,ires;
  RAISEDDBE MemoryRaise;
  int iret = RES_SUCCESS;
  exec sql begin declare section;
    char v_name [MAXOBJECTNAME];
    char v_text [256];
    char v_time[60];
    char v_owner[MAXOBJECTNAME];
  exec sql end declare section;

  for (i=0;i<imaxsessions;i++) {
    if ( !(session[i].bLocked && session[i].bIsDBESession) )
      continue;

    if (!session[i].SessionName[0]) {
      return RES_ERR;
    }

    if ( ExtractDbEventFromMemory(pDocInfo, i, praiseddbe) )
    {
        iret |= MASK_DBEEVENTTHERE;
        return iret;
    }

    ires = ActivateSession(i);
    if (ires!=RES_SUCCESS)
        return ires;

    exec sql commit;
    ManageSqlError(&sqlca);
    if (sqlca.sqlcode != 0 && sqlca.sqlcode != 710) {
        return RES_ERR;
    }

    exec sql get dbevent;
    ManageSqlError(&sqlca);
    if (sqlca.sqlcode != 0 && sqlca.sqlcode != 710) {
       return RES_ERR;
    }
    exec sql commit;
    ManageSqlError(&sqlca);
    if (sqlca.sqlcode != 0 && sqlca.sqlcode != 710) {
      return RES_ERR;
    }
    exec sql inquire_sql(:v_name=dbeventname,:v_owner=dbeventowner,
                         :v_time=dbeventtime,:v_text =dbeventtext);
    ManageSqlError(&sqlca);
    if (sqlca.sqlcode != 0 && sqlca.sqlcode != 710) {
       return RES_ERR;
    }

    exec sql commit;
    ManageSqlError(&sqlca);
    if (sqlca.sqlcode != 0 && sqlca.sqlcode != 710) {
      return RES_ERR;
    }

    if (v_name[0]=='\0') {
       continue;
    }

    // DBEvent found

    suppspace(v_time );
    suppspace(v_name );
    suppspace(v_owner);
    suppspace(v_text );

    if (!session[i].bIsDBE4Replic) {
       return RES_ERR;
    }
    else {
      struct lcllist * plist=mydbelist;
      while (plist->evt) {
        if (!x_stricmp(plist->evt,v_name)) {
            lstrcpy(MemoryRaise.StrTimeStamp,v_time );
            lstrcpy(MemoryRaise.DBEventName ,v_name );
            lstrcpy(MemoryRaise.DBEventOwner,v_owner);
            lstrcpy(MemoryRaise.DBEventText ,v_text );
            // save the current event into all documents different than current pDocInfo
            StoreEvent4AllDocs( i , &MemoryRaise);
            if ( ExtractDbEventFromMemory(pDocInfo, i, praiseddbe) )
            {
                iret |= MASK_DBEEVENTTHERE;
                return iret;
            }
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
                if (UpdateReplStatusinCacheNeedRefresh(NodeName,DBName,svrno,v_text))
                {
                    iret |= MASK_NEED_REFRESH_MONTREE;
                    return iret;
                }
            }
        }
        else
            myerror(ERR_REGEVENTS);
      }
    }
  }
  return iret;
}
#endif

#ifndef BLD_MULTI
int DBEReplMonInit(int hnode,LPUCHAR lpDBName,int *pires, void *pDoc)
{
  LPUCHAR lpVNodeName=GetVirtNodeName(hnode);
  if (!lpVNodeName) {
    myerror(ERR_DOMGETDATA);
    return RES_ERR;
  }
  return DBEReplGetEntry(lpVNodeName,lpDBName,pires, (-1),pDoc);
}
#endif

#ifndef BLD_MULTI
int DBEReplMonTerminate(int hdl,void *pDoc)
{
   return DBEReplReplReleaseEntry(hdl,(void *)pDoc);
}

int DBEReplReplReleaseEntry(int hdl, void *pDoc)
{
    LPDOCLIST lpDocCur,lpDocNext;
    if (!session[hdl].bIsDBESession ||
        !session[hdl].bIsDBE4Replic ||
        session[hdl].nbDBEEntries<1)  {
         return myerror(ERR_SESSION);
    }
    session[hdl].nbDBEEntries--;
    if (!session[hdl].nbDBEEntries) {
      LIBMON_CloseNodeStruct(session[hdl].hnode);
      session[hdl].bIsDBESession = FALSE;
      if ( session[hdl].lpDocLst )
      {
        lpDocCur = session[hdl].lpDocLst;
        while( lpDocCur )
        {
            lpDocNext = lpDocCur->lpNext;
            if (lpDocCur->pDBEventUntreated)
                lpDocCur->pDBEventUntreated = LIBMON_FreeAllInObjectList(lpDocCur->pDBEventUntreated);
            ESL_FreeMem(lpDocCur);
            lpDocCur = lpDocNext;
        }
        session[hdl].lpDocLst = NULL;
      }
      return ReleaseSession(hdl, RELEASE_COMMIT);
    }
    else
    {
        lpDocCur = SearchDocument( pDoc , hdl );
        if( lpDocCur )
            session[hdl].lpDocLst = LIBMON_FreeDocInDocList(session[hdl].lpDocLst, lpDocCur);
    }

    return RES_SUCCESS;
}
#endif

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

#ifndef BLD_MULTI
int DBEReplGetEntry(LPUCHAR lpNodeName,LPUCHAR lpDBName,int *pires, int serverno,void *pDoc)
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
    
    int thdl=0; /*Default thdl for non thread functions */

    // get DBOwner
    hnode=LIBMON_OpenNodeStruct(lpNodeName);
    if (hnode<0)
       return RES_ERR;
    ires= LIBMON_DOMGetFirstObject  ( hnode, OT_DATABASE, 0, NULL, TRUE,NULL,buf,bufowner,extradata, thdl);
    if (ires!=RES_SUCCESS && ires!=RES_ENDOFDATA) {
        LIBMON_CloseNodeStruct(hnode);
        return ires;
    }
    parentstrings[0]=lpDBName;
    parentstrings[1]=NULL;
    ires = DOMGetObjectLimitedInfo( hnode,
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
    LIBMON_CloseNodeStruct(hnode);
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
         if (pDoc)
             if (!SearchDocument( pDoc , isess ))
                 AddDocument( pDoc , isess );
         goto getentryend;
      }
    }
    hnode=LIBMON_OpenNodeStruct(NodeName);
    if (hnode<0)
       return RES_ERR;
    ires= LIBMON_DOMGetFirstObject  ( hnode, OT_DATABASE, 0, NULL, TRUE,NULL,buf,bufowner,extradata, thdl);
    if (ires!=RES_SUCCESS && ires!=RES_ENDOFDATA) {
        LIBMON_CloseNodeStruct(hnode);
        return ires;
    }

    ires = Getsession(SessionName, SESSION_TYPE_INDEPENDENT,&isess);
    if (ires!=RES_SUCCESS) {
       LIBMON_CloseNodeStruct(hnode);
        return ires;
    }

    ActivateSession(isess);
    *pires=isess;
    session[isess].bIsDBESession = TRUE;
    session[isess].bIsDBE4Replic = TRUE;
    session[isess].bMainDBERegistered = FALSE;
    session[isess].nbDBEEntries=0;
    session[isess].nbDBERegSvr=0;
    session[isess].hnode=hnode;
    if (pDoc && serverno == -1)
    {
        if (!SearchDocument( pDoc , isess ))
            AddDocument( pDoc , isess );
    }
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
             DBEReplReplReleaseEntry(isess,(void *)pDoc);
             return ires;
          } 
          if (session[isess].nbDBERegSvr>=MAXREGSVR) {
            //"Too many active Replication Servers on the same node"
            DBEReplReplReleaseEntry(isess,(void *)pDoc);
            return RES_TOOMANY_REPL_SERV;
          }
          session[isess].DBERegSvr[session[isess].nbDBERegSvr]=serverno;
          session[isess].nbDBERegSvr++;
       }
       ires=ExecSQLImm("raise dbevent dd_ping",TRUE, NULL, NULL, NULL);
       if (ires!=RES_SUCCESS && ires!=RES_MOREDBEVENT) {
           //"Cannot ping server %d on %s::%s"
            DBEReplReplReleaseEntry(isess,(void *)pDoc);
            return RES_CANNOT_PING_SVR;
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
                DBEReplReplReleaseEntry(isess,(void *) pDoc);
                return RES_CANNOT_REGISTER_SVR;
             }
             plist++;
           }
           session[isess].bMainDBERegistered=TRUE;
       }
    }
    return RES_SUCCESS;
}
#endif

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

static int ExistImaProc(int hnode,char *procname)
{
    exec sql begin declare section;
    long cnt1;
	char bufproc[100];
    exec sql end declare section;
	int iret,ilocsession;
	int iResult = RES_SUCCESS;
	x_strcpy(bufproc, procname);
  
	iret = GetImaDBsession((LPUCHAR)GetVirtNodeName (hnode), SESSION_TYPE_CACHENOREADLOCK, &ilocsession);
	if (iret !=RES_SUCCESS)
		return RES_ERR;

	exec sql repeated select count(*) into :cnt1
            from $ingres.iiprocedure where dbp_name=:bufproc;

	if (sqlca.sqlcode < 0) {
      iResult = RES_ERR;
	}
	else {
		if (cnt1==0)
			iResult = RES_ENDOFDATA;
	}

	ReleaseSession(ilocsession, RELEASE_COMMIT);

	return iResult;
}

int LIBMON_ExistRemoveSessionProc(int hnode)
{
	return ExistImaProc(hnode,"ima_remove_session");
}

int LIBMON_ExistShutDownSvr( int hnode)
{
	return ExistImaProc(hnode,"ima_shut_server");
}

int LIBMON_ExistProcedure4OpenCloseSvr( int hnode)
{
	int ires = ExistImaProc(hnode,"ima_open_server");
	if (ires!=RES_SUCCESS)
		return ires;
	return ExistImaProc(hnode,"ima_close_server");
}
