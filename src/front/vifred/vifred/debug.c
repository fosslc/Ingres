/*
**  general purpose debugging routines
**
**  Copyright (c) 2004 Ingres Corporation
**
**  History:
**	02/04/85 (drh)	Added vfdmpon and vfdmpoff routines from VMS bug fixes
**	08/14/87 (dkh) - ER changes.
**	10/25/87 (dkh) - Error message cleanup.
**	19-jan-90 (bruceb)
**		Use proper size LOfroms buffer.
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/


# include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"decls.h"
# include	"vffiles.h"
# include	<lo.h>
# include	<er.h>
# include	"ervf.h"

VOID
prFeature(p)
POS	*p;
{
	switch (p->ps_name)
	{
	case PFIELD:
		prFld((FIELD *) p->ps_feat);
		break;

	case PTRIM:
		prTrim((TRIM *) p->ps_feat);
		break;
	}
}

VOID
prFld(fd)
FIELD	*fd;
{
	FLDHDR	*hdr;

	hdr = FDgethdr(fd);
	FTrestore(FT_NORMAL);
	SIfprintf(stderr, ERx("\r\tFIELD %s\n"), hdr->fhtitle);
	SIfprintf(stderr, ERx("\r\t\t(%d, %d) extent (%d, %d)\n"), hdr->fhposx,
	    hdr->fhposy, hdr->fhmaxx, hdr->fhmaxy);
	SIflush(stderr);
	FTrestore(FT_FORMS);
}

VOID
prTrim(tr)
TRIM	*tr;
{
	FTrestore(FT_NORMAL);
	SIfprintf(stderr, ERx("\r\tTRIM %s\n"), tr->trmstr);
	SIfprintf(stderr, ERx("\r\t\t(%d, %d) extent (%d, %d)\n"), tr->trmx,
	    tr->trmy, STlength(tr->trmstr), tr->trmy);
	SIflush(stderr);
	FTrestore(FT_FORMS);
}

/*
** that portion used for testing
*/
/*
** allocate the test file
*/

vfTestAlloc()
{
	LOCATION	loc;
	char		buf[MAX_LOC + 1];

	if (vfdname != NULL)
	{
		STcopy(vfdname, buf);
		if (LOfroms(PATH & FILENAME, buf, &loc) != OK)
		{
			syserr(ERget(S_VF0033_vfTestAlloc_cant_co));
		}
		if (SIopen(&loc, ERx("w"), &vfdfile) != OK)
		{
			syserr(ERget(S_VF0034_vfTestAlloc_cant_op), vfdname);
		}
		FDdmpinit(vfdfile);
	}
	if (vfrname != NULL)
		FDrcvalloc(vfrname);
}

VOID
vfdmpon()
{
	VTdumpset(vfdfile);
}


VOID
vfdmpoff()
{
	VTdumpset(NULL);
}

vftestclose()
{
	if (vfdname != NULL)
		SIclose(vfdfile);
	if (vfrname != NULL)
		FDrcvclose(FALSE);
}

VOID
vfTestDump()
{
	FDdmpcur();
}
