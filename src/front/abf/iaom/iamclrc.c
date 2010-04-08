/*
** Copyright (c) 1987, 2008 Ingres Corporation
*/

#include	<compat.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<ooclass.h>
#include	<acatrec.h>
#include	"iamstd.h"

/**
** Name:	iamclrc.c -	Clean up ABF Catalog Record Module.
**
** Description:
**	Clean up catalog record based on class before it will be written
**	to the database.  Defines:
**
**	iiam
**
** History:
**	5/87 (bobm)	written
**      17-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
*/

static const char _[] = ERx("");

/*{
** Name:	clean_record - null unused record fields
**
** Description:
**	puts null data into catalog record fields that don't
**	apply to records of that class, and returns an error if record
**	is noticably bad.  Called only internally to AOM.
**
** Inputs:
**	rec	catalog record.
**
** Returns:
**	{STATUS}	OK		success
**			ILE_ARGBAD	bad record class
**
** History:
**	5/87 (bobm)	written
*/

STATUS
clean_record ( rec )
register ACATREC	*rec;
{
	/* arg array */
	switch (rec->class)
	{
	case OC_UNFRAME:
	case OC_UNPROC:
	case OC_DBPROC:
	case OC_OSLPROC:
	case OC_GLOBAL:
	case OC_CONST:
	case OC_RECORD:
	case OC_OLDOSLFRAME:
	case OC_AFORMREF:
		(rec->arg)[0] = _;	/* FALL THROUGH */
	case OC_OSLFRAME:
	case OC_HLPROC:
		(rec->arg)[1] = _;	/* FALL THROUGH */
	case OC_RWFRAME:
	case OC_QBFFRAME:
	case OC_GRFRAME:
	case OC_GBFFRAME:
		(rec->arg)[2] = _;
		(rec->arg)[3] = _;	/* FALL THROUGH */
	case OC_APPL:
		(rec->arg)[4] = _;
		(rec->arg)[5] = _;
		break;
	case OC_MUFRAME:
	case OC_APPFRAME:
	case OC_UPDFRAME:
	case OC_BRWFRAME:
		(rec->arg)[0] = (rec->arg)[1] = _;
		(rec->arg)[2] = (rec->arg)[3] = _;
		break;
	default:
		return ILE_ARGBAD;
	}

	/* source / symbol */
	switch (rec->class)
	{
	case OC_GLOBAL:
	case OC_CONST:
	case OC_RECORD:
	case OC_QBFFRAME:
	case OC_GRFRAME:
	case OC_GBFFRAME:
	case OC_UNFRAME:
	case OC_UNPROC:
		rec->source = _; /* FALL THROUGH */
	case OC_DBPROC:
	case OC_RWFRAME:
	case OC_APPL:
		rec->symbol = _;	/* FALL THROUGH */
	default:
		break;
	}

	/* return type */
	switch (rec->class)
	{
	case OC_GLOBAL:
	case OC_CONST:
	case OC_RECORD:
	case OC_OSLPROC:
	case OC_OSLFRAME:
	case OC_HLPROC:
	case OC_DBPROC:
		break;
	case OC_APPL:
		/* saves flags in data length */
		rec->retadf_type = DB_NODT;
		rec->retscale = 0;
		rec->rettype = _;
		break;
	default:
		rec->retadf_type = DB_NODT;
		rec->retlength = 0;
		rec->retscale = 0;
		/* fall through ... */
	case OC_UNFRAME:
	case OC_UNPROC:
		rec->rettype = _;
		break;
	}

	return OK;
}
