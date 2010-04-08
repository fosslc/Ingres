/*
** Copyright (c) 1999, 2003 Ingres Corporation All Rights Reserved.
*/

package com.ingres.jdbc;

/*
** Name: IngConst.java
**
** Description:
**	Defines interface providing Ingres JDBC Driver constants.
**
**  Interfaces:
**
**	IngConst
**
** History:
**	31-Oct-02 (gordy)
**	    Created.
**	15-Jul-03 (gordy)
**	    Added runtime configuration.  Removed class names.
**	24-Jan-06 <gordy)
**	    Re-branded for Ingres Corporation.
*/


/*
** Name: IngConst
**
** Description:
**	Interface which defines the Ingres JDBC Driver constants.
**
**  Constants:
**
**	ING_DRIVER_NAME		Ingres driver full name.
**	ING_PROTOCOL_ID		Ingres URL protocol ID.
**	ING_CONFIG_KEY		Ingres config key prefix.\
**	ING_PROP_FILE_NAME	Default property file name.
**	ING_PROP_FILE_KEY	Config key for property file.
**	ING_TRACE_NAME		Tracing short name.
**
** History:
**	31-Oct-02 (gordy)
**	    Created.
**	15-Jul-03 (gordy)
**	    Replace ING_TRACE_KEY with ING_CONFIG_KEY, added 
**	    ING_PROP_FILE_NAME, ING_PROP_FILE_KEY.  Removed
**	    class names.
**	24-Jan-06 <gordy)
**	    Re-branded for Ingres Corporation.
*/

interface
IngConst
{

    /*
    ** JDBC Driver identifiers.
    */
    String	ING_DRIVER_NAME		= "Ingres Corporation - JDBC Driver";
    String	ING_PROTOCOL_ID		= "ingres";
    String	ING_CONFIG_KEY	    	= "ingres.jdbc";
    String	ING_PROP_FILE_NAME	= "iijdbc.properties";
    String	ING_PROP_FILE_KEY	= "property_file";
    String	ING_TRACE_NAME		= "Ingres";


} // interface IngConst
