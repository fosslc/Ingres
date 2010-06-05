/*
**Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<pc.h>
# include	<gl.h>
# include	<cs.h>
# include	<er.h>
# include	<iicommon.h>
# include	<dbdbms.h>
# include	<duf.h>
# include	<lk.h>
# include	<usererror.h>
# include	<duerr.h>
# include	<duenv.h>
# include	<dustar.h>
# include       <duucatdef.h>
# include	<dudbms.h>
# include       <ddb.h>

/*
** Name:        ducdata.c
**
** Description: Global data for duc facility.
**
** History:
**
**      28-sep-96 (mcgem01)
**          Created.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**     24-oct-2008 (stial01)
**          Modularize create objects for increased ingres names (SIR 121123)
**     29-oct-2008 (stial01)
**          Updated to address string size limitation on windows
**          Annotate create statements with corresponding structure
**     16-nov-2008 (stial01)
**          When possible define sizeof structure used for catalog
**     06-dec-2008 (stial01)
**          Fix create statement for catalogs dependent on DB_MAXNAME
**     07-apr-2009 (stial01)
**          Add create/modify info for core catalogs created with quel
**     14-apr-2009 (stial01)
**          Add modify info for iirelation,iiattribute,iiindex
**          Previous change was for SYSCAT, core catalogs
**	12-Nov-2009 (kschendel) SIR 122882
**	    cmptlvl is now an integer.
**	16-Nov-2009 (kschendel) SIR 122890
**	    Remove NODUPLICATES from any catalogs with unique keys already.
**	    It's redundant and key-uniqueness suffices.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**     10-feb-2010 (maspa05) b122651
**          Add Duc_equiv_cats - list of equivalent catalogs for the 
**          purposes of modify. This is where modifying the 'equivalent'
**          catalog also modifies the required one e.g. modifying iirelation
**          to re-create/re-modify iirel_idx . Used by duc_modify_catalog.
**          In terms of this bug it allows me to call duc_modify_catalog for
**          all TCB2_PHYSLOCK_CONCUR catalogs (including iirel_idx) if needed
**          from upgradedb and verifydb
**     22-apr-2010 (stial01)
**          Use DB_EXTFMT_SIZE for register table newcolname IS 'ext_format'
**     29-Apr-2010 (stial01)
**          modify with compression=(data) all catalogs with long ids.
*/

/* ducommon.qsc */

GLOBALDEF       DU_ERROR        *Duc_errcb ZERO_FILL;
GLOBALDEF       bool            upg_dbg_out = FALSE;

/* ducreate.qsc */

GLOBALDEF DU_ENV       *Duc_dbenv ZERO_FILL;
GLOBALDEF i4            Duc_deadlock    ZERO_FILL;
GLOBALDEF i4            Duc_dlock_cnt   ZERO_FILL;
GLOBALDEF i4            Duc_1dlock_retry ZERO_FILL;
GLOBALDEF i4            Duc_iidbdb_read ZERO_FILL;
GLOBALDEF i4            Duc_destroydb_msg ZERO_FILL;
GLOBALDEF i4            Duc_distributed_flag ZERO_FILL;
GLOBALDEF DU_STARCB     Duc_star_cb;

/*
** NOTE about Duc_catdef
** 
** - Names should be defined with ##DB_MAXNAME##
**
**   DB_DB_MAXNAME	database 
**   DB_LOC_MAXNAME	location 
**   DB_OWN_MAXNAME	owner (and role)
**   DB_SCHEMA_MAXNAME  schema
**   DB_EVENT_MAXNAME	event
**   DB_ALARM_MAXNAME	alarm
**   DB_NODE_MAXNAME	node
**
** - The following are not names
**   DB_TYPE_MAXLEN	
**
** - Watch out for fields defined as char(32) that should NOT be changed
**   to ##DB_MAXNAME##, e.g. reserved fields
**
** - All fields that were char(32) need to be verified against 
**   corresponding C structures to see if they are meant to have size
**   32 or DB_MAXNAME. 
** 
** - All catalog definitions should be updated with a comment where the
**   corresponding C structure is defined!
**
** - Watch out for catalog row size > default page size
**
*/

GLOBALDEF DUC_CATDEF Duc_catdef[] = 
{
	/* Below are catalogs created in ALL databases */
{
0, /* sizeof(DMP_RELATION) in back!dmf!hdr dm.h */
"iirelation",
0, /* number of columns, MUST create/upgrade core catalog in back end */
NULL, /* create stmt: MUST create/upgrade core catalog in back end */
"MODIFY iirelation to hash with compression",
NULL, /* index 1 - no need to specify... iirel_idx gets created by sysmod */
NULL  /* index 2 */
},
{
sizeof(DB_ATTS),
"iiattribute",
0, /* number of columns, MUST create/upgrade core catalog in back end */
NULL, /* create stmt: MUST create/upgrade core catalog in back end */
"MODIFY iiattribute to hash with compression",
NULL, /* index 1 */
NULL  /* index 2 */
},
{
sizeof(DB_IIINDEX),
"iiindex",
0, /* number of columns, MUST create/upgrade core catalog in back end */
NULL, /* create stmt: MUST create/upgrade core catalog in back end */
"MODIFY iiindex to hash",
NULL, /* index 1 */
NULL  /* index 2 */
},
{
0,
"iicolcompare",
16, /* number of columns */
"CREATE TABLE iicolcompare("
" ctabbase1=i4 not null default 0,"
" ctabbase2=i4 not null default 0,"
" ctabidx1=i4 not null default 0,"
" ctabidx2=i4 not null default 0,"
" ccolcount=i4 not null default 0,"
" catno11=i2 not null default 0,"
" catno12=i2 not null default 0,"
" catno13=i2 not null default 0,"
" catno14=i2 not null default 0,"
" catno21=i2 not null default 0,"
" catno22=i2 not null default 0,"
" catno23=i2 not null default 0,"
" catno24=i2 not null default 0,"
" cltcount=f4 not null default 0,"
" ceqcount=f4 not null default 0,"
" cgtcount=f4 not null default 0)"
" with noduplicates",
"MODIFY iicolcompare to hash on ctabbase1, ctabidx1 where minpages=8",
NULL, /* index 1 */
NULL  /* index 2 */
},

{
sizeof(DB_DBCAPABILITIES),
"iidbcapabilities",
2, /* number of columns */
"CREATE TABLE iidbcapabilities("
" cap_capability char(32) not null default ' ',"
" cap_value char(32) not null default ' ')"
" with noduplicates",
NULL, /* MODIFY */
NULL, /* index 1 */
NULL  /* index 2 */
},

{
0,
"iidbdepends",
8, /* number of columns */
"CREATE TABLE iidbdepends("
" inid1 integer not null default 0,"
" inid2 integer not null default 0,"
" itype integer not null default 0,"
" i_qid integer not null default 0,"
" qid integer not null default 0,"
" deid1 integer not null default 0,"
" deid2 integer not null default 0,"
" dtype integer not null default 0)"
" with noduplicates",
"MODIFY iidbdepends to btree on inid1, inid2, itype, i_qid"
" with fillfactor = 100",
"CREATE INDEX iixdbdepends on iidbdepends (deid1, deid2, dtype, qid)"
" with structure=btree",
NULL  /* index 2 */
},

{
0,
"iidbms_comment",
7, /* number of columns */
"CREATE TABLE iidbms_comment("
" comtabbase integer not null,"
" comtabidx integer not null,"
" comattid smallint not null,"
" comtype smallint not null,"
" short_remark char(60) not null,"
" text_sequence smallint not null,"
" long_remark varchar(1600) not null)"
" with duplicates",
"MODIFY iidbms_comment to btree unique on"
" comtabbase, comtabidx, comattid, text_sequence"
" with fillfactor = 100",
NULL, /* index 1 */
NULL  /* index 2 */
},

{
0,
"iidefault",
3, /* number of columns */
"CREATE TABLE iidefault("
" defid1 integer not null,"
" defid2 integer not null,"
" defvalue varchar(1501))"
" with duplicates",
"MODIFY iidefault TO HASH ON defid1, defid2 WITH MINPAGES=32, COMPRESSION",
"CREATE INDEX iidefaultidx ON iidefault ( defvalue )"
" WITH MINPAGES  = 32, STRUCTURE=HASH, COMPRESSION",
NULL  /* index 2 */
},

{
0, /* iidevices is DMP_DEVICE in back!dmf!hdr dmp.h */
"iidevices",
4, /* number of columns */
NULL, /* MUST create with QUEL in duc_quel_create() */
"MODIFY iidevices to hash on devrelid, devrelidx",
NULL, /* index 1 */
NULL  /* index 2 */
},

{
0,
"iidistcol",
5, /* number of columns */
"CREATE TABLE iidistcol("
" mastid integer not null not default,"
" mastidx integer not null not default,"
" levelno smallint not null not default,"
" colseq smallint not null not default,"
" attid smallint not null not default)",
"MODIFY iidistcol to btree unique on mastid, mastidx, levelno, colseq"
" with fillfactor = 100",
NULL, /* index 1 */
NULL  /* index 2 */
},

{
0,
"iidistscheme",
6, /* number of columns */
"CREATE TABLE iidistscheme("
" mastid integer not null not default,"
" mastidx integer not null not default,"
" levelno smallint not null not default,"
" ncols smallint not null default 0,"
" nparts smallint not null default 0,"
" distrule smallint not null not default)",
"MODIFY iidistscheme to btree unique on mastid, mastidx, levelno"
" with fillfactor = 100",
NULL, /* index 1 */
NULL  /* index 2 */
},

{
0,
"iidistval",
8, /* number of columns */
"CREATE TABLE iidistval("
" mastid integer not null not default,"
" mastidx integer not null not default,"
" levelno smallint not null not default,"
" valseq smallint not null not default,"
" colseq smallint not null default 0,"
" partseq smallint not null not default,"
" oper i1 not null not default,"
" distvalue varchar(1500))",
"MODIFY iidistval to btree unique on mastid, mastidx, levelno, valseq, colseq"
" with fillfactor = 100, compression = (data)",
NULL, /* index 1 */
NULL  /* index 2 */
},

{
/* iievent catalog is DB_IIEVENT in back!hdr!hdr dbdbms.h */
sizeof(DB_IIEVENT),
"iievent",
9, /* number of columns */
"CREATE TABLE iievent("
" event_name char(##DB_EVENT_MAXNAME##) not null default ' ',"
" event_owner char(##DB_OWN_MAXNAME##) not null default ' ',"
" event_create ingresdate not null default ' ',"
" event_type integer not null default 0,"
" event_idbase integer not null default 0,"
" event_idx integer not null default 0,"
" event_qryid1 integer not null default 0,"
" event_qryid2 integer not null default 0,"
" event_free char(8) not null default ' ')"
" with noduplicates",
"MODIFY iievent TO HASH on event_name, event_owner with minpages = 30",
"CREATE INDEX iixevent on iievent (event_idbase, event_idx)"
" with structure = btree",
NULL  /* index 2 */
},

{
0,  /* DMP_ETAB_CATALOG in back!dmf!hdr dmp.h */
"iiextended_relation",
5, /* number of columns */
"CREATE TABLE iiextended_relation("
" etab_base integer not null,"
" etab_extension integer not null,"
" etab_type integer not null,"
" etab_attid integer not null,"
" etab_reserved char(16) not null)"
" with noduplicates",
"MODIFY iiextended_relation to hash on etab_base with minpages=2,allocation=4",
NULL, /* index 1 */
NULL  /* index 2 */
},

/* iigw06_attribute is GWSXA_XATT in back!gwf!gwfsxa gwfsxa.h  */
{
0,
"iigw06_attribute",
6, /* number of columns */
"CREATE TABLE iigw06_attribute("
" reltid integer not null,"
" reltidx integer not null,"
" attid smallint not null,"
" auditid smallint not null,"
" attname char(##DB_MAXNAME##) not null,"
" auditname char(##DB_EXTFMT_SIZE##) not null)"
" with noduplicates",
"MODIFY iigw06_attribute to hash on reltid, reltidx with compression",
NULL, /* index 1 */
NULL  /* index 2 */
},

/* iigw06_relation is GWSXA_XREL in back!gwf!gwfsxa gwfsxa.h  */
{
0,
"iigw06_relation",
5, /* number of columns */
"CREATE TABLE iigw06_relation("
" reltid integer not null,"
" reltidx integer not null,"
" audit_log char(256) not null,"
" reg_date integer not null,"
" flags integer not null)",
"MODIFY iigw06_relation to hash unique on reltid,reltidx",
NULL, /* index 1 */
NULL  /* index 2 */
},

{
0,
"iihistogram",
5, /* number of columns */
"CREATE TABLE iihistogram("
" htabbase integer not null default 0,"
" htabindex integer not null default 0,"
" hatno smallint not null default 0,"
" hsequence smallint not null default 0,"
" histogram byte(228) not null default ' ')"
" with noduplicates",
"MODIFY iihistogram to hash on htabbase, htabindex where minpages = 8",
NULL, /* index 1 */
NULL  /* index 2 */
},

{
0,
"iiintegrity",
52, /* number of columns */
NULL, /* MUST create with QUEL in duc_quel_create() */
"MODIFY iiintegrity to hash on inttabbase, inttabidx where minpages = 8, compression",
"CREATE INDEX iiintegrityidx on iiintegrity "
" (consschema_id1, consschema_id2, consname) "
" with structure = hash, minpages = 32",
NULL  /* index 2 */
},

{
0,
"iikey",
4, /* number of columns */
"CREATE TABLE iikey("
" key_consid1 integer not null,"
" key_consid2 integer not null,"
" key_attid smallint not null,"
" key_position smallint not null)"
" with duplicates",
"MODIFY iikey to hash on key_consid1, key_consid2 with minpages = 32",
NULL, /* index 1 */
NULL  /* index 2 */
},

{
/* iipartname catalog is DB_IIPARTNAME in  back!hdr!hdr dbdbms.h */
sizeof(DB_IIPARTNAME),
"iipartname",
5, /* number of columns */
"CREATE TABLE iipartname("
" mastid integer not null not default,"
" mastidx integer not null not default,"
" levelno smallint not null not default,"
" partseq smallint not null default 0,"
" partname char(##DB_MAXNAME##) not null default ' ')",
"MODIFY iipartname to btree unique on mastid, mastidx, levelno, partseq "
" with fillfactor = 100, compression = (data)",
NULL, /* index 1 */
NULL  /* index 2 */
},

{
/* iipriv catalog is DB_IIPRIV in back!hdr!hdr dbdbms.h */
sizeof(DB_IIPRIV),
"iipriv",
42, /* number of columns */
"CREATE TABLE iipriv("
" d_obj_id integer not null not default,"
" d_obj_idx integer not null not default,"
" d_priv_number smallint not null default 0,"
" d_obj_type smallint not null default 0,"
" i_obj_id integer not null default 0,"
" i_obj_idx integer not null default 0,"
" i_priv integer not null default 0,"
" i_priv_grantee char(##DB_OWN_MAXNAME##) not null default ' ',"
" prv_flags integer not null default 0,"
" i_priv_map1 integer not null default 0,"
" i_priv_map2 integer not null default 0,"
" i_priv_map3 integer not null default 0,"
" i_priv_map4 integer not null default 0,"
" i_priv_map5 integer not null default 0,"
" i_priv_map6 integer not null default 0,"
" i_priv_map7 integer not null default 0,"
" i_priv_map8 integer not null default 0,"
" i_priv_map9 integer not null default 0,"
" i_priv_mapa integer not null default 0,"
" i_priv_map11 integer not null default 0,"
" i_priv_map12 integer not null default 0,"
" i_priv_map13 integer not null default 0,"
" i_priv_map14 integer not null default 0,"
" i_priv_map15 integer not null default 0,"
" i_priv_map16 integer not null default 0,"
" i_priv_map17 integer not null default 0,"
" i_priv_map18 integer not null default 0,"
" i_priv_map19 integer not null default 0,"
" i_priv_map20 integer not null default 0,"
" i_priv_map21 integer not null default 0,"
" i_priv_map22 integer not null default 0,"
" i_priv_map23 integer not null default 0,"
" i_priv_map24 integer not null default 0,"
" i_priv_map25 integer not null default 0,"
" i_priv_map26 integer not null default 0,"
" i_priv_map27 integer not null default 0,"
" i_priv_map28 integer not null default 0,"
" i_priv_map29 integer not null default 0,"
" i_priv_map30 integer not null default 0,"
" i_priv_map31 integer not null default 0,"
" i_priv_map32 integer not null default 0,"
" i_priv_map33 integer not null default 0)"
" with duplicates",
"MODIFY  iipriv to btree on d_obj_id, d_obj_idx, d_priv_number"
" with fillfactor = 100",
"CREATE INDEX iixpriv on  iipriv (i_obj_id, i_obj_idx, i_priv, i_priv_grantee)"
" with structure = btree, key = (i_obj_id, i_obj_idx, i_priv)",
NULL  /* index 2 */
},

{
/* Note iiprivlist, these are listed in psluser.c, but are strings
*  The longest string we put in iiprivlist is "maintain locations"
*/
0,
"iiprivlist",
1, /* number of columns */
"CREATE TABLE iiprivlist("
" privname char(32) not null not default)"
" with duplicates",
NULL, /* MODIFY */
NULL, /* index 1 */
NULL  /* index 2 */
},

{
/* iiprocedure catalog is DB_PROCEDURE in back!hdr!hdr dbdbms.h */
sizeof(DB_PROCEDURE),
"iiprocedure",
18, /* number of columns */
"CREATE TABLE iiprocedure("
" dbp_name char(##DB_MAXNAME##) not null default ' ',"
" dbp_owner char(##DB_OWN_MAXNAME##) not null default ' ',"
" dbp_txtlen integer not null default 0,"
" dbp_txtid1 integer not null default 0,"
" dbp_txtid2 integer not null default 0,"
" dbp_mask1 integer not null default 0,"
" dbp_mask2 integer not null default 0,"
" dbp_id integer not null default 0,"
" dbp_idx integer not null default 0,"
" dbp_parameter_count integer not null default 0,"
" dbp_record_width integer not null default 0,"
" dbp_ubt_id integer not null default 0,"
" dbp_ubt_idx integer not null default 0,"
" dbp_create_date integer not null default 0,"
" dbp_rescol_count integer not null default 0,"
" dbp_resrow_width integer not null default 0,"
" dbp_est_rows integer not null default 0,"
" dbp_est_cost integer not null default 0)",
"MODIFY iiprocedure to btree unique on dbp_name, dbp_owner"
" with fillfactor = 100, compression = (data)",
"CREATE INDEX iixprocedure on iiprocedure (dbp_id, dbp_idx)"
" with structure = btree",
NULL  /* index 2 */
},

{
/* iiprocedure_parameter is DB_PROCEDURE_PARAMETER in back!hdr!hdr dbdbms.h */
sizeof(DB_PROCEDURE_PARAMETER),
"iiprocedure_parameter",
12, /* number of columns */
"CREATE TABLE iiprocedure_parameter("
" pp_procid1 integer not null,"
" pp_procid2 integer not null,"
" pp_name char(##DB_MAXNAME##) not null,"
" pp_defid1 integer not null,"
" pp_defid2 integer not null,"
" pp_flags integer not null,"
" pp_number smallint not null,"
" pp_offset smallint not null,"
" pp_length smallint not null,"
" pp_datatype smallint not null,"
" pp_precision smallint not null,"
" pp_pad smallint not null)"
" with duplicates",
"MODIFY iiprocedure_parameter to hash on pp_procid1, pp_procid2"
" with minpages = 32, compression = (data)",
NULL, /* index 1 */
NULL  /* index 2 */
},

{
/* iiprotect is DB_PROTECTION in back!hdr!hdr dbdbms.h */
sizeof(DB_PROTECTION),
"iiprotect",
62, /* number of columns */
NULL, /* MUST create with QUEL in duc_quel_create() */
"MODIFY iiprotect to btree on protabbase, protabidx with fillfactor=100, compression = (data)",
NULL, /* index 1 */
NULL  /* index 2 */
},

{
0,
"iiqrytext",
6, /* number of columns */
NULL, /* MUST create with QUEL in duc_quel_craete() */
"MODIFY iiqrytext to btree on txtid1, txtid2, mode, seq with fillfactor=100",
NULL, /* index 1 */
NULL  /* index 2 */
},

{
0,
"iirange",
14, /* number of columns */
"CREATE TABLE iirange("
" rng_baseid integer not null,"
" rng_indexid integer not null,"
" rng_ll1 float not null,"
" rng_ll2 float not null,"
" rng_ll3 float not null,"
" rng_ll4 float not null,"
" rng_ur1 float not null,"
" rng_ur2 float not null,"
" rng_ur3 float not null,"
" rng_ur4 float not null,"
" rng_dimension smallint not null,"
" rng_hilbertsize smallint not null,"
" rng_rangedt smallint not null,"
" rng_rangetype char(1) not null)"
" with duplicates",
"MODIFY iirange to hash on rng_baseid, rng_indexid "
" with minpages=2,allocation=4",
NULL, /* index 1 */
NULL  /* index 2 */
},

{
/* iirule catalog is DB_IIRULE in back!hdr!hdr dbdbms.h */
sizeof(DB_IIRULE),
"iirule",
53, /* number of columns */
"CREATE TABLE iirule("
" rule_name char(##DB_MAXNAME##) not null default ' ',"
" rule_owner char(##DB_OWN_MAXNAME##) not null default ' ',"
" rule_type smallint not null default 0,"
" rule_flags smallint not null default 0,"
" rule_tabbase integer not null default 0,"
" rule_tabidx integer not null default 0,"
" rule_qryid1 integer not null default 0,"
" rule_qryid2 integer not null default 0,"
" rule_treeid1 integer not null default 0,"
" rule_treeid2 integer not null default 0,"
" rule_statement integer not null default 0,"
" rule_dbp_name char(##DB_MAXNAME##) not null default ' ',"
" rule_dbp_owner char(##DB_OWN_MAXNAME##) not null default ' ',"
" rule_time_date ingresdate not null default ' ',"
" rule_time_int ingresdate not null default ' ',"
" rule_col1 integer not null default 0,"
" rule_col2 integer not null default 0,"
" rule_col3 integer not null default 0,"
" rule_col4 integer not null default 0,"
" rule_col5 integer not null default 0,"
" rule_col6 integer not null default 0,"
" rule_col7 integer not null default 0,"
" rule_col8 integer not null default 0,"
" rule_col9 integer not null default 0,"
" rule_cola integer not null default 0,"
" rule_col11 integer not null default 0,"
" rule_col12 integer not null default 0,"
" rule_col13 integer not null default 0,"
" rule_col14 integer not null default 0,"
" rule_col15 integer not null default 0,"
" rule_col16 integer not null default 0,"
" rule_col17 integer not null default 0,"
" rule_col18 integer not null default 0,"
" rule_col19 integer not null default 0,"
" rule_col20 integer not null default 0,"
" rule_col21 integer not null default 0,"
" rule_col22 integer not null default 0,"
" rule_col23 integer not null default 0,"
" rule_col24 integer not null default 0,"
" rule_col25 integer not null default 0,"
" rule_col26 integer not null default 0,"
" rule_col27 integer not null default 0,"
" rule_col28 integer not null default 0,"
" rule_col29 integer not null default 0,"
" rule_col30 integer not null default 0,"
" rule_col31 integer not null default 0,"
" rule_col32 integer not null default 0,"
" rule_col33 integer not null default 0,"
" rule_id1 integer not null default 0,"
" rule_id2 integer not null default 0,"
" rule_dbp_param smallint not null default 0,"
" rule_pad smallint not null default 0,"
" rule_free char(8) not null default ' ')"
" with noduplicates",
"MODIFY iirule TO CHASH on rule_tabbase, rule_tabidx with minpages = 32",
"CREATE INDEX iiruleidx ON iirule (rule_id1, rule_id2)"
" WITH STRUCTURE = HASH, MINPAGES = 32",
"CREATE UNIQUE INDEX iiruleidx1 ON iirule (rule_owner, rule_name)"
" WITH STRUCTURE = HASH, MINPAGES = 32"
},

{
/* iischema catalog is DB_IISCHEMA in back!hdr!hdr dbdbms.h */
sizeof(DB_IISCHEMA),
"iischema",
4, /* number of columns */
"CREATE TABLE iischema("
" schema_name char(##DB_SCHEMA_MAXNAME##) not null,"
" schema_owner char(##DB_OWN_MAXNAME##) not null,"
" schema_id integer not null,"
" schema_idx integer not null)"
" with duplicates",
"MODIFY  iischema to hash unique on schema_name where minpages = 8",
"CREATE index iischemaidx on  iischema (schema_id, schema_idx)"
" with structure = hash",
NULL  /* index 2 */
},

{
/* iisecalarm catalog is DB_SECALARM in back!hdr!hdr dbdbms.h */
/* subj_name is DB_OWN_NAME */
sizeof(DB_SECALARM),
"iisecalarm",
19, /* number of columns */
"CREATE TABLE iisecalarm("
" alarm_name char(##DB_ALARM_MAXNAME##) not null default ' ',"
" alarm_num integer not null default 0,"
" obj_id1 integer not null default 0,"
" obj_id2 integer not null default 0,"
" obj_name char(##DB_MAXNAME##) not null default ' ',"
" obj_type char(1) not null default ' ',"
" subj_type i1 not null default 0,"
" subj_name char(##DB_OWN_MAXNAME##) not null default ' ',"
" alarm_flags smallint not null default 0,"
" alarm_txtid1 integer not null default 0,"
" alarm_txtid2 integer not null default 0,"
" alarm_opctl integer not null default 0,"
" alarm_opset integer not null default 0,"
" event_id1 integer not null default 0,"
" event_id2 integer not null default 0,"
" event_text char(256) not null default ' ',"
" alarm_id1 integer not null default 0,"
" alarm_id2 integer not null default 0,"
" alarm_reserve char(32) not null default ' ')"
" with noduplicates",
"MODIFY  iisecalarm to hash on obj_type, obj_id1, obj_id2 with minpages=32, compression",
NULL, /* index 1 */
NULL  /* index 2 */
},

{
/* FIX ME iisectype */
0,
"iisectype",
5, /* number of columns */
"CREATE TABLE iisectype("
" sec_type c16,"
" sec_name c##DB_TYPE_MAXLEN##,"
" sec_typenum smallint,"
" sec_namenum smallint,"
" sec_index smallint)"
" with duplicates",
"MODIFY iisectype to BTREE UNIQUE on sec_type, sec_name",
NULL, /* index 1 */
NULL  /* index 2 */
},

{
/* iisequence catalog is DB_IISEQUENCE in back!hdr!hdr dbdbms.h */
sizeof(DB_IISEQUENCE),
"iisequence",
22, /* number of columns */
"CREATE TABLE iisequence("
" seq_name char(##DB_MAXNAME##) not null default ' ',"
" seq_owner char(##DB_OWN_MAXNAME##) not null default ' ',"
" seq_create ingresdate not null default ' ',"
" seq_MODIFY ingresdate not null default ' ',"
" seq_idbase integer not null default 0,"
" seq_idx integer not null default 0,"
" seq_type smallint not null default 0,"
" seq_length smallint not null default 0,"
" seq_prec integer not null default 0,"
" seq_start_dec decimal(31,0) not null default 0,"
" seq_start_int i8 not null default 0,"
" seq_incr_dec decimal(31,0) not null default 0,"
" seq_incr_int i8 not null default 0,"
" seq_next_dec decimal(31,0) not null default 0,"
" seq_next_int i8 not null default 0,"
" seq_min_dec decimal(31,0) not null default 0,"
" seq_min_int i8 not null default 0,"
" seq_max_dec decimal(31,0) not null default 0,"
" seq_max_int i8 not null default 0,"
" seq_cache integer not null default 0,"
" seq_flag integer not null default 0,"
" seq_free char(8) not null default ' ')"
" with noduplicates",
"MODIFY iisequence to hash on seq_name, seq_owner with minpages = 30, compression",
NULL, /* index 1 */
NULL  /* index 2 */
},

/* iistatistics could probably be HASH UNIQUE ??? */
{
0,
"iistatistics",
13, /* number of columns */
"CREATE TABLE iistatistics("
" stabbase integer not null default 0,"
" stabindex integer not null default 0,"
" snunique f4 not null default 0,"
" sreptfact f4 not null default 0,"
" snull f4 not null default 0,"
" satno smallint not null default 0,"
" snumcells smallint not null default 0,"
" sdomain smallint not null default 0,"
" scomplete i1 not null default 0,"
" sunique i1 not null default 0,"
" sdate ingresdate not null default ' ',"
" sversion char(8) not null default ' ',"
" shistlength smallint not null default 0)",
"MODIFY iistatistics to hash on stabbase, stabindex where minpages=8",
NULL, /* index 1 */
NULL  /* index 2 */
},

{
/* iisynonym catalog is DB_IISYNONYM in back!hdr!hdr dbdbms.h */
sizeof(DB_IISYNONYM),
"iisynonym",
6, /* number of columns */
"CREATE TABLE iisynonym("
" synonym_name char(##DB_MAXNAME##) not null,"
" synonym_owner char(##DB_OWN_MAXNAME##) not null,"
" syntabbase integer not null,"
" syntabidx integer not null,"
" synid integer not null,"
" synidx integer not null)"
" with duplicates",
"MODIFY iisynonym to hash unique on synonym_name, synonym_owner with compression",
"CREATE INDEX iixsynonym  on iisynonym (syntabbase, syntabidx)"
" with structure = btree",
NULL  /* index 2 */
},

{
0,
"iitree",
8, /* number of columns */
NULL, /* MUST create with QUEL in duc_quel_create() */
"MODIFY iitree to btree on treetabbase, treetabidx, treemode, treeseq"
" with fillfactor = 100, compression = (data)",
NULL, /* index 1 */
NULL  /* index 2 */
},

   /* Below are the iidbdb ONLY catalogs */

{
/* iidatabase is DU_DATABASE in common!hdr!hdr dudbms.qsh */
sizeof(DU_DATABASE),
"iidatabase",
13, /* number of columns */
"CREATE TABLE iidatabase("
" name char(##DB_DB_MAXNAME##) not null default ' ',"
" own char(##DB_OWN_MAXNAME##) not null default ' ',"
" dbdev char(##DB_LOC_MAXNAME##) not null default ' ',"
" ckpdev char(##DB_LOC_MAXNAME##) not null default ' ',"
" jnldev char(##DB_LOC_MAXNAME##) not null default ' ',"
" sortdev char(##DB_LOC_MAXNAME##) not null default ' ',"
" access integer not null default 0,"
" dbservice integer not null default 0,"
" dbcmptlvl integer not null default ' ',"
" dbcmptminor integer not null default 0,"
" db_id integer not null default 0,"
" dmpdev char(##DB_LOC_MAXNAME##) not null default ' ',"
" dbfree char(8) not null default ' ')",
"MODIFY iidatabase to hash unique on name where minpages = 5",
"CREATE unique index iidbid_idx on iidatabase (db_id) with structure = hash",
NULL /* index 2 */
},

{
/* iidbpriv is DB_PRIVILEGES in back!hdr!hdr dbdbms.h */
/* grantee is DB_OWN_NAME  */
sizeof(DB_PRIVILEGES),
"iidbpriv",
15, /* number of columns */
"CREATE TABLE iidbpriv("
" dbname char(##DB_DB_MAXNAME##) not null default ' ',"
" grantee char(##DB_OWN_MAXNAME##) not null default ' ',"
" gtype smallint not null default 0,"
" dbflags smallint not null default 0,"
" control integer not null default 0,"
" flags integer not null default 0,"
" qrow_limit integer not null default 0,"
" qdio_limit integer not null default 0,"
" qcpu_limit integer not null default 0,"
" qpage_limit integer not null default 0,"
" qcost_limit integer not null default 0,"
" idle_time_limit integer not null default 0,"
" connect_time_limit integer not null default 0,"
" priority_limit integer not null default 0,"
" reserve char(32) not null default ' ')",
/*
** Note: iidbpriv should remain btree since QEF will
** perform partial searches that are not allowed against
** hashed tables
*/
"MODIFY iidbpriv to btree unique on grantee, gtype, dbname with fillfactor=100",
NULL, /* index 1 */
NULL  /* index 2 */
},

{
/* iiddb_netcost is DD_NETCOST in back!hdr!hdr ddb.h */
sizeof(DD_NETCOST),
"iiddb_netcost",
5, /* number of columns */
"CREATE TABLE iiddb_netcost("
" net_src char(##DB_NODE_MAXNAME##) not null default ' ',"
" net_dest char(##DB_NODE_MAXNAME##) not null default ' ',"
" net_cost float not null default 0,"
" net_exp1 float not null default 0,"
" net_exp2 float not null default 0)"
" with noduplicates",
NULL, /* MODIFY */
NULL, /* index 1 */
NULL  /* index 2 */
},

{
/* iiddb_nodecosts is DD_COSTS in back!hdr!hdr ddb.h */
sizeof(DD_COSTS),
"iiddb_nodecosts",
11, /* number of columns */
"CREATE TABLE iiddb_nodecosts("
" cpu_name char(##DB_NODE_MAXNAME##) not null default ' ',"
" cpu_cost float not null default 0,"
" dio_cost float not null default 0,"
" create_cost float not null default 0,"
" page_size integer not null default 0,"
" block_read integer not null default 0,"
" sort_size integer not null default 0,"
" cache_size integer not null default 0,"
" cpu_exp0 float not null default 0,"
" cpu_exp1 float not null default 0,"
" cpu_exp2 float not null default 0)"
" with noduplicates",
NULL, /* MODIFY */
NULL, /* index 1 */
NULL  /* index 2 */
},

{
/* iiextend is DU_EXTEND in common!hdr!hdr dudbms.qsh */
sizeof(DU_EXTEND),
"iiextend",
5, /* number of columns */
"CREATE TABLE iiextend("
" lname varchar(##DB_LOC_MAXNAME##) not null default ' ',"
" dname varchar(##DB_DB_MAXNAME##) not null default ' ',"
" status integer not null default 0,"
" rawstart integer not null default 0,"
" rawblocks integer not null default 0)"
" with noduplicates",
"MODIFY iiextend to hash on dname where minpages = 4",
NULL, /* index 1 */
NULL  /* index 2 */
},

{
/* iigw07_attribute is GM_XATT_TUPLE in gwf/gwm/gwmint.h */
0, /* FIXME sizeof(GM_XATT_TUPLE) */
"iigw07_attribute",
4, /* number of columns */
"CREATE TABLE iigw07_attribute("
" tblid integer not null not default,"
" tblidx integer not null not default,"
" attnum smallint not null not default,"
" classid char(##DB_EXTFMT_SIZE##) not null not default)"
" with duplicates",
"MODIFY iigw07_attribute to btree on tblid, tblidx, attnum",
NULL, /* index 1 */
NULL  /* index 2 */
},

{
/* iigw07_index is GM_XIDX_TUPLE in gwf/gwm/gwmint.h */
0, /* FIXME sizeof(GM_XIDX_TUPLE) */
"iigw07_index",
4, /* number of columns */
"CREATE TABLE iigw07_index("
" tblid integer not null not default,"
" tblidx integer not null not default,"
" attnum smallint not null not default,"
" classid char(##DB_EXTFMT_SIZE##) not null not default)"
" with duplicates",
"MODIFY iigw07_index to btree on tblid, tblidx, attnum",
NULL, /* index 1 */
NULL  /* index 2 */
},

{
0,
"iigw07_relation",
3, /* number of columns */
"CREATE TABLE iigw07_relation("
" tblid integer not null not default,"
" tblidx integer not null not default,"
" flags integer not null not default)"
" with duplicates",
"MODIFY iigw07_relation to btree on tblid, tblidx",
NULL, /* index 1 */
NULL  /* index 2 */
},

{
/* iilocations is DU_LOCATIONS in common!hdr!hdr dudbms.qsh */
sizeof(DU_LOCATIONS),
"iilocations",
5, /* number of columns */
"CREATE TABLE iilocations("
" status integer not null default 0,"
" lname varchar(##DB_LOC_MAXNAME##) not null default ' ',"
" area varchar(128) not null default ' ',"
" free char(8) not null default ' ',"
" rawpct integer not null default 0)",
"MODIFY iilocations to hash unique on lname where minpages = 4",
NULL, /* index 1 */
NULL  /* index 2 */
},

{
/* iiprofile is DU_PROFILE in common!hdr!hdr dudbms.qsh */
/* name and default_group are DB_OWN_NAME */
sizeof(DU_PROFILE),
"iiprofile",
7, /* number of columns */
"CREATE TABLE iiprofile("
" name char(##DB_OWN_MAXNAME##) not null default ' ',"
" status integer not null default 0,"
" default_group char(##DB_OWN_MAXNAME##) not null default ' ',"
" reserve char(8) not null default ' ',"
" default_priv integer not null default 0,"
" expire_date ingresdate not null default ' ',"
" flags_mask integer not null default 0)",
"MODIFY iiprofile to hash unique on name",
NULL, /* index 1 */
NULL  /* index 2 */
},

{
/* iirole is DB_APPLICATION_ID in back!hdr!hdr dbdbms.h */
sizeof(DB_APPLICATION_ID),
"iirole",
6, /* number of columns */
"CREATE TABLE iirole("
" roleid char(##DB_OWN_MAXNAME##) not null default ' ',"
" rolepass char(24) not null default ' ',"
" reserve1 char(16) not null default ' ',"
" rolestatus integer not null default 0,"
" roleflags integer not null default 0,"
" reserve char(8) not null default ' ')",
"MODIFY iirole to hash unique on roleid",
NULL, /* index 1 */
NULL  /* index 2 */
},

{
/* iirolegrant is DB_ROLEGRANT in back!hdr!hdr dbdbms.h */
/* grantee is DB_OWN_NAME */
sizeof(DB_ROLEGRANT),
"iirolegrant",
5, /* number of columns */
"CREATE TABLE iirolegrant("
" rgr_rolename char(##DB_OWN_MAXNAME##) not null default ' ',"
" rgr_flags integer not null default 0,"
" rgr_gtype smallint not null default 0,"
" rgr_grantee char(##DB_OWN_MAXNAME##) not null default ' ',"
" rgr_reserve char(34) not null default ' ')",
"MODIFY iirolegrant to btree unique on rgr_grantee, rgr_rolename",
NULL, /* index 1 */
NULL  /* index 2 */
},

{
/* iisecuritystate is DU_SECSTATE in common!hdr!hdr dudbms.qsh */
sizeof(DU_SECSTATE),
"iisecuritystate",
3, /* number of columns */
"CREATE TABLE iisecuritystate("
" type integer not null default 0,"
" id integer not null default 0,"
" state integer not null default 0)"
" with noduplicates",
NULL, /* MODIFY */
NULL, /* index 1 */
NULL  /* index 2 */
},

{
/* iistar_cdbs is TPC_I1_STARCDBS in back!tpf!hdr tpfcat.h  */
0,
"iistar_cdbs",
11, /* number of columns */
"CREATE TABLE iistar_cdbs("
" ddb_name char(##DB_DB_MAXNAME##) not null default ' ',"
" ddb_owner char(##DB_OWN_MAXNAME##) not null default ' ',"
" cdb_name char(##DB_DB_MAXNAME##) not null default ' ',"
" cdb_owner char(##DB_OWN_MAXNAME##) not null default ' ',"
" cdb_node char(##DB_NODE_MAXNAME##) not null default ' ',"
" cdb_dbms char(##DB_TYPE_MAXLEN##) not null default ' ',"
" scheme_desc char(##DB_SCHEMA_MAXNAME##) not null default ' ',"
" create_date char(25) not null default ' ',"
" original char(8) not null default ' ',"
" cdb_id integer not null default 0,"
" cdb_capability integer not null default 0)"
" with noduplicates",
NULL, /* MODIFY */
"CREATE INDEX iicdbid_idx on iistar_cdbs (cdb_id) with structure = hash",
NULL  /* index 2 */
},

{
/* iiuser is DU_USER in common!hdr!hdr dudbms.qsh */
/* name and default group are DB_OWN_NAME */
sizeof(DU_USER),
"iiuser",
12, /* number of columns */
"CREATE TABLE iiuser("
" name char(##DB_OWN_MAXNAME##) not null default ' ',"
" gid smallint not null default 0,"
" mid smallint not null default 0,"
" status integer not null default 0,"
" default_group char(##DB_OWN_MAXNAME##) not null default ' ',"
" password char(24) not null default ' ',"
" reserve char(8) not null default ' ',"
" profile_name char(##DB_OWN_MAXNAME##) not null default ' ',"
" default_priv integer not null default 0,"
" expire_date ingresdate not null default ' ',"
" flags_mask integer not null default 0,"
" user_priv integer not null default 0)",
"MODIFY iiuser to hash unique on name where minpages = 4",
NULL, /* index 1 */
NULL  /* index 2 */
},

{
/* iiusergroup is DB_USERGROUP in back!hdr!hdr dbdbms.h */
/* groupid and groupmem are DB_OWN_NAME */
sizeof(DB_USERGROUP),
"iiusergroup",
3, /* number of columns */
"CREATE TABLE iiusergroup("
" groupid char(##DB_OWN_MAXNAME##) not null default ' ',"
" groupmem char(##DB_OWN_MAXNAME##) not null default ' ',"
" reserve char(32) not null default ' ')",
"MODIFY iiusergroup to btree unique on groupid, groupmem",
NULL, /* index 1 */
NULL  /* index 2 */
},

/*
** Below are tables created only in a STAR database
** Structures in:
** back!tpf!hdr tpfcat.h
** back!qef!hdr!qefcat.h
** are used when selecting from star catalogs
** The structures do not map to row images. 
** Instead they are defined to hold host variable data, 
** chars in these structs are sizeof(column) + 1 for null
*/
    
{
0, /* iidd_alt_columns QEC_L2_ALT_COLUMNS in back!qef!hdr qefcat.h */
"iidd_alt_columns",
5, /* number of columns */
"CREATE TABLE iidd_alt_columns("
" table_name char(##DB_MAXNAME##) not null default ' ',"
" table_owner char(##DB_OWN_MAXNAME##) not null default ' ',"
" key_id integer not null default 0,"
" column_name char(##DB_MAXNAME##) not null default ' ',"
" key_sequence smallint not null default 0)"
" with noduplicates",
"MODIFY iidd_alt_columns to hash on table_name, table_owner with compression",
NULL, /* index 1 */
NULL, /* index 2 */
},

{
0, /* iidd_columns is QEC_L3_COLUMNS in back!qef!hdr qefcat.h */
"iidd_columns",
12, /* number of columns */
"CREATE TABLE iidd_columns("
" table_name char(##DB_MAXNAME##) not null default ' ',"
" table_owner char(##DB_OWN_MAXNAME##) not null default ' ',"
" column_name char(##DB_MAXNAME##) not null default ' ',"
" column_datatype char(##DB_TYPE_MAXLEN##) not null default ' ',"
" column_length integer not null default 0,"
" column_scale smallint not null default 0,"
" column_nulls char(8) not null default ' ',"
" column_defaults char(8) not null default ' ',"
" column_sequence smallint not null default 0,"
" key_sequence smallint not null default 0,"
" sort_direction char(8) not null default ' ',"
" column_ingdatatype smallint not null default 0)"
" with noduplicates",
"MODIFY iidd_columns to hash on table_name, table_owner with compression",
NULL, /* index 1 */
NULL, /* index 2 */
},

{
0, /* iidd_dbcapabilities is QEC_L4_DBCAPABILITIES in back!qef!hdr qefcat.h */
"iidd_dbcapabilities",
2, /* number of columns */
"CREATE TABLE iidd_dbcapabilities("
" cap_capability char(32) not null default ' ',"
" cap_value char(32) not null default ' ')"
" with noduplicates",
"MODIFY iidd_dbcapabilities to heap",
NULL, /* index 1 */
NULL, /* index 2 */
},

{
0, /* iidd_ddb_dbdepends is QEC_D1_DBDEPENDS in back!qef!hdr qefcat.h */
"iidd_ddb_dbdepends",
7, /* number of columns */
"CREATE TABLE iidd_ddb_dbdepends("
" inid1 integer not null default 0,"
" inid2 integer not null default 0,"
" itype integer not null default 0,"
" deid1 integer not null default 0,"
" deid2 integer not null default 0,"
" dtype integer not null default 0,"
" qid integer not null default 0)"
" with noduplicates",
"MODIFY iidd_ddb_dbdepends to hash on inid1, inid2, itype",
NULL, /* index 1 */
NULL, /* index 2 */
},

{
0, /* iidd_ddb_dxldbs is TPC_D2_DXLDBS in back!tpf!hdr tpfcat.h */
"iidd_ddb_dxldbs",
11, /* number of columns */
"CREATE TABLE iidd_ddb_dxldbs("
" ldb_dxid1 integer not null not default,"
" ldb_dxid2 integer not null not default,"
" ldb_node char(##DB_NODE_MAXNAME##) not null not default,"
" ldb_name char(256) not null not default,"
" ldb_dbms char(##DB_TYPE_MAXLEN##) not null not default,"
" ldb_id integer not null not default,"
" ldb_lxstate integer not null not default,"
" ldb_lxid1 integer not null not default,"
" ldb_lxid2 integer not null not default,"
" ldb_lxname char(##DB_DB_MAXNAME##) not null not default,"
" ldb_lxflags integer not null not default)"
" with noduplicates",
"MODIFY iidd_ddb_dxldbs to heap",
NULL,
NULL
},

{
/* iidd_ddb_dxlog is TPC_D1_DXLOG in back!tpf!hdr tpfcat.h */
0,
"iidd_ddb_dxlog",
13, /* number of columns */
"CREATE TABLE iidd_ddb_dxlog("
" dx_id1 integer not null not default,"
" dx_id2 integer not null not default,"
" dx_name char(##DB_DB_MAXNAME##) not null not default,"
" dx_flags integer not null not default,"
" dx_state integer not null not default,"
" dx_create char(25) not null not default,"
" dx_MODIFY char(25) not null not default,"
" dx_starid1 integer not null not default,"
" dx_starid2 integer not null not default,"
" dx_ddb_node char(##DB_NODE_MAXNAME##) not null not default,"
" dx_ddb_name char(256) not null not default,"
" dx_ddb_dbms char(##DB_TYPE_MAXLEN##) not null not default,"
" dx_ddb_id integer not null not default)"
" with noduplicates",
"MODIFY iidd_ddb_dxlog to heap",
"CREATE UNIQUE INDEX iidd_ddb_xdxlog on iidd_ddb_dxlog (dx_id1, dx_id2)"
" with structure = hash",
NULL
},

{
0,
"iidd_ddb_ldb_columns",
4, /* number of columns */
"CREATE TABLE iidd_ddb_ldb_columns("
" object_base integer not null default 0,"
" object_index integer not null default 0,"
" local_column char(##DB_MAXNAME##) not null default ' ',"
" column_sequence integer not null default 0)"
" with noduplicates",
"MODIFY iidd_ddb_ldb_columns to hash on"
" object_base, object_index, column_sequence with compression",
NULL,
NULL
},

{
0,
"iidd_ddb_ldb_dbcaps",
4, /* number of columns */
"CREATE TABLE iidd_ddb_ldb_dbcaps("
" ldb_id integer not null default 0,"
" cap_capability char(32) not null default ' ',"
" cap_value char(32) not null default ' ',"
" cap_level integer not null default 0)"
" with noduplicates",
"MODIFY iidd_ddb_ldb_dbcaps to hash on ldb_id",
NULL,
NULL
},

{
0,
"iidd_ddb_ldbids",
8, /* number of columns */
"CREATE TABLE iidd_ddb_ldbids("
" ldb_node char(##DB_NODE_MAXNAME##) not null default ' ',"
" ldb_dbms char(##DB_TYPE_MAXLEN##) not null default ' ',"
" ldb_database char(##DB_DB_MAXNAME##) not null default ' ',"
" ldb_longname char(8) not null default ' ',"
" ldb_id integer not null default 0,"
" ldb_user char(8) not null default ' ',"
" ldb_dba char(8) not null default ' ',"
" ldb_dbaname char(##DB_OWN_MAXNAME##) not null default ' ')"
" with noduplicates",
"MODIFY iidd_ddb_ldbids to hash on ldb_node, ldb_dbms, ldb_database",
NULL,
NULL
},

{
0,
"iidd_ddb_long_ldbnames",
3, /* number of columns */
"CREATE TABLE iidd_ddb_long_ldbnames("
" long_ldbname char(256) not null default ' ',"
" ldb_id integer not null default 0,"
" long_alias char(##DB_DB_MAXNAME##) not null default ' ')"
" with noduplicates",
"MODIFY iidd_ddb_long_ldbnames to hash on long_ldbname, ldb_id",
NULL,
NULL
},

{
0,
"iidd_ddb_object_base",
1, /* number of columns */
"CREATE TABLE iidd_ddb_object_base("
" object_base integer not null default 0)"
" with noduplicates",
"MODIFY iidd_ddb_object_base to heap",
NULL,
NULL
},

{
0, /* back!qef!hdr qefcat.h QEC_D6_OBJECTS */
"iidd_ddb_objects",
12, /* number of columns */
"CREATE TABLE iidd_ddb_objects("
" object_name char(##DB_MAXNAME##) not null default ' ',"
" object_owner char(##DB_OWN_MAXNAME##) not null default ' ',"
" object_base integer not null default 0,"
" object_index integer not null default 0,"
" qid1 integer not null default 0,"
" qid2 integer not null default 0,"
" create_date char(25) not null default ' ',"
" object_type char(8) not null default ' ',"
" alter_date char(25) not null default ' ',"
" system_object char(8) not null default ' ',"
" to_expire char(8) not null default ' ',"
" expire_date char(25) not null default ' ')"
" with noduplicates",
"MODIFY iidd_ddb_objects to hash on object_name, object_owner with compression",
NULL,
NULL,
},

{
0, /* iidd_ddb_tableinfo is QEC_D9_TABLEINFO in back!qef!hdr qefcat.h */
"iidd_ddb_tableinfo",
11, /* number of columns */
"CREATE TABLE iidd_ddb_tableinfo("
" object_base integer not null default 0,"
" object_index integer not null default 0,"
" local_type char(8) not null default ' ',"
" table_name char(##DB_MAXNAME##) not null default ' ',"
" table_owner char(##DB_OWN_MAXNAME##) not null default ' ',"
" table_date char(25) not null default ' ',"
" table_alter char(25) not null default ' ',"
" table_relstamp1 integer not null default 0,"
" table_relstamp2 integer not null default 0,"
" columns_mapped char(8) not null default ' ',"
" ldb_id integer not null default 0)"
" with noduplicates",
"MODIFY iidd_ddb_tableinfo to hash on object_base, object_index with compression",
NULL,
NULL
},

{
0, /* iidd_ddb_tree is QEC_D10_TREE in back!qef!hdr qefcat.h */
"iidd_ddb_tree",
8, /* number of columns */
"CREATE TABLE iidd_ddb_tree("
" treetabbase integer not null default 0,"
" treetabidx integer not null default 0,"
" treeid1 integer not null default 0,"
" treeid2 integer not null default 0,"
" treeseq smallint not null default 0,"
" treemode smallint not null default 0,"
" treevers smallint not null default 0,"
" treetree varchar(1024) not null default ' ')"
" with noduplicates",
"MODIFY iidd_ddb_tree to hash on treetabbase, treetabidx, treemode, treeseq",
NULL,
NULL
},

{
0, /* iidd_histograms is QEC_L5_HISTOGRAMS in back!qef!hdr qefcat.h */
"iidd_histograms",
5, /* number of columns */
"CREATE TABLE iidd_histograms("
" table_name char(##DB_MAXNAME##) not null default ' ',"
" table_owner char(##DB_OWN_MAXNAME##) not null default ' ',"
" column_name char(##DB_MAXNAME##) not null default ' ',"
" text_sequence smallint not null default 0,"
" text_segment char(228) not null default ' ')"
" with noduplicates",
"MODIFY iidd_histograms to hash on table_name, table_owner, column_name with compression",
NULL,
NULL
},

{
0,  /* iidd_index_columns is QEC_L7_INDEX_COLUMNS in back!qef!hdr qefcat.h */
"iidd_index_columns",
5, /* number of columns */
"CREATE TABLE iidd_index_columns("
" index_name char(##DB_MAXNAME##) not null default ' ',"
" index_owner char(##DB_OWN_MAXNAME##) not null default ' ',"
" column_name char(##DB_MAXNAME##) not null default ' ',"
" key_sequence smallint not null default 0,"
" sort_direction char(8) not null default ' ')"
" with noduplicates",
"MODIFY iidd_index_columns to hash on index_name, index_owner with compression",
NULL,
NULL
},

{
0, /* iidd_indexes is QEC_L6_INDEXES in back!qef!hdr qefcat.h */
"iidd_indexes",
9, /* number of columns */
"CREATE TABLE iidd_indexes("
" index_name char(##DB_MAXNAME##) not null default ' ',"
" index_owner char(##DB_OWN_MAXNAME##) not null default ' ',"
" create_date char(25) not null default ' ',"
" base_name char(##DB_MAXNAME##) not null default ' ',"
" base_owner char(##DB_OWN_MAXNAME##) not null default ' ',"
" storage_structure char(16) not null default ' ',"
" is_compressed char(8) not null default ' ',"
" unique_rule char(8) not null default ' ',"
" index_pagesize integer not null default 0)"
" with noduplicates",
"MODIFY iidd_indexes to hash on index_name, index_owner with compression",
NULL,
NULL
},

{
0,
"iidd_integrities",
6, /* number of columns */
"CREATE TABLE iidd_integrities("
" table_name char(##DB_MAXNAME##) not null default ' ',"
" table_owner char(##DB_OWN_MAXNAME##) not null default ' ',"
" create_date char(25) not null default ' ',"
" integrity_number smallint not null default 0,"
" text_sequence smallint not null default 0,"
" text_segment text(240) not null default ' ')"
" with noduplicates",
"MODIFY iidd_integrities to hash on table_name, table_owner with compression",
NULL,
NULL
},

{
0,
"iidd_multi_locations",
4, /* number of columns */
"CREATE TABLE iidd_multi_locations("
" table_name char(##DB_MAXNAME##) not null default ' ',"
" table_owner char(##DB_OWN_MAXNAME##) not null default ' ',"
" loc_sequence smallint not null default 0,"
" location_name char(##DB_LOC_MAXNAME##) not null default ' ')"
" with noduplicates",
"MODIFY iidd_multi_locations to hash on table_name, table_owner with compression",
NULL,
NULL
},

{
0,
"iidd_permits",
8, /* number of columns */
"CREATE TABLE iidd_permits("
" object_name char(##DB_MAXNAME##) not null default ' ',"
" object_owner char(##DB_OWN_MAXNAME##) not null default ' ',"
" object_type char(8) not null default ' ',"
" create_date char(25) not null default ' ',"
" permit_user char(##DB_OWN_MAXNAME##) not null default ' ',"
" permit_number smallint not null default 0,"
" text_sequence smallint not null default 0,"
" text_segment text(240) not null default ' ')"
" with noduplicates",
"MODIFY iidd_permits to hash on object_name, object_owner with compression",
NULL,
NULL
},

{
0,
"iidd_procedure",
11, /* number of columns */
"CREATE TABLE iidd_procedure("
" dbp_name char(##DB_MAXNAME##) not null default ' ',"
" dbp_owner char(##DB_OWN_MAXNAME##) not null default ' ',"
" dbp_txtlen integer not null default 0,"
" dbp_txtid1 integer not null default 0,"
" dbp_txtid2 integer not null default 0,"
" dbp_mask1 integer not null default 0,"
" dbp_mask2 integer not null default 0,"
" dbp_id integer not null default 0,"
" dbp_idx integer not null default 0,"
" dbp_pad char(12) not null default ' ',"
" procedure_type char(8) not null default ' ')"
" with noduplicates",
"MODIFY iidd_procedure to hash on dbp_name, dbp_owner with compression",
NULL,
NULL
},

{
0,
"iidd_procedures",
6, /* number of columns */
"CREATE TABLE iidd_procedures("
" procedure_name char(##DB_MAXNAME##) not null default ' ',"
" procedure_owner char(##DB_OWN_MAXNAME##) not null default ' ',"
" create_date char(25) not null default ' ',"
" proc_subtype varchar(1) not null default ' ',"
" text_sequence integer not null default 0,"
" text_segment varchar(240) not null default ' ')"
" with noduplicates",
"MODIFY iidd_procedures to cheap",
NULL,
NULL
},

{
0,
"iidd_registrations",
7, /* number of columns */
"CREATE TABLE iidd_registrations("
" object_name char(##DB_MAXNAME##) not null default ' ',"
" object_owner char(##DB_OWN_MAXNAME##) not null default ' ',"
" object_dml char(8) not null default ' ',"
" object_type char(8) not null default ' ',"
" object_subtype char(8) not null default ' ',"
" text_sequence integer not null default 0,"
" text_segment varchar(240) not null default ' ')"
" with noduplicates",
"MODIFY iidd_registrations to hash on object_name, object_owner with compression",
NULL,
NULL
},

{
0,
"iidd_stats",
13, /* number of columns */
"CREATE TABLE iidd_stats("
" table_name char(##DB_MAXNAME##) not null default ' ',"
" table_owner char(##DB_OWN_MAXNAME##) not null default ' ',"
" column_name char(##DB_MAXNAME##) not null default ' ',"
" create_date char(25) not null default ' ',"
" num_unique f4 not null default 0,"
" rept_factor f4 not null default 0,"
" has_unique char(8) not null default ' ',"
" pct_nulls f4 not null default 0,"
" num_cells smallint not null default 0,"
" column_domain smallint not null default 0,"
" is_complete char(8) not null default ' ',"
" stat_version char(8) not null default ' ',"
" hist_data_length smallint not null default 0)"
" with noduplicates",
"MODIFY iidd_stats to hash on table_name, table_owner with compression",
NULL,
NULL
},

{
0,
"iidd_tables",
41, /* number of columns */
"CREATE TABLE iidd_tables("
" table_name char(##DB_MAXNAME##) not null default ' ',"
" table_owner char(##DB_OWN_MAXNAME##) not null default ' ',"
" create_date char(25) not null default ' ',"
" alter_date char(25) not null default ' ',"
" table_type char(8) not null default ' ',"
" table_subtype char(8) not null default ' ',"
" table_version char(8) not null default ' ',"
" system_use char(8) not null default ' ',"
" table_stats char(8) not null default ' ',"
" table_indexes char(8) not null default ' ',"
" is_readonly char(8) not null default ' ',"
" num_rows integer not null default 0,"
" storage_structure char(16) not null default ' ',"
" is_compressed char(8) not null default ' ',"
" duplicate_rows char(8) not null default ' ',"
" unique_rule char(8) not null default ' ',"
" number_pages integer not null default 0,"
" overflow_pages integer not null default 0,"
" row_width integer not null default 0,"
" expire_date integer not null default 0,"
" modify_date char(25) not null default ' ',"
" location_name char(##DB_LOC_MAXNAME##) not null default ' ',"
" table_integrities char(8) not null default ' ',"
" table_permits char(8) not null default ' ',"
" all_to_all char(8) not null default ' ',"
" ret_to_all char(8) not null default ' ',"
" is_journalled char(8) not null default ' ',"
" view_base char(8) not null default ' ',"
" multi_locations char(8) not null default ' ',"
" table_ifillpct smallint not null default 0,"
" table_dfillpct smallint not null default 0,"
" table_lfillpct smallint not null default 0,"
" table_minpages integer not null default 0,"
" table_maxpages integer not null default 0,"
" table_relstamp1 integer not null default 0,"
" table_relstamp2 integer not null default 0,"
" table_reltid integer not null default 0,"
" table_reltidx integer not null default 0,"
" table_pagesize integer not null default 0,"
" table_relversion smallint not null default 0,"
" table_reltotwid integer not null default 0)"
" with noduplicates",
"MODIFY iidd_tables to hash on table_name, table_owner with compression",
NULL,
NULL
},

{
0,
"iidd_views",
6, /* number of columns */
"CREATE TABLE iidd_views("
" table_name char(##DB_MAXNAME##) not null default ' ',"
" table_owner char(##DB_OWN_MAXNAME##) not null default ' ',"
" view_dml char(8) not null default ' ',"
" check_option char(8) not null default ' ',"
" text_sequence integer not null default 0,"
" text_segment varchar(256) not null default ' ')"
" with noduplicates",
"MODIFY iidd_views to hash on table_name, table_owner, text_sequence with compression",
NULL,
NULL
},

    /* Below are registered tables */
{
0,
"lgmo_lgd",
0, /* number of columns */
"REGISTER TABLE lgmo_lgd ("
" place varchar(4)  not null not default is 'VNODE',"
" lgd_status char(100) not null not default is 'exp.dmf.lg.lgd_status',"
" lgd_status_num i4 not null not default is 'exp.dmf.lg.lgd_status_num',"
" lgd_buf_cnt i4 not null not default is 'exp.dmf.lg.lgd_buf_cnt',"
" lgd_lpb_inuse i4 not null not default is 'exp.dmf.lg.lgd_lpb_inuse',"
" lgd_lxb_inuse i4 not null not default is 'exp.dmf.lg.lgd_lxb_inuse',"
" lgd_ldb_inuse i4 not null not default is 'exp.dmf.lg.lgd_ldb_inuse',"
" lgd_lpd_inuse i4 not null not default is 'exp.dmf.lg.lgd_lpd_inuse',"
" lgd_protect_count i4 not null not default"
" is 'exp.dmf.lg.lgd_protect_count',"
" lgd_n_logwriters i4 not null not default"
" is 'exp.dmf.lg.lgd_n_logwriters',"
" lgd_no_bcp i4 not null not default is 'exp.dmf.lg.lgd_no_bcp',"
" lgd_sbk_count i4 not null not default is 'exp.dmf.lg.lgd_sbk_count',"
" lgd_sbk_size i4 not null not default is 'exp.dmf.lg.lgd_sbk_size',"
" lgd_lbk_count i4 not null not default is 'exp.dmf.lg.lgd_lbk_count',"
" lgd_lbk_size i4 not null not default is 'exp.dmf.lg.lgd_lbk_size',"
" lgd_forced_lga char(30) not null not default"
" is 'exp.dmf.lg.lgd_forced_lga',"
" lgd_forced_high i4 not null not default"
" is 'exp.dmf.lg.lgd_forced_lga.lga_high',"
" lgd_forced_low i4 not null not default"
" is 'exp.dmf.lg.lgd_forced_lga.lga_low',"
" lgd_j_first_cp_high i4 not null not default"
" is 'exp.dmf.lg.lgd_j_first_cp.lga_high',"
" lgd_j_first_cp_lga char(30) not null not default"
" is 'exp.dmf.lg.lgd_j_first_cp',"
" lgd_j_first_cp_low i4 not null not default"
" is 'exp.dmf.lg.lgd_j_first_cp.lga_low',"
" lgd_j_last_cp_lga char(30) not null not default"
" is 'exp.dmf.lg.lgd_j_last_cp',"
" lgd_j_last_cp_high i4 not null not default"
" is 'exp.dmf.lg.lgd_j_last_cp.lga_high',"
" lgd_j_last_cp_low i4 not null not default"
" is 'exp.dmf.lg.lgd_j_last_cp.lga_low',"
" lgd_d_first_cp_lga char(30) not null not default"
" is 'exp.dmf.lg.lgd_d_first_cp',"
" lgd_d_first_cp_high i4 not null not default"
" is 'exp.dmf.lg.lgd_d_first_cp.lga_high',"
" lgd_d_first_cp_low i4 not null not default"
" is 'exp.dmf.lg.lgd_d_first_cp.lga_low',"
" lgd_d_last_cp_lga char(30) not null not default"
" is 'exp.dmf.lg.lgd_d_last_cp',"
" lgd_d_last_cp_high i4 not null not default"
" is 'exp.dmf.lg.lgd_d_last_cp.lga_high',"
" lgd_d_last_cp_low i4 not null not default"
" is 'exp.dmf.lg.lgd_d_last_cp.lga_low',"
" lgd_stat_add i4 not null not default"
" is 'exp.dmf.lg.lgd_stat.add',"
" lgd_stat_remove i4 not null not default"
" is 'exp.dmf.lg.lgd_stat.remove',"
" lgd_stat_begin i4 not null not default"
" is 'exp.dmf.lg.lgd_stat.begin',"
" lgd_stat_end i4 not null not default"
" is 'exp.dmf.lg.lgd_stat.end',"
" lgd_stat_write i4 not null not default"
" is 'exp.dmf.lg.lgd_stat.write',"
" lgd_stat_split i4 not null not default"
" is 'exp.dmf.lg.lgd_stat.split',"
" lgd_stat_force i4 not null not default"
" is 'exp.dmf.lg.lgd_stat.force',"
" lgd_stat_readio i4 not null not default"
" is 'exp.dmf.lg.lgd_stat.readio',"
" lgd_stat_writeio i4 not null not default"
" is 'exp.dmf.lg.lgd_stat.writeio',"
" lgd_stat_wait i4 not null not default"
" is 'exp.dmf.lg.lgd_stat.wait',"
" lgd_stat_group_force i4 not null not default"
" is 'exp.dmf.lg.lgd_stat.group_force',"
" lgd_stat_group_count i4 not null not default"
" is 'exp.dmf.lg.lgd_stat.group_count',"
" lgd_stat_inconsist_db i4 not null not default"
" is 'exp.dmf.lg.lgd_stat.inconsist_db',"
" lgd_stat_pgyback_check i4 not null not default"
" is 'exp.dmf.lg.lgd_stat.pgyback_check',"
" lgd_stat_pgyback_write i4 not null not default"
" is 'exp.dmf.lg.lgd_stat.pgyback_write',"
" lgd_stat_kbytes i4 not null not default"
" is 'exp.dmf.lg.lgd_stat.kbytes',"
" lgd_stat_free_wait i4 not null not default"
" is 'exp.dmf.lg.lgd_stat.free_wait',"
" lgd_stat_stall_wait i4 not null not default"
" is 'exp.dmf.lg.lgd_stat.stall_wait',"
" lgd_stat_bcp_stall_wait i4 not null not default"
" is 'exp.dmf.lg.lgd_stat.bcp_stall_wait',"
" lgd_stat_log_readio i4 not null not default"
" is 'exp.dmf.lg.lgd_stat.log_readio',"
" lgd_stat_dual_readio i4 not null not default"
" is 'exp.dmf.lg.lgd_stat.dual_readio',"
" lgd_stat_log_writeio i4 not null not default"
" is 'exp.dmf.lg.lgd_stat.log_writeio',"
" lgd_stat_dual_writeio i4 not null not default"
" is 'exp.dmf.lg.lgd_stat.dual_writeio',"
" lgd_cpstall i4 not null not default"
" is 'exp.dmf.lg.lgd_cpstall',"
" lgd_stall_limit i4 not null not default"
" is 'exp.dmf.lg.lgd_stall_limit',"
" lgd_check_stall i4 not null not default"
" is 'exp.dmf.lg.lgd_check_stall',"
" lgd_dmu_cnt i4 not null not default"
" is 'exp.dmf.lg.lgd_dmu_cnt',"
" lgd_active_log i4 not null not default"
" is 'exp.dmf.lg.lgd_active_log')"
" as import from 'tables'"
" with dbms = IMA,"
" structure = sortkeyed,"
" key = (place)",
NULL, /* modify */
NULL, /* index 1 */
NULL, /* index 2 */
},

{
0,
"lgmo_lpb",
0, /* number of columns */
"REGISTER TABLE lgmo_lpb ("
" place varchar(4)  not null not default is 'VNODE',"
" lpb_id_instance i4 not null not default"
" is 'exp.dmf.lg.lpb_id.id_instance',"
" lpb_id_id i4 not null not default"
" is 'exp.dmf.lg.lpb_id.id_id',"
" lpb_status char(100) not null not default"
" is 'exp.dmf.lg.lpb_status',"
" lpb_status_num i4 not null not default"
" is 'exp.dmf.lg.lpb_status_num',"
" lpb_pid i4 not null not default"
" is 'exp.dmf.lg.lpb_pid',"
" lpb_cond i4 not null not default"
" is 'exp.dmf.lg.lpb_cond',"
" lpb_bufmgr_id i4 not null not default"
" is 'exp.dmf.lg.lpb_bufmgr_id',"
" lpb_force_abort_sid i4 not null not default"
" is 'exp.dmf.lg.lpb_force_abort_sid',"
" lpb_gcmt_sid i4 not null not default"
" is 'exp.dmf.lg.lpb_gcmt_sid',"
" lpb_stat_readio i4 not null not default"
" is 'exp.dmf.lg.lpb_stat.readio',"
" lpb_stat_write i4 not null not default"
" is 'exp.dmf.lg.lpb_stat.write',"
" lpb_stat_force i4 not null not default"
" is 'exp.dmf.lg.lpb_stat.force',"
" lpb_stat_wait i4 not null not default"
" is 'exp.dmf.lg.lpb_stat.wait',"
" lpb_stat_begin i4 not null not default"
" is 'exp.dmf.lg.lpb_stat.begin',"
" lpb_stat_end i4 not null not default"
" is 'exp.dmf.lg.lpb_stat.end')"
" as import from 'tables'"
" with dbms = IMA,"
" structure = unique sortkeyed,"
" key = (place, lpb_id_id)",
NULL, /* modify */
NULL, /* index 1 */
NULL, /* index 2 */
},

{
0,
"lgmo_ldb",
0, /* number of columns */
"REGISTER TABLE lgmo_ldb ("
" place varchar(4)  not null not default is 'VNODE',"
" ldb_id_instance i4 not null not default"
" is 'exp.dmf.lg.ldb_id.id_instance',"
" ldb_id_id i4 not null not default"
" is 'exp.dmf.lg.ldb_id.id_id',"
" ldb_status char(100) not null not default"
" is 'exp.dmf.lg.ldb_status',"
" ldb_status_num i4 not null not default"
" is 'exp.dmf.lg.ldb_status_num',"
" ldb_l_buffer i4 not null not default"
" is 'exp.dmf.lg.ldb_l_buffer',"
" ldb_buffer char(200) not null not default"
" is 'exp.dmf.lg.ldb_buffer',"
" ldb_db_name char (##DB_DB_MAXNAME##) not null not default"
" is 'exp.dmf.lg.ldb_db_name',"
" ldb_db_owner char (##DB_OWN_MAXNAME##) not null not default"
" is 'exp.dmf.lg.ldb_db_owner',"
" ldb_j_first_lga char (30) not null not default"
" is 'exp.dmf.lg.ldb_j_first_cp',"
" ldb_j_first_high i4 not null not default"
" is 'exp.dmf.lg.ldb_j_first_cp.lga_high',"
" ldb_j_first_low i4 not null not default"
" is 'exp.dmf.lg.ldb_j_first_cp.lga_low',"
" ldb_j_last_lga char (30) not null not default"
" is 'exp.dmf.lg.ldb_j_last_cp',"
" ldb_j_last_high i4 not null not default"
" is 'exp.dmf.lg.ldb_j_last_cp.lga_high',"
" ldb_j_last_low i4 not null not default"
" is 'exp.dmf.lg.ldb_j_last_cp.lga_low',"
" ldb_d_first_lga char (30) not null not default"
" is 'exp.dmf.lg.ldb_d_first_cp',"
" ldb_d_first_high i4 not null not default"
" is 'exp.dmf.lg.ldb_d_first_cp.lga_high',"
" ldb_d_first_low i4 not null not default"
" is 'exp.dmf.lg.ldb_d_first_cp.lga_low',"
" ldb_d_last_lga char (30) not null not default"
" is 'exp.dmf.lg.ldb_d_last_cp',"
" ldb_d_last_high i4 not null not default"
" is 'exp.dmf.lg.ldb_d_last_cp.lga_high',"
" ldb_d_last_low i4 not null not default"
" is 'exp.dmf.lg.ldb_d_last_cp.lga_low',"
" ldb_sbackup_lga char (30) not null not default"
" is 'exp.dmf.lg.ldb_sbackup',"
" ldb_sbackup_high i4 not null not default"
" is 'exp.dmf.lg.ldb_sbackup.lga_high',"
" ldb_sbackup_low i4 not null not default"
" is 'exp.dmf.lg.ldb_sbackup.lga_low',"
" ldb_stat_read i4 not null not default"
" is 'exp.dmf.lg.ldb_stat.read',"
" ldb_stat_write i4 not null not default"
" is 'exp.dmf.lg.ldb_stat.write',"
" ldb_stat_begin i4 not null not default"
" is 'exp.dmf.lg.ldb_stat.begin',"
" ldb_stat_end i4 not null not default"
" is 'exp.dmf.lg.ldb_stat.end',"
" ldb_stat_force i4 not null not default"
" is 'exp.dmf.lg.ldb_stat.force',"
" ldb_stat_wait i4 not null not default"
" is 'exp.dmf.lg.ldb_stat.wait')"
" as import from 'tables'"
" with dbms = IMA,"
" structure = unique sortkeyed,"
" key = (place, ldb_id_id)",
NULL, /* modify */
NULL, /* index 1 */
NULL, /* index 2 */
},

{
0,
"lgmo_lxb",
0, /* number of columns */
"REGISTER TABLE lgmo_lxb ("
" place varchar(4)  not null not default is 'VNODE',"
" lxb_id_instance i4 not null not default"
" is 'exp.dmf.lg.lxb_id.id_instance',"
" lxb_id_id i4 not null not default"
" is 'exp.dmf.lg.lxb_id.id_id',"
" lxb_status char(100) not null not default"
" is 'exp.dmf.lg.lxb_status',"
" lxb_status_num i4 not null not default"
" is 'exp.dmf.lg.lxb_status_num',"
" lxb_db_name char (##DB_DB_MAXNAME##) not null not default"
" is 'exp.dmf.lg.lxb_db_name',"
" lxb_db_owner char (##DB_OWN_MAXNAME##) not null not default"
" is 'exp.dmf.lg.lxb_db_owner',"
" lxb_db_id_id i4 not null not default"
" is 'exp.dmf.lg.lxb_db_id_id',"
" lxb_pr_id_id i4 not null not default"
" is 'exp.dmf.lg.lxb_pr_id_id',"
" lxb_wait_reason i4 not null not default"
" is 'exp.dmf.lg.lxb_wait_reason',"
" lxb_sequence i4 not null not default"
" is 'exp.dmf.lg.lxb_sequence',"
" lxb_first_lga char (30) not null not default"
" is 'exp.dmf.lg.lxb_first_lga',"
" lxb_first_high i4 not null not default"
" is 'exp.dmf.lg.lxb_first_lga.lga_high',"
" lxb_first_low i4 not null not default"
" is 'exp.dmf.lg.lxb_first_lga.lga_low',"
" lxb_last_lga char (30) not null not default"
" is 'exp.dmf.lg.lxb_last_lga',"
" lxb_last_high i4 not null not default"
" is 'exp.dmf.lg.lxb_last_lga.lga_high',"
" lxb_last_low i4 not null not default"
" is 'exp.dmf.lg.lxb_last_lga.lga_low',"
" lxb_cp_lga char (30) not null not default"
" is 'exp.dmf.lg.lxb_cp_lga',"
" lxb_cp_high i4 not null not default"
" is 'exp.dmf.lg.lxb_cp_lga.lga_high',"
" lxb_cp_low i4 not null not default"
" is 'exp.dmf.lg.lxb_cp_lga.lga_low',"
" lxb_tran_id char(16) not null not default"
" is 'exp.dmf.lg.lxb_tran_id',"
" lxb_tran_high i4 not null not default"
" is 'exp.dmf.lg.lxb_tran_id.db_high_tran',"
" lxb_tran_low i4 not null not default"
" is 'exp.dmf.lg.lxb_tran_id.db_low_tran',"
" lxb_dis_tran_id_hexdump char(350) not null not default"
" is 'exp.dmf.lg.lxb_dis_tran_id_hexdump',"
" lxb_pid i4 not null not default"
" is 'exp.dmf.lg.lxb_pid',"
" lxb_sid i4 not null not default"
" is 'exp.dmf.lg.lxb_sid',"
" lxb_dmu_cnt i4 not null not default"
" is 'exp.dmf.lg.lxb_dmu_cnt',"
" lxb_user_name char(100) not null not default"
" is 'exp.dmf.lg.lxb_user_name',"
" lxb_is_prepared char (1) not null not default"
" is 'exp.dmf.lg.lxb_is_prepared',"
" lxb_is_xa_dis_tran_id char (1) not null not default"
" is 'exp.dmf.lg.lxb_is_xa_dis_tran_id',"
" lxb_stat_split i4 not null not default"
" is 'exp.dmf.lg.lxb_stat.split',"
" lxb_stat_write i4 not null not default"
" is 'exp.dmf.lg.lxb_stat.write',"
" lxb_stat_force i4 not null not default"
" is 'exp.dmf.lg.lxb_stat.force',"
" lxb_stat_wait i4 not null not default"
" is 'exp.dmf.lg.lxb_stat.wait')"
" as import from 'tables'"
" with dbms = IMA,"
" structure = unique sortkeyed,"
" key = (place, lxb_id_id)",
NULL, /* modify */
NULL, /* index 1 */
NULL, /* index 2 */
},

{
0,
"lgmo_xa_reduced_reg",
0, /* number of columns */
"REGISTER TABLE lgmo_xa_reduced_reg ("
" place varchar(4)  not null not default is 'VNODE',"
" lxb_id_id i4 not null not default"
" is 'exp.dmf.lg.lxb_id.id_id',"
" lxb_dis_tran_id_hexdump char(350) not null not default"
" is 'exp.dmf.lg.lxb_dis_tran_id_hexdump',"
" lxb_db_name char (##DB_DB_MAXNAME##) not null not default"
" is 'exp.dmf.lg.lxb_db_name',"
" lxb_is_prepared char (1) not null not default"
" is 'exp.dmf.lg.lxb_is_prepared',"
" lxb_is_xa_dis_tran_id char (1) not null not default"
" is 'exp.dmf.lg.lxb_is_xa_dis_tran_id')"
" as import from 'tables'"
" with dbms = IMA,"
" structure = unique sortkeyed,"
" key = (place, lxb_id_id)",
NULL, /* modify */
NULL, /* index 1 */
NULL, /* index 2 */
},

/* iiaudit see back!gwf!gwfsxa */
{
0,
"iiaudit",
0, /* number of columns */
"REGISTER TABLE iiaudit  ("
" audittime       ingresdate not null,"
" user_name char(##DB_OWN_MAXNAME##) not null,"
" real_name char(##DB_OWN_MAXNAME##) not null,"
" userprivileges char(32) not null,"
" objprivileges char(32) not null,"
" database char(##DB_DB_MAXNAME##) not null,"
" auditstatus char(1)  not null,"
" auditevent char(24) not null,"
" objecttype char(24) not null,"
" objectname char(##DB_MAXNAME##) not null,"
" objectowner char(##DB_OWN_MAXNAME##) not null,"
" description char(80) not null,"
" sessionid char(16) not null,"
" detailinfo char(256) not null,"
" detailnum integer not null,"
" querytext_sequence integer not null)"
" AS IMPORT FROM 'current'"
" WITH DBMS='sxa', NOUPDATE, STRUCTURE=NONE",
NULL, /* modify */
NULL, /* index 1 */
NULL, /* index 2 */
},

{
0,
NULL,
0,
NULL,
NULL,
NULL,
NULL
}

};

GLOBALDEF DUC_PROCDEF Duc_procdef[] = 
{

{
"iiqef_create_db",
"CREATE PROCEDURE iiqef_create_db("
"   dbname         = char(##DB_DB_MAXNAME##) not null not default,"
"   db_loc_name    = char(##DB_LOC_MAXNAME##) not null not default,"
"   jnl_loc_name   = char(##DB_LOC_MAXNAME##) not null not default,"
"   ckp_loc_name   = char(##DB_LOC_MAXNAME##) not null not default,"
"   dmp_loc_name   = char(##DB_LOC_MAXNAME##) not null not default,"
"   srt_loc_name   = char(##DB_LOC_MAXNAME##) not null not default,"
"   db_access      = integer  not null not default,"
"   collation      = char(##DB_COLLATION_MAXNAME##) not null not default,"
"   need_dbdir_flg = integer  not null not default,"
"   db_service     = integer  not null not default,"
"   translated_owner_name   = char(##DB_OWN_MAXNAME##) not null not default,"
"   untranslated_owner_name = char(##DB_OWN_MAXNAME##) not null not default,"
"   ucollation     = char(##DB_COLLATION_MAXNAME##) not null not default)"
" AS BEGIN EXECUTE INTERNAL; END"
},

{
"iiqef_alter_db",
"CREATE PROCEDURE iiqef_alter_db("
"   dbname = char(##DB_DB_MAXNAME##) not null not default,"
"   access_on = integer not null not default,"
"   access_off = integer not null not default,"
"   service_on = integer not null not default,"
"   service_off = integer not null not default,"
"   last_table_id = integer not null not default)"
" AS BEGIN EXECUTE INTERNAL; END"
},

{
"iiqef_destroy_db",
"CREATE PROCEDURE iiqef_destroy_db("
" dbname = char(##DB_DB_MAXNAME##) not null not default)"
" AS BEGIN EXECUTE INTERNAL; END"
},

{
"iiqef_add_location",
"CREATE PROCEDURE iiqef_add_location("
"   database_name = char(##DB_DB_MAXNAME##) not null not default,"
"   location_name = char(##DB_LOC_MAXNAME##) not null not default,"
"   access = integer not null not default,"
"   need_dbdir_flg = integer not null not default)"
" AS BEGIN EXECUTE INTERNAL; END"
},

{
"iiqef_alter_extension",
"CREATE PROCEDURE iiqef_alter_extension("
"   database_name = char(##DB_DB_MAXNAME##) not null not default,"
"   location_name = char(##DB_LOC_MAXNAME##) not null not default,"
"   drop_loc_type = integer not null not default,"
"   add_loc_type = integer not null not default)"
" AS BEGIN EXECUTE INTERNAL; END"
},

{
"iierror",
"CREATE PROCEDURE iierror("
"   errorno integer not null with default,"
"    detail  integer not null with default,"
"    p_count integer not null with default,"
"    p1      varchar(256) not null with default,"
"    p2      varchar(256) not null with default,"
"    p3      varchar(256) not null with default,"
"    p4      varchar(256) not null with default,"
"    p5      varchar(256) not null with default)"
" AS BEGIN EXECUTE INTERNAL; END"
},

{
"iiqef_del_location",
"CREATE PROCEDURE iiqef_del_location("
"   database_name = char(##DB_DB_MAXNAME##) not null not default,"
"   location_name = char(##DB_LOC_MAXNAME##) not null not default,"
"   access = integer not null not default,"
"   need_dbdir_flg = integer not null not default)"
" AS BEGIN EXECUTE INTERNAL; END"
},

{
NULL,
NULL
}

};

GLOBALDEF DUC_CATEQV Duc_equivcats[] = 
{
	{"iirel_idx","iirelation"},
	{NULL, NULL}
};
