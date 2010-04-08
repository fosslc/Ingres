/*
**	Copyright (c) 2007 Ingres Corporation
*/

#ifndef FRAME2_H_INCLUDED
#define FRAME2_H_INCLUDED
/*
**	FRAME2.H
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
**
**		06/14/88 (sandyd)
**			Integrated CMS change from Steve Wolf which removed
**			the special ifdef for "WSC".
**
**		11/29/90 (sandyd)
**			Removed bogus "#include <si.h>", since it was not
**			needed and si.h is not available at customer sites.
**			(This change was thoroughly discussed with Dave Hung 
**			and Bob Andrews, and Dave tested the change with a
**			5.0 form to verify that <si.h> is unnecessary.)
**	08-mar-1999 (hanch04)
**	    Replaced long with int.
**	11-Oct-2007 (kiria01) b118421
**	    Include guard against repeated inclusion.
*/


# ifndef GLOBALDEF
# define GLOBALDEF
# endif



/*
**  Special defines to handle casts in compiled forms for IBM. (dkh)
*/
# define	CAST_PCHAR	(char *)
# define	CAST_PFIELD	(FIELD **)
# define	CAST_PTRIM	(TRIM **)
# define	CAST_PVTREE	(struct vtree *)
# define	CAST_PVALST	(struct vallist *)


/* Pointers to INGRES data types */

/*
** have 3 structs so that C compilation will be able to recreate
** the data with the proper type (double, int, char *).
*/
typedef struct vallist
{
	int		listtag;	/* DS_ID for listdata */
	struct vallist	*listnext;
	double		listdata;	/* storage for double's */
} VALLIST;

typedef struct valist2
{
	int		listtag;	/* DS_ID for listdata */
	struct vallist	*listnext;
	int		listdata;	/* storage for int's */
	char		pad[sizeof(double) - sizeof(int)];
} VALIST2;

typedef struct valist3
{
	int		listtag;	/* DS_ID for listdata */
	struct vallist	*listnext;
	char		*listdata;	/* storage for char *'s */
	char		pad[sizeof(double) - sizeof(char *)];
} VALIST3;

typedef	struct	valroot
{
	struct vallist	*listroot;
	int		listtype;
} VALROOT;

/* VALIDATION struct created by parser */
/*
** have 3 structs so that C compilation will be able to recreated
** the data with the proper type (double's, int's, char *'s).
*/
typedef struct vtree
{
	int		datatag;	/* DS_ID for v_data */
	int		ptrtag;		/* DS_ID for v_right */
	int		v_datatype;
	int		v_nodetype;
	struct 	vtree	*v_left;
	char		*v_right;	/* changed union to char *. (bab) */
	double		v_data;		/* storage for double's */
} VTREE;

typedef struct vtree2
{
	int		datatag;	/* DS_ID for v_data */
	int		ptrtag;		/* DS_ID for v_right */
	int		v_datatype;
	int		v_nodetype;
	struct 	vtree	*v_left;
	char		*v_right;	/* changed union to char *. (bab) */
	int		v_data;		/* storage for int's */
	char		pad[sizeof(double) - sizeof(int)];
} VTREE2;

typedef struct vtree3
{
	int		datatag;	/* DS_ID for v_data */
	int		ptrtag;		/* DS_ID for v_right */
	int		v_datatype;
	int		v_nodetype;
	struct 	vtree	*v_left;
	char		*v_right;	/* changed union to char *. (bab) */
	char		*v_data;	/* storage for pointers */
	char		pad[sizeof(double) - sizeof(char *)];
} VTREE3;

typedef struct trmstr
{
	int	trmx, trmy;		/* position of trim */
	char	*trmstr;		/* trim string */
} TRIM;

/* FIELD STRUCTURE */

typedef struct fldhdrstr
{
	char	fhdname[16];		/* field name */
	int	fhseq;			/* field sequence number */
	char	*fhscr;			/* display window for screen */
	char	*fhtitle;		/* title for field */
	int	fhtitx,fhtity;		/* position of title in field */
	int	fhposx,fhposy;		/* position of field window */
	int	fhmaxx,fhmaxy;		/* number of cols and lines */
	int	fhintrp;		/* interrupt code for field */
	int	fhdflags;		/* flags (standout,box,etc.) */
					/* for a table field header this is */
					/* a mode field taken from fdtf
					** values */
} FLDHDR;

typedef struct fldtypinfo
{
	char	*ftdefault;		/* char default value */
	int	ftlength;		/* length of data type */
	int	ftwidth;		/* width of field */
	int	ftdatax;		/* position of data string */
	int	ftdataln;		/* max. line length for data */
	int	ftdatatype;		/* data type for field */
	int	ftoper;			/* operator for query mode */
	char	*ftvalstr;		/* validation string */
	char	*ftvalmsg;		/* message returned after valid */
	char	*ftvalchk;		/* validation check tree */
	char	*ftfmtstr;		/* format string */
	char	*ftfmt;			/* output format */
} FLDTYPE;

typedef struct fldval
{
	char	*fvstore;		/* storage for field
					** GAC changed INGTYPE to char *.  */
	char	*fvbufr;		/* store char representation */
	char	*fvdatawin;		/* display window for data */
	int	fvdatay;		/* data window's y coordinate */
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
	FLDHDR	tfhdr;			/* table field header information */
	int	tfscrup,		/* code to return on scroll up */
		tfscrdown,		/* code to return on scroll down  */
    		tfdelete,		/* code to return when deleted */
    		tfinsert;		/* code to return when insert */
    	int	tfstate;		/* state of table field */
    	int	tfputrow;		/* a row has been input from the user*/
	int	tfrows;			/* number of rows displayed */
	int	tfcurrow;		/* current row of table field on */
	int	tfcols;			/* number of columns */
	int	tfcurcol;		/* current column on */
    	int	tfstart;		/* line number of start of data rows */
    	int	tfwidth;		/* number of physical lines per row */
    	int	tflastrow;		/* row number of last actual row
					** displayed */
	FLDCOL	**tfflds;		/* an array of pointers to columns */
	FLDVAL	*tfwins;		/* two dimensional array of values */
	FLDVAL	*tftoprow;		/* extra row at top */
    	FLDVAL	*tfbotrow;		/* extra row at bottom */
} TBLFLD;

typedef struct fldstr
{
	int	fltag;			/* type of fields this is */
	char	*fld_var;	/* GAC changed union to char *.  */
} FIELD;


/* FRAME STRUCTURE */

typedef struct frmstr
{
	char	frname[16];		/* frame name */
	FIELD	**frfld;		/* array of fields */
	int	frfldno;		/* number of fields */
	FIELD	**frnsfld;		/* array of non-seq. fields */
	int	frnsno;			/* number of fields */
	TRIM	**frtrim;		/* frame trim strings */
	int	frtrimno;		/* number of trim strings */
	char	*frscr;			/* screen representation */
	char	*unscr;			/* undo screen */
	char	*untwo;			/* undo the undo screen */
	int	frundo;			/* flag indicating a change */
	int	frunfld;		/* field changed */
	int	frunx;			/* x coordinate of change */
	int	fruny;			/* y coordinate of change */
	int	frintrp;		/* frame interrupt code */
	int	frmaxx,frmaxy;		/* frame size */
	int	frposx,frposy;		/* frame position */
	int	frmode;			/* current frame driver mode */
	int	frchange;		/* flag for any changes in frame */
	int	frnumber;		/* number returned with intrp */
	int	frcurfld;		/* cur fld after interrupts */
	char	*frcurnm;		/* fld name after interrupts */
	int	frmflags;		/* flags (box,standout,etc.) */
} FRAME;

#endif /* FRAME2_H_INCLUDED */
