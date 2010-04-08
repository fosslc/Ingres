/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<me.h>
# include	<fe.h>
# include	<adf.h>
# include	<fmt.h>
# include	<rtype.h>
# include	<rglob.h>
# include	<afe.h>

/**
** Name:	rconvdbv.c	This file contains the routine r_convdbv.
**
** Description:
**	This file defines:
**
**	r_convdbv	Converts a given string into a DB_DATA_VALUE.
**
** History:
**	23-feb-87 (grant)	implemented.
**      21-jan-88 (rdesmond)    
**              Removed & before second arg of STcopy() call.
**	29-July-1991 (fredv)
**		Changed variable name strlen to strlength so that we can use
** 		the system function strlen(). Also changed the type of strlength
**		from i2 to i4  because that is what STlength returns.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/* # define's */
/* extern's */
/* static's */

/*{
** Name:	r_convdbv	Converts a given string into a DB_DATA_VALUE.
**
** Description:
**	This routine converts a given string to a given type as a DB_DATA_VALUE.
**	Memory is allocated for the specific data, but the DB_DATA_VALUE
**	structure itself must already be allocated.
**
** Inputs:
**	strval		The null-terminated string of the value to convert.
**
**	typestr		User datatype string.
**
**	dbv		Pointer to the DB_DATA_VALUE.
**
** Outputs:
**	dbv		The contents of the DB_DATA_VALUE are filled in with
**			the converted value.
**
**	Returns:
**		OK
**		error status returned by ADF routine adc_cvinto.
**
** History:
**	23-feb-87 (grant)	implemented.
**	29-July-1991 (fredv)
**		Changed variable name strlen to strlength so that we can use
** 		the system function strlen(). Also changed the type of strlength
**		from i2 to i4  because that is what STlength returns.
**	22-oct-1992 (rdrane)
**		Ensure db_prec is initialized, even though it's not used for
**		DB_LTXT_TYPE.
**		
*/

STATUS
r_convdbv(strval, typestr, dbv)
char		*strval;
char		*typestr;
DB_DATA_VALUE	*dbv;
{
    DB_DATA_VALUE	dvfrom;
    AFE_DCL_TXT_MACRO(DB_MAXSTRING)	buffer;
    DB_TEXT_STRING	*text;
    i4			strlength;
    DB_USER_TYPE	dbut;
    STATUS		status;


    strlength = STlength(strval);
    text = (DB_TEXT_STRING *) &buffer;
    text->db_t_count = strlength;
    MEcopy((PTR)strval, strlength, (PTR)(text->db_t_text));

    dvfrom.db_datatype = DB_LTXT_TYPE;
    dvfrom.db_length = strlength + DB_CNTSIZE;
    dvfrom.db_prec = 0;
    dvfrom.db_data = (PTR) text;

    dbut.dbut_kind = DB_UT_NOTNULL;
    STcopy(typestr, dbut.dbut_name);
    status = afe_tychk(Adf_scb, &dbut, dbv);
    if (status != OK)
    {
	FEafeerr(Adf_scb);
	return status;
    }
    dbv->db_data = (PTR) MEreqmem(0,(u_i4)dbv->db_length,TRUE,(STATUS *) NULL);

    return adc_cvinto(Adf_scb, &dvfrom, dbv);
}
