/*
** Copyright (c) 1987, 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<si.h>
# include	<cv.h>
# include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ooclass.h>
# include	<cu.h>
# include	<acatrec.h>

/**
** Name:	rtof.c -	Write an ACATREC to a copyapp file.
**
** Description:
**	This writes an ACATREC to a copyapp file.
**
**	iicaWrite()
**
** History:
**	21-jul-1987 (Joe)
**		Initial Version
**	09-dec-1993 (daver)
**		Add owner from ii_abfdependencies' abfdef_owner. Corresponds 
**		to owner.table for 6.5 and above.
**	10-dec-1993 (daver)
**		Initialize the "new" owner column to blank for DUMMY 
**		dependencies.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      17-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
**/

static STATUS	_WriteDep();

/*{
** Name:	iicaWrite() -	Write a ACATREC to a file.
**
** Description:
**	Given an ACATREC, this routine writes that record
**	to a file.
**
**	This writes a single detail line that starts with
**	a \t and ends with a \n.  The order of the fields
**	is that needed by a detail record for II_ABFOBJECTS
**	which is given in the TLIST for the CURECORD for
**	applications and application objects.
**
**	This file must do any conversions to the ACATREC that
**	the IAOM routine IIAMcaCatAdd does.  This includes:
**		lowercasing the name.
**		Making sure any strings that are NULL point to the empty string.
**
** Inputs:
**	arec	{ACATREC *}  The ABF application catalog record for an object
**				to write to the file.
**	long_remark {char *}  The long remark for the object.
**	level	{nat}  The level of this object.
**	fp	{FILE *}  The file to which to output the record.
**
** Returns:
**	{STATUS}  OK if succeeded.  Status of failure otherwise.
**
** History:
**	21-jul-1987 (Joe)
**		Initial Version
**	23-dec-1987 (Joe)
**		Lowercase the name in the record.
**	04/89 (jhw) -- Added 'long_remark' as parameter plus changes to
**		ACATREC structure.
**	10-dec-1993 (daver)
**		Initialize the "new" owner column to blank for DUMMY 
**		dependencies.
*/
STATUS
iicaWrite ( arec, long_remark, level, fp )
register ACATREC	*arec;
char			*long_remark;
i4			level;
register FILE		*fp;
{
	static const char	_Empty[1] = {EOS};

	register i4	i;
	OOID		deptype;

	switch(arec->class)
	{
	case OC_BRWFRAME:
	case OC_APPFRAME:
	case OC_UPDFRAME:
	case OC_MUFRAME:
		/*
		** metaframe type -- twiddle gen dates and flag bits
		*/
		IICAmtMetaTwiddle(arec);
		break;
	}

	CVlower(arec->name);
	cu_wrobj( arec->class, level,
			arec->name, arec->short_remark, long_remark, fp
	);
	SIfprintf( fp, ERx("\t%s\t%s\t%d\t%s\t"),
			( arec->source != NULL ) ? arec->source : _Empty,
			( arec->symbol != NULL ) ? arec->symbol : _Empty,
			arec->retadf_type,
			( arec->rettype != NULL ) ? arec->rettype : _Empty
	);
	SIfprintf( fp, ERx("%d\t%d\t%d\t"),
			arec->retlength,
			arec->retscale,
			arec->version
	);

	for ( i = 0 ; i < ACARGS ; ++i )
	{
		if ( arec->arg[i] == NULL )
			arec->arg[i] = _Empty;
	}
	SIfprintf( fp, ERx("%s\t%s\t%s\t%s\t%s\t%s\t%d\n"),
			arec->arg[0],
			arec->arg[1],
			arec->arg[2],
			arec->arg[3],
			arec->arg[4],
			arec->arg[5],
			arec->flags
	);

	switch ( arec->class )
	{
	case OC_RECORD:
		/* 
		** User-defined class -- write out the class attributes. 
		*/
		_VOID_ IICUclwCLassWrite(arec->ooid, fp);
		break;
	case OC_BRWFRAME:
	case OC_APPFRAME:
	case OC_UPDFRAME:
	case OC_MUFRAME:
		/*
		** metaframe type -- write out the auxiliary tables.
		*/
		IICAmfwMetaFrameWrite(arec->ooid, fp);
		break;
	}

	deptype = arec->class == OC_APPL ? CUC_ADEPEND : CUC_AODEPEND;
	if ( arec->dep_on == NULL )
	{
		ACATDEP	dep;

		/*
		** Add dummy dependency record.
		*/
		dep.class = 0;
		dep.deptype = 0;
		dep.name = ERx("DUMMY");
		dep.deplink = ERx("");
		dep.deporder = 0;
		dep.owner = ERx("");
		cu_wrcomp(deptype, fp);
		_WriteDep(&dep, fp);
	}
	else
	{
		register ACATDEP	*l;

		for (l = arec->dep_on; l != NULL; l = l->next)
		{
			if (l == arec->dep_on)
			{
				cu_wrcomp(deptype, fp);
			}
			_WriteDep(l, fp);
		}
	}
    return OK;
}

/*{
** Name:	_WriteDep() -	Write ACATDEP to a file.
**
** Description:
**	Given an ACATDEP this routine writes it to a copyapp
**	file.  This routine doesn't write out those of class
**	OC_ILCODE.
**
**	Layout of fields is given by CUTLIST for CUC_*64DEPEND.
**
** Inputs:
**	d		The ACATDEP to write out.
**
** Outputs:
**	fp		The file to output to.
**
**	Returns:
**		OK if succeeded, status of error otherwise.
** History:
**	21-jul-1987 (Joe)
**	09-dec-1993 (daver)
**		Add owner from ii_abfdependencies' abfdef_owner. Corresponds 
**		to owner.table for 6.5 and above.
*/
static STATUS
_WriteDep ( d, fp )
ACATDEP *d;
FILE	*fp;
{
    if (d->class != OC_ILCODE)
    {
	if (d->owner[0] == EOS)
	{
    		/* double tab corresponds to "owner" */
		SIfprintf(fp, ERx("\t%s\t\t%d\t%d\t%s\t%d\n"), d->name, 
			d->class, d->deptype, d->deplink, d->deporder);
	}
	else
	{
		SIfprintf(fp, ERx("\t%s\t%s\t%d\t%d\t%s\t%d\n"), d->name, 
			d->owner, d->class, d->deptype, d->deplink, 
			d->deporder);
	}
    }
    return OK;
}
