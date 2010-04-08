/*
**	oframe.h
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	oframe.h - Old frame structures.
**
** Description:
**	This file contains the "OLD" (5.0 and earlier) version
**	of the frame strucutres.  Needed for compiled form
**	conversions.
**
** History:
**	02/03/87 (dkh) - Initial version.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/


/* Pointers to OLD INGRES data types */
typedef union oingtype
{
	char	*i1_type;
	i2	*i2_type;
	i4	*i4_type;
	f4	*f4_type;
	f8	*f8_type;
	char	*str_type;
} OINGTYPE;


/* OLD TRIM STRUCTURE */
typedef struct otrmstr
{
	i4	trmx;		/* x coordinate of trim start position */
	i4	trmy;		/* y coordinate of trim start position */
	char	*trmstr;	/* trim string */
} OTRIM;


/* FIELD STRUCTURE */
typedef struct ofldhdrstr
{
	char	fhdname[16];		/* field name */
	i4	fhseq;			/* field sequence number */
	FTINTRN	*fhscr;			/* display window for screen */
	char	*fhtitle;		/* title for field */
	i4	fhtitx,fhtity;		/* position of title in field */
	i4	fhposx,fhposy;		/* position of field window */
	i4	fhmaxx,fhmaxy;		/* number of cols and lines */
	i4	fhintrp;		/* interrupt code for field */
	i4	fhdflags;		/* flags (standout,box,etc.) */
					/* for a table field header this is a */
					/* mode field taken from fdtf values */
} OFLDHDR;

typedef struct ofldtypinfo
{
	char	*ftdefault;		/* char default value */
	i4	ftlength;		/* length of data type */
	i4	ftwidth;		/* width of field */
	i4	ftdatax;		/* position of data string */
	i4	ftdataln;		/* max. line length for data */
	i4	ftdatatype;		/* data type for field */
	i4	ftoper;			/* operator for query mode */
	char	*ftvalstr;		/* validation string */
	char	*ftvalmsg;		/* message returned after valid */
	VTREE	*ftvalchk;		/* validation check tree */
	char	*ftfmtstr;		/* format string */
	FMT	*ftfmt;			/* output format */
} OFLDTYPE;

typedef struct ofldval
{
	char		*fvstore;	/* storage for field, was ING_TYPE */
	char		*fvbufr;	/* store char representation */
	FTINTRN		*fvdatawin;	/* display window for data */
	i4		fvdatay;	/* data window's y coordinate */
} OFLDVAL;

typedef struct oregfldstr
{
	OFLDHDR	flhdr;
	OFLDTYPE fltype;
	OFLDVAL  flval;
} OREGFLD;

typedef struct otblcolstr
{
	OFLDHDR	flhdr;
	OFLDTYPE fltype;
} OFLDCOL;


typedef struct otflfldstr
{
	OFLDHDR	tfhdr;			/* table field header information */
	i4	tfscrup,		/* code to return on scroll up */
		tfscrdown,		/* code to return on scroll down  */
    		tfdelete,		/* code to return when deleted */
    		tfinsert;		/* code to return when insert */
    	i4	tfstate;		/* state of table field */
    	i4	tfputrow;		/* a row has been input from the user*/
	i4	tfrows;			/* number of rows displayed */
	i4	tfcurrow;		/* current row of table field on */
	i4	tfcols;			/* number of columns */
	i4	tfcurcol;		/* current column on */
    	i4	tfstart;		/* line number of start of data rows */
    	i4	tfwidth;		/* number of physical lines per row */
    	i4	tflastrow;		/* row # of last actual row displayed */
	OFLDCOL	**tfflds;		/* an array of pointers to columns */
	OFLDVAL	*tfwins;		/* two dimensional array of values */
	OFLDVAL	*tftoprow;		/* extra row at top */
    	OFLDVAL	*tfbotrow;		/* extra row at bottom */
} OTBLFLD;


typedef struct ofldstr
{
	i4	fltag;			/* type of fields this is */
	union				/* union of regular field and table */
	{
		OREGFLD	*flregfld;
		OTBLFLD	*fltblfld;
	} fld_var;
} OFIELD;


/* FRAME STRUCTURE */
typedef struct ofrmstr
{
	char	frname[16];		/* frame name */
	OFIELD	**frfld;		/* array of fields */
	i4	frfldno;		/* number of fields */
	OFIELD	**frnsfld;		/* array of non-seq. fields */
	i4	frnsno;			/* number of fields */
	OTRIM	**frtrim;		/* frame trim strings */
	i4	frtrimno;		/* number of trim strings */
	FTINTRN	*frscr;			/* screen representation */
	FTINTRN	*unscr;			/* undo screen */
	FTINTRN	*untwo;			/* undo the undo screen */
	i4	frundo;			/* flag indicating a change */
	i4	frunfld;		/* field changed */
	i4	frunx;			/* x coordinate of change */
	i4	fruny;			/* y coordinate of change */
	i4	frintrp;		/* frame interrupt code */
	i4	frmaxx,frmaxy;		/* frame size */
	i4	frposx,frposy;		/* frame position */
	i4	frmode;			/* current frame driver mode */
	i4	frchange;		/* flag for any changes in frame */
	i4	frnumber;		/* number returned with intrp */
	i4	frcurfld;		/* cur fld after interrupts */
	char	*frcurnm;		/* fld name after interrupts */
	i4	frmflags;		/* flags (box,standout,etc.) */
} OFRAME;
