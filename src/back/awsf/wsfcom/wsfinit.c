/******************************************************************************
**
**Copyright (c) 2004 Ingres Corporation
**
******************************************************************************/

#include <wss.h>
#include <wcs.h>
#include <wsmInit.h>
#include <dds.h>
#include <ddginit.h>
#include <ddgexec.h>
#include <erwsf.h>
#include <wmo.h>
#include <wsfvar.h>
#include <wps.h>
#include <asct.h>

/*
**
**  Name: wsfinit.c - Initialization of the whole Web module
**
**  Description:
**	
**  History:    
**	5-Feb-98 (marol01)
**	    created
**      10-Sep-1998 (fanra01)
**	    Corrected case of wsfInit.
**          Fixup compiler warnings on unix.
**      04-Mar-1999 (fanra01)
**          Add trace message.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      23-Jan-2003 (hweho01)
**          For hybrid build 32/64 on AIX, the 64-bit shared lib   
**          oiddi needs to have suffix '64' to reside in the   
**          the same location with the 32-bit one, due to the alternate 
**          shared lib path is not available in the current OS.
**/

GLOBALDEF DDG_SESSION		wsf_session;
GLOBALDEF DDG_DESCR			wsf_descr;

/*
** Name: WSFInitialize() - 
**
** Description:
**	Grab configuration information and call each 
**	initialization functions of the Web module
**
** Inputs:
**
** Outputs:
**
** Returns:
**	GSTATUS	:	GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**      25-Sep-98 (fanra01)
**          Changed environment names to reflect trace.
**	20-Jun-2009 (kschendel) SIR 122138
**	    Hybrid add-on symbol changed, fix here.
*/
GSTATUS 
WSFInitialize () 
{
    GSTATUS err = GSTAT_OK;
    char    *dictionary_name = NULL;
    char    *node = NULL;
    char    *svrClass = NULL;
    char    *dllname = NULL;
    char    *hostname = NULL;
    char    module = 0;
    char    *v = NULL;
    char    szConfig [CONF_STR_SIZE];
    char    *size = NULL;
    u_i4   right_size = 50;
    u_i4   user_size = 50;
    u_i4   level = 0;
    char    *trace = NULL;

    hostname = PMhost();
    CVlower(hostname);

    NMgtAt("II_ICESVR_TRACE_LEVEL", &trace);
    if (trace != NULL && trace[0] != EOS)
            CVan(trace, (i4*)&level);

    NMgtAt("II_ICESVR_TRACE_LOG", &trace);

    DDFTraceInit(level, trace);
    G_TRACE(6)("trace opened %s", trace);

    STprintf (szConfig, CONF_REPOSITORY, hostname);
    if (PMget( szConfig, &dictionary_name ) != OK && dictionary_name == NULL)
        err = DDFStatusAlloc(E_WS0042_UNDEF_REPOSITORY);

#if defined(any_aix) && defined(conf_BUILD_ARCH_32_64) && defined(BUILD_ARCH64)
    STprintf (szConfig, CONF_DLL64_NAME, hostname);
#else
    STprintf (szConfig, CONF_DLL_NAME, hostname);
#endif  /* aix */
    if (err == GSTAT_OK && PMget( szConfig, &dllname ) != OK && dllname == NULL)
        err = DDFStatusAlloc(E_WS0043_UNDEF_DLL_NAME);

    STprintf (szConfig, CONF_NODE, hostname);
    if (err == GSTAT_OK)
        PMget( szConfig, &node );

    STprintf (szConfig, CONF_CLASS, hostname);
    if (err == GSTAT_OK)
        PMget( szConfig, &svrClass );

    STprintf (szConfig, CONF_RIGHT_SIZE, hostname);
    if (err == GSTAT_OK && PMget( szConfig, &size ) == OK && size != NULL)
        CVan(size, (i4*)&right_size);

    STprintf (szConfig, CONF_USER_SIZE, hostname);
    if (err == GSTAT_OK && PMget( szConfig, &size ) == OK && size != NULL)
        CVan(size, (i4*)&user_size);

    if (err == GSTAT_OK)
        err = DDFInitialize();

    if (err == GSTAT_OK)
        err = DDGInitialize(1,
            dllname,
            node,
            dictionary_name,
            svrClass,
            NULL,
            NULL,
            &wsf_descr);

    if (err == GSTAT_OK)
        err = DDSInitialize(dllname,
            node,
            dictionary_name,
            svrClass,
            NULL,
            NULL,
            right_size,
            user_size);

    if (err == GSTAT_OK)
    {
        err = DDGConnect(&wsf_descr, NULL, NULL, &wsf_session);

        if (err == GSTAT_OK)
        {
            if (err == GSTAT_OK)
                err = WSSInitialize();
            if (err == GSTAT_OK)
                err = WSFVARInitialize();
            if (err == GSTAT_OK)
                err = WCSInitialize();
            if (err == GSTAT_OK)
                err = WPSInitialize();
            if (err == GSTAT_OK)
                err = WSMInitialize();
            if (err == GSTAT_OK)
                err = WMOInitialize();
        }
    }
    asct_trace( ASCT_LOG|ASCT_INFO )
        ( ASCT_TRACE_PARAMS, "WSF Initialise status=%08x",
        (err ? err->number : 0) );
    return(err);
}

/*
** Name: WSFTerminate() - 
**
** Description:
**	clean the module by calling Terminate function
**	and closing repository connection and
**	
**
** Inputs:
**
** Outputs:
**
** Returns:
**	GSTATUS	:	GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/
GSTATUS 
WSFTerminate () 
{
    CLEAR_STATUS(WSMTerminate());
    CLEAR_STATUS(WPSTerminate());
    CLEAR_STATUS(WCSTerminate());
    CLEAR_STATUS(WSSTerminate());
    CLEAR_STATUS(DDSTerminate());
    CLEAR_STATUS(DDGDisconnect(&wsf_session));
    CLEAR_STATUS(DDGTerminate(&wsf_descr));
    CLEAR_STATUS(DDFTerminate());
    return(GSTAT_OK);
}
