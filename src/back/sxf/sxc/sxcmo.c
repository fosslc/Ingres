/*
**Copyright (c) 2004 Ingres Corporation
*/

# include    <compat.h>
# include    <gl.h>
# include    <cs.h>
# include    <di.h>
# include    <pc.h>
# include    <tm.h>
# include    <erglf.h>
# include    <sp.h>
# include    <mo.h>
# include    <sl.h>
# include    <iicommon.h>
# include    <dbdbms.h>
# include    <adf.h>
# include    <ulf.h>
# include    <ulm.h>
# include    <lk.h>
# include    <dmf.h>
# include    <dmrcb.h>
# include    <dmtcb.h>
# include    <dmucb.h>
# include    <sxf.h>
# include    <sxfint.h>

/*
** Name: SXCMO.C		- SXF Managed Objects
**
** Description:
**	This file contains the routines and data structures which define the
**	managed objects for SXF. Managed objects appear in the MIB, 
**	and allow monitoring, analysis, and control of SXF through the 
**	MO interface.
**
** History:
**	9-feb-94 (robf)
**	    Created
**	14-apr-94 (robf)
**          Added new queue stats
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

static MO_CLASS_DEF SXFmo_stat_classes[] =
{
    {0, "exp.sxf.audit.write_buffered",
	MO_SIZEOF_MEMBER(SXF_STATS,write_buff), MO_READ,
	0, CL_OFFSETOF(SXF_STATS, write_buff), 
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.sxf.audit.write_direct",
	MO_SIZEOF_MEMBER(SXF_STATS,write_direct), MO_READ,
	0, CL_OFFSETOF(SXF_STATS, write_direct), 
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.sxf.audit.write_full",
	MO_SIZEOF_MEMBER(SXF_STATS,write_full), MO_READ,
	0, CL_OFFSETOF(SXF_STATS, write_full), 
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.sxf.audit.read_buffered",
	MO_SIZEOF_MEMBER(SXF_STATS,read_buff), MO_READ,
	0, CL_OFFSETOF(SXF_STATS, read_buff), 
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.sxf.audit.read_direct",
	MO_SIZEOF_MEMBER(SXF_STATS,read_direct), MO_READ,
	0, CL_OFFSETOF(SXF_STATS, read_direct), 
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.sxf.audit.open_read",
	MO_SIZEOF_MEMBER(SXF_STATS,open_read), MO_READ,
	0, CL_OFFSETOF(SXF_STATS, open_read), 
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.sxf.audit.open_write",
	MO_SIZEOF_MEMBER(SXF_STATS,open_write), MO_READ,
	0, CL_OFFSETOF(SXF_STATS, open_write), 
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.sxf.audit.create_count",
	MO_SIZEOF_MEMBER(SXF_STATS,create_count), MO_READ,
	0, CL_OFFSETOF(SXF_STATS, create_count), 
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.sxf.audit.close_count",
	MO_SIZEOF_MEMBER(SXF_STATS,close_count), MO_READ,
	0, CL_OFFSETOF(SXF_STATS, close_count), 
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.sxf.audit.flush_count",
	MO_SIZEOF_MEMBER(SXF_STATS,flush_count), MO_READ,
	0, CL_OFFSETOF(SXF_STATS, flush_count), 
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.sxf.audit.flush_empty",
	MO_SIZEOF_MEMBER(SXF_STATS,flush_empty), MO_READ,
	0, CL_OFFSETOF(SXF_STATS, flush_empty), 
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.sxf.audit.flush_predone",
	MO_SIZEOF_MEMBER(SXF_STATS,flush_predone), MO_READ,
	0, CL_OFFSETOF(SXF_STATS, flush_predone), 
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.sxf.audit.logswitch_count",
	MO_SIZEOF_MEMBER(SXF_STATS,logswitch_count), MO_READ,
	0, CL_OFFSETOF(SXF_STATS, logswitch_count), 
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.sxf.audit.extend_count",
	MO_SIZEOF_MEMBER(SXF_STATS,extend_count), MO_READ,
	0, CL_OFFSETOF(SXF_STATS, extend_count), 
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.sxf.audit.audit_wakeup",
	MO_SIZEOF_MEMBER(SXF_STATS,audit_wakeup), MO_READ,
	0, CL_OFFSETOF(SXF_STATS, audit_wakeup), 
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.sxf.db_build",
	MO_SIZEOF_MEMBER(SXF_STATS,db_build), MO_READ,
	0, CL_OFFSETOF(SXF_STATS, db_build), 
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.sxf.db_purge",
	MO_SIZEOF_MEMBER(SXF_STATS,db_purge), MO_READ,
	0, CL_OFFSETOF(SXF_STATS, db_purge), 
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.sxf.db_purge_all",
	MO_SIZEOF_MEMBER(SXF_STATS,db_purge_all), MO_READ,
	0, CL_OFFSETOF(SXF_STATS, db_purge_all), 
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.sxf.mem_at_startup",
	MO_SIZEOF_MEMBER(SXF_STATS,mem_at_startup), MO_READ,
	0, CL_OFFSETOF(SXF_STATS, mem_at_startup), 
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.sxf.queue_flush",
	MO_SIZEOF_MEMBER(SXF_STATS,flush_queue), MO_READ,
	0, CL_OFFSETOF(SXF_STATS, flush_queue), 
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.sxf.queue_flush_empty",
	MO_SIZEOF_MEMBER(SXF_STATS,flush_qempty), MO_READ,
	0, CL_OFFSETOF(SXF_STATS, flush_qempty), 
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.sxf.queue_flush_all",
	MO_SIZEOF_MEMBER(SXF_STATS,flush_qall), MO_READ,
	0, CL_OFFSETOF(SXF_STATS, flush_qall), 
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.sxf.queue_flush_full",
	MO_SIZEOF_MEMBER(SXF_STATS,flush_qfull), MO_READ,
	0, CL_OFFSETOF(SXF_STATS, flush_qfull), 
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.sxf.queue_write",
	MO_SIZEOF_MEMBER(SXF_STATS,write_queue), MO_READ,
	0, CL_OFFSETOF(SXF_STATS, write_queue), 
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    {0, "exp.sxf.audit_writer_wakeup",
	MO_SIZEOF_MEMBER(SXF_STATS,audit_writer_wakeup), MO_READ,
	0, CL_OFFSETOF(SXF_STATS, audit_writer_wakeup), 
	MOintget, MOnoset, (PTR)0, MOcdata_index
    },
    { 0 }
};

/*
** sxc_mo_attach() - Attach to the MIB
**
** Inputs:
**		None
**
** Returns:
**		E_DB_OK    - Worked
**		E_DB_ERROR - Failure.
**
** History:
**	9-feb-94 (robf)
**          Created
*/
DB_STATUS
sxc_mo_attach(void)
{
    DB_STATUS	status=E_DB_OK;
    i4		i;
    SXF_STATS   *sptr= Sxf_svcb->sxf_stats;


    if( !sptr)
    {
	/*
	** No stats, so don't attach them
	*/
	return status;
    }
    /*
    ** Initialize data from sptr
    */
    for (i=0;
	 i< (sizeof(SXFmo_stat_classes) /sizeof(SXFmo_stat_classes[0]));
	 i++)
    {
	SXFmo_stat_classes[i].cdata = (PTR)sptr;
    }


    status = MOclassdef(MAXI2, SXFmo_stat_classes);

    if (status!=OK)
    {
	DB_STATUS local_err;
	/*
	**	Error attaching to MIB, so log this went wrong
	*/
        _VOID_ ule_format(E_SX10B9_BAD_MO_ATTACH, NULL, 
		ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 0);
	return E_DB_ERROR;
    }
    else
	 return E_DB_OK;
}
