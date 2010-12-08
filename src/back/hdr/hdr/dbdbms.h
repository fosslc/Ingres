/*
** Copyright (c) 1985, 2010 Ingres Corporation
**
**
*/
#ifndef    DBDBMS_INCLUDE
#define                 DBDBMS_INCLUDE  0
/**
** Name: DBDBMS.H - Types and names used by the entire DBMS, not visible to FE
**
** Description:
**	Types and names used by the entire DBMS, including information about
**	configurations, attributes, databases, errors, tables, and transactions.
**	This header depends on compat.h.
**
** History:
**      23-jul-85 (jeff)
**          written
**	09-Dec-85 (fred)
**	    Added DB_ERROR structure.
**	31-dec-85 (jeff)
**	    Added DB_MAXNAME, made several definitions use this constant
**	    instead of a hard-wired 12.
**	02-jan-86 (jeff)
**	    Added session configuration definitions.
**	18-Jan-86 (fred)
**	    Added DB_<fac_name>_ID constants for consistent definitions
**	    of facilities.  
**	03-feb-86 (jeff)
**	    Added pattern-matching constants.
**	04-feb-86 (jeff)
**	    Added DB_MAXSTRING.
**	18-feb-86 (jeff)
**	    Increased DB_ERR_SIZE from 100 to 256.
**	03-mar-86 (thurston)
**	    Added the following definitions:
**		DB_DT_ID	- Datatype id.
**		DB_DATA_VALUE	- Representation of a data value.
**		DB_DATE_FMT	- International date format.
**		DB_MONY_FMT	- International money format.
**      19-mar-86 (seputis)
**          Added DB_SORT_STORE storage structure used in optimizer model
**	25-mar-86 (fred)
**	    Enhanced 18-Jan-86 change by adding min & max facilities for
**		error processing
**	30-mar-86 (thurston)
**	    Corrected the #define for DB_MNY_TYPE.  This was defined as 4
**	    but should have been 5 ... we are not changing datatype ids for
**	    Jupiter!
**	01-apr-86 (thurston)
**	    Changed the definition of a DB_DATA_VALUE so that the .db_data
**	    member is the generic PTR instead of a DB_ANYTYPE.
**      02-apr-86 (thurston)
**	    In the DB_MONY_FMT typedef, I have changed the names of two
**	    symbolic constants because they were not unique to 8 characters:
**	    DB_MONY_LEAD becomes DB_LEAD_MONY, and
**	    DB_MONY_TRAIL becomes DB_TRAIL_MONY.
**	15-apr-86 (jeff)
**	    Added DB_QRY_ID.
**      16-apr-86 (thurston)
**	    In the DB_MONY_FMT typedef, I have added the DB_MNYRSVD_SIZE
**	    constant, and the .db_mny_rsvd member in order to keep the
**	    structure aligned properly.
**      06-may-86 (seputis)
**          Added TID definitions
**	16-may-86 (jeff)
**	    Added qrymod definitions: DB_PROTECTION, DB_INTEGRITY, DB_IIQRYTEXT,
**	    DB_IIVIEWS
**	16-may-86 (ericj)
**	    Changed db_mny_sign field name in DB_MONY_STRUCT to db_mny_sym
**	    to avoid confusion between money sign and money symbol.
**	18-may-86 (jeff)
**	    Added DB_INTRO_CHAR, DB_CURSOR_ID
**	20-may-86 (jeff)
**	    Added db_cur_name field to DB_CURSOR_ID.
**      25-jun-86 (seputis)
**          Added DB_1ZOPTSTAT DB_2ZOPTSTAT
**	08-jul-86 (jeff)
**	    Added DB_RETRIEVE, DB_RETP, DB_APPEND, DB_APPP, DB_REPLACE, DB_REPP,
**	    DB_DELETE, DB_DELP for storage in qrymod catalogs
**	11-jul-86 (jeff)
**	    Changed DB_SORT_STORE from 53 to 1 to make switches more efficient
**      15-jul-86 (seputis)
**          changed zopt1stat, zopt2stat relations
**	15-jul-86 (jeff)
**	    Added DB_IITREE structure.  Fixed segment in DB_IIQRYTEXT.
**	16-jul-86 (ac)
**	    Added DB_TREE_ID structure.
**	16-jul-86 (jeff)
**	    Changed DB_TREE_ID structure to a timestamp (sorry, Annie).
**      17-jul-86 (seputis)
**          Changed DB_ATT_ID to be an i2 from a u_i2
**	21-jul-86 (jeff)
**	    Added tree id to DB_PROTECTION, query id to DB_IIVIEWS
**      21-Jul-1986 (fred)
**          Added E_DB_SEVERE and E_DB_USERERROR
**      21-jul-1986 (fred) (later)
**          Deleted E_DB_USERERROR due to scheduling problems.
**      06-may-86 (seputis)
**          Added DB_TID definition
**	08-aug-86 (jeff)
**	    Fixed definition of DB_INTEGRITY
**      26-aug-86 (jennifer)
**	    Name space problem with DB_INTEGRITY and DB_INTEG.
**          Changed DB_INTEG TO DB_INTG.
**      02-sep-86 (seputis)
**          changed DB_TAB_OWN to follow standard coding
**          added DB_TBL_NM and DB_RES_COL
**      12-sep-86 (thurston)
**          Added the following datatype constants:
**		DB_CHA_TYPE	SQL's "char" datatype
**		DB_VCH_TYPE	SQL's "varchar" datatype
**		DB_DYC_TYPE	Dynamic char string for frontends
**		DB_LTXT_TYPE	"longtext" for frontends
**	29-sep-86 (thurston)
**	    Extended the comments documenting the use of DB_TEXT_STRING to
**	    include the VARCHAR and LONGTEXT datatypes.
**	15-oct-86 (thurston)
**	    Added the datatype constant DB_QRY_TYPE.
**      16-nov-86 (jennifer)
**          Change DB_MAXNAME to 24 instead of 12.
**	01-dec-86 (daved)
**	    remove typedef of DB_RPT_NAME since query names are identified
**	    by cursor_id.
**      01-dec-86 (seputis)
**          added some constants used to define the histogram relation
**	02-dec-86 (thurston)
**	    In the DB_TEXT_STRING typedef, I changed the .db_t_text member to
**	    be u_char instead of char, and the .db_t_count member to be u_i2
**	    instead of i2.
**	02-dec-86 (thurston)
**	    Made several cosmetic changes (moved typedefs, constant definitions,
**	    etc).  Attempted to make a typedef be defined before it gets used.
**	02-dec-86 (thurston)
**	    Added the datatype constant DB_DMY_TYPE.  This is used to let "copy"
**	    know to skip an input field.
**	02-dec-86 (thurston)
**	    Changed the datatype constant DB_QRY_TYPE to DB_QUE_TYPE since it
**	    was not unique in seven characters (conflicted with DB_QRY_ID).
**      02-dec-86 (thurston)
**          Added the DB_EMBEDDED_DATA typedef, along with the DB_DBV_TYPE and
**          DB_NUL_TYPE constants.
**      31-dec-86 (thurston)
**          Added a comment that a DB_EMBEDDED_DATA can also be of type
**          DB_VCH_TYPE (varchar).  This allows EQL programs to deal with
**          strings that contain NULLs in them; such could arise from a
**          retrieve of a CHAR or VARCHAR column.
**      19-jan-87 (thurston)
**          Added the DB_DTNUL_BIT, DB_DTNDEF_BIT and DB_DTBASE_MASK constants.
**          Also, changed the definition of the DB_NUL_TYPE datatype ID constant
**          so that the nullability bit is set, and added the DB_NULBASE_TYPE to
**          help in implementation details.
**      10-feb-87 (thurston)
**          Got rid of the DB_DTNUL_BIT, DB_DTNDEF_BIT, and DB_DTBASE_MASK
**          constants, since the design review for NULLs dictated this.  Made
**          DB_NUL_TYPE be the negation of DB_NULBASE_TYPE for same reason.
**          Changed the value of DB_NODT to be `0' instead of `-1', since now
**          `-1' is a legal nullable datatype ID.
**	10-feb-87 (thurston)
**	    Fixed up comments in the DB_TEXT_STRING typedef description to use
**          the term `bytes' instead of `characters' where appropriate (Kanji).
**	10-feb-87 (thurston)
**          Added the DB_CHAR_MAX constant.
**	10-feb-87 (thurston)
**	    Added the DB_EMB_NULL constant.
**      16-feb-87 (fred)
**          Altered values of DB_ERROR values so that they could be more
**          efficiently tested for.
**      25-feb-87 (fred)
**          Added application codes for temporary usage.
**	12-mar-87 (thurston)
**	    Added DB_BOO_TYPE and changed the definition of DB_NULLBASE_TYPE.
**      30-mar-87 (ericj)
**          Updated size of DB_TERM_NAME to correspond to size of proterm
**	    field in iiprotect catalog.
**      15-apr-87 (ericj)
**          Added DUF facility code.
**      04-may-87 (stec)
**          Added DB_TBL_ID define (for GRANT, CREATE PERMIT w/o qualification).
**      26-jun-87 (daved)
**          Modify qrymod catalog structures for standard catalog
**	    interface. Added DB_IIDBDEPENDS to describe dependencies
**	    between objects.
**      19-jan-88 (thurston)
**          Bumped the DB_CHAR_MAX constant up from 255 to 2000, thus allowing
**	    `c' and `char' values to have up to 2000 characters.
**      22-apr-88 (seputis)
**          defined structures to support iiprocedure catalog
**      25-apr-88 (thurston)
**          Added the DB_DEC_TYPE constant to represent the datatype ID for the
**	    new DECIMAL datatype.
**      19-may-88 (thurston)
**          Added the German date format constant, DB_GERM_DFMT.
**      19-may-88 (thurston)
**          Got rid of the DB_NULBASE_TYPE constant, and re-defined DB_NUL_TYPE
**          to be (-(DB_LTXT_TYPE)), since that datatype is guaranteed to be
**          coerceable into any other datatype.
**	25-may-88 (neil)
**	    Defined DB_TFLD_TYPE as a new front-end type.
**      25-may-88 (thurston)
**          Backed out the changes regarding the DB_NULBASE_TYPE and DB_NUL_TYPE
**	    (see 19-may-88 (thurston), above for details).  This was done
**          because it turns out the FE group has already used the DB_NUL_TYPE
**          constant in EQUEL/ESQL, so if we change it, all EQUEL/ESQL programs
**          would have to be re-preprocessed.
**      24-jun-88 (stec)
**	    Added PST_PARM_NAME typedef.
**	27-july-88 (andre)
**	    Added procedure id (db_procid) to DB_PROCEDURE.
**      28-july-88 (andre)
**	    Added DB_EXECUTE, DB_EXECP for storage in qrymod catalogs
**	15-aug-88 (stec)
**	    Update the definition of DB_PROCEDURE (iiprocedure tuple).
**	01-sep-88 (thurston)
**	    Added several new constants for Gateways, which represent certain
**	    maximums there.  The new constants are:
**		DB_GW1_MAXNAME
**		DB_GW2_MAX_COLS
**		DB_GW3_MAXTUP
**		DB_GW4_MAXSTRING
**	20-oct-88 (andre)
**	    Added iiqrytext values for CREATE LINK and REGISTER ... DB_CRT_LINK
**	    and DB_REGISTER, respectively.
**	06-feb-89 (ralph)
**	    Changed DB_MAX_COLS to 300;
**	    Added DB_COL_WORDS and DB_COL_BITS;
**	    Changed db[ip]_domset to use DB_COL_WORDS as extent;
**	    Added db[ip]_reserve.
**	10-Mar-89 (ac)
**	    Added 2PC support. Added DB_DIS_TRAN_ID.
**	14-mar-89 (ralph)
**	    GRANT Enhancements, Phase 1:
**	    Defined DB_SEQ as type for sequence fields;
**	    Added dbp_seq, dbp_gtype fields to DB_PROTECTION;
**	    Added dbi_seq field to DB_INTEGRITY;
**	    Created iiusergroup mapping structure, DB_USERGROUP;
**	    Created iiapplication_id mapping structure, DB_APPLICATION_ID.
**	21-mar-89 (sandyh)
**	    Added B1 security labeling.
**      10-Mar-89 (teg)
**          Defined new application code (DBA_PURGE_VFDB)
**	26-mar-89 (neil)
**	    Added DB_NAME, DB_DATE, DB_IIRULE and DB_RULE for rule processing.
**	31-mar-89 (ralph)
**	    GRANT Enhancements, Phase 2:
**	    Created iidbpriv mapping structure, DB_PRIVILEGES;
**	    Added grantee type flags, DBGR_{DEFAULT,USER,GROUP,APLID,PUBLIC}
**	17-apr-89 (mikem)
**	    Logical key development. Added definitions of two new datatypes.
**	24-apr-89 (ralph)
**	    GRANT Enhancements, Phase 2a:
**	    Create flags DBPR_VALUED, DBPR_BINARY, DBPR_ALL.
**	17-may-89 (mikem)
**	    Changed size of smaller logical key from 6 bytes to 8 bytes.
**      30-May-89 (teg)
**          Modifications for System Catalog structs for portability.
**	26-jun-89 (mikem)
**	    Changed defines for logical keys to be in the "correct" range.
**      10-jul-89 (cal)
**	    Add definition to get terminator IFDEFS compiled in frontend
**	    code.
**	07-sep-89 (neil)
**	    Alerters: Data structures:  DB_IIEVENT, DB_ALERT_NAME
**		      Constants:	DB_EVENT, DB_EVREGISTER & DB_EVRAISE.
**	19-sep-89 (jrb)
**	    Changed DB_LN_FRM_PR_MACRO to DB_PREC_TO_LEN_MACRO which is more
**	    readable.
**	08-oct-1989 (ralph)
**	    Added DB_AREANAME definition for B1.
**          Changed db_privileges.dbpr_fill1 to dbpr_dbflags.
**	    Added DBPR_ALLDBS, DBPR_UPDATE_SYSCAT.
**	    Added DB_ALMSUCCESS, DB_ALPSUCCESS, DB_ALMFAILURE, DB_ALPFAILURE
**	09-oct-89 (jrb)
**	    Increased DB_ERR_SIZE from 256 to 1000.
**      09-oct-89 (cal)
**	    Add defines for SUN KANJI release.
**	11-oct-89 (ralph)
**	    Fixed backslash problem at DBPR_BINARY
**	25-oct-89 (neil)
**	    Added B1 fields into DB_IIRULE and DB_IIEVENT.
**	28-oct-1989 (ralph)
**	    Changed DB_ALMFAILURE, DB_ALPFAILURE bit position
**      03-Nov-1989 (fred)
**	    Added DB_DIF_TYPE.  This type, internal to the system, is used to
**	    provide the ability to manage unsortable datatypes in the sorter.
**	    The sorter will assign this type to any unsortable datatypes,
**	    and will recognize and manage tuples which differ in this type.
**	    Also added DB_LVC_TYPE, the large object varchar type.
**	    (Note: DB_LVC_TYPE later changed to DB_LVCH_TYPE - fred)
**	22-nov-89 (jrb)
**	    Added DB_ALL_TYPE for outer join project.
**	20-dec-89 (andre)
**	    Added DB_CAN_ACCESS and DB_CAN_ACCESSP.
**	12-feb-90 (carol)
**	    Added the tuple definition for the iidbms_comment catalog,
**	    DB_IICOMMENT.
**	21-feb-90 (andre)
**	    Defined DB_1_IICOMMENT.  This structure will include DB_IICOMMENT +
**	    a flag field.
**	08-aug-90 (ralph)
**	    Added DBPR_DB_ADMIN, DBPR_GLOBAL_USAGE database privileges
**	    Added DBPR_RESTRICTED to identify restricted database privileges
**	    Added fields to DB_PROTECTION for FIPS, GRANT and REVOKE.
**	    Added DB_TIME_ID structure for permit time stamp.
**	    Changed all permit operation flags to long masks.
**	    Added DB_SCAN and DB_LOCKLIMIT permit operation flags.
**	11-oct-90 (ralph)
**	    6.3->6.5 merge:
**	13-mar-89 (linda)
**	    Increased DB_MAX_FACILITY to 15 for DB_GWF_ID
**	27-mar-90 (teresa)
**	    Defined DB_SYN_MAXNAME, DB_SYNNAME, DB_SYNOWN and DB_IISYNONYM for
**	    the synonyms project.
**	23-apr-90 (fourt)
**	    Change names DB_REGISTER and DB_CRT_LINK to DB_REG_IMPORT
**	    and DB_REG_LINK, to reflect changing of statements REGISTER
**	    and CREATE LINK to REGISTER...AS IMPORT and REGISTER...AS LINK.
**	23-may-90 (linda)
**	    Change constant names again.  STAR wants to recognize both
**	    "create ...as link" and "register...as link", so we've got to
**	    use a new name for "register...as import". So the names now
**	    are:
**		DB_CRT_LINK   ("create ... as link)
**				(orig. name, same value)
**		*DB_REG_LINK   ("register...as link")
**				(name change, same value)
**		DB_REG_IMPORT ("register...as import")
**				(new name, new value)
**		*NOTE THIS NAME CHANGE WHEN INTEGRATING STAR CODE.  It was
**		DB_REGISTER, but always stood for "register...as link".
**	15-jan-91 (ralph)
**	    Add DBPR_OPFENUM to indicate which dbprivs require OPF enumeration
**	28-jan-91 (andre)
**	    defined structure DB_SHR_RPTQRY_INFO to be used in determining if a
**	    given repeat query is shareable.
**	27-feb-91 (ralph)
**	    Add DB_MSGUSER and DB_MSGLOG constants to represent the
**	    DESTINATION clause of the MESSAGE and RAISE ERROR statements.
**	01-mar-91 (Mike S)
**	    Add DB_OBJ_TYPE, a non-ADF DB_DT_ID used for Windows4GL objects.
**	29-apr-91 (pete)
**	    Add DB_MAXTUP_DOC; the maximum documented tuple size (currently
**	    we document 2000 as the max, but 2008 is the real max).
**	09-jul-91 (andre)
**	    defined DB_GRANT_OPTION and DB_GROPTP
**	30-jan-1992 (bonobo)
**	    Removed the redundant typedefs and changed questionable 
**	    integration dates to quiesce the ANSI C compiler warnings.
**	30-jan-1992 (bonobo)
**	    Add closing comment delimiter to #define DB_ALARM statement
**      06-may-92 (stevet)
**          Added DB_YMD_DFMT, DB_MDY_DFMT and DB_DMY_DFMT for new
**          date formatting options.
**	29-may-92 (andre)
**	    defined DB_IIPRIV for GRANT/REVOKE project
**	31-may-92 (andre)
**	    added independent object id to IIDBDEPENDS
**	10-jun-92 (andre)
**	    defined DB_IIXPRIV for GRANT/REVOKE project
**	18-jun-92 (andre)
**	    defined DB_IIXPROCEDURE and DB_IIXEVENT
**	21-jul-92 (andre)
**	    defined DB_REFERENCES and DB_REFP
**	23-jul-92 (wolf) 
**	    Add closing comment delimiter to #define DB_ALARM statement
**	    again, it was dropped in the 20-jul Sybil integration
**	06-aug-92 (markg)
**	    Added new facility type for SXF.
**	04-sep-1992 (fred)
**	    Added new datatypes for peripheral datatype support.
**		long varchar - DB_LVCH_TYPE (changed from DB_LVC_TYPE)
**		bit	     - DB_BIT_TYPE
**		long bit     - DB_LBIT_TYPE
**	    and FE data handler type
**			     - DB_HDLR_TYPE
**	21-sep-1992 (fpang)
**	    Check if DB_MAXNAME is defined before defining it. Allows
**	    it to be defined on the command line for STAR.
**	21-sep-1992 (fred)
**	    Correctly terminated comment in above.
**
**	    Added DB_BIT_STRING typedef for use in handling the BIT types.
**      25-sep-1992 (nandak)
**          Modified the definition of DB_DIS_TRAN_ID.
**          Added DB_INGRES_DIS_TRAN_ID and DB_XA_DIS_TRAN_ID.
**      25-sep-1992 (stevet)
**         Added DB_MAX_F4PREC and DB_MAX_F8PREC.
**	4-oct-92 (ed)
**	    Added defines for names dependent on DB_MAXNAME
**	26-oct-92 (rogerk)
**	    Added Consistency Point thread, and buffer manager name and 
**	    owner constants.
**	27-oct-92 (rickh)
**	    27-jul-92 (rickh)
**	    FIPS CONSTRAINTS improvements.
**	    20-aug-92 (rblumer)
**		changed MAX default to 1500; added 2 new default types
**	27-oct-92 (rickh)
**	    Moved ADT_ATTS and ADT_CMP_LIST here and renamed them to be
**	    DB_ATTS and DB_CMP_LIST.
**	27-oct-92 (jhahn)
**	    Added DB_PROCEDURE_PARAMETER.
**	9-nov-92 (ed)
**	    DB_MAXNAME changed from 24 to 32, and related changes
**	19-nov-92 (rickh)
**	    Recast DB_SCHEMA_ID, DB_CONSTRAINT_ID, DB_RULE_ID, and DB_DEF_ID
**	    as DB_TAB_IDs since we allocate them that way and coerce them
**	    into IIDBDEPENDS tuples as such.
**	05-dec-92 (rblumer)
**	    Remove deftype and related define's from DB_IIDEFAULT structure.
**	14-dec-92 (rganski)
**	    Added DBA_OPTIMIZEDB application flag. Optimizedb will no longer
**	    use the DBA_VERIFYDB flag, which is now obsolete: the server will
**	    return an error if it sees it.
**	20-nov-92 (markg)
**	    Added DB_AUDITTHREAD_NAME define for the SXF audit thread.
**      14-dec-92 (anitap)
**          Added defines for DB_SCHEMA_NAME, DB_IISCHEMA and DB_IISCHEMAIDX.
**	18-jan-1993 (bryanp)
**	    Added maxname definitions for CP_TIMER thread.
**      18-jan-93 (mikem)
**         Fixed comment for DB_SEC_LABEL in DB_IISCHEMAIDX definition (it was
**         unterminated causing lint warnings).
**	18-jan-93 (rganski)
**	    Added constant DB_MAX_HIST_LENGTH.
**	    Added typedef DB_STAT_VERSION, which is type of field sversion in
**	    DB_1ZOPTSTAT.
**	    (Character Histogram Enhancements project).
**	8-mar-93 (ed)
**	    split from dbms.h, symbols only needed by DBMS and not FE
**	25-jan-93 (rblumer)
**	    changed DB_TEXT_STRING to a u_i2 and a char array in DB_IIDEFAULT
**	    and DB_IIDEFAULTIDX; added several canonical DEF_IDs (for UNKNOWN,
**	    logical keys, INITIAL_USER, and DBA).
**	22-feb-93 (rickh)
**	    Upped the length of the value string in IIDEFAULT by 1 byte to
**	    force 4 byte alignment.  Also accounted for the null indicator
**	    byte.
**	2-mar-93 (rickh)
**	    Added canonical schema id for the $ingres account.
**	9-mar-93 (rickh)
**	    Added canonical default ids for use by UPGRADEDB in cases where
**	    a UDT must be flagged and where the datatype is unrecognized.
**	17-mar-93 (rblumer)
**	    Added KNOWN_NOT_NULLABLE_TUPLE bit to DB_INTEGRITY,
**	    to be used in the tuple containing the bitmap of 
**	    known-not-nullable columns.
**	26-Mar-93 (jhahn)
**	    Removed extraneous field from DB_PROCEDURE_PARAMETER
**	19-may-1993 (robf)
**	    Added definitions for iilabelmap[idx] tables
**	15-jun-93 (rickh)
**	    Added tuple definition for IIXPROTECT.
**	2-Jul-1993 (daveb)
**	    DB_NOSESSION shouldn't be 0xffffffff, because it won't fit
**	    into a 16 bit int constant.  Use ~0 instead.
**	1-sep-93 (stephenb)
**	    Added dbc_tabname field to DB_1_IICOMMENT
**	13-sep-93 (robf)
**	    Add DBPR_QUERY_SYSCAT for QUERY_SYSCAT db privilege
**	    Add DBPR_TABLE_STAT	for TABLE_STATISTICS db privilege
**	    Add DB_COPY_FROM/INFO for  COPY_FROM/COPY_INTO table privs
**	    Add DB_TERMTHREAD_NAME
**	15-oct-93 (andre)
**	    Defined DB_IIXSYNONYM
**	07-oct-93 (swm)
**	    Bug #56447
**	    Removed define for DB_SESSID type as it is no longer used,
**	    instances of it having already been replaced with CS_SID on
**	    20-sep-93.
**	14-apr-94 (robf)
**          Added DB_SECAUD_WRITER_NAME for security  audit writer thread
**      24-Apr-1995 (cohmi01)
**          Add DB_BWRITALONG_NAME and DB_BREADAHEAD_NAME for
**          disk I/O master process write-along and read-ahead threads.
**	28-apr-1995 (shero03)
**	    Support RTree
**      06-mar-1996 (stial01)
**          Variable Page Size project: Added DB_MAX_CACHE
**      06-mar-1996 (nanpr01)
**          Variable Page Size project: Added DB_MIN_PAGESIZE
**      06-may-1996 (nanpr01)
**          New Page Format project: Added DB_COMPAT_PGSIZE, LINEID macro
**	    and DB_OFFSEQ
**	24-may-96 (stephenb)
**	    Add DB_REP_QMAN_NAME define for replicator queue management thread.
**      01-aug-1996 (nanpr01 for ICL keving)
**          Added DB_LK_CALLBACK_NAME for LK blocking callback thread.
**	15-Nov-1996 (jenjo02)
**	    Added DB_DEADLOCK_NAME for Deadlock Detector thread.
**	21-Mar-1997 (jenjo02)
**	    Table priority project:
**	    Added DB_MAX_TABLEPRI
**	3-feb-98 (inkdo01)
**	   Added DB_BSORTTHREAD_NAME.
**	06-may-1998 (canor01)
**	   Add License Checking thread.
**	10-nov-1998 (sarjo01)
**	   Added db priv DBPR_TIMEOUT_ABORT.
**	24-feb-98 (stephenb)
**	   Add new structure DB_BLOB_WKSP
**	01-mar-1999 (mcgem01)
**	    Added DBA_W4GLRUN_LIC_CHK and DBA_W4GLDEV_LIC_CHK and bumped
**	    DBA_MAX_APPLICATION to accommodate this change.
**	23-mar-1999 (thaju02)
**	    Defined DB_SESS_TEMP_OWNER. (B96047)
**      12-aug-1999 (stial01)
**          Changes to DB_BLOB_WKSP
**      31-aug-1999 (stial01)
**          Changes to DB_BLOB_WKSP
**      27-mar-2000 (stial01)
**          Delete unsed changes to DB_BLOB_WKSP added for VPS etab
**      12-oct-2000 (stial01)
**          Added DB_PAGE_OVERHEAD_MACRO
**      23-oct-2000 (stial01)
**          Variable Page Size - Variable Page Type support (SIR 102955)
**      01-feb-2001 (stial01)
**          Redefined DB_VPT_SIZEOF_TUPLE_HDR (Variable Page Type SIR 102955)
**          Also added DB_PG_V4
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	28-aug-2001 (hayke02)
**	    Added #define DB_TEMP_TAB_NAME for internal temporary table
**	    name of "$<Temporary Table>".
**      06-apr-2001 (stial01)
**          Added DB_MAX_PGTYPE
**	11-May-2001 (jenjo02)
**	    Deleted obsolete DB_RAWEXT_NAME.
**	11-may-2001 (devjo01)
**	    Add cluster thread names for s103715.
**	15-oct-2001 (abbjo03)
**	    Add DB_SRCPROP_NAME and DB_TGTPROP_NAME.
**	03-jan-2002 (abbjo03)
**	    Add DB_4_REPLIC_SVR for DB_DISTRIB. Remove obsolete #defines.
**	5-mar-02 (inkdo01)
**	    Add DB_IISEQUENCE to map iisequence tuples.
**	24-Jul-2002 (inifa01)
**	    Added CONS_BASEIX dbi_consflags bit to _DB_INTEGRITY for constraint
**	    where base table is index.
**	23-Oct-2002 (hanch04)
**	    Make pagesize variables of type long (L)
**	22-nov-2002 (mcgem01)
**	    Added an application identifier for Advantage OpenROAD 
**	    Transformation Runtime.
**	25-nov-02 (inkdo01)
**	    Introduce DB_MAXVARS to globally declare max number of range
**	    variables in a query.
**	19-Dec-2003 (schka24)
**	    Map the new partitioned table catalogs.
**	21-Jan-2004 (jenjo02)
**	    Added DB_TID8 structure and defines for
**	    Partitioned Table project.
**	29-Jan-2004 (schka24)
**	    Remove iipartition catalog, turns out not to be needed.
**	9-feb-04 (toumi01)
**	    Added name define for parallel query child thread.
**      14-apr-2004 (stial01)
**          Increased DB_MAXBTREE_KEY (SIR 108315)
**	15-Jul-2004 (schka24)
**	    Tid8 union needs tid-swap applied, just like tid4.
**	23-dec-2004 (srisu02)
**          Removed DB_MAXVARS from dbdbms.h and added in iicommon.h 
**          as it is being referenced in front!misc!hdr xf.qsh
**	29-Dec-2004 (schka24)
**	    Remove iixprotect index defs, not a useful index.
**	09-may-2005 (devjo01)
**	    Add DB_DIST_DEADLOCK_NAME.
**	21-jun-2005 (devjo01)
**	    Add DB_GLC_SUPPORT_NAME.
**	30-May-2006 (jenjo02)
**	    Added DB_MAXIXATTS, the maximum number of attributes in an index.
**	    DB_MAXKEYS defines the max number of key + non-key columns a
**	    user may name, but indexes on Clustered Tables may add
**	    up to DB_MAXKEYS internal Cluster Key attributes.
**      27-Nov-2003 (hanal04) Bug 108310 INGSTR44
**          Add comment that statistics version values are mirrored in
**          the common/hdr/hdr/dudbms.qsh Database compatibility values.
**      16-nov-2008 (stial01)
**          Redefined name constants without trailing blanks.
**      19-Nov-2008 (hweho01)
**          Add DB_STATVERS_6DBV930 for 9.3 release. 
**      06-jan-2009 (stial01)
**          Add DB_DBCAPABILITIES
**      21-apr-2009 (stial01)
**          Added DB_OLDMAXNAME_32
**      10-Jun-2009 (hweho01)
**          Add DB_STATVERS_6DBV1000 for 10.0 release. 
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Add DB_PG_V6, DB_PG_V7
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**	12-Jul-2010 (bonro01)
**          Add DB_STATVERS_6DBV1010 for 10.1 release. 
**	5-Oct-2010 (kschendel) SIR 124544
**	    Add Vectorwise storage structure codes.
**/

#define                 DB_OLDMAXNAME   24

#define			DB_NDINGRES_NAME    "ingres                          "
#define                 DB_INGRES_NAME	    "$ingres                         "
#define			DB_ABSENT_NAME	    "                                "
#define			DB_DBDB_NAME	    "iidbdb                          "
#define                 DB_DEFAULT_NAME	    "$default                        "

/* the following should be blank padded to DB_MAXNAME size, as needed*/
#define DB_UNKNOWN_DB       "UNKNOWN DB"
#define DB_UNKNOWN_OWNER    "UNKNOWN_OWNER"
#define DB_NAME_NOTAVAIL    "NOT_AVAILABLE"
#define DB_NOT_TABLE	    "NOT_A_TABLE"
#define	DB_TEMP_TABLE       "$<Temporary Table>"


/* The following should be blank padded before calling LG */
#define DB_OPENDB_USER	    "$opendb"
#define DB_ALLOCATE_USER    "$allocate"
#define DB_CACHEUTIL_INFO   "<Cache Utility>"
#define DB_CACHEUTIL_USER   "$cache_utility"
#define	DB_BUFMGR_INFO	    "<Buffer Manager>"
#define	DB_BUFMGR_USER      "$buffer_manager"
#define DB_RCPCONFIG_INFO   "$rcpconfig"
#define DB_ARCHIVER_INFO    "$archiver"
#define DB_RECOVERY_INFO    "$recovery"
#define DB_LOGDUMP_INFO	    "$log_dump"

/*
** Thread names and thread owners
** 
** Thread names do not need to be blank padded for CS
** CS passes the name (in gca_user_name) back to scd_alloc_scb() 
** scd_alloc_scb checks length of the thread name 
**
** Thread name/owner does have to be blank padded to NAME size if
** it is added to the logging/locking system in DMF
*/
#define	DB_CP_THREAD            "<Consistency Pt Thread>"
#define	DB_CP_THROWN            "$consistency_pt_thread"
#define DB_WRITEBEHIND_THREAD   "<Write Behind Thread>"
#define DB_WRITEBEHIND_THROWN   "$write_behind"
#define DB_RECOV_THREAD         " <Recovery Thread>"
#define DB_LOGWRITE_THREAD      " <Log Writer>"
#define DB_GROUPCOMMIT_THREAD   " <Group Commit Thread>"
#define DB_GROUPCOMMIT_THROWN   "$group_commit_thread"
#define DB_CPTIMER_THREAD       " <Consistency Point Timer>"
#define DB_CPTIMER_THROWN       "$consistency_point_timer"
#define DB_FORCEABORT_THREAD    " <Force Abort Thread>"
#define DB_DXRECOVERY_THREAD    " <DX recovery thread>"
#define DB_BWRITBEHIND_THREAD   " <Write Behind Thread>"
#define	DB_BCP_THREAD           " <Consistency Pt Thread>"
#define DB_SYSINIT_THREAD	" <SYSTEM INITIALIZATION>"
#define DB_EVENT_THREAD         " <Event Thread>"
#define DB_DEADPROC_THREAD      " <Dead Process Detector>"
#define DB_LOGFILEIO_THREAD     "<Logfile I/O Thread>"
#define DB_LOGFILEIO_THROWN     "$Logfile_I/O_Thread"
#define DB_AUDIT_THREAD         " <Security Audit Thread>"
#define DB_TERM_THREAD          " <Terminator Thread>"
#define DB_SECAUD_WRITER_THREAD " <Security Audit Writer Thread>"
#define DB_BWRITALONG_THREAD    " <Write Along Thread>"
#define DB_BREADAHEAD_THREAD    " <Read Ahead Thread>"
#define	DB_REP_QMAN_THREAD      " <Replicator Queue Management>"
#define DB_LK_CALLBACK_THREAD   " <LK Callback Thread>"
#define DB_DEADLOCK_THREAD      " <LK Deadlock Thread>"
#define DB_CSP_MAIN_THREAD      " <Cluster support>"
#define DB_CSP_MSGMON_THREAD    " <Cluster msg monitor>"
#define DB_CSP_MSG_THREAD       " <Cluster msg thread>"
#define	DB_EXCH_THREAD          " <Parallel Query Thread>"
#define	DB_DIST_DEADLOCK_THREAD " <CSP Deadlock Thread>"
#define	DB_GLC_SUPPORT_THREAD   " <GLC Support Thread>"

#define	DB_SYN_MAXNAME  DB_MAXNAME  /* for synonyms */
#define DB_MAX_CACHE     6  /* Max # buffer caches */
			    /* One for each page size */
#define DB_MIN_PAGESIZE  2048L  /* Min page size available */
#define DB_COMPAT_PGSIZE 2048L  /* compatibility page size */
#define DB_MAX_TABLEPRI  8  /* Max table priority */

#define	DB_SESS_TEMP_OWNER  "$Sess"

/*
**      Types and names associated with an installation.  For example, user ids
**      and location names.
*/


/*}
** Name: DB_NAME - Generic name structure.
**
** Description:
**      A typical blank-padded named object in the database.
**
** History:
**	06-mar-89 (neil)
**          Written for Terminator (rules).
*/
typedef struct _DB_NAME
{
    char	db_name[DB_MAXNAME];
} DB_NAME;

/*}
** Name: DB_SYNNAME - SYNONYM name structure.
**
** Description:
**      A structure to hold synonym names.
**	These are now the same as DB_NAME's, but weren't always.
**
** History:
**      26-mar-90 (teresa)
**          Created for SYNONYMS.
*/
typedef struct _DB_SYNNAME
{
    char        db_synonym[DB_SYN_MAXNAME];
} DB_SYNNAME;

/*}
** Name: DB_SYNOWN - SYNONYM Onwer Name structure.
**
** Description:
**      A structure to hold the name of a synonym owner.  Note: Right now
**	this is tied to DB_SYN_MAXNAME.  However, at some future time, it may
**	be appropriate to tie owner name to DB_MAXNAME instead.
**
** History:
**      26-mar-90 (teresa)
**          Created for SYNONYMS.
*/
typedef struct _DB_SYNOWN
{
    char        db_syn_own[DB_OWN_MAXNAME];
} DB_SYNOWN;

/*}
** Name: DB_OWN_NAME - Owner login name
**
** Description:
**      This structure defines an owner login name.
**
** History:
**     23-jul-85 (jeff)
**          written
*/
typedef struct _DB_OWN_NAME
{
    char            db_own_name[DB_OWN_MAXNAME];    /* Owner login name */
} DB_OWN_NAME;

/*}
** Name: DB_EVENT_NAME - Event name
**
** Description:
**      This structure defines an event name.
**
** History:
**     xx-mar-2010 (stial01)
**          written
*/
typedef struct _DB_EVENT_NAME
{
    char            db_ev_name[DB_EVENT_MAXNAME];    /* Event name */
} DB_EVENT_NAME;

/*}
** Name: DB_PASSWORD - Ingres Password 
**
** Description:
**      This structure defines an ingres password, created since DB_MAXNAME
**	no longer defines password length
**
** History:
**     5-oct-92 (ed)
**          written
*/
typedef struct _DB_PASSWORD
{
#define                 DB_PASSWORD_LENGTH 24
    char            db_password[DB_PASSWORD_LENGTH];    /* Ingres password */
} DB_PASSWORD;

/*}
** Name: DB_LOC_NAME - Location name.
**
** Description:
**      This structure defines a location name, which uniquely identifies a
**      location.
**
** History:
**     23-jul-85 (jeff)
**          written
*/
typedef struct _DB_LOC_NAME
{
    char            db_loc_name[DB_LOC_MAXNAME];    /* location id */
}  DB_LOC_NAME;

/*}
** Name: DB_TERM_NAME - Terminal name
**
** Description:
**      This structure defines a terminal name.  Note, the size of this
**	name must correspond to the size of the proterm field in the
**	iiprotect catalog.
**
** History:
**     01-oct-85 (jeff)
**          written
**     30-mar-87 (ericj)
**	    Updated size to correspond to proterm field in iiprotect catalog.
*/
typedef struct _DB_TERM_NAME
{
    char            db_term_name[16];    /* terminal name */
} DB_TERM_NAME;


/*}
** Name: DB_ATT_ID - Attribute id
**
** Description:
**      This structure defines an attribute id, which is used to uniquely
**      identify a database column within a table.  It is really a column
**	number.  Front ends know about column numbers too, so this must be
**	a datatype whose length is known so it can be converted to an equivalent
**	type on different hardware.
**
** History:
**     08-jul-85 (jeff)
**          written
**     17-jul-85 (seputis)
**          changed to i2 for OPF
**     30-jun-89 (teg)
**          changed struct so that it is portable to other platforms.  Having
**          a struct with only 1 i2 in it causes alignment issues when you
**          port to other machines.  This is modified to use DB_ATTNUM type, and
**          other structures that used DB_ATT_ID to contain DBMS catalogs are
**          modified to use DB_ATTNUM instead of DB_ATT_ID.
*/
typedef     i2      DB_ATTNUM;
typedef struct
{
        DB_ATTNUM                   db_att_id;          /* Attribute id */
} DB_ATT_ID;

/*
**      The types and names associated with databases.
*/

/*}
** Name: DB_DB_NAME - Database name
**
** Description:
**      This structure defines a database name.
**
** History:
**     08-jul-85 (jeff)
**          written
*/
typedef struct _DB_DB_NAME
{
    char            db_db_name[DB_DB_MAXNAME];      /* database name */
} DB_DB_NAME;

/*}
** Name: DB_DB_OWNER - Database owner.
**
** Description:
**      This structure defines a database owner name.
**
** History:
**     08-jul-85 (jeff)
**          written
*/
typedef struct _DB_DB_OWNER
{
    DB_OWN_NAME     db_db_owner;        /* database owner */
} DB_DB_OWNER;

/*}
** Name: DB_ALERT_NAME - Unique installation event/alert name.
**
** Description:
**	This structure defines the run-time definition of a registered or
**	raised event with the server.  The three names uniquely identify
**	the event within an installation.  This structure is shared by
**	PSF, OPF, QEF and SCF when processing the RAISE, REGISTER and REMOVE
**	statements, as well as when a server notifies applications registered
**	to receive events.
**	
**	This structure is not stored in the database, and thus, when the DBMS
**	supports larger names, the size of this structure will be increased.
**
** History:
**     07-sep-89 (neil)
**          Written for Terminator II/alerters
*/
typedef struct _DB_ALERT_NAME
{
    DB_EVENT_NAME dba_alert;		/* Name of the alert/event */
    DB_OWN_NAME	dba_owner;		/* Owner of the alert/event */
    DB_DB_NAME	dba_dbname;		/* Database of the alert/event */
} DB_ALERT_NAME;

/*
**	Types and names associated with errors.
*/


/*
**      The types and names associated with tables (for example, table names,
**	storage structures, etc.).
*/

/*
**	Codes for storage structures. Sort of looks like they are all prime
**	numbers - but I don't know why (I just added TPROC as 17 to be safe)
**	- dougi (5-may-2008)
**	(kschendel) no reason for it, future codes can use the holes.
*/

#define                 DB_SORT_STORE   1   /* sorted heap storage structure*/
                                            /* DB_SORT_STORE used only in the
                                            ** optimizer and QEF to model
                                            ** files from sorter, i.e. it
                                            ** not used in base relations
                                            */
#define                 DB_HEAP_STORE   3   /* heap storage structure id */
#define                 DB_ISAM_STORE   5   /* isam storage structure id */
#define                 DB_HASH_STORE   7   /* hash storage structure id */
#define                 DB_BTRE_STORE   11  /* btree storage structure id */
#define                 DB_RTRE_STORE   13  /* rtree storage structure id */
#define			DB_TPROC_STORE	17  /* table procedure */

#define			DB_STDING_STORE_MAX 17 /* Max "standard ingres" ID */

/* All storage structure codes > DB_STDING_STORE_MAX are Vectorwise codes */

#define			DB_X100_STORE	19  /* X100 table */
#define			DB_X100CL_STORE	23  /* X100 clustered table */
#define			DB_X100IX_STORE	29  /* X100 indexed table */
#define			DB_X100RW_STORE	31  /* X100 "row" table (PAX) */

/*
**	Various sizes.
**	
**	    The following case *must* hold:
**
**		DB_MAX_COLS <= DB_GW2_MAX_COLS
**		    (If not, the front ends will not be able to
**		     process the excess columns)		
**		
**	06-aug-91 (andre)
**	    map of attributea as it gets stored in DB_PROTECT and DB_INTEGRITY
**	    must accomodate DB_MAXCOLS+1 bits (attributes are numbered starting
**	    with 1, not 0).  Hence the definition of DB_COL_WORDS will be
**	    changed from ((DB_MAX_COLS+BITS_IN(i4)-1)/BITS_IN(i4)) to
**	    (DB_MAX_COLS/BITS_IN(i4)+1)
**	30-May-2006 (jenjo02)
**	    Added DB_MAXIXATTS, the maximum number of attributes in an index.
**	    DB_MAXKEYS defines the max number of key + non-key columns a
**	    user may name, but indexes on Clustered Tables may add
**	    up to DB_MAXKEYS internal Cluster Key attributes.
**	27-Apr-2010 (kschendel) SIR 123639
**	    Add DB_COL_BYTES for byte-izing the bitmap catalog columns.
*/

#define			DB_COL_WORDS  (DB_MAX_COLS/BITS_IN(i4)+1)
						/* # of words in col masks  */

/* Make sure we compute size of BYTE columns holding bitmaps in terms
** of the i4's, not the exact number of bytes needed, because that's how
** the structures and old catalogs were defined.
** (i.e. DB_MAX_COLS/8+1 would be wrong)
*/
#define			DB_COL_BYTES  (DB_COL_WORDS*sizeof(i4))
#define			DB_COL_BITS   ((DB_COL_WORDS*BITS_IN(i4))-1)
						/* # bits in col masks - 1  */
#define			DB_MAXKEYS	32	/* Max # of keys in 2ary idx */
#ifdef NOT_UNTIL_CLUSTERED_INDEXES
#define			DB_MAXIXATTS	2 * DB_MAXKEYS
						/* Max # of atts in 2ary idx */
#else
#define			DB_MAXIXATTS DB_MAXKEYS  /* ***** for now */
#endif

#define                 DB_MAXBTREE_KEY 2000    /* Max # bytes in BTREE key */

#define			DB_PG_INVALID   0
#define			DB_PG_V1	1
#define			DB_PG_V2	2
#define			DB_PG_V3	3
#define			DB_PG_V4	4
#define			DB_PG_V5	5
#define			DB_PG_V6	6
#define			DB_PG_V7	7
#define			DB_MAX_PGTYPE   7

#define			DB_VPT_PAGE_OVERHEAD_MACRO(page_type) \
				((page_type == DB_PG_V1) ? 38 : 80)

/* Line table Entry Size    */
#define			DB_VPT_SIZEOF_LINEID(page_type) \
				((page_type == DB_PG_V1) ? 2 : 4)

/* Tuple Header Size        */
#define			DB_VPT_SIZEOF_TUPLE_HDR(page_type)	\
		((page_type == DB_PG_V1) ? 0 : 			\
		(page_type == DB_PG_V2) ? 24 :			\
		(page_type == DB_PG_V3) ?  6 :			\
		(page_type == DB_PG_V4) ?  4 :			\
		(page_type == DB_PG_V5) ?  14 : /* FIX ME 14 for now */ \
		(page_type == DB_PG_V6) ?  12 :			\
		(page_type == DB_PG_V7) ?  20 : 0)

#define                 DB_MAXRTREE_DIM 4       /* Max # RTREE dimensions */
/*  MAXRTREE_NBR = MAXRTREE_DIM * 2 * NBR size or 4 * 2 * 3 or 24  */
#define                 DB_MAXRTREE_NBR 24      /* Max # bytes in RTREE NBR */
/*  MAXRTREE_KEY = MAXRTREE_DIM * 3 * NBR size + sizeof(TID) or 4*3*3+4 or 40 */
#define                 DB_MAXRTREE_KEY 40      /* Max # bytes in RTREE key */


/*}
** Name: DB_DBP_NAME - Procedure name
**
** Description:
**      This structure defines a procedure name.
**
** History:
**     08-apr-88 (seputis)
**          initial creation
*/
typedef struct _DB_DBP_NAME
{
    char            db_dbp_name[DB_DBP_MAXNAME];    /* procedure name */
} DB_DBP_NAME;

/*}
** Name: DB_AREANAME - Location area name
**
** Description:
**      This structure defines a location area name
**
** History:
**     31-aug-1989 (ralph)
**          initial creation
*/
typedef struct _DB_AREANAME
{
    char            db_areaname[DB_AREA_MAX];    /* area name */
} DB_AREANAME;

/*}
** Name: DB_PARM_NAME - Procedure parameter name
**
** Description:
**      This structure defines a procedure parameter name.
**
** History:
**     24-jun-88 (stec)
**          initial creation
*/
typedef struct _DB_PARM_NAME
{
    char            db_parm_name[DB_PARM_MAXNAME];    /* db proc parm name */
} DB_PARM_NAME;


/*}
** Name: DB_TAB_NAME - Table name
**
** Description:
**      This structure defines a table name.
**
** History:
**     08-jul-85 (jeff)
**          written
*/
typedef struct _DB_TAB_NAME
{
    char            db_tab_name[DB_TAB_MAXNAME];    /* table name */
} DB_TAB_NAME;

/*}
** Name: DB_SCHEMA_ID - Schema id.
**
** Description:
**      This structure defines an id which uniquely identifies a schema.
**	Currently, these 8 byte quantities are allocated the same way
**	as table ids.
**
** History:
**	27-jul-92 (rickh)
**	    Created for FIPS CONSTRAINTS.
**	19-nov-92 (rickh)
**	    Recast DB_SCHEMA_ID, DB_CONSTRAINT_ID, DB_RULE_ID, and DB_DEF_ID
**	    as DB_TAB_IDs since we allocate them that way and coerce them
**	    into IIDBDEPENDS tuples as such.
**	2-mar-93 (rickh)
**	    Added canonical schema id for the $ingres account.
**	9-apr-93 (rickh)
**	    Replaced redundant definition of SET_CANON_DEF_ID with a
**	    definition of SET_CANON_SCHEMA_ID.
*/
typedef struct _DB_TAB_ID	DB_SCHEMA_ID;

#define	SET_SCHEMA_ID( target, firstHalf, secondHalf )	\
	target.db_tab_base = firstHalf;			\
	target.db_tab_index = secondHalf;

/* macros for manipulating the canonical schema ids listed below
 */
#define SET_CANON_SCHEMA_ID(var,defid)		\
	{  var.db_tab_base  = defid;		\
	   var.db_tab_index = 0L;		\
	}

#define	DB_INGRES_SCHEMA_ID		0x00000001L

/*}
** Name: DB_TAB_OWN - Table owner name.
**
** Description:
**      This structure defines the name of a table owner.
**
** History:
**     08-jul-85 (jeff)
**          written
**     02-sep-86 (seputis)
**          changed to coding standard
*/
typedef struct
{
    DB_OWN_NAME     db_tab_own;         /* table owner name */
} DB_TAB_OWN;

/*}
** Name: DB_TAB_LOC - Table location name.
**
** Description:
**      This structure defines the name of a table location.
**
** History:
**     08-jul-85 (jeff)
**          written
*/
typedef struct
{
    DB_LOC_NAME     db_tab_loc;         /* table location name */
} DB_TAB_LOC;

/*}
** Name: DB_TAB_EXPIRES - Table expiration date.
**
** Description:
**      This structure defines the expiration date of a table in seconds
**      since Jan. 1, 1970.
**
** History:
**     08-jul-85 (jeff)
**          written
*/
typedef struct
{
    u_i4            db_tab_expires;     /* table expire date:secs from 1/1/70 */
} DB_TAB_EXPIRES;

/*}
** Name: DB_TAB_TIMESTAMP - Timestamp on table.
**
** Description:
**      This structure defines a timestamp on a table, for telling when it
**      was created, modified, etc.
**
** History:
**     08-jul-85 (jeff)
**          written
*/
typedef struct
{
    u_i4	    db_tab_high_time;
    u_i4	    db_tab_low_time;
} DB_TAB_TIMESTAMP;

/*
**      The types and names associated with transactions.
*/

/*}
** Name: DB_SP_NAME - Savepoint name.
**
** Description:
**      This structure defines a savepoint name.
**
** History:
**     23-jul-85 (jeff)
**          written
*/
typedef struct _DB_SP_NAME
{
    char            db_sp_name[DB_MAXNAME];     /* savepoint name */
} DB_SP_NAME;

/*
**  The types and names associated with sessions.
*/

#define	    DB_NOSESSION    (~0)  /* there are no sessions with this id */

/*}
** Name: DB_DISTRIB - Distributed/Non-distributed indicator
**
** Description:
**	This defines a distributed indicator id, which tells whether distributed
**	constructs will be allowed.
**
** History:
**      10-oct-85 (jeff)
**          written
**	02-jan-86 (jeff)
**	    moved here from a parser header
*/
typedef i4  DB_DISTRIB;
#define                 DB_DST_DONTCARE (DB_DISTRIB) 0    /* Not specified */
#define                 DB_DSTYES      (DB_DISTRIB) 1	    /* Do allow dist. */
#define                 DB_DSTNO       (DB_DISTRIB) 2	    /* Don't allow */
#define DB_1_LOCAL_SVR   0x0001L		/* ON if server supports
						** local sessions */
#define DB_2_DISTRIB_SVR 0x0002L		/* ON if server supports
						** distributed sessions */
#define DB_3_DDB_SESS    0x0004L		/* ON if session is distributed,
						** OFF if local */

/*
** The types and names associated with queries.
*/

#ifdef NOT_NEEDED

/*}
** Name: DB_RPT_NAME - Name of a repeat query.
**
** Description:
**      This structure defines the name of a repeat query as sent by the
**	front-ends.
**
** History:
**     01-apr-86 (jeff)
**          written
*/
typedef struct _DB_RPT_NAME
{
    char            db_tab_name[DB_TAB_MAXNAME]; /* Repeat query name */
}   DB_RPT_NAME;

#endif /* NOT_NEEDED */


/*}
** Name: DB_TIME_ID - Time stamp when permit was created.
**
** Description:
**      This structure contains a time stamp to indicate when a permit
**	was created.  The time stamp previously was the query text
**	id, but since query text can be regenerated as a result of
**	REVOKE or ALTER TABLE (?), that time stamp no longer represents
**	when the permit was originally created.  
**
** History:
**     29-may-90 (ralph)
**          written
*/
typedef struct _DB_TIME_ID
{
    u_i4	    db_tim_high_time;
    u_i4	    db_tim_low_time;
} DB_TIME_ID;


/*}
** Name: DB_TREE_ID - Tree id.
**
** Description:
**      This structure defines an tree id, which is used to uniquely
**      identify a query tree within a table. This identifier exists
**	because any table may have one or more views, protections or
**	integrity constraints associated with it, each requiring a
**	different tree.
**
**	A tree id is really just a timestamp.
**
** History:
**     16-jul-86 (ac)
**          written
*/
typedef struct _DB_TREE_ID
{
    u_i4	    db_tre_high_time;
    u_i4	    db_tre_low_time;
} DB_TREE_ID;


/*}
** Name: DB_CONSTRAINT_NAME - Constraint name as stored in iiintegrity relation
**
** Description:
**      This structure defines the name of a constraint as laid out in IIQRYTEXT
**	relation.
**
** History:
**     27-jul-92 (rickh)
**          written for FIPS CONSTRAINTS
*/
typedef struct _DB_CONSTRAINT_NAME
{
    char	db_constraint_name[ DB_CONS_MAXNAME ];
}   DB_CONSTRAINT_NAME;


/*}
** Name: DB_CONSTRAINT_ID - ID for query text stored in iikey relation
**
** Description:
**      This structure defines the id of a constraint as stored in IIKEY.
**	Currently, it's an 8 byte quantity allocated the same way as a table id.
**
** History:
**     27-jul-92 (rickh)
**          written for FIPS CONSTRAINTS
**	19-nov-92 (rickh)
**	    Recast DB_SCHEMA_ID, DB_CONSTRAINT_ID, DB_RULE_ID, and DB_DEF_ID
**	    as DB_TAB_IDs since we allocate them that way and coerce them
**	    into IIDBDEPENDS tuples as such.
*/
typedef struct _DB_TAB_ID	DB_CONSTRAINT_ID;

#define	COPY_CONSTRAINT_ID( source, target )		\
	target.db_tab_base = source.db_tab_base;	\
	target.db_tab_index = source.db_tab_index;

/*}
** Name: DB_RULE_ID - ID for query text stored in IIRULE relation
**
** Description:
**      This structure defines the id of a rule as stored in IIRULE.
**	Currently, it's an 8 byte quantity allocated the same way as a table id.
**
** History:
**     27-jul-92 (rickh)
**          written for FIPS CONSTRAINTS
**	19-nov-92 (rickh)
**	    Recast DB_SCHEMA_ID, DB_CONSTRAINT_ID, DB_RULE_ID, and DB_DEF_ID
**	    as DB_TAB_IDs since we allocate them that way and coerce them
**	    into IIDBDEPENDS tuples as such.
*/
typedef struct _DB_TAB_ID	DB_RULE_ID;


/*
** Types and names used by qrymod and RDF.
*/
/*}
** Name: DB_COLUMN_BITMAP - column bitmap in iirule
**
** Description:
**	This is a bitmap of columns.  If ANY of these columns are touched
**	in a query, then the rule is fired.
**
**
** History:
**     27-jul-92 (rickh)
**          written for FIPS CONSTRAINTS
*/
typedef struct _DB_COLUMN_BITMAP
{
    u_i4	    db_domset[DB_COL_WORDS];
}   DB_COLUMN_BITMAP;

/*
** Name: DB_COLUMN_BITMAP_INIT - every element of DB_COLUMN_BITMAP.db_domset to
**				 a specified value
**
** Input:
**	map	DB_COLUMN_BITMAP.db_domset
**	i	index var
**	val	val;ue to be assigned
**	
** History:
**	20-dec-92 (andre)
**	    written
*/
#define DB_COLUMN_BITMAP_INIT(map, i, val) for(i=0;i<DB_COL_WORDS;i++) map[i]=((u_i4) val);

/*}
** Name: DB_IIDEFAULTIDX - tuple in index on iidefault
**
** Description:
**	Layout of tuple in iidefaultidx system index.
**
**
** History:
**	2-Nov-92 (rickh)
**	    Created.
**	5-dec-92 (rblumer)
**	    took out default_type field.
**	25-jan-93 (rblumer)
**	    changed DB_TEXT_STRING to a u_i2 and a char array;
**	    since this is an on-disk structure, using DB_TEXT_STRING causes
**	    padding/alignment problems on some systems. 
**	22-feb-93 (rickh)
**	    Upped the length of the value string in IIDEFAULT by 1 byte to
**	    force 4 byte alignment.  Also accounted for the null indicator
**	    byte.
*/
typedef struct _DB_IIDEFAULTIDX
{
    u_i2	dbdi_def_length;    /* actual length of text in dbd_def_value
				    ** (this is a varchar field on disk)
				    */
    char	dbdi_def_value[DB_MAX_COLUMN_DEFAULT_LENGTH + 1 ];
				    /* we add 1 to account for the null
				    ** indicator byte.  note that the
				    ** null indicator byte plus the two
				    ** byte length field account for 3
				    ** bytes hidden from the user.
				    ** 1501 + 3 ends up being 4 byte aligned,
				    ** which is good, because we have one
				    ** more field in this structure, the tidp.
				    */
    u_i4	dbdi_tidp;	    /* index into IIDEFAULT*/

}   DB_IIDEFAULTIDX;

/*}
** Name: DB_IIKEY tuple in iikey system relation
**
** Description:
**	Layout of tuple in iikey system relation.
**
**
** History:
**     27-jul-92 (rickh)
**          written for FIPS CONSTRAINTS
**	28-jan-93 (rickh)
**	    Corrected a typo.  DB_DB_IIKEY should be DB_IIKEY.
*/
typedef struct _DB_IIKEY
{
    DB_CONSTRAINT_ID	dbk_constraint_id;
    DB_ATT_ID		dbk_column_id;
    i2			dbk_position;
}   DB_IIKEY;

/*}
** Name: DB_PROCEDURE - Procedure tuple in iiprocedure
**
** Description:
**      This structure defines an procedure tuple.  It is used by
**	in PSF and by RDF to determine how to lookup up text for a
**      procedure object in IIQRYTEXT.  The db_txtlen field is the
**      number of text bytes are stored in all IIQRYTEXT tuples
**      for this procedure.
**
** History:
**	18-apr-88 (seputis)
**          initial creation
**	12-Aug-1988 (andre)
**	    modified to include procedure id and date of creation
**	15-aug-88 (stec)
**	    Update the definition of DB_PROCEDURE (iiprocedure tuple).
**	21-mar-89 (sandyh)
**	    added security labeling
**	25-apr-89 (andre)
**	    Added DB_IPROC for internal procedures.
**	27-jul-92 (rickh)
**	    FIPS CONSTRAINTS: added setinput, constraint, and system generated
**	    flags to db_mask.
**	03-mar-92 (andre)
**	    Defined DB_DBPGRANT_OK and DB_ACTIVE_DBP
**	18-mar-92 (andre)
**	    defined DB_DBP_INDEP_LIST
**	28-oct-92 (jhahn)
**	    Added db_parameterCount and db_recordWidth.
**	21-jun-93 (robf)
**	    Added security label fields
**	01-sep-93 (andre)
**	    added db_dbp_ubt_id
**	26-oct-93 (andre)
**	    added db_created.  Until now, db_txtid contained a timestamp, so 
**	    one could apply gmt_timestamp() to its first half to obtain 
**	    creation date.  However, due to the fact that timestamps turned 
**	    out NOT to be guaranteed to be unique, we decided to use 
**	    "randomized" combination of dbproc id as a key into IIQRYTEXT 
**	30-Jun-2004 (schka24)
**	    security stuff -> free.
**	19-june-06 (dougi)
**	    Added DBP_DATA_CHANGE to identify procs with at least one INSERT, 
**	    UPDATE or DELETE statement - for BEFORE trigger validation.
**	17-march-2008 (dougi)
**	    Added DBP_ROW_PROC flag for row producing procedures.
*/
typedef struct _DB_PROCEDURE
{
    DB_DBP_NAME	    db_dbpname;		/* procedure name */
    DB_OWN_NAME	    db_owner;		/* Name of user defining procedure */
    i4		    db_txtlen;		/* total length of Query text for   
					** procedure
					*/
    DB_QRY_ID	    db_txtid;		/* Query text id of procedure text */
    i4		    db_mask[2];		/* procedure masks to be defined */

					/* internal dbproc */
#define		DB_IPROC	    ((i4) 0x0001)

					/*
					** grantable dbproc;
					** DB_DBPGRANT_OK --> DB_ACTIVE_DBP
					*/
#define		DB_DBPGRANT_OK	    ((i4) 0x0002)

					/*
					** dbproc is active (not dormant);
					** DB_ACTIVE_DBP --> DB_DBP_INDEP_LIST
					*/
#define		DB_ACTIVE_DBP	    ((i4) 0x0004)

					/*
					** list of independent
					** objects/privileges for this dbproc is
					** in IIDBDEPENDS/IIPRIV
					*/
#define		DB_DBP_INDEP_LIST   ((i4) 0x0008)

#define 	DBP_SETINPUT 	  ((i4) 0x010)  /* set-input proc */
#define		DBP_NOT_DROPPABLE ((i4) 0x020)  /* not user-droppable */
#define 	DBP_CONS	  ((i4) 0x040)  /* enforces a constraint so must
						** bypass privilege checking 
						** (i.e. bypass grantability) 
						*/
#define		DBP_SYSTEM_GENERATED ((i4) 0x080) /* created by the system */
#define		DBP_DATA_CHANGE	  ((i4) 0x0100)	/* proc contains ins/upd/del */
#define		DBP_OUT_PARMS	  ((i4) 0x0200)	/* proc contains at least one
					** OUT or INOUT parm */
#define		DBP_ROW_PROC	  ((i4) 0x0400) /* proc is row producing */
#define		DBP_TX_STMT	  ((i4) 0x0800)	/* proc has COMMIT/ROLLBACK */

    DB_TAB_ID	    db_procid;		/* procedure id  */
    i4		    db_parameterCount;	/* number of formal parameters.  there
					** will be a corresponding number
					** of tuples in IIPROCEDURE_PARAMETER*/
    i4		    db_recordWidth;	/* Total width of parameters. */
    DB_TAB_ID	    db_dbp_ubt_id;	/* underlying base table id */  
    u_i4	    db_created;		/* 
					** first half of timestamp by applying 
					** gmt_timestamp() to which one will 
					** obtain creation date for the dbproc 
					*/
    i4		    db_rescolCount;	/* number of result row columns. There
					** will be a corresponding number of
					** tuples of DB_PROCEDURE_PARAMETER */
    i4		    db_resrowWidth;	/* total width of result row */
    i4		    db_estRows;		/* Estimated number of rows returned
					** by invocation of table procedure */
    i4		    db_estCost;		/* Estimated cost (combined disk/CPU)
					** by invocation of table procedure */
#define		DBP_ROWEST	((f4) 50.0)
#define		DBP_DIOEST	((f4) 50.0)
}   DB_PROCEDURE;

/*}
** Name: DB_PROCEDURE_PARAMETER - Procedure tuple in iiprocedure_parameter
**
** Description:
**      This structure defines an procedure parameter tuple.
**
** History:
**	26-oct-92 (jhahn)
**          initial creation
**	04-nov-92 (rickh)
**	    Padded dbpp_precision for alignment purposes.
**	20-jan-93 (rblumer)
**	    Added NDEFAULT flag.
**	26-Mar-93 (jhahn)
**	    Removed extraneous field from DB_PROCEDURE_PARAMETER
**	7-june-06 (dougi)
**	    Added flags to enable IN, OUT, INOUT parameters.
**	17-march-2008 (dougi)
**	    Added DBPP_RESULT_COL flag for result row columns.
*/
typedef struct _DB_PROCEDURE_PARAMETER
{
    DB_TAB_ID	    dbpp_procedureID;	/* procedure id  */

    DB_ATT_NAME     dbpp_name;		/* Attribute name. */
    DB_DEF_ID	    dbpp_defaultID;	/* this is the index in IIDEFAULTS
					** of the tuple containing the
					** user specified default for this
					** parameter.
					*/
    i4              dbpp_flags;		/* Odd bits. */
#define	DBPP_NOT_NULLABLE	0x00000001L
#define	DBPP_NDEFAULT		0x00000002L
#define	DBPP_IN			0x00000004L	/* IN parameter */
#define	DBPP_OUT		0x00000008L	/* OUT parameter */
#define	DBPP_INOUT		0x00000010L	/* INOUT parameter */
#define	DBPP_RESULT_COL		0x00000020L	/* result row column */

    i2              dbpp_number;	/* Attribute ordinal number. */
    i2              dbpp_offset;	/* Offset in bytes of attribute
					** in record. */
    i2              dbpp_length;	/* Length in bytes. */
    i2              dbpp_datatype;	/* Attribute type.  E.g.:
					** DB_INT_TYPE
					** DB_FLT_TYPE
					** DB_CHR_TYPE
					** DB_TXT_TYPE
					** DB_DTE_TYPE
					** DB_MNY_TYPE
					** DB_CHA_TYPE
					** DB_VCH_TYPE */
    i2              dbpp_precision;	/* Precision for unsigned integer.*/
    i2		    dbpp_pad;		/* optimistic alignment! */
} DB_PROCEDURE_PARAMETER;




/*}
** Name: DB_DBPDEPENDS - Procedure dependency catalog
**
** Description:
**      This structure defines an procedure dependency tuple.  It is used
**	by QEF to determine the tables being referenced in a database
**	procedure.
**
** History:
**     02-may-88 (puree)
**          created for DB procedure project
**     21-mar-89 (sandyh)
**	    added security labeling
*/
typedef struct _DB_DBPDEPENDS
{
    DB_TAB_ID	    dbp_tab_id;		/* based table */
    DB_DBP_NAME	    dbp_dbpname;	/* procedure name */
    DB_OWN_NAME	    dbp_owner;		/* Name of user defining procedure */
}   DB_DBPDEPENDS;


/*}
** Name: DB_SEQ	    - Sequence field
**
** Description:
**	Defines type for sequence fields
**
** History:
**	14-mar-89 (ralph)
**	    written
*/
typedef	i4  DB_SEQ;

/*}
** Name: DB_INTEGRITY - Integrity tuple
**
** Description:
**      This structure defines an integrity tuple.  It is used by qrymod
**	in PSF and by RDF.
**
** History:
**     16-may-86 (jeff)
**          written
**     06-feb-89 (ralph)
**          changed dbi_domset to use DB_COL_WORDS in array dimension
**          added dbi_reserve for future expansion
**	14-mar-89 (ralph)
**	    GRANT Enhancements, Phase 1:
**	    added dbi_seq field for future expansion.
**     21-mar-89 (sandyh)
**	    added security labeling
**	27-jul-92 (rickh, rblumer)
**	    Added FIPS CONSTRAINTS fields:  constraint id, schema id,
**	    constraint flags.  Changed type of dbi_domset to 
**	    DB_COLUMN_BITMAP and changed its name to dbi_columns.
**	17-mar-93 (rblumer)
**	    Added KNOWN_NOT_NULLABLE_TUPLE bit to DB_INTEGRITY,
**	    to be used in the tuple containing the bitmap of 
**	    known-not-nullable columns.
**	26-oct-93 (andre)
**	    added dbi_created.  Until now, dbi_txtid contained a timestamp, so 
**	    one could apply gmt_timestamp() to its first half to obtain 
**	    creation date.  However, due to the fact that timestamps turned 
**	    out NOT to be guaranteed to be unique, we decided to use 
**	    "randomized" combination of object id and integrity (or constraint)
**	    number as a key into IIQRYTEXT and tuple id in IITREE (if there is 
**	    a tree associated with this integrity/constraint)
**	16-aug-99 (inkdo01)
**	    Added dbi_delrule/updrule plus accompanying #define's to describe
**	    delete/update rule of referential constraints.
**	24-Jul-2002 (inifa01)
**	    Bug 106821.  Added CONS_BASEIX dbi_consflags bit.  
*/
typedef struct _DB_INTEGRITY
{
    DB_TAB_ID       dbi_tabid;          /* Table id of restricted table */
    DB_QRY_ID	    dbi_txtid;		/* Query text id of integrity text */
    DB_TREE_ID	    dbi_tree;		/* Id of tree in iitree relation */
    i2		    dbi_resvar;		/* Number of result variable */
    i2		    dbi_number;		/* Integrity number */
    DB_SEQ	    dbi_seq;		/* Sequence number for expansion */
    DB_COLUMN_BITMAP    dbi_columns;	/* Bit map of restricted cols */

    DB_CONSTRAINT_NAME  dbi_consname;   /* constraint name */
    DB_CONSTRAINT_ID    dbi_cons_id;    /* constraint id */
    DB_SCHEMA_ID    dbi_consschema;
    i4              dbi_consflags;

/*      Constraint type, one of */

#define    CONS_NONE        0    /* current INGRES integrity */
#define    CONS_UNIQUE   0x00000001   /* unique constraint        */
#define    CONS_CHECK    0x00000002   /* check constraint         */
#define    CONS_REF      0x00000004   /* referential integrity constraint */

/*      in addition to the above, one of the following may be set */

#define    CONS_PRIMARY  0x00000008   /* unique constraint that is a primary key */

#define    CONS_BASEIX   0x00000010   /* index = base table structure */

#define CONS_NON_NULLABLE_DATATYPE 0x00000020
	/* a "known not nullable" constraint can be specified either
	** by tagging  "NOT NULL" on the datatype or by appending
	** "CHECK ... NOT NULL."  in both cases we register a constraint.
	** however, in the first case, we also allocate in the tuple
	** only enough space for a non-nullable datatype.  this bit
	** flags this fact and is used by UNLOADDB to figure out whether
	** to recreate the column with "NOT NULL" or "CHECK ... NOT NULL"
	** syntax.  
	** This flag is also used to tell use whether or not this
	** constraint can be dropped or not, as a NOT NULL constraint
	** cannot be dropped when it is enforced by a non-nullable datatype
	** instead of by a rule. (but this may be relaxed in the future).
	*/

/*      Initial Constraint enforcement time and Constraint defer-ability */

#define    CONS_DEFERRED 0x00000040   
         /* if set, constraint is deferred; enforced at end of transaction
         ** if not set, constraint is immediate; enforced at end of statement 
         ** not set (immediate) is the DEFAULT 
         ** (deferred will not be implemented until we implement Full SQL2)
         */
#define    CONS_CANDEFER 0x00000080   
         /* if set, constraint can be deferred until end of transaction
         ** if not set, constraint cannot be deferred; it is always immed.
         ** not set is the DEFAULT
         ** (deferrable will not be implemented until we implement Full SQL2)
         */
         /* NOTE 1: in release 6.5, the DEFERRED and CANDEFER bits
         **         will NOT be used;
         ** NOTE 2: in Full SQL2, only the following combinations will be valid:
         **           CONS_DEFERRED     CONS_CANDEFER
         **                0                 0
         **                0                 1
         **                1                 1
         */
#define    CONS_KNOWN_NOT_NULLABLE     0x00000100   
				/* check constraint that 
				** specifies NOT NULL for a column.
				** indicates that a separate DB_INTEGRITY
				** tuple contains a bitmap of columns
				** which are known not nullable.
				** This bitmap is stored in the (now 
				** overloaded) dbi_columns field of that
				** second tuple.
                                */
#define    CONS_KNOWN_NOT_NULLABLE_TUPLE     0x00000200 
				/* this tuple contains a bitmap of columns
				** that are known not nullable because of this
				** constraint. All other fields (except for
				** sequence number) are the same as for the
				** original tuple for this constraint.
				*/


    u_i4	    dbi_created;	/* 
					** first half of timestamp by applying 
					** gmt_timestamp() to which one will 
					** obtain creation date for the 
					** integrity or constraint
					*/
    u_i2	    dbi_delrule;	/*
					** for referential constraints, the
					** delete rule */
    u_i2	    dbi_updrule;	/*
					** for referential constraints, the
					** update rule */
#define	   DBI_RULE_NOACTION	0
#define	   DBI_RULE_RESTRICT	1
#define	   DBI_RULE_CASCADE	2
#define	   DBI_RULE_SETNULL	3
    char	    dbi_reserve[28];	/* Reserved		    */
}   DB_INTEGRITY;

/*}
** Name: DB_INTEGRITYIDX - Integrity index tuple
**
** Description:
**      This structure defines a tuple in the index on IIINTEGRITY.
**
** History:
**	2-NOV-92 (rickh)
**	    Creation.
**	13-apr-93 (andre)
**	    changed structure to match the external definition
*/
typedef struct _DB_INTEGRITYIDX
{
    DB_SCHEMA_ID    	  dbii_consschema;
    DB_CONSTRAINT_NAME    dbii_consname;     /* constraint name */
    u_i4		  dbii_tidp;	     /* index into IIINTEGRITY */
}   DB_INTEGRITYIDX;

/*}
** Name: DB_PROTECTION - Protection tuple
**
** Description:
**      This defines the structure of a protection tuple as used by qrymod
**	in PSF and by RDF.
**
** History:
**     16-may-86 (jeff)
**          written
**     06-feb-89 (ralph)
**          changed dbp_domset to use DB_COL_WORDS in array dimension
**          added dbp_reserve for future expansion
**	15-mar-89 (ralph)
**	    GRANT Ehhancements, Phase 1:
**	    added dbp_gtype to specifiy grantee type
**	    added dbp_seq for sequence number for future expansion
**     21-mar-89 (sandyh)
**	    added security labeling
**	31-mar-89 (ralph)
**	    GRANT Enhancements, Phase 2:
**	    Added grantee type flags, DBGR_{DEFAULT,USER,GROUP,APLID,PUBLIC}
**	08-aug-90 (ralph)
**	    Added fields to DB_PROTECTION for FIPS, GRANT and REVOKE.
**	27-nov-91 (andre)
**	    changed dbp_obname from DB_OWN_NAME to DB_TAB_NAME
**	07-jul-92 (andre)
**	    renamed dbp_fill1 to dbp_flags and defined DBP_SQL_PERM and
**	    DBP_GRANT_PERM over it.
**	09-jul-92 (andre)
**	    defined DBP_65_PLUS_PERM
**	21-aug-92 (andre)
**	    renamed dbp_fill2 to dbp_depth
**	    "depth" of a permit will be defined as a number associated with a
**	    permit s.t. given two permits on an object X, permit with depth N
**	    can be safely recreated before permit with depth N+k where k>=0
**	3-may-02 (inkdo01)
**	    Add code for sequence protections.
*/
typedef struct _DB_PROTECTION
{
    DB_TAB_ID       dbp_tabid;          /* Table id of restricted table */
    i2		    dbp_permno;		/* Permit number for this table */
    i2              dbp_flags;          /* useful information about permit */

					/* permit was created in SQL */
#define	    DBP_SQL_PERM	    ((i2) 0x01)

					/*
					** permit was created using GRANT
					** statement
					*/
#define	    DBP_GRANT_PERM	    ((i2) 0x02)

					/*
					** permit created in 6.5 or later - this
					** is important when running pre-6.5 and
					** 6.5+ permits coexist in a database -
					** hopefully an upgradedb time only
					** phenomenon
					*/
#define	    DBP_65_PLUS_PERM	    ((i2) 0x04)

    i2		    dbp_pdbgn;		/* Time of day permit begins	*/
    i2		    dbp_pdend;		/* Time of day permit ends	*/
    i2		    dbp_pwbgn;		/* Day of week permit begins	*/
    i2		    dbp_pwend;		/* Day of week permit ends	*/
    DB_QRY_ID	    dbp_txtid;		/* Query text id of permit statement */
    DB_TREE_ID	    dbp_treeid;		/* Query tree id, zero if none	*/
    DB_TIME_ID	    dbp_timestamp;	/* Time stamp when permit	*/
    i4		    dbp_popctl;		/* Bit map of defined   operations */
    i4		    dbp_popset;		/* Bit map of permitted operations */
    i4              dbp_value;          /* Value associated with permit */
    i2		    dbp_depth;		/* "depth" of permit		*/
    i1		    dbp_obtype;		/* Object type			*/
#define             DBOB_TABLE      'T' /* Object is a table		*/
#define             DBOB_VIEW       'V' /* Object is a view		*/
#define             DBOB_INDEX      'I' /* Object is an index		*/
#define             DBOB_DBPROC     'P' /* Object is a dbproc		*/
#define             DBOB_EVENT      'E' /* Object is an event		*/
#define		    DBOB_SEQUENCE   'S' /* Object is a sequence		*/
#define             DBOB_LOC        'L' /* Object is a location		*/
#define		    DBOB_DATABASE   'D' /* Object is a database         */
    i1		    dbp_obstat;		/* Object status		*/
#define		    DB_SYSTEM	    'S'	/* Object is system owned	*/
    DB_TAB_NAME	    dbp_obname;		/* Object name			*/
    DB_OWN_NAME     dbp_obown;          /* Object owner			*/
    DB_OWN_NAME     dbp_grantor;        /* Permit grantor		*/
    DB_OWN_NAME	    dbp_owner;		/* Name of user getting permission */
    DB_TERM_NAME    dbp_term;		/* Terminal id at which perm. granted */
    i2		    dbp_fill3;		/* Filler for alignment		*/
    i2		    dbp_gtype;		/* Grantee type:		*/
#define		    DBGR_DEFAULT    -1	/*  Installation default	*/
#define		    DBGR_USER	     0	/*  User			*/
#define		    DBGR_GROUP	     1	/*  Group			*/
#define		    DBGR_APLID	     2	/*  Application identifier	*/
#define		    DBGR_PUBLIC	     3	/*  Public			*/
    DB_SEQ	    dbp_seq;		/* Sequence number for expansion */
    i4		    dbp_domset[DB_COL_WORDS]; /*Bit map of permitted columns*/
    char	    dbp_reserve[32];	/* Reserved			*/
}   DB_PROTECTION;

/*}
** Name: DB_USERGROUP - iiusergroup tuple
**
** Description:
**      This structure defines an iiusergroup tuple.
**	It is used by qrymod in PSF, and by RDF, QEF, and SCF
**
** History:
**     05-mar-89 (ralph)
**          written
*/
typedef struct _DB_USERGROUP
{
	DB_OWN_NAME	    dbug_group;		/* Group identifier */
	DB_OWN_NAME	    dbug_member;	/* Group member
						** (blank for group anchor)
						*/
	char		    dbug_reserve[32];	/* Reserved */
}   DB_USERGROUP;

/*}
** Name: DB_APPLICATION_ID - iiapplication_id tuple
**
** Description:
**      This structure defines an iiapplication_id tuple.
**	It is used by qrymod in PSF, and by RDF, QEF, and SCF
**
** History:
**     05-mar-89 (ralph)
**          written
**	9-nov-92 (ed)
**	    use DB_PASSWORD instead of DB_OWN_NAME
**	7-mar-94 (robf)
**          add dbap_status field to hold role status mask, and
**	     dbap_flags fields to hold role flags mask.
*/
typedef struct _DB_APPLICATION_ID
{
	DB_OWN_NAME	    dbap_aplid;		/* Application identifier */
	DB_PASSWORD	    dbap_apass;		/* Application id password
						** (encrypted)
						*/
	char		    dbap_free[16];	/* Get rid of me */
	i4		    dbap_status;	/* Privilege/status mask */
	i4		    dbap_flagsmask;	/* Role flags mask */
	char		    dbap_reserve[8];	/* Reserved */
}   DB_APPLICATION_ID;

/*}
** Name: DB_PRIVILEGES - iidbpriv tuple
**
** Description:
**      This structure defines an iidbpriv tuple.
**	It is used by qrymod in PSF, and by RDF, QEF, and SCF
**
** History:
**     31-mar-89 (ralph)
**          written
**	24-apr-89 (ralph)
**	    GRANT Enhancements, Phase 2a:
**	    Create flags DBPR_VALUED, DBPR_BINARY, DBPR_ALL.
**     14-jul-89 (jennifer)
**          Added access to a database as dbpriv instead of dbaccess.
**     31-sep-89 (ralph)
**          Changed db_privileges.dbpr_fill1 to dbpr_dbflags.
**	    Added DBPR_ALLDBS, DBPR_UPDATE_SYSCAT
**	11-oct-89 (ralph)
**	    Fixed backslash problem at DBPR_BINARY
**	08-aug-90 (ralph)
**	    Added DBPR_DB_ADMIN, DBPR_GLOBAL_USAGE database privileges
**	    Added DBPR_RESTRICTED to identify restricted database privileges
**	    Added DBPR_ADMIN to identify administrative-only database privs
**	    Added DBPR_ALLPRIVS, DBPR_C_ALLPRIVS, and DBPR_F_ALLPRIVS to
**	    implement ALL PRIVILEGES.
**	15-jan-91 (ralph)
**	    Add DBPR_OPFENUM to indicate which dbprivs require OPF enumeration
**	13-sep-93 (robf)
**	    Add DBPR_QUERY_SYSCAT privilege
**	20-oct-93 (robf)
**          Add DBPR_TABLE_STATISTICS, DBPR_IDLE_LIMIT, DBPR_CONNECT_LIMIT,
**	        DBPR_PRIORITY_LIMIT
**	3-may-02 (inkdo01)
**	    Add DBPR_SEQ_CREATE as part of sequence implementation.
*/
typedef struct _DB_PRIVILEGES
{
	DB_DB_NAME	    dbpr_database;	/* Database name:
						** blank for installation
						** default
						*/
	DB_OWN_NAME	    dbpr_gname;		/* Grantee identifier:
						** blank for installation
						** default, and for PUBLIC.
						*/
	i2		    dbpr_gtype;		/* Grantee type: 	      */
						/*	(See DB_PROTECTION)   */
	i2		    dbpr_dbflags;	/* Database flags:	      */
#define DBPR_ALLDBS	    0x0001		/*  ON INSTALLATION specified */
	i4		    dbpr_control;	/* Control flags:
						**  ON  if defined;
						**  OFF if undefined.
						*/
#define DBPR_QROW_LIMIT	    0x00000001L		/* Query row  limit specified */
#define DBPR_QDIO_LIMIT	    0x00000002L		/* Query I/O  limit specified */
#define DBPR_QCPU_LIMIT	    0x00000004L		/* Query CPU  limit specified */
#define DBPR_QPAGE_LIMIT    0x00000008L		/* Query page limit specified */
#define DBPR_QCOST_LIMIT    0x00000010L		/* Query cost limit specified */
#define DBPR_TIMEOUT_ABORT  0x00000020L         /* Timout ABort     specified */
#define DBPR_TAB_CREATE	    0x00000100L		/* Create Table     specified */
#define DBPR_PROC_CREATE    0x00000200L		/* Create Procedure specified */
#define DBPR_LOCKMODE	    0x00000400L		/* Lockmode         specified */
#define DBPR_ACCESS	    0x00000800L		/* ACCESS	    specified */
#define DBPR_UPDATE_SYSCAT  0x00001000L		/* Update Syscat    specified */
#define DBPR_DB_ADMIN	    0x00002000L		/* DB_ADMIN	    specified */
#define DBPR_GLOBAL_USAGE   0x00004000L		/* GLOBAL_USAGE	    specified */
#define DBPR_QUERY_SYSCAT   0x00008000L		/* QUERY_SYSCAT     specified */
#define DBPR_TABLE_STATS    0x00010000L		/* TABLE_STATISTICS specified */
#define DBPR_IDLE_LIMIT     0x00020000L         /* IDLE_TIME_LIMIT  specified */
#define DBPR_CONNECT_LIMIT  0x00040000L         /* CONNECT_TIME_LIMIT */
#define DBPR_PRIORITY_LIMIT 0x00080000L         /* SESSION_PRIORITY specified */
#define DBPR_SEQ_CREATE	    0x00100000L		/* Create Sequence  specified */
#define	DBPR_ALLPRIVS	    0x80000000L		/* ALL PRIVILEGES   specified,
						** internally used for REVOKE,
						** this bit is not stored.
						*/

#define DBPR_VALUED	   (DBPR_QROW_LIMIT	\
			  | DBPR_QDIO_LIMIT	\
			  | DBPR_QCPU_LIMIT	\
			  | DBPR_QPAGE_LIMIT	\
			  | DBPR_IDLE_LIMIT     \
			  | DBPR_CONNECT_LIMIT  \
			  | DBPR_PRIORITY_LIMIT \
			  | DBPR_QCOST_LIMIT)	/* Valued privileges	    */

#define	DBPR_BINARY	   (DBPR_TAB_CREATE	\
			  | DBPR_PROC_CREATE	\
			  | DBPR_SEQ_CREATE	\
                          | DBPR_ACCESS		\
			  | DBPR_UPDATE_SYSCAT	\
			  | DBPR_QUERY_SYSCAT	\
			  | DBPR_TABLE_STATS	\
			  | DBPR_DB_ADMIN	\
			  | DBPR_TIMEOUT_ABORT	\
			  | DBPR_LOCKMODE)	/* Binary privileges	    */

#define DBPR_ALL	   (DBPR_VALUED		\
			  | DBPR_BINARY)	/* All privileges	    */

#define DBPR_UNDEFINED	   (DBPR_QCPU_LIMIT	\
			  | DBPR_QPAGE_LIMIT	\
			  | DBPR_QCOST_LIMIT)	/* Undefined privileges
						** that are not available to
						** GRANT ALL PRIVILEGES
						*/

#define	DBPR_ADMIN	   (DBPR_UPDATE_SYSCAT	\
                          | DBPR_QUERY_SYSCAT \
                          | DBPR_TABLE_STATS  \
			  | DBPR_DB_ADMIN)	/* Administrative privileges
						** that are not included in
						** GRANT ALL PRIVILEGES
						*/

#define	DBPR_RESTRICTED	   (DBPR_QROW_LIMIT	\
			  | DBPR_QDIO_LIMIT	\
			  | DBPR_QCPU_LIMIT	\
			  | DBPR_QPAGE_LIMIT	\
			  | DBPR_QCOST_LIMIT	\
			  | DBPR_IDLE_LIMIT     \
			  | DBPR_CONNECT_LIMIT  \
			  | DBPR_PRIORITY_LIMIT \
			  | DBPR_TAB_CREATE	\
			  | DBPR_SEQ_CREATE	\
			  | DBPR_PROC_CREATE)	/* Restricted KM privileges */

#define	DBPR_C_ALLPRIVS	   (DBPR_ALL		\
			  &~DBPR_UNDEFINED	\
			  &~DBPR_ADMIN)		/* Control for ALL PRIVS    */

#define	DBPR_F_ALLPRIVS	   (DBPR_BINARY		\
			  &~DBPR_UNDEFINED	\
			  &~DBPR_ADMIN)		/* Flags   for ALL PRIVS    */

#define DBPR_OPFENUM	   (DBPR_QROW_LIMIT	\
			  | DBPR_QDIO_LIMIT	\
			  | DBPR_QCPU_LIMIT	\
			  | DBPR_QPAGE_LIMIT	\
			  | DBPR_QCOST_LIMIT)	/* When these priivileges are
						** in effect, OPF enumeration
						** must be performed.
						*/

	i4		    dbpr_flags;		/* Privilege flags:
						**  ON  if privilege allowed
						**  OFF if privilege denied
						*/
	i4		    dbpr_qrow_limit;	/* Query row  limit	      */
	i4		    dbpr_qdio_limit;	/* Query I/O  limit	      */
	i4		    dbpr_qcpu_limit;	/* Query CPU  limit	      */
	i4		    dbpr_qpage_limit;	/* Query page limit	      */
	i4		    dbpr_qcost_limit;	/* Query cost limit	      */
	i4		    dbpr_idle_time_limit;/* Idle time limit           */
	i4		    dbpr_connect_time_limit;/* Connect time limit     */
	i4		    dbpr_priority_limit;/* Priority limit             */
	char		    dbpr_reserve[32];	/* Reserved */
}   DB_PRIVILEGES;

/*}
** Name: DB_IIRULE - Rule tuple
**
** Description:
**      This structure defines a rule tuple.  It is used by querymod in PSF,
**	RDF and QEF.
**
** History:
** 	06-mar-89 (neil)
**          Written for Terminator (rules).
**	25-oct-89 (neil)
**	    Added B1 labeling fields.
**	27-jul-92 (rickh)
**	    For FIPS CONSTRAINTS:  added rule id, statement level scope, and
**	    bitmap of triggering columns.  Removed dbr_column.
**	26-jun-93 (andre)
**	    defined DBR_CASCADED_CHECK_OPTION_RULE over dbr_flags
*/
typedef struct _DB_IIRULE
{
    DB_NAME	dbr_name;		/* Name of rule */
    DB_OWN_NAME dbr_owner;		/* Owner of rule.  If this is a 
					** table-type rule then this will be
					** the same as the table owner.
					*/
    i2		dbr_type;		/* Rule type */
#define		    DBR_T_ROW    1	/* Row level rule */
#define		    DBR_T_TIME   2	/* Time-type rule */
#define		    DBR_T_STMT   3	/* Statement level rule */
    i2		dbr_flags;		/* Rule modifier flags */
#define		    DBR_F_AFTER  0x001	/* After statement execution */
#define		    DBR_F_BEFORE 0x002	/* Before statement execution */
#define	 DBR_F_NOT_DROPPABLE	0x004	/* Not User Droppable */
#define	 DBR_F_SYSTEM_GENERATED	0x008
					/*
					** rule was created to enforce
					** CASCADED CHECK OPTION
					*/
#define	DBR_F_CASCADED_CHECK_OPTION_RULE	0x0010
					/*
					** Rule is linked to security alarm
					*/
#define DBR_F_USED_BY_ALARM		        0x00100
    DB_TAB_ID	dbr_tabid;		/* Table id of table-type rule.  Set
    					** to 0 if time-type rule.
					*/
    DB_QRY_ID	dbr_txtid;		/* Text id of saved rule statement */
    DB_TREE_ID	dbr_treeid;		/* Tree id of saved rule tree.  This
					** may be 0 if there is no tree.
					*/
    i4		dbr_statement;		/* Statement/action mask that fires
					** rule.  Set to 0 if time-type rule.
					*/
#define		    DBR_S_DELETE 0x01
#define		    DBR_S_INSERT 0x02
#define		    DBR_S_UPDATE 0x04
    DB_DBP_NAME	dbr_dbpname;		/* Procedure name to invoke */
    DB_OWN_NAME	dbr_dbpowner;		/* Procedure owner */
    DB_DATE	dbr_tm_date;		/* Date on which to fire time rule */
    DB_DATE	dbr_tm_int;		/* Repeat interval for time rules */
    DB_COLUMN_BITMAP dbr_columns;	/* Column bitmap triggering columns */
    DB_RULE_ID	dbr_ruleID;
    i2		dbr_dbpparam;		/* # procedure parameters from rule */
    i2		dbr_pad;		/* optimistic byte aligning! */
    char	dbr_free[8];		/* Get rid of me */
}   DB_IIRULE;

/*}
** Name: DB_IIRULEIDX - Rule index tuple
**
** Description:
**      This structure defines a tuple in the IIRULEIDX index on IIRULE.
**
** History:
**	2-Nov-92 (rickh)
**	    Created.
*/
typedef struct _DB_IIRULEIDX
{
    DB_RULE_ID	dbri_ruleID;
    u_i4	dbri_tidp;		/* index into IIRULE */
}   DB_IIRULEIDX;

/*}
** Name: DB_IIRULEIDX1 - Rule index tuple
**
** Description:
**      This structure defines a tuple in the IIRULEIDX1 index on IIRULE.
**
** History:
**	20-may-99 (nanpr01)
**	    Created.
*/
typedef struct _DB_IIRULEIDX1
{
    DB_NAME	dbri1_name;		/* Name of rule */
    DB_OWN_NAME dbri1_owner;		/* Owner of rule. */ 
    u_i4	dbri1_tidp;		/* index into IIRULE */
}   DB_IIRULEIDX1;

/*}
** Name: DB_IIEVENT - Tuple from the 'iievent' catalog.
**
** Description:
**	This structure defines an event tuple, contained within the 'iievent'
**	catalog.  This structure is set and used by the qrymod support
**	modules in PSF, RDF and QEF.
**
** History:
**	07-sep-89 (neil)
**	    Written for Terminator II/alerters
**	25-oct-89 (neil)
**	    Added B1 labeling fields.
**	21-jun-93 (robf)
**	    Added security label fields
*/
typedef struct _DB_IIEVENT
{
    DB_EVENT_NAME dbe_name;		/* Name of the event */
    DB_OWN_NAME	dbe_owner;		/* Owner of the event */
    DB_DATE	dbe_create;		/* Date when this event was created */
    i4		dbe_type;		/* Detailed type of this event */
# define		  DBE_T_ALERT	1	/* Alerter */
    DB_TAB_ID	dbe_uniqueid;		/* Unique numeric id used to identify
					** the event in some contexts.  This
					** is used for permission access which
					** identifies protection tuples by the
					** internal id (assumed to be unique
					** table  ids).
					*/
    DB_QRY_ID	dbe_txtid;		/* Text id of saved event text */
    char	dbe_free[8];		/* Get rid of me */
} DB_IIEVENT;

/*}
** Name: DB_IISEQUENCE - Tuple from the 'iisequence' catalog.
**
** Description:
**	This structure defines a sequence tuple, contained within the 'iisequence'
**	catalog.  This structure is set and used by the qrymod support
**	modules in PSF, RDF and QEF.
**
** History:
**	5-mar-02 (inkdo01)
**	    Written.
**	06-Mar-2003 (jenjo02)
**	    Added DBS_EXHAUSTED internal flag, set when all
**	    non-Cyclable values have been cached.
**	4-june-2008 (dougi)
**	    Upgrade integer sequence support to 64 bits.
**	4-june-2008 (dougi)
**	    Add system generated flag for identity columns. 
**	15-june-2008 (dougi)
**	    Add flags for sequenctial/unordered sequences.
**      23-June-2009 (coomi01) b122208
**          Add #defines for decimal precision and bcd string length 
**	27-Jun-2010 (kschendel) b123986
**	    Add a parser flag "decimal number seen in option" for parsing
**	    convenience.  This used to be psy_seqflag but there's no psy_cb
**	    when parsing a create table with identity column...
*/
#define DB_IISEQUENCE_DECPREC 31
#define DB_IISEQUENCE_DECLEN  DB_PREC_TO_LEN_MACRO(DB_IISEQUENCE_DECPREC)

typedef struct _DB_IISEQUENCE
{
    DB_NAME	dbs_name;		/* Name of the sequence */
    DB_OWN_NAME	dbs_owner;		/* Owner of the sequence */
    DB_DATE	dbs_create;		/* Date when this sequence was created */
    DB_DATE	dbs_modify;		/* Date when this sequence was last 
					** altered */
    DB_TAB_ID	dbs_uniqueid;		/* Unique numeric id used to identify
					** the sequence in some contexts.  This
					** is used for permission access which
					** identifies protection tuples by the
					** internal id (assumed to be unique
					** table  ids).
					*/
    DB_DT_ID	dbs_type;		/* Data type of sequence value */
    i2		dbs_length;		/* Size of sequence value */
    i4		dbs_prec;		/* If sequence value is DECIMAL, 
					** precision of value */
    /* Note : the tuple image has a decimal(31) and a 64-bit integer instance 
    ** for each of the sequence parameter values. */
    struct {				/* Start value of sequence */
	char	decval[DB_IISEQUENCE_DECLEN];		/* decimal sequence value */
	i8	intval;			/* integer sequence value */
    } dbs_start;
    struct {				/* Increment value of sequence */
	char	decval[DB_IISEQUENCE_DECLEN];		/* decimal sequence value */
	i8	intval;			/* integer sequence value */
    } dbs_incr;
# define R_RAAT
    struct {				/* Next value of sequence */
	char	decval[DB_IISEQUENCE_DECLEN];		/* decimal sequence value */
	i8	intval;			/* integer sequence value */
    } dbs_next;
    struct {				/* Minimum value of sequence */
	char	decval[DB_IISEQUENCE_DECLEN];		/* decimal sequence value */
	i8	intval;			/* integer sequence value */
    } dbs_min;
    struct {				/* Maximum value of sequence */
	char	decval[DB_IISEQUENCE_DECLEN];		/* decimal sequence value */
	i8	intval;			/* integer sequence value */
    } dbs_max;
    i4		dbs_cache;		/* Cache size */
    i4		dbs_flag;		/* Flag for various options */
#define	DBS_CYCLE	0x01		/* cycle option specified */
#define DBS_MAXVAL	0x02		/* maxvalue option specified */
#define DBS_MINVAL	0x04		/* minvalue option specified */
#define DBS_CACHE	0x08		/* cache option specified */
#define	DBS_ORDER	0x10		/* order option specified */
#define	DBS_NOCYCLE	0x20		/* nocycle option specified */
#define DBS_NOMAXVAL	0x40		/* nomaxvalue option specified */
#define DBS_NOMINVAL	0x80		/* nominvalue option specified */
#define DBS_NOCACHE	0x100		/* nocache option specified */
#define	DBS_NOORDER	0x200		/* noorder option specified */
#define	DBS_START	0x400		/* start with option specified */
#define	DBS_INCR	0x800		/* increment by option specified */
#define	DBS_RESTART	0x1000		/* restart with option specified */
#define DBS_SEQL	0x2000		/* ordered (sequential) sequence */
#define	DBS_UNORDERED	0x4000		/* unordered (random) sequence */
#define	DBS_EXHAUSTED	0x8000		/* internal flag, all values cached */
#define	DBS_SYSTEM_GENERATED 0x10000	/* sequence generated for identity
					** column */

/* The following flag is for parser convenience ONLY, and MUST be cleared
** by the time the parser is exited.
*/
#define DBS_DECSEEN	0x40000000	/* Decimal number seen in sequence
					** options while parsing */
	char	dbs_free[8];		/* Get rid of me */
} DB_IISEQUENCE;

/*
** Constant: DB_EVDATA_MAX - Maximum user text data associated with an event.
**	     This constant is documented so do not change without updating
** the documentation.
*/
#define	DB_EVDATA_MAX		256

/*}
** Name: DB_QMODE - defines query mode
**
** Description:
**      This type is used to define the query mode for the catalogs.  These
**	values must match their associate values in <psfparse.h>
**
** History:
**	22-apr-88 (seputis)
**          initial creation
**	06-mar-89 (neil)
**	    Added DB_RULE for rule trees.
**      07-sep-89 (neil)
**          Added DB_EVENT for Terminator II/alerters
**	11-oct-90 (ralph)
**	    6.3->6.5 merge:
**	    23-apr-90 (fourt)
**		Change names DB_REGISTER and DB_CRT_LINK to DB_REG_IMPORT
**		and DB_REG_LINK, to reflect changing of statements REGISTER
**		and CREATE LINK to REGISTER...AS IMPORT and REGISTER...AS LINK.
**	    23-may-90 (linda)
**		Changed names to DB_CRT_LINK, DB_REG_LINK and DB_REG_IMPORT.
**		See detailed history entry at top of file.
**	    31-may-92 (andre)
**		defined DB_PRIV_DESCR - it does not correspond to any query
**		mode - I will lobby for removing the requirement that DB_QMODE
**		values be dependent on PSF's query mode
**	27-jul-92 (rickh)
**	    Added DB_INDEX for constraint dependencies.
**	    31-july-92 (andre)
**		defined DB_QUEL_VIEW; this will be used ONLY in
**		IIPRIV.db_dep_obj_type to indicate that a view depends on any
**		SELECT privilege and not only on GRANT-compatible one
**	    15-sep-92 (andre)
**		got rid of DB_QUEL_VIEW.  IIPRIV tuples describing privileges on
**		which a QUEL view depends will contain DB_VIEW in
**		db_dep_obj_type but will NOT have DB_GRANT_COMPATIBLE_PRIV set
**		in db_prv_flags
**	    21-oct-93 (andre)
**		defined DB_SYNONYM
**	29-nov-93 (robf)
**          Added DB_ALM for security alarms.
**	5-mar-02 (inkdo01)
**	    Added DB_SEQUENCE for iisequence.
*/
typedef i2 DB_QMODE;
					    /*
					    ** privilege descriptor - can be
					    ** used in DB_IIDBDEPENDS.dbv_itype
					    */
#define			DB_PRIV_DESCR	1
#define                 DB_VIEW         17  /* MUST match PSQ_VIEW */
#define			DB_PROT		19  /* MUST match PSQ_PROT */
#define			DB_INTG		20  /* MUST match PSQ_INTEG */
#define			DB_DBP		87  /* MUST match PSQ_CREATE_PROC */
#define			DB_CRT_LINK    100  /* MUST match PSQ_0_CRT_LINK */
#define                 DB_REG_LINK    101  /* MUST match PSQ_REG_LINK */
#define                 DB_REG_IMPORT  103  /* MUST match PSQ_REG_IMPORT */
#define			DB_RULE	       110  /* MUST match PSQ_RULE */
#define			DB_EVENT	33  /* MUST match PSQ_EVENT */
#define			DB_INDEX	10  /* must match PSQ_INDEX */
#define			DB_SYNONYM     138  /* matches PSQ_CSYNONYM */
#define			DB_CONS	       149  /* must match PSQ_CONS */
#define			DB_ALM	       131  /* must match PSQ_CALARM */
#define			DB_SEQUENCE    179  /* must match PSQ_SEQUENCE */

/*}
** Name: DB_IIQRYTEXT - IIqrytext tuple
**
** Description:
**      This structure defines an IIqrytext tuple as used by qrymod in PSF
**	and in RDF.
**
** History:
**     16-may-86 (jeff)
**          written
**     21-mar-89 (sandyh)
**	    added security labeling
**	28-dec-89 (andre)
**	    defined DBQ_DBA_VIEW and DBQ_VGRANT_OK.
**	    Please note: these should never appear in IIQRYTEXT tuples.  It will
**	    allow PSF to communicate to RDF that a view being created is owned
**	    by DBA.  It is defined here so that in the future there is no
**	    conflict with other masks used with rdf_cb.rdf_rb.rdr_status.
**     21-may-90 (teresa)
**	    removed DBQ_DBA_VIEW and DBQ_VGRANT_OK definitions.  This info is
**	    now communicated via typedef RDR_INSTR.
*/
typedef struct _DB_IIQRYTEXT
{
    DB_QRY_ID       dbq_txtid;          /* The query text id. 
					** same as creation date.
					*/
    i4		    dbq_mode;		/* The query mode - see definition of
                                        ** DB_QMODE for values which can be
                                        ** contained in the variable
                                        ** FIXME - use DB_QMODE instead
                                        ** of i4 when catalogs can be changed */
    i4		    dbq_seq;		/* Sequence number of the segment */
    i4		    dbq_status;		/* interesting bits of info */
#define	DBQ_WCHECK	1		/* object created 'with check option'.
					** generally implies dbq_mode is view.
					*/
#define DBQ_SQL		2		/* object created as SQL */
    DB_TEXT_STRING  dbq_text;		/* The text string */
    char	    dbq_pad[239];	/* DB_TEXT_STRING is defined with a
					** dummy db_t_text field with only
					** one character.  This takes up
					** the rest of the field.  dbq_text
					** overflows into here.
					*/
}   DB_IIQRYTEXT;

/*}
** Name: DB_IIDBDEPENDS - IIdbdepends tuple
**
** Description:
**      This structure defines a tuple in the IIdbdepends relation, as used by
**	QEF. Some objects (the dependent object) require the 
**	existence of other objects (the independent object) for continued
**	meaning. For example, indexes require base tables. Views require
**	base tables.  Some of the dependencies are implicit in the database,
**	the others go into this table.  The base tables for views are listed
**	here. The base tables used in QUEL permits other than the table
**	getting the permit are listed here.  Integrities and SQL permits and
**	QUEL permits where the only table referenced is the one getting
**	the permit are handled implicitly by the database manager.
**
** History:
**	30-jun-87 (daved)
**	    written
**	21-mar-89 (sandyh)
**	    added security labeling
**	31-may-92 (andre)
**	    added dbv_i_qid for GRANT/REVOKE project; changed dbv_qid to i2
**	    since the permit number is an i2
**	15-sep-92 (andre)
**	    the goal was noble but the result was not so good: having dbv_qid
**	    as i2 causes allignment problems when reading IIXDBDEPENDS -
**	    dbv_i_qid and dbv_qid will become i4's
*/
typedef struct _DB_IIDBDEPENDS
{
    DB_TAB_ID       dbv_indep;          /* Table id independent object */
    i4		    dbv_itype;		/* type of independent object.
					** currently always a table or view.
					** can also be a privilege descriptor
					*/
#define DB_TABLE    0			/* DB_VIEW is defined elsewhere */
    i4		    dbv_i_qid;		/*
					** secondary id for independent object
					*/
    i4		    dbv_qid;		/* secondary id for dependent object */
    DB_TAB_ID	    dbv_dep;		/* Table id of dependent object */
    i4		    dbv_dtype;		/* type of dependent object. currently
					** views and QUEL permits.
					*/
}   DB_IIDBDEPENDS;

/*}
** Name: DB_IIXDBDEPENDS - IIdbdepends index
**
** Description:
**	This index is used to find the tuples that reference a
**  dependent object in the iidbdepends catalog.  The base
**  catalog is keyed on the independent object id.  If a
**  dependent object is destroyed, we need to delete tuples
**  from the base catalog that reference the dependent object.
**
** History:
**	30-jun-87 (daved)
**          written
**	21-mar-89 (sandyh)
**	    added security labeling
**	03-jun-92 (andre)
**	    changed dbvx_qid to i2 because DB_IIDBDEPENDS.dbv_qid is now an i2.
**	15-sep-92 (andre)
**	    changed DB_IIDBDEPENDS.dbv_qid back to i4 - dbvx_qid will go back to
**	    being an i4
*/
typedef struct _DB_IIXDBDEPENDS
{
    DB_TAB_ID	    dbvx_dep;		/* Table id of dependent object */
    i4		    dbvx_dtype;		/* type of dependent object. currently
					** views and QUEL permits.
					*/
    i4		    dbvx_qid;		/* secondary id for dependent object.*/
    u_i4	    dbvx_tidp;		/* index into iidbdepends */
}   DB_IIXDBDEPENDS;

/*}
** Name: DB_IIVIEWS - IIviews tuple
**
** Description:
**      This structure defines a tuple in the IIviews relation, as used by
**	qrymod in PSF and by RDF.
**
** History:
**     16-may-86 (jeff)
**          written
*/
typedef struct _DB_IIVIEWS
{
    DB_TAB_ID       dbv_base;           /* Table id of base table */
    DB_TAB_ID	    dbv_view;		/* Table id of overlying view */
    DB_QRY_ID	    dbv_qryid;		/* Query id of text in iiqrytext */
}   DB_IIVIEWS;

/*}
** Name: DB_IITREE - IItree tuple
**
** Description:
**      This structure defines a tuple for the iitree relation, as used by
**	RDF and QEF.
**
** History:
**      15-jul-86 (jeff)
**          written
**      21-mar-89 (sandyh)
**	    added security labeling
**      30-May-89 (teg)
**          changed defintion or dbt_tree for unix portability.  Modified
**          DB_TEXT_STRING type struct (which was incorrectly aligned on 2
**          byte boundary) to the two separate components that comprise a
**          varchar (the u_i2 of count and the char array.)
*/
#define     DB_TREELEN      1024    /* number of chars in iitree */

typedef struct _DB_IITREE
{
    DB_TAB_ID       dbt_tabid;          /* Table id this tree is linked with */
    DB_TREE_ID	    dbt_trid;		/* Tree id for this tree */
    i2		    dbt_trseq;		/* Sequence number of tree segment */
    DB_QMODE	    dbt_trmode;		/* Query mode of this tree */
    i2		    dbt_version;	/* Query tree version number */
    u_i2	    dbt_l_tree;         /* length of text string for tree */
    char	    dbt_tree[DB_TREELEN]; /* actual tree */
}   DB_IITREE;

/*}
** Partitioned table catalogs.
**
** Tables can be partitioned in multiple dimensions.  Each dimension
** is a "partition scheme" or "distribution scheme" which describes
** how to decide what rows go where, and how many partitions to send
** rows to in this dimension.  A distribution scheme may be value
** dependent, in which case we need catalog entries to describe the
** columns to use, and what value tests to apply.  The partitions
** described by a distribution scheme (dimension) are named so that
** partition maintenance operations can be applied (split a range,
** add a partition, that sort of thing.)
** 
** Intermediate levels of the partitioning don't really exist as tables.
** At the bottom level, where data is actually stored, physical
** partitions are stored as tables, so we need a cross-reference
** catalog that translates partition numbers into table ID numbers.
**
** The partitioned table at the very top is called the master table.
** (Base table being a term already used for views and indexes.)
**
** Please note the difference between a "partition sequence" and a
** "partition number".  The partition sequence is an index within that
** partitioning dimension.  The partition number is an absolute partition
** number with all the dimensions multiplied out.  Example:  suppose
** we have a 2-way partition, subpartitioned 3 ways:
**
**				p3
**			      /
**			  p1  - p4
**			/     \
**		       /	p5
**		 master
**		       \	p3
**			\     /
**			  p2  - p4 ((*))
**			      \
**				p5
**
** the partition marked ((*)) has sequence 2 (one-origin) but number 4
** (zero-origin).
**
** Sequences are all one-origined following the existing convention with
** column numbers, column key sequences, etc.  Physical partition numbers
** are zero-origined to make extending a tid4 into a tid8 just a matter
** of zero filling.
**
** All catalogs except iipartition "belong" to RDF.  The functions that
** comprehend the contents of iidistscheme, etc are RDF functions.
** iipartition is considered a purely physical catalog, and is maintained
** inside DMF, not RDF.
*/

/*}
** Name: DB_IIDISTSCHEME - iidistscheme tuple.
**
** This structure maps the iidistscheme catalog.  This catalog is used
** for partitioned tables, and contains one row per partitioning
** dimension.  Non-partitioned tables have no entries in this catalog.
**
** The iidistscheme catalog records the distribution rule being used
** for the level (hash, range, list, automatic).  As a convenience,
** it also contains the number of columns and partitions in this
** dimension of the overall partitioning scheme.
**
** Note that "number of partitions" means number of partitions described
** by this dimension, not number of partitions total that exist at this
** level.  The latter is the former times the number of partitions in
** higher dimensions.
**
** History:
**	19-Dec-2003 (schka24)
**	    Create for partitioned tables.
**	2-Jan-2004 (schka24)
**	    Define max number of dimensions allowed.
**	    Move rule codes to iicommon.h.
**
*/
typedef struct _DB_IIDISTSCHEME {
	DB_TAB_ID	master_tabid;	/* Master table ID */
	i2		levelno;	/* Level (dimension) of this scheme.
					** levels are one-origin, numbered
					** from the top down
					*/
	i2		ncols;		/* Number of columns controlling
					** row distribution at this level.
					** Zero for automatic distributions.
					*/
	u_i2		nparts;		/* Number of partitions described by
					** this dimension of the scheme.
					*/
	i2		distrule;	/* DBDS_RULE_xxx Distribution rule */
					/* The rule codes are in iicommon */
} DB_IIDISTSCHEME;

/* Define the maximum number of levels (dimensions) we allow.
** This is mostly to permit the use of static arrays in dealing with
** the partition catalogs.  Actually, a practical limit is 2, but
** if someone wants to try a 10-way partitioning, let 'em...
*/

#define DBDS_MAX_LEVELS		10	/* Maximum levelno */

/*}
** Name: DB_IIDISTCOL
**
** The iidistcol catalog has one row per column per partitioning
** dimension.  It describes the columns that participate in the
** distribution scheme at this dimension.  Nonpartitioned tables
** have no iidistcol entries.  A partitioning dimension will have
** no entries if the scheme is AUTOMATIC.
**
** History:
**	19-Dec-2003 (schka24)
**	    New for partitioned tables.
*/

typedef struct _DB_IIDISTCOL {
	DB_TAB_ID	master_tabid;	/* Master table ID */
	i2		levelno;	/* Level (dimension) of this scheme.
					** levels are one-origin, numbered
					** from the top down
					*/
	i2		colseq;		/* Column sequence for this dimension */
	i2		attid;		/* Column attribute number */
} DB_IIDISTCOL;

/*}
** Name: DB_IIDISTVAL
**
** The iidistval catalog describes how column values direct rows into
** partitions, at some dimension.  Some distribution rules (LIST) allow
** multiple values directing rows into the same partition, so we need
** a "value sequence".  Values can be multi-column values, so we need
** a column sequence too.  We need an operator saying what sort of
** test to apply, and a value to test against.  The value is stored as
** an uninterpreted nullable VARCHAR rather than an int, date, or
** whatever.
**
** History:
**	19-Dec-2003 (schka24)
**	    Create for partitioned tables.
*/

/* Define max length so as to 4-byte-align the row.  (Remember that
** there is also 2 varchar length bytes and a null indicator, and the
** value is occurring right after an aligned i1.)
** Alignment is not necessary (I think!) but there's no harm in it.
*/

#define DB_MAX_DIST_VAL_LENGTH 1500

typedef struct _DB_IIDISTVAL {
	DB_TAB_ID	master_tabid;	/* Master table ID */
	i2		levelno;	/* Level (dimension) of this scheme.
					** levels are one-origin, numbered
					** from the top down
					*/
	i2		valseq;		/* Value sequence number */
	i2		colseq;		/* Column sequence for this dimension */
	u_i2		partseq;	/* Sequence number of partition to
					** send rows that pass the test to
					*/
	i1		oper;		/* Operator to apply (see iicommon) */
 	char		value[DB_MAX_DIST_VAL_LENGTH + 3];  /* Nullable value */
} DB_IIDISTVAL;

/*}
** Name: DB_IIPARTNAME
**
** The iipartname catalog stores partition names.  Each partitioning
** dimension describes how to further subdivide rows.  The partitions
** defined for each dimension are named (by the user or by the system),
** to allow partition maintenance operations.  Partition names are
** unique for a given master table;  they may be nonunique across
** different master tables.
**
** History:
**	19-Dec-2003 (schka24)
**	    New for partitioned tables.
*/

typedef struct _DB_IIPARTNAME {
	DB_TAB_ID	master_tabid;	/* Master table ID */
	i2		levelno;	/* Level (dimension) of this scheme.
					** levels are one-origin, numbered
					** from the top down
					*/
	u_i2		partseq;	/* Sequence number of partition
					** within this dimension.
					*/
	DB_NAME		partname;	/* The partition name */
} DB_IIPARTNAME;

/*
** Types and names used with cursors.
*/

/*
** Introductory character used to introduce special sequences, like cursor
** id.
*/

#ifdef    EBCDIC

#define                 DB_INTRO_CHAR   0xebL

#else

#define                 DB_INTRO_CHAR	0200L

#endif

/*
** Characters following introductory character, and their meanings.
*/

/* "Define cursor"; what follows is a cursor id */
#define                 DB_DEF_CURS     1

/* "Direct update" */
#define			DB_DIR_UPD	2

/* "Deferred update" */
#define			DB_DFR_UPD	3

/* "Retrieve cursor"; cursor id follows */
#define			DB_RET_CURS	4

/* "Replace cursor"; cursor id follows */
#define			DB_REP_CURS	5

/* "Delete cursor"; cursor id follows */
#define			DB_DEL_CURS	6

/* "Close cursor"; cursor id follows */
#define			DB_CLS_CURS	7

/* "Define repeat cursor"; cursor id follows */
#define			DB_RPT_CURS	8

/* "Execute cursor"; cursor id follows */
#define			DB_EXE_CURS	9

/* Range variable follows: variable number */
#define			DB_RNG_VAR	10

/* Column number follows */
#define			DB_COL_NUM	11

/* Table name should be used in place of this symbol */
#define			DB_TBL_NM	12

/* Result column number follows */
#define			DB_RES_COL	13

/* Table id follows */
#define			DB_TBL_ID	14

/*
** Cursor Name
** There are assumptions in cursor related code that
** DB_CURSOR_MAXNAME = DB_TAB_MAXNAME = DB_DBP_MAXNAME (e.g. psqrecr.c)
*/
#define    DB_CURSOR_MAXNAME    DB_DBP_MAXNAME /* cursor name */


/*}
** Name: DB_CURSOR_ID - A cursor id
**
** Description:
**      A cursor id is a timestamp assigned to a cursor by a pre-processor.
**	It also contains the cursor name, for error reporting purposes.
**
** DB_CURSOR_ID is used to build a qso_name
** There is code that assumes that a DB_CURSOR_ID is (i4+i4+NAME)
** For example, qso_hash assumes that the name be right after 2*sizeof(i4)
**
** Code that builds a qso_name will copy sizeof(DB_CURSOR_ID) into qso_name.
** If there are any compiler added PAD bytes they must be EXPLICITYLY defined
** AFTER the name, and they must be initialized before copying into a qso_name.
**
** History:
**     18-may-86 (jeff)
**          written
*/
typedef struct _DB_CURSOR_ID
{
    i4              db_cursor_id[2];		/* A timestamp is 2 i4s */
    char	    db_cur_name[DB_CURSOR_MAXNAME]; /* Name of the cursor */
}   DB_CURSOR_ID;

/*
** DB_CURSOR_ID: name MUST be right after db_cursor_id
** DB_CURSOR_ID: compiler added pad bytes MUST be explicitly defined 
** pad bytes added MUST be init whenever db_cursor_id is init.
** If you change this, you must change all the code that is building 
** qso_names
*/
#define DB_CURSOR_ID_OFFSET  ( CL_OFFSETOF( DB_CURSOR_ID, db_cursor_id ) )
#define DB_CUR_NAME_OFFSET  ( CL_OFFSETOF( DB_CURSOR_ID, db_cur_name ) )


/*}
** Name: DB_STAT_VERSION - statistics version as stored in iistatistics
**			   relation 
**
** Description:
**      This structure defines the version id of a set of statistics on a
**      column.
**
** History:
**     18-jan-93 (rganski)
**          Written for Character Histogram Enhancements project.
**	12-Nov-2009 (kschendel) SIR 122882
**	    Stat/histo versions map to iidbcapabilities, not db
**	    compat versions.  Fix comment.
*/
#define		DB_STAT_VERSION_LENGTH	8

typedef struct _DB_STAT_VERSION
{
    char	db_stat_version[DB_STAT_VERSION_LENGTH];
}   DB_STAT_VERSION;

#define		DB_NO_STAT_VERSION	"        "

/* definitions for STAR compatibilty for reading histogram tuples correctly
** from previous versions.
** Some of these definitions are not used but are provided for completeness. 
** These values track and are equal to the STANDARD_CATALOG_LEVEL
** string in iidbcapabilities, which is defined in dudbms.qsh
** as DU_DB6_CUR_STDCAT_LEVEL.
*/
# define        DB_STATVERS_0DBV60		"00605   "
# define        DB_STATVERS_1DBV61A		"00601a  "
# define        DB_STATVERS_2DBV62		"00602   "
# define        DB_STATVERS_3DBV63		"00603   "
# define        DB_STATVERS_4DBV70		"00700   "
# define        DB_STATVERS_5DBV634		"006034  "
# define        DB_STATVERS_6DBV80		"00800   "
# define        DB_STATVERS_6DBV85		"00850   "
# define        DB_STATVERS_6DBV86		"00860   "
# define        DB_STATVERS_6DBV865		"00900   "
# define        DB_STATVERS_6DBV902		"00902   "
# define        DB_STATVERS_6DBV904		"00904   "
# define        DB_STATVERS_6DBV910		"00910   "
# define        DB_STATVERS_6DBV920		"00920   "
# define        DB_STATVERS_6DBV930		"00930   "
# define        DB_STATVERS_6DBV1000		"01000   "
# define        DB_STATVERS_6DBV1010		"01010   "



/*}
** Name: DB_1ZOPTSTAT - physical definition of tuple in zopt1stat relation
**
** Description:
**      The zopt1stat catalog contains database 
**      statistics that are collected by the optimizedb 
**      program.  These statistics include such things
**      as number of unique values in a table column and number of cells
**      in the histogram.  This information is used, if available, by the
**      query optimizer in deciding the best query execution plan.  
**
**      System Catalog definition
**
**      hash on "stabbase,stabindex" attribute of zopt1stat table
**
**      attribute type        attribute name     component of this structure
**      i4                    stabbase           db_tabid.db_tab_base
**      i4                    stabindex          db_tabid.db_tab_index
**      f4                    snunique           db_nunique
**      f4                    sreptfact          db_reptfact
**      f4                    snull              db_nulls
**      i2                    satno              db_atno
**      i2                    snumcells          db_numcells
**      i2                    sdomain            db_domain
**      i1                    scomplete          db_complete
**      i1                    sunique            db_unique
**      c12                   sdate              db_date
**	c5		      sversion		 db_version
**	i2		      shistlength	 db_histlength
**
** History:
**     25-jun-86 (seputis)
**          initial creation
**     21-mar-89 (sandyh)
**	    added security labeling
**     30-Jun-89 (teg)
**          changed  db_atno from DB_ATT_ID (struct containging an i2) to
**          DB_ATTNUM (typedef'd to i2) to remove alignment issues for porting
**          to non-VMS platforms.
**     09-nov-92 (rganski)
**	    Added 2 new fields: db_version and db_histlength, which correspond
**	    to new iistatistics columns sversion and shistlength. This is for
**	    the Character Histogram Enhancements project.
**     18-jan-93 (rganski)
**     	    Changed type of db_version to DB_STAT_VERSION
**     	    (Character Histogram Enhancements project).
**     	    
[@history_line@]...
*/
typedef struct _DB_1ZOPTSTAT
{
    DB_TAB_ID       db_tabid;           /* table id uniquely identifying the
                                        ** table upon which this histogram is
                                        ** based.
                                        */
    f4              db_nunique;         /* number of unique values in the
                                        ** in the column
                                        */
    f4              db_reptfact;        /* repitition factor - the inverse
                                        ** of the number of unique values
                                        ** i.e. number of rows / number
                                        ** of unique values
                                        */
    f4              db_nulls;           /* percentage (fraction of 1.0) of
                                        ** relation which contains NULL for
                                        ** this attribute
                                        */
    DB_ATTNUM	    db_atno;            /* attribute number of the column
                                        ** within the table to which the
                                        ** statistics apply
                                        */
    i2              db_numcells;        /* the number of cells in the
                                        ** histogram
                                        */
    i2              db_domain;          /* - domain of this attribute 
                                        ** - gives meaning to
                                        ** the db_complete domain flag i.e. the
                                        ** db_domains must be equal and the both
                                        ** attributes of a join complete, if
                                        ** the resultant attribute is to be
                                        ** complete
                                        */
    i1              db_complete;        /* TRUE - if all values which currently
                                        ** exist in the database for this
                                        ** domain also exists in this attribute
                                        */
    i1              db_unique;          /* TRUE if all values for that
                                        ** column are unique
                                        */
    char            db_date[12];	/* ADF date that this histogram was
                                        ** info was taken
                                        */
    DB_STAT_VERSION db_version;		/* Indicates in which version of the
					** DBMS these statistics were gathered
					*/
    i2		    db_histlength;	/* Length of histogram boundary values
					*/
}   DB_1ZOPTSTAT;

/*}
** Name: DB_HELEMENT - attribute type used to define histogram tuple
**
** Description:
**	In order to define a histogram tuple, a number of i4's are used since
**      this attribute type allows 'binary' data to be stored i.e. char cannot
**      be used since there are restrictions on the data that can
**      be stored in a char data type.
**
** History:
**      26-nov-86 (seputis)
**          initial creation
[@history_template@]...
*/
typedef i4          DB_HELEMENT;
#define             DB_NI4A              57
/* number of DB_HELEMENT's of data in the iihistogram relation */


/*}
** Name: DB_MAX_HIST_LENGTH - maximum length of a histogram boundary value
**
** Description:
**	This defines the maximum length of a single histogram boundary value
**	(also known as an "interval").
**
** History:
**      18-jan-93 (rganski)
**          Initial creation.
[@history_template@]...
*/
#define             DB_MAX_HIST_LENGTH              1950

/*}
** Name: DB_DEFAULT_CHARNUNIQUE
**	 DB_DEFAULT_CHARDENSITY
**	 			- default character set statistics
**
** Description:
**      These values define the default values for per-position character set
**      statistics used by the optimizer. If there are no character set
**      statistics in the histogram for a character-type column, then these are
**      used in each character position.
**
** History:
**     18-jan-93 (rganski)
**          Written for Character Histogram Enhancements project.
*/
#define		DB_DEFAULT_CHARNUNIQUE	11
#define		DB_DEFAULT_CHARDENSITY	1.0

/*}
** Name: DB_OPTDATA - defines data portion of the DB_2ZOPTSTAT structure
**
** Description:
**	Structure used to define data portion of the zopt2stat relation.
**
** History:
**      25-jun-86 (seputis)
**          initial creation
[@history_line@]...
*/
#define		    DB_OPTLENGTH	(DB_NI4A * sizeof(DB_HELEMENT))
typedef char DB_OPTDATA[DB_OPTLENGTH];

/*}
** Name: DB_2ZOPTSTAT - physical definition of tuple in zopt2stat relation
**
** Description:
**      The zopt2stat catalog contains database histograms that are collected 
**      by the optimizedb program.  This information is used, if available, 
**      by the query optimizer in deciding the best query execution plan.
**
**      Note that the system catalog definition of the histogram information
**      is in terms of i4's since character strings have semantics which
**      will terminate transmission of an attribute if an imbedded 0 is
**      found
**
**      System Catalog definition
**
**      hash on "htabbase,htabindex" attributes of zopt2stat table
**
**      attribute type        attribute name     component of this structure
**      i4                    htabbase           db_tabid.db_tab_base
**      i4                    htabindex          db_tabid.db_tab_index
**      i2                    hatno              db_atno
**      i2                    hsequence          db_sequence
**      i4                    h0                 db_optdata[0]...[3]
**      i4                    h1                 db_optdata[4]...[7]
**      ...
**      i4                    h56                db_optdata[224]...[227]
** History:
**     25-jun-86 (seputis)
**          initial creation
**     21-mar-89 (sandyh)
**	    added security labeling
**     30-Jun-89 (teg)
**          changed  db_atno from DB_ATT_ID (struct containging an i2) to
**          DB_ATTNUM (typedef'd to i2) to remove alignment issues for porting
**          to non-VMS platforms.
[@history_line@]...
*/
typedef struct _DB_2ZOPTSTAT
{
    DB_TAB_ID       db_tabid;           /* table id uniquely identifying the
                                        ** table upon which this histogram is
                                        ** based.
                                        */
    DB_ATTNUM	    db_atno;            /* attribute number of the column
                                        ** within the table to which the
                                        ** statistics apply
                                        */
    i2              db_sequence;        /* the sequence number for the
                                        ** histogram; there may be several
                                        ** rows in this table for each
                                        ** histogram
                                        */
    DB_OPTDATA      db_optdata;         /* histogram cells in binary pairs of
                                        ** ranges and counts.  These are coded
                                        ** into this set of attributes
                                        */
}   DB_2ZOPTSTAT;

/*}
** Name: DB_COLCOMPARE - physical definition of tuple in iicolcompare relation
**
** Description:
**      The iicolcompare catalog contains comparison statistics between 
**	pairs of columns in the same or different tables. If the columns are
**	in the same table, the statistics are the proportions of rows in the
**	table in which one column is less than, equal to or greater than the
**	other. If the columns are in separate tables, the proportions are of
**	the rows in the cross product join of the tables in which one column 
**	is less than, equal to or greater than the other.
**
** History:
**     5-jan-06 (dougi)
**          Created.
[@history_line@]...
*/
typedef struct _DB_COLCOMPARE
{
    DB_TAB_ID	cc_tabid1;		/* ID of 1st column(s) table */
    DB_TAB_ID	cc_tabid2;		/* ID of 2nd column(s) table */
    i4		cc_count;		/* count of columns */
    DB_ATTNUM	cc_col1[4];		/* array of 1st comparand columns */
    DB_ATTNUM	cc_col2[4];		/* array of 2nd comparand columns */
    f4		cc_ltcount;		/* proportion of rows: col1 < col2 */
    f4		cc_eqcount;		/* proportion of rows: col1 = col2 */
    f4		cc_gtcount;		/* proportion of rows: col1 > col2 */
} DB_COLCOMPARE;

/*}
** Name: DB_TID - The structure of a tuple identifier.
**
** Description:
**      This structure defines a tuple identifier and it associated constants.
**
** History:
**     6-aug-86 (seputis)
**          Created from DM_TID
**     4-feb-90 (seputis)
**          changed to reflect change in DM_TID
*/
typedef union _DB_TID
{
    struct
    {
#ifndef TID_SWAP
        u_i4        tid_line:9;         /* Index into offset table on page. */
#define                 DB_TIDEOF      511
        u_i4        tid_page:23;        /* The page number. */
#define			DB_MAX_PAGENO	0x7fffff	/* 2**23 - 1 */
#else
        u_i4        tid_page:23;        /* The page number. */
#define			DB_MAX_PAGENO	0x7fffff	/* 2**23 - 1 */
        u_i4        tid_line:9;         /* Index into offset table on page. */
#define                 DB_TIDEOF      511
#endif
    }   tid_tid;
    u_i4            tid_i4;             /* Above as an i4. */
} DB_TID;

/*
**  Constants associated with dmf tids (tuple identifiers)
*/
#define                 DB_IMTID        0
/* implicit tid attribute number - dmf attribute which is not physically
** stored in the table... it is the "disk address" of the tuple
*/
#define                 DB_TID_TYPE     DB_INT_TYPE
/* datatype used for implicit and explicit tids - see DMF document for
** description of tid contents
*/
#define                 DB_TID_LENGTH   4
/* length of datatype used for implicit and explicit tids */

/*}
** Name: DB_TID8 - The structure of a big tuple identifier.
**
** Description:
**      This structure defines a big tuple identifier, i.e., 
**	one which prefixes DM_TID with partitioning 
**	and extension information.
**
**	The intent is to map the tid so that the fields look like
**	an i8 constructed as:
**	partno<<48 + page_ext<<40 + row_ext<<32 + tid4
**
**	Unfortunately it doesn't seem to be possible to do that in
**	a portable manner without the arch specific TID_SWAP definition.
**
** History:
**	19-Dec-2003 (jenjo02)
**	    Created for Partitioned Table project,
**	    identical to DM_TID8.
**	15-Jul-2004 (schka24)
**	    Revise so that tid8 is just an extension of tid4 even on
**	    little-endian machines.
*/
typedef union _DB_TID8
{
#ifdef TID_SWAP
    /* The big-endian flavor, written in normal left to right */
    struct
    {
	u_i4	    tid_partno:16;	/* Partition number */
	u_i4	    tid_page_ext:8;	/* Page extension (not used) */
	u_i4	    tid_row_ext:8;	/* Row extension (not used) */
	u_i4	    tid_page:23;	/* The page number. */
	u_i4        tid_line:9;         /* Index into offset table on page. */
    }	tid;
    struct				/* Above as 2 i4's: */
    {
	u_i4	    tpf;		/* TID prefix */
	u_i4	    tid;		/* The TID only */
    }   tid_i4;
#else
    /* The little-endian flavor, members go right to left */
    struct
    {
	u_i4        tid_line:9;         /* Index into offset table on page. */
	u_i4	    tid_page:23;	/* The page number. */
	u_i4	    tid_row_ext:8;	/* Row extension (not used) */
	u_i4	    tid_page_ext:8;	/* Page extension (not used) */
	u_i4	    tid_partno:16;	/* Partition number */
    }	tid;
    struct				/* Above as 2 i4's: */
    {
	u_i4	    tid;		/* The TID only */
	u_i4	    tpf;		/* TID prefix */
    }   tid_i4;
#endif
} DB_TID8;

/*
**  Constants associated with dmf tid8s (tuple identifiers)
*/
#define                 DB_TID8_TYPE    DB_INT_TYPE
#define                 DB_TID8_LENGTH  8


/*
**  Query-specific names and constants.
*/

/*
**  The followoing constants are for use with the bit maps stored in the
**  iiprotect and iisecalarm catalog, for telling what operations are 
**  permitted on a column.
*/

#define                 DB_RETRIEVE	0x01L	/* Retrieve bit */
#define			DB_RETP		   0	/* Bit position of retrieve */
#define			DB_REPLACE	0x02L	/* Replace bit */
#define			DB_REPP		   1	/* Bit position of replace */
#define			DB_DELETE	0x04L	/* Delete bit */
#define			DB_DELP		   2	/* Bit position of delete */
#define			DB_APPEND	0x08L	/* Append bit */
#define			DB_APPP		   3	/* Bit position of append */
#define			DB_TEST		0x10L	/* Qual. test bit */
#define			DB_TESTP	   4	/* Bit position of qual test */
#define			DB_AGGREGATE	0x20L	/* Aggregate bit */
#define			DB_AGGP		   5	/* Bit position of aggregate */
#define			DB_EXECUTE	0x40L	/* Execute bit (dbproc only) */
#define			DB_EXECP	   6	/* Bit pos. of execute */
#define                 DB_ALARM        0x80L   /* Alarm bit (C2/B1 only) */
#define			DB_ALAP	           7	/* Bit pos. of alarm */
#define			DB_EVREGISTER 0x0100L	/* REGISTER EVENT permission */
#define			DB_EVRGP	   8	/* Bit pos of REGISTER permit */
#define			DB_EVRAISE    0x0200L	/* RAISE EVENT permission */
#define			DB_EVRAP	   9	/* Bit pos of RAISE permit */
#define                 DB_ALLOCATE   0x0400L   /* ALLOCATE permission */
#define                 DB_ALLOP          10    /* Bit pos of ALLOCATE permit */
#define                 DB_SCAN       0x0800L   /* SCAN permission */
#define                 DB_SCANP          11    /* Bit pos of SCAN permit */
#define                 DB_LOCKLIMIT  0x1000L   /* LOCK_LIMIT permission */
#define                 DB_LOCKP          12    /* Bit pos of LOCKLIM permit */
#define			DB_GRANT_OPTION 0x2000L /* permit WITH GRANT OPTION */
#define			DB_GROPTP	  13	/* bit pos of WGO indicator */
#define			DB_REFERENCES	0x4000L	/* REFERENCES permission */
#define			DB_REFP		  14	/* bit position of above */
#define			DB_CAN_ACCESS 0x8000L	/* object can be accessed by grantee */
#define			DB_CAN_ACCESSP	  15	/* bit pos. of above */
#define                 DB_NEXT       0x10000L  /* NEXT permission */
#define                 DB_NEXTP          16    /* Bit pos of NEXT permit */

#define		        DB_DISCONNECT    0x2000000L /* DISCONNECT */
#define                 DB_DISCONNECTP  25  	  /* bit pos. of above */

#define		        DB_CONNECT    0x4000000L /* CONNECT */
#define                 DB_CONNECTP  26  	  /* bit pos. of above */

#define                 DB_COPY_INTO  0x8000000L  /* COPY_INTO priv */
#define                 DB_COPY_INTOP  27  	  /* bit pos. of above */

#define                 DB_COPY_FROM  0x10000000L  /* COPY_FROM priv */
#define                 DB_COPY_FROMP  28  	   /* bit pos. of above */

#define                 DB_ALMFAILURE 0x20000000L /* Alarm on failure bit */
#define			DB_ALPFAILURE	  29	  /* Bit pos. of alarm/failure */
#define                 DB_ALMSUCCESS 0x40000000L /* Alarm on success bit */
#define			DB_ALPSUCCESS	  30	  /* Bit pos. of above */

#define			DB_NEGATE     0x80000000L /* NOprivilege specified
						  ** (internal use only)   */

/*
**  These constants are used by the various Ingres applications to identify
**  themselves to the DBMS.  This is a rather screwy way to do this, but is
**  necessary for temporarily getting things going.
**
**  All constants begin with the prefix DBA_ to indicate that they are DataBase
**  Applications.
*/

#define                 DBA_MIN_APPLICATION 0x1
#define			DBA_CREATEDB	    0x1
#define			DBA_SYSMOD	    0x2
#define			DBA_DESTROYDB	    0x3
#define			DBA_FINDDBS	    0x4
/* 
** DBA_VERIFYDB is now obsolete. Don't use it - and don't delete it
** (it's necessary to prevent old versions of optimizedb from connecting)
*/
#define			DBA_VERIFYDB	    0x5
#define			DBA_CONVERT	    0x6
#define			DBA_IIFE	    0x07
#define			DBA_NORMAL_VFDB	    0x8
#define			DBA_PRETEND_VFDB    0x9
#define			DBA_FORCE_VFDB	    0xA
#define                 DBA_PURGE_VFDB      0x0B
#define                 DBA_CREATE_DBDBF1   0x0C
#define                 DBA_OPTIMIZEDB	    0x0D
#define			DBA_W4GLRUN_LIC_CHK 0x0E
#define			DBA_W4GLDEV_LIC_CHK 0x0F
#define			DBA_W4GLTR_LIC_CHK  0x10
#define			DBA_MAX_APPLICATION 0x10


/*}
** Name: DB_MESSAGE - DB procedure message descriptor
**
** Description:
**      This structure describes the format of user-defined messages
**	in DB procedures.  The message text is optional, but the
**	message number is assumed to always have a value.  A value
**	of zero will be assumed if the message number is omitted
**	in the user's MESSAGE statement.
**
** History:
**      10-may-88 (puree)
**          created for DB procedures
**	27-feb-91 (ralph)
**	    Add DB_MSGUSER and DB_MSGLOG constants to represent the
**	    DESTINATION clause of the MESSAGE and RAISE ERROR statements.
**	    NOTE: I don't think this DB_MESSAGE structure is used anywhere
**		  in the server.  If this structure is removed, the destination
**		  constants must remain defined.
**	15-jul-93 (robf)
**	   Added DB_MSGAUDIT constants to represent writing to the 
**	   system security audit log.
*/
typedef struct _DB_MESSAGE
{
    i4	msg_number;	/* user-assigned message number */
    char	*msg_text;	/* ptr to the text of the message */
    i4		msg_length;	/* length of the message text above
				** 0 if there is no text */
#define		DB_MSGUSER	0x01	/* DESTINATION = (SESSION)	*/
#define		DB_MSGLOG	0x02	/* DESTINATION = (ERROR_LOG)	*/
#define	        DB_MSGAUDIT     0x04	/* DESTINATION = (AUDIT_LOG)    */
} DB_MESSAGE;

/*
**
** A temporary definition to get terminator IFDEFS compiled in FE code.
** 
*/
#define	    TERMINATOR	"TRUE"

/*
** DB_JNTYPE appears in QEN join nodes and in PSF query tree nodes and tells
** whether a join is an inner, a left, a right, or a full "outer" join.  The
** term "outer join" is an unfortunate one, since the "outer" relation of an
** "outer" join need not really be the outer.  For this reason, Eric has avoided
** the word 'outer' in the following defines.
**
** The following five values are the only legal values for the DB_JNTYPE
** datatype, and a DB_JNTYPE field or variable may only hold one of the
** following five values.  However the values have been chosen so that if a
** DB_JNTYPE field holds DB_FULL_JOIN, then it will evaluate to TRUE if it is
** bitwise OR'd with either DB_LEFT_JOIN or DB_RIGHT_JOIN.  This has been done
** because a "full" join is really a combination of a right and a left join.
** The ANTI types were added as a statement of intent.
**
**  17-aug-89 (andre)
**	Copied from Eric's document.
**	19-sep-05 (inkdo01)
**	    Added anti joins for EXCEPT and NOT IN/EXISTS/<> ANY subselects
**	    and intersect join for INTERSECT.
**	11-Nov-2009 (kiria01) SIR 121883
**	    Moved to enum for debug aid & rearranged upper values for
**	    subsetting so that they could be added to join operators more
**	    easily.
*/
typedef enum _DB_JNTYPE
{
    DB_NOJOIN =		0x01,
    DB_INNER_JOIN =	0x02,
    DB_LEFT_JOIN =	0x04,
    DB_RIGHT_JOIN =	0x08,
    DB_FULL_JOIN =	DB_LEFT_JOIN|DB_RIGHT_JOIN,
    DB_JOIN_MASK =	0x0f,
    DB_ANTI_MASK =	0x10,
    DB_LANTI_JOIN =	DB_ANTI_MASK|DB_LEFT_JOIN,
    DB_RANTI_JOIN =	DB_ANTI_MASK|DB_RIGHT_JOIN,
    DB_FANTI_JOIN =	DB_ANTI_MASK|DB_FULL_JOIN,
    DB_NAT_JOIN =	0x20,
    DB_INTERSECT_JOIN =	0x40,
} DB_JNTYPE;

/*}
** Name: DB_IICOMMENT - Comment tuple
**
** Description:
**      This structure defines a comment tuple.  It is used to insert comments
**	into the comments catalog (iidbms_comment) in PSF, RDF and QEF.
**
** History:
** 	12-feb-90 (carol)
**          Written for Terminator II (comments).
*/
#define		DBC_L_LONG	1600	/* Maximum length of a long remark */
#define		DBC_L_SHORT	60	/* Length of a short remark	   */
typedef struct _DB_IICOMMENT
{
    DB_TAB_ID    dbc_tabid;		/* Table id of the comment.  This
					** corresponds to reltid and reltidx
					** in iirelation.
					** This is the comtabbase and
					** comtabidx columns.
					** It is used to join with iirelation
					** and, if applicable, iiattribute.
					*/
    i2		dbc_attid;		/* Attribute id of the comment, if
					** applicable.  This correspond to
					** attid in iiattribute.  This is the
					** comattid column.  It is used to
					** join with iiattribute for column
					** comments.
					*/
    i2		dbc_type;		/* Comment type */
#define		    DBC_T_TABLE  1	/* Comment is on a table, view or
					** index.
					*/
#define		    DBC_T_COLUMN 2	/* Comment is on a column. */
    char	dbc_short[DBC_L_SHORT];   /* Text of short remark, if
					** present;  blank if not defined.
					** This is the short_remark column.
					*/
    i2		dbc_sequence;		/* Sequence number.  Always zero for
					** now.  May be used later if long
					** remarks are ever supported longer
					** than 1600 bytes.  This is the
					** text_sequence column.
					*/
    u_i2	dbc_len_long;		/* Length field for varchar column
					** which follows.
					*/
    char	dbc_long[DBC_L_LONG];	/* Text of long remark, if present.
					** This is the long_remark column.
					** This is a varchar column.
					*/
}   DB_IICOMMENT;

/*}
** Name: DB_1_IICOMMENT - Structure used to pass DB_IICOMMENT from PSF through
**			  RDF to QEF.
**
** Description:
**      This structure consists of DB_IICOMMENT + a flag field used to indicate
**	which of long remark and short remark were specified.
**
** History:
**	21-feb-90 (andre)
**	    Written
**	1-sep-93 (stephenb)
**	    Added dbc_tabname, tablename is needed to audit comment on table
**	    action in QEF.
*/
typedef struct _DB_1_IICOMMENT
{
    DB_IICOMMENT    dbc_tuple;
    i4		    dbc_flag;
						/* long remark was specified */
#define			DBC_LR_SPECIFIED    0x01
						/* short remark was specified */
#define			DBC_SR_SPECIFIED    0x02
    DB_TAB_NAME	    dbc_tabname;
} DB_1_IICOMMENT;

/*}
** Name: DB_IISYNONYM - Synonym tuple
**
** Description:
**      This structure defines a synonym tuple.  It is used to contain aliases
**	(or alternate names) for a table.
**
** History:
** 	27-mar-90 (teg)
**          Written for Terminator II (synonyms).
**	14-oct-93 (andre)
**	    added db_syn_id
*/
typedef struct _DB_IISYNONYM
{
    DB_SYNNAME	 db_synname;		/* name of the synonym */
    DB_SYNOWN	 db_synowner;		/* owner of the synonym */
    DB_TAB_ID    db_syntab_id;		/* Table id of the table that the
					** synonym is for.  This is the
					** to reltid and reltidx of the table
					** that the synonym references. */
    DB_TAB_ID	 db_syn_id;		/* 
					** synonym id - is used when we need
					** to describe depdency of a dbproc on
					** a synonym used in its definition
					*/
} DB_IISYNONYM;

/*}
** Name: DB_IIXSYNONYM - IIXSYNONYM index on IISYNONYM
**
** Description:
**      This structure defines an IIXSYNONYM tuple.  
**
** History:
** 	15-oct-93 (andre)
**          defined
*/
typedef struct _DB_IIXSYNONYM
{
    DB_TAB_ID	    dbx_syntab_id;	/* 
    					** id of the table on which synonym is 
					** defined
					*/
    u_i4	    dbx_tidp;		/* index into IISYNONYM */
} DB_IIXSYNONYM;

/*}
** Name: DB_IIPRIV - IIPRIV table
**
** Description:
**      This structure defines an IIPRIV tuple.  
**
** History:
** 	29-may-92 (andre)
**          Written for GRANT/REVOKE project
**	15-sep-92 (andre)
**	    added db_prv_flags and defined DB_GRANT_COMPATIBLE_PRIV over it
*/
typedef struct _DB_IIPRIV
{
    DB_TAB_ID	    db_dep_obj_id;	/* id of dependent object */
    u_i2	    db_descr_no;	/* privilege descriptor number */
    i2		    db_dep_obj_type;	/* type of dependent object */
    DB_TAB_ID	    db_indep_obj_id;	/* id of independent object */
    i4		    db_priv_map;	/* map of privileges */
					/*
					** grantee of privileges described in
					** db_priv_map on an object whose id is
					** stored in db_indep_obj_id
					*/
    DB_OWN_NAME	    db_grantee;		
    i4		    db_prv_flags;	/* flags describing the privilege */
#define	    DB_GRANT_COMPATIBLE_PRIV	    ((i4) 0x01)
					/*
					** map of attributes on which the
					** privileges must be granted
					*/
    i4		    db_priv_attmap[DB_COL_WORDS];
} DB_IIPRIV;

/*}
** Name: DB_IIXPRIV - IIXPRIV index on IIPRIV
**
** Description:
**      This structure defines an IIXPRIV tuple.  
**
** History:
** 	10-jun-92 (andre)
**          Written for GRANT/REVOKE project
*/
typedef struct _DB_IIXPRIV
{
    DB_TAB_ID	    dbx_indep_obj_id;	/* id of independent object */
    i4		    dbx_priv_map;	/* map of privileges */

					/*
					** grantee of privileges described in
					** db_priv_map on an object whose id is
					** stored in db_indep_obj_id
					*/
    DB_OWN_NAME	    dbx_grantee;
    
    u_i4	    dbx_tidp;		/* index into IIPRIV */
} DB_IIXPRIV;

/*}
** Name: DB_IIXPROCEDURE - IIXPROCEDURE index on IIPROCEDURE
**
** Description:
**      This structure defines an IIXPROCEDURE tuple.  
**
** History:
** 	18-jun-92 (andre)
**          Written for GRANT/REVOKE project
*/
typedef struct _DB_IIXPROCEDURE
{
    DB_TAB_ID	    dbx_procid;		/* dbproc id */
    u_i4	    dbx_tidp;		/* index into IIPROCEDURE */
} DB_IIXPROCEDURE;

/*}
** Name: DB_IIXEVENT - IIXEVENT index on IIEVENT
**
** Description:
**      This structure defines an IIXEVENT tuple.  
**
** History:
** 	18-jun-92 (andre)
**          Written for GRANT/REVOKE project
*/
typedef struct _DB_IIXEVENT
{
    DB_TAB_ID	    dbx_evid;		/* dbevent id */
    u_i4	    dbx_tidp;		/* index into IIEVENT */
} DB_IIXEVENT;

/*
** Name:	    DB_SHR_RPTQRY_INFO
**
** Description:	    this structure will contain information which will enable us
**		    to determine if a given QUEL repeat query is shareable.
**		    This structure will be used to address the problem which
**		    arose in the course of the project to begin supporting
**		    shareable QUEL repeat queries.
**			The problem was that almost any portion of a QUEL repeat
**		    query may be specified inside a variable + range variables
**		    may be representing different tables for different users
**		    thus making the traditional mechanism (FEs supplying an
**		    object id) unreliable when trying to determine if a given
**		    query is shareable.
**			The solution lies in keeping the (slightly massaged)
**		    text of the query and a list of tables used in its parsing
**		    as a part of a query plan and comparing this information
**		    with the similarly collected information for any potentially
**		    shareable query.
**
** History:
**	24-jan-91 (andre)
**	    written
*/
typedef struct _DB_SHR_RPTQRY_INFO
{
    i4		db_qry_len;
    u_char	*db_qry;
    i4		db_num_tblids;
    DB_TAB_ID	*db_tblids;
} DB_SHR_RPTQRY_INFO;

/*}
** Name: DB_SCHEMA_NAME - Definition of schema name
**
** Description:
*     This structure defines a schema name.
**
** History:
**      13-oct-92 (anitap)
**          Written for CREATE SCHEMA project.
*/
typedef struct _DB_SCHEMA_NAME
{
  char          db_schema_name[DB_SCHEMA_MAXNAME];     /* schema name */
} DB_SCHEMA_NAME;

/*}
** Name: DB_IISCHEMA - Definition of IISCHEMA catalog
**
** Description:
**      This structure defines an IISCHEMA tuple.
**
** History:
**    01-oct-92 (anitap)
**          Written for CREATE SCHEMA project
*/
typedef struct _DB_IISCHEMA
{
   DB_SCHEMA_NAME       db_schname;     /* schema name */
   DB_OWN_NAME          db_schowner;    /* name of user defining schema */
   DB_SCHEMA_ID         db_schid;       /* id of schema being defined */
} DB_IISCHEMA;

/*}
** Name: DB_IISCHEMAIDX - Definition of secondary index on iischema
**
** Description:
**      This structure defines the index record used to index the schema
**      table.
**
** History
**      01-nov-92 (anitap)
**         Created for CREATE SCHEMA project.
**      18-jan-93 (mikem)
**         Fixed comment for DB_SEC_LABEL in DB_IISCHEMAIDX definition (it was
**         unterminated causing lint warnings).
*/
typedef struct _DB_IISCHEMAIDX
{
     DB_SCHEMA_ID    db_sch_id;           /* schema id */
    i4        tidp;                  /* Identifier of where record
                                          ** resides in schema table.
                                          */
} DB_IISCHEMAIDX;

/*
** Name: DB_IIREL_IDX - structure of IIREL_IDX tuple
**
** Description:
**	This structure describes a tuple of IIREL_IDX
**
** History:
**	22-feb-93 (andre)
**	    plagiarized from DMP_RINDEX (DMP_RINDEX has been redefined in terms
**	    of DB_IIREL_IDX)
*/
typedef struct _DB_IIREL_IDX
{
     DB_TAB_NAME     relname;               /* The relation name. */
     DB_OWN_NAME     relowner;              /* The relation owner. */
     i4	     tidp;                  /* Identifier of where record 
                                            ** resides in relation table. */
} DB_IIREL_IDX;

/*}
** Name: DB_SECALARM - Security alarm tuple
**
** Description:
**      This defines the structure of a security alarm tuple as used by qrymod
**	in PSF, RDF, QEF
**
** History:
**	25-nov-93 (robf)
**	   Created, split out from DB_PROTECTION since alarms and grants
**	   are distinct entities.
*/
typedef struct _DB_SECALARM
{
    
    DB_EVENT_NAME   dba_alarmname;	/* Name of alarm */
    i4		    dba_alarmno;	/* Number of alarm */
    DB_TAB_ID       dba_objid;          /* Object id (if table etc)*/
    DB_NAME	    dba_objname;	/* Object name */
    i1		    dba_objtype;	/* Object type (table/database etc )*/
    i1		    dba_subjtype;	/* Subject type (group/role/user etc) */
    DB_OWN_NAME	    dba_subjname;	/* Subject name */
    i2              dba_flags;          /* useful information about alarm */
#define	    DBA_DBEVENT			0x001  /* Alarm has event info */
#define	    DBA_DBEVTEXT		0x002  /* Alarm has event text */
#define	    DBA_ALL_DBS			0x004  /* Alarm on all dbs     */
    DB_QRY_ID	    dba_txtid;		/* Query text id of alarm statement */
    i4		    dba_popctl;		/* Bit map of defined   operations */
    i4		    dba_popset;		/* Bit map of permitted operations */
    DB_TAB_ID	    dba_eventid;	/* Event id */
    char	    dba_eventtext[DB_EVDATA_MAX]; /* DBevent text */
    DB_TAB_ID	    dba_alarmid;	/* Alarm id */
    char	    dba_reserve[32];	/* Reserved			*/
}   DB_SECALARM;

/* [@type_definitions@] */
/*}
** Name: DB_MEMORY_ALLOCATOR - describes a memory allocator
**
** Description:
**	One facility's customers may want returned objects to be allocated out
**	of it's own memory stream.  This structure facilitates this usage.
**
**
** History:
**	23-nov-92 (rickh)
**	    Creation.
**
**
*/
typedef struct _DB_MEMORY_ALLOCATOR
{
    PTR		dma_arguments;		/* points at the function's args */
    DB_STATUS	( *dma_function )( );	/* the memory allocator itself */
}	DB_MEMORY_ALLOCATOR;

/*}
** Name: DB_ROLEGRANT - Role grant tuple
**
** Description:
**      This defines the structure of a role grant tuple as used by qrymod
**	in PSF, RDF, QEF
**
** History:
**	4-mar-94 (robf)
**	   Created.
*/
typedef struct _DB_ROLEGRANT
{
    
    DB_OWN_NAME	    rgr_rolename;	/* Name of role */
    i4              rgr_flags;          /* useful information about rolegrant */
#define DBRG_ADMIN_OPTION	0x01	/* WITH ADMIN OPTION */
    i2		    rgr_gtype;		/* grantee type */
    DB_OWN_NAME	    rgr_grantee;	/* grantee name */
    char	    rgr_reserve[34];	/* Reserved			*/
}   DB_ROLEGRANT;

/*
** Name: DB_IIINDEX - structure of IIINDEX tuple
**
** Description:
**	This structure describes a tuple of IIINDEX
**
** History:
**	01-aug-95 (athda01)
**	    plagiarized from DMP_INDEX n dmp.h
**	30-May-2006 (jenjo02)
**	    dbix_idom[DB_MAXKEYS] changed to dbix_idom[DB_MAXIXATTS]
*/
typedef struct _DB_IIINDEX
{
     i4     	 dbix_baseid;           /* id fo base table. */
     i4     	 dbix_indexid;          /* id of index table. */
     i2	         dbix_sequence;         /* sequence number of index */
     i2		 dbix_idom[DB_MAXIXATTS]; /* Number of base table that
					** corresponds to each attr of index
					*/
} DB_IIINDEX;


/*
** Name: DB_DBCAPABILITIES - structure of IIDBCAPABILITIES tuple
**
** Description:
**	This structure describes a tuple of IIDBCAPABILITIES
**
** History:
**	06-jan-09 (stial01)
**          created
*/

typedef struct _DB_DBCAPABILITIES
{
    char	cap_capability[DB_CAP_MAXLEN];
    char	cap_value[DB_CAPVAL_MAXLEN];
} DB_DBCAPABILITIES;


/*
** Name: DB_BLOB_WKSP - Workspace for BLOB inserts
**
** Description:
**
**	This area is an addendum to the ADP_POP_CB for BLOB operations.
**	It's used to pass optional information about the BLOB source
**	(for GET) or target (for PUT) to the DMF BLOB data-fetching routines.
**	In the absence of a WKSP, the BLOB handlers either have to operate
**	off of data found in the coupon itself, or give up and operate
**	against a holding temp etab.
**
**	The caller might pass:
**	- ACCESSID and ATTID.
**	  The caller already has the base table open and knows where the
**	  BLOB is coming from or going to.  This is the simplest case for
**	  dmpe.  A typical example might be the final put of a base row
**	  into the table, where LOB values are moved from where-ever into
**	  actual etabs for the proper table and column.
**
**	- TABLEID and ATTID.
**	  Similar to the first case, but the caller only knows the base table
**	  ID, there's no open table reference.  This might happen when the
**	  sequencer is couponifying incoming data against a prepared query;
**	  the query is already parsed, so the sequencer knows where the
**	  data is going, but the actual query isn't running yet.
**
**	- TABLENAME with or without ATTID.
**	  This case happens when the sequencer is couponifying an incoming
**	  LOB against a query that's known to be an INSERT, but the
**	  query isn't parsed yet.  The sequencer can extract the table name
**	  from the query text, and that's about it.  The ATTID probably will
**	  *not* be supplied, which means that if there is more than one
**	  LOB column in the table, we don't know which one to target and
**	  the data goes to a holding temp.
**
** History:
**	24-feb-98 (stephenb)
**	    Created.
**	10-May-2004 (schka24)
**	    Remove unused members, flags.
**	31-Mar-2010 (kschendel) SIR 123485
**	    Add stuff so that COPY FROM can pass along known table/att info
**	    into dmpe.  (later) more/different stuff for BQCB changes.
*/
typedef struct _DB_BLOB_WKSP
{
    DB_OWN_NAME		table_owner;	/* owner of base table */
    DB_TAB_NAME		table_name;	/* name of base table */
    void		*access_id;	/* An "access ID" (really RCB) opened
					** to base table in current thread
					*/
    DB_DT_ID		source_dt;	/* datatype of source data, needed if
					** COERCE flag is set
					*/
    i4			base_id;	/* Base table db_tab_base */
    i4                  base_attid;     /* base table blob column number */

    i4                  flags;
/* Set flags indicating what data in the wksp was known and passed by the
** caller: either table name or accessid, and optionally the attribute id.
**
** If sequencer thinks it can couponify blob data directly into etab, it
** sets the TABLENAME flag (and optionally the ATTID) flag.  If the optim
** is successful, dmpe returns with the FINAL flag turned on.
** If a put (or replace) of a row containing a blob requires moving the
** blob from a temp holding table to the final etab, the ATTID flag must
** be provided so that dmpe knows which att it's working with.  (Temp
** holding etabs are tricked out such that a new one is created for each blob,
** and they aren't attr specific.)
*/
#define    BLOBWKSP_TABLENAME	0x01	/* base table owner.name provided */
#define    BLOBWKSP_TABLEID	0x02	/* Table base_id provided */
#define    BLOBWKSP_ACCESSID	0x04	/* Access ID provided */
#define    BLOBWKSP_ATTID	0x08	/* base table blob column # provided */
#define    BLOBWKSP_FINAL	0x10	/* Result of blob put (e.g. couponify)
					** went to a real etab rather than a
					** holding temp.  The coupon itself
					** is flagged too, but the flag is in
					** the DMF part that others (sequencer)
					** can't see, so mirror it here.
					** This is an "out" flag, not an "in"
					** flag like the rest.
					*/
#define    BLOBWKSP_COERCE	0x20	/* If set, dmpe should check source_dt
					** to see if coercion to att needed.
					** This is only used by the sequencer
					** when an incoming value might not be
					** the same type as the table.  In all
					** other situations (copy, base row
					** dmpe-move puts, etc) the source data
					** is already the proper type.
					*/

} DB_BLOB_WKSP;

#endif /* DBDBMS_INCLUDE */
