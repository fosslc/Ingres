/*
**Copyright (c) 2004 Ingres Corporation
*/
#include    <compat.h>
#include    <gl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <cs.h>
#include    <pc.h>
#include    <tm.h>
#include    <tr.h>
#include    <lg.h>
#include    <lk.h>
#include    <di.h>
#include    <sr.h>
#include    <ex.h>
#include    <ulf.h>
#include    <adf.h>
#include    <dmf.h>
#include    <dmccb.h>
#include    <dmrcb.h>
#include    <dmtcb.h>
#include    <dmxcb.h>
#include    <dmucb.h>
#include    <dm.h>
#include    <dml.h>
#include    <dmp.h>
#include    <dmpp.h>
#include    <dm0p.h>
#include    <dm1p.h>
#include    <dm0pbmcb.h>
#include    <dmse.h>
#include    <dm1b.h>
#include    <dm2umct.h>
#include    <dm2umxcb.h>
#include    <dm2t.h>
#include    <dm2u.h>
#include    <dm0llctx.h>
#include    <dm0c.h>
#include    <dmfjsp.h>

/*
** Name: DMFDBG.C	 Debugging support routines for DMF utilities.
**
**	Function prototypes defined in DMFJSP.H.
**
** History
**	25-oct-1989 (Rachael)
**	    Modified argument passed to dump_tcb to contain the actual
**	    TCB and not the DM0C_TAB field. Since the DCB is valid at this
**	    point, all we needed to do was index into the DCB and pass the
**	    particular system catalog pointers that we wanted displayed by
**	    the dump_tcb() routine. Also removed the output of the iirelation
**	    index since all the pertinent data is contained in the iirelation.
**	7-may-90 (bryanp)
**	    Enhanced dump_dsc() to display more information from the DM0C_DSC
**	    as part of tracking down a checkpoint bug.
**	24-sep-1990 (rogerk)
**	    Merged 6.3 changes into 6.5.
**	25-sep-1991 (mikem) integrated following change: 9-apr-91 (seng)
**	    Need to add void declaration before dump_dcb(), dump_tcb(),
**	    dump_jnl(), dump_dsc(), dump_ext().  Apparently, forward
**	    references need to have a type declarator in front on AIX 3.1
**	    on the IBM RS/6000.
**	25-sep-1991 (mikem) integrated following change: 23-apr-1991 (bryanp)
**	    VOID belongs on the definition as well as on the declaration.
**	25-sep-1991 (mikem) integrated following change: 30-apr-1991 (bryanp)
**	    Brought trace statements up to date.
**	3-feb-1992 (bryanp)
**	    Add TCB_PEND_DESTROY to tcb_status.
**	21-may-1992 (jnash)
**	    Change TRdisplay of tcb->tcb_rel.relcmptlvl to I4.
**	8-july-1992 (ananth)
**	    Prototyping DMF.
**      24-sep-1992 (stevet)
**          Replaced dcb_timezone with dcb_tzcb.
**	05-oct-1992 (rogerk)
**	    Reduced Logging Project.  In preparation for support of Partial
**	    TCB's for the new recovery schemes, changed references to
**	    some tcb fields to use the new tcb_table_io structure fields.
**	26-oct-1992 (rogerk)
**	    Reduced Logging Project.  Changed tbio_flags to tbio_status.
**	    Added include of LG.H as its now needed to include dm0pbmcb.
**	16-feb-1993 (rogerk)
**	    Added tcb_open_count field.
**	24-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up process ID typedef.
**	26-jul-1993 (bryanp)
**	    Replace all uses of DM_LKID with LK_LKID.
**	23-aug-1993 (andys)
**	    Use CL_OFFSETOF rather than zero-pointer construction in TRdisplay
**	    statement. Bug caused by VMS C compiler when using /OPT. This fix
**	    is the same as that for bug 37300. [bug 46296]. 
**	    [was 04-sep-1992 (andys)]
**	28-jan-1994 (kbref)
**	    Enhance the dump_jnl and dump_dmp routines so that their config
**	    file output looks like that of INFODB's.
**	11-oct-93 (johnst)
**	    Bug #56449
**	    Changed TRdisplay format args from %x to %p where necessary to
**	    display pointer types. We need to distinguish this on, eg 64-bit
**	    platforms, where sizeof(PTR) > sizeof(int).
**	20-jun-1994 (jnash)
**	    Eliminate unused logical logging field. 
**      24-jan-1995 (stial01)
**          BUG 66473: dump_dmp(), dump_jnl() display if ckp_mode indicates 
**          TABLE level checkpoint.
**      08-Dec-1997 (hanch04)
**          Use new la_block field when block number is needed instead
**          of calculating it from the offset
**          Keep track of block number and offset into the current block.
**          for support of logs > 2 gig
**      12-nov-1999 (stial01)
**          Changes to how btree key info is stored in tcb
**      15-may-2000 (stial01)
**          Remove Smart Disk code
**      06-jul-2000 (stial01)
**          Changes to btree leaf/index klen in tcb (b102026)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      30-jun-2005 (horda03) Bug 114498/INGSRV3299
**          New flag for TCB status - also now use the macro
**          TCB_FLAG_MEANING rather than putting the values here.
**/

/*
** <tm.h> is supposed to define TM_SIZE_STR, but some current versions don't.
** Work around this for now, since this is a debugging file...
*/
#ifndef TM_SIZE_STR
#define TM_SIZE_STR	50
#endif

VOID
dump_file(f)
DM_FILE	*f;
{
    TRdisplay("filename= %12.12s\n",f->name);
}

VOID
dump_cnf(cnf)
DM0C_CNF *cnf;
{
    TRdisplay("============================cnf dump============================\n");
    TRdisplay("----------dcb------------\n");
    if (cnf->cnf_dcb)
	dump_dcb(cnf->cnf_dcb);
    else
	TRdisplay("NONE\n");
    TRdisplay("----------rel tab--------\n");
    if (cnf->cnf_dcb->dcb_rel_tcb_ptr)
	dump_tcb(cnf->cnf_dcb->dcb_rel_tcb_ptr);
    else
	TRdisplay("NONE\n");
    TRdisplay("----------att tab--------\n");
    if (cnf->cnf_dcb->dcb_att_tcb_ptr)
	dump_tcb(cnf->cnf_dcb->dcb_att_tcb_ptr);
    else
	TRdisplay("NONE\n");
    TRdisplay("----------idx tab--------\n");
    if (cnf->cnf_dcb->dcb_idx_tcb_ptr)
	dump_tcb(cnf->cnf_dcb->dcb_idx_tcb_ptr);
    else
	TRdisplay("NONE\n");

    TRdisplay("----------jnl------------\n");
    dump_jnl(cnf);
    TRdisplay("----------dmp------------\n");
    dump_dmp(cnf);
    TRdisplay("----------dsc------------\n");
    if (cnf->cnf_dsc)
	dump_dsc(cnf->cnf_dsc);
    else
	TRdisplay("NONE\n");
/*
    TRdisplay("----------ext------------\n");
    if (cnf->cnf_ext)
	dump_ext(cnf->cnf_ext);
    else
	TRdisplay("NONE\n");
*/
    TRdisplay("----cnf_c_ext = %d-------\n",cnf->cnf_c_ext);
    TRdisplay("=======================end of cnf dump==========================\n");
}


/*
** Name: dump_dsc	- dump the DM0C_DSC information
**
** History:
**	7-may-90 (bryanp)
**	    Dump more information as part of tracking down a checkpoint bug.
**	25-sep-1991 (mikem) integrated following change: 30-apr-1991 (bryanp)
**	    Brought trace information up to date.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	17-Nov-2009 (kschendel) SIR 122882
**	    cmptlvl is now an int, fix here.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Utilize DSC_STATUS, DSC_INCONST_CODE defines.
*/
VOID
dump_dsc(dsc)
DM0C_DSC    *dsc;
{
    char    time_buf_first[TM_SIZE_STR];
    char    time_buf_last [TM_SIZE_STR];

    TRdisplay("=========================dsc dump============================\n");
    TRdisplay("	Status: %v\n", DSC_STATUS, dsc->dsc_status);
    TRdisplay(" Open count=%x    Sync flag: %v    DBType: %w    Access: %w\n",
		dsc->dsc_open_count,
		"SYNC", dsc->dsc_sync_flag,
		",PRIVATE,PUBLIC,DISTRIBUTED", dsc->dsc_type,
		",READ,WRITE", dsc->dsc_access_mode);
    TRdisplay(" Version=(create: %w, current: %w)    Config Version=%x\n",
		"ANCIENT,V1,V2", dsc->dsc_c_version,
		"ANCIENT,V1,V2", dsc->dsc_version,
		dsc->dsc_cnf_version);
    TRdisplay(" DBID=%x    Sysmod=%x    Extents=%d    Last Table ID=%d\n",
		dsc->dsc_dbid, dsc->dsc_sysmod, dsc->dsc_ext_count,
		dsc->dsc_last_table_id);
    TRdisplay(" DBaccess=%x    DBservice=%x    Compat Level=(%d,%d)\n",
		dsc->dsc_dbaccess, dsc->dsc_dbservice,
		dsc->dsc_dbcmptlvl, dsc->dsc_1dbcmptminor);
    if (dsc->dsc_patchcnt)
    {
	_VOID_ TMcvtsecs( dsc->dsc_1st_patch, time_buf_first );
	_VOID_ TMcvtsecs( dsc->dsc_last_patch, time_buf_last );
	TRdisplay(" Patch count=%d    First patch=%s    Last patch=%s\n",
		dsc->dsc_patchcnt, time_buf_first, time_buf_last );
    }
    if (dsc->dsc_inconst_code)
    {
	_VOID_ TMcvtsecs( dsc->dsc_inconst_time, time_buf_first );
	TRdisplay(" Action code: %w    Action time=%s\n",
		DSC_INCONST_CODE, dsc->dsc_inconst_code,
		time_buf_first );
    }
    TRdisplay(" Collation name: %t\n",
		(i4)sizeof(dsc->dsc_collation), dsc->dsc_collation);

}


VOID
dump_loc_entry(entry)
DMP_LOC_ENTRY	*entry;
{
    TRdisplay("=======================dump location entry=====================\n");
    TRdisplay("	Location: %~t	Physical: %t\n",
	sizeof(entry->logical),&entry->logical,
	entry->phys_length,&entry->physical);
    TRdisplay("	Status: %v\n",
	"DCB_ROOT,DCB_DATA,DCB_JOURNAL,DCB_CKP,DCB_ALIAS,DCB_WORK,DCB_DUMP,DCB_AWORK",
	entry->flags);
}

/*{
** Name: dump_dcb	- dump the DCB control block to the trace log.
**
** Description:
**	This routine dumps the DCB to the trace log.
**
** Inputs:
**	dcb				DCB to dump
**
** Outputs:
**	None.
**
**	Returns:
**	    VOID
**
** History:
**	25-sep-1991 (mikem) integrated following change: 30-apr-1991 (bryanp)
**          Created.
*/
VOID
dump_dcb(dcb)
DMP_DCB *dcb;
{
    TRdisplay("===========================dcb dump=============================\n");
    TRdisplay("    Database (%~t,%~t)  Type: %w  Status: %v\n",
	sizeof(dcb->dcb_name), &dcb->dcb_name,
	sizeof(dcb->dcb_db_owner), &dcb->dcb_db_owner,
	",PRIVATE,PUBLIC,DISTRIBUTED", dcb->dcb_db_type,
	"JOURNAL,EXCLUSIVE,ROLL,RCP,CSP,FAST_COMMIT,INVALID,BACKUP,OFF_CSP",
	dcb->dcb_status);
    TRdisplay("    Identifier: 0x%x  Access mode: %w  Served: %w\n",
	dcb->dcb_id, ",READ,WRITE", dcb->dcb_access_mode,
	",MULTIPLE,SINGLE", dcb->dcb_served);
    TRdisplay("    Reference Count: %4d  Open Count: %4d Valid Stamp: 0x%x\n",
	dcb->dcb_ref_count, dcb->dcb_opn_count, dcb->dcb_valid_stamp);
    TRdisplay("    Relation  TCB: 0x%p    Attribute TCB: 0x%p    Index     TCB: 0x%p\n",
	dcb->dcb_rel_tcb_ptr, dcb->dcb_att_tcb_ptr, dcb->dcb_idx_tcb_ptr);
    TRdisplay("    Lock id: %x%x  Temp lock id: %x%x  Open lock id: %x%x\n",
	dcb->dcb_lock_id[0], dcb->dcb_lock_id[1],
	dcb->dcb_tti_lock_id[0], dcb->dcb_tti_lock_id[1],
	dcb->dcb_odb_lock_id[0], dcb->dcb_odb_lock_id[1]);
    TRdisplay("    Log id: %x  Mutex: %4.4{%x %} Timezone: 0x%x\n",
        dcb->dcb_log_id, &dcb->dcb_mutex, 0, dcb->dcb_tzcb);
    TRdisplay("    Location: %~t  Physical : %t\n",
	sizeof(dcb->dcb_location.logical), &dcb->dcb_location.logical,
	dcb->dcb_location.phys_length, &dcb->dcb_location.physical);
    TRdisplay("dcb root location:\n");
    if (dcb->dcb_root)
	dump_loc_entry(dcb->dcb_root);
    else
	TRdisplay("NONE\n");
    TRdisplay("dcb jnl location:\n");
    if (dcb->dcb_jnl)
	dump_loc_entry(dcb->dcb_jnl);
    else
	TRdisplay("NONE\n");
    TRdisplay("dcb ckp location:\n");
    if (dcb->dcb_ckp)
	dump_loc_entry(dcb->dcb_ckp);
    else
	TRdisplay("NONE\n");
    TRdisplay("dcb dmp location:\n");
    if (dcb->dcb_dmp)
	dump_loc_entry(dcb->dcb_dmp);
    else
	TRdisplay("NONE\n");
/*
    TRdisplay("dcb ext:\n");
    if (dcb->dcb_ext)
	dump_ext(dcb->dcb_ext);
    else
	TRdisplay("NONE\n");
*/
    TRdisplay("    Collation name: %~t\n", sizeof(dcb->dcb_colname),
		dcb->dcb_colname);
}

VOID
dump_ext(ext)
DMP_EXT *ext;
{
    TRdisplay("%8* Logical %16* Flags %10* Physical\n%8* %80*-\n");
    TRdisplay("%#.#{%8* %.#s  %16v %.*s\n%}",
	ext->ext_count, sizeof(DMP_LOC_ENTRY), ext->ext_entry,
	sizeof(DB_LOC_NAME), &(((DMP_LOC_ENTRY *)0)->logical),
	"ROOT,DATA,JNL,CKP,ALI,WRK,DMP,AWRK",&(((DMP_LOC_ENTRY *)0)->flags),
	&(((DMP_LOC_ENTRY *)0)->phys_length),
	&(((DMP_LOC_ENTRY *)0)->physical));
}


/*{
** Name: dump_tcb	- dump the TCB information to the trace log.
**
** Description:
**	This function dumps TCB information to the trace log.
**
** Inputs:
**	tcb				TCB to dump
**
** Outputs:
**	None
**
**	Returns:
**	    VOID
**
** History:
**      25-sep-1991 (mikem) integrated following change: 30-apr-1991 (bryanp)
**          Created.
**	3-feb-1992 (bryanp)
**	    Add TCB_PEND_DESTROY to tcb_status.
**	21-may-1992 (jnash)
**	    Change TRdisplay of tcb->tcb_rel.relcmptlvl to I4.
**	05-oct-1992 (rogerk)
**	    Reduced Logging Project.  Added support for partial TCB's
**	    which are not initialized with system catalog information.
**	    These TCB's are used only for IO operations.  The former
**	    tcb fields which carried information needed for IO operations
**	    have been moved into the tcb_table_io structure.
**	    Changed debug routine to reference new table_io fields and
**	    to only display system catalog information if it has been
**	    initialized.
**	26-jul-1993 (bryanp)
**	    Replace all uses of DM_LKID with LK_LKID.
**	20-jun-1994 (jnash)
**	    Eliminate unused logical logging field. 
**      12-Apr-1999 (stial01)
**          dump_tcb() Distinguish leaf/index info for BTREE/RTREE indexes
**	20-Oct-2009 (kschendel) SIR 122739
**	    Remove atts/katts count items.
**	12-Nov-2009 (kschendel) SIR 122882
**	    Update relcmptlbl display to decimal.
*/
VOID
dump_tcb(tcb)
DMP_TCB	    *tcb;
{
    DMP_TABLE_IO	*tbio = &tcb->tcb_table_io;
    DMP_LOCATION	*loc;
    i4			i;

    TRdisplay("    Table Name: (%~t,%~t)   Id: (%d,%d)\n",
	sizeof(tcb->tcb_rel.relid), &tcb->tcb_rel.relid,
	sizeof(tcb->tcb_rel.relowner), &tcb->tcb_rel.relowner, 
	tcb->tcb_rel.reltid.db_tab_base, tcb->tcb_rel.reltid.db_tab_index);
    TRdisplay("    Status: %v\n",
	TCB_FLAG_MEANING, tcb->tcb_status);
    TRdisplay("    Ref_count: %d  Valid_count: %d  Open_count: %d\n",
	tcb->tcb_ref_count, tcb->tcb_valid_count, tcb->tcb_open_count);
    TRdisplay("    Parent TCB: 0x%p  DCB:0x%p  Hash Bucket:0x%p\n",
	tcb->tcb_parent_tcb_ptr, tcb->tcb_dcb_ptr, tcb->tcb_hash_bucket_ptr);
    TRdisplay("    Tuple Adds: %d   Page Adds: %d\n",
	tcb->tcb_tup_adds, tcb->tcb_page_adds);
    TRdisplay("    Table Lockid: %x%x\n", 
	tcb->tcb_lk_id.lk_unique, tcb->tcb_lk_id.lk_common);

    if ((tcb->tcb_status & TCB_PARTIAL) == 0)
    {
	TRdisplay("    Attributes:  %w %w %w %w %w %w %w\n",
	    "PERMANENT,TEMPORARY", tcb->tcb_temporary,
	    "USER,SYSTEM", tcb->tcb_sysrel,
	    ",LOAD", tcb->tcb_loadfile,
	    "NOLOGGING,", tcb->tcb_logging,
	    ",NO_FILE", tcb->tcb_nofile,
	    "NO_UPDATE_IDX,UPDATE_IDX", tcb->tcb_update_idx,
	    ",NO_TIDS", tcb->tcb_no_tids);
	if (tcb->tcb_index_count)
	{
	    TRdisplay("                 %w\n",
		"INDEX_NONUNIQUE, INDEX_UNIQUE", tcb->tcb_unique_index);
	}
	TRdisplay("    INDEX Keys: %6.2{%5.2d%}\n", tcb->tcb_ikey_map, 0);
	if (tcb->tcb_rel.relspec == TCB_BTREE || 
	    tcb->tcb_rel.relspec == TCB_RTREE)

	    TRdisplay("    KEY Count: %d   IXklen %d Leafklen: %d   PerIndex: %d PerLeaf\n",
		tcb->tcb_keys, 
		tcb->tcb_ixklen, 
		tcb->tcb_klen,
		tcb->tcb_kperpage, 
		tcb->tcb_kperleaf);
	else
	    TRdisplay("    KEY Count: %d   Length: %d   Perpage: %d\n",
		tcb->tcb_keys, tcb->tcb_klen, tcb->tcb_kperpage);
	TRdisplay("    Logical Key: %d,%d Table-type: %w\n",
	    tcb->tcb_high_logical_key, tcb->tcb_low_logical_key,
	    ",,,HEAP,,ISAM,,HASH,,,,BTREE,,RTREE,GATEWAY",
	    tcb->tcb_table_type);
	TRdisplay("    LCT: 0x%p\n", tcb->tcb_lct_ptr);
	TRdisplay("    Relation Information:\n");
	TRdisplay("        Name: (%~t,%~t)   Id: (%d,%d)\n",
	    sizeof(tcb->tcb_rel.relid), &tcb->tcb_rel.relid,
	    sizeof(tcb->tcb_rel.relowner), &tcb->tcb_rel.relowner, 
	    tcb->tcb_rel.reltid.db_tab_base, tcb->tcb_rel.reltid.db_tab_index);
	TRdisplay("        Attributes: %d  Width: %d  Structure: %w Type: %w\n",
	    tcb->tcb_rel.relatts, tcb->tcb_rel.relwid,
	    ",,,HEAP,,ISAM,,HASH,,,,BTREE,,RTREE", tcb->tcb_rel.relspec,
	    "0,1,2,HEAP,4,ISAM,6,HASH,7,8,9,10,BTREE,12,RTREE,GATEWAY",
	    tcb->tcb_table_type);
	TRdisplay("        Status: %v\n",
	    "CATALOG,NOUPDATE,PROTUP,INTEG,CONCUR,VIEW,VBASE,INDEX,\
    BINARY,COMPRESSED,INDEX_COMPRESSED,?,PROTALL,\
    PROTECT,EXCATALOG,JON,UNIQUE,INDEXED,JOURNAL,NORECOVER,ZOPTSTAT,DUPLICATES,\
    MULTI_LOC,GATEWAY,RULE,SYSTEM_MAINTAINED,GW_NOUPDT,\
    COMMENT,SYNONYM,VGRANT,SECURE",
	    tcb->tcb_rel.relstat);
	TRdisplay("        Tuples: %d Pages Total: %d Primary: %d Main: %d\n",
	    tcb->tcb_rel.reltups, tcb->tcb_rel.relpages, tcb->tcb_rel.relprim,
	    tcb->tcb_rel.relmain);
	TRdisplay("        Location: %~t       Save Date: %d\n",
	    sizeof(tcb->tcb_rel.relloc), &tcb->tcb_rel.relloc, 
	    tcb->tcb_rel.relsave);
	TRdisplay("        Cmpt: %d Create: %x Mod: %x QID1: %d QID2: %d\n",
	    tcb->tcb_rel.relcmptlvl,
	    tcb->tcb_rel.relcreate, tcb->tcb_rel.relmoddate,
	    tcb->tcb_rel.relqid1, tcb->tcb_rel.relqid2);
	TRdisplay("        Index Count: %d  NumLocs: %d\n", 
	    tcb->tcb_rel.relidxcount, tcb->tcb_rel.relloccount);
	TRdisplay("        Ifill: %d Dfill: %d Lfill: %d Min: %d Max: %d\n",
	    tcb->tcb_rel.relifill, tcb->tcb_rel.reldfill, tcb->tcb_rel.rellfill,
	    tcb->tcb_rel.relmin, tcb->tcb_rel.relmax);
	TRdisplay("        Gateway_id: %d Gateway_other: %d\n",
	    tcb->tcb_rel.relgwid, tcb->tcb_rel.relgwother);
	TRdisplay("        LogicalKey(%x,%x)\n",
	    tcb->tcb_rel.relhigh_logkey, tcb->tcb_rel.rellow_logkey);
	TRdisplay("        Allocation: %d Extend: %d Free_header: %d\n",
	    tcb->tcb_rel.relallocation, tcb->tcb_rel.relextend,
	    tcb->tcb_rel.relfhdr);
	TRdisplay("     Timestamp - High: 0x%x  Low: 0x%x\n",
	    tcb->tcb_rel.relstamp12.db_tab_high_time,
	    tcb->tcb_rel.relstamp12.db_tab_low_time);
    }

    TRdisplay("    Table IO Information\n");
    TRdisplay("        Table IO Flags: %v %v\n",
	"VALID,OPEN,PARTIAL", tbio->tbio_status,
	"READONLY,FASTCOMMIT,MULTISERVER,RCP,ROLLFORWARD", 
	tbio->tbio_cache_flags);
    TRdisplay("        DBid: %d  Table Id: (%d,%d)\n",
	tbio->tbio_dcbid, tbio->tbio_reltid.db_tab_base, 
	tbio->tbio_reltid.db_tab_index);
    TRdisplay("        Table Type: %d  %w %w\n",
	tbio->tbio_table_type,
	"PERMANENT,TEMPORARY", tbio->tbio_temporary,
	"USER_REL,SYSTEM_REL", tbio->tbio_sysrel);
    TRdisplay("        Last Initialized Page: %d  Last Allocated Page: %d\n",
	tbio->tbio_lpageno, tbio->tbio_lalloc);
    TRdisplay("        Check EOF flag: %d\n", tbio->tbio_checkeof);
    TRdisplay("        Cache Priority: %d  Valid Stamp0x%x\n", 
	tbio->tbio_cache_priority, tbio->tbio_cache_valid_stamp);
    TRdisplay("    Location Count: %d  Location Array PTR: 0x%x\n", 
	tbio->tbio_loc_count, tbio->tbio_location_array);

    TRdisplay("    INDEX TCBs Next :0x%p  Previous: 0x%p Count: %d\n",
	tcb->tcb_iq_next, tcb->tcb_iq_prev, tcb->tcb_index_count);
    TRdisplay("    FREE     Next: 0x%p   Previous: 0x%p\n",
	tcb->tcb_fq_next, tcb->tcb_fq_prev);
    TRdisplay("    ACTIVE   Next: 0x%p   Previous: 0x%p\n",
	tcb->tcb_rq_next, tcb->tcb_rq_prev);
    TRdisplay("     FREE BASE TCBs  Next: 0x%p  Previous 0x%p\n",
	tcb->tcb_ftcb_list.q_next, tcb->tcb_ftcb_list.q_prev);
    TRdisplay("    Mutex: %#.4{%x %}\n", (sizeof(DM_MUTEX) / 4),
	&tcb->tcb_mutex, 0);
}

/*{
** Name: dump_jnl	- Dump the journal portion of the config file
**
** Description:
**	This function dumps the journal portion of the config file.
**
** Inputs:
**	cnf				- config file information.
**
** Outputs:
**	None
**
**	Returns:
**	    VOID
**
** History:
**	25-sep-1991 (mikem) integrated following change: 30-apr-1991 (bryanp)
**          History created.
**	8-nov-1992 (ed) 
**          remove DB_MAXNAME dependency
**     24-jan-1995 (stial01)
**          BUG 66473: display if ckp_mode indicates TABLE level checkpoint.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Utilize DSC_STATUS, DSC_INCONST_CODE defines.
**	05-Mar-2010 (frima01) SIR 121619
**	    For usability explicitly state that MVCC is enabled.
*/
VOID
dump_jnl(cnf)
DM0C_CNF    *cnf;
{
    i4		i;
    bool                node_info = FALSE;
    STATUS		log_hdr_status;
    LG_HEADER		log_hdr;
    i4		length;
    CL_ERR_DESC         sys_err;

    /* 
    ** Get the log header so we can do log address calculations. Don't worry
    ** if this fails. If it does, we'll bypass the log address calculations.
    */

    log_hdr_status = LGshow(LG_A_HEADER, (PTR)&log_hdr, sizeof(log_hdr),
			&length, &sys_err);

    /*
    ** The flag is 'DSC_DUMP_DIR_EXISTS', but we report it as 'CFG_BACKUP',
    ** since its current use is to enable config file auto-backup.
    */

    TRdisplay("=======================dump jnl================================\n");
    TRdisplay("%18*=%@ Database Information%18*=\n\n");
    TRdisplay("    Database : (%~t,%~t)  ID : 0x%x\n",
	sizeof(cnf->cnf_dsc->dsc_name), &cnf->cnf_dsc->dsc_name,
	sizeof(cnf->cnf_dsc->dsc_owner), &cnf->cnf_dsc->dsc_owner,
        cnf->cnf_dsc->dsc_dbid);
    TRdisplay("    Extents  : %d    Next Table id  : %d\n",
	cnf->cnf_dsc->dsc_ext_count, cnf->cnf_dsc->dsc_last_table_id);
    TRdisplay("    Config File Version Id : 0x%x   Database Version Id : %d\n", 
	cnf->cnf_dsc->dsc_cnf_version, cnf->cnf_dsc->dsc_c_version);
    TRdisplay("    Status   : %v\n\n",
	DSC_STATUS, cnf->cnf_dsc->dsc_status);

    /*
    ** Print database status information comments.
    */

    if (!(cnf->cnf_dsc->dsc_status & DSC_VALID))
    {
        TRdisplay("%15* The Database is Inconsistent.\n");
        TRdisplay("%19* Cause of Inconsistency:  %w\n",
            DSC_INCONST_CODE, cnf->cnf_dsc->dsc_inconst_code);  
    }

    if (cnf->cnf_dsc->dsc_status & DSC_CKP_INPROGRESS)
    {
        TRdisplay("%15* The Database has been Checkpointed.\n");
    }

    if (cnf->cnf_dsc->dsc_status & DSC_JOURNAL)
    {
        TRdisplay("%15* The Database is Journaled.\n");
    }
    else
    {
        TRdisplay("%15* The Database is not Journaled.\n");
    }

    if (cnf->cnf_dsc->dsc_status & DSC_DISABLE_JOURNAL)
    {
        TRdisplay("\n%15* Journaling has been disabled on this database by alterdb.\n");
    }

    if (cnf->cnf_dsc->dsc_dbid != IIDBDB_ID)
    {
        TRdisplay("\n%15* MVCC is %s in this database.\n",
          cnf->cnf_dsc->dsc_status & DSC_NOMVCC ? "disabled" : "enabled");
    }

    if (cnf->cnf_dsc->dsc_status & DSC_NOLOGGING)
    {
        TRdisplay("\n%15* Database is being accessed with Set Nologging, allowing\n");
        TRdisplay("%15* transactions to run while bypassing the logging system.\n");
    }

    /*
    ** Show the earliest checkpoint which journals may be applied to.
    */

    if (cnf->cnf_jnl->jnl_first_jnl_seq != 0)
    {
	TRdisplay("\n%15* Journals are valid from checkpoint sequence : %d\n",
	    cnf->cnf_jnl->jnl_first_jnl_seq);
    }
    else
    {
	TRdisplay("\n%15* Journals are not valid from any checkpoint.\n");
    }

    TRdisplay("\n");
    TRdisplay("----Journal information%57*-\n");
    TRdisplay("    Checkpoint sequence : %10d    Journal sequence : %17d\n",
	cnf->cnf_jnl->jnl_ckp_seq, cnf->cnf_jnl->jnl_fil_seq);
    TRdisplay("    Current journal block : %8d    Journal block size : %15d\n",
	cnf->cnf_jnl->jnl_blk_seq, cnf->cnf_jnl->jnl_bksz);
    TRdisplay("    Initial journal size : %9d    Target journal size : %14d\n",
	cnf->cnf_jnl->jnl_blkcnt, cnf->cnf_jnl->jnl_maxcnt);

    /*
    ** Dump the log address of the last record which has been journaled. If
    ** the log header is available, dump it in <sequence #, page, offset> form;
    ** otherwise, just dump the sequence # and offset.
    */

    if (!log_hdr_status)
    {
	TRdisplay("    Last Log Address Journaled : <%d:%d:%d>\n",
	    cnf->cnf_jnl->jnl_la.la_sequence, 
	    cnf->cnf_jnl->jnl_la.la_block,
	    cnf->cnf_jnl->jnl_la.la_offset);
    }
    else
    {
	TRdisplay("    Last Log Address Journaled : %8x %8x %8x\n",
	    cnf->cnf_jnl->jnl_la.la_sequence,
	    cnf->cnf_jnl->jnl_la.la_block, 
	    cnf->cnf_jnl->jnl_la.la_offset);
    }

    TRdisplay("----Checkpoint History%58*-\n");
    TRdisplay("    Date                      Ckp_sequence  First_jnl   Last_jnl  valid  mode\n    %76*-\n");
    if (cnf->cnf_jnl->jnl_count)
    {
	struct _JNL_CKP *p = 0;

	TRdisplay("%#.#{    %?    %11d  %9d  %8d  %5d  %v\n%}",
	    cnf->cnf_jnl->jnl_count, sizeof(struct _JNL_CKP), cnf->cnf_jnl->jnl_history,
	    &p->ckp_date, &p->ckp_sequence, &p->ckp_f_jnl, &p->ckp_l_jnl,
	    &p->ckp_jnl_valid,"OFFLINE,ONLINE,TABLE",&p->ckp_mode);
    }
    else
	TRdisplay("    None.\n");
    TRdisplay("----Cluster Journal History%53*-\n");
    TRdisplay("    Node ID   Current Journal   Current Block   Last Log Address\n    %60*-\n");
    for (i=0; i < DM_CNODE_MAX; i++)
    {
    	struct _JNL_CNODE_INFO *p = 0;
	p = &cnf->cnf_jnl->jnl_node_info[i];
	
	if (p->cnode_fil_seq || p->cnode_la.la_sequence)
        {	
	    TRdisplay("    %7d   %15d   %13d   %8x %8x %8x\n%",
		i, p->cnode_fil_seq, p->cnode_blk_seq, 
		p->cnode_la.la_sequence, 
		p->cnode_la.la_block,
		p->cnode_la.la_offset);
	    node_info = TRUE;
	}
    }
    if (node_info == FALSE)
	TRdisplay("    None.\n");

    TRdisplay("----Extent directory%60*-\n");
    TRdisplay(
	"    Location                          Flags             Physical_path\
	\n    %66*-\n");
    {
        DM0C_EXT *p = 0;

	TRdisplay("%#.#{    %#.#s  %16v  %.*s\n%}", 
	cnf->cnf_dsc->dsc_ext_count, sizeof(DM0C_EXT), cnf->cnf_ext,
	sizeof(DB_LOC_NAME), sizeof(DB_LOC_NAME), 
	CL_OFFSETOF(DM0C_EXT, ext_location.logical),
	"ROOT,DATA,JOURNAL,CHECKPOINT,ALIAS,WORK,DUMP,AWORK",
	CL_OFFSETOF(DM0C_EXT, ext_location.flags),
	CL_OFFSETOF(DM0C_EXT, ext_location.phys_length),
	CL_OFFSETOF(DM0C_EXT, ext_location.physical));
    }

    TRdisplay("%80*=\n");

}

/*
** Name: dump_dmp		- Trace the "DMP" portion of config file.
**
** History:
**	25-sep-1991 (mikem) integrated following change: 24-apr-1991 (bryanp)
**	    Added history comment, fixed lint complaints.
**	23-aug-1993 (andys)
**	    Use CL_OFFSETOF rather than zero-pointer construction in TRdisplay
**	    statement. Bug caused by VMS C compiler when using /OPT. This fix
**	    is the same as that for bug 37300. [bug 46296]. 
**	    [was 04-sep-1992 (andys)]
**      24-jan-1995 (stial01)
**          BUG 66473: display if ckp_mode indicates TABLE level checkpoint.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Utilize DSC_STATUS, DSC_INCONST_CODE defines.
*/
VOID
dump_dmp(cnf)
DM0C_CNF    *cnf;
{
    STATUS		log_hdr_status;
    LG_HEADER		log_hdr;
    i4		length;
    CL_ERR_DESC         sys_err;

    /* 
    ** Get the log header so we can do log address calculations. Don't worry
    ** if this fails. If it does, we'll bypass the log address calculations.
    */

    log_hdr_status = LGshow(LG_A_HEADER, (PTR)&log_hdr, sizeof(log_hdr),
			&length, &sys_err);

    TRdisplay("=======================dump dmp================================\n");
    TRdisplay("%18*=%@ Database Information%18*=\n\n");
    TRdisplay("    Database : (%~t,%~t)  ID : 0x%x\n",
	sizeof(cnf->cnf_dsc->dsc_name), &cnf->cnf_dsc->dsc_name,
	sizeof(cnf->cnf_dsc->dsc_owner), &cnf->cnf_dsc->dsc_owner,
        cnf->cnf_dsc->dsc_dbid);
    TRdisplay("    Extents  : %d    Next Table id  : %d\n",
	cnf->cnf_dsc->dsc_ext_count, cnf->cnf_dsc->dsc_last_table_id);
    TRdisplay("    Config File Version Id : 0x%x   Database Version Id : %d\n", 
	cnf->cnf_dsc->dsc_cnf_version, cnf->cnf_dsc->dsc_c_version);
    TRdisplay("    Status   : %v\n\n",
	DSC_STATUS, cnf->cnf_dsc->dsc_status);

    if (!(cnf->cnf_dsc->dsc_status & DSC_VALID))
    {
        TRdisplay("%15* The Database is Inconsistent.\n");
        TRdisplay("%19* Cause of Inconsistency:  %w\n",
            DSC_INCONST_CODE, cnf->cnf_dsc->dsc_inconst_code);  
    }

    if (cnf->cnf_dsc->dsc_status & DSC_CKP_INPROGRESS)
    {
        TRdisplay("%15* The Database has been Checkpointed.\n");
    }

    if (cnf->cnf_dsc->dsc_status & DSC_JOURNAL)
    {
        TRdisplay("%15* The Database is Journaled.\n");
    }
    else
    {
        TRdisplay("%15* The Database is not Journaled.\n");
    }

    if (cnf->cnf_dsc->dsc_status & DSC_DISABLE_JOURNAL)
    {
        TRdisplay("\n%15* Journaling has been disabled on this database by alterdb.\n");
    }

    if (cnf->cnf_dsc->dsc_status & DSC_NOMVCC)
    {
        TRdisplay("\n%15* MVCC has been disabled on this database by alterdb.\n");
    }

    if (cnf->cnf_dsc->dsc_status & DSC_NOLOGGING)
    {
        TRdisplay("\n%15* Database is being accessed with Set Nologging, allowing\n");
        TRdisplay("%15* transactions to run while bypassing the logging system.\n");
    }

    /*
    ** Show the earliest checkpoint which journals may be applied to.
    */

    if (cnf->cnf_jnl->jnl_first_jnl_seq != 0)
    {
	TRdisplay("\n%15* Journals are valid from checkpoint sequence : %d\n",
	    cnf->cnf_jnl->jnl_first_jnl_seq);
    }
    else
    {
	TRdisplay("\n%15* Journals are not valid from any checkpoint.\n");
    }

    TRdisplay("\n");
    TRdisplay("----Dump information%60*-\n");
    TRdisplay("    Checkpoint sequence : %7d	Dump sequence : %17d\n",
	cnf->cnf_dmp->dmp_ckp_seq, cnf->cnf_dmp->dmp_fil_seq);
    TRdisplay("    Current dump block : %8d	Dump block size : %15d\n",
	cnf->cnf_dmp->dmp_blk_seq, cnf->cnf_dmp->dmp_bksz);
    TRdisplay("    Initial dump size : %9d	Target dump size : %14d\n",
	cnf->cnf_dmp->dmp_blkcnt, cnf->cnf_dmp->dmp_maxcnt);

    /*
    ** Dump the log address of the last record which has been dumped. If
    ** the log header is available, dump it in <sequence #, page, offset> form;
    ** otherwise, just dump the sequence # and offset.
    */

    if (!log_hdr_status)
    {
	TRdisplay("    Last Log Address Dumped: <%d:%d:%d>\n",
	    cnf->cnf_dmp->dmp_la.la_sequence, 
	    cnf->cnf_dmp->dmp_la.la_block,
	    cnf->cnf_dmp->dmp_la.la_offset);
    }
    else
    {
	TRdisplay("    Last Log Address Dumped: %8x %8x %8x\n",
	    cnf->cnf_dmp->dmp_la.la_sequence,
	    cnf->cnf_dmp->dmp_la.la_block,
	    cnf->cnf_dmp->dmp_la.la_offset);
    }

    TRdisplay("----Checkpoint History%58*-\n");
    TRdisplay("    Date                      Ckp_sequence  First_dmp   Last_dmp  valid  mode\n    %76*-\n");

    if (cnf->cnf_dmp->dmp_count)
    {
	struct _DMP_CKP *p = 0;

	TRdisplay("%#.#{    %?    %11d  %9d  %8d  %5d  %v\n%}",
	    cnf->cnf_dmp->dmp_count, sizeof(struct _DMP_CKP), cnf->cnf_dmp->dmp_history,
	    &p->ckp_date, &p->ckp_sequence, &p->ckp_f_dmp, &p->ckp_l_dmp,
	    &p->ckp_dmp_valid,"OFFLINE,ONLINE,TABLE",&p->ckp_mode);
    }
    else
	TRdisplay("    None.\n");

    TRdisplay("----Extent directory%60*-\n");
    TRdisplay(
	"    Location                          Flags             Physical_path\
	\n    %66*-\n");
    {
        DM0C_EXT *p = 0;

	TRdisplay("%#.#{    %#.#s  %16v  %.*s\n%}", 
	cnf->cnf_dsc->dsc_ext_count, sizeof(DM0C_EXT), cnf->cnf_ext,
	sizeof(DB_LOC_NAME), sizeof(DB_LOC_NAME), 
	CL_OFFSETOF(DM0C_EXT, ext_location.logical),
	"ROOT,DATA,JOURNAL,CHECKPOINT,ALIAS,WORK,DUMP,AWORK",
	CL_OFFSETOF(DM0C_EXT, ext_location.flags),
	CL_OFFSETOF(DM0C_EXT, ext_location.phys_length),
	CL_OFFSETOF(DM0C_EXT, ext_location.physical));
    }

    TRdisplay("%80*=\n");

}
