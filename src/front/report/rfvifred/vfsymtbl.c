/*
**  buildst.c
**
**  Copyright (c) 2004 Ingres Corporation
**
**  History:
**	08/14/87 (dkh) - ER changes.
**	10/25/87 (dkh) - Error message cleanup.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"decls.h"
# include	"vfunique.h"
# include	<er.h>
# include	<ug.h>
# include	"ervf.h"

i4	vftfstchk();

void
vfsterrmsg(name, type)
char	*name;
i4	type;
{
	char	*cp;
	if (type == AFIELD)
	{
		cp = ERget(F_VF0011_field);
	}
	else
	{
		cp = ERget(F_VF0032_column);
	}
	IIUGerr(E_VF00FC_Warning_Duplicate, UG_ERR_ERROR, 2, cp, name);
}



void
vfstinconsist(name)
char	*name;
{
	IIUGerr(E_VF00FD_Inconsistent_symbol, UG_ERR_ERROR, 1, name);
}


i4
buildst(first)
bool	first;
{
	reg	i4	i;
	reg	VFNODE	**lp;
	reg	VFNODE	*lt;
	reg	FIELD	*field;
	reg	FLDHDR	*hdr;
	reg	POS	*ps;
	reg	i4	err = 0;
	reg	TBLFLD	*tf;

	if (!vfuchkon)
	{
		return (err);
	}
	lp = line;
	for (i = 0; i < endFrame; i++, lp++)
	{
		for (lt = *lp; lt != NNIL; lt = vflnNext(lt, i))
		{
			if (lt->nd_pos->ps_begy != i)
				continue;

			ps = lt->nd_pos;
			switch(ps->ps_name)
			{
			case PFIELD:
				field = (FIELD *) ps->ps_feat;
				hdr = FDgethdr(field);
				if (vfnmunique(hdr->fhdname,
					TRUE, AFIELD) == FOUND)
				{
					err++;
					if (first)
					{
						vfsterrmsg(
							hdr->fhdname, AFIELD);
					}
				}
				break;

			case PTABLE:
				field = (FIELD *) ps->ps_feat;
				tf = field->fld_var.fltblfld;
				hdr = &tf->tfhdr;
				if (vfnmunique(hdr->fhdname,
					TRUE, AFIELD) == FOUND)
				{
					err++;
					if (first)
					{
						vfsterrmsg(
							hdr->fhdname, AFIELD);
					}
				}
				if (vftfstchk(tf, first) != 0)
				{
					err++;
				}
				break;

			case PBOX:
			case PTRIM:
				break;

			default:
				syserr(ERget(S_VF00FE_buildst));
				break;
			}
		}
	}
	return (err);
}


i4
vftfstchk(tf, first)
TBLFLD	*tf;
bool	first;
{
	reg	FLDCOL	*col;
	reg	FLDHDR	*hdr;
	reg	i4	colcount;
	reg	i4	i;
	reg	i4	err = 0;

	if (!vfuchkon)
	{
		return (err);
	}
	colcount = tf->tfcols;
	for (i = 0; i < colcount; i++)
	{
		col = tf->tfflds[i];
		hdr = &col->flhdr;
		if (vfnmunique(hdr->fhdname, TRUE, ATBLFD) == FOUND)
		{
			err++;
			if (first)
			{
				vfsterrmsg(hdr->fhdname, ATBLFD);
			}
		}
	}
	vfstdel(ATBLFD);
	return (err);
}
