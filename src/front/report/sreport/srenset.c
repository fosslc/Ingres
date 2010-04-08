/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<cv.h>		/* 6-x_PC_80x86 */
# include	<si.h>
# include	<me.h>
# include	<st.h>
# include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<stype.h>
# include	<sglob.h>
# include	<ui.h>
# include	<oodefine.h>
# include	<ooclass.h>
# include	"ersr.h"

/*{
**   S_REN_SET - set up for a new report specification.	 It sets
**	up a REN structure for this report, and starts the linked lists.
**
**	Parameters:
**		rname	name of report (may be too long).
**		rtype	type of report (either "s" or "f", for sreport
**			or forms, respectively).
**
**	Returns:
**		none.
**
**	Called by:
**		s_nam_set, rFrep_set.
**
**	Side Effects:
**		Sets up the global variable Ptr_ren_top on first call.
**		Sets up the flags St_q_started, St_o_given,
**		St_s_given for this report.
**
**	Trace Flags:
**		13.0, 13.2.
**
**	Error Messages:
**		none.
**
** History:
**	4/4/82 (ps)	written.
**	09-feb-1987 (rdesmond)
**		changed call to IIOOidCheck to search report superclass.
**	03-mar-1988 (rdesmond)
**		added end transaction.
**	26-aug-1988 (danielt)
**		changed DB_MAXNAME to FE_MAXNAME
**		changed literal "now" to call to UGcat_now()		
**	10-jan-1989 (danielt) OO concurrency changes
**	19-jan-1989 (jhw)  Use input OOID or 'IIOOidCheck()' to determine if
**		object exists, not just the latter.  (The previous logic was
**		clearing the remarks and date, which was wrong!)
**	20-jan-1992 (rdrane)
**		Use new constants in (s/r)_reset invocations.
**		Demote to a plain vanilla ".c" file.
**	13-sep-1993 (rdrane)
**		Add defaulting of report level in the REN structure.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	23-Aug-2009 (kschendel) 121804
**	    Define IIOOnewId.
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/

char    *UGcat_now();
FUNC_EXTERN OOID IIOOnewId(void);

VOID
s_ren_set(rname,rtype, r_id)
char	*rname;
char	*rtype;
OOID     r_id;
{
	REN	*ren;			/* temp ptr to a REN struct */

	/* start of routine */

	/* initialize global vars */
	s_reset(RP_RESET_REPORT,RP_RESET_LIST_END);

	CVlower(rname);

	if (STlength(rname) > FE_MAXNAME)
	{	/* truncate the name to FE_MAXNAME chars */
		*(rname+FE_MAXNAME) = '\0';
	}	/* truncate the name to FE_MAXNAME chars */

	ren = (REN *) MEreqmem(0, (u_i4) sizeof(REN), TRUE, (STATUS *) NULL);

	if (Cact_ren == NULL)
	{	/* first one */
		Ptr_ren_top = ren;
	}	/* first one */
	else
	{	/* not the first one */
		Cact_ren->ren_below = ren;
	}

	Cact_ren = ren;

	/*
	** if -zfilename flag specified for REPORT, don't do catalog access as
	** report specification is in the given file.
	*/
	if (!St_ispec)
	{
	    OOID	repclass;

	    repclass = OC_REPORT;
	    if ( (ren->ren_id = r_id) <= 0 &&
			IIOOidCheck(&repclass, &(ren->ren_id), rname, Usercode)
				!= OK )
	    { /* report does not exist or not owned by user */
                ren->ren_id = IIOOnewId();
		ren->ren_shortrem = ren->ren_longrem = ERx("");
		ren->ren_created = STalloc(UGcat_now());	
	    }
	    else
	    { /* report exists. get previous remarks */
		register OO_OBJECT	*obj;

		obj = OOp(ren->ren_id);
		ren->ren_shortrem = obj->short_remark;
		ren->ren_longrem = obj->long_remark;
		ren->ren_created = obj->create_date;
	    }
	}

	ren->ren_report = (char *) STalloc(rname);
	ren->ren_owner = (char *) STalloc(Usercode);
	ren->ren_type = (char *) STalloc(rtype);
	ren->ren_modified = STalloc(UGcat_now());	
	ren->ren_level = LVL_MAX_REPORT;

	/* set up starts for other fields */

	ren->ren_acount = 0;
	ren->ren_scount = 0;
	ren->ren_bcount = 0;
	ren->ren_qcount = 0;

	/* print out message to user */

	if (!St_silent && (En_program != PRG_RBF))
	{
		SIprintf(ERget(S_SR000B_Start_of_report),En_sfile,rname);
		SIflush(stdout);
	}

	/* allocate space for RSO structures */

	s_rso_add(NAM_REPORT,FALSE);		/* add report structure */
	s_rso_add(NAM_PAGE,FALSE);		/* add page structure */
	s_rso_add(NAM_DETAIL,FALSE);		/* add detail structure */

	/* allocate space for SBR structures */

	s_sbr_add(NAM_REPORT);
	s_sbr_add(NAM_PAGE);
	s_sbr_add(NAM_DETAIL);

	/* Set up start to point to report header */

	s_rso_set(NAM_REPORT,B_HEADER);
	s_sbr_set(NAM_REPORT,B_HEADER);

	En_n_sorted = 0;			/* just to keep it uptodate */
	En_n_breaks = 0;

	return;
}
