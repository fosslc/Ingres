/*
** Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cm.h>
#include    <st.h>
#include    <tm.h>
#include    <tr.h>
#include    <gcxdebug.h>

/*
** Name: gcxdebug.c - The general set of PL GCC debug trace routines.
**
** Description:
**
**      This file contains the definitions of globals and  trace routine
**      functions.
**
**
** History:
**	31-May-1989 (jorge)
**	  added the module comment header.
**	13-June-89 (jorge)
**	    added descrip.h include in front of gcccl.h include
**	22-jun-89 (seiwald)
**	    Straightened out gcc[apst]l.c tracing.
**	    Moved declarations into gcxdebug.h.
**      18-Aug-89 (cmorris)
**          Changes for end-to-end bind.
**      24-Aug-89 (cmorris)
**          Added support for tracing a primitive's subtype.
**      07-Sep-89 (cmorris)
**          Added support for TL actions.
**      09-Sep-89 (cmorris)
**          Changes for fix to name server fast-selects.
**	05-Oct-89 (seiwald)
**	    VARVCHAR, VAR1VCHAR...
**	23-Oct-89 (jorge)
**	    changed name of GCAINTERNAL.H to GCAINT.H
**      02-Nov-89 (cmorris)
**          Removed IAPCIQ and IAPCIM input events and OAIMX output event.
**      08-Nov-89 (cmorris)
**          Updated for async. disassociate changes.
**      15-Nov-89 (cmorris)
**          Added tracing for actions in SL.
**	12-Jan-90 (seiwald)
**	    Commented out defunct actions.
**	25-Jan-90 (seiwald)
**	    Changes in PL input/output events.
**	07-Feb-90 (cmorris)
**	    Addition N_LSNCLUP primitive, ITNLSF event.
**	12-Feb-90 (cmorris)
**	    Added tracing of protocol deriver requests.
**	14-Fenb-90 (cmorris)
**	    Addition of N_ABORT event.
**	01-Mar-90 (cmorris)
**	    Additions for local/remote rejects.
**      07-Mar-90 (cmorris)
**	    Added T_P_ABORT.
**	01-May-90 (cmorrris)
**	    Added support for network bridge tracing.
**	03-Aug-90 (seiwald)
**	    New SARAB state.
**	08-Aug-90 (seiwald)
**	    New IAREJM and IAAFNM input events.
**  	17-Dec-90 (cmorris)
**  	    Added TL state STWND.
**  	17-Dec-90 (cmorris)
**  	    Rename state STCLG to STWDC.
**	10-Jan-91 (seiwald)
**	    New GCA_EVENT message.
**	27-Mar-91 (seiwald)
**	    Included all necessary CL headers as per PC group.
**	01-May-91 (cmorris)
**	    Updated support for Network Bridge tracing.
**	14-May-91 (cmorris)
**	    Changes for release collision fixes.
**	31-May-91 (cmorris)
**	    Got rid of remaining vestiges of attempts to multiplex.
**	31-May-91 (cmorris)
**	    Got rid of remaining vestiges of attempts to implement peer
**	    Transport layer flow control.
**	16-Jul-91 (cmorris)
**	    More changes for protocol bridge.
**  	12-Aug-91 (cmorris)
**  	    Added support for OPTOD output event.
**  	13-Aug-91 (cmorris)
**  	    Added include of tm.h.
**	14-Aug-91 (seiwald)
**	    Added missing GC_QTXT_ATOMIC type.
**  	16-Aug-91 (cmorris)
**  	    Added support for OSTFX event.
**	17-aug-91 (leighb) DeskTop Porting Change:
**	    Remove 2nd include of compat.h
**  	10-Jan-92 (cmorris)
**  	    Added support for IBNLSF event.
**  	10-Apr-92 (cmorris)
**  	    Added support for IBNCIP event.
**	14-Aug-92 (seiwald)
**	    Support for blobs - GCA_VARSEGAR.
**	21-Aug-92 (seiwald)
**	    Added non-atomic datatypes to gcx_datatypes.
**  	06-Oct-92 (cmorris)
**  	    Got rid of actions and events associated with command session.
**  	27-Nov-92 (cmorris)
**  	    Changed A_NOOP to A_LISTEN, and IAINT to IALSN. Removed
**  	    IAREJM and IAFNM.
**	04-Dec-92 (seiwald)
**	    Added GCA_VARZERO.
**  	08-Dec-92 (cmorris)
**  	    Added SAFSL, IAACEF, IAPAM and AAFSL for fast select support.
**	16-Dec-92 (seiwald)
**	    Rename OP_PUSH_UI2 to OP_PUSH_VCH make room for a new OP_PUSH_UI2
**	    that isn't specific to varchars.
**	16-Dec-92 (seiwald)
**	    New long varchar.
**  	08-Jan-92 (cmorris)
**  	    Got rid of unused TL input events.
**	11-Jan-92 (edg)
**	    Get trace level using gcu_get_tracesym().
**  	21-Jan-93 (cmorris)
**  	    Removed T_CONTINUE.
**  	04-Feb-93 (cmorris)
**  	    Removed ATRVE, made other transport events/actions consistent
**  	    with gcctl.h.
**  	05-Feb-93 (cmorris)
**  	    Got rid of N_EXP_DATA, T_EXP_DATA, ITTEX, ITED, OTTEX and OTED.
**	25-May-93 (seiwald)
**	    Added tabs() macro and gcx_gcc_eop[].
**      14-jul-93 (ed)
**          unnested dbms.h
**	25-aug-93 (ed)
**	    remove dbdbms.h
**	14-mar-94 (seiwald)
**	    gccod.h renamed to gcdcomp.h.
**	    New defines therein.
**	17-Mar-94 (seiwald)
**	    Gcc_tdump() relocated and renamed to gcx_tdump().
**	29-Mar-94 (seiwald)
**	    New OD_IMP_BEGIN.
**	1-Apr-94 (seiwald)
**	    Split OP_DEBUG into OP_DEBUG_TYPE and OP_DEBUG_BUFS.
**	4-Apr-94 (seiwald)
**	    New OP_DV_BEGIN/END.
**	24-Apr-94 (seiwald)
**	    New OP_BREAK.
**	 9-Nov-94 (gordy)
**	    new OP_MSG_RESTART.
**	27-mar-95 (peeje01)
**	    Cross integration from doublebyte label 6500db_su4_us42:
**	    13-jul-94 (kirke)
**	        Added strings for OP_CHAR_TYPE and OP_DATA_LEN.
**	16-Nov-95 (gordy)
**	    Updated GCA message list.
**	 1-Feb-96 (gordy)
**	    Added support for compressed VARCHARs.
**	 8-Mar-96 (gordy)
**	    Fixed processing or large message which span buffers.
**	21-Mar-96 (gordy)
**	    Fixed signed char problem in gcx_tdump().
**	29-Mar-96 (rajus01)
**	    Added support for Protocol Bridge.
**	 3-Apr-96 (gordy)
**	    Updated GCD operations.
**	21-Jun-96 (gordy)
**	    Added OP_COPY_MSG, updated the array type definitions.
**	30-sep-1996 (canor01)
**	    Move global data definitions into gcudata.c.
**	17-Feb-98 (gordy)
**	    Added MIB support for gcx tracing.
**	03-apr-1998 (canor01)
**	    Nullable long varchar message literal is longer than others,
**	    so gcx_look() buffer needs to be larger.
**	 8-Jan-99 (gordy)
**	    Added gcx_timestamp().
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	25-June-2001(wansh01) 
**	    Removed gcxlevel, gcx_classes and clean up all GLOBALREF. 
**	22-Mar-02 (gordy)
**	    Removed unneeded include files.
**      18-Nov-2004 (Gordon.Thorpe@ca.com and Ralph.Loen@ca.com)
**          Added new gcx_getGCAMsgMap() function to export the gcx_gca_msg
**          global definition.  Otherwise, GCF server processes cannot
**          resolve the symbol definition in Windows.
*/

/*
** Name: gcx_look
**
** Description:
**	Translate numeric identifier to string.
**	Four buffers are used to hold the output
**	strings, so result returned is invalid 
**	after four subsequent calls.
**
** Input:
**	table	Translation table.
**	number	Numeric identifier.
**
** Output:
**	None.
**
** Returns:
**	char *	Symbolic representation
*/

char *
gcx_look( GCXLIST *table, i4 number )
{
    static char bufs[ 4 ][ 32 ];
    static i4 rot = 0;
    char *nxtbuf = bufs[ rot = ( rot + 1 ) % 4 ];

    for( ; table->msg; table++ )
	if ( number == table->number )  break;

    if ( table->msg )
	STprintf( nxtbuf, "%s", table->msg );
    else
	STprintf( nxtbuf, "[0x%x]", number );

    return( nxtbuf );
}


/*
** Name: gcx_timestamp
**
** Description:
**	Print a timestamp in the trace log.
**
** Inputs:
**	None.
**
** Outputs:
**	None.
**
** Returns:
**	i4		Length of text printed to log.
**
** History:
**	 8-Jan-99 (gordy)
**	    Created.
*/

i4
gcx_timestamp( void )
{
    SYSTIME	systime;
    char	buff[ TM_SIZE_STAMP + 1 ];

    TMnow( &systime );
    TMstr( &systime, buff );
    TRdisplay( "%s ", buff );

    return( STlength( buff ) + 1 );
}



/*
** Name:	gcx_tdump	- Trace a TL buffer.
**
** Description:
**	Prints out the buffer in hex and ASCII for debugging purposes.
**
** Inputs:
**	buf		Pointer to buffer
**	len		Buffer length
**
** Outputs:
**	None.
**
** History:
**	30-Aug-1995 (sweeney)
**	    Added standard history section.
**      30-Aug-1995 (sweeney)
**	    Made the output a little more readable.
**	21-Mar-96 (gordy)
**	    Strip off high bits after downshifting character.  If
**	    char is a signed type, downshifting can maintain sign
**	    resulting in an bad array index.
*/

VOID					 
gcx_tdump( char *buf, i4  len )
{
    char *hexchar = "0123456789ABCDEF";
    char hex[ 49 ]; /* 3 * 16 + \0 */
    char asc[ 17 ]; 

    i4	i = 0;

    for( ; len--; buf++ )
    {
	hex[3*i] = hexchar[ (*buf >> 4) & (char)0x0F ];
	hex[3*i+1] = hexchar[ *buf & (char)0x0F ];
	hex[3*i+2] =  ' ';
	asc[i++] = CMprint(buf) && !(*buf & (char)0x80) ? *buf : '.' ;
	hex[3*i] = asc [i] = '\0';

	if( !( i %= 16 ) || !len )
	    TRdisplay("%48s    %s\n", hex, asc);
    }

    return;
}

GCXLIST *
gcx_getGCAMsgMap()
{
   return gcx_gca_msg;
}
