/*
** Copyright (c) 2008 Ingres Corporation
*/

#include <compat.h>

#include <cm.h>
#include <cv.h> 
#include <er.h>
#include <lo.h>
#include <me.h>
#include <nm.h> 
#include <qu.h>
#include <si.h>
#include <st.h> 
#include <erclf.h>
#include <sql.h>                    /* ODBC Core definitions */
#include <sqlstate.h>               /* ODBC driver sqlstate codes */
#include <erodbc.h>
#include <sqlca.h>
#include <idmsxsq.h>                /* control block typedefs */
#include <idmsutil.h>               /* utility stuff */
#include <idmseini.h>               /* ini file keys */
#include <odbccfg.h>
#include "config.h"

/*
** Name: config.c
**
** Description:
**      This file contains the external configuration routines for the unixODBC
**      configuration and installer utilities.
**
**      openConfig             Open ODBC config file and initialize interface.
**      closeConfig            Free ODBC configuration resources.
**      listConfig             List ODBC configuration entries.
**      setConfigEntry         Write an ODBC configuration entry.
**      delConfigEntry         Delete an ODBC configuration entry.
**
** History:
**      02-Jun-03 (loera01)
**          Created.
**      28-jan-04 (loera01)
**          Added port to VMS.
**      19-Apr-04 (loera01)
**          Removed include of sqltypes.h and sqlext.h.
**      20-Oct-04 (hweho01)
**          Avoid compiler error on AIX platform, put include
**          files of sql.h and sqlstate.h after erclf.h.
**      17-Feb-05 (Ralph.Loen@ca.com) SIR 113913
**          In getConfigEntry(), return an error if the driver or dsn
**          is not found.
**      13-Mar-2007 (Ralph Loen) SIR 117786
**          Added getTimeout() and setTimeout().
**      17-June-2008 (Ralph Loen) Bug 120506
**          In setTrace() and getTrace(), handle DRV_STR_ALT_TRACE_FILE
**          (TraceFile) in addition to DRV_STR_TRACE_FILE (Trace File).
**          
*/

GLOBALREF char *driverAttributes[]; 
GLOBALREF char *dsnAttributes[]; 
GLOBALDEF OCFG_STATIC ocfg_static ZERO_FILL;

/*
** Name: openConfig
**
** Description:
**      This function opens a driver or data source configuration file
**      and fills the ODBC configuration cache with any qualifying
**      information.
**
** Return value:
**      none.
**
** Re-entrancy:
**      This function does not update shared memory.
**
** Parameters:
**      inputHandle        Driver (environment) handle.
**      type               Driver or dsn type.
**      cfgPath            Alternate unixODBC configuration path.
**      outputHandle       Driver or dsn handle.
**      status             Error number if failure or OK.
**
** History:
**      03-Jun-03 (loera01)
**          Created.
**	17-Jun-2004 (schka24)
**	    Safer env variable handling.
*/

void openConfig( PTR inputHandle, i4 type, char *cfgPath, 
    PTR *outputHandle, STATUS *status )
{
    i4       id = OCFG_UNKNOWN;
    i4       strLen = 0;
    OCFG_CB  *drv_cb;
    OCFG_CB  *ocfg_cb;
    char     *odbcsysini,*odbcini;
    char     locbuf[MAX_LOC + 1];
    bool  hasDrvHndl=FALSE;
    bool  iniDefined=FALSE;

    *status = OK;
 
    if (inputHandle != NULL)
    {
        drv_cb = (OCFG_CB *)inputHandle;
	if (drv_cb->hdr.ocfg_id != OCFG_DRIVER)
	{
	    *status = F_OD0027_IDS_ERR_ARGUMENT;
	    return;
	}
	else
	    hasDrvHndl = TRUE;
    }
    switch ( type )
    {
        case OCFG_ALT_PATH :

            if (!cfgPath || *cfgPath == ' ' || *cfgPath == '\0') 
            {
               *status = F_OD0027_IDS_ERR_ARGUMENT;
               return;           /*    Bad path argument */
            }
            else
            {
                STcopy(cfgPath, locbuf);
            }
            break;

        case OCFG_SYS_PATH :

            NMgtAt("ODBCSYSINI",&odbcsysini);
            if (odbcsysini  && *odbcsysini)
                STlcopy(odbcsysini, locbuf, sizeof(locbuf)-1);
            else
# ifdef VMS
                NMgtAt(SYS_PATH, &odbcsysini);
                if (odbcsysini && *odbcsysini)
                   STlcopy(odbcsysini, locbuf, sizeof(locbuf)-1);
# else
                STcopy( SYS_PATH, locbuf );
# endif  /* VMS */
            break;

        case OCFG_USR_PATH :

	    if (!hasDrvHndl)
            {
                *status = F_OD0027_IDS_ERR_ARGUMENT;
                return;           /*    Bad path argument */
            }            
            NMgtAt("ODBCINI",&odbcini);
            if (odbcini  && *odbcini)
            {
                STlcopy(odbcini, locbuf, sizeof(locbuf)-1);
	        iniDefined=TRUE;
            }
# ifdef VMS
            else
	    {
                NMgtAt("SYS$LOGIN",&odbcini);
                if (odbcini  && *odbcini)
	        {
                    STlcopy(odbcini, locbuf, sizeof(locbuf)-10-1);
	            STcat(locbuf,ODBC_INI);
		    iniDefined = TRUE;
		}
		else
		{
		    *status = F_OD0027_IDS_ERR_ARGUMENT;
		    return;
		}
	    }
# else
            else
	    {
                NMgtAt("HOME",&odbcini);
                if (odbcini  && *odbcini)
	        {
                    STlcopy(odbcini, locbuf, sizeof(locbuf)-10-1);
	            STcat(locbuf,"/.odbc.ini");
		    iniDefined = TRUE;
		}
		else
		{
		    *status = F_OD0027_IDS_ERR_ARGUMENT;
		    return;
		}
	    }
# endif /* VMS */
	    break;

        default:

           *status = F_OD0027_IDS_ERR_ARGUMENT;
           return;           /*    Bad path argument */
    }

    if (hasDrvHndl)
    {
        ocfg_cb = &ocfg_static.dsn;
        ocfg_cb->hdr.ocfg_id = OCFG_DSN;
    }
    else
    {
        ocfg_cb = &ocfg_static.drv;
        ocfg_cb->hdr.ocfg_id = OCFG_DRIVER;
    }                
    *outputHandle = (PTR)ocfg_cb;

    STcopy(locbuf, ocfg_cb->path);

    ocfg_cb->ocfg_info_count = 0;
    ocfg_cb->ocfg_sect_count = 0;
    QUinit(&ocfg_cb->sect_q);
    QUinit(&ocfg_cb->info_q);
    if (!iniDefined)
    {
# ifndef VMS
        strLen = STlength(ocfg_cb->path);
        if (CMcmpcase(&ocfg_cb->path[strLen -1],"/")) 
        {
            ocfg_cb->path[strLen] = '/';
            ocfg_cb->path[strLen+1] = '\0';
        }
# endif /* VMS */
        if (hasDrvHndl)
            STcat(ocfg_cb->path,ODBC_INI);
	else
            STcat(ocfg_cb->path,ODBCINST_INI);
    }
    if ((*status = readConfigData(ocfg_cb)) != OK)
        return;

    if ((*status = formatConfigInfo(ocfg_cb)) != OK)
        return;

    return;
}

/*
** Name: closeConfig
**
** Description:
**      This function frees all resources in the ODBC configuration
**      cache.
**
** Return value:
**      none.
**
** Re-entrancy:
**      This function does not update shared memory.
**
** Parameters:
**      handle             Driver or DSN handle.
**      status             Error number if failure or OK.
**
** History:
**      03-Jun-03 (loera01)
**          Created.
*/

void closeConfig( PTR handle, STATUS *status)
{
    OCFG_CB *ocfg_cb = (OCFG_CB *)handle;

    *status = OK;
    *status = emptyCache( ocfg_cb );
    return;
}

/*
** Name: listConfig
**
** Description:
**      This function returns a list of recognized driver or DSN names.
**
** Return value:
**      listCount          Number of entries.
**
** Re-entrancy:
**      This function does not update shared memory.
**
** Parameters:
**      handle             Driver or DSN handle.
**      count              Requested entry count.
**      list               Entry list.
**      status             Error number if failure or OK.
**
** History:
**      03-Jun-03 (loera01)
**          Created.
*/

i4 listConfig( PTR handle, i4 count, char **list, STATUS * status )
{
    OCFG_CB *ocfg_cb = (OCFG_CB *)handle;
    OCFG_INFO *info;
    QUEUE *q;
    i4 retcount = 0;
    i4 i = 0, j; 

    *status = OK;
    if ( count < 0 )
       *status = F_OD0027_IDS_ERR_ARGUMENT;

    if ( count == 0 )
        return ocfg_cb->ocfg_info_count;

    for ( q = ocfg_cb->info_q.q_prev; q != &ocfg_cb->info_q; q = q->q_prev )
    {
        /*
        ** If the user requested less than the items available, return.
        */
        if ( i > count )
            return ocfg_cb->ocfg_info_count;

        info = (OCFG_INFO *)q;
        STcopy(info->info_name, list[i]);
        i++;
    } /* end for ( q = ocfg_cb->info_q... */

    /*
    ** if requested more than count, fill remaining list with null terminator.
    */
    for ( j = i; j < count; j++)
        *list[j] = EOS;  

    return ocfg_cb->ocfg_info_count;
}

/*
** Name: getConfigEntry
**
** Description:
**      This function returns a list of attributes for a named ODBC
**      configuration entry.
**
** Return value:
**      attributeCount     Count of configuration entries.
**
** Re-entrancy:
**      This function does not update shared memory.
**
** Parameters:
**      handle             Driver or DSN handle.
**      name               Driver or DSN name.
**      count              Requested entry count.
**      list               Entry list.
**      status             Error number if failure or OK.
**
** History:
**      03-Jun-03 (loera01)
**          Created.
*/

i4 getConfigEntry( PTR handle, char *name, i4 count,
    ATTR_ID **attrList, STATUS *status )
{
    OCFG_CB *ocfg_cb = (OCFG_CB *)handle;
    OCFG_INFO *info;
    ATTR_FORMAT *attr_format;
    QUEUE *q, *aq;
    i4 i= 0, j = 0;
    bool found = FALSE;

    *status = OK;

    if ( count < 0 )
    {
       *status = F_OD0027_IDS_ERR_ARGUMENT;
       return 0;
    }
    for ( q = ocfg_cb->info_q.q_prev; q != &ocfg_cb->info_q; q = q->q_prev )
    {
        info = (OCFG_INFO *)q;
        /* 
        **  Found the driver name.  Return count if none specified.   Fill
        **  return array up to count otherwise.
        */
        if ( !STbcompare(name, 0, info->info_name, 0, TRUE ) )
        {
            if ( count == 0 )
                return info->format_count;

            found = TRUE;

            /* 
            ** If user requested less attributes than available list,
            ** return.
            */
            if ( info->format_count > count )
                return info->format_count;

            for (aq = info->format_q.q_prev; aq != &info->format_q; 
                aq = aq->q_prev)
            {
                /*
                ** Otherwise, fill the attribute list.
                */
                attr_format = (ATTR_FORMAT *)aq;
                attrList[i]->id = attr_format->format_id;
                STcopy(attr_format->format_value,attrList[i]->value);
                i++;
            } /* end for (aq = info->format_q.q_prev... */
            break;
        } /* end if !STbcompare... */
    } /* end for ( q = ocfg_cb->info_q... */

    if (!found )
    {
       *status = F_OD0027_IDS_ERR_ARGUMENT;
       return 0;
    }
    /*
    ** if requested more than count, fill remaining list with null values.
    */
    for ( j = info->format_count; j < count; j++)
    {
        attrList[j]->id = OCFG_UNKNOWN;
        *attrList[j]->value = EOS;
    }
    return info->format_count;
}

/*
** Name: getEntryAttribute
**
** Description:
**      This function returns a list of attributes for a named ODBC
**      configuration entry. The attributes are unformatted.
**
** Return value:
**      attributeCount     Count of configuration entries.
**
** Re-entrancy:
**      This function does not update shared memory.
**
** Parameters:
**      handle             Driver or DSN handle.
**      name               Driver or DSN name.
**      count              Requested entry count.
**      list               Entry list.
**      status             Error number if failure or OK.
**
** History:
**      08-Jul-03 (loera01)
**          Created.
*/

i4 getEntryAttribute ( PTR handle, char *name, i4 count,
    ATTR_NAME **attrList, STATUS *status )
{
    OCFG_CB *ocfg_cb = (OCFG_CB *)handle;
    OCFG_SECT *section;
    ATTR_DATA *data;
    QUEUE *q, *aq;
    i4 i = 0, j = 0;
    bool match = FALSE;

    *status = OK;

    if ( count < 0 )
    {
       *status = F_OD0027_IDS_ERR_ARGUMENT;
       return 0;
    }

    for ( q = ocfg_cb->sect_q.q_prev; q != &ocfg_cb->sect_q; q = q->q_prev )
    {
        section = (OCFG_SECT *)q;
        if ( !STbcompare(name, 0, section->sect_name, 0, TRUE ) )
	{
            match = TRUE;
            /* 
            **  Found the driver or dsn name.  Return count if none specified. 
	    **  Fill return array up to count otherwise.
            */
            if ( count == 0 )
                return section->data_count;

            /* 
            ** If user requested less attributes than available list,
            ** return.
            */
            if ( section->data_count > count )
            {
		*status = F_OD0027_IDS_ERR_ARGUMENT;
                return section->data_count;
            }		   

            for (aq = section->data_q.q_prev; aq != &section->data_q; 
	        aq = aq->q_prev)
            {
                data = (ATTR_DATA *)aq;
                /*
                ** Otherwise, fill the attribute list.
                */
                STcopy(data->data_name,attrList[i]->name);
                STcopy(data->data_value,attrList[i]->value);
		i++;
            } /* end for (aq = section->data_q.q_prev... */
            break;
        } /* end if !STbcompare... */
    } /* end for ( q = ocfg_cb->sect_q... */

    if (!match)
        return 0;

    /*
    ** if requested more than count, fill remaining list with null values.
    */
    for ( j = section->data_count; j < count; j++)
    {
        *attrList[j]->name = EOS;
        *attrList[j]->value = EOS;
    }
    return section->data_count;
}

/*
** Name: getTrace
**
** Description:
**      This function returns the ODBC tracing status.
**
** Return value:
**      attributeCount     TRUE if tracing is enabled; FALSE if not.
**
** Re-entrancy:
**      This function does not update shared memory.
**
** Parameters:
**      handle             Driver handle.
**      path               Tracing log path and file name.
**      status             Error number if failure or OK.
**
** History:
**      11-Jul-03 (loera01)
**          Created.
**      17-June-2008 (Ralph Loen) Bug 120506
**          Handle DRV_STR_ALT_TRACE_FILE (TraceFile) in addition to 
**          DRV_STR_TRACE_FILE (Trace File).
*/

bool getTrace( PTR handle, char *path, STATUS *status )
{
    OCFG_CB *ocfg_cb = (OCFG_CB *)handle;
    OCFG_SECT *section;
    ATTR_DATA *data;
    QUEUE *q, *aq;
    bool ret=FALSE;

    *path = EOS;
    *status = OK;

    for ( q = ocfg_cb->sect_q.q_prev; q != &ocfg_cb->sect_q; q = q->q_prev )
    {
        section = (OCFG_SECT *)q;
	/*
	** Look for [ODBC] section.
	*/
        if ( !STbcompare(DRV_STR_ODBC, 0, section->sect_name, 0, TRUE ) )
	{
            for (aq = section->data_q.q_prev; aq != &section->data_q; 
	        aq = aq->q_prev)
            {
                data = (ATTR_DATA *)aq;
		if (!STbcompare(DRV_STR_TRACE, 0, data->data_name, 0, TRUE ))
		{
		    if (!STbcompare(DRV_STR_Y, 0, data->data_value, 0, TRUE ))
		        ret = TRUE;
		    else
			ret = FALSE;
		}
		if (!STbcompare(DRV_STR_TRACE_FILE, 0, data->data_name, 
		    0, TRUE ))
		{
		    STcopy(data->data_value, path);
		}
		if (!STbcompare(DRV_STR_ALT_TRACE_FILE, 0, data->data_name, 
		    0, TRUE ))
		{
		    STcopy(data->data_value, path);
		}
            } /* end for (aq = section->data_q.q_prev... */
        } /* end if !STbcompare... */
    } /* end for ( q = ocfg_cb->sect_q... */

    return ret;
}

/*
** Name: getTimeOut
**
** Description:
**      This function returns the ODBC connection pooling time-out value.
**
** Return value:
**      attributeCount     TRUE if tracing is enabled; FALSE if not.
**
** Re-entrancy:
**      This function does not update shared memory.
**
** Parameters:
**      handle             Driver handle.
**      path               Tracing log path and file name.
**      status             Error number if failure or OK.
**
** History:
**      11-Jul-03 (loera01)
**          Created.
*/

i4 getTimeout( PTR handle )
{
    OCFG_CB *ocfg_cb = (OCFG_CB *)handle;
    OCFG_SECT *section;
    ATTR_DATA *data;
    QUEUE *q, *aq;
    i4 timeout;
    
    for ( q = ocfg_cb->sect_q.q_prev; q != &ocfg_cb->sect_q; q = q->q_prev )
    {
        section = (OCFG_SECT *)q;
	/*
	** Look for [ODBC] section.
	*/
        if ( !STbcompare(DRV_STR_ODBC, 0, section->sect_name, 0, TRUE ) )
	{
            for (aq = section->data_q.q_prev; aq != &section->data_q; 
	        aq = aq->q_prev)
            {
                data = (ATTR_DATA *)aq;
		if (!STbcompare(DRV_STR_POOL_TIMEOUT, 0, data->data_name, 0, 
                    TRUE ))
		{
		    if (CVal(data->data_value, &timeout ) != OK)
                    {
                        /*
                        ** Default to -1 if conversion from ASCII was 
                        ** unsuccessful.
                        */
                        timeout = -1;
                    }
		}
            } /* end for (aq = section->data_q.q_prev... */
        } /* end if !STbcompare... */
    } /* end for ( q = ocfg_cb->sect_q... */

    return timeout;
}

/*
** Name: setConfigEntry
**
** Description:
**      This function adds or modifies the driver or DSN attributes to the
**      configuration cache and corresponding configuration file.
**
** Return value:
**      None.
**
** Re-entrancy:
**      This function does not update shared memory.
**
** Parameters:
**      handle             Driver or DSN handle.
**      name               Driver or DSN name.
**      count              Entry count.
**      list               Entry list.
**      status             Error number if failure or OK.
**
** History:
**      03-Jun-03 (loera01)
**          Created.
*/

void setConfigEntry( PTR handle, char *name, i4 count,
    ATTR_ID **list, STATUS *status )
{
    OCFG_CB *ocfg_cb = (OCFG_CB *)handle;
    OCFG_SECT *section;
    OCFG_INFO *info;
    ATTR_DATA *data;
    ATTR_FORMAT *format;
    i4 i = 0, k;
    QUEUE *q, *aq;
    char entryName[OCFG_MAX_STRLEN];
    BOOL new_section = TRUE, new_data = TRUE, new_format = TRUE;
    i4 drvAttr = OCFG_UNKNOWN;
    bool isDriver = ocfg_cb->hdr.ocfg_id == OCFG_DRIVER ? TRUE : FALSE;

    STcopy(name,entryName);
    /*
    ** Refresh cache from ini file.
    */
    if ( (*status = emptyCache(ocfg_cb)) != OK )
        return;
    QUinit(&ocfg_cb->sect_q);
    QUinit(&ocfg_cb->info_q);

    if ((*status = readConfigData(ocfg_cb)) != OK)
        return;

    if ((*status = formatConfigInfo(ocfg_cb)) != OK)
        return;

    /*
    **  For drivers, make sure "driver=Installed" is in the [ODBC Drivers] 
    **  section.  Add [ODBC] and trace bits as well, if needed.
    */
    if (isDriver)
    {
        *status = addInstalledDriver ( ocfg_cb, entryName );
        if (driverManager == MGR_VALUE_UNIX_ODBC)
            *status = addConfigOptions ( ocfg_cb );

    /*
    ** First see if the entry exists, and replace with the new
    ** attribute from the provided list if it does exist.
    */
    for ( q = ocfg_cb->sect_q.q_prev; q != &ocfg_cb->sect_q; q = q->q_prev )
    {
        section = (OCFG_SECT *)q;
        if ( !STbcompare(entryName, 0, section->sect_name, 0, TRUE ) )
        {
            new_section = FALSE;
            for (k = 0; k < count; k++)
            {
                new_data = TRUE;
                for (aq = section->data_q.q_prev; aq != &section->data_q; 
                    aq = aq->q_prev)
                {
                    data = (ATTR_DATA *)aq;
                    if ( list[k]->id == getAttributeID(driverAttributes, 
                        data->data_name) )
                    {
                        new_data = FALSE;
                        MEfree(data->data_value);
                        data->data_value = STalloc(list[k]->value);
                        break;
                    } 
                }
                if ( new_data )
                    data = addData ( section, 
                        getAttributeValue(driverAttributes,
                        list[k]->id), list[k]->value );
            } /* End for (k = 0; k < count; k++) */
            break;
        } /* End for ( q = ocfg_cb->sect_q.q_prev... */
    }
    for ( q = ocfg_cb->info_q.q_prev; q != &ocfg_cb->info_q; q = q->q_prev )
    {
        info = (OCFG_INFO *)q;
        if ( !STbcompare(entryName, 0, info->info_name, 0, TRUE ) )
        {
            for (k = 0; k < count; k++)
            {
                new_format = TRUE;
                for (aq = info->format_q.q_prev; aq != &info->format_q; 
                    aq = aq->q_prev)
                {
                    format = (ATTR_FORMAT *)aq;
                    if ( list[k]->id == format->format_id )
                    {
                        new_format = FALSE;
                        MEfree(format->format_value);
                        format->format_value = STalloc(list[k]->value);
                        break;
                    } 
                }
                if ( new_format )
                    format = addFormat ( info, list[k]->id,
                        list[k]->value );
            }
            break;
        }
    } /* End for ( q = ocfg_cb->info_q.q_prev... */
    /*
    ** New entries must be linked into the database.
    */
    if ( new_section )
    {
        section = addSection( ocfg_cb, entryName );
        section->recognized = TRUE;
        for (k = 0; k < count; k++)
        {
            if ( list[k]->id < 0 || list[k]->id > DRV_ATTR_MAX )
            {
                /* Ignore bogus attributes.  */
                continue;
            }
            data = addData( section, getAttributeValue( driverAttributes,
                list[k]->id), list[k]->value );
        }
        /*
        ** Put into the info queue.
        */
        info = addInfo ( ocfg_cb, entryName );
        for (k = 0; k < count; k++)
        {
            if ( list[k]->id < 0 || list[k]->id > DRV_ATTR_MAX )
            {
                /* Ignore bogus attributes.  */
                continue;
            }
            format = addFormat ( info, list[k]->id, list[k]->value);
        }
    }
    } /* end if (isDriver) */
    else
    {
       *status = addDataSources ( ocfg_cb, entryName );
       if (driverManager == MGR_VALUE_CAI_PT)
            *status = addConfigOptions ( ocfg_cb );
    /*
    ** First see if the entry exists, and replace with the new
    ** attribute from the provided list if it does exist.
    */
    for ( q = ocfg_cb->sect_q.q_prev; q != &ocfg_cb->sect_q; q = q->q_prev )
    {
        section = (OCFG_SECT *)q;
        if ( !STbcompare(entryName, 0, section->sect_name, 0, TRUE ) )
        {
            new_section = FALSE;
            for (k = 0; k < count; k++)
            {
                new_data = TRUE;
                for (aq = section->data_q.q_prev; aq != &section->data_q; 
                    aq = aq->q_prev)
                {
                    data = (ATTR_DATA *)aq;
                    if ( list[k]->id == getAttributeID(dsnAttributes, 
                        data->data_name) )
                    {
                        new_data = FALSE;
                        MEfree(data->data_value);
                        data->data_value = STalloc(list[k]->value);
                        break;
                    } 
                }
                if ( new_data )
                    data = addData ( section, 
                        getAttributeValue(dsnAttributes,
                        list[k]->id), list[k]->value );
            } /* End for (k = 0; k < count; k++) */
            break;
        } /* End for ( q = ocfg_cb->sect_q.q_prev... */
    }
    for ( q = ocfg_cb->info_q.q_prev; q != &ocfg_cb->info_q; q = q->q_prev )
    {
        info = (OCFG_INFO *)q;
        if ( !STbcompare(entryName, 0, info->info_name, 0, TRUE ) )
        {
            for (k = 0; k < count; k++)
            {
                new_format = TRUE;
                for (aq = info->format_q.q_prev; aq != &info->format_q; 
                    aq = aq->q_prev)
                {
                    format = (ATTR_FORMAT *)aq;
                    if ( list[k]->id == format->format_id )
                    {
                        new_format = FALSE;
                        MEfree(format->format_value);
                        format->format_value = STalloc(list[k]->value);
                        break;
                    } 
                }
                if ( new_format )
                    format = addFormat ( info, list[k]->id,
                        list[k]->value );
            }
            break;
        }
    } /* End for ( q = ocfg_cb->info_q.q_prev... */
    /*
    ** New entries must be linked into the database.
    */
    if ( new_section )
    {
        section = addSection( ocfg_cb, entryName );
        section->recognized = TRUE;
        for (k = 0; k < count; k++)
        {
            if ( list[k]->id < 0 || list[k]->id > DSN_ATTR_MAX )
            {
                /* Ignore bogus attributes.  */
                continue;
            }
            data = addData( section, getAttributeValue( dsnAttributes,
                list[k]->id), list[k]->value );
        }
        /*
        ** Put into the info queue.
        */
        info = addInfo ( ocfg_cb, entryName );
        for (k = 0; k < count; k++)
        {
            if ( list[k]->id < 0 || list[k]->id > DSN_ATTR_MAX )
            {
                /* Ignore bogus attributes.  */
                continue;
            }
            format = addFormat ( info, list[k]->id, list[k]->value);
        }
    }
    } /* if DSN */

    *status = rewriteINI ( ocfg_cb );
    return; 
}

/*
** Name: setTrace
**
** Description:
**      This function sets ODBC tracing on or off.
**
** Return value:
**      None.
**
** Re-entrancy:
**      This function does not update shared memory.
**
** Parameters:
**      handle             Driver or DSN handle.
**      name               Driver or DSN name.
**      count              Entry count.
**      list               Entry list.
**      status             Error number if failure or OK.
**
** History:
**      03-Jun-03 (loera01)
**          Created.
**      17-June-2008 (Ralph Loen) Bug 120506
**          Handle DRV_STR_ALT_TRACE_FILE (TraceFile) in addition to 
**          DRV_STR_TRACE_FILE (Trace File).
*/
void setTrace( PTR handle, bool trace_on, char *path, STATUS
*status )
{
    OCFG_CB *ocfg_cb = (OCFG_CB *)handle;
    OCFG_SECT *section;
    ATTR_DATA *data;
    i4 i = 0, k;
    QUEUE *q, *aq;
    char entryName[OCFG_MAX_STRLEN];
    i4 drvAttr = OCFG_UNKNOWN;

    /*
    ** Refresh cache from ini file.
    */
    if ( (*status = emptyCache(ocfg_cb)) != OK )
        return;
    QUinit(&ocfg_cb->sect_q);
    QUinit(&ocfg_cb->info_q);

    if ((*status = readConfigData(ocfg_cb)) != OK)
        return;

    if ((*status = formatConfigInfo(ocfg_cb)) != OK)
        return;

    /*
    **  Make sure [ODBC] section and trace info exists.
    */
    *status = addConfigOptions ( ocfg_cb );

    /*
    ** First see if the entry exists, and replace with the new
    ** attribute from the provided list if it does exist.
    */
    for ( q = ocfg_cb->sect_q.q_prev; q != &ocfg_cb->sect_q; q = q->q_prev )
    {
        section = (OCFG_SECT *)q;
        if ( !STbcompare(DRV_STR_ODBC, 0, section->sect_name, 0, TRUE ) )
        {
            for (aq = section->data_q.q_prev; aq != &section->data_q; 
                aq = aq->q_prev)
            {
                data = (ATTR_DATA *)aq;
                if ( !STbcompare(DRV_STR_TRACE, 0, data->data_name,
		    0, TRUE ) )
                {
		    if (trace_on)
			STcopy(DRV_STR_Y, data->data_value);
                    else
			STcopy(DRV_STR_N, data->data_value);
                } 
                if ( !STbcompare(DRV_STR_ALT_TRACE_FILE, 0, data->data_name,
		    0, TRUE ) )
		    STcopy(path, data->data_value);
                if ( !STbcompare(DRV_STR_TRACE_FILE, 0, data->data_name,
		    0, TRUE ) )
		    STcopy(path, data->data_value);
            }
            break;
        } /* End for ( q = ocfg_cb->sect_q.q_prev... */
    }
    *status = rewriteINI ( ocfg_cb );
    return; 
}

/*
** Name: setTimeout
**
** Description:
**      This function sets the connection pooling time-out value.
**
** Return value:
**      None.
**
** Re-entrancy:
**      This function does not update shared memory.
**
** Parameters:
**      handle             Driver or DSN handle.
**      timeout            timeout value - range 1 to MAXI4 or -1.
**
** History:
**      03-Jun-03 (loera01)
**          Created.
*/
void setTimeout( PTR handle, i4 timeout, STATUS *status )
{
    OCFG_CB *ocfg_cb = (OCFG_CB *)handle;
    OCFG_SECT *section;
    ATTR_DATA *data;
    i4 i = 0, k;
    QUEUE *q, *aq;
    char entryName[OCFG_MAX_STRLEN];
    i4 drvAttr = OCFG_UNKNOWN;

    /*
    ** Refresh cache from ini file.
    */
    if ( (*status = emptyCache(ocfg_cb)) != OK )
        return;
    QUinit(&ocfg_cb->sect_q);
    QUinit(&ocfg_cb->info_q);

    if ((*status = readConfigData(ocfg_cb)) != OK)
        return;

    if ((*status = formatConfigInfo(ocfg_cb)) != OK)
        return;

    /*
    **  Make sure [ODBC] section and trace info exists.
    */
    *status = addConfigOptions ( ocfg_cb );

    /*
    ** First see if the entry exists, and replace with the new
    ** attribute from the provided list if it does exist.
    */
    for ( q = ocfg_cb->sect_q.q_prev; q != &ocfg_cb->sect_q; q = q->q_prev )
    {
        section = (OCFG_SECT *)q;
        if ( !STbcompare(DRV_STR_ODBC, 0, section->sect_name, 0, TRUE ) )
        {
            for (aq = section->data_q.q_prev; aq != &section->data_q; 
                aq = aq->q_prev)
            {
                data = (ATTR_DATA *)aq;
                if ( !STbcompare(DRV_STR_POOL_TIMEOUT, 0, data->data_name,
		    0, TRUE ) )
                {
                    CVla((i4)timeout, data->data_value);
                    TRdisplay("writing %s from %d\n",data->data_value, timeout);
                } 
            }
            break;
        } /* End for ( q = ocfg_cb->sect_q.q_prev... */
    }
    *status = rewriteINI ( ocfg_cb );
    return; 
}

/*
** Name: delConfigEntry
**
** Description:
**      This function removes the driver or DSN name and attributes from the
**      configuration cache and corresponding configuration file.
**
** Return value:
**      None.
**
** Re-entrancy:
**      This function does not update shared memory.
**
** Parameters:
**      handle             Driver or DSN handle.
**      name               Driver or DSN name.
**      status             Error number if failure or OK.
**
** History:
**      03-Jun-03 (loera01)
**          Created.
*/

void delConfigEntry( PTR handle, char *name, STATUS *status )
{
    OCFG_CB *ocfg_cb = (OCFG_CB *)handle;
    OCFG_SECT *section;
    OCFG_INFO *info;
    ATTR_DATA *attr_data;
    ATTR_FORMAT *attr_format;
    i4 i, j, k = 0;
    QUEUE *q, *aq;
    BOOL update_required = FALSE;
    bool isDriver = ocfg_cb->hdr.ocfg_id == OCFG_DRIVER ? TRUE : FALSE;

    *status = OK;
    /*
    ** Refresh cache from ini file.
    */
    if ( (*status = emptyCache(ocfg_cb)) != OK )
        return;

    QUinit(&ocfg_cb->sect_q);
    QUinit(&ocfg_cb->info_q);

    if ((*status = readConfigData(ocfg_cb)) != OK)
        return;

    if ((*status = formatConfigInfo(ocfg_cb)) != OK)
        return;

    /*
    ** Remove definition from the info queue.
    */
    q = ((QUEUE *)ocfg_cb->info_q.q_prev);
    for (i = 0; i < ocfg_cb->ocfg_info_count; i++)
    {
        info = (OCFG_INFO *)q;
        q = q->q_prev;
        if ( !STbcompare( name, 0, info->info_name, 0, TRUE ) )
        {
            update_required = TRUE;
            i++;
            MEfree((PTR)info->info_name);
            for (j = 0; j < info->format_count; j++)
            {
                attr_format = (ATTR_FORMAT *)info->format_q.q_prev;
                MEfree((PTR)attr_format->format_value);
                QUremove((QUEUE *)attr_format);
                MEfree((PTR)attr_format);
            } 
            QUremove((QUEUE *)info);
            MEfree((PTR)info);
            ocfg_cb->ocfg_info_count--;
            break;
        }
    }

    /*
    ** Remove information from the data queue.
    */
    q = ((QUEUE *)ocfg_cb->sect_q.q_prev);
    for (i = 0; i < ocfg_cb->ocfg_sect_count; i++)
    {
        section = (OCFG_SECT *)q;
        q = q->q_prev;
        if ( !STbcompare( name, 0, section->sect_name, 0, TRUE ) )
        {
            update_required = TRUE;
            i++;
            MEfree(section->sect_name);
            for (j = 0; j < section->data_count; j++)
            {
                attr_data = (ATTR_DATA *)section->data_q.q_prev;
                MEfree((PTR)attr_data->data_value);
                QUremove((QUEUE *)attr_data);
                MEfree((PTR)attr_data);
            }
            QUremove((QUEUE *)section);
            MEfree((PTR)section);
            ocfg_cb->ocfg_sect_count--;
            break;
        }
    }
    /*
    ** For drivers, remove "driverName = Installed" bit from ODBC_DRIVERS
    ** section.
    */
    if (isDriver)
    {
        for (q = ocfg_cb->sect_q.q_prev; q != &ocfg_cb->sect_q; q = q->q_prev)
        {
            section = (OCFG_SECT *)q;
            if ( getAttributeID(driverAttributes, section->sect_name)
                == DRV_ATTR_ODBC_DRIVERS)
            {
                for (aq = section->data_q.q_prev; aq != &section->data_q; 
                    aq = aq->q_prev)
                {
                    attr_data = (ATTR_DATA *)aq;
                    if ( !STbcompare(name, 0, attr_data->data_name, 0, TRUE ) )
                    {
                        update_required = TRUE;
                        QUremove((QUEUE *)attr_data); 
                        MEfree((PTR)attr_data);
                        section->data_count--;
                        break;
                    }
                }
            }
        }
    }
	else
    /*
    ** For DSN's, remove "dsn = [Driver]" bit from ODBC_DATA_SOURCES
    ** section.
    */
    {
        for (q = ocfg_cb->sect_q.q_prev; q != &ocfg_cb->sect_q; q = q->q_prev)
        {
            section = (OCFG_SECT *)q;
            if ( getAttributeID(dsnAttributes, section->sect_name)
                == DSN_ATTR_ODBC_DATA_SOURCES)
            {
                for (aq = section->data_q.q_prev; aq != &section->data_q; 
                    aq = aq->q_prev)
                {
                    attr_data = (ATTR_DATA *)aq;
                    if ( !STbcompare(name, 0, attr_data->data_name, 0, TRUE ) )
                    {
                        update_required = TRUE;
                        QUremove((QUEUE *)attr_data); 
                        MEfree((PTR)attr_data);
                        section->data_count--;
                        break;
                    }
                }
            }
        }
    }
    if ( !update_required )
        return;

    *status = rewriteINI ( ocfg_cb );
    return;
}
