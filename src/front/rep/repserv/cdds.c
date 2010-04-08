/*
** Copyright (c) 2004 Ingres Corporation
*/
# include <compat.h>
# include <me.h>
# include <er.h>
# include <iiapi.h>
# include <sapiw.h>
# include "conn.h"
# include "cdds.h"

/**
** Name:	cdds.c - CDDS routines for RepServer
**
** Description:
**	Defines
**		RScdds_cache_init	- allocate and load the CDDS cache
**		RScdds_lookup		- look up a CDDS
**
** History:
**	16-dec-96 (joea)
**		Created based on cdds.sc in replicator library.
**	21-apr-98 (joea)
**		Convert to use OpenAPI instead of ESQL.
**	08-jul-98 (abbjo03)
**		Add new arguments to IIsw_selectSingleton.
**	24-feb-99 (abbjo03)
**		Replace II_PTR error handle parameters with IIAPI_GETEINFOPARM
**		error parameter blocks.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	26-mar-2004 (gupsh01)
**	    Added getQinfoParm as input parameter to II_sw functions.
**/

static RS_CDDS	*Cdds = NULL;		/* cdds array */
static i4	Cdds_Size = 0;		/* size of array */
static RS_CDDS	*last_ptr = NULL;	/* last found cdds */


/*{
** Name:	RScdds_cache_init - allocate and load the CDDS cache
**
** Description:
**	Allocates the Cdds array and initializes the array with the CDDS
**	information.
**
** Inputs:
**	none
**
** Outputs:
**	none
**
** Returns:
**	OK	- cache successfully initialized
**	FAIL	- an error occurred
*/
STATUS
RScdds_cache_init()
{
	RS_CDDS			cdds;
	i4			i;
	RS_CONN			*conn = &RSlocal_conn;
	IIAPI_DATAVALUE		cdata[3];
	II_PTR			stmtHandle;
	IIAPI_GETQINFOPARM	getQinfoParm;
	IIAPI_GETEINFOPARM	errParm;
	IIAPI_STATUS		status;

	if (Cdds != NULL)
		return (OK);

	SW_COLDATA_INIT(cdata[0], Cdds_Size);
	if (IIsw_selectSingleton(conn->connHandle, &conn->tranHandle,
		ERx("SELECT COUNT(*) FROM dd_cdds"), 0, NULL, NULL, 1, cdata,
		NULL, NULL, &stmtHandle, &getQinfoParm, &errParm) != IIAPI_ST_SUCCESS)
	{
		/* Error obtaining CDDS count */
		messageit(1, 1710);
		IIsw_close(stmtHandle, &errParm);
		IIsw_rollback(&conn->tranHandle, &errParm);
		return (FAIL);
	}
	IIsw_close(stmtHandle, &errParm);
	if (Cdds_Size == 0)
	{
		IIsw_rollback(&conn->tranHandle, &errParm);
		/* No CDDSs are defined */
		messageit(1, 1711);
		return (FAIL);
	}

	Cdds = (RS_CDDS *)MEreqmem((u_i4)0, (u_i4)(Cdds_Size *
		sizeof(RS_CDDS)), TRUE, (STATUS *)NULL);
	if (Cdds == NULL)
	{
		IIsw_rollback(&conn->tranHandle, &errParm);
		/* Error allocating CDDS cache */
		messageit(1, 1712);
		return (FAIL);
	}

	SW_COLDATA_INIT(cdata[0], cdds.cdds_no);
	SW_COLDATA_INIT(cdata[1], cdds.collision_mode);
	SW_COLDATA_INIT(cdata[2], cdds.error_mode);
	stmtHandle = NULL;
	for (i = 0; i <= Cdds_Size; ++i)
	{
		status = IIsw_selectLoop(conn->connHandle, &conn->tranHandle,
			ERx("SELECT cdds_no, collision_mode, error_mode FROM \
dd_cdds ORDER BY cdds_no"),
			0, NULL, NULL, 3, cdata, &stmtHandle, &getQinfoParm, 
			&errParm);
		if (status == IIAPI_ST_NO_DATA || status != IIAPI_ST_SUCCESS)
			break;
		Cdds[i] = cdds;		/* struct */
	}
	if (status == IIAPI_ST_NO_DATA && i != Cdds_Size && status !=
		IIAPI_ST_SUCCESS)
	{
		/* Error selecting CDDS information */
		messageit(1, 1713);
		IIsw_close(stmtHandle, &errParm);
		IIsw_rollback(&conn->tranHandle, &errParm);
		return (FAIL);
	}
	IIsw_close(stmtHandle, &errParm);

	if (IIsw_commit(&conn->tranHandle, &errParm) != IIAPI_ST_SUCCESS)
	{
		/* Error committing */
		messageit(1, 1798, ERx("INIT_CDDS_CACHE"),
			errParm.ge_errorCode, errParm.ge_message);
		IIsw_rollback(&conn->tranHandle, &errParm);
		return (FAIL);
	}

	return (OK);
}


/*{
** Name:	RScdds_lookup - look up a cdds in the Cdds array
**
** Description:
**	Looks up a cdds number in the CDDS array.  A binary search is
**	used.
**
** Inputs:
**	cdds_no		- the cdds number
**
** Outputs:
**	cdds_ptr	- pointer to a RS_CDDS structure
**
** Returns:
**	OK	Function completed normally.
**	FAIL	No such table was found.
*/
STATUS
RScdds_lookup(
i2	cdds_no,
RS_CDDS	**cdds_ptr)
{
	i4	low, mid, high, i;		/* binary search indices */

	if (Cdds == NULL)
	{
		/* Attempt to look up CDDS %d prior to cache initialization */
		messageit(1, 1716, cdds_no);
		return (FAIL);
	}

	/* check cache */
	if (last_ptr != NULL && last_ptr->cdds_no == cdds_no)
	{
		*cdds_ptr = last_ptr;
		return (OK);
	}

	/* initialize the binary search indices */
	low = 0;
	high = Cdds_Size - 1;

	/* perform the look-up */
	while (low <= high)
	{
		mid = (low + high) / 2;
		i = cdds_no - Cdds[mid].cdds_no;
		if (i < 0)
			high = mid - 1;		/* too big */
		else if (i > 0)
			low = mid + 1;		/* too small */
		else
		{
			last_ptr = &Cdds[mid];	/* found */
			*cdds_ptr = &Cdds[mid];
			return (OK);
		}
	}

	return (FAIL);				/* not found */
}
