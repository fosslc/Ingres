/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	eqfw.h - EQFW users' forms with clause header file
**
** Description:
**	Contains the function and structure declaration and definitions for 
**	users of the EQFW module which stores and validates the
**	forms with clause options for the language preprocessors.
**
** History:
**	Revision 6.1
**	22-apr-1988	Written.  (marge)
**	13-may-1988	Removed unused defines: IIFW_MAX_VALUE_LENGTH,
**			EQFW_MAX_ID_LENGTH.  (marge) 
**	18-may-1988	Clarified comments regarding od_rtsubopid.  (marge)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/*}
**  Name:	IIFWOD_OPTION_DESC -	Description of forms with clause options
**
**  Description:
**	This structure defines the static option/value information in a forms
**	with clause.  This information is used by the EQFW routines
**	to validate the options and can be used with option value list 
**	pointed to be gi_ovp, to generate the appropriate runtime calls 
**	to the IIFRgp module.
**
**  Notes:
**	1.  Each option value entry in the option value list has a pointer
**	    back into its appropriate option description in this structure.
**      2.  The current implementation of the forms with clause does
**	    not use the full capabilities of the clause.  Currently:
**		a.  All od_reqflag == FALSE
**
**  History:
**	25-mar-1988	Written (marge).
*/

typedef struct {
    i4      od_wcid;		/* with clause pair id */
    i4      od_subopid;		/* internal suboption id */
    i4      od_rtsubopid;	/* runtime option id (IIFRgp's "pid") */
    i4      od_reqflag;		/* (UNUSED) TRUE=required | FALSE=optional */
    i4      od_stmtlist;	/* legal statements for this combo */
				/* of with clause option and option */
    i4      od_type;		/* value type expected */
    i4	    od_default;		/* default value of option */
} IIFWOD_OPTION_DESC;

/*}
**  Name: IIFWOV_OPTION_VALUE	- dynamic value information for fwc option
**
**  Description:
**	Array which holds the user-supplied value information for the 
**	forms with clause options
**
**  History:
**	28-mar-1988	Written. 	(marge)	
*/

typedef struct {
    char    			*ov_pval;	
    PTR    			ov_varinfo;	/* value user var ptr */
    IIFWOD_OPTION_DESC		*ov_odptr;  	/* ptr to IIFWODlist entry */
} IIFWOV_OPTION_VALUE;

/*}
**  Name: IIFWGI_GLOBAL_INFO - Global info re: current forms with clause processing 
**
**  Description:
**	Holds the value information needed to generate IIFR calls to
**	the runtime system.  This includes the type of statement which owns 
**	the forms with clause, whether or not an error has occurred during 
**	the processing of this statement, and a ptr into the entry into the 
**	forms with list table.  It is initialized by an EQFWopen call.
**
**  History:
**	25-mar-1988	Written.  (marge)	
*/

typedef struct {
    i4	 		gi_stmtid;  	/* Statement id of fwc owner         */
					/* Set by EQFWopen call              */
    i4   		gi_curwcid;	/* Current with clause option id     */
					/* May change within stmt processing */
    i4	 		gi_rtopenflag;	/* (UNUSED) Val of flag for IIFRgp   */
					/* open call.  Set by EQFWopen 	     */
    bool 		gi_err;		/* TRUE indicates error has occurred */
					/* Set in EQFWset or EQFWvalidate    */	

    IIFWOV_OPTION_VALUE *gi_ovp; 	/* Ptr to option value list */
	/*  gi_ovp: Points to the option value list which holds the 
	**  definable part of an option.
	**  Any options from option description list, IIFWODlist which
	**  have been given a value, will be defined in this list along
	**  with a ptr back into the IIFWODlist to its corresponding static
	**  option descriptor.
	**  Subsequent EQFWsuboption/EQFWwcoption calls add new definitions to 
	**  this list.
	*/
} IIFWGI_GLOBAL_INFO;

FUNC_EXTERN VOID	EQFWdump(void);
FUNC_EXTERN IIFWGI_GLOBAL_INFO	*EQFWopen(i4 statement_id);
FUNC_EXTERN VOID	EQFWsuboption(char *option, i4  type, 
                	              char *value, PTR varinfo);
FUNC_EXTERN VOID	EQFWwcoption(char *wcoption, char *wcvalue);
FUNC_EXTERN VOID	EQFWvalidate(void);
FUNC_EXTERN VOID	EQFW3glgen();
FUNC_EXTERN IIFWGI_GLOBAL_INFO	*EQFWgiget(void);

/*{
**  Name:	EQFW_ISDEFAULT() -	Is Option Value Default?  (MACRO)
**
**  Description:
**	Determine if the option value is same as default.
**
**  Note:
**	This is not a check against the stored default value, since THAT
**	default must be sent in a runtime IIFR call.  This function is
**	looking for a value of 0.
**
**  Returns:
**	{bool}	TRUE:	value is 0
**		FALSE:	value is not same as default or it is unknown
**
** History:
**	05/88 (jhw) -- Written from MargeB's 'fw_isdefault()'.
*/
#define EQFW_ISDEFAULT(ovp) ( (ovp)->ov_varinfo == NULL && \
				STequal((ovp)->ov_pval, "0") )
