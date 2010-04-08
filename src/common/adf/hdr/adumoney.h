/*
** Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: ADUMONEY.H - defines the internal representation for money in 
**		      the INGRES money abstract data type.
**
** Description:
**        This file defines the interal representation for the Ingres
**	abstract datatype, money.  Its also defines various magic constanst
**	associated with the money datatype.
**
** History: $Log-for RCS$
**      22-may-86 (ericj)
**          Converted for Jupiter.
**	19-jun-86 (ericj)
**	    Moved histogram value definitions from this file to adfhist.h
**	25-nov-86 (thurston)
**	    Renamed most of the constants defined here to conform to Jupiter
**	    coding standards.
**      04-jan-1993 (stevet)
**          Added function prototypes for adu_11mny_round.
**/

/*
[@forward_type_references@]
[@forward_function_references@]
*/


/*
**  Defines of other constants.
*/

/*
**  Define constants for various lengths associated with money datatype.
*/

/* Define the length of internal money representation. */
# define	ADF_MNY_LEN		sizeof (AD_MONEYNTRNL)

/* Define the money output length */
# define	AD_5MNY_OUTLENGTH	20

/* Define the value used when rounding money. */
# define	AD_7MNY_ROUND_VAL	1.0


/* 
** Max and min external and internal money values allowed
*/

#ifdef IEEE_FLOAT	/* two less digit of precision than VAX D_FLOAT type */

#   define	AD_1MNY_MAX_NTRNL   ( 99999999999999.00)
#   define	AD_2MNY_MAX_XTRNL   (   999999999999.99)
#   define	AD_3MNY_MIN_NTRNL   (-99999999999999.00)
#   define	AD_4MNY_MIN_XTRNL   (  -999999999999.99)

    /* Define the maximum number portion of money length */
#   define	AD_6MNY_NUMLEN		18

#else

#   define	AD_1MNY_MAX_NTRNL   ( 9999999999999999.00)
#   define	AD_2MNY_MAX_XTRNL   (   99999999999999.99)
#   define	AD_3MNY_MIN_NTRNL   (-9999999999999999.00)
#   define	AD_4MNY_MIN_XTRNL   (  -99999999999999.99)

    /* Define the maximum number portion of money length */
#   define	AD_6MNY_NUMLEN		20

#endif

/*
**  Constants used for error messages with adt_error.  These values will
** be replaced by error number definitions in ADF.h.  They're being kept
** around for now to make sure we have all the errors covered.
*/
/*
# define	BADCH_MONEY	4400
# define	MAX_MNY		4401
# define	MIN_MNY		4402
# define	BLANKS_MNY	4403
# define	MDOLLARSGN	4404
# define	MSIGN_MNY	4405
# define	DECPT_MNY	4406
# define	COMMA_MNY	4407
# define	SIGN_MNY	4408
# define	DOLLARSGN	4409
# define	BAD_MNYDIV	4410
*/

/*
[@global_variable_references@]
*/


/*}
** Name: AD_MONEYNTRNL - Internal representation for money.
**
** Description:
**        This struct defines the internal representation for money.
**
** History:
**	22-may-86 (ericj)
**          Converted for Jupiter.
**	25-nov-86 (thurston)
**	    Renamed this typedef to conform to Jupiter coding standards.  Old
**	    name was MONEYNTRNL.  Also, renamed member .cents to be .mny_cents.
*/

typedef struct
{
    f8	    mny_cents;
} AD_MONEYNTRNL;

/*  [@type_definitions@]    */


/*
**  Forward and/or External function references.
*/


FUNC_EXTERN DB_STATUS adu_11mny_round(ADF_CB         *adf_scb,
				      AD_MONEYNTRNL  *a);


