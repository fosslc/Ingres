/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
** Name: iceconf.c
**
** Description:
**
**      ic_initialize           Create a connection context and connection.
**      ic_seterror             Store ICE api error status and information.
**      ic_saveapi              Save a connection context whilst performing
**                              another option.
**      ic_restapi              Restore a previously saved connection context.
**      ic_close                Disconnect from the ice server.
**      ic_getermessage         Get the error message text for the error.
**      ic_errormsg             Return the error message text for the error
**                              status.
**      ic_getfileparts         Return the parts of the filespec.
**      ic_terminate            Indicates that the use of the ICE C API has
**                              completed.
**      ic_select               Initiates a request to obtain a set of
**                              information from the ice server.
**      ic_idselect             Retrieve an entry from the ice repository.
**      ic_getnext              Fetch a row from a select or an idselect.
**      ic_idretrieve           Retrieve the entry from the ice repository
**                              with the specified id.
**      ic_getval               Return first value from the returned row.
**      ic_getnxtval            Return subsequent values from the row.
**      ic_getvalbyref          Return first value from the returned row.
**      ic_getnxtvalbyref       Return subsequent values from the row.
**      ic_release              Release the ICE API request parameter list
**                              for the specified function name.
**      ic_getitem              Return the value of a parameter based on a
**                              column value in the row.
**      ic_getitemwith          Return the value of a parameter based on a
**                              list of parameter values.
**
**      ic_createsessgrp        Create the named session group.
**      ic_deletesessgrp        Delete a session group.
**      ic_createrole           Create a role.
**      ic_deleterole           Delete a role.
**      ic_createdbuser         Create a database user.
**      ic_deletedbuser         Delete a database user.
**      ic_createuser           Create an ice user.
**      ic_deleteuser           Delete an ice user.
**      ic_createloc            Create a location.
**      ic_deleteloc            Delete a location.
**      ic_createbu             Create a business unit.
**      ic_deletebu             Delete a business unit.
**      ic_createdoc            Create a document.
**      ic_deletedoc            Delete a document.
**      ic_createprofile        Create a profile.
**      ic_deleteprofile        Delete a profile.
**      ic_assocunitrole        Create a unit and role association.
**      ic_releaseunitrole      Release a unit role association.
**      ic_assocprofilerole     Create profile role association.
**      ic_releaseprofilerole   Release a profile role association.
**      ic_retvariable          Return the value of the variable in the scope.
**      ic_setvariable          Set the value of the variable in the scope.
**
** History:
**      21-Feb-2001 (fanra01)
**          Sir 103813
**          Created.
**      04-May-2001 (fanra01)
**          Sir 103813
**          Add the ability to share an ICE session.  This provides
**          functionality to share an ICE session context with an external
**          process.
**          Add functions to retrieve and set variables.
**          Add byref functions for returning values.
*/
# include <iiapi.h>
# include <iceconf.h>
# include "icelocal.h"

/*
** Name: ic_initialize
**
** Description:
**      Create a connection context and connection.  Initialise the ICE C API
**      and connect to the ice server.  Connection information is stored
**      within the context structure until closed.
**
** Inputs:
**      node            Node name of the ice server to connect.
**                      Null value specifies local machine.
**      name            User name to connect to ice server.
**      password        Password to connect to ice server.
**      cookie          Server context identification.
**
** Outputs:
**      hicectx         Pointer to context handled returned from function.
**
** Returns:
**      OK                          Completed successfully.
**      E_IC_INVALID_USERNAME       User name has not been specified.
**      E_IC_INVALID_PASSWORD       Password has not been specified.
**      E_IC_INVALID_OUTPARAM       Pointer for the output value not specified.
**
** History:
**      21-Feb-2001 (fanra01)
**          Created.
**      04-May-2001 (fanra01)
**          Add connection type based on whether a cookie is supplied or not.
*/
ICE_STATUS
ic_initialize( II_CHAR* node, II_CHAR* name, II_CHAR* password,
    II_CHAR* cookie, HICECTX* hicectx )
{
    ICE_STATUS status = OK;
    PICECTX picectx;
    PICESTK picestk;

    if (cookie == NULL)
    {
        if (name == NULL)
        {
            status = E_IC_INVALID_USERNAME;
        }
        else if (password == NULL)
        {
            status = E_IC_INVALID_PASSWORD;
        }
    }
    if ((status == OK) && (hicectx != NULL))
    {
        MEMALLOC( sizeof(ICECTX), PICECTX, TRUE, status, picectx );
        if (picectx != NULL)
        {
            if ((status = ICE_C_Initialize()) == OK)
            {
                picectx->initialized = TRUE;
                picectx->node = (node != NULL) ? STRDUP( node ) : NULL;
                picectx->user = STRDUP( name );
                picectx->passwd = STRDUP( password );
                MEMALLOC( sizeof(ICESTK), PICESTK, TRUE, status, picestk );
                if (picestk != NULL)
                {
                    picectx->curr = picestk;
                }
                picectx->select     =   ic_select;
                picectx->idselect   =   ic_idselect;
                picectx->idretrieve =   ic_idretrieve;
                picectx->getnext    =   ic_getnext;
                picectx->getval     =   ic_getval;
                picectx->getnxtval  =   ic_getnxtval;
                picectx->release    =   ic_release;
                picectx->seterror   =   ic_seterror;
                picectx->getitem    =   ic_getitem;
                picectx->getitemwith=   ic_getitemwith;
                picectx->saveapi    =   ic_saveapi;
                picectx->restapi    =   ic_restapi;

                if (cookie == NULL)
                {
                    status = ICE_C_Connect( node, name, password,
                        &picectx->sessid );
                    if (status == OK)
                    {
                        picectx->connecttype = IA_SESSION_EXCLUSIVE;
                    }
                }
                else
                {
                    status = ICE_C_ConnectAs( node, cookie, &picectx->sessid );
                    if (status == OK)
                    {
                        picectx->connecttype = IA_SESSION_SHARED;
                    }
                }

                if (status == OK)
                {
                    status = ic_getfreeentry( picectx, hicectx );
                }
            }
            if (status != OK)
            {
                picectx->status = status;
                picectx->errinfo= ICE_C_LastError( picectx->sessid );
                if (picestk != NULL)
                {
                    MEMFREE( picestk );
                }
                MEMFREE( picectx );
            }
        }
    }
    else
    {
        status = (status == OK) ? E_IC_INVALID_OUTPARAM : status;
    }
    return(status);
}

/*
** Name: ic_seterror
**
** Description:
**      Store ICE api error status and information.
**
** Inputs:
**      hicectx         Handle to the connection context returned from a
**                      a call to ic_initialize.
**      status          Value of the status to record in the context structure.
**
** Outputs:
**      None.
**
** Returns:
**      None.
**
** History:
**      21-Feb-2001 (fanra01)
**          Created.
*/
II_VOID
ic_seterror( HICECTX hicectx, II_INT4 status )
{
    PICECTX     icectx;

    ic_getentry( hicectx, &icectx );

    icectx->status = status;
    icectx->errinfo= ICE_C_LastError( icectx->sessid );
}

/*
** Name: ic_saveapi
**
** Description:
**      Save a connection context whilst performing another option.
**      It is sometimes necessary to retrieve additional information for the
**      fulfilment of a request.  To allow this the save and restore have
**      been provided to allow nesting of requests.
**      The ic_saveapi function returns the original context and creates a
**      new one to be used in its place.
**
** Inputs:
**      hicectx         Handle to the connection context returned from a
**                      a call to ic_initialize.
**
** Outputs:
**      icestck         Pointer to the original ice context.
**
** Returns:
**      None.
**
** History:
**      21-Feb-2001 (fanra01)
**          Created.
*/
II_VOID
ic_saveapi( HICECTX hicectx, PICESTK* icestk )
{
    ICE_STATUS  status = OK;
    PICESTK     picestk;
    PICECTX     icectx;

    ic_getentry( hicectx, &icectx );

    MEMALLOC( sizeof(ICESTK), PICESTK, TRUE, status, picestk );
    *icestk = icectx->curr;
    icectx->curr = picestk;
    return;
}

/*
** Name: ic_restapi
**
** Description:
**      Restore a previously saved connection context and free the existing
**      in-use context.
**
** Inputs:
**      hicectx         Handle to the connection context returned from a
**                      a call to ic_initialize.
**      icestk          Pointer to context, returned from ic_saveapi, to
**                      restore.
**
** Outputs:
**      None.
**
** Returns:
**      None.
**
** History:
**      21-Feb-2001 (fanra01)
**          Created.
*/
II_VOID
ic_restapi( HICECTX hicectx, PICESTK icestk )
{
    PICESTK     picestk;
    PICECTX     icectx;

    ic_getentry( hicectx, &icectx );

    picestk = icectx->curr;
    icectx->curr = icestk;
    MEMFREE( picestk );
}

/*
** Name: ic_close
**
** Description:
**      Disconnect from the ice server.
**
** Inputs:
**      hicectx         Handle to the connection context returned from a
**                      a call to ic_initialize.
**
** Outputs:
**      None.
**
** Returns:
**      OK                          Completed successfully.
**      E_IC_INVALID_INPARAM        Specified handle does not reference a
**                                  valid context.
**
** History:
**      21-Feb-2001 (fanra01)
**          Created.
**      04-May-2001 (fanra01)
**          Test the connection type.  Only if the session is exclusive should
**          the ICE session be terminated on close.
*/
ICE_STATUS
ic_close( HICECTX hicectx )
{
    ICE_STATUS status = OK;
    PICECTX     icectx;

    ic_getentry( hicectx, &icectx );

    if (icectx != NULL)
    {
        if (icectx->connecttype == IA_SESSION_EXCLUSIVE)
        {
            status = ICE_C_Disconnect( &icectx->sessid );
        }
        if (status == OK)
        {
            if (icectx->node != NULL)
            {
                MEMFREE( icectx->node );
            }
            if (icectx->user != NULL)
            {
                MEMFREE( icectx->user );
            }
            if (icectx->passwd != NULL)
            {
                MEMFREE( icectx->passwd );
            }
            MEMFREE( icectx );
            ic_freeentry( hicectx );
        }
        else
        {
            icectx->seterror( hicectx, status );
        }
    }
    else
    {
        status = E_IC_INVALID_INPPARAM;
    }
    return(status);
}

/*
** Name: ic_getermessage
**
** Description:
**      Function looks up the message text for the err or id and returns a
**      copy of it.
**
** Inputs:
**      id              ER error id
**
** Outputs:
**      msg             Pointer to the error message
**
** Returns:
**      OK                          Completed successfully.
**      !OK                         Failed with CL error value.
**
** History:
**      21-Feb-2001 (fanra01)
**          Created.
*/
static ICE_STATUS
ic_getermessage(  II_INT4 id, II_INT4 flags, II_CHAR** msg )
{
    ICE_STATUS  status = OK;
    CL_ERR_DESC clerror;
    II_CHAR     msgstr[MAX_MSG_SIZE];
    II_INT4     msglen = 0;

    *msg = NULL;
    if ((status = ERslookup ( (ER_MSGID)id, NULL, flags, NULL, msgstr,
        sizeof(msgstr), -1, &msglen, &clerror, 0, NULL )) == OK)
    {
        *msg = STRDUP( msgstr );
    }
    return(status);
}

/*
** Name: ic_errormsg
**
** Description:
**      Return the error message text for the error status.  If the stored
**      ICE C API status indicates an error append the error message to
**      the message text.
**
** Inputs:
**      hicectx         Handle to the connection context returned from a
**                      a call to ic_initialize.
**      status          Value of the status to get message for.
**
** Outputs:
**      errormsg        Pointer to error message string.
**
** Returns:
**      None.
**
** History:
**      21-Feb-2001 (fanra01)
**          Created.
*/
II_VOID
ic_errormsg( HICECTX hicectx, ICE_STATUS status, II_CHAR** errormsg )
{
    ICE_STATUS  err;
    PICECTX     icectx;
    II_CHAR*    msg;
    II_INT4     len = 0;
    II_CHAR*    outmsg;

    if (errormsg != NULL)
    {
        ic_getentry( hicectx, &icectx );
        if (icectx != NULL)
        {
            if (ic_getermessage( status, 0, &msg ) == OK)
            {
                if (msg != NULL)
                {
                    len = STLENGTH( msg );
                }
                if ((icectx->status != OK) && (icectx->errinfo != NULL))
                {
                    len += STLENGTH(icectx->errinfo);
                    len += 1;       /* Add space for newline character */
                }
                len += 1;           /* Add space for EOS terminator */
                MEMALLOC( len, II_CHAR*, TRUE, err, outmsg );
                if (outmsg != NULL)
                {
                    if (msg != NULL)
                    {
                        STRCOPY( msg, outmsg );
                        if (icectx->errinfo != NULL)
                        {
                            STRCAT( outmsg, "\n" );
                            STRCAT( outmsg, icectx->errinfo );
                        }
                    }
                    *errormsg = outmsg;
                }
            }
        }
    }
}

/*
** Name: ic_getfileparts
**
** Description:
**      Return the parts of the filespec.
**
** Inputs:
**      filename        Filename string.
**
** Outputs:
**      file            file portion of the filename.
**      ext             extension portion of the filename.
**
** Returns:
**      OK                          Completed successfully.
**      E_IC_INVALID_INPARAM        The filename has not been specified.
**
** History:
**      21-Feb-2001 (fanra01)
**          Created.
*/
ICE_STATUS
ic_getfileparts( II_CHAR* filename, II_CHAR** file, II_CHAR** ext )
{
    ICE_STATUS status = OK;
    LOCATION    loc;
    II_CHAR     pathstr[MAX_LOC + 1];
    II_CHAR     dev[MAX_LOC + 1];
    II_CHAR     path[MAX_LOC + 1];
    II_CHAR     prefix[MAX_LOC + 1];
    II_CHAR     suffix[MAX_LOC + 1];
    II_CHAR     version[MAX_LOC + 1];

    if(filename)
    {
        STRCOPY( filename, pathstr );
        if ((status = LOfroms( (PATH&FILENAME), pathstr, &loc )) == OK)
        {
            status = LOdetail( &loc, dev, path, prefix, suffix, version);
            if (status == OK)
            {
                *file = NULL;
                *ext  = NULL;
                if (prefix[0] != 0)
                {
                    *file = STRDUP( prefix );
                }
                if (suffix[0] != 0)
                {
                    *ext = STRDUP( suffix );
                }
            }
        }
    }
    else
    {
        status = E_IC_INVALID_INPPARAM;
    }
    return(status);
}

/*
** Name: ic_terminate
**
** Description:
**      Indicates that the use of the ICE C API has completed.
**
** Inputs:
**      None.
**
** Outputs:
**      None.
**
** Returns:
**      None.
**
** History:
**      21-Feb-2001 (fanra01)
**          Created.
*/
II_VOID
ic_terminate()
{
    ICE_C_Terminate();
    return;
}

/*
** Name: ic_select
**
** Description:
**      Initiates a request to obtain a set of information from the ice server.
**
** Inputs:
**      hicectx         Handle to the connection context returned from a
**                      a call to ic_initialize.
**      func            Ice server function to obtain list with.
**
** Outputs:
**      None.
**
** Returns:
**      OK                          Completed successfully.
**      E_IC_INVALID_INPARAM        Specified handle does not reference a
**                                  valid context or function name has not
**                                  been specified.
**
** History:
**      21-Feb-2001 (fanra01)
**          Created.
*/
ICE_STATUS
ic_select( HICECTX hicectx, II_CHAR* func )
{
    ICE_STATUS status = OK;
    PICEAPI iceapi;
    II_INT4 i;
    PICECTX     icectx;

    ic_getentry( hicectx, &icectx );

    if ((icectx != NULL) && (func != NULL))
    {
        if ((status=ia_create( func, &iceapi )) == OK)
        {
            icectx->curr->iceapi = iceapi;
            if ((status=iceapi->setaction( iceapi, IA_STR_SELECT )) == OK)
            {
                /*
                ** Indicate new select
                */
                icectx->curr->endset = 0;
                /*
                ** Action type already set start count from 1.
                ** Remaining values should be output.
                */
                for(i=1; (i < iceapi->fcount) && (status == OK); i+=1)
                {
                    status = iceapi->settype( iceapi, i, ICE_OUT );
                }
                status=ICE_C_Execute( icectx->sessid, func, iceapi->fparms );
                if (status != OK)
                {
                    icectx->seterror( hicectx, status );
                }
            }
        }
    }
    else
    {
        status = E_IC_INVALID_INPPARAM;
    }
    return(status);
}

/*
** Name: ic_idselect
**
** Description:
**      Retrieve an entry from the ice repository.  For ice server functions
**      that define a select for value instead of a retrieve.
**
** Inputs:
**      hicectx         Handle to the connection context returned from a
**                      a call to ic_initialize.
**      func            Ice server function to obtain list with.
**      id              Identifier for specific repository entry.
**
** Outputs:
**      None.
**
** Returns:
**      OK                          Completed successfully
**      E_IC_INVALID_INPARAM        Specified handle does not reference a
**                                  valid context or function name has not
**                                  been specified.
**
** History:
**      21-Feb-2001 (fanra01)
**          Created.
*/
ICE_STATUS
ic_idselect( HICECTX hicectx, II_CHAR* func, II_INT4 id )
{
    ICE_STATUS status = OK;
    PICEAPI iceapi;
    II_INT4 i;
    II_CHAR parmstr[20];
    PICECTX     icectx;

    ic_getentry( hicectx, &icectx );

    if ((icectx != NULL) && (func != NULL))
    {
        if ((status=ia_create( func, &iceapi )) == OK)
        {
            icectx->curr->iceapi = iceapi;
            if ((status=iceapi->setaction( iceapi, IA_STR_SELECT )) == OK)
            {
                /*
                ** Indicate new select
                */
                icectx->curr->endset = 0;
                /*
                ** Action type already set start count from 1.
                ** Remaining values should be output.
                */
                for(i=1; (i < iceapi->fcount) && (status == OK); i+=1)
                {
                    status = iceapi->settype( iceapi, i, ICE_OUT );
                }
                if (status == OK)
                {
                    status = iceapi->settype( iceapi, 1, ICE_IN | ICE_OUT );
                    STRPRINTF( parmstr, "%d", id );
                    status = iceapi->setvalue( iceapi, 1, parmstr );
                }
                status=ICE_C_Execute( icectx->sessid, func, iceapi->fparms );
                if (status != OK)
                {
                    icectx->seterror( hicectx, status );
                }
            }
        }
    }
    else
    {
        status = E_IC_INVALID_INPPARAM;
    }
    return(status);
}

/*
** Name: ic_getnext
**
** Description:
**      Fetch a row from a select or an idselect.
**      Updates all the values in the parameter list.
**
** Inputs:
**      hicectx         Handle to the connection context returned from a
**                      a call to ic_initialize.
**
** Outputs:
**      None.
**
** Returns:
**      OK                          Completed successfully
**      E_IC_INVALID_INPARAM        Specified handle does not reference a
**                                  valid context.
**      E_IC_OUT_OF_DATA            No next row to return.  All results
**                                  returned.
**      E_IC_DATASET_EXHAUSTED      Request has completed and all resulting
**                                  rows have been returned.
**
** History:
**      21-Feb-2001 (fanra01)
**          Created.
*/
ICE_STATUS
ic_getnext( HICECTX hicectx )
{
    ICE_STATUS status = OK;
    PICEAPI iceapi;
    II_INT4 i;
    II_CHAR*    value = NULL;
    PICECTX     icectx;

    ic_getentry( hicectx, &icectx );

    if (icectx != NULL)
    {
        iceapi = icectx->curr->iceapi;
        if (icectx->curr->endset == 0)
        {
            if ((status = ICE_C_Fetch( icectx->sessid, &icectx->curr->endset)) == OK)
            {
                if( icectx->curr->endset != 0)
                {
                    /*
                    ** start index to skip action
                    */
                    for(i=1; (i < iceapi->fcount) && (status == OK); i+=1)
                    {
                        status = iceapi->setvalue( iceapi, i, value );
                    }
                    ICE_C_Close( icectx->sessid );
                    status = E_IC_OUT_OF_DATA;
                }
                else
                {
                    /*
                    ** start index to skip action
                    */
                    for(i=1; (i < iceapi->fcount) && (status == OK); i+=1)
                    {
                        value = ICE_C_GetAttribute( icectx->sessid, i );
                        if (value != NULL)
                        {
                            status = iceapi->setvalue( iceapi, i, STRDUP( value ) );
                        }
                        else
                        {
                            status=iceapi->setvalue( iceapi, i, value );
                        }
                    }
                }
            }
            else
            {
                icectx->seterror( hicectx, status );
            }
        }
        else
        {
            /*
            ** All rows retrieved.  Must select again.
            */
            status = E_IC_DATASET_EXHAUSTED;
        }
    }
    else
    {
        status = E_IC_INVALID_INPPARAM;
    }
    return(status);
}

/*
** Name: ic_idretrieve
**
** Description:
**      Retrieve the entry from the ice repository with the specified id.
**
** Inputs:
**      hicectx         Handle to the connection context returned from a
**                      a call to ic_initialize.
**      func            Ice server function to obtain entry with.
**      id              Identifier for specific repository entry.
**
** Outputs:
**      None.
**
** Returns:
**      OK                          Completed successfully
**      E_IC_INVALID_INPARAM        Specified handle does not reference a
**                                  valid context or function name has not
**                                  been specified.
**
** History:
**      21-Feb-2001 (fanra01)
**          Created.
*/
ICE_STATUS
ic_idretrieve( HICECTX hicectx, II_CHAR* func, II_INT4 id )
{
    ICE_STATUS status = OK;
    PICEAPI iceapi;
    ICE_C_PARAMS* param;
    II_INT4 i;
    II_CHAR idstr[20];
    PICECTX     icectx;

    ic_getentry( hicectx, &icectx );

    if ((icectx != NULL) && (func != NULL))
    {
        if ((status=ia_create( func, &iceapi )) == OK)
        {
            icectx->curr->iceapi = iceapi;
            if ((status=iceapi->setaction( iceapi, IA_STR_RETRIEVE )) == OK)
            {
                /*
                ** Indicate new retrieve
                */
                icectx->curr->endset = 0;
                /*
                ** Action type already set start count from 1.
                ** Remaining values should be output.
                */
                for(i=1; (i < iceapi->fcount) && (status == OK); i+=1)
                {
                    status = iceapi->settype( iceapi, i, ICE_OUT );
                }
                status = iceapi->getparmpos( icectx->curr->iceapi, 1, &param );
                if (status == OK)
                {
                    STRPRINTF( idstr, "%d", id );
                    param->type |= ICE_IN;
                    param->value = STRDUP( idstr );
                    status=ICE_C_Execute( icectx->sessid, func, iceapi->fparms );
                    if (status != OK)
                    {
                        icectx->seterror( hicectx, status );
                    }
                }
            }
        }
    }
    else
    {
        status = E_IC_INVALID_INPPARAM;
    }
    return(status);
}

/*
** Name: ic_getval
**
** Description:
**      Return first value from the returned row.
**
** Inputs:
**      hicectx         Handle to the connection context returned from a
**                      a call to ic_initialize.
**
** Outputs:
**      value           Pointer to returned value from column.
**
** Returns:
**      OK                          Completed successfully
**      E_IC_INVALID_INPARAM        Specified handle does not reference a
**                                  valid context.
**      E_IC_INVALID_OUTPARAM       Pointer for the output value not specified.
**
** History:
**      21-Feb-2001 (fanra01)
**          Created.
*/
ICE_STATUS
ic_getval( HICECTX hicectx, II_CHAR** value )
{
    ICE_STATUS status = OK;
    PICEAPI iceapi;
    II_CHAR* name;
    PICECTX     icectx;

    ic_getentry( hicectx, &icectx );

    if (icectx != NULL)
    {
        if (value != NULL)
        {
            iceapi = icectx->curr->iceapi;
            icectx->curr->colcount = 0;
            if ((status = iceapi->getname( iceapi, 0, &name )) == OK)
            {
                if (STRCMP( IA_STR_ACTION, name ) == 0)
                {
                    icectx->curr->colcount = 1;
                }
            }
            status = iceapi->getvalue( iceapi, icectx->curr->colcount, value );
            if (status == OK)
            {
                icectx->curr->colcount += 1;
            }
        }
        else
        {
            status = E_IC_INVALID_OUTPARAM;
        }
    }
    else
    {
        status = E_IC_INVALID_INPPARAM;
    }
    return(status);
}

/*
** Name: ic_getnxtval
**
** Description:
**      Return subsequent values from the row.
**
** Inputs:
**      hicectx         Handle to the connection context returned from a
**                      a call to ic_initialize.
**
** Outputs:
**      value           Pointer to returned value from column.
**
** Returns:
**      OK                          Completed successfully
**      E_IC_INVALID_INPARAM        Specified handle does not reference a
**                                  valid context.
**      E_IC_INVALID_OUTPARAM       Pointer for the output value not specified.
**      E_IC_ROW_COMPLETE           No more values in the row to return.
**
** History:
**      21-Feb-2001 (fanra01)
**          Created.
*/
ICE_STATUS
ic_getnxtval( HICECTX hicectx, II_CHAR** value )
{
    ICE_STATUS status = OK;
    PICEAPI iceapi;
    PICECTX     icectx;

    ic_getentry( hicectx, &icectx );

    if (icectx != NULL)
    {
        if (value != NULL)
        {
            iceapi = icectx->curr->iceapi;
            if (icectx->curr->colcount < iceapi->fcount)
            {
                status = iceapi->getvalue( iceapi, icectx->curr->colcount, value );
                if (status == OK)
                {
                    icectx->curr->colcount += 1;
                }
            }
            else
            {
                status = E_IC_ROW_COMPLETE;
            }
        }
        else
        {
            status = E_IC_INVALID_OUTPARAM;
        }
    }
    else
    {
        status = E_IC_INVALID_INPPARAM;
    }
    return(status);
}

/*
** Name: ic_getvalbyref
**
** Description:
**      Return first value from the returned row.
**
** Inputs:
**      hicectx         Handle to the connection context returned from a
**                      a call to ic_initialize.
**
** Outputs:
**      value           Pointer to buffer to receive value.
**
** Returns:
**      OK                          Completed successfully
**      E_IC_INVALID_INPARAM        Specified handle does not reference a
**                                  valid context.
**      E_IC_INVALID_OUTPARAM       Pointer for the output value not specified.
**
** History:
**      21-Feb-2001 (fanra01)
**          Created.
*/
ICE_STATUS
ic_getvalbyref( HICECTX hicectx, II_CHAR* value )
{
    ICE_STATUS status = OK;
    PICEAPI iceapi;
    II_CHAR* name;
    PICECTX     icectx;
    II_CHAR*    apival = NULL;

    ic_getentry( hicectx, &icectx );

    if (icectx != NULL)
    {
        if (value != NULL)
        {
            iceapi = icectx->curr->iceapi;
            icectx->curr->colcount = 0;
            if ((status = iceapi->getname( iceapi, 0, &name )) == OK)
            {
                if (STRCMP( IA_STR_ACTION, name ) == 0)
                {
                    icectx->curr->colcount = 1;
                }
            }
            status = iceapi->getvalue( iceapi, icectx->curr->colcount, &apival );
            if (status == OK)
            {
                STRCOPY( apival, value );
                icectx->curr->colcount += 1;
            }
        }
        else
        {
            status = E_IC_INVALID_OUTPARAM;
        }
    }
    else
    {
        status = E_IC_INVALID_INPPARAM;
    }
    return(status);
}

/*
** Name: ic_getnxtvalbyref
**
** Description:
**      Return subsequent values from the row.
**
** Inputs:
**      hicectx         Handle to the connection context returned from a
**                      a call to ic_initialize.
**
** Outputs:
**      value           Pointer to buffer to receive value.
**
** Returns:
**      OK                          Completed successfully
**      E_IC_INVALID_INPARAM        Specified handle does not reference a
**                                  valid context.
**      E_IC_INVALID_OUTPARAM       Pointer for the output value not specified.
**      E_IC_ROW_COMPLETE           No more values in the row to return.
**
** History:
**      21-Feb-2001 (fanra01)
**          Created.
*/
ICE_STATUS
ic_getnxtvalbyref( HICECTX hicectx, II_CHAR* value )
{
    ICE_STATUS status = OK;
    PICEAPI iceapi;
    PICECTX     icectx;
    II_CHAR*    apival = NULL;

    ic_getentry( hicectx, &icectx );

    if (icectx != NULL)
    {
        if (value != NULL)
        {
            iceapi = icectx->curr->iceapi;
            if (icectx->curr->colcount < iceapi->fcount)
            {
                status = iceapi->getvalue( iceapi, icectx->curr->colcount, &apival );
                if (status == OK)
                {
                    STRCOPY( apival, value );
                    icectx->curr->colcount += 1;
                }
            }
            else
            {
                status = E_IC_ROW_COMPLETE;
            }
        }
        else
        {
            status = E_IC_INVALID_OUTPARAM;
        }
    }
    else
    {
        status = E_IC_INVALID_INPPARAM;
    }
    return(status);
}

/*
** Name: ic_release
**
** Description:
**      Release the ICE API request parameter list for the specified function
**      name.
**
** Inputs:
**      hicectx         Handle to the connection context returned from a
**                      a call to ic_initialize.
**      func            Ice server function to release.
**
** Outputs:
**      None.
**
** Returns:
**      OK                          Completed successfully
**      E_IC_INVALID_INPARAM        Specified handle does not reference a
**                                  valid context or function name has not
**                                  been specified.
**      E_IC_INVALID_FNAME          Internal function name does not match
**                                  release request.
**
** History:
**      21-Feb-2001 (fanra01)
**          Created.
*/
ICE_STATUS
ic_release( HICECTX hicectx, II_CHAR* func )
{
    ICE_STATUS status = OK;
    PICEAPI iceapi;
    II_CHAR*    fname;
    PICECTX     icectx;

    ic_getentry( hicectx, &icectx );

    if ((icectx != NULL) && (func != NULL))
    {
        iceapi = icectx->curr->iceapi;
        status = iceapi->getfuncname( iceapi, iceapi->fnumber, &fname );
        if (status == OK)
        {
            if (STRCMP( func, fname) == 0)
            {
                if ((status = ia_destroy( iceapi )) == OK)
                {
                    icectx->curr->iceapi = NULL;
                }
            }
            else
            {
                status = E_IC_INVALID_FNAME;
            }
        }
    }
    else
    {
        status = E_IC_INVALID_INPPARAM;
    }
    return(status);
}

/*
** Name: ic_getitem
**
** Description:
**      Return the value of a parameter based on a column value in the row.
**
** Inputs:
**      hicectx         Handle to the connection context returned from a
**                      a call to ic_initialize.
**      fnumber         Function number to perform request.
**      cmpname         Name of parameter to compare value.
**      cmpval          Value to match.
**      resname         Name of parameter for which the value will be returned.
**
** Outputs:
**      resval          Pointer to the returned parameter value.
**
** Returns:
**      OK                          Completed successfully
**      E_IC_INVALID_INPARAM        Specified handle does not reference a
**                                  valid context or one or more arguments
**                                  have not been initialized.
**      E_IC_INVALID_OUTPARAM       Pointer for the output value not specified.
**
** History:
**      21-Feb-2001 (fanra01)
**          Created.
*/
ICE_STATUS
ic_getitem( HICECTX hicectx, II_INT4 fnumber, II_CHAR* cmpname, II_CHAR* cmpval,
    II_CHAR* resname, II_CHAR** resval)
{
    ICE_STATUS status = OK;
    II_CHAR* rvalue;
    PICEAPI iceapi;
    ICE_C_PARAMS* parm;
    II_CHAR*    fname;
    PICECTX     icectx;

    ic_getentry( hicectx, &icectx );

    if ((icectx != NULL) && (cmpname != NULL) && (cmpval != NULL) &&
        (resname != NULL))
    {
        if (resval != NULL)
        {
            ia_retfuncname( fnumber, &fname );
            if ((status = icectx->select( hicectx, fname ))
                == OK)
            {
                iceapi = icectx->curr->iceapi;
                do
                {
                    rvalue = NULL;
                    status = iceapi->getparmname( iceapi, cmpname, &parm );
                    if (status == OK)
                    {
                        if (parm->value != NULL)
                        {
                            if (STRCMP( cmpval, parm->value ) == 0)
                            {
                                status = iceapi->getparmname( iceapi,
                                    resname, &parm );
                                if (status == OK)
                                {
                                    *resval = NULL;
                                    if (parm->value != NULL)
                                    {
                                        *resval = STRDUP( parm->value );
                                    }
                                    break;
                                }
                            }
                        }
                    }
                    status = icectx->getnext( hicectx );
                }
                while(status == OK);
                icectx->release( hicectx, fname );
            }
        }
        else
        {
            status = E_IC_INVALID_OUTPARAM;
        }
    }
    else
    {
        status = E_IC_INVALID_INPPARAM;
    }
    return(status);
}

/*
** Name: ic_getitemwith
**
** Description:
**      Return the value of a parameter based on a list of parameter values.
**
** Inputs:
**      hicectx         Handle to the connection context returned from a
**                      a call to ic_initialize.
**      fnumber         Function number to perform request.
**      params          List of parameters and their values.
**      resname         Name of column value to return.
**
** Outputs:
**      resval          Pointer to the returned parameter value.
**
** Returns:
**      OK                          Completed successfully
**      E_IC_INVALID_INPARAM        Specified handle does not reference a
**                                  valid context or one or more arguments
**                                  have not been initialized.
**      E_IC_INVALID_OUTPARAM       Pointer for the output value not specified.
**
** History:
**      21-Feb-2001 (fanra01)
**          Created.
*/
ICE_STATUS
ic_getitemwith( HICECTX hicectx, II_INT4 fnumber, ICE_C_PARAMS* params,
II_CHAR* resname, II_CHAR** resval)
{
    ICE_STATUS status = OK;
    II_CHAR* rvalue;
    PICEAPI iceapi;
    ICE_C_PARAMS* parm;
    ICE_C_PARAMS* iparm;
    II_CHAR*    fname;
    II_INT4     found;
    PICECTX     icectx;

    ic_getentry( hicectx, &icectx );

    if ((icectx != NULL) && (params != NULL) && (resname != NULL))
    {
        if (resval != NULL)
        {
            ia_retfuncname( fnumber, &fname );
            if ((status = icectx->select( hicectx, fname )) == OK)
            {
                iceapi = icectx->curr->iceapi;
                do
                {
                    rvalue = NULL;
                    for(found = 0, iparm = params; iparm->name != NULL;
                        iparm+=1)
                    {
                        if (iparm->type == ICE_IN)
                        {
                            status = iceapi->getparmname( iceapi, iparm->name,
                                &parm );
                            if (status == OK)
                            {
                                if (parm->value != NULL)
                                {
                                    if (STRCMP( iparm->value, parm->value )
                                        != 0)
                                    {
                                        break;
                                    }
                                    else
                                    {
                                        found = 1;
                                    }
                                }
                            }
                        }
                    }
                    if (found == 1)
                    {
                        status = iceapi->getparmname( iceapi, resname, &parm );
                        if (status == OK)
                        {
                            *resval = NULL;
                            if (parm->value != NULL)
                            {
                                *resval = STRDUP( parm->value );
                            }
                        }
                    }
                    else
                    {
                        status = icectx->getnext( hicectx );
                    }
                }
                while((status == OK) && (!found));
                icectx->release( hicectx, fname );
            }
        }
        else
        {
            status = E_IC_INVALID_OUTPARAM;
        }
    }
    else
    {
        status = E_IC_INVALID_INPPARAM;
    }
    return(status);
}

/*
** Name: ic_createsessgrp
**
** Description:
**      Request creation of a session group.
**
** Inputs:
**      hicectx         Handle to the connection context returned from a
**                      a call to ic_initialize.
**      name            Name of the session group to create.
**
** Outputs:
**      sessid          Identifier for the created session group.
**
** Returns:
**      OK                          Completed successfully
**      E_IC_INVALID_INPARAM        Specified handle does not reference a
**                                  valid context.
**
** History:
**      21-Feb-2001 (fanra01)
**          Created.
*/
ICE_STATUS
ic_createsessgrp( HICECTX hicectx, II_CHAR* name, II_INT4* sessid )
{
    ICE_STATUS status = OK;
    PICEAPI iceapi;
    ICE_C_PARAMS* param;
    ICE_C_PARAMS* sessparam;
    II_CHAR* func;
    PICECTX     icectx;

    ic_getentry( hicectx, &icectx );

    if ((icectx != NULL) && (name != NULL))
    {
        if (sessid != NULL)
        {
            status = ia_retfuncname( ICE_SESS, &func );
            if ((status=ia_create( func , &iceapi )) == OK)
            {
                icectx->curr->iceapi = iceapi;
                if (status == OK)
                {
                    status = iceapi->setaction( iceapi, IA_STR_INSERT );
                    if (status == OK)
                    {
                        status = iceapi->getparmpos( iceapi, 1, &sessparam );
                        if (status == OK)
                        {
                            sessparam->type = ICE_OUT;
                        }
                        status = iceapi->getparmpos( iceapi, 2, &param );
                        if (status == OK)
                        {
                            param->type = ICE_IN;
                            param->value = STRDUP( name );
                            status=ICE_C_Execute( icectx->sessid, func,
                                iceapi->fparms );
                            if (status != OK)
                            {
                                icectx->seterror( hicectx, status );
                            }
                            else
                            {
                                II_CHAR* value;

                                value = ICE_C_GetAttribute( icectx->sessid, 1 );
                                if (value != NULL)
                                {
                                    ASCTOI( value, sessid );
                                }
                                ICE_C_Close( icectx->sessid );
                            }
                        }
                    }
                }
                icectx->release( hicectx, func );
            }
        }
    }
    else
    {
        status = E_IC_INVALID_INPPARAM;
    }
    return(status);
}

/*
** Name: ic_deletesessgrp
**
** Description:
**      Remove the named session group.
**
** Inputs:
**      hicectx         Handle to the connection context returned from a
**                      a call to ic_initialize.
**      name            Name of the session group to delete.
**
** Outputs:
**      None.
**
** Returns:
**      OK                          Completed successfully
**      E_IC_INVALID_INPARAM        Specified handle does not reference a
**                                  valid context.
**
** History:
**      21-Feb-2001 (fanra01)
**          Created.
*/
ICE_STATUS
ic_deletesessgrp( HICECTX hicectx, II_CHAR* name )
{
    ICE_STATUS status = OK;
    PICEAPI iceapi;
    ICE_C_PARAMS* param;
    II_CHAR* func;
    II_CHAR* sessid;
    PICESTK picestk;
    PICECTX     icectx;

    ic_getentry( hicectx, &icectx );

    if ((icectx != NULL) && (name != NULL))
    {
        status = ia_retfuncname( ICE_SESS, &func );
        if ((status=ia_create( func , &iceapi )) == OK)
        {
            icectx->curr->iceapi = iceapi;
            if (status == OK)
            {
                status = iceapi->setaction( iceapi, IA_STR_DELETE );
                if (status == OK)
                {
                    icectx->saveapi( hicectx, &picestk );
                    status = icectx->getitem( hicectx, ICE_SESS, "sess_name",
                        name, "sess_id", &sessid );
                    icectx->restapi( hicectx, picestk );
                    if (status == OK)
                    {
                        status = iceapi->getparmname( icectx->curr->iceapi,
                            "sess_id", &param );
                        if (status == OK)
                        {
                            param->type = ICE_IN;
                            param->value= sessid;
                            status=ICE_C_Execute( icectx->sessid, func,
                                iceapi->fparms );
                            if (status != OK)
                            {
                                icectx->seterror( hicectx, status );
                            }
                        }
                    }
                }
            }
        }
    }
    else
    {
        status = E_IC_INVALID_INPPARAM;
    }
    return(status);
}

/*
** Name: ic_createrole
**
** Description:
**      Create an ice role.
**
** Inputs:
**      hicectx         Handle to the connection context returned from a
**                      a call to ic_initialize.
**      name            Name of the role to create
**      comment         An option comment string.
**
** Outputs:
**      roleid
**
** Returns:
**      OK                          Completed successfully
**      E_IC_INVALID_INPARAM        Specified handle does not reference a
**                                  valid context or the name has not been
**                                  specified.
**
** History:
**      21-Feb-2001 (fanra01)
**          Created.
*/
ICE_STATUS
ic_createrole( HICECTX hicectx, II_CHAR* name, II_CHAR* comment, II_INT4* roleid )
{
    ICE_STATUS status = OK;
    PICEAPI iceapi;
    ICE_C_PARAMS* param;
    ICE_C_PARAMS* roleparam;
    II_CHAR* func;
    PICECTX     icectx;

    ic_getentry( hicectx, &icectx );

    if ((icectx != NULL) && (name != NULL))
    {
        if (roleid != NULL)
        {
            status = ia_retfuncname( ICE_ROLE, &func );
            if ((status=ia_create( func , &iceapi )) == OK)
            {
                icectx->curr->iceapi = iceapi;
                if (status == OK)
                {
                    status = iceapi->setaction( iceapi, IA_STR_INSERT );
                    if (status == OK)
                    {
                        status = iceapi->getparmpos( iceapi, 1, &roleparam );
                        if (status == OK)
                        {
                            roleparam->type = ICE_OUT;
                        }
                        status = iceapi->getparmpos( iceapi, 2, &param );
                        if (status == OK)
                        {
                            param->type = ICE_IN;
                            param->value = STRDUP( name );
                        }
                        status = iceapi->getparmpos( iceapi, 3, &param );
                        if (status == OK)
                        {
                            param->type = ICE_IN;
                            param->value = STRDUP( comment );
                            status=ICE_C_Execute( icectx->sessid, func,
                                iceapi->fparms );
                            if (status != OK)
                            {
                                icectx->seterror( hicectx, status );
                            }
                            else
                            {
                                II_CHAR* value;

                                value = ICE_C_GetAttribute( icectx->sessid, 1 );
                                if (value != NULL)
                                {
                                    ASCTOI( value, roleid );
                                }
                                ICE_C_Close( icectx->sessid );
                            }
                        }
                    }
                }
                icectx->release( hicectx, func );
            }
        }
    }
    else
    {
        status = E_IC_INVALID_INPPARAM;
    }
    return(status);
}

/*
** Name: ic_deleterole
**
** Description:
**      Delete the named role.
**
** Inputs:
**      hicectx         Handle to the connection context returned from a
**                      a call to ic_initialize.
**      name            Name of the role to create
**
** Outputs:
**      None.
**
** Returns:
**      OK                          Completed successfully
**      E_IC_INVALID_INPARAM        Specified handle does not reference a
**                                  valid context or the name has not been
**                                  specified.
**
** History:
**      21-Feb-2001 (fanra01)
**          Created.
*/
ICE_STATUS
ic_deleterole( HICECTX hicectx, II_CHAR* name )
{
    ICE_STATUS status = OK;
    PICEAPI iceapi;
    ICE_C_PARAMS* param;
    II_CHAR* func;
    II_CHAR* roleid;
    PICESTK picestk;
    PICECTX     icectx;

    ic_getentry( hicectx, &icectx );

    if ((icectx != NULL) && (name != NULL))
    {
        status = ia_retfuncname( ICE_ROLE, &func );
        if ((status=ia_create( func , &iceapi )) == OK)
        {
            icectx->curr->iceapi = iceapi;
            if (status == OK)
            {
                status = iceapi->setaction( iceapi, IA_STR_DELETE );
                if (status == OK)
                {
                    icectx->saveapi( hicectx, &picestk );
                    status = icectx->getitem( hicectx, ICE_ROLE, "role_name",
                        name, "role_id", &roleid );
                    icectx->restapi( hicectx, picestk );
                    if (status == OK)
                    {
                        status = iceapi->getparmname( icectx->curr->iceapi,
                            "role_id", &param );
                        if (status == OK)
                        {
                            param->type = ICE_IN;
                            param->value= roleid;
                            status=ICE_C_Execute( icectx->sessid, func,
                                iceapi->fparms );
                            if (status != OK)
                            {
                                icectx->seterror( hicectx, status );
                            }
                        }
                    }
                }
            }
        }
    }
    else
    {
        status = E_IC_INVALID_INPPARAM;
    }
    return(status);
}

/*
** Name: ic_createdbuser
**
** Description:
**      Create an aliased ice database user.
**
** Inputs:
**      hicectx         Handle to the connection context returned from a
**                      a call to ic_initialize.
**      alias           Name that ice will use for the database user.
**      name            Actual database user name.
**      password        Password for database user.
**      comment         Optional comment for the entry.
**
** Outputs:
**      dbuserid        Pointer to returned dbuser id.
**
** Returns:
**      OK                          Completed successfully
**      E_IC_INVALID_INPARAM        Specified handle does not reference a
**                                  valid context or the name has not been
**                                  specified.
**
** History:
**      21-Feb-2001 (fanra01)
**          Created.
*/
ICE_STATUS
ic_createdbuser( HICECTX hicectx, II_CHAR* alias, II_CHAR* name,
    II_CHAR* password, II_CHAR* comment, II_INT4* dbuserid )
{
    ICE_STATUS status = OK;
    PICEAPI iceapi;
    ICE_C_PARAMS* param;
    ICE_C_PARAMS* usrparam;
    II_CHAR* func;
    PICECTX     icectx;

    ic_getentry( hicectx, &icectx );

    if ((icectx != NULL) && (name != NULL))
    {
        if (dbuserid != NULL)
        {
            status = ia_retfuncname( ICE_DBUSER, &func );
            if ((status=ia_create( func , &iceapi )) == OK)
            {
                icectx->curr->iceapi = iceapi;
                if (status == OK)
                {
                    status = iceapi->setaction( iceapi, IA_STR_INSERT );
                    if (status == OK)
                    {
                        status = iceapi->getparmpos( iceapi, 1, &usrparam );
                        if (status == OK)
                        {
                            usrparam->type = ICE_OUT;
                        }
                        status = iceapi->getparmpos( iceapi, 2, &param );
                        if (status == OK)
                        {
                            param->type = ICE_IN;
                            param->value = STRDUP( alias );
                        }
                        status = iceapi->getparmpos( iceapi, 3, &param );
                        if (status == OK)
                        {
                            param->type = ICE_IN;
                            param->value = STRDUP( name );
                        }
                        status = iceapi->getparmpos( iceapi, 4, &param );
                        if (status == OK)
                        {
                            param->type = ICE_IN;
                            param->value = STRDUP( password );
                        }
                        status = iceapi->getparmpos( iceapi, 5, &param );
                        if (status == OK)
                        {
                            param->type = ICE_IN;
                            param->value = STRDUP( password );
                        }
                        status = iceapi->getparmpos( iceapi, 6, &param );
                        if (status == OK)
                        {
                            param->type = ICE_IN;
                            param->value = STRDUP( comment );
                            status=ICE_C_Execute( icectx->sessid, func,
                                iceapi->fparms );
                            if (status != OK)
                            {
                                icectx->seterror( hicectx, status );
                            }
                            else
                            {
                                II_CHAR* value;

                                value = ICE_C_GetAttribute( icectx->sessid, 1 );
                                if (value != NULL)
                                {
                                    ASCTOI( value, dbuserid );
                                }
                                ICE_C_Close( icectx->sessid );
                            }
                        }
                    }
                }
                icectx->release( hicectx, func );
            }
        }
    }
    else
    {
        status = E_IC_INVALID_INPPARAM;
    }
    return(status);
}

/*
** Name: ic_deletedbuser
**
** Description:
**      Delete the database user entry specified by the alias name.
**
** Inputs:
**      hicectx         Handle to the connection context returned from a
**                      a call to ic_initialize.
**      alias           Ice alias name to delete.
**
** Outputs:
**
** Returns:
**      OK                          Completed successfully
**      E_IC_INVALID_INPARAM        Specified handle does not reference a
**                                  valid context or the name has not been
**                                  specified.
**
** History:
**      21-Feb-2001 (fanra01)
**          Created.
*/
ICE_STATUS
ic_deletedbuser( HICECTX hicectx, II_CHAR* alias )
{
    ICE_STATUS status = OK;
    PICEAPI iceapi;
    ICE_C_PARAMS* param;
    II_CHAR* func;
    II_CHAR* dbuserid;
    PICESTK picestk;
    PICECTX     icectx;

    ic_getentry( hicectx, &icectx );

    if ((icectx != NULL) && (alias != NULL))
    {
        status = ia_retfuncname( ICE_DBUSER, &func );
        if ((status=ia_create( func , &iceapi )) == OK)
        {
            icectx->curr->iceapi = iceapi;
            if (status == OK)
            {
                status = iceapi->setaction( iceapi, IA_STR_DELETE );
                if (status == OK)
                {
                    icectx->saveapi( hicectx, &picestk );
                    status = icectx->getitem( hicectx, ICE_DBUSER, "dbuser_alias",
                        alias, "dbuser_id", &dbuserid );
                    icectx->restapi( hicectx, picestk );
                    if (status == OK)
                    {
                        status = iceapi->getparmname( icectx->curr->iceapi,
                            "dbuser_id", &param );
                        if (status == OK)
                        {
                            param->type = ICE_IN;
                            param->value= dbuserid;
                            status=ICE_C_Execute( icectx->sessid, func,
                                iceapi->fparms );
                            if (status != OK)
                            {
                                icectx->seterror( hicectx, status );
                            }
                        }
                    }
                }
            }
        }
    }
    else
    {
        status = E_IC_INVALID_INPPARAM;
    }
    return(status);
}

/*
** Name: ic_createuser
**
** Description:
**      Create a named ICE user.
**
** Inputs:
**      hicectx         Handle to the connection context returned from a
**                      a call to ic_initialize.
**      name            Name of user to create.
**      password        Password for authentication.
**      dbuserid        Default database user identifer.
**      usrflag         Flags to define additional permissions.
**                          IA_USER_ADM
**                          IA_USER_SEC
**                          IA_USER_UNIT
**                          IA_USER_MONIT
**      timeout         Inactivity timeout after which the session is removed.
**      comment         Optional comment string.
**      authtype        Authentication type.
**                          IA_STR_OS_AUTH
**                          IA_STR_ICE_AUTH
**
** Outputs:
**      userid          Pointer to returned user identifier.
**
** Returns:
**      OK                          Completed successfully
**      E_IC_INVALID_INPARAM        Specified handle does not reference a
**                                  valid context or name argument has not
**                                  been specified.
**
** History:
**      21-Feb-2001 (fanra01)
**          Created.
*/
ICE_STATUS
ic_createuser( HICECTX hicectx, II_CHAR* name, II_CHAR* password,
    II_INT4 dbuserid, II_INT4 usrflag, II_INT4 timeout, II_CHAR* comment,
    II_INT4 authtype, II_INT4* userid )
{
    ICE_STATUS status = OK;
    PICEAPI iceapi;
    ICE_C_PARAMS* param;
    ICE_C_PARAMS* usrparam;
    II_CHAR* func;
    II_CHAR parmstr[20];
    PICECTX     icectx;

    ic_getentry( hicectx, &icectx );

    if ((icectx != NULL) && (name != NULL))
    {
        if (userid != NULL)
        {
            status = ia_retfuncname( ICE_USER, &func );
            if ((status=ia_create( func , &iceapi )) == OK)
            {
                icectx->curr->iceapi = iceapi;
                if (status == OK)
                {
                    status = iceapi->setaction( iceapi, IA_STR_INSERT );
                    if (status == OK)
                    {
                        status = iceapi->getparmpos( iceapi, 1, &usrparam );
                        if (status == OK)
                        {
                            usrparam->type = ICE_OUT;
                        }
                        status = iceapi->getparmpos( iceapi, 2, &param );
                        if (status == OK)
                        {
                            param->type = ICE_IN;
                            param->value = STRDUP( name );
                        }
                        status = iceapi->getparmpos( iceapi, 3, &param );
                        if (status == OK)
                        {
                            param->type = ICE_IN;
                            param->value = STRDUP( password );
                        }
                        status = iceapi->getparmpos( iceapi, 4, &param );
                        if (status == OK)
                        {
                            param->type = ICE_IN;
                            param->value = STRDUP( password );
                        }
                        status = iceapi->getparmpos( iceapi, 5, &param );
                        if (status == OK)
                        {
                            param->type = ICE_IN;
                            STRPRINTF( parmstr, "%d", dbuserid );
                            param->value = STRDUP( parmstr );
                        }
                        status = iceapi->getparmpos( iceapi, 6, &param );
                        if (status == OK)
                        {
                            param->type = ICE_IN;
                            param->value = STRDUP( comment );
                        }
                        status = iceapi->getparmpos( iceapi, 7, &param );
                        if (status == OK)
                        {
                            if (usrflag & IA_USER_ADM)
                            {
                                param->type = ICE_IN;
                                param->value = STRDUP( IA_STR_CHECKED );
                            }
                        }
                        status = iceapi->getparmpos( iceapi, 8, &param );
                        if (status == OK)
                        {
                            if (usrflag & IA_USER_SEC)
                            {
                                param->type = ICE_IN;
                                param->value = STRDUP( IA_STR_CHECKED );
                            }
                        }
                        status = iceapi->getparmpos( iceapi, 9, &param );
                        if (status == OK)
                        {
                            if (usrflag & IA_USER_UNIT)
                            {
                                param->type = ICE_IN;
                                param->value = STRDUP( IA_STR_CHECKED );
                            }
                        }
                        status = iceapi->getparmpos( iceapi, 11, &param );
                        if (status == OK)
                        {
                            if (usrflag & IA_USER_MONIT)
                            {
                                param->type = ICE_IN;
                                param->value = STRDUP( IA_STR_CHECKED );
                            }
                        }
                        status = iceapi->getparmpos( iceapi, 10, &param );
                        if (status == OK)
                        {
                            param->type = ICE_IN;
                            STRPRINTF( parmstr, "%d", timeout );
                            param->value = STRDUP( parmstr );
                        }
                        status = iceapi->getparmpos( iceapi, 12, &param );
                        if (status == OK)
                        {
                            param->type = ICE_IN;
                            switch(authtype)
                            {
                                case IA_OS_AUTH:
                                    param->value = STRDUP( IA_STR_OS_AUTH );
                                    break;
                                case IA_ICE_AUTH:
                                default:
                                    param->value = STRDUP( IA_STR_ICE_AUTH );
                                    break;
                            }
                            status=ICE_C_Execute( icectx->sessid, func,
                                iceapi->fparms );
                            if (status != OK)
                            {
                                icectx->seterror( hicectx, status );
                            }
                            else
                            {
                                II_CHAR* value;

                                value = ICE_C_GetAttribute( icectx->sessid, 1 );
                                if (value != NULL)
                                {
                                    ASCTOI( value, userid );
                                }
                                ICE_C_Close( icectx->sessid );
                            }
                        }
                    }
                }
                icectx->release( hicectx, func );
            }
        }
    }
    else
    {
        status = E_IC_INVALID_INPPARAM;
    }
    return(status);
}

/*
** Name: ic_deleteuser
**
** Description:
**      Delete the named ICE user.
**
** Inputs:
**      hicectx         Handle to the connection context returned from a
**                      a call to ic_initialize.
**      name            Name of user to remove.
**
** Outputs:
**      None.
**
** Returns:
**      OK                          Completed successfully
**      E_IC_INVALID_INPARAM        Specified handle does not reference a
**                                  valid context or name argument has not
**                                  been specified.
**
** History:
**      21-Feb-2001 (fanra01)
**          Created.
*/
ICE_STATUS
ic_deleteuser( HICECTX hicectx, II_CHAR* name )
{
    ICE_STATUS status = OK;
    PICEAPI iceapi;
    ICE_C_PARAMS* param;
    II_CHAR* func;
    II_CHAR* userid;
    PICESTK picestk;
    PICECTX     icectx;

    ic_getentry( hicectx, &icectx );

    if ((icectx != NULL) && (name != NULL))
    {
        status = ia_retfuncname( ICE_USER, &func );
        if ((status=ia_create( func , &iceapi )) == OK)
        {
            icectx->curr->iceapi = iceapi;
            if (status == OK)
            {
                status = iceapi->setaction( iceapi, IA_STR_DELETE );
                if (status == OK)
                {
                    icectx->saveapi( hicectx, &picestk );
                    status = icectx->getitem( hicectx, ICE_USER, "user_name",
                        name, "user_id", &userid );
                    icectx->restapi( hicectx, picestk );
                    if (status == OK)
                    {
                        status = iceapi->getparmname( icectx->curr->iceapi,
                            "user_id", &param );
                        if (status == OK)
                        {
                            param->type = ICE_IN;
                            param->value= userid;
                            status=ICE_C_Execute( icectx->sessid, func,
                                iceapi->fparms );
                            if (status != OK)
                            {
                                icectx->seterror( hicectx, status );
                            }
                        }
                    }
                }
            }
        }
    }
    else
    {
        status = E_IC_INVALID_INPPARAM;
    }
    return(status);
}

/*
** Name: ic_createloc
**
** Description:
**      Create a location entry in the repository.
**
** Inputs:
**      hicectx         Handle to the connection context returned from a
**                      a call to ic_initialize.
**      locname         Name of the location to be used within ICE.
**      type            Type of location.
**                          IA_HTTP_LOC
**                          IA_II_LOC
**                          IA_PUBLIC_LOC
**      path            Path of location.  Physical path if IA_II_LOC or
**                      relative path if IA_HTTP_LOC.
**      ext             Optional extension parameter if the location will
**                      be used for certain file extensions.
**
** Outputs:
**      locid           Pointer to returned location identifier.
**
** Returns:
**      OK                          Completed successfully
**      E_IC_INVALID_INPARAM        Specified handle does not reference a
**                                  valid context or locname or path argument
**                                  have not been specified.
**
** History:
**      21-Feb-2001 (fanra01)
**          Created.
*/
ICE_STATUS
ic_createloc( HICECTX hicectx, II_CHAR* locname, II_INT4 type,
    II_CHAR* path, II_CHAR* ext, II_INT4* locid )
{
    ICE_STATUS status = OK;
    PICEAPI iceapi;
    ICE_C_PARAMS* param;
    ICE_C_PARAMS* locparam;
    II_CHAR* func;
    PICECTX     icectx;

    ic_getentry( hicectx, &icectx );

    if ((icectx != NULL) && (locname != NULL) && (path != NULL) )
    {
        if (locid != NULL)
        {
            status = ia_retfuncname( ICE_LOCATION, &func );
            if ((status=ia_create( func , &iceapi )) == OK)
            {
                icectx->curr->iceapi = iceapi;
                if (status == OK)
                {
                    status = iceapi->setaction( iceapi, IA_STR_INSERT );
                    if (status == OK)
                    {
                        status = iceapi->getparmpos( iceapi, 1, &locparam );
                        if (status == OK)
                        {
                            locparam->type = ICE_OUT;
                        }
                        status = iceapi->getparmpos( iceapi, 2, &param );
                        if (status == OK)
                        {
                            param->type = ICE_IN;
                            param->value = STRDUP( locname );
                        }
                        status = iceapi->getparmpos( iceapi, 3, &param );
                        if (status == OK)
                        {
                            param->type = ICE_IN;
                            param->value = STRDUP( path );
                        }
                        if (ext != NULL)
                        {
                            status = iceapi->getparmpos( iceapi, 4, &param );
                            if (status == OK)
                            {
                                param->type = ICE_IN;
                                param->value = STRDUP( ext );
                            }
                        }
                        status = iceapi->getparmpos( iceapi, 5, &param );
                        if (status == OK)
                        {
                            if (type & IA_HTTP_LOC)
                            {
                                param->type = ICE_IN;
                                param->value = STRDUP( IA_STR_CHECKED );
                            }
                        }
                        status = iceapi->getparmpos( iceapi, 6, &param );
                        if (status == OK)
                        {
                            if (type & IA_II_LOC)
                            {
                                param->type = ICE_IN;
                                param->value = STRDUP( IA_STR_CHECKED );
                            }
                        }
                        status = iceapi->getparmpos( iceapi, 7, &param );
                        if (status == OK)
                        {
                            if (type & IA_PUBLIC_LOC)
                            {
                                param->type = ICE_IN;
                                param->value = STRDUP( IA_STR_CHECKED );
                            }
                            status=ICE_C_Execute( icectx->sessid, func,
                                iceapi->fparms );
                            if (status != OK)
                            {
                                icectx->seterror( hicectx, status );
                            }
                            else
                            {
                                II_CHAR* value;

                                value = ICE_C_GetAttribute( icectx->sessid, 1 );
                                if (value != NULL)
                                {
                                    ASCTOI( value, locid );
                                }
                                ICE_C_Close( icectx->sessid );
                            }
                        }
                    }
                }
                icectx->release( hicectx, func );
            }
        }
    }
    else
    {
        status = E_IC_INVALID_INPPARAM;
    }
    return(status);
}

/*
** Name: ic_deleteloc
**
** Description:
**      Delete a location entry in the repository.
**
** Inputs:
**      hicectx         Handle to the connection context returned from a
**                      a call to ic_initialize.
**      locname         Name of the location to be used within ICE.
**
** Outputs:
**      None.
**
** Returns:
**      OK                          Completed successfully
**      E_IC_INVALID_INPARAM        Specified handle does not reference a
**                                  valid context or locname argument
**                                  has not been specified.
**
** History:
**      21-Feb-2001 (fanra01)
**          Created.
*/
ICE_STATUS
ic_deleteloc( HICECTX hicectx, II_CHAR* locname )
{
    ICE_STATUS status = OK;
    PICEAPI iceapi;
    ICE_C_PARAMS* param;
    II_CHAR* func;
    II_CHAR* locid;
    PICESTK picestk;
    PICECTX     icectx;

    ic_getentry( hicectx, &icectx );

    if ((icectx != NULL) && (locname != NULL))
    {
        status = ia_retfuncname( ICE_LOCATION, &func );
        if ((status=ia_create( func , &iceapi )) == OK)
        {
            icectx->curr->iceapi = iceapi;
            if (status == OK)
            {
                status = iceapi->setaction( iceapi, IA_STR_DELETE );
                if (status == OK)
                {
                    icectx->saveapi( hicectx, &picestk );
                    status = icectx->getitem( hicectx, ICE_LOCATION, "loc_name",
                        locname, "loc_id", &locid );
                    icectx->restapi( hicectx, picestk );
                    if (status == OK)
                    {
                        status = iceapi->getparmname( icectx->curr->iceapi,
                            "user_id", &param );
                        if (status == OK)
                        {
                            param->type = ICE_IN;
                            param->value= locid;
                            status=ICE_C_Execute( icectx->sessid, func,
                                iceapi->fparms );
                            if (status != OK)
                            {
                                icectx->seterror( hicectx, status );
                            }
                        }
                    }
                }
            }
        }
    }
    else
    {
        status = E_IC_INVALID_INPPARAM;
    }
    return(status);
}

/*
** Name: ic_createbu
**
** Description:
**      Create a business unit.
**
** Inputs:
**      hicectx         Handle to the connection context returned from a
**                      a call to ic_initialize.
**      buname          Name of business unit to create.
**      owner           Name of ICE user who owns this business unit.
**
** Outputs:
**      buid            Pointer to returned business unit identifier.
**
** Returns:
**      OK                          Completed successfully
**      E_IC_INVALID_INPARAM        Specified handle does not reference a
**                                  valid context or locname argument
**                                  has not been specified.
**
** History:
**      21-Feb-2001 (fanra01)
**          Created.
*/
ICE_STATUS
ic_createbu( HICECTX hicectx, II_CHAR* buname, II_CHAR* owner, II_INT4* buid )
{
    ICE_STATUS status = OK;
    PICEAPI iceapi;
    ICE_C_PARAMS* param;
    ICE_C_PARAMS* buparam;
    II_CHAR* func;
    PICECTX     icectx;

    ic_getentry( hicectx, &icectx );

    if ((icectx != NULL) && (buname != NULL) && (owner != NULL))
    {
        if (buid != NULL)
        {
            status = ia_retfuncname( ICE_UNIT, &func );
            if ((status=ia_create( func , &iceapi )) == OK)
            {
                icectx->curr->iceapi = iceapi;
                if (status == OK)
                {
                    status = iceapi->setaction( iceapi, IA_STR_INSERT );
                    if (status == OK)
                    {
                        status = iceapi->getparmpos( iceapi, 1, &buparam );
                        if (status == OK)
                        {
                            buparam->type = ICE_OUT;
                        }
                        status = iceapi->getparmpos( iceapi, 2, &param );
                        if (status == OK)
                        {
                            param->type = ICE_IN;
                            param->value = STRDUP( buname );
                        }
                        status = iceapi->getparmpos( iceapi, 3, &param );
                        if (status == OK)
                        {
                            param->type = ICE_IN;
                            param->value = STRDUP( owner );
                            status=ICE_C_Execute( icectx->sessid, func,
                                iceapi->fparms );
                            if (status != OK)
                            {
                                icectx->seterror( hicectx, status );
                            }
                            else
                            {
                                II_CHAR* value;

                                value = ICE_C_GetAttribute( icectx->sessid, 1 );
                                if (value != NULL)
                                {
                                    ASCTOI( value, buid );
                                }
                                ICE_C_Close( icectx->sessid );
                            }
                        }
                    }
                }
                icectx->release( hicectx, func );
            }
        }
    }
    else
    {
        status = E_IC_INVALID_INPPARAM;
    }
    return(status);
}

/*
** Name: ic_deletebu
**
** Description:
**      Delete a business unit.
**
** Inputs:
**      hicectx         Handle to the connection context returned from a
**                      a call to ic_initialize.
**      buname          Name of business unit to delete.
**
** Outputs:
**      None.
**
** Returns:
**      OK                          Completed successfully
**      E_IC_INVALID_INPARAM        Specified handle does not reference a
**                                  valid context or locname argument
**                                  has not been specified.
** History:
**      21-Feb-2001 (fanra01)
**          Created.
*/
ICE_STATUS
ic_deletebu( HICECTX hicectx, II_CHAR* buname )
{
    ICE_STATUS status = OK;
    PICEAPI iceapi;
    ICE_C_PARAMS* param;
    II_CHAR* func;
    II_CHAR* buid;
    PICESTK picestk;
    PICECTX     icectx;

    ic_getentry( hicectx, &icectx );

    if ((icectx != NULL) && (buname != NULL))
    {
        status = ia_retfuncname( ICE_UNIT, &func );
        if ((status=ia_create( func , &iceapi )) == OK)
        {
            icectx->curr->iceapi = iceapi;
            if (status == OK)
            {
                status = iceapi->setaction( iceapi, IA_STR_DELETE );
                if (status == OK)
                {
                    icectx->saveapi( hicectx, &picestk );
                    status = icectx->getitem( hicectx, ICE_UNIT, "unit_name",
                        buname, "unit_id", &buid );
                    icectx->restapi( hicectx, picestk );
                    if (status == OK)
                    {
                        status = iceapi->getparmname( icectx->curr->iceapi,
                            "unit_id", &param );
                        if (status == OK)
                        {
                            param->type = ICE_IN;
                            param->value= buid;
                            status=ICE_C_Execute( icectx->sessid, func,
                                iceapi->fparms );
                            if (status != OK)
                            {
                                icectx->seterror( hicectx, status );
                            }
                        }
                    }
                }
            }
        }
    }
    else
    {
        status = E_IC_INVALID_INPPARAM;
    }
    return(status);
}

/*
** Name: ic_createdoc
**
** Description:
**      Register a document in the ice repository.
**
** Inputs:
**      hicectx         Handle to the connection context returned from a
**                      a call to ic_initialize.
**      buid            Business unit identifier.
**      docname         Name portion of filename.
**      docext          Extension portion of filename.
**      doctype         Processing type of document
**                          IA_PAGE
**                          IA_FACET
**      locid           Location identifer for path to document.
**      docflags        Properties of document
**                          IA_PUBLIC
**                          IA_EXTERNAL
**                          IA_PRE_CACHE
**                          IA_FIX_CACHE
**                          IA_SESS_CACHE
**      extfilespec     Physical path and filename of file to be loaded into
**                      the repository.  Only used when IA_EXTERNAL is not set.
**      owner           Ice user name of owner of document.
**
** Outputs:
**      docid           Pointer to returned document identifier.
**
** Returns:
**      OK                          Completed successfully
**      E_IC_INVALID_INPARAM        Specified handle does not reference a
**                                  valid context or document name or document
**                                  extension or owner have not been specified.
**
** History:
**      21-Feb-2001 (fanra01)
**          Created.
*/
ICE_STATUS
ic_createdoc( HICECTX hicectx, II_INT4 buid, II_CHAR* docname,
     II_CHAR* docext, II_INT4 doctype, II_INT4 locid, II_INT4 docflags,
     II_CHAR* extfilespec, II_CHAR* owner, II_INT4* docid )
{
    ICE_STATUS status = OK;
    PICEAPI iceapi;
    ICE_C_PARAMS* param;
    ICE_C_PARAMS* docparam;
    II_CHAR* func;
    II_CHAR parmstr[20];
    PICESTK picestk;
    II_CHAR* buname;
    PICECTX     icectx;

    ic_getentry( hicectx, &icectx );

    if ((icectx != NULL) && (docname != NULL) && (docext != NULL) &&
        (owner != NULL))
    {
        if (docid != NULL)
        {
            status = ia_retfuncname( ICE_DOC, &func );
            if ((status=ia_create( func , &iceapi )) == OK)
            {
                icectx->curr->iceapi = iceapi;
                if (status == OK)
                {
                    status = iceapi->setaction( iceapi, IA_STR_INSERT );
                    if (status == OK)
                    {
                        status = iceapi->getparmpos( iceapi, 1, &docparam );
                        if (status == OK)
                        {
                            docparam->type = ICE_OUT;
                        }
                        status = iceapi->getparmpos( iceapi, 2, &param );
                        if (status == OK)
                        {
                            param->type = ICE_IN;
                            STRPRINTF( parmstr, "%d", buid );
                            param->value = STRDUP( parmstr );
                            icectx->saveapi( hicectx, &picestk );
                            status = icectx->getitem( hicectx, ICE_UNIT,
                                "unit_id", parmstr, "unit_name", &buname );
                            icectx->restapi( hicectx, picestk );
                            if ((status == OK) && (buname != NULL))
                            {
                                if ((status = iceapi->getparmpos( iceapi, 3,
                                    &param )) == OK)
                                {
                                    param->type = ICE_IN;
                                    param->value = STRDUP( buname );
                                }
                            }
                            else
                            {
                                if (status == OK)
                                {
                                    status = E_IC_INVALID_UNIT_NAME;
                                }
                            }
                        }
                        status = iceapi->getparmpos( iceapi, 4, &param );
                        if (status == OK)
                        {
                            param->type = ICE_IN;
                            param->value = STRDUP( docname );
                        }
                        status = iceapi->getparmpos( iceapi, 5, &param );
                        if (status == OK)
                        {
                            param->type = ICE_IN;
                            param->value = STRDUP( docext );
                        }
                        status = iceapi->getparmpos( iceapi, 6, &param );
                        if (status == OK)
                        {
                            if (docflags & IA_PUBLIC)
                            {
                                param->type = ICE_IN;
                                param->value = STRDUP( IA_STR_CHECKED );
                            }
                        }
                        status = iceapi->getparmpos( iceapi, 14, &param );
                        if (status == OK)
                        {
                            param->type = ICE_IN;
                            param->value = STRDUP( owner );
                        }
                        status = iceapi->getparmpos( iceapi, 16, &param );
                        if (status == OK)
                        {
                            param->type = ICE_IN;
                            if (doctype & IA_PAGE)
                            {
                                param->value = STRDUP( IA_STR_PAGE );
                            }
                            if (doctype & IA_FACET)
                            {
                                param->value = STRDUP( IA_STR_FACET );
                            }
                        }
                        if (docflags & IA_EXTERNAL)
                        {
                            status = iceapi->getparmpos( iceapi, 11, &param );
                            if (status == OK)
                            {
                                param->type = ICE_IN;
                                STRPRINTF( parmstr, "%d", locid );
                                param->value = STRDUP( parmstr );
                            }
                            status = iceapi->getparmpos( iceapi, 12, &param );
                            if (status == OK)
                            {
                                param->type = ICE_IN;
                                param->value = STRDUP( docname );
                            }
                            status = iceapi->getparmpos( iceapi, 13, &param );
                            if (status == OK)
                            {
                                param->type = ICE_IN;
                                param->value = STRDUP( docext );
                            }
                        }
                        else
                        {
                            status = iceapi->getparmpos( iceapi, 7, &param );
                            if (status == OK)
                            {
                                param->type = ICE_IN;
                                if (docflags & IA_PRE_CACHE)
                                {
                                    param->type = ICE_IN;
                                    param->value = STRDUP( IA_STR_CHECKED );
                                }
                            }
                            status = iceapi->getparmpos( iceapi, 8, &param );
                            if (status == OK)
                            {
                                param->type = ICE_IN;
                                if (docflags & IA_FIX_CACHE)
                                {
                                    param->type = ICE_IN;
                                    param->value = STRDUP( IA_STR_CHECKED );
                                }
                            }
                            status = iceapi->getparmpos( iceapi, 9, &param );
                            if (status == OK)
                            {
                                param->type = ICE_IN;
                                if (docflags & IA_SESS_CACHE)
                                {
                                    param->type = ICE_IN;
                                    param->value = STRDUP( IA_STR_CHECKED );
                                }
                            }
                            if (extfilespec != NULL)
                            {
                                if ((status = iceapi->getparmpos( iceapi, 10,
                                    &param )) == OK)
                                {
                                    param->type = ICE_IN | ICE_BLOB;
                                    param->value = STRDUP( extfilespec );
                                }
                            }
                        }
                        if (status == OK)
                        {
                            status=ICE_C_Execute( icectx->sessid, func,
                                iceapi->fparms );
                            if (status != OK)
                            {
                                icectx->seterror( hicectx, status );
                            }
                            else
                            {
                                II_CHAR* value;

                                value = ICE_C_GetAttribute( icectx->sessid, 1 );
                                if (value != NULL)
                                {
                                    ASCTOI( value, docid );
                                }
                                ICE_C_Close( icectx->sessid );
                            }
                        }
                    }
                }
                icectx->release( hicectx, func );
            }
        }
    }
    else
    {
        status = E_IC_INVALID_INPPARAM;
    }
    return(status);
}

/*
** Name: ic_deletedoc
**
** Description:
**      Deregister a document from the ice repository.
**
** Inputs:
**      hicectx         Handle to the connection context returned from a
**                      a call to ic_initialize.
**      docname         Repository name of the document.
**      docext          Repository extension of the document.
**      locid           Location identifier for path of document.
**
** Outputs:
**      None.
**
** Returns:
**      OK                          Completed successfully
**      E_IC_INVALID_INPARAM        Specified handle does not reference a
**                                  valid context or document name or document
**                                  extension have not been specified.
**
** History:
**      21-Feb-2001 (fanra01)
**          Created.
*/
ICE_STATUS
ic_deletedoc( HICECTX hicectx, II_CHAR* docname, II_CHAR* docext, II_INT4 locid )
{
    ICE_STATUS status = OK;
    PICEAPI iceapi;
    ICE_C_PARAMS* param;
    II_CHAR* func;
    II_CHAR* docid;
    PICESTK picestk;
    II_CHAR parmstr[20];
    PICECTX     icectx;

    ic_getentry( hicectx, &icectx );

    if ((icectx != NULL) && (docname != NULL) && (docext != NULL))
    {
        status = ia_retfuncname( ICE_DOC, &func );
        if ((status=ia_create( func , &iceapi )) == OK)
        {
            icectx->curr->iceapi = iceapi;
            if (status == OK)
            {
                status = iceapi->setaction( iceapi, IA_STR_DELETE );
                if (status == OK)
                {
                    if ((status = iceapi->createparms( func, &param )) == OK)
                    {
                        param[4].type = ICE_IN;
                        param[4].value = STRDUP( docname );
                        param[5].type = ICE_IN;
                        param[5].value = STRDUP( docext );
                        STRPRINTF( parmstr, "%d", locid );
                        param[11].type = ICE_IN;
                        param[11].value = STRDUP( parmstr );

                        icectx->saveapi( hicectx, &picestk );

                        status = icectx->getitemwith( hicectx, ICE_DOC, param,
                            "doc_id", &docid );

                        icectx->restapi( hicectx, picestk );
                        if (status == OK)
                        {
                            iceapi->destroyparms( param );
                            if (status == OK)
                            {
                                status = iceapi->getparmname(
                                    icectx->curr->iceapi,
                                    "doc_id", &param );
                            }
                        }
                        else
                        {
                            switch(status)
                            {
                                case E_IC_DATASET_EXHAUSTED:
                                case E_IC_OUT_OF_DATA:
                                    status = E_ID_INVALID_FILENAME;
                                    break;
                                default:
                                    break;
                            }
                        }
                    }
                    if (status == OK)
                    {
                        param->type = ICE_IN;
                        param->value= docid;
                        status=ICE_C_Execute( icectx->sessid, func,
                            iceapi->fparms );
                        if (status != OK)
                        {
                            icectx->seterror( hicectx, status );
                        }
                    }
                }
            }
        }
    }
    else
    {
        status = E_IC_INVALID_INPPARAM;
    }
    return(status);
}

/*
** Name: ic_createprofile
**
** Description:
**      Create a named profile.
**
** Inputs:
**      hicectx         Handle to the connection context returned from a
**                      a call to ic_initialize.
**      name            Name of profile to create.
**      dbuser          Database user identifier.
**      perms           Permissions mask
**                          IA_PROF_ADM
**                          IA_PROF_SEC
**                          IA_PROF_UNIT
**                          IA_PROF_MONIT
**      timeout         Timeout on inactivity value.
**
** Outputs:
**      profid          Pointer to returned profile identifier.
**
** Returns:
**      OK                          Completed successfully
**      E_IC_INVALID_INPARAM        Specified handle does not reference a
**                                  valid context or profile name has not
**                                  been specified.
**
** History:
**      21-Feb-2001 (fanra01)
**          Created.
*/
ICE_STATUS
ic_createprofile( HICECTX hicectx, II_CHAR* name, II_INT4 dbuser, II_INT4 perms,
     II_INT4 timeout, II_INT4* profid )
{
    ICE_STATUS status = OK;
    PICEAPI iceapi;
    ICE_C_PARAMS* param;
    ICE_C_PARAMS* profparam;
    II_CHAR* func;
    II_CHAR parmstr[20];
    PICECTX     icectx;

    ic_getentry( hicectx, &icectx );

    if ((icectx != NULL) && (name != NULL))
    {
        if (profid != NULL)
        {
            status = ia_retfuncname( ICE_PROFILE, &func );
            if ((status=ia_create( func , &iceapi )) == OK)
            {
                icectx->curr->iceapi = iceapi;
                if (status == OK)
                {
                    status = iceapi->setaction( iceapi, IA_STR_INSERT );
                    if (status == OK)
                    {
                        status = iceapi->getparmpos( iceapi, 1, &profparam );
                        if (status == OK)
                        {
                            profparam->type = ICE_OUT;
                        }
                        status = iceapi->getparmpos( iceapi, 2, &param );
                        if (status == OK)
                        {
                            param->type = ICE_IN;
                            param->value = STRDUP( name );
                        }
                        status = iceapi->getparmpos( iceapi, 3, &param );
                        if (status == OK)
                        {
                            STRPRINTF( parmstr, "%d", dbuser );
                            param->type = ICE_IN;
                            param->value = STRDUP( parmstr );
                        }
                        if (perms > 0)
                        {
                            status = iceapi->getparmpos( iceapi, 4, &param );
                            if (status == OK)
                            {
                                if (perms & IA_PROF_ADM)
                                {
                                    param->type = ICE_IN;
                                    param->value = STRDUP( IA_STR_CHECKED );
                                }
                            }
                            status = iceapi->getparmpos( iceapi, 5, &param );
                            if (status == OK)
                            {
                                if (perms & IA_PROF_SEC)
                                {
                                    param->type = ICE_IN;
                                    param->value = STRDUP( IA_STR_CHECKED );
                                }
                            }
                            status = iceapi->getparmpos( iceapi, 6, &param );
                            if (status == OK)
                            {
                                if (perms & IA_PROF_UNIT)
                                {
                                    param->type = ICE_IN;
                                    param->value = STRDUP( IA_STR_CHECKED );
                                }
                            }
                            status = iceapi->getparmpos( iceapi, 7, &param );
                            if (status == OK)
                            {
                                if (perms & IA_PROF_MONIT)
                                {
                                    param->type = ICE_IN;
                                    param->value = STRDUP( IA_STR_CHECKED );
                                }
                            }
                        }
                        status = iceapi->getparmpos( iceapi, 8, &param );
                        if (status == OK)
                        {
                            param->type = ICE_IN;
                            STRPRINTF( parmstr, "%d", (timeout > 0) ? timeout : 0 );
                            param->value = STRDUP( parmstr );
                        }
                        status=ICE_C_Execute( icectx->sessid, func,
                            iceapi->fparms );
                        if (status != OK)
                        {
                            icectx->seterror( hicectx, status );
                        }
                        else
                        {
                            II_CHAR* value;

                            value = ICE_C_GetAttribute( icectx->sessid, 1 );
                            if (value != NULL)
                            {
                                ASCTOI( value, profid );
                            }
                            ICE_C_Close( icectx->sessid );
                        }
                    }
                }
                icectx->release( hicectx, func );
            }
        }
    }
    else
    {
        status = E_IC_INVALID_INPPARAM;
    }
    return(status);
}

/*
** Name: ic_deleteprofile
**
** Description:
**      Delete the named ice profile.
**
** Inputs:
**      hicectx         Handle to the connection context returned from a
**                      a call to ic_initialize.
**      name            Name of profile to delete.
**
** Outputs:
**      None.
**
** Returns:
**      OK                          Completed successfully
**      E_IC_INVALID_INPARAM        Specified handle does not reference a
**                                  valid context or profile name has not
**                                  been specified.
**
** History:
**      21-Feb-2001 (fanra01)
**          Created.
*/
ICE_STATUS
ic_deleteprofile( HICECTX hicectx, II_CHAR* name )
{
    ICE_STATUS status = OK;
    PICEAPI iceapi;
    ICE_C_PARAMS* param;
    II_CHAR* func;
    II_CHAR* profid;
    PICESTK picestk;
    PICECTX     icectx;

    ic_getentry( hicectx, &icectx );

    if ((icectx != NULL) && (name != NULL))
    {
        status = ia_retfuncname( ICE_PROFILE, &func );
        if ((status=ia_create( func , &iceapi )) == OK)
        {
            icectx->curr->iceapi = iceapi;
            if (status == OK)
            {
                status = iceapi->setaction( iceapi, IA_STR_DELETE );
                if (status == OK)
                {
                    icectx->saveapi( hicectx, &picestk );
                    status = icectx->getitem( hicectx, ICE_PROFILE, "profile_name",
                        name, "profile_id", &profid );
                    icectx->restapi( hicectx, picestk );
                    if (status == OK)
                    {
                        status = iceapi->getparmname( icectx->curr->iceapi,
                            "profile_id", &param );
                        if (status == OK)
                        {
                            param->type = ICE_IN;
                            param->value= profid;
                            status=ICE_C_Execute( icectx->sessid, func,
                                iceapi->fparms );
                            if (status != OK)
                            {
                                icectx->seterror( hicectx, status );
                            }
                        }
                    }
                }
            }
        }
    }
    else
    {
        status = E_IC_INVALID_INPPARAM;
    }
    return(status);
}

/*
** Name: ic_assocunitrole
**
** Description:
**      Create an association between a business unit and a role.
**
** Inputs:
**      hicectx         Handle to the connection context returned from a
**                      a call to ic_initialize.
**      uname           Business unit name.
**      rname           Role name.
**      perms           Role permissions
**                          IA_EXE_PERM
**                          IA_READ_PERM
**                          IA_INSERT_PERM
** Outputs:
**      None.
**
** Returns:
**      OK                          Completed successfully
**      E_IC_INVALID_INPARAM        Specified handle does not reference a
**                                  valid context or unit name or role name
**                                  have not been specified.
**      E_IC_INVALID_ROLE_NAME      The role name was not found.
**      E_IC_INVALID_UNIT_NAME      The business unit name was not found.
**
** History:
**      21-Feb-2001 (fanra01)
**          Created.
*/
ICE_STATUS
ic_assocunitrole( HICECTX hicectx, II_CHAR* uname, II_CHAR* rname, II_INT4 perms )
{
    ICE_STATUS status = OK;
    PICEAPI iceapi;
    ICE_C_PARAMS* param;
    II_CHAR* func;
    PICESTK picestk;
    II_CHAR* buid;
    II_CHAR* rlid;
    PICECTX     icectx;

    ic_getentry( hicectx, &icectx );

    if ((icectx != NULL) && (uname != NULL) && (rname != NULL))
    {
        status = ia_retfuncname( ICE_UNITROLE, &func );
        if ((status=ia_create( func , &iceapi )) == OK)
        {
            icectx->curr->iceapi = iceapi;
            if (status == OK)
            {
                status = iceapi->setaction( iceapi, IA_STR_UPDATE );
                if (status == OK)
                {
                    icectx->saveapi( hicectx, &picestk );
                    status = icectx->getitem( hicectx, ICE_UNIT, "unit_name",
                        uname, "unit_id", &buid );
                    icectx->restapi( hicectx, picestk );
                    if ((status == OK) && (buid != NULL))
                    {
                        icectx->saveapi( hicectx, &picestk );
                            status = icectx->getitem( hicectx, ICE_ROLE,
                                "role_name", rname, "role_id", &rlid );
                        icectx->restapi( hicectx, picestk );
                        if ((status == OK) && (rlid != NULL))
                        {
                            status = iceapi->getparmpos( iceapi, 1, &param );
                            if (status == OK)
                            {
                                param->type = ICE_IN;
                                param->value = buid;
                            }
                            status = iceapi->getparmpos( iceapi, 2, &param );
                            if (status == OK)
                            {
                                param->type = ICE_IN;
                                param->value = rlid;
                            }
                            status = iceapi->getparmpos( iceapi, 4, &param );
                            if (status == OK)
                            {
                                if (perms & IA_EXE_PERM)
                                {
                                    param->type = ICE_IN;
                                    param->value = STRDUP( IA_STR_CHECKED );
                                }
                            }
                            status = iceapi->getparmpos( iceapi, 5, &param );
                            if (status == OK)
                            {
                                if (perms & IA_READ_PERM)
                                {
                                    param->type = ICE_IN;
                                    param->value = STRDUP( IA_STR_CHECKED );
                                }
                            }
                            status = iceapi->getparmpos( iceapi, 6, &param );
                            if (status == OK)
                            {
                                if (perms & IA_INSERT_PERM)
                                {
                                    param->type = ICE_IN;
                                    param->value = STRDUP( IA_STR_CHECKED );
                                }
                            }
                            status=ICE_C_Execute( icectx->sessid, func,
                                iceapi->fparms );
                            if (status != OK)
                            {
                                icectx->seterror( hicectx, status );
                            }
                        }
                        else
                        {
                            if (status == OK)
                            {
                                status = E_IC_INVALID_ROLE_NAME;
                            }
                        }
                    }
                    else
                    {
                        if (status == OK)
                        {
                            status = E_IC_INVALID_UNIT_NAME;
                        }
                    }
                }
            }
        }
    }
    else
    {
        status = E_IC_INVALID_INPPARAM;
    }
    return(status);
}

/*
** Name: ic_releaseunitrole
**
** Description:
**      Remove an association between the named business unit and the
**      named role.
**
** Inputs:
**      hicectx         Handle to the connection context returned from a
**                      a call to ic_initialize.
**      uname           Business unit name.
**      rname           Role name.
**
** Outputs:
**      None.
**
** Returns:
**      OK                          Completed successfully
**      E_IC_INVALID_INPARAM        Specified handle does not reference a
**                                  valid context or unit name or role name
**                                  have not been specified.
**      E_IC_INVALID_ROLE_NAME      The role name was not found.
**      E_IC_INVALID_UNIT_NAME      The business unit name was not found.
**
** History:
**      21-Feb-2001 (fanra01)
**          Created.
*/
ICE_STATUS
ic_releaseunitrole( HICECTX hicectx, II_CHAR* uname, II_CHAR* rname )
{
    ICE_STATUS status = OK;
    PICEAPI iceapi;
    ICE_C_PARAMS* param;
    II_CHAR* func;
    PICESTK picestk;
    II_CHAR* buid;
    II_CHAR* rlid;
    PICECTX     icectx;

    ic_getentry( hicectx, &icectx );

    if ((icectx != NULL) && (uname != NULL) && (rname != NULL))
    {
        status = ia_retfuncname( ICE_UNITROLE, &func );
        if ((status=ia_create( func , &iceapi )) == OK)
        {
            icectx->curr->iceapi = iceapi;
            if (status == OK)
            {
                status = iceapi->setaction( iceapi, IA_STR_UPDATE );
                if (status == OK)
                {
                    icectx->saveapi( hicectx, &picestk );
                    status = icectx->getitem( hicectx, ICE_UNIT, "unit_name",
                        uname, "unit_id", &buid );
                    icectx->restapi( hicectx, picestk );
                    if ((status == OK) && (buid != NULL))
                    {
                        icectx->saveapi( hicectx, &picestk );
                            status = icectx->getitem( hicectx, ICE_ROLE,
                                "role_name", rname, "role_id", &rlid );
                        icectx->restapi( hicectx, picestk );
                        if ((status == OK) && (rlid != NULL))
                        {
                            status = iceapi->getparmpos( iceapi, 4, &param );
                            if (status == OK)
                            {
                                param->type = ICE_IN;
                                param->value = NULL;
                            }
                            status = iceapi->getparmpos( iceapi, 5, &param );
                            if (status == OK)
                            {
                                param->type = ICE_IN;
                                param->value = NULL;
                            }
                            status = iceapi->getparmpos( iceapi, 6, &param );
                            if (status == OK)
                            {
                                param->type = ICE_IN;
                                param->value = NULL;
                            }
                            status=ICE_C_Execute( icectx->sessid, func,
                                iceapi->fparms );
                            if (status != OK)
                            {
                                icectx->seterror( hicectx, status );
                            }
                        }
                        else
                        {
                            if (status == OK)
                            {
                                status = E_IC_INVALID_ROLE_NAME;
                            }
                        }
                    }
                    else
                    {
                        if (status == OK)
                        {
                            status = E_IC_INVALID_UNIT_NAME;
                        }
                    }
                }
            }
        }
    }
    else
    {
        status = E_IC_INVALID_INPPARAM;
    }
    return(status);
}

/*
** Name: ic_assocprofilerole
**
** Description:
**      Create an association between a profile and a role.
**
** Inputs:
**      hicectx         Handle to the connection context returned from a
**                      a call to ic_initialize.
**      pname           Profile name.
**      rname           Role name.
**
** Outputs:
**      None.
**
** Returns:
**      OK                          Completed successfully
**      E_IC_INVALID_INPARAM        Specified handle does not reference a
**                                  valid context or profile name or role name
**                                  have not been specified.
**      E_IC_INVALID_ROLE_NAME      The role name was not found.
**      E_IC_INVALID_PROF_NAME      The profile name was not found.
**
** History:
**      21-Feb-2001 (fanra01)
**          Created.
*/
ICE_STATUS
ic_assocprofilerole( HICECTX hicectx, II_CHAR* pname, II_CHAR* rname )
{
    ICE_STATUS status = OK;
    PICEAPI iceapi;
    ICE_C_PARAMS* param;
    II_CHAR* func;
    PICESTK picestk;
    II_CHAR* profid;
    II_CHAR* rlid;
    PICECTX     icectx;

    ic_getentry( hicectx, &icectx );

    if ((icectx != NULL) && (pname != NULL) && (rname != NULL))
    {
        status = ia_retfuncname( ICE_PROFROLE, &func );
        if ((status=ia_create( func , &iceapi )) == OK)
        {
            icectx->curr->iceapi = iceapi;
            if (status == OK)
            {
                status = iceapi->setaction( iceapi, IA_STR_INSERT );
                if (status == OK)
                {
                    icectx->saveapi( hicectx, &picestk );
                    status = icectx->getitem( hicectx, ICE_PROFILE, "profile_name",
                        pname, "profile_id", &profid );
                    icectx->restapi( hicectx, picestk );
                    if ((status == OK) && (profid != NULL))
                    {
                        icectx->saveapi( hicectx, &picestk );
                        status = icectx->getitem( hicectx, ICE_ROLE,
                            "role_name", rname, "role_id", &rlid );
                        icectx->restapi( hicectx, picestk );
                        if ((status == OK) && (rlid != NULL))
                        {
                            status = iceapi->getparmpos( iceapi, 1, &param );
                            if (status == OK)
                            {
                                param->type = ICE_IN;
                                param->value = profid;
                            }
                            status = iceapi->getparmpos( iceapi, 2, &param );
                            if (status == OK)
                            {
                                param->type = ICE_IN;
                                param->value = rlid;
                            }
                            status=ICE_C_Execute( icectx->sessid, func,
                                iceapi->fparms );
                            if (status != OK)
                            {
                                icectx->seterror( hicectx, status );
                            }
                        }
                        else
                        {
                            if (status == OK)
                            {
                                status = E_IC_INVALID_ROLE_NAME;
                            }
                        }
                    }
                    else
                    {
                        if (status == OK)
                        {
                            status = E_IC_INVALID_PROF_NAME;
                        }
                    }
                }
            }
        }
    }
    else
    {
        status = E_IC_INVALID_INPPARAM;
    }
    return(status);
}

/*
** Name: ic_releaseprofilerole
**
** Description:
**      Remove an association between the named profile and the
**      named role.
**
** Inputs:
**      hicectx         Handle to the connection context returned from a
**                      a call to ic_initialize.
**      pname           Profile name.
**      rname           Role name.
**
** Outputs:
**      None.
**
** Returns:
**      OK                          Completed successfully
**      E_IC_INVALID_INPARAM        Specified handle does not reference a
**                                  valid context or profile name or role name
**                                  have not been specified.
**      E_IC_INVALID_ROLE_NAME      The role name was not found.
**      E_IC_INVALID_PROF_NAME      The profile name was not found.
**
** History:
**      21-Feb-2001 (fanra01)
**          Created.
*/
ICE_STATUS
ic_releaseprofilerole( HICECTX hicectx, II_CHAR* pname, II_CHAR* rname )
{
    ICE_STATUS status = OK;
    PICEAPI iceapi;
    ICE_C_PARAMS* param;
    II_CHAR* func;
    PICESTK picestk;
    II_CHAR* profid;
    II_CHAR* rlid;
    II_CHAR* profile;
    PICECTX     icectx;

    ic_getentry( hicectx, &icectx );

    if ((icectx != NULL) && (pname != NULL) && (rname != NULL))
    {
        status = ia_retfuncname( ICE_PROFROLE, &func );
        if ((status=ia_create( func , &iceapi )) == OK)
        {
            icectx->curr->iceapi = iceapi;
            if (status == OK)
            {
                status = iceapi->setaction( iceapi, IA_STR_DELETE );
                if (status == OK)
                {
                    icectx->saveapi( hicectx, &picestk );
                    status = icectx->getitem( hicectx, ICE_PROFILE, "profile_name",
                        pname, "profile_id", &profid );
                    icectx->restapi( hicectx, picestk );
                    if ((status == OK) && (profid != NULL))
                    {
                        icectx->saveapi( hicectx, &picestk );
                        status = icectx->getitem( hicectx, ICE_ROLE,
                            "role_name", rname, "role_id", &rlid );
                        icectx->restapi( hicectx, picestk );
                        if ((status == OK) && (rlid != NULL))
                        {
                            if ((status = iceapi->createparms( func, &param )) == OK)
                            {
                                param[1].type = ICE_IN;
                                param[1].value = STRDUP( profid );
                                param[2].type = ICE_IN;
                                param[2].value = STRDUP( rlid );

                                icectx->saveapi( hicectx, &picestk );

                                status = icectx->getitemwith( hicectx, ICE_PROFROLE, param,
                                    "profile_id", &profile );

                                icectx->restapi( hicectx, picestk );
                                iceapi->destroyparms( param );
                                if (status == OK)
                                {
                                    status = iceapi->getparmpos( iceapi, 1, &param );
                                    if (status == OK)
                                    {
                                        param->type = ICE_IN;
                                        param->value = profile;
                                    }
                                }
                            }
                            status=ICE_C_Execute( icectx->sessid, func,
                                iceapi->fparms );
                            if (status != OK)
                            {
                                icectx->seterror( hicectx, status );
                            }
                        }
                        else
                        {
                            if (status == OK)
                            {
                                status = E_IC_INVALID_ROLE_NAME;
                            }
                        }
                    }
                    else
                    {
                        if (status == OK)
                        {
                            status = E_IC_INVALID_PROF_NAME;
                        }
                    }
                }
            }
        }
    }
    else
    {
        status = E_IC_INVALID_INPPARAM;
    }
    return(status);
}

/*
** Name: ic_retvariable
**
** Description:
**      Return the value of the variable in the scope.
**
** Inputs:
**      hicectx         Handle to the connection context returned from a
**                      a call to ic_initialize.
**      scope           Scope of requested variable
**      name            Name of variable
**
** Outputs:
**      value           Value of requested variable.
**
** Returns:
**      OK                          Completed successfully
**      E_IC_INVALID_INPARAM        Specified handle does not reference a
**                                  valid context or profile name or role name
**                                  have not been specified.
**
** History:
**      21-Feb-2001 (fanra01)
**          Created.
*/
ICE_STATUS
ic_retvariable( HICECTX hicectx, II_CHAR* scope, II_CHAR* name,
    II_CHAR** value )
{
    ICE_STATUS  status = OK;
    PICECTX     icectx;
    II_CHAR*    func;
    PICEAPI     iceapi;
    ICE_C_PARAMS* param;
    ICE_C_PARAMS* valueparam;
    II_INT4     end;

    ic_getentry( hicectx, &icectx );

    if ((icectx != NULL) && (scope != NULL) && (name != NULL))
    {
        if (value != NULL)
        {
            status = ia_retfuncname( ICE_VAR, &func );
            if ((status=ia_create( func , &iceapi )) == OK)
            {
                icectx->curr->iceapi = iceapi;
                if (status == OK)
                {
                    status = iceapi->setaction( iceapi, IA_STR_RETRIEVE );
                    if (status == OK)
                    {
                        status = iceapi->getparmpos( iceapi, 1, &param );
                        if (status == OK)
                        {
                            param->type = ICE_IN;
                            param->value = STRDUP( scope );
                        }
                        status = iceapi->getparmpos( iceapi, 2, &param );
                        if (status == OK)
                        {
                            param->type = ICE_IN;
                            param->value = STRDUP( name );
                        }
                        status = iceapi->getparmpos( iceapi, 3, &valueparam );
                        if (status == OK)
                        {
                            valueparam->type = ICE_OUT;
                            status=ICE_C_Execute( icectx->sessid, func,
                                iceapi->fparms );
                            if (status != OK)
                            {
                                icectx->seterror( hicectx, status );
                            }
                            else
                            {
                                II_CHAR* var;
                                if ((status = ICE_C_Fetch( icectx->sessid,
                                    &end)) == OK)
                                {
                                    var = ICE_C_GetAttribute( icectx->sessid, 3 );
                                    if (var != NULL)
                                    {
                                        *value = STRDUP( var );
                                    }
                                }
                                ICE_C_Close( icectx->sessid );
                            }
                        }
                    }
                    icectx->release( hicectx, func );
                }
            }
        }
        else
        {
            status = E_IC_INVALID_OUTPARAM;
        }
    }
    else
    {
        status = E_IC_INVALID_INPPARAM;
    }
    return(status);
}

/*
** Name: ic_setvariable
**
** Description:
**      Set the value of the variable in the scope.
**
** Inputs:
**      hicectx         Handle to the connection context returned from a
**                      a call to ic_initialize.
**      scope           Scope of requested variable
**      name            Name of variable
**      value           Value of variable. NULL deletes the variable.
**
** Outputs:
**
** Returns:
**      OK                          Completed successfully
**      E_IC_INVALID_INPARAM        Specified handle does not reference a
**                                  valid context or profile name or role name
**                                  have not been specified.
**
** History:
**      21-Feb-2001 (fanra01)
**          Created.
*/
ICE_STATUS
ic_setvariable( HICECTX hicectx, II_CHAR* scope, II_CHAR* name,
    II_CHAR* value )
{
    ICE_STATUS  status = OK;
    PICECTX     icectx;
    II_CHAR*    func;
    PICEAPI     iceapi;
    ICE_C_PARAMS* param;

    ic_getentry( hicectx, &icectx );

    if ((icectx != NULL) && (scope != NULL) && (name != NULL))
    {
        status = ia_retfuncname( ICE_VAR, &func );
        if ((status=ia_create( func , &iceapi )) == OK)
        {
            icectx->curr->iceapi = iceapi;
            if (status == OK)
            {
                if (value != NULL)
                {
                    status = iceapi->setaction( iceapi, IA_STR_INSERT );
                }
                else
                {
                    status = iceapi->setaction( iceapi, IA_STR_DELETE );
                }
                if (status == OK)
                {
                    status = iceapi->getparmpos( iceapi, 1, &param );
                    if (status == OK)
                    {
                        param->type = ICE_IN;
                        param->value = STRDUP( scope );
                    }
                    status = iceapi->getparmpos( iceapi, 2, &param );
                    if (status == OK)
                    {
                        param->type = ICE_IN;
                        param->value = STRDUP( name );
                    }
                    status = iceapi->getparmpos( iceapi, 3, &param );
                    if (status == OK)
                    {
                        param->type = ICE_IN;
                        if (value != NULL)
                        {
                            param->value = STRDUP( value );
                        }
                        status=ICE_C_Execute( icectx->sessid, func,
                            iceapi->fparms );
                        if (status != OK)
                        {
                            icectx->seterror( hicectx, status );
                        }
                        else
                        {
                            ICE_C_Close( icectx->sessid );
                        }
                    }
                }
                icectx->release( hicectx, func );
            }
        }
    }
    else
    {
        status = E_IC_INVALID_INPPARAM;
    }
    return(status);
}
