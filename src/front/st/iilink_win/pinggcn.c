/*
** Copyright (c) 1997 Ingres Corporation all rights reserved
*/

# include <compat.h>
# include <gl.h>
# include <iicommon.h>
# include <lo.h>
# include <me.h>
# include <er.h>
# include <qu.h>
# include <gc.h>
# include <gcccl.h>
# include <gca.h>
# include <gcn.h>
# include <gcnint.h>
# include <erst.h>

/*
** Description:
**
** ping the name server to see if the installation is currently 
** running. 
**
** History:
**	10-02-97 (somsa01)
**	    File created.
**	28-sep-1999 (somsa01)
**	    Make sure we call GCA_TERMINATE on successful ping.
**	24-sep-2000 (mcgem01)
**		More nat to i4 changes.
*/
STATUS 
ping_iigcn( void )
{
	GCA_PARMLIST	parms;
	STATUS		status;
	CL_ERR_DESC	cl_err;
	i4   		assoc_no;
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

		IIGCa_call( GCA_TERMINATE, &parms,
			    (i4) GCA_SYNC_FLAG, /* Synchronous */
			    (PTR) 0,		/* async id */
			    (i4) -1,		/* no timeout */
			    &tmp_stat );

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
                      (i4) GCA_SYNC_FLAG,    /* Synchronous */
                      (PTR) 0,                /* async id */
                      (i4) -1,           /* no timeout */
                      &status);
	return( FAIL );
}
