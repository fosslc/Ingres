/*
** Copyright (c) 2004 Ingres Corporation
*/

/*
** Name: blobstor.c
**
** Description:
**      Loads a blob from file into a database
**
** History:
**      02-Apr-97 (fanra01)
**          CL'ized on behalf of wilan06.
**      18-apr-96 (harpa06)
**          Moved some strings into message file
**
**          Bumped IO_BUF_SIZE to 32K to speed up
**          storing of blob.
**
**          Print hash marks instead of
**          "put xxxx bytes of param data".
**
**          Included icemain.h to use already defined 
**          constants.
**
**          Store the filename (icedataname) withtout the path.
**      29-Apr-97 (fanra01)
**          Bug 81819   Program exceptions when performing an update with a
**          where clause.  Caused by uninitialised call back fields in API
**          parameter structure.  Add initialisation call prior to use during
**          update.
**      01-may-97 (harpa06)
**          Remove include file icemain.h and replace occurences of STR_NEWLINE
**          with the newline character ('\n') so blobstor can build on UNIX.
**	24-sep-97 (mcgem01)
**	    Clean up compiler warnings.
**	04-10-98 (toumi01)
**	    During Linux port add MEadvise( ME_INGRES_ALLOC ).
**      15-Jun-00 (hweho01)
**          Changed the data type of cRead from size_t to i4. The length
**          of size_t type is 8-byte in AIX 64-bit port (ris_u64), cRead is
**          passed to SIread() as a counter which is expected to be a int
**          type ( 4-byte ); the mismatch cause error "ILLEGAL_XACT_STMT".
**	08-may-2003 (horda03) Bug 110208
**          On VMS size_t isn't defined. Added the declaration if the macro
**          _SIZE_T is not defined.
**	29-Sep-2004 (drivi01)
**	    Updated NEEDLIBS to link dynamic library SHQLIB and SHFRAMELIB
**	    to replace its static libraries. 
**	13-Apr-2004 (lakvi01)
**	    Added _SIZE_T_DEFINED to _SIZE_T as a check to see if size_t is 
**	    defined. For WIN64 platforms size_t was defined when the flag 
**	    _SIZE_T_DEFINED was set.(see stddef.h in PSDK)
**	15-May-2005 (hanje04)
**	    We don't appear to use size_t in this function so remove problematic
**	    typedef.
**	10-Sep-2009 (frima01) 122490
**	    Add include of fe.h and iicommon.h.
*/

# include <compat.h>
# include <iiapi.h>

# include <er.h>
# include <lo.h>
# include <me.h>
# include <pc.h>
# include <si.h>
# include <st.h>

# include <gl.h>
# include <pm.h>
# include <erwb.h>
# include <iicommon.h>
# include <fe.h>

typedef void (FN)(void *);
typedef FN *PFN;

# ifndef DESKTOP
typedef void * PVOID;
typedef bool   BOOL;
# endif

/* predeclarations */
void usage (void);
BOOL StoreImage (II_PTR hconn, char* pszTable, char* pszNamecol,
                 char* pszBlobcol, char* pszFile, II_BOOL fUpdate,
                 char* pszWhere);
BOOL DoInsertImage (II_PTR hconn, char* pszQuery, PFN parmCallback, PVOID user);
void PrintDescriptor (IIAPI_DESCRIPTOR* pdesc);
char* ColTypeName (II_LONG coltype);
char* TypeName (IIAPI_DT_ID type);
BOOL GetStmtDesc (II_PTR hstmt, int* pcParms, IIAPI_DESCRIPTOR** pdesc);
BOOL DBConnect (char* pszNode, II_PTR* phconn);
BOOL DBDisconnect (II_PTR hconn);
BOOL GetApiResult (IIAPI_GENPARM* genParm);
void InitGenParm (void* genParm);
BOOL CommitTran (II_PTR htran);
void PrintErrorInfo (IIAPI_GENPARM* genParm);
void PrintStatusCode (char* message, int status);
void GetQueryInfo (II_PTR stmtHandle);

char szProgName[] = { "BLOBSTORE" };
char szProg[] = { "blobstor" };

/*
** Name: main
**
** Description:
**      Parses arguments and connects to database and stores blob.
**
** Inputs:
**      argc        number of args
**      argv        argument list
**
** Outputs:
**      none
**
** Returns:
**      0           successful
**      1           otherwise
**
** History:
**      02-Apr-97 (fanra01)
**          Modified.
**      18-apr-97 (harpa06)
**          Moved strings to message file.
**
PROGRAM =       blobstor
**
NEEDLIBS =      APILIB SHFRAMELIB SHQLIB SHCOMPATLIB \
		SHEMBEDLIB 
*/

int
main (int argc, char* argv[])
{
int nRet = 0;
IIAPI_INITPARM      initParm;
II_PTR              hconn;
char*               pszTable = NULL;
char*               pszBlobcol = NULL;
char*               pszNamecol = NULL;
char*               pszNode = NULL;
char*               pszFile = NULL;
char*               pszWhere = NULL;
II_BOOL             fUpdate = FALSE;

    MEadvise( ME_INGRES_ALLOC );
    FEcopyright (szProgName, ERx ("1997"));
    SIprintf (ERx ("\n"));

    if (argc < 3 )
    {
        usage ();
        PCexit (1);
    }
    else
    {
        ++argv;
        while (argc>0)
        {
            if (argv && *argv && *argv[0] == '-')
            {
                switch ((*argv)[1])
                {
                    case 't':
                        pszTable = *++argv;
                        argc--;
                        break;

                    case 'b':
                        pszBlobcol = *++argv;
                        argc--;
                        break;

                    case 'n':
                        pszNamecol = *++argv;
                        argc--;
                        break;

                    case 'u':
                        fUpdate = TRUE;
                        break;

                    case 'w':
                        pszWhere = *++argv;
                        argc--;
                        break;
                                
                    default:
                        usage();
                        PCexit (1);
                }
                argv++;
                argc--;
            }
            else
            {
                pszNode = argv[0];
                pszFile = argv[1];
                break;
            }
        }
        if ((!pszNode && *pszNode) || (!pszFile && pszFile))
        {
            usage();
            PCexit (1);
        }
        if (!(pszTable && *pszTable))
            pszTable = ERx ("iceimage");
        if (!(pszBlobcol && *pszBlobcol))
            pszBlobcol = ERx ("icedata");
        if (!(pszNamecol && *pszNamecol) && !fUpdate)
            pszNamecol = ERx ("icedataname");

        initParm.in_timeout = -1;
        initParm.in_version = IIAPI_VERSION_1;
        /*
        ** Invoke API function call.
        */
        IIapi_initialize( &initParm );

        if ( initParm.in_status != IIAPI_ST_SUCCESS )
        {
            SIprintf ("API initialization failed.\n");
        }
        else
        {
            if (DBConnect (pszNode, &hconn))
            {
                StoreImage (hconn, pszTable, pszNamecol, pszBlobcol, pszFile,
                           fUpdate, pszWhere);
                DBDisconnect (hconn);
            }
        }
    }
    return (nRet);
}

#define INSERT_PARM_COUNT       2
#define UPDATE_PARM_COUNT       1
#define IO_BUF_SIZE             32768

/*
** Name: usage
**
** Description:
**      Display usage message.
**
** Inputs:
**      none.
**
** Outputs:
**      none.
**
** Returns:
**      none.
**
** History:
**      02-Apr-97 (fanra01)
**          Modified.
**      17-apr-97 (harpa06)
**          Moved string to message file.
*/
void
usage (void)
{
    SIprintf (ERget (E_WB0141_BLBST_USAGE), szProg);
}
    
/*
** Name: StoreImage
**
** Description:
**      Reads blob from file and inserts into database.
**
** Inputs:
**      hconn               Session handle
**      pszTable            name of table
**      pszNamecol          name column
**      pszBlobcol          blob column
**      pszFile             blob file
**      fUpdat              insert/update flag
**      pszWhere            where clause
**
** Outputs:
**      none.
**
** Returns:
**      TRUE
**      FALSE
**
** History:
**      02-Apr-97 (fanra01)
**          Modified.
**      17-apr-97 (harpa06)
**          Moved string to message file.
**          Print hash mark for every IO_BUF_SIZE bytes stored.
**          Store filename (icedataname) withtout the path.
**      29-Apr-97 (fanra01)
**          Bug 81819   Moved call to InitGenParm for both insert and update.
*/
BOOL
StoreImage (II_PTR hconn, char* pszTable, char* pszNamecol,
            char* pszBlobcol, char* pszFile, II_BOOL fUpdate,
            char* pszWhere)
{
char*               pszIns = ERx ("insert into %s (%s, %s) values( ~V , ~V )");
char*               pszUpd = ERx ("update %s set %s = ~V where %s");
char*               szQuery = NULL;
II_PTR              hstmt = NULL;
IIAPI_QUERYPARM     queryParm;
IIAPI_CLOSEPARM     cp;
int                 cParms = 0;
IIAPI_DESCRIPTOR*   desc = NULL;
BOOL                ret = FALSE;
int                 i=0;
char*               pszQuery;
II_BOOL             fOK = FALSE;
int                 nSize;
STATUS              status = FAIL;

    if (fUpdate)
    {
        nSize = STlength (pszUpd) + STlength (pszTable) +
                STlength (pszWhere) + STlength (pszBlobcol) + 1;
    }
    else
    {
        nSize = STlength (pszIns) + STlength (pszTable) +
                STlength (pszNamecol) + STlength (pszBlobcol) + 1;
    }
    if ((pszQuery = (char *) MEreqmem (0, nSize, TRUE, &status)) != NULL)
    {
        if (fUpdate)
        {
            STprintf (pszQuery, pszUpd, pszTable, pszBlobcol, pszWhere);
        }
        else
        {
            STprintf (pszQuery, pszIns, pszTable, pszNamecol, pszBlobcol);
        }
        SIprintf ("StoreImage: %s\n", pszFile);
        queryParm.qy_genParm.gp_callback = NULL;
        queryParm.qy_genParm.gp_closure = NULL;
        queryParm.qy_connHandle = hconn;
        queryParm.qy_queryType = IIAPI_QT_QUERY;
        queryParm.qy_queryText = pszQuery;
        queryParm.qy_parameters = TRUE;
        queryParm.qy_tranHandle = NULL;
        queryParm.qy_stmtHandle = ( II_PTR )NULL;

        IIapi_query (&queryParm);

        if (ret = GetApiResult (&queryParm.qy_genParm))
        {
        int i = 0;
        IIAPI_SETDESCRPARM sdp;
        int parmCount;

            parmCount = (fUpdate) ? UPDATE_PARM_COUNT : INSERT_PARM_COUNT;

            hstmt = queryParm.qy_stmtHandle;

            desc = (IIAPI_DESCRIPTOR *) MEreqmem (0, parmCount *
                                    sizeof (IIAPI_DESCRIPTOR), TRUE, &status);

            if (desc != NULL)
            {
                sdp.sd_genParm.gp_callback = NULL;
                sdp.sd_genParm.gp_closure = NULL;

                sdp.sd_stmtHandle = hstmt;
                sdp.sd_descriptorCount = parmCount;
                sdp.sd_descriptor = desc;

                if (!fUpdate)
                {
                    desc[i].ds_dataType = IIAPI_VCH_TYPE;
                    desc[i].ds_nullable = TRUE;
                    desc[i].ds_length = 256;
                    desc[i].ds_precision = 0;
                    desc[i].ds_columnType = IIAPI_COL_QPARM;
                    desc[i].ds_columnName = NULL;
                    i++;
                }
                desc[i].ds_dataType = IIAPI_LBYTE_TYPE;
                desc[i].ds_nullable = TRUE;
                desc[i].ds_length = 0x7FFF;
                desc[i].ds_precision = 0;
                desc[i].ds_columnType = IIAPI_COL_QPARM;
                desc[i].ds_columnName = NULL;
                i++;

                IIapi_setDescriptor (&sdp);
                if (GetApiResult (&sdp.sd_genParm))
                {
                    char                buffer [IO_BUF_SIZE+sizeof (II_UINT2)];
                    char                szFdev [MAX_LOC + 1];
                    char                szFpath [MAX_LOC +1];
                    char                szFprefix [MAX_LOC + 1];
                    char                szFsuffix [MAX_LOC + 1];
                    char                szFversion [MAX_LOC + 1];
                    char                szLocString[MAX_LOC + 1];
                    char*               pszTemp;
                    LOCATION            sLoc;
                    IIAPI_PUTPARMPARM   ppp;
                    IIAPI_DATAVALUE*    dv = NULL;
                    FILE*               fp = NULL;
                    i4                  cRead = 0;

                    /* pass the non-long varchar parms */
                    nSize = (INSERT_PARM_COUNT-1) * sizeof (IIAPI_DATAVALUE);
                    if ((dv = (IIAPI_DATAVALUE *)MEreqmem (0, nSize, TRUE, 
                                                           &status)) != NULL)
                    {
                        i = 0;
                        InitGenParm (&ppp);
                        if (!fUpdate)
                        {
                            /* ICEDataName */

                            /* Extract the filename */
                            STcopy (pszFile, szLocString);
                            LOfroms (PATH & FILENAME, szLocString, &sLoc);
                            LOdetail (&sLoc, szFdev, szFpath, szFprefix,
                                      szFsuffix, szFversion);
                            LOcompose (NULL, NULL, szFprefix, szFsuffix, 
                                       szFversion, &sLoc);
                            LOtos (&sLoc, &pszTemp);

                            dv[i].dv_null = FALSE;
                            dv[i].dv_length = STlength (pszTemp) + 2;
                            *((II_UINT2*)buffer) = dv[i].dv_length - 2;
                            STcopy (pszTemp, &(buffer[sizeof(II_UINT2)]));
                            dv[i].dv_value = buffer;
                            i++;

                            ppp.pp_stmtHandle = hstmt;
                            ppp.pp_parmCount = INSERT_PARM_COUNT-1;
                            ppp.pp_parmData = dv;
                            ppp.pp_moreSegments = FALSE;

                            IIapi_putParms (&ppp);
                            fOK = GetApiResult (&ppp.pp_genParm);
                        }
                        else
                        {
                            fOK = TRUE;
                        }
                        if (fOK)
                        {
                            STcopy (pszFile, szLocString);
                            LOfroms (PATH & FILENAME, szLocString, &sLoc);
                            dv->dv_value = buffer;
                            dv->dv_null = FALSE;

                            /* Now pass the long varchar */
                            SIfopen (&sLoc, "r", SI_BIN, 0, &fp);
                            if (fp)
                            {
                                ppp.pp_stmtHandle = hstmt;
                                ppp.pp_parmCount = 1;
                                ppp.pp_moreSegments = TRUE;
                                ppp.pp_parmData = dv;
                                SIprintf (ERget (E_WB0140_BLBST_HASH_MARK), 
                                          IO_BUF_SIZE);
                                do
                                {
                                    status = SIread (fp, IO_BUF_SIZE, &cRead,
                                                 &(buffer[sizeof(II_UINT2)]));
                                    if (cRead > 0)
                                    {
                                        dv->dv_length = cRead + 2;
                                        *((II_UINT2*)buffer) = (II_UINT2)cRead;
                                        if (cRead < IO_BUF_SIZE)
                                            ppp.pp_moreSegments = FALSE;
                                        IIapi_putParms (&ppp);
                                        SIprintf (ERx ("#"));
                                        if (!GetApiResult (&ppp.pp_genParm))
                                        {
                                        SIprintf ("\n\nGetApiResult failed\n");
                                        break;
                                        }
                                    }
                                }
                                while (status == OK);
                                SIclose (fp);
                                SIprintf (ERx ("\n"));
                                SIprintf (ERx ("\n"));
                            }
                            else
                            {
                                IIAPI_CANCELPARM cnp;
                                InitGenParm (&cnp.cn_genParm);
                                cnp.cn_stmtHandle = hstmt;
                                IIapi_cancel (&cnp);
                                GetApiResult (&cnp.cn_genParm);
                                ret = FALSE;
                            }
                        }
                        MEfree (dv);
                    }
                }
                else
                {
                    IIAPI_CANCELPARM cnp;
                    InitGenParm (&cnp.cn_genParm);
                    cnp.cn_stmtHandle = hstmt;
                    IIapi_cancel (&cnp);
                    GetApiResult (&cnp.cn_genParm);
                }
                MEfree (desc);
            }
            GetQueryInfo (hstmt);
            InitGenParm (&cp.cl_genParm);
            cp.cl_stmtHandle = queryParm.qy_stmtHandle;
            IIapi_close (&cp);
            GetApiResult (&cp.cl_genParm);
            CommitTran (queryParm.qy_tranHandle);
        }
        MEfree (pszQuery);
    }

    return ret;
}

/*
** Name: CommitTran
**
** Description:
**      Commit the specified transaction
**
** Inputs:
**      htran       handle to a transaction
**
** Outputs:
**      none.
**
** Returns:
**      TRUE        success
**      FALSE       failed
**
** History:
**      02-Apr-97 (fanra01)
**          Modified.
*/
BOOL
CommitTran (II_PTR htran)
{
    IIAPI_COMMITPARM    cp;

    InitGenParm (&cp);
    cp.cm_tranHandle = htran;

    IIapi_commit (&cp);

    return GetApiResult (&cp.cm_genParm);
}

/*
** Name: PrintDescriptor
**
** Description:
**      Prints the contents of an API descriptor
**
** Inputs:
**      pdesc               API descriptor
**
** Outputs:
**      none.
**
** Returns:
**      none.
**
** History:
**      02-Apr-97 (fanra01)
**          Updated.
*/
void
PrintDescriptor (IIAPI_DESCRIPTOR* pdesc)
{
    SIprintf ("Descriptor:\n");
    SIprintf ("\tDatatype:\t%s\n", TypeName (pdesc->ds_dataType));
    SIprintf ("\tLength:\t%d\n", pdesc->ds_length);
    SIprintf ("\tScale:\t%d\n", pdesc->ds_scale);
    SIprintf ("\tColType:\t%s\n", ColTypeName(pdesc->ds_columnType));
    SIprintf ("\tColName:\t%s\n", pdesc->ds_columnName);
}

/*
** Name: ColTypeName
**
** Description:
**      Prints the name of a column type
**
** Inputs:
**      coltype
**
** Outputs:
**      none.
**
** Returns:
**      ret         pointer to const string
**      NULL        no match
**
** History:
**      02-Apr-97 (fanra01)
**          Modified.
*/
char*
ColTypeName (II_LONG coltype)
{
    char* ret = NULL;

    switch (coltype)
    {
        case IIAPI_COL_TUPLE:
            ret = "IIAPI_COL_TUPLE";
            break;
        case IIAPI_COL_PROCBYREFPARM:
            ret = "IIAPI_COL_PROCBYREFPARM";
            break;
        case IIAPI_COL_PROCPARM:
            ret = "IIAPI_COL_PROCPARM";
            break;
        case IIAPI_COL_SVCPARM:
            ret = "IIAPI_COL_SVCPARM";
            break;
        case IIAPI_COL_QPARM:
            ret = "IIAPI_COL_QPARM";
            break;
    }
    return (ret);
}

/*
** Name: TypeName
**
** Description:
**      Prints the name of the specified API datatype
**
** Inputs:
**      type        datatype
**
** Outputs:
**      none.
**
** Returns:
**      ret         pointer to const string
**      NULL        no match
**
** History:
**      02-Apr-97 (fanra01)
**          Modified.
*/
char*
TypeName (IIAPI_DT_ID type)
{
    char* ret = NULL;
    
    switch (type)
    {
        case IIAPI_BYTE_TYPE:
            ret = "IIAPI_BYTE_TYPE";
            break;
        case IIAPI_CHA_TYPE:
            ret = "IIAPI_CHA_TYPE";
            break;
        case IIAPI_CHR_TYPE:
            ret = "IIAPI_CHR_TYPE";
            break;
        case IIAPI_DEC_TYPE:
            ret = "IIAPI_DEC_TYPE";
            break;
        case IIAPI_DTE_TYPE:
            ret = "IIAPI_DTE_TYPE";
            break;
        case IIAPI_FLT_TYPE:
            ret = "IIAPI_FLT_TYPE";
            break;
        case IIAPI_HNDL_TYPE:
            ret = "IIAPI_HNDL_TYPE";
            break;
        case IIAPI_INT_TYPE:
            ret = "IIAPI_INT_TYPE";
            break;
        case IIAPI_LOGKEY_TYPE:
            ret = "IIAPI_LOGKEY_TY";
            break;
        case IIAPI_LBYTE_TYPE:
            ret = "IIAPI_LBYTE_TYP";
            break;
        case IIAPI_LTXT_TYPE:
            ret = "IIAPI_LTXT_TYPE";
            break;
        case IIAPI_LVCH_TYPE:
            ret = "IIAPI_LVCH_TYPE";
            break;
        case IIAPI_MNY_TYPE:
            ret = "IIAPI_MNY_TYPE";
            break;
        case IIAPI_TABKEY_TYPE:
            ret = "IIAPI_TABKEY_TY";
            break;
        case IIAPI_TXT_TYPE:
            ret = "IIAPI_TXT_TYPE";
            break;
        case IIAPI_VBYTE_TYPE:
            ret = "IIAPI_VBYTE_TYP";
            break;
        case IIAPI_VCH_TYPE:
            ret = "IIAPI_VCH_TYPE";
            break;
        default:
            ret = "Unknown";
            break;
    }
    return ret;
}
        
/*
** Name: GetStmtDesc
**
** Description:
**      Return the number of parameters in a statement
**
** Inputs:
**      hstmt
**      pcParms
**      ppdesc
**
** Outputs:
**      none.
**
** Returns:
**      TRUE            sucess
**      FALSE           failed
**
** History:
**      02-Apr-97 (fanra01)
**          Modified.
*/
BOOL
GetStmtDesc (II_PTR hstmt, int* pcParms, IIAPI_DESCRIPTOR** ppdesc)
{
    IIAPI_GETDESCRPARM   getDescrParm;
    BOOL                ret = FALSE;
    
    getDescrParm.gd_genParm.gp_callback = NULL;
    getDescrParm.gd_genParm.gp_closure = NULL;
    getDescrParm.gd_stmtHandle = hstmt;
    getDescrParm.gd_descriptorCount = 0;
    getDescrParm.gd_descriptor = NULL;

    IIapi_getDescriptor (&getDescrParm);
    
    if (ret = GetApiResult (&getDescrParm.gd_genParm))
    {
        *pcParms = getDescrParm.gd_descriptorCount;
        *ppdesc = getDescrParm.gd_descriptor;
    }
    else
    {
        *pcParms = 0;
        *ppdesc = NULL;
    }
    return ret;
}

/*
** Name: DBConnect
**
** Description:
**      Connect to the specified vnode::database. Return the connection handle
**      in *phconn
**
** Inputs:
**      pszNode         Node name to connect to
**
** Outputs:
**      phconn          Connection handle
**
** Returns:
**      TRUE            Success
**      FALSE           Failed
**
** History:
**      02-Apr-97 (fanra01)
**          Modified.
*/
BOOL
DBConnect (char* pszNode, II_PTR* phconn)
{
    IIAPI_CONNPARM      connParm;
    BOOL                ret = FALSE;

    SIprintf ("DBConnect: %s\n", pszNode);
    connParm.co_genParm.gp_callback = NULL;
    connParm.co_genParm.gp_closure = NULL;
    connParm.co_target = pszNode;
    connParm.co_connHandle = NULL;
    connParm.co_tranHandle = NULL;
    connParm.co_username = NULL;
    connParm.co_password = NULL;
    connParm.co_timeout = 60 * 1000; /*connection timeout of 1 min */

    IIapi_connect (&connParm);

    if (ret = GetApiResult (&connParm.co_genParm))
    {
        *phconn = connParm.co_connHandle;
    }
    else
    {
        *phconn = NULL;
    }
    return ret;
}

/*
** Name: DBDisconnect
**
** Description:
**      Disconnect the session associated with hconn
**
** Inputs:
**      hconn               Session handle
**
** Outputs:
**      none.
**
** Returns:
**      TRUE                Success
**      FALSE               Fail
**
** History:
**      02-Apr-97 (fanra01)
**          Modified.
*/
BOOL
DBDisconnect (II_PTR hconn)
{
    IIAPI_DISCONNPARM dc;

    dc.dc_genParm.gp_callback = NULL;
    dc.dc_genParm.gp_closure = NULL;

    dc.dc_connHandle = hconn;

    IIapi_disconnect (&dc);
    return GetApiResult (&dc.dc_genParm);
}
    
/*
** Name: GetApiResult
**
** Description:
**
** Inputs:
**      genParm
**
** Outputs:
**      none.
**
** Returns:
**      TRUE            Sucess
**      FALSE           Fail
**
** History:
**      02-Apr-97 (fanra01)
**          Modified.
*/
BOOL
GetApiResult (IIAPI_GENPARM* genParm)
{
    IIAPI_WAITPARM waitParm = {-1, IIAPI_ST_SUCCESS};
    BOOL ret = FALSE;

    while( (genParm->gp_completed == FALSE ) &&
           (waitParm.wt_status == IIAPI_ST_SUCCESS))
    {
        IIapi_wait( &waitParm );
    }
    if (waitParm.wt_status == IIAPI_ST_SUCCESS)
    {
        switch (genParm->gp_status)
        {
            case IIAPI_ST_SUCCESS:
            case IIAPI_ST_NO_DATA:
                ret = TRUE;
                break;
            case IIAPI_ST_MESSAGE:
            case IIAPI_ST_WARNING:
                PrintErrorInfo (genParm);
                break;
            default:
                PrintErrorInfo (genParm);
                ret = FALSE;
                break;
        }
    }
    else
    {
        PrintStatusCode ("IIapi_wait", waitParm.wt_status);
    }
    return ret;
}


/*
** Name: PrintErrorInfo
**
** Description:
**      Print error info
**
** Inputs:
**      genParm
**
** Outputs:
**      none.
**
** Returns:
**      TRUE
**      FALSE
**
** History:
**      02-Apr-97 (fanra01)
**          Modified.
*/
void PrintErrorInfo (IIAPI_GENPARM* genParm)
{
    IIAPI_GETEINFOPARM eip;

    MEfill (sizeof (eip), 0, &eip );
    eip.ge_errorHandle = genParm->gp_errorHandle;
    do
    {
        IIapi_getErrorInfo (&eip);

        if (eip.ge_status == IIAPI_ST_SUCCESS)
        {
            switch (eip.ge_type)
            {
                case IIAPI_GE_ERROR:
                    SIprintf ("Error: ");
                    break;
                case IIAPI_GE_WARNING:
                    SIprintf ("Warning: ");
                    break;
                case IIAPI_GE_MESSAGE:
                    SIprintf ("Message: ");
                    break;
                default:
                    SIprintf ("Unknown Error Type: %d ", eip.ge_type);
            }
            SIprintf ("SQLState: %s Code: %d\n",eip.ge_SQLSTATE,
                      eip.ge_errorCode);
            if (eip.ge_message)
                SIprintf ("Message: %s\n", eip.ge_message);
        }
    }
    while (eip.ge_status == IIAPI_ST_SUCCESS);
}

/*
** Name: PrintStatusCode
**
** Description:
**
** Inputs:
**      message
**      status
**
** Outputs:
**      none.
**
** Returns:
**      none
**
** History:
**      02-Apr-97 (fanra01)
**          Modified.
*/
void
PrintStatusCode (char* message, int status)
{
    switch (status)
    {
        case IIAPI_ST_NO_DATA:
            SIprintf ("%s: IIAPI_ST_No_DATA\n", message);
            break;
        case IIAPI_ST_NOT_INITIALIZED:
            SIprintf ("%s: IIAPI_ST_NoT_INITIALIZED\n", message);
            break;
        case IIAPI_ST_INVALID_HANDLE:
            SIprintf ("%s: IIAPI_ST_INVALID_HANDLE\n", message);
            break;
        default:
            SIprintf ("%s: Unknown Status\n", message);
            break;
    }
} 

        
/*
** Name: InitGenParm
**
** Description:
**
** Inputs:
**      genParm
**
** Outputs:
**      none.
**
** Returns:
**      none.
**
** History:
**      02-Apr-97 (fanra01)
**          Modified.
*/
void
InitGenParm (void* genParm)
{
    ((IIAPI_GENPARM*)genParm)->gp_callback = NULL;
    ((IIAPI_GENPARM*)genParm)->gp_closure = NULL;
}


/*
** Name: GetQueryInfo
**
** Description:
**
** Inputs:
**      stmtHandle
**
** Outputs:
**      none
**
** Returns:
**      none
**
** History:
**      02-Apr-97 (fanra01)
**          Modified.
*/
void
GetQueryInfo (II_PTR stmtHandle)
{
    IIAPI_GETQINFOPARM          gqip;
    char tableKey [IIAPI_TBLKEYSZ + 1];
    char objectKey [IIAPI_OBJKEYSZ + 1];

    gqip.gq_genParm.gp_callback = NULL;
    gqip.gq_genParm.gp_closure = NULL;
    gqip.gq_stmtHandle = stmtHandle;

    IIapi_getQueryInfo (&gqip);

    GetApiResult (&gqip.gq_genParm);

    if ( gqip.gq_flags & IIAPI_GQF_FAIL )
        SIprintf( "\tflag = IIAPI_GQF_FAIL\n" );

    if ( gqip.gq_flags & IIAPI_GQF_ALL_UPDATED )
        SIprintf( "\tflag = IIAPI_GQF_ALL_UPDATED\n" );

    if ( gqip.gq_flags & IIAPI_GQF_NULLS_REMOVED )
        SIprintf( "\tflag = IIAPI_GQF_NULLS_REMOVED\n" );

    if ( gqip.gq_flags & IIAPI_GQF_UNKNOWN_REPEAT_QUERY )
        SIprintf( "\tflag = IIAPI_GQF_UNKNOWN_REPEAT_QUERY\n" );

    if ( gqip.gq_flags & IIAPI_GQF_END_OF_DATA )
        SIprintf( "\tflag = IIAPI_GQF_END_OF_DATA\n" );

    if ( gqip.gq_flags & IIAPI_GQF_CONTINUE )
        SIprintf( "\tflag = IIAPI_GQF_CONTINUE\n" );

    if ( gqip.gq_flags & IIAPI_GQF_INVALID_STATEMENT )
        SIprintf( "\tflag = IIAPI_GQF_INVALID_STATEMENT\n" );

    if ( gqip.gq_flags & IIAPI_GQF_TRANSACTION_INACTIVE )
        SIprintf( "\tflag = IIAPI_GQF_TRANSACTION_INACTIVE\n" );

    if ( gqip.gq_flags & IIAPI_GQF_OBJECT_KEY )
        SIprintf( "\tflag = IIAPI_GQF_OBJECT_KEY\n" );

    if ( gqip.gq_flags & IIAPI_GQF_TABLE_KEY )
        SIprintf( "\tflag = IIAPI_GQF_TABLE_KEY\n" );

    if ( gqip.gq_flags & IIAPI_GQF_NEW_EFFECTIVE_USER )
        SIprintf( "\tflag = IIAPI_GQF_NEW_EFFECTIVE_USER\n" );
    if ( gqip.gq_flags & IIAPI_GQF_NEW_EFFECTIVE_USER )
        SIprintf( "\tflag = IIAPI_GQF_NEW_EFFECTIVE_USER\n" );

    if ( gqip.gq_flags & IIAPI_GQF_FLUSH_QUERY_ID )
        SIprintf( "\tflag = IIAPI_GQF_FLUSH_QUERY_ID\n" );

    if ( gqip.gq_flags & IIAPI_GQF_ILLEGAL_XACT_STMT )
        SIprintf( "\tflag = IIAPI_GQF_ILLEGAL_XACT_STMT\n" );


    if ( gqip.gq_mask & IIAPI_GQ_ROW_COUNT )
        SIprintf( "\trow count = %d\n", gqip.gq_rowCount );

    if ( gqip.gq_mask & IIAPI_GQ_CURSOR )
        SIprintf( "\treadonly = TRUE\n" );
    else
        SIprintf( "\treadonly = FALSE\n" );

    if ( gqip.gq_mask & IIAPI_GQ_PROCEDURE_RET )
        SIprintf( "\tprocedure return = %d\n", gqip.gq_procedureReturn );

    if ( gqip.gq_mask & IIAPI_GQ_PROCEDURE_ID )
        SIprintf( "\tprocedure handle = 0x%x\n", gqip.gq_procedureHandle );

    if ( gqip.gq_mask & IIAPI_GQ_REPEAT_QUERY_ID )
        SIprintf( "\trepeat query ID = 0x%x\n", gqip.gq_repeatQueryHandle );

    if ( gqip.gq_mask & IIAPI_GQ_TABLE_KEY )
    {
        STlcopy (gqip.gq_tableKey, tableKey, IIAPI_TBLKEYSZ);
        tableKey [IIAPI_TBLKEYSZ] = '\0';
        SIprintf( "\ttable key        = %s\n",tableKey);
    }

    if ( gqip.gq_mask & IIAPI_GQ_OBJECT_KEY )
    {
        STlcopy (gqip.gq_objectKey, objectKey, IIAPI_OBJKEYSZ);
        objectKey [IIAPI_OBJKEYSZ] = '\0';
        SIprintf( "\tobject key       = %s\n", objectKey);
    }
    return;
}
