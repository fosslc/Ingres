/*
** Copyright (c) 1999, 2007 Ingres Corporation All Rights Reserved.
*/

package com.ingres.gcf.util;

/*
** Name: GcfErr.java
**
** Description:
**	Defines interface providing GCF error codes and messages.
**
**  Interfaces
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
**	    Added support for JDBC 2.0 extensions.
**	31-Jan-01 (gordy)
**	    Added invalid date format E_JD0019_INVALID_DATE.
**      29-Aug-01 (loera01) SIR 105641
**          Added invalid output parameter: E_JD0020_INVALID_OUT_PARAM.
**          Added no result set returned: E_JD0021_NO_RESULT_SET.
**          Added result set not permitted: E_JD0022_RESULT_SET_NOT_PERMITTED.
**	11-Sep-02 (gordy)
**	    Moved to GCF.  Renamed to remove specific product reference.
**	    Translated JDBC error codes into new GCF error codes.
**	31-Oct-02 (gordy)
**	    Changed naming conventions to be more compatible with Ingres.
**	    Added error code constants.
**	26-Dec-02 (gordy)
**	    Added GC4009_BAD_CHARSET.
**	14-Feb-03 (gordy)
**	    Added GC401F_XACT_STATE, GC4020_NO_PARAM.
**	 1-Nov-03 (gordy)
**	    Added GC4021_INVALID_ROW.
**	10-May-05 (gordy)
**	    Added GC4811_NO_STMT.
**	 5-Dec-07 (gordy)
**	    Added additional server error codes to provide local exceptions.
**	17-Nov-08 (rajus01) SIR 121238
**	    Replaced SqlEx references with SqlExFactory as SqlEx becomes 
**	    obsolete to support JDBC 4.0 SQLException hierarchy.
**	11-Jun-09 (rajus01) SIR 121238
**	    Added GC4022_LOB_FREED.
**	23-Jun-09 (rajus01) SIR 121238
**	    Added E_GC4023_NO_OBJECT.
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
**	ERR_GC4022_LOB_FREED
**	ERR_GC4023_NO_OBJECT
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
**	    Translated JDBC error codes into new GCF error codes.
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
**	 5-Dec-07 (gordy)
**	    Added additional server error codes to provide local exceptions:
**	    E_GC480D_IDLE_LIMIT, E_GC480E_CLIENT_MAX, E_GC481E_SESS_ABORT, 
**	    E_GC481F_SVR_CLOSED, ERR_GC480D_IDLE_LIMIT, ERR_GC480E_CLIENT_MAX, 
**	    ERR_GC4811_NO_STMT, ERR_GC481E_SESS_ABORT, ERR_GC481F_SVR_CLOSED.
*/

public interface
GcfErr
{

    int		GCF_ERR_CLASS		= 12;
    int		E_GCF_MASK		= GCF_ERR_CLASS << 16;

    int		E_GC4000_BAD_URL			= E_GCF_MASK | 0x4000;
    int		E_GC4001_CONNECT_ERR			= E_GCF_MASK | 0x4001;
    int		E_GC4002_PROTOCOL_ERR			= E_GCF_MASK | 0x4002;
    int		E_GC4003_CONNECT_FAIL			= E_GCF_MASK | 0x4003;
    int		E_GC4004_CONNECTION_CLOSED		= E_GCF_MASK | 0x4004;
    int		E_GC4005_SEQUENCING			= E_GCF_MASK | 0x4005;
    int		E_GC4006_TIMEOUT			= E_GCF_MASK | 0x4006;
    int		E_GC4007_BLOB_IO			= E_GCF_MASK | 0x4007;
    int		E_GC4008_SERVER_ABORT			= E_GCF_MASK | 0x4008;
    int		E_GC4009_BAD_CHARSET			= E_GCF_MASK | 0x4009;
    
    int		E_GC4010_PARAM_VALUE			= E_GCF_MASK | 0x4010;
    int		E_GC4011_INDEX_RANGE			= E_GCF_MASK | 0x4011;
    int		E_GC4012_INVALID_COLUMN_NAME		= E_GCF_MASK | 0x4012;
    int		E_GC4013_UNMATCHED			= E_GCF_MASK | 0x4013;
    int		E_GC4014_CALL_SYNTAX			= E_GCF_MASK | 0x4014;
    int		E_GC4015_INVALID_OUT_PARAM		= E_GCF_MASK | 0x4015;
    int		E_GC4016_RS_CHANGED			= E_GCF_MASK | 0x4016;
    int		E_GC4017_NO_RESULT_SET			= E_GCF_MASK | 0x4017;
    int		E_GC4018_RESULT_SET_NOT_PERMITTED	= E_GCF_MASK | 0x4018;
    int		E_GC4019_UNSUPPORTED			= E_GCF_MASK | 0x4019;
    int		E_GC401A_CONVERSION_ERR			= E_GCF_MASK | 0x401A;
    int		E_GC401B_INVALID_DATE			= E_GCF_MASK | 0x401B;
    int		E_GC401C_BLOB_DONE			= E_GCF_MASK | 0x401C;
    int		E_GC401D_RESULTSET_CLOSED		= E_GCF_MASK | 0x401D;
    int		E_GC401E_CHAR_ENCODE			= E_GCF_MASK | 0x401E;
    int		E_GC401F_XACT_STATE			= E_GCF_MASK | 0x401F;
    int		E_GC4020_NO_PARAM			= E_GCF_MASK | 0x4020;
    int		E_GC4021_INVALID_ROW			= E_GCF_MASK | 0x4021;
    int		E_GC4022_LOB_FREED			= E_GCF_MASK | 0x4022;
    int		E_GC4023_NO_OBJECT			= E_GCF_MASK | 0x4023;
    
    int		E_GC480D_IDLE_LIMIT			= E_GCF_MASK | 0x480D;
    int		E_GC480E_CLIENT_MAX			= E_GCF_MASK | 0x480E;
    int		E_GC4811_NO_STMT			= E_GCF_MASK | 0x4811;
    int		E_GC481E_SESS_ABORT			= E_GCF_MASK | 0x481E;
    int		E_GC481F_SVR_CLOSED			= E_GCF_MASK | 0x481F;

    SqlExFactory.ErrInfo	ERR_GC4000_BAD_URL = 
	new SqlExFactory.ErrInfo( E_GC4000_BAD_URL, "50000", 
			   ErrRsrc.E_GC4000, "E_GC4000_BAD_URL" );

    SqlExFactory.ErrInfo	ERR_GC4001_CONNECT_ERR = 
	new SqlExFactory.ErrInfo( E_GC4001_CONNECT_ERR, "08001", 
			   ErrRsrc.E_GC4001, "E_GC4001_CONNECT_ERR" );

    SqlExFactory.ErrInfo	ERR_GC4002_PROTOCOL_ERR = 
	new SqlExFactory.ErrInfo( E_GC4002_PROTOCOL_ERR, "40003", 
			   ErrRsrc.E_GC4002, "E_GC4002_PROTOCOL_ERR" );

    SqlExFactory.ErrInfo	ERR_GC4003_CONNECT_FAIL = 
	new SqlExFactory.ErrInfo( E_GC4003_CONNECT_FAIL, "40003", 
			   ErrRsrc.E_GC4003, "E_GC4003_CONNECT_FAIL" );

    SqlExFactory.ErrInfo	ERR_GC4004_CONNECTION_CLOSED = 
	new SqlExFactory.ErrInfo( E_GC4004_CONNECTION_CLOSED, "08003", 
			   ErrRsrc.E_GC4004, "E_GC4004_CONNECTION_CLOSED" );

    SqlExFactory.ErrInfo	ERR_GC4005_SEQUENCING = 
	new SqlExFactory.ErrInfo( E_GC4005_SEQUENCING, "5000R", 
			   ErrRsrc.E_GC4005, "E_GC4005_SEQUENCING" );

    SqlExFactory.ErrInfo	ERR_GC4006_TIMEOUT = 
	new SqlExFactory.ErrInfo( E_GC4006_TIMEOUT, "5000R", 
			   ErrRsrc.E_GC4006, "E_GC4006_TIMEOUT" );

    SqlExFactory.ErrInfo	ERR_GC4007_BLOB_IO = 
	new SqlExFactory.ErrInfo( E_GC4007_BLOB_IO, "50000", 
			   ErrRsrc.E_GC4007, "E_GC4007_BLOB_IO" );

    SqlExFactory.ErrInfo	ERR_GC4008_SERVER_ABORT = 
	new SqlExFactory.ErrInfo( E_GC4008_SERVER_ABORT, "40003", 
			   ErrRsrc.E_GC4008, "E_GC4008_SERVER_ABORT" );

    SqlExFactory.ErrInfo	ERR_GC4009_BAD_CHARSET = 
	new SqlExFactory.ErrInfo( E_GC4009_BAD_CHARSET, "50000", 
			   ErrRsrc.E_GC4009, "E_GC4009_BAD_CHARSET" );

    SqlExFactory.ErrInfo	ERR_GC4010_PARAM_VALUE = 
	new SqlExFactory.ErrInfo( E_GC4010_PARAM_VALUE, "22023", 
			   ErrRsrc.E_GC4010, "E_GC4010_PARAM_VALUE" );

    SqlExFactory.ErrInfo	ERR_GC4011_INDEX_RANGE = 
	new SqlExFactory.ErrInfo( E_GC4011_INDEX_RANGE, "50000", 
			   ErrRsrc.E_GC4011, "E_GC4011_INDEX_RANGE" );

    SqlExFactory.ErrInfo 	ERR_GC4012_INVALID_COLUMN_NAME = 
	new SqlExFactory.ErrInfo( E_GC4012_INVALID_COLUMN_NAME, "5000R", 
			   ErrRsrc.E_GC4012, "E_GC4012_INVALID_COLUMN_NAME" );

    SqlExFactory.ErrInfo	ERR_GC4013_UNMATCHED = 
	new SqlExFactory.ErrInfo( E_GC4013_UNMATCHED, "42000", 
			   ErrRsrc.E_GC4013, "E_GC4013_UNMATCHED" );

    SqlExFactory.ErrInfo	ERR_GC4014_CALL_SYNTAX = 
	new SqlExFactory.ErrInfo( E_GC4014_CALL_SYNTAX, "42000", 
			   ErrRsrc.E_GC4014, "E_GC4014_CALL_SYNTAX" );

    SqlExFactory.ErrInfo	ERR_GC4015_INVALID_OUT_PARAM = 
	new SqlExFactory.ErrInfo( E_GC4015_INVALID_OUT_PARAM, "22023", 
			   ErrRsrc.E_GC4015, "E_GC4015_INVALID_OUT_PARAM" );

    SqlExFactory.ErrInfo	ERR_GC4016_RS_CHANGED = 
	new SqlExFactory.ErrInfo( E_GC4016_RS_CHANGED, "22023", 
			   ErrRsrc.E_GC4016, "E_GC4016_RS_CHANGED" );

    SqlExFactory.ErrInfo	ERR_GC4017_NO_RESULT_SET = 
	new SqlExFactory.ErrInfo( E_GC4017_NO_RESULT_SET, "07005", 
			   ErrRsrc.E_GC4017, "E_GC4017_NO_RESULT_SET" );

    SqlExFactory.ErrInfo	ERR_GC4018_RESULT_SET_NOT_PERMITTED = 
	new SqlExFactory.ErrInfo( E_GC4018_RESULT_SET_NOT_PERMITTED, "21000", 
			   ErrRsrc.E_GC4018, "E_GC4018_RESULT_SET_NOT_PERMITTED" );

    SqlExFactory.ErrInfo	ERR_GC4019_UNSUPPORTED = 
	new SqlExFactory.ErrInfo( E_GC4019_UNSUPPORTED, "0A000", 
			   ErrRsrc.E_GC4019, "E_GC4019_UNSUPPORTED" );

    SqlExFactory.ErrInfo	ERR_GC401A_CONVERSION_ERR = 
	new SqlExFactory.ErrInfo( E_GC401A_CONVERSION_ERR, "07006", 
			   ErrRsrc.E_GC401A, "E_GC401A_CONVERSION_ERR" );

    SqlExFactory.ErrInfo	ERR_GC401B_INVALID_DATE = 
	new SqlExFactory.ErrInfo( E_GC401B_INVALID_DATE, "22007", 
			   ErrRsrc.E_GC401B, "E_GC401B_INVALID_DATE" );

    SqlExFactory.ErrInfo	ERR_GC401C_BLOB_DONE = 
	new SqlExFactory.ErrInfo( E_GC401C_BLOB_DONE, "50000", 
			   ErrRsrc.E_GC401C, "E_GC401C_BLOB_DONE" );

    SqlExFactory.ErrInfo	ERR_GC401D_RESULTSET_CLOSED = 
	new SqlExFactory.ErrInfo( E_GC401D_RESULTSET_CLOSED, "5000R", 
			   ErrRsrc.E_GC401D, "E_GC401D_RESULTSET_CLOSED" );

    SqlExFactory.ErrInfo	ERR_GC401E_CHAR_ENCODE = 
	new SqlExFactory.ErrInfo( E_GC401E_CHAR_ENCODE, "07006", 
			   ErrRsrc.E_GC401E, "E_GC401E_CHAR_ENCODE" );

    SqlExFactory.ErrInfo	ERR_GC401F_XACT_STATE = 
	new SqlExFactory.ErrInfo( E_GC401F_XACT_STATE, "25000", 
			   ErrRsrc.E_GC401F, "E_GC401F_XACT_STATE" );

    SqlExFactory.ErrInfo	ERR_GC4020_NO_PARAM = 
	new SqlExFactory.ErrInfo( E_GC4020_NO_PARAM, "07001", 
			   ErrRsrc.E_GC4020, "E_GC4020_NO_PARAM" );

    SqlExFactory.ErrInfo	ERR_GC4021_INVALID_ROW = 
	new SqlExFactory.ErrInfo( E_GC4021_INVALID_ROW, "24000", 
			   ErrRsrc.E_GC4021, "E_GC4021_INVALID_ROW" );

    SqlExFactory.ErrInfo	ERR_GC4022_LOB_FREED = 
	new SqlExFactory.ErrInfo( E_GC4022_LOB_FREED, "50000", 
			   ErrRsrc.E_GC4022, "E_GC4022_LOB_FREED" );

    SqlExFactory.ErrInfo	ERR_GC4023_NO_OBJECT = 
	new SqlExFactory.ErrInfo( E_GC4023_NO_OBJECT, "50000", 
			   ErrRsrc.E_GC4023, "E_GC4023_NO_OBJECT" );

    SqlExFactory.ErrInfo	ERR_GC480D_IDLE_LIMIT = 
	new SqlExFactory.ErrInfo( E_GC480D_IDLE_LIMIT, "50000", 
			   ErrRsrc.E_GC480D, "E_GC480D_IDLE_LIMIT" );

    SqlExFactory.ErrInfo	ERR_GC480E_CLIENT_MAX = 
	new SqlExFactory.ErrInfo( E_GC480E_CLIENT_MAX, "08004", 
			   ErrRsrc.E_GC480E, "E_GC480E_CLIENT_MAX" );

    SqlExFactory.ErrInfo	ERR_GC4811_NO_STMT = 
	new SqlExFactory.ErrInfo( E_GC4811_NO_STMT, "50000", 
			   ErrRsrc.E_GC4811, "E_GC4811_NO_STMT" );

    SqlExFactory.ErrInfo	ERR_GC481E_SESS_ABORT = 
	new SqlExFactory.ErrInfo( E_GC481E_SESS_ABORT, "50000", 
			   ErrRsrc.E_GC481E, "E_GC481E_SESS_ABORT" );

    SqlExFactory.ErrInfo	ERR_GC481F_SVR_CLOSED = 
	new SqlExFactory.ErrInfo( E_GC481F_SVR_CLOSED, "50000", 
			   ErrRsrc.E_GC481F, "E_GC481F_SVR_CLOSED" );

} // interface GcfErr
