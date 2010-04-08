/*
** Copyright (c) 2004 Ingres Corporation
*/

package ca.edbc.io;

/*
** Name: IoErr.java
**
** Description:
**	Defines the IO error messages used by the driver.
**
**  Interfaces:
**
**	IoErr
**
** History:
**	10-Sep-99 (gordy)
**	    Created.
**	29-Sep-99 (gordy)
**	    Added support for BLOBs: E_JD000B_BLOB_IO.
**	 4-Feb-00 (gordy)
**	    Added sequencing error E_JD0015_SEQUENCING.
*/

import	ca.edbc.util.EdbcEx;
import	ca.edbc.util.EdbcRsrc;


/*
** Name: IoErr
**
** Description:
**	Interface defining the IO error messages used in driver.
**
**  Constants
**
**	_JD_CLASS	EDBC error class.
**	E_JD_MASK	Class in error code mask format.
**
**  Public Data:
**
**
** History:
**	10-Sep-99 (gordy)
**	    Created.
**	29-Sep-99 (gordy)
**	    Added support for BLOBs: E_JD000B_BLOB_IO.
**	 4-Feb-00 (gordy)
**	    Added sequencing error E_JD0015_SEQUENCING.
**	20-Apr-00 (gordy)
**	    Removed EDBC errors not used by the IO system.
**	    Renamed IO errors but kept the underlying EDBC errors.
*/

interface
IoErr
{
    /*
    ** The IO errors are currently duplicates of EDBC errors
    ** (this may change in the future).
    */
    int		_JD_CLASS = 237;
    int		E_JD_MASK = _JD_CLASS << 16;

    EdbcEx.ErrInfo	E_IO0001_CONNECT_ERR = new EdbcEx.ErrInfo
    ( E_JD_MASK | 0x0001, "08001", EdbcRsrc.E_JD0001, "E_JD0001_CONNECT_ERR" );

    EdbcEx.ErrInfo	E_IO0002_PROTOCOL_ERR = new EdbcEx.ErrInfo
    ( E_JD_MASK | 0x0002, "40003", EdbcRsrc.E_JD0002, "E_JD0002_PROTOCOL_ERR" );

    EdbcEx.ErrInfo	E_IO0003_BAD_URL = new EdbcEx.ErrInfo
    ( E_JD_MASK | 0x0003, "50000", EdbcRsrc.E_JD0003, "E_JD0003_BAD_URL" );

    EdbcEx.ErrInfo	E_IO0004_CONNECTION_CLOSED = new EdbcEx.ErrInfo
    ( E_JD_MASK | 0x0004, "08003", EdbcRsrc.E_JD0004, "E_JD0004_CONNECTION_CLOSED" );

    EdbcEx.ErrInfo	E_IO0005_CONNECT_FAIL = new EdbcEx.ErrInfo
    ( E_JD_MASK | 0x0005, "40003", EdbcRsrc.E_JD0005, "E_JD0005_CONNECT_FAIL" );

    EdbcEx.ErrInfo	E_IO0006_SEQUENCING = new EdbcEx.ErrInfo
    ( E_JD_MASK | 0x0015, "5000R", EdbcRsrc.E_JD0015, "E_JD0015_SEQUENCING" );

    EdbcEx.ErrInfo	E_IO0007_BLOB_IO = new EdbcEx.ErrInfo
    ( E_JD_MASK | 0x000B, "50000", EdbcRsrc.E_JD000B, "E_JD000B_BLOB_IO" );

} // interface IoErr
