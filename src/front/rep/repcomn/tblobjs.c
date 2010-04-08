/*
** Copyright (c) 2004 Ingres Corporation
*/
# include <compat.h>
# include <st.h>
# include <er.h>
# include <rpu.h>
# include <tblobjs.h>
# include <cm.h>
# include <si.h>

/**
** Name:	tblobjs.c - table object name routines
**
** Description:
**	Defines
**		RPtblobj_name - generate a table object name
**
** History:
**	16-dec-96 (joea)
**		Created based on tblobjs.sc in replicator library.
**	23-jul-98 (abbjo03)
**		Remove objects made obsolete by generic RepServer.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	06-Oct-2005 (inifa01) INGREP180  b115337
**	    createkeys->bothqueeuandshadow through repmgr fails in an 
**	    SQL-92 installation.  
**	    FIX: Don't asume lowercase in generating the object names
**	    check table_name case and affix proper extension. 
**/

# define TRUNC_TBL_NAMELEN	10	/* truncated table name len */


/*{
** Name:	RPtblobj_name - generate a table object name
**
** Description:
**	Generates the name for a table object.
**
** Inputs:
**	table_name	- the table name
**	table_no	- the table number
**	object_type	- table object type constant
**
** Outputs:
**	object_name	- the table object name
**
** Returns:
**	OK or FAIL
*/
STATUS
RPtblobj_name(
char	*table_name,
i4	table_no,
i4	object_type,
char	*object_name)
{

	char *tmp_table_name;
	bool isSQL92 = FALSE;

	/* initialize object_name in case of early return */
	*object_name = EOS;

	/* validate table name */
	if (table_name == NULL || *table_name == EOS)
	{
		/* invalid table name */
		SIprintf("TBLOBJ_NAME: Null or blank table name.\n");
		return (FAIL);
	}

	/* validate table number */
	if (table_no < TBLOBJ_MIN_TBLNO || table_no > TBLOBJ_MAX_TBLNO)
	{
		SIprintf("TBLOBJ_NAME: Invalid table number of %d\n",
			table_no);
		return (FAIL);
	}

	/* 
	** Don't assume lowercase, check for case of the table_name.  
	** SQL92 objects are uppercase.
	*/
	for(tmp_table_name = table_name; *tmp_table_name != '\0'; tmp_table_name++)
            if(CMupper(tmp_table_name))
	    {   
                isSQL92 = TRUE;
                tmp_table_name = NULL;
		break;
	    } else if (CMlower(tmp_table_name))
		break;

	/* make the specified object name */
	switch (object_type)
	{
	/* Table object names */
	case TBLOBJ_ARC_TBL:  /* archive table */
		STprintf(object_name, ERx("%.*s%0*d%s"), TRUNC_TBL_NAMELEN,
			RPedit_name(EDNM_ALPHA, table_name, NULL),
			TBLOBJ_TBLNO_NDIGITS, (i4)table_no, isSQL92 ? ERx("ARC") : ERx("arc"));
		break;

	case TBLOBJ_SHA_TBL:  /* shadow table */
		STprintf(object_name, ERx("%.*s%0*d%s"), TRUNC_TBL_NAMELEN,
			RPedit_name(EDNM_ALPHA, table_name, NULL),
			TBLOBJ_TBLNO_NDIGITS, (i4)table_no, isSQL92 ? ERx("SHA") : ERx("sha"));
		break;

	case TBLOBJ_TMP_TBL:  /* temporary table */
		STprintf(object_name, ERx("%.*s%0*d%s"), TRUNC_TBL_NAMELEN,
			RPedit_name(EDNM_ALPHA, table_name, NULL),
			TBLOBJ_TBLNO_NDIGITS, (i4)table_no, ERx("tmp"));
		break;

	case TBLOBJ_REM_INS_PROC:  /* remote insert procedure */
		STprintf(object_name, ERx("%.*s%0*d%s"), TRUNC_TBL_NAMELEN,
			RPedit_name(EDNM_ALPHA, table_name, NULL),
			TBLOBJ_TBLNO_NDIGITS, (i4)table_no, ERx("rmi"));
		break;

	case TBLOBJ_REM_UPD_PROC:  /* remote update procedure */
		STprintf(object_name, ERx("%.*s%0*d%s"), TRUNC_TBL_NAMELEN,
			RPedit_name(EDNM_ALPHA, table_name, NULL),
			TBLOBJ_TBLNO_NDIGITS, (i4)table_no, ERx("rmu"));
		break;

	case TBLOBJ_REM_DEL_PROC:  /* remote delete procedure */
		STprintf(object_name, ERx("%.*s%0*d%s"), TRUNC_TBL_NAMELEN,
			RPedit_name(EDNM_ALPHA, table_name, NULL),
			TBLOBJ_TBLNO_NDIGITS, (i4)table_no, ERx("rmd"));
		break;

	case TBLOBJ_SHA_INDEX1:  /* shadow index 1 */
		STprintf(object_name, ERx("%.*s%0*d%s"), TRUNC_TBL_NAMELEN,
			RPedit_name(EDNM_ALPHA, table_name, NULL),
			TBLOBJ_TBLNO_NDIGITS, (i4)table_no, isSQL92 ? ERx("SX1") : ERx("sx1"));
		break;

	default:  /* invalid object type */
		SIprintf("TBLOBJ_NAME: Invalid object type of %d.\n",
			object_type);
		return (FAIL);
	}

	return (OK);
}
