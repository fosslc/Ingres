/*
** Copyright (c) 2004 Ingres Corporation
*/
# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include   <st.h>
# include	<fe.h>
# include	<adf.h>
# include	<fmt.h>
# include	<cm.h>
# include	<ug.h>
# include	<rtype.h>
# include	<rglob.h>
# include	<errw.h>

/*
**   R_NXT_SET - check for a change in the break of the current action,
**	and call r_act_set to set up TCMD structures for this command.
**	If break, set the global variables to reflect the next
**	break.	If the new break name and type do not represent a break,
**	simply set up a new Cact_tcmd structure, call r_act_set, and return.
**	Note that the Cact_attribute, Cact_type, Cact_command
**	and Cact_rtext already
**	contain the break and command information on entry.
**
**	Parameters:
**		none.
**
**	Return:
**		none.
**
**	Called by:
**		r_tcmd_set, r_m_action.
**
**	Side Effects:
**		Sets Cact_attribute, Cact_type,Cact_tname,
**			Cact_break, Cact_tcmd.
**
**	Error Messages:
**		Syserr on bad break.
**
**	Trace Flags:
**		100, 122.
**
**	History:
**		6/6/81 (ps)	written.
**		2/10/82 (ps)	incorporated most of r_tcmd_set in here.
**		11/14/85 (mgw)	Bug 6180 - disallow blocks that start in a
**				page section and don't end there. This just
**				sets up some things for the real fix in
**				r_act_set().
**		8/9/89 (elein) garnet
**			add parameter recognition for attribute names
**		06-oct-89 (sylviap)
**			Changed call from E_RW0036_r_nxt_set__Bad_break_ to
**			E_RW13F9_Bad_Break. 
**		1/12/90 (elein)
**			IIUGlbo's for IBM gateways
**			ug 9619: runtime casing required because of copyapp
**		2/7/90 (marty)
**			Coding standards cleanup.
**		3/20/90 (elein) performance
**			Change STcompare to STequal
**		05-aug-92 (rdrane)
**			The ANSI compiler complains about the infinite FOR
**			loop which will always only ever be executed once
**			(it gets around a torturous nested if and/or an
**			explicit (gasp!) goto).  In the interest of expediency,
**			convert it to a dowhile construct with a test condition
**			which is guaranteed to fail, e.g., (x != x).
**			Converted r_error() err_num value to hex to facilitate
**			lookup in errw.msg file.  Eliminate references to
**			r_syserr() and use IIUGerr() directly.  Allow for
**			delimited identifiers and normalized them before lookup.
**		9-dec-1993 (rdrane)
**			Oops!  The sort attribute name is already stored as 
**			normalized (b54950).  So, don't forget to normalize any
**			parameter specification.
**		13-march-1997(angusm)
**		        Oops again ! Previous change did not take into 
**			account situation where the value substituted for 
**			run-time parameter is longer than parameter name 
**			within report (bug 73526). Make a safe buffer
**			for value before normalising.
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/


VOID
r_nxt_set()
{
	/* internal declarations */

	ATTRIB		ordinal;		/* ordinal of break column */
	BRK		*brk;			/* fast brk pointer */
	char		*attrname;
	char		*aname;
	char		*parname;

	/* static declarations */

	static	TCMD	*otcmd = NULL;		/* last TCMD found */

	/* start of routine */


	if (!(STequal(Cact_attribute,Oact_attribute)) ||
	    !(STequal(Cact_type,Oact_type)))
	{	/* new break.  Set up the variables */
		/* preset stuff */

		r_end_type();		/* end the stack if needed */

		STcopy(Cact_attribute, Oact_attribute);
		STcopy(Cact_type, Oact_type);

		Cact_break = NULL;
		Cact_tname = NULL;

		do
		{
			r_g_set(Cact_attribute);
			if( r_g_skip() == TK_DOLLAR )
			{
				CMnext(Tokchar);
				attrname = r_g_name();
				if( (aname = r_par_get(attrname)) == NULL)
				{
					 r_error(0x3F0, NONFATAL, attrname,
						 NULL);
					 break;
				}
				/*
				** Make buffer of right length before
				** normalising - bug 73526
				*/
				parname = STalloc(aname);
				MEfill((u_i2)STlength(parname), ' ', parname);
				/*
				** Normalize any parameter name ...
				*/
				if  (IIUGdlm_ChkdlmBEobject(aname,
					parname,FALSE) == UI_BOGUS_ID)
				{
					IIUGerr(E_RW13F9_Bad_Break,
						UG_ERR_FATAL,2,
						Cact_tname,Cact_attribute);
				}
				Cact_attribute = parname;
			}
			if (STequal(Cact_attribute,NAM_REPORT))
			{
				Cact_break = Ptr_brk_top;
				break;
			}
			if (STequal(Cact_attribute,NAM_PAGE))
			{
				Cact_break = Ptr_pag_brk;
				break;
			}

			if (STequal(Cact_attribute,NAM_DETAIL))
			{
				Cact_break = Ptr_det_brk;
				break;
			}

			/* check if in sort list */
			if ((ordinal = r_mtch_att(Cact_attribute)) > 0)
			{	/* in sort list */
				for(brk=Ptr_brk_top;brk!=NULL;brk=brk->brk_below)
				{	/* go through linked list */
					if (brk->brk_attribute == ordinal)
					{
						Cact_break = brk;
						break;
					}
				}
			}
		} while (Cact_break != Cact_break);

		/* now make sure the break type is ok */

		if (STequal(Cact_type,B_HEADER))
		{
			Cact_tname = NAM_HEADER;
		}
		else
		{
			if (STequal(Cact_type,B_FOOTER))
			{
				Cact_tname = NAM_FOOTER;
			}
			else	/* bad break type */
			{
				Cact_break = NULL;
			}
		}

		/* if Cact_break set, then something good was found */

		if (Cact_break == NULL)
		{	/* something is wrong */
			IIUGerr(E_RW13F9_Bad_Break,UG_ERR_FATAL,2,
				Cact_tname,Cact_attribute);
		}

		Cact_tcmd = r_new_tcmd((TCMD *)NULL);
		otcmd = NULL;

		if (STequal(Cact_type,B_HEADER))
		{
			if (Cact_break->brk_header == NULL)
			{	/* no previous RACTION */
				Cact_break->brk_header = Cact_tcmd;
			}
			else
			{
				/* find end of previous TCMD list */
				for(otcmd=Cact_break->brk_header;
				    otcmd->tcmd_below!=NULL;
				    otcmd=otcmd->tcmd_below);
				otcmd->tcmd_below = Cact_tcmd;
			}
		}
		else		/* must be footer */
		{
			if (Cact_break->brk_footer == NULL)
			{	/* no previous RACTION */
				Cact_break->brk_footer = Cact_tcmd;
			}
			else
			{
				/* find end of previous TCMD list */
				for(otcmd=Cact_break->brk_footer;
				    otcmd->tcmd_below!=NULL;
				    otcmd=otcmd->tcmd_below);
				otcmd->tcmd_below = Cact_tcmd;
			}
		}
	}
	else
	{	/* no break found.  Simply get the next Cact_tcmd */
		otcmd = Cact_tcmd;
		Cact_tcmd = r_new_tcmd(Cact_tcmd);
	}

	r_act_set();

	return;
}
