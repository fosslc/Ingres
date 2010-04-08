/*
**	globs.h
*/

/*
** GLOBS.H
**	Global declaration header file for VIFRED.
**
**	Copyright (c) 2004 Ingres Corporation
**
**  History:
**	08/04/87 (dkh) - Deleted ref to vfqbfcat and valerr.
**	08/14/87 (dkh) - ER changes.
**	10/01/87 (dkh) - Changed externs to GLOBALREFs.
**	11/28/87 (dkh) - Added testing support for calling QBF.
**	06/07/88 (dkh) - Removed ref to frmwidth since it is no longer defined.
**	01-jun-89 (bruceb)
**		Added IIVFmtMiscTag tag id.
**	01-sep-89 (cmr)	 updated decl of Sections
**	14-dec-89 (bruceb)
**		Added IIVFrfRoleFlag, IIVFpwPasswd and IIVFgidGroupID.
**	12/23/89 (dkh) - Added support for owner.table.
**	19-apr-90 (bruceb)
**		Removed refs of upLimit, lowLimit.
**	02-may-90 (bruceb)
**		Removed IIVFrfRoleFlag.
**	06/09/90 (esd) - Add GLOBALREF for IIVFsfaSpaceForAttr.
**	21-jun-90 (cmr)
**		Added RbfAggSeq.
**	08/09/92 (dkh) - Added reference to IIVFhcross, IIVFvcross,
**			 IIVFru_disp and IIVFxh_disp for rulers support.
**	09/25/92 (dkh) - Added support for owner.table.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/*
** ARGUMENTS
*/
GLOBALREF char	*vfdname;		/* name of above file */
GLOBALREF char	*vfuname;		/* user name for -u */
GLOBALREF bool	vfuserflag;		/* is -u set */
GLOBALREF char	vfspec;			/* letter of -f, -t or -j specified */
GLOBALREF bool	vfnoload;		/* -e specified to not load names into
					** catalogs.
					*/
GLOBALREF bool	iivfRTIspecial;		/* -Q special RTI functionality */
GLOBALREF i4	vforigintype;		/* how form originated */
GLOBALREF char	vforgname[];		/* name of table/joindef created from */
GLOBALREF char	IIVFtoTblOwner[];	/* name of owner from owner.table */
GLOBALREF bool	IIVFiflg;		/* -I flag specified */
GLOBALREF bool	IIVFzflg;		/* -Z flag specified */
GLOBALREF char	*IIVFpwPasswd;		/* password for -P */
GLOBALREF char	*IIVFgidGroupID;	/* group id for -G */

/*
**  Global strings used by VIFRED.
*/
GLOBALREF	char	*IIVF_yes1;
GLOBALREF	char	*IIVF_no1;

/*
** vfRecover taken out since no one uses it. dkh 4-16-84
*/

GLOBALREF char	*Vfformname;		/* the name in a -f call */
GLOBALREF char	*Vfcompile;		/* the name for a -C call */
GLOBALREF char	*Vfcfsym;		/* the name for a -S call */

/*
** PIPES
*/
GLOBALREF char	*Vfxpipe;		/* The pipes for equel */

/*
** WRITE FILES
** Used for copying the frame definition to the database.
**
** Code ifdef'd so linker will keep quiet when linking RBF.
** dkh 4-16-84.
*/

/*
**  Vars do not appear to be used by anyone anymore. 11-20-84 (dkh)
*/


GLOBALREF char	*vfrname;		/* name of recovery file */
GLOBALREF i4	globy, globx, numTrim, numField;
GLOBALREF FRAME	*frame;
GLOBALREF VFNODE	**line;
GLOBALREF char	dbname[];
GLOBALREF bool	noChanges;
GLOBALREF i4	endFrame;
GLOBALREF i4	endxFrm;
GLOBALREF bool	Vfeqlcall;		/* whether called from (linked to) VQ */
GLOBALREF i4	IIVFsfaSpaceForAttr;

/*
** RBF globals
*/
GLOBALREF CS	Other;
GLOBALREF CS	*Cs_top;
GLOBALREF Sec_List Sections;
GLOBALREF i2	Cs_length;
GLOBALREF i2	Agg_count;
GLOBALREF i2	RbfAggSeq;
GLOBALREF FRAME	*Rbf_frame;
GLOBALREF bool	Rbfchanges;

GLOBALREF bool	RBF;

/*
** Tags for dynamic memory allocation (12/21/86 - DRH)
*/

GLOBALREF	i2	vffrmtag;	/*  frame storage tag  */
GLOBALREF	i2	vflinetag;	/*  line table storage tag */
GLOBALREF	bool	vfendfrmtag;	/*  if true, need to end frame storage tag */
GLOBALREF	i2	IIVFmtMiscTag;	/*  misc memory storage tag  */

GLOBALREF	POS	*IIVFhcross;	/* POS pointer to horizontal x-hair */
GLOBALREF	POS	*IIVFvcross;	/* POS pointer to vertical x-hair */

GLOBALREF	bool	IIVFxh_disp;	/* display cross hairs? */
GLOBALREF	bool	IIVFru_disp;	/* display ruler? */

# define	COMLEAD		033
# define	STANDOUT	'$'
# define	WINCHAR		'_'
# define	NONSEQ		(-1)
# define	SEQ		0
# ifndef min
# define	min(a, b)	((a) < (b) ? (a) : (b))
# define	max(a, b)	((a) > (b) ? (a) : (b))
# endif

/*
** symbols for the undo commands
** these are bit flags for easy grouping of commands
*/
# define	unNONE		0

/*
** object groups
*/
# define	unTRIM		   01
# define	unLINE		   02
# define	unTITLE		  010
# define	unDATA		  020
# define	unATTR		  040
# define	unBOX	      0100000 

/*
** command groups
*/
# define	unSEL		 0100
# define	unENT		 0200
# define	unEDT		 0400
# define	unDEL		01000
# define	unOPEN		02000
# define	unCOLUMN	04000
# define	unTABLE        010000
# define	unBMARGIN      020000
# define	unRMARGIN      040000

/*
** define some masks to make breaking things up easier
** since no constant folding is done make them explicit
*/
# define	unFIELD		 (unATTR|unDATA|unTITLE)

# define	slBOX		(unSEL|unBOX)
# define	slTRIM		(unSEL|unTRIM)
# define	slFIELD		(unSEL|unFIELD)
# define	slTABLE		(unSEL|unTABLE)
# define	slTITLE		(unSEL|unTITLE)
# define	slDATA		(unSEL|unDATA)
# define	slCOLUMN	(unSEL|unCOLUMN)
# define	edBOX		(unEDT|unBOX)
# define	edTRIM		(unEDT|unTRIM)
# define	edTITLE		(unEDT|unTITLE)
# define	edDATA		(unEDT|unDATA)
# define	edATTR		(unEDT|unATTR)
# define	edTABLE		(unEDT|unTABLE)
# define	enBOX		(unENT|unBOX)
# define	enTRIM		(unENT|unTRIM)
# define	enFIELD		(unENT|unFIELD)
# define	enTABLE		(unENT|unTABLE)
# define	enCOLUMN	(unENT|unCOLUMN)
# define	dlBOX		(unDEL|unBOX)
# define	dlTRIM		(unDEL|unTRIM)
# define	dlFIELD		(unDEL|unFIELD)
# define	dlTABLE		(unDEL|unTABLE)
# define	dlCOLUMN	(unDEL|unCOLUMN)
# define	dlLINE		(unDEL|unLINE)
# define	opLINE		(unOPEN|unLINE)
# define	slBMARGIN	(unSEL|unBMARGIN)
# define	slRMARGIN	(unSEL|unRMARGIN)


/*
**  Number of tries to write out form
**  Fix for bug 1835. (dkh)
*/

# define	MAXDLOCK_TRIES		15

/*
**  A very large buffer size.
*/

# define	LRGBUFSZ		4096

/*
**	Values for vforigintype
*/

# define	OT_EDIT		0
# define	OT_BLANK	1
# define	OT_TABLE	2
# define	OT_JOINDEF	3

/*
**  Constants for determing what we are
**  prompting for in IIVFgtmGetTblName()
*/
# define	TF_TBL_PROMPT	0
# define	QN_TBL_PROMPT	1
# define	QN_JDF_PROMPT	2
