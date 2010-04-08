# include <compat.h>
# include <gl.h>
# include <sl.h>
# include <iicommon.h>
# include <me.h>
# include <lo.h>
# include <pc.h>
# include <si.h>
# include <cv.h>
# include <nm.h>
# include <er.h>
# include <qu.h>
# include <gc.h>
# include <gcccl.h>
# include <gca.h>
# include <gcn.h>
# include <gcnint.h>
# include <cs.h>
# include <cm.h>
# include <st.h>
# include <pm.h>
# include <pmutil.h>
# include <util.h>
# include <simacro.h>
# include <erst.h>
# include <ci.h>

/*
**  Name: pinggcn.c
**
**  Description:
**	This utility pings the Name Server to see if it is up.
**
**  History:
**	28-jan-1999 (somsa01)
**	    Converted into standalone executable.
**      16-Feb-2001 (zhahu02)
**          Modified for doing 25 patch on NT
*/

STATUS 
main( void )
{
	GCA_PARMLIST	parms;
	STATUS		status;
	CL_ERR_DESC	cl_err;
	long		assoc_no;
	STATUS		tmp_stat;
	char		gcn_id[ GCN_VAL_MAX_LEN ];

	MEfill( sizeof(parms), 0, (PTR) &parms );

	(void) IIGCa_call( GCA_INITIATE, &parms, GCA_SYNC_FLAG, 0,
		-1L, &status);
			
	if( status != OK || ( status = parms.gca_in_parms.gca_status ) != OK )
		return( FAIL );

	status = GCnsid( GC_FIND_NSID, gcn_id, GCN_VAL_MAX_LEN, &cl_err );

	if( status == OK )
	{
		/* make GCA_REQUEST call */
		MEfill( sizeof(parms), 0, (PTR) &parms);
		parms.gca_rq_parms.gca_partner_name = gcn_id;
		parms.gca_rq_parms.gca_modifiers = GCA_CS_BEDCHECK |
			GCA_NO_XLATE;
		(void) IIGCa_call( GCA_REQUEST, &parms, GCA_SYNC_FLAG, 
			0, GCN_RESTART_TIME, &status );
		if( status == OK )
			status = parms.gca_rq_parms.gca_status;

		/* make GCA_DISASSOC call */
		assoc_no = parms.gca_rq_parms.gca_assoc_id;
		MEfill( sizeof( parms ), 0, (PTR) &parms);
		parms.gca_da_parms.gca_association_id = assoc_no;
		(void) IIGCa_call( GCA_DISASSOC, &parms, GCA_SYNC_FLAG, 
			0, -1L, &tmp_stat );
		/* existing name servers answer OK, new ones NO_PEER */
		if( status == E_GC0000_OK || status == E_GC0032_NO_PEER )
			return( OK );
	}
 
        /*
        ** Terminate the connection because we will initiate it again on
        ** entry to this routine.
        */
        IIGCa_call(   GCA_TERMINATE,
                      &parms,
                      (int) GCA_SYNC_FLAG,    /* Synchronous */
                      (PTR) 0,                /* async id */
                      (long) -1,           /* no timeout */
                      &status);
	return( FAIL );
}
