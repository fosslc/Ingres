/*
**	static	char	Sccsid[] = "@(#)vfqbf.h	1.2	2/6/85";
*/

/*
**  QBF.H  -  CONSTANTS, STRUCTURES, and GLOBAL VARIABLES FOR QBF
**	
**	This file contains the structures used by QBF to describe the 
**	table it is working with.  The variables are global to qbf and 
**	include command line flags.  The constants are used through 
**	out QBF for describing various functions.
**	
**	JEN - Oct 23, 1981
**	NML - 9/23/83 - added constant for deadlock detection mechanism.
**			added globals for -uusername flag.
**	03/13/87 (dkh) - Added support for ADTs and PC integration.
**	20-jul-88 (kenl)
**		Added field vfqfmand to structure VFQFATT.  This field
**		is needed to hold onto information concerning whether
**		a column is defined as NOT NULL NOT DEFAULT in its underlying
**		table.  This field will be copied to an appropriate
**		ATTRIBINFO structure in file getrel.qsc in preparation for
**		calling QBF's default form building routines.
**	06-oct-88 (sylviap)
**		Added TITAN changes.  DB_MAXTUP   -> DB_GW3_MAXTUP
**				      DB_MAX_COLS -> DB_GW2_MAX_COLS
**	12/23/89 (dkh) - Added support for owner.table.
**	10/14/90 (emerson) - Added support for logical keys (bug 8593).
**	09/24/92 (dkh) - Added support for owner.table.
*/


typedef	struct	vfqfatt		/* ATTRIBUTE DEFINITION STRUCTURE */
{
	char		vfqfname[FE_MAXNAME + 1];	/* attribute name */
	DB_DATA_VALUE	vfqfdbv;
	i2		vfqfflags;
	i2		vfqfmand;	/* This field will be set to 1 if
					** column in table is NOT NUL NOT
					** DEFAULT, otherwise 0 */
	i2		lkeyinfo;	/* Logical key information; bits set
					** as specified in <mqtypes.qsh>. */
} VFQFATT;

typedef	struct vfqftbl		/* TABLE DEFINITION STRUCTURE */
{
	char	vfqfrel[FE_MAXTABNAME + 1];	/* relation name */
	i4	vfqfldno;		/* number of fields in table */
	i4	vfqfflags;		/* other flags */
	struct	vfqfatt	vfqflds[DB_GW2_MAX_COLS + 1]; /* array of attributes */
} VFQFTBL;

# define	MAXRECL		2 * DB_GW3_MAXTUP  /* maximum record length */
# define	s_VIEW		040	/* test bit for a view in INGRES */

# define	VFQF_BAD_DTYPE	01	/* bad datatype found in table */
