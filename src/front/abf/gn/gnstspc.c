/*
**Copyright (c) 1987, 2004 Ingres Corporation
** All rights reserved.
*/

# include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<er.h>
# include	<gn.h>
# include	"gnspace.h"
# include	"gnint.h"

/**
** Name:	gnstspc.c - start name space
**
** Defines
**		IIGNssStartSpace
**
** History:
**	21-sep-88 (kenl)
**		Changed FEtalloc() call to FEreqmem().
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

FUNC_EXTERN i4  hash_func();
FUNC_EXTERN i4  comp_func();

extern SPACE *List;

/*{
** Name:   IIGNssStartSpace - Start a new name space
**
** Description:
**
**     Called to initialize a new numbered name space.	Must be called
**     once with each number to be used before any other calls pertaining
**     to that numbered space.	Not all of the input parameters would be
**     needed to use this library simply for compiler symbol generation.
**     The crule parameter merely sets the treatment for generated names,
**     which are generated in a case insensitive fashion no matter what
**     the setting, except for crule = GN_SIGNIFICANT
**
** Inputs:
**
**     gcode - generation class code, ie. numeric identifier for table.
**     pfx - generated name prefix.
**     ulen - TOTAL characters (counting prefix) in which name must be unique.
**     tlen - truncation length, ie. maximum length of name.
**     crule - case rule:
**
**	 GN_UPPER    upper case names
**	 GN_LOWER    lower case names
**	 GN_MIXED    names preserve case of user names
**
**	 GN_SIGNIFICANT names have case significance.
**
**     arule - non-alphanumeric treatment rule:
**
**	GNA_REJECT	illegal name
**	GNA_XUNDER	translate to underscores
**	GNA_ACCEPT	accept
**
**     afail - routine to call for allocation failures in manipulating
**		the space.  if NULL, memory allocation failure will be
**		fatal.	If this routine doesn't return, GNE_MEM returns
**		won't be possible in this or other routines dealing
**		with this numbered space.
**
** Outputs:
**
**   return:
**	OK		success
**	GNE_PARM	bad parameters
**	GNE_CODE	gcode already being used
**	GNE_MEM		allocation failure
*/

/*
** default alloc fail routine
*/
static VOID
def_afail()
{
	IIUGbmaBadMemoryAllocation(ERx("GN, def_afail"));
}

STATUS
IIGNssStartSpace (gcode,pfx,ulen,tlen,crule,arule,afail)
i4  gcode;
char *pfx;
i4  ulen;
i4  tlen;
i4  crule;
i4  arule;
VOID (*afail)();
{
	SPACE *sp;
	VOID (*hfail)();
	i2 tag;
	i4 plen;	/* length of prefix */

	/*
	** check legal parameters before we do anything else
	*/
	switch(crule)
	{
	case GN_UPPER:
	case GN_LOWER:
	case GN_MIXED:
	case GN_SIGNIFICANT:
		break;
	default:
		return (GNE_PARM);
	}
	switch(arule)
	{
	case GNA_REJECT:
	case GNA_XUNDER:
	case GNA_XPUNDER:
	case GNA_ACCEPT:
		break;
	default:
		return (GNE_PARM);
	}
	plen = STlength(pfx);
	if (tlen < ulen || ulen <= (plen+MINULEN))
		return (GNE_PARM);

	/*
	** Already exist?
	*/
	if (find_space(gcode,&sp) != NULL)
		return (GNE_CODE);


	/*
	** if afail is NULL, we use the default function, and pass
	** NULL to the hash table routines, to get THEIR fatal message
	** if that's where alloc fails.	 If the caller specified afail,
	** the hash table routines get it also.
	*/
	if (afail != NULL)
		hfail = afail;
	else
	{
		hfail = NULL;
		afail = def_afail;
	}

	/*
	** get a new SPACE block with a new tag
	*/
	tag = FEgettag();
	if ((sp = (SPACE *)FEreqmem((u_i4)(tag), (u_i4)sizeof(SPACE),
		FALSE, (STATUS *)NULL)) == NULL)
	{
		(*afail)();
		return (GNE_MEM);
	}

	/*
	** create a hash table to go with it:
	** we pass in our own hash function & comparison routine
	** which will utilize the length restrictions desired.
	** because, WE'RE supplying a hash function HtabInit won't
	** adjust size if needed.  Hence we do our own size adjustment -
	** hash_func wants a prime table size.
	*/
	if (IIUGhiHtabInit(IIUGnpNextPrime(TABSIZE), hfail,
				comp_func, hash_func, &(sp->htab)) != OK)
		return (GNE_MEM);

	/*
	** record parameters for future use, and link onto list.
	*/
	sp->gcode = gcode;
	sp->afail = afail;
	sp->tag = tag;
	sp->pfxlen = plen;
	sp->pfx = pfx;
	sp->crule = crule;
	sp->arule = arule;
	sp->ulen = ulen;
	sp->tlen = tlen;
	sp->next = List;
	List = sp;

	return (OK);
}
