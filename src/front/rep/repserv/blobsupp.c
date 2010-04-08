/*
** Copyright (c) 2004 Ingres Corporation
*/
# include <compat.h>
# include <st.h>
# include <nm.h>
# include <lo.h>
# include <si.h>
# include <iiapi.h>
# include <sapiw.h>
# include "repserv.h"

/**
** Name:	blobsupp.c - blob support functions
**
** Description:
**	Defines
**		RSblob_gcCallback	- IIapi_getColumns callback for blobs
**		RSblob_ppCallback	- IIsw_putParms callback for blobs
**
** History:
**	09-jul-98 (abbjo03)
**		Created.
**	08-mar-99 (abbjo03)
**		Remove temporary file after reading the last segment.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/*{
** Name:	RSblob_gcCallback - IIapi_getColumns callback for blobs
**
** Description:
**	If the blob is smaller than DB_MAXTUP-2 bytes, set the blob size and
**	return.  Otherwise, open a temporary file in II_TEMPORARY and write the
**	blob segments to it.
**
** Inputs:
**	closure		- closure (unused)
**	parmBlock	- IIAPI_GETCOLPARM parameter block
**
** Outputs:
**	RS_BLOB buffer is updated.
**
** Returns:
**	none
*/
II_VOID II_FAR II_CALLBACK
RSblob_gcCallback(
II_PTR	closure,
II_PTR	parmBlock)
{
	IIAPI_GETCOLPARM	*gcolParm = (IIAPI_GETCOLPARM *)parmBlock;
	RS_BLOB			*blob =
				(RS_BLOB *)gcolParm->gc_columnData[0].dv_value;
	LOCATION		loc;
	char			*p;
	i4			cnt;

	if (!blob->fp && !blob->size)
	{
		/*
		** If this is the only segment, just set the size from the
		** length.
		*/
		if (!gcolParm->gc_moreSegments)
		{
			blob->size = blob->len;
			return;
		}
		/*
		** If there are more segments and the file hasn't been opened,
		** create a new temporary file.
		*/
		NMloc(TEMP, PATH, NULL, &loc);
		LOuniq(ERx("bl"), ERx("tmp"), &loc);
		LOtos(&loc, &p);
		STcopy(p, blob->filename);
		if (SIfopen(&loc, ERx("w"), SI_VAR, sizeof blob->buf, &blob->fp)
			!= OK)
		{
			gcolParm->gc_genParm.gp_status = IIAPI_ST_OUT_OF_MEMORY;
			return;
		}
	}
	/* write the current segment to a file and add to the size */
	if (SIwrite((i4)blob->len, blob->buf, &cnt, blob->fp) != OK)
	{
		gcolParm->gc_genParm.gp_status = IIAPI_ST_OUT_OF_MEMORY;
		return;
	}
	blob->size += blob->len;
	/* if this is the last segment, close the temporary file */
	if (!gcolParm->gc_moreSegments)
	{
		if (SIclose(blob->fp) != OK)
		{
			gcolParm->gc_genParm.gp_status = IIAPI_ST_OUT_OF_MEMORY;
			return;
		}
		blob->fp = NULL;
	}
}


/*{
** Name:	RSblob_ppCallback - IIsw_putParms callback for blobs
**
** Description:
**	If the blob is smaller than DB_MAXTUP-2 bytes, set the blob size and
**	return.  Otherwise, open a temporary file in II_TEMPORARY and write the
**	blob segments to it.
**
** Inputs:
**	closure		- closure (unused)
**	parmBlock	- IIAPI_PUTPARMPARM parameter block
**
** Outputs:
**	RS_BLOB buffer is updated.
**
** Returns:
**	none
*/
II_VOID II_FAR II_CALLBACK
RSblob_ppCallback(
II_PTR	closure,
II_PTR	parmBlock)
{
	IIAPI_PUTPARMPARM	*putParm = (IIAPI_PUTPARMPARM *)parmBlock;
	RS_BLOB			*blob =
				(RS_BLOB *)putParm->pp_parmData[0].dv_value;
	LOCATION		loc;
	i4			cnt;
	STATUS			status;

	if (!blob->fp)
	{
		/* If there is only one segment, indicate this and return */
		if (blob->size <= sizeof blob->buf)
		{
			putParm->pp_moreSegments = FALSE;
			return;
		}
		/* Open the temporary file */
		LOfroms(PATH & FILENAME, blob->filename, &loc);
		if (SIfopen(&loc, ERx("r"), SI_VAR, sizeof blob->buf, &blob->fp)
			!= OK)
		{
			putParm->pp_genParm.gp_status = IIAPI_ST_OUT_OF_MEMORY;
			return;
		}
	}
	/*
	** Read the next segment from the file. If it's the last segment, close
	** the temporary file.
	*/
	status = SIread(blob->fp, sizeof blob->buf, &cnt, blob->buf);
	putParm->pp_moreSegments = TRUE;
	blob->len = (short)cnt;
	if (status == ENDFILE)
	{
		SIclose(blob->fp);
		blob->fp = NULL;
		putParm->pp_moreSegments = FALSE;
		LOfroms(PATH & FILENAME, blob->filename, &loc);
		LOdelete(&loc);
	}
	else if (status != OK)
	{
		putParm->pp_genParm.gp_status = IIAPI_ST_OUT_OF_MEMORY;
	}
}
