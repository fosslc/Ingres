/*
** Copyright (c) 2004 Ingres Corporation
*/
/*	static char	Sccsid[] = "@(#)rtype.h	30.1	11/16/84";	*/

# include	<fe.h> 
# include	<ui.h> 
# include	<rcons.h> 
# include	<ooclass.h>

/*
**	HISTORY
**	6/19/89 (elein) garnet
**		Added declaration of setclean (SC) structure for
**		holding the setup/cleanup queries.
**	8/11/89 (elein) garnet
**		Added ITEM to tcmd structure to hold expressions
**		for evalutation for various commands.
**	02-oct-89 (sylviap)
**		Created CTRL and BUF for control sequences (Q format).
**	12/14/89 (martym) (garnet)
**		Added LABST_INFO structure.
**	30-jan-90 (cmr)	
**		Get rid of obsolete fields in the OPT struct.
**	25-jul-90 (sylviap)	
**		Added TBL and TBLPTR data structures for joindef reports
**		in RBF.
**	27-sep-90 (sylviap)	
**		Changed BLKSIZE to 1000. (#33540)
**	3/11/91 (elein)
**		PC Porting changes * 6-x_PC_80x86 *
**	05-jun-92 (rdrane)
**		Add boolean to REN structure (ren_dlm_owner) to indicate
**		whether or not owner name string is a delimited identifier.
**		Add boolean to REN structure (ren_spec_owner) to indicate
**		that the owner name string was explicitly specified as part of
**		an "owner.report" string.
**	06-jul-92 (sweeney)
**		elided a ^A character that was troubling my compiler
**		(apl_us5)
**	1-oct-92 (rdrane)
**		Rework the 05-jun-92 changes to replace REN report name and
**		owner with a pointer to an FE_RSLV_NAME structure.  Force
**		inclusion of fe.h and ui.h so that we're assured of their
**		function declarations and of their 6.5 datatypes.  Add
**		*prec to ACC, CUM, and EXP typedefs for DECIMAL support (holds
**		equivalent of DB_DATA_VALUE.db_prec).
**	10-nov-1992 (rdrane)
**		Add owner field to TBL structure for RBF support of
**		owner.tablename.
**	12/09/92 (dkh) - Added Choose Columns support for rbf.
**	23-feb-1993 (rdrane)
**		Add CTRL chain pointer to LN structure so that q0 formats are
**		now associated with specific lines (46393 and 46713).  Also add
**		memory tag to CTRL structure since no longer using global pool.
**	22-jun-1993 (rdrane)
**		Modify OPT structure definition to add support for suppression
**		of initial formfeed (this realy belongs in rbftype.h).
**	1-jul-1993 (rdrane)
**		Add rdate_fmt, rtime_fmt, and rpageno_fmt to OPT structure
**		to support user override of RBF hdr/ftr timestamp/page #
**		formatting.  If the associated bool's are TRUE, then generate
**		the appropriate .DECLARE and .PRINT statements with the
**		format value.
**	18-aug-1993 (rdrane)
**		Add level field to REN structure definition.
**      12-aug-2004 (lazro01)
**	        BUG 112819   
**	        Changed tcmd_seqno to accept values greater than 32K.
**	13-Jun-2007 (kibro01) b118486/b117033/b117376
**		Added par_set to indicate that a parameter has been set through
**		the command-line interface.
**	23-Feb-2009 (gupsh01)
**		Added UTF8_WORKBUF_SIZE for temporary UTF8 work buffer
**		lengths.
**      06-mar-2009 (huazh01)
**              Remove UTF8_WORKBUF_SIZE. The temp UTF8 work buffer is 
**              now being dynamically allocated based on the value of 
**              En_utf8_lmax. (bug 121758)
*/
 
/*
**	Data Structures for Report Writer
**	---------------------------------
**
**	This text includes the typedef descriptions of the 
**	major data structures used by the INGRES report
**	formatter.  The following subsections are included
**	(within subsections, structures are arranged alpha-
**	betically, unless declaration order precludes it.):
**
**		1. Global Data structures.
**		2. Aggregate Data structures.
**		3. Text command data structure.
**		4. Break and attribute Data structures.
**
*/

/*
**	Global Data Structures
**	----------------------
**
**	These data structures refer to global structures to the
**	report formatter.  Some correspond to data structures in
**	the data base, and others reflect status flags for the
**	program.
*/

# define BLKSIZE  1000	/* needed for BUF */


/*
**	ACT - structure contains the data from the database
**		referring to one action.  This is used simply
**		to temporarily hold the action data from the
**		database before putting them in the TCMD structures.
*/

typedef struct act
{
	char	*act_section;		/* header or footer */
	char	*act_attid;		/* attribute name */
	char	*act_command;		/* command name */
	char	*act_text;		/* text */
}   ACT;


/*
**	QUR - structure contains the data from the database
**		referring to a part of a query.
**		This is used simply
**		to temporarily hold the query data from the
**		database before putting them into the proper
**		structures.
*/

typedef struct qur
{
	char	*qur_section;		/* section of query */
	char	*qur_attid;		/* attid */
	char	*qur_text;		/* table name or list */
}  QUR;


/*aem
**	Setup/Cleanup control structure table.
*/

typedef struct setclean
{
	char     	*sc_text;		/* ptr to query text*/
	struct setclean *sc_below;		/* ptr to next element*/
} SC;

/*
**	RTEXT control word table.  This represents the valid 
**		RW commands and info about them.
*/

typedef struct	rtext
{
	char	*rt_name;			/* ptr to one allowable name */
	ACTION	rt_code;			/* associated TCMD code */
	i4	rt_type;			/* type of command (for print) */
} RTEXT;

/*
**	CTRL - A new structure, CTRL, with the LN stucture creates a RW line.
**		CTRL's create a linked list of all control sequences in a line.
**		A control sequence SHOULD be non-printable, and therefore take 
**		up no space on the report.  (Should a string be printable, 
**		RW still treats the string as if it takes up no space, but 
**		it will appear in the report, and formatting of that line 
**		will be wrong).  Control sequences are stored separately 
**		from the printable characters (ln_buf), so positioning 
**		commands (CENTER, TAB, etc.) work based only on the 
**		printable characters. (sylviap)
*/

typedef struct ctrl
{
	i4	cr_number;		/* sequence number where control 
					** sequence is inserted before */
	TAGID	cr_tag;			/* tag of cblk and buffer memory */
	char	*cr_buf;		/* actual control sequence buffer */
	i4	cr_len;			/* length of control sequence buffer */
	struct ctrl *cr_below;		/* pointer to next line */
}  CTRL;


/*
**	LN - structure contains information about a forming line.
**		This is the start of a linked list of lines.  The list
**		of lines is used when buffering up the printout for
**		a column (which may span more than one physical line
**		on the page) using the cn.m format.  Three of these linked
**		lists are used--one for centering and justification, one
**		for the report line, and one for page breaks.  More lines
**		are allocated as needed in the running of the report.  
**		The linked list is therefore as long as the longest set 
**		of lines yet encountered.
**
*/


typedef struct ln
{
	char	*ln_buf;		/* actual line buffer */
	i2	ln_started;		/* type of start of line.
					** can be LN_NONE,LN_WRAP,or LN_NEW */
	char	*ln_ulbuf;		/* buffer for underlining */
	bool	ln_has_ul;		/* TRUE if this line has any underlining */
	i4	ln_curpos;		/* current position in this line */
	i4	ln_sequence;		/* sequence number in this linked list,
					** used in checking for maximum */
	i4	ln_curwrap;		/* start of line for cur wrap */
	CTRL	*ln_ctrl;		/*
					** pointer to CTRL chain or NULL if
					** no q0 formats
					*/
	i4	ln_utf8_adjcnt;		/* On UTF8 charset keep track of adjustment 
					** needed due to printing UTF-8 data. Added
					** to support multiline output.
					*/
	struct ln *ln_below;		/* pointer to next line */
}  LN;


/*
**	BUF - BUF is used as a memory pool for control sequences (cr_buf).
*/

typedef struct buf
{
	struct buf *next_buf;		/* pointer to next buffer */
	char	    mem_buf[BLKSIZE];	/* memory buffer pool for  
					** ctrl->cr_buf */
}  BUF;

/*
**	SORT - contains the information for one sort attribute.
**		An array of these structures will be created -- one for
**		each sort attribute in the data relation.  The structure
**		initially holds the "raw" database strings, at which time
**		sort_attid is significant and sort_ordinal is meaningless.
**		after sort_ordinal has been set, sort_attid is NULL.
*/

typedef struct sort		/* 6-x_PC_80x86 */
{
	ATTRIB	sort_ordinal;	/* ordinal of this attribute */
	char	*sort_attid;	/* attribute string */
	char	*sort_break;	/* Used in RBF only, this
				** indicates whether this is
				** a break column or not */
	char	*sort_direct;	/* direction of the sort:
				** "a" - ascending (default)
				** "d" - descending.
				*/
}   SORT;



/*
**	PAR - contains the information for one runtime parameter or
**		declared variable.
**		Runtime parameters are strings specified on the
**		command call to REPORT in a parenthesized list
**		of the form: ({ param = value }).
**		They are stored in a linked list of PAR structures,
**		which contain name, value pairs.
*/

typedef struct par
{
	char		*par_name;	/* name of parameter */
	char		*par_string;	/* entered string value for parameter */
	DB_DATA_VALUE	par_value;	/* value of parameter */
	bool		par_declared;	/* TRUE if declared in .DECLARE cmd */
	char		*par_prompt;	/* prompt string */
	struct par 	*par_below;	/* ptr to next in linked list */
	bool		par_set;	/* TRUE if set by command-line value */
}   PAR;




/*
**	RNG - contains the information for one RANGE statement used
**		in specifying the query.  This is a simple linked
**		list containing the range_var name and table name,
**		with a link.
*/

typedef struct rng
{
	char	*rng_rvar;		/* name of range_var */
	char	*rng_rtable;		/* name of table */
	struct rng *rng_below;		/* next in linked list */
}   RNG;



/*
**	Data Structures for Aggregations
**	--------------------------------
**
**	These data structures define the accumulators that are
**	used in aggregations.  The basic accumulators are in
**	linked lists (ACC) and all in f8 precision
**	numeric format for simplicity of design.  When setting
**	up the data structures, a linked list of ACC's are
**	set up for each attribute in the data relation for each
**	primitive operation.  The starts of these linked lists
**	are kept in an array, Nattributes long, of LAC's.
**	For each attribute/operation combination, keep a linked
**	list, going from minor to major, of the attributes which
**	aggregate over this attribute.  This information is ferreted
**	from the RTEXT for the actions (usually the "break footer"
**	text).  The non-primitive aggregates, average and
**	cumulative, are defined in terms of the primitives.
**	When processing breaks, the structure here is ignored,
**	and direct references are made to the specific accumulators
**	within the ACC structures, as the addresses of the 
**	accumulatores are stored in TCMD structures in the
**	break text.
*/




/*
**	PRS - preset data structure.  This defines the presetting
**		to be done when clearing out an aggregate.  
*/

typedef struct prs
{
	i2	prs_type;		/* type of presetting to be done.
					** PT_CONSTANT - constant.
					** PT_ATTRIBUTE - set to current value
					**		of an attribute.
					*/
	union
	{
		ATTRIB		pval_ordinal;		/* if PT_ATTRIBUTE */
		DB_DATA_VALUE	pval_constant;		/* if PT_CONSTANT */
	}  prs_pval;

	struct prs	*prs_below;	/* ptr to next PRS structure */
}   PRS;


/*
**	ACC - defines one accumulator for one primitive
**		aggregate, for one attribute in the report.  This
**		is an element in a linked list of accumulators
**		associated with a attribute.
**		The accumulators are directly referenced
**		for aggregations and clearing.
*/

typedef struct acc
{
	ATTRIB	acc_break;	/* break attribute associated with this
				** accumulator.  When a break occurs
				** in this attribute, the accumulator 
				** will be referenced.  If there are
				** any ACC's further down the
				** linked list (with a more major
				** sort key), that accumulator will
				** be "opped" with this one when a
				** break occurs.  This is a very
				** efficient method of multiple
				** break aggregations.
				*/
	ATTRIB	acc_attribute;	/* attribute ordinal being aggregated */
	bool	acc_unique;	/* TRUE if aggregate is prime or unique */
	PRS	*acc_preset;	/* ptr to a PRS presetting structure, if
				** the aggregate is to be preset */
	ADF_AG_STRUCT acc_ags;	/* aggregate structure used by ADF
				** containing the fid and accumulator
				** workspace */
	DB_DT_ID acc_datatype;	/* aggregate result datatype */
	i4	 acc_length;	/* aggregate result data length */
	i2	 acc_prec;	/* aggregate result data precision */
	struct acc *acc_below;	/* link to next accumulator */
	struct acc *acc_above;	/* link to accumulator before 
				** this one, with smaller break
				** associated with it.  Used only if the
				** aggregates are levelled. */
}   ACC;



/*
**	LAC - structure contains pointer to the start
**		of the accumulator linked lists for one
**		the primitive operations. These LACs are linked together
**		and the top of the list is pointed at from att_lac.
*/

typedef struct lac
{
	ACC	*lac_acc;	/* start of linked list of specific agg */
	struct lac *lac_next;	/* next start of linked lists */
}   LAC;



/*
**   CUM - structure used by r_x_cum to calculate a cumulative.  This
**	is set up as part of a TCMD structure whenever a cumulative
**	is called for.  It contains ptrs to the correct place in the
**	ACC linked list to start the accumulation, as well as the
**	operation to perform.
*/

typedef struct cum
{
	ATTRIB	cum_break;		/* break for this cumulative */
	ATTRIB	cum_attribute;		/* attribute to aggregate */
	bool	cum_unique;		/* TRUE if aggregate is prime or
					** unique */
	ADF_AG_STRUCT cum_ags;		/* aggregate structure used by ADF
					** containing the fid and accumulator
					** workspace */
	DB_DT_ID cum_datatype;		/* aggregate result datatype */
	i4	 cum_length;		/* aggregate result data length */
	i4	 cum_prec;		/* aggregate result data precision */
	PRS	*cum_prs;		/* ptr to the preset structure */
}   CUM;





/*
**	Text Action Data Structure
**	---------------------------
**
**	Text action (RTEXT strings) are kept in memory at all
**	times in a semi-compiled fashion.  Raw RTEXT strings
**	from the RACTION relation are converted into codes
**	indicating printing of text, newline, tabs, and addresses of
**	parameters for substitution.  When they are "compiled",
**	they are put into the TCMD structures as codes and
**	parameters to codes.  These codes are such things
**	as "writeline", "endstring", "tab", etc. (similar to
**	RTEXT commands, but not quite the same).  The parameter
**	is a variant structure depending on the type of action.
**	These text elements are in linked lists to provide
**	flexibility 
**	
*/





/*
**	AP - used in TCMD structures for .FORMAT or .TFORMAT.
*/

typedef struct ap		/* 6-x_PC_80x86 */
{
	ATTRIB	ap_ordinal;		/* ordinal of attribute to print */
	FMT	*ap_format;		/* format structure */
}   AP;


/*
**	PS - used in TCMD structures to set a default position for
**		a column in response to the position command.
*/

typedef	struct ps			/* 6-x_PC_80x86 */
{
	ATTRIB	ps_ordinal;		/* ordinal of column for which the
					** position is to be set.  */
	i4	ps_position;		/* output position to set it to */
}   PS;



/*
**	WS - used in TCMD structures to set a default width for
**		a column in response to the width command.
*/

typedef	struct ws			/* 6-x_PC_80x86 */
{
	ATTRIB	ws_ordinal;		/* ordinal of column for which the
					** width is to be set.  */
	i4	ws_width;		/* output width to set it to */
}   WS;


/*
**	VP - variable positioning structure used with positioning commands
**		tab, right, left, center, npage, newline, and any others
**		that can use a value either relatively, absolutely, as
**		default, or with a column name (for some).  The 
**		associated processing routine actually sets it up.
*/

typedef struct vp		/* 6-x_PC_80x86 */
{
	i2	vp_type;	/* type of positioning.  Values are
				** B_ABSOLUTE, B_RELATIVE, B_DEFAULT,
				** or B_COLUMN */
	i4	vp_value;	/* value used with positioning. This
				** is the new position, number of lines,
				** column ordinal, etc. depending on command */
}  VP;




/*   ITEM - structure for an item.
*/

typedef struct item
{
	i2	item_type;	/* type of item */

	union			/* actual item */
	{
		struct exp	*i_v_exp;	/* root of another subtree */
		DB_DATA_VALUE	*i_v_con;	/* data constant */
		i2		i_v_pc;		/* program constant */
		i4		*i_v_pv;	/* program variable */
		ATTRIB		i_v_att;	/* attribute */
		PAR		*i_v_par;	/* parameter */
		ACC		*i_v_acc;	/* aggregates */
		CUM		*i_v_cum;	/* cumulative aggregates */
	} item_val;
}   ITEM;






/*   EXP - data structure for node of parse tree of an expression, boolean or
**	arithmetic.  It contains the operator to perform and its one (if it's
**	unary) or two (if it's binary) operands.
*/

typedef struct exp
{
	ADI_OP_ID	exp_opid;		/* operator id */
	ADI_FI_ID	exp_fid;		/* function id */
	DB_DT_ID	exp_restype;		/* result type */
	i4		exp_reslen;		/* result length */
	i2		exp_resprec;		/* result precision */
	ITEM		exp_operand[MAX_OPANDS];/* operand(s) */
}   EXP;





/*   IFS - data structure for IF statement.  This structure points to the
**	boolean expression data structure (EXP), the TCMDs to execute if
**	the expression is true, and the TCMDs to execute otherwise.
*/

typedef struct ifs
{
	EXP		*ifs_exp;	/* parse tree of boolean expression */
	struct tcmd	*ifs_then;	/* TCMDS for then */
	struct tcmd	*ifs_else;	/* TCMDS for else */
}   IFS;






/*
**	PEL - print element data structure. This has an item for an
**		arbitrary expression and an optional pointer to a format.
*/

typedef struct pel
{
	ITEM		pel_item;	/* parse tree of expression */
	FMT		*pel_fmt;	/* format for printing */
}   PEL;




/*
**	LET - assignment structure. The value of an item (right side) is
**		assigned to the DB_DATA_VALUE of a variable (left side).
*/

typedef struct let
{
	DB_DATA_VALUE	*let_left;	/* left side of the assignment */
	ITEM		let_right;	/* right side of the assignment */
}   LET;





/*
**	TCMD - text action command data structure.  This is an
**		element in a linked list of text actions to
**		take when a report action occurs.  These are
**		made up of codes for the type of action to 
**		take, and a variant parameter indicating the
**		parameter (optional) to the code.
**		
**		Note:  the codes used in this may change from
**		time to time, and should be kept current in the
**		routine r_act_set.
*/

typedef struct tcmd
{
	ACTION		tcmd_code;	/* code for type of action to take.
					** See rcons.h for a current
					** list of codes.
					*/
	i4		tcmd_seqno;	/* sequence number in rcommands
					** that tcmd was created from
					** for runtime error reporting.
					*/
	union		/* variant types for parameters to codes.*/
	{
		i4		t_v_long;	/* numeric codes */
		char		t_v_char;	/* for single chars */
		char		*t_v_str;	/* for string */
		ACC		*t_v_acc;	/* for operating on aggregate */
		AP		*t_v_ap;	/* process FORMAT or TFORMAT command */
		PS		*t_v_ps;	/* process POSITION command */
		WS		*t_v_ws;	/* process WIDTH command */
		VP		*t_v_vp;	/* variant positioning */
		FMT		*t_v_fmt;	/* FMT structure */
		IFS		*t_v_ifs;	/* IFS structure */
		PEL		*t_v_pel;	/* for printing element */
		LET		*t_v_let;	/* for a LET assignment */
	}		tcmd_val;
	ITEM		*tcmd_item;		/* ITEM struct for exprs in tcmds */

	struct tcmd	*tcmd_below;	/*next link in list */
}   TCMD;






/*
**   STK - data structure for stack of TCMD's.  This is a stack used
**	in potentially nested BEGIN/END blocks.  Each time a BEGIN
**	is seen the previous TCMD is pushed onto the stack.  When
**	END is found, it is popped.
*/

typedef struct stk
{
	TCMD	*stk_tcmd;		/* TCMD structure on stack.  Can
					** be NULL */
	struct stk *stk_above;		/* ptr to next STK above */
	struct stk *stk_below;		/* ptr to next STK below */
}   STK;



/*
**	ESTK - environment stack used to push down the current page
**		environment (margins, etc), when the page break is
**		written and when centering is specified.
*/

typedef struct estk
{
	i4	estk_lm;		/* left margin */
	i4	estk_rm;		/* right margin */
	bool	estk_ul;		/* Underlining status */
	LN	*estk_cline;		/* current line being written */
	LN	*estk_c_lntop;		/* top of linked list of cur line */
	struct estk *estk_below;	/* below in linked list */
	struct estk *estk_above;	/* above in linked list */
}   ESTK;



/*
**	Data Structures for Breaks
**	--------------------------
**
**	Breaks are defined as changes in any variable in 
**	the sort list, or the start or end of the report.
**	Detail action is the action taken after each data
**	tuple is read (i.e. done once for every tuple in 
**	the data relation).  
**
*/



/*
**	BRK - define the actions to take when a break
**		occurs.  This defines TCMD lists to process.  The BRK's
**		are set up as doubly linked lists for ease
**		in traversing.
*/

typedef struct brk
{
	ATTRIB	brk_attribute;	/* attribute ordinal associated
				** with this break.  When break occurs
				** in this variable, this break is
				** is taken.
				*/
	TCMD	*brk_header;	/* start of text path to
				** process before break
				*/
	TCMD	*brk_footer;	/* start of text path to
				** to process after break
				*/
	
	struct brk *brk_above;	/* above link in linked list */
	struct brk *brk_below;	/* below link in linked list */
}   BRK;



/*
**	ATT - structure contains data flags for each attribute
**		in the data relation.  Most of this data comes
**		from the RTEXT for the report heading.
*/

typedef struct att		/* 6-x_PC_80x86 */
{
	char	*att_name;	/* name of attribute */
	ATTRIB	att_brk_ordinal; /* sort order (if in break structure also) */
	FMT	*att_format;	/* default format for this attribute */
	FMT	*att_tformat;	/* temporary format set by ".tformat" RTEXT
				** command.  Format to use for first detail
				** printing only.
				*/
	COLUMN	att_width;	/* width (in chars) of column */
	i4	att_position;	/* default position in output line of
				** this attribute		*/
	i4	att_line;	/* vertical position used by RBF */
	LAC	*att_lac;	/* list of lists of aggregates on this column */
	DB_DATA_VALUE att_value; /* value of attribute, including datatype and
				 ** length */
	DB_DATA_VALUE att_prev_val; /* previous value of attribute */
	bool	att_dformatted; /* TRUE if there is a format for displaying
				** attribute.
				** If att_brk_ordinal > 0 (sorted on),
				** this implies att_curr_disp and att_prev_disp
				** exists. */
	bool	pre_deleted;	/* column was pre-deleted via choose columns */
	i2	att_dispwidth;	/* length of LONGTEXT for formatted display */
	DB_TEXT_STRING *att_cdisp; /* current formatted display */
	DB_TEXT_STRING *att_pdisp; /* previous formatted display */
}   ATT;







/*
**	Types for the Report specifier 
*/

/*
**	RSO - structure contains fields for the RSORT relation
**		containing info on the breaks in the report.
*/

typedef struct rso
{
	char	*rso_name;			/* name of sort attribute */
	i4	rso_order;			/* ordinal of sort order */
	char	*rso_direct;			/* direction of sort order */
	i4	rso_sqheader;			/* current seq number of header
						** text */
	i4	rso_sqfooter;			/* current seq number of footer
						** text */
	struct	rso	*rso_below;		/* link to next in list */
}   RSO;




/*
**	SBR - structure containing info on the breaks in the report.
*/

typedef struct sbr
{
	char	*sbr_name;			/* name of break attribute */
	i4	sbr_order;			/* ordinal of break order */
	i4	sbr_sqheader;			/* current seq number of header
						** text */
	i4	sbr_sqfooter;			/* current seq number of footer
						** text */
	struct	sbr	*sbr_below;		/* link to next in list */
}   SBR;




/*
**	REN - structure contains the REPORTS table structure.
*/

typedef struct ren
{
	OOID	ren_id;				/* report id */
	char	*ren_report;			/* report name */
	char	*ren_owner;			/* report owner */
	bool	ren_dlm_owner;			/*
						** TRUE if owner is a
						** delimited identifier
						*/
	bool	ren_spec_owner;			/*
						** TRUE if owner was explicitly
						** specified (owner.report)
						*/
	char	*ren_created;			/* created date */
	char	*ren_modified;			/* modified date */
	char	*ren_type;			/* type of report */
	i4	ren_acount;			/* count of actions */
	i4	ren_scount;			/* count of sort vars */
	i4	ren_bcount;			/* count of break vars */
	i4	ren_qcount;			/* count of query records */
	RSO	*ren_rsotop;			/* start of RSO linked list */
	SBR	*ren_sbrtop;			/* start of SBR linked list */
	char	*ren_shortrem;			/* short remark */
	char	*ren_longrem;			/* long remark */
	struct ren	*ren_below;		/* next REN in linked list */
	i4	ren_level;			/* Report level	*/
}   REN;


/*
**	structure for RCOMMAND retrieved from database.  Fixed buffer
**	size for each field, consisting of maximum length arrays.  Used to
**	give us a convenient package for writes to temporary file.  Also
**	provides one declaration of buffer sizes to use for these fields
**	Currently, b_type and b_seq don't really need to be written out,
**	but the penalty isn't that great, and b_seq provides an error check.
*/

typedef struct rcmd_rec			/* 6-x_PC_80x86 */
{
	i4 b_seq;
	char b_type [MAXRNAME+1];
	char b_sect [MAXRNAME+1];
	char b_attid [FE_MAXNAME+1];
	char b_cmd [MAXRNAME+1];
	char b_txt [MAXRTEXT+1];
} RCMD_REC;



/*
**	The OPT structure is used to contain the information
**		in the Options Form in RBF.  
*/

typedef struct opt			/* 6-x_PC_80x86 */
{
	char	rpl[5];			/* report page length */
	char	rulchar;		/* underline char */
	char	rffs;			/* insert formfeeds or not */
	char	no1stff;		/* suppress initial formfeed or not */
	char	rnulstr[33];		/* user specified nullstring */
	char	rphdr_first;		/* print pg hdr on first page */
	char	rdate_inc_fmt;		/* hdr/ftr date/time page# formats */
	i4	rdate_w_fmt;
	char	rdate_fmt[MAX_FMTSTR];
	i4	rtime_w_fmt;
	char	rtime_inc_fmt;
	char	rtime_fmt[MAX_FMTSTR];
	char	rpageno_inc_fmt;
	i4	rpageno_w_fmt;
	char	rpageno_fmt[MAX_FMTSTR];
}  OPT;





/*
**	DCL - structure containing info on the declared variables in the report.
*/

typedef struct dcl
{
	char		*dcl_name;	/* variable name */
	char		*dcl_datatype;	/* variable datatype and maybe prompt */
	struct	dcl	*dcl_below;	/* link to next in list */
}   DCL;




/*
** LABST_INFO - structure containing the dimensions of a labels style report.
*/

typedef struct labst_info
{
	i4 LineCnt;	/* The count of labels across the page */
	i4 MaxFldSiz;	/* The maximum size of a field */
	i4 MaxPerLin;	/* The maximum number of labels that would fit across */
	i4 VerticalSpace; /* The number of blank spaces between labels across */
	i4 HorizSpace; /* The number of blank lines between labels */
} LABST_INFO;

/*
**	TBL - TBL is a linked list of tables used in RBF for a joindef report
*/

typedef struct tbl
{
	char	    tbl_name[DB_GW1_MAXNAME+1];	/* table name */
	char	    tbl_owner[DB_GW1_MAXNAME+1];/* owner name */
	char	    rangevar[DB_GW1_MAXNAME+1];	/* range name */
	bool	    type;			/* TRUE=master; FALSE=detail */
	struct tbl *next_tbl;			/* pointer to next table */
}  TBL;

typedef struct tblptr
{
	struct tbl *first_tbl;			/* pointer to first table */
	struct tbl *last_tbl;			/* pointer to last table */
}  TBLPTR;

