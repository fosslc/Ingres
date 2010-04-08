/*
**	fttermcap.c
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	ftgrt.c - General routine to get termcap capabilities.
**
** Description:
**	A general routine to get termcap capabilities.
**
** History:
**	08/01/87 (dkh) - Initial version.
**	12/24/87 (dkh) - Changes to TDtgetent interface.
**	09/23/89 (dkh) - Porting changes integration.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

# include	<compat.h>
# include	<cv.h>		/* 6-x_PC_80x86 */
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<nm.h> 
# include	<er.h>
# include	<lo.h>


FUNC_EXTERN	i4	TDtgetent();
FUNC_EXTERN	i4	TDtgetnum();
FUNC_EXTERN	char	*TDtgetstr();
FUNC_EXTERN	bool	TDtgetflag();
FUNC_EXTERN	VOID	TDtcaploc();

static	char		capspace[2048];

/*{
** Name:	IIFTgrtGetRungraphTcap - Get termcap entries for rungraph.
**
** Description:
**	This is a routine used by "rungraph" to obtain graphics
**	capabilities from the termcap file.  Basic scheme is that
**	rungraph passes down list boolean, numeric and string
**	capabilities and address of where to place the capabilites.
**	Somewhat similar to QUEL param statements.
**
**	Warning - It is assumed that the passed in address are valid
**	and that the number of capabilites match the number of addresses.
**
** Inputs:
**	bcaps	List of boolean capabilities.
**	ncaps	List of numeric capabilities.
**	scaps	List of string capabilities.
**
** Outputs:
**	bargv	Boolean capabilities are set to what's in termcap.
**	nargv	Numeric capabilities are set to what's in termcap.
**	sargv	String capabilities are set to what's in termcap.
**
**	Returns:
**		OK	Terminal type (as defined by TERM_INGRES)
**			found in termcap file.
**		FAIL	Unknown terminal type or termcap file could not
**			be opened.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	08/01/87 (dkh) - Initial version.
**	17-Jun-2004 (schka24)
**	    Safe env var handling.
*/
STATUS
IIFTgrtGetRungraphTcap(bcaps, bargv, ncaps, nargv, scaps, sargv)
char	*bcaps;
bool	**bargv;
char	*ncaps;
i4	**nargv;
char	*scaps;
char	***sargv;
{
	char		*namp;
	char		***sp;
	char		*aoftspace = capspace;
	bool		**bp;
	i4		**np;
	char		genbuf[2048];
	char		termbuf[100];
	char		*terminal;
	LOCATION	tloc;
	char		locbuf[MAX_LOC + 1];

	NMgtAt(ERx("TERM_INGRES"), &terminal);
	if (terminal == NULL || *terminal == EOS)
	{
		NMgtAt(TERM_TYPE, &terminal);
	}
	if (terminal == NULL || *terminal == EOS)
	{
		return(FAIL);
	}
	STlcopy(terminal, termbuf, sizeof(termbuf)-1);
	CVlower(termbuf);

	TDtcaploc(&tloc, locbuf);

	if(TDtgetent(genbuf, termbuf, &tloc) != 1)
	{
		return(FAIL);
	}

	/*
	**  Get Boolean capabilities.
	*/
	if (bcaps != NULL && bargv != NULL)
	{
		namp = bcaps;
		bp = bargv;
		do
		{
			**bp = TDtgetflag(namp);
			bp++;
			namp += 2;
		} while (*namp);
	}

	/*
	**  Get Numeric capabilities.
	*/
	if (ncaps != NULL && nargv != NULL)
	{
		namp = ncaps;
		np = nargv;
		do
		{
			**np = TDtgetnum(namp);
			np++;
			namp += 2;
		} while (*namp);
	}

	/*
	**  Get String capabilities.
	*/
	if (scaps != NULL && sargv != NULL)
	{
		namp = scaps;
		sp = sargv;
		do
		{
			**sp = TDtgetstr(namp, &aoftspace);
			sp++;
			namp += 2;
		} while (*namp);
	}

	return(OK);
}
