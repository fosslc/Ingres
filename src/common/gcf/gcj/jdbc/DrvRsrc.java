/*
** Copyright (c) 1999, 2009 Ingres Corporation All Rights Reserved.
*/

package com.ingres.gcf.jdbc;

/*
** Name: DrvRsrc.java
**
** Description:
**	Locale specific information.  This file defines the
**	default resources values (english,USA) for the JDBC 
**	Driver.  These resources may be localized for different 
**	languages and customs by providing suitably named 
**	classes (see documentation for ResourceBundle) which 
**	define the same keys as this file and provides the 
**	translated values.
**
**  Classes
**
**	DrvRsrc		Driver resources
**	EmptyRsrc	Empty resource bundle.
**
** History:
**	 2-Sep-99 (gordy)
**	    Created.
**	17-Sep-99 (rajus01)
**	    Added connection parameters.
**	14-Jan-00 (gordy)
**	    Added Connection Pool parameter.
**	18-Apr-00 (gordy)
**	    Moved to util package. Enhanced to be self-contained.
**	19-May-00 (gordy)
**	    Added connection property for select loops.
**	17-Oct-00 (gordy)
**	    Added autocommit mode definitions.
**	28-Mar-01 (gordy)
**	    Renamed effective user resource to DBMS user to
**	    be more generic and pair with DBMS password.
**	20-Aug-01 (gordy)
**	    Added 'Cursor Mode' connection property.
**      19-Aug-02 (wansh01) 
**	    Added 'Login type' connection property.
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.
**	26-Dec-02 (gordy)
**	    Added character encoding connection property.
**	18-Feb-03 (gordy)
**	    Rename RSRC_CP_LOGIN to RSRC_CP_VNODE.
**	 7-Jul-03 (gordy)
**	    Added connection properties for Ingres configuration.
**	24-Jan-06 <gordy)
**	    Re-branded for Ingres Corporation.
**	27-Jul-07 (gordy)
**	    Added date alias connection property.
**	 9-Jan-09 (gordy)
**	    Cleanup related to problems reported by the findbugs utility.
**      11-Feb-10 (rajus01) Bug 123277
**          Added connection property 'send_ingres_dates' to allow the
**          driver to send the date/time/timestamp values as INGRESDATE
**          datatype instead of TS_W_TZ (ANSI TIMESTAMP WITH TIMEZONE).
*/

import	java.util.ResourceBundle;
import	java.util.ListResourceBundle;


/*
** Name: DrvRsrc
**
** Description:
**	ResourceBundle sub-class which defines the default resource
**	keys and values (english,USA) for the JDBC Driver.
**
**  Public Data:
**
**	Resource keys for Connection Properties:
**
**	RSRC_CP_DB		Database
**	RSRC_CP_USR		User ID
**	RSRC_CP_PWD		Password
**	RSRC_CP_GRP		Group ID
**	RSRC_CP_ROLE		Role ID
**	RSRC_CP_DBUSR		DBMS user ID
**	RSRC_CP_DBPWD		DBMS password
**	RSRC_CP_POOL		Connection pool
**	RSRC_CP_XACM		Autocommit mode
**	RSRC_CP_LOOP		Select loops
**	RSRC_CP_CRSR		Cursor mode
**	RSRC_CP_VNODE		VNODE usage
**	RSRC_CP_ENCODE		Character encoding
**	RSRC_CP_TZ		Timezone
**	RSRC_CP_DEC_CHR		Decimal character
**	RSRC_CP_DTE_FMT		Date format
**	RSRC_CP_MNY_FMT		Money format
**	RSRC_CP_MNY_PRC		Money precision
**	RSRC_CP_DTE_TYP		Date alias
**	RSRC_CP_SND_ING_DTE	Send Ingres Dates
**
**  Public methods:
**
**	getContents	Returns the array of keys/values for defined Locale.
**	getResource	Returns the driver resources for default Locale
**
**  Private Data:
**
**	info		Array of keys/value resources.
**	RSRC_CLASS	Base resource class.
**	resource	Resource for default Locale.
**
** History:
**	 2-Sep-99 (gordy)
**	    Created.
**	17-Sep-99 (rajus01)
**	    Added connection parameters.
**	14-Jan-00 (gordy)
**	    Added Connection Pool parameter.
**	18-Apr-00 (gordy)
**	    Added static data, initializer, getResource() method,
**	    and EmptyRsrc class definition to make class self-
**	    contained.
**	19-May-00 (gordy)
**	    Added connection property for select loops.
**	17-Oct-00 (gordy)
**	    Added autocommit mode definitions.
**	28-Mar-01 (gordy)
**	    Renamed effective user resource to DBMS user to
**	    be more generic and pair with DBMS password.
**	20-Aug-01 (gordy)
**	    Added 'Cursor Mode' connection property.
**	20-Aug-02 (wansh01)
**	    Added 'Login Type' connection property.
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.
**	26-Dec-02 (gordy)
**	    Added character encoding connection property.
**	18-Feb-03 (gordy)
**	    Rename RSRC_CP_LOGIN to RSRC_CP_VNODE.
**	 7-Jul-03 (gordy)
**	    Added RSRC_CP_TZ, RSRC_CP_DEC_CHR, RSRC_CP_DTE_FMT,
**	    RSRC_CP_MNY_FMT, RSRC_CP_MNY_PRC.
**	24-Jan-06 <gordy)
**	    Re-branded for Ingres Corporation.
**	23-Jul-07 (gordy)
**	    Added RSRC_CP_DTE_TYPE.
**      11-Feb-10 (rajus01) Bug 123277
**	    Added RSRC_CP_SND_ING_DTE.
*/

public class
DrvRsrc
    extends ListResourceBundle
{

    /*
    ** Connection property keys
    */
    public static final String	RSRC_CP_DB	= "RSRC_CP_DB";
    public static final String	RSRC_CP_USR	= "RSRC_CP_USR";
    public static final String	RSRC_CP_PWD	= "RSRC_CP_PWD";
    public static final String	RSRC_CP_GRP	= "RSRC_CP_GRP";
    public static final String	RSRC_CP_ROLE	= "RSRC_CP_ROLE";
    public static final String	RSRC_CP_DBUSR	= "RSRC_CP_DBUSR";
    public static final String	RSRC_CP_DBPWD	= "RSRC_CP_DBPWD";
    public static final String	RSRC_CP_POOL	= "RSRC_CP_POOL";
    public static final String	RSRC_CP_XACM	= "RSRC_CP_XACM";
    public static final String	RSRC_CP_LOOP	= "RSRC_CP_LOOP";
    public static final String	RSRC_CP_CRSR	= "RSRC_CP_CRSR";
    public static final	String	RSRC_CP_VNODE	= "RSRC_CP_VNODE";
    public static final	String	RSRC_CP_ENCODE	= "RSRC_CP_ENCODE";
    public static final String	RSRC_CP_TZ	= "RSRC_CP_TZ";
    public static final String	RSRC_CP_DEC_CHR	= "RSRC_CP_DEC_CHR";
    public static final String	RSRC_CP_DTE_FMT	= "RSRC_CP_DTE_FMT";
    public static final String	RSRC_CP_MNY_FMT = "RSRC_CP_MNY_FMT";
    public static final String	RSRC_CP_MNY_PRC = "RSRC_CP_MNY_PRC";
    public static final String	RSRC_CP_DTE_TYP = "RSRC_CP_DTE_TYP";
    public static final String  RSRC_CP_SND_ING_DTE = "RSRC_CP_SND_ING_DTE";

    /*
    ** Key/value resource list.
    */
    private static final Object   info[][] = 
    {
	/*
	** Connection properties.
	*/
	{ RSRC_CP_DB,		"Database" },
	{ RSRC_CP_USR,		"User ID" },
	{ RSRC_CP_PWD,		"Password" },
	{ RSRC_CP_GRP,		"Group ID" },
	{ RSRC_CP_ROLE,		"Role ID" },
	{ RSRC_CP_DBUSR,	"DBMS User ID" },
 	{ RSRC_CP_DBPWD,	"DBMS Password" },
	{ RSRC_CP_POOL,		"Connection Pool" },
	{ RSRC_CP_XACM,		"Autocommit Mode" },
	{ RSRC_CP_LOOP,		"Select Loops" },
	{ RSRC_CP_CRSR,		"Cursor Mode" },
	{ RSRC_CP_VNODE,	"VNODE Usage" },
	{ RSRC_CP_ENCODE,	"Character Encoding" },
	{ RSRC_CP_TZ,		"Timezone" },
	{ RSRC_CP_DEC_CHR,	"Decimal Character" },
	{ RSRC_CP_DTE_FMT,	"Date Format" },
	{ RSRC_CP_MNY_FMT,	"Money Format" },
	{ RSRC_CP_MNY_PRC,	"Money Precision" },
	{ RSRC_CP_DTE_TYP,	"Date Alias" },
	{ RSRC_CP_SND_ING_DTE,	"Send Ingres Dates" },
    };


/*
** Name: getContents
**
** Description:
**	Returns an array of the defined key/value pairs.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	Object[][]	Key/value pair array.
**
** History:
**	 2-Sep-99 (gordy)
**	    Created.
**	 9-Jan-09 (gordy)
**	    Cleanup related to problems reported by the findbugs utility.
**	    Changed access to protected to match super-class and restrict
**	    public access to internal info list.
*/

protected Object[][]
getContents()
{
    return( info );
} // getContents


/*
** The following static data, method and class definition 
** are only needed in the resource base class DrvRsrc.
*/

    /*
    ** Base resource class definition for
    ** loading Locale specific resources.
    */
    private static final String		RSRC_CLASS  = 
						"com.ingres.gcf.jdbc.DrvRsrc";

    /*
    ** Locale specific resource.
    */
    private static ResourceBundle	resource    = null;


/*
** Name: getResource
**
** Description:
**	Returns the JDBC Driver resource for the default Locale.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	ResourceBundle	Resource for default Locale.
**
** History:
**	18-Apr-00 (gordy)
**	    Created.
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.
*/

public static ResourceBundle
getResource()
{
    if ( resource == null )
    {
	/*
	** Load the Locale specific resources for the default Locale
	** and make sure that a resource bundle (even empty) exists.
	*/
	try { resource = ResourceBundle.getBundle( RSRC_CLASS ); }
	catch( Exception ex ){ resource = new EmptyRsrc(); }
    }

    return( resource );
}


/*
** Name: EmptyRsrc
**
** Description:
**	ResourceBundle sub-class which contains no resources.
**
** History:
**	 2-Sep-99 (gordy)
**	    Created.
*/

private static class
EmptyRsrc
    extends ListResourceBundle
{
    private static final Object   contents[][] = new Object[0][];

public Object[][]
getContents()
{
    return( contents );
} // getContents


} // class EmptyRsrc

} // class DrvRsrc
