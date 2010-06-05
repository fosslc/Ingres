/*
** Copyright (c) 1996, 2008 Ingres Corporation
*/

# include	<compat.h>
# include	<duucatdef.h>

#include	<iicommon.h>
#include	<dbdbms.h>
#include	<adf.h> 
#include	<er.h>
#include	<pc.h>
#include	<cs.h>
#include	<lg.h>
#include	<lk.h>
#include	<dm.h>
#include	<dmf.h>
#include	<dmp.h>

/*
** Name:        dubdata.c
**
** Description: Global data for dub facility.
**
** History:
**
**      28-sep-96 (mcgem01)
**          Created.
**	16-dec-1996 (nanpr01)
**	    Removed the duplicate line causing bug 79773.
**	20-may-1999 (nanpr01)
**	    SIR 89267 : added index in iirule catalog for existence check.
**      02-nov-2001 (stial01)
**          Added modify statements for iiprivlist, iirange, iiextended_relation
**      02-nov-2001 (stial01)
**          Added modify statements for iidbcapabilities
**	22-Jan-2003 (jenjo02)
**	    Added modify statements for iisequence
**	31-Dec-2003 (schka24)
**	    Add partitioning catalogs
**	29-jan-2004 (schka24)
**	    Take iipartition back out, not needed.
**	29-Dec-2004 (schka24)
**	    Make iiprotect btree, no index;  add fillfactor 100 to btrees.
**	04-Jan-2007 (kibro01) b117114
**	    Add modify statements for gw07 tables
**      17-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
**      10-Feb-2010 (maspa05) b122651
**          Added dub_mandflags and dub_num_mandflags
*/

GLOBALDEF   char        *Dub_00corecat_commands[]       =
{
    "iiattribute",
    "modify iiattribute to hash on attrelid, attrelidx where minpages = 32",

    "iidevices",
    "modify iidevices to hash on devrelid, devrelidx where minpages = 8",

    "iiindex",
    "modify iiindex to hash on baseid where minpages = 8",

    "iirelation",
    "modify iirelation to hash on reltid where minpages = 16",
};

GLOBALDEF   DUU_CATDEF  Dub_01corecat_defs[DU_0MAXCORE_CATDEFS+1] ZERO_FILL;

/* *Attention* A string must start with one of three things:
** m.... means a modify string
** ii.... means a catalog name
** in... means a create-index (quel index-on) statement
** Lowercase!  No leading spaces inside the text strings!
** There is always a name and a modify string.   There might not be index
** strings for a catalog.
*/

GLOBALDEF   const    char        *Dub_10dbmscat_commands[]   =
{

    "iidbdepends",
    "modify iidbdepends to btree on inid1, inid2, itype, i_qid with fillfactor=100",
    "index on iidbdepends is iixdbdepends(deid1, deid2, dtype, qid) \
with structure=btree",

    "iihistogram",
    "modify iihistogram to hash on htabbase, htabindex where minpages =16",

    "iiintegrity",
    "modify iiintegrity to hash on inttabbase, inttabidx where minpages=16",
    "index on iiintegrity is iiintegrityidx \
        (consschema_id1, consschema_id2, consname) \
        with structure = hash, minpages = 32",

    "iiprotect",
    "modify iiprotect to btree on protabbase, protabidx with fillfactor=100",

    "iiqrytext",
    "modify iiqrytext to btree on txtid1, txtid2, mode, seq with fillfactor=100",

    "iistatistics",
    "modify iistatistics to hash on stabbase, stabindex where minpages=16",

    "iitree",
    "modify iitree to btree on treetabbase, treetabidx, treemode, treeseq \
with fillfactor=100, compression=(data)",

    "iidevices",
    "modify iidevices to hash on devrelid, devrelidx",

    "iiprocedure",
    "modify iiprocedure to btree unique on dbp_name, dbp_owner with fillfactor=100",
    "index on iiprocedure is iixprocedure(dbp_id, dbp_idx) \
with structure = btree",

    "iirule",
    "modify iirule to hash on rule_tabbase, rule_tabidx with minpages=16",
    "index on iirule is iiruleidx( rule_id1, rule_id2 ) \
        with structure = hash, minpages = 32",
    "index on iirule is iiruleidx1( rule_owner, rule_name) \
        with structure = hash, minpages = 32",

    "iievent",
    "modify iievent to hash on event_name, event_owner with minpages=30",
    "index on iievent is iixevent(event_idbase, event_idx) \
with structure = btree",

    "iisynonym",
    "modify iisynonym to hash unique on synonym_name, synonym_owner",
    "index on iisynonym is iixsynonym(syntabbase, syntabidx) \
        with structure=btree",

    "iidbms_comment",
    "modify iidbms_comment to btree unique on comtabbase, comtabidx, comattid, text_sequence with fillfactor=100",

    "iipriv",
    "modify iipriv to btree on d_obj_id, d_obj_idx, d_priv_number with fillfactor=100",
    "index on iipriv is iixpriv(i_obj_id, i_obj_idx, i_priv, i_priv_grantee) \
with structure=btree, key=(i_obj_id, i_obj_idx, i_priv)",

    /*
    **  Next two catalogs are used by the C2-audit gateway
    */
    "iigw06_relation",
    "modify iigw06_relation to hash unique on reltid, reltidx",

    "iigw06_attribute",
    "modify iigw06_attribute to hash on reltid,reltidx",

    "iikey",
    "modify iikey to hash on key_consid1, key_consid2 with minpages = 32",

    "iidefault",
    "modify iidefault to hash on defid1, defid2 with minpages = 32, \
compression",
    "index on iidefault is iidefaultidx(defvalue) with minpages  = 32, \
structure = hash, compression",

    "iiprocedure_parameter",
    "modify iiprocedure_parameter to hash on pp_procid1, pp_procid2 \
with minpages = 32",

    "iischema",
    "modify iischema to hash unique on schema_name \
where minpages= 8",
    "index on iischema is iischemaidx (schema_id, schema_idx) \
        with structure = hash",

    "iisectype",
    "modify iisectype to btree unique on sec_type, sec_name",

    /* security catalogs */
    "iisecalarm",
    "modify iisecalarm to hash on obj_type, obj_id1, obj_id2 \
        with minpages=32, compression",

    "iiprivlist",
    "modify iiprivlist to heap",
    
    "iirange",
    "modify iirange to hash on rng_baseid, rng_indexid \
	WITH MINPAGES = 2, ALLOCATION = 4",

    "iiextended_relation",
    "modify iiextended_relation to hash on etab_base \
	WITH MINPAGES = 2, ALLOCATION = 4",

    "iidbcapabilities",
    "modify iidbcapabilities to heap",

    "iisequence",
    "modify iisequence to hash on seq_name, seq_owner \
	with minpages=30",

    "iidistscheme",
    "modify iidistscheme to btree unique on mastid, mastidx, levelno \
	with fillfactor=100",

    "iidistcol",
    "modify iidistcol to btree unique on mastid, mastidx, levelno, colseq \
	with fillfactor=100",

    "iidistval",
    "modify iidistval to btree unique on mastid, mastidx, levelno, \
	valseq, colseq with fillfactor=100, compression=(data)",

    "iipartname",
    "modify iipartname to btree unique on mastid, mastidx, levelno, partseq \
	with fillfactor=100",

    "iigw07_attribute",
    "modify iigw07_attribute to btree on tblid, tblidx, attnum",

    "iigw07_index",
    "modify iigw07_index to btree on tblid, tblidx, attnum",

    "iigw07_relation",
    "modify iigw07_relation to btree on tblid, tblidx",

    NULL
};

GLOBALDEF   DUU_CATDEF  Dub_11dbmscat_defs[DU_1MAXDBMS_CATDEFS+1] ZERO_FILL;


/* *Warning* This array is hardcoded to match dubdbmsmod.c -- if you change
** it, fix there too.  (Or fix the hardcoding!)
*/

GLOBALDEF   const    char        *Dub_30iidbdbcat_commands[]     =
{
    "iidatabase",
    "modify iidatabase to hash unique on name where minpages = 8",
    "index unique on iidatabase is iidbid_idx(db_id) with structure=hash",

    "iiuser",
    "modify iiuser to hash unique on name where minpages = 8",

    "iilocations",
    "modify iilocations to hash unique on lname where minpages = 4",

    "iiextend",
    "modify iiextend to hash on dname where minpages = 8",

    "iiusergroup",
    "modify iiusergroup to btree unique on groupid, groupmem with fillfactor=100",

    "iirole",
    "modify iirole to hash unique on roleid where minpages = 4",

    "iidbpriv",
    "modify iidbpriv to btree unique on grantee, gtype, dbname with fillfactor=100",

    "iiprofile",
    "modify iiprofile to hash unique on name",

    "iirolegrant",
    "modify iirolegrant to btree unique on rgr_grantee, rgr_rolename with fillfactor=100",

    NULL
};

GLOBALDEF   DUU_CATDEF  Dub_31iidbdbcat_defs[DU_3MAXIIDBDB_CATDEFS+1] ZERO_FILL;

GLOBALDEF   char        *Dub_40ddbcat_commands[]        =
{
    "iialt_columns",
    "modify iidd_alt_columns to hash on table_name, table_owner",

    "iicolumns",
    "modify iidd_columns to hash on table_name, table_owner",

    "iidbcapabilities",
    "modify iidd_dbcapabilities to hash on cap_capability",

    "iiddb_dbdepends",
    "modify iidd_ddb_dbdepends to hash on inid1, inid2, itype",

    "iiddb_ldbids",
    "modify iidd_ddb_ldbids to hash on ldb_node, ldb_dbms, ldb_database",

    "iiddb_ldb_columns",
    "modify iidd_ddb_ldb_columns to hash on object_base, object_index, \
column_sequence",

    "iiddb_ldb_dbcaps",
    "modify iidd_ddb_ldb_dbcaps to hash on ldb_id",

    "iiddb_long_ldbnames",
    "modify iidd_ddb_long_ldbnames to hash on long_ldbname, ldb_id",

    "iiddb_objects",
    "modify iidd_ddb_objects to hash on object_name, object_owner",

    "iiddb_object_base",
    "modify iidd_ddb_object_base to hash on object_base",

    "iiddb_tableinfo",
    "modify iidd_ddb_tableinfo to hash on object_base, object_index",

    "iiddb_tree",
    "modify iidd_ddb_tree to hash on treetabbase, treetabidx, treemode, treeseq"
,

    "iihistograms",
    "modify iidd_histograms to hash on table_name, table_owner, column_name",

    "iiindexes",
    "modify iidd_indexes to hash on index_name, index_owner",

    "iiindex_columns",
    "modify iidd_index_columns to hash on index_name, index_owner",

    "iiintegrities",
    "modify iidd_integrities to hash on table_name, table_owner",

    "iimulti_locations",
    "modify iidd_multi_locations to hash on table_name, table_owner",

    "iipermits",
    "modify iidd_permits to hash on object_name, object_owner",

    "iiprocedure",
    "modify iidd_procedure to hash on dbp_name, dbp_owner",

    "iiregistrations",
    "modify iidd_registrations to hash on object_name, object_owner",

    "iistats",
    "modify iidd_stats to hash on table_name, table_owner",

    "iitables",
    "modify iidd_tables to hash on table_name, table_owner",

    "iiviews",
    "modify iidd_views to hash on table_name, table_owner, text_sequence",

    NULL
};

GLOBALDEF   DUU_CATDEF  Dub_41ddbcat_defs[DU_4MAXDDB_CATDEFS+1] ZERO_FILL;

/* dub_mandflags[] - list of mandatory relstat, relstat2 flags. These get
** enforced by upgradedb and checked/fixed by verifydb.
**
** For each element
**
**     du_relname - name of the catalog
**     du_relstat - a bitmask for the mandatory relstat flags
**     du_relstat2 - a bitmask for the mandatory relstat2 flags
**
*/

GLOBALDEF DUU_MANDFLAGS dub_mandflags[]=
{
        {"iirelation", 0, TCB2_PHYSLOCK_CONCUR },
        {"iirel_idx", 0, TCB2_PHYSLOCK_CONCUR },
        {"iiattribute", 0, TCB2_PHYSLOCK_CONCUR },
        {"iiindex", 0, TCB2_PHYSLOCK_CONCUR },
        {"iidevices", 0, TCB2_PHYSLOCK_CONCUR },
        {"iisequence", 0, TCB2_PHYSLOCK_CONCUR }
};

GLOBALDEF i4 dub_num_mandflags = (i4) (sizeof(dub_mandflags)/sizeof(DUU_MANDFLAGS));
