/*
**	Copyright (c) 1993, 2004 Ingres Corporation
*/

/*
** Name: impexp.h
**
** Description: header file for iiimport, iiexport executables
**
** Defines:
**	IEPARAMS array element struture
**	Some Constants
**
** History:
**	29-jun-1993 (daver)
**	First Written
**	04-jan-1994 (daver)
**		Added some extra bit masks and constants.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/


/*}
** Name:        IEPARAMS         - List of parameters for iiimport/iiexport
**
** Description:
**      This structure defines a linked list element for each of the 
**	iiexport parameter objects. As used as a member in an array, 
**	the array index denotes which type of parameter it is: 
**	frame, procedure, record type, constant, or global.
**
** History:
**      29-jun-1993 (daver)
**      First Written.
*/

typedef struct _ieparams
{
    struct	_ieparams	*next;
    char        name[FE_MAXNAME+1];
    i4		id;
} IEPARAMS;


/*}
** Name:        IEOBJREC         - Structure to hold an OBJECT Record
**
** Description:
**	This structure is used to hold the five elements in an OBJECT
**	Record. It is filled, and then passed to the appropriate
**	processing routine. It also includes the object's id, just for fun.
**
** History:
**      15-jul-1993 (daver)
**      First Written.
*/
typedef struct _ieobjrec
{
    i4		class;		/* should this be OOID (i4)? */
    i4		level;
    char	name[FE_MAXNAME+1];
    char	short_remark[OOSHORTREMSIZE+1];
    char	long_remark[OOLONGREMSIZE+1];
    i4		id;
    bool	update;
} IEOBJREC;


/*
** Maximum size of file line -- taken from cu.qsh
*/
# define	IE_LINEMAX	4800

/*
** iiexport constants for parameter list array; each constant represents
** and array index, whose element is the head of a linked list of object
** names (e.g, frame=foo,bar,bleh would have a linked list of 3 buckets at
** array index EXP_FRAME)
*/
# define        EXP_OBJECTS     5       /* number of elements in the array */
# define        EXP_FRAME       0
# define        EXP_PROC        1
# define        EXP_GLOB        2
# define        EXP_CONST       3
# define        EXP_REC         4

/*
** iiiexport flags for parameters
*/
# define        ALLFRAMES       0x01
# define        ALLPROCS        0x02
# define        ALLRECS         0x04
# define        ALLGLOBS        0x08
# define        ALLCONSTS       0x10

/* 
** iiimport flags for parameters
*/
# define	CHECKFLAG	0x01
# define	COPYSRCFLAG	0x02
# define	DROPFILEFLAG	0x04
# define	LISTFILEFLAG	0x08

# define	CHECKOK		10
# define	CHECKFAIL	5


/* function declarations ... */
FUNC_EXTERN	VOID	IIIEeum_ExportUsageMsg();
FUNC_EXTERN	VOID	IIIEium_ImportUsageMsg();
FUNC_EXTERN	VOID	IIIEesp_ExtractStringParam();
FUNC_EXTERN	VOID	IIIEepv_ExtractParamValue();
FUNC_EXTERN	VOID	IIIEfoe_FileOpenError();

FUNC_EXTERN	bool	IIIEfco_FoundCursorObject();

FUNC_EXTERN	STATUS	IIIEsor_SkipObjectRec();
FUNC_EXTERN	STATUS	IIIEwac_WriteIIAbfClasses();
FUNC_EXTERN	STATUS	IIIEwao_WriteIIAbfObjects();
FUNC_EXTERN	STATUS	IIIEwf_WriteIIForms();
FUNC_EXTERN	STATUS	IIIEwjd_WriteIIJoinDefs();
FUNC_EXTERN	STATUS	IIIEwqn_WriteIIQbfNames();
FUNC_EXTERN	STATUS	IIIEwr_WriteIIReports();
FUNC_EXTERN	STATUS	IIIEdcr_DoComponentRec();
FUNC_EXTERN	STATUS	IIIEbe_Bad_Exit();


FUNC_EXTERN	char	*cu_gettok();

extern	bool	IEDebug;
