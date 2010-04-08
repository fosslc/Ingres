/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <cv.h>
#include    <qu.h>
#include    <me.h>
#include    <st.h>
#include    <tm.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <dmrcb.h>
#include    <dmacb.h>
#include    <ulf.h>
#include    <qsf.h>
#include    <ulm.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <sxf.h>
#include    <qefmain.h>
#include    <qefqeu.h>
#include    <psfparse.h>
#include    <psfindep.h>
#include    <pshparse.h>
#include    <usererror.h>
#include    <psyaudit.h>
#include    <scf.h>
#include    <dudbms.h>

/**
**
**  Name: PSYLOC.C - Functions to manipulate the iilocations catalog.
**
**  Description:
**      This file contains functions necessary to create, alter and drop
**	locations:
**
**          psy_loc   - Routes requests to manipulate iilocations
**          psy_cloc  - Creates locations
**          psy_aloc  - Changes locations' usages
**          psy_kloc  - Drops locations
**
**
**  History:
**	04-sep-89 (ralph)
**	    written
**	30-oct-89 (ralph)
**	    call qeu_audit() only if failure due to authorization check
**      11-apr-90 (jennifer)
**          Bug 20457 - the object names for the qeu_audit message was too
**          long.  Use DB_NAME instead of DB_AREANAME.
**	12-feb-91 (ralph)
**	    Fold location name to lower case.
**	    Correct potential string handling bugs.
**	14-jan-92 (barbara)
**	    Included ddb.h for Star.
**	03-aug-92 (barbara)
**	    Call pst_rdfcb_init to initialize RDF_CB before calling RDF.
**      2-oct-1992 (ed)
**          Use DB_MAXNAME to replace hard coded numbers
**          - also created defines to replace hard coded character strings
**          dependent on DB_MAXNAME
**	06-nov-1992 (rog)
**	    <ulf.h> must be included before <qsf.h>.
**	04-dec-1992 (pholman)
**	    C2: replace obsolete use of qeu_audit with psy_secaudit.
**	29-jun-93 (andre)
**	    #included CV.H
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	10-aug-93 (andre)
**	    removed cause of a compiler warning
**	18-nov-93 (stephenb)
**	    Include psyaudit.h for prototyping.
**      28-apr-95 (harpa06) bug #68214
**          changed psy_cloc, psy_kloc, and psy_aloc to convert location names
**          into their FIPS equivalents before storing them in iilocations if
**          using a FIPS installation.
**	17-nov-95 (pchang)
**	    Backed out some of the changes by harpa06 28/4/95 and ralph 12/2/91.
**	    All case conversion to location names should be handled within the 
**	    parser for the SQL grammar.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	10-Jan-2001 (jenjo02)
**	    Supply session id to SCF instead of DB_NOSESSION.
**	17-Jan-2001 (jenjo02)
**	    Short-circuit calls to psy_secaudit() if not C2SECURE.
**	07-mar-2002 (somsa01)
**	    Changed STzapblank() to STtrmwhite() in psy_cloc() to allow
**	    location areas to have embedded spaces.
**      24-aug-2009 (stial01)
**          psy_cloc() Use DB_MAXNAME stack buffer for location name
**/

/*
**  Definition of static variables and forward static functions.
*/


/*{
** Name: psy_loc - Dispatch location qrymod routines
**
**  INTERNAL PSF call format: status = psy_loc(&psy_cb, sess_cb);
**  EXTERNAL     call format: status = psy_call (PSY_LOC,&psy_cb, sess_cb);
**
** Description:
**	This procedure checks the psy_loflag field of the PSY_CB
**	to determine which qrymod processing routine to call:
**		PSY_CLOC results in call to psy_cloc()
**		PSY_ALOC results in call to psy_aloc()
**		PSY_KLOC results in call to psy_kloc()
**
**	This procedure is called for SQL language only.
**
** Inputs:
**      psy_cb
**	    .psy_loflag			location operation
**	sess_cb				Pointer to session control block
**					(Can be NULL)
**
** Outputs:
**      psy_cb
**	    .psy_error			Filled in if error happens
**	Returns:
**	    E_DB_OK			Function completed normally.
**	    E_DB_WARN			Function completed with warning(s);
**	    E_DB_ERROR			Function failed; non-catastrophic error
**	    E_DB_SEVERE			Function failed; catastrophic error
**	Exceptions:
**	    none
**
** Side Effects:
**	    None.
**
** History:
**	04-sep-89 (ralph)
**          written
**	30-oct-89 (ralph)
**	    call qeu_audit() only if failure due to authorization check
**      11-apr-90 (jennifer)
**          Bug 20457 - the object names for the qeu_audit message was too
**          long.  Use DB_NAME instead of DB_AREANAME.
**	17-jun-93 (andre)
**	    changed interface of psy_secaudit() to accept PSS_SESBLK
*/
DB_STATUS
psy_loc(
	PSY_CB             *psy_cb,
	PSS_SESBLK	   *sess_cb)
{
    DB_STATUS		status, local_status;
    DB_ERROR		e_error;
    i4		err_code;
    char		dbname[DB_MAXNAME];
    SCF_CB		scf_cb;
    SCF_SCI		sci_list[1];

    i4			access = 0;    		        
    i4		msgid;
    PSY_TBL		*psy_tbl = (PSY_TBL *)psy_cb->psy_tblq.q_next;
    bool		leave_loop = TRUE;

    /* This code is called for SQL only */

    /* Make sure session is authorized and connected to iidbdb */

    do
    {
	/* Ensure we're authorized to CREATE/ALTER/DROP LOCATION */
	
	if (!(sess_cb->pss_ustat & DU_USYSADMIN))
	{
	    /* Session is not authorized */
	    if ( Psf_srvblk->psf_capabilities & PSF_C_C2SECURE )
	    {
		if ((psy_cb->psy_loflag & (PSY_CLOC | PSY_ALOC | PSY_KLOC))
					== PSY_CLOC)
		{	
		    access = SXF_A_FAIL | SXF_A_CREATE;
		    msgid = I_SX200B_LOCATION_CREATE;
		}
		else if ((psy_cb->psy_loflag & (PSY_CLOC | PSY_ALOC | PSY_KLOC))
					== PSY_ALOC)
		{
		    access = SXF_A_FAIL | SXF_A_ALTER;
		    msgid = I_SX202B_LOCATION_ALTER;
		}
		else if ((psy_cb->psy_loflag & (PSY_CLOC | PSY_ALOC | PSY_KLOC))
					== PSY_KLOC)
		{
		    access = SXF_A_FAIL | SXF_A_DROP;
		    msgid = I_SX202C_LOCATION_DROP;
		}	    
		local_status = psy_secaudit(FALSE, sess_cb,
			    (char *)&psy_tbl->psy_tabnm,
			    (DB_OWN_NAME *)NULL,
			    sizeof(DB_NAME), SXF_E_LOCATION,
			    msgid, access, 
			    &e_error);
	    }

	    err_code = E_US18D3_6355_NOT_AUTH;
	    (VOID) psf_error(E_US18D3_6355_NOT_AUTH, 0L,
			     PSF_USERERR, &err_code, &psy_cb->psy_error, 1,
			     sizeof("CREATE/ALTER/DROP LOCATION")-1,
			     "CREATE/ALTER/DROP LOCATION");
	    status   = E_DB_ERROR;
	    break;
	}

	/* Ensure we're connected to the iidbdb database */

	scf_cb.scf_length	= sizeof (SCF_CB);
	scf_cb.scf_type	= SCF_CB_TYPE;
	scf_cb.scf_facility	= DB_PSF_ID;
        scf_cb.scf_session	= sess_cb->pss_sessid;
        scf_cb.scf_ptr_union.scf_sci     = (SCI_LIST *) sci_list;
        scf_cb.scf_len_union.scf_ilength = 1;
        sci_list[0].sci_length  = sizeof(dbname);
        sci_list[0].sci_code    = SCI_DBNAME;
        sci_list[0].sci_aresult = (char *) dbname;
        sci_list[0].sci_rlength = NULL;

        status = scf_call(SCU_INFORMATION, &scf_cb);
        if (status != E_DB_OK)
        {
	    err_code = scf_cb.scf_error.err_code;
	    (VOID) psf_error(E_PS0D13_SCU_INFO_ERR, 0L,
			     PSF_INTERR, &err_code, &psy_cb->psy_error, 0);
	    status = (status > E_DB_SEVERE) ? status : E_DB_SEVERE;
	    break;
        }

	if (MEcmp((PTR)dbname, (PTR)DB_DBDB_NAME, sizeof(dbname)))
	{
	    /* Session not connected to iidbdb */
	    err_code = E_US18D4_6356_NOT_DBDB;
	    (VOID) psf_error(E_US18D4_6356_NOT_DBDB, 0L,
			     PSF_USERERR, &err_code, &psy_cb->psy_error, 1,
			     sizeof("CREATE/ALTER/DROP LOCATION")-1,
			     "CREATE/ALTER/DROP LOCATION");
	    status   = E_DB_ERROR;
	    break;
	}

	/* leave_loop has already been set to TRUE */
    } while (!leave_loop);

    /* Select the proper routine to process this request */

    if (status == E_DB_OK)
    switch (psy_cb->psy_loflag & (PSY_CLOC | PSY_ALOC | PSY_KLOC))
    {
	case PSY_CLOC:
	    status = psy_cloc(psy_cb, sess_cb);
	    break;

	case PSY_ALOC:
	    status = psy_aloc(psy_cb, sess_cb);
	    break;

	case PSY_KLOC:
	    status = psy_kloc(psy_cb, sess_cb);
	    break;

	default:
	    status = E_DB_SEVERE;
	    err_code = E_PS0D47_INVALID_LOC_OP;
	    (VOID) psf_error(E_PS0D47_INVALID_LOC_OP, 0L,
		    PSF_INTERR, &err_code, &psy_cb->psy_error, 0);
	    break;
    }

    return    (status);
}

/*{
** Name: psy_cloc - Create location
**
**  INTERNAL PSF call format: status = psy_cloc(&psy_cb, sess_cb);
**
** Description:
**	This procedure creates a location.  If the
**	location already exists, the statement is aborted.
**	If the location does not exist, it is created.
**	This procedure is called for SQL language only.
**
** Inputs:
**      psy_cb
**	    .psy_tblq			location list
**	sess_cb				Pointer to session control block
**					(Can be NULL)
**
** Outputs:
**      psy_cb
**	    .psy_error			Filled in if error happens
**	Returns:
**	    E_DB_OK			Function completed normally.
**	    E_DB_WARN			Function completed with warning(s);
**	    E_DB_ERROR			Function failed; non-catastrophic error
**	    E_DB_SEVERE			Function failed; catastrophic error
**	Exceptions:
**	    none
**
** Side Effects:
**	    Stores tuples in iilocations.
**
** History:
**	04-sep-89 (ralph)
**          written
**	12-mar-90 (andre)
**	    set rdr_2types_mask to 0.
**      22-may-90 (teg)
**          init rdr_instr to RDF_NO_INSTR
**	12-feb-91 (ralph)
**	    Fold location name to lower case.
**	    Correct potential string handling bugs.
**      28-apr-95 (harpa06) bug #68214
**          Changed to convert location names into their FIPS equivalents
**          before storing them in iilocations if using a FIPS installation.
**	17-nov-95 (pchang)
**	    Backed out some of the changes by harpa06 28/4/95 and ralph 12/2/91.
**	    All case conversion to location names should be handled within the 
**	    parser for the SQL grammar.
**	11-mar-99 (nanpr01)
**	    Initialize the new fields in iilocation catalog for raw locations.
**	02-Apr-2001 (jenjo02)
**	    Initialize du_raw_blocks from psy_loc_rawblocks.
**	11-May-2001 (jenjo02)
**	    rawblocks changed to rawpct.
**	07-mar-2002 (somsa01)
**	    Changed STzapblank() to STtrmwhite() to allow location areas to
**	    have embedded spaces.
*/
DB_STATUS
psy_cloc(
	PSY_CB             *psy_cb,
	PSS_SESBLK	   *sess_cb)
{
    DB_STATUS		status, stat;
    RDF_CB		rdf_cb;
    i4		err_code;
    register RDR_RB	*rdf_rb = &rdf_cb.rdf_rb;
    DU_LOCATIONS	lotuple;
    register DU_LOCATIONS   *lotup = &lotuple;
    PSY_TBL		*psy_obj;

    char		area_name[DB_AREA_MAX+1];
    char		loc_name[DB_MAXNAME+1];

    /* This code is called for SQL only */

    /*
    ** Fill in the part of RDF request block that will be constant.
    */
    pst_rdfcb_init(&rdf_cb, sess_cb);
    rdf_rb->rdr_update_op   = RDR_APPEND;
    rdf_rb->rdr_status	    = DB_SQL;
    rdf_rb->rdr_types_mask  = RDR_LOCATION;
    rdf_rb->rdr_qrytuple    = (PTR) lotup;
    rdf_rb->rdr_qtuple_count = 1;

    if (psy_cb->psy_loflag & PSY_LOCUSAGE)
	lotup->du_status = (i4) psy_cb->psy_lousage;
    else
	lotup->du_status = 0;

    lotup->du_raw_pct = psy_cb->psy_loc_rawpct;

    if (psy_cb->psy_loflag & PSY_LOCAREA)
    {
	MEmove(	sizeof(psy_cb->psy_area), (PTR)&psy_cb->psy_area,
		'\0', sizeof(area_name), (PTR)area_name);
	lotup->du_l_area = STtrmwhite(area_name);

	MEmove(	lotup->du_l_area, (PTR)area_name,
		' ', sizeof(lotup->du_area), (PTR) lotup->du_area);
    }
    else
    {
	MEfill(sizeof(lotup->du_area),
	       (u_char)' ',
	       (PTR) lotup->du_area);
	lotup->du_l_area = 0;
    }

    status = E_DB_OK;

    for (psy_obj  = (PSY_TBL *)  psy_cb->psy_tblq.q_next;
	 psy_obj != (PSY_TBL *) &psy_cb->psy_tblq;
	 psy_obj  = (PSY_TBL *)  psy_obj->queue.q_next
	)
    {
	MEmove(	sizeof(psy_obj->psy_tabnm), (PTR)&psy_obj->psy_tabnm,
		'\0', sizeof(loc_name), (PTR)loc_name);

	lotup->du_l_lname = STtrmwhite(loc_name);

	MEmove(	lotup->du_l_lname, (PTR)loc_name,
		' ', sizeof(lotup->du_lname), (PTR) lotup->du_lname);

	stat = rdf_call(RDF_UPDATE, (PTR) &rdf_cb);
	status = (stat > status) ? stat : status;

	if (DB_FAILURE_MACRO(stat))
	    break;
    }

    if (DB_FAILURE_MACRO(status))
	(VOID) psf_rdf_error(RDF_UPDATE, &rdf_cb.rdf_error, 
	    &psy_cb->psy_error);

    return    (status);
}

/*{
** Name: psy_aloc - Alter location
**
**  INTERNAL PSF call format: status = psy_aloc(&psy_cb, sess_cb);
**
** Description:
**	This procedure alters a location.  If the
**	location does not exist, the statement is aborted.
**	If the location does exist, the associated
**	iilocation tuple is replaced.
**	This procedure is called for SQL language only.
**
** Inputs:
**      psy_cb
**	    .psy_tblq			location list
**	sess_cb				Pointer to session control block
**					(Can be NULL)
**
** Outputs:
**      psy_cb
**	    .psy_error			Filled in if error happens
**	Returns:
**	    E_DB_OK			Function completed normally.
**	    E_DB_WARN			Function completed with warning(s);
**	    E_DB_ERROR			Function failed; non-catastrophic error
**	    E_DB_SEVERE			Function failed; catastrophic error
**	Exceptions:
**	    none
**
** Side Effects:
**	    Replaces tuples in iilocations.
**
** History:
**	04-sep-89 (ralph)
**          written
**	12-mar-90 (andre)
**	    set rdr_2types_mask to 0.
**      22-may-90 (teg)
**          init rdr_instr to RDF_NO_INSTR
**	12-feb-91 (ralph)
**	    Fold location name to lower case.
**	    Correct potential string handling bugs.
**      28-apr-95 (harpa06) bug #68214
**          Changed to convert location names into their FIPS equivalents 
**	    before storing them in iilocations if using a FIPS installation.
**	17-nov-95 (pchang)
**	    Backed out some of the changes by harpa06 28/4/95 and ralph 12/2/91.
**	    All case conversion to location names should be handled within the 
**	    parser for the SQL grammar.
*/
DB_STATUS
psy_aloc(
	PSY_CB             *psy_cb,
	PSS_SESBLK	   *sess_cb)
{
    DB_STATUS		status, stat;
    RDF_CB		rdf_cb;
    i4		err_code;
    register RDR_RB	*rdf_rb = &rdf_cb.rdf_rb;
    DU_LOCATIONS	lotuple;
    register DU_LOCATIONS   *lotup = &lotuple;
    PSY_TBL		*psy_obj;
    char		tempstr[DB_MAXNAME+1];

    /* This code is called for SQL only */

    /*
    ** Fill in the part of RDF request block that will be constant.
    */
    pst_rdfcb_init(&rdf_cb, sess_cb);
    rdf_rb->rdr_update_op   = RDR_REPLACE;
    rdf_rb->rdr_status	    = DB_SQL;
    rdf_rb->rdr_types_mask  = RDR_LOCATION;
    rdf_rb->rdr_qrytuple    = (PTR) lotup;
    rdf_rb->rdr_qtuple_count = 1;

    lotup->du_status = (i4) psy_cb->psy_lousage;

    MEfill(sizeof(lotup->du_area),
	   (u_char)' ',
	   (PTR) lotup->du_area);

    lotup->du_l_area = 0;

    status = E_DB_OK;

    for (psy_obj  = (PSY_TBL *)  psy_cb->psy_tblq.q_next;
	 psy_obj != (PSY_TBL *) &psy_cb->psy_tblq;
	 psy_obj  = (PSY_TBL *)  psy_obj->queue.q_next
	)
    {
	MEmove(	sizeof(psy_obj->psy_tabnm), (PTR)&psy_obj->psy_tabnm,
		'\0', sizeof(tempstr), (PTR)tempstr);

	lotup->du_l_lname = STtrmwhite(tempstr);

	MEmove(	lotup->du_l_lname, (PTR)tempstr,
		' ', sizeof(lotup->du_lname), (PTR) lotup->du_lname);

	stat = rdf_call(RDF_UPDATE, (PTR) &rdf_cb);
	status = (stat > status) ? stat : status;

	if (DB_FAILURE_MACRO(stat))
	    break;
    }

    if (DB_FAILURE_MACRO(status))
	(VOID) psf_rdf_error(RDF_UPDATE, &rdf_cb.rdf_error, 
	    &psy_cb->psy_error);

    return    (status);
} 

/*{
** Name: psy_kloc - Destroy location
**
**  INTERNAL PSF call format: status = psy_kloc(&psy_cb, sess_cb);
**
** Description:
**	This procedure deletes a location.  If the
**	location does not exist, the statement is aborted.
**	If the location does exist, the associated
**	iilocations tuple is deleted.
**	This procedure is called for SQL language only.
**
** Inputs:
**      psy_cb
**	    .psy_tblq			location list
**	sess_cb				Pointer to session control block
**					(Can be NULL)
**
** Outputs:
**      psy_cb
**	    .psy_error			Filled in if error happens
**	Returns:
**	    E_DB_OK			Function completed normally.
**	    E_DB_INFO			Function completed with warning(s).
**	    E_DB_WARN			One ore more roles were rejected.
**	    E_DB_ERROR			Function failed; non-catastrophic error
**	Exceptions:
**	    none
**
** Side Effects:
**	    Removed tuples from iilocations.
**
** History:
**	04-sep-89 (ralph)
**          written
**	12-mar-90 (andre)
**	    set rdr_2types_mask to 0.
**      22-may-90 (teg)
**          init rdr_instr to RDF_NO_INSTR
**	12-feb-91 (ralph)
**	    Fold location name to lower case.
**	    Correct potential string handling bugs.
**      28-apr-95 (harpa06) bug #68214
**          Changed to convert location names into their FIPS equivalents
**          before storing them in iilocations if using a FIPS installation.
**	17-nov-95 (pchang)
**	    Backed out some of the changes by harpa06 28/4/95 and ralph 12/2/91.
**	    All case conversion to location names should be handled within the 
**	    parser for the SQL grammar.
*/
DB_STATUS
psy_kloc(
	PSY_CB             *psy_cb,
	PSS_SESBLK	   *sess_cb)
{
    DB_STATUS		status, stat;
    RDF_CB		rdf_cb;
    register RDR_RB	*rdf_rb = &rdf_cb.rdf_rb;
    DU_LOCATIONS	lotuple;
    register DU_LOCATIONS   *lotup = &lotuple;
    PSY_TBL		*psy_obj;
    char		tempstr[DB_MAXNAME+1];

    /* This code is called for SQL only */

    /*
    ** Fill in the part of RDF request block that will be constant.
    */
    pst_rdfcb_init(&rdf_cb, sess_cb);
    rdf_rb->rdr_update_op   = RDR_DELETE;
    rdf_rb->rdr_status	    = DB_SQL;
    rdf_rb->rdr_types_mask  = RDR_LOCATION;
    rdf_rb->rdr_qrytuple    = (PTR) lotup;
    rdf_rb->rdr_qtuple_count = 1;

    lotup->du_status = (i4) 0;

    MEfill(sizeof(lotup->du_area),
	   (u_char)' ',
	   (PTR) lotup->du_area);

    lotup->du_l_area = 0;

    status = E_DB_OK;

    for (psy_obj  = (PSY_TBL *)  psy_cb->psy_tblq.q_next;
	 psy_obj != (PSY_TBL *) &psy_cb->psy_tblq;
	 psy_obj  = (PSY_TBL *)  psy_obj->queue.q_next
	)
    {
	MEmove(	sizeof(psy_obj->psy_tabnm), (PTR)&psy_obj->psy_tabnm,
		'\0', sizeof(tempstr), (PTR)tempstr);

	lotup->du_l_lname = STtrmwhite(tempstr);

	MEmove(	lotup->du_l_lname, (PTR)tempstr,
		' ', sizeof(lotup->du_lname), (PTR) lotup->du_lname);

	stat = rdf_call(RDF_UPDATE, (PTR) &rdf_cb);
	status = (stat > status) ? stat : status;

	if (DB_FAILURE_MACRO(stat))
	    break;
    }

    if (DB_FAILURE_MACRO(status))
	(VOID) psf_rdf_error(RDF_UPDATE, &rdf_cb.rdf_error, 
			     &psy_cb->psy_error);

    return    (status);
} 
