/*
**	Copyright (c) 1986, 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<si.h>		 
# include	<st.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<er.h>
#include "ioistd.h"
#include "ioifil.h"

/**
** Name:	ioiopen.c - open new image file
**
** Description:
**	open new image file to write frames to it.
*/

GLOBALDEF	i4	IIOIimageBld = 0;	/* Build Interpreted Image? */		 
     /* 0 -> no; 1 -> yes, use std iiinterp; >1 -> yes, use custom iiinterp */		 

/*{
** Name:	IIOIioImgOpen - open new image file
**
** Description:
**	open new image file to write frames to it.  This may be a legitimate
**	image file being created by the image builder, or the temp. file
**	being maintained by the ILRF.  Since the intent is to open a NEW file,
**	we catch "rw" mode and treat it specially, by creating the file first.
**
** Inputs:
**	fname	file name
**	mode	open mode
**
**		CAUTION: fname must point to enough storage for use in an
**		LOfroms call (MAX_LOC)
**
** Outputs:
**	ohdr	image file information (open file pointer)
**
**	return:
**		OK		success
**
** History:
**	9/86 (rlm)	written
**
**	30-aug-91 (leighb) Revived ImageBld:
**		Write iiinterp.exe to front of *.rtt file if INTERP2RTT
**		is defined.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

static	STATUS	writeInterpToRtt();			 

static	char	_Lbuf[MAX_LOC+1];

STATUS
IIOIioImgOpen (fname,mode,ohdr)
char *fname;
char *mode;
OLIMHDR *ohdr;
{
	/*
	** open file.  This is SI_RACC filetype so size can be backpatched
	** by DS (IS_RACC sh_type)
	*/
	STlcopy(fname, _Lbuf, sizeof(_Lbuf));
	if (LOfroms(FILENAME&PATH, _Lbuf, &(ohdr->loc)) != OK)
		return (ILE_FILOPEN);

	/*
	** for "rw", create a zero length file.
	*/
	if (STcompare(mode,ERx("rw")) == 0)
	{
		if (SIfopen(&(ohdr->loc),ERx("w"),SI_RACC,
						IMRECLEN,&(ohdr->fp)) != OK)
			return (ILE_FILOPEN);
		SIclose(ohdr->fp);
	}

	if (SIfopen(&(ohdr->loc),mode,SI_RACC,IMRECLEN,&(ohdr->fp)) != OK)
		return (ILE_FILOPEN);
	if (IIOIimageBld && (STindex(mode, "w", STlength(mode))))
		return(writeInterpToRtt(ohdr->fp));
	return (OK);
}

/*
** Name:	IIOIsvTestImageName - Save Test Image Name
**
** Description:
**	Save the path and name of file for the interpreter to use when
**	building interpreted images.  This file will be copied to the
**	beginning of the Run Time Table file if INTERP2RTT is defined.
**
** Inputs:
**	tin	path and name of file for the interpreter to use.
**
** Outputs:
**	Loads 'test_image_name'
**
**	return:
**		none.
**
** History:
**	30-aug-91 (leighb) DeskTop Porting Change: Created.
*/

static	char *	test_image_name = NULL;

VOID
IIOIsvTestImageName(tin)
char * 	tin;
{
	test_image_name = tin;
}

/*
** Name:	writeInterpToRtt - write IIINTERP.EXE to RTT file
**
** Description:
**	Copy the IIINTERP file in 'test_image_name' to the beginning of
**	the Run Time Table file if INTERP2RTT is defined.  Leave the file
**	open and positioned after the copy of IIINTERP.
**
** Inputs:
**	rttfp	File Pointer for open Run Time Table file.
**
** Outputs:
**	IIINTERP to RTT file - leaves RTT file open & positioned after IIINTERP
**
**	return:
**		STATUS = OK
**		STATUS = ILE_FILOPEN if unable to open IIINTERP file.
**
** History:
**	30-aug-91 (leighb) DeskTop Porting Change: Created.
*/

static
STATUS
writeInterpToRtt(rttfp)
FILE *	rttfp;
{
#ifdef	INTERP2RTT					 

	FILE *		interpfp;
	LOCATION	loc;
	i4		count;
	char		buf[IMRECLEN * 2 + 1];

	LOfroms(NODE & PATH & FILENAME, test_image_name, &loc);
	if (SIfopen(&loc, ERx("r"), SI_RACC, IMRECLEN * 2, &interpfp) != OK)
		return (ILE_FILOPEN);
	while (SIread(interpfp, IMRECLEN * 2, &count, buf) != ENDFILE)
	{
		SIwrite(count, buf, &count, rttfp);
	}
	SIclose(interpfp);
#endif	/* INTERP2RTT */				 
	return(OK);
}
