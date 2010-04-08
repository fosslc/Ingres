/*
**	Copyright (c) 2004 Ingres Corporation
*/

/**
** Name:	tuconst.h -	constants used globally in tables utility.
**
** Description:
**	This contains #defines and the following structures used 
**	in the Tables Utility:
**
**	TUSYMTBL	structure for mini symbol table.
**	TU_KEYSTRUCT	used in printing the key sequence.
**
** History:
**	24-apr-1987	(daver)	First Commented.
**	01-jul-1987	(daver) Changed colname in TU_KEYSTRUCT to use 
**				FE_MAXNAME+1; THIS ASSUMES THAT FE.H HAS 
**				ALREADY BEEN INCLUDED!!!!!
**	07-jan-1990	(pete)  Added TU_... Logical Key return code symbols.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	24-Aug-2009 (kschendel) 121804
**	    Add some prototypes for tblutil.
**/

/* # define's */

# define	TUBUFSIZE	8192		/* For create statement */

# define	NM_FOUND		0
# define	NM_NOT_FOUND		1

# define	HASH_TBL_SIZE		17

# define	H_BLOCK		0200
# define	H_COLUMN	0400
# define	H_WRAP		01000
# define	H_DEFAULT	02000

/* states for rows in data set */
# define	stNEWEMPT	-1	/* space appended at runtime	*/
# define	stUNDEF		0	/* space just allocated		*/
# define	stNEW		1	/* appended and filled by user	*/
# define	stUNCHANGED	2	/* added through Equel program	*/
# define	stCHANGE	3	/* changed by Equel or user	*/
# define	stDELETE	4	/* added by Equel,later deleted	*/

# define	starterr(cmd)

/* info about size of frame titles */
# define	TU_TITLE_SIZE	80

/* information about a datatype: tells if it's a logical key or not
** and if "with system_maintained" or "not system_maintained" was
** specified.
*/
# define        TU_NOT_LOGKEY   11      /* not a logical key */
# define        TU_LOGKEY_WITH  12      /* with system_maintained specified */
# define        TU_LOGKEY_NOT   13      /* not system_maintained specified */
# define        TU_LOGKEY       14      /* only "table_key" or "object_key"
                                        ** was specified. No "with" or "not"
                                        ** system_maintained clause specified.
                                        */
/* extern's */
/* static's */

struct TUSYMTBL
{
	i4	length;
	char	name[FE_MAXNAME+1];
	struct	TUSYMTBL	*next;
};

/*}
** Name:	TU_KEYSTRUCT	-  structure for holding key info.
**
** Description:
**	Used in prscreate.qc and helptbl.qc. In prscreate.qc, it is used
**	to store the column names of the keyed columns so we can later
**	modify the newly created table to those column names. In helptbl.qc,
**	it is used to hold the key info which comes from the IIKEY_COLUMNS
**	catalog rather than the FEatt_info structure.
**
** History:
**	01-jul-87 (daver)	First Documented.
*/
struct TU_KEYSTRUCT
{
	i4	keynum;
	char	name[FE_MAXNAME+1];
	struct	TU_KEYSTRUCT	*next;
};

/*}
** Name:	LISTVALS - Prompt information for ListPicks
**
** Description:
**	Used in creattbl.qsc to display a  ListPick of Unique Key or
**	Defaultability indicators.
**
** History:
**	29-dec-1989 (pete)	Initial Version.
**	18-oct-1992 (mgw)	renamed from UKEY to LISTVALS for generalization
*/
typedef struct {
        ER_MSGID        abbrev;         /* abbreviation */
        ER_MSGID        descrip;        /* description of selection */
} LISTVALS;

/*}
** Name:	DTYPE - information about a datatype
**
** Description:
**	Used in creattbl.qsc to display a  ListPick of data type names.
**
** History:
**	29-dec-1989 (pete)	Initial Version.
*/
typedef struct {
        ER_MSGID        type;           /* Name of data type */
        bool            openSql;        /* TRUE if OPEN SQL data type;
                                        ** FALSE if Ingres data type only.
                                        */
} DTYPE;

/* Function prototypes */

FUNC_EXTERN bool IITUedEditDefault(i4 *);
