/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<st.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<adf.h>
# include	<afe.h>

/**
** Name:	aftyotpt.c -	Convert from ADF type to type string.
**
** Description:
**	This file contains and defines:
**
** 	afe_tyoutput()	convert from ADF type to type string.
**
** History:
**	Revision 6.3  89/11  wong
**	Added decimal support.
**
**	Revision 6.0  87/02/06  daver
**	Initial revision.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

DB_STATUS	adi_tyname();
DB_STATUS	adc_lenchk();

/*{
** Name:	afe_tyoutput() -	Convert from ADF type to type string.
**
** Description:
**	This routines uses an ADF type to fill in a DB_USER_TYPE
**	that can then be used by the caller to display the type
**	to the user.
**
** Inputs:
**	cb			A pointer to the current ADF_CB 
**				control block.
**
**	dbv
**		.db_datatype	The ADF typeid of the ADF type.
**
**		.db_length	The internal length of the ADF type.
**
**	user_type
**		.adut_name	Points to a buffer to hold the type name.
** Outputs
**	user_type
**		.adut_kind	Set to the nullability and defaultability
**				of the type.
**		.adut_name	Filled in with the type name for the type.
**
** Returns:
**	OK
**	returns from:
**    	    adi_tyname
**    	    adc_lenchk
**
** History:
**	Written	2/6/87	(dpr)
**	03-aug-1989	(mgw)
**		adc_lenchk() interface changed to take DB_DATA_VALUE last
**		argument instead of i4.
**	11/89 (jhw) -- Rewrote for DECIMAL support.
**       9-Jan-2008 (hanal04) Bug 121484
**          Time types may have a precision. If specified use the db_prec
**          value and reformat the type name that iicolumns supplied that did
**          not include the precision.
**      20-Aug-2010 (hanal04) Bug 124285
**          DB_INDS_TYPE should show the precision at the end of the type
**          so that the precision follws the word seconds.
*/
DB_STATUS
afe_tyoutput ( cb, dbv, user_type )
ADF_CB			*cb;
register DB_DATA_VALUE	*dbv;
register DB_USER_TYPE	*user_type;
{
	DB_STATUS	rval;
	DB_DATA_VALUE	dblength;
	ADI_DT_NAME	name;

	if ( (rval = adi_tyname(cb, dbv->db_datatype, &name)) != OK
	   || (rval = adc_lenchk(cb, FALSE, dbv, &dblength)) != OK )
		return rval;

	/*
	** Ok, adi_tyname gets us our 'type name' from the ADF. 
	** all we need now is the nullability info to load into
	** the dbut_kind field of the DB_USER_TYPE.
	**
	**	We now have AFE_NULLABLE_MACRO.
	**	HOW DO WE FIND OUT IF A COLUMN IS DEFAULTABLE ??
	**
	** For now, lets just assume that it is. all types get back
	** either DB_UT_NULL or DB_UT_DEF, and DB_UT_NOTNULL is iced.
	*/
	user_type->dbut_kind =
		AFE_NULLABLE_MACRO(dbv->db_datatype) ? DB_UT_NULL : DB_UT_DEF;

	/*
	** The user length of the datatype is returned by 'adc_lenchk()', above,
	** in the data value descriptor, 'dblength'.  If a fixed length datatype
	** is passed, 'adc_lenchk()' will return a user length of 0.  If the
	** datatype is NOT fixed length, and it is NOT of type i, f, c, or
	** decimal (types DB_INT_TYPE, DB_FLT_TYPE, DB_CHR_TYPE, DB_DEC_TYPE),
	** then it is variable type, and parentheses need to be printed around
	** the length.
	*/
	if ( dblength.db_length == 0 )
	{       /* fixed length, don't print the length unless TIME
                ** with precision required
                */
                switch ( AFE_DATATYPE(dbv) )
                {
                  case DB_INDS_TYPE:
                        if(dblength.db_prec != 0)
                        {
			    char *fmt = "%s(%d)";
                            /* Need to add precision to INTERVAL DAY TO SEC */
                            _VOID_ STprintf( user_type->dbut_name, fmt,
                                             name.adi_dtname,
                                             dblength.db_prec);

                        }
                        else
		            STcopy(name.adi_dtname, user_type->dbut_name);
                        break;

                  case DB_TMWO_TYPE:
                  case DB_TMW_TYPE:
                  case DB_TSWO_TYPE:
                  case DB_TSW_TYPE:
                  case DB_TME_TYPE:
                  case DB_TSTMP_TYPE:
                        if(dblength.db_prec != 0)
                        {
                            /* Need to add precision to TIME */
			    char *fmt = "%s(%d)";
		            char *dtname = name.adi_dtname;
                            char *sptr;

                            sptr = STindex(name.adi_dtname, " ", 0);
                            if(sptr == NULL)
                            {
                                _VOID_ STprintf( user_type->dbut_name, fmt,
                                                 dtname,
                                                 dblength.db_prec);
                            }
                            else
                            {
			        fmt = "%s(%d) %s";
                                *sptr = EOS; 
                                _VOID_ STprintf( user_type->dbut_name, fmt,
                                                 dtname,
                                                 dblength.db_prec,
                                                 (sptr + 1));
                                *sptr = ' ';
                            }
 
                            break;
                        }
                        /* else fall through to default */

                  default:
		        STcopy(name.adi_dtname, user_type->dbut_name);
                        break;
                }
	}
	else
	{
		char	*fmt = "%s%ld";
		char	*dtname = name.adi_dtname;
		i4	preclen = dblength.db_length;
		i4	scale;

		switch ( AFE_DATATYPE(dbv) )
		{
		  case DB_INT_TYPE:
			dtname = "i";
			break;
		  case DB_FLT_TYPE:
			dtname = "f";
			break;
		  case DB_DEC_TYPE:
			fmt = "%s(%ld,%d)";
			preclen = DB_P_DECODE_MACRO(dblength.db_prec);
			scale = DB_S_DECODE_MACRO(dblength.db_prec);
			break;
		  case DB_CHR_TYPE:
			break;
		  default:
			/* assumed to be TEXT, VARCHAR, CHAR */
			fmt = "%s(%d)";
			break;
		} /* end switch */
		_VOID_ STprintf( user_type->dbut_name, fmt,
					dtname,
					preclen,
					scale
		);
	}

	return OK;
}
