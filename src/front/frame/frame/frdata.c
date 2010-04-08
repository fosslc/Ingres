
/*
** Copyright (c) 2004 Ingres Corporation
*/

/* Jam hints
** 
LIBRARY = IMPFRAMELIBDATA
**
**
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ft.h>
# include	<fmt.h>
# include	<adf.h>
# include	<frame.h>
# include	<frsctblk.h>
# include	<me.h>
# include	<nm.h>
# include	<si.h>
# include       <er.h>
# include       <ds.h>
# include       <feds.h>

/*
** History:
**	29-Sep-2004 (drivi01)
**	    Added LIBRARY jam hint to put this file into IMPFRAMELIBDATA
**	    in the Jamfile.
**	19-May-2009 (kschendel) b122041
**	    The only "dsrecords" func is a void func, declare it that way here.
**
**
*/

/*
**  Data from fddstab.c
*/

# ifdef NT_GENERIC
FACILITYREF DSTEMPLATE  DSframe,
# else
GLOBALREF DSTEMPLATE    DSframe,
# endif
			DSfield,
			DSregfld,
			DSpregfld,
			DStblfld,
			DSptblfld,
			DSfldhdr,
			DSfldtype,
			DSfldval,
			DSfldcol,
			DStrim,
			DSi4,
			DSf8,
			DSchar,
                        DSstring;

GLOBALDEF DSTEMPLATE *DSTab[] = {
	&DSframe,
	&DSfield,
	&DSregfld,
	&DSpregfld,
	&DStblfld,
	&DSptblfld,
	&DSfldhdr,
	&DSfldtype,
	&DSfldval,
	&DSfldcol,
	&DStrim,
	&DSi4,
	&DSf8,
	&DSchar,
	&DSstring
};

GLOBALDEF DSTARRAY FdDStab = { sizeof(DSTab)/sizeof(DSTab[0]), DSTab };

/*
**  Data from fdmvtab.c
*/

GLOBALDEF	bool	FDvalqbf = FALSE;

/*
**  Data from formdata.c
*/

GLOBALDEF i4	_formdata_ = 0;

/*
**  Data from frcreate.qsc
*/

GLOBALDEF	i4	fdprform = 0;

/*
**  Data from getdata.c
*/

GLOBALDEF	PTR	IIFDcdCvtData = NULL;
GLOBALDEF	char	IIFDftFldTitle[fdTILEN + 1] = { 0 };
GLOBALDEF	char	IIFDfinFldInternalName[FE_MAXNAME + 1] = { 0 };

/*
**  Data from initfd.c
*/

GLOBALDEF	FRS_RFCB	*IIFDrfcb = NULL;
GLOBALDEF	FRS_GLCB	*IIFDglcb = NULL;

GLOBALDEF	FRS_EVCB	*IIFDevcb = NULL;

GLOBALDEF	i4		(*IIFDldrLastDsRow)() = NULL;
GLOBALDEF	void		(*IIFDdsrDSRecords)() = NULL;

/*
**  Data from setparse.c
*/

GLOBALDEF	i4	FDparscol = 0;
