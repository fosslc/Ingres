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
# include	<afe.h>
# include	<er.h>
# include	"erfv.h"
# include	<ug.h>

/*
**	CHKLIST.c  -  check to see if a field matches a list
**
**	This routine takes the union "value" and checks to see if
**	that value is contained in the list passed.  A boolean value
**	is returned in the union "result" in the integer portion as
**	1 or 0.	 The list is a linear linked list.  The routine quits
**	when it finds a matching element or when it reaches the end.
**
**	Basic assumptions made in this routine:
**	1) All the constants in the list are of the same type.
**	2) The datatype of the constants is compatible with that
**	   of the "value" we are checking on.  This is ensure
**	   by the setup code.
**	3) NO conversion is necessary between any of the character
**	   datatypes before calling the function instance for the
**	   comparison.	All string constants in a list are of
**	   type DB_TXT_TYPE as necessitated by afe_txtconst.
**	4) For non-character datatypes, the datatype of the constants
**	   in the list and the value being checked are the same.
**	   The set up of this structure will do any necessary
**	   conversion of the list values to be the same datatype
**	   as on the left hand side.
**	5) List checking is for equality only.
**
**	Arguments:  result  -  union containing integer 1 or 0
**		    value   -  value of element to be compared (union)
**		    list    -  list to check
**
**	Returns:  1  -	List check properly regardless of outcome
**		  0  -	Error detected in list
**
**	History:  15 Dec 1981  -  written (JEN)
**
**		  11 Mar 1983  -  changed fields referenced in VDATA
**				  structure (value). Wrong fields
**				  were being referenced - nml
**	04/03/87 (dkh) - Added support for ADTs.
**	05/06/87 (dkh) - Fixed VMS compilation problems.
**	19-jun-87 (bruceb) Code cleanup.
**	08/14/87 (dkh) - ER changes.
**	12/12/87 (dkh) - Changed swinerr to IIUGerr.
**	03/27/90 (dkh) - Fixed bug 8644.
**	18-apr-90 (bruceb)
**		Lint cleanup; vl_chklist now has only 3 args.
**	11/08/90 (dkh) - Put in change to correctly return
**			 a V_NULL result if we are comparing
**			 a null value to a list of values.
**	06/14/92 (dkh) - Added support for decimal datatype in 6.5.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      19-oct-2009 (joea)
**          Declare equals in vl_chklist as i1 to match DB_BOO_TYPE usage.
*/

FUNC_EXTERN	ADF_CB		*FEadfcb();
FUNC_EXTERN	STATUS		afe_clinstd();


i4
vl_chklist(result, value, list)		/* CHECK LIST: */
VDATA	*result;			/* result of check */
VDATA	*value;				/* value to be checked */
VALROOT *list;				/* root pointer of list */
{
	VALLIST *node;			/* pointer to current element in list */
	ADI_FI_ID	fid;
	DB_DATA_VALUE	res;
	AFE_OPERS	ops;
	DB_DATA_VALUE	*oparr[2];
	ADF_CB		*ladfcb;
	i4		isnull;
	DB_DATA_VALUE	*cdbv;
	DB_DATA_VALUE	*coparr[2];
	AFE_OPERS	cops;
	i1		equals[4];	/* sneaky way to get space for
					** nullable data area */

	ladfcb = FEadfcb();

	/*
	**  If the the field value is NULL, then
	**  set result to V_NULL to indicate
	**  list comparison is NULL as well.
	*/
	if (AFE_ISNULL(&(value->v_dbv)))
	{
		result->v_res = V_NULL;
		return(TRUE);
	}

	/*
	**  Use nullable DB_BOO_TYPE in case we are matching
	**  a null value.
	*/
	res.db_datatype = DB_BOO_TYPE;
	res.db_length = sizeof(equals[0]);
	res.db_prec = 0;
	res.db_data = (PTR) &equals[0];
	AFE_MKNULL(&res);

	ops.afe_opcnt = 2;
	ops.afe_ops = oparr;

	result->v_res = FALSE;

	for (node = list->listroot; node != NULL; node = node->listnext)
	{
		if ((fid = node->v_lfiid) == ADI_NOFI)
		{
			return(FALSE);
		}

		/*
		**  We may need to convert the "value" dbv before we can
		**  succesfully compare.  This is necessary since the
		**  values in the list can be all three numeric types
		**  (int, float and decimal).  Since we can't convert the
		**  field value to the correct type until we know which
		**  value in the list we are comparing against, we have to
		**  delay conversion until now.  This will only cost users
		**  that mix datatypes in their validation list.
		*/
		if ((cdbv = node->left_dbv) != NULL)
		{
			cops.afe_opcnt = 1;
			cops.afe_ops = coparr;
			cops.afe_ops[0] = &(value->v_dbv);
			if (afe_clinstd(ladfcb, node->left_cfid, &cops,
				cdbv) != OK)
			{
                                IIUGerr(E_FV0005_Conversion_error,
                                        UG_ERR_ERROR, 0);
                                return(FALSE);
			}

			/*
			**  Now that we have successfully converted,
			**  use the converted value in the check.
			*/
			oparr[0] = cdbv;
		}
		else
		{
			oparr[0] = &(value->v_dbv);
		}

		oparr[1] = &(node->vl_dbv);
		if (afe_clinstd(ladfcb, fid, &ops, &res) != OK)
		{
			IIUGerr(E_FV0001_Internal_error, UG_ERR_ERROR, 0);
			result->v_res = FALSE;
			return(TRUE);
		}
		if (!AFE_ISNULL(&res) && equals[0])
		{
			result->v_res = TRUE;
			break;
		}
	}
	return(TRUE);
}
