/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:    abfosl.h -	ABF Run-Time System OSL Interface Definitions File.
**
** Description:
**	Contains the declarations of the objects defined by the ABF run-time
**	system to support OSL features.
**
**	IMPORTANT NOTE:
**		The structures declared here are also declared in
**		oslhdr.h which is in the ingres/files directory.
**		Any changes to structures here must be reflected
**		in that file.  Also, some of these structures
**		are built into users' ABF applications, so changes to
**		them have to account for compatibility with USERS'
**		applications.
** 		DO NOT change the structures or constants declared
**		here without checking with OSL development.
**
** History:
**	Revision 6.4/03
**	09/20/92 (emerson)
**		Defined new constant QR_VERSION to take the place of
**		QG_VERSION when setting/testing the qr_version field
**		of the QRY structure.  Also added defines to allow
**		using the high-order 24 bits of qr_version as flags.
**		Note that none of these need be defined in oslhdr.h.
**		(A name like "qr_flags" would now make more sense than
**		"qr_version", but I didn't want to mess with the backward
**		compatibility issues in oslhdr.h).
**
**	Revision 6.4
**	03/23/91 (emerson)
**		Changed type of qr_argv in QRY structure from (QRY_ARGV (*)[])
**		to (QRY_ARGV *), which matches the declaration in oslhdr.h,
**		and appears to be the real type: Code in abfrt and interp
**		that referenced qr_argv was casting it to (QRY_ARGV *);
**		this will no longer be necessary.
**
**	Revision 6.3/03/01
**	02/17/91 (emerson)
**		Defined new constant PR_VERSION to take the place of
**		FD_VERSION when setting/testing the pr_version field
**		of the ABRTSPRM structure (for bug 35946 - see fdesc.h).
**		Note that PR_VERSION need not be defined in oslhdr.h.
**
**	Revision 6.0  87/07  arthur
**	Added new query structure for 6.0.
**
**	Revision 5.1  86/08  arthur
**	Changed some nat's back to i4's.  As noted above, this
**		file MUST remain consistent with oslhdr.h.
**		Note also that nat's and i4's ARE different on the PC.
**		8/14/86 (agh)
**
**	Revision 3.0  84/02/13  joe
**	Initial revision.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/*
** PARAMETERS
*/
typedef struct
{
	char	*tf_name;
	char	*tf_column;
	i4	*tf_expr;
	i4	*tf_value;
} ABRTSTFLD;

/*}
** Name: QRYV0		-	Pre-6.0 QRY structure
**
** Description:
**	This is the QRY structure used by the ABF runtime system and
**	QG, the query generation module, up through v5.0.  A definition
**	of this structure is preserved because users' code from 5.0
**	and before builds these structures and passes them to the ABF RTS.
*/
typedef struct qryv0type
{
	char		**qr_qlist;	/* names of fields in the form */
	char		*qr_putparam;	/* target list for putform */
	char		**qr_argv;	/* argument vector for this param */
	char		*qr_wparam;	/* target list for where clause */
	char		**qr_wargv;	/* arg vector for where clause */
	char		*qr_form;	/* form the query is attached to */
	char		*qr_table;	/* table field (if any) the query
					** is attached to */
	struct qryv0type *qr_child;	/* child of this QRY */
	QDV0		*qr_qdesc;	/* QDV0 (pre-6.0 QDESC) for this QRY */
} QRYV0;

/*}
** Name: QRY		-	6.0 QRY structure
**
** Description:
**	This is the QRY structure used by the ABF runtime system and
**	QG, the query generation module, from v6.0 on.
**	Users' code builds these structures and passes them to the ABF RTS.
**	The first word of this structure must be zero:  this is what
**	distinguishes it from a QRYV0..
*/
typedef struct qrytype
{
	i4		qr_zero;	/* must always be 0 */
	i4		qr_version;	/* version number and flags: */

# define	QR_VERSION_MASK	0xff	/* version number bits */
# define	QR_VERSION	3	/* current version number */
# define	QR_0_ROWS_OK	0x100	/* input to IIARgn1RtsgenV1:
					** return TRUE if no rows found
					** and no more serious error occurred */
# define	QR_ROWS_FOUND	0x200	/* set by IIARgn1RtsgenV1:
					** rows were found */

	QRY_ARGV	*qr_argv;	/* argument vector for this query */
	char		*qr_putparam;	/* string of 'a' for displayed ADT,
					** 'd' for hidden field */
	char		*qr_form;	/* form the query is attached to */
	char		*qr_table;	/* table field (if any) the query
					** is attached to */
	struct qrytype	*qr_child;	/* child of this QRY */
	QDESC		*qr_qdesc;	/* QDESC for this QRY */
} QRY;


typedef struct
{
	char		*pr_tlist;
	char		**pr_argv;
	ABRTSTFLD	*pr_tfld;
	QRY		*pr_qry;
} ABRTSOPRM;

typedef struct
{
	i4	abpvtype;	/* Type of the argument */
	char	*abpvvalue;	/* Pointer to value */
} OABRTSPV;

/*
** Note this must match the OL_PV struct.
*/
typedef struct
{
	i4	abpvtype;	/* Type of the argument */
	char	*abpvvalue;	/* Pointer to value */
	i4	abpvsize;
} ABRTSPV;

/*}
** Name:	ABRTSPRM
**
** Description:
**	The new parameter structure for 4.0.
**	The first word of this structure must be zero.  That
**	distinguishes it from a ABRTSOPRM.
*/
typedef struct
{
	i4		pr_zero;	/* Must always be 0 */
	i4		pr_version;
	ABRTSOPRM	*pr_oldprm;
	ABRTSPV		*pr_actuals;
	char		**pr_formals;
	i4		pr_argcnt;
	i4		pr_rettype;
	i4		*pr_retvalue;	/* A pointer to different types. */
	i4		pr_flags;	/* Uses constants ABPXXXX */
} ABRTSPRM;

# define	PR_VERSION	2

/*
** Constants for pr_flags.
*/

# define	ABPIGNORE	01 	/* Ignore extra arguments */

/*
** For predicate function.
*/
typedef struct
{
	char	*abprtable;	/* Name of any table field */
	char	*abprfld;	/* Name of the field */
	char	*abprexpr;	/* Expression to equate with */
	i4	abprtype;	/* Type of the field */
} ABRTSPR;

/*
** For runtime strings.
*/
typedef struct
{
	i2	abstrall;	/* Amount allocated */
	i2	abstrlen;	/* Current string length */
	char	*abstrptr;
} ABRTSSTR;


/*
** These constants used to be in abf/abfobjs.qc.
** They are now used by both ABF and OSL.
** These constants are used in system catalogs so they
** CANNOT change!!
**
** No longer true.  For 6.0, OC_ encodes are used in the catalogs.
** these constants are still around for OSL use, but bear no relation
** to any ABF catalog encodes.
*/

/*
** OBJECTS
** 	objects in the application
*/
# define	ABOFRM			2
# define	ABOPROC			3
# define	ABODEP			4
/*
** These aren't used in the catalogs
*/
# define	ABONOTYPE		0
# define	ABONAME			5
# define	ABOFILE			6
# define	ABOAPP			7
# define	ABOFDESC		8
# define	ABOQDESC		9
# define	ABOPARAM		10
# define	ABOREL			11
# define	ABOFORM			12


/*
** Name:    NEWPARAM_MACRO() -	Check ABF RTS Parameter Structure Version.
**
** Description:
**	This macro tests whether the input ABF RTS parameter structure is a new
**	version (greater than 0.)
**
**	NOTE:  A NULL parameter structure address does not imply a version of
**	zero.  The parameter structure reference must be tested for NULL before
**	using this macro.
*/
#define	NEWPARAM_MACRO(p)   ((p)->pr_zero == 0 && (p)->pr_version > 0)
/*
** Name:    RETEXPECT_MACRO() -	Check ABF RTS Parameter Structure for
**					Expected Return Value.
** Description:
**	This macro checks to see if return is expected.  This applies only to
**	"new" parameter structures and should only be called for them.
*/
#define	RETEXPECT_MACRO(p)   ((p)->pr_retvalue != NULL && (p)->pr_rettype != OL_NOTYPE)
