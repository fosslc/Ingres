/*
**	 Copyright (c) 2004 Ingres Corporation
*/
# include	<compat.h>
# include	<cv.h>		/* 6-x_PC_80x86 */
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<stype.h>
# include	<sglob.h>
# include	<si.h>
# include	"errc.h"
# include	<er.h>

/*
**   CR_DMP_REP - write out a set of report from a database into
**	a named file.
**
**	Parameters:
**		ownerid		2 letter code of owner of reports.
**		renlist		start of linked list of REN structs for reports.
**		fileptr		name of file to write to.  If null, to terminal.
**		makecopy	TRUE if an exact copy is to be made, including
**				the RBF commands, FALSE otherwise.
**	Returns:
**		TRUE		if everything went well.
**		FALSE		if any errors occured.
**
**	Side Effects:
**		many indirectly.
**
**	Error Messages:
**		E_RW1004_, E_RW1323_.
**
**	Called by:
**		COPYREP, rFcatalog.
**
**	History:
**		9/12/82 (ps)	written.
**		12/1/84 (rlm)	ACT reduction - add r_reset call to free memory
**		1/25/90 (elein) 
**		Create file_name large enough for LOCation.  Use that instead
**		of the fileptr.
**		13-aug-1992 (rdrane)
**			Use new constants in (s/r)_reset invocations.
**			Converted r_error() err_num value to hex to facilitate
**			lookup in errw.msg file.
*/

bool
cr_dmp_rep(ownerid,renlist,fileptr,makecopy)
char	*ownerid;
REN	*renlist;
char	*fileptr;
bool	makecopy;
{
	/* internal declarations */

	register REN		*ren,*nren;
          			/*
          			** file where the report will be written
				*/
		LOCATION  	loc;
		char 		file_name[MAX_LOC+1];
		bool		anyerror;


	/* start of routine */

	anyerror = FALSE;
	if (renlist == NULL)
	{
		return(TRUE);
	}

	/* Now open up the output file */

	if (fileptr == NULL)
	{
		En_rf_unit = Fl_stdout;		/* default */
	}
	else
	{
		fileptr = s_chk_file(fileptr);
		STcopy(fileptr, file_name);
		if (r_fcreate(file_name, &En_rf_unit, &loc) < 0)
		{	/* error in open */
			if (En_program == PRG_COPYREP)
				r_error(0x323,NONFATAL,file_name,0);
			return(FALSE);
		}
		St_rf_open = TRUE;
	}

	for (ren=renlist; ren!=NULL; ren=ren->ren_below)
	{	/* next report to this file */
		/* fix for bug 3909 */
		En_rid = 0;
		CVlower(ren->ren_report);
		ren->ren_owner = ownerid;
		CVlower(ren->ren_owner);
		nren = r_chk_rep(ren);
		if (nren == NULL)
		{	/* report not found */
			if (En_program == PRG_COPYREP)
				r_error(0x04,NONFATAL,ren->ren_report,0);
			else
				return (FALSE);
			anyerror = TRUE;
			continue;
		}
		if (!St_silent && (fileptr!=NULL))
		{	/* give message */
			if (En_program == PRG_COPYREP)
			{	/* called by COPYREP */
				SIprintf(ERget(S_RC0004_Writing_report),
					nren->ren_report,file_name);
				SIflush(stdout);
			}
		}
		if (!cr_wrt_rep(nren,En_rf_unit,makecopy))
		{	/* something wrong */
			return(FALSE);
		}

		/* do post load phase cleanup in the report writer */
		r_reset(RP_RESET_CLEANUP,RP_RESET_LIST_END);
	}

	if (St_rf_open)
	{
		r_fclose(En_rf_unit);
		St_rf_open = FALSE;
	}

	return(!anyerror);
}

