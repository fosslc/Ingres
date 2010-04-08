/******************************************************************************
**
**Copyright (c) 2004 Ingres Corporation
**
******************************************************************************/

#include <actsession.h>
#include <usrsession.h>
#include <wsfvar.h>
#include <wsmvar.h>
#include <erwsf.h>

/*
**
**  Name: wsmvar.c - Web Session Manager Variables
**
**  Description:
**		permit access to different variable levels enabled by the system
**
**  History:    
**	5-Feb-98 (marol01)
**	    created
**      11-Sep-1998 (fanra01)
**          Corrected case for actsession and usrsession to match piccolo.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

GLOBALREF DDFHASHTABLE system_var;

/*
** Name: WSMAddVariable() - insert or update a variable
**
** Description:
**      Add or update a system, session or user variable.
**
** Inputs:
**      ACT_PSESSION    : active session
**      char*           : variable name
**      u_i4           : variable name length
**      char*           : value
**      u_i4           : value length
**      u_i4           : level (WSM_ACTIVE, WSM_USER, WSM_SYSTEM)
**
** Outputs:
**      None.
**
** Returns:
**      GSTATUS    :    GSTAT_OK
**
** Exceptions:
**      None
**
** Side Effects:
**      None
**
** History:
**      07-Jun-1999 (fanra01)
**          Updated logical flow to incorporate semaphore protection to the
**          variable hash table.
*/
GSTATUS
WSMAddVariable (
    ACT_PSESSION session,
    char *name,
    u_i4 nlen,
    char *value,
    u_i4 vlen,
    u_i4 level)
{
    GSTATUS         err = GSTAT_OK;
    WSF_VAR*        var = NULL;
    char*           tmp = NULL;
    u_i2            tag = 0;
    USR_PSESSION    usrsess = NULL;

    if (level & WSM_SYSTEM)
    {
        /*
        ** System level variables hash table is semaphore protected.
        ** The WSFAddVariable takes care of add or update actions.
        */
        err = WSFAddVariable( name, nlen, value, vlen );
    }
    else if ((level & WSM_USER) && (session->user_session != NULL))
    {
        /*
        ** Session context variables hash table is semaphore protected since
        ** the use of frames allows multiple active connections of the same
        ** session.
        */
        usrsess = session->user_session;
        tag = usrsess->mem_tag;
        if ((err = DDFSemOpen( &usrsess->usrvarsem, TRUE)) == GSTAT_OK)
        {
            err = DDFgethash( usrsess->vars, name, nlen, (PTR*) &var );
            if ((err == GSTAT_OK) && (var != NULL))
            {
                /*
                ** The variable already exists remove its current value and
                ** allocate a new one.
                */
                if (var->value != NULL)
                {
                    MEfree(var->value);
                    var->value = NULL;
                }
                if (value != NULL && vlen >= 0)
                {
                    err = G_ME_REQ_MEM(tag, var->value, char, vlen+1);
                    if (err == GSTAT_OK)
                    {
                        MECOPY_VAR_MACRO(value, vlen, var->value);
                        var->value[vlen] = EOS;
                    }
                }
            }
            else
            {
                /*
                ** Create a new variable.
                */
                err = WSFCreateVariable( tag, name, nlen, value, vlen, &var );
                if (err == GSTAT_OK)
                {
                    err = DDFputhash( usrsess->vars, var->name, HASH_ALL,
                        (PTR)var );
                    if (err != GSTAT_OK)
                    {
                        CLEAR_STATUS(WSFDestroyVariable( &var ));
                    }
                }
            }
            err = DDFSemClose( &usrsess->usrvarsem );
        }
    }
    else if ((level & WSM_ACTIVE) && (session !=NULL) && (var == NULL))
    {
        tag = session->mem_tag;
        err = DDFgethash( session->vars, name, nlen, (PTR*) &var );
        if ((err == GSTAT_OK) && (var != NULL))
        {
            /*
            ** The variable already exists remove its current value and
            ** allocate a new one.
            */
            if (var->value != NULL)
            {
                MEfree(var->value);
                var->value = NULL;
            }
            if (value != NULL && vlen >= 0)
            {
                err = G_ME_REQ_MEM(tag, var->value, char, vlen+1);
                if (err == GSTAT_OK)
                {
                    MECOPY_VAR_MACRO(value, vlen, var->value);
                    var->value[vlen] = EOS;
                }
            }
        }
        else
        {
            /*
            ** Create a new variable.
            */
            err = WSFCreateVariable( tag, name, nlen, value, vlen, &var );
            if (err == GSTAT_OK)
            {
                err = DDFputhash( session->vars, var->name, HASH_ALL,
                    (PTR)var );
                if (err != GSTAT_OK)
                {
                    CLEAR_STATUS(WSFDestroyVariable(&var));
                }
            }
        }
    }
    return(err);
}

/*
** Name: WSMGetVariable() - Return the value of the variable
**
** Description:
**
** Inputs:
**	ACT_PSESSION : active session
**	char*		 : variable name
**	u_i4		 : variable name length
**	u_i4		 : level (WSM_ACTIVE, WSM_USER, WSM_SYSTEM)
**
** Outputs:
**	char*		 : value (NULL if the variable doesn't exist)
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
**      07-Jun-1999 (fanra01)
**          Add semaphore protection to the user session variable hash table.
*/
GSTATUS 
WSMGetVariable (
    ACT_PSESSION session,
    char *name,
    u_i4 nlen,
    char **value,
    u_i4 level)
{
    GSTATUS err = GSTAT_OK;
    struct __WSF_VAR *var = NULL;

    *value = NULL;

    if (level & WSM_ACTIVE)
    {
        err = DDFgethash( session->vars, name, nlen, (PTR*) &var );
    }

    if ((var == NULL) && (session->user_session != NULL) &&
        (level & WSM_USER))
    {
        if ((err = DDFSemOpen( &session->user_session->usrvarsem, TRUE))
            == GSTAT_OK)
        {
            err = DDFgethash( session->user_session->vars, name, nlen,
                (PTR*)&var);
            DDFSemClose( &session->user_session->usrvarsem );
        }
    }

    if ((var == NULL) && (level & WSM_SYSTEM))
    {
        err = WSFGetVariable( name, nlen, value );
    }

    if (var != NULL)
    {
        *value = var->value;
    }
    return(err);
}

/*
** Name: WSMDelVariable() - Remove a variable
**
** Description:
**
** Inputs:
**	ACT_PSESSION : active session
**	char*		 : variable name
**	u_i4		 : variable name length
**	u_i4		 : level (WSM_ACTIVE, WSM_USER, WSM_SYSTEM)
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
**      07-Jun-1999 (fanra01)
**          Add semaphore protection to the user session variable hash table.
*/
GSTATUS 
WSMDelVariable (
    ACT_PSESSION session,
    char *name,
    u_i4 nlen,
    u_i4 level )
{
    GSTATUS err = GSTAT_OK;
    struct __WSF_VAR *var = NULL;

    if (level & WSM_ACTIVE)
    {
        err = DDFdelhash( session->vars, name, nlen, (PTR*) &var );
    }
    if (err == GSTAT_OK && var != NULL)
    {
        MEfree( (PTR)var->value);
        MEfree( (PTR)var->name);
        MEfree( (PTR)var);
        var = NULL;
    }

    if (session->user_session != NULL && level & WSM_USER)
    {
        if ((err = DDFSemOpen( &session->user_session->usrvarsem, TRUE))
            == GSTAT_OK)
        {
            err = DDFdelhash( session->user_session->vars, name, nlen,
                (PTR*) &var);
            DDFSemClose( &session->user_session->usrvarsem );
        }
    }

    if (err == GSTAT_OK && var != NULL)
    {
        MEfree( (PTR)var->value);
        MEfree( (PTR)var->name);
        MEfree( (PTR)var);
    }

    if (level & WSM_SYSTEM)
        err = WSFDelVariable(name, nlen);
    return(err);
}

/*
** Name: WSMOpenVariable() - Open a variable scanner
**
** Description:
**
** Inputs:
**	WSM_PSCAN_VAR : scanner
**	ACT_PSESSION : active session
**	u_i4		 : level (WSM_ACTIVE, WSM_USER, WSM_SYSTEM)
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
WSMOpenVariable (
    WSM_PSCAN_VAR scanner,
    ACT_PSESSION  session,
    u_i4 level )
{
    GSTATUS err = GSTAT_OK;

    scanner->session = session;
    scanner->level = level;
    if (level & WSM_ACTIVE)
    {
        DDF_INIT_SCAN(scanner->scanner, session->vars);
        scanner->current_level = WSM_ACTIVE;
    }
    else if (session->user_session != NULL && level & WSM_USER)
    {
        DDF_INIT_SCAN(scanner->scanner, session->user_session->vars);
        scanner->current_level = WSM_USER;
    }
    else if (session->user_session != NULL && level & WSM_SYSTEM)
    {
        DDF_INIT_SCAN(scanner->scanner, system_var);
        scanner->current_level = WSM_SYSTEM;
    }
    return(err);
}

/*
** Name: WSMScanVariable() - Fetch 
**
** Description:
**
** Inputs:
**	WSM_PSCAN_VAR : scanner
**
** Outputs:
**	char**		 : variable name
**	char**		 : value
**	u_nat*		 : level (WSM_ACTIVE, WSM_USER, WSM_SYSTEM)
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
**      14-Jun-1999 (fanra01)
**          If scanning for user variables use the semaphore.
*/
GSTATUS 
WSMScanVariable (
    WSM_PSCAN_VAR scanner,
    char **name,
    char **value,
    u_i4 *level )
{
    GSTATUS err = GSTAT_OK;
    struct __WSF_VAR *var = NULL;
    ACT_PSESSION    actsess = scanner->session;

    *name = NULL;
    *value = NULL;

    if (scanner->current_level == WSM_USER)
    {
        err = DDFSemOpen( &actsess->user_session->usrvarsem, TRUE );
    }
    if (DDF_SCAN(scanner->scanner, var) && var != NULL)
    {
        *name = var->name;
        *value = var->value;
        if (level != NULL)
            *level = scanner->current_level;
    }
    if (scanner->current_level == WSM_USER)
    {
        DDFSemClose( &actsess->user_session->usrvarsem );
    }

    if (var == NULL && actsess->user_session != NULL)
    {
        if (scanner->current_level == WSM_ACTIVE &&
            scanner->level & WSM_USER)
        {
            DDF_INIT_SCAN(scanner->scanner, actsess->user_session->vars);
            scanner->current_level = WSM_USER;
            err = WSMScanVariable(scanner, name, value, level);
        }
        else if ((scanner->current_level == WSM_ACTIVE ||
                  scanner->current_level == WSM_USER) &&
                scanner->level & WSM_SYSTEM)
        {
            DDF_INIT_SCAN(scanner->scanner, system_var);
            scanner->current_level = WSM_SYSTEM;
            err = WSMScanVariable(scanner, name, value, level);
        }
    }
    return(err);
}

/*
** Name: WSMCloseVariable() - Close the scanner
**
** Description:
**
** Inputs:
**	WSM_PSCAN_VAR : scanner
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
WSMCloseVariable (
    WSM_PSCAN_VAR scanner)
{
    scanner->level = 0;
    return(GSTAT_OK);
}
