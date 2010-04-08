/*
** Copyright (c) 2003, 2009 Ingres Corporation. All Rights Reserved.
*/

namespace Ingres.Utility
{
	/*
	** Name: gcferr.cs
	**
	** Description:
	**	Defines interface providing GCF error codes and messages.
	**
	**  classes
	**
	**	GcfErr
	**
	** History:
	**	10-Sep-99 (gordy)
	**	    Created.
	**	29-Sep-99 (gordy)
	**	    Added support for BLOBs: E_JD000B_BLOB_IO,
	**	    E_JD000C_BLOB_ACTIVE, E_JD000D_BLOB_DONE.
	**	26-Oct-99 (gordy)
	**	    Added ODBC escape parsing: E_JD000E_UNMATCHED.
	**	16-Nov-99 (gordy)
	**	    Added query timeouts: E_JD000F_TIMEOUT, and the
	**	    associated API error E_AP0009_QUERY_CANCELLED.
	**	20-Dec-99 (rajus01)
	**	    Implemented DatabaseMetaData: E_JD0010_CATALOG_UNSUPPORTED,
	**	    E_JD0011_SQLTYPE_UNSUPPORTED.
	**	21-Dec-99 (gordy)
	**	    Added generic unsupported E_JD0012_UNSUPPORTED.
	**	25-Jan-00 (rajus01)
	**	    Added ResultsSet closed E_JD0013_RESULTSET_CLOSED.
	**	31-Jan-00 (rajus01)
	**	    Added column not found E_JD0014_INVALID_COLUMN_NAME.
	**	 4-Feb-00 (gordy)
	**	    Added sequencing error E_JD0015_SEQUENCING.
	**	13-Jun-00 (gordy)
	**	    Added procedure call syntax error E_JD0016_CALL_SYNTAX.
	**	15-Nov-00 (gordy)
	**	    Added support for 2.0 extensions.
	**	31-Jan-01 (gordy)
	**	    Added invalid date format E_JD0019_INVALID_DATE.
	**      29-Aug-01 (loera01) SIR 105641
	**          Added invalid output parameter: E_JD0020_INVALID_OUT_PARAM.
	**          Added no result set returned: E_JD0021_NO_RESULT_SET.
	**          Added result set not permitted: E_JD0022_RESULT_SET_NOT_PERMITTED.
	**	11-Sep-02 (gordy)
	**	    Moved to GCF.  Renamed to remove specific product reference.
	**	    Translated error codes into new GCF error codes.
	**	31-Oct-02 (gordy)
	**	    Changed naming conventions to be more compatible with Ingres.
	**	    Added error code constants.
	**	26-Dec-02 (gordy)
	**	    Added GC4009_BAD_CHARSET.
	**	14-Feb-03 (gordy)
	**	    Added GC401F_XACT_STATE, GC4020_NO_PARAM.
	**	 9-Dec-03 (thoda04)
	**	    Ported to .NET.
	**	21-jun-04 (thoda04)
	**	    Cleaned up code for Open Source.
	**	 1-Nov-03 (gordy)
	**	    Added GC4021_INVALID_ROW.
	**	10-May-05 (gordy)
	**	    Added GC4811_NO_STMT.
	**	 3-Nov-08 (gordy, ported by thoda04)
	**	    Added additional server error codes to provide local exceptions.
	**	12-may-09 (thoda04)
	**	    Added GC4207_CONNECTION_STRING_MISSING_PAREN.
	**	 9-dec-09 (thoda04)
	**	    Factor out Boolean string parsing into a
	**	    BooleanParse() for BOOLEAN support.
	*/


	/*
	** Name: GcfErr
	**
	** Description:
	**	Interface defining GCF error codes and messages.
	**
	**  Constants
	**
	**	GCF_ERR_CLASS				GCF error class.
	**	E_GCF_MASK				GCF error code mask.
	**
	**	E_GC4000_BAD_URL			GCF driver error codes.
	**	E_GC4001_CONNECT_ERR
	**	E_GC4002_PROTOCOL_ERR
	**	E_GC4003_CONNECT_FAIL
	**	E_GC4004_CONNECTION_CLOSED
	**	E_GC4005_SEQUENCING
	**	E_GC4006_TIMEOUT
	**	E_GC4007_BLOB_IO
	**	E_GC4008_SERVER_ABORT
	**	E_GC4009_BAD_CHARSET
	**	E_GC4010_PARAM_VALUE
	**	E_GC4011_INDEX_RANGE
	**	E_GC4012_INVALID_COLUMN_NAME
	**	E_GC4013_UNMATCHED
	**	E_GC4014_CALL_SYNTAX
	**	E_GC4015_INVALID_OUT_PARAM
	**	E_GC4016_RS_CHANGED
	**	E_GC4017_NO_RESULT_SET
	**	E_GC4018_RESULT_SET_NOT_PERMITTED
	**	E_GC4019_UNSUPPORTED
	**	E_GC401A_CONVERSION_ERR
	**	E_GC401B_INVALID_DATE
	**	E_GC401C_BLOB_DONE
	**	E_GC401D_RESULTSET_CLOSED
	**	E_GC401E_CHAR_ENCODE
	**	E_GC401F_XACT_STATE
	**	E_GC4020_NO_PARAM
	**
	**	E_GC480D_IDLE_LIMIT			GCF server error codes.
	**	E_GC480E_CLIENT_MAX
	**	E_GC4811_NO_STMT
	**	E_GC481E_SESS_ABORT
	**	E_GC481F_SVR_CLOSED
	**
	**	ERR_GC4000_BAD_URL			GCF error info objects.
	**	ERR_GC4001_CONNECT_ERR
	**	ERR_GC4002_PROTOCOL_ERR
	**	ERR_GC4003_CONNECT_FAIL
	**	ERR_GC4004_CONNECTION_CLOSED
	**	ERR_GC4005_SEQUENCING
	**	ERR_GC4006_TIMEOUT
	**	ERR_GC4007_BLOB_IO
	**	ERR_GC4008_SERVER_ABORT
	**	ERR_GC4009_BAD_CHARSET
	**	ERR_GC4010_PARAM_VALUE
	**	ERR_GC4011_INDEX_RANGE
	**	ERR_GC4012_INVALID_COLUMN_NAME
	**	ERR_GC4013_UNMATCHED
	**	ERR_GC4014_CALL_SYNTAX
	**	ERR_GC4015_INVALID_OUT_PARAM
	**	ERR_GC4016_RS_CHANGED
	**	ERR_GC4017_NO_RESULT_SET
	**	ERR_GC4018_RESULT_SET_NOT_PERMITTED
	**	ERR_GC4019_UNSUPPORTED
	**	ERR_GC401A_CONVERSION_ERR
	**	ERR_GC401B_INVALID_DATE
	**	ERR_GC401C_BLOB_DONE
	**	ERR_GC401D_RESULTSET_CLOSED
	**	ERR_GC401E_CHAR_ENCODE
	**	ERR_GC401F_XACT_STATE
	**	ERR_GC4020_NO_PARAM
	**	ERR_GC4021_INVALID_ROW
	**	ERR_GC480D_IDLE_LIMIT
	**	ERR_GC480E_CLIENT_MAX
	**	ERR_GC4811_NO_STMT
	**	ERR_GC481E_SESS_ABORT
	**	ERR_GC481F_SVR_CLOSED
	**
	** History:
	**	10-Sep-99 (gordy)
	**	    Created.
	**	29-Sep-99 (gordy)
	**	    Added support for BLOBs: E_JD000B_BLOB_IO,
	**	    E_JD000C_BLOB_ACTIVE, E_JD000D_BLOB_DONE.
	**	26-Oct-99 (gordy)
	**	    Added ODBC escape parsing: E_JD000E_UNMATCHED.
	**	16-Nov-99 (gordy)
	**	    Added query timeouts: E_JD000F_TIMEOUT, and the
	**	    associated API error E_AP0009_QUERY_CANCELLED.
	**	20-Dec-99 (rajus01)
	**	    Implemented DatabaseMetaData: E_JD0010_CATALOG_UNSUPPORTED,
	**	    E_JD0011_SQLTYPE_UNSUPPORTED.
	**	21-Dec-99 (gordy)
	**	    Added generic unsupported E_JD0012_UNSUPPORTED.
	**	25-Jan-00 (rajus01)
	**	    Added ResultsSet closed E_JD0013_RESULTSET_CLOSED.
	**	31-Jan-00 (rajus01)
	**	    Added column not found E_JD0014_INVALID_COLUMN_NAME.
	**	 4-Feb-00 (gordy)
	**	    Added sequencing error E_JD0015_SEQUENCING.
	**	13-Jun-00 (gordy)
	**	    Added procedure call syntax error E_JD0016_CALL_SYNTAX.
	**	15-Nov-00 (gordy)
	**	    Added E_JD0017_RS_CHANGED, E_JD0018_UNKNOWN_TIMEZONE.
	**	31-Jan-01 (gordy)
	**	    Added invalid date format E_JD0019_INVALID_DATE.
	**	11-Sep-02 (gordy)
	**	    Renamed to remove specific product reference.
	**	    Translated error codes into new GCF error codes.
	**	31-Oct-02 (gordy)
	**	    Added error code constants (E_GCXXXX_*) to be more consistent
	**	    with Ingres error coding standards.  The error message objects
	**	    now are of the form ERR_GCXXXX_*.
	**	26-Dec-02 (gordy)
	**	    Added GC4009_BAD_CHARSET.
	**	14-Feb-03 (gordy)
	**	    Added GC401F_XACT_STATE, GC4020_NO_PARAM.
	**	 1-Nov-03 (gordy)
	**	    Added GC4021_INVALID_ROW.
	**	10-May-05 (gordy)
	**	    Added GC4811_NO_STMT to test for server error condition.
	**	 3-Nov-08 (gordy, ported by thoda04)
	**	    Added additional server error codes to provide local exceptions:
	**	    E_GC480D_IDLE_LIMIT, E_GC480E_CLIENT_MAX, E_GC481E_SESS_ABORT, 
	**	    E_GC481F_SVR_CLOSED, ERR_GC480D_IDLE_LIMIT, ERR_GC480E_CLIENT_MAX, 
	**	    ERR_GC4811_NO_STMT, ERR_GC481E_SESS_ABORT, ERR_GC481F_SVR_CLOSED.
	*/

	internal class GcfErr : System.Object

	{
		const int	GCF_ERR_CLASS		= 12;
		const int	E_GCF_MASK   		= GCF_ERR_CLASS << 16;

		const int	E_GC4000_BAD_URL            = E_GCF_MASK | 0x4000;
		const int	E_GC4001_CONNECT_ERR        = E_GCF_MASK | 0x4001;
		const int	E_GC4002_PROTOCOL_ERR       = E_GCF_MASK | 0x4002;
		const int	E_GC4003_CONNECT_FAIL       = E_GCF_MASK | 0x4003;
		const int	E_GC4004_CONNECTION_CLOSED  = E_GCF_MASK | 0x4004;
		const int	E_GC4005_SEQUENCING         = E_GCF_MASK | 0x4005;
		const int	E_GC4006_TIMEOUT            = E_GCF_MASK | 0x4006;
		const int	E_GC4007_BLOB_IO            = E_GCF_MASK | 0x4007;
		public
		const int	E_GC4008_SERVER_ABORT       = E_GCF_MASK | 0x4008;
		const int	E_GC4009_BAD_CHARSET        = E_GCF_MASK | 0x4009;

		const int	E_GC4010_PARAM_VALUE        = E_GCF_MASK | 0x4010;
		const int	E_GC4011_INDEX_RANGE        = E_GCF_MASK | 0x4011;
		const int	E_GC4012_INVALID_COLUMN_NAME= E_GCF_MASK | 0x4012;
		const int	E_GC4013_UNMATCHED          = E_GCF_MASK | 0x4013;
		const int	E_GC4014_CALL_SYNTAX        = E_GCF_MASK | 0x4014;
		const int	E_GC4015_INVALID_OUT_PARAM  = E_GCF_MASK | 0x4015;
		const int	E_GC4016_RS_CHANGED         = E_GCF_MASK | 0x4016;
		const int	E_GC4017_NO_RESULT_SET      = E_GCF_MASK | 0x4017;
		const int	E_GC4018_RESULT_SET_NOT_PERMITTED= E_GCF_MASK | 0x4018;
		const int	E_GC4019_UNSUPPORTED        = E_GCF_MASK | 0x4019;
		const int	E_GC401A_CONVERSION_ERR     = E_GCF_MASK | 0x401A;
		const int	E_GC401B_INVALID_DATE       = E_GCF_MASK | 0x401B;
		const int	E_GC401C_BLOB_DONE          = E_GCF_MASK | 0x401C;
		const int	E_GC401D_RESULTSET_CLOSED   = E_GCF_MASK | 0x401D;
		const int	E_GC401E_CHAR_ENCODE        = E_GCF_MASK | 0x401E;
		const int	E_GC401F_XACT_STATE         = E_GCF_MASK | 0x401F;
		const int	E_GC4020_NO_PARAM           = E_GCF_MASK | 0x4020;
		const int	E_GC4021_INVALID_ROW        = E_GCF_MASK | 0x4021;

		const int	E_GC480D_IDLE_LIMIT         = E_GCF_MASK | 0x480D;
		public
		const int E_GC480E_CLIENT_MAX           = E_GCF_MASK | 0x480E;
		public
		const int	E_GC4811_NO_STMT            = E_GCF_MASK | 0x4811;
		const int	E_GC481E_SESS_ABORT         = E_GCF_MASK | 0x481E;
		public
		const int E_GC481F_SVR_CLOSED           = E_GCF_MASK | 0x481F;

		const int	E_GC4201_CONNECTION_STRING_BAD_KEY      = E_GCF_MASK | 0x4201;
		const int	E_GC4202_CONNECTION_STRING_BAD_VALUE    = E_GCF_MASK | 0x4202;
		const int	E_GC4203_CONNECTION_STRING_BAD_QUOTE    = E_GCF_MASK | 0x4203;
		const int	E_GC4204_CONNECTION_STRING_DUP_EQUAL    = E_GCF_MASK | 0x4204;
		const int	E_GC4205_CONNECTION_STRING_MISSING_KEY  = E_GCF_MASK | 0x4205;
		const int	E_GC4206_CONNECTION_STRING_MISSING_QUOTE= E_GCF_MASK | 0x4206;
		const int	E_GC4207_CONNECTION_STRING_MISSING_PAREN= E_GCF_MASK | 0x4207;


		internal readonly static SqlEx.ErrInfo	ERR_GC4000_BAD_URL = 
			new SqlEx.ErrInfo( E_GC4000_BAD_URL, "50000", 
			ErrRsrc.E_GC4000, "E_GC4000_BAD_URL" );

		internal readonly static SqlEx.ErrInfo	ERR_GC4001_CONNECT_ERR = 
			new SqlEx.ErrInfo( E_GC4001_CONNECT_ERR, "08001", 
			ErrRsrc.E_GC4001, "E_GC4001_CONNECT_ERR" );

		internal readonly static SqlEx.ErrInfo	ERR_GC4002_PROTOCOL_ERR = 
			new SqlEx.ErrInfo( E_GC4002_PROTOCOL_ERR, "40003", 
			ErrRsrc.E_GC4002, "E_GC4002_PROTOCOL_ERR" );

		internal readonly static SqlEx.ErrInfo	ERR_GC4003_CONNECT_FAIL = 
			new SqlEx.ErrInfo( E_GC4003_CONNECT_FAIL, "40003", 
			ErrRsrc.E_GC4003, "E_GC4003_CONNECT_FAIL" );

		internal readonly static SqlEx.ErrInfo	ERR_GC4004_CONNECTION_CLOSED = 
			new SqlEx.ErrInfo( E_GC4004_CONNECTION_CLOSED, "08003", 
			ErrRsrc.E_GC4004, "E_GC4004_CONNECTION_CLOSED" );

		internal readonly static SqlEx.ErrInfo	ERR_GC4005_SEQUENCING = 
			new SqlEx.ErrInfo( E_GC4005_SEQUENCING, "5000R", 
			ErrRsrc.E_GC4005, "E_GC4005_SEQUENCING" );

		internal readonly static SqlEx.ErrInfo	ERR_GC4006_TIMEOUT = 
			new SqlEx.ErrInfo( E_GC4006_TIMEOUT, "5000R", 
			ErrRsrc.E_GC4006, "E_GC4006_TIMEOUT" );

		internal readonly static SqlEx.ErrInfo	ERR_GC4007_BLOB_IO = 
			new SqlEx.ErrInfo( E_GC4007_BLOB_IO, "50000", 
			ErrRsrc.E_GC4007, "E_GC4007_BLOB_IO" );

		internal readonly static SqlEx.ErrInfo	ERR_GC4008_SERVER_ABORT = 
			new SqlEx.ErrInfo( E_GC4008_SERVER_ABORT, "40003", 
			ErrRsrc.E_GC4008, "E_GC4008_SERVER_ABORT" );

		internal readonly static SqlEx.ErrInfo	ERR_GC4009_BAD_CHARSET = 
			new SqlEx.ErrInfo( E_GC4009_BAD_CHARSET, "50000", 
			ErrRsrc.E_GC4009, "E_GC4009_BAD_CHARSET" );

		internal readonly static SqlEx.ErrInfo	ERR_GC4010_PARAM_VALUE = 
			new SqlEx.ErrInfo( E_GC4010_PARAM_VALUE, "22023", 
			ErrRsrc.E_GC4010, "E_GC4010_PARAM_VALUE" );

		internal readonly static SqlEx.ErrInfo	ERR_GC4011_INDEX_RANGE = 
			new SqlEx.ErrInfo( E_GC4011_INDEX_RANGE, "50000", 
			ErrRsrc.E_GC4011, "E_GC4011_INDEX_RANGE" );

		internal readonly static SqlEx.ErrInfo 	ERR_GC4012_INVALID_COLUMN_NAME = 
			new SqlEx.ErrInfo( E_GC4012_INVALID_COLUMN_NAME, "5000R", 
			ErrRsrc.E_GC4012, "E_GC4012_INVALID_COLUMN_NAME" );

		internal readonly static SqlEx.ErrInfo	ERR_GC4013_UNMATCHED = 
			new SqlEx.ErrInfo( E_GC4013_UNMATCHED, "42000", 
			ErrRsrc.E_GC4013, "E_GC4013_UNMATCHED" );

		internal readonly static SqlEx.ErrInfo	ERR_GC4014_CALL_SYNTAX = 
			new SqlEx.ErrInfo( E_GC4014_CALL_SYNTAX, "42000", 
			ErrRsrc.E_GC4014, "E_GC4014_CALL_SYNTAX" );

		internal readonly static SqlEx.ErrInfo	ERR_GC4015_INVALID_OUT_PARAM = 
			new SqlEx.ErrInfo( E_GC4015_INVALID_OUT_PARAM, "22023", 
			ErrRsrc.E_GC4015, "E_GC4015_INVALID_OUT_PARAM" );

		internal readonly static SqlEx.ErrInfo	ERR_GC4016_RS_CHANGED = 
			new SqlEx.ErrInfo( E_GC4016_RS_CHANGED, "22023", 
			ErrRsrc.E_GC4016, "E_GC4016_RS_CHANGED" );

		internal readonly static SqlEx.ErrInfo	ERR_GC4017_NO_RESULT_SET = 
			new SqlEx.ErrInfo( E_GC4017_NO_RESULT_SET, "07005", 
			ErrRsrc.E_GC4017, "E_GC4017_NO_RESULT_SET" );

		internal readonly static SqlEx.ErrInfo	ERR_GC4018_RESULT_SET_NOT_PERMITTED = 
			new SqlEx.ErrInfo( E_GC4018_RESULT_SET_NOT_PERMITTED, "21000", 
			ErrRsrc.E_GC4018, "E_GC4018_RESULT_SET_NOT_PERMITTED" );

		internal readonly static SqlEx.ErrInfo	ERR_GC4019_UNSUPPORTED = 
			new SqlEx.ErrInfo( E_GC4019_UNSUPPORTED, "5000R", 
			ErrRsrc.E_GC4019, "E_GC4019_UNSUPPORTED" );

		internal readonly static SqlEx.ErrInfo	ERR_GC401A_CONVERSION_ERR = 
			new SqlEx.ErrInfo( E_GC401A_CONVERSION_ERR, "07006", 
			ErrRsrc.E_GC401A, "E_GC401A_CONVERSION_ERR" );

		internal readonly static SqlEx.ErrInfo	ERR_GC401B_INVALID_DATE = 
			new SqlEx.ErrInfo( E_GC401B_INVALID_DATE, "22007", 
			ErrRsrc.E_GC401B, "E_GC401B_INVALID_DATE" );

		internal readonly static SqlEx.ErrInfo	ERR_GC401C_BLOB_DONE = 
			new SqlEx.ErrInfo( E_GC401C_BLOB_DONE, "50000", 
			ErrRsrc.E_GC401C, "E_GC401C_BLOB_DONE" );

		internal readonly static SqlEx.ErrInfo	ERR_GC401D_RESULTSET_CLOSED = 
			new SqlEx.ErrInfo( E_GC401D_RESULTSET_CLOSED, "5000R", 
			ErrRsrc.E_GC401D, "E_GC401D_RESULTSET_CLOSED" );

		internal readonly static SqlEx.ErrInfo	ERR_GC401E_CHAR_ENCODE = 
			new SqlEx.ErrInfo( E_GC401E_CHAR_ENCODE, "07006", 
			ErrRsrc.E_GC401E, "E_GC401E_CHAR_ENCODE" );

		internal readonly static SqlEx.ErrInfo	ERR_GC401F_XACT_STATE = 
			new SqlEx.ErrInfo( E_GC401F_XACT_STATE, "25000", 
			ErrRsrc.E_GC401F, "E_GC401F_XACT_STATE" );

		internal readonly static SqlEx.ErrInfo	ERR_GC4020_NO_PARAM = 
			new SqlEx.ErrInfo( E_GC4020_NO_PARAM, "07001", 
			ErrRsrc.E_GC4020, "E_GC4020_NO_PARAM" );

		internal readonly static SqlEx.ErrInfo ERR_GC4021_INVALID_ROW =
			new SqlEx.ErrInfo(E_GC4021_INVALID_ROW, "24000",
			ErrRsrc.E_GC4021, "E_GC4021_INVALID_ROW");

		internal readonly static SqlEx.ErrInfo ERR_GC480D_IDLE_LIMIT =
			new SqlEx.ErrInfo(E_GC480D_IDLE_LIMIT, "50000",
			ErrRsrc.E_GC480D, "E_GC480D_IDLE_LIMIT");

		internal readonly static SqlEx.ErrInfo ERR_GC480E_CLIENT_MAX =
			new SqlEx.ErrInfo(E_GC480E_CLIENT_MAX, "08004",
			ErrRsrc.E_GC480E, "E_GC480E_CLIENT_MAX");

		internal readonly static SqlEx.ErrInfo ERR_GC4811_NO_STMT =
			new SqlEx.ErrInfo(E_GC4811_NO_STMT, "50000",
			ErrRsrc.E_GC4811, "E_GC4811_NO_STMT");

		internal readonly static SqlEx.ErrInfo ERR_GC481E_SESS_ABORT =
			new SqlEx.ErrInfo(E_GC481E_SESS_ABORT, "50000",
			ErrRsrc.E_GC481E, "E_GC481E_SESS_ABORT");

		internal readonly static SqlEx.ErrInfo ERR_GC481F_SVR_CLOSED =
			new SqlEx.ErrInfo(E_GC481F_SVR_CLOSED, "50000",
			ErrRsrc.E_GC481F, "E_GC481F_SVR_CLOSED");



		internal readonly static SqlEx.ErrInfo	ERR_GC4201_CONNECTION_STRING_BAD_KEY =
			new SqlEx.ErrInfo(E_GC4201_CONNECTION_STRING_BAD_KEY, "42000",
			ErrRsrc.E_GC4201, "E_GC4201_CONNECTION_STRING_BAD_KEY");

		internal readonly static SqlEx.ErrInfo	ERR_GC4202_CONNECTION_STRING_BAD_VALUE =
			new SqlEx.ErrInfo(E_GC4202_CONNECTION_STRING_BAD_VALUE, "42000",
			ErrRsrc.E_GC4202, "E_GC4202_CONNECTION_STRING_BAD_VALUE");

		internal readonly static SqlEx.ErrInfo	ERR_GC4203_CONNECTION_STRING_BAD_QUOTE =
			new SqlEx.ErrInfo(E_GC4203_CONNECTION_STRING_BAD_QUOTE, "42000",
			ErrRsrc.E_GC4203, "E_GC4203_CONNECTION_STRING_BAD_QUOTE");

		internal readonly static SqlEx.ErrInfo	ERR_GC4204_CONNECTION_STRING_DUP_EQUAL =
			new SqlEx.ErrInfo(E_GC4204_CONNECTION_STRING_DUP_EQUAL, "42000",
			ErrRsrc.E_GC4204, "E_GC4204_CONNECTION_STRING_DUP_EQUAL");

		internal readonly static SqlEx.ErrInfo	ERR_GC4205_CONNECTION_STRING_MISSING_KEY =
			new SqlEx.ErrInfo(E_GC4205_CONNECTION_STRING_MISSING_KEY, "42000",
			ErrRsrc.E_GC4205, "E_GC4205_CONNECTION_STRING_MISSING_KEY");

		internal readonly static SqlEx.ErrInfo	ERR_GC4206_CONNECTION_STRING_MISSING_QUOTE =
			new SqlEx.ErrInfo(E_GC4206_CONNECTION_STRING_MISSING_QUOTE, "42000",
			ErrRsrc.E_GC4206, "E_GC4206_CONNECTION_STRING_MISSING_QUOTE");

		internal readonly static SqlEx.ErrInfo	ERR_GC4207_CONNECTION_STRING_MISSING_PAREN =
			new SqlEx.ErrInfo(E_GC4207_CONNECTION_STRING_MISSING_PAREN, "42000",
			ErrRsrc.E_GC4207, "E_GC4207_CONNECTION_STRING_MISSING_PAREN");


		internal readonly static SqlEx.ErrInfo	ERR_AP0009_QUERY_CANCELLED =
			new SqlEx.ErrInfo(DbmsConst.E_AP0009_QUERY_CANCELLED, "5000R",
			ErrRsrc.E_AP0009, "E_AP0009_QUERY_CANCELLED");


		internal readonly static System.Globalization.CultureInfo InvariantCulture =
			System.Globalization.CultureInfo.InvariantCulture;

		/// <summary>
		/// Parse the string into a decimal number using
		/// System.Globalization.CultureInfo.InvariantCulture
		/// to avoid decimal point is comma confusion.
		/// </summary>
		/// <param name="str"></param>
		/// <returns></returns>
		internal static decimal DecimalParseInvariant(string str)
		{
			return System.Decimal.Parse(str, InvariantCulture);
		}

		/// <summary>
		/// Parse the string into a DateTime using
		/// System.Globalization.CultureInfo.InvariantCulture.
		/// </summary>
		/// <param name="str"></param>
		/// <returns></returns>
		internal static System.DateTime DateTimeParseInvariant(string str)
		{
			return System.DateTime.Parse(str, InvariantCulture);
		}

		/// <summary>
		/// Parse the string into a Boolean.
		/// Accept "True", "False", "0", and "1", case insensitive.
		/// Return false if null.
		/// </summary>
		/// <param name="str"></param>
		/// <returns></returns>
		internal static System.Boolean BooleanParse(string str)
		{
			if (str == null) return false;

			str = ToInvariantLower(str.Trim());
			if (str.Equals("0")) return false;
			if (str.Equals("1")) return true;

			return System.Boolean.Parse(str);
		}

		internal static string ToInvariantLower(string str)
		{
			if (str == null)
				return null;
			return str.ToLower(InvariantCulture);
		}

		internal static string ToInvariantUpper(string str)
		{
			if (str == null)
				return null;
			return str.ToUpper(InvariantCulture);
		}

		internal static char ToInvariantLower(char c)
		{
			return System.Char.ToLower(c, InvariantCulture);
		}

		internal static char ToInvariantUpper(char c)
		{
			return System.Char.ToUpper(c, InvariantCulture);
		}

		public static string toHexString(int val)
		{
			return toHexStringTrim(val.ToString("x8"));
		}

		public static string toHexString(short val)
		{
			return toHexStringTrim(val.ToString("x4"));
		}

		public static string toHexString(byte val)
		{
			return val.ToString("x2");
		}

		/// <summary>
		/// Trim leading zeros from the string.
		/// </summary>
		/// <param name="str"></param>
		private static string toHexStringTrim(string str)
		{
			int len = str.Length - 2;
			int i;
			for (i = 0; i < len; i+=2)
			{
				if (str[i] == '0'  &&  str[i+1] == '0')
					continue;
			}
			return str.Substring(i, str.Length - i);
		}


	} // GcfErr

}  // namespace
