/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
** Name: iceapi.c
**
** Description:
**      Module provides functions to create, manipulate and destroy
**      ICE API parameter structures.
**      Simplifying the use of ICE parameter structures for specific
**      functions.
**
**      ia_create           Create an ICE parameter list initialised for
**                          a particular function.
**      ia_destroy          Free the parameter list structure and the
**                          containing structure.
**      ia_retfuncname      Return the functions name for a given function
**                          number without the function context parameter.
**      ia_getfuncname      Return the functions name for a given function
**                          number.
**      ia_getaction        Return the value of the action parameter if it
**                          exists in the parameter list.
**      ia_setaction        Return the value of the action parameter if it
**                          exists in the parameter list.
**      ia_gettype          Return the ICE parameter type.
**      ia_settype          Set the ICE parameter type.
**      ia_getname          Return the name of the parameter.
**      ia_setname          Set the name of the parameter.
**      ia_getvalue         Return the value of the parameter.
**      ia_setvalue         Set the value of the parameter.
**      ia_getpos           Return the position number of the named parameter.
**      ia_getparmname      Return a pointer to the parameter by name.
**      ia_getparmpos       Return a pointer to the parameter by position.
**      ia_getcount         Return the maximum number of parameters.
**      ia_getnumber        Return the function number for the current
**                          parameters.
**      ia_createparms      Create parameter list only.
**      ia_destroyparms     Destroy parameter list.
**
** History:
**      21-Feb-2001 (fanra01)
**          Sir 103813
**          Created.
**      04-May-2001 (fanra01)
**          sir 103813
**          Add ice_var and parameters to list of functions.  Also add a
**          test action function for determining if an action is permitted.
*/
# include <iiapi.h>
# include <iceconf.h>

# include "icelocal.h"

II_CHAR*   srvparms[31][30] = {

/* DBuser          */   { "action", "dbuser_id", "dbuser_alias", "dbuser_name", "dbuser_password1", "dbuser_password2", "dbuser_comment", NULL},
/* Role            */   { "action", "role_id", "role_name", "role_comment", NULL},
/* User            */   { "action", "user_id", "user_name", "user_password1", "user_password2", "user_dbuser", "user_comment", "user_administration", "user_security", "user_unit", "user_timeout", "user_monitor", "user_authtype", NULL},
/* DB              */   { "action", "db_id", "db_name", "db_dbname", "db_dbuser", "db_comment", NULL},
/* UserRole        */   { "action", "ur_user_id", "ur_role_id", "ur_role_name", NULL},
/* UserDB          */   { "action", "ud_user_id", "ud_db_id", "ud_db_name", NULL},
/* sess            */   { "action", "sess_id", "sess_name", NULL},
/* unit            */   { "action", "unit_id", "unit_name", "unit_owner", NULL},
/* doc             */   { "action", "doc_id", "doc_unit_id", "doc_unit_name", "doc_name",
                          "doc_suffix", "doc_public", "doc_pre_cache", "doc_perm_cache",
                          "doc_session_cache", "doc_file", "doc_ext_loc", "doc_ext_file",
                          "doc_ext_suffix", "doc_owner", "doc_transfer" , "doc_type", NULL},
/* docRole         */   { "action", "dr_doc_id", "dr_role_id", "dr_role_name", "dr_read", "dr_update", "dr_delete", "dr_execute", NULL},
/* docUser         */   { "action", "du_doc_id", "du_user_id", "du_user_name", "du_read", "du_update", "du_delete", "du_execute", NULL},
/* unitRole        */   { "action", "ur_unit_id", "ur_role_id", "ur_role_name", "ur_execute", "ur_read", "ur_insert", NULL},
/* unitUser        */   { "action", "uu_unit_id", "uu_user_id", "uu_user_name", "uu_execute", "uu_read", "uu_insert", NULL},
/* unitLoc         */   { "action", "ul_unit_id", "ul_location_id", "ul_location_name", NULL},
/* unitCopy        */   { "action", "unit_id", "copy_file", "default_loc", NULL},
/* location        */   { "action", "loc_id", "loc_name", "loc_path", "loc_extensions", "loc_http", "loc_ice", "loc_public", NULL},
/* activeSession   */   { "action", "name", "ice_user", "host", "query", "err_count", NULL},
/* userSession     */   { "action", "name", "user", "req_count", "timeout", NULL},
/* userTransaction */   { "action", "key", "name", "owner", "connection", NULL},
/* userCursor      */   { "action", "key", "name", "query", "owner", NULL},
/* cache           */   { "action", "key", "name", "loc_name", "status", "file_counter", "exist", "owner", "timeout", "in_use", "req_count", NULL},
/* connection      */   { "action", "key", "driver", "dbname", "used", "timeout", NULL},
/* profile         */   { "action", "profile_id", "profile_name", "profile_dbuser", "profile_administration", "profile_security", "profile_unit", "profile_monitor", "profile_timeout", NULL},
/* profileRole     */   { "action", "pr_profile_id", "pr_role_id", "pr_role_name", NULL},
/* profileDB       */   { "action", "pd_profile_id", "pd_db_id", "pd_db_name", NULL},
/* tag2String      */   { "tags", "string", NULL},
/* getLocFiles     */   { "location_id", "prefix", "suffix", NULL},
/* getVariables    */   { "page", "session", "server", "name", "value", NULL},
/* getSvrVariables */   { "action", "name", "value", NULL},
/* ice_var         */   { "action", "scope", "name", "value", NULL },
                        { NULL, NULL }
};

SRVFUNC srvfunc[] =
{
    { "dbuser",                 &srvparms[0][0] , IA_SELECT | IA_RETRIEVE | IA_INSERT | IA_UPDATE | IA_DELETE },
    { "role",                   &srvparms[1][0] , IA_SELECT | IA_RETRIEVE | IA_INSERT | IA_UPDATE | IA_DELETE },
    { "user",                   &srvparms[2][0] , IA_SELECT | IA_RETRIEVE | IA_INSERT | IA_UPDATE | IA_DELETE },
    { "database",               &srvparms[3][0] , IA_SELECT | IA_RETRIEVE | IA_INSERT | IA_UPDATE | IA_DELETE },
    { "user_role",              &srvparms[4][0] , IA_SELECT | IA_INSERT | IA_DELETE },
    { "user_database",          &srvparms[5][0] , IA_SELECT | IA_INSERT | IA_DELETE },
    { "session_grp",            &srvparms[6][0] , IA_SELECT | IA_RETRIEVE | IA_INSERT | IA_UPDATE | IA_DELETE },
    { "unit",                   &srvparms[7][0] , IA_SELECT | IA_RETRIEVE | IA_INSERT | IA_UPDATE | IA_DELETE },
    { "document",               &srvparms[8][0] , IA_SELECT | IA_RETRIEVE | IA_INSERT | IA_UPDATE | IA_DELETE },
    { "document_role",          &srvparms[9][0] , IA_RETRIEVE | IA_UPDATE },
    { "document_user",          &srvparms[10][0], IA_RETRIEVE | IA_UPDATE },
    { "unit_role",              &srvparms[11][0], IA_RETRIEVE | IA_UPDATE },
    { "unit_user",              &srvparms[12][0], IA_RETRIEVE | IA_UPDATE },
    { "unit_location",          &srvparms[13][0], IA_SELECT | IA_INSERT | IA_DELETE },
    { "unit_copy",              &srvparms[14][0], IA_IN | IA_OUT },
    { "ice_locations",          &srvparms[15][0], IA_SELECT | IA_RETRIEVE | IA_INSERT | IA_UPDATE | IA_DELETE },
    { "active_users",           &srvparms[16][0], IA_SELECT | IA_DELETE },
    { "ice_users",              &srvparms[17][0], IA_SELECT | IA_DELETE },
    { "ice_user_transactions",  &srvparms[18][0], IA_SELECT | IA_DELETE },
    { "ice_user_cursors",       &srvparms[19][0], IA_SELECT | IA_DELETE },
    { "ice_cache",              &srvparms[20][0], IA_SELECT | IA_DELETE },
    { "ice_connect_info",       &srvparms[21][0], IA_SELECT | IA_DELETE },
    { "profile",                &srvparms[22][0], IA_SELECT | IA_RETRIEVE | IA_INSERT | IA_UPDATE | IA_DELETE },
    { "profile_role",           &srvparms[23][0], IA_SELECT | IA_INSERT | IA_DELETE },
    { "profile_database",       &srvparms[24][0], IA_SELECT | IA_INSERT | IA_DELETE },
    { "TagToString",            &srvparms[25][0], 0 },
    { "dir",                    &srvparms[26][0], 0 },
    { "getVariables",           &srvparms[27][0], 0 },
    { "getSvrVariables",        &srvparms[28][0], IA_RETRIEVE },
    { "ice_var",                &srvparms[29][0], IA_SELECT | IA_RETRIEVE | IA_INSERT | IA_UPDATE | IA_DELETE },
    { NULL, NULL, 0 }
};

/*
** Name: ia_create
**
** Description:
**      Creates a structure for the required function including a complete
**      set of correctly named parameter entries.
**
**      The structure is also initialised with a set of function pointers
**      for accessing the data.
**
** Inputs:
**      funcname        Name of the function to create parameters for.
**
** Outputs:
**      iceapi          Pointer to the created ICEAPI structure.
**
** Returns:
**      OK                      Command completed successfully.
**      E_IA_INVALID_OUTPARM    Invalid input parameter.
**
** History:
**      21-Feb-2001 (fanra01)
**          Created.
*/
ICE_STATUS
ia_create( II_CHAR* funcname, PICEAPI* iceapi )
{
    ICE_STATUS  status = FAIL;
    II_INT4         i;
    II_INT4         j;
    II_INT4         k;
    II_CHAR**       p;
    II_INT4         size;
    ICE_C_PARAMS*   mptr;
    PICEAPI         piceapi;

    if (iceapi != NULL)
    {
        for(i=0; srvfunc[i].fname != NULL; i+=1)
        {
            if (STRCMP(srvfunc[i].fname, funcname) == 0)
            {
                MEMALLOC( sizeof(ICEAPI), PICEAPI, TRUE, status, piceapi );
                if (piceapi != NULL)
                {
                    for(j=0, p=srvfunc[i].srvparms; *p != NULL; j+=1, p+=1);
                    if (j > 0)
                    {
                        /*
                        ** ice c api requires a null terminated array so alloc
                        ** one more than we need
                        */
                        size = (j + 1) * sizeof(ICE_C_PARAMS);
                        MEMALLOC( size, ICE_C_PARAMS* , TRUE, status, mptr );
                        if (mptr != NULL)
                        {
                            piceapi->fnumber = i;
                            piceapi->fparms = mptr;
                            piceapi->fcount = j;
                            for(k=0, p=srvfunc[i].srvparms;
                                (k < j) && (*p != NULL); k+=1, p+=1, mptr+=1)
                            {
                                mptr->name = *p;
                            }

                            piceapi->getfuncname=   ia_getfuncname;
                            piceapi->getaction  =   ia_getaction;
                            piceapi->setaction  =   ia_setaction;
                            piceapi->testaction =   ia_testaction;
                            piceapi->gettype    =   ia_gettype;
                            piceapi->settype    =   ia_settype;
                            piceapi->getname    =   ia_getname;
                            piceapi->setname    =   ia_setname;
                            piceapi->getvalue   =   ia_getvalue;
                            piceapi->setvalue   =   ia_setvalue;
                            piceapi->getpos     =   ia_getpos;
                            piceapi->getparmname=   ia_getparmname;
                            piceapi->getparmpos =   ia_getparmpos;
                            piceapi->getcount   =   ia_getcount;
                            piceapi->getnumber  =   ia_getnumber;
                            piceapi->createparms=   ia_createparms;
                            piceapi->destroyparms=  ia_destroyparms;
                            *iceapi = piceapi;
                            status = OK;
                        }
                        break;
                    }
                }
            }
        }
    }
    else
    {
        status = E_IA_INVALID_OUTPARM;
    }
    return(status);
}

/*
** Name: ia_destroy
**
** Description:
**      Free the parameter list structures and the containing structure.
**
** Inputs:
**      iceapi          Pointer to the ICEAPI structure returned from a
**                      call to ia_create.
** Outputs:
**      None.
**
** Returns:
**      OK                      Command completed successfully.
**      E_IA_INVALID_INPPARM    Invalid input parameter.
**
** History:
**      21-Feb-2001 (fanra01)
**          Created.
*/
ICE_STATUS
ia_destroy( PICEAPI iceapi )
{
    ICE_STATUS status = OK;
    ICE_C_PARAMS*  fparms;
    II_INT4 i;

    if (iceapi != NULL)
    {
        fparms = iceapi->fparms;
        if (fparms != NULL)
        {
            for(i=0; i < iceapi->fcount; i+=1)
            {
                if (fparms[i].value != NULL)
                {
                    MEMFREE( fparms[i].value );
                }
            }
            MEMFREE( iceapi->fparms );
        }
        MEMFREE( iceapi );
    }
    else
    {
        status = E_IA_INVALID_INPPARM;
    }
    return(status);
}

/*
** Name: ia_retfuncname
**
** Description:
**      Return the functions name for a given function number without the
**      function context parameter.
**
** Inputs:
**      fnumber         Function number.
**
** Outputs:
**      func            Pointer to the returned name.
**
** Returns:
**      OK                      Command completed successfully.
**      E_IA_INVALID_OUTPARM    Invalid output parameter.
**
** History:
**      21-Feb-2001 (fanra01)
**          Created.
*/
ICE_STATUS
ia_retfuncname( II_INT4 fnumber, II_CHAR** func )
{
    ICE_STATUS status = OK;
    if (func != NULL)
    {
        *func = srvfunc[fnumber].fname;
    }
    else
    {
        status = E_IA_INVALID_OUTPARM;
    }
    return(status);
}

/*
** Name: ia_getfuncname
**
** Description:
**      Return the functions name for a given function number.
**
** Inputs:
**      iceapi          Pointer to the ICEAPI structure returned from a
**                      call to ia_create.
**      fnumber         Function number.
**
** Outputs:
**      func            Pointer to the returned name.
**
** Returns:
**      OK                      Command completed successfully.
**      E_IA_INVALID_INPPARM    Invalid input parameter.
**      E_IA_INVALID_OUTPARM    Invalid output parameter.
**
** History:
**      21-Feb-2001 (fanra01)
**          Created.
*/
ICE_STATUS
ia_getfuncname( PICEAPI iceapi, II_INT4 fnumber, II_CHAR** func )
{
    ICE_STATUS status = OK;
    if (iceapi != NULL)
    {
        if (func != NULL)
        {
            *func = srvfunc[iceapi->fnumber].fname;
        }
        else
        {
            status = E_IA_INVALID_OUTPARM;
        }
    }
    else
    {
        status = E_IA_INVALID_INPPARM;
    }
    return(status);
}

/*
** Name: ia_getaction
**
** Description:
**      Return the value of the action parameter if it  exists in the
**      parameter list.
**
** Inputs:
**      iceapi          Pointer to the ICEAPI structure returned from a
**                      call to ia_create.
** Outputs:
**      action          Pointer to the returned action string.
**
** Returns:
**      OK                      Command completed successfully.
**      E_IA_INVALID_INPPARM    Invalid input parameter.
**      E_IA_INVALID_OUTPARM    Invalid output parameter.
**      E_IA_UNKNOWN_FIELD      Action field is unavailable for the function.
**      E_IA_VALUE_NULL         The action field has no value.
**
** History:
**      21-Feb-2001 (fanra01)
**          Created.
*/
ICE_STATUS
ia_getaction( PICEAPI iceapi, II_CHAR** action )
{
    ICE_STATUS status = OK;
    ICE_C_PARAMS*  fparms;

    if( iceapi != NULL)
    {
        fparms = iceapi->fparms;
        if (action != NULL)
        {
            if (STRCMP( fparms[0].name, "action" ) == 0)
            {
                if ((*action = fparms[0].value) == NULL)
                {
                    status = E_IA_VALUE_NULL;
                }
            }
            else
            {
                status = E_IA_UNKNOWN_FIELD;
            }
        }
        else
        {
            status = E_IA_INVALID_OUTPARM;
        }
    }
    else
    {
        status = E_IA_INVALID_INPPARM;
    }
    return(status);
}

/*
** Name: ia_setaction
**
** Description:
**      Return the value of the action parameter if it
**      exists in the parameter list.
**
** Inputs:
**      iceapi          Pointer to the ICEAPI structure returned from a
**                      call to ia_create.
**      action          Point to the string to set as value of action.
**
** Outputs:
**      None.
**
** Returns:
**      OK                      Command completed successfully.
**      E_IA_INVALID_INPPARM    Invalid input parameter.
**      E_IA_INVALID_OUTPARM    Invalid output parameter.
**      E_IA_UNKNOWN_FIELD      Action field is unavailable for the function.
**
** History:
**      21-Feb-2001 (fanra01)
**          Created.
*/
ICE_STATUS
ia_setaction( PICEAPI iceapi, II_CHAR* action )
{
    ICE_STATUS status = OK;
    ICE_C_PARAMS*  fparms;

    if( iceapi != NULL)
    {
        fparms = iceapi->fparms;
        if (action != NULL)
        {
            if (STRCMP( fparms[0].name, "action" ) == 0)
            {
                fparms[0].value = STRDUP( action );
                fparms[0].type  = ICE_IN;
            }
            else
            {
                status = E_IA_UNKNOWN_FIELD;
            }
        }
        else
        {
            status = E_IA_INVALID_OUTPARM;
        }
    }
    else
    {
        status = E_IA_INVALID_INPPARM;
    }
    return(status);
}

/*
** Name: ia_testaction
**
** Description:
**      Compares a set of action flags for a function.
**
** Inputs:
**      params          Pointer to the list to destroy.
**
** Outputs:
**      None.
**
** Returns:
**      OK                      Command completed successfully.
**      E_IA_INVALID_INPPARM    Invalid input parameter.
**      E_IA_INVALID_OUTPARM    Invalid output parameter.
**
** History:
**      01-May-2001 (fanra01)
**          Created.
*/
ICE_STATUS
ia_testaction( II_INT4 fnumber, II_INT4 actions, II_BOOL* compared )
{
    ICE_STATUS status = OK;

    if ((fnumber >= ICE_DBUSER) && (fnumber <= ICE_VAR ))
    {
        if (compared != NULL)
        {
            *compared = (srvfunc[fnumber].actions & actions) ? TRUE : FALSE;
        }
        else
        {
            status = E_IA_INVALID_OUTPARM;
        }
    }
    else
    {
        status = E_IA_INVALID_INPPARM;
    }
    return(status);
}

/*
** Name: ia_gettype
**
** Description:
**      Return the ICE parameter type.
**
** Inputs:
**      iceapi          Pointer to the ICEAPI structure returned from a
**                      call to ia_create.
**      pos             Index of parameter in parameter list.
**
** Outputs:
**      type            Pointer to returned type.
**
** Returns:
**      OK                      Command completed successfully.
**      E_IA_INVALID_INPPARM    Invalid input parameter.
**      E_IA_INVALID_OUTPARM    Invalid output parameter.
**
** History:
**      21-Feb-2001 (fanra01)
**          Created.
*/
ICE_STATUS
ia_gettype( PICEAPI iceapi, II_INT4 pos, II_INT4* type )
{
    ICE_STATUS status = OK;
    ICE_C_PARAMS*  fparms;

    if(( iceapi != NULL) && (pos < iceapi->fcount))
    {
        fparms = iceapi->fparms;
        if (type != NULL)
        {
            *type = fparms[pos].type;
        }
        else
        {
            status = E_IA_INVALID_OUTPARM;
        }
    }
    else
    {
        status = E_IA_INVALID_INPPARM;
    }
    return(status);
}

/*
** Name: ia_settype
**
** Description:
**      Set the ICE parameter type.
**
** Inputs:
**      iceapi          Pointer to the ICEAPI structure returned from a
**                      call to ia_create.
**      pos             Index of parameter in parameter list.
**      type            Value of type.
**
** Outputs:
**      None.
**
** Returns:
**      OK                      Command completed successfully.
**      E_IA_INVALID_INPPARM    Invalid input parameter.
**      E_IA_INVALID_OUTPARM    Invalid output parameter.
**
** History:
**      21-Feb-2001 (fanra01)
**          Created.
*/
ICE_STATUS
ia_settype( PICEAPI iceapi, II_INT4 pos, II_INT4 type )
{
    ICE_STATUS status = OK;
    ICE_C_PARAMS*  fparms;

    if(( iceapi != NULL) && (pos < iceapi->fcount))
    {
        fparms = iceapi->fparms;
        fparms[pos].type = type;
    }
    else
    {
        status = E_IA_INVALID_INPPARM;
    }
    return(status);
}

/*
** Name: ia_getname
**
** Description:
**      Return the name of the parameter.
**
** Inputs:
**      iceapi          Pointer to the ICEAPI structure returned from a
**                      call to ia_create.
**      pos             Index of parameter in parameter list.
**
** Outputs:
**      name            Pointer to the returned name string of parameter.
**
** Returns:
**      OK                      Command completed successfully.
**      E_IA_INVALID_INPPARM    Invalid input parameter.
**      E_IA_INVALID_OUTPARM    Invalid output parameter.
**
** History:
**      21-Feb-2001 (fanra01)
**          Created.
*/
ICE_STATUS
ia_getname( PICEAPI iceapi, II_INT4 pos, II_CHAR** name )
{
    ICE_STATUS status = OK;
    ICE_C_PARAMS*  fparms;

    if(( iceapi != NULL) && (pos < iceapi->fcount))
    {
        fparms = iceapi->fparms;
        if (name != NULL)
        {
            *name = fparms[pos].name;
        }
        else
        {
            status = E_IA_INVALID_OUTPARM;
        }
    }
    else
    {
        status = E_IA_INVALID_INPPARM;
    }
    return(status);
}

/*
** Name: ia_setname
**
** Description:
**       Set the name of the parameter.
**
** Inputs:
**      iceapi          Pointer to the ICEAPI structure returned from a
**                      call to ia_create.
**      pos             Index of parameter in parameter list.
**      name            Name of the parameter.
**
** Outputs:
**      None.
**
** Returns:
**      OK                      Command completed successfully.
**      E_IA_INVALID_INPPARM    Invalid input parameter.
**      E_IA_INVALID_OUTPARM    Invalid output parameter.
**
** History:
**      21-Feb-2001 (fanra01)
**          Created.
*/
ICE_STATUS
ia_setname( PICEAPI iceapi, II_INT4 pos, II_CHAR* name )
{
    ICE_STATUS status = OK;
    ICE_C_PARAMS*  fparms;

    if(( iceapi != NULL) && (pos < iceapi->fcount))
    {
        fparms = iceapi->fparms;
        if (name != NULL)
        {
            fparms[pos].name = name;
        }
        else
        {
            status = E_IA_INVALID_INPPARM;
        }
    }
    else
    {
        status = E_IA_INVALID_INPPARM;
    }
    return(status);
}

/*
** Name: ia_getvalue
**
** Description:
**      Return the value of the parameter.
**
** Inputs:
**      iceapi          Pointer to the ICEAPI structure returned from a
**                      call to ia_create.
**      pos             Index of parameter in parameter list.
**
** Outputs:
**      value           Pointer to the returned value of the parameter.
**
** Returns:
**      OK                      Command completed successfully.
**      E_IA_INVALID_INPPARM    Invalid input parameter.
**      E_IA_INVALID_OUTPARM    Invalid output parameter.
**
** History:
**      21-Feb-2001 (fanra01)
**          Created.
*/
ICE_STATUS
ia_getvalue( PICEAPI iceapi, II_INT4 pos, II_CHAR** value )
{
    ICE_STATUS status = OK;
    ICE_C_PARAMS*  fparms;

    if(( iceapi != NULL) && (pos < iceapi->fcount))
    {
        fparms = iceapi->fparms;
        if (value != NULL)
        {
            *value = fparms[pos].value;
        }
        else
        {
            status = E_IA_INVALID_OUTPARM;
        }
    }
    else
    {
        status = E_IA_INVALID_INPPARM;
    }
    return(status);
}

/*
** Name: ia_setvalue
**
** Description:
**      Set the value of the parameter.
**
** Inputs:
**      iceapi          Pointer to the ICEAPI structure returned from a
**                      call to ia_create.
**      pos             Index of parameter in parameter list.
**      value           Value to set the parameter.
**
** Outputs:
**      None.
**
** Returns:
**      OK                      Command completed successfully.
**      E_IA_INVALID_INPPARM    Invalid input parameter.
**      E_IA_INVALID_OUTPARM    Invalid output parameter.
**
** History:
**      21-Feb-2001 (fanra01)
**          Created.
*/
ICE_STATUS
ia_setvalue( PICEAPI iceapi, II_INT4 pos, II_CHAR* value )
{
    ICE_STATUS status = OK;
    ICE_C_PARAMS*  fparms;

    if(( iceapi != NULL) && (pos < iceapi->fcount))
    {
        fparms = iceapi->fparms;
        if (value != NULL)
        {
            if (fparms[pos].value != NULL)
            {
                MEMFREE( fparms[pos].value );
            }
            fparms[pos].value = STRDUP( value );
        }
        else
        {
            status = E_IA_INVALID_INPPARM;
        }
    }
    else
    {
        status = E_IA_INVALID_INPPARM;
    }
    return(status);
}

/*
** Name: ia_getpos
**
** Description:
**      Return the position number of the named parameter.
**
** Inputs:
**      iceapi          Pointer to the ICEAPI structure returned from a
**                      call to ia_create.
**      name            Name of the parameter requested.
**
** Outputs:
**      pos             Pointer to the returned index in parameter list.
**
** Returns:
**      OK                      Command completed successfully.
**      E_IA_INVALID_INPPARM    Invalid input parameter.
**      E_IA_INVALID_OUTPARM    Invalid output parameter.
**
** History:
**      21-Feb-2001 (fanra01)
**          Created.
*/
ICE_STATUS
ia_getpos( PICEAPI iceapi, II_CHAR* name, II_INT4* pos )
{
    ICE_STATUS  status = OK;
    ICE_C_PARAMS*  fparms;
    II_INT4 i;

    if((iceapi != NULL) && (name != NULL))
    {
        fparms = iceapi->fparms;
        if (pos != NULL)
        {
            for(i=0; i < iceapi->fcount; i+=1)
            {
                if (STRCMP( fparms[i].name, name ) == 0)
                {
                    *pos = i;
                }
            }
        }
        else
        {
            status = E_IA_INVALID_OUTPARM;
        }
    }
    else
    {
        status = E_IA_INVALID_INPPARM;
    }
    return(status);
}

/*
** Name: ia_getparmname
**
** Description:
**      Return a pointer to the parameter by name.
**
** Inputs:
**      iceapi          Pointer to the ICEAPI structure returned from a
**                      call to ia_create.
**      name            Name of the parameter requested.
**
** Outputs:
**      param           Pointer to the returned parameter entry.
**
** Returns:
**      OK                      Command completed successfully.
**      E_IA_INVALID_INPPARM    Invalid input parameter.
**      E_IA_INVALID_OUTPARM    Invalid output parameter.
**
** History:
**      21-Feb-2001 (fanra01)
**          Created.
*/
ICE_STATUS
ia_getparmname( PICEAPI iceapi, II_CHAR* name, ICE_C_PARAMS** parm )
{
    ICE_STATUS  status = OK;
    ICE_C_PARAMS*  fparms;
    II_INT4 i;

    if((iceapi != NULL) && (name != NULL))
    {
        fparms = iceapi->fparms;
        if (parm != NULL)
        {
            for(i=0; i < iceapi->fcount; i+=1)
            {
                if (STRCMP( fparms[i].name, name ) == 0)
                {
                    *parm = &fparms[i];
                }
            }
        }
        else
        {
            status = E_IA_INVALID_OUTPARM;
        }
    }
    else
    {
        status = E_IA_INVALID_INPPARM;
    }
    return(status);
}

/*
** Name: ia_getparmpos
**
** Description:
**      Return a pointer to the parameter by position.
**
** Inputs:
**      iceapi          Pointer to the ICEAPI structure returned from a
**                      call to ia_create.
**      pos             Index of parameter in parameter list.
**
** Outputs:
**      parm            Pointer to the returned parameter entry.
**
** Returns:
**      OK                      Command completed successfully.
**      E_IA_INVALID_INPPARM    Invalid input parameter.
**      E_IA_INVALID_OUTPARM    Invalid output parameter.
**
** History:
**      21-Feb-2001 (fanra01)
**          Created.
*/
ICE_STATUS
ia_getparmpos( PICEAPI iceapi, II_INT4 pos, ICE_C_PARAMS** parm )
{
    ICE_STATUS  status = OK;
    ICE_C_PARAMS*  fparms;

    if((iceapi != NULL) && (pos < iceapi->fcount))
    {
        fparms = iceapi->fparms;
        if (parm != NULL)
        {
            *parm = &fparms[pos];
        }
        else
        {
            status = E_IA_INVALID_OUTPARM;
        }
    }
    else
    {
        status = E_IA_INVALID_INPPARM;
    }
    return(status);
}

/*
** Name: ia_getcount
**
** Description:
**      Return the maximum number of parameters.
**
** Inputs:
**      iceapi          Pointer to the ICEAPI structure returned from a
**                      call to ia_create.
**
** Outputs:
**      fcount          Pointer to the returned number parameter entries.
**
** Returns:
**      OK                      Command completed successfully.
**      E_IA_INVALID_INPPARM    Invalid input parameter.
**      E_IA_INVALID_OUTPARM    Invalid output parameter.
**
** History:
**      21-Feb-2001 (fanra01)
**          Created.
*/
ICE_STATUS
ia_getcount ( PICEAPI iceapi, II_INT4* fcount )
{
    ICE_STATUS  status = OK;

    if(iceapi != NULL)
    {
        if (fcount != NULL)
        {
            *fcount = iceapi->fcount;
        }
        else
        {
            status = E_IA_INVALID_OUTPARM;
        }
    }
    else
    {
        status = E_IA_INVALID_INPPARM;
    }
    return(status);
}

/*
** Name: ia_getnumber
**
** Description:
**      Return the function number for the current parameters.
**
** Inputs:
**      iceapi          Pointer to the ICEAPI structure returned from a
**                      call to ia_create.
**
** Outputs:
**      fnumber         Pointer to the returned function number.
**
** Returns:
**      OK                      Command completed successfully.
**      E_IA_INVALID_INPPARM    Invalid input parameter.
**      E_IA_INVALID_OUTPARM    Invalid output parameter.
**
** History:
**      21-Feb-2001 (fanra01)
**          Created.
*/
ICE_STATUS
ia_getnumber ( PICEAPI iceapi, II_INT4* fnumber )
{
    ICE_STATUS  status = OK;

    if(iceapi != NULL)
    {
        if (fnumber != NULL)
        {
            *fnumber = iceapi->fnumber;
        }
        else
        {
            status = E_IA_INVALID_OUTPARM;
        }
    }
    else
    {
        status = E_IA_INVALID_INPPARM;
    }
    return(status);
}

/*
** Name: ia_createparms
**
** Description:
**      Create parameter list only.
**
** Inputs:
**      funcname        Name of the function for parameter list.
**
** Outputs:
**      params          Pointer to the created parameter list.
**
** Returns:
**      OK                      Command completed successfully.
**      E_IA_INVALID_OUTPARM    Invalid output parameter.
**
** History:
**      21-Feb-2001 (fanra01)
**          Created.
*/
ICE_STATUS
ia_createparms( II_CHAR* funcname, ICE_C_PARAMS** params )
{
    ICE_STATUS  status = FAIL;
    II_INT4         i;
    II_INT4         j;
    II_INT4         k;
    II_CHAR**       p;
    II_INT4         size;
    ICE_C_PARAMS*   mptr;

    if (params != NULL)
    {
        for(i=0; srvfunc[i].fname != NULL; i+=1)
        {
            if (STRCMP(srvfunc[i].fname, funcname) == 0)
            {
                for(j=0, p=srvfunc[i].srvparms; *p != NULL; j+=1, p+=1);
                if (j > 0)
                {
                    /*
                    ** ice c api requires a null terminated array so alloc
                    ** one more than we need
                    */
                    size = (j + 1) * sizeof(ICE_C_PARAMS);
                    MEMALLOC( size, ICE_C_PARAMS* , TRUE, status, mptr );
                    if (mptr != NULL)
                    {
                        *params = mptr;
                        for(k=0, p=srvfunc[i].srvparms;
                            (k < j) && (*p != NULL); k+=1, p+=1, mptr+=1)
                        {
                            mptr->name = *p;
                        }
                        status = OK;
                    }
                    break;
                }
            }
        }
    }
    else
    {
        status = E_IA_INVALID_OUTPARM;
    }
    return(status);
}

/*
** Name: ia_destroyparms
**
** Description:
**      Destroy parameter list.
**
** Inputs:
**      params          Pointer to the list to destroy.
**
** Outputs:
**      None.
**
** Returns:
**      OK                      Command completed successfully.
**      E_IA_INVALID_INPPARM    Invalid input parameter.
**
** History:
**      21-Feb-2001 (fanra01)
**          Created.
*/
ICE_STATUS
ia_destroyparms( ICE_C_PARAMS* params )
{
    ICE_STATUS status = OK;
    II_INT4 i;

    if (params != NULL)
    {
        for(i=0; params[i].name != NULL; i+=1)
        {
            if (params[i].value != NULL)
            {
                MEMFREE( params[i].value );
            }
        }
        MEMFREE( params );
    }
    else
    {
        status = E_IA_INVALID_INPPARM;
    }
    return(status);
}
