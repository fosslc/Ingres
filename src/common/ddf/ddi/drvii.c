/******************************************************************************
**
** Copyright (c) 2004 Ingres Corporation
**
******************************************************************************/

#include <dbmethod.h>
#include <drviiapi.h>
#include <drviiutil.h>

#define  IsBlobColumn(X) (X.ds_dataType == IIAPI_LBYTE_TYPE || X.ds_dataType == IIAPI_LVCH_TYPE || X.ds_dataType == IIAPI_GEOM_TYPE || X.ds_dataType == IIAPI_POINT_TYPE || X.ds_dataType == IIAPI_MPOINT_TYPE || X.ds_dataType == IIAPI_LINE_TYPE || X.ds_dataType == IIAPI_MLINE_TYPE || X.ds_dataType == IIAPI_POLY_TYPE || X.ds_dataType == IIAPI_MPOLY_TYPE || X.ds_dataType == IIAPI_GEOMC_TYPE)
 
/**
**
**  Name: drvii.c - Data Dictionary Services Facility Ingres Driver
**
**  Description:
**		Handle every communication between the Ingres DBMS and the DDF module
**		This API is based on Ingres/OpenAPI
**
**  History:    
**	5-Feb-98 (marol01)
**	    created
**  28-May-98 (fanra01)
**      Updated message ids.
**  25-Aug-98 (marol01)
**			Add timeout into PDBConnect paramters.
**  05-Oct-1998 (fanra01)
**          Modified percent character checking to exclude those delimited
**          with quotes.
**  18-Jan-1999 (fanra01)
**          Cleaned up compiler warnings on unix.
**  01-Apr-1999 (fanra01)
**          Add a pre-increment flag to handle blobs within a result row.
**          Group column must be pre-incremented following a regular type
**          and post incremented following a blob type or at the beginning
**          of the row.
**  16-Apr-1999 (peeje01)
**          Fix memory alignment problems on Unix
**  20-Apr-1999 (fanra01)
**          Fix integer conversions when smaller than i4.
**  12-Mar-1999 (schte01)
**          Moved #ifdefs and #endifs to column 1.
**  16-Mar-1999 (muhpa01)
**	    On HPUX, '&va_arg' produces compile error - in PDBParams(),
**	    modified cases for int & float types handling of va_arg.
**  10-May-1999 (fanra01)
**          Correct sizing of output buffer when dealing with char fields.
**  06-Aug-1999 (fanra01)
**          Changed all nat, u_nat and longnat to i4, u_i4 and i4 respectively
**          for building in the 3.0 codeline.
**  07-Jul-2000 (fanra01)
**          Bug 102050
**          Add decimal point character to float to text conversion.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      16-Mar-2001 (fanra01)
**          Bug 104298
**          Enable processing of sets of results where there are no blob
**          blob columns in the result set.  Determine the number of rows
**          to be processed by the limitation of the memory block for holding
**          the result data.
**      29-Jun-2001 (fanra01)
**          Sir 103694
**          Add processing for NCHAR and NVARCHAR columns in a database.
**          Convert the UCS2 format to UTF8 for display in the browser.
**          No additional processing added for LNVARCHAR type.
**      20-Jul-2001 (fanra01)
**          Sir 103694
**          Changes required to build on unix.  Use of STATICDEF and readonly
**          breaks the compilation.  Also cleaned a couple of warnings.
**      25-Jan-2002 (fanra01)
**          Bug 106926
**          Separate the IIAPI_NCHA_TYPE form the IIAPI_NVCH_TYPE.
**	18-Sep-2002 (hanje04)
**	   Using & operator on va_arg causes compiler errors on Itanium Linux
**	   so use HP method in PDBParams(). Smallest 'type' that va_arg() will
**	   take on Itanium Linux is int so use II_INT4 case for both INT2 &
**	   INT4
**	11-oct-2002 (somsa01)
**	    Changed PDBParams() such that we pass in addresses of int and
**	    float arguments.
**      17-Aug-2010 (thich01)
**          Make changes to treat spatial types like LBYTEs or NBR type as BYTE.
**/

/*
** Prototype for forward reference of conversion function.
*/
static GSTATUS
PDBucs2_to_utf8( ucs2** sourceStart, ucs2* sourceEnd,
    utf8** targetStart, utf8* targetEnd );

static char			driverName[] = "ingres II";
static bool			initStatus = FALSE;

/*
** Name: OIRepOIStruct()	- Internal function to initialize Ingres structure
**
** Description:
**	Match Ingres types with Driver types
**
** Inputs:
**	OI_Param	: Ingres structure
**	u_nat		: driver type code
**
** Outputs:
**	OI_Param	: initialized Ingres structure 
**
** Returns:
**	GSTATUS		: E_OG0001_OUT_OF_MEMORY
**				  GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/

GSTATUS  
OIRepOIStruct (
	OI_Param **last, 
	u_i4 type) 
{
	GSTATUS err = GSTAT_OK;
	OI_Param  *buf = NULL;

	err = G_ME_REQ_MEM(0,  buf , OI_Param, 1);
	switch (type) 
	{
	case DDF_DB_INT : 
		buf->type = IIAPI_INT_TYPE;
		buf->size = sizeof(II_INT4);
		break;
	case DDF_DB_FLOAT : 
		buf->type = IIAPI_FLT_TYPE;
		buf->size = sizeof(II_FLOAT8);
		break;
	case DDF_DB_TEXT :
		buf->type = IIAPI_CHA_TYPE;
		buf->size = -1;
		break;
	case DDF_DB_DATE :
		buf->type = IIAPI_DTE_TYPE;
		buf->size = -1;
		break;
	case DDF_DB_BLOB :
		buf->type = IIAPI_LBYTE_TYPE;
		buf->size = 0x7FFF;
		break;
	default :;
	}
	buf->columnType = IIAPI_COL_QPARM;
	buf->next = NULL;
	if (*last != NULL)
		(*last)->next = buf;
	(*last) = buf;
return(err);
}

/*
** Name: OIInitQuery()	- Internal function to initialize default 
**						  query GCA parameters
**
** Description:
**	
**
** Inputs:
**	POI_QUERY	: Pointer to an ingres query
**
** Outputs:
**
** Returns:
**	GSTATUS		: GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/

GSTATUS  
OIInitQuery(
	POI_QUERY query) 
{
	GSTATUS err = GSTAT_OK;
	query->queryParm.qy_genParm.gp_callback = NULL;
	query->queryParm.qy_genParm.gp_closure = NULL;
	query->queryParm.qy_queryText = query->text;
	query->queryParm.qy_stmtHandle = ( II_PTR )NULL;
	query->getDescrParm.gd_genParm.gp_callback = NULL;
	query->getDescrParm.gd_genParm.gp_closure = NULL;
	query->getDescrParm.gd_descriptorCount = 0;
	query->getDescrParm.gd_descriptor = NULL;
return(err);
}

/*
** Name: PDBConnect()	- Connect function
**
** Description:
**	Open a connection to the ingres database
**
** Inputs:
**	char *		: network node can be NULL
**	char *		: database name mustn't be NULL
**	char *		: ingres server class can be NULL
**	char *		: user name can be NULL (current user will be used)
**	char *		: password can be NULL
**
** Outputs:
**	PDB_SESSION : session pointer 
**
** Returns:
**	GSTATUS		:	E_DF0033_DB_CONNECT_ALR_OPEN
**					E_DF0039_DB_INCORRECT_NAME
**					E_DF0039_DB_INCORRECT_NAME
**					E_OD0002_DB_ST_ERROR   :
**					E_OD0003_DB_ST_FAILURE :
**					E_OD0011_DB_ST_NOT_INITIALIZED :
**					E_OD0004_DB_ST_INVALID_HANDLE  :
**					E_OD0005_DB_ST_OUT_OF_MEMORY
**					E_OD0006_DB_ST_UNKNOWN_ERROR
**					GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/

GSTATUS  
PDBConnect	(
	PDB_SESSION session, 
	char *node, 
	char *dbname, 
	char *svrClass, 
	char *user, 
	char *password,
	i4	timeout) 
{
   GSTATUS				err = GSTAT_OK;
   IIAPI_CONNPARM	connParm;
   POI_SESSION		oisession = NULL;

   G_ASSERT (session->SpecificInfo, E_DF0030_DB_SESSION_ALR_OPENED);

   if (!initStatus) 
   {
	   err = OI_Initialize((timeout < 1) ? -1 : timeout);
	   initStatus = TRUE;
   }
	
   if (err == GSTAT_OK) 
   {
	   char *name = NULL;

	   err = G_ME_REQ_MEM(0, oisession, struct __OI_SESSION, 1);
	   if (err == GSTAT_OK) 
		   session->SpecificInfo = (PTR) oisession;

	   err = G_ME_REQ_MEM(
				0, 
				name, 
				char, 
				((node) ? STlength(node) : 0) + 
				((dbname) ? STlength(dbname) : 0) + 
				((svrClass) ? STlength(svrClass) : 0) + 4);

	   if (err == GSTAT_OK) 
	   {
		    name[0] = EOS;
			if (node != NULL && node[0] != EOS) 
			{
				STcat(name, node);
				STcat(name, "::");
			}
			if (dbname != NULL)
				STcat(name, dbname);
			if (svrClass != NULL && svrClass[0] != EOS) 
			{
				STcat(name, "/");
				STcat(name, svrClass);
			}

			if (name == EOS) 
			{
				err = DDFStatusAlloc (E_DF0039_DB_INCORRECT_NAME);
			} 
			else 
			{
				oisession->timeout = (timeout < 1) ? -1 : timeout;

				connParm.co_genParm.gp_callback = NULL;
				connParm.co_genParm.gp_closure = NULL;
				connParm.co_target = name;
				connParm.co_connHandle = NULL;
				connParm.co_tranHandle = NULL;
		    connParm.co_username = user;
				if (password == NULL || password[0] == EOS)
					connParm.co_password = NULL;
				else
					connParm.co_password = password;

				connParm.co_timeout = oisession->timeout;

				IIapi_connect( &connParm );

				err = OI_GetResult( &connParm.co_genParm, oisession->timeout);
				if (err == GSTAT_OK) 
				{
					oisession->connHandle = connParm.co_connHandle;
					oisession->tranHandle = connParm.co_tranHandle;
				} 
			}
			MEfree((PTR) name);
	   }
	   if (err != GSTAT_OK) 
	   {
	 	 MEfree((PTR)oisession);
		 session->SpecificInfo = NULL;
	   }
   }
return(err);
}

/*
** Name: OI_Parse()	- Parse the Ingres Query
**
** Description:
**	Read the query, detect variables, create Ingres params list 
**	and format the query
**
** Inputs:
**	u_nat		: type : 
**					OI_SELECT_QUERY, 
**					OI_EXECUTE_QUERY, 
**					OI_SYSTEM_QUERY, 
**					OI_PROCEDURE
**	char*		: query text
**
** Outputs:
**	POI_PARAM*	: pointer to the param list
**	II_INT2*	: number of paramters
**
** Returns:
**	GSTATUS		:	GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**      05-Oct-1998 (fanra01)
**          Modified percent character checking to exclude those delimited
**          with quotes.
*/

GSTATUS  
OI_Parse (
	u_i4 type, 
	char *stmt, 
	POI_PARAM *params, 
	II_INT2 *number) 
{
	GSTATUS err = GSTAT_OK;
	OI_Param *first = NULL, *last = NULL, *buf = NULL;
	char rep1, rep2;
	II_INT2 numberOfParams = 0;
    bool delim = FALSE;

	rep1 = ' ';
	rep2 = '?';
	while (err == GSTAT_OK && stmt[0] != EOS) 
	{
		if (type == OI_SELECT_QUERY &&
			(stmt[0] == 'w' || stmt[0] == 'W') &&
			(stmt[1] == 'h' || stmt[1] == 'H') &&
			(stmt[2] == 'e' || stmt[2] == 'E') &&
			(stmt[3] == 'r' || stmt[3] == 'R') &&
			(stmt[4] == 'e' || stmt[4] == 'E')) 
		{
			rep1 = '~';
			rep2 = 'V';
			stmt += 5;
		}
        if (stmt[0] == '\'')
            delim = (delim == FALSE) ? TRUE : FALSE;
		if (stmt[0] == '%' && delim == FALSE) 
		{
			numberOfParams++;
			err = OIRepOIStruct (&last, stmt[1]);
			if (first == NULL) 
				first = last;
			stmt[0] = rep1;
			stmt[1] = rep2;
			stmt++;
		}
		stmt++;
	}
	if (err != GSTAT_OK) 
	{
		while (first != NULL) 
		{
			buf = first;
			first = first->next;
			MEfree((PTR)buf);
		}
		*params = NULL;
		*number = 0;
	} 
	else 
	{
		*params = first;
		*number = numberOfParams;
	}
return(err);
}

/*
** Name: PDBPrepare()	- Prepare a query for a later execution
**
** Description:
**	initialize a query in order to execute it later. 
**	This is not equivalent to an SQL prepare
**
** Inputs:
**	char*		: statement
**      i4              : rowset
**      i4              : setsize
**
** Outputs:
**	PDB_QUERY 	: query structure
**
** Returns:
**	GSTATUS		: E_OG0001_OUT_OF_MEMROY
**				  GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**      19-Mar-2001 (fanra01)
**          Update the PDBPrepare function to take the rowset and setsize
**          and store them in the query structure.
*/
GSTATUS
PDBPrepare (
    PDB_QUERY qry,
    char *statement,
    i4 rowset,
    i4 setsize )
{
    GSTATUS     err = GSTAT_OK;
    POI_QUERY   query = NULL;

    err = G_ME_REQ_MEM(0, query, OI_QUERY, 1);
    if (err == GSTAT_OK)
    {
        query->select.minrowset = rowset;
        query->select.maxsetsize= setsize;

        err = OI_Query_Type(statement, &(query->type));
        if (err == GSTAT_OK)
        {
            u_i4 length = STlength(statement) + 1;
            STprintf(query->name, "ice%x", query);
            switch (query->type)
            {
            case OI_SELECT_QUERY:
                err = G_ME_REQ_MEM(0, query->text, char, length + 20);
                if (err == GSTAT_OK)
                    STprintf(query->text, "%s for readonly", statement);
                break;
            default:
                G_ST_ALLOC(query->text, statement);
            }
        }
        if (err == GSTAT_OK)
            err = OI_Parse(
                    query->type,
                    query->text,
                    &(query->params),
                    &(query->numberOfParams));

        if (err == GSTAT_OK)
        {
            switch (query->type)
            {
            case OI_SELECT_QUERY:
                {
                    POI_PARAM buf;
                    err = G_ME_REQ_MEM(0,  buf , OI_Param, 1);
                    if (err == GSTAT_OK)
                    {
                        buf->type = IIAPI_CHA_TYPE;
                        buf->value = query->name;
                        buf->size = STlength( query->name );
                        buf->columnType = IIAPI_COL_SVCPARM;
                        buf->next = query->params;
                        query->params = buf;
                        query->numberOfParams++;
                    }
                }
                break;
            case OI_EXECUTE_QUERY:
                if (query->numberOfParams > 0)
                {
                    char *tmp = query->text;
                    err = G_ME_REQ_MEM(
                            0,
                            query->text,
                            char,
                            STlength(tmp) + STlength(query->name) + 20);

                    if (err == GSTAT_OK)
                    {
                        STprintf(
                            query->text,
                            "prepare %s from %s",
                            query->name,
                            tmp);

                        MEfree(tmp);
                    }
                }
                break;
            default: ;
            }
        }
        if (err != GSTAT_OK)
        {
            MEfree(query->text);
            MEfree((PTR)query);
        }
        else
        {
            err = OIInitQuery(query);
            qry->SpecificInfo = (PTR) query;
        }
    }
    return(err);
}

/*
** Name: PDBGetStatement()	- 
**
** Description:
**
** Inputs:
**	PDB_QUERY 	: query structure
**
** Outputs:
**	char**		: statement
**
** Returns:
**				  GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/

GSTATUS  
PDBGetStatement (
	PDB_QUERY qry, 
	char* *statement) 
{
	GSTATUS		err = GSTAT_OK;
	POI_QUERY oiquery;

	oiquery = (POI_QUERY) qry->SpecificInfo;
	*statement = oiquery->text;
return(err);
}

/*
** Name: PDBCommit()	- Commit a transaction
**
** Description:
**
** Inputs:
**	PDB_SESSION	: session pointer
**
** Outputs:
**
** Returns:
**	GSTATUS		: 
**				  E_OD0002_DB_ST_ERROR   :
**				  E_OD0003_DB_ST_FAILURE :
**				  E_OD0011_DB_ST_NOT_INITIALIZED :
**				  E_OD0004_DB_ST_INVALID_HANDLE  :
**				  E_OD0005_DB_ST_OUT_OF_MEMORY
**				  E_OD0006_DB_ST_UNKNOWN_ERROR
**				  GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/

GSTATUS  
PDBCommit(
	PDB_SESSION session) 
{
	GSTATUS err = GSTAT_OK;
	IIAPI_COMMITPARM  commitParm;
	POI_SESSION oisession = NULL;

	G_ASSERT(!session->SpecificInfo, E_DF0029_DB_SESSION_NOT_OPENED);
	oisession = (POI_SESSION) session->SpecificInfo;

 	if (oisession->tranHandle != NULL) 
	{
		commitParm.cm_genParm.gp_callback = NULL;
		commitParm.cm_genParm.gp_closure = NULL;
		commitParm.cm_tranHandle = oisession->tranHandle;

		IIapi_commit( &commitParm );
		err = OI_GetResult( &commitParm.cm_genParm, oisession->timeout);
		oisession->tranHandle = NULL;
	}
return(err);
}

/*
** Name: PDBRollback()	- Rollback a transaction
**
** Description:
**
** Inputs:
**	PDB_SESSION	: session pointer
**
** Outputs:
**
** Returns:
**	GSTATUS		: 
**				  E_OD0002_DB_ST_ERROR   :
**				  E_OD0003_DB_ST_FAILURE :
**				  E_OD0011_DB_ST_NOT_INITIALIZED :
**				  E_OD0004_DB_ST_INVALID_HANDLE  :
**				  E_OD0005_DB_ST_OUT_OF_MEMORY
**				  E_OD0006_DB_ST_UNKNOWN_ERROR
**				  GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/

GSTATUS  
PDBRollback(
	PDB_SESSION session) 
{
	GSTATUS					err = GSTAT_OK;
  IIAPI_ROLLBACKPARM		rollbackParm;
	POI_SESSION				oisession = NULL;

	G_ASSERT(!session->SpecificInfo, E_DF0029_DB_SESSION_NOT_OPENED);
	oisession = (POI_SESSION) session->SpecificInfo;

	if (oisession->tranHandle != NULL) 
	{
		rollbackParm.rb_genParm.gp_callback = NULL;
		rollbackParm.rb_genParm.gp_closure = NULL;
		rollbackParm.rb_tranHandle = oisession->tranHandle;
		rollbackParm.rb_savePointHandle = NULL;

		IIapi_rollback( &rollbackParm );
		err = OI_GetResult( &rollbackParm.rb_genParm, oisession->timeout);

		oisession->tranHandle = ( II_PTR )NULL;
	}
return(err);
}

/*
** Name: OI_RunExecute()	- Internal function to execute a non select query
**
** Description:
**
** Inputs:
**	POI_SESSION	: session pointer
**	POI_QUERY	: query pointer
**
** Outputs:
**
** Returns:
**	GSTATUS		: 
**				  I_OD0001_DB_ST_NO_DATA :
**				  E_OD0002_DB_ST_ERROR   :
**				  E_OD0003_DB_ST_FAILURE :
**				  E_OD0011_DB_ST_NOT_INITIALIZED :
**				  E_OD0004_DB_ST_INVALID_HANDLE  :
**				  E_OD0005_DB_ST_OUT_OF_MEMORY
**				  E_OD0006_DB_ST_UNKNOWN_ERROR
**				  GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/

GSTATUS
OI_RunExecute (
	POI_SESSION oisession, 
	POI_QUERY oiquery ) 
{
	GSTATUS err = GSTAT_OK;
	char	*stmt = NULL;
	u_i4	count = 0;

	oiquery->rowcount = 0;
	oiquery->queryParm.qy_queryType = IIAPI_QT_QUERY;
	oiquery->queryParm.qy_parameters = FALSE;
	oiquery->queryParm.qy_connHandle = oisession->connHandle;
	oiquery->queryParm.qy_tranHandle = oisession->tranHandle;
	oiquery->queryParm.qy_stmtHandle = ( II_PTR )NULL;

	IIapi_query( &(oiquery->queryParm) );
	oisession->tranHandle = oiquery->queryParm.qy_tranHandle;
	err = OI_GetResult( &(oiquery->queryParm.qy_genParm), oisession->timeout );

	if (err == GSTAT_OK && oiquery->numberOfParams > 0) 
	{
		err = OI_GetQInfo( oiquery->queryParm.qy_stmtHandle, &count, oisession->timeout);
		CLEAR_STATUS ( OI_Close(oiquery->queryParm.qy_stmtHandle, oisession->timeout));
		if (err == GSTAT_OK) 
			err = G_ME_REQ_MEM(0, stmt, char, STlength(oiquery->name) + 10);
		if (err == GSTAT_OK) 
		{
			STprintf(stmt, "execute %s", oiquery->name);
			oiquery->queryParm.qy_queryText = stmt;
			oiquery->queryParm.qy_parameters = TRUE;
		
			oiquery->queryParm.qy_queryType = IIAPI_QT_EXEC;
			oiquery->queryParm.qy_tranHandle = oisession->tranHandle;
			oiquery->queryParm.qy_stmtHandle = ( II_PTR )NULL;
			
			IIapi_query( &(oiquery->queryParm) );
			oisession->tranHandle = oiquery->queryParm.qy_tranHandle;
			err = OI_GetResult( &oiquery->queryParm.qy_genParm, oisession->timeout);
			oiquery->queryParm.qy_queryText = oiquery->text;				

			if (! err && oiquery->numberOfParams > 0)
				err = OI_PutParam( 
							&oiquery->queryParm, 
							oiquery->params, 
							oiquery->numberOfParams,
							oisession->timeout);		
		}
	}

	if (err == GSTAT_OK)
		err = OI_GetQInfo(	oiquery->queryParm.qy_stmtHandle, 
												&oiquery->rowcount, 
												oiquery->session->timeout );
return(err);
}

/*
** Name: InitFetchRowBlock
**
** Description:
**      Create a row fetch block. This contains the following information:
**    - The total number of columns in the select set
**    - The number of column groups in the select set. A group consists of a
**      single blob column, or any number of contiguous non-blob columns.
**    - The total number of blob columns in the select set.
**    - A pointer to a block of memory big enough to buffer the entire row.
**      If there are blob coloumns, the buffer will only contain space for
**      the first segment of the column.
**    - An array of IIAPI_GETCOLPARM structs, one for each column group
**    - An array of void pointers, one for each blob column. The array item n
**      is passed to the blob handler callback function for blob n.
**
** Inputs:
**      POI_QUERY		OpenIngres query
**
** Outputs:
**
** Returns:
**      GSTATUS
**
** History:
**      11-dec-1996 (wilan06)
**          Created
**      16-Mar-2001 (fanra01)
**          Determine when multiple rows can be handled and populate the
**          result set memory with tuple values.
*/
GSTATUS 
InitFetchRowBlock (POI_QUERY oiquery )
{
    GSTATUS             err = GSTAT_OK;
    u_i4                cbRow = 0;
    u_i4                group = 0;
    char*               p = NULL;
    u_i4                cBlobs = 0;
    i4                  i;
    i4                  j;
    IIAPI_DATAVALUE*    pdv = NULL;
    IIAPI_GETCOLPARM*   pgcp = NULL;
    IIAPI_GETDESCRPARM* pgdp = &oiquery->getDescrParm;
    OI_GROUP*           groups = NULL;
    i4                  multirow;
    i4                  rows;
    i4                  memsize;
    i4                  maxtupmem = oiquery->select.maxsetsize;

    /*
    ** Initialise the row set fields in the select structure.
    */
    rows = oiquery->select.minrowset;
    oiquery->select.currentRow = -1;
    oiquery->select.rowsReturned = 0;

    for (i=0; i < pgdp->gd_descriptorCount; i++)
        cbRow += pgdp->gd_descriptor[i].ds_length;

    if (cbRow > 0)
    {
        int cGroups = 0;
        /* if the SELECT includes BLOB parameters, we need to
        ** allocate an array of IIAPI_GETCOLPARM structures. This
        ** is because we need to make separate calls to getColumns
        ** for each block of non-BLOB params, and each BLOB param.
        ** We count the number of groups and blobs, as follows: Each
        ** blob column is a group. Each time a blob column preceeds
        ** a non blob column, this is a group. The first column
        ** is always a group.
        */
        for (i = 0; i < pgdp->gd_descriptorCount; i++)
        {
            /* Each blob column is a group */
            if (IsBlobColumn (pgdp->gd_descriptor[i]))
            {
                cBlobs++;
                cGroups++;
            }
            else if (i > 0)
            {
                /* if the last column was a blob, and this isn't
                ** that's a group */
                if (IsBlobColumn(pgdp->gd_descriptor[i-1]))
                    cGroups++;
            }
            else
            {
                /* First column is automatically a group */
                cGroups++;
            }
        }
        /*
        ** if there are no blobs in this result set then it can be bulk
        ** processed.
        */
        if ((multirow = ((cBlobs == 0) && (cGroups == 1))) == 1)
        {
            /*
            ** Calculate the maximum number of rows that will fit into the
            ** configured memory area.
            */
            i4  maxrows = (maxtupmem/cbRow);
            /*
            ** set rows accroding to the returned number of rows
            */
            if ((rows = oiquery->select.rowsReturned) == 0)
            {
                /*
                ** if no indication of the number of rows returned
                ** use the minrowset for the transaction.
                ** if the row doesn't fit into the memory block deal with
                ** a single row at a time.
                */
                rows = (maxrows == 0) ?
                    DEFAULT_ROW_SET : oiquery->select.minrowset;
            }
            else
            {
                /*
                ** if row size greater than the configured set size revert
                ** to row-at-a-time processing
                ** otherwise limit the number of rows to be processed to
                ** the amount of configured memory.
                */
                if (maxrows == 0)
                {
                    rows = DEFAULT_ROW_SET;
                }
                else
                {
                    rows = (rows > maxrows) ? maxrows : rows;
                }
            }
            memsize =  rows * cbRow;
        }
        else
        {
            /*
            ** blobs in result set revert to row-at-a-time processing.
            */
            rows = DEFAULT_ROW_SET;
            memsize = cbRow;
        }
        /*
        ** We need to allocate space for the OI_GROUP struct,
        ** the array of getColumns parameters, the array of
        ** user ptrs and the array of IIAPI_DATAVALUE structs.
        ** We do this in in a single block, because it's slighly
        ** more efficient than allocating 4 small blocks.
        */
        err = G_ME_REQ_MEM(    0,
            groups,
            OI_GROUP,
            sizeof (OI_GROUP) +
            (sizeof (IIAPI_GETCOLPARM) * cGroups) +
            (sizeof (IIAPI_DATAVALUE)* (pgdp->gd_descriptorCount * rows)));
        if (err == GSTAT_OK)
        {
            oiquery->select.groups = groups;
            /* allocate space for the row data in a separate block, because it
            ** is liable to be fairly large
            */
            err = G_ME_REQ_MEM(0, groups->groupFirst, i4, cGroups);
            if (err == GSTAT_OK)
                err = G_ME_REQ_MEM(0, groups->pRowBuffer, char, memsize);
            if (err == GSTAT_OK)
            {
                p = groups->pRowBuffer;
                groups->cColumns = pgdp->gd_descriptorCount;
                groups->cColumnGroups = cGroups;
                groups->cBlobs = cBlobs;
                groups->grppreinc = FALSE;
                /* The array of IIAPI_GETCOLPARM starts after the
                ** OI_GROUP
                */
                groups->agcp = (IIAPI_GETCOLPARM*)(groups + 1);
                /* column data values start after the array of IIAPI_GETCOLPARM
                */
                pdv = (IIAPI_DATAVALUE*)(groups->agcp + cGroups);
                /* Initialise each IIAPI_GETCOLPARM, and its array of data values.
                ** because we requested zero-initialised memory, we only init
                ** non-zero fields
                */
                for (i=0; i < groups->cColumns; i++)
                {
                    if (IsBlobColumn(pgdp->gd_descriptor[i]) ||
                        ((i > 0) && IsBlobColumn (pgdp->gd_descriptor[i-1])) ||
                        (i == 0))
                    {
                        groups->groupFirst[group] = i;
                        pgcp = &(groups->agcp[group]);
                        /* make the column data array point to the data value */
                        pgcp->gc_columnData = pdv;
                        pgcp->gc_stmtHandle = pgdp->gd_stmtHandle;
                        pgcp->gc_rowCount = (i2)rows;
                        pgcp->gc_columnCount = 1;
                        group++;
                    }
                    else
                    {
                        pgcp->gc_columnCount++;
                    }
                    /* Initialise the data value for this item */
                    pdv->dv_length = pgdp->gd_descriptor[i].ds_length;
                    pdv->dv_value = p;
                    p += pdv->dv_length;
                    pdv++;
                }
                /*
                ** if processing multiple rows copy all the remaining rows.
                */
                if (multirow)
                {
                    IIAPI_DATAVALUE* srcpdv;
                    for (i=0; i < pgcp->gc_rowCount-1; i+=1)
                    {
                        srcpdv = pgcp->gc_columnData;
                        for (j=0; j < groups->cColumns; j+=1, pdv+=1, srcpdv+=1)
                        {
                            MEcopy(srcpdv, sizeof(IIAPI_DATAVALUE), pdv);
                            pdv->dv_value = p;
                            p += pdv->dv_length;
                        }
                    }
                }
            }
        }
    }
    return(err);
}

/*
** Name: OI_RunOpen ()	- Internal function to execute a select query
**
** Description:
**
** Inputs:
**	POI_SESSION	: session pointer
**	POI_QUERY	: query pointer
**
** Outputs:
**
** Returns:
**	GSTATUS		: 
**				  I_OD0001_DB_ST_NO_DATA :
**				  E_OD0002_DB_ST_ERROR   :
**				  E_OD0003_DB_ST_FAILURE :
**				  E_OD0011_DB_ST_NOT_INITIALIZED :
**				  E_OD0004_DB_ST_INVALID_HANDLE  :
**				  E_OD0005_DB_ST_OUT_OF_MEMORY
**				  E_OD0006_DB_ST_UNKNOWN_ERROR
**				  GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**      16-Mar-2001 (fanra01)
**          Add a call to OI_GetQInfo to get the number of rows returned.
*/

GSTATUS  
OI_RunOpen( POI_SESSION oisession, POI_QUERY oiquery )
{
    GSTATUS err = GSTAT_OK;
    u_i4    type = 0;

    oiquery->queryParm.qy_queryType = IIAPI_QT_OPEN;
    oiquery->queryParm.qy_parameters = TRUE;
    oiquery->queryParm.qy_connHandle = oisession->connHandle;
    oiquery->queryParm.qy_tranHandle = oisession->tranHandle;
    oiquery->queryParm.qy_stmtHandle = ( II_PTR )NULL;
    
    IIapi_query( &oiquery->queryParm );
    oisession->tranHandle = oiquery->queryParm.qy_tranHandle;

    err = OI_GetResult( &(oiquery->queryParm.qy_genParm), oisession->timeout );
    if (err == GSTAT_OK )
    {
        err = OI_PutParam(
            &oiquery->queryParm,
            oiquery->params,
            oiquery->numberOfParams,
            oisession->timeout);

    }
    if (err == GSTAT_OK)
    {

        err = OI_GetDescriptor(
            &oiquery->getDescrParm,
            oiquery->queryParm.qy_stmtHandle,
            oisession->timeout);

    }
    if (err == GSTAT_OK)
    {
        /*
        ** Get the number of rows for this query and store it in the
        ** structure for use in later processing.
        */
        err = OI_GetQInfo( oiquery->queryParm.qy_stmtHandle,
            (u_i4*)&oiquery->select.rowsReturned,
            oisession->timeout);
    }
    if (err == GSTAT_OK)
    {
        err = InitFetchRowBlock (oiquery);
    }
    if (err == GSTAT_OK)
    {
        oiquery->select.status = TRUE;
    }
    return(err);
}

/*
** Name: PDBParams ()	- Initialize query parameter from a dynamic list
**
** Description:
**
** Inputs:
**	PDB_QUERY	: query pointer
**	char*		: parameter definition. Used to check the correspondance 
**				  between query parameters and function parameters 
**	...			: dynamic parameters
**
** Outputs:
**
** Returns:
**	GSTATUS		: E_DF0036_DB_UNSUP_QRY_TYPE
**				  E_DF0037_DB_PARAMS_NOT_MATCH
**				  GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/

GSTATUS  
PDBParams (
	PDB_QUERY qry, 
	char *paramdef, 
	...) 
{
	GSTATUS err = GSTAT_OK;
	OI_Param *first = NULL;
	POI_QUERY oiquery = NULL;
	va_list	ap;
	
	oiquery = (POI_QUERY) qry->SpecificInfo;

	switch (oiquery->type) 
	{
	case OI_EXECUTE_QUERY:
		first = oiquery->params;
		break;
	case OI_SELECT_QUERY:
		first = oiquery->params->next;
		break;
	default:
		err = DDFStatusAlloc (E_DF0036_DB_UNSUP_QRY_TYPE);
	}


	if (err == GSTAT_OK) 
	{
		u_i4 i = 0;
		va_start( ap, paramdef );
		while (first != NULL) 
		{
#ifdef DEBUG
			if (*paramdef++ != '%') 
			{ 
				err = DDFStatusAlloc (E_DF0037_DB_PARAMS_NOT_MATCH); 
				return(err); 
			}
#endif	

			switch(first->type) 
			{
			case IIAPI_INT_TYPE :
#ifdef DEBUG
				if (*paramdef++ != DDF_DB_INT) 
				{ 
					err = DDFStatusAlloc (E_DF0037_DB_PARAMS_NOT_MATCH); 
					return(err); 
				}
#endif	
				switch(first->size) 
				{
				    case sizeof(II_INT2) :
#ifndef i64_lnx
					first->value = (PTR) va_arg(ap,
								    II_INT2*);
					break;
#endif  /* i64_lnx */
				    case sizeof(II_INT4) : 
					first->value = (PTR) va_arg(ap,
								    II_INT4*);
					break;
				    default :;
				}
				break;
			case IIAPI_FLT_TYPE : 
#ifdef DEBUG
				if (*paramdef++ != DDF_DB_FLOAT) 
				{ 
					err = DDFStatusAlloc (E_DF0037_DB_PARAMS_NOT_MATCH); 
					return(err); 
				}
#endif	
				switch(first->size) 
				{
				    case sizeof(II_FLOAT4) :
					first->value = (PTR) va_arg(ap,
								    II_FLOAT4*);
					break;
				    case sizeof(II_FLOAT8) : 
					first->value = (PTR) va_arg(ap,
								    II_FLOAT8*);
					break;
				    default :;
				}
				break;
			case IIAPI_LBYTE_TYPE:
			case IIAPI_GEOM_TYPE :
			case IIAPI_POINT_TYPE :
			case IIAPI_MPOINT_TYPE :
			case IIAPI_LINE_TYPE :
			case IIAPI_MLINE_TYPE :
			case IIAPI_POLY_TYPE :
			case IIAPI_MPOLY_TYPE :
			case IIAPI_GEOMC_TYPE :
#ifdef DEBUG
				if (*paramdef++ != DDF_DB_BLOB) 
				{
					err = DDFStatusAlloc (E_DF0037_DB_PARAMS_NOT_MATCH); 
					return(err); 
				}
#endif	
				first->value = va_arg( ap, char*);
				break;
			case IIAPI_CHA_TYPE :
#ifdef DEBUG
				if (*paramdef++ != DDF_DB_TEXT) 
				{
					err = DDFStatusAlloc (E_DF0037_DB_PARAMS_NOT_MATCH); 
					return(err); 
				}
#endif	
				first->value = va_arg( ap, char*);
				first->size = STlength((char*) first->value);
				break;
			case IIAPI_DTE_TYPE :
#ifdef DEBUG
				if (*paramdef++ != DDF_DB_DATE) 
				{ 
					err = DDFStatusAlloc (E_DF0037_DB_PARAMS_NOT_MATCH); 
					return(err); 
				}
#endif	
				first->value = va_arg( ap, char*);
				first->size = STlength((char*) first->value);
				break;
			default :;
			}
			first = first->next;
		}
		va_end( ap );
	}
return(err);
}

/*
** Name: PDBParamsFromTab ()	- Initialize query parameter from a table
**
** Description:
**
** Inputs:
**	PDB_QUERY	: query pointer
**	PDBValue	: table of value
**
** Outputs:
**
** Returns:
**	GSTATUS		: E_DF0036_DB_UNSUP_QRY_TYPE
**				  GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/

GSTATUS  
PDBParamsFromTab (
	PDB_QUERY qry, 
	PDB_VALUE values) 
{
	GSTATUS		err = GSTAT_OK;
	OI_Param	*first = NULL;
	POI_QUERY	oiquery = NULL;
	oiquery = (POI_QUERY) qry->SpecificInfo;

	switch (oiquery->type) 
	{
	case OI_EXECUTE_QUERY:
		first = oiquery->params;
		break;
	case OI_SELECT_QUERY:
		first = oiquery->params->next;
		break;
	default:
		err = DDFStatusAlloc (E_DF0036_DB_UNSUP_QRY_TYPE);
	}

	if (err == GSTAT_OK) 
	{
		u_i4 i = 0;
		while (first != NULL) 
		{
			switch(first->type) 
			{
			case IIAPI_INT_TYPE :
			case IIAPI_FLT_TYPE : 
				first->value = values[i++];
				break;
			case IIAPI_CHA_TYPE :
			case IIAPI_DTE_TYPE :
			case IIAPI_LBYTE_TYPE:
			case IIAPI_GEOM_TYPE :
			case IIAPI_POINT_TYPE :
			case IIAPI_MPOINT_TYPE :
			case IIAPI_LINE_TYPE :
			case IIAPI_MLINE_TYPE :
			case IIAPI_POLY_TYPE :
			case IIAPI_MPOLY_TYPE :
			case IIAPI_GEOMC_TYPE :
				first->value = values[i++];
				if (first->value == NULL)
					first->size = 0;
				else
					first->size = STlength((char*) first->value);
				break;
			default :;
			}			
			first = first->next;
		}
	}
return(err);
}

/*
** Name: PDBExecute ()	- Execute all kind of query
**
** Description:
**	Determine the type of the query, and then call the appropriate function
**
** Inputs:
**	PDB_SESSION	: session pointer
**	PDB_QUERY	: query pointer
**
** Outputs:
**
** Returns:
**	GSTATUS		: E_DF0036_DB_UNSUP_QRY_TYPE
**				  E_DF0029_DB_SESSION_NOT_OPENED
**				  E_OD0002_DB_ST_ERROR   :
**				  E_OD0003_DB_ST_FAILURE :
**				  E_OD0011_DB_ST_NOT_INITIALIZED :
**				  E_OD0004_DB_ST_INVALID_HANDLE  :
**				  E_OD0005_DB_ST_OUT_OF_MEMORY
**				  E_OD0006_DB_ST_UNKNOWN_ERROR
**				  GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/

GSTATUS  
PDBExecute	(
	PDB_SESSION session, 
	PDB_QUERY		qry) 
{
	GSTATUS		err = GSTAT_OK;
	POI_SESSION oisession = NULL;
	POI_QUERY	oiquery = NULL;

	G_ASSERT(!session->SpecificInfo, E_DF0029_DB_SESSION_NOT_OPENED);
	oisession = (POI_SESSION) session->SpecificInfo;
	oiquery = (POI_QUERY) qry->SpecificInfo;
	oiquery->session = oisession;

	switch (oiquery->type) 
	{
	case OI_EXECUTE_QUERY:
	case OI_SYSTEM_QUERY:
		err = OI_RunExecute(oisession, oiquery);
		break;
	case OI_SELECT_QUERY:
		err = OI_RunOpen(oisession, oiquery);
		break;
	default:
		err = DDFStatusAlloc (E_DF0036_DB_UNSUP_QRY_TYPE);
	}
return(err);
}


/*
** Name: PDBNext ()	- Go to the next tuple
**
** Description:
**	execute a fetch sql command
**
** Inputs:
**	PDB_QUERY	: query pointer
**
** Outputs:
**	bool		: FALSE if no tuple is available otherwise TRUE
**
** Returns:
**	GSTATUS		: 
**				  E_OD0002_DB_ST_ERROR   :
**				  E_OD0003_DB_ST_FAILURE :
**				  E_OD0011_DB_ST_NOT_INITIALIZED :
**				  E_OD0004_DB_ST_INVALID_HANDLE  :
**				  E_OD0005_DB_ST_OUT_OF_MEMORY
**				  E_OD0006_DB_ST_UNKNOWN_ERROR
**				  GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/

GSTATUS  
PDBNext ( PDB_QUERY qry, bool *exist )
{
    GSTATUS             err = GSTAT_OK;
    POI_QUERY           oiquery;
    IIAPI_GETCOLPARM*   pgcp = NULL;

    oiquery = (POI_QUERY) qry->SpecificInfo;
    if (oiquery->type != OI_SELECT_QUERY)
    {
        *exist = FALSE;
    }
    else
    {
        *exist = TRUE;
        oiquery->select.group_counter = 0;
        oiquery->select.groups->grppreinc = FALSE;

        /*
        ** if this is the first time through or we've read all the rows from
        ** this fetch
        */
        if ((oiquery->select.rowsReturned == 0) ||
            (oiquery->select.currentRow == (oiquery->select.rowsReturned-1)))
        {
            /*
            ** Reset the current row value.
            */
            oiquery->select.currentRow = -1;
            pgcp = &(oiquery->select.groups->agcp[0]);
            IIapi_getColumns( pgcp );
            err = OI_GetResult( &(pgcp->gc_genParm), oiquery->session->timeout );

            if (err != GSTAT_OK && err->number == E_DF0020_DB_ST_NO_DATA)
            {
                u_i4 count = 0;
                *exist = FALSE;
                DDFStatusFree(TRC_INFORMATION, &err);
            }
            if (oiquery->select.groups->cBlobs == 0)
            {
                /*
                ** Get the number of returned rows.
                */
                err = OI_GetQInfo( oiquery->queryParm.qy_stmtHandle,
                    (u_i4*)&oiquery->select.rowsReturned,
                    oiquery->session->timeout );
            }
        }

        if (err == GSTAT_OK && *exist == TRUE)
        {
            oiquery->rowcount++;
            if (oiquery->select.groups->cBlobs == 0)
            {
                /*
                ** bump the current row position.
                */
                oiquery->select.currentRow += 1;
            }
        }
    }
    return(err);
}

/*
** Name: PDBInteger ()	- Drag Integer value. Allocate memory if necessary
**
** Description:
**
** Inputs:
**	PDB_QUERY	: query pointer
**	PROP_NUMBER : column number wanted 
**
** Outputs:
**	nat			: value
**
** Returns:
**	GSTATUS		: 
**				  E_DF0038_DB_INVALID_PROP_NUM
**				  E_OD0002_DB_ST_ERROR  
**				  E_OD0003_DB_ST_FAILURE
**				  E_OD0011_DB_ST_NOT_INITIALIZED
**				  E_OD0004_DB_ST_INVALID_HANDLE 
**				  E_OD0005_DB_ST_OUT_OF_MEMORY
**				  E_OD0006_DB_ST_UNKNOWN_ERROR
**				  GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**      01-Apr-1999 (fanra01)
**          Set group pre-increment to true for integer types.
*/

GSTATUS  
PDBInteger (
    PDB_QUERY qry,
    PROP_NUMBER property,
    i4 **value)
{
    GSTATUS             err = GSTAT_OK;
    POI_QUERY           oiquery = NULL;
    IIAPI_DATAVALUE     *rowStart;
    IIAPI_DATAVALUE     *dataValue;
    IIAPI_DESCRIPTOR    *descriptor;

    oiquery = (POI_QUERY) qry->SpecificInfo;
    G_ASSERT(property < 1 ||
            (II_INT2) property > oiquery->getDescrParm.gd_descriptorCount,
            E_DF0038_DB_INVALID_PROP_NUM);

    property--;
    descriptor = &oiquery->getDescrParm.gd_descriptor[property];
    if ( abs( descriptor->ds_dataType ) != IIAPI_INT_TYPE)
        err = DDFStatusAlloc (E_DF0040_DB_INVALID_TYPE);
    else
    {
        OI_GROUP *group = oiquery->select.groups;
        i4 counter = oiquery->select.group_counter;
        i4 first = group->groupFirst[counter];
        i4 columns;
        i4 current;

        /*
        ** Add setup of position to offset into tuple data
        */
        oiquery->select.groups->grppreinc = TRUE;
        columns = oiquery->select.groups->cColumns;
        current = (oiquery->select.currentRow > 0) ?
            oiquery->select.currentRow : 0;

        rowStart = &group->agcp[counter].gc_columnData[columns * current];
        dataValue = &rowStart[property - first];
        if ( dataValue->dv_null )
        {
            MEfree((PTR)*value);
            *value = NULL;
        }
        else
        {
            err = GAlloc((PTR*)value, dataValue->dv_length, FALSE);
            if (err == GSTAT_OK)
                MECOPY_VAR_MACRO( dataValue->dv_value, dataValue->dv_length, *value);
        }
    }
    return(err);
}

/*
** Name: PDBFloat ()	- Drag Float value. Allocate memory if necessary
**
** Description:
**
** Inputs:
**	PDB_QUERY	: query pointer
**	PROP_NUMBER : column number wanted 
**
** Outputs:
**	float		: value
**
** Returns:
**	GSTATUS		: 
**				  E_DF0038_DB_INVALID_PROP_NUM
**				  E_OD0002_DB_ST_ERROR  
**				  E_OD0003_DB_ST_FAILURE
**				  E_OD0011_DB_ST_NOT_INITIALIZED
**				  E_OD0004_DB_ST_INVALID_HANDLE 
**				  E_OD0005_DB_ST_OUT_OF_MEMORY
**				  E_OD0006_DB_ST_UNKNOWN_ERROR
**				  GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/

GSTATUS  
PDBFloat (
	PDB_QUERY qry, 
	PROP_NUMBER property, 
	float **value) 
{
	GSTATUS err = GSTAT_OK;
	property--;
return(err);
}

/*
** Name: PDBDate ()	- Drag Date value. Allocate memory if necessary
**
** Description:
**
** Inputs:
**	PDB_QUERY	: query pointer
**	PROP_NUMBER : column number wanted 
**
** Outputs:
**	char**		: converted value
**
** Returns:
**	GSTATUS		: 
**				  E_DF0038_DB_INVALID_PROP_NUM
**				  E_OD0002_DB_ST_ERROR  
**				  E_OD0003_DB_ST_FAILURE
**				  E_OD0011_DB_ST_NOT_INITIALIZED
**				  E_OD0004_DB_ST_INVALID_HANDLE 
**				  E_OD0005_DB_ST_OUT_OF_MEMORY
**				  E_OD0006_DB_ST_UNKNOWN_ERROR
**				  GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/

GSTATUS  PDBDate (
	PDB_QUERY qry, 
	PROP_NUMBER property, 
	char **value) 
{
	GSTATUS err = GSTAT_OK;
	property--;
return(err);
}

/*
** Name: PDBText ()	- Drag text value. Allocate memory if necessary
**
** Description:
**
** Inputs:
**	PDB_QUERY	: query pointer
**	PROP_NUMBER : column number wanted 
**
** Outputs:
**	char*		: converted value
**
** Returns:
**	GSTATUS		: 
**				  E_DF0040_DB_INVALID_TYPE
**				  E_DF0038_DB_INVALID_PROP_NUM
**				  E_OD0002_DB_ST_ERROR  
**				  E_OD0003_DB_ST_FAILURE
**				  E_OD0011_DB_ST_NOT_INITIALIZED
**				  E_OD0004_DB_ST_INVALID_HANDLE 
**				  E_OD0005_DB_ST_OUT_OF_MEMORY
**				  E_OD0006_DB_ST_UNKNOWN_ERROR
**				  GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**      20-Apr-1999 (fanra01)
**          Initialise the integer variable that values get read into before
**          conversion.  Ensures that values smaller than i4 don't get
**          random elements.
**      10-May-1999 (fanra01)
**          Correct the sizing of the output buffer when dealing with char
**          fields.  Use the dv_length value.
**      07-Jul-2000 (fanra01)
**          Add decimal point character to float to text conversion
**      29-Jun-2001 (fanra01)
**          Add handling for NCHAR, NVCHAR and LNVARCHAR.
**      25-Jan-2002 (fanra01)
**          Add separate IIAPI_NCHA_TYPE handling.
*/
GSTATUS
PDBText (
    PDB_QUERY qry,
    PROP_NUMBER property,
    char **value)
{
    GSTATUS err = GSTAT_OK;
    POI_QUERY oiquery;
    IIAPI_DATAVALUE        *rowStart;
    IIAPI_DATAVALUE        *dataValue;
    IIAPI_DESCRIPTOR    *descriptor;
    II_INT2 size = 0;
    OI_GROUP *group;
    i4  counter;
    i4  first;
    i4  bufsize = 0;

    oiquery = (POI_QUERY) qry->SpecificInfo;
    G_ASSERT(property < 1 ||
        (II_INT2) property > oiquery->getDescrParm.gd_descriptorCount,
        E_DF0038_DB_INVALID_PROP_NUM);

    property--;
    descriptor = &oiquery->getDescrParm.gd_descriptor[property];
    group = oiquery->select.groups;
    counter = oiquery->select.group_counter;

    if (IsBlobColumn((*descriptor)))
    {
        IIAPI_GETCOLPARM*   pgcp;

        if (oiquery->select.groups->grppreinc == TRUE)
        {
            counter += 1;
            oiquery->select.group_counter = counter;
        }
        pgcp = &(oiquery->select.groups->agcp[counter]);

        if (oiquery->select.groups->grppreinc == TRUE)
        {
            IIapi_getColumns( pgcp );
            err = OI_GetResult( &(pgcp->gc_genParm),
                oiquery->session->timeout );
        }
    }
    else
    {
        oiquery->select.groups->grppreinc = TRUE;
    }

    if (err == GSTAT_OK)
    {
        i4 columns;
        i4 current;

        /*
        ** Add setup of position to offset into tuple data
        */
        first = group->groupFirst[counter];
        columns = oiquery->select.groups->cColumns;
        current = (oiquery->select.currentRow > 0) ?
            oiquery->select.currentRow : 0;

        rowStart = &group->agcp[counter].gc_columnData[columns * current];
        dataValue = &rowStart[property - first];

        if ( dataValue->dv_null )
        {
            MEfree(*value);
            *value = NULL;
        }
        else
        {
            switch( abs( descriptor->ds_dataType ) )
            {
                case IIAPI_LBYTE_TYPE:
                case IIAPI_LTXT_TYPE:
                case IIAPI_LVCH_TYPE:
		case IIAPI_GEOM_TYPE :
		case IIAPI_POINT_TYPE :
		case IIAPI_MPOINT_TYPE :
		case IIAPI_LINE_TYPE :
		case IIAPI_MLINE_TYPE :
		case IIAPI_POLY_TYPE :
		case IIAPI_MPOLY_TYPE :
		case IIAPI_GEOMC_TYPE :
                {
                        char *cat = NULL;
                        char *ext = NULL;
                        size = dataValue->dv_length - sizeof (II_UINT2);
                        err = GuessFileType( ((char*) dataValue->dv_value) + sizeof( II_INT2 ),
                            size,
                            &cat,
                            &ext);
                    if (err == GSTAT_OK && ext != NULL)
                    {
                        size = STlength(ext) + 2;
                        err = GAlloc((PTR*)value, size + 1, FALSE);
                        if (err == GSTAT_OK)
                            STprintf(*value, "%s.%s", "", ext);
                    }
                    break;
                }
                case IIAPI_TXT_TYPE:
                case IIAPI_VBYTE_TYPE:
                case IIAPI_VCH_TYPE:
                    MECOPY_CONST_MACRO(dataValue->dv_value,
                        sizeof(II_INT2), &size);
                    err = GAlloc((PTR*)value, size + 1, FALSE);
                    if (err == GSTAT_OK)
                    {
                        MECOPY_VAR_MACRO(
                            ((char*) dataValue->dv_value) + sizeof( size ),
                            size,
                            *value);
                            (*value)[size] = EOS;
                    }
                    break;

                case IIAPI_NCHA_TYPE:
                    size = dataValue->dv_length;    /* width of field in octets */
                    bufsize = (size + 1) * MAX_UTF8_WIDTH;
                    err = GAlloc((PTR*)value,  bufsize, FALSE);
                    if (err == GSTAT_OK)
                    {
                        ucs2* srcstart = (ucs2*)dataValue->dv_value;
                        /*
                        ** end of field is the offset of ucs2 characters
                        */
                        ucs2* srcend = srcstart + (size / sizeof(II_INT2));
                        utf8* tgtstart = (utf8 *)*value;
                        utf8* tgtend = tgtstart + bufsize;
                        err = PDBucs2_to_utf8( &srcstart, srcend, &tgtstart,
                            tgtend);
                    }
                    break;

                case IIAPI_NVCH_TYPE:
                    MECOPY_CONST_MACRO(dataValue->dv_value,
                        sizeof(II_INT2), &size);
                    /*
                    ** Allocate size number of unicode + one end of string
                    ** character
                    */
                    bufsize = (size + 1) * MAX_UTF8_WIDTH;
                    if ((err = GAlloc((PTR*)value, bufsize, FALSE)) == GSTAT_OK)
                    {
                        ucs2* srcstart = (ucs2*)(((char*) dataValue->dv_value) +
                            sizeof( size ));
                        ucs2* srcend = srcstart + size;
                        utf8* tgtstart = (utf8 *)*value;
                        utf8* tgtend = tgtstart + bufsize;
                        err = PDBucs2_to_utf8( &srcstart, srcend, &tgtstart,
                            tgtend);
                    }
                    break;

                case IIAPI_BYTE_TYPE:
                case IIAPI_NBR_TYPE:
                case IIAPI_CHA_TYPE:
                case IIAPI_CHR_TYPE:
                    size = dataValue->dv_length;
                    err = GAlloc((PTR*)value,  size + 1, FALSE);
                    if (err == GSTAT_OK)
                    {
                        MECOPY_VAR_MACRO(
                            (char*) dataValue->dv_value,
                            size,
                            *value);
                        (*value)[size] = EOS;
                    }
                      break;
                case IIAPI_FLT_TYPE:
                    err = GAlloc((PTR*)value, 20, FALSE);
                    if (err == GSTAT_OK)
                    {
                        switch( dataValue->dv_length )
                        {
                            case 4:
                            {
                                II_FLOAT4 fd_value;
                                MECOPY_CONST_MACRO(dataValue->dv_value,
                                       sizeof(II_FLOAT4),
                                       &fd_value);
                                STprintf(*value,
                                    "%f",
                                    fd_value, '.');
                                break;
                            }
                            case 8:
                            {
                                II_FLOAT8 fd_value;
                                MECOPY_CONST_MACRO(dataValue->dv_value,
                                       sizeof(II_FLOAT8),
                                       &fd_value);
                                STprintf(*value,
                                    "%f",
                                    fd_value, '.');
                                break;
                            }
                            default:
                                err = DDFStatusAlloc (E_DF0040_DB_INVALID_TYPE);
                        }
                    }
                    break;
                case IIAPI_INT_TYPE:
                    err = GAlloc((PTR*)value, 20, FALSE);
                    if (err == GSTAT_OK)
                    {
                        switch( dataValue->dv_length )
                        {
                            case 1:
                            {
                            II_INT1 ld_value = 0;
                            MECOPY_CONST_MACRO(dataValue->dv_value,
                                   sizeof(II_INT1),
                                   &ld_value);
                            STprintf(*value,
                                "%ld",
                                ld_value);
                            }
                                break;
                            case 2:
                            {
                            II_INT2 ld_value = 0;
                            MECOPY_CONST_MACRO(dataValue->dv_value,
                                   sizeof(II_INT2),
                                   &ld_value);
                            STprintf(*value,
                                "%ld",
                                ld_value);
                            }
                            break;
                            case 4:
                            {
                            II_INT4 ld_value = 0;
                            MECOPY_CONST_MACRO(dataValue->dv_value,
                                   sizeof(II_INT4),
                                   &ld_value);
                            STprintf(*value,
                                "%ld",
                                ld_value);
                            }
                                break;
                            default:
                                err = DDFStatusAlloc (E_DF0040_DB_INVALID_TYPE);
                        }
                    }
                    break;
                case IIAPI_DTE_TYPE:
                case IIAPI_DEC_TYPE:
                case IIAPI_MNY_TYPE:
                {
                    IIAPI_CONVERTPARM    cv;
                    cv.cv_srcDesc.ds_dataType = descriptor->ds_dataType;
                    cv.cv_srcDesc.ds_nullable = descriptor->ds_nullable;
                    cv.cv_srcDesc.ds_length = descriptor->ds_length;
                    cv.cv_srcDesc.ds_precision = descriptor->ds_precision;
                    cv.cv_srcDesc.ds_scale = descriptor->ds_scale;
                    cv.cv_srcDesc.ds_columnType = descriptor->ds_columnType;
                    cv.cv_srcDesc.ds_columnName = descriptor->ds_columnName;

                    cv.cv_srcValue.dv_null = dataValue->dv_null;
                    cv.cv_srcValue.dv_length = dataValue->dv_length;
                    cv.cv_srcValue.dv_value = dataValue->dv_value;

                    cv.cv_dstDesc.ds_dataType = IIAPI_CHA_TYPE;
                    cv.cv_dstDesc.ds_nullable = FALSE;
                    cv.cv_dstDesc.ds_length = 32;
                    cv.cv_dstDesc.ds_precision = 0;
                    cv.cv_dstDesc.ds_scale = 0;
                    cv.cv_dstDesc.ds_columnType = IIAPI_COL_TUPLE;
                    cv.cv_dstDesc.ds_columnName = NULL;

                    cv.cv_dstValue.dv_null = FALSE;
                    cv.cv_dstValue.dv_length = cv.cv_dstDesc.ds_length;

                    err = GAlloc((PTR*)value, cv.cv_dstValue.dv_length+1, FALSE);
                    if (err == GSTAT_OK)
                    {
                        cv.cv_dstValue.dv_value = *value;
                        IIapi_convertData( &cv );

                        if ( cv.cv_status != IIAPI_ST_SUCCESS )
                            err = DDFStatusAlloc (E_DF0040_DB_INVALID_TYPE);
                        else
                            (*value)[ cv.cv_dstValue.dv_length ] = '\0';
                    }
                    break;
                }
                default: ;
                    err = DDFStatusAlloc (E_DF0040_DB_INVALID_TYPE);
            }
        }
    }
    return(err);
}

/*
** Name: PDBBlob ()	- Drag blob block. Allocate memory if necessary
**
** Description:
**
** Inputs:
**	PDB_QUERY	: query pointer
**	PROP_NUMBER : column number wanted 
**
** Outputs:
**	char**		: value
**
** Returns:
**	GSTATUS		: 
**				  E_DF0040_DB_INVALID_TYPE
**				  E_DF0038_DB_INVALID_PROP_NUM
**				  E_OD0002_DB_ST_ERROR  
**				  E_OD0003_DB_ST_FAILURE
**				  E_OD0011_DB_ST_NOT_INITIALIZED
**				  E_OD0004_DB_ST_INVALID_HANDLE 
**				  E_OD0005_DB_ST_OUT_OF_MEMORY
**				  E_OD0006_DB_ST_UNKNOWN_ERROR
**				  GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/

GSTATUS  
PDBBlob (
	PDB_QUERY qry, 
	PROP_NUMBER property, 
	LOCATION *loc) 
{
	GSTATUS err = GSTAT_OK;
	POI_QUERY oiquery;	
	IIAPI_DATAVALUE		*dataValue;
	IIAPI_DESCRIPTOR	*descriptor;
	IIAPI_GETCOLPARM*   pgcp = NULL;
	u_i4				length;
	FILE				*fp;

	oiquery = (POI_QUERY) qry->SpecificInfo;
	G_ASSERT(property < 1 || 
			(II_INT2) property > oiquery->getDescrParm.gd_descriptorCount, 
			E_DF0038_DB_INVALID_PROP_NUM); 

	property--;
	descriptor = &oiquery->getDescrParm.gd_descriptor[property];

	pgcp = &(oiquery->select.groups->agcp[oiquery->select.group_counter]);
	dataValue = pgcp->gc_columnData;

	if (!dataValue->dv_null && IsBlobColumn((*descriptor)))
	{
		if (SIfopen (loc, "w", SI_BIN, 0, &fp) == OK)
		{
			length = dataValue->dv_length - sizeof (II_UINT2);
			SIwrite(
				length, 
				((char*) dataValue->dv_value) + sizeof( II_INT2 ), 
				(i4*)&length, 
				fp);

			while (err == GSTAT_OK && pgcp->gc_moreSegments)
			{
				IIapi_getColumns( pgcp );
				err = OI_GetResult( &(pgcp->gc_genParm), oiquery->session->timeout );
				if (err == GSTAT_OK)
				{
					length = dataValue->dv_length - sizeof (II_UINT2);
					SIwrite(
						length, 
						((char*) dataValue->dv_value) + sizeof( II_INT2 ), 
						(i4*)&length, 
						fp);
				}
			}
			SIclose(fp);
		}
		else
			err = DDFStatusAlloc (E_DF0062_DB_CANNOT_OPEN);
	}
	else
		err = DDFStatusAlloc (E_DF0040_DB_INVALID_TYPE);

        if (err == GSTAT_OK &&
                (oiquery->select.group_counter + 1) < oiquery->select.groups->cColumnGroups)
        {
            oiquery->select.groups->grppreinc = FALSE;
            oiquery->select.group_counter += 1;
            pgcp = &(oiquery->select.groups->agcp[oiquery->select.group_counter]);
            IIapi_getColumns( pgcp );
            err = OI_GetResult( &(pgcp->gc_genParm), oiquery->session->timeout );
        }

	if (err != GSTAT_OK && err->number == E_DF0020_DB_ST_NO_DATA) 
		DDFStatusFree(TRC_INFORMATION, &err);
return(err);
}

/*
** Name: PDBNumberOfProperties ()	- Number of columns return by the query
**
** Description:
**
** Inputs:
**	PDB_QUERY	: query pointer
**
** Outputs:
**	u_nat		: number
**
** Returns:
**	GSTATUS		: 
**				  E_DF0036_DB_UNSUP_QRY_TYPE
**				  E_OD0002_DB_ST_ERROR  
**				  E_OD0003_DB_ST_FAILURE
**				  E_OD0011_DB_ST_NOT_INITIALIZED
**				  E_OD0004_DB_ST_INVALID_HANDLE 
**				  E_OD0005_DB_ST_OUT_OF_MEMORY
**				  E_OD0006_DB_ST_UNKNOWN_ERROR
**				  GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/

GSTATUS  
PDBNumberOfProperties (
	PDB_QUERY qry, 
	PROP_NUMBER *result) 
{
	GSTATUS err = GSTAT_OK;
	POI_QUERY oiquery;	
	oiquery = (POI_QUERY) qry->SpecificInfo;

	switch (oiquery->type) 
	{
	case OI_EXECUTE_QUERY:
	case OI_SYSTEM_QUERY:
		*result = 0;
		break;
	case OI_SELECT_QUERY:
		*result = oiquery->getDescrParm.gd_descriptorCount;
		break;
	default:
		err = DDFStatusAlloc (E_DF0036_DB_UNSUP_QRY_TYPE);
	}
return(err);
}

/*
** Name: PDBName ()	- Drag column name. Allocate memory if necessary
**
** Description:
**
** Inputs:
**	PDB_QUERY	: query pointer
**	PROP_NUMBER : column number wanted 
**
** Outputs:
**	char**		: name
**
** Returns:
**	GSTATUS		: 
**				  E_DF0038_DB_INVALID_PROP_NUM
**				  E_OD0002_DB_ST_ERROR  
**				  E_OD0003_DB_ST_FAILURE
**				  E_OD0011_DB_ST_NOT_INITIALIZED
**				  E_OD0004_DB_ST_INVALID_HANDLE 
**				  E_OD0005_DB_ST_OUT_OF_MEMORY
**				  E_OD0006_DB_ST_UNKNOWN_ERROR
**				  GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/

GSTATUS  
PDBName (
	PDB_QUERY qry, 
	PROP_NUMBER property, 
	char **name) 
{
	GSTATUS err = GSTAT_OK;
	POI_QUERY oiquery;	
	char *tmp;
	u_i4 length;

	oiquery = (POI_QUERY) qry->SpecificInfo;
	G_ASSERT(property < 1 || 
			(II_INT2) property > oiquery->getDescrParm.gd_descriptorCount, 
			E_DF0038_DB_INVALID_PROP_NUM); 

	property--;

	tmp = oiquery->getDescrParm.gd_descriptor[property].ds_columnName;
	length = STlength(tmp)+1;
	err = GAlloc((PTR*)name, length, FALSE);
	MECOPY_VAR_MACRO(tmp, length, *name); 
return(err);
}

/*
** Name: PDBType ()	- Drag column type
**
** Description:
**
** Inputs:
**	PDB_QUERY	: query pointer
**	PROP_NUMBER : column number wanted 
**
** Outputs:
**	u_nat		: type : DDF_DB_*
**
** Returns:	
**	GSTATUS		: 	
**				  E_DF0040_DB_INVALID_TYPE
**				  E_DF0038_DB_INVALID_PROP_NUM
**				  E_OD0002_DB_ST_ERROR  
**				  E_OD0003_DB_ST_FAILURE
**				  E_OD0011_DB_ST_NOT_INITIALIZED
**				  E_OD0004_DB_ST_INVALID_HANDLE 
**				  E_OD0005_DB_ST_OUT_OF_MEMORY
**				  E_OD0006_DB_ST_UNKNOWN_ERROR
**				  GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/

GSTATUS  PDBType (
	PDB_QUERY qry, 
	PROP_NUMBER property, 
	char *type)
{
	GSTATUS err = GSTAT_OK;
	POI_QUERY oiquery;	
	IIAPI_DESCRIPTOR	*descriptor;

	oiquery = (POI_QUERY) qry->SpecificInfo;
	G_ASSERT(property < 1 || 
			(II_INT2) property > oiquery->getDescrParm.gd_descriptorCount, 
			E_DF0038_DB_INVALID_PROP_NUM); 

	property--;
	descriptor = &oiquery->getDescrParm.gd_descriptor[property];

    switch( abs( descriptor->ds_dataType ) )
	{
	case IIAPI_LOGKEY_TYPE:
	case IIAPI_TABKEY_TYPE:
	case IIAPI_TXT_TYPE:
	case IIAPI_VBYTE_TYPE:
	case IIAPI_VCH_TYPE:
	case IIAPI_BYTE_TYPE:
	case IIAPI_CHA_TYPE:
	case IIAPI_CHR_TYPE:
        case IIAPI_NCHA_TYPE:
        case IIAPI_NVCH_TYPE:
		*type = DDF_DB_TEXT;
		break;
	case IIAPI_LBYTE_TYPE:
	case IIAPI_GEOM_TYPE :
	case IIAPI_POINT_TYPE :
	case IIAPI_MPOINT_TYPE :
	case IIAPI_LINE_TYPE :
	case IIAPI_MLINE_TYPE :
	case IIAPI_POLY_TYPE :
	case IIAPI_MPOLY_TYPE :
	case IIAPI_NBR_TYPE :
	case IIAPI_GEOMC_TYPE :
	case IIAPI_LTXT_TYPE:
	case IIAPI_LVCH_TYPE:
        case IIAPI_LNVCH_TYPE:
		*type = DDF_DB_BLOB;
		break;
	case IIAPI_FLT_TYPE:
	case IIAPI_DEC_TYPE:
	case IIAPI_MNY_TYPE:
		*type = DDF_DB_FLOAT;
		break;
	case IIAPI_INT_TYPE:
		*type = DDF_DB_INT;
		break;
	case IIAPI_DTE_TYPE:
		*type = DDF_DB_DATE;
		break;
	default:
		err = DDFStatusAlloc (E_DF0040_DB_INVALID_TYPE);
	}
return(err);
}

/*
** Name: PDBClose ()	- Close an executed query 
**
** Description:
**
** Inputs:
**	PDB_QUERY	: query pointer
**
** Outputs:
**
** Returns:	
**	GSTATUS		: 	
**				  
**				  E_OD0002_DB_ST_ERROR  
**				  E_OD0003_DB_ST_FAILURE
**				  E_OD0011_DB_ST_NOT_INITIALIZED
**				  E_OD0004_DB_ST_INVALID_HANDLE 
**				  E_OD0005_DB_ST_OUT_OF_MEMORY
**				  E_OD0006_DB_ST_UNKNOWN_ERROR
**				  GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/

GSTATUS  
PDBClose( 
	PDB_QUERY   qry,
	u_i4		*count) 
{
	GSTATUS err = GSTAT_OK;
	POI_QUERY oiquery;

	oiquery = (POI_QUERY) qry->SpecificInfo;

	err = OI_Close(oiquery->queryParm.qy_stmtHandle, 
								 oiquery->session->timeout);

	if (oiquery->getDescrParm.gd_descriptorCount > 0) 
	{	/* this is a select */
		if (oiquery->select.groups != NULL)
		{
			MEfree( (PTR) oiquery->select.groups->groupFirst );
			oiquery->select.groups->groupFirst = NULL;
			MEfree( (PTR) oiquery->select.groups->pRowBuffer );
			oiquery->select.groups->pRowBuffer = NULL;
			MEfree( (PTR) oiquery->select.groups );
			oiquery->select.groups = NULL;
		}
		oiquery->select.status = FALSE;
	}
	if (count != NULL)
		*count = oiquery->rowcount;
return(err);
}

/*
** Name: PDBRelease ()	- Release query 
**
** Description:
**	(clear memory and unintialized query previously prepared)
**
** Inputs:
**	PDB_QUERY	: query pointer
**
** Outputs:
**
** Returns:	
**	GSTATUS		: 	
**				  
**				  E_OD0002_DB_ST_ERROR  
**				  E_OD0003_DB_ST_FAILURE
**				  E_OD0011_DB_ST_NOT_INITIALIZED
**				  E_OD0004_DB_ST_INVALID_HANDLE 
**				  E_OD0005_DB_ST_OUT_OF_MEMORY
**				  E_OD0006_DB_ST_UNKNOWN_ERROR
**				  GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/

GSTATUS  PDBRelease (
	PDB_QUERY qry) 
{
	GSTATUS err = GSTAT_OK;
	OI_Param *buf, *first;
	POI_QUERY oiquery;

	oiquery = (POI_QUERY) qry->SpecificInfo;
	if (oiquery != NULL) 
	{
		first = oiquery->params;
		while (first != NULL) 
		{
			buf = first;
			first = first->next;
			MEfree((PTR)buf);
		}
		oiquery->queryParm.qy_queryText = NULL;
		MEfree(oiquery->text);
		MEfree((PTR)oiquery);
		qry->SpecificInfo = NULL;
	}

return(err);
}

/*
** Name: PDBDisconnect ()	- Release query 
**
** Description:
**	(clear memory and unintialized query previously prepared)
**
** Inputs:
**	PDB_SESSION	: session pointer
**
** Outputs:
**
** Returns:	
**	GSTATUS		: 	
**				  E_DF0029_DB_SESSION_NOT_OPENED				  
**				  E_OD0002_DB_ST_ERROR  
**				  E_OD0003_DB_ST_FAILURE
**				  E_OD0011_DB_ST_NOT_INITIALIZED
**				  E_OD0004_DB_ST_INVALID_HANDLE 
**				  E_OD0005_DB_ST_OUT_OF_MEMORY
**				  E_OD0006_DB_ST_UNKNOWN_ERROR
**				  GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/

GSTATUS  
PDBDisconnect(
	PDB_SESSION session) 
{
	GSTATUS err = GSTAT_OK;
    IIAPI_DISCONNPARM disconnParm;
    POI_SESSION oisession;

	G_ASSERT(!session->SpecificInfo, E_DF0029_DB_SESSION_NOT_OPENED);
	oisession = (POI_SESSION) session->SpecificInfo;

	err = PDBRollback(session);
	if (err == GSTAT_OK) 
	{
		disconnParm.dc_genParm.gp_callback = NULL;
		disconnParm.dc_genParm.gp_closure = NULL;
		disconnParm.dc_connHandle = oisession->connHandle;
    
		IIapi_disconnect( &disconnParm );
    
		err = OI_GetResult( &disconnParm.dc_genParm, oisession->timeout);
		oisession->connHandle = NULL;
		oisession->tranHandle = NULL;

		MEfree((PTR) oisession);
		session->SpecificInfo = NULL;
	}
return(err);
}

/*
** Name: PDBDriverName ()	- 
**
** Description:
**
** Inputs:
**
** Outputs:
**	char**	: name
**
** Returns:	
**	GSTATUS		: 	
**				  GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/
GSTATUS
PDBDriverName(
	char*	*name) 
{
	*name = driverName;
return(GSTAT_OK);
}

/*
** Name: PDBucs2_to_utf8
**
** Description:
**      Function to convert UCS2 data to UTF8.
**
** Inputs:
**      sStart     	Address of pointer to the UCS2 data.
**      sEnd       	Pointer to the end of the UCS2 data.
**      targetStart     Address of pointer to the UTF8 target buffer.
**      targetend       Pointer to end of the UTF8 target buffer.
**
** Outputs:
**      targetStart     Pointer to the UTF8 target buffer.
**
** Returns:	
**      err             GSTAT_OK
**                      E_DF0040_DB_INVALID_TYPE
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**      28-Jun-2001 (fanra01)
**          Created based on the covert routine in nvarchar.c udadts.
**      20-Jul-2001 (fanra01)
**          Removed readonly definitions.
**	02-Dec-2002 (jenjo02)
**	    Check for unaligned UCS2 source, copy to temp memory
**	    to ensure UCS2 alignment.
*/
static GSTATUS
PDBucs2_to_utf8( ucs2** sStart, ucs2* sEnd,
    utf8** targetStart, utf8* targetEnd )
{
    GSTATUS         err = GSTAT_OK;
    STATUS	    result = OK;
    ucs2*           source;
    ucs2*           sourceEnd;
    utf8*           target = *targetStart;
    ucs4            ch, ch2;
    u_i2            bytesToWrite;
    ucs4            byteMask = 0xBF;
    ucs4            byteMark = 0x80;
    i4              halfShift = 10;
    utf8            firstByteMark[7] = { 0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC };

    ucs4            halfBase = 0x0010000UL;
    ucs4            halfMask = 0x3FFUL;
    ucs4            kReplacementCharacter = 0x0000FFFDUL;
    ucs4            kMaximumUCS2 = 0x0000FFFFUL;
    ucs4            kMaximumUCS4 = 0x7FFFFFFFUL;
    ucs4            kSurrogateHighStart = 0xD800UL;
    ucs4            kSurrogateHighEnd = 0xDBFFUL;
    ucs4            kSurrogateLowStart = 0xDC00UL;
    ucs4            kSurrogateLowEnd = 0xDFFFUL;

    ucs2	    aSource[1024];
    ucs2*	    tSource = (ucs2*)NULL;

    /* Source ucs2-aligned? */
    if ( ME_ALIGN_MACRO((PTR)*sStart, sizeof(ucs2)) == (PTR)*sStart )
    {
	source = *sStart;
	sourceEnd = sEnd;
    }
    else
    {
	i4	sSize = (PTR)sEnd - (PTR)*sStart;
	
	/* Get temp memory iff stack variable is too small */
	if ( sSize <= sizeof(aSource) )
	    source = aSource;
	else if ( err = G_ME_REQ_MEM(0, tSource, ucs2, sSize) )
	    return(err);
	else
	    source = tSource;

	sourceEnd = (ucs2*)((PTR)source + sSize);

	/* Copy unaligned source */
	MEcopy((PTR)*sStart, sSize, (PTR)source);
    }

    while (source < sourceEnd)
    {
        bytesToWrite = 0;
        ch = *source++;
        if (ch >= kSurrogateHighStart && ch <= kSurrogateHighEnd
            && source < sourceEnd)
        {
            ch2 = *source;
            if (ch2 >= kSurrogateLowStart && ch2 <= kSurrogateLowEnd)
            {
                ch = ((ch - kSurrogateHighStart) << halfShift)
                    + (ch2 - kSurrogateLowStart) + halfBase;
                ++source;
            }
        }
        if (ch < 0x80)
              bytesToWrite = 1;
        else if (ch < 0x800)
              bytesToWrite = 2;
        else if (ch < 0x10000)
              bytesToWrite = 3;
        else if (ch < 0x200000)
              bytesToWrite = 4;
        else if (ch < 0x4000000)
              bytesToWrite = 5;
        else if (ch <= kMaximumUCS4)
              bytesToWrite = 6;
        else
        {
            bytesToWrite = 2;
            ch = kReplacementCharacter;
        }
                
        target += bytesToWrite;
        if (target > targetEnd)
        {
            target -= bytesToWrite;
            result = E_DF0040_DB_INVALID_TYPE;
            break;
        }
        switch (bytesToWrite)   /* note: code falls through cases! */
        {
            case 6:
                *--target = (ch | byteMark) & byteMask; ch >>= 6;
            case 5:
                *--target = (ch | byteMark) & byteMask; ch >>= 6;
            case 4:
                *--target = (ch | byteMark) & byteMask; ch >>= 6;
            case 3:
                *--target = (ch | byteMark) & byteMask; ch >>= 6;
            case 2:
                *--target = (ch | byteMark) & byteMask; ch >>= 6;
            case 1:
                *--target =  ch | firstByteMark[bytesToWrite];
        }
        target += bytesToWrite;
    }

    /* Free aligned source memory, if gotten */
    if ( tSource )
	MEfree((PTR)tSource);

    *targetStart = target;
    if (result != OK)
    {
	err = DDFStatusAlloc( result );
    }
    return(err);
}
