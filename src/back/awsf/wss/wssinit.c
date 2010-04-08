/******************************************************************************
**
**Copyright (c) 2004 Ingres Corporation
**
******************************************************************************/

#include <ddgexec.h>
#include <wsf.h>
#include <erwsf.h>
#include <wssprofile.h>
#include <wssapp.h>

/*
**
**  Name: wssinit.c - Web Security Services
**
**  Description:
**
**  History:    
**	5-Feb-98 (marol01)
**	    created
**      19-Nov-1998 (fanra01)
**          Add case flag for ice users.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

GLOBALREF DDG_SESSION	wsf_session;	/* repository connection */
GLOBALREF char*				privileged_user;
GLOBALREF bool  ignore_user_case;

GSTATUS WSSMemCreateProfile (u_i4 id, char *name, u_i4 dbuser_id, i4 flags, i4 timeout);
GSTATUS WSSMemCreateProfileRole (u_i4 profile_id, u_i4 role_id);
GSTATUS WSSMemCreateProfileDB (u_i4 profile_id, u_i4 db_id);


/*
** Name: WSSInitialize() - 
**
** Description:
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
**      19-Nov-1998 (fanra01)
**          Initialise the ignore_user_case flag.
*/
GSTATUS
WSSInitialize () 
{
    GSTATUS     err = GSTAT_OK;
    bool        exist;
    u_i4*      id = NULL;
    char*       name = NULL;
    char*       usrcase = NULL;
    u_i4*      id2 = NULL;
    i4*    flags = NULL;
    i4*    timeout = NULL;

    STATUS      status;
    char        szConfig [CONF_STR_SIZE];

    STprintf (szConfig, CONF_WSS_CASE_USER, PMhost());
    status = PMget( szConfig, &usrcase );
    if (status == OK)
    {
        if (!STbcompare (usrcase, 0, "OFF", 0, TRUE))
            ignore_user_case = FALSE;
        else
            ignore_user_case = TRUE;
    }
    else
    {
        err = DDFStatusAlloc(status);
    }
    STprintf (szConfig, CONF_WSS_PRIV_USER, PMhost());
    status = PMget( szConfig, &privileged_user );
    if (status != OK)
        err = DDFStatusAlloc(status);

    if (err == GSTAT_OK)
    {
        err = WSSProfileInitialize();
        if (err == GSTAT_OK)
        {
            /* loading repository information about PROFILE */
            err = DDGSelectAll(&wsf_session, WSS_OBJ_PROFILE);
            while (err == GSTAT_OK)
            {
                err = DDGNext(&wsf_session, &exist);
                if (err != GSTAT_OK || exist == FALSE)
                    break;
                err = DDGProperties(&wsf_session, "%d%s%d%d%d", &id, &name, &id2, &flags, &timeout);
                if (err == GSTAT_OK)
                    CLEAR_STATUS(WSSMemCreateProfile (*id, name, *id2, *flags, *timeout));
            }
            CLEAR_STATUS( DDGClose(&wsf_session) );

            err = DDGSelectAll(&wsf_session, WSS_OBJ_PROFILE_ROLE);
            while (err == GSTAT_OK)
            {
                err = DDGNext(&wsf_session, &exist);
                if (err != GSTAT_OK || exist == FALSE)
                    break;
                err = DDGProperties(&wsf_session, "%d%d", &id, &id2);
                if (err == GSTAT_OK)
                    CLEAR_STATUS(WSSMemCreateProfileRole (*id, *id2));
            }
            CLEAR_STATUS( DDGClose(&wsf_session) );

            err = DDGSelectAll(&wsf_session, WSS_OBJ_PROFILE_DB);
            while (err == GSTAT_OK)
            {
                err = DDGNext(&wsf_session, &exist);
                if (err != GSTAT_OK || exist == FALSE)
                    break;
                err = DDGProperties(&wsf_session, "%d%d", &id, &id2);
                if (err == GSTAT_OK)
                    CLEAR_STATUS(WSSMemCreateProfileDB (*id, *id2));
            }
            CLEAR_STATUS( DDGClose(&wsf_session) );
        }
    }

    if (err == GSTAT_OK)
    {
        err = WSSAppInitialize();
        if (err == GSTAT_OK)
        {/* loading repository information about APPLICATION */
            err = DDGSelectAll(&wsf_session, WSS_OBJ_APPLICATION);
            while (err == GSTAT_OK)
            {
                err = DDGNext(&wsf_session, &exist);
                if (err != GSTAT_OK || exist == FALSE)
                    break;
                err = DDGProperties(&wsf_session, "%d%s", &id, &name);
                if (err == GSTAT_OK)
                    CLEAR_STATUS(WSSAppLoad(*id, name));
            }
            CLEAR_STATUS( DDGClose(&wsf_session) );
        }
    }
    MEfree((PTR)id);
    MEfree((PTR)name);
    MEfree((PTR)id2);
    MEfree((PTR)flags);
    MEfree((PTR)timeout);
return(err);
}

/*
** Name: WSSTerminate() - 
**
** Description:
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
WSSTerminate () 
{
	GSTATUS err = GSTAT_OK;
	CLEAR_STATUS ( WSSProfileTerminate());
	CLEAR_STATUS ( WSSAppTerminate());
return(err);
}
