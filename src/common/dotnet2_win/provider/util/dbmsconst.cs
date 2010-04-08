/*
** Copyright (c) 1999, 2010 Ingres Corporation. All Rights Reserved.
*/

namespace Ingres.Utility
{
	using System;

	/*
	** Name: dbmsconst.cs
	**
	** Description:
	**	Defines constants for DBMS Servers.
	**
	** History:
	**	 8-Sep-99 (gordy)
	**	    Created.
	**	18-Apr-00 (gordy)
	**	    Extracted DBMS constants from AdvanMsgConst.
	**	10-May-01 (gordy)
	**	    Added UCS2 datatypes.
	**	20-Feb-01 (gordy)
	**	    Added DBMS protocol levels.
	**	11-Sep-02 (gordy)
	**	    Moved to GCF.
	**	31-Oct-02 (gordy)
	**	    Added constants for dbmsinfo(), iidbcapabilities and AIF errors.
	**	20-Mar-03 (gordy)
	**	    Added DBMS_DBCAP_SAVEPOINT.
	**	 3-Jul-03 (gordy)
	**	    Added DBMS_TYPE_LOGKEY and DBMS_TYPE_TBLKEY.
	**	24-Jul-02 (thoda04)
	**	    Ported to .NET Provider.
	**	15-Mar-04 (gordy)
	**	    Support BIGINT.
	**	21-jun-04 (thoda04)
	**	    Cleaned up code for Open Source.
	**	28-May-04 (gordy)
	**	    Add max column lengths for NCS columns.
	**	10-May-05 (gordy)
	**	    Added DBMS_DBINFO_SERVER.
	**	16-Jun-06 (gordy)
	**	    ANSI Date/Time support.
	**	14-Sep-06 (lunbr01) 
	**	    Add DBMS_SQL_LEVEL_IMS_VSAM to identify EDBC IMS & VSAM SQL
	**	    restrictions.
	**	22-Sep-06 (gordy)
	**	    Updated keywords for Ingres and ANSI date types.
	**	22-Mar-07 (thoda04)
	**	    Simplified keywords for ANSI timestamp and time types.
	**	15-Sep-08 (gordy, ported by thoda04)
	**	    Added max decimal precision as returned by DBMS.
	**	 7-Dec-09 (gordy, ported by thoda04)
	**	    Boolean support.
	**	 3-Mar-10 (thoda04)  SIR 123368
	**	    Added support for IngresType.IngresDate parameter data type.
	*/


	/*
	** Name: DbmsConst
	**
	** Description:
	**	Interface defining Ingres DBMS constants.
	**
	**  Constants:
	**
	**	DBMS_API_PROTO_0	Base DBMS API protocol level.
	**	DBMS_API_PROTO_1	DBMS API protocol: decimal, byte, varbyte
	**	DBMS_API_PROTO_2	DBMS API protocol: nchar, nvarchar
	**	DBMS_API_PROTO_3	DBMS API protocol: bigint
	**	DBMS_API_PROTO_4	DBMS API protocol: ANSI date/time
	**	DBMS_API_PROTO_5	DBMS API protocol: LOB Locators
	**	DBMS_API_PROTO_6	DBMS API protocol: boolean
	**
	**	DBMS_COL_MAX		Max length of CHAR/VARCHAR column.
	**	DBMS_PREC_MAX		Max decimal precision.
	**	DBMS_OI10_IDENT_LEN	Current identifier length,
	**	DBMS_ING64_IDENT_LEN	Old identifier length.
	**	DBMS_TBLS_INSEL		Max tables in select.
	**
	**	DBMS_DBINFO_VERSION	Entries for dbmsinfo().
	**	DBMS_DBINFO_SERVRE
	**
	**	DBMS_DBCAP_DBMS_TYPE	Entries for iidbcapabilities.
	**	DBMS_DBCAP_ING_SQL_LVL
	**	DBMS_DBCAP_OPEN_SQL_LVL
	**	DBMS_DBCAP_STD_CAT_LVL
	**	DBMS_DBCAP_NAME_CASE
	**	DBMS_DBCAP_DELIM_CASE
	**	DBMS_DBCAP_ESCAPE
	**	DBMS_DBCAP_ESCAPE_CHAR
	**	DBMS_DBCAP_IDENT_CHAR
	**	DBMS_DBCAP_NULL_SORTING
	**	DBMS_DBCAP_OSQL_DATES
	**	DBMS_DBCAP_OUTER_JOIN
	**	DBMS_DBCAP_OWNER_NAME
	**	DBMS_DBCAP_SAVEPOINT
	**	DBMS_DBCAP_SQL92_LEVEL
	**	DBMS_DBCAP_UCS2_TYPES
	**	DBMS_DBCAP_UNION
	**	DBMS_DBCAP_MAX_SCH_NAME
	**	DBMS_DBCAP_MAX_USR_NAME
	**	DBMS_DBCAP_MAX_TAB_NAME;
	**	DBMS_DBCAP_MAX_COL_NAME
	**	DBMS_DBCAP_MAX_BYT_LIT
	**	DBMS_DBCAP_MAX_CHR_LIT
	**	DBMS_DBCAP_MAX_BYT_COL
	**	DBMS_DBCAP_MAX_VBY_COL
	**	DBMS_DBCAP_MAX_CHR_COL
	**	DBMS_DBCAP_MAX_VCH_COL
	**	DBMS_DBCAP_MAX_NCHR_COL
	**	DBMS_DBCAP_MAX_NVCH_COL
	**	DBMS_DBCAP_MAX_DEC_PREC
	**	DBMS_DBCAP_MAX_ROW_LEN
	**	DBMS_DBCAP_MAX_COLUMNS
	**	DBMS_DBCAP_MAX_STMTS
	**
	**	DBMS_TYPE_INGRES	Values for iidbcapabilities.
	**	DBMS_SQL_LEVEL_IMS_VSAM
	**	DBMS_SQL_LEVEL_OI10
	**	DBMS_SQL_LEVEL_ANSI_DT
	**	DBMS_NAME_CASE_LOWER
	**	DBMS_NAME_CASE_MIXED
	**	DBMS_NAME_CASE_UPPER
	**	DBMS_NULL_SORT_FIRST
	**	DBMS_NULL_SORT_LAST
	**	DBMS_NULL_SORT_HIGH
	**	DBMS_NULL_SORT_LOW
	**	DBMS_SQL92_NONE
	**	DBMS_SQL92_ENTRY
	**	DBMS_SQL92_FIPS
	**	DBMS_SQL92_INTERMEDIATE
	**	DBMS_SQL92_FULL
	**	DBMS_UNION_ALL
	**
	**	DBMS_TYPE_IDATE		Ingres Date datatype.
	**	DBMS_TYPE_INGRESDATE Ingres Date datatype if ANSI supported.
	**	DBMS_TYPE_MONEY		Money datatype.
	**	DBMS_TYPE_DECIMAL	Decimal datatype.
	**	DBMS_TYPE_CHAR		Char datatype.
	**	DBMS_TYPE_VARCHAR	Varchar datatype.
	**	DBMS_TYPE_LONG_CHAR	Long varchar datatype.
	**	DBMS_TYPE_BYTE		Byte datatype.
	**	DBMS_TYPE_VARBYTE	Byte varying datatype.
	**	DBMS_TYPE_LONG_BYTE	Long byte datatype.
	**	DBMS_TYPE_NCHAR		NChar datatype.
	**	DBMS_TYPE_NVARCHAR	NVarchar datatype.
	**	DBMS_TYPE_LONG_NCHAR	Long nvarchar datatype.
	**	DBMS_TYPE_INT		Integer datatype.
	**	DBMS_TYPE_FLOAT		Float datatype.
	**	DBMS_TYPE_C		'C' datatype.
	**	DBMS_TYPE_TEXT		Text datatype.
	**	DBMS_TYPE_BOOL		Boolean
	**	DBMS_TYPE_LONG_TEXT	Long text datatype.
	**
	**	AIF_ERR_CLASS		AIF Error code info.
	**	E_AIF_MASK
	**	E_AP0009_QUERY_CANCELLED
	**
	**  Public Data:
	**
	**	typeMap			Mapping table for most DBMS datatypes.
	**	intMap			Mapping table for DBMS integer types.
	**	floatMap		Mapping table for DBMS float types.
	**
	** History:
	**	 8-Sep-99 (gordy)
	**	    Created.
	**	11-Nov-99 (rajus01)
	**	    Implement DatabaseMetaData: Add DBMS_NAME_LEN, DBMS_TBLS_INSEL. 
	**	10-May-01 (gordy)
	**	    Added UCS2 datatypes: DBMS_TYPE_NCHAR, DBMS_TYPE_NVARCHAR,
	**	    DBMS_TYPE_LONG_NCHAR.
	**	20-Feb-01 (gordy)
	**	    Added DBMS protocol levels: DBMS_PROTO_[0,1,2].
	**	24-Jul-02 (thoda04)
	**	    Ported to .NET Provider.
	**	15-Mar-04 (gordy)
	**	    Added DBMS_API_PROTO_3 for BIGINT support.
	**	28-May-04 (gordy)
	**	    Add max column lengths for NCS columns.
	**	10-May-05 (gordy)
	**	    Added dbmsinfo key DBMS_DBINFO_SERVER.
	**	16-Jun-06 (gordy)
	**	    ANSI Date/Time support.
	**	15-Sep-08 (gordy, ported by thoda04)
	**	    Added max decimal precision: DBMS_DBCAP_MAX_DEC_PREC,
	**	    DBMS_PREC_MAX.
	**	 7-Dec-09 (gordy, ported by thoda04)
	**	    Boolean support.
	*/

	internal class DbmsConst : GcfErr
	{
		/*
		** Server/DBMS API protocol levels.
		*/
		public const byte	DBMS_API_PROTO_0	= 0;	// Base level.
		public const byte	DBMS_API_PROTO_1	= 1;	// Added decimal, byte, varbyte
		public const byte	DBMS_API_PROTO_2	= 2;	// Added nchar, nvarchar
		public const byte	DBMS_API_PROTO_3	= 3;	// Added bigint
		public const byte	DBMS_API_PROTO_4	= 4;	// Added ANSI date/time
		public const byte	DBMS_API_PROTO_5	= 5;	// Added LOB Locators
		public const byte	DBMS_API_PROTO_6	= 6;	// Added boolean

		/*
		** Misc constants.
		*/
		public const short  DBMS_COL_MAX         = 2000;  // Max col length.
		public const short  DBMS_PREC_MAX        = 31;    // Default max dec precision.
		public const short  DBMS_OI10_IDENT_LEN  = 32;    // Max identifier length,
		public const short  DBMS_ING64_IDENT_LEN = 20;    // Old identifier length.
		public const short  DBMS_TBLS_INSEL      = 30;    // Max tables in select.

		/*
		** Entries for dbmsinfo()
		*/
		public const String	DBMS_DBINFO_VERSION	= "_version";
		public const String	DBMS_DBINFO_SERVER = "ima_server";

		/*
		** Entries for iidbcapabilities.
		*/
		public const String	DBMS_DBCAP_DBMS_TYPE	= "DBMS_TYPE";
		public const String	DBMS_DBCAP_ING_SQL_LVL	= "INGRES/SQL_LEVEL";
		public const String	DBMS_DBCAP_OPEN_SQL_LVL	= "OPEN/SQL_LEVEL";
		public const String	DBMS_DBCAP_STD_CAT_LVL	= "STANDARD_CATALOG_LEVEL";
		public const String	DBMS_DBCAP_NAME_CASE	= "DB_NAME_CASE";
		public const String	DBMS_DBCAP_DELIM_CASE	= "DB_DELIMITED_CASE";
		public const String	DBMS_DBCAP_ESCAPE	= "ESCAPE";
		public const String	DBMS_DBCAP_ESCAPE_CHAR	= "ESCAPE_CHAR";
		public const String	DBMS_DBCAP_IDENT_CHAR	= "IDENT_CHAR";
		public const String	DBMS_DBCAP_NULL_SORTING	= "NULL_SORTING";
		public const String	DBMS_DBCAP_OSQL_DATES	= "OPEN_SQL_DATES";
		public const String	DBMS_DBCAP_OUTER_JOIN	= "OUTER_JOIN";
		public const String	DBMS_DBCAP_OWNER_NAME	= "OWNER_NAME";
		public const String	DBMS_DBCAP_SAVEPOINT	= "SAVEPOINTS";
		public const String	DBMS_DBCAP_SQL92_LEVEL	= "SQL92_LEVEL";
		public const String	DBMS_DBCAP_UCS2_TYPES	= "NATIONAL_CHARACTER_SET";
		public const String	DBMS_DBCAP_UNION	= "UNION";
		public const String	DBMS_DBCAP_MAX_SCH_NAME	= "SQL_MAX_SCHEMA_NAME_LEN";
		public const String	DBMS_DBCAP_MAX_USR_NAME	= "SQL_MAX_USER_NAME_LEN";
		public const String	DBMS_DBCAP_MAX_TAB_NAME	= "SQL_MAX_TABLE_NAME_LEN";
		public const String	DBMS_DBCAP_MAX_COL_NAME	= "SQL_MAX_COLUMN_NAME_LEN";
		public const String	DBMS_DBCAP_MAX_BYT_LIT	= "SQL_MAX_BYTE_LITERAL_LEN";
		public const String	DBMS_DBCAP_MAX_CHR_LIT	= "SQL_MAX_CHAR_LITERAL_LEN";
		public const String	DBMS_DBCAP_MAX_BYT_COL	= "SQL_MAX_BYTE_COLUMN_LEN";
		public const String	DBMS_DBCAP_MAX_VBY_COL	= "SQL_MAX_VBYT_COLUMN_LEN";
		public const String	DBMS_DBCAP_MAX_CHR_COL	= "SQL_MAX_CHAR_COLUMN_LEN";
		public const String	DBMS_DBCAP_MAX_VCH_COL	= "SQL_MAX_VCHR_COLUMN_LEN";
		public const String DBMS_DBCAP_MAX_NCHR_COL = "SQL_MAX_NCHAR_COLUMN_LEN";
		public const String DBMS_DBCAP_MAX_NVCH_COL = "SQL_MAX_NVCHR_COLUMN_LEN";
		public const String DBMS_DBCAP_MAX_DEC_PREC = "SQL_MAX_DECIMAL_PRECISION";
		public const String DBMS_DBCAP_MAX_ROW_LEN = "SQL_MAX_ROW_LEN";
		public const String	DBMS_DBCAP_MAX_COLUMNS	= "MAX_COLUMNS";
		public const String	DBMS_DBCAP_MAX_STMTS	= "SQL_MAX_STATEMENTS";

		/*
		** Value for iidbcapabilities.
		*/
		public const String	DBMS_TYPE_INGRES	= "INGRES";
		public const int   	DBMS_SQL_LEVEL_IMS_VSAM = 601;
		public const int   	DBMS_SQL_LEVEL_OI10     = 605;
		public const int   	DBMS_SQL_LEVEL_ANSI_DT  = 910;
		public const String DBMS_NAME_CASE_LOWER = "LOWER";
		public const String	DBMS_NAME_CASE_MIXED	= "MIXED";
		public const String	DBMS_NAME_CASE_UPPER	= "UPPER";
		public const String	DBMS_NULL_SORT_FIRST	= "FIRST";
		public const String	DBMS_NULL_SORT_LAST	= "LAST";
		public const String	DBMS_NULL_SORT_HIGH	= "HIGH";
		public const String	DBMS_NULL_SORT_LOW	= "LOW";
		public const String	DBMS_SQL92_NONE		= "NONE";
		public const String	DBMS_SQL92_ENTRY	= "ENTRY";
		public const String	DBMS_SQL92_FIPS		= "FIPS-IEF";
		public const String	DBMS_SQL92_INTERMEDIATE	= "INTERMEDIATE";
		public const String	DBMS_SQL92_FULL		= "FULL";
		public const String	DBMS_UNION_ALL		= "ALL";

		public const short DBMS_NAME_LEN = 32;  	// Max Identifier length

		public const short DBMS_TYPE_IDATE = 3;
		public const short DBMS_TYPE_INGRESDATE=103;
		public const short DBMS_TYPE_ADATE = 4;
		public const short DBMS_TYPE_MONEY = 5;
		public const short DBMS_TYPE_TMWO = 6;
		public const short DBMS_TYPE_TMTZ = 7;
		public const short DBMS_TYPE_TIME = 8;
		public const short DBMS_TYPE_TSWO = 9;
		public const short DBMS_TYPE_DECIMAL = 10;
		public const short DBMS_TYPE_TSTZ = 18;
		public const short DBMS_TYPE_TS = 19;
		public const short DBMS_TYPE_CHAR = 20;
		public const short DBMS_TYPE_VARCHAR = 21;
		public const short DBMS_TYPE_LONG_CHAR = 22;
		public const short DBMS_TYPE_BYTE = 23;
		public const short DBMS_TYPE_VARBYTE = 24;
		public const short DBMS_TYPE_LONG_BYTE = 25;
		public const short DBMS_TYPE_NCHAR = 26;
		public const short DBMS_TYPE_NVARCHAR = 27;
		public const short DBMS_TYPE_LONG_NCHAR = 28;
		public const short DBMS_TYPE_INT = 30;
		public const short DBMS_TYPE_FLOAT = 31;
		public const short DBMS_TYPE_C = 32;
		public const short DBMS_TYPE_INTYM = 33;
		public const short DBMS_TYPE_INTDS = 34;
		public const short DBMS_TYPE_TEXT = 37;
		public const short DBMS_TYPE_BOOL = 38;
		public const short DBMS_TYPE_LONG_TEXT = 41;

		public readonly static IdMap[] typeMap;
		public readonly static IdMap[] intMap;
		public readonly static IdMap[] floatMap;

		/*
		** AIF Error codes
		*/
		public const int		AIF_ERR_CLASS		= 201;
		public const int		E_AIF_MASK		= AIF_ERR_CLASS << 16;
		public const int		E_AP0009_QUERY_CANCELLED	= E_AIF_MASK | 0x0009;


		static DbmsConst()
		{
			typeMap = new IdMap[]
			{	new IdMap(DBMS_TYPE_IDATE,      "date"),
				new IdMap(DBMS_TYPE_INGRESDATE, "ingresdate" ),
				new IdMap(DBMS_TYPE_ADATE,      "ansidate" ),
				new IdMap(DBMS_TYPE_MONEY,      "money"),
				new IdMap(DBMS_TYPE_DECIMAL,    "decimal"),
				new IdMap(DBMS_TYPE_TMWO,       "time" ),      // without time zone
				new IdMap(DBMS_TYPE_TMTZ,       "time with time zone" ),
				new IdMap(DBMS_TYPE_TIME,       "time with local time zone" ),
				new IdMap(DBMS_TYPE_TSWO,       "timestamp" ), // without time zone
				new IdMap(DBMS_TYPE_TSTZ,       "timestamp with time zone" ),
				new IdMap(DBMS_TYPE_TS,         "timestamp with local time zone" ),
				new IdMap(DBMS_TYPE_CHAR,       "char"),
				new IdMap(DBMS_TYPE_VARCHAR,    "varchar"),
				new IdMap(DBMS_TYPE_LONG_CHAR,  "long varchar"),
				new IdMap(DBMS_TYPE_BYTE,       "byte"),
				new IdMap(DBMS_TYPE_VARBYTE,    "byte varying"),
				new IdMap(DBMS_TYPE_LONG_BYTE,  "long byte"),
				new IdMap(DBMS_TYPE_NCHAR,      "nchar"),
				new IdMap(DBMS_TYPE_NVARCHAR,   "nvarchar"),
				new IdMap(DBMS_TYPE_LONG_NCHAR, "long nvarchar"),
				new IdMap(DBMS_TYPE_C,          "c"),
				new IdMap(DBMS_TYPE_INTYM,      "interval year to month" ),
				new IdMap(DBMS_TYPE_INTDS,      "interval day to second" ),
				new IdMap(DBMS_TYPE_TEXT,       "text"),
				new IdMap(DBMS_TYPE_LONG_TEXT,  "long text"),
				new IdMap(DBMS_TYPE_BOOL,       "boolean")
			};
			intMap = new IdMap[]
			{
				new IdMap(1, "integer1"),
				new IdMap(2, "smallint"),
				new IdMap(4, "integer"),
				new IdMap(8, "bigint")
			};
			floatMap = new IdMap[]
			{
				new IdMap(4, "real"),
				new IdMap(8, "float8")
			};
		}
	} // DbmsConst

}