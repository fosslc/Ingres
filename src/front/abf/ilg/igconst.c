/*
**	Copyright (c) 1986, 2004 Ingres Corporation
*/

#include	<compat.h>
#include	<ex.h>
#include	<st.h>
#include	<er.h>
#include 	<si.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#ifndef FEalloc
#define FEalloc(size, status) FEreqmem(0, (size), FALSE, (status))
#endif
#include	<adf.h>
#include	<iltypes.h>
#include	<ilops.h>

/**
** Name:    igconst.c -		OSL Interpreted Frame Object Generator
**						Constants Module.
** Description:
**	Contains routines that allocate and manage constants while generating
**	an interpreted frame object.  Defines:
**
**	IGsetConst()	set and return offset for constant.
**
**	(ILG internal)
**	IGgenConsts()	output interpreted frame object constants.
**
** History:
**	Revision 5.1  86/09/23  11:58:51  wong
**	Initial revision.
**
**	Revision 6.0  87/05  wong
**	Added support for SQL variable-length character strings.
**
**	Revision 6.4
**	03/13/91 (emerson)
**		Modify IGsetConst to get the decimal point character
**		from the logical symbol II_4GL_DECIMAL.
**
**	Revision 6.5
**	18-jun-92 (davel)
**		added support for decimal datatype.
**      06-dec-93 (smc)
**		Bug #58882
**              Commented lint pointer truncation warning.
**	25-may-95 (emmag)
**		Conflict with Microsoft's INT.
**	16-jun-95 (emmag)
**		Conflict with Microsoft VC's CONST.
**      15-nov-95 (toumi01; from 1.1 axp_osf port)
**              added kchin's change for axp_osf
**              03/12/94 (kchin)
**              Pointer address should be cast to long on axp_osf before
**              being used for any sort of computation such as % in
**              IGsetConst().  Previously it was cast to i4  (int)
**              which return a garbage value that causes a segmentation
**              fault on axp_osf when compiling a 4GL program with
**              oslsql.  Surprisingly the same code that has this cast
**              to i4  never fails on OSF/1 v1.x.
**      23-mar-1999 (hweho01)
**              Extended the change for axp_osf to ris_u64 (AIX 64-bit
**              platform).
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      08-Sep-2000 (hanje04)
**              Extended the change for axp_osf to axp_lnx (Alpha Linux).
**	06-Mar-2003 (hanje04)
**		Extend axp_osf changes to all 64bit platforms (LP64).
**	24-apr-2003 (somsa01)
**		Use i8 instead of long for LP64 case.
*/

GLOBALREF char  osDecimal;

#ifdef NT_GENERIC
#ifdef CONST
#undef CONST
#endif
#endif
/*
** Name:    CONST -		Constants Table Entry Structure Definition.
**
*/
typedef struct c_entry {
    char		*c_value;	/* constant value */
    i4			c_offset;	/* constant offset */
    struct c_entry	*c_list;	/* constants list */
    struct c_entry	*c_next;	/* constants hash list */
} CONST;

static FREELIST	*listp = NULL;	/* list pool */

#ifdef NT_GENERIC
#ifdef INT
#undef INT
#endif
#endif
#define CHAR	0
#define INT	1
#define FLOAT	2
#define VCHAR	3
#define DECIMAL	4

#define NUM_TYPES	5

static CONST	*consts_tab[NUM_TYPES];
static i4	n_consts[NUM_TYPES];

/*{
** Name:    IGsetConst() -	Set and Return Offset for Constant.
**
** Description:
**	This routine allocates and sets a constant for an interpreted frame
**	frame object, if it has not already been allocated and set.  It then
**	returns a reference to this constant.
**
** Input:
**	type	{DB_DT_ID}  Type of constant (DB_CHR_TYPE, DB_CHA_TYPE,
**			    DB_INT_TYPE, DB_DEC_TYPE, DB_FLT_TYPE, DB_VCH_TYPE)
**	value	{char *}    String representing the constant value.
**
** History:
**	08/86 (jhw) -- Written.
**	05/87 (jhw) -- Added support for SQL variable-length character strings.
**	07/90 (jhw) -- Ensure that floating point and decimal constants are
**		stored using '.' as the decimal point.  Bug #21811.
**	Revision 6.4
**	03/13/91 (emerson)
**		Get the decimal point character from the logical symbol
**		II_4GL_DECIMAL (whose value is now in osDecimal),
**		instead of II_DECIMAL.
*/
#define		HASHSIZE 71
static CONST	*consts_hash[NUM_TYPES][HASHSIZE] = {NULL};
static i4	consts_offset[NUM_TYPES] = {1, 1, 1, 1, 1};

ILREF
IGsetConst (type, value)
DB_DT_ID	type;
char		*value;
{
    register CONST	*lt;
    register char	*strp;
    register char	*cp;
    i4			itype;	/* internal type */
    CONST		**ent;
    i4			hashvalue;
    char		decimal[2];

    char	*iiIG_string();
    char	*iiIG_vstr();
    ADF_CB	*FEadfcb();

    switch(type)
    {
	case DB_CHA_TYPE:
	case DB_CHR_TYPE:
		itype = CHAR;
		break;
	case DB_INT_TYPE:
		itype = INT;
		break;
	case DB_FLT_TYPE:
		itype = FLOAT;
		break;
	case DB_DEC_TYPE:
		itype = DECIMAL;
		break;
	case DB_VCH_TYPE:
	case DB_TXT_TYPE:
		itype = VCHAR;
		break;
	default:
		EXsignal(EXFEBUG, 1, ERx("(in procedure `IGsetConst')"));
    } /* end switch */

    if ( itype == VCHAR )
	strp = value;
    else if ( ( itype != FLOAT && itype != DECIMAL )
		|| (decimal[1] = EOS, decimal[0] = osDecimal) == '.'
		|| (cp = STindex(value, decimal, 0)) == NULL )
    {
    	strp = iiIG_string(value);
    }
    else
    { /* replace decimal point with '.' */
	char	buf[64];

	_VOID_ STlcopy(value, buf, sizeof(buf));
	buf[cp - value] = '.';
	strp = iiIG_string(buf);
    }

    /* lint truncation warning if size of ptr > i4, but code valid */
#if defined(LP64)
    if ((hashvalue = ((i8)strp % HASHSIZE)) < 0)
#else /* lp64 */
    if ((hashvalue = ((i4)strp % HASHSIZE)) < 0)
#endif /* lp64 */
	hashvalue = 0;
    ent = &consts_hash[itype][hashvalue];
    for (lt = *ent; lt != NULL; lt = lt->c_next)
	if (lt->c_value == strp)
	    return - lt->c_offset;
    /*
    ** Constant was not in the table so add it.
    */
    while ((listp == NULL && (listp = FElpcreate(sizeof(CONST))) == NULL) ||
			(lt = (CONST *)FElpget(listp)) == NULL)
	EXsignal(EXFEMEM, 1, ERx("IGsetConst"));
    lt->c_value = strp;
    lt->c_offset = consts_offset[itype];
    consts_offset[itype] += 1;

    /* Add to hash list. */

    lt->c_next = *ent;
    *ent = lt;

    /* Add to constants list */

    if (*(ent = &consts_tab[itype]) == NULL)
	lt->c_list = lt;
    else
    {
	lt->c_list = (*ent)->c_list;
	(*ent)->c_list = lt;
    }
    *ent = lt;

    n_consts[itype] += 1;

    return - lt->c_offset;
}

/*
** Name:    IGgenConsts() -	Output Interpreted Frame Object Constants.
**
** Description:
**
** Input:
**	dfile_p		Debug file pointer.
**
** History:
**	08/86 (jhw) -- Written.
**	05/87 (jhw) -- Added support for SQL variable-length character strings.
**	18-jun-92 (davel)
**		added decimal support.
*/

VOID
IGgenConsts (dfile_p)
FILE	*dfile_p;
{
    i4	i;

    i4	IIAMscSConst();
    i4	IIAMicIConst();
    i4	IIAMfcFConst();
    i4	IIAMxcXConst();
    i4	IIAMdcDConst();

    for (i = (NUM_TYPES-1) ; i >= CHAR ; --i)
    {
	register char	**cp;
	char		**clist;
	STATUS		status;

	static i4  	(*func[])() = {
					IIAMscSConst,
					IIAMicIConst,
					IIAMfcFConst,
					IIAMxcXConst,
					IIAMdcDConst
			};
	static char	*type[] = {
				    ERx("CHAR"),
				    ERx("INT"),
				    ERx("FLOAT"),
				    ERx("VCHAR"),
				    ERx("DECIMAL")
			};

	register CONST   *con_p = consts_tab[i];
	if (con_p == NULL)
		continue;

	while ((clist =
		(char **)FEalloc((n_consts[i] + 1) * sizeof(*clist), &status)
	       ) == NULL)
	{
		EXsignal(EXFEMEM, 1, ERx("IGgenConst"));
	}
	con_p = con_p->c_list;
	consts_tab[i]->c_list = NULL;
	for (cp = clist ; con_p != NULL ; ++cp)
	{
		*cp = con_p->c_value;
		con_p = con_p->c_list;
	}
	*cp = NULL;
	(*func[i])(clist, n_consts[i]);
	if (dfile_p != NULL)
	{
		IIAMpcPrintConsts(dfile_p, type[i], clist, i == VCHAR);
	}
	consts_tab[i] = NULL;
    }
    if (listp != NULL)
    {
	FElpdestroy(listp);
	listp = NULL;
    }
}
