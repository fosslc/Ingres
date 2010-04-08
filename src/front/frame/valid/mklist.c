/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ft.h>
# include	<fmt.h>
# include	<adf.h>
# include	<frame.h>
# include	<valid.h>
# include	<ds.h>
# include	<feds.h>
# include	<me.h>
# include	<er.h>
# include	"erfv.h"

/*
**	MKLIST.c  -  Make a list
**
**	These two routines create a list to be evaluated in chklist.c
**	The first routine, v_mklist(), creates the root pointer to the
**	list and assigns the data type to the list.  The second routine
**	v_addlist(), adds the respective attributes to end of the list.
**	The root struct, current node, and the current datatype are
**	stored in static variables for only these routines to see and
**	are initialized in v_mklist().
**
**	(c) 1981  -  Relational Technology Inc.	  Berkeley, CA
**
**	V_MKLIST  -
**
**	  Arguments:  type  -  datatype of the list
**	  Returns:  pointer to the root
**
**	V_ADDLIST  -
**
**	  Arguments:  value  -	value of the item to be added
**		      type   -	type of the item to be added
**
**	History:  15 Dec 1981  -  written  (JEN)
**		  20 APR 1982  -  modified (JRC) (syntax only)
**		5-jan-1987 (peter)	Changed calls to IIbadmem.
**	04/07/87 (dkh) - Added support for ADTs.
**	19-jun-87 (bab) Code cleanup.
**	13-jul-87 (bab) Changed memory allocation to use [FM]Ereqmem.
**	08/14/87 (dkh) - ER changes.
**	09/29/87 (dkh) - Changed iiugbma* to IIUGbma*.
**	09/30/87 (dkh) - Added procname as param to IIUGbma*.
**	02/04/88 (dkh) - By decree of LRC, all string constants are varchar.
**	07-apr-88 (bruceb)
**		Changed from using sizeof(DB_TEXT_STRING)-1 to using
**		DB_CNTSIZE.  Previous calculation is in error.
**	08/31/89 (dkh) - Changed YYSTYPE to IIFVvstype.
**	06/13/92 (dkh) - Added support for decimal datatype for 6.5.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/


static	VALROOT *_root = NULL;		/* root to current list */
static	VALLIST **_node = NULL;		/* pointer to current list element */

VALROOT *	/* added fact that v_mklist returns a ptr to VALROOT -nml */
vl_mklist(type)			/* MAKE LIST: */
i4	type;				/* data type of the new list */
{
	/* create the root */
	if ((_root = (VALROOT *)MEreqmem((u_i4)0, (u_i4)(sizeof(VALROOT)),
	    TRUE, (STATUS *)NULL)) == NULL)
	{
		IIUGbmaBadMemoryAllocation(ERx("vl_mklist"));
	}
	_root->listtype = type;		/* set the current type */
	_node = &_root->listroot; /* current node points to head of the list */
	*_node = NULL;

	return(_root);
}

vl_addlist(value, type)		/* ADD to a LIST: */
IIFVvstype	*value;		/* value to add to a list */
i4		type;		/* type of the list */
{
	DB_DATA_VALUE	*dbvptr;
	DB_TEXT_STRING	*text;
	i4		*i4ptr;
	f8		*f8ptr;
	PTR		decdata;
	DB_DATA_VALUE	*decdbv;
	char		*procname = ERx("vl_addlist");

	/*
	**  No cross type checking is done at this point.  When
	**  vl_maketree() is later called, all the values in the
	**  list will then be coerced to the correct field data type.
	*/

	/* allocate the current node */
	if ((*_node = (VALLIST *)MEreqmem((u_i4)0, (u_i4)(sizeof(VALLIST)),
	    TRUE, (STATUS *)NULL)) == NULL)
	{
		IIUGbmaBadMemoryAllocation(procname);
	}
	dbvptr = &((*_node)->vl_dbv);
	switch (type)		/* add value according to type */
	{
	  case vINT:
		dbvptr->db_datatype = DB_INT_TYPE;
		dbvptr->db_length = 4;
		dbvptr->db_prec = 0;
		if ((i4ptr = (i4 *)MEreqmem((u_i4)0, (u_i4)sizeof(i4), FALSE,
		    (STATUS *)NULL)) == NULL)
		{
			IIUGbmaBadMemoryAllocation(procname);
		}
		*i4ptr = value->I4_type;
		dbvptr->db_data = (PTR) i4ptr;
		break;

	  case vFLOAT:
		dbvptr->db_datatype = DB_FLT_TYPE;
		dbvptr->db_length = 8;
		dbvptr->db_prec = 0;
		if ((f8ptr = (f8 *)MEreqmem((u_i4)0, (u_i4)sizeof(f8), FALSE,
		    (STATUS *)NULL)) == NULL)
		{
			IIUGbmaBadMemoryAllocation(procname);
		}
		*f8ptr = value->F8_type;
		dbvptr->db_data = (PTR) f8ptr;
		break;

	  case vDEC:
		decdbv = value->dec_type;
		dbvptr->db_datatype = DB_DEC_TYPE;
		dbvptr->db_length = decdbv->db_length;
		dbvptr->db_prec = decdbv->db_prec;
		if ((dbvptr->db_data = MEreqmem((u_i4) 0,
			(u_i4) dbvptr->db_length, FALSE,
			(STATUS *) NULL)) == NULL)
		{
			IIUGbmaBadMemoryAllocation(procname);
		}
		MEcopy(decdbv->db_data, (u_i2) dbvptr->db_length,
			dbvptr->db_data);
		break;

	  case vSTRING:
		/* changed both references from str_type to string_type - nml */
		text = (DB_TEXT_STRING *) value->string_type;
		dbvptr->db_datatype = DB_VCH_TYPE;
		dbvptr->db_length = text->db_t_count + DB_CNTSIZE;
		dbvptr->db_prec = 0;
		dbvptr->db_data = (PTR) text;
		break;

	  default:
		vl_par_error(ERx("ADDLIST"), ERget(E_FV001C_Unknown_constant));
		break;
	}
	_node = &(*_node)->listnext;		/* point to next node */
	*_node = NULL;
}
