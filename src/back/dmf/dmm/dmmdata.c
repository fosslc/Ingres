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
**	06-May-2010
**	    Remove unused variable.
**	11-may-2010 (stephenb)
**	    iirelation changes: make reltups and relpages i8, move relstat2
**	    next to relstat to make correct i8 bus alignment, move relid,
**	    relowner, and relloc to the end of the table to avoid offset
**	    issues with compression.
**      14-May-2010 (stial01)
**          Add relattnametot to iirelation, move attname to end of iiattribute
**          Clean up attdef macro, define relextra,attexta as byte
**	9-Jul-2010 (kschendel) SIR 123450
**	    Core catalogs use OLD STANDARD compression.  dm2d isn't quite
**	    smart enough to figure out multiple core-catalog compression types,
**	    and now is not the time to teach it.
*/

/* dmmcre.c */
/*
**  This table contains the initialized IIRELATION records for the
**  core system tables.  At server startup, we space pad names to
**  DB_TAB_MAXNAME positions and fill in the number of attributes per relation.
**  
**  *** WARNING WARNING WARNING *****
**  If you make alterations to any core catalogs you must alter the
**  verifydb versions of their structures in the various SQL
**  header files in dbutil!duf!hdr
*/

/* core table ids */

#define NOT_AN_INDEX            0

#define REL_TABID   { DM_B_RELATION_TAB_ID, DM_I_RELATION_TAB_ID }
#define RIDX_TABID  { DM_B_RIDX_TAB_ID, DM_I_RIDX_TAB_ID  }
#define ATT_TABID   { DM_B_ATTRIBUTE_TAB_ID, DM_I_ATTRIBUTE_TAB_ID}
#define IND_TABID { DM_B_INDEX_TAB_ID, DM_I_INDEX_TAB_ID }

/* covers for CL_OFFSETOF */

#define REL_OFFSET( fieldName ) ( CL_OFFSETOF( DMP_RELATION, fieldName ) )
#define RIDX_OFFSET( fieldName ) ( CL_OFFSETOF( DMP_RINDEX, fieldName ) )
#define ATT_OFFSET( fieldName ) ( CL_OFFSETOF( DMP_ATTRIBUTE, fieldName ) )
#define IND_OFFSET( fieldName ) ( CL_OFFSETOF( DMP_INDEX, fieldName ) )

/* uninitialized fields should identify themselves to the developer */

#define DUMMY_RELATTS	-1
#define DUMMY_RELTUPS	-1
#define DUMMY_RELNAMESZ	-1
#define DUMMY_ATTID	-1
#define DUMMY_RELTUPS	-1
#define DUMMY_DEFID	{0,0}

/*
** NOTE there is code in dmverep/dmveput/dmvedel that assumes 
** that reltid is first column in reltid
** NOTE: the location of relid and relowner are defined in
** DM_RELID_FIELD_NO and DM_RELOWNER_FIELD_NO in dmm.h; these
** values are used in dm2d.c to bootsrap iirel_idx when
** the database is opened. If you move either of these
** fields or add new fields in front of them (more likely),
** remember to update the definitions.
** NOTE: any changes to iirelation must be reflected in the
** verifydb version of the structure in dbutil!duf!hdr duverel.sh
** DUMMY fields are initialized in dmm_init_catalog_templates()
*/
#define reldef(tabid, relid, relwid, relkeys, relidxcount, relstat, relmin, relcomptype)\
    tabid, /* reltid,reltidx */\
    DUMMY_RELATTS,\
    0, /* reltcpri */\
    relkeys,\
    TCB_HASH, /* relspec */\
    relstat,\
    TCB2_PHYSLOCK_CONCUR, /* relstat2 */\
    DUMMY_RELTUPS,\
    1, /* relpages */\
    1, /* relprim */\
    1, /* relmain */\
    0, /* relsave */\
    {0, 0}, /* relstamp1,2 */\
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
    1, /* relloccount */\
    0, /* relversion */\
    relwid, /* relwid */\
    relwid, /* reltotwid */\
    relwid, /* reldatawid */\
    relwid, /* reltotdatawid */\
    DUMMY_RELNAMESZ, /* relattnametot */\
    0, /* relnparts */\
    0, /* relnpartlevels */\
    0, /* relencflags */\
    0, /* relencver */\
    { ' ' }, /* relenckey */\
    { ' ' }, /* relfree */\
    {"$default"}, /* relloc */\
    {"$ingres"},/* relowner */\
    {relid}
 
#define CORE_RELSTAT (TCB_CATALOG | TCB_NOUPDT | TCB_CONCUR | TCB_PROALL | TCB_SECURE | TCB_DUPLICATES)
#define REL_RELSTAT (CORE_RELSTAT | TCB_COMPRESSED | TCB_IDXD)
#define RIDX_RELSTAT (CORE_RELSTAT | TCB_COMPRESSED | TCB_INDEX)
#define ATT_RELSTAT (CORE_RELSTAT | TCB_COMPRESSED)
#define IND_RELSTAT (CORE_RELSTAT)

GLOBALDEF DMP_RELATION DM_core_relations[DM_CORE_REL_CNT] =
{
    {
        /* reltid,relid,relwid,relkeys,relidxcount,relstat,relmin,relcomptype */
	reldef(REL_TABID, "iirelation", sizeof(DMP_RELATION), 1, 1, REL_RELSTAT, 16, TCB_C_STD_OLD)
    },
    {
	reldef(RIDX_TABID, "iirel_idx", sizeof(DMP_RINDEX), 2, 0, RIDX_RELSTAT,  8, TCB_C_STD_OLD)
    },
    {
	reldef(ATT_TABID, "iiattribute", sizeof(DMP_ATTRIBUTE), 2, 0, ATT_RELSTAT, 32, TCB_C_STD_OLD)
    },
    {
	reldef(IND_TABID, "iiindex", DMP_INDEX_SIZE, 1, 0, IND_RELSTAT, 8, TCB_C_NONE)
    },
};

GLOBALDEF DMP_RELATION DM_ucore_relations[DM_CORE_REL_CNT];



/*
**  This table contains the initialized IIATTRIBUTE records for the
**  core system tables.
**  NOTE: Any changes to iiattribute must be reflected in the 
**  verifydb structure in dbutil!duf!hdr duveatt.sh
**  DUMMY fields are initialized in dmm_init_catalog_templates()
*/
#define attdef(tabid, type, name, offset, len, kdom)\
 tabid,/* attrelid, attrelidx */ \
 DUMMY_ATTID, /* attid */ \
 0, /* attxtra */ \
 offset, /* attoff */ \
 len, /* attfrml */ \
 kdom, /* attkdom */ \
 0, /* attflag */ \
 DUMMY_DEFID, /* attdefid1, attdefid2 */ \
 0, /* attintl_id */ \
 0, /* attver_added */ \
 0, /* attver_dropped */ \
 0, /* attval_from */ \
 type, /* attfrmt */ \
 0, /* attfrmp */ \
 0, /* attver_altcol */\
 0, /* attcollID */ \
 0, /* attsrid */ \
 0, /* attgeomtype */ \
 0, /* attencflags */ \
 0, /* attencwid */ \
 {0},/* attfree */ \
 name /* attname */

GLOBALDEF DMP_ATTRIBUTE DM_core_attributes[] =
{
    /* tabid, attname, offset, len, iskey */
    { attdef(REL_TABID, ATT_INT, "reltid", REL_OFFSET(reltid.db_tab_base), 4, 1) },
    { attdef(REL_TABID, ATT_INT, "reltidx", REL_OFFSET(reltid.db_tab_index), 4, 0) },
    { attdef(REL_TABID, ATT_INT, "relatts", REL_OFFSET(relatts), 2, 0) },
    { attdef(REL_TABID, ATT_INT, "reltcpri", REL_OFFSET(reltcpri), 2, 0) },
    { attdef(REL_TABID, ATT_INT, "relkeys", REL_OFFSET(relkeys), 2, 0) },
    { attdef(REL_TABID, ATT_INT, "relspec", REL_OFFSET(relspec), 2, 0) },
    { attdef(REL_TABID, ATT_INT, "relstat", REL_OFFSET(relstat), 4, 0) },
    { attdef(REL_TABID, ATT_INT, "relstat2", REL_OFFSET(relstat2), 4, 0) },
    { attdef(REL_TABID, ATT_INT, "reltups", REL_OFFSET(reltups), 8, 0) },
    { attdef(REL_TABID, ATT_INT, "relpages", REL_OFFSET(relpages), 8, 0) },
    { attdef(REL_TABID, ATT_INT, "relprim", REL_OFFSET(relprim), 4, 0) },
    { attdef(REL_TABID, ATT_INT, "relmain", REL_OFFSET(relmain), 4, 0) },
    { attdef(REL_TABID, ATT_INT, "relsave", REL_OFFSET(relsave), 4, 0) },
    { attdef(REL_TABID, ATT_INT, "relstamp1", REL_OFFSET(relstamp12.db_tab_high_time), 4, 0) },
    { attdef(REL_TABID, ATT_INT, "relstamp2", REL_OFFSET(relstamp12.db_tab_low_time), 4, 0) },
    { attdef(REL_TABID, ATT_INT, "relcmptlvl", REL_OFFSET(relcmptlvl), 4, 0) },
    { attdef(REL_TABID, ATT_INT, "relcreate", REL_OFFSET(relcreate), 4, 0) },
    { attdef(REL_TABID, ATT_INT, "relqid1", REL_OFFSET(relqid1), 4, 0) },
    { attdef(REL_TABID, ATT_INT, "relqid2", REL_OFFSET(relqid2), 4, 0) },
    { attdef(REL_TABID, ATT_INT, "relmoddate", REL_OFFSET(relmoddate), 4, 0) },
    { attdef(REL_TABID, ATT_INT, "relidxcount", REL_OFFSET(relidxcount), 2, 0) },
    { attdef(REL_TABID, ATT_INT, "relifill", REL_OFFSET(relifill), 2, 0) },
    { attdef(REL_TABID, ATT_INT, "reldfill", REL_OFFSET(reldfill), 2, 0) },
    { attdef(REL_TABID, ATT_INT, "rellfill", REL_OFFSET(rellfill), 2, 0) },
    { attdef(REL_TABID, ATT_INT, "relmin", REL_OFFSET(relmin), 4, 0) },
    { attdef(REL_TABID, ATT_INT, "relmax", REL_OFFSET(relmax), 4, 0) },
    { attdef(REL_TABID, ATT_INT, "relpgsize", REL_OFFSET(relpgsize), 4, 0) },
    { attdef(REL_TABID, ATT_INT, "relgwid", REL_OFFSET(relgwid), 2, 0) },
    { attdef(REL_TABID, ATT_INT, "relgwother", REL_OFFSET(relgwother), 2, 0) },
    { attdef(REL_TABID, ATT_INT, "relhigh_logkey", REL_OFFSET(relhigh_logkey), 4, 0) },
    { attdef(REL_TABID, ATT_INT, "rellow_logkey", REL_OFFSET(rellow_logkey), 4, 0) },
    { attdef(REL_TABID, ATT_INT, "relfhdr", REL_OFFSET(relfhdr), 4, 0) },
    { attdef(REL_TABID, ATT_INT, "relallocation", REL_OFFSET(relallocation), 4, 0) },
    { attdef(REL_TABID, ATT_INT, "relextend", REL_OFFSET(relextend), 4, 0) },
    { attdef(REL_TABID, ATT_INT, "relcomptype", REL_OFFSET(relcomptype), 2, 0) },
    { attdef(REL_TABID, ATT_INT, "relpgtype", REL_OFFSET(relpgtype), 2, 0) },
    { attdef(REL_TABID, ATT_INT, "relloccount", REL_OFFSET(relloccount), 2, 0) },
    { attdef(REL_TABID, ATT_INT, "relversion", REL_OFFSET(relversion), 2, 0) },
    { attdef(REL_TABID, ATT_INT, "relwid", REL_OFFSET(relwid), 4, 0) },
    { attdef(REL_TABID, ATT_INT, "reltotwid", REL_OFFSET(reltotwid), 4, 0) },
    { attdef(REL_TABID, ATT_INT, "reldatawid", REL_OFFSET(reldatawid), 4, 0) },
    { attdef(REL_TABID, ATT_INT, "reltotdatawid", REL_OFFSET(reltotdatawid), 4, 0) },
    { attdef(REL_TABID, ATT_INT, "relattnametot", REL_OFFSET(relattnametot), 4, 0) },
    { attdef(REL_TABID, ATT_INT, "relnparts", REL_OFFSET(relnparts), 2, 0) },
    { attdef(REL_TABID, ATT_INT, "relnpartlevels", REL_OFFSET(relnpartlevels), 2, 0) },
    { attdef(REL_TABID, ATT_INT, "relencflags", REL_OFFSET(relencflags), 2, 0) },
    { attdef(REL_TABID, ATT_INT, "relencver", REL_OFFSET(relencver), 2, 0) },
    { attdef(REL_TABID, ATT_BYTE, "relenckey", REL_OFFSET(relenckey), 64, 0) },
    { attdef(REL_TABID, ATT_BYTE, "relfree", REL_OFFSET(relfree), 16, 0) },
    { attdef(REL_TABID, ATT_CHA, "relloc", REL_OFFSET(relloc), DB_LOC_MAXNAME, 0) },
    { attdef(REL_TABID, ATT_CHA, "relowner", REL_OFFSET(relowner), DB_OWN_MAXNAME, 0) },
    { attdef(REL_TABID, ATT_CHA, "relid", REL_OFFSET(relid), DB_TAB_MAXNAME, 0) },
    { attdef(RIDX_TABID, ATT_CHA, "relid", RIDX_OFFSET(relname), DB_TAB_MAXNAME, 1) },
    { attdef(RIDX_TABID, ATT_CHA, "relowner", RIDX_OFFSET(relowner), DB_OWN_MAXNAME, 2) },
    { attdef(RIDX_TABID, ATT_INT, "tidp", RIDX_OFFSET(tidp), 4, 0) },
    { attdef(ATT_TABID, ATT_INT, "attrelid", ATT_OFFSET(attrelid.db_tab_base), 4, 1) },
    { attdef(ATT_TABID, ATT_INT, "attrelidx", ATT_OFFSET(attrelid.db_tab_index), 4, 2) },
    { attdef(ATT_TABID, ATT_INT, "attid", ATT_OFFSET(attid), 2, 0) },
    { attdef(ATT_TABID, ATT_INT, "attxtra", ATT_OFFSET(attxtra), 2, 0) },
    { attdef(ATT_TABID, ATT_INT, "attoff", ATT_OFFSET(attoff), 4, 0) },
    { attdef(ATT_TABID, ATT_INT, "attfrml", ATT_OFFSET(attfml), 4, 0) },
    { attdef(ATT_TABID, ATT_INT, "attkdom", ATT_OFFSET(attkey), 2, 0) },
    { attdef(ATT_TABID, ATT_INT, "attflag", ATT_OFFSET(attflag), 2, 0) },
    { attdef(ATT_TABID, ATT_INT, "attdefid1", ATT_OFFSET(attDefaultID.db_tab_base), 4, 0) },
    { attdef(ATT_TABID, ATT_INT, "attdefid2", ATT_OFFSET(attDefaultID.db_tab_index), 4, 0) },
    { attdef(ATT_TABID, ATT_INT, "attintl_id", ATT_OFFSET(attintl_id), 2, 0) },
    { attdef(ATT_TABID, ATT_INT, "attver_added", ATT_OFFSET(attver_added), 2, 0) },
    { attdef(ATT_TABID, ATT_INT, "attver_dropped", ATT_OFFSET(attver_dropped), 2, 0) },
    { attdef(ATT_TABID, ATT_INT, "attval_from", ATT_OFFSET(attval_from), 2, 0) },
    { attdef(ATT_TABID, ATT_INT, "attfrmt", ATT_OFFSET(attfmt), 2, 0) },
    { attdef(ATT_TABID, ATT_INT, "attfrmp", ATT_OFFSET(attfmp), 2, 0) },
    { attdef(ATT_TABID, ATT_INT, "attver_altcol", ATT_OFFSET(attver_altcol), 2, 0) },
    { attdef(ATT_TABID, ATT_INT, "attcollid", ATT_OFFSET(attcollID), 2, 0) },
    { attdef(ATT_TABID, ATT_INT, "attsrid", ATT_OFFSET(attsrid), 4, 0) },
    { attdef(ATT_TABID, ATT_INT, "attgeomtype", ATT_OFFSET(attgeomtype), 2, 0) },
    { attdef(ATT_TABID, ATT_INT, "attencflags", ATT_OFFSET(attencflags), 2, 0) },
    { attdef(ATT_TABID, ATT_INT, "attencwid", ATT_OFFSET(attencwid), 4, 0) },
    { attdef(ATT_TABID, ATT_BYTE, "attfree", ATT_OFFSET(attfree), 4, 0) },
    { attdef(ATT_TABID, ATT_CHA, "attname", ATT_OFFSET(attname), DB_ATT_MAXNAME, 0) },
    { attdef(IND_TABID, ATT_INT, "baseid", IND_OFFSET(baseid), 4, 1) },
    { attdef(IND_TABID, ATT_INT, "indexid", IND_OFFSET(indexid), 4, 0) },
    { attdef(IND_TABID, ATT_INT, "sequence", IND_OFFSET(sequence), 2, 0) },
    { attdef(IND_TABID, ATT_INT, "idom1", IND_OFFSET(idom[0]), 2, 0) },
    { attdef(IND_TABID, ATT_INT, "idom2", IND_OFFSET(idom[1]), 2, 0) },
    { attdef(IND_TABID, ATT_INT, "idom3", IND_OFFSET(idom[2]), 2, 0) },
    { attdef(IND_TABID, ATT_INT, "idom4", IND_OFFSET(idom[3]), 2, 0) },
    { attdef(IND_TABID, ATT_INT, "idom5", IND_OFFSET(idom[4]), 2, 0) },
    { attdef(IND_TABID, ATT_INT, "idom6", IND_OFFSET(idom[5]), 2, 0) },
    { attdef(IND_TABID, ATT_INT, "idom7", IND_OFFSET(idom[6]), 2, 0) },
    { attdef(IND_TABID, ATT_INT, "idom8", IND_OFFSET(idom[7]), 2, 0) },
    { attdef(IND_TABID, ATT_INT, "idom9", IND_OFFSET(idom[8]), 2, 0) },
    { attdef(IND_TABID, ATT_INT, "idom10", IND_OFFSET(idom[9]), 2, 0) },
    { attdef(IND_TABID, ATT_INT, "idom11", IND_OFFSET(idom[10]), 2, 0) },
    { attdef(IND_TABID, ATT_INT, "idom12", IND_OFFSET(idom[11]), 2, 0) },
    { attdef(IND_TABID, ATT_INT, "idom13", IND_OFFSET(idom[12]), 2, 0) },
    { attdef(IND_TABID, ATT_INT, "idom14", IND_OFFSET(idom[13]), 2, 0) },
    { attdef(IND_TABID, ATT_INT, "idom15", IND_OFFSET(idom[14]), 2, 0) },
    { attdef(IND_TABID, ATT_INT, "idom16", IND_OFFSET(idom[15]), 2, 0) },
    { attdef(IND_TABID, ATT_INT, "idom17", IND_OFFSET(idom[16]), 2, 0) },
    { attdef(IND_TABID, ATT_INT, "idom18", IND_OFFSET(idom[17]), 2, 0) },
    { attdef(IND_TABID, ATT_INT, "idom19", IND_OFFSET(idom[18]), 2, 0) },
    { attdef(IND_TABID, ATT_INT, "idom20", IND_OFFSET(idom[19]), 2, 0) },
    { attdef(IND_TABID, ATT_INT, "idom21", IND_OFFSET(idom[20]), 2, 0) },
    { attdef(IND_TABID, ATT_INT, "idom22", IND_OFFSET(idom[21]), 2, 0) },
    { attdef(IND_TABID, ATT_INT, "idom23", IND_OFFSET(idom[22]), 2, 0) },
    { attdef(IND_TABID, ATT_INT, "idom24", IND_OFFSET(idom[23]), 2, 0) },
    { attdef(IND_TABID, ATT_INT, "idom25", IND_OFFSET(idom[24]), 2, 0) },
    { attdef(IND_TABID, ATT_INT, "idom26", IND_OFFSET(idom[25]), 2, 0) },
    { attdef(IND_TABID, ATT_INT, "idom27", IND_OFFSET(idom[26]), 2, 0) },
    { attdef(IND_TABID, ATT_INT, "idom28", IND_OFFSET(idom[27]), 2, 0) },
    { attdef(IND_TABID, ATT_INT, "idom29", IND_OFFSET(idom[28]), 2, 0) },
    { attdef(IND_TABID, ATT_INT, "idom30", IND_OFFSET(idom[29]), 2, 0) },
    { attdef(IND_TABID, ATT_INT, "idom31", IND_OFFSET(idom[30]), 2, 0) },
    { attdef(IND_TABID, ATT_INT, "idom32", IND_OFFSET(idom[31]), 2, 0) },
#ifdef NOT_UNTIL_CLUSTERED_INDEXES
    { attdef(IND_TABID, ATT_INT, "idom33", IND_OFFSET(idom[32]), 2, 0) },
    { attdef(IND_TABID, ATT_INT, "idom34", IND_OFFSET(idom[33]), 2, 0) },
    { attdef(IND_TABID, ATT_INT, "idom35", IND_OFFSET(idom[34]), 2, 0) },
    { attdef(IND_TABID, ATT_INT, "idom36", IND_OFFSET(idom[35]), 2, 0) },
    { attdef(IND_TABID, ATT_INT, "idom37", IND_OFFSET(idom[36]), 2, 0) },
    { attdef(IND_TABID, ATT_INT, "idom38", IND_OFFSET(idom[37]), 2, 0) },
    { attdef(IND_TABID, ATT_INT, "idom39", IND_OFFSET(idom[38]), 2, 0) },
    { attdef(IND_TABID, ATT_INT, "idom40", IND_OFFSET(idom[39]), 2, 0) },
    { attdef(IND_TABID, ATT_INT, "idom41", IND_OFFSET(idom[40]), 2, 0) },
    { attdef(IND_TABID, ATT_INT, "idom42", IND_OFFSET(idom[41]), 2, 0) },
    { attdef(IND_TABID, ATT_INT, "idom43", IND_OFFSET(idom[42]), 2, 0) },
    { attdef(IND_TABID, ATT_INT, "idom44", IND_OFFSET(idom[43]), 2, 0) },
    { attdef(IND_TABID, ATT_INT, "idom45", IND_OFFSET(idom[44]), 2, 0) },
    { attdef(IND_TABID, ATT_INT, "idom46", IND_OFFSET(idom[45]), 2, 0) },
    { attdef(IND_TABID, ATT_INT, "idom47", IND_OFFSET(idom[46]), 2, 0) },
    { attdef(IND_TABID, ATT_INT, "idom48", IND_OFFSET(idom[47]), 2, 0) },
    { attdef(IND_TABID, ATT_INT, "idom49", IND_OFFSET(idom[48]), 2, 0) },
    { attdef(IND_TABID, ATT_INT, "idom50", IND_OFFSET(idom[49]), 2, 0) },
    { attdef(IND_TABID, ATT_INT, "idom51", IND_OFFSET(idom[50]), 2, 0) },
    { attdef(IND_TABID, ATT_INT, "idom52", IND_OFFSET(idom[51]), 2, 0) },
    { attdef(IND_TABID, ATT_INT, "idom53", IND_OFFSET(idom[52]), 2, 0) },
    { attdef(IND_TABID, ATT_INT, "idom54", IND_OFFSET(idom[53]), 2, 0) },
    { attdef(IND_TABID, ATT_INT, "idom55", IND_OFFSET(idom[54]), 2, 0) },
    { attdef(IND_TABID, ATT_INT, "idom56", IND_OFFSET(idom[55]), 2, 0) },
    { attdef(IND_TABID, ATT_INT, "idom57", IND_OFFSET(idom[56]), 2, 0) },
    { attdef(IND_TABID, ATT_INT, "idom58", IND_OFFSET(idom[57]), 2, 0) },
    { attdef(IND_TABID, ATT_INT, "idom59", IND_OFFSET(idom[58]), 2, 0) },
    { attdef(IND_TABID, ATT_INT, "idom60", IND_OFFSET(idom[59]), 2, 0) },
    { attdef(IND_TABID, ATT_INT, "idom61", IND_OFFSET(idom[60]), 2, 0) },
    { attdef(IND_TABID, ATT_INT, "idom62", IND_OFFSET(idom[61]), 2, 0) },
    { attdef(IND_TABID, ATT_INT, "idom63", IND_OFFSET(idom[62]), 2, 0) },
    { attdef(IND_TABID, ATT_INT, "idom64", IND_OFFSET(idom[63]), 2, 0) },
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
** (DSCV9 -> aka Ingres 10.0)
*/

GLOBALDEF DMP_ATTRIBUTE DM_frozen_DSCV9_attributes[] =
{
    /* tabid, type, attname, offset, len, iskey */
    { attdef(REL_TABID, ATT_INT, "reltid", -1, 4, 1) },
    { attdef(REL_TABID, ATT_INT, "reltidx", -1, 4, 0) },
    { attdef(REL_TABID, ATT_INT, "relatts", -1, 2, 0) },
    { attdef(REL_TABID, ATT_INT, "reltcpri", -1, 2, 0) },
    { attdef(REL_TABID, ATT_INT, "relkeys", -1, 2, 0) },
    { attdef(REL_TABID, ATT_INT, "relspec", -1, 2, 0) },
    { attdef(REL_TABID, ATT_INT, "relstat", -1, 4, 0) },
    { attdef(REL_TABID, ATT_INT, "relstat2", -1, 4, 0) },
    { attdef(REL_TABID, ATT_INT, "reltups", -1, 8, 0) },
    { attdef(REL_TABID, ATT_INT, "relpages", -1, 8, 0) },
    { attdef(REL_TABID, ATT_INT, "relprim", -1, 4, 0) },
    { attdef(REL_TABID, ATT_INT, "relmain", -1, 4, 0) },
    { attdef(REL_TABID, ATT_INT, "relsave", -1, 4, 0) },
    { attdef(REL_TABID, ATT_INT, "relstamp1", -1, 4, 0) },
    { attdef(REL_TABID, ATT_INT, "relstamp2", -1, 4, 0) },
    { attdef(REL_TABID, ATT_INT, "relcmptlvl", -1, 4, 0) },
    { attdef(REL_TABID, ATT_INT, "relcreate", -1, 4, 0) },
    { attdef(REL_TABID, ATT_INT, "relqid1", -1, 4, 0) },
    { attdef(REL_TABID, ATT_INT, "relqid2", -1, 4, 0) },
    { attdef(REL_TABID, ATT_INT, "relmoddate", -1, 4, 0) },
    { attdef(REL_TABID, ATT_INT, "relidxcount", -1, 2, 0) },
    { attdef(REL_TABID, ATT_INT, "relifill", -1, 2, 0) },
    { attdef(REL_TABID, ATT_INT, "reldfill", -1, 2, 0) },
    { attdef(REL_TABID, ATT_INT, "rellfill", -1, 2, 0) },
    { attdef(REL_TABID, ATT_INT, "relmin", -1, 4, 0) },
    { attdef(REL_TABID, ATT_INT, "relmax", -1, 4, 0) },
    { attdef(REL_TABID, ATT_INT, "relpgsize", -1, 4, 0) },
    { attdef(REL_TABID, ATT_INT, "relgwid", -1, 2, 0) },
    { attdef(REL_TABID, ATT_INT, "relgwother", -1, 2, 0) },
    { attdef(REL_TABID, ATT_INT, "relhigh_logkey", -1, 4, 0) },
    { attdef(REL_TABID, ATT_INT, "rellow_logkey", -1, 4, 0) },
    { attdef(REL_TABID, ATT_INT, "relfhdr", -1, 4, 0) },
    { attdef(REL_TABID, ATT_INT, "relallocation", -1, 4, 0) },
    { attdef(REL_TABID, ATT_INT, "relextend", -1, 4, 0) },
    { attdef(REL_TABID, ATT_INT, "relcomptype", -1, 2, 0) },
    { attdef(REL_TABID, ATT_INT, "relpgtype", -1, 2, 0) },
    { attdef(REL_TABID, ATT_INT, "relloccount", -1, 2, 0) },
    { attdef(REL_TABID, ATT_INT, "relversion", -1, 2, 0) },
    { attdef(REL_TABID, ATT_INT, "relwid", -1, 4, 0) },
    { attdef(REL_TABID, ATT_INT, "reltotwid", -1, 4, 0) },
    { attdef(REL_TABID, ATT_INT, "reldatawid", -1, 4, 0) },
    { attdef(REL_TABID, ATT_INT, "reltotdatawid", -1, 4, 0) },
    { attdef(REL_TABID, ATT_INT, "relattnametot", -1, 4, 0) },
    { attdef(REL_TABID, ATT_INT, "relnparts", -1, 2, 0) },
    { attdef(REL_TABID, ATT_INT, "relnpartlevels", -1, 2, 0) },
    { attdef(REL_TABID, ATT_INT, "relencflags", -1, 2, 0) },
    { attdef(REL_TABID, ATT_INT, "relencver", -1, 2, 0) },
    { attdef(REL_TABID, ATT_BYTE, "relenckey", -1, 64, 0) },
    { attdef(REL_TABID, ATT_BYTE, "relfree", -1, 16, 0) },
    { attdef(REL_TABID, ATT_CHA, "relloc", -1, 32, 0) },
    { attdef(REL_TABID, ATT_CHA, "relowner", -1, 32, 0) },
    { attdef(REL_TABID, ATT_CHA, "relid", -1, 256, 0) },
    { attdef(RIDX_TABID, ATT_CHA, "relid", -1, 256, 1) },
    { attdef(RIDX_TABID, ATT_CHA, "relowner", -1, 32, 2) },
    { attdef(RIDX_TABID, ATT_INT, "tidp", -1, 4, 0) },
    { attdef(ATT_TABID, ATT_INT, "attrelid", -1, 4, 1) },
    { attdef(ATT_TABID, ATT_INT, "attrelidx", -1, 4, 2) },
    { attdef(ATT_TABID, ATT_INT, "attid", -1, 2, 0) },
    { attdef(ATT_TABID, ATT_INT, "attxtra", -1, 2, 0) },
    { attdef(ATT_TABID, ATT_INT, "attoff", -1, 4, 0) },
    { attdef(ATT_TABID, ATT_INT, "attfrml", -1, 4, 0) },
    { attdef(ATT_TABID, ATT_INT, "attkdom", -1, 2, 0) },
    { attdef(ATT_TABID, ATT_INT, "attflag", -1, 2, 0) },
    { attdef(ATT_TABID, ATT_INT, "attdefid1", -1, 4, 0) },
    { attdef(ATT_TABID, ATT_INT, "attdefid2", -1, 4, 0) },
    { attdef(ATT_TABID, ATT_INT, "attintl_id", -1, 2, 0) },
    { attdef(ATT_TABID, ATT_INT, "attver_added", -1, 2, 0) },
    { attdef(ATT_TABID, ATT_INT, "attver_dropped", -1, 2, 0) },
    { attdef(ATT_TABID, ATT_INT, "attval_from", -1, 2, 0) },
    { attdef(ATT_TABID, ATT_INT, "attfrmt", -1, 2, 0) },
    { attdef(ATT_TABID, ATT_INT, "attfrmp", -1, 2, 0) },
    { attdef(ATT_TABID, ATT_INT, "attver_altcol", -1, 2, 0) },
    { attdef(ATT_TABID, ATT_INT, "attcollid", -1, 2, 0) },
    { attdef(ATT_TABID, ATT_INT, "attsrid", -1, 4, 0) },
    { attdef(ATT_TABID, ATT_INT, "attgeomtype", -1, 2, 0) },
    { attdef(ATT_TABID, ATT_INT, "attencflags", -1, 2, 0) },
    { attdef(ATT_TABID, ATT_INT, "attencwid", -1, 4, 0) },
    { attdef(ATT_TABID, ATT_BYTE, "attfree", -1, 4, 0) },
    { attdef(ATT_TABID, ATT_CHA, "attname", -1, 256, 0) },
};

GLOBALDEF i4 DM_frozen_DSCV9_att_cnt = sizeof(DM_frozen_DSCV9_attributes) / sizeof(DMP_ATTRIBUTE); 
