
/*
** Copyright (c) 2004 Ingres Corporation 
*/

/**CL_SPEC
** Name: SL.H - Definitions of types and constants.
**
** Description:
**      This file contains the definitions of types, functions and constants
**	needed to make calls on the compatibility library SL functions.  These
**	routines perform all the security label handling services for INGRES.
**
** Specification:
**
**      The SL component of the compatibility library is used for 
**      operations required to support security label handling 
**      for multi-level secure INGRES systems.
** 
** Intended Uses:
**
**      The SL routines are intended for use by a multi-level secure INGRES
**	system identify, compare and convert security labels associated 
**      with users (subjects).   These labels will be used to 
**      provide the Mandatory Access Controls (MAC) required by 
**      a multi-level secure DBMS.
**
** History: 
**	17-mar-1993 (robf)
**		Created from old 1.0 SL interface. 
**		Added function prototypes, new definitions.
**	15-feb-1994 (ajc)
**		Added hp-bls functionality.
**	 4-mar-94 (robf)
**              Added initial prototype for SLsetplabel()
**	31-may-94 (stephenb)
**		Add SLcolate prototype.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/
# ifndef SL_DEFINED
# define SL_DEFINED 1

/*
** Registry of known Label types. SL_LABEL_TYPE should be defined in
** slcl.h  to one of these values.
*/
# define SL_LABEL_TY_NONE	0	/* No labels */
# define SL_LABEL_TY_TEST	1 	/* "Test" (internal Ingres) labels */
# define SL_LABEL_TY_SUN_CMW	2	/* SUN-CMW Labels */
# define SL_LABEL_TY_HP_BLS	3	/* HP-BLS (secureware vers XX) Labels */

# include <slcl.h>

/*
**  Forward and/or External typedef/struct references.
*/

typedef struct IISL_LABEL SL_LABEL;
typedef struct IISL_USER SL_USER;
typedef struct IISL_EXTERNAL SL_EXTERNAL;

/*
**  Forward and/or External function references.
*/
FUNC_EXTERN STATUS SLcollate(SL_LABEL *label1, SL_LABEL *label2);

FUNC_EXTERN STATUS  SLproclabel(SL_USER*user,
			SL_LABEL *label,
			CL_ERR_DESC *err_code);      

FUNC_EXTERN STATUS  SLsetplabel(SL_LABEL *label,
			CL_ERR_DESC *err_code);      

FUNC_EXTERN STATUS  SLcompare(SL_LABEL *label1, SL_LABEL *label2);	
FUNC_EXTERN STATUS  SLdominates(SL_LABEL *label1, SL_LABEL *label2);

FUNC_EXTERN STATUS  SLinternal(SL_EXTERNAL *elabel, 
				SL_LABEL*label, 
				CL_ERR_DESC *err_code);

FUNC_EXTERN STATUS  SLexternal(SL_LABEL *label, 
				SL_EXTERNAL *elabel,
				CL_ERR_DESC *err_code);

FUNC_EXTERN STATUS  SLextlen(SL_LABEL *label, i4 *outlen);

FUNC_EXTERN STATUS  SLempty (SL_LABEL *label);

FUNC_EXTERN STATUS  SLisvalid(SL_LABEL *label);

FUNC_EXTERN STATUS  SLlub( SL_LABEL *label1,
			   SL_LABEL *label2,
			   SL_LABEL *result );

FUNC_EXTERN STATUS  SLhaslabels ();


/* 
** External Security Label defintion. 
*/
struct IISL_EXTERNAL
{
    i4			l_emax;		/* Maximum size of this elabel */
    i4                  l_elabel;	/* Actual length of data used */
    char                *elabel;	/* Pointer to data */

};
/* 
** Security user definition
*/
struct IISL_USER
{
    char                name[GL_MAXNAME];

};
/* 
** SL common constants
*/
# define	SL_LABEL_SIZE		sizeof(SL_LABEL)

# define	SL_CODE_OFFSET 		(E_CL_MASK+E_SL_MASK)  

# define	SL_EQUAL		(1+SL_CODE_OFFSET)
# define	SL_DOMINATES		(2+SL_CODE_OFFSET)
# define	SL_NOT_DOMINATES	(3+SL_CODE_OFFSET)
# define	SL_DISJOINT		(4+SL_CODE_OFFSET)
# define	SL_GREATER		(5+SL_CODE_OFFSET)
# define	SL_LESS			(6+SL_CODE_OFFSET)

# define	SL_LABEL_ERROR		(100+SL_CODE_OFFSET)	/* Invalid label format */
# define	SL_CNVT_ERROR		(101+SL_CODE_OFFSET)	/* Conversion error */
# define	SL_USER_ERROR		(102+SL_CODE_OFFSET)	/* User caller error */
# define	SL_BAD_PARAM		(103+SL_CODE_OFFSET)	/* Invalid input */
# define	SL_NOT_SUPPORT		(104+SL_CODE_OFFSET)	/* No labels */

# define	SL_EMPTY		(200+SL_CODE_OFFSET)	/* Label is empty */
# define	SL_LABEL_TRUNC		(201+SL_CODE_OFFSET)	/* Label truncated */
# define	SL_USER_TRUNC		(202+SL_CODE_OFFSET)	/* User truncated */
#endif
