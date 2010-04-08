/*
** Copyright (c) 2004 Ingres Corporation
*/

package ca.edbc.jdbcx;

/*
** Name: EdbcDSErr.java
**
** Description:
**	Defines the EDBC DataSource error messages.
**
**  Interfaces:
**
**	EdbcDSErr
**
** History:
**	30-Jan-01 (gordy)
**	    Created.
*/

import	ca.edbc.util.EdbcEx;
import	ca.edbc.util.EdbcRsrc;


/*
** Name: EdbcDSErr
**
** Description:
**	Interface defining the EDBC DataSource error messages.
**
**  Constants
**
**	_JD_CLASS	EDBC error class.
**	E_JD_MASK	Class in error code mask format.
**
** History:
**	30-Jan-01 (gordy)
**	    Created.
**	14-Dec-04 (rajus01) Startrak #EDJDBC94; Bug # 113625
**	    Added E_JD0023_POOLED_CONNECTION_NOT_USABLE error message.
*/

interface
EdbcDSErr
{

    /*
    ** The Data Source  errors are currently duplicates 
    ** of EDBC errors (this may change in the future).
    */
    int		_JD_CLASS = 237;
    int		E_JD_MASK = _JD_CLASS << 16;

    EdbcEx.ErrInfo	E_JD0004_CONNECTION_CLOSED = new EdbcEx.ErrInfo
    ( E_JD_MASK | 0x0004, "08003", EdbcRsrc.E_JD0004, "E_JD0004_CONNECTION_CLOSED" );

    EdbcEx.ErrInfo	E_JD0012_UNSUPPORTED = new EdbcEx.ErrInfo
    ( E_JD_MASK | 0x0012, "5000R", EdbcRsrc.E_JD0012, "E_JD0012_UNSUPPORTED" );

    EdbcEx.ErrInfo    E_JD0023_POOLED_CONNECTION_NOT_USABLE = new EdbcEx.ErrInfo
    ( E_JD_MASK | 0x0023, "42000", EdbcRsrc.E_JD0023, 
      "E_JD0023_POOLED_CONNECTION_NOT_USABLE" );

} // interface EdbcDSErr
