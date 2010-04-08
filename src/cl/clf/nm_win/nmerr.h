/*
** nmerr.h -- built from NMerr.def
*/

/* static char	*Sccsid = "@(#)nmerr.h	3.1  9/30/83"; */


# define	 NM_PWDOPN	2600	/*  NMpathIng: Unable to open passwd file. */
# define	 NM_PWDFMT	2605	/*  NMpathIng: Bad passwd file format. */
# define	 NM_INGUSR	2610	/*  NMpathIng: There is no INGRES user in the passwd file. */
# define	 NM_INGPTH	2615	/*  NM[sg]tIngAt: No path returned for INGRES user. */
# define	 NM_STOPN	2620	/*  NM[sg]tIngAt: Unable to open symbol table. */
# define	 NM_STPLC	2625	/*  NMstIngAt: Unable to position to record in symbol table. */
# define	 NM_STREP	2630	/*  NMstIngAt: Unable to replace record in symbol table. */
# define	 NM_STAPP	2635	/*  NMstIngAt: Unable to append symbol to symbol table. */
# define	NM_LOC		2640	/*  NM_loc: first argument must be 'f' or 't' */
# define	NM_BAD_PWD	2645	/*  NMpathIng: Bad home directory for INGRES superuser */
# define	NM_NO_II_SYSTEM	2650	/*  NMgtAt: II_SYSTEM must be defined */
# define	NM_PRTLNG	2655	/*  NMsetpart: Part name too long */
# define	NM_PRTFMT	2660	/*  NMsetpart: Illegal part name format */
# define	NM_PRTNUL	2665	/*  NMsetpart: Part name is a null string */
# define	NM_PRTALRDY	2670	/*  NMsetpart: Part name already set */
