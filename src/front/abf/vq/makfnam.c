/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<er.h>
# include	<abclass.h>
# include	<adf.h>
# include	<ft.h>
# include	<fmt.h>
# include	<frame.h>
# include	<oocat.h>
# include	<cm.h>
# include	<cv.h>

# include	<metafrm.h>


/**
** Name:	ff.c -	make frame flow diagram frame
**
** Description:
**	This file has routines to dynamically create the frame flow
**	diagram frame.
**
**	This file defines:
**
**	IIVQmfnMakeFrameName	create a app unique frame name
**
** History:
**	06/15/89 (tom) - created
**	06/06/91 (tom) - change string compare so that it won't mistakenly
**			trigger postpending numbers if we already have a frame
**			called 'abc' and we attempt to create a frame with
**			'abc' as the first 3 chars. (A misunderstanding of
**			STbcompare() operation).
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

FUNC_EXTERN bool IIVQikwIsKeyWord();


/*{
** Name:	IIVQmfnMakeFrameName - form a new frame name from seed text 
**
** Description:
**	Scan the structures of the application and form a unique name.
**	If there are collisions.. then we disambiguate by postfixing
**	with a number.
**
**	*** this routine has some possible concurrancy problems..
**	    consider if some other user tries to create a frame
**	    of the same name since we snapped the application frames.
**
** Inputs:
**	char *seed;	- seed text to base the frame name on
**	char *bufr;	- callers result buffer 
**
** Outputs:
**	Returns:
**		none
**	Exceptions:
**
** Side Effects:
**
** History:
**	06/12/89 (tom) - created
**
**	02/17/93 (connie) Bug #43319, #45109, #46985
**		Allow the length of the frame name up to FE_MAXNAME
**
**      02/22/93 (connie) Bug #43319, #45109, #46985
**              For the previous fix, change to truncate at FE_MAXNAME - 4
**              to make room for tacking number in case of duplication
**	06/11/06 (kiria01) Bug #117042
**		Conform CMnmstart macro calls to relaxed bool form
*/
VOID 
IIVQmfnMakeFrameName(appl, seed, bufr)
APPL *appl;
char *seed;
char *bufr;
{
	i4 len;
	i4 len2;
	i4 tmp;
	i4 collision;
	register char *p;
	register char *q;
	register APPL_COMP *ao;
	

	p = seed; 
	q = bufr;

	/* pass invalid characters */
	while (*p != EOS && !CMnmstart(p))
		CMnext(p);

	len = 1;
	if (*p == EOS)		/* insure a good leadin character */
	{
		*q++ = '_';
	}
	else			/* copy the leadin character */
	{
		CMcpyinc(p, q);
	}

	while (*p != EOS)
	{
		if (CMnmchar(p))
		{
			CMcpyinc(p, q);

			/* increase from 12 to FE_MAXNAME - 4 */
			if (++len >= FE_MAXNAME - 4)
				break;
		}
		else
		{
			CMnext(p);
		}
	}
	*q = EOS;

	/* convert to lower case.. cause all application object names
	   are considered to be lower case */
	CVlower(bufr);

	/* test for whether the proposed name is a keyword or not..
	   if so we set up so that the collision logic will be
	   run, thus adding a number to the end */
	collision =  (IIVQikwIsKeyWord(bufr) == TRUE);


	for (ao = (APPL_COMP*) appl->comps; ao; ao = ao->_next)
	{
		/* we make the name unique amoung all objects which could
		   be in the namespace */
		switch (ao->class)
		{
		case OC_GLOBAL:
		case OC_CONST:
		case OC_HLPROC:
		case OC_OSLPROC:
		case OC_DBPROC:

		case OC_QBFFRAME:
		case OC_GRFRAME:
		case OC_GBFFRAME:
		case OC_RWFRAME:
		case OC_OSLFRAME:
		case OC_OLDOSLFRAME:
		case OC_MUFRAME:
		case OC_APPFRAME:
		case OC_UPDFRAME:
		case OC_BRWFRAME:
			/* see if there could be a collision */
			len2 = STlength(ao->name);
			if (	len2 >= len 
			   && 	STbcompare(bufr, len, ao->name, len, FALSE) == 0
			   )
			{
				if (ao->name[len] == EOS && collision == 0)
				{
					collision = 1;
				}
				else if (  CVan(ao->name + len, &tmp) == OK
					&& tmp > 0
				        && collision <= tmp
					)
				{
					collision = tmp + 1;
				}
			}
		}
	}
	/* if there was a collision, then concatenate on disabiguating number */
	if (collision > 0)
	{
		CVna(collision, q);
	}
	else
	{
		*q = EOS; 
	}
}
