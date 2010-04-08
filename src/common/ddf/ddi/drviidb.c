/******************************************************************************
**
** Copyright (c) 2004 Ingres Corporation
**
******************************************************************************/

#include <dbmethod.h>
#include <drviiapi.h>

/**
**
**  Name: drviidb.c - Data Dictionary Services Facility Ingres Driver 
**					  for Data Dictionary function
**
**  Description:
**		provide Data Dictionary functions like performing standard queries
**		used functions in the drviiapi.c files
**
**  History:    
**	5-Feb-98 (marol01)
**	    created
**	29-Jul-1998 (fanra01)
**	    Cleaned up compiler warning.
**	11-Aug-1998 (fanra01)
**	    Update ambiguous defines.
**      06-Aug-1999 (fanra01)
**          Changed all nat, u_nat and longnat to i4, u_i4 and i4 respectively
**          for building in the 3.0 codeline.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      16-Mar-2001 (fanra01)
**          Bug 104298
**          Add default values for the additional parameters now passed to
**          PDBPrepare.
**/

/*
** Name: PDBRepInsert()	- Perform ingres insert for Data Dictionary API
**
** Description:
**
** Inputs:
**	char *		: table name
**	PDB_PROPERTY: properties list
**
** Outputs:
**	PDB_QUERY	: Initialized query
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
**      16-Mar-2001 (fanra01)
**          Add default values for the additional parameters now passed to
**          PDBPrepare.
*/
GSTATUS
PDBRepInsert (
    char *name,
    PDB_PROPERTY properties,
    PDB_QUERY query)
{
    GSTATUS        err = GSTAT_OK;
    char        stmt[2000];
    PDB_PROPERTY prop = NULL;
    u_i4        arg = 0, count = 0;

    STprintf(stmt, "insert into %s (", name);
    prop = properties;
    arg = 0;
    while (prop != NULL)
    {
        if (arg++ > 0)
            STcat(stmt, ",");
        STcat(stmt, prop->name);
        prop = prop->next;
    }
    STcat(stmt, ") values (");
    prop = properties;
    count = strlen(stmt);
    arg = 0;
    while (err == GSTAT_OK && prop)
    {
        if (arg++ > 0)
            stmt[count++]= ',';
        stmt[count++]= '%';
        stmt[count++]= prop->type;
        prop = prop->next;
    }
    stmt[count++] =')';
    stmt[count] =EOS;
    if (err == GSTAT_OK)
        err = PDBPrepare( query, stmt, DEFAULT_ROW_SET, DEFAULT_SET_SIZE );
    return(err);
}

/*
** Name: PDBRepUpdate()	- Perform ingres update for Data Dictionary API
**
** Description:
**
** Inputs:
**	char *		: table name
**	PDB_PROPERTY: properties list
**
** Outputs:
**	PDB_QUERY	: Initialized query
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
**      16-Mar-2001 (fanra01)
**          Add default values for the additional parameters now passed to
**          PDBPrepare.
*/
GSTATUS
PDBRepUpdate (
    char *name,
    PDB_PROPERTY properties,
    PDB_QUERY query)
{
    GSTATUS        err = GSTAT_OK;
    char        stmt[2000];
    PDB_PROPERTY prop = NULL;
    u_i4        arg = 0, count = 0;

    STprintf(stmt, "update %s set ", name);
    count = strlen(stmt);
    prop = properties;
    arg = 0;
    while (prop != NULL)
    {
        if (prop->key == FALSE)
        {
            if (arg++ > 0)
                stmt[count++]= ',';
            stmt[count] = EOS;
            STcat(stmt, prop->name);
            count+= strlen(prop->name);
            stmt[count++]= '=';
            stmt[count++]= '%';
            stmt[count++]= prop->type;
        }
        prop = prop->next;
    }
    stmt[count] = EOS;
    STcat(stmt, " where ");
    count = strlen(stmt);
    prop = properties;
    arg = 0;
    while (prop != NULL)
    {
        if (prop->key == TRUE)
        {
            if (arg++ > 0)
            {
                stmt[count++]= ' ';
                stmt[count++]= 'a';
                stmt[count++]= 'n';
                stmt[count++]= 'd';
                stmt[count++]= ' ';
                stmt[count]= EOS;
            }
            stmt[count]= EOS;
            STcat(stmt, prop->name);
            count+= strlen(prop->name);
            stmt[count++]= '=';
            stmt[count++]= '%';
            stmt[count++]= prop->type;
        }
        prop = prop->next;
    }
    stmt[count] =EOS;
    if (err == GSTAT_OK)
        err = PDBPrepare( query, stmt, DEFAULT_ROW_SET, DEFAULT_SET_SIZE );
    return(err);
}

/*
** Name: PDBRepDelete()	- Perform ingres delete for Data Dictionary API
**
** Description:
**
** Inputs:
**	char *		: table name
**	PDB_PROPERTY: properties list
**
** Outputs:
**	PDB_QUERY	: Initialized query
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
**      16-Mar-2001 (fanra01)
**          Add default values for the additional parameters now passed to
**          PDBPrepare.
*/
GSTATUS
PDBRepDelete (
    char *name,
    PDB_PROPERTY properties,
    PDB_QUERY query)
{
    GSTATUS        err = GSTAT_OK;
    char        stmt[2000];
    PDB_PROPERTY prop = NULL;
    u_i4        arg = 0, count = 0;

    STprintf(stmt, "delete from %s where ", name);
    count = strlen(stmt);
    prop = properties;
    arg = 0;
    while (prop != NULL)
    {
        if (prop->key == TRUE)
        {
            if (arg++ > 0)
            {
                stmt[count++]= ' ';
                stmt[count++]= 'a';
                stmt[count++]= 'n';
                stmt[count++]= 'd';
                stmt[count++]= ' ';
                stmt[count]= EOS;
            }
            STcat(stmt, prop->name);
            count+= strlen(prop->name);
            stmt[count++]= '=';
            stmt[count++]= '%';
            stmt[count++]= prop->type;
        }
        prop = prop->next;
    }
    stmt[count] =EOS;
    if (err == GSTAT_OK)
        err = PDBPrepare( query, stmt, DEFAULT_ROW_SET, DEFAULT_SET_SIZE );
    return(err);
}


/*
** Name: PDBRepSelect()	- Perform ingres select_all for Data Dictionary API
**
** Description:
**
** Inputs:
**	char *		: table name
**	PDB_PROPERTY: properties list
**
** Outputs:
**	PDB_QUERY	: Initialized query
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
**      16-Mar-2001 (fanra01)
**          Add default values for the additional parameters now passed to
**          PDBPrepare.
*/
GSTATUS
PDBRepSelect (
    char *name,
    PDB_PROPERTY properties,
    PDB_QUERY query)
{
    GSTATUS        err = GSTAT_OK;
    char        stmt[2000];
    PDB_PROPERTY prop = NULL;
    u_i4        arg = 0, count = 0;

    STprintf(stmt, "select ");
    prop = properties;
    arg = 0;
    while (prop != NULL)
    {
        if (arg++ > 0)
            STcat(stmt, ",");
        STcat(stmt, prop->name);
        prop = prop->next;
    }
    STcat(stmt, " from ");
    STcat(stmt, name);
    STcat(stmt, " order by ");

    prop = properties;
    arg = 0;
    while (prop != NULL)
    {
        if (prop->key == TRUE)
        {
            if (arg++ > 0) STcat(stmt, ",");
            STcat(stmt, prop->name);
        }
        prop = prop->next;
    }
    if (err == GSTAT_OK)
        err = PDBPrepare( query, stmt, DEFAULT_ROW_SET, DEFAULT_SET_SIZE );
    return(err);
}

/*
** Name: PDBRepSelectKey()	- Perform ingres select from key for DD API
**
** Description:
**
** Inputs:
**	char *		: table name
**	PDB_PROPERTY: properties list
**
** Outputs:
**	PDB_QUERY	: Initialized query
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
**      16-Mar-2001 (fanra01)
**          Add default values for the additional parameters now passed to
**          PDBPrepare.
*/
GSTATUS
PDBRepKeySelect (
    char *name,
    PDB_PROPERTY properties,
    PDB_QUERY query)
{
    GSTATUS        err = GSTAT_OK;
    char        stmt[2000];
    PDB_PROPERTY prop = NULL;
    u_i4        arg = 0, count = 0;

    STprintf(stmt, "select ");
    prop = properties;
    arg = 0;
    while (prop != NULL)
    {
        if (!prop->key)
        {
            if (arg++ > 0)
                STcat(stmt, ",");
            STcat(stmt, prop->name);
        }
        prop = prop->next;
    }
    STcat(stmt, " from ");
    STcat(stmt, name);
    STcat(stmt, " where ");
    count = strlen(stmt);
    prop = properties;
    arg = 0;
    while (prop != NULL)
    {
        if (prop->key == TRUE)
        {
            if (arg++ > 0)
            {
                stmt[count++]= ' ';
                stmt[count++]= 'a';
                stmt[count++]= 'n';
                stmt[count++]= 'd';
                stmt[count++]= ' ';
                stmt[count]= EOS;
            }
            STcat(stmt, prop->name);
            count+= strlen(prop->name);
            stmt[count++]= '=';
            stmt[count++]= ' ';
            stmt[count++]= '%';
            stmt[count++]= prop->type;
        }
        prop = prop->next;
    }
    stmt[count] =EOS;
    if (err == GSTAT_OK)
        err = PDBPrepare( query, stmt, DEFAULT_ROW_SET, DEFAULT_SET_SIZE );
    return(err);
}

/*
** Name: PDBRepObjects()	- Open query select all Data Dictionary objects
**
** Description:
**
** Inputs:
**	PDB_SESSION	: session
**	u_nat		: module number
**
** Outputs:
**	PDB_QUERY	: Initialized query
**
** Returns:
**	GSTATUS		: 
**					E_OD0002_OI_ST_ERROR   :
**					E_OD0003_OI_ST_FAILURE :
**					E_OD0011_OI_ST_NOT_INITIALIZED :
**					E_OD0004_OI_ST_INVALID_HANDLE  :
**					E_OD0005_OI_ST_OUT_OF_MEMORY
**					E_OD0006_OI_ST_UNKNOWN_ERROR
**					GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**      16-Mar-2001 (fanra01)
**          Add default values for the additional parameters now passed to
**          PDBPrepare.
*/
GSTATUS
PDBRepObjects (
    PDB_SESSION session,
    u_i4 module,
    PDB_QUERY query)
{
    GSTATUS err = GSTAT_OK;
    char stmt[250];
    STprintf(
        stmt,
        "select id, name from RepDescObject "
        "where module_id = %d order by id desc",
        module);

    err = PDBPrepare( query, stmt, DEFAULT_ROW_SET, DEFAULT_SET_SIZE );
    if (err == GSTAT_OK)
        err = PDBExecute (session, query);
    return(err);
}

/*
** Name: PDBRepProperties()	- Open query select all Data Dictionary properties
**
** Description:
**
** Inputs:
**	PDB_SESSION	: session
**	u_nat		: module number
**
** Outputs:
**	PDB_QUERY	: Initialized query
**
** Returns:
**	GSTATUS		: 
**					E_OD0002_OI_ST_ERROR   :
**					E_OD0003_OI_ST_FAILURE :
**					E_OD0011_OI_ST_NOT_INITIALIZED :
**					E_OD0004_OI_ST_INVALID_HANDLE  :
**					E_OD0005_OI_ST_OUT_OF_MEMORY
**					E_OD0006_OI_ST_UNKNOWN_ERROR
**					GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**      23-Nov-1998 (fanra01)
**          Add isunique to select statement.
**      16-Mar-2001 (fanra01)
**          Add default values for the additional parameters now passed to
**          PDBPrepare.
*/
GSTATUS
PDBRepProperties (
    PDB_SESSION session,
    u_i4 module,
    PDB_QUERY query)
{
    GSTATUS err = GSTAT_OK;
    char stmt[250];
    STprintf(
        stmt,
        "select id, name, object, isKey, type, length, isnull, isunique "
        "from RepDescProp "
        "where module_id = %d "
        "order by object, id desc",
        module);

    err = PDBPrepare( query, stmt, DEFAULT_ROW_SET, DEFAULT_SET_SIZE );
    if (err == GSTAT_OK)
        err = PDBExecute (session, query);
    return(err);
}

/*
** Name: PDBRepStatements()	- Open query select all Data Dictionary statement
**
** Description:
**
** Inputs:
**	PDB_SESSION	: session
**	u_nat		: module number
**
** Outputs:
**	PDB_QUERY	: Initialized query
**
** Returns:
**	GSTATUS		: 
**					E_OD0002_OI_ST_ERROR   :
**					E_OD0003_OI_ST_FAILURE :
**					E_OD0011_OI_ST_NOT_INITIALIZED :
**					E_OD0004_OI_ST_INVALID_HANDLE  :
**					E_OD0005_OI_ST_OUT_OF_MEMORY
**					E_OD0006_OI_ST_UNKNOWN_ERROR
**					GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**      16-Mar-2001 (fanra01)
**          Add default values for the additional parameters now passed to
**          PDBPrepare.
*/
GSTATUS
PDBRepStatements (
    PDB_SESSION session,
    u_i4 module,
    PDB_QUERY query)
{
    GSTATUS err = GSTAT_OK;
    char stmt[250];
    STprintf(
        stmt,
        "select id, statement "
        "from RepDescExecute where module_id = %d order by id desc",
        module);

    err = PDBPrepare( query, stmt, DEFAULT_ROW_SET, DEFAULT_SET_SIZE );
    if (err == GSTAT_OK)
        err = PDBExecute (session, query);
    return(err);
}

/*
** Name: PDBGenerate()	- Generate Data Dictionary environment
**
** Description:
**	Create and initialize a table and modify the structure 
**  for each object declared in the database 
**
** Inputs:
**	PDB_SESSION	: session
**	u_nat		: module number
**
** Outputs:
**	PDB_QUERY	: Initialized query
**
** Returns:
**	GSTATUS		: 
**					E_OD0002_OI_ST_ERROR   :
**					E_OD0003_OI_ST_FAILURE :
**					E_OD0011_OI_ST_NOT_INITIALIZED :
**					E_OD0004_OI_ST_INVALID_HANDLE  :
**					E_OD0005_OI_ST_OUT_OF_MEMORY
**					E_OD0006_OI_ST_UNKNOWN_ERROR
**					GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**	29-Jul-1998 (fanra01)
**	    Clean compiler warning by changing type of variable existing.
**      23-Nov-1998 (fanra01)
**          Add handling of isunique flag for column contraint.
**      16-Mar-2001 (fanra01)
**          Add default values for the additional parameters now passed to
**          PDBPrepare.
*/

GSTATUS 
PDBGenerate (
    PDB_SESSION session,
    char *name,
    PDB_PROPERTY properties)
{
    GSTATUS         err = GSTAT_OK;
    DB_QUERY        query;
    char            stmt[2000];
    char            buf[50];
    PDB_PROPERTY    prop;
    bool            existing;
    u_i4           arg = 0;

    err = PDBPrepare( &query,
        "select relid from iirelation where relid = %s",
        DEFAULT_ROW_SET, DEFAULT_SET_SIZE );

    if (err == GSTAT_OK)
    {
        err = PDBParams (&query, "%s", name);
        if (err == GSTAT_OK)
        {
            err = PDBExecute (session, &query);
            if (err == GSTAT_OK)
            {
                err = PDBNext(&query, &existing);
                if (err == GSTAT_OK)
                    err = PDBClose(&query, NULL);
            }
        }
        PDBRelease(&query);
    }
    if (err == GSTAT_OK && !existing)
    {
        STprintf(stmt, "create table %s (", name);
        prop = properties;
        while (err == GSTAT_OK && prop != NULL)
        {
            if (arg++ > 0) STcat(stmt, ",");
            STcat(stmt, prop->name);
            switch (prop->type)
            {
            case DDF_DB_INT :
                STcat(stmt, " Integer");
                break;
            case DDF_DB_FLOAT :
                STcat(stmt, " Float");
                break;
            case DDF_DB_TEXT :
                STprintf(buf, " varchar(%d)", prop->length);
                STcat(stmt, buf);
                break;
            case DDF_DB_DATE :
                STcat(stmt, " Date");
                break;
            case DDF_DB_BLOB :
                STcat(stmt, " long byte");
                break;
            default :
                err = DDFStatusAlloc( E_DF0016_DDG_UNSUPPORTED_TYPE );
            }
            if (err == GSTAT_OK)
            {
                if (prop->isunique == TRUE)
                {
                    STcat (stmt, " unique not null");
                }
                else
                {
                    if (prop->isnull == FALSE)
                        STcat(stmt, " not null");
                }
            }
            prop = prop->next;
        }
        if (err == GSTAT_OK)
        {
            STcat(stmt, ")");
            err = PDBPrepare( &query, stmt, DEFAULT_ROW_SET, DEFAULT_SET_SIZE );
            if (err == GSTAT_OK)
                err = PDBExecute (session, &query);
            if (err == GSTAT_OK)
                err = PDBClose (&query, NULL);
            CLEAR_STATUS( PDBRelease (&query));
        }

        if (err == GSTAT_OK)
        {
            STprintf(stmt, "modify %s to btree unique on ", name);
            prop = properties;
            arg = 0;
            while (prop != NULL)
            {
                if (prop->key == TRUE)
                {
                    if (arg++ > 0)
                        STcat(stmt, ",");
                    STcat(stmt, prop->name);
                }
                prop = prop->next;
            }
            err = PDBPrepare( &query, stmt, DEFAULT_ROW_SET, DEFAULT_SET_SIZE );
            if (err == GSTAT_OK)
                err = PDBExecute (session, &query);
            if (err == GSTAT_OK)
                err = PDBClose (&query, NULL);
            CLEAR_STATUS( PDBRelease (&query));
        }
    }
return(err);
}
