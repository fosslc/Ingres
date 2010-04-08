/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:    qg.h -	Query Generator Module Interface Definitions File.
**
** Description:
**	This file defines the interface for the query generator module.  It
**	contains the definitions of the query descriptor structure, input
**	values, and output values that are the external interface of the
**	module.  Both QBF and the ABF/OSL RTS use this module.
**
**	Note:  This file is included directly as part of "oslhdr.h".
**
** History:
**	Please put history at the bottom of this section.
**
**	Revision 6.4/03
**	5/92 (Mike S)
**	Add support for true singleton SELECTs.
**	18-sep-92 (kirke)
**	    Removed bogus trigraph from comment to silence compiler.
**      09/20/92 (emerson)
**		Changed comments to reflect the fact that QG_VERSION
**		is no longer used for qr_version in ABF's QRY structure.
**
**	Revision 6.4/02
**	3/92 (Mike S)
**	Add support for EXECUTE IMMEDIATE INTO selects.
**	4/92 (Mike S)
**	Change qs_type back to u_i1 for compatibility with existing
**	ABF-produced .c files.
**
**      Revision 6.3/03/01
**      02/20/91 (emerson)
**		Bumped QG_VERSION from 2 to 3 for bug 35798;
**		see abrtnqry.qsc for details.
**
**	Revision 6.3/03/00  89/08  wong
**	Added QI_DESCRIBE, QI_PREVIOUS and QI_ISDONE.
**
**	Revision 6.3  90/01  wong
**	Version 2:  Query ID changed to be long.  JupBug #7899.
**
**	Revision 6.1  88/08  wong
**	Added QS_VALGNSV.
**
**	Revision 6.0  87/04  wong
**	Support for SQL and ADTs and also increase performance as version 1.
**
**	Revision 3.0  84/02  joe
**	Initial revision.
**
**	10/07/92 (dkh) - Removed QI_PREVIOUS and QI_ISDONE as no FE code
**			 references them.  Note that removal was prompted
**			 by alpha cross compiler which indicated that
**			 the values assigned to the constants were not
**			 legal octal values.  Any new constants that
**			 are added should avoid this problem.
*/

/*
** Query Generator Version Number.
**
** This is used only by ABF (to set and test qg_version in the QDESC structure,
** defined in this file).  See abrtnqry.qsc for details.
*/

#define		QG_VERSION	3
#define 	QG_V2   	2
#define 	QG_V1   	1

/*}
** Name:    QRY_SPEC -	Query Specification Entry.
**
** Description:
**	This structure defines an element of a query specification list.  A
**	query specification is a list of text, dynamic values, data values,
**	and query specification generators that together specify a query or
**	part of a query.
**
**	The query generate module sends a query specification to the DBMS
**	to define a query using the LIBQ interface.  Each element of the
**	query specification is a reference to one of the following:
**
**	    A text string that contains part of the text for a query, and is
**	    sent directly to the DBMS.  ("QS_TEXT".)
**
**	    A dynamic value that contains part of the text for a query as a
**	    string value, which was dynamically set, and is sent as a string
**	    value to the DBMS.  ("QS_QRYVAL" or "QS_QRYVAL|QS_BASE".)
**
**	    A data value that contains a data value for the query, and is sent
**	    directly to the DBMS (as a DB_DATA_VALUE.)  ("QS_VALUE" or
**	    "QS_VALUE|QS_BASE".)
**
**	    A query specification generator (QSGEN) that specifies a function
**	    and data from which a part of a query specification will be
**	    generated and sent to the DBMS.  ("QS_VALGEN" or "QS_VALGNSV".)
**	    (The generated query specifications will be saved and sent each
**	    time the query is subsequently run if the query is a child query
**	    and the specification type is QS_VALGEN.)
**
**	A query specification list is terminated by an element with type
**	"QS_END" (defined below.)
**
** History:
**	02/87 (jhw) -- Designed.
**	05/87 (jhw) -- Added QSGEN variant.
**	08/88 (jhw) -- Added QS_VALGNSV.
*/
typedef struct {
    PTR	    qs_var;	/* Query specification variant:   contains either
			**  {char *}		a query text string  or
			**  {DB_DATA_VALUE *}	a query value or data value  or
			**  {QSGEN *}		a query spec. generator or
			**  {u_i2 ; u_i1}	a query value or data value
			**			    reference index.
			**
			** A cast value is used rather than a union so that
			** a query specification can be created by static
			** initialization.  Also, for portability reasons the
			** query DB_DATA_VALUE reference structure is instead
			** two seperate members distinct from the cast value
			** (below.)
			*/
    u_i2    qs_index;		/* a DB_DATA_VALUE reference index */
    u_i1    qs_base;		/* a DB_DATA_VALUE reference base */

    u_i1    qs_type;	/* Query specification type */
#define		QS_END	    0x00
#define		QS_TEXT     0x01
#define		QS_QRYVAL   0x02
#define		QS_VALUE    0x04
#define		QS_VALGEN   0x08
#define		QS_MARK	    0x10	/* place marker, ERROR */
#define		QS_BASE	    0x20	/* base/offset reference */
#define		QS_VALGNSV  0x40
#define		QS_EITEXT   0x80	/* string from Execute Immediate */
} QRY_SPEC;

/*}
** Name:    QSGEN -	Query Specification Generator Element.
**
** Description:
**	Query specification structure referenced by the 'qs_var' field of a
**	QRYSPEC that is of type QS_VALGEN or QS_VALGNSV.  This structure
**	contains information needed to call a generator of QRYSPECs.
**
**	The field 'qsg_gen' points to a function that is a generator of
**	QRYSPECs.  When the generator function is called with the data
**	pointed to by 'qsg_value', it will generate QRYSPECs.  The data
**	referenced by 'qsg_value' is specific to the particular generator
**	function and its purpose is to give the generator function the
**	information needed to generate the query specifications.
**
**	N.B.: THIS FUNCTION CANNOT RETURN A QRY_SPEC OF TYPE QS_VALGEN
**	OR QS_VALGNSV!
**
**	The function pointed to by 'qsg_gen' should be a function that
**	is declared like this example generator function 'genfunc()'.
**
**	Inputs:
**		genstruct	{PTR}  This is the generator specific data that
**				was given in the 'qsg_value' field.  (Note that
**				the function must cast this internally to the
**				correct type.)
**
**		qgvalue		{PTR}  This is a reference to a value that must
**				be passed to the next routine.
**
**		qgfunc		{STATUS (*)()}  This is a function that must
**				be called once for each QRYSPEC the generator
**				function produces.  The function needs to be
**				called as:
**
**					(*qgfunc)(qgvalue, qryspec)
**
**				where 'qryspec' is the QRYSPEC being generated.
**				If '(*qgfunc)()' returns anything other than OK
**				(except for QG_NOSPEC,) the generation process
**				should be stopped and the value returned.
**
**	Returns:
**		{STATUS}	OK is everything went fine.
**				QG_NOSPEC   if no QRYSPEC was generated.
**				any other value means there was a problem.
**		
**	Declaration:
**		STATUS
**		genfunc ( genstruct, qgfunc, qgvalue )
**		PTR	genstruct
**		STATUS	(*qgfunc)();
**		PTR	qgvalue;
**
**	An example of how this might be used is the implementation of the 4GL
**	qualification function.  The qualification 
**
**		qualification (R.x = x, R.y = y)
**
**	would have a single QRYSPEC in the query specification that is of type
**	QS_VALGEN.  The QSGEN would point to a function in the ABF RunTime
**	System.  The 'qsg_value' would point to a data structure that contains:
**
**		the form name,
**		the list
**			R.x, x
**			R.y, y
**
**	With this data, the function in ABFRTS would look through the
**	form and generate QRYSPECs for the fields x and y if they contain
**	a query operator/value.
**
** History:
**	04/87 (joe) -- Designed.
*/
typedef struct
{
    PTR		qsg_value;	/* Generator specific data */
    STATUS	(*qsg_gen)();	/* The generator function */
} QSGEN;

/*}
** Name:    QRY_ARGV -	Query Retrieved Value Descriptor.
**
** Description:
**	This structure defines a descriptor for a value that may be retrieved
**	by a query.  It specifies the DB_DATA_VALUE for the value and the name
**	of the field or table field column for which the data value should be
**	retrieved.
**
**	A list of descriptors describes the column values that may be retrieved
**	by a query.  This list is terminated with a descriptor that has a NULL
**	reference to a DB_DATA_VALUE.
**
**	As of version 6.0, the name is unused.  The descriptors and columns
**	retrieved from the DBMS must be in one-to-one correspondence in the
**	order returned by the DBMS, or a LIBQ run-time error will be reported.
**
**	For some future release or version, this restriction on one-to-one
**	correspondence will be relaxed.  The name will be used to determine the
**	retrieved column that corresponds to a particular descriptor allowing
**	extraneous columns to be ignored.  Note also that (named) descriptors
**	that do not correspond to any of the retrieved columns will not have
**	their values changed.  This will mean that actual values retrieved will
**	be an intersection of the descriptors with the columns retrieved by the
**	DBMS for the query.
**
** History:
**	02/87 (jhw) -- Designed.
*/
typedef struct {
    /* Retrieved DB_DATA_VALUE variant:  contains either
    **  {DB_DATA_VALUE *}	a reference to the value  or
    **  {u_i2 ; u_i1}		an index reference to the value.
    **
    ** For portability reasons the DB_DATA_VALUE reference structure is
    ** instead two seperate members distinct from the pointer reference.
    */
	DB_DATA_VALUE	*qv_dbval;	/* data value for retrieved value */
	u_i2		qv_index;	/* DB_DATA_VALUE reference index */
	u_i1		qv_base;	/* DB_DATA_VALUE reference base */

    char	    *qv_name;   /* field/table field column name for value */
} QRY_ARGV;

/*}
** Name:    QDESC -		Query Generator Query Description.
**
** Description:
**	This structure contains information that describes a query.  It
**	specifies for which DML the query was stated and the mode in which
**	it is to be executed, and contains a list of descriptors for the values
**	retrieved and query specifications for the various parts of the query,
**	which can be NULL, with an optional base if the DB data values are to
**	be referenced by offset instead of directly.  (The query could have
**	been a single query specification but was not for possible debugging
**	convienence.)  It also contains a reference to a child query if this
**	query describes a master-detail query.  Finally, it contains a reference
**	to query generator internal structures used when the query is active.
**
**	Queries can be run in one of three modes:  normal, cursor or mapped.
**	Normal mode is as a retrieve/select loop.  Cursor mode is as a cursor.
**	And, mapped mode retrieves the data into a file out of which tuples
**	are retrieved.  Additionally, any of these modes can have the query
**	be a repeated query.
**
**	Note that the mode of any detail query is independent of the mode
**	of the parent.  Both must be specified, but may be different.
**
** History:
**	?/84 (joe) -- Version 0 (see "generate/qgv0.h".)
**
**	02/86 (jhw) -- Version 1.
**
**	01/89 (jhw) -- Changed query ID 'qg_num[]' to be long nat's.  Version 2.
**	5-aug-92 (blaise)
**		Changed trigraph sequence which the ansi C compiler didn't
**		like.
*/
typedef struct _qdesc {
    char	*qg_new;		/* NULL if not v0 */
    char	*qg_name;		/* name for cursor or repeat */
    u_i1	qg_fill[2];		/* formerly ID */
    u_i1	qg_version;		/* version number */
    u_i1	qg_dml;			/* query DBMS language:
					**	    DB_QUEL or DB_SQL
					*/
    u_i1	qg_mode;		/* query mode */
#define		    QM_NORMAL 0x0	    /* query loop */
#define		    QM_REPEAT 0x1	    /* repeat query */
#define		    QM_CURSOR 0x2	    /* cursor query */
#define		    QM_MAP    0x4	    /* query mapped to file */
#define		    QM_EXECIMM 0x8	    /* EXECUTE IMMEDIATE INTO query */
#define		    QM_SINGLE 0x10	    /* True singleton select */

    u_i4	qg_num[2];		/* ID for cursor or repeat */

    QRY_ARGV    (*qg_argv)[];	/* query retrieved value descriptors */

    QRY_SPEC    (*qg_tlist)[];	/* query or 6.0 QBF target list */
    QRY_SPEC    (*qg_from)[];	/* 6.0 QBF SQL query from list */
    QRY_SPEC    (*qg_where)[];	/* 6.0 QBF query where clause */
    QRY_SPEC    (*qg_order)[];	/* 6.0 QBF order clause */
#define		qg_sort	qg_order

    struct _qdesc   *qg_child;	/* detail query */
    struct qgint    *qg_internal;

    i4		    qg_bcnt;	/* base count */
#define		    QG_BCNT 2
    DB_DATA_VALUE   (*qg_base[QG_BCNT])[];	/* base reference DB_DATA_VALUE
						**	indexing arrays
						*/
} QDESC;


/*
** Name:    QG_INPUT -	Query Generator Input Values.
**
** Description:
**	Constant input values representing the methods recognized by the
**	'IIQG_generate()' for a query object.
*/
#define		QI_BREAK	0101
#define		QI_GET		0102
#define		QI_START	0103
#define		QI_NMASTER	0104
#define		QI_USTART	0106	/* 6.0 QBF and pre 6.0 OSL only */
#define		QI_DESCRIBE	0107

/*
** Name:    QG_STATUS -	Query Generator Status Values.
*/
# define	QO_QDONE	0201
# define	QO_DDONE	0202
# define	QO_QIN		0203

/*
** Name:    QG_ERROR -	Query Generator Error Status Values.
**
** Description:
**	Except for NOSPEC these are internal program error statuses,
**	and as such, represent fatal bugs.
*/
#define	    QE_NOINTERNAL	E_QG0001_No_Internal
#define	    QE_NOSTART		E_QG0002_NoStart
#define	    QE_MEM		E_QG0003_NoMemory
#define	    QG_NOSPEC		E_QG0004_NoSpec
#define	    QG_NOTALLA		E_QG0005_BadVersion	/* DBDVs passed to V0 */
#define	    QG_BADSPEC		E_QG0006_BadSpec

#define	    QE_TFILE		E_QG0010_TempFile
#define	    QE_TFWRITE		E_QG0011_WriteFile

#include	<erqg.h>
