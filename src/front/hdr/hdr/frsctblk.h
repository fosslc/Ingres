/*
**	FRS Control Block Header File.
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	frsctlblk.h -	Control Block Definitions for 6.0 Forms System.
**
** Description:
**	This file contains the definitions for new control block
**	structures that will be used in the 6.0 Forms System.
**	Three control blocks will be defined: 1) an overall
**	frs control block, 2) a control block for when activation/validation
**	settings and 3) information about events that the upper
**	levels of the Forms System is interested in.
**
** History:
**	12/22/86 (dkh) - Initial version.
**	05/17/88 (dkh) - Added new members to FRS_EVCB to support
**			 inquire on current field position info.
**	05/20/88 (dkh) - More fiddling with the control blocks.
**	07/23/88 (dkh) - Added exec sql immediate hook.
**	28-feb-89 (bruceb)
**		Added act_entry field to FRS_AVCB--indicate entry activation
**		is enabled.  Added entry_act to FRS_EVCB to indicate that an
**		entry activation has occured.
**	14-mar-89 (bruceb)
**		Added lastcmd structure member to the evcb.
**	24-apr-89 (bruceb)
**		Removed 'timeout' from frs_globs struct and added shell_enabled.
**	07-jul-89 (bruceb)
**		Added FRS_RFCB struct.  Contains function pointers (to
**		runtime/tbacc) used by frame code.
**	07/12/89 (dkh) - Added support for emerald internal hooks.
**	25-jul-89 (bruceb)
**		Added in_validation to FRS_GLCB structure.  Only set when
**		not in query mode (form or table field.)
**	08/01/89 (dkh/bruceb) - Added call back routine pointers for
**			 aggregate processing.
**	27-sep-89 (bruceb)
**		Changed i4  shell_enabled to i4 enabled and started use
**		of bit masks.
**	12/01/89 (dkh) - Added support for WindowView GOTOS.
**	12/27/89 (dkh) - Added call back function pointer gotochk_proc
**			 due to IIFTgcGotoChk() being moved to frame!fdmove.c.
**	01-feb-90 (bruceb)
**		Added ks_ch entry to evcb for single keystroke find capability.
**		ks_ch is bigger than the definition in kst.h (it's 4 bytes
**		instead of 3) to facilitate 4-byte alignment of the structure.
**		Added KEYFIND flag to FRS_GLCB for toggling this feature.
**	07-feb-90 (bruceb)
**		Added GETMSGS_DISP flag to FRS_GLCB for requesting that error
**		messages be displayed when getform/getrow/unloadtable are
**		called.  Default (when unset) is to supress 'user' errors.
**	13-feb-90 (bruceb)
**		Default for the above changed to NOT supress; and flag now
**		called GETMSGS_OFF to compensate.
**	10-apr-90 (bruceb)
**		Added TFXSCROLL flag to FRS_GLCB for requesting table field
**		scroll 'hotspots' at intersections of top and bottom lines
**		with the column separators.  (Default is for the entire top
**		and bottom lines to perform that duty.)
**	6-may-92 (leighb) DeskTop Porting Change:
**		Added 'dsrecords_proc' to FRS_UTPR struct.
**	09/03/92 (dkh) - Added support for controlling behavior of "Out
**			 of Data" message when scrolling a table field.
**	04/23/94 (dkh) - Added new define (UP_DOWN_EXIT) to control whether 
**			 field exit is allowed with the up/down arrows.
**			 This is for backwards compatibility so existing
**			 applications won't break.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	19-May-2009 (kschendel) b122041
**	    The only DSRecords proc (in tbacc) is void, fix here.
**/


/*
**  A control block to hold information about the various
**  activation/validation settings in the forms system.
**  This control block can be set/inquire on by the
**  upper levels (above FT) of the Forms System.  The
**  lower levels (FT and below) of the Forms System may
**  ONLY inquire on the control block.
*/

typedef struct frs_actval_cb
{
	bool	val_next;		/* validate going to next field */
	bool	val_prev;		/* validate going to previous field */
	bool	val_mu;			/* validate on menu key selection */
	bool	val_mi;			/* validate on menu item selection */
	bool	val_frsk;		/* validate on frs key selection */
	bool	act_next;		/* activate going to next field */
	bool	act_prev;		/* activate going to previous field */
	bool	act_mu;			/* activate on menu key selection */
	bool	act_mi;			/* activate on menu item selection */
	bool	act_frsk;		/* activate on frs key selection */
	bool	act_entry;		/* activate on field entry */
} FRS_AVCB;


/*
**  A control block to gather information about interesting events
**  that was triggered by user input.  Examples are tabbing to
**  the next field or pressing the menu key.
**  The upper (above FT) levels of the Forms System may set/inquire
**  on the control block.  The lower (FT and below) layers of the
**  Forms System normally only set information in the control block
**  with inquiry also permitted.
**
**  New structure members added to support inquire on current input area
**  position.  Members xcursor and ycursor provide
**  screen relative position of screen cursor.  These all start at 1.
**  Members scrxoffset and yscroffset provide scroll offset for current
**  form.  This tells us how many rows and columns the current form has
**  scrolled.  For example, if scryoffset is -30, then the form has
**  scrolled 30 rows, or 30 rows are not visible off the top of the
**  terminal screen.
**
**  To calculate where a certain field starts at relative to the screen,
**  one does the following calculations.
**  For a simple field:
**	get hdr for simple field.
**	get val for simple field.
**	get type for simple field.
**  Calculate position relative to form.
**	posx = hdr->fhposx + type->ftdatax;
**	posy = hdr->fhposy + val->fvdatay;
**  Calculate position relative to screen and go to 1 addressing.
**	posx += scrxoffset + 1;
**	posy += scryoffset + 1;
**
**  For a table field cell:
**	check to see if current row is valid.  we may be in the middle of
**	a table field scroll.  this can make the current row < 0 or >
**	number of physical rows.  if this is the case, just return 0.
**	get hdr for table field.
**	get val for cell in table field.
**	get type for cell in table field.
**  Perform same calculatins as above.
*/

typedef struct frs_event_cb
{
	i4	event;			/* what event occured */
	i4	curfld;			/* current field when event occured */
	i4	mf_num;			/* a menu item or frs key number */
	i4	intr_val;		/* interrupt value returned by FT */
	i4	scrxoffset;		/* scroll x offset for form */
	i4	scryoffset;		/* scroll y offset for form */
	i4	xcursor;		/* x position of screen cursor */
	i4	ycursor;		/* y position of screen cursor */
	i4	timeout;		/* timeout value in seconds */
	i1	val_event;		/* whether validation should occur */
	i1	act_event;		/* whether activation should occur */
	i1	continued;		/* whether a continued was issued */
	i1	processed;		/* event has already been processed */
	i4	lastcmd;		/* 'last command' value--often=event */
	bool	entry_act;		/* field entry activation in progress */
	bool	eval_aggs;		/* should aggregates be evaluated? */
	i4	gotofld;		/* field number for goto command */
	i4	gotocol;		/* tf column number for goto command */
	i4	gotorow;		/* tf row number for goto command */
	u_char	ks_ch[4];		/* Char typed for single char find */
} FRS_EVCB;


/*
**  A global value control block for holding information that is
**  global to the forms system.
*/

typedef struct frs_glob_cb
{
	i4	enabled;		/* Is this feature enabled? */

# define	SHELL_FN	01	/* Shell key is enabled. */
# define	EDITOR_FN	02	/* Field edit function is enabled. */
# define	GETMSGS_OFF	04	/* Supress error messages on 'gets'. */
# define	KEYFIND		010	/* Single character find function. */
# define	TFXSCROLL	020	/* Mouse TF scroll on intersections. */
# define	ODMSG_OFF	040	/* Don't print "out of data" msg */
# define	ODMSG_BELL	0100	/* Beep instead of "out of data" msg */
# define	UP_DOWN_EXIT	0200	/* Can exit field with up/down arrows */

	i4	(*key_intcp)();		/* Func pointer for key interception */
	i4	intr_frskey0;		/* Interrupt value for frskey0 */
	i4	traceflgs;		/* Flags for various tracing info */

# define	DRV_0TRACE	01	/* Derived field trace info 0 */
# define	DRV_1TRACE	02	/* Derived field trace info 1 */
# define	DRV_2TRACE	04	/* Derived field trace info 2 */
# define	SCR_0TRACE	010	/* Screen trace info 0 */
# define	SCR_1TRACE	020	/* Screen trace info 1 */
# define	SCR_2TRACE	040	/* Screen trace info 2 */

	i4	in_validation;		/* Indicate validation code running. */

# define	VLD_FORM	01	/* Validate the form. */
# define	VLD_ROW		02	/* Validate table field row. */
# define	VLD_TFLD	04	/* Validate a table field. */
# define	VLD_BULK	07	/* Mass validation. */
# define	VLD_FLD		010	/* Validate a simple field. */
# define	VLD_COL		020	/* Validate a table field column. */
} FRS_GLCB;

/*
**  A control block for the Forms System.  For now, it just
**  consists of pointers to the two control blocks defined
**  above.  This control block is initialized in initfd()
**  and passed to FTrun() and FTgetmenu().
*/

typedef struct frs_cb
{
	FRS_AVCB	*frs_actval;	/* pointer to a frs_actval_cb struct */
	FRS_EVCB	*frs_event;	/* pointer to a frs_event_cb struct */
	FRS_GLCB	*frs_globs;	/* global info for forms system */
} FRS_CB;


/*
**  A control block that contains function pointers to utility
**  routines that are needed by the FT layers.  This eliminates
**  any backward (and possibly undefined) references from FT to
**  layers above it at link time.  Information defined here is
**  not incorporated into the above control blocks since we only
**  need to pass this control block, typically when the forms
**  system is started (e.g., when FTinit() is called.
*/

typedef struct frs_utprocs
{
	FLDHDR	*(*gethdr_proc)();
	FLDTYPE	*(*gettype_proc)();
	FLDVAL	*(*getval_proc)();
	VOID	(*error_proc)();
	STATUS	(*rngchk_proc)();
	i4	(*valfld_proc)();
	VOID	(*dmpmsg_proc)();
	VOID	(*dmpcur_proc)();
	i4	(*fldval_proc)();
	VOID	(*sqlexec_proc)();
	VOID	(*inval_agg_proc)();
	VOID	(*eval_agg_proc)();
	VOID	(*inval_allaggs_proc)();
	i4	(*gotochk_proc)();
	i4	(*lastdsrow_proc)();
	void	(*dsrecords_proc)();

} FRS_UTPR;

/*
**  A control block that contains function pointers to utility routines
**  needed by the Frame layers for entry activation and derived field
**  processing.  This eliminates backward references from frame or below to
**  runtime/tbacc code.  This information is not incorporated into the
**  FRS_CB since only one copy is needed, and this can be passed down by
**  IIforms().
*/

typedef struct frs_func_cb
{
	i4		(*newDSrow)();	/* EA:  is user on a new dataset row */
	i4		(*getTFrow)();	/* Derive:  return TF row number */
	i4		*(*getTFflags)(); /* Derive:  return flags for cell */
	DB_DATA_VALUE	*(*getTFdbv)();	/* Derive:  return idbv for cell */
	STATUS		(*aggsetup)();	/* aggregate setup routine */
	VOID		(*aggnxval)();	/* get next value from dataset */
} FRS_RFCB;
