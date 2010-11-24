/*
** Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: USERERROR.H - Contains #defines for all user errors
**
** Description:
**      This file contains the #define entries for all user errors
**      known to Ingres.  All are constructed by "oring" the
**      E_US_MASK mask with the error number in question.
**
**      Note that although users are used to seeing errors in
**      decimal, we still construct the error numbers in hex
**      so that they satisfy all the silly compilers on which
**      we have to run.
**
**	As to why this file exists instead of just using erusf.h,
**	I have no clue.  (kschendel)
**
** History:
**      17-Jul-1986 (fred)
**          Created on Jupiter.
**	17-Jul-1986 (ericj)
**	    Added date's and money's datatype error numbers.
**	07-aug-1986 (ericj)
**	    Added math exception user error and warning numbers.
**      11-dec-1986 (fred)
**          Added startup errors
**      29-jan-87 (thurston)
**          Added E_US1080_NOPRINT_CHAR error.
**      18-mar-87 (thurston)
**          Added E_US100F_BAD_CVTONUM error.
**      29-apr-87 (thurston)
**          Added E_US1071_BAD_MATH_ARG.
**      15-oct-87 (thurston)
**          Added E_US0A96_BAD_CHAR_IN_STR.
**      26-sep-88 (thurston)
**          Added E_US1036_BAD_DI_FILENAME.
**	01-dec-88 (jrb)
**	    Added E_US10DC_NOABSDATES.
**	05-dec-88 (jrb)
**	    Added E_US10DD_ABSDATEINAG.
**	07-mar-89 (neil)
**	    Added RULE errors.
**	20-apr-89 (ralph)
**	    Added new GRANT errors.
**	15-may-89 (rogerk)
**	    Added Transaction quota errors.
**	23-may-89 (jrb)
**	    Renumber US0983 to US0985 because of conflict with Titan number.
**	24-jun-89 (ralph)
**	    GRANT Enhancements, Phase 2e: MAXQUERY
**	    Add messages US1860-1864
**	29-jun-89 (ralph)
**	    CONVERSION:  Added messages US0042-0043
**	20-aug-89 (ralph)
**	    CONVERSION:  Added message  US0044; Removed US0042-43
**	29-sep-89 (neil)
**	    Rules: Added message 6314
**	04-oct-89 (ralph)
**	    B1/C2: Add new message constants US18D3-4
**	10-oct-89 (mikem)
**	    logkey bug fixes: added messages E_US1900_6400_UPD_LOGKEY and
**	    E_US1901_6401_BAD_SYSMNT .
**	28-oct-89 (ralph)
**	    B1/C2: Added message constant US18DC.
**	16-jan-90 (ralph)
**	    Added US18FF_BAD_USER_ID for user passwords
**	08-aug-90 (ralph)
**	    Added E_US1712_TRACE_PERM
**	    Added E_US18D5_6357_FORM_NOT_AUTH and E_US18D6_6358_ALTER_BAD_PASS
**	08-nov-91 (ralph)
**	    6.4->6.5 merge
**		20-dec-90 (rickh)
**		    Added E_US0050_DB_SERVER_INCOMPAT and
**		    E_US0051_DB_SERVER_MISMATCH: database and server are
**		    incompatible.
**		13-aug-91 (fpang)
**		    Added E_US13FE_NO_STAR_SUPP for things that are unsupported
**		    by STAR.
**	28-dec-92 (rickh)
**	    Added E_US1905_INTEGRITY_VIOLATION.
**	10-feb-93 (ralph)
**	    Added CUI messages
**	04-mar-93 (andre)
**	    defined E_US1910_6416_CHECK_OPTION_ERR
**	04-apr-93 (rblumer)
**	    defined E_US18AB_6315_NOT_DROPPABLE, for internal objects.
**	16_jun-93 (robf)
**	    Added E_US0060_CREATEDB_NO_PRIV, E_US0061_USER_EXPIRED
**	26-jul-1993 (rmuth)
**	    Add E_US14E4_5348_NON_FAST_COPY, 
**	    E_US14E5_5349_MODIFY_NO_WITH
**	23-aug-1993 (rmuth)
**	    Add E_US14E8_5352_DROP_READONLY.
**	11-oct-93 (rblumer)
**	    Added E_US1906_REF - E_US1909_REF constraint violation errors.
**	02-nov-93 (andre)
**	    added E_US088E_2190_INACCESSIBLE_TBL, 
**	    E_US088F_2191_INACCESSIBLE_EVENT, and E_US0890_2192_INACCESSIBLE_DBP
**	26-nov-93 (robf)
**	    Add E_US2473_9331_ALARM_ABSENT, E_US2472_9330_ALARM_EXISTS
**	19-jan-94 (rblumer)
**	    Add E_US1A26_CU_ID_PARAM_ERROR.
**	06-jan-95 (forky01)
**	    Add E_US2481_9345_MODIFY_TO_NO_WITH.
**      17-may-95 (dilma04)
**          Add E_US14E6_5350_READONLY_TBL_ERR.
**	27-Feb-1997 (jenjo02)
**	    Extended <set transaction statement> to support <transaction access mode>
**	    (READ ONLY | READ WRITE) per SQL-92.
**	    Added E_US138F_TRAN_ACCESS_CONFLICT.
**	20-oct-98 (inkdo01)
**	    Added E_US1911_REFINT_RESTR_VIOLATION (why do we have to add 'em
**	    here as well as in erusf.msg?)
**	04-mar-1999 (walro03)
**	    Compiler warnings caused by trigraph unintentionally left in
**	    20-oct-98 history entry.  Removed trigraph.
**	09-Jun-1999 (shero03)
**	    Added E_US1010_BAD_HEX_CHAR.
**      04-feb-2000 (gupsh01)
**	    Added new error message E_US1081_ILLEGAL_IP_ADDRESS  for illegal 
**	    IP address entry. (bug 100265)
**    07-mar-2000 (gupsh01)
**        Added new error E_US0090_CREATEDB_RONLY_NOTEXIST for the readonly
**        databases, for cases where the user provides the createdb command
**        for readonly databse and the database directory does not exist at 
**        the specified data location.                                             
**	02-Apr-2001 (jenjo02)
**	    Added E_US0088_RAW_AREA_IN_USE when extending a location to an
**	    area already extended to by another db,
**	    E_US0089_RAW_LOC_IN_USE when extending a location that has
**	    already been extended to another database,
**	    E_US18DE_INVALID_USAGE if CREATING/ALTERING a raw location to
**	    a usage other than DATABASE.
[@history_template@]...
**     
**      1-Nov-2000 (zhahu02)
**          Added E_US2481_9350_MODIFY_TO_TABLE_OPTION (bug 103046)
**	19-may-2001 (somsa01)
**	    Modified E_US2481_9350_MODIFY_TO_TABLE_OPTION to
**	    E_US248A_9354_MODIFY_TO_TABLE_OPTION.
**	15-Oct-2001 (jenjo02)
**	    Changed E_US18DE_INVALID_USAGE to INVALID_RAW_USAGE,
**	    Added E_US18DF, E_US18E0, E_US18E1, E_US18D7.
**	04-Dec-2001 (jenjo02)
**	    Added E_US18E5_6373_INVALID_ROOT_LOC.
**	05-Mar-2002 (jenjo02)
**	    Added US1912,US1913,US1915 for Sequence Generators.
**	14-may-02 (inkdo01)
**	    Added US0891 for sequences.
**	27-Jan-2004 (schka24)
**	    Added partition= syntax messages.
**	21-Dec-2004 (jenjo02)
**	    Added US008A, US008B for unextenddb.
**	29-Jul-2005 (thaju02)
**	    Added E_US008C, E_US008D.
**	12-Jun-2006 (kschendel)
**	    More messages for sequence defaults.
**	15-june-06 (dougi)
**	    Added E_US18B4 & E_US18B5.
**	05-sep-06 (gupsh01)
**	    Added E_US10DF.
**	15-sep-06 (gupsh01)
**	    Added E_US10E0_DATE_NOALIAS.
**	11-nov-06 (gupsh01)
**	    Added E_US10E1_DATE_COERCE.
**	22-nov-06 (gupsh01)
**	    Added E_US10E2_ANSIDATE_4DIGYR.
**	10-dec-06 (gupsh01)
**	    Added E_US10E3_ANSIDATE_DATESUB and E_US10E4_ANSIDATE_DATEADD
**	12-dec-06 (gupsh01)
**	    Added error checking for valid ANSI date/time format.
**	08-jan-07 (gupsh01)
**	    Added E_US10ED_ANSITIME_INTVLADD for checking addition of
**	    interval types and time data types. 
**	1-feb-2007 (dougi)
**	    Added E_US10F0_4336 for attempts to retrieve data type not 
**	    supported by client.
**	10-Feb-2008 (kiria01) b120043
**	    Added E_US10F1_DATE_DIV_BY_ZERO, E_US10F2_DATE_DIV_ANSI_INTV
**	    and E_US10F3_DATE_ARITH_NOABS
> **    12-Mar-2008  (coomi01) Issue:125580
> **        Add E_US10F4_4340_TRIM_SPEC for ansi trim spec error.
**	28-Apr-2008 (thaju02)
**	    Added E_US1950_6468_NO_PARTRULE_SPEC. (B120283)
**      21-Jan-2009 (horda03) Bug 121519
**          Add E_US082D_2093 for CREATE TABLE failure when all Table Ids
**          have been used.
**	28-May-2009 (kschendel) b122118
**	    Add 6499 for improved modify keyword checking.
**	20-Aug-2009 (kschendel) b122510
**	    Add 2512 message for bad cast type.
**	03-Mar-2010 (jonj)
**	    SIR 121619 MVCC, blob support:
**	    Add E_US120C_INVALID_LOCK_LEVEL,
**	    E_US125B_UNABLE_TO_SERIALIZE
**	18-Mar-2010 (kiria01) b123438
**	    Added SINGLETON aggregate user message E_US1196_NOT_ZEROONE_ROWS
**      23-Mar-2010 (hanal04) Bug 122436
**          Added E_US1072_BAD_LOCATE_ARG.
**      23-Jun-2010 (coomi01) b123763 
**          Add E_US18E8_6376_EXT_IIDBDB_LOC
**      01-Jul-2010 (horda03) B123234
**          Added new error E_US10F5_INTERVAL_IN_ABS_FUNC.
**	11-Oct-2010 (kschendel) SIR 124544
**	    DDL WITH-option code reworked, new messages here.
**/

/*
**  Defines of other constants.
*/

/*
**      The High level mask
*/
#define                 E_US_MASK       0x00000000L
#define			E_USST_MASK	0x00000000L
#define                 E_USPS_MASK     0x000007D0L
#define			E_USAD_MASK	0x00000FA0L

/*
** Startup Errors
*/
#define                 E_US0001_TOO_MANY_USERS 0x1
#define			E_US0002_NO_FLAG_PERM	0x2
#define			E_US0003_NO_DB_PERM	0x3
#define			E_US0004_CANNOT_ACCESS_DB   0x4
#define			E_US0005_RELATION_TABLE	0x5
#define			E_US0006_ATTRIBUTE_TABLE    0x6
#define			E_US0007_CONFIG_FILE	0x7
#define			E_US0008_CONVERT_DB	0x8
#define			E_US0009_BAD_PARAM	0x9
#define			E_US000A_USER_TABLE_GONE    0xA
#define			E_US000B_USER_TABLE_OPEN    0xB
#define			E_US000C_INVALID_REAL_USER  0xC
#define			E_US000D_INVALID_EFFECTIVE_USER	0xD
#define			E_US000E_DATABASE_TABLE_GONE	0xE
#define			E_US000F_DATABASE_TABLE	    0xF
#define			E_US0010_NO_SUCH_DB	    0x10
#define			E_US0011_DBACCESS_TBL_GONE  0x11
#define			E_US0012_DBACCESS_TBL_OPEN  0x12
#define			E_US0013_LOCK_FAIL	    0x13
#define			E_US0014_DB_UNAVAILABLE	    0x14
#define			E_US0015_XACT_SYSTEM	    0x15
#define			E_US0016_WRITE_CONFIG	    0x16
#define			E_US0017_FIND_CONFIG	    0x17
#define			E_US0018_ANCIENT	    0x18
#define			E_US0019_DB_INCONSIST	    0x19
#define			E_US001A_MISSING_DB_AREA    0x1A
#define			E_US001B_MISSING_CKP_AREA   0x1B
#define			E_US001C_MISSING_JNL_AREA   0x1C
#define			E_US001D_IILOCATIONS_ERROR  0x1D
#define			E_US001E_NO_LOCATIONNAMES   0x1E
#define			E_US001F_IIEXTEND_ERROR	    0x1F
#define			E_US0020_IILOCATIONS_OPEN   0x20
#define			E_US0021_LOCNAMES_ERROR	    0x21
#define			E_US0022_FLAG_FORMAT	    0x22
#define			E_US0023_FLAG_AUTH	    0x23
#define			E_US0024_BAD_DB_ACCESS	    0x24
#define			E_US0025_BAD_JNL_ACCESS	    0x25
#define			E_US0026_DB_INCONSIST	    0x26
#define			E_US0027_NO_JNL_SLOTS	    0x27
#define			E_US0028_OPEN_JNL	    0x28
#define			E_US0029_GS_INCOMPAT	    0x29
#define			E_US002A_CACHE_LOCK	    0x2A
#define			E_US002B_DBDB_OPEN	    0x2B
#define			E_US002C_DBDB_UNAVAILABLE   0x2C
#define			E_US002D_CLOSE_DB	    0x2D
#define			E_US002E_LOCK_EXCEPTION	    0x2E
#define			E_US002F_MAP_GL_SECTION	    0x2F
#define			E_US0030_DMF_START_EXCEPT   0x30
#define			E_US0031_EXCEPT_SQUARED	    0x31
#define			E_US0032_EXCEPTION_DBDIR    0x32
#define			E_US0033_DBDIR_INIT	    0x33
#define			E_US0034_DBDIR_RESET	    0x34
#define			E_US0035_DMF_BUF_QUE	    0x35
#define			E_US0036_CONFIG_EXCEPT	    0x36
#define			E_US0037_EXC_DESC_CACHE	    0x37
#define			E_US0038_CONFIG_LOCK_EXC    0x38
#define			E_US0039_CONFIG_OPEN_EXC    0x39
#define			E_US003A_XACT_EXCETP	    0x3A
#define			E_US003B_CONGIG_REWRT_EXC   0x3B
#define			E_US003C_CONFIG_CLOSE_EXC   0x3C
#define			E_US003D_CONFIG_UNL_EXC	    0x3D
#define			E_US003E_RELAT_OPEN_EXC	    0x3E
#define			E_US003F_CONFIG_OPEN_EXC    0x3F
#define			E_US0040_APPL_0_ONLY	    0x40
#define			E_US0041_DBDB_CONFIG_INCON  0x41
#define			E_US0042_COMPAT_LEVEL	    0x42
#define			E_US0043_COMPAT_MINOR	    0x43
#define			E_US0044_CONVERT_DBDB	    0x44
#define			E_US0046_TIMEZONE_ABSENT    0x46
#define			E_US0048_DB_QUOTA_EXCEEDED  0x48
#define			E_US0049_ERROR_ACCESSING_DB 0x49
#define                 E_US0050_DB_SERVER_INCOMPAT 0x50
#define                 E_US0051_DB_SERVER_MISMATCH 0x51
#define			E_US0060_CREATEDB_NO_PRIV   0x60
#define			E_US0061_USER_EXPIRED  	    0x61
#define			E_US0065_IBUF_OVERFLOW	    0x65
#define			E_US0067_REMNODE_CONFIGURE  0x67
#define			E_US0068_LOCNODE_CONFIGURE  0x68
#define			E_US0069_LOCNODE_LICENSE    0x69
#define			E_US006A_REMNODE_LICENSE    0x6A
#define			E_US006B_LOCDBMS_LICENSE    0x6B
#define			E_US006C_REMDBMS_LICENSE    0x6C
#define			E_US006D_IIEXTEND_MISSING   0x6D
#define			E_US0073_IIDBDB_LOCKED	    0x73
#define			E_US0080_LOCATION_NOT_FOUND 0x80
#define			E_US0081_LOCATION_NOT_ALLOWED	0x81
#define			E_US0082_DATABASE_UPDATE    0x82
#define			E_US0083_EXTEND_UPDATE	    0x83
#define			E_US0084_DBACCESS_UPDATE    0x84
#define			E_US0085_DATABASE_EXISTS    0x85
#define			E_US0086_DBPRIV_UPDATE	    0x86
#define			E_US0087_EXTEND_ALREADY	    0x87
#define			E_US0088_RAW_AREA_IN_USE    0x88
#define			E_US0089_RAW_LOC_IN_USE     0x89
#define			E_US008A_NOT_EXTENDED       0x8A
#define			E_US008B_LOCATION_NOT_EMPTY 0x8B
#define			E_US008C_LOC_WORK_AND_AWORK 0x8C
#define			E_US008D_LOC_AREA_WORK_AND_AWORK    0x8D
#define			E_US0090_CREATEDB_RONLY_NOTEXIST    0x90

#define			E_US07D7_2007_UNKNOWN_WITHOPT (E_US_MASK + 0x07D7)
#define			E_US07FE_2046_WITHOPT_USAGE (E_US_MASK + 0x07FE)
#define			E_US07FF_2047_WITHOPT_VALUE (E_US_MASK + 0x07FF)
#define			E_US0805_2053_SEQDEF_TYPE   (E_US_MASK + 0x0805)

#define                 E_US082D_2093               (E_US_MASK + 0x082D)

/* 
** new vague errors for when a user references an object which either does exist
** or for which the user lacks required privileges
** Errors are placed here because they may be referenced from PSF or QEF and 
** erusf.msg seemed like a nice neutral turf
*/
#define			E_US088E_2190_INACCESSIBLE_TBL		0x088E
#define			E_US088F_2191_INACCESSIBLE_EVENT	0x088F
#define			E_US0890_2192_INACCESSIBLE_DBP		0x0890
#define			E_US0891_2193_INACCESSIBLE_SEQ		0x0891

#define			E_US09D0_2512_INVALID_CAST_TYPE	(E_US_MASK + 0x09d0)

#define			E_US1260_IIDBDB_NO_LOCKS    0x1260

/* Two-Phase commit errors are in decimal range 4710- */

#define			E_US1266_4710_TRAN_ID_NOTUNIQUE (E_US_MASK + 0x1266L)
#define			E_US1267_4711_DIS_TRAN_UNKNOWN	(E_US_MASK + 0x1267L)
#define			E_US1268_4712_DIS_TRAN_OWNER	(E_US_MASK + 0x1268L)
#define			E_US1269_4713_NOSECUREINCLUSTER (E_US_MASK + 0x1269L)
#define			E_US126A_4714_ILLEGAL_STMT	(E_US_MASK + 0x126AL)
#define			E_US126B_4715_DIS_DB_UNKNOWN	(E_US_MASK + 0x126BL)

/*
** More transaction errors - these are generally in the 4700 range, but not
** all defined here in usererror.h
*/
#define			E_US1271_4721_IIDBDB_TRAN_LIMIT (E_US_MASK + 0x1271L)
#define			E_US1272_4722_DB_TRAN_LIMIT	(E_US_MASK + 0x1272L)

/* MVCC-related errors: */
#define			E_US120C_INVALID_LOCK_LEVEL	(E_US_MASK + 0x120CL)
#define			E_US125B_UNABLE_TO_SERIALIZE	(E_US_MASK + 0x125BL)



/*[@defined_constants@]... someone should fill these in */


/*
**  This group of error codes are errors that the scanner used to generate.
*/

#define			E_US0A96_BAD_CHAR_IN_STR	(E_US_MASK + 0x0A96L)


/*
**    User errors from ADF.  These used to be classified as OVQP
**  errors.
*/
/* {@fix_me@} */
/*    What follows is a list of OVQP decimal errors that have yet to
**  be defined.  For the most part, these are ADF errors.  They all
**  should be mapped and removed from this list before ADF is done.
**
**	4100 (0x1004) - 4101 (0x1005),
**	4105 (0x1009) - 4110 (0x100E),
**	4112 (0x1010) - 4114 (0x1012),
**	4205 (0x106D) - 4208 (0x1070),
**	4210 (0x1072) - 4214 (0x1076),
**	4219 (0x107C) - 4223 (0x107F),
**	4225 (0x1081)
*/

/*
**  Define formerly called OVQP errors.
*/
#define			E_US100F_BAD_CVTONUM 	(E_US_MASK + 0x100FL)

/*
**  New user errors for 6.0 and beyond.
*/
#define			E_US1036_BAD_DI_FILENAME (E_US_MASK + 0x1036L)

/*
**  Define math exceptions errors associated with intrinsic datatypes.
*/
#define                 E_US1068_INTOVF_ERROR	(E_US_MASK + 0x1068L)
#define                 E_US1069_INTDIV_ERROR	(E_US_MASK + 0x1069L)
#define                 E_US106A_FLTOVF_ERROR	(E_US_MASK + 0x106AL)
#define                 E_US106B_FLTDIV_ERROR	(E_US_MASK + 0x106BL)
#define                 E_US106C_FLTUND_ERROR	(E_US_MASK + 0x106CL)

/*
**  Invalid argument in call to math function.
*/
#define			E_US1071_BAD_MATH_ARG	(E_US_MASK + 0x1071L)

/*
**  Invalid argument in call to locate function.
*/
#define                 E_US1072_BAD_LOCATE_ARG (E_US_MASK + 0x1072L)

/*
**  Define math exceptions warnings associated with intrinsic datatypes.
*/
#define                 E_US1077_INTOVF_WARN	(E_US_MASK + 0x1077L)
#define                 E_US1078_INTDIV_WARN	(E_US_MASK + 0x1078L)
#define                 E_US1079_FLTOVF_WARN	(E_US_MASK + 0x1079L)
#define                 E_US107A_FLTDIV_WARN	(E_US_MASK + 0x107AL)
#define                 E_US107B_FLTUND_WARN	(E_US_MASK + 0x107BL)

/*
**  Warning for "Non-printing character(s) converted to blank(s)."
*/
#define                 E_US1080_NOPRINT_CHAR	(E_US_MASK + 0x1080L)
#define                 E_US1081_BAD_IP_ADDRESS (E_US_MASK + 0x1081L)
/*
**  Define constants for "date" datatype user-errors.  The decimal range
**  for these numbers is 4300 (0x10CC) - 4316 (0x10DC).
*/
#define                 E_US10CC_DATEADD	(E_US_MASK + 0x10CCL)
#define			E_US10CD_DATESUB	(E_US_MASK + 0x10CDL)
#define			E_US10CE_DATEVALID	(E_US_MASK + 0x10CEL)
#define			E_US10CF_DATEYEAR	(E_US_MASK + 0x10CFL)
#define			E_US10D0_DATEMONTH	(E_US_MASK + 0x10D0L)
#define			E_US10D1_DATEDAY	(E_US_MASK + 0x10D1L)
#define			E_US10D2_DATETIME	(E_US_MASK + 0x10D2L)
/* {@fix_me@} */
/* ERROR: DATEQUAL may be a dead error number */
#define			E_US10D3_DATEQUAL	(E_US_MASK + 0x10D3L)
#define			E_US10D4_DATEBADCHAR	(E_US_MASK + 0x10D4L)
#define			E_US10D5_DATEAMPM	(E_US_MASK + 0x10D5L)
#define			E_US10D6_DATEYROVFLO	(E_US_MASK + 0x10D6L)
#define			E_US10D7_DATEYR		(E_US_MASK + 0x10D7L)
#define			E_US10D8_DOWINVALID	(E_US_MASK + 0x10D8L)
#define			E_US10D9_DATEABS	(E_US_MASK + 0x10D9L)
/* {@fix_me@} */
/* ERRRO: DATENA may be a dead error number */
#define			E_US10DA_DATENA	        (E_US_MASK + 0x10DAL)
#define			E_US10DB_DATEINTERVAL	(E_US_MASK + 0x10DBL)
#define			E_US10DC_NOABSDATES	(E_US_MASK + 0x10DCL)
#define			E_US10DD_ABSDATEINAG	(E_US_MASK + 0x10DDL)
#define			E_US10DE_DATEOVFL	(E_US_MASK + 0x10DEL)
#define			E_US10DF_DATEANSI	(E_US_MASK + 0x10DFL)
#define			E_US10E0_DATE_NOALIAS	(E_US_MASK + 0x10E0L)
#define			E_US10E1_DATE_COERCE	(E_US_MASK + 0x10E1L)
#define			E_US10E2_ANSIDATE_4DIGYR (E_US_MASK + 0x10E2L)
#define			E_US10E3_ANSIDATE_DATESUB (E_US_MASK + 0x10E3L)
#define			E_US10E4_ANSIDATE_DATEADD (E_US_MASK + 0x10E4L)
#define			E_US10E5_ANSIDATE_BADFMT (E_US_MASK + 0x10E5L)
#define			E_US10E6_ANSITMWO_BADFMT (E_US_MASK + 0x10E6L)
#define			E_US10E7_ANSITMW_BADFMT  (E_US_MASK + 0x10E7L)
#define			E_US10E8_ANSITSWO_BADFMT (E_US_MASK + 0x10E8L)
#define			E_US10E9_ANSITSW_BADFMT  (E_US_MASK + 0x10E9L)
#define			E_US10EA_ANSIINYM_BADFMT (E_US_MASK + 0x10EAL)
#define			E_US10EB_ANSIINDS_BADFMT (E_US_MASK + 0x10EBL)
#define			E_US10EC_ANSITMZONE_BADFMT (E_US_MASK + 0x10ECL)
#define			E_US10ED_ANSITIME_INTVLADD (E_US_MASK + 0x10EDL)
			/* E_US10F0_4336 */
#define			E_US10F1_DATE_DIV_BY_ZERO (E_US_MASK + 0x10F1L)
#define			E_US10F2_DATE_DIV_ANSI_INTV (E_US_MASK + 0x10F2L)
#define			E_US10F3_DATE_ARITH_NOABS (E_US_MASK + 0x10F3L)
#define                 E_US10F4_4340_TRIM_SPEC   (E_US_MASK + 0x10F4L)
#define			E_US10F5_INTERVAL_IN_ABS_FUNC  (E_US_MASK + 0x10F5L)

/*
**  Define constants for "money" datatype user-errors.  The decimal range
**  for these numbers is 4400 (0x1130) - 4410 (0x113A).
*/
#define			E_US1130_BADCH_MNY	(E_US_MASK + 0x1130L)
#define			E_US1131_MAXMNY_OVFL	(E_US_MASK + 0x1131L)
#define			E_US1132_MINMNY_OVFL	(E_US_MASK + 0x1132L)
#define			E_US1133_BLANKS_MNY	(E_US_MASK + 0x1133L)
#define			E_US1134_MDOLLARSGN	(E_US_MASK + 0x1134L)
#define			E_US1135_MSIGN_MNY	(E_US_MASK + 0x1135L)
#define			E_US1136_DECPT_MNY	(E_US_MASK + 0x1136L)
#define			E_US1137_COMMA_MNY	(E_US_MASK + 0x1137L)
#define			E_US1138_SIGN_MNY	(E_US_MASK + 0x1138L)
#define			E_US1139_DOLLARSGN	(E_US_MASK + 0x1139L)
#define			E_US113A_BAD_MNYDIV	(E_US_MASK + 0x113AL)
#define			E_US113B_BAD_HEX_CHAR	(E_US_MASK + 0x113BL)

/* User error for cardinality violation */
#define			E_US1196_NOT_ZEROONE_ROWS (E_US_MASK + 0x1196L)

/* User errors for new DBP statements */
#define			E_US0984_2436_RSER_TYPE	  (E_US_MASK + 0x0984L)
#define			E_US0985_2437_RSER_NUL	  (E_US_MASK + 0x0985L)
#define			E_US0986_2438_EXEC_COL	  (E_US_MASK + 0x0986L)
#define			E_US0987_2439_EXEC_TYPE	  (E_US_MASK + 0x0987L)
#define			E_US0989_2441_TTAB_PARM	  (E_US_MASK + 0x0989L)

/*
**	Miscellaneous 
*/
#define			E_US14DC_5340_WITHOPT_TOOMANY (E_US_MASK + 0x14DC)
#define			E_US14E4_5348_NON_FAST_COPY  (E_US_MASK + 0x14E4L)
#define                 E_US14E6_5350_READONLY_TBL_ERR (E_US_MASK + 0x14E6L)
#define 		E_US14E8_5352_DROP_READONLY  (E_US_MASK + 0x14E8L)
#define			E_US1712_TRACE_PERM	     (E_US_MASK + 0x1712L)
#define			E_US138F_TRAN_ACCESS_CONFLICT (E_US_MASK + 0x138FL)

/*
**	User errors associated with MAXQUERY
*/
#define			E_US1860_EXCESSIVE_MAXCOST  (E_US_MASK + 0x1860L)
#define			E_US1861_EXCESSIVE_MAXCPU   (E_US_MASK + 0x1861L)
#define			E_US1862_EXCESSIVE_MAXIO    (E_US_MASK + 0x1862L)
#define			E_US1863_EXCESSIVE_MAXPAGE  (E_US_MASK + 0x1863L)
#define			E_US1864_EXCESSIVE_MAXROW   (E_US_MASK + 0x1864L)


/*
**	User errors associated with group/application identifiers
*/
#define			E_US186A_IIUSERGROUP	    (E_US_MASK + 0x186AL)
#define			E_US186B_IIAPPLICATION_ID   (E_US_MASK + 0x186BL)
#define			E_US186C_BAD_GRP_ID	    (E_US_MASK + 0x186CL)
#define			E_US186D_BAD_APL_ID	    (E_US_MASK + 0x186DL)
#define			E_US186E_GRPID_NOT_AUTH	    (E_US_MASK + 0x186EL)
#define			E_US186F_APLID_NOT_AUTH	    (E_US_MASK + 0x186FL)
#define			E_US1870_GRPID_NOT_DBDB	    (E_US_MASK + 0x1870L)
#define			E_US1871_APLID_NOT_DBDB	    (E_US_MASK + 0x1871L)
#define			E_US1872_GRPID_EXISTS	    (E_US_MASK + 0x1872L)
#define			E_US1873_APLID_EXISTS	    (E_US_MASK + 0x1873L)
#define			E_US1874_GRPID_NOT_EXIST    (E_US_MASK + 0x1874L)
#define			E_US1875_APLID_NOT_EXIST    (E_US_MASK + 0x1875L)
#define			E_US1876_GRPMEM_EXISTS	    (E_US_MASK + 0x1876L)
#define			E_US1877_GRPMEM_NOT_EXIST   (E_US_MASK + 0x1877L)
#define			E_US1878_GRPID_NOT_EMPTY    (E_US_MASK + 0x1878L)

/*
**	User errors associated with GRANT/REVOKE database privileges
*/
#define			E_US1888_IIDBPRIV	    (E_US_MASK + 0x1888L)
#define			E_US1889_DBPRIV_NOT_AUTH    (E_US_MASK + 0x1889L)
#define			E_US188A_DBPRIV_NOT_DBDB    (E_US_MASK + 0x188AL)
#define			E_US1893_DBPRIV_DB_UNKNOWN  (E_US_MASK + 0x1892L)
#define			E_US1894_DBPRIV_NOT_GRANTED (E_US_MASK + 0x1894L)

/* RULE errors are in decimal range 6300-6325 */
#define			E_US189C_6300_RULE_EXISTS (E_US_MASK + 0x189CL)
#define			E_US189D_6301_RULE_ABSENT (E_US_MASK + 0x189DL)
#define			E_US189E_6302_RULE_QUAL   (E_US_MASK + 0x189EL)
#define			E_US189F_6303_RULE_COL    (E_US_MASK + 0x189FL)
#define			E_US18A0_6304_RULE_STAR   (E_US_MASK + 0x18A0L)
#define			E_US18A1_6305_RULE_II     (E_US_MASK + 0x18A1L)
#define			E_US18A2_6306_RULE_COLTAB (E_US_MASK + 0x18A2L)
#define			E_US18A3_6307_RULE_STMT   (E_US_MASK + 0x18A3L)
#define			E_US18A4_6308_RULE_VIEW   (E_US_MASK + 0x18A4L)
#define			E_US18A5_6309_RULE_OWN    (E_US_MASK + 0x18A5L)
#define			E_US18A6_6310_RULE_OLDNEW (E_US_MASK + 0x18A6L)
#define			E_US18A7_6311_RULE_WHEN   (E_US_MASK + 0x18A7L)
#define			E_US18A8_6312_RULE_DRPTAB (E_US_MASK + 0x18A8L)
#define			E_US18A9_6313_RULE_CAT	  (E_US_MASK + 0x18A9L)
#define                 E_US18AA_6314_SET_NORULES (E_US_MASK + 0x18AAL)
#define                 E_US18AB_6315_NOT_DROPPABLE (E_US_MASK + 0x18ABL)
#define			E_US18B4_6324_NOSTMTBEFORE (E_US_MASK + 0x18B4L)
#define			E_US18B5_6325_NODCBEFORE  (E_US_MASK + 0x18B5L)

/* 
** USER, LOCATION, DBACCESS and security errors are in 
** decimal range 6326-6399
*/
#define			E_US18B6_6326_USER_EXISTS (E_US_MASK + 0x18B6L)
#define			E_US18B7_6327_USER_ABSENT (E_US_MASK + 0x18B7L)
#define			E_US18B8_6328_LOC_EXISTS  (E_US_MASK + 0x18B8L)
#define			E_US18B9_6329_LOC_ABSENT  (E_US_MASK + 0x18B9L)
#define			E_US18BA_6330_DB_UNKNOWN  (E_US_MASK + 0x18BAL)
#define			E_US18BB_6331_USR_UNKNOWN (E_US_MASK + 0x18BBL)
#define			E_US18BC_6332_DBACC_EXISTS (E_US_MASK + 0x18BCL)
#define			E_US18BD_6333_DBACC_ABSENT (E_US_MASK + 0x18BDL)
#define                 E_US18BE_6334_IIUSER      (E_US_MASK + 0x18BEL)
#define                 E_US18BF_6335_IILOCATION  (E_US_MASK + 0x18BFL)
#define                 E_US18C0_6336_IIDBACCESS  (E_US_MASK + 0x18C0L)
#define                 E_US18C1_6337_IISECURITYSTATE	(E_US_MASK + 0x18C1L)
#define                 E_US18D3_6355_NOT_AUTH	  (E_US_MASK + 0x18D3L)
#define                 E_US18D4_6356_NOT_DBDB	  (E_US_MASK + 0x18D4L)
#define                 E_US18D5_6357_FORM_NOT_AUTH	(E_US_MASK + 0x18D5L)
#define                 E_US18D6_6358_ALTER_BAD_PASS	(E_US_MASK + 0x18D6L)
#define                 E_US18D7_6359_NO_RAW_SPACE	(E_US_MASK + 0x18D7L)
#define                 E_US18DC_LOC_IN_USE	  (E_US_MASK + 0x18DCL)
#define                 E_US18DE_6366_INVALID_RAW_USAGE	  (E_US_MASK + 0x18DEL)
#define                 E_US18DF_6367_RUN_MKRAWAREA	  (E_US_MASK + 0x18DFL)
#define                 E_US18E0_6368_AREA_IS_RAW	  (E_US_MASK + 0x18E0L)
#define                 E_US18E1_6369_AREA_NOT_RAW	  (E_US_MASK + 0x18E1L)
#define                 E_US18E5_6373_INVALID_ROOT_LOC	  (E_US_MASK + 0x18E5L)
#define			E_US18FF_BAD_USER_ID	  (E_US_MASK + 0x18FFL)
#define                 E_US18E8_6376_EXT_IIDBDB_LOC      (E_US_MASK + 0x18E8L)

/* Logical key errors are in decimal range 6400-6404 */
#define                 E_US1900_6400_UPD_LOGKEY  (E_US_MASK + 0x1900L)
#define                 E_US1901_6401_BAD_SYSMNT  (E_US_MASK + 0x1901L)

/* ANSI INTEGRITY errors are in decimal range 6405-6410 */
#define                 E_US1905_INTEGRITY_VIOLATION  (E_US_MASK + 0x1905L)
#define                 E_US1906_REFING_FK_INS_VIOLATION  (E_US_MASK + 0x1906L)
#define                 E_US1907_REFING_FK_UPD_VIOLATION  (E_US_MASK + 0x1907L)
#define                 E_US1908_REFED_PK_DEL_VIOLATION   (E_US_MASK + 0x1908L)
#define                 E_US1909_REFED_PK_UPD_VIOLATION   (E_US_MASK + 0x1909L)

/* CHECK OPTION error; doubt if there will be more than one */
#define			E_US1910_6416_CHECK_OPTION_ERR (E_US_MASK + 0x1910L)

/* One more ANSI referential integrity message for "RESTRICT" refacts. */
#define                 E_US1911_REFINT_RESTR_VIOLATION   (E_US_MASK + 0x1911L)

/* Sequence generator errors */
#define                 E_US1912_6418_SEQ_EXISTS	  (E_US_MASK + 0x1912L)
#define                 E_US1913_6419_SEQ_NOT_EXISTS	  (E_US_MASK + 0x1913L)
#define                 E_US1915_6421_SEQ_LIMIT_EXCEEDED  (E_US_MASK + 0x1915L)

/* PARTITION definition and usage errors */
#define			E_US1930_6448_PARTITION_EXPECTS	(E_US_MASK + 0x1930)
#define			E_US1931_6449_PARTITION_BADOPT	(E_US_MASK + 0x1931)
#define			E_US1932_6450_PARTITION_NOTALLOWED (E_US_MASK + 0x1932)
#define			E_US1933_6451_PART_TOOMANY	(E_US_MASK + 0x1933)
#define			E_US1934_6452_PART_BADCOL	(E_US_MASK + 0x1934)
#define			E_US1935_6453_PART_BADCOLTYPE	(E_US_MASK + 0x1935)
#define			E_US1936_6454_PART_NNAMES	(E_US_MASK + 0x1936)
#define			E_US1937_6455_PART_VALTOOLONG	(E_US_MASK + 0x1937)
#define			E_US1938_6456_PART_VALTYPE	(E_US_MASK + 0x1938)
#define			E_US1939_6457_PART_VALCOUNT	(E_US_MASK + 0x1939)
/* FIXME: the next few errors are numbered wrong!  I forgot to count
** in hex instead of decimal.  (kschendel)
** Not sure of best fix, probably leave the E_USxxxx numbering alone since
** that's what a program sees, and adjust the decimal part.
*/
#define			E_US1940_6458_PART_INVALID	(E_US_MASK + 0x1940)
#define			E_US1941_6459_NOT_PART		(E_US_MASK + 0x1941)
#define			E_US1942_6460_EXPECTED_BUT	(E_US_MASK + 0x1942)
#define			E_US1943_6461_NOT_SUBPART	(E_US_MASK + 0x1943)
#define			E_US1944_6462_STRUCTURE_REQ	(E_US_MASK + 0x1944)
#define			E_US1945_6463_LPART_NOTALLOWED	(E_US_MASK + 0x1945)
#define			E_US1946_6464_PPART_ONLY	(E_US_MASK + 0x1946)
#define			E_US1947_6465_X_NOTWITH_Y	(E_US_MASK + 0x1947)
#define			E_US1948_6466_UNIQUE_PART_INCOMPAT (E_US_MASK + 0x1948)
#define			E_US1949_6467_PART_TOOMANYP	(E_US_MASK + 0x1949)
#define			E_US1950_6468_NO_PARTRULE_SPEC  (E_US_MASK + 0x1950)
#define			E_US1951_6481_PART_ONEP		(E_US_MASK + 0x1951)
/* **** End of the group that has bad E_USxxxx numbering. */
#define			E_US1962_6498_X_REQUIRES_Y	(E_US_MASK + 0x1962)
#define			E_US1963_6499_X_ONLYWITH_Y	(E_US_MASK + 0x1963)

/* More USER/PRIVILEGE errors */
#define			E_US1967_6503_DEF_PRIVS_NSUB	(E_US_MASK + 0x1967L)
#define			E_US1968_6504_UNOPRIV_WDEFAULT	(E_US_MASK + 0x1968L)

/* Errors related to translation of identifiers */
#define			E_US_IDERR			(E_US_MASK + 0x1A10L)
#define			E_US1A11_ID_ANSI_TOO_LONG	(E_US_IDERR + 0x01L)
#define			E_US1A12_ID_ANSI_END		(E_US_IDERR + 0x02L)
#define			E_US1A13_ID_ANSI_BODY		(E_US_IDERR + 0x03L)
#define			E_US1A14_ID_ANSI_START		(E_US_IDERR + 0x04L)
#define			E_US_ID_MAXWARN			(E_US_ID_ANSI_START)
#define			E_US1A20_ID_START		(E_US_IDERR + 0x10L)
#define			E_US1A21_ID_DLM_BODY		(E_US_IDERR + 0x11L)
#define			E_US1A22_ID_BODY		(E_US_IDERR + 0x12L)
#define			E_US1A23_ID_DLM_END		(E_US_IDERR + 0x13L)
#define			E_US1A24_ID_TOO_SHORT		(E_US_IDERR + 0x14L)
#define			E_US1A25_ID_TOO_LONG		(E_US_IDERR + 0x15L)
#define			E_US1A26_CU_ID_PARAM_ERROR	(E_US_IDERR + 0x16L)

/*  [@defined_constants@]...	*/


/*
**      User errors from SCF
**      These are primarily used to inform users that some operation didn't
**      work and that they should call the product vendor.
*/
#define			E_US1265_QEP_INVALID	(E_US_MASK + 0x1265L)


/*
**      STAR errors.
*/
#define                 E_US13FE_NO_STAR_SUPP   (E_US_MASK + 0x13feL)
/*
** 	Misc errors
*/
#define			E_US10F0_4336		 	 (E_US_MASK + 0x10F0)
#define			E_US18E7_6375		 	 (E_US_MASK + 0x18E7)
#define			E_US246E_9326_NEEDS_B1		 (E_US_MASK + 0x246E)
#define			E_US246F_9327_SESS_LBL_NO_PRIV   (E_US_MASK + 0x246F)
#define			E_US2470_9328_DEF_LBL_ERROR      (E_US_MASK + 0x2470)
#define			E_US2471_9329_INVLD_SET_PRIV     (E_US_MASK + 0x2471)
#define		        E_US2472_9330_ALARM_EXISTS       (E_US_MASK + 0x2472)
#define		        E_US2473_9331_ALARM_ABSENT       (E_US_MASK + 0x2473)
#define		        E_US2475_9333_IISECALARM_OPEN    (E_US_MASK + 0x2475)
#define		        E_US2476_9334_IISECALARM_ERROR   (E_US_MASK + 0x2476)
#define		        E_US2477_9335_ALARM_EVENT_ERR    (E_US_MASK + 0x2477)
#define		        E_US2478_9336_BAD_ALARM_GRANTEE  (E_US_MASK + 0x2478)
#define		        E_US2479_9337_ALARM_USES_EVENT   (E_US_MASK + 0x2479)
#define		        E_US247A_9338_MISSING_ALM_EVENT  (E_US_MASK + 0x247A)
#define                 E_US247B_9339_IIROLEGRANT_OPEN   (E_US_MASK + 0x247B)
#define                 E_US247C_9340_IIROLEGRANT_ERROR  (E_US_MASK + 0x247C)
#define                 E_US247D_9341_GRANTEE_ROLE_TYPE  (E_US_MASK + 0x247D)
#define                 E_US247E_9342_ROLE_RG_DBDB       (E_US_MASK + 0x247E)
#define                 E_US247F_9343_ROLE_RG_NOT_AUTH   (E_US_MASK + 0x247F)

#define                 E_US248A_9354_MODIFY_TO_TABLE_OPTION (E_US_MASK + 0x248A)
/*[@group_of_defined_constants@]...*/

