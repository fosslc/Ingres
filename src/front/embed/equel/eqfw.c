/*
** Copyright (c) 2004 Ingres Corporation
*/

# include <compat.h>
# include <cv.h>		/* 6-x_PC_80x86 */
# include <st.h>		/* 6-x_PC_80x86 */
# include <er.h>
# include <si.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include <equel.h>
# include <fsicnsts.h>
# include <eqsym.h>		/* Needed by eqgen.h */
# include <eqgen.h>		/* Needed for IIMESSAGE, IIDISPFRM, etc */
# include <eqfw.h> 
# include <equtils.h>
# include <ereq.h>

/*
+*  Filename: 	eqfw.c 
**
**  Purpose:	Store and validate the options in a forms with clause
**
**  Defines:
**	EQFWopen()	- Logically open the EQFW Unit; initialize structures.	
**	EQFWwcoption()	- Define a forms with clause option.
**	EQFWsuboption() - Define a forms with clause suboption.
**	EQFWvalidate() 	- Perform overall validation of option list(now a noop)
**	EQFWdump()	- Dump the EQFW structures (for debugging)
**	EQFWgiget()	- Return pointer to EQFWGIinfo, access to EQFW structs 
**
**  Locals:
**	fw_idget()	- Given string, return id if valid
**	fw_numsave()	- For ints the unit generates (e.g.ids), save its 
**			  string rep 
**	fw_odget()	- Given option and suboption, find option description 
**	fw_ovset()	- Set an option value in option value list
**	fw_strget()	- Given id, return string
**	fw_wcidget()	- Given with clause option lhs id, rhs id, 
**	fw_valopt()	- Validates a given 'suboption=subvalue' combination.
**
**  History:
**	22-apr-1988	Written.	(marge)
**	13-jul-1988	Added FSBD_SINGLE for border option. (marge)
**	10-aug-1989	Added 80/132 screen width support for 8.0. (teresa)
**	30-July-1991 (fredv)
**		Renamed the variable strlen to strlength so that we take
**		advantage of the fast strlen function provided by the system
**		library. Otherwise, compiler complains redeclaration error.
**		No risk at all.
**	14-mar-1996 (thoda04)
**		Added function prototypes for static functions.
**		Added missing return(0) to fw_wcidget() if id combo not found.
**
**  Overview:
**	The EQFW unit validates and stores the option values in a forms 
**	with clause on a per-statement basis, after any special words have
**	been translated into their internal ids (e.g. "floating" becomes
**	FSV_FLOATING).  The table in which these option settings 
**	reside can then be accessed by code generation functions to 
**	generate the appropriate IIFR calls.
**
**  Rules of Use:
**   	The EQFW Unit is accessed in the following manner:
**		EQFWopen(statement_type)
**		EQFWwcoption(wcoption, wcvalue)
**		{ EQFW-access-statement }
**
**      where EQFW-access-statement = 
**		EQFWsuboption(option, type, ival, value) 
**   		| EQFWvalidate()
**		| EQFW3glgen()
**
**	1.  The EQFW unit must be logically opened using the EQFWopen call
**	    before any other EQFW call can be made.
**
**	2.  An EQFWwcoption must follow the EQFWopen and must be made before any
**	    EQFWsuboption calls are made.
**
**	3.  EQFWvalidate, EQFWsuboption, and EQFW3glgen calls are optional and
**	    can be made in any order as long as they follow an EQFWopen call.
**	
** Maintenance Notes for the EQFW Unit
**
**    I.  Adding a new statement to the EQFW Unit:
**
**		Create a new entry for the statement in the 
**		EQFWSM_STATEMENT_MASK.
**
**		Determine which with clause option/suboption pairs 
**		the statement will support. 
**
**		For each of these with clause option/suboption pairs in 
**		the EQFW option description table (EQFWODlist), 
**		include the statement in the stmtlist (od_stmtlist).
**
**		Edit the master grammar to include forms with clause support
**		for this statement.  (See MESSAGE in grammar as an example).
**
**    II. Adding A New With Clause Option to the Forms With Clause
**
**		Verify that the option name and value name 
**		(e.g "style", "popup", etc) are not reserved words in any 
**		other language.  If they are, special steps must be taken in 
**		that grammar to allow forms with clause productions to 
**		include that reserved word.
**
**		Add the option id to fsiconstants.h
**
**		Edit the with clause options list (EQFWWClist) to include 
**		the added option.
**
**		Determine which suboptions will be supported with this option.
**
**		For each (supported) suboption, add an entry in the EQFWODlist 
**		option description list for the new with clause 
**		option/suboption pair. 
**
**		Ensure that the runtime system routines know about this option
**		and the appropriate support actions are taken by the 
**		runtime routines.
**
**   III.  Adding A New Suboption to the Forms With Clause
**
**		Verify that the suboption name (e.g startrow, maxrow, etc)
**		is not a reserved word in any other language.  If it is, 
**		special steps must be taken in that grammar to allow forms 
**		with clause productions to include that reserved word.
**
**		Add the option id to fsiconstants.h
**
**		Edit the string translation list (EQFWSTlist) to include the 
**		added suboption and its id.
**
**		Determine which with clause options will support this new 
**		suboption.
**
**		For each with clause option which supports the new suboption, 
**		add an entry in the option description list (EQFWODlist) 
**		for the new suboption.
**
**		Ensure that the runtime system routines know about this 
**		suboption and the appropriate support actions are taken by 
**		the runtime routines.
**
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/*
**	Internal Functions
*/
i4  			fw_idget(char *s, i4  locid);
i4  			fw_id2rtid(i4 id);
char			*fw_numsave(i4 num);
i4  			fw_valopt(i4 valid, i4  optid);
IIFWOD_OPTION_DESC 	*fw_odget(i4 wcid, i4  opid, i4  stmtid);
VOID			fw_ovset(char * value, PTR varinfo, 
    			         IIFWOD_OPTION_DESC *, char *opstr);
char			*fw_strget(i4 id);
i4  			fw_wcidget(i4 opid, i4  valid, i4  stmtid);
i4  			fw_smget(i4 stmtid);

/*}
**  Name: EQFWSM_STATEMENT_MASK - II statement id-to-statement mask list
**
**  Description:
**	Used by the EQFW routines to translate internal statement id
**	to statement mask.
**
**  History:
**	30-mar-1988	Written.	(marge)
*/

# define  _DISP		  	0x001	
# define  _MESS			0x002
# define  _PROMPT		0x004

typedef struct {
    i4   sm_id; 		/* internal statement id */
    i4   sm_mask;		/* statement mask */
}  EQFWSM_STATEMENT_MASK;


static EQFWSM_STATEMENT_MASK EQFWSMlist[] =
{
/* statement id,  statement mask */
   {IIDISPFRM,	_DISP},
   {IIMESSAGE,	_MESS},
   {IINPROMPT,	_PROMPT},
   {0,0}
};

/*}
**  Name: EQFWST_STRING_TRANS - String-to-id table for forms with clause 
**
**  Description:
**	Used by the EQFW routines to translate forms with clause 
**	strings into internal integer ids.
**
**  History:
**	25-Mar-1988	Written (marge).	
*/

				/*  Bit masks for st_loc */
# define EQFWST_SUBOPTION	0x001
# define EQFWST_SUBVALUE	0x002
# define EQFWST_WCOPTION	0x004
# define EQFWST_WCVALUE		0x008

/*  Internal ids	
**	These are needed because there is a m:1 relationship between
**	special words and runtime ids 	(Example: "menuline" & "fullscreen"
**	both map to FSP_NOPOP)
*/

# define	_STYLE		1
# define	_POPUP		2
# define	_FULLSCREEN	3
# define	_MENULINE	4
# define	_SROW		5
# define	_SCOL		6
# define	_ROWS		7
# define	_COLS		8
# define	_BORDER		9
# define	_NONE		10
# define	_FLOATING	11
# define	_DEFAULT	12
# define 	_SINGLE		13
# define 	_SCRWID		14
# define 	_NARROW		15
# define 	_WIDE		16
# define 	_CURRENT	17


/*
**
**	Suboption mask used in verifying valid suboption and subvalue
**	combinations.
**
*/

# define  __SROW	  	0x001	
# define  __SCOL		0x002
# define  __ROWS		0x004
# define  __COLS		0x008
# define  __BORD		0x010
# define  __SCRW		0x020

typedef struct {
    i4   so_id; 		/* internal suboption id */
    i4   so_mask;		/* suboption mask */
}  EQFWSO_SUBOPTION_MASK;


static EQFWSO_SUBOPTION_MASK EQFWSOlist[] =
{
/* statement id,  statement mask */
   {_SROW,	__SROW},
   {_SCOL,	__SCOL},
   {_ROWS,	__ROWS},
   {_COLS,	__COLS},
   {_BORDER,	__BORD},
   {_SCRWID,	__SCRW},
   {0,0}
};

typedef struct {
    char  *st_string;
    i4    st_id;		/* internal id */
    i4	  st_rtid;		/* Runtime id for EQFWST_SUBVALUES */
    i4    st_loc;		/* legal locations for this string */
    i4	  st_sub;		/* valid suboptions for given subvalue */
}  EQFWST_STRING_TRANS;

static EQFWST_STRING_TRANS EQFWSTlist[] =
{
/*   string, 		eqfw id,     runtime id,  legal location    valid sub */
    {ERx("style"),	_STYLE,	     FSP_STYLE,	  EQFWST_WCOPTION,  0},
    {ERx("popup"),	_POPUP,	     FSSTY_POP,	  EQFWST_WCVALUE,   0},
    {ERx("fullscreen"), _FULLSCREEN, FSSTY_NOPOP, EQFWST_WCVALUE,   0},
    {ERx("menuline"), 	_MENULINE,   FSSTY_NOPOP, EQFWST_WCVALUE,   0},
    {ERx("startrow"), 	_SROW,	     FSP_SROW,	  EQFWST_SUBOPTION, 0},
    {ERx("startcolumn"),_SCOL,	     FSP_SCOL,	  EQFWST_SUBOPTION, 0},
    {ERx("rows"), 	_ROWS,	     FSP_ROWS,	  EQFWST_SUBOPTION, 0},
    {ERx("columns"),	_COLS,	     FSP_COLS,	  EQFWST_SUBOPTION, 0},
    {ERx("border"),	_BORDER,     FSP_BORDER,  EQFWST_SUBOPTION, 0},
    {ERx("screenwidth"),_SCRWID,     FSP_SCRWID,  EQFWST_SUBOPTION, 0},
    {ERx("none"),	_NONE,	     FSBD_NONE,	  EQFWST_SUBVALUE,  __BORD},
    {ERx("single"),	_SINGLE,     FSBD_SINGLE, EQFWST_SUBVALUE,  __BORD},
    {ERx("floating"),	_FLOATING,   FSV_FLOATING,EQFWST_SUBVALUE,  __SCOL|__SROW},
    {ERx("narrow"),	_NARROW,     FSSW_NARROW, EQFWST_SUBVALUE,  __SCRW},
    {ERx("wide"),	_WIDE,	     FSSW_WIDE,	  EQFWST_SUBVALUE,  __SCRW},
    {ERx("current"),	_CURRENT,    FSSW_CUR,	  EQFWST_SUBVALUE,  __SCRW},
    				/* Since "default" means 'take defined default 
				** as value', this is checked for by code */
    {ERx("default"),	_DEFAULT,    0,		  EQFWST_WCVALUE,   0},
    {(char *)0,0,0,0} 
};

/*}
**  Name: EQFWWC_WITH_CLAUSE_OPTION 
**		- string-to-id list for forms with clause option and value
**
**  Description:
**	Used by the EQFW routines to translate the forms with clause 
**	option and value from strings into their internal integer id.
**
**  History:
**	25-mar-1988	Written (marge).
**	13-may-1988	Added field for legal stmts (marge).
*/

typedef struct {
    i4    wc_opid;		/* option id */
    i4    wc_valid;		/* with clause value id */
    i4    wc_wcid;		/* with clause internal id */
    i4	  wc_stmtlist;		/* statements which allow this with clause */
} EQFWWC_WITH_CLAUSE_OPTION;

static EQFWWC_WITH_CLAUSE_OPTION EQFWWClist[] =
{
/*  wc opid,    wc value id, wc id */
    {_STYLE,	_POPUP,		FSSTY_POP,	_DISP|_PROMPT|_MESS},
    {_STYLE, 	_FULLSCREEN, 	FSSTY_NOPOP,	_DISP},
    {_STYLE, 	_MENULINE, 	FSSTY_NOPOP,	_PROMPT|_MESS},
    {_STYLE, 	_DEFAULT, 	FSSTY_DEF,	_DISP|_PROMPT|_MESS},
    {0,0,0}
};

/* EQFWODlist - Table of constant information for forms with validation 
** 	        The actual values of these options, if defined, are stored
**		in IIFWOV_OPTION_VALUE list pointed to by EQFWGIinfo.gi_ovp.
*/

static IIFWOD_OPTION_DESC EQFWODlist[] = {
/* wcid,    subopid,  rtsubopid, reqfl, stmtmask,   type,  default */
/*	STYLE = POPUP	*/
{FSSTY_POP,_STYLE,	FSP_STYLE,0,_DISP|_MESS|_PROMPT,DB_INT_TYPE,FSSTY_POP},
{FSSTY_POP,_SROW,	FSP_SROW,0, _DISP|_MESS|_PROMPT,DB_INT_TYPE,0},
{FSSTY_POP,_SCOL,	FSP_SCOL,0, _DISP|_MESS|_PROMPT,DB_INT_TYPE,0},
{FSSTY_POP,_ROWS,	FSP_ROWS,0, _MESS|_PROMPT,DB_INT_TYPE,0},
{FSSTY_POP,_COLS,	FSP_COLS,0, _MESS|_PROMPT,DB_INT_TYPE,0},
{FSSTY_POP,_BORDER,	FSP_BORDER,0, _DISP,DB_INT_TYPE,FSBD_RDFORM},
/*	STYLE = MENULINE | FULLSCREEN	( virtually "STYLE = NOPOP" ) */
{FSSTY_NOPOP,_STYLE,	FSP_STYLE,0,_DISP|_MESS|_PROMPT,DB_INT_TYPE,FSSTY_NOPOP},
{FSSTY_NOPOP,_SCRWID,	FSP_SCRWID,0, _DISP, DB_INT_TYPE, FSSW_DEF},
/*	STYLE = DEFAULT */
{FSSTY_DEF,_STYLE,	FSP_STYLE,0,_DISP|_MESS|_PROMPT,DB_INT_TYPE,FSSTY_DEF},
{0,0,0,0,0,0,0}
};

		/* Set max # of values to be # entries in EQFWODlist */
# define EQFWOVLIST_MAX (sizeof(EQFWODlist) / sizeof(IIFWOD_OPTION_DESC))

				/* info needed by module user */
static IIFWGI_GLOBAL_INFO    	EQFWGIinfo ZERO_FILL; 
static IIFWOV_OPTION_VALUE 	EQFWOVlist[EQFWOVLIST_MAX] ZERO_FILL;	

/*}
**  Name:	EQFWSS_stored_strings 
**
**  Description:
**		String buffer used to save any integers which must be
**		converted to strings for the scope of 1 statement.
**
**  History:
**	4-May-88 Written (marge).
*/

# define	EQFWSS_MAX_INT_STRING	6  /* Covers up to say, -2054+null */
# define	EQFWSS_SIZE		(EQFWOVLIST_MAX * EQFWSS_MAX_INT_STRING)

static char 	EQFWSS_stored_strings[ EQFWSS_SIZE ];
static char	*EQFWSSend;		/* Position to add new strings */


/*{
**  Name:  EQFWopen - 	Start a new with clause storage and validation
**
**  Description: 	
**	Initializes structures used by EQFW Unit.
**	Clears the option value table
**	Returns ptr to the global information structure needed by caller	
**
**  Inputs:
**	statement_id	- Id of statement owning the with clause
**			  Same as statement ids defined in eqgen.h
**  Outputs:
**	Returns: 
**	    Pointer to global information structure FWGIinfo
**	Errors:
**	    E_EQ0450_FWC_ILLEGAL_FWC 
**		This statement does not support the forms with clause. 
**
**  Example:
**
**  Source Code:
**	## message "Hi There"  with style = popup (startrow = 2000);
**
**  Internal call to begin EQFW access:
**	EQFWopen( IIMESSAGE , info_ptr);
**
*/
IIFWGI_GLOBAL_INFO *
EQFWopen(statement_id)
i4  	statement_id;
{
    IIFWOV_OPTION_VALUE	*ovp, *ovend;
    IIFWGI_GLOBAL_INFO	*gip= &EQFWGIinfo;

    gip->gi_ovp = EQFWOVlist; 			/* Initialize OVlist */

    EQFWSS_stored_strings[0] = '\0';		/* Null terminate string list */
    EQFWSSend = EQFWSS_stored_strings;

    /*  Initialize GIinfo */
    gip->gi_stmtid = statement_id;
    gip->gi_curwcid = 0; 
    gip->gi_rtopenflag = 0;		
    gip->gi_err = FALSE; 
    for (ovp = gip->gi_ovp, ovend = gip->gi_ovp + EQFWOVLIST_MAX; 
		ovp < ovend; ovp++)
    {
	ovp->ov_pval = NULL;
	ovp->ov_varinfo = (PTR)NULL;
	ovp->ov_odptr = NULL;
    }

    /* 
    ** Verify this statement_id is valid for any existing 
    ** with clause and any existing suboption 
    */
    if (fw_odget(0, 0, statement_id) == NULL)	/* Not really possible? */
    {
	frs_error(E_EQ0450_fwILLEGAL_FWC, EQ_ERROR, 0);
        gip->gi_err = TRUE; 
    }
    return(gip);
}



/*{
**  Name:  EQFWwcoption - 	- Defines the forms with clause option 
**
**  Description: 	
**	Verifies that the with clause option/value pair 
**	(i.e. "style = popup") is a valid pair and saves their definition
**
**  Inputs:
**	wcoption	- Left hand (option) side of with clause option
**	wcvalue		- Right hand (value) side of with clause option
**
**  Outputs:
**	Returns: 
**	    Nothing
**	Errors:
**	    E_EQ0451_fwINVALID_OP_SUBOP
**		Invalid with clause option or suboption: '%0c' = '%1c'
**	    E_EQ0452_fwBAD_COMBO
**		Invalid combination of statement, option, and suboption 
**		(current option: '%0c' = '%1c')
**	    E_EQ0455_fwTABLE_FULL	(from fw_ovset call)
**		Fatal attempt to access internal table.
**		An internal forms with clause table is full.
**	    E_EQ0454_fwOP_REDEF		(from fw_ovset call)
**		Forms with clause option '%0c' has been redefined.
**		The last definition will be used.
**
**  Example:
**  
**      Source Code: 
**	    ## message msg_var with style = popup; 
**
**	Internal call to set with clause option:
**	    EQFWwcoption("style", "popup");
**
**	Runtime call to be generated later:
**	    IIFRgpset(FSP_STYLE,(short *)0,0,30,4,FSSTY_POP);
**
*/
VOID
EQFWwcoption(wcoption, wcvalue)
char *wcoption;
char *wcvalue;
{
    i4  	wcopid;
    i4  	wcvalid;
    IIFWOD_OPTION_DESC 	*odp;
    IIFWGI_GLOBAL_INFO	*gip = &EQFWGIinfo;
    
    if ( (wcopid = fw_idget(wcoption, EQFWST_WCOPTION))==0
       ||(wcvalid= fw_idget(wcvalue,EQFWST_WCVALUE))==0)
    {
	frs_error(E_EQ0451_fwINVALID_OP_SUBOP, EQ_ERROR, 2, wcoption, wcvalue);
	gip->gi_err = TRUE;
    }
    else if ((gip->gi_curwcid = fw_wcidget(wcopid,wcvalid,gip->gi_stmtid))==0
       || (odp = fw_odget(gip->gi_curwcid, wcopid, gip->gi_stmtid))==NULL)
    {
	frs_error(E_EQ0452_fwBAD_COMBO, EQ_ERROR, 2, wcoption, wcvalue);
	gip->gi_err = TRUE;
    }
    else						/* SUCCESS */
    {
        fw_ovset(fw_numsave(odp->od_default), (PTR)NULL, odp, wcoption); 
    }
    return;
}



/*{
**  Name: EQFWsuboption	- Validate and define an option value 
**
**  Description:
**	.  Perform validation for this suboption-subvalue pair 
**  	.  Report an error if an attempt is made to set a suboption to a value 
**	which is the wrong type.
**	.  Any suboption value of type DB_CHR_TYPE will be examined to
**	determine if it is a special word.  Any special words which are 
**	available as suboption values will be translated and stored by the 
**	function as its translated integer string and its type will
**	be set appropriately. 
**	Special words (as of 8/10/89):
**		"default"
**		"none"
**		"floating"
**		"single"
**		"narrow"
**		"wide"
**		"current"
**	.  Define the option's value 
**
**  Inputs:
**	opstring	- option as input string
**	type		- type of value to be stored
**			  This is a DB Data type as defined in dbms.h
**	value		- ptr to char or var name
**	varinfo		- This is a ptr available to 3GL to store its
**			  SYM *, and to 4GL to store any ptr it may need
**			  to generate IIFR calls.
**			  Its contents are not examined by these routines, 
**			  but saved for code generation phase
**		 	  Note if varinfo != NULL, then this is a variable
**
**	Returns: 
**	    Nothing
**	Errors:
**	    E_EQ0451_fwINVALID_OP_SUBOP
**		Invalid with clause option or suboption: '%0c' = '%1c'
**	    E_EQ0452_fwBAD_COMBO
**		Invalid combination of statement, option, and suboption 
**		(current option: '%0c' = '%1c')
**	    E_EQ0453_fwTYPE_MISMATCH
**		Type of variable '%0c' does not match type for suboption '%1c'
**	    E_EQ0455_fwTABLE_FULL	(from fw_ovset call)
**		Fatal attempt to access internal table.
**		An internal forms with clause table is full.
**	    E_EQ0454_fwOP_REDEF		(from fw_ovset call)
**		Forms with clause option '%0c' has been redefined.
**		The last definition will be used.
**	    E_EQ0456_fw_BAD_VALUE
**		'%0c' is not a valid value for suboption '%1c'.
**  Example:
**
**    Source Code: (Hypothetical)
**    
**    ## display form_name UPDATE with style = popup 
**    		(startrow = FLOATING, startcolumn = integer_var, 
**		 border = default)
**
**    Internal calls to define these options:
**    
**    	 EQFWsuboption("startrow", DB_INT_TYPE, "FLOATING", NULL); 
**    	 EQFWsuboption("startcolumn", DB_INT_TYPE, gr->gr_id,  gr->gr_sym); 
**    	 EQFWsuboption("border", DB_FLT_TYPE, "default", NULL); 
**
**    Should result in calls to IIFR by the code generation function which
**    reads the option value table:
**
**	IIFRgpcontrol(1,0);			* IIFR open *
**	IIFRgpsetio(6,(short *)0,0,30,4,301);	* Set style = popup *
**	IIFRgpsetio(1,(short *)0,0,30,4,-1);	* Set startrow *
**	IIFRgpsetio(2,(short *)0,1,30,4,&integer_var);	* Set startcolumn *
**	IIFRgpsetio(5,(short *)0,0,30,4,200);	* Set border *
**	IIFRgpcontrol(2,0);			* IIFR close *
*/
VOID
EQFWsuboption(option, type, value, varinfo ) 
char	*option;
i4	type;
char	*value;
PTR	varinfo;
{
    i4  		opid;
    i4			valid;		/* set if value is special word */
    IIFWOD_OPTION_DESC 	*odp;
    IIFWGI_GLOBAL_INFO	*gip = &EQFWGIinfo;

    if ((opid = fw_idget(option, EQFWST_SUBOPTION)) == 0)
    {
	frs_error(E_EQ0451_fwINVALID_OP_SUBOP, EQ_ERROR, 2, option, value);
	gip->gi_err = TRUE;
    }
    else if ((odp = fw_odget(gip->gi_curwcid, opid, gip->gi_stmtid)) == NULL)
    {
	frs_error(E_EQ0452_fwBAD_COMBO, EQ_ERROR, 2, option, value);
	gip->gi_err = TRUE;
    }
    else
    { 
    /* 
    **	Convert value to appropriate internal representation 
    */
	if (varinfo != NULL)
	{
	    fw_ovset(value, varinfo, odp, option);
	}
	else if ( (STbcompare (value, 0, ERx("default"), 0, TRUE) == 0) 
	         || (STbcompare (value, 0, "0", 0, TRUE) == 0) )
	{
	    type = DB_INT_TYPE;
            fw_ovset(fw_numsave(odp->od_default), varinfo, odp, option);
	}
	else 
	{
	    if (type == DB_CHR_TYPE 	   	/* special word? */
	       && (valid = fw_idget(value, EQFWST_SUBVALUE)) != 0)
	    {
		if (fw_valopt(valid, opid) == 0)
		{
	    	    frs_error(E_EQ0456_fwBAD_VALUE, EQ_ERROR, 2, value, option);
	    	    gip->gi_err = TRUE;
		    return;
		}
		else
		{
	            type = DB_INT_TYPE;
                    fw_ovset(fw_numsave(fw_id2rtid(valid)), varinfo, odp, option);
		}
	    }
	    else
	    {					/* It's not a special word */
	        fw_ovset(value, varinfo, odp, option);
	    }
	}
	if (type != odp->od_type)		/* Type Mismatch? */
	{
	    frs_error(E_EQ0453_fwTYPE_MISMATCH, EQ_ERROR, 2, value, option);
	    gip->gi_err = TRUE;
	}
    }
}



/*{
**  Name: EQFWvalidate - Perform overall validation of the forms with clause entries
**
**  Description:
**	Verify that all required fields have been defined
**	Since there are no required fields at present, this function does nothing.
**
**  Inputs:
**	None
**  Outputs:
**	Returns: Nothing
**	Errors:  None
*/
VOID
EQFWvalidate()
{
	/* does nothing for now */
}




/*{
** 
** Procedure:	EQFWdump
** Purpose:	Dump the global forms with clause structures for debugging.
**
** Parameters:	None
-* Returns:	None
**
** History:
**		1-apr-1988	 - Written (marge)
**
*/
void
EQFWdump()
{
    IIFWOV_OPTION_VALUE		*ovp;
    IIFWOD_OPTION_DESC  	*odp;
    IIFWGI_GLOBAL_INFO	*gip = &EQFWGIinfo;

    FILE *df = eq->eq_dumpfile;
    i4  i;

    gip = &EQFWGIinfo;
    SIfprintf(df, 
	ERx("\n----------------------------------------------------------------------\n"));
    SIfprintf(df, ERx("\nEQFWGIinfo: Global Information Structure Dump\n") );
    SIfprintf(df, 
	ERx("\n----------------------------------------------------------------------\n"));
    SIfprintf(df, 
        ERx("gi_stmtid:\tStatement id\t\t%d\n"), gip->gi_stmtid);
    SIfprintf(df, 
        ERx("gi_curwcid:\tCurrent Pair Id\t\t%d\n"), gip->gi_curwcid);
    SIfprintf(df, 
        ERx("gi_rtopenflag:\tRun-Time Open Flag\t%d\n"), 
	gip->gi_rtopenflag);
    SIfprintf(df, 
        ERx("gi_err:\t\tWith Clause Error Exists?\t%s\n"), gip->gi_err ? ERx("TRUE"):ERx("FALSE"));
    SIfprintf(df, 
	ERx("\n----------------------------------------------------------------------\n"));
    SIfprintf(df, ERx("\nOPTION VALUE TABLE CONTAINS:\n"));
    SIfprintf(df, 
ERx("Entry#   PVAL  VARINFO  WCID   PID=STRING      TYPE    DEFAULT"));
    SIfprintf(df, 
	ERx("\n----------------------------------------------------------------------\n"));
	
    /* Test ovp for zero in case there has been no EQFWopen init */
    for (ovp = gip->gi_ovp, i=0; 
	    i < EQFWOVLIST_MAX && ovp != 0 && ovp->ov_pval != NULL; 
	    ovp++, i++)
    {
	odp = ovp->ov_odptr;
  	SIfprintf(df, 
	    ERx("[%d]:\t %s\t%s\t%d\t%d=%s\t\t%d\t%d\n"),
            i, ovp->ov_pval, ovp->ov_varinfo, odp->od_wcid, odp->od_rtsubopid, 
        fw_strget(odp->od_subopid), odp->od_type, odp->od_default);
    }
    SIfprintf(df, 
        ERx("\n----------------------------------------------------------------------\n"));
    SIfprintf(df, "\n");
    SIflush( df );
} /* EQFWdump */



/*{
** 
** Procedure:	EQFWgiget
** Purpose:     Return a pointer to EQFWGIinfo	
** Parameters:	None
-* Returns: 	IIFWGI_GLOBAL_INFO	*	
** History:
**		1-apr-1988	 - Written (marge)
**
*/
IIFWGI_GLOBAL_INFO *
EQFWgiget()
{
	return(&EQFWGIinfo);
}
/*****************************************************************************
**
**		Internal Routines for EQFW Unit
**
******************************************************************************
*/



/*{
**  Name: 	fw_idget
**
**  Description: 	
**  	Translate string into its id if the string is valid for that location
**
**  Inputs:
**	s		* string to be translated
**	locid		* valid location for string
**  Outputs:
**	Returns: 
**		Id for string/loc
**		| 0	* not found
**	Errors:
**		None.
**
*/
i4
fw_idget(s, locid)
char 	*s;		/* string to translate */
i4  	locid;		/* legal location */
{
    EQFWST_STRING_TRANS 	*stp;

    /* Look for special word in string table */

    for (stp = EQFWSTlist; stp->st_string != NULL; stp++)
    {
	if(STbcompare(s, 0, stp->st_string, 0, TRUE) == 0	/* match s */
	   && (locid == 0 || (stp->st_loc & locid)))	       	/* OK w/loc? */
		return(stp->st_id);			       	/* OK! */
    }
    return(0);
}   

/*{
**  Name: 	fw_id2rtid
**
**  Description: 	
**  	Determine runtime id given internal id
**
**  Inputs:
**	id:	internal id for special word
**  Outputs:
**	Returns: 
**		runtime id
**	Errors:
**		None.
**
*/
i4
fw_id2rtid(id)
i4  	id;		/* internal id */
{
    EQFWST_STRING_TRANS 	*stp;

    /* Look for id in string table */

    for (stp = EQFWSTlist; stp->st_string != NULL; stp++)
    {
	if(stp->st_id == id)
		return(stp->st_rtid);			       	/* OK! */
    }
    return(0);
}   

/*{
**  Name:	fw_numsave 
**
**  Description:
**	Takes an integer, converts it into a string, and stores that
**	string in a buffer which will be available for the scope of
**	this language statement (That is, until a new EQFWopen).
**  Inputs:
**	num	- number to be converted and stored
**  Returns:
**	Pointer to safely stored string representation of number
**  Errors:
**	E_EQ0455_fwTABLE_FULL
**		Fatal attempt to access internal table.
**		An internal forms with clause table is full.
**
*/
char	*
fw_numsave(num)
i4	num;
{
    	IIFWGI_GLOBAL_INFO	*gip = &EQFWGIinfo;
	char	cnvtval[EQFWSS_MAX_INT_STRING*2]; /* Double size for safety */
	i4	strlength;
	char	*newstring;	

	/*
	** Use cnvtval buf & strlength to check for overflow before storing 
	*/
	CVna(num, cnvtval);		/* Convert to string */
	strlength = STlength(cnvtval) + 1;
	if (EQFWSSend + strlength > EQFWSS_stored_strings + EQFWSS_SIZE)
	{				/* Buffer overflow */
		frs_error(E_EQ0455_fwTABLE_FULL, EQ_ERROR, 0);
		gip->gi_err = TRUE;
		return(ERx("$$$"));	/* Return weird string */
	}
	STcopy(cnvtval, EQFWSSend);
	newstring = EQFWSSend;
	EQFWSSend = EQFWSSend + strlength + 1;	
	return(newstring);
}	


/*{
**  Name: 	fw_valopt
**
**  Description: 	
**  	Verify that the suboption and subvalue are a valid combination.
**
**  Inputs:
**	s		* subvalue id
**	locid		* suboption id
**  Outputs:
**	Returns: 
**		0	* Illegal combination
**		!= 0	* Good combination
**	Errors:
**		None.
**
*/
i4
fw_valopt(valid, optid)
i4	valid;		/* subvalue id number */
i4  	optid;		/* suboption id number */
{
    EQFWST_STRING_TRANS 	*stp;
    EQFWSO_SUBOPTION_MASK	*smp;
    i4				optmask;
    i4				valmask;

    optmask = 0;
    valmask = 0;
    /* Look up subvalue in string table get valid suboption mask */

    for (stp = EQFWSTlist; stp->st_string != NULL; stp++)
    {
	if(stp->st_id == valid)				/* match subvalue id */
		valmask = stp->st_sub;	
    }

    /* Look up current suboption - get it's mask value */

    for (smp= EQFWSOlist; smp->so_id != 0; smp++)
    {
	if(smp->so_id == optid)				/* match suboption id */
		optmask = smp->so_mask;
    }	
    return(valmask & optmask);
}   

/*{

/*{
**  Name: 	fw_odget	
**
**  Description:  
**	Get the description of the option indicated by
**	the combination of the option id, suboption id,
**	and statement.
**
**  Inputs:
**	 wcid	- with clause option id
**	 opid	- option or suboption id
**	 stmtid - statement id
**  Outputs:
**	Returns: Pointer to option description or NULL if not found
**	Errors:
**
*/
IIFWOD_OPTION_DESC *
fw_odget(wcid, opid, stmtid)
i4  wcid;
i4  opid;
i4  stmtid;
{
    IIFWOD_OPTION_DESC *odp;
    i4  stmtmask;

    if ((stmtmask = fw_smget(stmtid)) != 0)
    {
        for (odp = EQFWODlist; odp->od_wcid != 0; odp++)
        {
            if ( (odp->od_wcid == wcid || wcid == 0) 
	       &&(odp->od_subopid == opid || opid == 0)
	       &&((odp->od_stmtlist & stmtmask) || stmtid == 0))
    	        	return (odp);
	}
    }
    return (NULL);
}



/*{
**  Name: 	fw_ovset
**
**  Description: 	
**	Store the option value information in the ov table
**
**  Inputs:
**	value	- string representation of value 
**	type	- type for value 
** 	varinfo	- some kind of ptr to variable info | NULL 
**	opstr	- string representation of option
**  Outputs:
** 	odptr	- returns pointer into option description table
**  Returns: 
**	nothing
**  Errors:
**	E_EQ0455_fwTABLE_FULL
**		Fatal attempt to access internal table.
**		An internal forms with clause table is full.
**	E_EQ0454_fwOP_REDEF
**		Forms with clause option '%0c' has been redefined.
**		The last definition will be used.
**
*/
VOID
fw_ovset(value, varinfo, odptr, opstr)
char 			*value;		/* string representation of value */
PTR 			varinfo;
IIFWOD_OPTION_DESC 	*odptr;
char			*opstr;
{

    IIFWOV_OPTION_VALUE *ovp, *ovpend;
    IIFWGI_GLOBAL_INFO	*gip = &EQFWGIinfo;

    /* 	NOTE: 	Last entry (gip->ovp[EQFWOVLIST_MAX-1])
    **		in ovlist must always be nulls to indicate end of table
    **		for any users of this global information.
    **		This is initialized on an EQFWopen and protected here
    */
    ovpend = gip->gi_ovp + EQFWOVLIST_MAX;
    for (ovp = gip->gi_ovp; ; ovp++)
    { 
	if (ovp == ovpend)		/* Option value table is full */
 	{
	    frs_error(E_EQ0455_fwTABLE_FULL, EQ_ERROR, 0);
	    gip->gi_err = TRUE;
	    return;
	}
	else if (ovp->ov_odptr == odptr)	/* option already defined */
	{
	    frs_error(E_EQ0454_fwOP_REDEF, EQ_WARN, 1, opstr);
	    break;
	}
	else if (ovp->ov_pval== NULL)	/* Empty place found! */
	    break;
    }
    ovp->ov_pval= value;
    ovp->ov_varinfo = varinfo;
    ovp->ov_odptr = odptr;
}



/*{
**  Name: 	fw_strget
**
**  Description: 	
**  	Translate id to string for debugging routine.
**  Inputs:
**	id	- id to be translated
**  Outputs:
**	Returns: 
**		string pointer | NULL if not found
**
*/
char	*
fw_strget(id)
i4  	id;
{
    EQFWST_STRING_TRANS 	*stp;

    /* Look for special word in string table */

    for (stp = EQFWSTlist; stp->st_string != NULL; stp++)
    {
	if (stp->st_id == id)
		return (stp->st_string);
    }
    return(NULL);
}   



/*{
**  Name: 	fw_wcidget
**
**  Description: 	
**	Given ids for option left-hand-side & right-hand-side, return
**	an id for this combination.
**	Example:  style = popup results in an internal call to
**	fw_wcidget(_STYLE, _POP) which returns FSSTY_POP.
**	This is the id which currently represents this combination
**	and which is used as a key to the option description table.
**
**  Inputs:
**	opid	- id for left-hand-side of with clause option
**	valid	- id for right-had-side of with clause option
**	stmtid  - id for statement, used to determine if this is valid together
**  Returns: 
**	with clause id | 0 if not found
**
*/
i4
fw_wcidget(opid, valid, stmtid)
i4  opid, valid, stmtid;
{
    EQFWWC_WITH_CLAUSE_OPTION *wcp;
    i4		stmtmask;

    stmtmask = fw_smget(stmtid);
    for(wcp = EQFWWClist; wcp->wc_opid != 0; wcp++)
    {
	if(wcp->wc_opid == opid 
	   && wcp->wc_valid == valid 
	   && (wcp->wc_stmtlist & stmtmask) )
	{
	    return (wcp->wc_wcid);
	}
    }
    return(0);
}
i4
fw_smget(stmtid)
i4	stmtid;
{

    EQFWSM_STATEMENT_MASK	*smp;

    /* Get statement's mask */
    for (smp = EQFWSMlist; smp->sm_id != 0; smp++)
    {
	if (smp->sm_id == stmtid)
	    break;
    }
    return(smp->sm_mask);
}
