/*
**  FTframe.h
**
**  Copyright (c) 2004 Ingres Corporation
**
**  History:
**	03/04/87 (dkh) - Added support for ADTs.
**	05/02/87 (dkh) - Changed to call FD routines via passed pointers.
**	07/23/88 (dkh) - Added exec sql immediate hook.
**	02-aug-89 (bruceb)
**		Added agg hooks;
**	10/19/89 (dkh) - Added declarations to remove duplicate FT
**			 files in GT directory.
**	12/27/89 (dkh) - Added call back routine pointer for goto checking.
**	05/22/90 (dkh) - Defined MWS to enable MACWS changes.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/*
**	The following include file is different in the FT directory
**	than in the GT directory.  Since some files are linked to GT
**	from FT, this provides a method for keeping the FTframe.h the
**	same between them.
*/
# include	<gtdef.h>

# define	FTINTRN		WINDOW

# define	MWS		1

# include	<termdr.h>
# include	<tdkeys.h>
# include	<ft.h>
# include	<fmt.h>
# include	<adf.h>
# include	<frame.h>

GLOBALREF	i2	froperation[];
GLOBALREF	i1	fropflg[];

GLOBALREF	WINDOW	*FTwin;
GLOBALREF	WINDOW	*FTcurwin;
GLOBALREF	WINDOW	*FTutilwin;
GLOBALREF	i4	FTwmaxy;
GLOBALREF	i4	FTwmaxx;

GLOBALREF	FRAME	*FTiofrm;

GLOBALREF	FLDHDR	*(*FTgethdr)();
GLOBALREF	FLDTYPE	*(*FTgettype)();
GLOBALREF	FLDVAL	*(*FTgetval)();
GLOBALREF	VOID	(*FTgeterr)();
GLOBALREF	STATUS	(*FTrngchk)();
GLOBALREF	i4	(*FTvalfld)();
GLOBALREF	VOID	(*FTdmpmsg)();
GLOBALREF	VOID	(*FTdmpcur)();
GLOBALREF	i4	(*FTfieldval)();
GLOBALREF	VOID	(*FTsqlexec)();
GLOBALREF	VOID	(*IIFTiaInvalidateAgg)();
GLOBALREF	VOID	(*IIFTpadProcessAggDeps)();
GLOBALREF	VOID	(*IIFTiaaInvalidAllAggs)();
GLOBALREF	i4	(*IIFTgtcGoToChk)();

GLOBALREF	u_i2	FTgtflg;
GLOBALREF	VOID	(*iigtclrfunc)();
GLOBALREF	VOID	(*iigtreffunc)();
GLOBALREF	VOID	(*iigtctlfunc)();
GLOBALREF	VOID	(*iigtsupdfunc)();
GLOBALREF	bool	(*iigtmreffunc)();
GLOBALREF	VOID	(*iigtslimfunc)();
GLOBALREF	i4	(*iigteditfunc)();
