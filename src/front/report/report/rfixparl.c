/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<me.h>
# include	<adf.h>
# include	<afe.h>
# include	<ex.h>
# include	<fmt.h>
# include	<ade.h>
# include	 <rtype.h>
# include	 <rglob.h>
# include	<si.h>
# include	<errw.h>
# include	<er.h>
# include	<ug.h>

/*
**   R_FIX_PARL - fixup the parameter list.
**		If no value is specified and -p flag has been
**		set, the user is prompted to enter a value.
**		If any values do not match their type
**		(e.g. datatype is a date and value is not a valid
**		date, or datatype is a number and value is not a valid number),
**		an error is given and the user is prompted to enter a
**		value again.
**
**	Parameters:
**		do_declared	TRUE if declared variables with prompts are to
**					be prompted for.
**				FALSE if runtime parameters are to be prompted.
**
**	Returns:
**		none.
**
**	Called by:
**		r_rep_do.
**
**	Side effects:
**		none.
**
**	Error Messages:
**		1023.
**
**	Trace Flags:
**		none.
**
**	History:
**		7/7/81 (ps)	written.
**		6/24/83 (gac)	added date templates & numeric formats.
**		1/4/84 (gac)	rewrote for expressions.
**		1/30/84 (gac)	loop until good response given.
**		2/2/84 (gac)	added date type.
**		9/26/88 (elein)	B3451-set length of parameter of no particular
**				type.  Otherwise it uses maximum.
**		19-dec-88 (sylviap)
**			Changed memory allocation to use FEreqmem.
**		7/26/89 (elein)
**			JB7212, JB5982/bugs 9638
**		02-jan-90 (sylviap)
**			Calls error 1023 (RW13FF) instead of 121 (RW1079).  Not
**			all the parameters were set when 121 was called.
**		09-jan-90 (sylviap)
**			Fixed bug so all declared variables get prompted, not
**			 just the first one.
**		2/13/90 (elein) b8377
**			Adjust the length of undeclared variables of type CHR
**			which have had their types set by prior usage.  For
**			example being used in a conditional expression.
**                      Also remove testing result of r_par_req where
**                      par->par_string might be null, except for BATCH.
**			It will never be null, except for batch, since
**			St_prompt will never be false (it is obsolete and
**			hanging around as TRUE).
**		8/8/90 (elein) with value clause change
**			Just give error, don't prompt when not prompt
**			string present.  This is the case where with value
**			was used.
**      	8/20/90 (elein) 32395
**              	Added Warning_count for initialization errors.  These
**			really are warnings and are only noted so that we can
**			allow hit return for them to be viewed.
**              8/27/90 (elein) b32654
**                      adc_minmaxdv doesn't handle nullable datatypes.
**                      Check and make nullable explicitly
**		8/29/90 (elein) b32850
**			Set signal handlers around fmt_cvt--it can call
**			adc_cvinto via afe.
**		3/11/91 (elein) b36325
**			Change size setting for undeclared CHR type variables
**			to STlength rather than STlength+1. No EOS stored.
**			The extra size didnt matter except that the format
**			generated was one char too long.
**		22-oct-1992 (rdrane)
**			Set/propagate precision to PAR DB_DATA_VALUE.
**              18-Feb-1999 (hweho01)
**                      Changed EXCONTINUE to EXCONTINUES to avoid compile 
**                      error of redefinition on AIX 4.3, it is used in 
**                      <sys/context.h>.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      14-dec-2007 (smeke01)
**	    Make WITH VALUE & WITH PROMPT work correctly together.
**	    Check to see if non-NULL return from r_par_req() is 
**	    EOS. If it isn't then we have a value to save.
**      20-dec-2007 (smeke01)
**	    Fix bug b117376 caused by fix for bug b117033.
**	    Re-cast the changes made in ffb b117033 so that only those declared 
**	    parameters with par_string set to NULL have their par_value 
**	    initialised to null via adc_getempty().
**	12-Jun-2007 (kibro01) b118486
**	    Fix bug 118486 caused by fix for bug 117376.
**	    We request a variable if either there is a prompt or it has not
**	    been declared.
**	30-Oct-2007 (kibro01) b119382
**	    Default unassigned parameters to be VARCHAR rather than CHAR
**	14-Nov-2007 (kibro01) b119487
**	    If there is no default, "" is OK
**	11-Jan-2007 (kibro01) b119729
**	    For unknown-length VCH parameters, add CNTSIZE to the given length
**	    so when we convert back we don't lose characters
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/

VOID
r_fix_parl(do_declared)
bool	do_declared;
{

	PAR	*par;
	char	*r_par_req();
	DB_DATA_VALUE	dbv;
	AFE_DCL_TXT_MACRO(256)	buffer;
	DB_TEXT_STRING	*text;
	STATUS	status;
	PTR	area;
	i4	areasize;
	FMT	*fmt;
	char	fmtstr[MAX_FMTSTR];
	i4	fmtlen;
	bool	unknowntype=FALSE; /*B3451*/
	bool	nullable;
        i4      pa_exhandler();
        EX_CONTEXT      context;
	bool	exception_seen;


	/* start of routine */

	exception_seen = FALSE;
	dbv.db_datatype = DB_LTXT_TYPE;
	dbv.db_length = sizeof(buffer);
	dbv.db_prec = 0;
	dbv.db_data = (PTR)&buffer;
	text = (DB_TEXT_STRING *)&buffer;

	for (par = Ptr_par_top; par != NULL; par = par->par_below)
	{
	    /* 7212 */
	    unknowntype=FALSE;
	    /* B5982*/
	    if (do_declared != par->par_declared)
	    /* B5982*/
	    {
		    continue;
	    }
	    else
	    {
		/* if the type of the runtime parameter has not been
		** determined by now, make it a character type. */
		/* Actually, make it varchar so we don't end up with a 32K
		** blank string if it has no default and is left blank
		** by the user (kibro01) b119382
		*/

		if (par->par_value.db_datatype == DB_NODT)
		{
		    par->par_value.db_datatype = DB_VCH_TYPE;
		    unknowntype = TRUE;
		}

		/* determine largest length of type */
		if (par->par_value.db_length == ADE_LEN_UNKNOWN)
		{
		    if( par->par_value.db_datatype < 0)
			nullable = TRUE;
		    else
			nullable = FALSE;
		    par->par_value.db_datatype=abs(par->par_value.db_datatype);
		    status = adc_minmaxdv(Adf_scb, NULL, &(par->par_value));
		    if (status != OK)
		    {
			FEafeerr(Adf_scb);
		    }
		    if( nullable)
		    	AFE_MKNULL(&(par->par_value));
		}
	    }

            if ((par->par_value.db_data = (PTR)FEreqmem ((u_i4)Rst4_tag,
		(u_i4)(par->par_value.db_length), TRUE, (STATUS *)NULL)) == NULL)
		{
			IIUGbmaBadMemoryAllocation(ERx("r_fix_parl"));
		}
											 

	    /* IF we have not set this through a command-line setting,
	    ** AND it either has a prompt (so it's supposed to be asked)
	    **     or it is a non-declared variable (which also gets asked)
	    ** THEN request a value for it.
	    ** If it is already set, then that value is left alone even if
	    ** blank, so if set to blank on the command line the default is
	    ** not used (if you want to set it to the default through the 
	    ** command line then set to that value directly).
	    ** (kibro01) b118486 (also b117033,b117376)
	    */
	    if (!par->par_set && (par->par_prompt || !par->par_declared))
	    {
                /* we have a prompt so prompt the user */
                char* value = r_par_req(par->par_name, par->par_prompt);

		/* 
		** r_par_req() returns NULL as a special error value iff 
		** called in batch mode. Error already displayed in 
		** r_par_req() so just return.
		*/
		if (value == NULL) 
		{
		    return;
		}
                else
		/*
		** r_par_req() returns EOS if user pressed
		** return without entering anything else.
		** If there is no default, "" is OK (kibro01) b119487
		*/
                if (*value != EOS || par->par_string == NULL)
                {
		    /* 
		    ** value returned was not EOS so we save value, 
		    ** overwriting any WITH VALUE default previously 
		    ** stored in par->par_string.
		    */
                    par->par_string = value;
                }
            }

	    if (par->par_string == NULL || *par->par_string == EOS)
	    {
		/* initialize declared variable without prompt string
		** to null or empty */
		adc_getempty(Adf_scb, &(par->par_value));
		continue;
	    }

	    /* B3451/B8377: If we just decided what the type was (CHR) or if it was
	    ** set to CHR earlier for this UNDECLARED variable (this could happen
	    ** if the variable were used in an expression), shrink the length to
	    ** be the same as the length of the value. (It is not going to change.)
	    ** CAVEAT:  If CHR types are restructured this will no longer work.
	    */
	    if( unknowntype ||
	    (!par->par_declared &&  par->par_value.db_datatype == DB_CHR_TYPE) )
		{
	    	par->par_value.db_length = STlength(par->par_string);
	    	par->par_value.db_prec = 0;
		if (par->par_value.db_datatype == DB_VCH_TYPE)
		    par->par_value.db_length += DB_CNTSIZE;
		}

	    /* set up default format for parameter */
	    status = fmt_deffmt(Adf_scb, &(par->par_value), 256, TRUE, fmtstr);
	    if (status != OK)
	    {
		FEafeerr(Adf_scb);
	    }

	    status = fmt_areasize(Adf_scb, fmtstr, &areasize);
	    if (status != OK)
	    {
		FEafeerr(Adf_scb);
	    }

	    area = MEreqmem(0,(u_i4)areasize,TRUE,(STATUS *)NULL);

	    status = fmt_setfmt(Adf_scb, fmtstr, area, &fmt, &fmtlen);
	    if (status != OK)
	    {
		FEafeerr(Adf_scb);
	    }

            if( EXdeclare(pa_exhandler, &context) )
            {
		 exception_seen = TRUE;
            }
	    for (;;)
	    {
		/*
		** now convert from string to parameter's DB_DATA_VALUE
		** This could raise an exception handled from above
		*/

		if( exception_seen == FALSE)
		{
			text->db_t_count = STlength(par->par_string);
			MEcopy((PTR)(par->par_string), (u_i2)(text->db_t_count),
		       	(PTR)(text->db_t_text));

			status = fmt_cvt(Adf_scb, fmt, &dbv, &(par->par_value),
					FALSE, 0);
		}
		if (status == OK && exception_seen == FALSE)
		{
		    break;
		}
		else if( par->par_prompt == NULL)
		{
		    	SIprintf(ERget(E_RW140E_),par->par_string,
							par->par_name);
			Warning_count++;
			break;
		}
		else
		{
		    /*
		    ** Exception or error found. Prompt again 
		    */
		    if( exception_seen == FALSE )
		    	FEafeerr(Adf_scb);
		    else
		    	exception_seen = FALSE; 
		    SIprintf(ERget(E_RW0026_Illegal_value_specifi));
		    SIflush(stdout);
		    par->par_string = r_par_req(par->par_name,
							par->par_prompt);
		}
	    }
            EXdelete();

	    MEfree(area);
	}
}

/*
**   PA_HANDLER - Exception handler for evaluating parameter prompts.
**		This is a clone of re_handler stashed in reitem.c
**		Only the messages changed
**
**	Parameters:
**
**	Returns:
**		none.
**
**	Called by:
**		r_fix_parl
**
**	Side effects:
**		none.
**
**	Error Messages:
**		1023.
**
**	Trace Flags:
**		none.
**
**	History:
**	8/29/90 (elein) written
*/
i4
pa_exhandler(exargs)
EX_ARGS *exargs;
{
	EX	num;

	num = exargs->exarg_num;

	if (num == EXFLTDIV || num == EXINTDIV || num == EXMNYDIV)
	{
		SIprintf(ERget(E_RW1410_));
	}
	else if (num == EXFLTOVF || num == EXINTOVF)
	{
		SIprintf(ERget(E_RW1411_));
	}
	else if (num == EXFLTUND)
	{
		SIprintf(ERget(E_RW1412_));
	}
	else
	{
		adx_handler(Adf_scb, exargs);
		if (Adf_scb->adf_errcb.ad_errcode == E_AD0001_EX_IGN_CONT)
		{
			return EXCONTINUES;
		}
		else if (Adf_scb->adf_errcb.ad_errcode == E_AD0115_EX_WRN_CONT)
		{
			while (adx_chkwarn(Adf_scb) != E_DB_OK)
			{
				FEafeerr(Adf_scb);
			}
			return EXCONTINUES;
		}
		else if (Adf_scb->adf_errcb.ad_errcode == E_AD0116_EX_UNKNOWN)
		{
			return EXRESIGNAL;
		}
		else	/* an ADF error not handled by RW */
		{
			FEafeerr(Adf_scb);
		}
	}

	return EXDECLARE;
}
