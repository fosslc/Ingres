/*
**Copyright (c) 2004 Ingres Corporation
*/

/*
**
LIBRARY = IMPGWFLIBDATA
**
*/

# include    <compat.h>
# include    <gl.h>
# include    <sl.h>
# include    <iicommon.h>
# include    <dbdbms.h>
# include    <dudbms.h>
# include    <me.h>
# include    <st.h>
# include    <tm.h>
# include    <tr.h>
# include    <erglf.h>
# include    <sp.h>
# include    <mo.h>
# include    <cs.h>
# include    <adf.h>
# include    <scf.h>
# include    <ulm.h>
# include    <dmf.h>
# include    <dmrcb.h>
# include    <dmtcb.h>
# include    <dmucb.h>
# include    <sxf.h>

# include    <gwf.h>
# include    <gwfint.h>
# include    <gwftrace.h>
# include    "gwfsxa.h"

/*
** Name:	gwfsxadata.c
**
** Description:	Global data for gwfsxa facility.
**
** History:
**
**	23-sep-96 (canor01)
**	    Created.
**	29-Sep-2004 (drivi01)
**	    Added LIBRARY jam hint to put this file into IMPGWFLIBDATA
**	    in the Jamfile.
**      24-Feb-2004 (kodse01)
**          Removed gwxit.h inclusion which is not required.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
*/

/* gwfsxa.c */
GLOBALDEF CS_SEMAPHORE GWsxamsg_sem ZERO_FILL; /*Semaphore to protect msg list*/
GLOBALDEF CS_SEMAPHORE GWsxa_sem ZERO_FILL ; /* Semaphore to protect general increments*/
GLOBALDEF ULM_RCB gwsxa_ulm ZERO_FILL;
GLOBALDEF      GWSXA_XATT  Gwsxa_xatt_tidp = {{0, 0}, 0, 0, "tidp", "tidp"};
GLOBALDEF GWSXASTATS GWsxastats ZERO_FILL;
GLOBALDEF   char    *GWsxa_version = "GWSXA1.0";

/* gwfsxamo.c */
GLOBALDEF MO_CLASS_DEF SXAmo_stat_classes[] =
{
    {0, "exp.gwf.sxa.register_count",
	MO_SIZEOF_MEMBER(GWSXASTATS,sxa_num_register), MO_READ,
	0, CL_OFFSETOF(GWSXASTATS, sxa_num_register), 
	MOintget, MOnoset, (PTR)&(GWsxastats), MOcdata_index
    },
    {0, "exp.gwf.sxa.reg_fail_count",
	MO_SIZEOF_MEMBER(GWSXASTATS,sxa_num_reg_fail), MO_READ,
	0, CL_OFFSETOF(GWSXASTATS, sxa_num_reg_fail), 
	MOintget, MOnoset, (PTR)&(GWsxastats), MOcdata_index
    },
    {0, "exp.gwf.sxa.open_count",
	MO_SIZEOF_MEMBER(GWSXASTATS,sxa_num_open), MO_READ,
	0, CL_OFFSETOF(GWSXASTATS, sxa_num_open), 
	MOintget, MOnoset, (PTR)&(GWsxastats), MOcdata_index
    },
    {0, "exp.gwf.sxa.open_fail_count",
	MO_SIZEOF_MEMBER(GWSXASTATS,sxa_num_open_fail), MO_READ,
	0, CL_OFFSETOF(GWSXASTATS, sxa_num_open_fail), 
	MOintget, MOnoset, (PTR)&(GWsxastats), MOcdata_index
    },
    {0, "exp.gwf.sxa.close_count",
	MO_SIZEOF_MEMBER(GWSXASTATS,sxa_num_close), MO_READ,
	0, CL_OFFSETOF(GWSXASTATS, sxa_num_close), 
	MOintget, MOnoset, (PTR)&(GWsxastats), MOcdata_index
    },
    {0, "exp.gwf.sxa.close_fail_count",
	MO_SIZEOF_MEMBER(GWSXASTATS,sxa_num_close_fail), MO_READ,
	0, CL_OFFSETOF(GWSXASTATS, sxa_num_close_fail), 
	MOintget, MOnoset, (PTR)&(GWsxastats), MOcdata_index
    },
    {0, "exp.gwf.sxa.get_count",
	MO_SIZEOF_MEMBER(GWSXASTATS,sxa_num_get), MO_READ,
	0, CL_OFFSETOF(GWSXASTATS, sxa_num_get), 
	MOintget, MOnoset, (PTR)&(GWsxastats), MOcdata_index
    },
    {0, "exp.gwf.sxa.get_fail_count",
	MO_SIZEOF_MEMBER(GWSXASTATS,sxa_num_get_fail), MO_READ,
	0, CL_OFFSETOF(GWSXASTATS, sxa_num_get_fail), 
	MOintget, MOnoset, (PTR)&(GWsxastats), MOcdata_index
    },
    {0, "exp.gwf.sxa.msgid_fail_count",
	MO_SIZEOF_MEMBER(GWSXASTATS,sxa_num_msgid_fail), MO_READ,
	0, CL_OFFSETOF(GWSXASTATS, sxa_num_msgid_fail), 
	MOintget, MOnoset, (PTR)&(GWsxastats), MOcdata_index
    },
    {0, "exp.gwf.sxa.msgid_count",
	MO_SIZEOF_MEMBER(GWSXASTATS,sxa_num_msgid), MO_READ,
	0, CL_OFFSETOF(GWSXASTATS, sxa_num_msgid), 
	MOintget, MOnoset, (PTR)&(GWsxastats), MOcdata_index
    },
    {0, "exp.gwf.sxa.msgid_lookup_count",
	MO_SIZEOF_MEMBER(GWSXASTATS,sxa_num_msgid_lookup), MO_READ,
	0, CL_OFFSETOF(GWSXASTATS, sxa_num_msgid_lookup), 
	MOintget, MOnoset, (PTR)&(GWsxastats), MOcdata_index
    },
    {0, "exp.gwf.sxa.reg_index_try_count",
	MO_SIZEOF_MEMBER(GWSXASTATS,sxa_num_reg_index_tries), MO_READ,
	0, CL_OFFSETOF(GWSXASTATS, sxa_num_reg_index_tries), 
	MOintget, MOnoset, (PTR)&(GWsxastats), MOcdata_index
    },
    {0, "exp.gwf.sxa.update_attempts",
	MO_SIZEOF_MEMBER(GWSXASTATS,sxa_num_update_tries), MO_READ,
	0, CL_OFFSETOF(GWSXASTATS, sxa_num_update_tries), 
	MOintget, MOnoset, (PTR)&(GWsxastats), MOcdata_index
    },
    {0, "exp.gwf.sxa.key_position_tries",
	MO_SIZEOF_MEMBER(GWSXASTATS,sxa_num_position_keyed), MO_READ,
	0, CL_OFFSETOF(GWSXASTATS, sxa_num_position_keyed), 
	MOintget, MOnoset, (PTR)&(GWsxastats), MOcdata_index
    },
    {0, "exp.gwf.sxa.get_bytid",
	MO_SIZEOF_MEMBER(GWSXASTATS,sxa_num_tid_get), MO_READ,
	0, CL_OFFSETOF(GWSXASTATS, sxa_num_tid_get), 
	MOintget, MOnoset, (PTR)&(GWsxastats), MOcdata_index
    },
    {0, "exp.gwf.sxa.get_bytid_rescan",
	MO_SIZEOF_MEMBER(GWSXASTATS,sxa_num_tid_scanstart), MO_READ,
	0, CL_OFFSETOF(GWSXASTATS, sxa_num_tid_scanstart), 
	MOintget, MOnoset, (PTR)&(GWsxastats), MOcdata_index
    },
    { 0 }
};
