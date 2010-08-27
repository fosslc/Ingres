/*
** Copyright (c) 1989, 2008 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cm.h>
#include    <me.h>
#include    <nm.h>
#include    <st.h>
#include    <lo.h>
#include    <si.h>
#include    <cs.h>
#include    <lnmdef.h>
#include    <iodef.h>
#include    <fab.h>
#include    <rab.h>
#include    <ssdef.h>
#include    <starlet.h>


/**
**
**  Name: CMFILE.C - Routines to read and write collation files
**
**  Description:
**	The file contains routines to read and write collation sequence
**	description file.  This seporate from DI because it is needed
**	by FE programs and it is not SI because that is not legal for
**	the DB server.
**
**		CMopen_col	- open collation file for reading
**		CMread_col	- read collation file
**		CMclose_col	- close collation file
**		CMdump_col	- create and write a collation file.
**
**  History:
**      16-Jun-89 (anton)
**	    Created.
**      16-jul-93 (ed)
**	    added gl.h
**      11-aug-93 (ed)
**          added missing includes
**	19-jul-2000 (kinte01)
**	    Correct prototype definitions by adding missing includes
**	    and external function declarations
**	01-dec-2000	(kinte01)
**		Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**		from VMS CL as the use is no longer allowed
**	14-jan-2004 (gupsh01)
**	    Added CM_dumpUniMap function, to write and read data from
**	    $II_SYSTEM/ingres/files/unicharmaps, which contain mapping
**	    between unicode and local character sets.
**	16-jan-2004 (gupsh01)
**	    Fixed CMopen_col function.
**	18-feb-2004 (gupsh01)
**	    Added CM_DEFAULTFILE_LOC which allows looking for default unicode
**	    coercion mapping file in $II_SYSTEM/ingres/files directory which
**	    is not yet copied to $II_SYSTEM/ingres/files/ucharmaps directory,
**	    at install time.
**	11-nov-2008 (joea)
**	    Use CL_CLEAR_ERR to initialize CL_ERR_DESC.
**	27-Jul-2010 (frima01) Bug 124137
**	    Evaluate return codes of NMloc to avoid using corrupt pointers.
**/

FUNC_EXTERN bool cer_issem();

typedef	struct _ER_SEMFUNCS
{
    STATUS	(*er_p_semaphore)();
    STATUS	(*er_v_semaphore)();
    CS_SEMAPHORE	er_sem;
} ER_SEMFUNCS;

static	struct _CMCOLDFLE
{
    struct	RAB	rab;
    struct	FAB	fab;
    ER_SEMFUNCS		*sems;
} file;

/*{
** Name: CMopen_col - open collation file for reading
**
** Description:
**	open collation file for reading
**
** Inputs:
**	colname				collation name
**
** Outputs:
**	syserr				OS specific error
**
**	Returns:
**	    E_DB_OK
**	    FAIL
**
**	Exceptions:
**	    none
**
** Side Effects:
**	May acquire a semaphore to protect itself from reentry and
**	hold that semaphore until the file is closed.
**	>>>>>CMclose_col must be called to release this semaphore<<<<<<
**
** History:
**      16-Jun-89 (anton)
**          Created.
**	14-jan-2004 (gupsh01)
**	    Added CM_dumpUniMap function, to write and read data from
**	    $II_SYSTEM/ingres/files/unicharmaps, which contain mapping
**	    between unicode and local character sets.
*/
STATUS
CMopen_col(
char		*colname,
CL_ERR_DESC	*syserr,
i4		loctype)
{
    LOCATION	loc;
    char	*chrp;
    STATUS	status;

    CL_CLEAR_ERR(syserr);

    if (cer_issem(&file.sems))
    {
	(*file.sems->er_p_semaphore)(TRUE, &file.sems->er_sem);
    }

    if ( NMloc(FILES, PATH, (char *)NULL, &loc) != OK )
    {
	if (file.sems)
	{
	    (*file.sems->er_v_semaphore)(&file.sems->er_sem);
	}
	return FAIL;
    }

    if (loctype == CM_UCHARMAPS_LOC)
	LOfaddpath(&loc, "ucharmaps", &loc);
    else if (loctype == CM_COLLATION_LOC)
	LOfaddpath(&loc, "collation", &loc);
    LOfstfile(colname, &loc);
    LOtos(&loc, &chrp);

    MEfill(sizeof file.fab, '\0', (PTR)&file.fab);
    MEfill(sizeof file.rab, '\0', (PTR)&file.rab);
    file.fab.fab$b_bid = FAB$C_BID;
    file.fab.fab$b_bln = FAB$C_BLN;
    file.rab.rab$b_bid = RAB$C_BID;
    file.rab.rab$b_bln = RAB$C_BLN;
    file.rab.rab$l_fab = &file.fab;
    file.fab.fab$l_fna = chrp;
    file.fab.fab$b_fns = STlength(chrp);
    file.fab.fab$b_rfm = FAB$C_FIX;
    file.fab.fab$b_fac = FAB$M_GET;
    file.fab.fab$b_shr = FAB$M_SHRGET;
    syserr->error = status = sys$open(&file.fab);
    if ((status & 1) == 0)
    {
	if (file.sems)
	{
	    (*file.sems->er_v_semaphore)(&file.sems->er_sem);
	}
	return FAIL;
    }
    syserr->error = status = sys$connect(&file.rab);
    if ((status & 1) == 0)
    {
	if (file.sems)
	{
	    (*file.sems->er_v_semaphore)(&file.sems->er_sem);
	}
	return FAIL;
    }
    file.rab.rab$w_usz = COL_BLOCK;
    return OK;
}

/*{
** Name: CMread_col - read from collation file
**
** Description:
**	read a COL_BLOCK size record from open collation file
**
** Inputs:
**	bufp				pointer to buffer
**
** Outputs:
**	syserr				OS specific error
**
**	Returns:
**	    E_DB_OK
**	    FAIL
**
**	Exceptions:
**	    none
**
** Side Effects:
**	moves open file to next record
**
** History:
**      16-Jun-89 (anton)
**          Created.
*/
STATUS
CMread_col(bufp, syserr)
char		*bufp;
CL_ERR_DESC	*syserr;
{
    CL_CLEAR_ERR(syserr);
    file.rab.rab$l_ubf = bufp;
    if (((syserr->error = sys$get(&file.rab)) & 1) == 0)
    {
	return FAIL;
    }
    return OK;
}

/*{
** Name: CMclose_col - close collation file
**
** Description:
**	close an open collation file
**
** Inputs:
**	none
**
** Outputs:
**	syserr				OS specific error
**
**	Returns:
**	    E_DB_OK
**	    FAIL
**
**	Exceptions:
**	    none
**
** Side Effects:
**	Releases semaphore which may have been acquired durring CMopen_col
**
** History:
**      16-Jun-89 (anton)
**          Created.
**	14-Jan-2004 (gupsh01)
**	    Added the file location type in input parameter, it can now
**	    be CM_COLLATION_LOC for $II_SYSTEM/ingres/files/collation or
**	    CM_UCHARMAPS_LOC for $II_SYSTEM/ingres/filese/ucharmaps
**	    for unicode and local encoding mapping files.
*/
STATUS
CMclose_col(
CL_ERR_DESC	*syserr,
i4		loctype)
{
    CL_CLEAR_ERR(syserr);
    syserr->error = sys$close(&file.fab);
    if (file.sems)
    {
	(*file.sems->er_v_semaphore)(&file.sems->er_sem);
    }
    if ((syserr->error & 1) == 0)
    {
	return FAIL;
    }
    return OK;
}

/*{
** Name: CMdump_col - open collation file for reading
**
** Description:
**	open collation file for reading
**
** Inputs:
**	colname				collation / target file name
**	tablep				collation table pointer
**	tablen				collation table length
**	loctype				type of location, collation
**					or charmap
**
** Outputs:
**	syserr				OS specific error
**
**	Returns:
**	    E_DB_OK
**	    FAIL
**
**	Exceptions:
**	    none
**
** Side Effects:
**	none
**
** History:
**      16-Jun-89 (anton)
**          Created.
**	14-Jan-2004 (gupsh01)
**	    Added the file location type in input parameter, it can now
**	    be CM_COLLATION_LOC for $II_SYSTEM/ingres/files/collation or
**	    CM_UCHARMAPS_LOC for $II_SYSTEM/ingres/filese/ucharmaps
**	    for unicode and local encoding mapping files.
*/
STATUS
CMdump_col(
char		*colname,
PTR		tablep,
i4		tablen,
CL_ERR_DESC	*syserr,
i4		loctype)
{
    LOCATION	loc;
    char	*tp, *chrp;
    STATUS	status;

    CL_CLEAR_ERR(syserr);
    if ( NMloc(FILES, PATH, (char *)NULL, &loc) != OK )
	return FAIL;
    if (loctype == CM_UCHARMAPS_LOC)
	LOfaddpath(&loc, "ucharmaps", &loc);
    else if (loctype == CM_COLLATION_LOC)
	LOfaddpath(&loc, "collation", &loc);
    LOfstfile(colname, &loc);
    LOtos(&loc, &chrp);
    MEfill(sizeof file.fab, '\0', (PTR)&file.fab);
    MEfill(sizeof file.rab, '\0', (PTR)&file.rab);
    file.fab.fab$b_bid = FAB$C_BID;
    file.fab.fab$b_bln = FAB$C_BLN;
    file.fab.fab$b_fac = FAB$M_PUT|FAB$M_GET;
    file.fab.fab$l_fna = chrp;
    file.fab.fab$b_fns = STlength(chrp);
    file.fab.fab$b_rfm = FAB$C_FIX;
    file.fab.fab$w_mrs = COL_BLOCK;
    status = syserr->error = sys$create(&file.fab);
    if ((status & 1) == 0)
	return FAIL;
    file.rab.rab$b_bid = RAB$C_BID;
    file.rab.rab$b_bln = RAB$C_BLN;
    file.rab.rab$l_fab = &file.fab;
    status = syserr->error = sys$connect(&file.rab);
    if ((status & 1) == 0)
	return FAIL;
    tp = (char *)tablep;
    while (tablen > 0)
    {
	file.rab.rab$w_rsz = COL_BLOCK;
	file.rab.rab$l_rbf = tp;
	status = syserr->error = sys$put(&file.rab);
	if ((status & 1) == 0)
	    return FAIL;
	tp += COL_BLOCK;
	tablen -= COL_BLOCK;
    }
    _VOID_ sys$disconnect(&file.rab);
    status = syserr->error = sys$close(&file.fab);
    if ((status & 1) == 0)
	return FAIL;
    return OK;
}
