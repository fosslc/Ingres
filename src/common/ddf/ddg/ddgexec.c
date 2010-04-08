/******************************************************************************
**
** Copyright (c) 2004 Ingres Corporation
**
******************************************************************************/

#include <ddgexec.h>
#include <erddf.h>

/**
**
**  Name: ddgexec.c - Data Dictionary Services Facility Execution
**
**  Description:
**		Provide a set of funtions to execute transparent query to the Data 
**		Dictionary Manage concurrent access
**
**  History:    
**	5-Feb-98 (marol01)
**	    created
**      28-May-98 (marol01)
**          Modified integer conversion to longnat.
**      30-Nov-1998 (fanra01)
**          Remove ifdef DEBUG from DDG_CHECK_PROPERTY as string checks
**          require the parameter string to be incremented causing
**          E_DF0017_DDG_BAD_PARAM error.
**      06-Aug-1999 (fanra01)
**          Change nat to i4, u_nat to u_i4, longnat to i4, u_longnat to u_i4
**	16-mar-1999 (muhpa01)
**	    Needed modified version of DDG_SET_PROPERTY macro for HP as
**	    the address-of operator cannot be used on va_arg for longnat
**	    or float types - produces compile error.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      16-Mar-2001 (fanra01)
**          Bug 104298
**          Add rowset and setsize parameters to the DBPrepare call.
**	18-Sep-2002 (hanje04)
**	   Itanium Linux (i64_lnx) to use HP version of DDG_SET_PROPERTY
**	   as GCC also complains about & operator being used on va_arg().
**	11-oct-2002 (somsa01)
**	    Changed DDG_SET_PROPERTY so that for int and float types, a
**	    pointer is retrieved via va_arg().
**/

/* internal use */
#define DDG_CHECK_PROPERTY(X, Y) \
		if (*Y++ != '%') \
			err = DDFStatusAlloc (E_DF0017_DDG_BAD_PARAM);\
		else if (X != *Y++) \
			err = DDFStatusAlloc (E_DF0017_DDG_BAD_PARAM);

#define DDG_SET_PROPERTY(X, Y) switch(X) {\
		case DDF_DB_INT  :	Y =	(DB_VALUE) va_arg( ap, i4*);	break;\
		case DDF_DB_FLOAT :	Y =	(DB_VALUE) va_arg( ap, float*);	break;\
		case DDF_DB_TEXT :	Y =	(DB_VALUE) va_arg( ap, char*);	break;\
		case DDF_DB_DATE :	Y =	(DB_VALUE) va_arg( ap, char*);	break;\
		case DDF_DB_BLOB :	Y =	(DB_VALUE) va_arg( ap, char*);	break;\
		default : err = DDFStatusAlloc (E_DF0016_DDG_UNSUPPORTED_TYPE);\
		}

#define DDG_GET_PROPERTY(A, B, C, D) \
		switch(A) {\
		case DDF_DB_INT  : { i4  **tmp = va_arg( ap, i4**); \
						 err = (err, DBInteger(B)(C, D, tmp));	break;	}\
		case DDF_DB_FLOAT: { float **tmp = va_arg( ap, float**); \
						 err = (err, DBFloat(B)(C, D, tmp));	break;	}\
		case DDF_DB_TEXT : { char **tmp = va_arg( ap, char **); \
						 err = (err, DBText(B)(C, D, tmp));	break;	}\
		case DDF_DB_DATE : { char **tmp = va_arg( ap, char **); \
						 err = (err, DBText(B)(C, D, tmp));	break;	}\
		case DDF_DB_BLOB : { LOCATION *loc = va_arg( ap, LOCATION*); \
						 err = DBBlob(B)(C, D, loc); break; }\
		default : err = DDFStatusAlloc (E_DF0016_DDG_UNSUPPORTED_TYPE);\
		}

/*
** Name: DDGPrepare()	- Standard Query Preparation
**
** Description:
**
** Inputs:
**	PDDG_DESCR		: pointer on a Data Dictionary description
**  PDDG_SESSION	: pointer on a Data Dictionary session
**
** Outputs:
**
** Returns:
**	GSTATUS		: 
**					E_OG0001_OUT_OF_MEMORY
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
**          Add default rowset parameters to the DBPrepare call.
*/
GSTATUS
DDGPrepare(
    PDDG_DESCR descr,
    PDDG_SESSION session)
{
    GSTATUS err = GSTAT_OK;
    PDDG_OBJECT tmp1;
    PDDG_STATEMENT tmp2;
    u_i4 i;
    u_i2 tag;

    tag = session->memory_tag = MEgettag();

    if (descr->objects != NULL)
    {
        err = G_ME_REQ_MEM(
                tag,
                session->dbinsert,
                DB_QUERY,
                descr->object_max+1);

        if (err == GSTAT_OK)
            err = G_ME_REQ_MEM(
                    tag,
                    session->dbupdate,
                    DB_QUERY,
                    descr->object_max+1);

        if (err == GSTAT_OK)
            err = G_ME_REQ_MEM(
                    tag,
                    session->dbdelete,
                    DB_QUERY,
                    descr->object_max+1);

        if (err == GSTAT_OK)
            err = G_ME_REQ_MEM(
                    tag,
                    session->dbselect,
                    DB_QUERY,
                    descr->object_max+1);

        if (err == GSTAT_OK)
            err = G_ME_REQ_MEM(
                    tag,
                    session->dbkeyselect,
                    DB_QUERY,
                    descr->object_max+1);

        if (err == GSTAT_OK)
            err = G_ME_REQ_MEM(
                    tag,
                    session->dbexecute,
                    DB_QUERY,
                    descr->stmt_max+1);


        for (i = 0; err == GSTAT_OK && i <= descr->object_max; i++)
        {
            tmp1 = &(descr->objects[i]);
            if (tmp1->name != NULL && tmp1->name[0] != EOS)
            {
                err = DBRepInsert(descr->dbtype)(
                        tmp1->name,
                        tmp1->properties,
                        &session->dbinsert[i]);

                if (err == GSTAT_OK)
                    err = DBRepUpdate(descr->dbtype)(
                        tmp1->name,
                        tmp1->properties,
                        &session->dbupdate[i]);

                if (err == GSTAT_OK)
                    err = DBRepDelete(descr->dbtype)(
                        tmp1->name,
                        tmp1->properties,
                        &session->dbdelete[i]);

                if (err == GSTAT_OK)
                    err = DBRepSelect(descr->dbtype)(
                        tmp1->name,
                        tmp1->properties,
                        &session->dbselect[i]);

                if (err == GSTAT_OK)
                    err = DBRepKeySelect(descr->dbtype)(
                        tmp1->name,
                        tmp1->properties,
                        &session->dbkeyselect[i]);
            }
        }
    }

    if (descr->statements != NULL) {
        for (i = 0; err == GSTAT_OK && i <= descr->stmt_max; i++)
        {
            tmp2 = &(descr->statements[i]);
            if (tmp2->stmt != NULL && tmp2->stmt[0] != EOS)
            {
                err = DBPrepare(descr->dbtype)(
                    &session->dbexecute[i],
                    descr->statements[i].stmt, DEFAULT_ROW_SET,
                    DEFAULT_SET_SIZE );
            }
        }
    }
    return(err);
}

/*
** Name: DDGRelease()	- Standard Query Release
**
** Description:
**
** Inputs:
**  PDDG_SESSION	: pointer on a Data Dictionary session
**
** Outputs:
**
** Returns:
**	GSTATUS		: 
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
DDGRelease(
	PDDG_SESSION session) 
{
	GSTATUS err = GSTAT_OK;
	u_i4 i, dbtype;
	dbtype = session->descr->dbtype;

	if (session->descr->objects != NULL) 
	{
		for (i = 0; err == GSTAT_OK && i <= session->descr->object_max; i++)
		{
			DBRelease(dbtype)(&session->dbinsert[i]);
			DBRelease(dbtype)(&session->dbupdate[i]);
			DBRelease(dbtype)(&session->dbdelete[i]);
			DBRelease(dbtype)(&session->dbselect[i]);
			DBRelease(dbtype)(&session->dbkeyselect[i]);
		}
	}

	if (session->descr->statements != NULL) 
	{
		for (i = 0; err == GSTAT_OK && i <= session->descr->stmt_max; i++)
			DBRelease(dbtype)(&session->dbexecute[i]);
	}
	MEfreetag(session->memory_tag);
return(err);
}

/*
** Name: DDGConnect()	- Open a connection to the Data Dictionary system
**
** Description:
**
** Inputs:
**	PDDG_DESCR		: pointer on a Data Dictionary description
**  PDDG_SESSION	: pointer on a Data Dictionary session
**
** Outputs:
**
** Returns:
**	GSTATUS		: 
**					E_OG0001_OUT_OF_MEMORY
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
*/
	
GSTATUS  
DDGConnect(
	PDDG_DESCR descr, 
	char*	user_name,
	char*	password,
	PDDG_SESSION session) 
{
	GSTATUS err = GSTAT_OK;

	err = DDFSemCreate(&(session->sem), "Data Dictionary");
	if (err == GSTAT_OK) 
	{
		err = DDFSemOpen(&(session->sem), TRUE);
		if (err == GSTAT_OK) 
		{
			session->descr = descr;
			err = DBCachedConnect	(
								descr->dbtype,
								descr->node, 
								descr->dbname, 
								descr->svrClass,
								NULL,
								user_name, 
								password,
								0,
								&(session->cached_session));

			if (err == GSTAT_OK) 
			{
				err = DDGPrepare(descr, session);
			}
		}
		CLEAR_STATUS(DDFSemClose(&(session->sem)));
	}
return(err);
}

/*
** Name: DDGCommit()	- Commit transaction into the Data Dictionary system
**
** Description:
**
** Inputs:
**  PDDG_SESSION	: pointer on a Data Dictionary session
**
** Outputs:
**
** Returns:
**	GSTATUS		: 
**					E_OG0001_OUT_OF_MEMORY
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
*/

GSTATUS  
DDGCommit(
	PDDG_SESSION session) 
{
	GSTATUS err = GSTAT_OK;
	err = DDFSemOpen(&(session->sem), TRUE);
	if (err == GSTAT_OK)
	{
		err = DBCommit(session->descr->dbtype)(&(session->cached_session->session));
		CLEAR_STATUS(DDFSemClose(&(session->sem)));
	}
return err;
}

/*
** Name: DDGRollback()	- Rollback transaction into the Data Dictionary system
**
** Description:
**
** Inputs:
**  PDDG_SESSION	: pointer on a Data Dictionary session
**
** Outputs:
**
** Returns:
**	GSTATUS		: 
**					E_OG0001_OUT_OF_MEMORY
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
*/

GSTATUS  
DDGRollback(
	PDDG_SESSION session) 
{
	GSTATUS err = GSTAT_OK;
	err = DDFSemOpen(&(session->sem), TRUE);
	if (err == GSTAT_OK)
	{
		err = DBRollback(session->descr->dbtype)(&(session->cached_session->session));
		CLEAR_STATUS(DDFSemClose(&(session->sem)));
	}
return err;
}


/*
** Name: DDGDisconnect()	- Close Data Dictionary connection
**
** Description:
**
** Inputs:
**  PDDG_SESSION	: pointer on a Data Dictionary session
**
** Outputs:
**
** Returns:
**	GSTATUS		: 
**					E_OG0001_OUT_OF_MEMORY
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
*/

GSTATUS  
DDGDisconnect(
	PDDG_SESSION session) 
{
	GSTATUS err = GSTAT_OK;
	u_i4 i = 0;

	if (session->descr != NULL)
	{
		err = DDFSemOpen(&(session->sem), TRUE);
		if (err == GSTAT_OK) 
		{
			CLEAR_STATUS(DBRollback(session->descr->dbtype)(&(session->cached_session->session)));
			CLEAR_STATUS(DDGRelease(session));
			CLEAR_STATUS(DBCachedDisconnect (&session->cached_session));
			CLEAR_STATUS(DDFSemClose(&(session->sem)));
		}
		CLEAR_STATUS( DDFSemDestroy(&(session->sem)));
	}
return(err);
}

/*
** Name: DDGInsert()	- Insert a new instance into the Data Dictionary system
**
** Description:
**
** Inputs:
**  PDDG_SESSION	: pointer on a Data Dictionary session
**	DDG_OBJECT_NUMBER : Data Dictionary object number
**	char*			: parameter definition (used to check parameters matching)
**	...			: list of variables
**
** Outputs:
**
** Returns:
**	GSTATUS		: 
**					E_OG0001_OUT_OF_MEMORY
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
**	8-Sep-98 (marol01)
**	    change err = DBClose(...) by CLEAR_STATUS(DBClose(...))
**			because if an error occured from the DBExecute function
**			that db access is never closed and the database session is incoherent.
*/

GSTATUS  
DDGInsert(
	PDDG_SESSION session, 
	DDG_OBJECT_NUMBER number, 
	char *paramDef, 
	...) 
{
	GSTATUS err = GSTAT_OK;
	PDB_VALUE values;
	PDB_PROPERTY tmp = session->descr->objects[number].properties;
	va_list	ap;

	err = DDFSemOpen(&(session->sem), TRUE);
	if (err == GSTAT_OK) 
	{
		err = G_ME_REQ_MEM(
				0, 
				values, 
				DB_VALUE, 
				session->descr->objects[number].prop_max);

		if (err == GSTAT_OK) 
		{
			u_i4 i = 0;
			va_start( ap, paramDef );
			while (err == GSTAT_OK && tmp != NULL) 
			{
				DDG_CHECK_PROPERTY (tmp->type, paramDef);	
				if (err == GSTAT_OK) 
				{
					DDG_SET_PROPERTY (tmp->type, values[i++]);
				}
				tmp = tmp->next;
			}
			va_end( ap );
			if (err == GSTAT_OK) 
			{
				err = DBParamsFromTab(session->descr->dbtype)(
						&session->dbinsert[number], 
						values);

				if (err == GSTAT_OK) 
				{
					err = DBExecute(session->descr->dbtype)(
						&(session->cached_session->session), 
						&session->dbinsert[number]);

					CLEAR_STATUS(DBClose(session->descr->dbtype)(
								&session->dbinsert[number], NULL));	
				}
			}
			MEfree((PTR)values);
		}
		CLEAR_STATUS(DDFSemClose(&(session->sem)));
	}
return(err);
}

/*
** Name: DDGUpdate()	- Update a Data Dictionary instance
**
** Description:
**
** Inputs:
**  PDDG_SESSION	: pointer on a Data Dictionary session
**	DDG_OBJECT_NUMBER : Data Dictionary object number
**	char*			: parameter definition (used to check parameters matching)
**	...			: list of variables
**
** Outputs:
**
** Returns:
**	GSTATUS		: 
**					E_OG0001_OUT_OF_MEMORY
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
**	8-Sep-98 (marol01)
**	    change err = DBClose(...) by CLEAR_STATUS(DBClose(...))
**			because if an error occured from the DBExecute function
**			that db access is never closed and the database session is incoherent.
*/

GSTATUS  
DDGUpdate(
	PDDG_SESSION session, 
	DDG_OBJECT_NUMBER number, 
	char *paramDef, 
	...) 
{
	GSTATUS err = GSTAT_OK;
	va_list	ap;
	PDB_VALUE values;
	PDB_PROPERTY tmp;

	err = DDFSemOpen(&(session->sem), TRUE);
	if (err == GSTAT_OK) 
	{
		err = G_ME_REQ_MEM(
				0, 
				values, 
				DB_VALUE, 
				session->descr->objects[number].prop_max);

		if (err == GSTAT_OK) 
		{
			u_i4 i = 0;
			va_start( ap, paramDef );
			tmp = session->descr->objects[number].properties;
			while (err == GSTAT_OK && tmp != NULL) 
			{
				if ( tmp->key == FALSE) 
				{
					DDG_CHECK_PROPERTY (tmp->type, paramDef);	
					if (err == GSTAT_OK)
						DDG_SET_PROPERTY (tmp->type, values[i++]);
				}
				tmp = tmp->next;
			}
			tmp = session->descr->objects[number].properties;
			while (err == GSTAT_OK && tmp != NULL) 
			{
				if ( tmp->key == TRUE) 
				{
					DDG_CHECK_PROPERTY (tmp->type, paramDef);	
					if (err == GSTAT_OK)
						DDG_SET_PROPERTY (tmp->type, values[i++]);
				}
				tmp = tmp->next;
			}
			va_end( ap );
			if (err == GSTAT_OK) 
			{
				err = DBParamsFromTab(session->descr->dbtype)(
						&session->dbupdate[number], 
						values);

				if (err == GSTAT_OK) 
				{
					err = DBExecute(session->descr->dbtype)(
						&(session->cached_session->session), 
						&session->dbupdate[number]);

					CLEAR_STATUS(DBClose(session->descr->dbtype)(
								&session->dbupdate[number], NULL));	
				}
			}
			MEfree((PTR)values);
		}
		CLEAR_STATUS(DDFSemClose(&(session->sem)));
	}
return(err);
}

/*
** Name: DDGDelete()	- Delete a Data Dictionary instance
**
** Description:
**
** Inputs:
**  PDDG_SESSION	: pointer on a Data Dictionary session
**	DDG_OBJECT_NUMBER : Data Dictionary object number
**	char*			: parameter definition (used to check parameters matching)
**	...			: list of variables
**
** Outputs:
**
** Returns:
**	GSTATUS		: 
**					E_OG0001_OUT_OF_MEMORY
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
**	8-Sep-98 (marol01)
**	    change err = DBClose(...) by CLEAR_STATUS(DBClose(...))
**			because if an error occured from the DBExecute function
**			that db access is never closed and the database session is incoherent.
*/

GSTATUS  DDGDelete(
	PDDG_SESSION session, 
	DDG_OBJECT_NUMBER number, 
	char *paramDef, 
	...) 
{
	GSTATUS err = GSTAT_OK;
	va_list	ap;
	PDB_VALUE values;
	PDB_PROPERTY tmp;

	err = DDFSemOpen(&(session->sem), TRUE);
	if (err == GSTAT_OK) 
	{
		err = G_ME_REQ_MEM(
				0, 
				values, 
				DB_VALUE, 
				session->descr->objects[number].key_max);

		if (err == GSTAT_OK) 
		{
			u_i4 i = 0;
			va_start( ap, paramDef );
			tmp = session->descr->objects[number].properties;
			while (err == GSTAT_OK && tmp != NULL) 
			{
				if ( tmp->key == TRUE) 
				{
					DDG_CHECK_PROPERTY (tmp->type, paramDef);	
					if (err == GSTAT_OK)
						DDG_SET_PROPERTY (tmp->type, values[i++]);
				}
				tmp = tmp->next;
			}
			va_end( ap );
			if (err == GSTAT_OK) 
			{
				err = DBParamsFromTab(session->descr->dbtype)(
						&session->dbdelete[number], 
						values);

				if (err == GSTAT_OK) 
				{
					err = DBExecute(session->descr->dbtype)(
							&(session->cached_session->session), 
							&session->dbdelete[number]);

					CLEAR_STATUS(DBClose(session->descr->dbtype)(
								&session->dbdelete[number], NULL));	
				}
			}
			MEfree((PTR)values);
		}
		CLEAR_STATUS(DDFSemClose(&(session->sem)));
	}
return(err);
}

/*
** Name: DDGSelectAll()	- Select all instance of a specific DD object
**
** Description:
**
** Inputs:
**  PDDG_SESSION	: pointer on a Data Dictionary session
**	DDG_OBJECT_NUMBER : Data Dictionary object number
**
** Outputs:
**
** Returns:
**	GSTATUS		: 
**					E_OG0001_OUT_OF_MEMORY
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
*/

GSTATUS  
DDGSelectAll(
	PDDG_SESSION session, 
	DDG_OBJECT_NUMBER number) 
{
	GSTATUS err = GSTAT_OK;
	PDB_PROPERTY tmp = session->descr->objects[number].properties;
	
	session->current = &session->dbselect[number];
	err = DDFSemOpen(&(session->sem), TRUE);
	if (err == GSTAT_OK) 
	{
		err = DBExecute(session->descr->dbtype)(
				&(session->cached_session->session), 
				&session->dbselect[number]);

		if (err == GSTAT_OK)
		{
			err = DBNumberOfProperties(session->descr->dbtype)(
														session->current,
														&session->numberOfProp);
		}
	}
return(err);
}

/*
** Name: DDGKeySelect()	- Select an unique instance from its key value
**
** Description:
**
** Inputs:
**  PDDG_SESSION	: pointer on a Data Dictionary session
**	DDG_OBJECT_NUMBER : Data Dictionary object number
**	char*			: parameter definition (used to check parameters matching)
**	...			: list of variables
**
** Outputs:
**
** Returns:
**	GSTATUS		: 
**					E_OG0001_OUT_OF_MEMORY
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
*/
GSTATUS
DDGKeySelect(
	PDDG_SESSION session, 
	DDG_OBJECT_NUMBER number, 
	char *paramDef, 
	...) 
{
	GSTATUS err = GSTAT_OK;
	va_list	ap;
	PDB_VALUE values;
	PDB_PROPERTY tmp;

	err = DDFSemOpen(&(session->sem), TRUE);
	if (err == GSTAT_OK) 
	{
		err = G_ME_REQ_MEM(
				0, 
				values, 
				DB_VALUE, 
				session->descr->objects[number].key_max);

		if (err == GSTAT_OK) 
		{
			u_i4 i = 0;
			va_start( ap, paramDef );
			tmp = session->descr->objects[number].properties;
			while (err == GSTAT_OK && tmp != NULL) 
			{
				if ( tmp->key == TRUE) 
				{
					DDG_CHECK_PROPERTY (tmp->type, paramDef);	
					if (err == GSTAT_OK)
						DDG_SET_PROPERTY (tmp->type, values[i++]);
				}
				tmp = tmp->next;
			}
			if (err == GSTAT_OK) 
			{
				err = DBParamsFromTab(session->descr->dbtype)(
						&session->dbkeyselect[number], 
						values);

				if (err == GSTAT_OK) 
				{
					err = DBExecute(session->descr->dbtype)(
						&(session->cached_session->session), 
						&session->dbkeyselect[number]);

					if (err == GSTAT_OK) 
					{
						bool exist;
						err = DBNext(session->descr->dbtype)(
							&session->dbkeyselect[number], 
							&exist);

						if (err == GSTAT_OK && exist) 
						{
							u_i4 prop = 1;
							u_i4	max;
							tmp = session->descr->objects[number].properties;
							err = DBNumberOfProperties(session->descr->dbtype)(
																		&session->dbkeyselect[number],
																		&max);

							while (err == GSTAT_OK && prop <= max) 
							{
								if (err == GSTAT_OK && tmp->key == FALSE) 
								{
								DDG_CHECK_PROPERTY (tmp->type, paramDef);
								DDG_GET_PROPERTY (
									tmp->type, 
									session->descr->dbtype, 
									&session->dbkeyselect[number], 
									prop);

									prop++;
								}
								tmp = tmp->next;
							}

							if (err == GSTAT_OK && paramDef[0] != EOS)
								err = DDFStatusAlloc (E_DF0017_DDG_BAD_PARAM);

						}
						CLEAR_STATUS(DBClose(session->descr->dbtype)(
																	&session->dbkeyselect[number], NULL));	
					}
				}
			}
			va_end( ap );

			
			MEfree((PTR)values);
		}
		CLEAR_STATUS(DDFSemClose(&(session->sem)));
	}
return(err);
}

/*
** Name: DDGExecute()	- Execute a Data Dictionary query
**
** Description:
**
** Inputs:
**  PDDG_SESSION	: pointer on a Data Dictionary session
**	DDG_OBJECT_NUMBER : Data Dictionary query number
**	char*			: parameter definition (used to check parameters matching)
**	...			: list of variables
**
** Outputs:
**
** Returns:
**	GSTATUS		: 
**					E_OG0001_OUT_OF_MEMORY
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
*/

GSTATUS  
DDGExecute(
	PDDG_SESSION session, 
	DDG_OBJECT_NUMBER number, 
	char *paramDef, 
	...) 
{
	GSTATUS err = GSTAT_OK;
	va_list	ap;
	PDB_VALUE values;
	char *tmp = session->descr->statements[number].params;

	session->current = &session->dbexecute[number];
	session->numberOfProp = 0;

	err = DDFSemOpen(&(session->sem), TRUE);
	if (err == GSTAT_OK) 
	{
		err = G_ME_REQ_MEM(
				0, 
				values, 
				DB_VALUE, 
				session->descr->statements[number].prop_max);

		if (err == GSTAT_OK) 
		{
			u_i4 i = 0;
			va_start( ap, paramDef );
			while (err == GSTAT_OK && tmp[0] != EOS) 
			{
				DDG_CHECK_PROPERTY (tmp[0], paramDef);	
				if (err == GSTAT_OK) 
				{
					DDG_SET_PROPERTY (tmp[0], values[i++]);
				}
				tmp++;
			}
			va_end( ap );
			if (err == GSTAT_OK) 
			{
				err = DBParamsFromTab(session->descr->dbtype)(
						&session->dbexecute[number],
						values);

				if (err == GSTAT_OK) 
				{
					err = DBExecute(session->descr->dbtype)(
						&(session->cached_session->session), 
						&session->dbexecute[number]);

					if (err == GSTAT_OK)
						err = DBNumberOfProperties(session->descr->dbtype)(
														&session->dbexecute[number],
														&session->numberOfProp);
				}
			}
			MEfree((PTR)values);
		}
	}
return(err);
}

/*
** Name: DDGProperties()	- Drag all properties of the current instance
**
** Description:
**
** Inputs:
**  PDDG_SESSION	: pointer on a Data Dictionary session
**	char*			: parameter definition (used to check parameters matching)
**
** Outputs:
**	...			: list of variables
**
** Returns:
**	GSTATUS		: 
**					E_OG0001_OUT_OF_MEMORY
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
*/

GSTATUS  
DDGProperties(
	PDDG_SESSION session, 
	char *paramDef, 
	...) 
{
	GSTATUS err = GSTAT_OK;

	if (session->current &&
		paramDef != NULL &&
		paramDef[0] != EOS) 
	{
		char*	tmp = paramDef;
		u_i4	prop = 1;
		va_list	ap;

		va_start( ap, paramDef );

		while (err == GSTAT_OK && prop <= session->numberOfProp) 
		{
			if (tmp[0] != '%')
				err = DDFStatusAlloc (E_DF0017_DDG_BAD_PARAM);
			else
			{
				DDG_GET_PROPERTY (
					tmp[1],
					session->descr->dbtype,
					session->current,
					prop);
				tmp++;
			}
			tmp++;
			prop++;
		}

		if (err == GSTAT_OK && tmp[0] != EOS)
			err = DDFStatusAlloc (E_DF0017_DDG_BAD_PARAM);

		va_end( ap );
	}
return(err);
}

/*
** Name: DDGNext()	- Change the current instance by going to the next one
**
** Description:
**
** Inputs:
**  PDDG_SESSION	: pointer on a Data Dictionary session
**
** Outputs:
**	bool			: FALSE if no instance are available
**
** Returns:
**	GSTATUS		: 
**					E_OG0001_OUT_OF_MEMORY
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
*/

GSTATUS  
DDGNext(
	PDDG_SESSION session, 
	bool *existing) 
{
	GSTATUS err = GSTAT_OK;
	if (session->current != NULL) 
		err = DBNext(session->descr->dbtype)(session->current, existing);	
	else
		*existing = FALSE;
return(err);
}

/*
** Name: DDGClose()	- Close any queries
**
** Description:
**
** Inputs:
**  PDDG_SESSION	: pointer on a Data Dictionary session
**
** Outputs:
**
** Returns:
**	GSTATUS		: 
**					E_OG0001_OUT_OF_MEMORY
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
*/

GSTATUS  
DDGClose(
	PDDG_SESSION session) 
{
	GSTATUS err = GSTAT_OK;
	if (session->current) 
	{
		err = DBClose(session->descr->dbtype)(session->current, NULL);
		session->current = NULL;
		CLEAR_STATUS(DDFSemClose(&(session->sem)));
	}
return(err);
}
