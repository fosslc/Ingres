/*
** Copyright (C) 1996, 1997 Ingres Corporation All Rights Reserved.
*/

/*
** Name: gccdrv.c
**
** Description:
**      Initialisation function for DLL protocol drivers.
**
**      GCloadprotocol
**      GCunloadprotocol
**
**      This module is responsible for loading and initialising the structures
**      necessary to include a protocol driver.
**
** Assumptions:
**      1.  protocol DLL can accept parameters defined in gcccl.h.
**      2.  The exported functions from DLL are named
**              module_init
**              module_entry
**              module_exit
**              module_drvinit
**              module_addr
**              module_trace
**          where module is the prefix defined in protocol.net
**          If the driver type is defined as Winsock the entry function is
**          optional.  If the driver entry function is not defined the
**          GCwinsock entry point is assumed.
**      4.  DLL is located in the PATH.
**      5.  The file protocol.net is located in the II_SYSTEM\ingres\files
**          directory.
**      6.  Each protocol driver has an entry similar to
**
**     ID=LANMAN,PORT=II,DPKT=2048,PREFIX=GClanman,TYPE=oem,DRIVER=iilmgnbi.dll
**
** History:
**      19-May-97 (fanra01)
**          Created.
**      01-Jul-97 (fanra01)
**          Modified the madatory list of functions required for either the
**          oem driver or a winsock driver.
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
**	20-Jul-2004 (lakvi01)
**		SIR #112703, cleaned-up warnings.
**      26-Jul-2004 (lakvi01)
**          Backed-out the above change to keep the open-source stable.
**          Will be revisited and submitted at a later date. 
**	13-Jan-2010 (wanfr01) Bug 123139
**	    Include cv.h for function defintions
*/

# include <compat.h>
# include <gcccl.h>
# include <er.h>
# include <gc.h>
# include <lo.h>
# include <me.h>
# include <nm.h>
# include <qu.h>
# include <si.h>
# include <st.h>
# include "gcclocal.h"
# include <cv.h>

# define MAX_NAME               256
# define MAX_PROTOCOL_ENTRY     256
# define PROTOCOL_DIR           ERx("name")
# define PROTOCOL_NET           ERx("protocol.net")

/*
**  GCIMPORT    structure to define function attributes from dll.
**
**  blOptional      FALSE function is mandatory and must be in the DLL.
**                  TRUE  function may not be in the DLL, in which case the
**                        function defined in GCC_PCT will be used as the
**                        entry point.
**  pszFunction    pointer to string which is converted to function name
**                 defined in the DLL.
**  fnDriver       instance of driver function from loaded DLL.
*/
typedef struct tagGCIMPORT
{
    i4      nOptional;
    char*   pszGCFunction;
    STATUS  (*fnDriver)();
}GCIMPORT, *PGCIMPORT;

/*
**  Protocol driver parameter index enumerations
**  N.B.
**      These enumerations correspond to the driver parameter indices. Ordering
**      must be maintained.
*/
enum EntryParam
{
    IDENTIFIER, PORT, PACKET, PREFIX, TYPE, DRIVER, MAX_ENTRY_PARAM
};

/*
**  Protocol driver type enumerations
**  N.B.
**      These enumerations correspond to the driver type indices. Ordering
**      must be maintained.
*/
enum DriverType
{
    OEM, WINSOCK, MAX_DRIVER_TYPES
};

/*
**  PROT_LIST       structure to define a list of protocol drivers.
**
**  pEntryParam     pointer to an array of driver parameters.
*/
typedef struct tagPROT_LIST
{
    struct tagPROT_LIST * qNext;
    char*       pEntryParam[MAX_ENTRY_PARAM];
    i4          nPctIndex;
    HANDLE      hProtDrv;
    PGCIMPORT   pGCimport;
}PROT_LIST, *PPROT_LIST;

FUNC_EXTERN STATUS GCloadprotocol (GCC_PCT* pPct);
FUNC_EXTERN STATUS GCunloadprotocol (GCC_PCT* pPct);

GLOBALREF       STATUS          (*IIGCc_prot_init[])();
GLOBALREF       STATUS          (*IIGCc_prot_exit[])();

static PPROT_LIST pstProtHead = NULL;
static PPROT_LIST pstProtTail = NULL;

/*
**  Ordering of the list must correspond to the enumerations.
*/
static char* pszLabels[] = {"ID", "PORT", "DPKT", "PREFIX",
                            "TYPE", "DRIVER", NULL };

static char* pszTypes[]  = { "OEM", "WINSOCK", NULL };

/*
**  Function index enumnerations.
**
**  N.B.
**      These enumerations correspond to the function indices. Ordering must
**      be maintained.
*/
enum FnIndex
{
    GC_PROT_INIT, GC_PROT_ENTRY, GC_PROT_EXIT, GC_PROT_DRVINIT, GC_PROT_ADDR,
    GC_PROT_TRACE, GC_MAX_FUNC
};

# define OEMOPT     1           /* function not required by oem driver */
# define WINOPT     2           /* function not required by winsock driver */

/*
**  Ordering of the list must correspond to the enumerations.
*/
static GCIMPORT sGCImport[] = {
                                {        WINOPT,  "%s_init"   , NULL },
                                {        WINOPT,  "%s_entry"  , NULL },
                                {        WINOPT,  "%s_exit"   , NULL },
                                { OEMOPT,         "%s_drvinit", NULL },
                                { OEMOPT,         "%s_addr"   , NULL },
                                { OEMOPT|WINOPT,  "%s_trace"  , NULL },
                                { 0    ,  NULL       , NULL }
                              };

static i4  EnumStringVal (char* * pszList, char* pszString);

static VOID ParseProtEntry (PPROT_LIST* pProtHead, PPROT_LIST* pProtTail,
                            char* pszProtEntry, i4 * pnProt);

static STATUS LoadProtDriver (PGCIMPORT pImport, PPROT_LIST pEntry,
                              GCC_PCE * pPce, STATUS (* *fnInit)(),
                              STATUS (* *fnExit)());

/*
** Name: EnumStringVal
**
** Description:
**      Function resutns an enumeration of a string within a list of strings.
**
** Input:
**      pszList     pointer to a list of strings
**      pszString   pointer to string to enumerate
**
** Output:
**      none.
**
** Return:
**      !0          success
**      -1          failure
*/
static i4 
EnumStringVal (char* * pszList, char* pszString)
{
i4      i;

    for (i=0; pszList[i] != NULL; i++)
    {
        if (STbcompare (pszList[i], 0, pszString, 0, TRUE) == 0)
        {                               /* compare value with list of values */
            return(i);                  /* return index                      */
        }
    }
    return ((i4 )-1);
}

/*
** Name: ParseProtEntry
**
** Description:
**      Function reads the protocol.net file from %II_SYSTEM\ingres\files.
**      Scans each line in the file for a complete protocol driver descriptor
**      and stores the values into a queued structure.
**
**      Assumes an entry
**      keyword=value, keyword=value
**      eg.
**
**  ID=LANMAN,PORT=II,DPKT=2048,PREFIX=GClanman,TYPE=oem,DRIVER=iilmgnbi.dll
**
** Inputs:
**      pszProtEntry        line containing protocol driver parameters
**
** Outputs:
**      pProtHead           head of queue of protocol driver structures.
**      pProtTail           tail of queue of protocol driver structures.
**
** Returns:
**      none.
**
** History:
**      19-May-97 (fanra01)
**          Created.
*/
static VOID
ParseProtEntry (PPROT_LIST* pProtHead, PPROT_LIST* pProtTail,
                char* pszProtEntry, i4 * pnProt)
{
STATUS status = OK;
i4  i;
i4  j;
char* pPtr;
char* pStart;
char* pEnd;
char szBuf[MAX_PROTOCOL_ENTRY];
PPROT_LIST pProt;
bool blCopy;

    /*
    **  Line feed terminates line
    */
    if ((pPtr = STindex (pszProtEntry, "\n", 0)) != NULL)
    {
        *pPtr = '\0';
    }
    /*
    **  Hash character '#' terminates line
    */
    if ((pPtr = STindex (pszProtEntry, "#", 0)) != NULL)
    {
        *pPtr = '\0';
    }

    if (STlength(pszProtEntry) > 0)
    {
        if ((pProt = (PPROT_LIST) MEreqmem (0, sizeof(PROT_LIST),
                                            TRUE, &status)) != NULL)
        {
            for (i=0; pszLabels[i] != NULL; i++)
            {
                if ((pPtr=STstrindex (pszProtEntry, pszLabels[i],
                                      0, TRUE))!= NULL)
                {
                    if ((pStart = STindex (pPtr, "=", 0)) != NULL)
                    {
                        pStart+=1;
                        pEnd = STindex (pPtr, ",", 0);
                        for (j=0, blCopy = TRUE; blCopy == TRUE; j++)
                        {
                            szBuf[j] = *pStart++;
                            blCopy = (pEnd == NULL) ? (*pStart != '\0') :
                                                      (pStart < pEnd);
                        }
                        szBuf[j] = '\0';
                        pPtr = STskipblank (szBuf, STlength(szBuf));
                        STtrmwhite (pPtr);
                        pProt->pEntryParam[i] = STalloc (pPtr);
                    }
                    else
                    {
                        status = FAIL;
                        /* syntax error */
                    }
                }
                else
                {
                    status = FAIL;
                    /* entry is incomplete */
                }
            }
            if (status == OK)
            {
                *pnProt +=1;
                pProt->qNext =  NULL;
                if (*pProtHead == NULL)
                {
                    *pProtHead = pProt;
                    *pProtTail = pProt;
                }
                else
                {
                    ((PPROT_LIST) *pProtTail)->qNext = pProt;
                    ((PPROT_LIST) *pProtTail) = pProt;
                }
            }
            else
            {
                MEfree (pProt);
            }
        }
    }
    return;
}

/*
** Name: LoadProtDriver
**
** Description:
**      Function takes a protocol driver parameter structure and attempts to
**      load the driver DLL.
**
**      Allocate a function instance structure to hold a list of function
**      instances.
**
** Inputs:
**      pImports        pointer to a static imports template structure.
**      pEntry          pointer to a protocol driver description structure.
**      pPce            pointer to protocol control entry structure.
** Outputs:
**      fnInit          address of function pointer for driver initialisation.
**      fnExit          address of function pointer for driver termination.
**
** Returns:
**      status      OK      sucess
**                  !0      failure
** History:
**      01-Jul-97 (fanra01)
**          Moved the enumeration of type and use this to set the mask which
**          determines whether a function is required.
*/
static STATUS
LoadProtDriver (PGCIMPORT pImport, PPROT_LIST pEntry, GCC_PCE * pPce,
                STATUS (* *fnInit)(), STATUS (* *fnExit)())
{
STATUS status = OK;
HANDLE hProtDrv = NULL;
char szItemName[MAX_NAME];
i4  i;
PGCIMPORT pDrvDesc = NULL;
i4  nDrvType;
i4  nMask;

    nDrvType = EnumStringVal(pszTypes,pEntry->pEntryParam[TYPE]);

    if ((hProtDrv = LoadLibrary (pEntry->pEntryParam[DRIVER])) != NULL)
    {
        pDrvDesc = (PGCIMPORT)MEreqmem (0, sizeof(GCIMPORT) * (GC_MAX_FUNC+1),
                                        TRUE, &status);
        if (pDrvDesc != NULL)
        {
            MEcopy (pImport, sizeof(GCIMPORT) * (GC_MAX_FUNC+1), pDrvDesc);
        }
        for (i=0; (status == OK) &&
                  (pDrvDesc[i].pszGCFunction != NULL); i++)
        {
            STprintf (szItemName, pDrvDesc[i].pszGCFunction,
                                  pEntry->pEntryParam[PREFIX]);
            pDrvDesc[i].fnDriver=(STATUS (*)(void))GetProcAddress(hProtDrv,
                                                                  szItemName);
            if (pDrvDesc[i].fnDriver == NULL)
            {
                /*
                **  if this is not an optional function return the error
                */
                switch (nDrvType)
                {
                    default:
                    case OEM:
                        nMask = OEMOPT;
                        break;

                    case WINSOCK:
                        nMask = WINOPT;
                        break;
                }

                if ((pDrvDesc[i].nOptional & nMask) == 0)
                {
                    status = GetLastError();
                    MEfree (pDrvDesc);
                    pDrvDesc = NULL;
                }
            }
        }
        if (status == OK)
        {
            pEntry->hProtDrv  = hProtDrv;
            pEntry->pGCimport = pDrvDesc;
            CVal(pEntry->pEntryParam[PACKET], &pPce->pce_pkt_sz);
            STcopy(pEntry->pEntryParam[IDENTIFIER],pPce->pce_pid);
            STcopy(pEntry->pEntryParam[PORT],pPce->pce_port);
            switch (nDrvType)
            {
                default:
                case OEM:
                    if (pDrvDesc[GC_PROT_ENTRY].fnDriver != NULL)
                    {
                        pPce->pce_protocol_rtne =
                                        pDrvDesc[GC_PROT_ENTRY].fnDriver;
                        *fnInit = pDrvDesc[GC_PROT_INIT].fnDriver;
                        *fnExit = pDrvDesc[GC_PROT_EXIT].fnDriver;
                    }
                    break;

                case WINSOCK:
                    if (pDrvDesc[GC_PROT_ENTRY].fnDriver != NULL)
                    {
                        pPce->pce_protocol_rtne =
                                        pDrvDesc[GC_PROT_ENTRY].fnDriver;
                    }
                    else
                    {
                        pPce->pce_protocol_rtne = GCwinsock;
                    }
                    *fnInit = GCwinsock_init;
                    *fnExit = (STATUS (*)()) GCwinsock_exit;
                    break;

            }
            pPce->pce_driver = (PTR) MEreqmem (0, sizeof (WS_DRIVER), TRUE,
                                               &status);
            if ( pPce->pce_driver != NULL)
            {
            WS_DRIVER * pWsDrv = (WS_DRIVER *) pPce->pce_driver;
                pWsDrv->init_func = pDrvDesc[GC_PROT_DRVINIT].fnDriver;
                pWsDrv->addr_func = pDrvDesc[GC_PROT_ADDR].fnDriver;
            }
        }
        else
        {
            status = GetLastError();
            MEfree (pDrvDesc);
            pDrvDesc = NULL;
        }
    }
    return (status);
}

/*
** Name: GCloadprotocol
**
** Description:
**      Function opens and reads the protocol.net file a line at a time and
**      extracts the parameters. If parse is successfull the protocol driver
**      count is incremented.
**
**      Once all lines have been successfully read and parsed the protocol
**      control table is extended to include the loaded drivers.
**      The load fails if the number of entries in the protocol.net file
**      exceeds GCC_L_PCT currently set to 10.
**
**      The loading of the driver DLL updates the protocol control table
**      structure as well as the respective IIGCc_prot_init IIGCc_prot_exit
**      entries.
**
** Inputs:
**      pPct        pointer to the protocol control table.
**
** Outputs:
**      pce entries in the pPct table will be updated with new protocol
**      entires.
**
** Returns:
**      status      OK      success
**                  !0      failure
** History:
**      19-May-97 (fanra01)
**          Created.
*/
STATUS
GCloadprotocol (GCC_PCT* pPct)
{
STATUS status = OK;
i4  nProt = 0;
i4  i;
i4  nMem;
LOCATION stProtLoc;
char szFilesDir[MAX_LOC + 1];
char szProtPath[MAX_LOC + 1];
FILE* pFile = NULL;
char szProtocolEntry[MAX_PROTOCOL_ENTRY + 1];
PPROT_LIST pEntry;

    if ((status = NMloc(FILES, PATH, (char *)NULL, &stProtLoc)) == OK)
    {
        LOcopy(&stProtLoc, szFilesDir, &stProtLoc);
        if ((status = LOfstfile (PROTOCOL_NET, &stProtLoc)) == OK)
        {
            status = SIfopen (&stProtLoc, ERx("r"), SI_TXT, 0, &pFile);
            if ( status == OK)
            {
                do
                {
                    status = SIgetrec (szProtocolEntry, MAX_PROTOCOL_ENTRY,
                                       pFile);
                    if (status != ENDFILE)
                    {
                        ParseProtEntry (&pstProtHead, &pstProtTail,
                                        szProtocolEntry, &nProt);
                    }
                } while (status == OK);
                if (status == ENDFILE)
                {
                    status = OK;
                }
                SIclose (pFile);
            }
        }
    }

    /*
    **  Count current entries in the protocol control table
    */
    for ( i = 0; i < pPct->pct_n_entries; i++ )
    {
        if ( !pPct->pct_pce[i].pce_pid[0] )
            break;
    }
    /*
    **  if there are new drivers and we have not exceeded GCC_PCT table
    */
    if ((nProt != 0) && ((nProt + i) < GCC_L_PCT))
    {
        nProt+=i;
        for (pEntry = pstProtHead;
             (status == OK) && (pEntry != NULL) && (i  < nProt);
             i++, pEntry=pEntry->qNext)
        {
            status = LoadProtDriver (&sGCImport[0], pEntry, &pPct->pct_pce[i],
                                     &IIGCc_prot_init[i],
                                     &IIGCc_prot_exit[i]);
            if (status == OK)
            {
                pEntry->nPctIndex = i;
            }
        }
        pPct->pct_n_entries = i;
    }
    return (status);
}

/*
** Name: GCunloadprotocol
**
** Description:
**      Frees memory and resorces associated with a protocol driver.
**
** Inputs:
**      pPct        pointer to the protocol control table.
**
** Outputs:
**      none.
**
** Returns:
**      status      OK      success
**                  !0      failure
**
** History:
**      19-May-97 (fanra01)
**          Created.
*/
STATUS
GCunloadprotocol (GCC_PCT* pPct)
{
STATUS status = OK;
PPROT_LIST pEntry;
i4  i;

    for (pEntry = pstProtHead;
         (status == OK) && (pEntry != NULL);
         pEntry=pstProtHead)
    {
        for (i=0; i < MAX_ENTRY_PARAM; i++)
        {
            MEfree (pEntry->pEntryParam[i]);
        }
        MEfree (pPct->pct_pce[pEntry->nPctIndex].pce_driver);
        MEfree (pEntry->pGCimport);
        FreeLibrary (pEntry->hProtDrv);
        pstProtHead = pEntry->qNext;
        MEfree (pEntry);
    }

    return(status);
}
