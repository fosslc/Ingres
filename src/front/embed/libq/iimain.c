/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<pc.h>		 
# include	<st.h>		 
# include	<er.h>
# include	<si.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ug.h>
# include	<erlq.h>

/**
+*  Name: iimain.c - Open channels for user programs.
**
**  Defines:
**	IImain - Open user channels.
-*
**  History:
**      07-oct-93 (sandyd)
**          Added <fe.h>, which is now required by <ug.h>.
**/

/*{
+*  Name: IImain - Open run-time channels.
**
**  Description:
**	Open standard channels for user programs.
**	Open up Stdin, Stdout, Stderr.  For runtime errors, status info etc.
**	For an internal front-end written on the CL, these are automatically
**	opened earlier.  For an arbitrary EQUEL program, they are opened here.
**
**  Inputs:
**	None
**
**  Outputs:
**	Returns:
**	    Nothing
**	Errors:
**	    Errors reported from SIeqinit.
**
**  Side Effects:
**	
**  History:
**	10-oct-1986	- Rewrote. (ncg)
**	20-aug-92 (leighb) DeskTop Porting Change: Chg SIprintf to SIfprintf.
*/

/* Should be large enough for "IImain: Cannot open ..." msg in any language */
# define II_SILEN 100	

void
IImain()
{
    char	errbuf[ER_MAX_LEN +1];
    STATUS	stat;
    char	*cp;		

    if (!SIisopen(stdout))
    {
	if ((stat = SIeqinit()) != OK)
	{
	    /* At this point we are not yet in INGRES */
	    IIUGfmt(errbuf, II_SILEN, ERget(S_LQ0211_SINOTOPEN), 1, 
		    ERx("IImain"));
	    cp = errbuf + STlength(errbuf);
	    *cp++ = '\n';
	    ERreport(stat, cp);
	    SIfprintf(stderr, errbuf);		 
	    PCexit(-1);
	}
    }
}
