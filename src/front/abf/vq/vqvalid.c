
/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<me.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<er.h>
# include	<abclass.h>
# include	<st.h>
# include       <metafrm.h>
# include       "ervq.h"


/**
** Name:	vqvalid.c - Validate the Visual query
**
** Description:
**	This file defines:
**			IIVQveVqError report error in vq
**
** History:
**	01/11/89 (tom)	- created
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

static ER_MSGID vq_XlatErr();
FUNC_EXTERN STATUS IIVQvvqValidateVQ();
FUNC_EXTERN VOID IIVQveVqError();
FUNC_EXTERN STATUS IIVQvqcVQCheck();


/*{
** Name:	vq_XlatErr	- Translate error into an errorid 
**
** Description:
**	Return a string to the caller's buffer to represent the error.
**	(this is called from the table reconciliation)
**
** Inputs:
**	METAFRAME *mf;	- metaframe to check 
**	char *buf;	- buffer to put the message in
**	i4 size;	- size of callers buffer
**
** Outputs:
**	Returns:
**		none
**
**	Exceptions:
** Side Effects:
**
** History:
**	11/29/89 (tom) -  created
*/
static ER_MSGID
vq_XlatErr(err)
STATUS err;
{
	ER_MSGID id;

	switch (err)
	{
	case OK:
		id = F_VQ008D_No_Err;
		break;

	case VQ_NOMCOL:
		id = F_VQ008E_No_Mcol;
		break;

	case VQ_NODCOL:
		id = F_VQ008F_No_Dcol;
		break;

	case VQ_NOJOIN:
		id = F_VQ0090_No_Join;
		break;

	case VQ_NOMDJO:
		id = F_VQ0091_No_Mdjo;
		break;

	case VQ_BADMDJ:
		id = F_VQ0092_Bad_Joi;
		break;
	}
	
	return (id);
}
/*{
** Name:	IIVQvvqValidateVQ	- validate the vq
**
** Description:
**	Test the Visual query for validity.. 
**	Return a string to the caller's buffer to represent the error.
**	(this is called from the table reconciliation)
**
** Inputs:
**	METAFRAME *mf;	- metaframe to check 
**	char *buf;	- buffer to put the message in
**	i4 size;	- size of callers buffer
**
** Outputs:
**	Returns:
**		none
**
**	Exceptions:
** Side Effects:
**
** History:
**	11/29/89 (tom) -  created
*/
STATUS
IIVQvvqValidateVQ(mf, buf, size)
METAFRAME *mf;
char *buf;
i4  size;
{
	STATUS stat;
	ER_MSGID err;
	char dummy[FE_MAXNAME+1];

	/* actually check the vq structures */
	stat = IIVQvqcVQCheck(mf, dummy);

	/* set the metaframe struct as having an error, or clear if no err */
	if (stat == OK)
	{
		mf->state &= ~MFST_VQEERR;
	}
	else
	{
		mf->state |= MFST_VQEERR;
	}
	MFST_ER_SET_MACRO(mf, stat);

	/* translate the error status into a message id */ 
	err = vq_XlatErr(stat);
	
	/* copy error string into caller's buffer */
	STlcopy(ERget(err), buf, size);


	return (stat);
}

/*{
** Name:	IIVQveVqError	- return str indicating error in VQ
**
** Description:
**	Test the Visual query for validity.. 
**	Return a string to the caller's buffer to represent the error.
**	(this is initially called from the error listing frame)
**
** Inputs:
**	METAFRAME *mf;	- metaframe to check 
**	char *buf;	- buffer to put the message in
**	i4 size;	- size of callers buffer
**
** Outputs:
**	Returns:
**		none
**
**	Exceptions:
** Side Effects:
**
** History:
**	11/29/89 (tom) -  created
*/
VOID
IIVQveVqError(mf, buf, size)
METAFRAME *mf;
char *buf;
i4  size;
{
	ER_MSGID err;

	err = vq_XlatErr(MFST_ER_GET_MACRO(mf));
	STlcopy(ERget(err), buf, size);
}


/*{
** Name:	IIVQvqcVQCheck	- check the visual query for validity
**
** Description:
**	Test the Visual query for validity.. this function just returns
**	status, since it's callers want to process errors differently 
**
** Inputs:
**	none
**
** Outputs:
**	Returns:
**		none
**
**	Exceptions:
** Side Effects:
**
** History:
**	11/06/89 (tom) -  created
*/
STATUS 
IIVQvqcVQCheck(mf, arg)
METAFRAME *mf;
char *arg;
{
	register i4  i;
	register MFJOIN *j;
	register i4  flags;
	bool mdjoin_ok;
	bool col_used;
	i4 idx;
	MFTAB *tab;
	bool saw_md = FALSE;
	bool jtab[MF_MAXTABS];


	/* we must check that the primary tables have at least one column
	   marked as being used */
	idx = IIVQptPrimaryTab(mf, TAB_MASTER);
	tab = mf->tabs[idx];

	/* mdjoin_ok is a special test which is relevent to 
	   append frames only */
	mdjoin_ok = (mf->apobj->class == OC_APPFRAME 
		    && mf->mode == MF_MASDET) ? FALSE : TRUE;

	for (col_used = FALSE, i = tab->numcols; i--; )
	{
		flags = tab->cols[i]->flags;

		/* for append frames, make sure that at least one of 
		   the m/d join fields is either displayed or in a variable
		   or has a non-null info column */
		if (mdjoin_ok == FALSE && (flags & COL_DETJOIN))
		{
			if (flags & (COL_USED|COL_VARIABLE))
			{
				mdjoin_ok = TRUE; 
			}
			else
			{
				char *p = tab->cols[i]->info;

				if (  *p != EOS
				   && STbcompare(ERx("NULL"), 0, p, 0, TRUE ) 
					!= 0
				   )
				{
					mdjoin_ok = TRUE;
				}
			}
		}

		/* test that at least one of the columns is to be used */
		if (flags & COL_USED)
		{
			col_used = TRUE;
		}
	}

	/* if we went all the way through the columns and there are 
	   no used columns.. then give an error */
	if (col_used == FALSE)
	{
		STcopy(ERget(F_VQ0093_Master), arg);
		return (VQ_NOMCOL);
	}

	if ((mf->mode & MFMODE) == MF_MASDET)
	{
		idx = IIVQptPrimaryTab(mf, TAB_DETAIL);
		tab = mf->tabs[idx];

		for (i = tab->numcols; i--; )
		{
			if (tab->cols[i]->flags & COL_USED)
			{
				break;
			}
		}
		if (i < 0)
		{
			STcopy(ERget(F_VQ0094_Detail), arg);
			return (VQ_NODCOL);
		}
	}

	/* not concerned with joins if only one table */ 
	if (mf->numtabs == 1)
	{
		return (OK);
	}

	MEfill(sizeof(jtab), (unsigned char) FALSE, (PTR) jtab);

	/* go through the join array and mark the table as having
	   been joined */
	for (i = mf->numjoins; i--; )
	{
		j = mf->joins[i];

		if (j->type == JOIN_MASDET)
		{
			saw_md = TRUE;
		}
		jtab[j->tab_1] = jtab[j->tab_2] = TRUE;
	}


	/* go through the tables, and if we find one that was not joined
	   then we have an error */
	for (i = mf->numtabs; i--; )
	{
		if (jtab[i] == FALSE)
		{
			STcopy(mf->tabs[i]->name, arg);
			return (VQ_NOJOIN);
		}
	}

	/* if this is a master detail frame, but we have no master
	   detail join.. it is an error */
	if ((mf->mode & MFMODE) == MF_MASDET && saw_md == FALSE)
	{
		return (VQ_NOMDJO);
	}

	/* if we didn't find a master/detail join column either displayed
	   or with a non-null assignment expression... this is tested
	   last cause it is a more obscure type of error */
	if (mdjoin_ok == FALSE)
	{
		return (VQ_BADMDJ);
	}

	return (OK);
}
