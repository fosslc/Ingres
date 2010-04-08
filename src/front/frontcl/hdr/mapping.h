/*
**  mapping.h
**
**  Header file for the new control/function key mapping facility.
**
**  Copyright (c) 2004 Ingres Corporation
**
**  History:
**	Created - 07/09/85 (dkh)
**	85/07/19  dave
**		Initial revision
**	85/08/09  dave
**		Last set of changes for function key support. (dkh)
**	85/08/22  dave
**		Initial revision
**	85/10/05  dave
**		Added new constants ftPRSCR and ftSKPFLD. (dkh)
**	85/11/04  dave
**		Added constant DEF_BY_TCAP. (dkh)
**	86/04/15  root
**		4.0/02 certified codes
**	86/05/07  dave
**		Documented some of the structures and constants. (dkh)
**	86/05/08  rpm
**		vms/unix integration.
**	86/05/08  bruceb
**		correct include syntax (angle brackets).
**	86/11/19  barbara
**		Initial60 edit of files
**	12/23/86 (dkh) - Added support for new activations.
**	24-sep-87 (bab)
**		Added constant ftSHELL, for the shell function key.
**	02/27/88 (dkh) - Added definition of ftNXITEM.
**	08/03/89 (dkh) - Added support for mapping arrow keys.
**	01/11/90 (esd) - Modifications for FT3270 (bracketed by #ifdef)
*/


/*
**  Warning !!!!!!!!!!!!!!!
**
**  Do NOT update constants in this file without making the
**  corresponding changes in fsiconsts.h in the frontend
**  header directory. (dkh)
*/

# include	<frscnsts.h>


struct mapping
{
	char	*label;		/* label for control/function key */
	i2	cmdval;		/* command key is mapped to */
	i1	cmdmode;	/* mode command is valid in */
	i1	who_def;	/* who defined key; user, application, etc. */
};


struct frsop
{
	char	*exp;		/* explanation of FRSKEY, unused */
	i2	intrval;	/* value to return if selected */
	i1	val;		/* validate flag */
	i1	act;		/* activate flag */
};

GLOBALREF	char		*menumap[];
GLOBALREF	struct	mapping	keymap[];
GLOBALREF	struct	frsop	frsoper[];

# ifdef  FT3270
# define	MAX_CTRL_KEYS		0       /* max number of control keys */
# else
# define	MAX_CTRL_KEYS		28	/* max number of control keys */
# endif /* FT3270 */

# define	CTRL_ESC		26	/* magic constant for ESCAPE */
# define	CTRL_DEL		27	/* magic constant for DELETE */

# define	NO_DEF_YET		0	/* not currently defined */
# define	DEF_INIT		1	/* default definition */
# define	DEF_BY_SYSTEM		2	/* def system definition */
# define	DEF_BY_TCAP		3	/* default for terminal */
# define	DEF_BY_USER		4	/* user has set this */
# define	DEF_BY_APPL		5	/* application has set this */
# define	DEF_BY_EQUEL		6	/* set via set/inquire stmts. */


# define	ftKEYLOCKED		1000	/* key is locked */
# define	ftNOTDEF		1001	/* key is undefined */


# define	ftCMDOFFSET		2000	/* offset for forms commands */

# define	ftNXTFLD		2000	/* next field command */
# define	ftPRVFLD		2001	/* previous field command */
# define	ftNXTWORD		2002	/* next word command */
# define	ftPRVWORD		2003	/* previous word command */
# define	ftMODE40		2004	/* toggle editing mode */
# define	ftREDRAW		2005	/* refresh terminal screen */
# define	ftDELCHAR		2006	/* delete char under cursor */
# define	ftRUBOUT		2007	/* delete char left of cursor */
# define	ftEDITOR		2008	/* start editor on field */
# define	ftLEFTCH		2009	/* move left one char */
# define	ftRGHTCH		2010	/* move right one char */
# define	ftDNLINE		2011	/* move down one line */
# define	ftUPLINE		2012	/* move up one line */
# define	ftNEWROW		2013	/* new row or next field */
# define	ftCLEAR			2014	/* clear field */
# define	ftCLRREST		2015	/* clear to end of field */
# define	ftMENU			2016	/* menu key pressed */
# define	ftUPSCRLL		2017	/* scroll form up */
# define	ftDNSCRLL		2018	/* scroll form down */
# define	ftLTSCRLL		2019	/* scroll form left */
# define	ftRTSCRLL		2020	/* scroll form right */
# define	ftAUTODUP		2021	/* auto-duplicate field */
# define	ftPRSCR			2022	/* print screen */
# define	ftSKPFLD		2023	/* skip fields */

# define	ftTPSCRLL		2024	/* top-scroll (FT3270 only)  */
# define	ftBTSCRLL		2025	/* btm-scroll (FT3270 only)  */
# define	ftCOMMAND		2026	/* issue cmd  (FT3270 only)  */
# define	ftDIAG			2027	/* enter diag (FT3270 only)  */

# define	ftSHELL			2028	/* call command interpreter */
# define	ftNXITEM		2029	/* next item command. */

# define	ftMUOFFSET		3000	/* offset for menu mapping */

# define	ftMENU1			3000	/* mapped to menu position 1 */
# define	ftMENU2			3001	/* mapped to menu position 2 */
# define	ftMENU3			3002	/* mapped to menu position 3 */
# define	ftMENU4			3003	/* mapped to menu position 4 */
# define	ftMENU5			3004	/* mapped to menu position 5 */
# define	ftMENU6			3005	/* mapped to menu position 6 */
# define	ftMENU7			3006	/* mapped to menu position 7 */
# define	ftMENU8			3007	/* mapped to menu position 8 */
# define	ftMENU9			3008	/* mapped to menu position 9 */
# define	ftMENU10		3009	/* mapped to menu position 10 */
# define	ftMENU11		3010	/* mapped to menu position 11 */
# define	ftMENU12		3011	/* mapped to menu position 12 */
# define	ftMENU13		3012	/* mapped to menu position 13 */
# define	ftMENU14		3013	/* mapped to menu position 14 */
# define	ftMENU15		3014	/* mapped to menu position 15 */
# define	ftMENU16		3015	/* mapped to menu position 16 */
# define	ftMENU17		3016	/* mapped to menu position 17 */
# define	ftMENU18		3017	/* mapped to menu position 18 */
# define	ftMENU19		3018	/* mapped to menu position 19 */
# define	ftMENU20		3019	/* mapped to menu position 20 */
# define	ftMENU21		3020	/* mapped to menu position 21 */
# define	ftMENU22		3021	/* mapped to menu position 22 */
# define	ftMENU23		3022	/* mapped to menu position 23 */
# define	ftMENU24		3023	/* mapped to menu position 24 */
# define	ftMENU25		3024	/* mapped to menu position 25 */


# define	ftFRSOFFSET		4000	/* offset for FRSKEY mapping */

# define	ftFRS1			4000	/* mapped to FRSKEY 1 */
# define	ftFRS2			4001	/* mapped to FRSKEY 2 */
# define	ftFRS3			4002	/* mapped to FRSKEY 3 */
# define	ftFRS4			4003	/* mapped to FRSKEY 4 */
# define	ftFRS5			4004	/* mapped to FRSKEY 5 */
# define	ftFRS6			4005	/* mapped to FRSKEY 6 */
# define	ftFRS7			4006	/* mapped to FRSKEY 7 */
# define	ftFRS8			4007	/* mapped to FRSKEY 8 */
# define	ftFRS9			4008	/* mapped to FRSKEY 9 */
# define	ftFRS10			4009	/* mapped to FRSKEY 10 */
# define	ftFRS11			4010	/* mapped to FRSKEY 11 */
# define	ftFRS12			4011	/* mapped to FRSKEY 12 */
# define	ftFRS13			4012	/* mapped to FRSKEY 13 */
# define	ftFRS14			4013	/* mapped to FRSKEY 14 */
# define	ftFRS15			4014	/* mapped to FRSKEY 15 */
# define	ftFRS16			4015	/* mapped to FRSKEY 16 */
# define	ftFRS17			4016	/* mapped to FRSKEY 17 */
# define	ftFRS18			4017	/* mapped to FRSKEY 18 */
# define	ftFRS19			4018	/* mapped to FRSKEY 19 */
# define	ftFRS20			4019	/* mapped to FRSKEY 20 */
# define	ftFRS21			4020	/* mapped to FRSKEY 21 */
# define	ftFRS22			4021	/* mapped to FRSKEY 22 */
# define	ftFRS23			4022	/* mapped to FRSKEY 23 */
# define	ftFRS24			4023	/* mapped to FRSKEY 24 */
# define	ftFRS25			4024	/* mapped to FRSKEY 25 */
# define	ftFRS26			4025	/* mapped to FRSKEY 26 */
# define	ftFRS27			4026	/* mapped to FRSKEY 27 */
# define	ftFRS28			4027	/* mapped to FRSKEY 28 */
# define	ftFRS29			4028	/* mapped to FRSKEY 29 */
# define	ftFRS30			4029	/* mapped to FRSKEY 30 */
# define	ftFRS31			4030	/* mapped to FRSKEY 31 */
# define	ftFRS32			4031	/* mapped to FRSKEY 32 */
# define	ftFRS33			4032	/* mapped to FRSKEY 33 */
# define	ftFRS34			4033	/* mapped to FRSKEY 34 */
# define	ftFRS35			4034	/* mapped to FRSKEY 35 */
# define	ftFRS36			4035	/* mapped to FRSKEY 36 */
# define	ftFRS37			4036	/* mapped to FRSKEY 37 */
# define	ftFRS38			4037	/* mapped to FRSKEY 38 */
# define	ftFRS39			4038	/* mapped to FRSKEY 39 */
# define	ftFRS40			4039	/* mapped to FRSKEY 40 */
# define	ftFRS41			4040	/* mapped to FRSKEY 41 */
# define	ftFRS42			4041	/* mapped to FRSKEY 42 */
# define	ftFRS43			4042	/* mapped to FRSKEY 43 */
# define	ftFRS44			4043	/* mapped to FRSKEY 44 */
# define	ftFRS45			4044	/* mapped to FRSKEY 45 */
# define	ftFRS46			4045	/* mapped to FRSKEY 46 */
# define	ftFRS47			4046	/* mapped to FRSKEY 47 */
# define	ftFRS48			4047	/* mapped to FRSKEY 48 */
# define	ftFRS49			4048	/* mapped to FRSKEY 49 */
# define	ftFRS50			4049	/* mapped to FRSKEY 50 */
# define	ftFRS51			4050	/* mapped to FRSKEY 51 */
# define	ftFRS52			4051	/* mapped to FRSKEY 52 */
# define	ftFRS53			4052	/* mapped to FRSKEY 53 */
# define	ftFRS54			4053	/* mapped to FRSKEY 54 */
# define	ftFRS55			4054	/* mapped to FRSKEY 55 */
# define	ftFRS56			4055	/* mapped to FRSKEY 56 */
# define	ftFRS57			4056	/* mapped to FRSKEY 57 */
# define	ftFRS58			4057	/* mapped to FRSKEY 58 */
# define	ftFRS59			4058	/* mapped to FRSKEY 59 */
# define	ftFRS60			4059	/* mapped to FRSKEY 60 */
# define	ftFRS61			4060	/* mapped to FRSKEY 61 */
# define	ftFRS62			4061	/* mapped to FRSKEY 62 */
# define	ftFRS63			4062	/* mapped to FRSKEY 63 */
# define	ftFRS64			4063	/* mapped to FRSKEY 64 */
# define	ftFRS65			4064	/* mapped to FRSKEY 65 */
# define	ftFRS66			4065	/* mapped to FRSKEY 66 */
# define	ftFRS67			4066	/* mapped to FRSKEY 67 */
# define	ftFRS68			4067	/* mapped to FRSKEY 68 */


# define	ftMAXCMD		4067	/* last command defintion */

# define	ftUPARROW		7000	/* up arrow key */
# define	ftDNARROW		7001	/* down arrow key */
# define	ftRTARROW		7002	/* right arrow key */
# define	ftLTARROW		7003	/* left arrow key */
