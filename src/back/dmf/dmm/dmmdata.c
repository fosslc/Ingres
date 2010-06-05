/*
**Copyright (c) 2004, 2010 Ingres Corporation
*/


/* Jam hints
**
LIBRARY = IMPDMFLIBDATA
**
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <di.h>
#include    <lo.h>
#include    <me.h>
#include    <pc.h>
#include    <sl.h>
#include    <tm.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <lk.h>
#include    <lg.h>
#include    <st.h>
#include    <adf.h>
#include    <ulf.h>
#include    <dmf.h>
#include    <dmccb.h>
#include    <dmmcb.h>
#include    <dmrcb.h>
#include    <dmtcb.h>
#include    <dmxcb.h>
#include    <dm.h>
#include    <dml.h>
#include    <dmp.h>
#include    <dmpp.h>
#include    <dm1c.h>
#include    <dm1p.h>
#include    <dm2t.h>
#include    <dm2f.h>
#include    <dm0c.h>
#include    <dm0m.h>
#include    <dm1b.h>
#include    <dm0l.h>
#include    <dm0s.h>
#include    <dmm.h>
#include    <dudbms.h>



/*
** Name:	dmmdata.c
**
** Description:	Global data for dmm facility.
**
** History:
**
**	23-sep-96 (canor01)
**	    Created.
**	24-Mar-1997 (jenjo02)
**	    Table priority project:
**	    Added reltcpri (table cache priority) to DMP_RELATION
**      23-oct-2000 (stial01)
**          Variable Page Size - Variable Page Type support (SIR 102955)
**	18-Dec-2003 (schka24)
**	    Added iirelation stuff for partitioned tables.
**	18-feb-2004 (stial01)
**          iirelation changes for 256k rows, relwid, reltotwid i2->i4
**          iiattribute changes for 256k rows, attoff i2->i4
**          Also changed attfml i2->i4
**	23-Feb-2004 (gupsh01)
**	    Added alter table alter column support.
**	30-Jun-2004 (schka24)
**	    relsecid -> relfree1.
**	29-Sep-2004 (drivi01)
**	    Added LIBRARY jam hint to put this file into IMPDMFLIBDATA
**	    in the Jamfile.
**	13-dec-04 (inkdo01)
**	    Added attcollID.
**	30-May-2006 (jenjo02)
**	    Add 32 more idom attributes to IIINDEX for indexes on
**	    clustered tables.
**	29-Sept-2009 (troal01)
**		Added attgeomtype and attsrid
**	12-Nov-2009 (kschendel) SIR 122882
**	    Change type of relcmptlvl from char(4) to i4.
**	16-Nov-2009 (kschendel) SIR 122890
**	    Make iirelation and iiattribute be "with duplicates".
**	    Dup checking is pointless, given that dmu is already doing
**	    explicit no-dup-checking puts into these catalogs.
**      04-feb-2010 (stial01)
**          Use macros reldef, attdef* and init ucore from core (in dmmcre.c)
**      01-apr-2010 (stial01)
**          Changes for Long IDs, varchar names in catalogs
**      10-Feb-2010 (maspa05) b122651
**          Added TCB2_PHYSLOCK_CONCUR as this is now how we check for
**          the need to use physical locks
**	01-apr-2010 (toumi01) SIR 122403
**	    For encryption add reldatawid, reltotdatawid, relencflags,
**	    relencver, relenckey, attencflags, attencwid.
**	28-apr-2010 (miket) SIR 122403
**	    Change relenckey from char to byte, maybe not critical to do
**	    but better represents the nature of the stored data.
**      29-Apr-2010 
**          Create core catalogs having long ids with compression=(data)
**	4-May-2010 (kschendel)
**	    Make a frozen copy of the final DSC_V9 catalogs;  we'll need it
**	    eventually, for future converters.
*/

/* dmmcre.c */
/*
**  This table contains the initialized IIRELATION records for the
**  core system tables.  At server startup, we space pad names to
**  DB_TAB_MAXNAME positions and fill in the number of attributes per relation.
*/

/* core table ids */

#define NOT_AN_INDEX            0

#define REL_TAB_ID   { DM_B_RELATION_TAB_ID, DM_I_RELATION_TAB_ID }
#define RIDX_TAB_ID  { DM_B_RIDX_TAB_ID, DM_I_RIDX_TAB_ID  }
#define ATT_TAB_ID   { DM_B_ATTRIBUTE_TAB_ID, DM_I_ATTRIBUTE_TAB_ID}
#define IND_TAB_ID { DM_B_INDEX_TAB_ID, DM_I_INDEX_TAB_ID }

/* covers for CL_OFFSETOF */

#define REL_OFFSET( fieldName ) ( CL_OFFSETOF( DMP_RELATION, fieldName ) )
#define RIDX_OFFSET( fieldName ) ( CL_OFFSETOF( DMP_RINDEX, fieldName ) )
#define ATT_OFFSET( fieldName ) ( CL_OFFSETOF( DMP_ATTRIBUTE, fieldName ) )
#define IND_OFFSET( fieldName ) ( CL_OFFSETOF( DMP_INDEX, fieldName ) )

/* uninitialized fields should identify themselves to the developer */

#define DUMMY_ATTRIBUTE_COUNT   -1
#define DUMMY_ATTRIBUTE_NUMBER  -1
#define DUMMY_TUPLE_COUNT       -1

/*
** NOTE there is code in dmverep/dmveput/dmvedel that assumes 
** that reltid is first column in reltid
*/
#define reldef(tabid, relid, relwid, relkeys, relidxcount, relstat, relstat2, relmin, relcomptype)\
    tabid, /* reltid,reltidx */\
    {relid},\
    {"$ingres"},\
    DUMMY_ATTRIBUTE_COUNT,\
    0, /* reltcpri */\
    relkeys,\
    TCB_HASH, /* relspec */\
    relstat,\
    DUMMY_TUPLE_COUNT,\
    1, /* relpages */\
    1, /* relprim */\
    1, /* relmain */\
    0, /* relsave */\
    {0, 0}, /* relstamp1,2 */\
    {"$default"}, /* relloc */\
    DMF_T_VERSION, /* relcmptlvl */\
    0, /* relcreate */\
    0, /* relqid1 */\
    0, /* relqid2 */\
    0, /* relmoddate */\
    relidxcount,\
    0, /* relifill */\
    50, /* reldfill */\
    0, /* rellfill */\
    relmin,\
    0, /* relmax */\
    DM_PG_SIZE, /* relpgsize */\
    0, /* relgwid */\
    0, /* relgwother */\
    1, /* relhigh_logkey */\
    65536, /* rellow_logkey */\
    DM_TBL_DEFAULT_FHDR_PAGENO, /* relfhdr */\
    DM_TBL_DEFAULT_ALLOCATION, /* relallocation */\
    DM_TBL_DEFAULT_EXTEND, /* relextend */\
    relcomptype,\
    TCB_PG_V1, /* relpgtype */\
    relstat2, /* relstat2 */\
    { ' ' }, /* relfree1 */\
    1, /* relloccount */\
    0, /* relversion */\
    relwid, /* relwid */\
    relwid, /* reltotwid */\
    relwid, /* reldatawid */\
    relwid, /* reltotdatawid */\
    0, /* relnparts */\
    0, /* relnpartlevels */\
    0, /* relencflags */\
    0, /* relencver */\
    { ' ' }, /* relenckey */\
    { ' ' } /* relfree */

#define CORE_RELSTAT (TCB_CATALOG | TCB_NOUPDT | TCB_CONCUR | TCB_PROALL | TCB_SECURE | TCB_DUPLICATES)
#define REL_RELSTAT (CORE_RELSTAT | TCB_COMPRESSED)
#define RIDX_RELSTAT (CORE_RELSTAT | TCB_COMPRESSED)
#define ATT_RELSTAT (CORE_RELSTAT | TCB_COMPRESSED)
#define IND_RELSTAT (CORE_RELSTAT)

GLOBALDEF DMP_RELATION DM_core_relations[DM_CORE_REL_CNT] =
{
    {
        /* reltid,relid,relwid,relkeys,relidxcount,relstat,relmin,relcomptype */
	reldef(REL_TAB_ID, "iirelation", sizeof(DMP_RELATION), 1, 1, REL_RELSTAT | TCB_IDXD, TCB2_PHYSLOCK_CONCUR, 16, TCB_C_STANDARD)
    },
    {
	reldef(RIDX_TAB_ID, "iirel_idx", sizeof(DMP_RINDEX), 2, 0, RIDX_RELSTAT | TCB_INDEX, TCB2_PHYSLOCK_CONCUR, 8, TCB_C_STANDARD)
    },
    {
	reldef(ATT_TAB_ID, "iiattribute", sizeof(DMP_ATTRIBUTE), 2, 0, ATT_RELSTAT, TCB2_PHYSLOCK_CONCUR, 32, TCB_C_STANDARD)
    },
    {
	reldef(IND_TAB_ID, "iiindex", DMP_INDEX_SIZE, 1, 0, IND_RELSTAT, TCB2_PHYSLOCK_CONCUR, 8, TCB_C_NONE)
    },
};

GLOBALDEF DMP_RELATION DM_ucore_relations[DM_CORE_REL_CNT];



/*
**  This table contains the initialized IIATTRIBUTE records for the
**  core system tables.
*/
#define attdef_int(tabid, name, offset, len, iskey)\
 tabid, DUMMY_ATTRIBUTE_NUMBER, 0, name, offset, len, iskey, 0, {DB_DEF_ID_0}, 0, 0,0,0, ATT_INT, 0,0,0,0,0,0,0, {' '}

#define attdef_cha(tabid, name, offset, len, iskey)\
tabid, DUMMY_ATTRIBUTE_NUMBER, 0, name, offset, len, iskey, 0, {DB_DEF_ID_BLANK}, 0, 0,0,0, ATT_CHA, 0,0,0,0,0,0,0, {' '}

#define attdef_byte(tabid, name, offset, len, iskey)\
tabid, DUMMY_ATTRIBUTE_NUMBER, 0, name, offset, len, iskey, 0, {DB_DEF_ID_BLANK}, 0, 0,0,0, ATT_BYTE, 0,0,0,0,0,0,0, {' '}

#define attdef_free(tabid, name, offset, len, iskey)\
tabid, DUMMY_ATTRIBUTE_NUMBER, 0, name, offset, len, iskey, 0, {DB_DEF_ID_0}, 0, 0,0,0, ATT_CHA, 0,0,0,0,0,0,0, {' '}

GLOBALDEF DMP_ATTRIBUTE DM_core_attributes[] =
{
    /* tabid, attname, offset, len, iskey */
    { attdef_int(REL_TAB_ID, "reltid", REL_OFFSET(reltid.db_tab_base), 4, 1) },
    { attdef_int(REL_TAB_ID, "reltidx", REL_OFFSET(reltid.db_tab_index), 4, 0) },
    { attdef_cha(REL_TAB_ID, "relid", REL_OFFSET(relid), DB_TAB_MAXNAME, 0) },
    { attdef_cha(REL_TAB_ID, "relowner", REL_OFFSET(relowner), DB_OWN_MAXNAME, 0) },
    { attdef_int(REL_TAB_ID, "relatts", REL_OFFSET(relatts), 2, 0) },
    { attdef_int(REL_TAB_ID, "reltcpri", REL_OFFSET(reltcpri), 2, 0) },
    { attdef_int(REL_TAB_ID, "relkeys", REL_OFFSET(relkeys), 2, 0) },
    { attdef_int(REL_TAB_ID, "relspec", REL_OFFSET(relspec), 2, 0) },
    { attdef_int(REL_TAB_ID, "relstat", REL_OFFSET(relstat), 4, 0) },
    { attdef_int(REL_TAB_ID, "reltups", REL_OFFSET(reltups), 4, 0) },
    { attdef_int(REL_TAB_ID, "relpages", REL_OFFSET(relpages), 4, 0) },
    { attdef_int(REL_TAB_ID, "relprim", REL_OFFSET(relprim), 4, 0) },
    { attdef_int(REL_TAB_ID, "relmain", REL_OFFSET(relmain), 4, 0) },
    { attdef_int(REL_TAB_ID, "relsave", REL_OFFSET(relsave), 4, 0) },
    { attdef_int(REL_TAB_ID, "relstamp1", REL_OFFSET(relstamp12.db_tab_high_time), 4, 0) },
    { attdef_int(REL_TAB_ID, "relstamp2", REL_OFFSET(relstamp12.db_tab_low_time), 4, 0) },
    { attdef_cha(REL_TAB_ID, "relloc", REL_OFFSET(relloc), DB_LOC_MAXNAME, 0) },
    { attdef_int(REL_TAB_ID, "relcmptlvl", REL_OFFSET(relcmptlvl), 4, 0) },
    { attdef_int(REL_TAB_ID, "relcreate", REL_OFFSET(relcreate), 4, 0) },
    { attdef_int(REL_TAB_ID, "relqid1", REL_OFFSET(relqid1), 4, 0) },
    { attdef_int(REL_TAB_ID, "relqid2", REL_OFFSET(relqid2), 4, 0) },
    { attdef_int(REL_TAB_ID, "relmoddate", REL_OFFSET(relmoddate), 4, 0) },
    { attdef_int(REL_TAB_ID, "relidxcount", REL_OFFSET(relidxcount), 2, 0) },
    { attdef_int(REL_TAB_ID, "relifill", REL_OFFSET(relifill), 2, 0) },
    { attdef_int(REL_TAB_ID, "reldfill", REL_OFFSET(reldfill), 2, 0) },
    { attdef_int(REL_TAB_ID, "rellfill", REL_OFFSET(rellfill), 2, 0) },
    { attdef_int(REL_TAB_ID, "relmin", REL_OFFSET(relmin), 4, 0) },
    { attdef_int(REL_TAB_ID, "relmax", REL_OFFSET(relmax), 4, 0) },
    { attdef_int(REL_TAB_ID, "relpgsize", REL_OFFSET(relpgsize), 4, 0) },
    { attdef_int(REL_TAB_ID, "relgwid", REL_OFFSET(relgwid), 2, 0) },
    { attdef_int(REL_TAB_ID, "relgwother", REL_OFFSET(relgwother), 2, 0) },
    { attdef_int(REL_TAB_ID, "relhigh_logkey", REL_OFFSET(relhigh_logkey), 4, 0) },
    { attdef_int(REL_TAB_ID, "rellow_logkey", REL_OFFSET(rellow_logkey), 4, 0) },
    { attdef_int(REL_TAB_ID, "relfhdr", REL_OFFSET(relfhdr), 4, 0) },
    { attdef_int(REL_TAB_ID, "relallocation", REL_OFFSET(relallocation), 4, 0) },
    { attdef_int(REL_TAB_ID, "relextend", REL_OFFSET(relextend), 4, 0) },
    { attdef_int(REL_TAB_ID, "relcomptype", REL_OFFSET(relcomptype), 2, 0) },
    { attdef_int(REL_TAB_ID, "relpgtype", REL_OFFSET(relpgtype), 2, 0) },
    { attdef_int(REL_TAB_ID, "relstat2", REL_OFFSET(relstat2), 4, 0) },
    { attdef_free(REL_TAB_ID, "relfree1", REL_OFFSET(relfree1), 8, 0) },
    { attdef_int(REL_TAB_ID, "relloccount", REL_OFFSET(relloccount), 2, 0) },
    { attdef_int(REL_TAB_ID, "relversion", REL_OFFSET(relversion), 2, 0) },
    { attdef_int(REL_TAB_ID, "relwid", REL_OFFSET(relwid), 4, 0) },
    { attdef_int(REL_TAB_ID, "reltotwid", REL_OFFSET(reltotwid), 4, 0) },
    { attdef_int(REL_TAB_ID, "reldatawid", REL_OFFSET(reldatawid), 4, 0) },
    { attdef_int(REL_TAB_ID, "reltotdatawid", REL_OFFSET(reltotdatawid), 4, 0) },
    { attdef_int(REL_TAB_ID, "relnparts", REL_OFFSET(relnparts), 2, 0) },
    { attdef_int(REL_TAB_ID, "relnpartlevels", REL_OFFSET(relnpartlevels), 2, 0) },
    { attdef_int(REL_TAB_ID, "relencflags", REL_OFFSET(relencflags), 2, 0) },
    { attdef_int(REL_TAB_ID, "relencver", REL_OFFSET(relencver), 2, 0) },
    { attdef_byte(REL_TAB_ID, "relenckey", REL_OFFSET(relenckey), 64, 0) },
    { attdef_free(REL_TAB_ID, "relfree", REL_OFFSET(relfree), 12, 0) },
    { attdef_cha(RIDX_TAB_ID, "relid", RIDX_OFFSET(relname), DB_TAB_MAXNAME, 1) },
    { attdef_cha(RIDX_TAB_ID, "relowner", RIDX_OFFSET(relowner), DB_OWN_MAXNAME, 2) },
    { attdef_int(RIDX_TAB_ID, "tidp", RIDX_OFFSET(tidp), 4, 0) },
    { attdef_int(ATT_TAB_ID, "attrelid", ATT_OFFSET(attrelid.db_tab_base), 4, 1) },
    { attdef_int(ATT_TAB_ID, "attrelidx", ATT_OFFSET(attrelid.db_tab_index), 4, 2) },
    { attdef_int(ATT_TAB_ID, "attid", ATT_OFFSET(attid), 2, 0) },
    { attdef_int(ATT_TAB_ID, "attxtra", ATT_OFFSET(attxtra), 2, 0) },
    { attdef_cha(ATT_TAB_ID, "attname", ATT_OFFSET(attname), DB_ATT_MAXNAME, 0) },
    { attdef_int(ATT_TAB_ID, "attoff", ATT_OFFSET(attoff), 4, 0) },
    { attdef_int(ATT_TAB_ID, "attfrml", ATT_OFFSET(attfml), 4, 0) },
    { attdef_int(ATT_TAB_ID, "attkdom", ATT_OFFSET(attkey), 2, 0) },
    { attdef_int(ATT_TAB_ID, "attflag", ATT_OFFSET(attflag), 2, 0) },
    { attdef_int(ATT_TAB_ID, "attdefid1", ATT_OFFSET(attDefaultID.db_tab_base), 4, 0) },
    { attdef_int(ATT_TAB_ID, "attdefid2", ATT_OFFSET(attDefaultID.db_tab_index), 4, 0) },
    { attdef_int(ATT_TAB_ID, "attintl_id", ATT_OFFSET(attintl_id), 2, 0) },
    { attdef_int(ATT_TAB_ID, "attver_added", ATT_OFFSET(attver_added), 2, 0) },
    { attdef_int(ATT_TAB_ID, "attver_dropped", ATT_OFFSET(attver_dropped), 2, 0) },
    { attdef_int(ATT_TAB_ID, "attval_from", ATT_OFFSET(attval_from), 2, 0) },
    { attdef_int(ATT_TAB_ID, "attfrmt", ATT_OFFSET(attfmt), 2, 0) },
    { attdef_int(ATT_TAB_ID, "attfrmp", ATT_OFFSET(attfmp), 2, 0) },
    { attdef_int(ATT_TAB_ID, "attver_altcol", ATT_OFFSET(attver_altcol), 2, 0) },
    { attdef_int(ATT_TAB_ID, "attcollid", ATT_OFFSET(attcollID), 2, 0) },
    { attdef_int(ATT_TAB_ID, "attsrid", ATT_OFFSET(attsrid), 4, 0) },
    { attdef_int(ATT_TAB_ID, "attgeomtype", ATT_OFFSET(attgeomtype), 2, 0) },
    { attdef_int(ATT_TAB_ID, "attencflags", ATT_OFFSET(attencflags), 2, 0) },
    { attdef_int(ATT_TAB_ID, "attencwid", ATT_OFFSET(attencwid), 4, 0) },
    { attdef_free(ATT_TAB_ID, "attfree", ATT_OFFSET(attfree), 4, 0) },
    { attdef_int(IND_TAB_ID, "baseid", IND_OFFSET(baseid), 4, 1) },
    { attdef_int(IND_TAB_ID, "indexid", IND_OFFSET(indexid), 4, 0) },
    { attdef_int(IND_TAB_ID, "sequence", IND_OFFSET(sequence), 2, 0) },
    { attdef_int(IND_TAB_ID, "idom1", IND_OFFSET(idom[0]), 2, 0) },
    { attdef_int(IND_TAB_ID, "idom2", IND_OFFSET(idom[1]), 2, 0) },
    { attdef_int(IND_TAB_ID, "idom3", IND_OFFSET(idom[2]), 2, 0) },
    { attdef_int(IND_TAB_ID, "idom4", IND_OFFSET(idom[3]), 2, 0) },
    { attdef_int(IND_TAB_ID, "idom5", IND_OFFSET(idom[4]), 2, 0) },
    { attdef_int(IND_TAB_ID, "idom6", IND_OFFSET(idom[5]), 2, 0) },
    { attdef_int(IND_TAB_ID, "idom7", IND_OFFSET(idom[6]), 2, 0) },
    { attdef_int(IND_TAB_ID, "idom8", IND_OFFSET(idom[7]), 2, 0) },
    { attdef_int(IND_TAB_ID, "idom9", IND_OFFSET(idom[8]), 2, 0) },
    { attdef_int(IND_TAB_ID, "idom10", IND_OFFSET(idom[9]), 2, 0) },
    { attdef_int(IND_TAB_ID, "idom11", IND_OFFSET(idom[10]), 2, 0) },
    { attdef_int(IND_TAB_ID, "idom12", IND_OFFSET(idom[11]), 2, 0) },
    { attdef_int(IND_TAB_ID, "idom13", IND_OFFSET(idom[12]), 2, 0) },
    { attdef_int(IND_TAB_ID, "idom14", IND_OFFSET(idom[13]), 2, 0) },
    { attdef_int(IND_TAB_ID, "idom15", IND_OFFSET(idom[14]), 2, 0) },
    { attdef_int(IND_TAB_ID, "idom16", IND_OFFSET(idom[15]), 2, 0) },
    { attdef_int(IND_TAB_ID, "idom17", IND_OFFSET(idom[16]), 2, 0) },
    { attdef_int(IND_TAB_ID, "idom18", IND_OFFSET(idom[17]), 2, 0) },
    { attdef_int(IND_TAB_ID, "idom19", IND_OFFSET(idom[18]), 2, 0) },
    { attdef_int(IND_TAB_ID, "idom20", IND_OFFSET(idom[19]), 2, 0) },
    { attdef_int(IND_TAB_ID, "idom21", IND_OFFSET(idom[20]), 2, 0) },
    { attdef_int(IND_TAB_ID, "idom22", IND_OFFSET(idom[21]), 2, 0) },
    { attdef_int(IND_TAB_ID, "idom23", IND_OFFSET(idom[22]), 2, 0) },
    { attdef_int(IND_TAB_ID, "idom24", IND_OFFSET(idom[23]), 2, 0) },
    { attdef_int(IND_TAB_ID, "idom25", IND_OFFSET(idom[24]), 2, 0) },
    { attdef_int(IND_TAB_ID, "idom26", IND_OFFSET(idom[25]), 2, 0) },
    { attdef_int(IND_TAB_ID, "idom27", IND_OFFSET(idom[26]), 2, 0) },
    { attdef_int(IND_TAB_ID, "idom28", IND_OFFSET(idom[27]), 2, 0) },
    { attdef_int(IND_TAB_ID, "idom29", IND_OFFSET(idom[28]), 2, 0) },
    { attdef_int(IND_TAB_ID, "idom30", IND_OFFSET(idom[29]), 2, 0) },
    { attdef_int(IND_TAB_ID, "idom31", IND_OFFSET(idom[30]), 2, 0) },
    { attdef_int(IND_TAB_ID, "idom32", IND_OFFSET(idom[31]), 2, 0) },
#ifdef NOT_UNTIL_CLUSTERED_INDEXES
    { attdef_int(IND_TAB_ID, "idom33", IND_OFFSET(idom[32]), 2, 0) },
    { attdef_int(IND_TAB_ID, "idom34", IND_OFFSET(idom[33]), 2, 0) },
    { attdef_int(IND_TAB_ID, "idom35", IND_OFFSET(idom[34]), 2, 0) },
    { attdef_int(IND_TAB_ID, "idom36", IND_OFFSET(idom[35]), 2, 0) },
    { attdef_int(IND_TAB_ID, "idom37", IND_OFFSET(idom[36]), 2, 0) },
    { attdef_int(IND_TAB_ID, "idom38", IND_OFFSET(idom[37]), 2, 0) },
    { attdef_int(IND_TAB_ID, "idom39", IND_OFFSET(idom[38]), 2, 0) },
    { attdef_int(IND_TAB_ID, "idom40", IND_OFFSET(idom[39]), 2, 0) },
    { attdef_int(IND_TAB_ID, "idom41", IND_OFFSET(idom[40]), 2, 0) },
    { attdef_int(IND_TAB_ID, "idom42", IND_OFFSET(idom[41]), 2, 0) },
    { attdef_int(IND_TAB_ID, "idom43", IND_OFFSET(idom[42]), 2, 0) },
    { attdef_int(IND_TAB_ID, "idom44", IND_OFFSET(idom[43]), 2, 0) },
    { attdef_int(IND_TAB_ID, "idom45", IND_OFFSET(idom[44]), 2, 0) },
    { attdef_int(IND_TAB_ID, "idom46", IND_OFFSET(idom[45]), 2, 0) },
    { attdef_int(IND_TAB_ID, "idom47", IND_OFFSET(idom[46]), 2, 0) },
    { attdef_int(IND_TAB_ID, "idom48", IND_OFFSET(idom[47]), 2, 0) },
    { attdef_int(IND_TAB_ID, "idom49", IND_OFFSET(idom[48]), 2, 0) },
    { attdef_int(IND_TAB_ID, "idom50", IND_OFFSET(idom[49]), 2, 0) },
    { attdef_int(IND_TAB_ID, "idom51", IND_OFFSET(idom[50]), 2, 0) },
    { attdef_int(IND_TAB_ID, "idom52", IND_OFFSET(idom[51]), 2, 0) },
    { attdef_int(IND_TAB_ID, "idom53", IND_OFFSET(idom[52]), 2, 0) },
    { attdef_int(IND_TAB_ID, "idom54", IND_OFFSET(idom[53]), 2, 0) },
    { attdef_int(IND_TAB_ID, "idom55", IND_OFFSET(idom[54]), 2, 0) },
    { attdef_int(IND_TAB_ID, "idom56", IND_OFFSET(idom[55]), 2, 0) },
    { attdef_int(IND_TAB_ID, "idom57", IND_OFFSET(idom[56]), 2, 0) },
    { attdef_int(IND_TAB_ID, "idom58", IND_OFFSET(idom[57]), 2, 0) },
    { attdef_int(IND_TAB_ID, "idom59", IND_OFFSET(idom[58]), 2, 0) },
    { attdef_int(IND_TAB_ID, "idom60", IND_OFFSET(idom[59]), 2, 0) },
    { attdef_int(IND_TAB_ID, "idom61", IND_OFFSET(idom[60]), 2, 0) },
    { attdef_int(IND_TAB_ID, "idom62", IND_OFFSET(idom[61]), 2, 0) },
    { attdef_int(IND_TAB_ID, "idom63", IND_OFFSET(idom[62]), 2, 0) },
    { attdef_int(IND_TAB_ID, "idom64", IND_OFFSET(idom[63]), 2, 0) },
#endif /* NOT_UNTIL_CLUSTERED_INDEXES */
};

#define CORE_ATT_COUNT (sizeof(DM_core_attributes)/ sizeof(DMP_ATTRIBUTE))
GLOBALDEF i4 DM_core_att_cnt = CORE_ATT_COUNT; 
GLOBALDEF DMP_ATTRIBUTE DM_ucore_attributes[CORE_ATT_COUNT];


/*
**  This table contains the initialized Relation index records for the
**  core system tables.
*/

GLOBALDEF DMP_RINDEX DM_core_rindex[DM_CORE_RINDEX_CNT] =
{
    { "iirelation", "$ingres", 0 },
    { "iirel_idx", "$ingres", 1 },
    { "iiattribute", "$ingres", 2 },
    { "iiindex", "$ingres", 3 }
};

GLOBALDEF DMP_RINDEX DM_ucore_rindex[DM_CORE_RINDEX_CNT];

/*
**  This table contains the initialized IIINDEX records for the
**  core system tables.
*/


GLOBALDEF DMP_INDEX DM_core_index[] =
{
    {
        DM_B_RIDX_TAB_ID, DM_I_RIDX_TAB_ID, 0, { 3, 4 }
    }
};

#define CORE_INDEX_COUNT (sizeof(DM_core_index)/ sizeof(DMP_INDEX))
GLOBALDEF i4 DM_core_index_cnt = CORE_INDEX_COUNT;
GLOBALDEF DMP_INDEX DM_ucore_index[CORE_INDEX_COUNT];


/* ------------------------------------------------------------- */
/*
** What follows are frozen copies of compressed-catalog attributes,
** for core catalog versions >= DSC_V9.
**
** The reason that these exist is that the core catalog converter in dm2d
** has to build a DMP_ROWACCESS for decompressing old-format rows.
** Since they are in fact old format, the row-accessor has to be built
** using the list of attributes that are in the old version catalog.
** Those attributes are archived here.
**
** We only need attributes for iirelation, iirel_idx, and iiattribute;
** iiindex is not compressed and doesn't need a row-accessor.
**
** The row-accessor builder only needs a few items (data type and length,
** mostly).  Names are unimportant, and there is no need for both
** upper- and lower-case versions in the frozen copies.  Offsets are
** also unimportant, and indeed would be incorrect in reference to the
** old-version catalog, so all offsets are set to -1 to prevent mistakes.
**
** And finally, since these are FROZEN, all lengths are defined in terms
** of the final V9 values, rather than using DB_TAB_MAXNAME etc.
*/

GLOBALDEF DMP_ATTRIBUTE DM_frozen_V9_attributes[] =
{
    /* tabid, attname, offset, len, iskey */
    { attdef_int(REL_TAB_ID, "reltid", -1, 4, 1) },
    { attdef_int(REL_TAB_ID, "reltidx", -1, 4, 0) },
    { attdef_cha(REL_TAB_ID, "relid", -1, 256, 0) },
    { attdef_cha(REL_TAB_ID, "relowner", -1, 32, 0) },
    { attdef_int(REL_TAB_ID, "relatts", -1, 2, 0) },
    { attdef_int(REL_TAB_ID, "reltcpri", -1, 2, 0) },
    { attdef_int(REL_TAB_ID, "relkeys", -1, 2, 0) },
    { attdef_int(REL_TAB_ID, "relspec", -1, 2, 0) },
    { attdef_int(REL_TAB_ID, "relstat", -1, 4, 0) },
    { attdef_int(REL_TAB_ID, "reltups", -1, 4, 0) },
    { attdef_int(REL_TAB_ID, "relpages", -1, 4, 0) },
    { attdef_int(REL_TAB_ID, "relprim", -1, 4, 0) },
    { attdef_int(REL_TAB_ID, "relmain", -1, 4, 0) },
    { attdef_int(REL_TAB_ID, "relsave", -1, 4, 0) },
    { attdef_int(REL_TAB_ID, "relstamp1", -1, 4, 0) },
    { attdef_int(REL_TAB_ID, "relstamp2", -1, 4, 0) },
    { attdef_cha(REL_TAB_ID, "relloc", -1, 32, 0) },
    { attdef_int(REL_TAB_ID, "relcmptlvl", -1, 4, 0) },
    { attdef_int(REL_TAB_ID, "relcreate", -1, 4, 0) },
    { attdef_int(REL_TAB_ID, "relqid1", -1, 4, 0) },
    { attdef_int(REL_TAB_ID, "relqid2", -1, 4, 0) },
    { attdef_int(REL_TAB_ID, "relmoddate", -1, 4, 0) },
    { attdef_int(REL_TAB_ID, "relidxcount", -1, 2, 0) },
    { attdef_int(REL_TAB_ID, "relifill", -1, 2, 0) },
    { attdef_int(REL_TAB_ID, "reldfill", -1, 2, 0) },
    { attdef_int(REL_TAB_ID, "rellfill", -1, 2, 0) },
    { attdef_int(REL_TAB_ID, "relmin", -1, 4, 0) },
    { attdef_int(REL_TAB_ID, "relmax", -1, 4, 0) },
    { attdef_int(REL_TAB_ID, "relpgsize", -1, 4, 0) },
    { attdef_int(REL_TAB_ID, "relgwid", -1, 2, 0) },
    { attdef_int(REL_TAB_ID, "relgwother", -1, 2, 0) },
    { attdef_int(REL_TAB_ID, "relhigh_logkey", -1, 4, 0) },
    { attdef_int(REL_TAB_ID, "rellow_logkey", -1, 4, 0) },
    { attdef_int(REL_TAB_ID, "relfhdr", -1, 4, 0) },
    { attdef_int(REL_TAB_ID, "relallocation", -1, 4, 0) },
    { attdef_int(REL_TAB_ID, "relextend", -1, 4, 0) },
    { attdef_int(REL_TAB_ID, "relcomptype", -1, 2, 0) },
    { attdef_int(REL_TAB_ID, "relpgtype", -1, 2, 0) },
    { attdef_int(REL_TAB_ID, "relstat2", -1, 4, 0) },
    { attdef_free(REL_TAB_ID, "relfree1", -1, 8, 0) },
    { attdef_int(REL_TAB_ID, "relloccount", -1, 2, 0) },
    { attdef_int(REL_TAB_ID, "relversion", -1, 2, 0) },
    { attdef_int(REL_TAB_ID, "relwid", -1, 4, 0) },
    { attdef_int(REL_TAB_ID, "reltotwid", -1, 4, 0) },
    { attdef_int(REL_TAB_ID, "reldatawid", -1, 4, 0) },
    { attdef_int(REL_TAB_ID, "reltotdatawid", -1, 4, 0) },
    { attdef_int(REL_TAB_ID, "relnparts", -1, 2, 0) },
    { attdef_int(REL_TAB_ID, "relnpartlevels", -1, 2, 0) },
    { attdef_int(REL_TAB_ID, "relencflags", -1, 2, 0) },
    { attdef_int(REL_TAB_ID, "relencver", -1, 2, 0) },
    { attdef_byte(REL_TAB_ID, "relenckey", -1, 64, 0) },
    { attdef_free(REL_TAB_ID, "relfree", -1, 12, 0) },
    { attdef_cha(RIDX_TAB_ID, "relid", -1, 256, 1) },
    { attdef_cha(RIDX_TAB_ID, "relowner", -1, 32, 2) },
    { attdef_int(RIDX_TAB_ID, "tidp", -1, 4, 0) },
    { attdef_int(ATT_TAB_ID, "attrelid", -1, 4, 1) },
    { attdef_int(ATT_TAB_ID, "attrelidx", -1, 4, 2) },
    { attdef_int(ATT_TAB_ID, "attid", -1, 2, 0) },
    { attdef_int(ATT_TAB_ID, "attxtra", -1, 2, 0) },
    { attdef_cha(ATT_TAB_ID, "attname", -1, 256, 0) },
    { attdef_int(ATT_TAB_ID, "attoff", -1, 4, 0) },
    { attdef_int(ATT_TAB_ID, "attfrml", -1, 4, 0) },
    { attdef_int(ATT_TAB_ID, "attkdom", -1, 2, 0) },
    { attdef_int(ATT_TAB_ID, "attflag", -1, 2, 0) },
    { attdef_int(ATT_TAB_ID, "attdefid1", -1, 4, 0) },
    { attdef_int(ATT_TAB_ID, "attdefid2", -1, 4, 0) },
    { attdef_int(ATT_TAB_ID, "attintl_id", -1, 2, 0) },
    { attdef_int(ATT_TAB_ID, "attver_added", -1, 2, 0) },
    { attdef_int(ATT_TAB_ID, "attver_dropped", -1, 2, 0) },
    { attdef_int(ATT_TAB_ID, "attval_from", -1, 2, 0) },
    { attdef_int(ATT_TAB_ID, "attfrmt", -1, 2, 0) },
    { attdef_int(ATT_TAB_ID, "attfrmp", -1, 2, 0) },
    { attdef_int(ATT_TAB_ID, "attver_altcol", -1, 2, 0) },
    { attdef_int(ATT_TAB_ID, "attcollid", -1, 2, 0) },
    { attdef_int(ATT_TAB_ID, "attsrid", -1, 4, 0) },
    { attdef_int(ATT_TAB_ID, "attgeomtype", -1, 2, 0) },
    { attdef_int(ATT_TAB_ID, "attencflags", -1, 2, 0) },
    { attdef_int(ATT_TAB_ID, "attencwid", -1, 4, 0) },
    { attdef_free(ATT_TAB_ID, "attfree", -1, 4, 0) },
};

GLOBALDEF i4 DM_frozen_V9_att_cnt = sizeof(DM_frozen_V9_attributes) / sizeof(DMP_ATTRIBUTE); 
