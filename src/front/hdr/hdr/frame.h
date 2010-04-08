/*
**	Copyright (c) 2004 Ingres Corporation
*/
# ifndef	FRAME_H_INCLUDED
# define	FRAME_H_INCLUDED

# ifndef ADE_OPERAND
# ifdef int_w32
# include	<gl.h>
# endif /* int_w32 */
# include	<ade.h>
# endif

# include	<adf.h>
# include	<ft.h>
# include	<fmt.h>

/*
**  frame.h
**
**  MODIFIED
**
**  4/11/84 (jrc)
**    Made frame structure word aligned for compatibility.
**
**  5-7-85 (dkh)
**    Added new field flags for lower/upper case, (no)autotab and noecho.
**
**  08/19/85 - DKH - Added new field flag fdTFINVIS for invisible tbl fld.
**  12/22/86 - (dkh) - Added support for new activations.
**  01/19/87 - (dkh) Changed FRAME structure for long name/ADT support.
**  16-mar-87 (bruceb)
**	added fdREVDIR and fdopSPACE for support of right-to-left fields.
**	fdREVDIR indicates that field is RL.  fdopSPACE is returned in
**	BROWSE mode when the space key is pressed (ascii only) as opposed
**	to the old return of fdopRIGHT.
**  03/19/87 (dkh) - Added constants fdREVDIR, fdNOCOLSEP and fdopSPACE.
**  04/07/87 (dkh) - Changed validation structures for ADT support.
**  05/02/87 (dkh) - Added definition of fdROWHLIT.
**  06/01/87 (dkh) - Code cleanup.
**  24-sep-87 (bruceb)
**	Added constant fdopSHELL, for the shell function key.
**  16-nov-87 (bruceb)
**	Added constant fdSCRLFD for scrolling fields.
**	Comment vifred's runtime-only use of ftoper.
**  10-feb-88 (bruceb)
**	Changed fdSCRLFD value from 020000000000 to 0100.  It's now set
**	in fhd2flags (and frmflags) instead of fhdflags.  Changed because
**	copyform dies when the most significant bit of an int (such as
**	fhdflags) is on.
**  01/23/88 (dkh) - Added constants fdTILEN, fdVSTRLEN, fdTRIMLEN,
**		     fdVMSGLEN, fdDEFLEN and fdFMTLEN.
**  02/27/88 (dkh) - Added definition of fdopNXITEM and CMD_NXITEM.
**  04/09/88 (dkh) - Added VENUS support.
**  05/03/88 (dkh) - Added return codes for FTsyncdisp().
**  05/18/88 (dkh) - Added fd2SCRFLD flag.
**  06/14/88 (sandyd) - Integrated IBM change from Emerson, which just
**	removed the duplicate definition of "fdREVDIR".
**  11/09/88 (dkh) - Added flag fdNEED_TO_FORMAT.
**  12/01/88 (dkh) - Changed fdNEED_TO_FORMAT to fdXREFVAL.
**  19-dec-88 (bruceb)
**	Added fdREADONLY and fdINVISIBLE (identical value to fdTFINVIS).
**  10-jan-89 (bruceb)
**	Added fdREBUILD as a frame flag.  If this flag is set, FTbuild
**	will rebuild everything.
**  30-jan-89 (bruceb)
**	Changed FRMVERSION from 6 to 7 so as to identify for the users
**	which forms will work with fdREADONLY and fdINVISIBLE.
**  01-mar-89 (bruceb)
**	Defined FRRES2 for use with entry activations.
**	Defined BADFLDNO (-1) for use with entry activation.
**  17-mar-89 (wolf)
**	Bracket definition of fdSCRLFD with #ifdef FT3270
**  06-apr-89 (bruceb)
**	Added fdVQLOCK as a field flag.  If this flag is set (by VQ) then
**	disallow certain actions in VIFRED.
**  20-jun-89 (bruceb)
**	Additions for derived fields.
**	Defined BADCOLNO (-1).  Defined D_VERSION 8--frame version derived
**	fields were introduced.  Added fdDERIVED to indicate a derived field.
**	Added fdVISITED for use in runtime processing of derived fields.
**	Added fdINVALID, fdNOCHG and fdCHNG (values, not flags).
**  06/23/89 (dkh) - Derived fields support changes.
**  06-jul-89 (bruceb)
**	Added some #defines for use in compiling derived field code.
**  07/07/89 (dkh) - Added # defines for internal hooks.
**  07/12/89 (dkh) - Added # defines for internal hooks.
**  07/12/89 (dkh) - Removed # defines for internal hooks.  Done in "dem" path.
**  07/31/89 (dkh) - Added support for derived field aggregates.
**  08/04/89 (dkh) - Added definition of fdNSCRWID and fdWSCRWID to
**		     support 80/132 runtime switching.
**  09/01/89 (dkh) - Put ifndef around include of ade.h to reduce problems
**		     with source files including both ade.h and frame.h.
**		     Note that this only works if the source file includes
**		     ade.h first.
**  19-sep-89 (bruceb)
**	Re-define fdTFINVIS to 010 (same as fdKPREV, which isn't used for
**	table fields), and now use to indicate that all table field columns
**	are invisible.
**  09/28/89 (dkh) - Added fdSPDRAW so big forms show up better the first
**		     time they are displayed.
**  11/24/89 (dkh) - Added support for goto's from wview.
**  12/19/89 (dkh) - VMS shared lib changes.
**  12/27/89 (dkh) - Added support for hot spot trim.
**  12/30/89 (dkh) - Added definition of fdDISPRBLD.
**  01-feb-90 (bruceb)
**	Added definition of fd1CHRFIND.
**  19-mar-90 (bruceb)
**	Added locator support.  Includes definition of FHOTSPOT, FSCRUP and
**	FSCRDN and new entry to FRRES2 struct.
**  04/06/90 (dkh) - Added support for allowing user to get off newly
**		     opened row at end of a table field.
**  07-apr-90 (esd)
**	Remove '#ifdef FT3270' around definition of fdSCRLFD
**  16-apr-90 (bruceb)
**	Added fdCLRMAND in frmflags to indicate that FDvalidate should
**	generate a different error message than usual for an empty,
**	mandatory field.
**  24-apr-90 (bruceb)
**	Added fdopSELECT to support 'mouse click to select' for listpicks.
**	Added frmaxy to frres2 for locator support.
**  09-jun-90 (esd)
**	Add #define for fdFRMSTRTCHD.
**  07/25/90 (dkh) - Added various constant definitions for table field
**		     cell highlighting support.
**  22-aug-90 (esd)
**	Change FRMVERSION to 9 and add #define for FRMSTRTCHD_VERSION.
**  02/05/91 (dkh) - Added support for 4gl table field qualification
**		     attributes tfDISP_EMPTY and dsDISP_EMPTY.
**  03/20/91 (dkh) - Added support for (alerter) event in FRS.
**  08/11/91 (dkh) - Added fdISDYN to aid in reclaiming all memory
**		     allocated for a dynamically created form.
**  08/11/91 (dkh) - Added fdIN_SCRLL and fdNO_MI_SEL to block out alerter
**		     events when no menuitem was selected or for a
**		     column/scroll activation combination.
**  08/18/91 (dkh) - Added fdVIS_CMPVW (to indicate a vision compressed
**		     view form) to fix bug 38752.
**  04/29/92 (dkh) - Changed macro fdtrctime to keep ANSI C happy.
**  06/05/92 (dkh) - Added flags fdLANDABLE and fdUSEEMASK for 6.5.
**  06/14/92 (dkh) - Added support for decimal datatype for 6.5.
**	18-sep-92 (kirke)
**	    Changed name of fdtrctime() macro substitution variable from
**	    s to str so that the compiler doesn't get confused by the %s.
**  10/23/92 (rdrane) - Added #define for flprec for decimal datatype for 6.5
**			and declaration of FDgethdr();
**  10/28/92 (rdrane) - Back out addition of #define for flprec for decimal
**			datatype.  It made everything nicely consistent, but
**			violated CL coding standards (the existing fl*
**			#defines had been grandfathered).
**  12/12/92 (dkh) - Added flag fdEMASK so that user can control on a field
**		     basis whether edit masks are to be used or not on input.
**  02/23/93 (mohan) - Added tags to structures for making automatic prototypes.
**  12-mar-93 (fraser)
**	    Changed structure tag name so that it doesn't begin with
**	    underscore.
**  11-jan-1996 (toumi01; from 1.1 axp_osf port)
**          Added dkhor's change to FRAMEUNION for axp_osf
**          10-sep-93 (dkhor)
**          On axp_osf, pointers are 8 bytes.  fui4 is sometimes used
**          to hold a pointer.  Declare this as PTR.
**	09-apr-1996 (chech02)
**	   Windows 3.1 port changes - add more function prototypes.
**	02-may-1997 (cohmi01)
**	    Add fdopSCRTO for MWS scroll bar button positioning.
**	10-dec-2003 (rigka01)
**	    include sl.h (and gl.h) for inclusion of systypes.h which includes
**	    sys\types.h as required by the fix to bug 111096, change 465421
**	    for ade.h use of off_t (OFFSET_TYPE) for opr_offset in ADE_OPERAND 
**	21-jan-03 (hayke02)
**	    Make the above change specific to int_w32.
**	21-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
**	    Drop the WIN16 section, misleading and out of date.  Better to
**	    just reconstruct as needed.
*/

/* Structure to handle coercion of a field's datatype */

typedef struct vattrcv
{
	ADI_FI_ID	vc_fiid;
	DB_DATA_VALUE	*vc_dbv;
} VATTRCV;

/* Validation data types */
typedef union vdatstr
{
	i4		v_oper;
	i4		v_res;
	struct	fldstr	*v_fld;
	DB_DATA_VALUE	v_dbv;
} VDATA;

/* LIST of VALID ENTRIES */
typedef struct vallist
{
	struct vallist	*listnext;
	DB_DATA_VALUE	vl_dbv;
	ADI_FI_ID	v_lfiid;
	DB_DATA_VALUE	*left_dbv;
	ADI_FI_ID	left_cfid;
} VALLIST;

typedef	struct	valroot
{
	struct vallist	*listroot;
	i4		listtype;
	i4		vr_flags;

# define	IS_TBL_LOOKUP	1	/* flag to indicate tbl lookup list */

	DB_DATA_VALUE	v_listdbv;
} VALROOT;

/* VALIDATION struct created by parser */
typedef struct vtree
{
	i4		v_nodetype;
	i4		v_constype;
	ADI_FI_ID	v_fiid;
	struct 	vtree	*v_left;
	union
	{
		struct 	vtree	*v_treright;
		struct 	valroot	*v_lstright;
		struct	vattrcv	*v_atcvt;
	} v_right;
	union	vdatstr	v_data;
} VTREE;

/* TRIM STRUCTURE */
typedef struct trmstr
{
	i4	trmx;		/* x coordinate of trim start position */
	i4	trmy;		/* y coordinate of trim start position */
	i4	trmflags;	/* NEW - flags for trim */
	i4	trm2flags;	/* NEW - more flags for trim */
	i4	trmfont;	/* NEW - font information */
	i4	trmptsz;	/* NEW - point size information */
	char	*trmstr;	/* trim string */
} TRIM;

/* FIELD STRUCTURE */
typedef struct fldhdrstr
{
	char	fhdname[36];		/* CHANGED - field name */
	i4	fhseq;			/* field sequence number */
	PTR	*fhscr;			/* display window for screen */
	char	*fhtitle;		/* title for field */
	i4	fhtitx,fhtity;		/* position of title in field */
	i4	fhposx,fhposy;		/* position of field window */
	i4	fhmaxx,fhmaxy;		/* number of cols and lines */
	i4	fhintrp;		/* interrupt code for field */
	i4	fhdflags;		/* flags (standout,box,etc.) */
					/* for a table field header this is a */
					/* mode field taken from fdtf values */
	i4	fhd2flags;		/* more space for flags */
	i4	fhdfont;		/* NEW - font information */
	i4	fhdptsz;		/* NEW - point size information */
	i4	fhenint;		/* entry activation interrupt code */
	PTR	fhpar;			/* NEW */
	PTR	fhsib;			/* NEW */
	struct	derive 	*fhdrv;		/* Derived field support information.
					** Can be NULL if not a derived
					** field or a derivation source field.
					*/
	PTR	fhd2fu;			/* NEW */
} FLDHDR;

typedef struct fldtypinfo
{
	char	*ftdefault;		/* char default value */
	i4	ftlength;		/* length of data type */
	i4	ftprec;			/* NEW - precision of data type */
	i4	ftwidth;		/* width of field */
	i4	ftdatax;		/* position of data string */
	i4	ftdataln;		/* max. line length for data */
	i4	ftdatatype;		/* data type for field */
	i4	ftoper;			/* operator for query mode */
					/*
					** Also used in Vifred as a temporary
					** per-field location for storing the
					** scroll buffer length.
					*/
	char	*ftvalstr;		/* validation string */
	char	*ftvalmsg;		/* message returned after valid */
	VTREE	*ftvalchk;		/* validation check tree */
	char	*ftfmtstr;		/* format string */
	FMT	*ftfmt;			/* output format */
	PTR	ftfu;			/* NEW */
} FLDTYPE;

typedef struct fldval
{
	DB_DATA_VALUE	*fvdbv;		/* CHANGED - dbv storage for field */
	DB_DATA_VALUE	*fvdsdbv;	/* NEW - dbv for display buffer */
	u_char		*fvbufr;	/* store char representation */
	PTR		*fvdatawin;	/* display window for data */
	i4		fvdatay;	/* data window's y coordinate */
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
    	i4	tflastrow;		/* row number of last actual row displayed */
	FLDCOL	**tfflds;		/* an array of pointers to columns */
	FLDVAL	*tfwins;		/* two dimensional array of values */
	PTR	tf1pad;			/* was extra row at top */
    	PTR	tf2pad;			/* was extra row at bottom */
	i4	*tffflags;		/* NEW - flags for cells in tf */
	i4	tfnscr;			/* NEW */
	PTR	tffuture;		/* NEW - future expansion */
	PTR	tf2fu;			/* NEW */
} TBLFLD;

/*
** a macro to help access tfwins
*/
# define	fdtblacc(tbl, row, col)  (tbl)->tfwins + ((row) * (tbl)->tfcols) + (col)

typedef struct fldstr
{
	i4	fltag;			/* type of fields this is */
	union				/* union of regular field and table */
	{
		REGFLD	*flregfld;
		TBLFLD	*fltblfld;
	} fld_var;
} FIELD;

# define	fldname		fld_var.flregfld->flhdr.fhdname
# define	flseq		fld_var.flregfld->flhdr.fhseq
# define	flstore		fld_var.flregfld->flval.fvstore
# define	flbufr		fld_var.flregfld->flval.fvbufr
# define	fldefault	fld_var.flregfld->fltype.ftdefault
# define	fllength	fld_var.flregfld->fltype.ftlength
# define	flscr		fld_var.flregfld->flhdr.fhscr
# define	flposx		fld_var.flregfld->flhdr.fhposx
# define	flposy		fld_var.flregfld->flhdr.fhposy
# define	flmaxx		fld_var.flregfld->flhdr.fhmaxx
# define	flmaxy		fld_var.flregfld->flhdr.fhmaxy
# define	fltitle		fld_var.flregfld->flhdr.fhtitle
# define	fltitx		fld_var.flregfld->flhdr.fhtitx
# define	fltity		fld_var.flregfld->flhdr.fhtity
# define	flwidth		fld_var.flregfld->fltype.ftwidth
# define	fldatawin	fld_var.flregfld->flval.fvdatawin
# define	fldatax		fld_var.flregfld->fltype.ftdatax
# define	fldatay		fld_var.flregfld->flval.fvdatay
# define	fldataln	fld_var.flregfld->fltype.ftdataln
# define	fldatatype	fld_var.flregfld->fltype.ftdatatype
# define	fldflags	fld_var.flregfld->flhdr.fhdflags
# define	floper		fld_var.flregfld->fltype.ftoper
# define	flintrp		fld_var.flregfld->flhdr.fhintrp
# define	flvalstr	fld_var.flregfld->fltype.ftvalstr
# define	flvalmsg	fld_var.flregfld->fltype.ftvalmsg
# define	flvalchk	fld_var.flregfld->fltype.ftvalchk
# define	flfmtstr	fld_var.flregfld->fltype.ftfmtstr
# define	flfmt		fld_var.flregfld->fltype.ftfmt

/* Expansion struct for FRAME's; pointed to by frres2. */
typedef struct frres2
{
	/* Used, at runtime only, as part of entry activation code. */
	i4	origfld;
	i4	origrow;
	i4	origcol;
	PTR	origdsrow;	/* dataset row.  Really a TBROW */
	bool	savetblpos;	/* set to TRUE in FT if scroll attempt */
				/* changes position without guaranteeing */
				/* successful scroll */

	/* Used, as part of derivation code. */
	i4	rownum;		/* If TF, then # if visible or NOTVISIBLE. */
	PTR	dsrow;		/* Points to dataset row if NOTVISIBLE. */
	i4	formmode;	/* Form mode a la runtime. (i.e. fdrtQRY) */
# define	fdrtQUERY	5	/* Same as fdrtQRY in runtime.h. */

	/* Used for runtime locator support. */
	PTR	coords;		/* Points to array of 'location' pointers. */
	i4	frmaxy;		/* Maxy length of form; for form resizing. */
} FRRES2;

# define	BADFLDNO	(-1)	/* setting of origfld for EA */

/* FRAME STRUCTURE */
typedef struct frmstr
{
	char	frfiller[4];		/* NEW - filler space for change */
	i4	frversion;		/* NEW - version number of struct */
	char	frname[36];		/* CHANGED - frame name */
	FIELD	**frfld;		/* array of fields */
	i4	frfldno;		/* number of fields */
	FIELD	**frnsfld;		/* array of non-seq. fields */
	i4	frnsno;			/* number of fields */
	TRIM	**frtrim;		/* frame trim strings */
	i4	frtrimno;		/* number of trim strings */
	FTINTRN	*frscr;			/* screen representation */
	FTINTRN	*unscr;			/* undo screen */
	FTINTRN	*untwo;			/* undo the undo screen */
	i4	frrngtag;		/* tag for range query allocation */
	PTR	frrngptr;		/* pointer to range query structs */
	i4	frintrp;		/* frame interrupt code */
	i4	frmaxx,frmaxy;		/* frame size */
	i4	frposx,frposy;		/* frame position */
	i4	frmode;			/* current frame driver mode */
	i4	frchange;		/* flag for any changes in frame */
	i4	frnumber;		/* number returned with intrp */
	i4	frcurfld;		/* cur fld after interrupts */
	char	*frcurnm;		/* fld name after interrupts */
	i4	frmflags;		/* flags (box,standout,etc.) */
	i4	frm2flags;		/* NEW - more flag space */
	PTR	frres1;			/* NEW - reserved for future use */
	FRRES2	*frres2;		/* NEW - for EA and any future use */
	PTR	frfuture;		/* NEW - future expansion */
	i4	frscrtype;		/* NEW - screen type (alpha/bit) */
	i4	frscrxmax;		/* NEW - max horizontal positions */
	i4	frscrymax;		/* NEW - max vertical positions */
	i4	frscrxdpi;		/* NEW - x screen resolution */
	i4	frscrydpi;		/* NEW - y screen resolution */
	i4	frtag;			/* NEW - alloc tag for form */
} FRAME;


/*}
**  Name:	DRVREL - Derived field relationship structure.
**
**  Description:
**	Derived field relationship structure.
**	Convienent repository for keeping track of the list of (source) fields
**	that a particular derived field depends on or list of (dependent) fields
**	that depend on a particular source field.  Note that a derived field
**	is allowed to be a source field for another derived field.
**	The sequence number of a table field column is stored in member "colno".
**
**  History:
**	06/23/89 (dkh) - Initial version.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	24-Feb-2010 (frima01) Bug 122490
**	    Add function prototypes as neccessary
**	    to eliminate gcc 4.3 warnings.
*/
typedef struct drvrel
{
	FIELD 		*fld;	/* Source/dependent field pointer */
	i4		colno;	/* Sequence number of source/dependent column
				** if above field is a table field.  A value
				** of BADCOLNO (i.e., no column number) if
				** field is a simple field.
				*/
# define	BADCOLNO	(-1)	/* setting of colno for simple fields */

	struct drvrel	*next;	/* Pointer to another source/dependent field */
} DRVREL;

# define	NOTVISIBLE	(-1)	/* TF row number if row not visible. */


/*}
**  Name:	DAGGVAL - Aggregate value structure.
**
**  Description:
**	Structure to support calculating and saving a derivation
**	aggregate value.
**
**  History:
**	07/12/89 (dkh) - Initial version.
*/
typedef struct dagg_val
{
	i4		agvflags;	/* flags for aggregate value */
	DB_DATA_VALUE	aggdbv;		/* DBV for aggregate result */
	ADF_AG_STRUCT	adfagstr;	/* structure for running aggregate */
	ADE_EXCB	*agg_excb;	/* CX for aggregate */
} DAGGVAL;


/*}
**  Name:	DRVAGG - Derived field aggregate support strucutre.
**
**  Description:
**	Derived Field aggregate structure.
**	This structure holds aggregate values for a particular column
**	of a table field.  Each of the possible aggregate values
**	are stored in there own separate pointer.
**	Member "aggflags" contains information as to whether the current
**	values are valid.
**
**  History:
**	07/12/89 (dkh) - Initial version.
*/
typedef struct drvagg
{
	i4	aggflags;		/* flags for storing attributes */

# define	AGG_ISVALID	01	/* aggregate value is valid */

	DAGGVAL	*cntagg;		/* COUNT aggregate value */
	DAGGVAL	*maxagg;		/* MAX aggregate value */
	DAGGVAL	*minagg;		/* MIN aggregate value */
	DAGGVAL	*sumagg;		/* SUM aggregate value */
	DAGGVAL	*avgagg;		/* AVG aggregate value */
} DRVAGG;


/*}
**  Name:	DERIVE - Derived field support structure.
**
**  Description:
**	Top level data structure to support derived fields.
**	One of these structures exists (and is pointed at by a FLDHDR)
**	if a field/column is a derived field or a source for a derived field.
**
**  History:
**	06/23/89 (dkh) - Initial vesrion.
**	07/12/89 (dkh) - Modified for aggregate support.
**	08/07/89 (dkh) - Modified for CX support.
*/
typedef struct derive
{
	DRVREL	*srclist;	/* List of source fields for a derived field. */
	DRVREL	*deplist;	/* List of fields that depend
				** on a source field.
				*/
	DRVAGG	*agginfo;	/* Running aggregate values for a table field
				** column.  This can be NULL if there are no
				** derived fields that depend on the aggregate
				** value of this particular column.
				*/
	i4	drvflags;	/* Attributes of a derivation formula */

# define	fhDRVCNST	01	/* derivation is all constants */
# define	fhDRVDCNST	02	/* constant derivation already done */
# define	fhDRVAGGDEP	04	/* field depends on a derived agg */
# define	fhDRVTFCDEP	010	/* field depends on a tf column */

} DERIVE;


/* Union to facilitate passing (FRAME *) parameters to ##addform as (i4) */
typedef union s_FRAMEUNION {
	PTR	fui4;
	FRAME	*fuframe;
} FRAMEUNION;
# define	FRAMECAST(f)	(_framecast.fuframe=f, _framecast.fui4)

FUNC_EXTERN	TRIM	*FDnewtrim();
FUNC_EXTERN	FIELD	*FDnewfld();
FUNC_EXTERN	FRAME	*FDnewfrm();
FUNC_EXTERN	char    *FDGFName();
FUNC_EXTERN	char    *FDGCName();
FUNC_EXTERN	FIELD	*FDfind();
FUNC_EXTERN	FMT	*FDchkFmt();
FUNC_EXTERN	FMT	*f_chkfmt();
FUNC_EXTERN	FLDCOL	*FDnewcol();
FUNC_EXTERN	FLDHDR	*FDgethdr();

/*
**  Some definitions of character string lengths declared in the
**  database catalogs.
*/
# define	fdTILEN		50	/* max length of title string */
# define	fdVSTRLEN	240	/* max length of valdation string */
# define	fdTRIMLEN	150	/* max length of trim string */
# define	fdVMSGLEN	100	/* max length of val message string */
# define	fdDEFLEN	50	/* max length of default value string */
# define	fdFMTLEN	50	/* max length of format string */

/*
**  Magic values for intercepting keystrokes.
*/
# define        FRS_PROCESS     0       /* Let the FRS process the keystroke */
# define        FRS_IGNORE      1       /* Drop keystroke in /dev/null */
# define        FRS_RETURN      2       /* Drop keystroke and exit via frskey0*/

# define        FUNCTION_KEY    0       /* keystroke type was a function key */
# define        CONTROL_KEY     1       /* keystroke type was a control key */
# define        NORM_KEY        2       /* keystroke type was a normal char */

/*
**  Current version for forms data structures.
*/
# define	FRMVERSION	9

/* Version in which derived field support was added. */
# define	D_VERSION	8

/*
**  Number of first version that keeps the fdFRMSTRCHD bit accurate.
*/
# define	FRMSTRTCHD_VERSION	9

/* FRAME flags */
# define	fdBOXFR		01	/* Popup is boxed */
/*
**  Value 02 (fdTRHSPOT) indicates that there are hot spot
**  trim in the form.
*/
# define	fdDISPRBLD	04	/* rebuild display */
# define	fdEDTMD		010	/* unused */
# define	fdBRWSMD	020	/* unused */
# define	fdRNGMD		040	/* Form supports field sideway scroll */
/*
**  Value 0100 (fdSCRLFD) indicates form has scrolling fields.
*/
# define	fdISPOPUP	0200	/* form is a popup */
# define	fd2SCRFLD	0400	/* Secondary scroll flag to use with
					** QBF range query mode.
					*/
# define	fdREBUILD	01000	/* Entirely rebuild the form. */
# define	fdINVALAGG	02000	/* Agg derivations are invalid. */
# define	fdNSCRWID	04000	/* Display form in narrow mode */
# define	fdWSCRWID	010000	/* Display form in wide mode */
/* Value 040000 (fdREDRAW) indicates form needs to be redrawn */
# define	fdSPDRAW	0100000	/* unused - special handling for big forms */
# define        fdIN_SCRLL      0200000         /* In middle of tf scrolling */
# define        fdNO_MI_SEL     0400000         /* No menuitems selected */
# define        fdISDYN         01000000        /* form dynamically created */
# define        fdVIS_CMPVW	02000000        /* is a vision compress view */
# define	fdCLRMAND	040000000	/* clearing mandatory field */
# define	fdFRMSTRTCHD	010000000000	/* form has space reserved
						** for attribute bytes
						*/

/* FIELD types */
# define	FREGULAR	0	/* a regular field */
# define	FTABLE		1	/* a table field */
# define	FCOLUMN		2	/* a column in a table field */

# define	FHOTSPOT	3	/* hotspot trim */
# define	FSCRUP		4	/* table field scroll point */
# define	FSCRDN		5	/* table field scroll point */

/* FIELD flags */
# define	fdBOXFLD	01		/* box a field */
# define	fdSTOUT		02		/* Obsolete */
# define	fdQUERYONLY	04		/* field is query only */
# define	fdKPREV		010		/* keep previous value */
# define	fdMAND		020		/* field is mandatory */
# define	fdCHK		020		/* same as mandatory */
# define	fdNOTFLINES	040		/* no separating lines in TF */
# define	fdRVVID		0400		/* reverse video */
# define	fdBLINK		01000		/* blinking */
# define	fdUNLN		02000		/* underline */
# define	fdCHGINT	04000		/* change display intensity */
# define	fd1COLOR	0200000		/* foreground color 1 */
# define	fd2COLOR	0400000		/* foreground color 2 */
# define	fd3COLOR	01000000	/* foreground color 3 */
# define	fd4COLOR	02000000	/* foreground color 4 */
# define	fd5COLOR	04000000	/* foreground color 5 */
# define	fd6COLOR	010000000	/* foreground color 6 */
# define	fd7COLOR	020000000	/* foreground color 7 */
# define	fdCOLOR		037600000	/* short hand for all colors */
# define	fdATTR		07400		/* short hand reg attribs */
# define	fdDISPATTR	(fdRVVID | fdBLINK | fdUNLN | fdCHGINT | fdCOLOR)
# define	fdMINCOLOR	0
# define	fdMAXCOLOR	7

/*
**  New flags for tffflags memeber in TBLFLD struct.
**  These do overlap with other field flags but not
**  with use in "tffflags".
*/

# define	fdtfFKROW	040000000	/* this is a fake row */
# define	fdtfRWCHG	0100000000	/* something changed in row */

/*
**  New field flags for 4.0. (dkh)
*/

# define	fdLOWCASE	0100		/* force lower case */
# define	fdUPCASE	0200		/* force upper case */
# define	fdNOAUTOTAB	010000		/* no autotab */
# define	fdNOECHO	020000		/* no echo */

# define	fdNOTFTITLE	040000		/* don't draw any title space */
# define	fdNOTFTOP	0100000		/* don't draw any top line */
# define	fdTFXBOT	040000000	/* bottom line in tf is special */
# define	fdINVISIBLE	0100000000	/* invisible field or column */

/*
**  New flags for 6.0.
*/
# define	fdREVDIR	0200000000	/* reverse dir for entry */
# define	fdNOCOLSEP	0400000000	/* no column separators */
# define	fdI_CHG		01000000000	/* internal change bit */
# define	fdX_CHG		02000000000	/* external change bit */
# define	fdVALCHKED	04000000000	/* field already validated */
# define	fdROWHLIT	010000000000	/* Row highlight for tf */
/* DO NOT USE 020000000000 -- most significant bit */

/*
**  Internal flag to determine when the formatting routines
**  need to be called to reformat a field's value.
*/
# define	fdXREFVAL	020000000000

/* fhd2flags */

# define	fdLANDABLE	01		/* field is landable */
# define	fdUSEEMASK	02		/* allow fld to use edit mask*/
# define	fdEMASK		04		/* user has enabled edit mask*/

/* fdSCRLFD is used in BOTH fhd2flags and frmflags */
# define	fdSCRLFD	0100		/* Scrolling field */

/* Note that fd2SCRFLD	       (0400) is set in both fhd2flags and frmflags */

# define	fdREADONLY	01000		/* Readonly field */
# define	fdVQLOCK	02000		/* VQ restricted field */
# define	fdDERIVED	04000		/* Derived field */
# define	fdVISITED	010000		/* Derived field processing */
# define	fdAGGSRC	020000		/* Aggregate source fld/col */
# define	fdCOLSET	040000		/* Col set via IItcoputio(). */

/*
**  Flag for frm->frmflags that indicates the form needs to be
**  all redrawn since a clear screen was done while the form
**  was active (i.e., displayed).
*/
# define	fdREDRAW	040000
/* fdCOLOR	037600000 is reserved for frmflags (fd1COLOR-fd7COLOR) */

/*
**  Flag definitions for attributes stored in a table field
**  dataset or for a table field cell.  These values are used
**  in member tffflags of struct TBLFLD and in the c_chgoffset
**  member of struct COLDESC.
*/
# define	dsCHGBIT	01		/* change bit, only for 6.3 */
# define	ds1COLOR	02		/* color 1 for dataset */
# define	ds2COLOR	04		/* color 2 for dataset */
# define	ds3COLOR	010		/* color 3 for dataset */
# define	ds4COLOR	020		/* color 4 for dataset */
# define	ds5COLOR	040		/* color 5 for dataset */
# define	ds6COLOR	0100		/* color 6 for dataset */
# define	ds7COLOR	0200		/* color 7 for dataset */
# define	dsRVVID		0400		/* reverse video for dataset */
# define	dsBLINK		01000		/* blinking for dataset */
# define	dsUNLN		02000		/* underline for dataset */
# define	dsCHGINT	04000		/* change intensity for dataset */
# define	dsCOLOR		0376		/* all colors for dataset */
# define	dsATTR		07400		/* all reg attr for dataset */
# define	dsALLATTR	07776		/* all attr for dataset */
# define	dsDISP_EMPTY	010000		/* ds cell display was empty */

# define	tf1COLOR	010000		/* color 1 for cell */
# define	tf2COLOR	020000		/* color 2 for cell */
# define	tf3COLOR	040000		/* color 3 for cell */
# define	tf4COLOR	0100000		/* color 4 for cell */
# define	tf5COLOR	0200000		/* color 5 for cell */
# define	tf6COLOR	0400000		/* color 6 for cell */
# define	tf7COLOR	01000000	/* color 7 for cell */
# define	tfRVVID		02000000	/* reverse video for cell */
# define	tfBLINK		04000000	/* blinking for cell */
# define	tfUNLN		010000000	/* underline for cell */
# define	tfCHGINT	020000000	/* change intensity for cell */
# define	tfCOLOR		01770000	/* all colors for cell */
# define	tfATTR		036000000	/* all reg attr for cell */
# define	tfALLATTR	037770000	/* all attr for dataset */
# define	tfDISP_EMPTY	0400000000	/* tf cell display is empty */
/*
**  Special value (in trmflags and frmflags) to indicate there are
**  hot spot trims in the form.  This is for emerald internal use
**  for now.  Note that the 02 value does not overlap with other values
**  in trmflags and frmflags.
*/
# define	fdTRHSPOT	02

/*
** FLAGS for table field header.
** These flags are used together with the display and attribute flags
** shown above.  The display modes of a table field are display attributes
** that can be set at runtime.  When assigning new attribute masks make sure
** that they do not conflict with exsisting masks.
*/
# define	fdtfMASK	07400		/* bits for table field */
# define	fdtfREADONLY	01000		/* readonly field mask */
# define	fdtfUPDATE	02000		/* can update values mask */
# define	fdtfAPPEND	04000		/*   and add new rows mask */
# define	fdtfQUERY	00400		/* query mode mask */

/*
** Table field header flag--all the table field columns are invisible.
** The table field itself may be either visible or invisible.
** Overlaps the fdKPREV flag value (which is only used for simple fields.)
*/
# define	fdTFINVIS	010		/* all tblfld cols invisible */
/*
** Table field header flag--this table field allows Single Character Find.
** Overlaps the fdMAND flag value (which is only used for simple fields.)
*/
# define	fd1CHRFIND	020

/*
** COLUMN header flags.
** Columns can be read only (specified in Vifred) but cannot be on the frnsfld
** array.  Consequently there must be a specific bit in the header that allows
** this.  The first bit is used and does not conflict with fdBOXFLD as a column
** can never be boxed anyway.
*/
# define	fdtfCOLREAD	01

/* TABLE FIELD states */
# define	tfNORMAL	1
# define	tfSCRUP		2
# define	tfSCRDOWN	3
# define	tfDELETE	4
# define	tfINSERT	5
# define	tfFILL		6

 
/* TABLE FIELD ROWS */
/* anything >=0 implies a particular row */
# define	fdtfTOP		-1		/* extra top row */
# define	fdtfBOT		-2		/* extra bot row */
# define	fdtfCUR		-3		/* current row */

/* FRAME DRIVER DISPLAY MODES */
# define	fdmdUPD		1
# define	fdmdADD		2
# define	fdmdFILL	3
/* Query mode is sent as fdmdFILL from the form system (ncg) */
# define	fdmdQRY		fdmdFILL

/*
**  New mode added for FT and 3270 terminals. (dkh)
*/

# define	fdmdBROWSE	4

/* FRAME DRIVE COMMAND MODES */
# define	fdcmNULL	0
# define	fdcmEDIT	1
# define	fdcmINSRT	2
# define	fdcmBRWS	4
# define	fdcmINBR	(fdcmINSRT | fdcmBRWS)
# define	fdcmGREDIT	8

/* QUERY OPERATORS */
# define	fdNOP		0
# define	fdEQ		1
# define	fdNE		2
# define	fdLT		3
# define	fdGT		4
# define	fdLE		5
# define	fdGE		6

/* FRAME DRIVER OPERATIONS */

# define	fdNOCODE	0		/* normal return */
# define	fdopERR		(i4)(-1)	/* key has no command */
					/* Window positioning functions */
# define	fdopLEFT	(i4)(-2)	/* move cursor left */
# define	fdopRIGHT	(i4)(-3)	/* move cursor right */
# define	fdopUP		(i4)(-4)	/* move cursor up */
# define	fdopDOWN	(i4)(-5)	/* move cursor down */
# define	fdopBEGF	(i4)(-6)
# define	fdopENDF	(i4)(-7)
# define	fdopWORD	(i4)(-8)	/* move forward one word */
# define	fdopBKWORD	(i4)(-9)	/* move backward one word */
# define	fdopENDW	(i4)(-10)
# define	fdopBKENDW	(i4)(-11)
# define	fdopFIND	(i4)(-12)
# define	fdopBKFIND	(i4)(-13)
# define	fdopUPTO	(i4)(-14)
# define	fdopBKUPTO	(i4)(-15)
					/* Window editing functions */
# define	fdopCHG		(i4)(-16)
# define	fdopDEL		(i4)(-17)
# define	fdopAPP		(i4)(-18)
# define	fdopINSRT	(i4)(-19)
# define	fdopSUBS	(i4)(-20)
# define	fdopREPLCH	(i4)(-21)
# define	fdopDELCH	(i4)(-22)
# define	fdopCASECH	(i4)(-23)
# define	fdopCLRF	(i4)(-24)	/* clear entire field */
# define	fdopDELF	(i4)(-25)	/* delete char under cursor */
# define	fdopUNDO	(i4)(-26)
					/* General frame functions */
# define	fdopESC		(i4)(-27)	/* escape key types */
# define	fdopVERF	(i4)(-28)	/* fork editor on field */
# define	fdopNEXT	(i4)(-29)	/* go to next field */
# define	fdopPREV	(i4)(-30)	/* go to previous field */
# define	fdopREFR	(i4)(-31)	/* redisplay current image */
# define	fdopSCRUP	(i4)(-32)	/* scroll form up */
# define	fdopSCRDN	(i4)(-33)	/* scroll form down */
# define	fdopFEXIT	(i4)(-34)
# define	fdopMODE	(i4)(-35)	/* toggle editing modes */
# define	fdopRUB		(i4)(-36)	/* erase char left of cursor */
# define	fdopRET		(i4)(-37)	/* clear rest of field */
# define	fdopADUP	(i4)(-38)	/* auto-duplicate field */
# define	fdopBRWSGO	(i4)(-39)	/* force into browse mode */
# define	fdopMENU	(i4)(-40)	/* goto to the menu */
# define	fdopNROW	(i4)(-41)	/* goto next row/field */
# define	fdopTOTOP	(i4)(-42)	/* goto top of form */
# define	fdopSCRLT	(i4)(-43)	/* scroll form left */
# define	fdopSCRRT	(i4)(-44)	/* scroll form right */
# define	fdopPRSCR	(i4)(-45)	/* print current screen */
# define	fdopSKPFLD	(i4)(-46)	/* skip fields */
# define	fdopMUITEM	(i4)(-47)	/* menu item selected */
# define	fdopFRSK	(i4)(-48)	/* frs key selected */
# define	fdopSPACE	(i4)(-49)	/* the space command */
# define	fdopSHELL	(i4)(-50)	/* the shell command */
# define	fdopNXITEM	(i4)(-51)	/* next item command */
# define	fdopTMOUT	(i4)(-52)	/* input timeout occurred */
# define	fdopGOTO	(i4)(-53)	/* goto a field from wview */
# define	fdopHSPOT	(i4)(-54)	/* hot spot cmd for emerald */
# define	fdopNULL	(i4)(-55)	/* hot spot cmd for emerald */
# define	fdopDELROW	(i4)(-56)	/* delete fake row at tf end */
# define	fdopSELECT	(i4)(-57)	/* select row in listpick */
# define	fdopALEVENT	(i4)(-58)	/* alerter event found */
# define	fdopSCRTO  	(i4)(-59)	/* Scroll to anywhere in ds */

/* data type */
# define	fdINT		1
# define	fdFLOAT		2
# define	fdSTRING	3

/*
** to allocate windows or not to allocate windows
*/
# define	WIN		TRUE
# define	NOWIN		FALSE

/*
** Constants used by
** TRAVERSE
** to decide how many rows of a table field to traverse
*/
# define	FDTOPROW	(i4)1	/* Only the top row */
# define	FDALLROW	(i4)2	/* All the rows */
# define	FDDISPROW	(i4)3	/* Only the currently displayed rows */

/*
**  Constants for supporting the inquire_frs on the command that
**  caused an activation. (dkh)
*/
# define	CMD_UNDEF	0	/* undefined command */
# define	CMD_MENU	1	/* menu key */
# define	CMD_FRSK	2	/* frs key */
# define	CMD_MUITEM	3	/* a menu item */
# define	CMD_NEXT	4	/* next field */
# define	CMD_PREV	5	/* previous field */
# define	CMD_DOWN	6	/* down line */
# define	CMD_UP		7	/* up line */
# define	CMD_NROW	8	/* new row */
# define	CMD_RET		9	/* return key, aka clear rest */
# define	CMD_SCRUP	10	/* scroll up */
# define	CMD_SCRDN	11	/* scroll down */
# define	CMD_NXITEM	12	/* next item */
# define	CMD_TMOUT	13	/* input timeout occured */
# define	CMD_GOTO	14	/* goto command */


/*
**  Define a convienent structure for some popup information.
*/
typedef struct popup_info
{
	i4	begy;		/* Beginning row number for popup */
	i4	begx;		/* Beginning column number for popup */
	i4	maxy;		/* Number of rows occupied by the popup */
	i4	maxx;		/* Number of columns occupied by popup */
} POPINFO;

/* Default value for a POPINFO structure member. */
# define	POP_DEFAULT	0

/*
**  Return codes from FTsyncdisp() for building a popup style form.
*/
# define	PFRMFITS	OK	/* popup form fits at desired spot. */
# define	PFRMMOVE	1	/* popup form was moved to fit. */
# define	PFRMSIZE	2	/* popup is too large to fit. */

# ifdef	DEBUG
GLOBALREF FILE *outf;
# endif


/* ERROR MESSAGE NUMBERS MOVED TO FRSERRNO.H */

/*
**  Special defines for qbf to do raw gets and puts into a field. (dkh)
*/

# define	QBFRAW_GET	(i4) 0		/* Raw get */
# define	QBFRAW_PUT	(i4) 1		/* Raw put */


/* Derived field processing values. */
# define	fdINVALID	1	/* Invalid field */
# define	fdNOCHG		2	/* Valid field--value not changed */
# define	fdCHNG		3	/* Valid field--value changed */

/* Derived aggregates invalidation processing values. */
# define	fdIASET		1	/* Perform a 'set' evaluation */
# define	fdIAGET		2	/* Perform a 'get' evaluation */
# define	fdIANODE	3	/* Perform a single node evaluation */

/*
** A macro to do point to point timing
*/
# define	fdtrctime(m)	(SIprintf("\r%s = %d\n\r", (m), TMcpu() - fdtime),fdtime = TMcpu())

/* function prototypes */
FUNC_EXTERN	bool	FDqryop(FRAME *, char *, i4 *);
FUNC_EXTERN	bool	FDsetparse(bool);

FUNC_EXTERN TRIM *IIFDabtAddBoxTrim(FRAME *, i4, i4, i4, i4, i4);
FUNC_EXTERN TRIM *IIFDattAddTextTrim(FRAME *, i4, i4, i4, const char *);
FUNC_EXTERN bool IIFDcfCloseFrame(FRAME *);
FUNC_EXTERN char *IIFDftFormatTitle(char *);
FUNC_EXTERN FRAME *IIfindfrm(char *);
FUNC_EXTERN i4 FDfralloc(reg FRAME *frm);
FUNC_EXTERN VOID IIresfrsk();
FUNC_EXTERN i4 FDwflalloc();
FUNC_EXTERN i4 FDwtblalloc();
FUNC_EXTERN VOID FDfrprint();
FUNC_EXTERN VOID dumpform();

#endif /* FRAME_H_INCLUDED */
