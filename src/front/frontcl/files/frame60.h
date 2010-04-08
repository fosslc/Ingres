/*
**	Copyright (c) 2007 Ingres Corporation
*/

#ifndef FRAME60_H_INCLUDED
#define FRAME60_H_INCLUDED
/*
**	FRAME60.H
**
**	History
**		3/14/83 (gac)
**			basically a copy of FRAME.H except that INGTYPE
**			argument of FIELD is changed to char * so it can be
**			initialized as UNIX's C compiler won't allow union
**			initialization.
**		12/7/84 (bab)
**			added definitions of VTREE[2,3], VALROOT,
**			VALLIST[2,3] so that validation parse trees can be
**			saved in compiled-form files.  Changed the v_right
**			union in VTREE to be a char * for initialization.
**			Also created multiple copies of VTREE and VALLIST
**			so that unions could be replaced by the various
**			datatypes that they store.
**
**			BE AWARE:  VTREE(2,3) and VALIST(2,3) have extra
**			unused pad sections so as to ensure that they
**			are exactly the same size as VTREE and VALLIST
**			respectively.  These pads will work AS LONG AS
**			the alignment works properly, and none of the
**			data types are larger than the original doubles.
**			IF THIS MACHINE DOESN'T MEET THESE REQUIREMENTS,
**			THIS FILE WILL NEED TO BE CHANGED.
**
**		12/11/84 (agh)
**			Added definition of GLOBALDEF (as nothing) so
**			that compiled forms can look the same on VMS and Unix.
**
**		6/28/85 (dkh)
**			Put back casts for VTREE and VALLIST.
**
**		11/23/85 (cgd)
**			On 'make install' this file is now specially
**			modified so that it is shippable to customers.
**			The include of compat.h and SI.h is changed to
**			an include of stdio.h.  Thus only one version
**			is needed.
**		03/07/87 (dkh) - Changed for 6.0.
**		06/18/88 (dkh) - Integrated changes from CMS.
**	08-mar-1999 (hanch04)
**	    Replace long with int.
**	11-Oct-2007 (kiria01) b118421
**	    Include guard against repeated inclusion.
*/

# ifndef GLOBALDEF
# define GLOBALDEF
# endif


# define	CAST_PCHAR	(char *)
# define	CAST_PFIELD	(FIELD **)
# define	CAST_PTRIM	(TRIM **)


typedef struct trmstr
{
	int	trmx, trmy;
	int	trmflags;
	int	trm2flags;
	int	trmfont;
	int	trmptsz;
	char	*trmstr;
} TRIM;


typedef struct fldhdrstr
{
	char	fhdname[36];
	int	fhseq;
	char	*fhscr;
	char	*fhtitle;
	int	fhtitx,fhtity;
	int	fhposx,fhposy;
	int	fhmaxx,fhmaxy;
	int	fhintrp;
	int	fhdflags;
	int	fhd2flags;
	int	fhdfont;
	int	fhdptsz;
	int	fhenint;
	char	*fhpar;
	char	*fhsib;
	char	*fhdfuture;
	char	*fhd2fu;
} FLDHDR;

typedef struct fldtypinfo
{
	char	*ftdefault;
	int	ftlength;
	int	ftprec;
	int	ftwidth;
	int	ftdatax;
	int	ftdataln;
	int	ftdatatype;
	int	ftoper;
	char	*ftvalstr;
	char	*ftvalmsg;
	char	*ftvalchk;
	char	*ftfmtstr;
	char	*ftfmt;
	char	*ftfu;
} FLDTYPE;

typedef struct fldval
{
	char	*fvdbv;
	char	*fvdsdbv;
	char	*fvbufr;
	char	*fvdatawin;
	int	fvdatay;
} FLDVAL;

typedef struct regfldstr
{
	FLDHDR	flhdr;
	FLDTYPE fltype;
	FLDVAL  flval;
} REGFLD;

typedef struct tblcolstr
{
	FLDHDR	flhdr;
	FLDTYPE fltype;
} FLDCOL;

typedef struct tflfldstr
{
	FLDHDR	tfhdr;
	int	tfscrup,
		tfscrdown,
    		tfdelete,
    		tfinsert;
    	int	tfstate;
    	int	tfputrow;
	int	tfrows;
	int	tfcurrow;
	int	tfcols;
	int	tfcurcol;
    	int	tfstart;
    	int	tfwidth;
    	int	tflastrow;
	FLDCOL	**tfflds;
	FLDVAL	*tfwins;
	char	*tf1pad;
    	char	*tf2pad;
	int	*tffflags;
	int	tfnscr;
	char	*tffuture;
	char	*tf2fu;
} TBLFLD;

typedef struct fldstr
{
	int	fltag;
	char	*fld_var;
} FIELD;



typedef struct frmstr
{
	char	frfiller[4];
	int	frversion;
	char	frname[36];
	FIELD	**frfld;
	int	frfldno;
	FIELD	**frnsfld;
	int	frnsno;
	TRIM	**frtrim;
	int	frtrimno;
	char	*frscr;
	char	*unscr;
	char	*untwo;
	int	frrngtag;
	char	*frrngptr;
	int	frintrp;
	int	frmaxx,frmaxy;
	int	frposx,frposy;
	int	frmode;
	int	frchange;
	int	frnumber;
	int	frcurfld;
	char	*frcurnm;
	int	frmflags;
	int	frm2flags;
	char	*frres1;
	char	*frres2;
	char	*frfuture;
	int	frscrtype;
	int	frscrxmax;
	int	frscrymax;
	int	frscrxdpi;
	int	frscrydpi;
	int	frtag;
} FRAME;

#endif /* FRAME60_H_INCLUDED */
