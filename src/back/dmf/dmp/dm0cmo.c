/*
**Copyright (c) 2010 Ingres Corporation
*/
#include    <compat.h>
#include    <gl.h>
#include    <erglf.h>

#include    <bt.h>
#include    <cs.h>
#include    <cv.h>
#include    <di.h>
#include    <lo.h>
#include    <me.h>
#include    <pc.h>
#include    <tm.h>
#include    <mo.h>

#include    <sl.h>
#include    <st.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <dudbms.h>

#include    <lg.h>
#include    <lk.h>

#include    <adf.h>
#include    <ulf.h>

#include    <dmf.h>
#include    <dmccb.h>
#include    <dmrcb.h>
#include    <dmtcb.h>
#include    <dmxcb.h>

#include    <dm.h>

#include    <dml.h>
#include    <dmp.h>
#include    <dm0m.h>
#include    <dm0c.h>
#include    <scf.h>

/*
** Name: DM0CMO.C		- Database Managed Objects
**
** Description:
**
**	This file contains the routines and data structures which define
**	the managed objects for a Database config file.
**
**	dm0c_mo_attach(cnf)	Attach database objects
**	dm0c_mo_detach(dcb)	Detach database objects
**
**	Factotum thread:
**	dm0c_mo_refresh(dcb)	Refresh database objects in response to
**				IMADB execute procedure ima_db_refresh(dbid)
**
**	Much of this code relies on those parts of MO architecture
**	that mutexes MO stuff while MO things are happening, and that
**	databases are opened and closed while the dcb_mutex is held, though
**	race conditions are anticipated and hopefully caught.
**
**	Databases and objects are keyed on database id (dbid) 
**	rather than dbname+owner, it being less cumbersome.
**
** History:
**	08-Oct-2010 (jonj) SIR 124738
**	    Created
*/
static STATUS dm0cMOinit = -1;

static STATUS dm0c_mo_init( void );
static VOID dm0c_mo( DMP_DCB *dcb, DM0C_CNF *cnf );
static DB_STATUS dm0c_mo_refresh( SCF_FTX *ftx );

static MO_GET_METHOD DM0Cmo_dsc_status_get;
static MO_GET_METHOD DM0Cmo_dcb_status_get;
static MO_GET_METHOD DM0Cmo_dsc_unormalization_get;
static MO_GET_METHOD DM0Cmo_dsc_inconsistent_get;
static MO_GET_METHOD DM0Cmo_dsc_dbaccess_get;
static MO_GET_METHOD DM0Cmo_dsc_dbservice_get;
static MO_GET_METHOD DM0Cmo_ext_flags_get;
static MO_GET_METHOD DM0Cmo_la_get;
static MO_GET_METHOD DM0Cmo_ckp_date_get;
static MO_GET_METHOD DM0Cmo_ckp_mode_get;

static MO_SET_METHOD DM0Cmo_refresh_set;


/*
** Managed objects for a database:
**     dm0c_dsc_classes:  
**       - Database information from DM0C_DSC
**       - One record is returned.
*/
static char dsc_index_class[] = "exp.dmf.dm0c.dsc_index";

static MO_CLASS_DEF dm0c_dsc_classes[]=
{
     {MO_INDEX_CLASSID|MO_CDATA_INDEX, dsc_index_class,
         MO_SIZEOF_MEMBER(DM0C_MO_DSC,dsc.dsc_dbid),
	 MO_READ, 0,
         CL_OFFSETOF(DM0C_MO_DSC,dsc.dsc_dbid), MOintget, MOnoset,
	 (PTR)NULL, MOidata_index
     },
     /* Set method to trigger "refresh" (detach/attach) of database MO */
     {MO_CDATA_INDEX, "exp.dmf.dm0c.dm0c_refresh_config", 
         0, MO_READ |
	     MO_SERVER_WRITE|MO_SYSTEM_WRITE|MO_SECURITY_WRITE,
	 dsc_index_class,
	 0, MOzeroget, DM0Cmo_refresh_set,
	 0, MOidata_index
     },
     {MO_CDATA_INDEX, "exp.dmf.dm0c.dsc_cnf_version", 
	 MO_SIZEOF_MEMBER(DM0C_MO_DSC,dsc.dsc_cnf_version),
         MO_READ, dsc_index_class, 
	 CL_OFFSETOF(DM0C_MO_DSC,dsc.dsc_cnf_version),
         MOintget, MOnoset, (PTR)NULL, MOidata_index
     },
     {MO_CDATA_INDEX, "exp.dmf.dm0c.dsc_c_version", 
	 MO_SIZEOF_MEMBER(DM0C_MO_DSC,dsc.dsc_c_version),
         MO_READ, dsc_index_class, 
	 CL_OFFSETOF(DM0C_MO_DSC,dsc.dsc_c_version),
         MOintget, MOnoset, (PTR)NULL, MOidata_index
     },
     {MO_CDATA_INDEX, "exp.dmf.dm0c.dsc_status_num",
         MO_SIZEOF_MEMBER(DM0C_MO_DSC,dsc.dsc_status),
         MO_READ, dsc_index_class, 
	 CL_OFFSETOF(DM0C_MO_DSC,dsc.dsc_status),
         MOintget, MOnoset, (PTR)NULL, MOidata_index
     },
     {MO_CDATA_INDEX, "exp.dmf.dm0c.dsc_status",
         MO_SIZEOF_MEMBER(DM0C_MO_DSC,dsc.dsc_status),
         MO_READ, dsc_index_class,
	 CL_OFFSETOF(DM0C_MO_DSC,dsc.dsc_status),
         DM0Cmo_dsc_status_get, MOnoset, (PTR)NULL, MOidata_index
     },
     {MO_CDATA_INDEX, "exp.dmf.dm0c.dcb_status",
         MO_SIZEOF_MEMBER(DM0C_MO_DSC,dcb_ptr),
         MO_READ, dsc_index_class,
	 CL_OFFSETOF(DM0C_MO_DSC,dcb_ptr),
         DM0Cmo_dcb_status_get, MOnoset, (PTR)NULL, MOidata_index
     },
     {MO_CDATA_INDEX, "exp.dmf.dm0c.dsc_dbid",
         MO_SIZEOF_MEMBER(DM0C_MO_DSC,dsc.dsc_dbid),
         MO_READ, dsc_index_class,
	 CL_OFFSETOF(DM0C_MO_DSC,dsc.dsc_dbid),
         MOintget, MOnoset, (PTR)NULL, MOidata_index
     },
     {MO_CDATA_INDEX, "exp.dmf.dm0c.dsc_ext_count",
         MO_SIZEOF_MEMBER(DM0C_MO_DSC,dsc.dsc_ext_count),
         MO_READ, dsc_index_class,
	 CL_OFFSETOF(DM0C_MO_DSC,dsc.dsc_ext_count),
         MOintget, MOnoset, (PTR)NULL, MOidata_index
     },
     {MO_CDATA_INDEX, "exp.dmf.dm0c.dsc_last_table_id",
         MO_SIZEOF_MEMBER(DM0C_MO_DSC,dsc.dsc_last_table_id),
         MO_READ, dsc_index_class,
	 CL_OFFSETOF(DM0C_MO_DSC,dsc.dsc_last_table_id),
         MOuintget, MOnoset, (PTR)NULL, MOidata_index
     },
     {MO_CDATA_INDEX, "exp.dmf.dm0c.dsc_dbaccess_num",
         MO_SIZEOF_MEMBER(DM0C_MO_DSC,dsc.dsc_dbaccess),
         MO_READ, dsc_index_class,
	 CL_OFFSETOF(DM0C_MO_DSC,dsc.dsc_dbaccess),
         MOintget, MOnoset, (PTR)NULL, MOidata_index
     },
     {MO_CDATA_INDEX, "exp.dmf.dm0c.dsc_dbaccess",
         MO_SIZEOF_MEMBER(DM0C_MO_DSC,dsc.dsc_dbaccess),
         MO_READ, dsc_index_class,
	 CL_OFFSETOF(DM0C_MO_DSC,dsc.dsc_dbaccess),
         DM0Cmo_dsc_dbaccess_get, MOnoset, (PTR)NULL, MOidata_index
     },
     {MO_CDATA_INDEX, "exp.dmf.dm0c.dsc_dbservice_num",
         MO_SIZEOF_MEMBER(DM0C_MO_DSC,dsc.dsc_dbservice),
         MO_READ, dsc_index_class,
	 CL_OFFSETOF(DM0C_MO_DSC,dsc.dsc_dbservice),
         MOintget, MOnoset, (PTR)NULL, MOidata_index
     },
     {MO_CDATA_INDEX, "exp.dmf.dm0c.dsc_dbservice",
         MO_SIZEOF_MEMBER(DM0C_MO_DSC,dsc.dsc_dbservice),
         MO_READ, dsc_index_class,
	 CL_OFFSETOF(DM0C_MO_DSC,dsc.dsc_dbservice),
         DM0Cmo_dsc_dbservice_get, MOnoset, (PTR)NULL, MOidata_index
     },
     {MO_CDATA_INDEX, "exp.dmf.dm0c.dsc_inconsistent",
         MO_SIZEOF_MEMBER(DM0C_MO_DSC,dsc.dsc_inconst_code),
         MO_READ, dsc_index_class,
	 CL_OFFSETOF(DM0C_MO_DSC,dsc.dsc_inconst_code),
         DM0Cmo_dsc_inconsistent_get, MOnoset, (PTR)NULL, MOidata_index
     },
     {MO_CDATA_INDEX, "exp.dmf.dm0c.dsc_name",
         MO_SIZEOF_MEMBER(DM0C_MO_DSC,dsc.dsc_name),
         MO_READ, dsc_index_class,
	 CL_OFFSETOF(DM0C_MO_DSC,dsc.dsc_name),
         MOstrget, MOnoset, (PTR)NULL, MOidata_index
     },
     {MO_CDATA_INDEX, "exp.dmf.dm0c.dsc_owner",
         MO_SIZEOF_MEMBER(DM0C_MO_DSC,dsc.dsc_owner),
         MO_READ, dsc_index_class,
	 CL_OFFSETOF(DM0C_MO_DSC,dsc.dsc_owner),
         MOstrget, MOnoset, (PTR)NULL, MOidata_index
     },
     {MO_CDATA_INDEX, "exp.dmf.dm0c.dsc_collation",
         MO_SIZEOF_MEMBER(DM0C_MO_DSC,dsc.dsc_collation),
         MO_READ, dsc_index_class,
	 CL_OFFSETOF(DM0C_MO_DSC,dsc.dsc_collation),
         MOstrget, MOnoset, (PTR)NULL, MOidata_index
     },
     {MO_CDATA_INDEX, "exp.dmf.dm0c.dsc_ucollation",
         MO_SIZEOF_MEMBER(DM0C_MO_DSC,dsc.dsc_ucollation),
         MO_READ, dsc_index_class,
	 CL_OFFSETOF(DM0C_MO_DSC,dsc.dsc_ucollation),
         MOstrget, MOnoset, (PTR)NULL, MOidata_index
     },
     {MO_CDATA_INDEX, "exp.dmf.dm0c.dsc_unormalization",
         MO_SIZEOF_MEMBER(DM0C_MO_DSC,dsc.dsc_dbservice),
         MO_READ, dsc_index_class,
	 CL_OFFSETOF(DM0C_MO_DSC,dsc.dsc_dbservice),
         DM0Cmo_dsc_unormalization_get, MOnoset, (PTR)NULL, MOidata_index
     },
     {MO_CDATA_INDEX, "exp.dmf.dm0c.jnl_count",
         MO_SIZEOF_MEMBER(DM0C_MO_DSC,jnl_count),
	 MO_READ, dsc_index_class,
	 CL_OFFSETOF(DM0C_MO_DSC,jnl_count),
	 MOintget, MOnoset, (PTR)NULL, MOidata_index
     },
     {MO_CDATA_INDEX, "exp.dmf.dm0c.jnl_node_count",
         MO_SIZEOF_MEMBER(DM0C_MO_DSC,jnl_node_count),
	 MO_READ, dsc_index_class,
	 CL_OFFSETOF(DM0C_MO_DSC,jnl_node_count),
	 MOintget, MOnoset, (PTR)NULL, MOidata_index
     },
     {MO_CDATA_INDEX, "exp.dmf.dm0c.jnl_ckp_seq",
         MO_SIZEOF_MEMBER(DM0C_MO_DSC,jnl_ckp_seq),
	 MO_READ, dsc_index_class,
	 CL_OFFSETOF(DM0C_MO_DSC,jnl_ckp_seq),
	 MOintget, MOnoset, (PTR)NULL, MOidata_index
     },
     {MO_CDATA_INDEX, "exp.dmf.dm0c.jnl_fil_seq",
         MO_SIZEOF_MEMBER(DM0C_MO_DSC,jnl_fil_seq),
	 MO_READ, dsc_index_class,
	 CL_OFFSETOF(DM0C_MO_DSC,jnl_fil_seq),
	 MOintget, MOnoset, (PTR)NULL, MOidata_index
     },
     {MO_CDATA_INDEX, "exp.dmf.dm0c.jnl_blk_seq",
         MO_SIZEOF_MEMBER(DM0C_MO_DSC,jnl_blk_seq),
	 MO_READ, dsc_index_class,
	 CL_OFFSETOF(DM0C_MO_DSC,jnl_blk_seq),
	 MOintget, MOnoset, (PTR)NULL, MOidata_index
     },
     {MO_CDATA_INDEX, "exp.dmf.dm0c.jnl_bksz",
         MO_SIZEOF_MEMBER(DM0C_MO_DSC,jnl_bksz),
	 MO_READ, dsc_index_class,
	 CL_OFFSETOF(DM0C_MO_DSC,jnl_bksz),
	 MOintget, MOnoset, (PTR)NULL, MOidata_index
     },
     {MO_CDATA_INDEX, "exp.dmf.dm0c.jnl_blkcnt",
         MO_SIZEOF_MEMBER(DM0C_MO_DSC,jnl_blkcnt),
	 MO_READ, dsc_index_class,
	 CL_OFFSETOF(DM0C_MO_DSC,jnl_blkcnt),
	 MOintget, MOnoset, (PTR)NULL, MOidata_index
     },
     {MO_CDATA_INDEX, "exp.dmf.dm0c.jnl_maxcnt",
         MO_SIZEOF_MEMBER(DM0C_MO_DSC,jnl_maxcnt),
	 MO_READ, dsc_index_class,
	 CL_OFFSETOF(DM0C_MO_DSC,jnl_maxcnt),
	 MOintget, MOnoset, (PTR)NULL, MOidata_index
     },
     {MO_CDATA_INDEX, "exp.dmf.dm0c.jnl_la",
         MO_SIZEOF_MEMBER(DM0C_MO_DSC,jnl_la),
	 MO_READ, dsc_index_class,
	 CL_OFFSETOF(DM0C_MO_DSC,jnl_la),
	 DM0Cmo_la_get, MOnoset, (PTR)NULL, MOidata_index
     },
     {MO_CDATA_INDEX, "exp.dmf.dm0c.jnl_la_seq",
         MO_SIZEOF_MEMBER(DM0C_MO_DSC,jnl_la.la_sequence),
	 MO_READ, dsc_index_class,
	 CL_OFFSETOF(DM0C_MO_DSC,jnl_la.la_sequence),
	 MOuintget, MOnoset, (PTR)NULL, MOidata_index
     },
     {MO_CDATA_INDEX, "exp.dmf.dm0c.jnl_la_block",
         MO_SIZEOF_MEMBER(DM0C_MO_DSC,jnl_la.la_block),
	 MO_READ, dsc_index_class,
	 CL_OFFSETOF(DM0C_MO_DSC,jnl_la.la_block),
	 MOuintget, MOnoset, (PTR)NULL, MOidata_index
     },
     {MO_CDATA_INDEX, "exp.dmf.dm0c.jnl_la_offset",
         MO_SIZEOF_MEMBER(DM0C_MO_DSC,jnl_la.la_offset),
	 MO_READ, dsc_index_class,
	 CL_OFFSETOF(DM0C_MO_DSC,jnl_la.la_offset),
	 MOuintget, MOnoset, (PTR)NULL, MOidata_index
     },
     {MO_CDATA_INDEX, "exp.dmf.dm0c.jnl_first_jnl_seq",
         MO_SIZEOF_MEMBER(DM0C_MO_DSC,jnl_first_jnl_seq),
	 MO_READ, dsc_index_class,
	 CL_OFFSETOF(DM0C_MO_DSC,jnl_first_jnl_seq),
	 MOintget, MOnoset, (PTR)NULL, MOidata_index
     },
     {MO_CDATA_INDEX, "exp.dmf.dm0c.dmp_count",
         MO_SIZEOF_MEMBER(DM0C_MO_DSC,dmp_count),
	 MO_READ, dsc_index_class, 
	 CL_OFFSETOF(DM0C_MO_DSC,dmp_count),
	 MOintget, MOnoset, (PTR)NULL, MOidata_index
     },
     {MO_CDATA_INDEX, "exp.dmf.dm0c.dmp_node_count",
         MO_SIZEOF_MEMBER(DM0C_MO_DSC,dmp_node_count),
	 MO_READ, dsc_index_class, 
	 CL_OFFSETOF(DM0C_MO_DSC,dmp_node_count),
	 MOintget, MOnoset, (PTR)NULL, MOidata_index
     },
     {MO_CDATA_INDEX, "exp.dmf.dm0c.dmp_ckp_seq",
         MO_SIZEOF_MEMBER(DM0C_MO_DSC,dmp_ckp_seq),
	 MO_READ, dsc_index_class, 
	 CL_OFFSETOF(DM0C_MO_DSC,dmp_ckp_seq),
	 MOintget, MOnoset, (PTR)NULL, MOidata_index
     },
     {MO_CDATA_INDEX, "exp.dmf.dm0c.dmp_fil_seq",
         MO_SIZEOF_MEMBER(DM0C_MO_DSC,dmp_fil_seq),
	 MO_READ, dsc_index_class, 
	 CL_OFFSETOF(DM0C_MO_DSC,dmp_fil_seq),
	 MOintget, MOnoset, (PTR)NULL, MOidata_index
     },
     {MO_CDATA_INDEX, "exp.dmf.dm0c.dmp_blk_seq",
         MO_SIZEOF_MEMBER(DM0C_MO_DSC,dmp_blk_seq),
	 MO_READ, dsc_index_class, 
	 CL_OFFSETOF(DM0C_MO_DSC,dmp_blk_seq),
	 MOintget, MOnoset, (PTR)NULL, MOidata_index
     },
     {MO_CDATA_INDEX, "exp.dmf.dm0c.dmp_bksz",
         MO_SIZEOF_MEMBER(DM0C_MO_DSC,dmp_bksz),
	 MO_READ, dsc_index_class, 
	 CL_OFFSETOF(DM0C_MO_DSC,dmp_bksz),
	 MOintget, MOnoset, (PTR)NULL, MOidata_index
     },
     {MO_CDATA_INDEX, "exp.dmf.dm0c.dmp_blkcnt",
         MO_SIZEOF_MEMBER(DM0C_MO_DSC,dmp_blkcnt),
	 MO_READ, dsc_index_class, 
	 CL_OFFSETOF(DM0C_MO_DSC,dmp_blkcnt),
	 MOintget, MOnoset, (PTR)NULL, MOidata_index
     },
     {MO_CDATA_INDEX, "exp.dmf.dm0c.dmp_maxcnt",
         MO_SIZEOF_MEMBER(DM0C_MO_DSC,dmp_maxcnt),
	 MO_READ, dsc_index_class, 
	 CL_OFFSETOF(DM0C_MO_DSC,dmp_maxcnt),
	 MOintget, MOnoset, (PTR)NULL, MOidata_index
     },
     {MO_CDATA_INDEX, "exp.dmf.dm0c.dmp_la",
         MO_SIZEOF_MEMBER(DM0C_MO_DSC,dmp_la),
	 MO_READ, dsc_index_class, 
	 CL_OFFSETOF(DM0C_MO_DSC,dmp_la),
	 DM0Cmo_la_get, MOnoset, (PTR)NULL, MOidata_index
     },
     {MO_CDATA_INDEX, "exp.dmf.dm0c.dmp_la_seq",
         MO_SIZEOF_MEMBER(DM0C_MO_DSC,dmp_la.la_sequence),
	 MO_READ, dsc_index_class, 
	 CL_OFFSETOF(DM0C_MO_DSC,dmp_la.la_sequence),
	 MOuintget, MOnoset, (PTR)NULL, MOidata_index
     },
     {MO_CDATA_INDEX, "exp.dmf.dm0c.dmp_la_block",
         MO_SIZEOF_MEMBER(DM0C_MO_DSC,dmp_la.la_block),
	 MO_READ, dsc_index_class, 
	 CL_OFFSETOF(DM0C_MO_DSC,dmp_la.la_block),
	 MOuintget, MOnoset, (PTR)NULL, MOidata_index
     },
     {MO_CDATA_INDEX, "exp.dmf.dm0c.dmp_la_offset",
         MO_SIZEOF_MEMBER(DM0C_MO_DSC,dmp_la.la_offset),
	 MO_READ, dsc_index_class, 
	 CL_OFFSETOF(DM0C_MO_DSC,dmp_la.la_offset),
	 MOuintget, MOnoset, (PTR)NULL, MOidata_index
     },
     { 0 }
 };

/*
** Managed objects for a database:
**     dm0c_ext_classes:  
**       - Extent information from DM0C_EXT
**       - One record is returned for each in dsc_ext_count.
*/
static char ext_index_class[] = "exp.dmf.dm0c.ext_index";

static MO_CLASS_DEF dm0c_ext_classes[]=
{
     {MO_INDEX_CLASSID|MO_CDATA_INDEX, ext_index_class,
         MO_SIZEOF_MEMBER(DM0C_MO_EXT,dbid),
	 MO_READ, 0,
         CL_OFFSETOF(DM0C_MO_EXT,dbid), MOintget, MOnoset,
	 (PTR)NULL, MOidata_index
     },
     {MO_CDATA_INDEX, "exp.dmf.dm0c.ext_flags_num",
         MO_SIZEOF_MEMBER(DM0C_MO_EXT,ext.flags),
	 MO_READ, ext_index_class,
	 CL_OFFSETOF(DM0C_MO_EXT,ext.flags),
	 MOintget, MOnoset, (PTR)NULL, MOidata_index
     },
     {MO_CDATA_INDEX, "exp.dmf.dm0c.ext_flags",
         MO_SIZEOF_MEMBER(DM0C_MO_EXT,ext.flags),
	 MO_READ, ext_index_class,
	 CL_OFFSETOF(DM0C_MO_EXT,ext.flags),
	 DM0Cmo_ext_flags_get, MOnoset, (PTR)NULL, MOidata_index
     },
     {MO_CDATA_INDEX, "exp.dmf.dm0c.ext_location",
         MO_SIZEOF_MEMBER(DM0C_MO_EXT,ext.logical),
	 MO_READ, ext_index_class,
         CL_OFFSETOF(DM0C_MO_EXT,ext.logical),
	 MOstrget, MOnoset, (PTR)NULL, MOidata_index
     },
     {MO_CDATA_INDEX, "exp.dmf.dm0c.ext_phys_length",
         MO_SIZEOF_MEMBER(DM0C_MO_EXT,ext.phys_length),
	 MO_READ, ext_index_class,
	 CL_OFFSETOF(DM0C_MO_EXT,ext.phys_length),
	 MOintget, MOnoset, (PTR)NULL, MOidata_index
     },
     {MO_CDATA_INDEX, "exp.dmf.dm0c.ext_path",
         MO_SIZEOF_MEMBER(DM0C_MO_EXT,ext.physical),
	 MO_READ, ext_index_class,
         CL_OFFSETOF(DM0C_MO_EXT,ext.physical),
	 MOstrget, MOnoset, (PTR)NULL, MOidata_index
     },
     { 0 }
 };

/*
** Managed objects for a database:
**     dm0c_jckp_classes:  
**       - Checkpoint history information from JNL_CKP
**       - One record is returned for each in jnl_count
*/
static char jckp_index_class[] = "exp.dmf.dm0c.jckp_index";

static MO_CLASS_DEF dm0c_jckp_classes[]=
{
     {MO_INDEX_CLASSID|MO_CDATA_INDEX, jckp_index_class,
         MO_SIZEOF_MEMBER(DM0C_MO_JCKP,dbid),
	 MO_READ, 0,
         CL_OFFSETOF(DM0C_MO_JCKP,dbid), MOintget, MOnoset,
	 (PTR)NULL, MOidata_index
     },
     {MO_CDATA_INDEX, "exp.dmf.dm0c.jckp_sequence",
         MO_SIZEOF_MEMBER(DM0C_MO_JCKP,jckp.ckp_sequence),
	 MO_READ, jckp_index_class,
	 CL_OFFSETOF(DM0C_MO_JCKP,jckp.ckp_sequence),
	 MOintget, MOnoset, (PTR)NULL, MOidata_index
     },
     {MO_CDATA_INDEX, "exp.dmf.dm0c.jckp_date",
         MO_SIZEOF_MEMBER(DM0C_MO_JCKP,jckp.ckp_date),
	 MO_READ, jckp_index_class,
	 CL_OFFSETOF(DM0C_MO_JCKP,jckp.ckp_date),
	 DM0Cmo_ckp_date_get, MOnoset, (PTR)NULL, MOidata_index
     },
     {MO_CDATA_INDEX, "exp.dmf.dm0c.jckp_f_jnl",
         MO_SIZEOF_MEMBER(DM0C_MO_JCKP,jckp.ckp_f_jnl),
	 MO_READ, jckp_index_class,
	 CL_OFFSETOF(DM0C_MO_JCKP,jckp.ckp_f_jnl),
	 MOintget, MOnoset, (PTR)NULL, MOidata_index
     },
     {MO_CDATA_INDEX, "exp.dmf.dm0c.jckp_l_jnl",
         MO_SIZEOF_MEMBER(DM0C_MO_JCKP,jckp.ckp_l_jnl),
	 MO_READ, jckp_index_class,
	 CL_OFFSETOF(DM0C_MO_JCKP,jckp.ckp_l_jnl),
	 MOintget, MOnoset, (PTR)NULL, MOidata_index
     },
     {MO_CDATA_INDEX, "exp.dmf.dm0c.jckp_jnl_valid",
         MO_SIZEOF_MEMBER(DM0C_MO_JCKP,jckp.ckp_jnl_valid),
	 MO_READ, jckp_index_class,
	 CL_OFFSETOF(DM0C_MO_JCKP,jckp.ckp_jnl_valid),
	 MOintget, MOnoset, (PTR)NULL, MOidata_index
     },
     {MO_CDATA_INDEX, "exp.dmf.dm0c.jckp_mode",
         MO_SIZEOF_MEMBER(DM0C_MO_JCKP,jckp.ckp_mode),
	 MO_READ, jckp_index_class,
	 CL_OFFSETOF(DM0C_MO_JCKP,jckp.ckp_mode),
	 DM0Cmo_ckp_mode_get, MOnoset, (PTR)NULL, MOidata_index
     },
     { 0 }
 };

/*
** Managed objects for a database:
**     dm0c_jnode_classes:  
**       - Node journal information from JNL_CNODE_INFO
**       - One record is returned for each in jnl_node_count
*/
static char jnode_index_class[] = "exp.dmf.dm0c.jnode_index";

static MO_CLASS_DEF dm0c_jnode_classes[]=
{
     {MO_INDEX_CLASSID|MO_CDATA_INDEX, jnode_index_class,
         MO_SIZEOF_MEMBER(DM0C_MO_JNODE,dbid),
	 MO_READ, 0,
         CL_OFFSETOF(DM0C_MO_JNODE,dbid), MOintget, MOnoset,
	 (PTR)NULL, MOidata_index
     },
     {MO_CDATA_INDEX, "exp.dmf.dm0c.jnode_fil_seq",
         MO_SIZEOF_MEMBER(DM0C_MO_JNODE,jnode.cnode_fil_seq),
	 MO_READ, jnode_index_class,
	 CL_OFFSETOF(DM0C_MO_JNODE,jnode.cnode_fil_seq),
	 MOintget, MOnoset, (PTR)NULL, MOidata_index
     },
     {MO_CDATA_INDEX, "exp.dmf.dm0c.jnode_blk_seq",
         MO_SIZEOF_MEMBER(DM0C_MO_JNODE,jnode.cnode_blk_seq),
	 MO_READ, jnode_index_class,
	 CL_OFFSETOF(DM0C_MO_JNODE,jnode.cnode_blk_seq),
	 MOintget, MOnoset, (PTR)NULL, MOidata_index
     },
     {MO_CDATA_INDEX, "exp.dmf.dm0c.jnode_la",
         MO_SIZEOF_MEMBER(DM0C_MO_JNODE,jnode.cnode_la),
	 MO_READ, jnode_index_class,
	 CL_OFFSETOF(DM0C_MO_JNODE,jnode.cnode_la),
	 DM0Cmo_la_get, MOnoset, (PTR)NULL, MOidata_index
     },
     { 0 }
 };

/*
** Managed objects for a database:
**     dm0c_dckp_classes:  
**       - Checkpoint history information from DMP_CKP
**       - One record is returned for each in dmp_count
*/
static char dckp_index_class[] = "exp.dmf.dm0c.dckp_index";

static MO_CLASS_DEF dm0c_dckp_classes[]=
{
     {MO_INDEX_CLASSID|MO_CDATA_INDEX, dckp_index_class,
         MO_SIZEOF_MEMBER(DM0C_MO_DCKP,dbid),
	 MO_READ, 0,
         CL_OFFSETOF(DM0C_MO_DCKP,dbid), MOintget, MOnoset,
	 (PTR)NULL, MOidata_index
     },
     {MO_CDATA_INDEX, "exp.dmf.dm0c.dckp_sequence",
         MO_SIZEOF_MEMBER(DM0C_MO_DCKP,dckp.ckp_sequence),
	 MO_READ, dckp_index_class,
	 CL_OFFSETOF(DM0C_MO_DCKP,dckp.ckp_sequence),
	 MOintget, MOnoset, (PTR)NULL, MOidata_index
     },
     {MO_CDATA_INDEX, "exp.dmf.dm0c.dckp_date",
         MO_SIZEOF_MEMBER(DM0C_MO_DCKP,dckp.ckp_date),
	 MO_READ, dckp_index_class,
	 CL_OFFSETOF(DM0C_MO_DCKP,dckp.ckp_date),
	 DM0Cmo_ckp_date_get, MOnoset, (PTR)NULL, MOidata_index
     },
     {MO_CDATA_INDEX, "exp.dmf.dm0c.dckp_f_dmp",
         MO_SIZEOF_MEMBER(DM0C_MO_DCKP,dckp.ckp_f_dmp),
	 MO_READ, dckp_index_class,
	 CL_OFFSETOF(DM0C_MO_DCKP,dckp.ckp_f_dmp),
	 MOintget, MOnoset, (PTR)NULL, MOidata_index
     },
     {MO_CDATA_INDEX, "exp.dmf.dm0c.dckp_l_dmp",
         MO_SIZEOF_MEMBER(DM0C_MO_DCKP,dckp.ckp_l_dmp),
	 MO_READ, dckp_index_class,
	 CL_OFFSETOF(DM0C_MO_DCKP,dckp.ckp_l_dmp),
	 MOintget, MOnoset, (PTR)NULL, MOidata_index
     },
     {MO_CDATA_INDEX, "exp.dmf.dm0c.dckp__dmp_valid",
         MO_SIZEOF_MEMBER(DM0C_MO_DCKP,dckp.ckp_dmp_valid),
	 MO_READ, dckp_index_class,
	 CL_OFFSETOF(DM0C_MO_DCKP,dckp.ckp_dmp_valid),
	 MOintget, MOnoset, (PTR)NULL, MOidata_index
     },
     {MO_CDATA_INDEX, "exp.dmf.dm0c.dckp_mode",
         MO_SIZEOF_MEMBER(DM0C_MO_DCKP,dckp.ckp_mode),
	 MO_READ, dckp_index_class,
	 CL_OFFSETOF(DM0C_MO_DCKP,dckp.ckp_mode),
	 DM0Cmo_ckp_mode_get, MOnoset, (PTR)NULL, MOidata_index
     },
     { 0 }
 };

/*
** Managed objects for a database:
**     dm0c_dnode_classes:  
**       - Node dump information from DMP_CNODE_INFO
**       - One record is returned for each in dmp_node_count
*/
static char dnode_index_class[] = "exp.dmf.dm0c.dnode_index";

static MO_CLASS_DEF dm0c_dnode_classes[]=
{
     {MO_INDEX_CLASSID|MO_CDATA_INDEX, dnode_index_class,
         MO_SIZEOF_MEMBER(DM0C_MO_DNODE,dbid),
	 MO_READ, 0,
         CL_OFFSETOF(DM0C_MO_DNODE,dbid), MOintget, MOnoset,
	 (PTR)NULL, MOidata_index
     },
     {MO_CDATA_INDEX, "exp.dmf.dm0c.dnode_fil_seq",
         MO_SIZEOF_MEMBER(DM0C_MO_DNODE,dnode.cnode_fil_seq),
	 MO_READ, dnode_index_class,
	 CL_OFFSETOF(DM0C_MO_DNODE,dnode.cnode_fil_seq),
	 MOintget, MOnoset, (PTR)NULL, MOidata_index
     },
     {MO_CDATA_INDEX, "exp.dmf.dm0c.dnode_blk_seq",
         MO_SIZEOF_MEMBER(DM0C_MO_DNODE,dnode.cnode_blk_seq),
	 MO_READ, dnode_index_class,
	 CL_OFFSETOF(DM0C_MO_DNODE,dnode.cnode_blk_seq),
	 MOintget, MOnoset, (PTR)NULL, MOidata_index
     },
     {MO_CDATA_INDEX, "exp.dmf.dm0c.dnode_la",
         MO_SIZEOF_MEMBER(DM0C_MO_DNODE,dnode.cnode_la),
	 MO_READ, dnode_index_class,
	 CL_OFFSETOF(DM0C_MO_DNODE,dnode.cnode_la),
	 DM0Cmo_la_get, MOnoset, (PTR)NULL, MOidata_index
     },
     { 0 }
 };

/*{
** Name: dm0c_mo_init - Set up DM0C MO classes, called once.
**
** Description:
**
** Inputs:
**	dm0cMOinit with some value
**
** Outputs:
**      dm0cMOinit is set to FAIL or OK
**
**	Returns:
**	    OK
**	    FAIL
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	11-Oct-2010 (jonj) SIR 124738
**	    Created
*/
static STATUS
dm0c_mo_init( void )
{
    STATUS	status;
    DB_ERROR	localDBerr;
    i4		errcode;

    /* Only do this once */
    if ( dm0cMOinit == -1 )
    {
	status = MOclassdef(MAXI2, dm0c_dsc_classes );

	if ( status == OK )
	    status = MOclassdef(MAXI2, dm0c_ext_classes );

	if ( status == OK)
	    status = MOclassdef(MAXI2, dm0c_jckp_classes );

	if ( status == OK)
	    status = MOclassdef(MAXI2, dm0c_jnode_classes );

	if ( status == OK)
	    status = MOclassdef(MAXI2, dm0c_dckp_classes );

	if ( status == OK)
	    status = MOclassdef(MAXI2, dm0c_dnode_classes );

	if ( status )
	{
	    uleFormat(&localDBerr, status, NULL, ULE_LOG,
	    		NULL, NULL, 0, NULL, &errcode, 0);
	    dm0cMOinit = FAIL;
	}
	else
	    dm0cMOinit = OK;
    }
        
    return( dm0cMOinit );
}

/*{
** Name: dm0c_mo_attach - Make Database known to MO.
**
** Description:
**	Attaches DM0C_CNF parts,
**	allocates dmp_mo memory.
**
**	It's presumed that the database's DCB's mutex
**	is held when this function is called.
**
** Inputs:
**	cnf		Pointer to DM0C_CNF for database
**	   cnf_dcb	Pointer to its DMP_DCB
**
** Outputs:
**	dcb_mo		Points to config file parts
**			attached to MO
**      None	
**
** Returns:
**	VOID
**
** Side Effects:
**	Chews up some DMF memory, and MO memory
**
** History:
**	11-Oct-2010 (jonj) SIR 124738
**	    Created
*/
VOID
dm0c_mo_attach( DM0C_CNF *cnf )
{
    return(dm0c_mo((DMP_DCB*)NULL, cnf));
}


/*{
** Name: dm0c_mo_detach - Make Database unknown to MO.
**
** Description:
**	Detaches DM0C_CNF parts previously attached,
**	deallocates dmp_mo memory.
**
** Inputs:
**	dcb		Pointer to DMP_DCB for database
**	   dcb_mo	Pointer to config file parts memory chunk
**
** Outputs:
**	dcb_mo		Is NULL
**
** Returns:
**	VOID
**
** Side Effects:
**	None
**
** History:
**	11-Oct-2010 (jonj) SIR 124738
**	    Created
*/
VOID
dm0c_mo_detach( DMP_DCB *dcb )
{
    return(dm0c_mo(dcb, (DM0C_CNF*)NULL));
}


/*{
** Name: dm0c_mo - Common Attach/Detach code
**
** Description:
**
**	Attach:	Allocate DMF memory to contain config file
**		parts to attach to MO. The DM0C_CNF cnf
**		memory is not persistent and will soon
**		be deallocated, but this MO memory will
**		persist until the DB is deleted from the
**		server via dm2d_del_db().
**
**		Copy CNF structures, or elements of structures
**		to dcb_mo memory chunk, attach the objects
**		to MO.
**
**		Note that all objects are keyed on dsc_dbid
**
**	Detach:	Detaches objects attached in dcb_mo,
**		ignoring MO errors, deallocates the memory.
**
** Inputs:
**
**  Attach:
**	dcb		NULL
**	cnf		Pointer to database's DM0C_CNF
**	   cnf_dcb	Pointer to database's DMP_DCB
**  Detach:
**	dcb		Pointer to DMP_DCB for database
**	   dcb_mo	Will be deallocated after objects
**				are detached.
**	cnf		NULL
**
** Outputs:
**
**  Attach:
**	dcb_mo		Anchors allocated object memory
**
**  Detach:
**	dcb_mo		Is deallocated, set to NULL
**
** Returns:
**	VOID
**
** Side Effects:
**	None
**
** History:
**	11-Oct-2010 (jonj) SIR 124738
**	    Created
*/
static VOID
dm0c_mo( DMP_DCB *dcb, DM0C_CNF *cnf )
{
    STATUS		status = OK;
    DM0C_DSC		*dsc;
    DM0C_EXT		*ext;
    DM0C_JNL		*jnl;
    DM0C_DMP		*dmp;
    JNL_CNODE_INFO	*jnode;
    DMP_CNODE_INFO	*dnode;
    DM0C_MO_DSC		*MOdsc;
    DM0C_MO_EXT		*MOext;
    DM0C_MO_JCKP	*MOjckp;
    DM0C_MO_JNODE	*MOjnode;
    DM0C_MO_DCKP	*MOdckp;
    DM0C_MO_DNODE	*MOdnode;
    i4			i;
    i4			jnodeCount;
    i4			dnodeCount;
    char 		buf[80];
    i4			memsize;
    DB_ERROR		localDBerr;
    i4			errcode;
    bool		Attach = (dcb == NULL);

    if ( Attach )
    {
	if ( dm0cMOinit != OK )
	{
	    status = dm0c_mo_init();
	    if ( status )
	        return;
	}

	dcb = cnf->cnf_dcb;
	dsc = cnf->cnf_dsc;
	ext = cnf->cnf_ext;
	jnl = cnf->cnf_jnl;
	dmp = cnf->cnf_dmp;

	/* Get enough memory to hold all the pieces */
	memsize = sizeof(DM0C_MO_DSC) +
		  (dsc->dsc_ext_count * sizeof(DM0C_MO_EXT)) +
		  (jnl->jnl_count * sizeof(DM0C_MO_JCKP)) +
		  (dmp->dmp_count * sizeof(DM0C_MO_DCKP));

	/* Get counts of cluster things that don't have explicit counts */

	jnodeCount = 0;
	dnodeCount = 0;

	for ( i = 0; i < DM_CNODE_MAX; i++ )
	{
	    if ( jnl->jnl_node_info[i].cnode_fil_seq || 
		 jnl->jnl_node_info[i].cnode_la.la_sequence )
	    {
		memsize += sizeof(DM0C_MO_JNODE);
		jnodeCount++;
	    }
	    if ( dmp->dmp_node_info[i].cnode_fil_seq || 
		 dmp->dmp_node_info[i].cnode_la.la_sequence )
	    {
		memsize += sizeof(DM0C_MO_DNODE);
		dnodeCount++;
	    }
	}

	status = dm0m_allocate(sizeof(DMP_MISC) + memsize,
	    (i4)DM0M_LONGTERM, (i4)MISC_CB, (i4)MISC_ASCII_ID, (char *)cnf, 
	    (DM_OBJECT **)&dcb->dcb_mo, &localDBerr);

	if ( status )
	{
	    /* Can't get memory - report the error, then return */
	    uleFormat(&localDBerr, 0, NULL, ULE_LOG,
	    		NULL, NULL, 0, NULL, &errcode, 0);
	    return;
	}
    }
    else if ( dcb->dcb_mo == NULL )
    {
        /* Detach with nothing to detach, quick exit */
        return;
    }

    /* DSC */
    MOdsc = (DM0C_MO_DSC*)((char*)dcb->dcb_mo + sizeof(DMP_MISC));

    if ( Attach )
    {
	/* Preload counts of various objects */
	MOdsc->dsc.dsc_ext_count = dsc->dsc_ext_count;
	MOdsc->jnl_count = jnl->jnl_count;
	MOdsc->jnl_node_count = jnodeCount;
	MOdsc->dmp_count = dmp->dmp_count;
	MOdsc->dmp_node_count = dnodeCount;
    }
	
    /* Point to (begining of) objects in dcb_mo  */
    MOext = (DM0C_MO_EXT*)((char*)MOdsc + sizeof(DM0C_MO_DSC));
    MOjckp = (DM0C_MO_JCKP*)((char*)MOext +
	    (MOdsc->dsc.dsc_ext_count * sizeof(DM0C_MO_EXT)));
    MOjnode = (DM0C_MO_JNODE*)((char*)MOjckp +
	    (MOdsc->jnl_count * sizeof(DM0C_MO_JCKP)));
    MOdckp = (DM0C_MO_DCKP*)((char*)MOjnode +
	    (MOdsc->jnl_node_count * sizeof(DM0C_MO_JNODE)));
    MOdnode = (DM0C_MO_DNODE*)((char*)MOdckp +
	    (MOdsc->dmp_count * sizeof(DM0C_MO_DCKP)));

    /* One DSC */
    status = MOptrout(MO_VALUE_TRUNCATED, (PTR)MOdsc, sizeof(buf), buf);
    if ( status == OK )
    {
	if ( Attach )
	{
	    /* Point to DCB */
	    MOdsc->dcb_ptr = dcb;
	    /* Copy entire DM0C_DSC */
	    MOdsc->dsc = *dsc;
	    /* Copy journal info */
	    MOdsc->jnl_ckp_seq = jnl->jnl_ckp_seq;
	    MOdsc->jnl_fil_seq = jnl->jnl_fil_seq;
	    MOdsc->jnl_blk_seq = jnl->jnl_blk_seq;
	    MOdsc->jnl_bksz = jnl->jnl_bksz;
	    MOdsc->jnl_blkcnt = jnl->jnl_blkcnt;
	    MOdsc->jnl_maxcnt = jnl->jnl_maxcnt;
	    MOdsc->jnl_la = jnl->jnl_la;
	    MOdsc->jnl_first_jnl_seq = jnl->jnl_first_jnl_seq;
	    /* Copy dump info */
	    MOdsc->dmp_ckp_seq = dmp->dmp_ckp_seq;
	    MOdsc->dmp_fil_seq = dmp->dmp_fil_seq;
	    MOdsc->dmp_blk_seq = dmp->dmp_blk_seq;
	    MOdsc->dmp_bksz = dmp->dmp_bksz;
	    MOdsc->dmp_blkcnt = dmp->dmp_blkcnt;
	    MOdsc->dmp_maxcnt = dmp->dmp_maxcnt;
	    MOdsc->dmp_la = dmp->dmp_la;
	    status = MOattach(MO_INSTANCE_VAR, dsc_index_class, buf, (PTR)MOdsc);
	}
	else
	    (void) MOdetach(dsc_index_class, buf);
    }

    /* dsc_ext_count Extents */
    for ( i = 0; status == OK && i < MOdsc->dsc.dsc_ext_count; i++, MOext++ )
    {
	status = MOptrout(MO_VALUE_TRUNCATED, (PTR)MOext, sizeof(buf), buf);
	if ( status == OK )
	{
	    if ( Attach )
	    {
		MOext->dbid = dsc->dsc_dbid;
		MOext->ext = cnf->cnf_ext[i].ext_location;
		status = MOattach(MO_INSTANCE_VAR, ext_index_class, buf, (PTR)MOext);
	    }
	    else
		(void) MOdetach(ext_index_class, buf);
	}
    }

    /* jnl_count Journal checkpoint history */
    for ( i = 0; status == OK && i < MOdsc->jnl_count; i++, MOjckp++ )
    {
	status = MOptrout( MO_VALUE_TRUNCATED, (PTR)MOjckp, sizeof(buf), buf );
	if ( status == OK )
	{
	    if ( Attach )
	    {
		MOjckp->dbid = dsc->dsc_dbid;
		MOjckp->jckp = jnl->jnl_history[i];
		status = MOattach(MO_INSTANCE_VAR, jckp_index_class, buf, (PTR)MOjckp);
	    }
	    else
		(void) MOdetach(jckp_index_class, buf);
	}
    }

    /* jnl_node_count cluster journal history */
    for ( i = 0; status == OK && i < MOdsc->jnl_node_count; i++ )
    {
	status = MOptrout(MO_VALUE_TRUNCATED, (PTR)MOjnode, sizeof(buf), buf);
	if ( status == OK )
	{
	    if ( Attach )
	    {
		jnode = &jnl->jnl_node_info[i];
		/* Anticipate holes */
		if ( jnode->cnode_fil_seq || jnode->cnode_la.la_sequence )
		{
		    MOjnode->dbid = dsc->dsc_dbid;
		    MOjnode->jnode = *jnode;
		    status = MOattach(MO_INSTANCE_VAR, jnode_index_class, buf, (PTR)MOjnode);
		    MOjnode++;
		}
	    }
	    else
	    {
		(void) MOdetach(jnode_index_class, buf);
		MOjnode++;
	    }
	}
    }

    /* dmp_count Dump checkpoint history */
    for ( i = 0; status == OK && i < MOdsc->dmp_count; i++, MOdckp++ )
    {
	status = MOptrout(MO_VALUE_TRUNCATED, (PTR)MOdckp, sizeof(buf), buf);
	if ( status == OK )
	{
	    if ( Attach )
	    {
		MOdckp->dbid = dsc->dsc_dbid;
		MOdckp->dckp = dmp->dmp_history[i];
		status = MOattach(MO_INSTANCE_VAR, dckp_index_class, buf, (PTR)MOdckp);
	    }
	    else
		(void) MOdetach(dckp_index_class, buf);
	}
    }

    /* dmp_node_count cluster dump history */
    for ( i = 0; status == OK && i < MOdsc->dmp_node_count; i++ )
    {
	status = MOptrout(MO_VALUE_TRUNCATED, (PTR)MOdnode, sizeof(buf), buf);
	if ( status == OK )
	{
	    if ( Attach )
	    {
		dnode = &dmp->dmp_node_info[i];
		/* Anticipate holes */
		if ( dnode->cnode_fil_seq || dnode->cnode_la.la_sequence )
		{
		    MOdnode->dbid = dsc->dsc_dbid;
		    MOdnode->dnode = *dnode;
		    status = MOattach(MO_INSTANCE_VAR, dnode_index_class, buf, (PTR)MOdnode);
		    MOdnode++;
		}
	    }
	    else
	    {
		(void) MOdetach(dnode_index_class, buf);
		MOdnode++;
	    }
	}
    }

    /* If any Attach error, try to clean up any mess left behind */
    if ( status )
    {
	/*
	** Report the error.
	**
	** Failure to attach MO objects to a DB shouldn't
	** prevent use of the DB.
	*/
	uleFormat(&localDBerr, status, NULL, ULE_LOG,
			NULL, NULL, 0, NULL, &errcode, 0);
	if ( dcb->dcb_mo )
	    dm0c_mo_detach(dcb);
    }

    /* On detach, deallocate dcb_mo */
    if ( !Attach && dcb->dcb_mo )
        dm0m_deallocate((DM_OBJECT**)&dcb->dcb_mo);

    return;
}


/*{
** Name: DM0Cmo_dsc_status_get - MO Get function for dsc_status
**
** Description:
**	Returns readable interpretation of dsc_status
**
** Inputs:
**	offset				- ignored
**	objsize				- ignored
**	object				- pointer to dsc_status
**	luserbuf			- length of the output buffer
**
** Outputs:
**	userbuf				- human readable status string
**
** Returns:
**	STATUS
**
** History:
**	08-Oct-2010 (jonj) SIR 124738
**	    Created.
*/
static STATUS	DM0Cmo_dsc_status_get(
    i4	    offset,
    i4	    objsize,
    PTR	    object,
    i4	    luserbuf,
    char    *userbuf )
{
    i4		dsc_status = *(i4*)(object + offset);
    char	format_buf[256];

    MEfill(sizeof(format_buf), 0, format_buf);

    TRformat(NULL, 0, format_buf, sizeof(format_buf), "%v",
		DSC_STATUS, dsc_status);

    return(MOstrout( MO_VALUE_TRUNCATED, format_buf, luserbuf, userbuf ));
}


/*{
** Name: DM0Cmo_dcb_status_get - MO Get function for dcb_status
**
** Description:
**	Returns readable interpretation of dcb_status
**
** Inputs:
**	offset				- ignored
**	objsize				- ignored
**	object				- Pointer to DMP_DCB pointer
**	luserbuf			- length of the output buffer
**
** Outputs:
**	userbuf				- human readable status string
**
** Returns:
**	STATUS
**
** History:
**	08-Oct-2010 (jonj) SIR 124738
**	    Created.
*/
static STATUS	DM0Cmo_dcb_status_get(
    i4	    offset,
    i4	    objsize,
    PTR	    object,
    i4	    luserbuf,
    char    *userbuf )
{
    DMP_DCB	*dcb = *(DMP_DCB**)(object + offset);
    char	format_buf[256];

    MEfill(sizeof(format_buf), 0, format_buf);

    TRformat(NULL, 0, format_buf, sizeof(format_buf), "%v",
		DCB_STATUS, dcb->dcb_status);
    return(MOstrout( MO_VALUE_TRUNCATED, format_buf, luserbuf, userbuf ));
        
}


/*{
** Name: DM0Cmo_dsc_dbaccess_get - MO Get function for dbaccess
**
** Description:
**	Returns readable interpretation of dbaccess
**
** Inputs:
**	offset				- ignored
**	objsize				- ignored
**	object				- pointer to dbaccess
**	luserbuf			- length of the output buffer
**
** Outputs:
**	userbuf				- human readable dbaccess string
**
** Returns:
**	STATUS
**
** History:
**	08-Oct-2010 (jonj) SIR 124738
**	    Created.
*/
static STATUS	DM0Cmo_dsc_dbaccess_get(
    i4	    offset,
    i4	    objsize,
    PTR	    object,
    i4	    luserbuf,
    char    *userbuf )
{
    u_i4	dbaccess = *(u_i4*)(object + offset);
    char	format_buf[256];

    MEfill(sizeof(format_buf), 0, format_buf);

    TRformat(NULL, 0, format_buf, sizeof(format_buf), "%v",
"GLOBAL,RES_DDB,DESTROYDB,CREATEDB,\
OPERATIVE,CONVERTING,UPGRADING,MLSDB,\
DESDBNOWAIT,PRODUCTION,NOBACKUP,RDONLY,\
MUSTLOG",
		dbaccess);
    return(MOstrout( MO_VALUE_TRUNCATED, format_buf, luserbuf, userbuf ));
}


/*{
** Name: DM0Cmo_dsc_dbservice_get - MO Get function for dbservice
**
** Description:
**	Returns readable interpretation of dbservice
**
** Inputs:
**	offset				- ignored
**	objsize				- ignored
**	object				- pointer to dbaccess
**	luserbuf			- length of the output buffer
**
** Outputs:
**	userbuf				- human readable dbservice string
**
** Returns:
**	STATUS
**
** History:
**	08-Oct-2010 (jonj) SIR 124738
**	    Created.
*/
static STATUS	DM0Cmo_dsc_dbservice_get(
    i4	    offset,
    i4	    objsize,
    PTR	    object,
    i4	    luserbuf,
    char    *userbuf )
{
    u_i4	dbaccess = *(u_i4*)(object + offset);
    char	format_buf[256];

    MEfill(sizeof(format_buf), 0, format_buf);

    TRformat(NULL, 0, format_buf, sizeof(format_buf), "%v",
"DDB,CDB,RMS,8,\
16,32,64,128,\
256,512,1024,2048,\
4096,8192,16384,32768,\
NAME_UPPER,LP64,DELIM_UPPER,DELIM_MIXED,\
USER_MIXED,NFC,4194304,8388608,\
16777217,33554432,67108864,134217728,\
536870912,FORCED_CONSISTENT,UTYPES_ALLOWED",
		dbaccess);
    return(MOstrout( MO_VALUE_TRUNCATED, format_buf, luserbuf, userbuf ));
}


/*{
** Name: DM0Cmo_dsc_unormalization_get - MO Get function for unicode normalization
**
** Description:
**	Returns "NFC" or "NFD" if unicode present
**
** Inputs:
**	offset				- ignored
**	objsize				- ignored
**	object				- pointer to dsc_dbservice
**	luserbuf			- length of the output buffer
**
** Outputs:
**	userbuf				- human readable status string
**
** Returns:
**	STATUS
**
** History:
**	08-Oct-2010 (jonj) SIR 124738
**	    Created.
*/
static STATUS	DM0Cmo_dsc_unormalization_get(
    i4	    offset,
    i4	    objsize,
    PTR	    object,
    i4	    luserbuf,
    char    *userbuf )
{
    u_i4	dsc_dbservice = *(u_i4*)(object + offset);
    char	format_buf[256];

    MEfill(sizeof(format_buf), 0, format_buf);

    if ( dsc_dbservice & DU_UTYPES_ALLOWED )
    {
	TRformat(NULL, 0, format_buf, sizeof(format_buf), "%s",
		(dsc_dbservice & DU_UNICODE_NFC) ? "NFC" : "NFD");
    }

    return(MOstrout( MO_VALUE_TRUNCATED, format_buf, luserbuf, userbuf ));
}


/*{
** Name: DM0Cmo_dsc_inconsistent_get - MO Get function for dsc_inconst_code
**
** Description:
**	Returns readable interpretation of dsc_inconst_code
**
** Inputs:
**	offset				- ignored
**	objsize				- ignored
**	object				- pointer to dsc_inconst_code
**	luserbuf			- length of the output buffer
**
** Outputs:
**	userbuf				- human readable status string
**
** Returns:
**	STATUS
**
** History:
**	08-Oct-2010 (jonj) SIR 124738
**	    Created.
*/
static STATUS	DM0Cmo_dsc_inconsistent_get(
    i4	    offset,
    i4	    objsize,
    PTR	    object,
    i4	    luserbuf,
    char    *userbuf )
{
    i4		code = *(i4*)(object + offset);
    char	format_buf[256];

    MEfill(sizeof(format_buf), 0, format_buf);

    TRformat(NULL, 0, format_buf, sizeof(format_buf), "%w",
		DSC_INCONST_CODE, code);

    return(MOstrout( MO_VALUE_TRUNCATED, format_buf, luserbuf, userbuf ));
}


/*{
** Name: DM0Cmo_ext_flags_get - MO Get function for DMP_LOC_ENTRY flags
**
** Description:
**	Returns readable interpretation of flags
**
** Inputs:
**	offset				- ignored
**	objsize				- ignored
**	object				- pointer to DMP_LOC_ENTRY flags
**	luserbuf			- length of the output buffer
**
** Outputs:
**	userbuf				- human readable status string
**
** Returns:
**	STATUS
**
** History:
**	08-Oct-2010 (jonj) SIR 124738
**	    Created.
*/
static STATUS	DM0Cmo_ext_flags_get(
    i4	    offset,
    i4	    objsize,
    PTR	    object,
    i4	    luserbuf,
    char    *userbuf )
{
    i4		*flags = (i4*)(object + offset);
    char	format_buf[256];

    MEfill(sizeof(format_buf), 0, format_buf);

    TRformat(NULL, 0, format_buf, sizeof(format_buf), "%v",
		DCB_LOC_FLAGS, *flags);

    return(MOstrout( MO_VALUE_TRUNCATED, format_buf, luserbuf, userbuf ));
}


/*{
** Name: DM0Cmo_la_get - MO Get function for LG_LA
**
** Description:
**	Returns readable interpretation of LG_LA
**
** Inputs:
**	offset				- ignored
**	objsize				- ignored
**	object				- pointer to LG_LA
**	luserbuf			- length of the output buffer
**
** Outputs:
**	userbuf				- formatted log address
**
** Returns:
**	STATUS
**
** History:
**	08-Oct-2010 (jonj) SIR 124738
**	    Created.
*/
static STATUS	DM0Cmo_la_get(
    i4	    offset,
    i4	    objsize,
    PTR	    object,
    i4	    luserbuf,
    char    *userbuf )
{
    LG_LA	*lga = (LG_LA*)(object + offset);
    char	format_buf[256];

    MEfill(sizeof(format_buf), 0, format_buf);

    TRformat(NULL, 0, format_buf, sizeof(format_buf), "<%d,%d,%d>",
		lga->la_sequence,
		lga->la_block,
		lga->la_offset);

    return(MOstrout( MO_VALUE_TRUNCATED, format_buf, luserbuf, userbuf ));
}


/*{
** Name: DM0Cmo_ckp_date_get - MO Get function for ckp_date
**
** Description:
**	Returns readable interpretation of ckp_date
**
** Inputs:
**	offset				- ignored
**	objsize				- ignored
**	object				- pointer to ckp_date
**	luserbuf			- length of the output buffer
**
** Outputs:
**	userbuf				- formatted date string
**
** Returns:
**	STATUS
**
** History:
**	08-Oct-2010 (jonj) SIR 124738
**	    Created.
*/
static STATUS	DM0Cmo_ckp_date_get(
    i4	    offset,
    i4	    objsize,
    PTR	    object,
    i4	    luserbuf,
    char    *userbuf )
{
    DB_TAB_TIMESTAMP	*date = (DB_TAB_TIMESTAMP*)(object + offset);
    char	format_buf[256];

    MEfill(sizeof(format_buf), 0, format_buf);

    TRformat(NULL, 0, format_buf, sizeof(format_buf), "%?",
		date);

    return(MOstrout( MO_VALUE_TRUNCATED, format_buf, luserbuf, userbuf ));
}


/*{
** Name: DM0Cmo_ckp_mode_get - MO Get function for ckp_mode
**
** Description:
**	Returns readable interpretation of ckp_mode
**
** Inputs:
**	offset				- ignored
**	objsize				- ignored
**	object				- pointer to ckp_mode
**	luserbuf			- length of the output buffer
**
** Outputs:
**	userbuf				- human readable string
**
** Returns:
**	STATUS
**
** History:
**	08-Oct-2010 (jonj) SIR 124738
**	    Created.
*/
static STATUS	DM0Cmo_ckp_mode_get(
    i4	    offset,
    i4	    objsize,
    PTR	    object,
    i4	    luserbuf,
    char    *userbuf )
{
    i4		*mode = (i4*)(object + offset);
    char	format_buf[256];

    MEfill(sizeof(format_buf), 0, format_buf);

    TRformat(NULL, 0, format_buf, sizeof(format_buf), "%v",
		"OFFLINE,ONLINE,TABLE", *mode);

    return(MOstrout( MO_VALUE_TRUNCATED, format_buf, luserbuf, userbuf ));
}


/*{
** Name: DM0Cmo_refresh_set - MO Set method to initiate refresh of db MO
**
** Description:
**	
**	Attaches a factotum thread to refresh a database's
**	managed objects.
**
**	While it's be nice to do the detach/attach in this function,
**	a factotum better serves the purpose as we can't really
**	detach/attach object instances on which this Set method
**	has been invoked - plus there are all sorts of opportunities
**	for MO mutex deadlocks.
**
**	Be very resilient to unanticipated errors; if the refresh can't
**	happen, it's no big deal, not worth crashing the server.
**
** Inputs:
**	offset				- ignored
**	objsize				- ignored
**	object				- pointer to DM0C_MO_DSC
**	sbuf				- database_id
**
** Outputs:
**	None
**
** Returns:
**	OK
**
** History:
**	08-Oct-2010 (jonj) SIR 124738
**	    Created.
*/
static STATUS
DM0Cmo_refresh_set(
i4	offset,
i4	lsbuf,
char	*sbuf,
i4	size,
PTR	object )
{
    DM0C_MO_DSC	*MOdsc = (DM0C_MO_DSC*)object;
    DMP_DCB	*dcb = MOdsc->dcb_ptr;
    i4		dbid;
    SCF_CB	scf_cb;
    SCF_FTC	ftc;
    STATUS	status;
    i4		errcode;
    char	thread_name[DB_MAXNAME];

    CVal(sbuf, &dbid);

    /* Quick sanity checks */
    if ( dcb && dcb->dcb_id == dbid )
    {
	/* Launch a factotum to do the refresh */
	scf_cb.scf_type = SCF_CB_TYPE;
	scf_cb.scf_length = sizeof(SCF_CB);
	scf_cb.scf_session = DB_NOSESSION;
	scf_cb.scf_facility = DB_DMF_ID;
	scf_cb.scf_ptr_union.scf_ftc = &ftc;

	ftc.ftc_facilities = 1 << DB_DMF_ID;
	ftc.ftc_data = (char*)&dcb->dcb_id;
	ftc.ftc_data_length = sizeof(dcb->dcb_id);
	STprintf(thread_name, " <dm0cMOrefresh %x> ", dcb->dcb_id);
	ftc.ftc_thread_name = &thread_name[0];
	ftc.ftc_priority = SCF_CURPRIORITY;
	ftc.ftc_thread_entry = dm0c_mo_refresh;
	ftc.ftc_thread_exit = NULL;

	status = scf_call(SCS_ATFACTOT, &scf_cb);

	/* Report any error, but return OK */
	if ( status )
	    uleFormat(&scf_cb.scf_error, 0, NULL, ULE_LOG,
			NULL, NULL, 0, NULL, &errcode, 0);
    }

    return(OK);
}


/*{
** Name: dm0c_mo_refresh - Factotum function to detach/attach db MO
**
** Description:
**	
**	The factotum code to "refresh" a database's managed objects.
**
**	As the database may be closed by the time this thread
**	runs, find the DMP_DCB by dbid in the server's list of
**	open databases.
**
** Inputs:
**	ftc				SCF_FTC pointer with
**	    ftc_data			Database's dbid
**
** Outputs:
**	None
**
** Returns:
**	E_DB_OK, always
**
** History:
**	08-Oct-2010 (jonj) SIR 124738
**	    Created.
*/
static DB_STATUS
dm0c_mo_refresh(
SCF_FTX		*ftx)
{
    i4		dbid = *(i4*)ftx->ftx_data;
    DM0C_CNF	*cnf;
    i4		errcode;
    STATUS	status;
    DMP_DCB	*dcb, *ndcb;

    /* Mutex server's open database list */
    dm0s_mlock(&dmf_svcb->svcb_dq_mutex);

    dcb = NULL;

    for ( ndcb = dmf_svcb->svcb_d_next;
          !dcb && ndcb != (DMP_DCB*)&dmf_svcb->svcb_d_next;
	  ndcb = ndcb->dcb_q_next )
    {
	/* Match on dbid, ignore if no MO attached */
        if ( ndcb->dcb_id == dbid && ndcb->dcb_mo )
	    dcb = ndcb;
    }
    dm0s_munlock(&dmf_svcb->svcb_dq_mutex);
    
    /* Do nothing if not found */
    if ( dcb )
    {
	/*
	** Mutex the DCB, using CSp_semaphore instead of dm0s_mlock.
	** When the dq_mutex is released, there's an opportunity
	** for the DCB to be deallocated and it's mutex released;
	** dm0s_mlock() doesn't tolerate "bad" mutexes and will
	** dmd_check if it gets a p_semaphore error, but we don't care.
	*/
	if ( (status = CSp_semaphore(TRUE, &dcb->dcb_mutex)) == OK )
	{
	    /* Recheck DCB after waiting for mutex */
	    if ( dcb->dcb_id == dbid && dcb->dcb_mo )
	    {
		/* Detach extant MO */
		dm0c_mo_detach(dcb);

		/* Open config file */
		status = dm0c_open(dcb, 0, dmf_svcb->svcb_lock_list, 
					&cnf, &ftx->ftx_error);

		if ( status )
		    uleFormat(&ftx->ftx_error, 0, NULL, ULE_LOG,
				NULL, NULL, 0, NULL, &errcode, 0);
		else
		{
		    /* Attach MO, close cnf */
		    dm0c_mo_attach(cnf);
		    (void)dm0c_close(cnf, 0, &ftx->ftx_error);
		}
	    }
	    CSv_semaphore(&dcb->dcb_mutex);
	}
    }

    /* Return to dissolve factotum thread */
    return(E_DB_OK);
}
