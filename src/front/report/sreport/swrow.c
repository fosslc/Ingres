/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<cv.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<stype.h>
# include	<sglob.h>
# include	<rglob.h>
# include	<nm.h>
# include	<lo.h>
# include	<er.h>
# include	<st.h>
# include	<ug.h>
# include	"ersr.h"

/*{
** Name:	s_w_row() -	Output Report Command Row.
**
** Description:
**   S_W_ROW - write out the current fields in COMMAND to a file for later
**	use by the QUEL copy.
**
** Parameters:
**	none.
**
** Returns:
**	STATUS. OK   -- If successful.
**		FAIL -- If not successful.
**
** Side Effects:
**	May set St_rco_open, En_rcommands.
**
** Called by:
**	Many routines.
**
** Trace Flags:
**	13.0, 13.5.
**
** History:
**	6/1/81 (ps)	written.
**	12/22/81 (ps)	modified for two file version.
**	6/14/84 (gac)	added network stuff for equel copy.
**	7/17/85 (drh)	Increment En_qcount for sql queries.
**	5/08/86 (jhw)	Removed 'En_temp_loc' global; it was used incorrectly.
**	8/30/89 (elein)	Added parameter to r_fcreate
**	1/25/90 (elein) Ensure file name buffers have the same scope as loc
**	2/16/90 (martym) Changed to return STATUS and not abort when out of 
**			file space and the program running is RBF.
**      3/20/90 (elein) performance
**                      Change STcompare to STequal
**	10/15/90 (steveh) Split out code which created En_rcommands allowing
**			this code to be called elsewhere (bug 32954)
**	26-aug-1992 (rdrane)
**			Converted r_error() err_num value to hex to facilitate
**			lookup in errw.msg file. Eliminate references to
**			r_syserr() and use IIUGerr() directly.  Use established
**			#define for IIUGerr() parameterization.
**	23-feb-1993 (rdrane)
**		Re-parameterize r_wrt_eol() invocation to include LN structure
**		pointer (changed due to fix for q0 format bugs 46393 and
**		46713).
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/

STATUS
s_w_row()
{
	char	line[MAXFLINE+1];	/* buf to hold text line for file */
	char	sbuffer[20];		/* hold ascii version of i4  */

	/* start of routine */
	make_En_rcommands();

	/* increment sequence information */

	Csequence++;
	if (STequal(Ctype,NAM_ACTION) )
	{	/* ACTION types */
		Cact_ren->ren_acount++;
		if (STequal(Csection,B_HEADER) )
		{
			if (Cact_ren->ren_bcount > 0)
			{
				Cact_sbr->sbr_sqheader++;
			}
			else
			{
				Cact_rso->rso_sqheader++;
			}
		}
		else if (STequal(Csection,B_FOOTER) )
		{
			if (Cact_ren->ren_bcount > 0)
			{
				Cact_sbr->sbr_sqfooter++;
			}
			else
			{
				Cact_rso->rso_sqfooter++;
			}
		}
	}
	else if (STequal(Ctype, NAM_QUERY)  ||
			STequal(Ctype, NAM_SQL) )
	{	/* QUERY types */
		Cact_ren->ren_qcount++;
	}
	else if (STequal(Ctype, NAM_SORT) )
	{
		Cact_ren->ren_scount++;
	}
	else if (STequal(Ctype, NAM_BREAK) )
	{
		Cact_ren->ren_bcount++;
	}


	/* concatenate the RACTION text file line */

	CVna(Cact_ren->ren_id, sbuffer);
	STcopy(sbuffer, line);
	STpolycat(2, line, F_DELIM, line);
	STpolycat(2, line, Ctype, line);
	STpolycat(2, line, F_DELIM, line);
	CVna(Csequence,sbuffer);			/* sequence number */
	STpolycat(2, line, sbuffer, line);
	STpolycat(2, line, F_DELIM, line);
	STpolycat(2, line, Csection, line);
	STpolycat(2, line, F_DELIM, line);
	STpolycat(2, line, Cattid, line);
	STpolycat(2, line, F_DELIM, line);
	STpolycat(2, line, Ccommand, line);
	STpolycat(2, line, F_DELIM, line);
	STpolycat(2, line, Ctext, line);

	/* now write it out */

	if (r_wrt_eol(Fl_rcommand,line,(LN *)NULL) < 0)
	{
		if (En_program != PRG_RBF)
		{
			IIUGerr(E_SR0013_s_w_row_Bad_write,UG_ERR_FATAL,0);
		}
		else
		{
			IIUGerr(E_SR0017_s_w_row_Write_failed,UG_ERR_ERROR,1,
				En_rcommands);
			return(FAIL);
		}
	}

	return(OK);
}


/*{
** Name:	make_En_rcommands - construct file name for En_rcommands
**
** Description:
**   		This routine constructs the file name for En_rcommands.
**		No operation takes place if already called.
**
** Parameters:
**	none.
**
** Returns:
**	VOID
**
** Side Effects:
**	May set St_rco_open, En_rcommands.
**
** Called by:
**	s_w_row
**
** History:
**	10-15-90 (steveh)
**		Pulled code out of s_w_row so that it could be called
**		from other locations.
*/

VOID
make_En_rcommands()
{
	if (!St_rco_open)
	{
		LOCATION	temp_loc;
		char		buf[MAX_LOC+1];
		char		*bufp;

		/* First set up a file name for the RCOMMANDS file */

		buf[0] = EOS;
		LOfroms(PATH & FILENAME, buf, &temp_loc);
		LOuniq(FN_RCOMMAND, ERx("tmp"), &temp_loc);
		NMloc(TEMP, FILENAME, buf, &temp_loc);
		LOtos(&temp_loc, &bufp);
		STcopy(bufp,En_rcommands);

		if (r_fcreate(En_rcommands, &Fl_rcommand, &temp_loc) < 0)
		{
			r_error(0x398, FATAL, En_rcommands, NULL); /* ABORT! */
		}

		St_rco_open = TRUE;
	}

	return;
}

