/*
** Copyright (c) 2004 Ingres Corporation
*/
# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<si.h> 
# include	<lo.h> 
# include	<rcons.h> 
# include	<er.h>
# include	<adf.h>
# include	<fmt.h>
# include	<rtype.h>
# include	<rglob.h>

/*
**   R_FCREATE - create a file for writing.
**
**	Parameters:
**		name - file name.
**
**	Returns:
**		OK if things are ok.
**		-1 if problems occur.
**
**	Trace Flags:
**		2.0, 2.14.
**
**	Error Messages:
**		none.
**
**	History:
**		6/1/81 (ps) - written.
**		4/5/83 (mmm)- now use LOCATIONS and SI routines
**		29-jun-89 (sylviap)
**			Added loc as a parameter.  Needed for direct printing.
**		19-Feb-93 (fredb)
**			MPE/iX (hp9_mpe) needs the extra control offered by
**			SIfopen over SIopen.
**		06-Apr-93 (fredb)  hp9_mpe
**			Do some fancy coding to establish the record length
**			for the SIfopen call as suggested by rdrane.
**		09-Apr-93 (fredb)  hp9_mpe
**			Fix record length calculation. MOD ('%') operator
**			should have been integer division ('/').
**		13-Apr-93 (fredb)  hp9_mpe
**			Make further adjustments to the record length
**			calculation.  It has been shown to fail with initial
**			lengths between (MPE_SECTOR-(MPE_VRECLGTH-1)) and
**			MPE_SECTOR.  Humbug ...
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/

#ifdef hp9_mpe
/*
** MPE's file system uses a physical sector size of 256 bytes.  When the
** file type indicates variable length records, 4 bytes out of each record 
** are reserved for an actual length field.  The record can span multiple
** sectors, but the file system works best with a maximum record length that
** is a multiple of the sector size.  The actual length field must be included
** in that length.  For instance, a maximum record length of 508 plus the 4
** bytes the file system adds for the actual record length makes a "real"
** record length of 512 bytes -- exactly 2 disk sectors.
**
** The maximum allowable record size is beyond the scope of what we might
** use here (about 32k) so there is no need to establish an upper limit.
**
** The calculation for the record length to ask the O/S for is a little
** tricky, so here are the steps in plain English:
**	1) Calculate total bytes required for 1 record on disk
**		a = En_lmax + MPE_VRECLGTH;
**	2) Reduce it by 1 as part of the numerical method used here
**		a = a - 1;
**	3) Calculate the number of full sectors required
**		a = a / MPE_SECTOR;
**	4) Add 1 to account for a partial sector being used
**		a = a + 1;
**	5) Calculate the total bytes required on disk from an integral
**	   number of sectors
**		a = a * MPE_SECTOR;
**	6) Subtract the number of bytes the O/S will add automatically
**		a = a - MPE_VRECLGTH;
**
**	All together now ...
**	MPE_recsize = (((((En_lmax + MPE_VRECLGTH)-1) / MPE_SECTOR)+1) *
**		       MPE_SECTOR)-MPE_VRECLGTH;
*/

#define MPE_SECTOR   256
#define MPE_VRECLGTH  4
#endif

i4
r_fcreate(name, fp, loc)
register char	*name;
FILE		**fp;
LOCATION	*loc;
{
	/* external declarations */


	/* internal declarations */

	i4		MPE_recsize;
	STATUS		status;

	/* start of routine */



	LOfroms(PATH & FILENAME, name, loc);

#ifdef hp9_mpe
	/* calculate record size first */
	MPE_recsize = (((((En_lmax + MPE_VRECLGTH)-1) / MPE_SECTOR)+1) *
		       MPE_SECTOR)-MPE_VRECLGTH;
	status = SIfopen(loc, ERx("w"), SI_TXT, MPE_recsize, fp);
#else
	status = SIopen(loc, ERx("w"), fp);
#endif



	if (status != OK)
	{
		*fp = NULL;
		return(-1);
	}
	else
	{
		return(OK);
	}
}
