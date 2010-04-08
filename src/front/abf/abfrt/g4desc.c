/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<er.h>
# include	<st.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include       <erg4.h>
# include       <eqlang.h>
# include       <fdesc.h>
# include       <abfrts.h>
# include       "g4globs.h"
# include	"rts.h"


/**
** Name:	g4desc.c	- SQLDA-related routines
**
** Description:
**	This file defines:
**
**	IIAG4fdFillDscr		Fill an SQLDA
**	IIAG4udUseDscr		Use an SQLDA to transfer data
**
** History:
**	17-dec-92 (davel)
**		Initial version, based on W4GL version developed by MikeS.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	26-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**	24-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
**/

/* # define's */
/* GLOBALDEF's */

/* extern's */
FUNC_EXTERN	STATUS	iiarRiRecordInfo();
FUNC_EXTERN	bool	IIsqdSetSqlvar();

/* static's */


/*{
** Name:	IIAG4fdFillDscr  -  Fill an SQLDA with attribute descriptors.
**
** Description:
**	Fill the SQLDA with descriptors for all the attributes of an object.
**
** Inputs:
**	object		i4		Object handle
**	language 	i4		3GL language 
**
**	descriptor	IISQLDA *	SQLDA pointer
**
**	Returns:
**		STATUS
**
** History:
**	17-dec-92 (davel)
**		Initial version, based on W4GL version developed by MikeS.
**	15-sep-93 (donc) bug 54736
**		If language is C, NULL terminate sqlname.sqlnamec
*/
STATUS
IIAG4fdFillDscr(i4 object, i4  language, IISQLDA *sqlda)
{
    STATUS status;
    G4ERRDEF g4errdef;
    ABRTSTYPE *rec_type;
    bool is_array;
    DB_DATA_VALUE dbv;
    DB_DATA_VALUE odbv;
    i4  i;
    i4 dummy_rn;
    PTR	    dummy_rd;

    CLEAR_ERROR;

    /* First check that object is valid, and if so, convert to a DBV */
    if ((status = IIAG4chkobj(object, G4OA_OBJECT, 0, G4DESCRIBE_ID)) != OK)
	return status;
    odbv.db_datatype = DB_DMY_TYPE;
    odbv.db_data = iiAG4savedObject;

    /* Sanity check: is SQLDA non-NULL? */
    if (sqlda == NULL)
    {
	g4errdef.errmsg = E_G42720_NULL_SQLDA;
	g4errdef.numargs = 1;
	g4errdef.args[0] = (PTR)iiAG4routineNames[G4DESCRIBE_ID];
	IIAG4semSetErrMsg(&g4errdef, TRUE);
        return E_G42720_NULL_SQLDA;
    }
    
    /* get the class information into rec_type (ignore other returns) */
    if ( iiarRiRecordInfo( &odbv, &rec_type, &dummy_rd, &dummy_rn ) != OK )
    {
	g4errdef.errmsg = E_G42711_BADVALUE;
	g4errdef.numargs = 2;
	g4errdef.args[0] = (PTR)iiAG4routineNames[G4DESCRIBE_ID];
	g4errdef.args[1] = (PTR)ERget(F_G40001_OBJECT);
	IIAG4semSetErrMsg(&g4errdef, TRUE);
        return E_G42711_BADVALUE;
    }

    /* also see if this object is an array - we'll use this later to add
    ** a _state attribute to the list.
    */
    is_array = iiarIarIsArray(&odbv);

    /* Init the SQLDA */
    STmove(ERx("SQLDA"), ' ', 8, sqlda->sqldaid);
    sqlda->sqldabc = sizeof(IISQLDA) + 
			(sizeof(sqlda->sqlvar[0]) * (sqlda->sqln-1));
    sqlda->sqld = 0;

    /* Process the class's attribute descriptors */
    for (i = 0; i < rec_type->abrtcnt; i++)
    {
	register ABRTSATTR *adesc = &rec_type->abrtflds[i];
	register DB_DATA_VALUE *adbv = &adesc->abradattype;

	if (adbv->db_datatype == DB_DMY_TYPE)
	{
	    /* Objects will be returned to the user as i4's */
	    dbv.db_datatype = DB_OBJ_TYPE;	/* switch to DB_OBJ_TYPE */
	    dbv.db_length = sizeof(i4);
	    dbv.db_prec = 0;
	}
	else
	{
	    dbv.db_datatype = adbv->db_datatype;
	    dbv.db_length = adbv->db_length;
	    dbv.db_prec = adbv->db_prec;
	}
	IIsqdSetSqlvar(language, sqlda, 0, adesc->abraname, &dbv);
	if ( language == EQ_C ) 
	    sqlda->sqlvar[i].sqlname.sqlnamec
		[sqlda->sqlvar[i].sqlname.sqlnamel] = EOS;
    }

    /*
    ** For an array, add _rowstate attribute descriptor
    */
    if (is_array)
    {
	dbv.db_datatype = DB_INT_TYPE;
	dbv.db_prec = 0;
	dbv.db_length = sizeof(i2);
	IIsqdSetSqlvar(language, sqlda, 0, ERx("_state"), &dbv);
    }

    return OK;
}
			

/*{
** Name:	IIAG4udUseDscr - Get or Set attributes via descriptor.
**
** Description:
**	Use the SQLDA descriptor to get or set attributes for an object.
**
** Inputs:
**	object		i4		object handle
**	access		i4		object acess
**	index		i4		array index
**	language	i4		3GL language
**	direction	i4		G4SD_SET = SET ATTRIBUTE USING
**					G4SD_GET = GET ATTRIBUTE USING
**	sqlda		IISQLDA *	SQLDA
**
**
**	Returns:
**		STATUS
** History:
**	17-dec-92 (davel)
**		Initial version, based on W4GL version developed by MikeS.
*/
STATUS
IIAG4udUseDscr (i4 language, i4  direction, IISQLDA *sqlda)
{
    i4  caller = (direction == G4SD_SET) ? G4SATTR_ID : G4GATTR_ID;
    STATUS status;
    STATUS rval;
    i4 i;
    G4ERRDEF g4errdef;

    CLEAR_ERROR;

    /* Sanity check -- is SQLDA non-NULL */
    if (sqlda == NULL)
    {
	g4errdef.errmsg = E_G42720_NULL_SQLDA;
	g4errdef.numargs = 1;
	g4errdef.args[0] = (PTR)iiAG4routineNames[caller];
	IIAG4semSetErrMsg(&g4errdef, TRUE);
        return E_G42720_NULL_SQLDA;
    }

    /* Move data using descriptor */
    for (i = 0; i < min(sqlda->sqln, sqlda->sqld) ; i++)
    {
	struct sqvar	*sqv = sqlda->sqlvar + i;
	i4 sqtype;
	i4 sqlen;
	i2 *sqind;
	char sqname[IISQD_NAMELEN];

	/* Do some sanity checks */
	if (sqv->sqldata == NULL)
	{
	    g4errdef.errmsg = E_G42721_NULL_DATA;
	    g4errdef.numargs = 2;
	    g4errdef.args[0] = (PTR)iiAG4routineNames[caller];
	    g4errdef.args[1] = (PTR)&i;
	    IIAG4semSetErrMsg(&g4errdef, TRUE);
	    continue;
	}
	else if (sqv->sqlname.sqlnamel <= 0)
	{
	    g4errdef.errmsg = E_G42722_BAD_NAME;
	    g4errdef.numargs = 2;
	    g4errdef.args[0] = (PTR)iiAG4routineNames[caller];
	    g4errdef.args[1] = (PTR)&i;
	    IIAG4semSetErrMsg(&g4errdef, TRUE);
	    continue;
	}

	sqtype = abs(sqv->sqltype);
	sqlen = sqv->sqllen;
	sqind = (sqv->sqltype < 0) ? sqv->sqlind : NULL;
	STlcopy(sqv->sqlname.sqlnamec, sqname, 
		min(sqv->sqlname.sqlnamel, FE_MAXNAME));

	/* DO some SQL-type into embedded-type fixups */
	if (sqtype == DB_CHR_TYPE && language != EQ_C)
	{
	    /*
	    **  Convert character data into "C string" only if language is C.
	    */
	    sqtype = DB_CHA_TYPE;
	}
	else if (sqtype == DB_CHA_TYPE && language == EQ_C)
	{
	    sqtype = DB_CHR_TYPE;
	}
	else if (sqtype == DB_VCH_TYPE)
	{
	    /* Include character count for Varchar's */
	    sqlen += DB_CNTSIZE;
	}

	/* Call the desired routine to move the data */
	if (direction == G4SD_SET)
	{
	    rval = IIAG4saSetAttr(sqname, sqind, 1, sqtype, sqlen,sqv->sqldata);
	}
	else
	{
	    rval = IIAG4gaGetAttr(sqind, 1, sqtype, sqlen, sqv->sqldata,sqname);
	}
	if (rval != OK)
	    status = rval;
    }
    return status;
}
