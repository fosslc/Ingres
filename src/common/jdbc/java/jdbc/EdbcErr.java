/*
** Copyright (c) 2004 Ingres Corporation
*/

package ca.edbc.jdbc;

/*
** Name: EdbcErr.java
**
** Description:
**	Defines the EDBC error messages used by the driver.
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
**      14-Dec-04 (rajus01) Startrak# EDJDBC94; Bug# 113625
**	    Added pooled connection error E_JD0023_POOLED_CONNECTION_NOT_USABLE.
*/

import	ca.edbc.util.EdbcEx;
import	ca.edbc.util.EdbcRsrc;


/*
** Name: EdbcErr
**
** Description:
**	Interface defining the error messages used in driver.
**
**  Constants
**
**	_JD_CLASS	EDBC error class.
**	_AP_CLASS	OpenAPI error class.
**	E_JD_MASK	EDBC class as error code mask.
**	E_AP_MASK	OpenAPI class as an error code mask.
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
*/

interface
EdbcErr
{
    int		_JD_CLASS = 237;
    int		_AP_CLASS = 201;
    int		E_JD_MASK = _JD_CLASS << 16;
    int		E_AP_MASK = _AP_CLASS << 16;

    EdbcEx.ErrInfo	E_JD0001_CONNECT_ERR = new EdbcEx.ErrInfo
    ( E_JD_MASK | 0x0001, "08001", EdbcRsrc.E_JD0001, "E_JD0001_CONNECT_ERR" );

    EdbcEx.ErrInfo	E_JD0002_PROTOCOL_ERR = new EdbcEx.ErrInfo
    ( E_JD_MASK | 0x0002, "40003", EdbcRsrc.E_JD0002, "E_JD0002_PROTOCOL_ERR" );

    EdbcEx.ErrInfo	E_JD0003_BAD_URL = new EdbcEx.ErrInfo
    ( E_JD_MASK | 0x0003, "50000", EdbcRsrc.E_JD0003, "E_JD0003_BAD_URL" );

    EdbcEx.ErrInfo	E_JD0004_CONNECTION_CLOSED = new EdbcEx.ErrInfo
    ( E_JD_MASK | 0x0004, "08003", EdbcRsrc.E_JD0004, "E_JD0004_CONNECTION_CLOSED" );

    EdbcEx.ErrInfo	E_JD0005_CONNECT_FAIL = new EdbcEx.ErrInfo
    ( E_JD_MASK | 0x0005, "40003", EdbcRsrc.E_JD0005, "E_JD0005_CONNECT_FAIL" );

    EdbcEx.ErrInfo	E_JD0006_CONVERSION_ERR = new EdbcEx.ErrInfo
    ( E_JD_MASK | 0x0006, "07006", EdbcRsrc.E_JD0006, "E_JD0006_CONVERSION_ERR" );

    EdbcEx.ErrInfo	E_JD0007_INDEX_RANGE = new EdbcEx.ErrInfo
    ( E_JD_MASK | 0x0007, "50000", EdbcRsrc.E_JD0007, "E_JD0007_INDEX_RANGE" );

    EdbcEx.ErrInfo	E_JD0008_NOT_QUERY = new EdbcEx.ErrInfo
    ( E_JD_MASK | 0x0008, "07005", EdbcRsrc.E_JD0008, "E_JD0008_NOT_QUERY" );

    EdbcEx.ErrInfo	E_JD0009_NOT_UPDATE = new EdbcEx.ErrInfo
    ( E_JD_MASK | 0x0009, "21000", EdbcRsrc.E_JD0009, "E_JD0009_NOT_UPDATE" );

    EdbcEx.ErrInfo	E_JD000A_PARAM_VALUE = new EdbcEx.ErrInfo
    ( E_JD_MASK | 0x000A, "22023", EdbcRsrc.E_JD000A, "E_JD000A_PARAM_VALUE" );

    EdbcEx.ErrInfo	E_JD000B_BLOB_IO = new EdbcEx.ErrInfo
    ( E_JD_MASK | 0x000B, "50000", EdbcRsrc.E_JD000B, "E_JD000B_BLOB_IO" );

    EdbcEx.ErrInfo	E_JD000C_BLOB_ACTIVE = new EdbcEx.ErrInfo
    ( E_JD_MASK | 0x000C, "50000", EdbcRsrc.E_JD000C, "E_JD000C_BLOB_ACTIVE" );

    EdbcEx.ErrInfo	E_JD000D_BLOB_DONE = new EdbcEx.ErrInfo
    ( E_JD_MASK | 0x000D, "50000", EdbcRsrc.E_JD000D, "E_JD000D_BLOB_DONE" );

    EdbcEx.ErrInfo	E_JD000E_UNMATCHED = new EdbcEx.ErrInfo
    ( E_JD_MASK | 0x000E, "42000", EdbcRsrc.E_JD000E, "E_JD000E_UNMATCHED" );

    EdbcEx.ErrInfo	E_JD000F_TIMEOUT = new EdbcEx.ErrInfo
    ( E_JD_MASK + 0x000F, "5000R", EdbcRsrc.E_JD000F, "E_JD000F_TIMEOUT" );

    EdbcEx.ErrInfo	E_JD0010_CATALOG_UNSUPPORTED = new EdbcEx.ErrInfo
    ( E_JD_MASK | 0x0010, "3D000", EdbcRsrc.E_JD0010, "E_JD0010_CATALOG_UNSUPPORTED" );

    EdbcEx.ErrInfo	E_JD0011_SQLTYPE_UNSUPPORTED = new EdbcEx.ErrInfo
    ( E_JD_MASK | 0x0011, "22018", EdbcRsrc.E_JD0011, "E_JD0011_SQLTYPE_UNSUPPORTED" );

    EdbcEx.ErrInfo	E_JD0012_UNSUPPORTED = new EdbcEx.ErrInfo
    ( E_JD_MASK | 0x0012, "5000R", EdbcRsrc.E_JD0012, "E_JD0012_UNSUPPORTED" );

    EdbcEx.ErrInfo	E_JD0013_RESULTSET_CLOSED = new EdbcEx.ErrInfo
    ( E_JD_MASK | 0x0013, "5000R", EdbcRsrc.E_JD0013, "E_JD0013_RESULTSET_CLOSED" );

    EdbcEx.ErrInfo 	E_JD0014_INVALID_COLUMN_NAME = new EdbcEx.ErrInfo
    ( E_JD_MASK | 0x0014, "5000R", EdbcRsrc.E_JD0014, "E_JD0014_INVALID_COLUMN_NAME" );

    EdbcEx.ErrInfo	E_JD0015_SEQUENCING = new EdbcEx.ErrInfo
    ( E_JD_MASK | 0x0015, "5000R", EdbcRsrc.E_JD0015, "E_JD0015_SEQUENCING" );

    EdbcEx.ErrInfo	E_JD0016_CALL_SYNTAX = new EdbcEx.ErrInfo
    ( E_JD_MASK | 0x0016, "5000R", EdbcRsrc.E_JD0016, "E_JD0016_CALL_SYNTAX" );

    EdbcEx.ErrInfo	E_JD0017_RS_CHANGED = new EdbcEx.ErrInfo
    ( E_JD_MASK | 0x0017, "22023", EdbcRsrc.E_JD0017, "E_JD0017_RS_CHANGED" );

    EdbcEx.ErrInfo	E_JD0018_UNKNOWN_TIMEZONE = new EdbcEx.ErrInfo
    ( E_JD_MASK | 0x0018, "22023", EdbcRsrc.E_JD0018, "E_JD0018_UNKNOWN_TIMEZONE" );

    EdbcEx.ErrInfo	E_JD0019_INVALID_DATE = new EdbcEx.ErrInfo
    ( E_JD_MASK | 0x0019, "22007", EdbcRsrc.E_JD0019, "E_JD0019_INVALID_DATE" );

    EdbcEx.ErrInfo	E_JD0020_INVALID_OUT_PARAM = new EdbcEx.ErrInfo
    ( E_JD_MASK | 0x0020, "22023", EdbcRsrc.E_JD0020, "E_JD0020_INVALID_OUT_PARAM" );

    EdbcEx.ErrInfo	E_JD0021_NO_RESULT_SET = new EdbcEx.ErrInfo
    ( E_JD_MASK | 0x0021, "42505", EdbcRsrc.E_JD0021, "E_JD0021_NO_RESULT_SET" );

    EdbcEx.ErrInfo	E_JD0022_RESULT_SET_NOT_PERMITTED = new EdbcEx.ErrInfo
    ( E_JD_MASK | 0x0022, "42000", EdbcRsrc.E_JD0022, "E_JD0022_RESULT_SET_NOT_PERMITTED" );

    EdbcEx.ErrInfo    E_JD0023_POOLED_CONNECTION_NOT_USABLE = new EdbcEx.ErrInfo
    ( E_JD_MASK | 0x0023, "42000", EdbcRsrc.E_JD0023, "E_JD0023_POOLED_CONNECTION_NOT_USABLE" );

    EdbcEx.ErrInfo	E_AP0009_QUERY_CANCELLED = new EdbcEx.ErrInfo
    ( E_AP_MASK | 0x0009, "5000R", "E_AP0009", "E_AP0009_QUERY_CANCELLED" );


} // interface EdbcErr
