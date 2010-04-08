/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: DMFINIT.H - Function prototypes for routines in dmfinit.c.
**
** Description:
**      This file contains the types and constants used by dmfinit.c
**
** History:
**	26-apr-1993 (bryanp)
**	    Created this file because I didn't think that the dmfinit.c
**	    function prototypes belonged in dm.h.
**	26-jul-1993 (bryanp)
**	    Added csp_lock_list argument to dmf_init for use by CSP.
**	18-apr-1994 (jnash)
**	    Added dmf_setup_optim() FUNC_EXTERN.
**      06-mar-1996 (stial01)
**          Variable page size, added buffers parm to dmf_init
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	3-Jan-2006 (kschendel)
**	    Revise parameter list.
**	15-May-2007 (toumi01)
**	    For supportability add process info to shared memory.
**	12-Nov-2008 (jonj)
**	    SIR 120874 Change function prototypes to pass DB_ERROR *dberr
**	    instead of i4 *err_code.
**	5-Nov-2009 (kschendel) SIR 122757
**	    Rename setup-optim to setup-directio.
**/

/*
**  Forward and/or External function references.
*/

FUNC_EXTERN DB_STATUS dmf_init(
    i4	dm0l_flag,
    bool connect_only,
    char *cache_name,
    i4     buffers[],
    i4	csp_lock_list,
    i4	size,
    i4	lg_id,
    i4	maxdb,
    DB_ERROR	*dberr,
    char *lgk_info);


FUNC_EXTERN PTR dmf_getadf(VOID);

FUNC_EXTERN STATUS  dmf_udt_lkinit(
    i4	lock_list,
    i4		create_flag,
    DB_ERROR	*dberr);

FUNC_EXTERN DB_STATUS	dmf_sxf_bgn_session(
	DB_OWN_NAME *ruser,
	DB_OWN_NAME *user,
	char *dbname,
	i4 ustat);

FUNC_EXTERN DB_STATUS	dmf_sxf_end_session(void);

FUNC_EXTERN DB_STATUS	dmf_sxf_terminate(DB_ERROR *dberr);


FUNC_EXTERN DB_STATUS dmf_term(
    DB_ERROR	*dberr);

FUNC_EXTERN STATUS dmf_setup_directio(void);
