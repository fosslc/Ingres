/******************************************************************************
**
** Copyright (c) 2004 Ingres Corporation
**
******************************************************************************/

#include <ddginit.h>
#include <erddf.h>

/**
**
**  Name: ddginit.c - Data Dictionary Services Facility for Initialization
**
**  Description:
**		Provide a set of funtions to initialize 
**		and load in memory the Data Dictionary module
**
**  History:    
**	5-Feb-98 (marol01)
**	    created
**	29-Jul-1998 (fanra01)
**	    Cleaned up compiler warnings on unix.
**      20-Nov-1998 (fanra01)
**          Add isunique property.  This flag forces isnull to FALSE.
**      06-Aug-1999 (fanra01)
**          Change nat to i4, u_nat to u_i4, longnat to i4, u_longnat to u_i4
**	08-aug-1999 (mcgem01)
**	    Seems that Ray missed a couple of longnats in his last change.
**          Change nat to i4, u_nat to u_i4, i4 to i4, u_i4 to u_i4
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/*
** Name: DDGInitObject()	- Initialize object in memory
**
** Description:
**
** Inputs:
**	PDDG_DESCR		: pointer on a Data Dictionary description
**  u_nat			: object id
**  char*			: object name
**
** Outputs:
**
** Returns:
**	GSTATUS		: 
**					E_DF0011_DDG_INVALID_OBJ_NUM
**					E_DF0001_OUT_OF_MEMORY
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
DDGInitObject(
	PDDG_DESCR descr, 
	u_i4 id, 
	char *name) 
{
    GSTATUS err = GSTAT_OK;

  if (id < 0) 
	{
		err = DDFStatusAlloc (E_DF0011_DDG_INVALID_OBJ_NUM);
		DDFStatusInfo(err, name);
  } 
  else if (!descr->objects || descr->object_max < id) 
  {
		PDDG_OBJECT tmp = descr->objects;
		err = G_ME_REQ_MEM(0, descr->objects, DDG_OBJECT, id + 1);
		if (err == GSTAT_OK) 
		{
	    if (tmp) 
				MEcopy(
		       tmp, 
		       sizeof(DDG_OBJECT) * (descr->object_max + 1), 
		       descr->objects);

				descr->object_max = id;
		} 
		else
	    descr->objects = tmp;
  }

  if (err == GSTAT_OK) 
  {
		err = G_ST_ALLOC(descr->objects[id].name, name);
  }
return(err);
}

/*
** Name: DDGInitProperty()	- Initialize property in memory
**
** Description:
**
** Inputs:
**	PDDG_DESCR		: pointer on a Data Dictionary description
**  u_nat			: property id
**  char*			: property name
**  u_nat			: reference to the object id
**	bool			: TRUE if it is a key
**  char*			: type of the property
**  u_nat			: length of that property
**
** Outputs:
**
** Returns:
**	GSTATUS		: 
**					E_DF0011_DDG_INVALID_OBJ_NUM
**					E_DF0001_OUT_OF_MEMORY
**					GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**      20-Nov-1998 (fanra01)
**          Add isunique property.  This flag forces isnull to FALSE.
*/

GSTATUS  
DDGInitProperty(
    PDDG_DESCR  descr,
    u_i4       id,
    char*       name,
    u_i4       objId,
    bool        isKey,
    char*       type,
    u_i4       length,
    bool        isnull,
    bool        isunique)
{
    GSTATUS err = GSTAT_OK;

    if (id < 0 || objId < 0 || objId > descr->object_max)
    {
          err = DDFStatusAlloc (E_DF0013_DDG_INVALID_PROP_NUM);
          DDFStatusInfo(err, name);
    }
    else
    {
        PDDG_OBJECT object;
        PDB_PROPERTY tmp;
        object = &(descr->objects[objId]);

        err = G_ME_REQ_MEM(0, tmp, DB_PROPERTY, 1);
        if (err == GSTAT_OK)
        {
            err = G_ST_ALLOC(tmp->name, name);
            if (err == GSTAT_OK)
            {
                if (isKey)
                    descr->objects[objId].key_max++;
                descr->objects[objId].prop_max++;
                tmp->key = isKey;
                tmp->type = type[0];
                tmp->length = length;
                if (isunique == TRUE)
                {
                    tmp->isunique = TRUE;
                    tmp->isnull = FALSE;
                }
                else
                {
                    tmp->isnull = isnull;
                }
                tmp->next = object->properties;
                object->properties = tmp;
            }
        }
    }
    return(err);
}

/*
** Name: DDGInitStatement()	- Initialize statement in memory
**
** Description:
**
** Inputs:
**	PDDG_DESCR		: pointer on a Data Dictionary description
**  u_nat			: statement id
**  char*			: statement
**
** Outputs:
**
** Returns:
**	GSTATUS		: 
**					E_DF0015_DDG_INVALID_STMT_NUM
**					E_DF0001_OUT_OF_MEMORY
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
DDGInitStatement(
	PDDG_DESCR descr, 
	u_i4 id, 
	char *stmt) 
{
  GSTATUS err = GSTAT_OK;

  if (id < 0) 
  {
		err = DDFStatusAlloc (E_DF0015_DDG_INVALID_STMT_NUM);
		DDFStatusInfo(err, stmt);
  } 
  else if (!descr->statements || descr->stmt_max < id) 
  {
		PDDG_STATEMENT tmp = descr->statements;
		err = G_ME_REQ_MEM(0, descr->statements, DDG_STATEMENT, id+1);
		if (err == GSTAT_OK) 
		{
	    if (tmp) 
				MEcopy(
								tmp, 
								sizeof(DDG_STATEMENT) * (descr->stmt_max+1), 
								descr->statements);

			descr->stmt_max = id;
		} 
		else
	    descr->statements = tmp;
  }
  if (err == GSTAT_OK) 
  {
		char *tmp;
		u_i4 count = 0;

		G_ST_ALLOC(descr->statements[id].stmt, stmt);
		for (tmp = stmt; tmp[0] != EOS; tmp++)
	    if (tmp[0] == '%')
				count++;		

		err = G_ME_REQ_MEM(0, descr->statements[id].params, char, count+1);
		if (err == GSTAT_OK)
		{
			descr->statements[id].prop_max = count;	
	    for (tmp = stmt, count = 0; tmp[0] != EOS; tmp++)
			if (tmp[0] == '%')
		    descr->statements[id].params[count++] = tmp[1];
	    descr->statements[id].params[count++] = EOS;
		}
  }
return(err);
}

/*
** Name: DDGUnInitObject()	- Release object from memory
**
** Description:
**	Release properties and object
**
** Inputs:
**	PDDG_DESCR		: pointer on a Data Dictionary description
**  u_nat			: object id
**
** Outputs:
**
** Returns:
**	GSTATUS		: 
**					E_DF0011_DDG_INVALID_OBJ_NUM
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
DDGUnInitObject(
	PDDG_DESCR descr, 
	u_i4 id) 
{
  GSTATUS err = GSTAT_OK;
  PDB_PROPERTY tmp;
  PDDG_OBJECT	 obj;

  if (id < 0 || descr->object_max < id) 
  {
		err = DDFStatusAlloc (E_DF0011_DDG_INVALID_OBJ_NUM);
  } 
  else 
  {
		obj = &(descr->objects[id]);
		if (obj) 
		{
			MEfree(obj->name);
			while (obj->properties) 
			{
				tmp = obj->properties;
				obj->properties = obj->properties->next;
				MEfree(tmp->name);
				MEfree((PTR)tmp);
			}
		}
  }
return(err);
}

/*
** Name: DDGUnInitStatement()	- Release statement from memory
**
** Description:
**
** Inputs:
**	PDDG_DESCR		: pointer on a Data Dictionary description
**  u_nat			: statement id
**
** Outputs:
**
** Returns:
**	GSTATUS		: 
**					E_DF0015_DDG_INVALID_STMT_NUM
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
DDGUnInitStatement(
	PDDG_DESCR descr, 
	u_i4 id) 
{
  GSTATUS err = GSTAT_OK;
  PDDG_STATEMENT obj;

  if (id < 0 || descr->stmt_max < id) 
  {
		err = DDFStatusAlloc (E_DF0015_DDG_INVALID_STMT_NUM);
  } 
  else 
  {
		obj = &descr->statements[id];
		if (obj) 
		{
	    MEfree(obj->stmt);
	    MEfree(obj->params);		
		}
  }
return(err);
}

/*
** Name: DDGInitialization()	- Initialization of the Data Dictionary System
**
** Description:
**
** Inputs:
**	u_nat			: module number
**	char*			: dll name
**  char*			: dll path
**  char*			: node
**	char*			: dbname
**  char*			: server class
**	PDDG_DESCR		: pointer a description structure
**
** Outputs:
**	PDDG_DESCR		: initialized description structure
**
** Returns:
**	GSTATUS		: 
**					E_DF0001_OUT_OF_MEMORY
**					GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**      20-Nov-1998 (fanra01)
**          Add isunique flag.
*/

GSTATUS  
DDGInitialize(
    u_i4 module,
    char *name,
    char *node,
    char *dbname,
    char *svrClass,
    char *user_name,
    char *password,
    PDDG_DESCR descr)
{
    GSTATUS err = GSTAT_OK;
    bool existing = FALSE;

    DB_SESSION session = {NULL};

    descr->objects = NULL;
    descr->object_max = 0;
    descr->statements = NULL;
    descr->stmt_max = 0;
    descr->node = NULL;
    descr->dbname = NULL;
    descr->svrClass = NULL;
    if (node)
        err = G_ST_ALLOC(descr->node, node);
    if (!err && dbname)
        err = G_ST_ALLOC(descr->dbname, dbname);
    if (!err && svrClass)
        err = G_ST_ALLOC(descr->svrClass, svrClass);

    if (err == GSTAT_OK)
        err = DriverLoad(name, &descr->dbtype);

    if (err == GSTAT_OK)
    {
        err = DBConnect(descr->dbtype)(
            &session,
            node,
            dbname,
            svrClass,
            user_name,
            password,
            0);

        if (err == GSTAT_OK)
        {
            DB_QUERY query= {NULL};
            u_i4 *Id = NULL, *ObjectId = NULL;
            char *name = NULL;
            char *type = NULL;
            bool *isnull = NULL;
            u_i4 *isKey = NULL, *length = NULL;
            bool* isunique = NULL;

            err = DBRepObjects(descr->dbtype)(&session, module, &query);
            if (err == GSTAT_OK)
            {
                do
                {
                    err = DBNext(descr->dbtype)(&query, &existing);
                    if (err == GSTAT_OK && existing == TRUE)
                    {
                        err = DBInteger(descr->dbtype)(&query, 1, (i4 **)&ObjectId);
                        if (err == GSTAT_OK)
                            err = DBText(descr->dbtype)(&query, 2, &name);
                        if (err == GSTAT_OK)
                            err = DDGInitObject(descr, *ObjectId, name);
                    }
                } while (err == GSTAT_OK && existing == TRUE);

                if (err == GSTAT_OK)
                    err = DBClose(descr->dbtype)(&query, NULL);
                CLEAR_STATUS( DBRelease(descr->dbtype)(&query) );
            }

            if (err == GSTAT_OK)
                err = DBRepProperties(descr->dbtype)(&session, module, &query);

            if (err == GSTAT_OK)
            {
                do
                {
                    err = DBNext(descr->dbtype)(&query, &existing);
                    if (err == GSTAT_OK && existing == TRUE)
                    {
                        err = DBInteger(descr->dbtype)(&query, 1, (i4**)&Id);
                        if (err == GSTAT_OK)
                            err =DBText(descr->dbtype)(&query, 2, &name);
                        if (err == GSTAT_OK)
                            err =DBInteger(descr->dbtype)(&query,3,(i4**)&ObjectId);
                        if (err == GSTAT_OK)
                            err =DBInteger(descr->dbtype)(&query, 4, (i4**)&isKey);
                        if (err == GSTAT_OK)
                            err =DBText(descr->dbtype)(&query, 5, &type);
                        if (err == GSTAT_OK)
                            err =DBInteger(descr->dbtype)(&query, 6, (i4**)&length);
                        if (err == GSTAT_OK)
                            err =DBInteger(descr->dbtype)(&query, 7, (i4**)&isnull);
                        if (err == GSTAT_OK)
                            err =DBInteger(descr->dbtype)(&query, 8, (i4**)&isunique);
                        if (err == GSTAT_OK)
                            err =DDGInitProperty(
                                descr, *Id, name, *ObjectId, *isKey, type,
                                *length, *isnull, *isunique);
                    }
                }
                while (err == GSTAT_OK && existing == TRUE);

                if (err == GSTAT_OK)
                    err = DBClose(descr->dbtype)(&query, NULL);
                CLEAR_STATUS( DBRelease(descr->dbtype)(&query) );
            }

            if (err == GSTAT_OK)
                err = DBRepStatements(descr->dbtype)(&session, module, &query);

            if (err == GSTAT_OK)
            {
                do
                {
                    err = DBNext(descr->dbtype)(&query, &existing);
                    if (err == GSTAT_OK && existing == TRUE)
                    {
                        err = DBInteger(descr->dbtype)(&query, 1, (i4**)&ObjectId);
                        if (err == GSTAT_OK)
                        err = DBText(descr->dbtype)(&query, 2, &name);
                        if (err == GSTAT_OK)
                        err = DDGInitStatement(descr, *ObjectId, name);
                    }
                } while (err == GSTAT_OK && existing == TRUE);
                if (err == GSTAT_OK)
                    err = DBClose(descr->dbtype)(&query, NULL);
                CLEAR_STATUS( DBRelease(descr->dbtype)(&query) );
            }
            if (err == GSTAT_OK)
            {
                u_i4 i;
                for (i=0; err == GSTAT_OK && i <= descr->object_max; i++)
                    if (descr->objects[i].name != NULL &&
                        descr->objects[i].name[0] != EOS)
                        err = DBGenerate(descr->dbtype)(
                            &session,
                            descr->objects[i].name,
                            descr->objects[i].properties);
            }
            if (err == GSTAT_OK)
                err = DBCommit(descr->dbtype)(&session);
            else
                CLEAR_STATUS(DBRollback(descr->dbtype)(&session));
            CLEAR_STATUS(DBDisconnect(descr->dbtype)(&session));

            MEfree((PTR)Id);
            MEfree((PTR)ObjectId);
            MEfree((PTR)name);
            MEfree((PTR)type);
            MEfree((PTR)isKey);
            MEfree((PTR)length);
            MEfree((PTR)isnull);
        }
    }
    if (err)
          CLEAR_STATUS(DDGTerminate(descr));
    return(err);
}

/*
** Name: DDGTerminate()	- UnInitialization of the Data Dictionary System
**
** Description:
**
** Inputs:
**	PDDG_DESCR		: pointer a description structure
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
DDGTerminate(
	PDDG_DESCR descr) 
{
  GSTATUS err = GSTAT_OK;
  u_i4 i = 0;
	if (descr->dbname != NULL)
	{
    descr->dbtype = 0;

    for (i = 0; i <= descr->object_max; i++)
			CLEAR_STATUS(DDGUnInitObject(descr, i));
    MEfree((PTR)descr->objects);
    descr->objects = NULL;

    for (i = 0; i <= descr->stmt_max; i++)
			CLEAR_STATUS(DDGUnInitStatement(descr, i));
    MEfree((PTR)descr->statements);
    descr->statements = NULL;
	}
return(GSTAT_OK);
}
