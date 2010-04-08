/*
** Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: ADUSTRCMP.H - Definitions of items needed for key building and string
**		       comparisons for the string datatypes; especially
**		       pattern match related stuff.
**
** Description:
**      This file contains definitions needed by a couple of finite state
**      machines used in the process of comparing strings (and building keys for
**	such comparisons) for the various INGRES string datatypes that can have
**	pattern match characters.
**
** History: $Log-for RCS$
**      22-jun-88 (thurston)
**          Initial creation.
[@history_template@]...
**/


/*
**  Define the character classes used by the finite state machines:
*/
#define	    AD_CC0_NORMAL	0   /* any char not in another class      */
#define	    AD_CC1_DASH		1   /* the non-escaped dash ('-')         */
#define	    AD_CC2_ONE		2   /* the `MATCH-ONE' character          */
#define	    AD_CC3_ANY		3   /* the `MATCH-ANY' character          */
#define	    AD_CC4_LBRAC	4   /* the `BEGIN-RANGE' character        */
#define	    AD_CC5_RBRAC	5   /* the `END-RANGE' character          */
#define	    AD_CC6_EOS		6   /* no char ... end of the pattern str */

/*
**  Define the states for the finite state machines:
*/
#define	    AD_S0_NOT_IN_RANGE		0   /* not in a range ... start here */
#define	    AD_S1_IN_RANGE_DASH_NORM	1   /* in range, dash is normal      */
#define	    AD_S2_IN_RANGE_DASH_IS_OK	2   /* in range, dash is ok          */
#define	    AD_S3_IN_RANGE_AFTER_DASH	3   /* in range, after a dash        */
#define	    AD_S4_IN_RANGE_DASH_NOT_OK	4   /* in range, dash is NOT ok      */
#define	    AD_SX_EXIT			99  /* exit                          */

/*
**  Define the actions performed by the finite state machines; the actual
**  actions taken depend on what the operator was:
*/
#define	    AD_ACT_Z	 0
#define	    AD_ACT_A	 1
#define	    AD_ACT_B	 2
#define	    AD_ACT_C	 3
#define	    AD_ACT_D	 4
#define	    AD_ACT_E	 5
#define	    AD_ACT_F	 6
#define	    AD_ACT_G	 7
#define	    AD_ACT_H	 8
#define	    AD_ACT_I	 9
#define	    AD_ACT_J	10
#define	    AD_ACT_K	11
#define	    AD_ACT_L	12


/*}
** Name: AD_TRAN_STATE - Used as an element in a transition/state table.
**
** Description:
**      Each transition/state table will be a 2-dimensional array of these
**	structures.  The rows of the table will represent the current state
**	of the finite state machine.  The columns will represent the character
**	class of the next character.  Each of the elements contains two pieces
**	of information:  (1) The action to take when moving to ... (2) the next
**	state.
**
**	At least one of these tables are used in the key build process for the
**	SQL LIKE operator.
**
** History:
**      22-jun-88 (thurston)
**          Initial creation.
[@history_template@]...
*/

typedef struct _AD_TRAN_STATE
{
    i4              ad_action;	    /* The action to take when moving to... */
    i4              ad_next_state;  /* ... the next state */
}   AD_TRAN_STATE;
