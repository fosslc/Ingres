/*
**  frconsts.h
**
**  Constants used throughout all parts of the forms system.
**
**  Copyright (c) 2004 Ingres Corporation
**
**  History:
**
**	Created - 07/29/85 (dkh)
**  12/22/86 (dkh) - Added support for new activations.
**
 * Revision 60.2  86/11/19  13:36:02  barbara
 * Initial60 edit of files
 * 
 * Revision 60.1  86/11/19  13:35:50  barbara
 * *** empty log message ***
 * 
 * Revision 4002.2  86/05/08  12:00:50  rpm
 * vms/unix integration.
 * 
 * Revision 40.3  86/05/07  13:14:52  dave
 * Documented some constants. (dkh)
 *
 * Revision 4002.1  86/04/15  21:44:27  root
 * 4.0/02 certified codes
 * 
 * Revision 40.2  85/12/05  20:37:23  dave
 * Added support for activation on a field and made constants unique
 * in first 8 chars. (dkh)
 * 
 * Revision 40.1  85/08/22  15:52:40  dave
 * Initial revision
 * 
 * Revision 1.1  85/08/09  11:21:39  dave
 * Initial revision
 * 
*/

/*
**  The maximum number of frs keys is arrived at by adding
**  the maximum number of control keys and maximum number
**  of function keys.  If any of the latter two numbers
**  change, this constant must also change for things
**  to work correctly.
*/

# define	MAX_FRS_KEYS		68	/* max number of FRSKEYS */
# define	FRS_NO			0	/* don't validate for FRSKEY */
# define	FRS_YES			1	/* validate for FRSKEY */
# define	FRS_UF			2	/* don't care for FRSKEY */

/*
**  Constants for deciding when to validate/activate on a field
*/

# define	SETVALNEXT		0	/* validate to next field */
# define	SETVALPREV		1	/* validate to prev field */
# define	SETVALMENU		2	/* validate on menu key */
# define	SETVALANMU		3	/* validate on any menuitem */
# define	SETVALFRSK		4	/* validate on frs key */
# define	SETACTNEXT		5	/* activate to next field */
# define	SETACTPREV		6	/* activate to prev field */
# define	SETACTMENU		7	/* activate on menu key */
# define	SETACTANMU		8	/* activate on any menuitem */
# define	SETACTFRSK		9	/* activate on frs key */
