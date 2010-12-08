/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <cv.h>
#include    <di.h>
#include    <me.h>
#include    <pc.h>
#include    <tm.h>
#include    <tr.h>
#include    <erglf.h>
#include    <sp.h>
#include    <mo.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ulf.h>
#include    <qsf.h>

#include    <ddb.h>
#include    <adf.h>
#include    <scf.h>
#include    <ulm.h>

#include    <dmf.h>
#include    <sxf.h>
#include    <qefmain.h>
#include    <qefscb.h>
#include    <qefrcb.h>
#include    <qefcb.h>
#include    <qefprotos.h>

/*
**
**  Name: QEFMO.C - Provide QEF interface to Management Objects (MO).
**
**  Description:
**      This file holds all of the MO functions for QEF.
**
**	External functions:
**
**	 qef_mo_register		register QEF objects with the IMA gateway
**
**
**	Static functions:
**
**	 getLongnatFromQEFCB	peek at a i4 field in the QEFCB
**	 putLongnatIntoQEFCB	poke a i4 field in the QEFCB
**	 getQEFCB		retrieve this session's QEF control block
**
**
**  History:
**	20-JAN-94 (rickh)
**	    Creation.
**      13-Apr-1994 (daveb) 62320
**          Turn off the object for now.  It segv's in the RCP when
**          there is no QEF session.  SCF cheerfully returns E_DB_OK
**          and a pointer of 0xffffffff to getQEFCB().
**	1-apr-1998 (hanch04)
**	    Calling SCF every time has horendous performance implications,
**	    re-wrote routine to call CS directly with qef_session.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	10-Jan-2001 (jenjo02)
**	    Replaced qef_session() function with GET_QEF_CB macro.
**	2-Dec-2010 (kschendel) SIR 124685
**	    Prototype fixes: include qefprotos (leading to header hell, ugh).
*/

GLOBALREF	QEF_S_CB	*Qef_s_cb;

static	MO_GET_METHOD	getLongnatFromQEFCB;
static	MO_SET_METHOD	putLongnatIntoQEFCB;


/* MO object class definitions */
static MO_CLASS_DEF QEFCBclasses[ ] =
{
# ifdef FIXED_BUG_62320
    {
	0, "exp.qef.qef_cb.qef_spillThreshold",
	MO_SIZEOF_MEMBER( QEF_CB, qef_spillThreshold ), MO_READ | MO_WRITE, 0,
	CL_OFFSETOF( QEF_CB, qef_spillThreshold ), getLongnatFromQEFCB,
	putLongnatIntoQEFCB, (PTR) 0, MOcdata_index
    },
# endif
    { 0 }
};

/*{
** Name: qef_mo_register - Management Object initialization for QEF
**
** Description:
**      This function registers QEF's Management Objects with the internal
**	IMA gateway.  This routine is called at server startup time.
**        
** Inputs:
**	None
**
** Outputs:
**	None
**
** Returns:
**	OK				Everything was attached successfully
**	other				An MO-specific error
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**	20-jan-94 (rickh)
**          Initial creation.
*/

STATUS
qef_mo_register( void )
{
    STATUS	status = OK;
    i4	err_code;

    status = MOclassdef( MAXI2, QEFCBclasses );
    if (status != OK)
    {
	ule_format(status, (CL_ERR_DESC *)0, ULE_LOG,
			NULL, (char *)0, 0L, (i4 *)0, &err_code, 0);
    }

    return (status);
}

/*
** Name: getLongnatFromQEFCB - MO Get function for i4s in the QEF_CB
**
** Description:
**	This routine gets a i4 field from the current session's
**	QEF control block.
**
**	To get your hands on this field, you need to register an IMA
**	table (you must be $ingres to do this and $ingres must own the
**	database).  Then you can peek and poke it using selects and updates:
**
**	First make sure that your database contains the catalogs needed
**	to register objects.  These catalogs exist in IIDBDB but probably
**	not in other databases:
**
**	drop table iigw07_relation;
**	\p\g
**	drop table iigw07_attribute;
**	\p\g
**	drop table iigw07_index;
**	\p\g
**	
**	create table iigw07_relation (
**		tblid	integer not null not default,
**		tblidx	integer not null not default,
**		flags	integer not null not default
**	);
**	\p\g
**	
**	create table iigw07_attribute (
**		tblid  integer not null not default,
**		tblidx integer not null not default,
**		attnum i2 not null not default,
**		classid char(64) not null not default
**	);
**	\p\g
**	
**	create table iigw07_index (
**		tblid integer not null not default,	
**		tblidx integer not null not default,	
**		attnum i2 not null not default,
**		classid char(64) not null not default
**	);
**	\p\g
**	
**	modify iigw07_relation to btree unique on tblid, tblidx;\p\g
**	modify iigw07_attribute to btree unique on tblid, tblidx, attnum;\p\g
**	modify iigw07_index to btree unique on tblid, tblidx, attnum;\p\g
**	commit;
**	\p\g
**
**	Then register your objects and peek and poke them:
**
**
**	remove table qefcb_tab;
**
**	register table qefcb_tab (
**		server		varchar(64) not null not default is 'SERVER',
**		classid		varchar(64) not null not default is 'CLASSID',
**		instance	varchar(64) not null not default is 'INSTANCE',
**		value		varchar(64) not null not default is 'VALUE'
**	)
**	as import from 'objects'
**	with dbms = ima,
**		structure = unique sortkeyed,
**		key = ( server, classid, instance ),
**		update;
**	\p\g
**
**
**	select value from qefcb_tab
**		where	server = dbmsinfo( 'ima_server' )
**		and	instance='0'
**		and	classid = 'exp.qef.qef_cb.qef_spillThreshold';\p\g
**
**	update qefcb_tab set value='30'
**		where	server = dbmsinfo( 'ima_server' )
**		and	instance='0'
**		and	classid = 'exp.qef.qef_cb.qef_spillThreshold';\p\g
**
**
**
** Inputs:
**	offset			    - offset of field into the QEF_CB
**	objsize			    - size of the field, ignored
**	object			    - a fictional address. ignored.
**	luserbuf		    - length of the output buffer
**
** Outputs:
**	userbuf			    - i4 decoded into string of digits
**
** Returns:
**	STATUS
**
** History:
**	20-jan-94 (rickh)
**	    Written.
**
*/

static STATUS
getLongnatFromQEFCB(
	i4 offset,
	i4 objsize,
	PTR object,
	i4 luserbuf,
	char *userbuf )
{
    QEF_CB		*qef_cb;
    i4		*field;
    CS_SID		sid;

    /* first get our hands on this session's QEF control block */

    CSget_sid(&sid);
    qef_cb = GET_QEF_CB(sid);

    /* calculate the address of this field in the QEFCB */

    field = ( i4 * ) ( ( ( char * ) qef_cb ) + offset );

    /* now print the field */

    return (MOlongout( MO_VALUE_TRUNCATED, *field, luserbuf, userbuf));
}

/*{
**  Name: putLongnatIntoQEFCB - fill a i4 field in the QEF_CB
**
**  Description:
**	This routine pokes a value into a i4 field in the current
**	session's QEF control block.
**
**	See the header of getLongnatFromQEFCB( ) for a sample script
**	that peeks and pokes a QEF_CB variable.
**
**	
** Inputs:
**	offset		byte offset of this field in the QEF_CB
**	luserbuf	length of the user buffer; ignored.
**	userbuf		the user string to convert.
**	objsize		size of this field. ignored.
**	object		meant to be the base of the QEF_CB. garbage. ignored.
**
** Outputs:
**	None
**
** Side Effects:
**	None
**
** Returns:
**	OK
**		if the operation succeseded
**	MO_BAD_SIZE
**		if the object size isn't something that can be handled.
**	other
**		conversion-specific failure status as appropriate.
**  History:
**	20-jan-94 (rickh)
**	    Created.
*/

static STATUS 
putLongnatIntoQEFCB(
	i4 offset,
	i4 luserbuf,
	char *userbuf,
	i4 objsize,
	PTR object
)
{
    STATUS		status;
    QEF_CB		*qef_cb;
    i4		*field;
    CS_SID		sid;

    /* first get our hands on this session's QEF control block */

    CSget_sid(&sid);
    qef_cb = GET_QEF_CB(sid);

    /* calculate the address of this field in the QEFCB */

    field = ( i4 * ) ( ( ( char * ) qef_cb ) + offset );

    status = CVal( userbuf, field );
    return( status );
}
