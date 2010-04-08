/*
** Copyright (c) 2004 Ingres Corporation
*/
#include    <compat.h>
#include    <gl.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <qu.h>
#include    <cs.h>
#include    <mu.h>
#include    <lo.h>
#include    <si.h>
#include    <st.h>
#include    <tr.h>
#include    <nm.h>
#include    <gca.h>
#include    <gc.h>
#include    <gcaint.h>
#include    <gcr.h>

/**
**
**  Name: gcasave.c - Contains the function gca_save.
**
**  Description:
**        
**      gcasave.c contains the following functions: 
**
**          gca_save() - save current context prior to process switching
**
**
**  History:
**      23-Apr-87 (jbowers)
**          Initial module implementation
**      30-Sept-87 (jbowers)
**          Changed to remove specific save mechanism down to GCsave in CL.
**	13-Jun-89 (neil)
**	    Do not assume that only first session is saved.
**	21-Sep-89 (seiwald)
**	    Removed call to gca_find_acb().  IIGCa_call() now fetches 
**	    ACB's for all services.
**	25-Oct-89 (seiwald)
**	    Shortened gcainternal.h to gcaint.h.
**	14-Feb-90 (seiwald)
**	    Use acb from svc_parms.
**	31-Dec-90 (seiwald)
**	    Bulk of GCA_SAVE and GCA_RESTORE moved into mainline from CL, 
**	    using a new version independent GCA_SAVE_DATA structure.
**	    Previously, the CL (GCsave/GCrestore) was responsible for
**	    transporting the user, GCA, and CL level data between parent 
**	    and child.  Now GCA handles that, using the old save file 
**	    mechanism but with a version independent format.  GCsave and 
**	    GCrestore simply format and interpret the CL level data.
**	11-Mar-91 (seiwald)
**	    Included all necessary CL headers as per PC group.
**	10-Apr-91 (seiwald)
**	    Use SIfopen() rather than the obsolescent SIopen().
**	17-aug-91 (leighb) DeskTop Porting Change:
**	    GCsave must be declared VOID, not STATUS.
**	07-Jan-93 (edg)
**	    Removed FUNC_EXTERN's (now in gcaint.h) and #include gcagref.h.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
**      12-jun-1995 (canor01)
**          semaphore protect NMloc()'s static buffer (MCT)
**	27-jun-95 (emmag)
**	    Cleaned up a typo in the previous submission.
**	10-Oct-95 (gordy)
**	    Completion moved to gca_service().
**	 3-Nov-95 (gordy)
**	    Removed MCT since GCA_SAVE not done in threaded environment.
**	20-Nov-95 (gordy)
**	    Added prototypes.
**	21-Nov-95 (gordy)
**	    Added gcr.h for embedded Comm Server configuration.
**	18-Feb-97 (gordy)
**	    Moved heterogeneous boolean to flags.
**	10-Apr-97 (gordy)
**	    GCA and GC service parameters separated.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/


/*{
** Name: gca_save()	- Save current GCA context and status
**
** Description:
**        
**      gca_save is invoked by the user in preparation for switching to a new 
**      process.  It allows GCA to save internal data structures and status  
**      that will be required to successfully continue uninterrupted in the  
**      new process.  In addition, a block of user-specified information may 
**      also be saved if the user so specifies. 
**        
**      The data are saved in a system-specific way by invoking GCsave();
**	The save mechanism used is a CL-level function.
**
** Inputs:
**      svc_parms                       Pointer to the collective parm list 
**                                      created by gca_call.  The following
**                                      elements of svc_parms are used by 
**                                      gca_save:
**
**	.gca_association_id		The association identifier returned by
**					at association initiation.  Identifies
**					association to be saved.
**
**      .parameter list                 Pointer to the invoker's service
**                                      parameter list.  The folloing 
**                                      elements of the parameter list are 
**                                      used by gca_save:
**
**      .gca_ptr_user_data              Pointer to a block of user data to be 
**                                      saved.
**      .gca_length_user_data           Length of the user's data block.
**
** Outputs:
**      .gca_save_name                  An identifier returned to the user to 
**                                      be used to identify the saved data for
**                                      subsequent restoration.
**      .gca_status			The element of .parameter_list in which
**                                      the result of the save operation is 
**                                      reported to the user.
**	    E_GC_0000_OK
**	    E_GC0014_SAVE_FAIL
**	    E_GC0005_INV_ASSOC_ID       Invalid association identifier
**
**	Returns:
**	    VOID
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      23-APR-87 (jbowers)
**          Initial function implementation
**	10-Oct-95 (gordy)
**	    Completion moved to gca_service().
**	18-Feb-97 (gordy)
**	    Moved heterogeneous boolean to flags.
**	10-Apr-97 (gordy)
**	    GCA and GC service parameters separated.
*/

VOID
gca_save( GCA_SVC_PARMS *svc_parms )
{
    GCA_ACB		*acb = svc_parms->acb;
    GCA_SV_PARMS        *sv_parms = (GCA_SV_PARMS *)svc_parms->parameter_list;
    GCA_SAVE_DATA	save_data;
    PTR			user_buf = sv_parms->gca_ptr_user_data;
    i4			user_size = sv_parms->gca_length_user_data;
    char		cl_buf[ GCA_MAX_CL_SAVE_SIZE ];
    i4			cl_size;
    LOCATION		file_loc;
    LOCATION		tmp_loc;
    char		loc_buf[ MAX_LOC ];
    FILE		*save_file;
    char		*save_ptr;
    char		file_id[ MAX_LOC ];
    i4			j;

    /* Initialize status in service invoker's parameter list  */

    sv_parms->gca_status = E_GCFFFF_IN_PROCESS;

    /* Get CL save info */

    svc_parms->gc_parms.svc_buffer = (PTR)cl_buf;
    svc_parms->gc_parms.reqd_amount = sizeof( cl_buf );
    GCsave( &svc_parms->gc_parms );

    if ( svc_parms->gc_parms.status != OK )
	goto complete;

    cl_size = svc_parms->gc_parms.rcv_data_length;

    /* Format GCA_SAVE_DATA structure */

    save_data.save_level_major = GCA_SAVE_LEVEL_MAJOR;
    save_data.save_level_minor = GCA_SAVE_LEVEL_MINOR;
    save_data.cl_save_size = cl_size;
    save_data.user_save_size = user_size;
    save_data.assoc_id = acb->assoc_id;
    save_data.size_advise = acb->size_advise;
    save_data.heterogeneous = acb->flags.heterogeneous;

    /* Create the save file name.  */

    file_id[0] = '\0';

    (VOID) LOfroms(PATH & FILENAME, file_id, &tmp_loc);
    (VOID) LOuniq("IIGC", "TMP", &tmp_loc);
    (VOID) NMloc(TEMP, FILENAME, file_id, &tmp_loc);
    (VOID) LOcopy( &tmp_loc, loc_buf, &file_loc );
    (VOID) LOtos( &file_loc, &save_ptr );
    (VOID) STcopy( save_ptr, sv_parms->gca_save_name );

    /* Open save file. */

    if( SIfopen( &file_loc, "w", SI_VAR, 0, &save_file ) != OK )
    {
	GCA_DEBUG_MACRO(1)( "gca_send bad file open\n" );
	svc_parms->gc_parms.status = E_GC0014_SAVE_FAIL;
	goto complete;
    }

    /* Write GCA, CL, and user data to save file. */

    if( SIwrite( sizeof( save_data ), (PTR)&save_data, &j, save_file ) != OK 
     || cl_size && SIwrite( cl_size, (PTR)cl_buf, &j, save_file ) != OK 
     || user_size && SIwrite( user_size, user_buf, &j, save_file ) != OK
     || SIclose( save_file ) != OK )
    {
	GCA_DEBUG_MACRO(1)( "gca_send bad file write/close\n" );
	(VOID) SIclose( save_file );
	(VOID) LOdelete( &file_loc );
	svc_parms->gc_parms.status = E_GC0014_SAVE_FAIL;
    }

complete:

    return;
}
