/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
# include	<nm.h>		 
#include	<me.h>
#define _calloc(size)	MEreqmem(0, (size), TRUE, (STATUS *)NULL)
#include	<ut.h>
#include	<si.h>
#include	<st.h>
#include	<lo.h>
#include	<er.h>
#include	<pc.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<ug.h>
#include	<uf.h>
#include	<adf.h>
#include	<afe.h>
#include	<feconfig.h>
#include	<abfcnsts.h>
#include	<fdesc.h>
#include	<abfrts.h>
#include	<rtsdata.h>

/**
** Name:	abrtexe.c -	ABF Run-Time Program Execution Module.
**
** Description:
**	Contains the routines that execute other programs.  Defines:
**
**	abexeprog()	execute an Ingres sub-system.
**	IIARprogram()	execute an Ingres sub-system for application.
**	IIARsystem()	execute a system command or shell.
**	IIARprint()	print a file.
**
** History:
**      12-nov-1993 (smc)
**		Bug #58882
**              Changed type of args in abexeprog() from i4 to portable PTR,
**              and removed the cast to i4 when the args were assigned.
**
**	21-Jan-93 (fredb) hp9_mpe
**		Porting changes to abexeprog and abexesys.
**
**	Revision 6.3/03/01
**	03/16/91 (emerson)
**		Fix interoperability bug 36589:
**		Change all calls to abexeprog to remove the 3rd parameter
**		so that images generated in 6.3/02 will continue to work.
**		(Generated C code may contain calls to abexeprog).
**		This parameter was introduced in 6.3/03/00, but all calls
**		to abexeprog specified it as TRUE.  Also changed abexeprog
**		itself (in this file).  Refer to it for further details.
**
**	Revision 6.3  90/03  wong
**	Increased allowed sub-system parameters to 24, and added check for any
**	exceeded.
**
**	Revision 6.0  87/10  wong
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	26-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
*/

VOID	IIARutxUTerr();

#define MAX_ARGS	24

/*{
** Name:	abexeprog() -	Execute an Ingres Sub-system.
**
** Description:
**	Execute a program based on a code.
**
** Input:
**	prog	{char *}  The Ingres sub-system (program) name.
**	fmt	{char *}  A format argument to be passed to 'UTexe()'
**	a1-a24	{PTR}  Printf style arguments which build the command line.
**
** Called by:
**	ABF or user application code.
**
** Side Effects:
**	May spawn a sub process.
**
** Error Messages:
**	ABNOPROG	Bad code passed.
**
** History:
**	Written 5/20/82 (jrc)
**	12-jun-86 (marian)	bug # 9521
**		Don't call FTrestore() in abexeprog() if a report
**		is being run because the last page is cleared.	Call
**		FTprtrestore(), print the 'hit return' message and
**		then call FTrestore()
**	08/11/86 (a.dea) -- Add abfconsts.h for ABBADTYPE definition.
**	11/04/86 (wolf)  -- bug #10300 fix to abexesys()
**	16-dec-86 (marian)	bug 10508
**		Changed abexelpr() so it now contains the printer
**		name as an argument.
**	6/17/87 (jrc) -- Changed to use 'IIUFrtm...()' for use by "abfimage".
**			(Removed all 'FTrestore()' and 'FTprtrestore()'.)
**	09-aug-89 (sylviap)
**		Added check for ING_PRINT before calling UTprint.  Also
**		added new parameters for UTprint.
**	05-jan-90 (sylviap)
**		Added new parameter to UTexe.
**		Added new parameters to PCcmdline.
**	08-mar-90 (sylviap)
**		Added CL_ERR_DESC to UTexe and PCcmdline calls.
**	03/16/91 (emerson)
**		Fix interoperability bug 36589:
**		Removed the 3rd parameter from the calling sequence
**		so that images generated in 6.3/02 will continue to work.
**		(Generated C code may contain calls to abexeprog).
**		This parameter was introduced in 6.3/03/00 (apparently
**		on 89/10/23), but all calls to abexeprog specified it as TRUE.
**		Also added logic to support images generated in 6.3/03/00
**		which *did* have this extra parameter.
**	10-sep-92 (leighb) DeskTop Porting Change: for Windows 3.x only:
**		Call IIload_interpw() to load correct data segment for DLL.
**	16-nov-92 (davel)
**		Do not call subsystem if not database session is active.
**	21-Jan-93 (fredb) hp9_mpe
**		Porting changes:
**		  (1) Added an entry to the err_map for UTENOBIN, mapping it
**		      the same as UTENOPROG.
**		  (2) Pass LOCATION containing the filename '$STDLIST' to 
**		      UTexe so that stdin and stdout will not be redirected.
*/

/*
** This maps from UTexe errors to ABF errors.
** The last element is OK which is a sentinal for a search
** loop.  The default ABF error is ABPROGERR.
*/
static struct emap
{
	STATUS	UTenum;
	i4	abenum;
	i4	argcnt;
	bool	user;
} err_map[] = {
	{UTEBIGCOM,	E_AR001B_CmdTooBig,	4,	TRUE,},	/* user error */
	{UTENOEQUAL,	E_AR001C_4GLPrmList,	4,	FALSE},	/* 4GL error */
	{UTENOPERCENT,	E_AR001C_4GLPrmList,	4,	FALSE},	/* 4GL error */
	{UTENOSPEC,	E_AR001C_4GLPrmList,	4,	FALSE},	/* 4GL error */
	{UTEBADARG,	E_AR001C_4GLPrmList,	4,	FALSE},	/* 4GL error */
	{UTEBADSPEC,	E_AR001D_UTBadSpec,	6,	FALSE},	/* UT error */
	{UTEBADTYPE,	E_AR001E_4GLPrmType,	6,	FALSE},	/* 4GL error */
	{UTENOSYM,	E_AR001F_NoAppSymbol,	4,	TRUE},	/* user error */
	{UTENOPROG,	E_AR0020_NoSubSystem,	5,	TRUE},	/* user error */
	{UTENOBIN,	E_AR0020_NoSubSystem,	5,	TRUE},	/* user error */
	{UTENoModuleType,E_AR0021_UTBadModule,	5,	FALSE},	/* UT error */
	{UTENOARG,	E_AR0022_BadParam,	5,	TRUE},	/* user error */
	OK
};

/*VARARGS4*/
STATUS
abexeprog ( prog, fmt, param_cnt,
		a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12,
		a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24,
		a25 )
char	*prog;
char	*fmt;
i4	param_cnt;
PTR	a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12,
	a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24,
	a25 /* a25 is only for calls from images created in 6.3/03/00 */;
{
	GLOBALREF char	*IIarTest;	/* Names for testing files (-Z) */
	GLOBALREF char	*IIarDebug;	/* Name of debug output file (-D) */
	GLOBALREF char	*IIarDump;	/* Name of keystroke output file (-I)*/

	char	*IIxflag();

	STATUS	rval;
	i4	argcnt = 0;
	PTR	args[MAX_ARGS+3+1];	/* + DB + -X + names + NULL */
	char	template[ABBUFSIZE];
	char	names[MAX_LOC+2+1];	/* plus flag "-Z|D|I" */
	CL_ERR_DESC	err_code;
	LOCATION	err_loc;
	char		err_locb[MAX_LOC + 1];

	/*
	** Images created by release 6.3/03/00 contain calls to abexeprog
	** with an extra parameter (called "do_rest") after the second parameter
	** ("fmt") and before the current third parameter ("param_cnt").
	** This "do_rest" parameter was always TRUE (i.e. 1).  We need to
	** detect this situation and shift the remaining parameters.
	**
	** Our life is made easier by the fact that 6.3/03/00 was released only
	** to VAX/VMS, and on VAX: (1) pointers and integers are the same size
	** and can be assigned to each other without causing an exception, and
	** (2) legitimate non-null pointers, when cast to unsigned integers,
	** cannot be less than 512.  Since the current fourth parameter
	** is a pointer, and it should always be non-null if param_cnt
	** (the current third parameter) is 1, and since param_cnt should
	** always be less than 512 (in fact, it should be no more than 24)
	** the code below should handle calls from images created in 6.3/03/00.
	*/
# ifdef VMS
	if ( param_cnt == 1 && (u_i4)a1 < 512 )
	{
		param_cnt = (u_i4)a1;
		a1 = a2;
		a2 = a3;
		a3 = a4;
		a4 = a5;
		a5 = a6;
		a6 = a7;
		a7 = a8;
		a8 = a9;
		a9 = a10;
		a10 = a11;
		a11 = a12;
		a12 = a13;
		a13 = a14;
		a14 = a15;
		a15 = a16;
		a16 = a17;
		a17 = a18;
		a18 = a19;
		a19 = a20;
		a20 = a21;
		a21 = a22;
		a22 = a23;
		a23 = a24;
		a24 = a25;
	}
# endif

	if ( param_cnt > MAX_ARGS )
		abusrerr(E_AR0027_MaxSubSysPrms, prog, (char *)NULL);

	/* if there is no current database session, fail the subsystem call */
	if ( IIarDbname == NULL )
	{
		abusrerr(E_AR0066_NoDBSession, prog, (char *)NULL);
		return FAIL;
	}

	/* set up the invariant part of the arguments. */
	STcopy(ERx("database = %S, equel = %S"), template),
	args[argcnt++] = IIarDbname;
	args[argcnt++] = IIxflag();

	/* Are we calling a forms-based program? */
	if ( STequal(prog, ERx("interp")) 
			|| STequal(prog, ERx("vifred")) 
			|| STequal(prog, ERx("qbf")) 
			|| STequal(prog, ERx("ingmenu")) 
			|| STequal(prog, ERx("tables")) 
			|| STequal(prog, ERx("rbf")) 
			|| STequal(prog, ERx("vigraph")) 
			|| STequal(prog, ERx("abf")) 
			|| STequal(prog, ERx("iquel")) 
			|| STequal(prog, ERx("isql")) 
			|| STequal(prog, ERx("rtingres")) )
	{ 
		names[0] = EOS;
		/* Check for testing files */
		if (IIarDump != NULL)
			FEinames(names);
		else if (IIarTest != NULL)
			FEtnames(names);

		if ( names[0] != EOS )
		{
			STcat(template, ERx(", flags = %S"));
			args[argcnt++] = names;
		}
	}

	param_cnt += argcnt;

	/* setup the rest of the arguments */
	if (fmt != NULL && *fmt != EOS)
	{
		STcat(template, ERx(", "));
		STcat(template, fmt);

		args[argcnt++] = a1;
		args[argcnt++] = a2;
		args[argcnt++] = a3;
		args[argcnt++] = a4;
		args[argcnt++] = a5;
		args[argcnt++] = a6;
		args[argcnt++] = a7;
		args[argcnt++] = a8;
		args[argcnt++] = a9;
		args[argcnt++] = a10;
		args[argcnt++] = a11;
		args[argcnt++] = a12;
		args[argcnt++] = a13;
		args[argcnt++] = a14;
		args[argcnt++] = a15;
		args[argcnt++] = a16;
		args[argcnt++] = a17;
		args[argcnt++] = a18;
		args[argcnt++] = a19;
		args[argcnt++] = a20;
		args[argcnt++] = a21;
		args[argcnt++] = a22;
		args[argcnt++] = a23;
		args[argcnt++] = a24;
	}
	args[argcnt] = NULL;

	IIUFrtmRestoreTerminalMode(IIUFNORMAL);

#ifdef hp9_mpe
	STcopy( ERx("$STDLIST"), err_locb);
	LOfroms( FILENAME, err_locb, &err_loc);
	rval = UTexe(UT_WAIT|UT_STRICT, &err_loc, NULL, NULL, 
			 prog, &err_code, template, param_cnt, 
			 args[0], args[1], args[2], args[3], args[4], 
			 args[5], args[6], args[7], args[8], args[9], 
			 args[10],args[11],args[12],args[13],args[14],
			 args[15],args[16],args[17],args[18],args[19],
			 args[20],args[21],args[22],args[23],args[24],
			 args[25],args[26],args[27]
		);
#else
	rval = UTexe(UT_WAIT|UT_STRICT, NULL, NULL, NULL, 
			 prog, &err_code, template, param_cnt, 
			 args[0], args[1], args[2], args[3], args[4], 
			 args[5], args[6], args[7], args[8], args[9], 
			 args[10],args[11],args[12],args[13],args[14],
			 args[15],args[16],args[17],args[18],args[19],
			 args[20],args[21],args[22],args[23],args[24],
			 args[25],args[26],args[27]
		);
#endif

#ifdef	PMFEWIN3			 
	IIload_interpw();		 
#endif					 

	if (rval == OK)
	{
		if (!STequal(prog, ERx("osl"))
					&& !STequal(prog, ERx("oslsql")))
			IIUFrtmRestoreTerminalMode(IIUFFORMS);
	}
	else
	{
		register struct emap	*em;

		for (em = err_map; em->UTenum != OK && em->UTenum != rval; ++em)
			;

		if (em->UTenum == OK)
		{
			bool	report;

			report = !(STequal(prog, ERx("osl")) ||
						STequal(prog, ERx("oslsql")) ||
						STequal(prog, ERx("sreport"))
					);

			/* report an error, showing the magic number 'rval' */
			IIARutxUTerr(prog, rval, report);

			/* 
			** We didn't find a UT error.  
			** UTexe may have returned a system status that we
			** can't handle.  So we better map it to FAIL.
			*/
			rval = FAIL;
		}
		else
		{
			/*
			** BUG 3662
			**
			** Call 'abusrerr()' with arguments in the proper order.
			*/
			register i4	i;

			char		*pbuf[3];
			char		errnobuf[30];
			char		hexerr[30];

			i = 0;
/*			if (errv != NULL)
**			{
**				register char	**ev;
**
**				ev = errv->cl_argv;
**				while (i < errv->cl_errcnt && i < 3)
**				{
**					pbuf[i++] = *ev++;
**				}
**			}
*/
			while (i < 3)
				pbuf[i++] = ERx("");

			IIUFrtmRestoreTerminalMode(IIUFFORMS);
			if ( em->user )
				abusrerr( em->abenum, prog,
					STprintf(errnobuf, ERx("%d"), rval),
					STprintf(hexerr, ERx("%x"), rval),
					pbuf[0], pbuf[1], pbuf[2], (char *)NULL
				);
			else
				IIUGerr( em->abenum, UG_ERR_ERROR, 6, prog,
					STprintf(errnobuf, ERx("%d"), rval),
					STprintf(hexerr, ERx("%x"), rval),
					pbuf[0], pbuf[1], pbuf[2]
				);
		}
	}
	return rval;
}

/*{
** Name:	IIARutxUTerr() -	Handle an ABPROGERR
**
** Description:
**	Handle the printing of an ABPROGERR.  This error occurs when
**	a system program returns an error.  If report is set to TRUE
**	then the error is reported to the user.  If report is set to
**	FALSE then the error is printed only if ERreport placed something
**	in the buffer.
**
** Inputs:
**	program		The program being run.
**	rval		The return status from the program.
**	report		Whether to always report the error.
**
** History:
**	13-jan-1986 (joe)
**		First Written.
*/
VOID
IIARutxUTerr (program, rval, report)
char	*program;
i4	rval;
bool	report;
{
	char	errbuf[ER_MAX_LEN];

	IIUFrtmRestoreTerminalMode(IIUFPROMPT);

	/*
	** HACK: we used to call ERreport here to fill errbuf.
	**
	** In 6.0, the results were usually entirely bogus.
	**
	** In the interests of not changing the logic much we will
	** simply force an empty string, and leave the check for
	** whether we want to produce a message or not alone.
	** In many instances, the message produced by this routine
	** isn't needed, but we don't want to lose the message if
	** this is the ONLY error message we're going to get.
	*/
	errbuf[0] = EOS;
	/*
	** If the status was FAIL we won't get any good information
	** so only print the error if requested AND meaningful
	*/
	if ( report || rval != FAIL )
	{
		char	errnobuf[30];
		char	hexno[30];

		abusrerr(ABPROGERR, program, STprintf(errnobuf, ERx("%d"),rval),
				STprintf(hexno, ERx("%x"), rval),
				errbuf, (char *)NULL
		);
	}
}

/*{
** Name:	IIARprogram() -	Execute Ingres Sub-system for Application.
**
** Description:
**	Execute an Ingres sub-system for a user application.  This routine
**	accepts DB_DATA_VALUEs as the parameters to be passed to the sub-system.
**
** History:
**	03/90 (jhw) -- Corrected to pass integers by value.  JupBug #20182.
*/
VOID
IIARprogram ( prog, fmt, param_cnt, args )
char		*prog;
char		*fmt;
i4		param_cnt;
DB_DATA_VALUE	args[];
{
	PTR	data = NULL;
	PTR	argv[MAX_ARGS];

	if ( param_cnt > MAX_ARGS )
		abusrerr(E_AR0027_MaxSubSysPrms, prog, (char *)NULL);

	if ( param_cnt > 0 )
	{ /* Convert DBV parameters to embedded values passed to 'UTexe()'. */
		register DB_EMBEDDED_DATA	*edv;
		register DB_DATA_VALUE		*ap;
		register i4			cnt;
		register PTR			*utarg;
		i4				size;

		ADF_CB			*cb;
		DB_EMBEDDED_DATA	edvs[MAX_ARGS];

		ADF_CB	*FEadfcb();

		cb = FEadfcb();

		/* Determine embedded types and sizes (aligned) */

		edv = edvs;
		size = 0;
		cnt = param_cnt;
		for ( ap = args ; --cnt >= 0 ; ++ap )
		{ /* Determine embedded types and sizes for each DBV actual */
			/* Integers and floats of size i4 and f8, resp.,
			** need not be converted.
			*/
			if ( ( AFE_DATATYPE(ap) != DB_INT_TYPE
					|| ap->db_length < sizeof(i4) )
				&& ( AFE_DATATYPE(ap) != DB_FLT_TYPE
					|| ap->db_length < sizeof(f8) ) )
			{ /* DBVs that must be converted ... */
				i4 align;
	
				(VOID)adh_dbtoev(cb, ap, edv);
				edv->ed_null = 0;
				if (edv->ed_type == DB_INT_TYPE)
				{
					edv->ed_length = sizeof(i4);
					if ((align = (size % sizeof(i4))) != 0)
						align = sizeof(i4) - align;
				}
				else if (edv->ed_type == DB_FLT_TYPE)
				{
					edv->ed_length = sizeof(f8);
					if ((align = size % sizeof(f8)) != 0)
						align = sizeof(f8) - align;
				}
				else
				{ /* edv->ed_type == DB_CHR_TYPE */
					size += 1;	/* allow for EOS */
					align = 0;
				}
				size += align + edv->ed_length;
				++edv;
			}
		} /* end for */

		/* Allocate data space and convert ... */

		if ((data = _calloc(size)) == NULL)
			abproerr(ERx("IIARprogram()"), OUTOFMEM, (char *)NULL);
		edv = edvs;
		utarg = argv;
		size = 0;
		cnt = param_cnt;
		for ( ap = args ; --cnt >= 0 ; ++ap )
		{ /* Allocate data space and convert each DBV actual */
			if ( AFE_DATATYPE(ap) == DB_INT_TYPE
					&& ap->db_length >= sizeof(i4) )
			{ /* integer type of size i4 */
				*utarg++ = (PTR)(SCALARP)*(i4 *)ap->db_data;
			}
			else if ( AFE_DATATYPE(ap) == DB_FLT_TYPE
					&& ap->db_length >= sizeof(f8) )
			{
				*utarg++ = ap->db_data;
			}
			else
			{
				i4 align;
	
				if ( edv->ed_type == DB_INT_TYPE )
				{
					if ((align = (size % sizeof(i4))) != 0)
						align = sizeof(i4) - align;
				}
				else if (edv->ed_type == DB_FLT_TYPE)
				{
					if ((align = size % sizeof(f8)) != 0)
						align = sizeof(f8) - align;
				}
				else
				{ /* edv->ed_type == DB_CHR_TYPE */
					align = 0;
				}
				edv->ed_data = data + size + align;
				if ( adh_dbcvtev(cb, ap, edv) != E_DB_OK )
					FEafeerr(cb);
				size += align + edv->ed_length;
				if ( edv->ed_type == DB_CHR_TYPE )
					size += 1;	/* + EOS */
				if ( edv->ed_type == DB_INT_TYPE )
					*utarg++ = (PTR)(SCALARP)*(i4 *)edv->ed_data;
				else
					*utarg++ = edv->ed_data;
				++edv;
			}
		} /* end conversion for */
	}

	_VOID_ abexeprog(prog, fmt, param_cnt, 
			argv[0], argv[1], argv[2], argv[3], argv[4],
			argv[5], argv[6], argv[7], argv[8], argv[9],
			argv[10],argv[11],argv[12],argv[13],argv[14],
			argv[15],argv[16],argv[17],argv[18],argv[19],
			argv[20],argv[21],argv[22],argv[23]
	);

	if ( data != NULL )
		MEfree((PTR) data);
}

/*{
** Name:	IIARsystem() -	Execute a System Command or Shell.
**
** Description:
**
** Input:
**	com	{char *}  System command line to execute (or execute
**				  command interpreter if NULL or empty.)
** History:
**	xx/xx/xx (xxx)
**		Written.
**	21-Jan-93 (fredb) hp9_mpe
**		Porting changes:
**		  Pass LOCATION containing the filename '$STDLIST' to 
**		  PCcmdline so that stdin and stdout will not be redirected.
*/
STATUS
abexesys (com)
char	*com;
{
	STATUS	    rval;
	CL_ERR_DESC err_code;
	LOCATION    err_loc;
	char        err_locb[MAX_LOC + 1];

	IIUFrtmRestoreTerminalMode(IIUFNORMAL);

#ifdef hp9_mpe
	STcopy( ERx("$STDLIST"), err_locb);
	LOfroms( FILENAME, err_locb, &err_loc);
	if ((rval = PCcmdline((LOCATION *)NULL, com, PC_WAIT, &err_loc, 
		&err_code)) != OK)
#else
	if ((rval = PCcmdline((LOCATION *)NULL, com, PC_WAIT, NULL, 
		&err_code)) != OK)
#endif
		IIARutxUTerr(ERx("system"), rval, FALSE);
	else
		IIUFrtmRestoreTerminalMode(IIUFMORE);

	return rval;
}

/*
** BUG 2456
** UTprint takes an argument that tells whether to delete the
** file.  Added the argument here.
*/
STATUS
IIARprint (u_print, file, delete)
char				*u_print;	/* printer to print to */
register LOCATION	*file;
bool				delete;
{
	STATUS	rval;

	IIUFrtmRestoreTerminalMode(IIUFNORMAL);

	/* bug 10508
	**	Pass the printer name to UTprint.  u_print will be NULL if
	**	no printer specified.
	*/
	if (u_print == (char *)NULL || *u_print == '\0')
	{
		/*
		** u_print is set by II_PRINT_DELETE.  If not set, then
		** check if ING_PRINT is set.
		*/
		NMgtAt(ERx("ING_PRINT"), &u_print);
	}

	if ((rval = UTprint(u_print, file, delete, 1, (char*)NULL, (char*)NULL))
			!= OK )
		IIARutxUTerr(ERx("printer"), rval, TRUE);
	else
		IIUFrtmRestoreTerminalMode(IIUFFORMS);

	return rval;
}
