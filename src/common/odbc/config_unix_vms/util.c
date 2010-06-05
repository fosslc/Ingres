/*
** Copyright (c) 1993, 2008 Ingres Corporation
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
** Name: util.c
**
** Description:
**      This file contains internal utility routines for the unixODBC
**      configuration API.
**
**      getAttributeID           Get configuration attribute ID.
**      getAttributeValue        Get configuration attribute string.
**      readConfigData           Read the configuration file.
**      formatConfigInfo         Convert configuration data to ATTR_ID format.
**      driverIsInstalled        Is driver listed as installed?
**      isDriverDef              Is driver recognized by Ingres?
**      addInstalledDriver       Add "Driver=installed" definition.
**      addDataSources           Add "Data Source=driver" definition.
**      addConfigOptions         Add configuration options to driver cache.
**      addSection               Add a section to configuration cache.
**      addInfo                  Add attribute section to config cache.
**      addData                  Add ATTRIB_DATA to configuration cache.
**      addFormat                Add ATTRIB_ID to configuration cache. 
**      rewriteINI               Rewrite the configuration file.
**      emptyCache               Unlink and dereference the config cache.
**
** History:
**      02-Jun-03 (loera01)
**          Created.
**      28-jan-04 (loera01)
**          Added port to VMS.
**      19-Apr-04 (loera01)
**          Removed include of sqltypes.h and sqlext.h.
**	17-jun-2004 (somsa01)
**	    Cleaned up code for Open Source.
**	17-Jun-2004 (schka24)
**	    Safer env variable handling.
**      04-Oct-04 (hweho01)
**          Avoid compiler error on AIX platform, put include
**          files of sql.h and sqlstate.h after erclf.h.
**      04-Oct-04 (hweho01)
**          Avoid compiler error on AIX platform, put include
**          files of sql.h and sqlstate.h after erclf.h.
**      17-Feb-05 (Ralph.Loen@ca.com)
**          In getMgrInfo(), invoke SIclose() after opening the file.
**      24-Feb-05 (Ralph.Loen@ca.com) Bug 113943
**          Do not return status from SIclose(), as various platforms
**          treat the status differently.
**      29-apr-2005 (loera01)
**          Ignore terminating brackets when reading the DriverPath in
**          the configuration manager file.
**      18-jul-2005 (loera01)  Bug 114869
**          Added DSN_STR_DBMS_PWD and DSN_STR_GROUP to DSN attributes table.
**      18-Jan-2006 (loera01) SIR 115615
**          In formatConfigInfo() and isDriverDef(), the vendor attributes
**          DRV_ATTR_COMP_ASSOC and DRV_ATTR_INGRES_CORP are valid.
**      20-Jan-2006 (loera01) SIR 115615
**          Add DRV_STR_COMP_ASSOC to the list of valid driver attributes for
**          backward compatibililty.
**      16-Mar-2006 (Ralph Loen) Bug 115833
**          Added DSN_STR_MULTIBYTE_FILL_CHAR to the DSN attributes table.
**      13-Mar-2007 (Ralph Loen) SIR 117786
**         Replaced addTrace() with addConfigOptions(), which adds both
**         tracing and pooling timeout.
**   17-June-2008 (Ralph Loen) Bug 120506
**          In addTrace(), write DRV_STR_ALT_TRACE_FILE (TraceFile) in addition
**          to DRV_STR_TRACE_FILE (Trace File).
**   17-Nov-2008 (Ralph Loen) SIR 121228
**          Add DSN_ATTR_INGRESDATE to default dsnAttributes table.
**   28-Apr-2009 (Ralph Loen) SIR 122007
**          Add support for "StringTruncation" ODBC configuration attribute.
**          If set to Y, string truncation errors are reported.  If set to
**          N (default), strings are truncated silently.
**   27-Apr-2010 (Ralph Loen) SIR 123641
**          Remove reliance on ocfginfo.dat getDefaultInfo().  The alternate 
**          driver name is now "Ingres XX", where XX is the installation code. 
**          Replace numeric array references for the default attributes array
**          with pre-defined constants, i.e., defAttr[0] is not
**          defAttr[INFO_ATTR_DRIVER_FILE_NAME].
**   28-Apr-2010 (Ralph Loen) 
**          In getDefaultInfo(), mispelled libiiodbcdriver.1.SLSFX and 
**          libiiodbcdriverro.1.SLSFX.
**   03-May-2010 (Ralph Loen) Bug 123676
**          In readConfigData(), return error F_OD0172_IDS_ERR_CORRUPT_SECT
**          if a section header is corrupted.
**  
*/

GLOBALDEF i2 driverManager=MGR_VALUE_UNIX_ODBC;

GLOBALDEF char *driverAttributes[] =
{
    DRV_STR_ODBC_DRIVERS,
    DRV_STR_INGRES,
    DRV_STR_DONT_DLCLOSE,
    DRV_STR_DRIVER,
    DRV_STR_DRIVER_ODBC_VER,
    DRV_STR_DRIVER_INTERNAL_VER,
    DRV_STR_DRIVER_READONLY,
    DRV_STR_DRIVER_TYPE,
    DRV_STR_VENDOR,
    DRV_STR_ODBC,
    DRV_STR_TRACE,
    DRV_STR_ALT_TRACE_FILE,
    DRV_STR_TRACE_FILE,
    "\0"
};

GLOBALDEF char *driverValues[] =
{
    DRV_STR_INSTALLED,   
    DRV_STR_ZERO,
    DRV_STR_ONE,
    DRV_STR_N,
    DRV_STR_Y,
    DRV_STR_NO,
    DRV_STR_YES,
    DRV_STR_INGRES,
    DRV_STR_COMP_ASSOC,
    DRV_STR_INGRES_CORP,
    "\0"
};

GLOBALDEF char *dsnAttributes[] =
{
    DSN_STR_ODBC_DATA_SOURCES,
    DSN_STR_DRIVER,
    DSN_STR_DESCRIPTION,
    DSN_STR_VENDOR,
    DSN_STR_DRIVER_TYPE,
    DSN_STR_SERVER,
    DSN_STR_DATABASE,
    DSN_STR_SERVER_TYPE,
    DSN_STR_PROMPT_UID_PWD,
    DSN_STR_WITH_OPTION,
    DSN_STR_ROLE_NAME,
    DSN_STR_ROLE_PWD,
    DSN_STR_DISABLE_CAT_UNDERSCORE,
    DSN_STR_ALLOW_PROCEDURE_UPDATE,
    DSN_STR_USE_SYS_TABLES,
    DSN_STR_BLANK_DATE,
    DSN_STR_DATE_1582,
    DSN_STR_CAT_CONNECT,
    DSN_STR_NUMERIC_OVERFLOW,
    DSN_STR_SUPPORT_II_DECIMAL,
    DSN_STR_CAT_SCHEMA_NULL,
    DSN_STR_READ_ONLY,
    DSN_STR_SELECT_LOOPS,
    DSN_STR_CONVERT_3_PART,
    DSN_STR_DBMS_PWD,
    DSN_STR_GROUP,
    DSN_STR_MULTIBYTE_FILL_CHAR,
    DSN_STR_INGRESDATE,
    DSN_STR_STRING_TRUNCATION,
    "\0"              
};

GLOBALDEF char *dsnValues[] = 
{
    DSN_STR_N,
    DSN_STR_Y,
    DSN_STR_IGNORE,
    DSN_STR_FAIL,
    DSN_STR_SELECT,
    DSN_STR_CURSOR,
    DSN_STR_INGRES,
    DSN_STR_INGRES_CORP,
    "\0"
};

/*
** Name: readConfigData
**
** Description:
**      This function reads the configuration file and stores the
**      data in the OCFG_SECT cache.
**
** Return value:
**      status              Error number or OK.
**
** Re-entrancy:
**      This function does not update shared memory.
**
** Parameters:
**      ocfg_cb            ODBC configuration control block.
**
** History:
**      03-Jun-03 (loera01)
**          Created.
**   03-May-2010 (Ralph Loen) Bug 123676
**          Return error F_OD0172_IDS_ERR_CORRUPT_SECT if a section header is 
**          corrupted.
*/

STATUS readConfigData(OCFG_CB *ocfg_cb)
{
    LOCATION loc;
    FILE *   pfile = NULL;
    char     Token[OCFG_MAX_STRLEN], *pToken = &Token[0];
    char     buf[OCFG_MAX_STRLEN];                  /* ini file buffer  */
    char     dv[OCFG_MAX_STRLEN];
    char     locbuf[MAX_LOC + 1];
    char     *p;
    OCFG_SECT *section;
    ATTR_DATA *attr_data;
    STATUS status = OK;
    
    if (( status = LOfroms(PATH & FILENAME, ocfg_cb->path, &loc)) != OK)
       return status;

    /*
    ** If file isn't available for read, try opening for write.
    */
    if (SIfopen(&loc, "r", (i4)SI_TXT, OCFG_MAX_STRLEN, &pfile) != OK)
    {
# ifndef VMS
        if ((status = SIfopen(&loc, "w", (i4)SI_TXT, OCFG_MAX_STRLEN, &pfile)) 
            != OK)
# endif /* VMS */
            return status; 
    }

    while (SIgetrec (buf, (i4)sizeof(buf), pfile) == OK)
    {
        p = skipSpaces (buf);   /* skip whitespace */
        if (CMcmpcase(p,ERx("[")) == 0) /* if new section */
        {
            CMnext(p);
            p = getFileToken(p, pToken, FALSE); /* get entry keyword */
            section = addSection(ocfg_cb, pToken);
        }
        else if ( !STindex(buf, "[", 0) && STindex(buf, "]", 0))
        {
            return F_OD0172_IDS_ERR_CORRUPT_SECT;
        }
        else if ( !STindex(buf, "]", 0) && STindex(buf, "[", 0))
        {
            return F_OD0172_IDS_ERR_CORRUPT_SECT;
        }
        else
        {
            p = getFileToken(p, pToken, FALSE); /* get entry keyword */
        }
        if (*pToken == EOS)      /* if missing entry keyword */
            continue;            /* then skip the record     */
        p = skipSpaces (p);  /* skip leading whitespace   */
        if (CMcmpcase(p,ERx("=")) != 0)
        {
            continue; /* if missing '=' in key = val, skip record */
        }
        /*
        ** Save the first token.
        */
        STcopy(pToken, dv); 
        CMnext(p);   /* skip the '='              */
# ifdef VMS
        /*
        ** For VMS, don't terminate the token at ']' for path entries.
        */
        if ( !STbcompare(dv, 0, DRV_STR_DRIVER, 0, TRUE ) ||
             !STbcompare(dv, 0, DRV_STR_TRACE_FILE, 0, TRUE ) || 
             !STbcompare(dv, 0, DRV_STR_TRACE_FILE, 0, TRUE) )
	    p = getFileToken(p, pToken, TRUE); /* get entry value */
        else
# endif
            p = getFileToken(p, pToken, FALSE); /* get entry value */
	attr_data = addData(section, dv, pToken);
    }   /* end while loop processing entries */
    SIclose (pfile);
    return status; 
}

/*
** Name: formatConfigInfo
**
** Description:
**      This function converts data in the OCFG_SECT cache to OCFG_INFO 
**      format.
**
** Return value:
**      status              Error number or OK.
**
** Re-entrancy:
**      This function does not update shared memory.
**
** Parameters:
**      ocfg_cb            ODBC configuration control block.
**
** History:
**      03-Jun-03 (loera01)
**          Created.
*/

STATUS formatConfigInfo(OCFG_CB *ocfg_cb)
{
    OCFG_SECT *section;
    OCFG_INFO *info;
    ATTR_DATA *attr_data;
    ATTR_FORMAT *attr_format;
    BOOL got_vendor = FALSE, got_driver_type = FALSE;
    i4 attrID = OCFG_UNKNOWN;
    i4 attrValue = OCFG_UNKNOWN;
    QUEUE *q,*aq;
    BOOL isDriver = ocfg_cb->hdr.ocfg_id == OCFG_DRIVER ? TRUE : FALSE;

    /*
    ** Search the OCFG_SECT queue for driver definitions for "DriverType"
    ** and "Vendor" attributes, and if the definitions are recognized,
    ** mark them as such.
    */
    for (q = ocfg_cb->sect_q.q_prev; q != &ocfg_cb->sect_q; q = q->q_prev)
    {
        section = (OCFG_SECT *)q;
        section->recognized = got_driver_type = got_vendor = FALSE;  
        for (aq = section->data_q.q_prev; aq != &section->data_q; 
            aq = aq->q_prev)
        {
            attr_data = (ATTR_DATA *)aq;
            attrID = getAttributeID(driverAttributes, attr_data->data_name);
            if (attrID == DRV_ATTR_VENDOR)
            {
                attrValue = getAttributeID(driverValues, attr_data->data_value); 
                if (attrValue == DRV_VALUE_COMP_ASSOC)
                {
                    got_vendor = TRUE;
                }
                else if (attrValue == DRV_VALUE_INGRES_CORP)
                {
                    got_vendor = TRUE;
                }
            }
            if (attrID == DRV_ATTR_DRIVER_TYPE)
            {
                attrValue = getAttributeID(driverValues, attr_data->data_value); 
                if (attrValue == DRV_VALUE_INGRES)
                    got_driver_type = TRUE;
            }
            if (got_driver_type && got_vendor)
            {
                if (isDriver && driverIsInstalled(ocfg_cb, section->sect_name))
                    section->recognized = TRUE;
		else if (!isDriver && validDSN(ocfg_cb, section->sect_name))
		    section->recognized = TRUE;
                break;
            }
        } /* end for (aq = section->attr... */
    } /* end for (q = ocfg_cb... */

    /*
    ** Validate and convert the data for recognized sections to the
    ** OCFG_INFO format.
    */
    for (q = ocfg_cb->sect_q.q_prev; q != &ocfg_cb->sect_q; q = q->q_prev)
    {
        section = (OCFG_SECT *)q;
        if (section->recognized)
        {
            info = addInfo(ocfg_cb,section->sect_name);
            for (aq = section->data_q.q_prev; aq != &section->data_q; 
                aq = aq->q_prev)
            {
                attr_data = (ATTR_DATA *)aq;
                if (isDriver)
                    attrID = getAttributeID(driverAttributes,
                        attr_data->data_name);
                else
                    attrID = getAttributeID(dsnAttributes,
                        attr_data->data_name); 
                if (attrID >= 0)
                {
                    attr_format = addFormat(info,attrID,
                        attr_data->data_value);
                }
            } /* end for (aq = section->data... */
        } /* end if section->recognized... */
    } /* end q = ocfg_cb->sect_q... */
    return OK;
}

/*
** Name: emptyCache
**
** Description:
**      This function frees allocated memory in the ODBC configuration
**      cache and removes the associated pointers from their respective
**      queues.
**
** Return value:
**      status              Error number or OK.
**
** Re-entrancy:
**      This function does not update shared memory.
**
** Parameters:
**      ocfg_cb            ODBC configuration control block.
**
** History:
**      03-Jun-03 (loera01)
**          Created.
*/

STATUS emptyCache( OCFG_CB * ocfg_cb )
{
   STATUS status = OK;
   OCFG_SECT *section;
   OCFG_INFO *info;
   ATTR_DATA *attr_data;
   ATTR_FORMAT *attr_format;
   i4 i,j;

   for (i = 0; i < ocfg_cb->ocfg_info_count; i++)
   {
       info = ((OCFG_INFO *)ocfg_cb->info_q.q_prev);
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
   }
   for (i = 0; i < ocfg_cb->ocfg_sect_count; i++)
   {
       section = ((OCFG_SECT *)ocfg_cb->sect_q.q_prev);
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
   }
   ocfg_cb->ocfg_info_count = 0;
   ocfg_cb->ocfg_sect_count = 0;
   return status;
}

/*
** Name: getAttributeValue
**
** Description:
**      This function returns the default definition string for a
**      given attribute ID.
**
** Return value:
**      attribStr          Attribute string.
**
** Re-entrancy:
**      This function does not update shared memory.
**
** Parameters:
**      attribTable        Attribute table.
**      attrID             Attribute ID.
**
** History:
**      03-Jun-03 (loera01)
**          Created.
*/

char *getAttributeValue( char **attribTable, i4 attrID )
{
     return attribTable[attrID];
}

/*
** Name: getAttributeID
**
** Description:
**      This function returns the attribute ID for a given
**      attribute value.
**
** Return value:
**      attrID             Attribute ID. 
**
** Re-entrancy:
**      This function does not update shared memory.
**
** Parameters:
**      attribTable        Attribute table.
**      attribName         Attribute name.
**
** History:
**      03-Jun-03 (loera01)
**          Created.
*/

i4 getAttributeID( char **attribTable, char *attrName )
{
    i4 i=0;
    
    while ( attribTable[i][0] != '\0')
    {
        if ( !STbcompare(attribTable[i], 0, attrName, 0, TRUE ) )
            return i;
        i++;
    }
    return OCFG_UNKNOWN;
}

/*
** Name: driverIsInstalled
**
** Description:
**      This function determines whether a driver definition ls listed
**      as installed in the [ODBC DRIVERS] section.
**
** Return value:
**      isInstalled        True if listed, false otherwise.
**
** Re-entrancy:
**      This function does not update shared memory.
**
** Parameters:
**      ocfg_cb            ODBC configuration control block.
**      driverName         Driver name. 
**
** History:
**      03-Jun-03 (loera01)
**          Created.
*/

BOOL driverIsInstalled( OCFG_CB *ocfg_cb, char *driverName )
{
    QUEUE *q, *aq;
    OCFG_SECT *section;
    ATTR_DATA *attr_data;

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
                if ( !STbcompare(driverName, 0, attr_data->data_name, 
                    0, TRUE ) )
                {
                    if ( getAttributeID(driverValues, attr_data->data_value) 
                        == DRV_VALUE_INSTALLED)
                        return TRUE;
                    else
                        return FALSE;
                }
            }
        }
    }
    return FALSE;
}
/*
** Name: validDSN
**
** Description:
**      This function determines whether a dsn definition is listed
**      as one of the recognized drivers.  At present it only checks to
**      see that the driver name is listed in the [ODBC DATA SOURCES]
**      section.
**
** Return value:
**      isInstalled        True if listed, false otherwise.
**
** Re-entrancy:
**      This function does not update shared memory.
**
** Parameters:
**      ocfg_cb         ODBC configuration control block.
**      dsnName         DSN name. 
**
** History:
**      03-Jun-03 (loera01)
**          Created.
*/

bool validDSN( OCFG_CB *ocfg_cb, char *dsnName )
{
    QUEUE *q, *aq;
    OCFG_SECT *section;
    ATTR_DATA *attr_data;

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
                if ( !STbcompare(dsnName, 0, attr_data->data_name, 
                    0, TRUE ) )
                    return TRUE;
            }
        }
    }
    return FALSE;
}


/*
** Name: isDriverDef
**
** Description:
**      This function determines section validates the "Vendor" and 
**      "DriverType" attributes for a driver definition.
**
** Return value:
**      isDefined           True if the attributes are validated, false  
**                          otherwise.
**
** Re-entrancy:
**      This function does not update shared memory.
**
** Parameters:
**      attr_id             Driver attributes.
**      count               Attribute count.
**
** History:
**      03-Jun-03 (loera01)
**          Created.
*/

BOOL isDriverDef ( ATTR_ID *attr_id, i4 count)
{
    i4 i;
    BOOL got_vendor = FALSE, got_driver_type = FALSE;
    i4 drvAttId = OCFG_UNKNOWN, driverValue = OCFG_UNKNOWN;

    for (i = 0; i < count; i++)
    { 
        if ( attr_id[i].id == DRV_ATTR_VENDOR )
        {
            if (getAttributeID(driverValues, attr_id[i].value)
                == DRV_VALUE_COMP_ASSOC )    
                    got_vendor = TRUE;
            else if (getAttributeID(driverValues, attr_id[i].value)
                == DRV_VALUE_INGRES_CORP )    
                    got_vendor = TRUE;
        }
        if ( attr_id[i].id == DRV_ATTR_DRIVER_TYPE ) 
        {
            driverValue = getAttributeID(driverValues,
                attr_id[i].value); 
            if (driverValue == DRV_VALUE_INGRES)
                    got_driver_type = TRUE;
        }
    }
    return ( got_vendor && got_driver_type ? TRUE : FALSE );
}

/*
** Name: addDataSources
**
** Description:
**      This function adds a "dsn=driver" definition to the
**      cache that represents the [ODBC DATA SOURCES] section.
**
** Return value:
**      status              Error number or OK.
**
** Re-entrancy:
**      This function does not update shared memory.
**
** Parameters:
**      ocfg_cb            ODBC configuration control block.
**
** History:
**      03-Jun-03 (loera01)
**          Created.
*/

STATUS addDataSources ( OCFG_CB *ocfg_cb, char *dsnName )
{
    STATUS status = OK;
    OCFG_SECT *section;
    ATTR_DATA *data;
    QUEUE *q,*aq;
    bool section_exists = FALSE, is_linked = FALSE;
    char **defAttr = getDefaultInfo();

    /*
    ** See if an [ODBC DATA SOURCES] section exists in the first place.
    */
    for (q = ocfg_cb->sect_q.q_prev; q != &ocfg_cb->sect_q; q = q->q_prev)
    {
        section = (OCFG_SECT *)q; 
        if ( !STbcompare(DSN_STR_ODBC_DATA_SOURCES, 0, section->sect_name, 
            0, TRUE ) )
        {
            section_exists = TRUE;
            /*
            ** See if the dsn is linked to a driver.
            */
            for (aq = section->data_q.q_prev; aq != &section->data_q;
                aq = aq->q_prev)
            {
                data = (ATTR_DATA *)aq;
                if ( !STbcompare(dsnName, 0, data->data_name, 0, TRUE ) )
		{
		    /*
		    ** Try to recover from missing ocfginfo.dat file in
		    ** $II_SYSTEM/ingres/install.
		    */
                    if ( defAttr == NULL && !STbcompare(DSN_STR_INGRES, 0,
                        data->data_value, 0, TRUE ) )
                    {
                         is_linked = TRUE; 
                         break;
                    }
		    /*
		    ** Got the file, so read through the list of
		    ** acceptable drivers.
		    */
		    else 
		    {
                        if ( !STbcompare(defAttr[INFO_ATTR_DRIVER_NAME], 0,
                                data->data_value, 0, TRUE ) )
                        {
                                is_linked = TRUE; 
			        break;
			}
                        if ( !STbcompare(defAttr[INFO_ATTR_ALT_DRIVER_NAME], 0,
                                data->data_value, 0, TRUE ) )
                        {
                                is_linked = TRUE; 
			        break;
			}
		    }
                }
            }
            break;
        }
    }
    /*
    ** if no [ODBC Data Sources] section, put one in the cache.
    */
    if (!section_exists)
    {
        section = addSection(ocfg_cb, DSN_STR_ODBC_DATA_SOURCES);
        data = addData(section, dsnName, DSN_STR_INGRES);
        return OK;
    }

    /*
    ** Otherwise, add the "dsnName = Driver" bit as required.
    */
    if (!is_linked)
    {
        data = addData(section, dsnName, DRV_STR_INGRES);
    }

    return OK;
}

/*
** Name: addInstalledDriver
**
** Description:
**      This function adds a "driver=installed" definition to the
**      cache that represents the [ODBC DRIVERS] section.
**
** Return value:
**      status              Error number or OK.
**
** Re-entrancy:
**      This function does not update shared memory.
**
** Parameters:
**      ocfg_cb            ODBC configuration control block.
**
** History:
**      03-Jun-03 (loera01)
**          Created.
*/

STATUS addInstalledDriver ( OCFG_CB *ocfg_cb, char *driverName )
{
    STATUS status = OK;
    OCFG_SECT *section;
    ATTR_DATA *data;
    QUEUE *q,*aq;
    bool section_exists = FALSE, is_installed = FALSE;

    /*
    ** See if an [ODBC Drivers] section exists in the first place.
    */
    for (q = ocfg_cb->sect_q.q_prev; q != &ocfg_cb->sect_q; q = q->q_prev)
    {
        section = (OCFG_SECT *)q; 
        if ( !STbcompare(DRV_STR_ODBC_DRIVERS, 0, section->sect_name, 
            0, TRUE ) )
        {
            section_exists = TRUE;
            /*
            ** See if the driver is listed as installed.
            */
            for (aq = section->data_q.q_prev; aq != &section->data_q;
                aq = aq->q_prev)
            {
                data = (ATTR_DATA *)aq;
                if ( !STbcompare(driverName, 0, data->data_name, 
                    0, TRUE ) && !STbcompare(DRV_STR_INSTALLED, 0,
                    data->data_value, 0, TRUE ) )
                {
                     is_installed = TRUE; 
                     break;
                }
            }
            break;
        }
    }
    /*
    ** if no [ODBC Drivers] section, put one in the cache.
    */
    if (!section_exists)
    {
        section = addSection(ocfg_cb, DRV_STR_ODBC_DRIVERS);
        data = addData(section, driverName, DRV_STR_INSTALLED);
        return OK;
    }

    /*
    ** Otherwise, add the "driverName = Installed" bit as required.
    */
    if (!is_installed)
    {
        data = addData(section, driverName, DRV_STR_INSTALLED);
    }

    return OK;
}

/*
** Name: addConfigOptions
**
** Description:
**      This function adds default configuration attributes to the cache 
**      representing the [ODBC] section, if required.
**
** Return value:
**      status              Error number or OK.
**
** Re-entrancy:
**      This function does not update shared memory.
**
** Parameters:
**      ocfg_cb            ODBC configuration control block.
**
** History:
**      03-Jun-03 (loera01)
**          Created.
*/

STATUS addConfigOptions ( OCFG_CB *ocfg_cb )
{
    STATUS status = OK;
    OCFG_SECT *section;
    ATTR_DATA *data;
    QUEUE *q,*aq;
    BOOL section_exists = FALSE, has_Trace = FALSE, has_TraceFile = FALSE,
        has_alt_traceFile = FALSE;
    BOOL has_Timeout = FALSE;

    /*
    ** See if an [ODBC] section exists in the first place.
    */
    for (q = ocfg_cb->sect_q.q_prev; q != &ocfg_cb->sect_q; q = q->q_prev)
    {
        section = (OCFG_SECT *)q; 
        if ( !STbcompare(DRV_STR_ODBC, 0, section->sect_name, 
            0, TRUE ) )
        {
            section_exists = TRUE;
            /*
            ** See if the [ODBC] section has a Trace and TraceFile entry.
            */
            for (aq = section->data_q.q_prev; aq != &section->data_q;
                aq = aq->q_prev)
            {
                data = (ATTR_DATA *)aq;
                if ( !STbcompare(DRV_STR_TRACE, 0, data->data_name, 0, TRUE ) )
                     has_Trace = TRUE; 
                if ( !STbcompare(DRV_STR_TRACE_FILE, 0, data->data_name, 0, 
                     TRUE ) )
                     has_TraceFile = TRUE; 
                if ( !STbcompare(DRV_STR_ALT_TRACE_FILE, 0, data->data_name, 0, 
                     TRUE ) )
                     has_alt_traceFile = TRUE; 
                if ( !STbcompare(DRV_STR_POOL_TIMEOUT, 0, data->data_name, 0, 
                     TRUE ) )
                     has_Timeout = TRUE; 
            }
            break;
        }
    }
    /*
    ** if no [ODBC] section, put one in the cache.
    */
    if (!section_exists)
    {
        section = addSection( ocfg_cb, DRV_STR_ODBC );
        data = addData(section, DRV_STR_TRACE, DRV_STR_N);
        data = addData(section, DRV_STR_TRACE_FILE, "odbctrace.log");
        data = addData(section, DRV_STR_ALT_TRACE_FILE, "odbctrace.log");
        data = addData(section, DRV_STR_POOL_TIMEOUT, DRV_STR_NEG_ONE);
        return OK;
    }

    /*
    ** Otherwise, add the "Trace" and "TraceFile" entries as required.
    */
    if (! has_Trace)
        data = addData(section, DRV_STR_TRACE, DRV_STR_N);
    if (! has_TraceFile)
        data = addData(section, DRV_STR_TRACE_FILE, "odbctrace.log");
    if (! has_alt_traceFile)
        data = addData(section, DRV_STR_ALT_TRACE_FILE, "odbctrace.log");
    if (! has_Timeout)
        data = addData(section, DRV_STR_POOL_TIMEOUT, DRV_STR_NEG_ONE);

    return OK;
}

/*
** Name: addSection
**
** Description:
**      This function adds OCFG_SECT info to the cache.
**
** Return value:
**      sectPtr            Pointer to the OCFG_SECT data.
**
** Re-entrancy:
**      This function does not update shared memory.
**
** Parameters:
**      ocfg_cb            ODBC configuration control block.
**      sectName           Section name.
**
** History:
**      03-Jun-03 (loera01)
**          Created.
*/

OCFG_SECT *addSection ( OCFG_CB *ocfg_cb, char *sectName )
{
    OCFG_SECT *section;

    ocfg_cb->ocfg_sect_count++;
    section = (OCFG_SECT *) MEreqmem( (u_i2) 0, 
        (u_i4)sizeof(OCFG_SECT), TRUE, (STATUS *) NULL);
    QUinsert((QUEUE *)section,&ocfg_cb->sect_q);
    QUinit(&section->data_q);
    section->sect_name = STalloc(sectName);
    section->data_count = 0;

    return section;
}

/*
** Name: addInfo
**
** Description:
**      This function adds OCFG_INFO data to the cache.
**
** Return value:
**      ocfgInfo           Pointer to OCFG_INFO data.
**
** Re-entrancy:
**      This function does not update shared memory.
**
** Parameters:
**      ocfg_cb            ODBC configuration control block.
**      infoName           Name of the configuration attribute.
**
** History:
**      03-Jun-03 (loera01)
**          Created.
*/

OCFG_INFO *addInfo ( OCFG_CB *ocfg_cb, char *infoName )
{
    OCFG_INFO *info;

    ocfg_cb->ocfg_info_count++;
    info = (OCFG_INFO *) MEreqmem( (u_i2) 0, 
        (u_i4)sizeof(OCFG_INFO), TRUE, (STATUS *) NULL);
    QUinsert((QUEUE *)info, &ocfg_cb->info_q);
    QUinit(&info->format_q);
    info->info_name = STalloc(infoName);
    info->format_count = 0;

    return info;
}

/*
** Name: addData
**
** Description:
**      This function adds data to the OCFG_SECT block in the cache.
**
** Return value:
**      attrData           Poiner to ATTR_DATA block.
**
** Re-entrancy:
**      This function does not update shared memory.
**
** Parameters:
**      section            Section data.
**      dataName           Attribute data name. 
**      dataValue          Attribute data value. 
**
** History:
**      03-Jun-03 (loera01)
**          Created.
*/

ATTR_DATA *addData ( OCFG_SECT *section, char *dataName, char* dataValue )
{
    ATTR_DATA *data;

    section->data_count++; 
    data = (ATTR_DATA *) MEreqmem( (u_i2) 0, 
        (u_i4)sizeof(ATTR_DATA), TRUE, (STATUS *) NULL);
    QUinsert((QUEUE *)data,&section->data_q);
    data->data_name = STalloc(dataName);
    data->data_value = STalloc(dataValue);
    return data;
}

/*
** Name: addFormat
**
** Description:
**      This function adds formatted data to a OCFG_INFO block in
**      the cache.
**
** Return value:
**      attrFormat         Pointer to ATTR_FORMAT block.
**
** Re-entrancy:
**      This function does not update shared memory.
**
** Parameters:
**      ocfg_info          Formatted info section.
**      id                 Attribute ID.
**      formatValue        Value of the attribute. 
**
** History:
**      03-Jun-03 (loera01)
**          Created.
*/

ATTR_FORMAT *addFormat ( OCFG_INFO *info, i4 id, char *formatValue )
{
    ATTR_FORMAT *format;

    info->format_count++;
    format = (ATTR_FORMAT *)MEreqmem( (u_i2) 0,
        (u_i4)sizeof(ATTR_FORMAT), TRUE, (STATUS *) NULL);
    QUinsert((QUEUE *)format,&info->format_q);
    format->format_id = id;
    format->format_value = STalloc(formatValue);
    return format;
}

/*
** Name: rewriteINI
**
** Description:
**      This function rewrites the driver or DSN configuration file.
**
** Return value:
**      status              Error number or OK.
**
** Re-entrancy:
**      This function does not update shared memory.
**
** Parameters:
**      ocfg_cb            ODBC configuration control block.
**
** History:
**      03-Jun-03 (loera01)
**          Created.
*/

STATUS rewriteINI( OCFG_CB *ocfg_cb )
{
    FILE *pfile;
    LOCATION loc;
    OCFG_SECT *section;
    OCFG_INFO *info;
    ATTR_DATA *attr_data;
    ATTR_FORMAT *attr_format;
    char cfgStr[OCFG_MAX_STRLEN];
    i4  i = 0;
    QUEUE *q, *aq;
    STATUS status = OK;
    BOOL isDriver = ocfg_cb->hdr.ocfg_id == OCFG_DRIVER ? TRUE : FALSE;

    /*
    ** Find the file and open it for write.
    */
    if (( status = LOfroms(PATH & FILENAME, ocfg_cb->path, &loc)) != OK)
       return status;

    if ((status = SIfopen(&loc, "w", (i4)SI_TXT, OCFG_MAX_STRLEN, 
        &pfile)) != OK)
        return status; 
    /*
    ** Rewrite the file from the new database.  First write the
    ** "ODBC drivers" section, then the drivers, then the 
    ** "ODBC" section.
    */
    if (isDriver)
    {
    for (q = ocfg_cb->sect_q.q_prev; q != &ocfg_cb->sect_q; q = q->q_prev)
    {
        section = (OCFG_SECT *)q;
        if ( !STbcompare(DRV_STR_ODBC_DRIVERS, 0, section->sect_name, 0, 
            TRUE ) )
        {
            /*
            ** Rewrite section name.
            */
            STprintf(cfgStr, SECTION_FORMAT, section->sect_name); 
            SIputrec(cfgStr, pfile);
            for (aq = section->data_q.q_prev; aq != &section->data_q; 
                aq = aq->q_prev)
            {
                attr_data = (ATTR_DATA *)aq;
                STprintf(cfgStr, ATTRIB_FORMAT, attr_data->data_name,
                    attr_data->data_value);
                SIputrec(cfgStr, pfile);
            }
            break;
        }
    }

    SIputrec("\n", pfile);
    i = 0;
    for (q = ocfg_cb->sect_q.q_prev; q != &ocfg_cb->sect_q; q = q->q_prev)
    {
            section = (OCFG_SECT *)q;
        if ( STbcompare(DRV_STR_ODBC_DRIVERS, 0, section->sect_name, 0, TRUE )
            && STbcompare(DRV_STR_ODBC, 0, section->sect_name, 0, TRUE ) )
        {
            if (i++)
	        SIputrec("\n", pfile);
            /*
            ** Rewrite section name.
            */
            STprintf(cfgStr, SECTION_FORMAT, section->sect_name); 
            SIputrec(cfgStr, pfile);
            for (aq = section->data_q.q_prev; aq != &section->data_q; 
                aq = aq->q_prev)
            {
                attr_data = (ATTR_DATA *)aq;
                STprintf(cfgStr, ATTRIB_FORMAT, attr_data->data_name,
                    attr_data->data_value);
                SIputrec(cfgStr, pfile);
            }
        }
    } /* End for (q = ocfg_cb->sect_q.q_prev... */

    SIputrec("\n", pfile);

    for (q = ocfg_cb->sect_q.q_prev; q != &ocfg_cb->sect_q; q = q->q_prev)
    {
        section = (OCFG_SECT *)q;
        if ( !STbcompare(DRV_STR_ODBC, 0, section->sect_name, 0, TRUE ) )
        {
            /*
            ** Rewrite section name.
            */
            STprintf(cfgStr, SECTION_FORMAT, section->sect_name); 
            SIputrec(cfgStr, pfile);
            for (aq = section->data_q.q_prev; aq != &section->data_q; 
                aq = aq->q_prev)
            {
                attr_data = (ATTR_DATA *)aq;
                STprintf(cfgStr, ATTRIB_FORMAT, attr_data->data_name,
                    attr_data->data_value);
                SIputrec(cfgStr, pfile);
            }
            break;
        }
    }
    }
    else  /* DSN def */
    {
    for (q = ocfg_cb->sect_q.q_prev; q != &ocfg_cb->sect_q; q = q->q_prev)
    {
        section = (OCFG_SECT *)q;
        if ( !STbcompare(DSN_STR_ODBC_DATA_SOURCES, 0, section->sect_name, 0, 
            TRUE ) )
        {
            /*
            ** Rewrite section name.
            */
            STprintf(cfgStr, SECTION_FORMAT, section->sect_name); 
            SIputrec(cfgStr, pfile);
            for (aq = section->data_q.q_prev; aq != &section->data_q; 
                aq = aq->q_prev)
            {
                attr_data = (ATTR_DATA *)aq;
                STprintf(cfgStr, ATTRIB_FORMAT, attr_data->data_name,
                    attr_data->data_value);
                SIputrec(cfgStr, pfile);
            }
            break;
        }
    }

    SIputrec("\n", pfile);

    i = 0;
    for (q = ocfg_cb->sect_q.q_prev; q != &ocfg_cb->sect_q; q = q->q_prev)
    {
        section = (OCFG_SECT *)q;
        if ( STbcompare(DSN_STR_ODBC_DATA_SOURCES, 0, section->sect_name, 
            0, TRUE ) )
        {
            if (i++)
	        SIputrec("\n", pfile);
            /*
            ** Rewrite section name.
            */
            STprintf(cfgStr, SECTION_FORMAT, section->sect_name); 
            SIputrec(cfgStr, pfile);
            for (aq = section->data_q.q_prev; aq != &section->data_q; 
                aq = aq->q_prev)
            {
                attr_data = (ATTR_DATA *)aq;
                STprintf(cfgStr, ATTRIB_FORMAT, attr_data->data_name,
                    attr_data->data_value);
                SIputrec(cfgStr, pfile);
            }
        }
    } /* End for (q = ocfg_cb->sect_q.q_prev... */
    } /* if (isDriver) */
    
    SIclose( pfile);
    return status;
}

/*
** Name: getDefaultInfo
**
** Description:
**      This function retrieves the default ODBC configuration information
**      from the $II_SYSTEM/ingres/install/ocfginfo.dat file.
**
** Return value:
**      defaultInfo        Array of default configuration values.
**
** Re-entrancy:
**      This function does not update shared memory.
**
** Parameters:
**      none.
**
** History:
**      03-Jun-03 (loera01)
**          Created.
*/

char **getDefaultInfo()
{
    char *dirPath;
    static char *defAttr[4];
    static bool attrDefined = FALSE;
    i4 i;
    char *ii_installation;
  
    if (attrDefined)
        return (char **)defAttr;

    NMgtAt("II_INSTALLATION",&ii_installation);

#ifdef VMS
    defAttr[INFO_ATTR_DRIVER_FILE_NAME] = (char *)MEreqmem( (u_i2) 0,
        (u_i4)16, TRUE, (STATUS *) NULL);
    if (ii_installation == NULL)
        STcopy("ODBCFELIB.EXE", defAttr[INFO_ATTR_DRIVER_FILE_NAME]);
    else
        STprintf(defAttr[INFO_ATTR_DRIVER_FILE_NAME], "ODBCFELIB%s.EXE", 
            ii_installation);
    defAttr[INFO_ATTR_RONLY_DRV_FNAME] = (char *)MEreqmem( (u_i2) 0,
        (u_i4)18, TRUE, (STATUS *) NULL);
    if (ii_installation == NULL)
        STcopy("ODBCFEROLIB.EXE", defAttr[INFO_ATTR_RONLY_DRV_FNAME]);
    else
        STprintf(defAttr[INFO_ATTR_RONLY_DRV_FNAME], "ODBCFEROLIB%s.EXE", 
            ii_installation);
#else
    defAttr[INFO_ATTR_DRIVER_FILE_NAME] = MEreqmem(0, STlength("libiiodbcdriver.1.") + STlength(SLSFX) + 1, TRUE, NULL);
    STprintf(defAttr[INFO_ATTR_DRIVER_FILE_NAME], "libiiodbcdriver.1.%s", 
        SLSFX);
    defAttr[INFO_ATTR_RONLY_DRV_FNAME] = MEreqmem(0, 
        STlength("libiiodbcdriverro.1.") + STlength(SLSFX) + 1, TRUE, NULL);
    STprintf(defAttr[INFO_ATTR_RONLY_DRV_FNAME], 
        "libiiodbcdriverro.1.%s", SLSFX);
#endif /* defined(hpb_us5) || defined(hp2_us5) || defined(i64_hpu) */
    defAttr[INFO_ATTR_DRIVER_NAME] = STalloc("Ingres");

/*
** System VMS installation take a default II_INSTALLATION value of AA.
*/
#ifdef VMS
    if (ii_installation == NULL)
    {
       ii_installation = STalloc("AA");
    }
#endif

   defAttr[INFO_ATTR_ALT_DRIVER_NAME] = (char *)MEreqmem( (u_i2) 0, 
        (u_i4)10, TRUE, (STATUS *) NULL);
    STprintf(defAttr[INFO_ATTR_ALT_DRIVER_NAME], "Ingres %s", ii_installation);
   
    attrDefined = TRUE;

    return (char **)defAttr;
}

char * skipSpaces(char * p)
{
   while (CMspace(p))  CMnext(p);       /* scan past blanks */
      return(p);
}

char * getFileToken(char *p, char * szToken, bool ignoreBracket)
{
    char *   pToken;
    i4       lenToken;

    pToken = p = skipSpaces (p);  /* ptoken -> start of token */
# ifdef VMS
    if (ignoreBracket)
        for(;  *p  &&  CMcmpcase(p,ERx("=")) != 0
               &&  CMcmpcase(p,ERx("\n"))!= 0
               &&  CMcmpcase(p,ERx(";")) != 0; /* stop scan if comment */  
           CMnext(p));  /* scan the token */
    else
# endif /* VMS */
        for(;  *p  &&  CMcmpcase(p,ERx("]")) != 0
               &&  CMcmpcase(p,ERx("=")) != 0
               &&  CMcmpcase(p,ERx("\n"))!= 0
               &&  CMcmpcase(p,ERx(";")) != 0; /* stop scan if comment */  
           CMnext(p));  /* scan the token */
    lenToken = p - pToken;             /* lenToken = length of token */
    if (lenToken > 255)                /* safety check */
        lenToken = 255;
    if (lenToken)
       MEcopy(pToken, lenToken, szToken); /* copy token to token buf */
    szToken[lenToken] = EOS;           /* null terminate the token*/
    STtrmwhite(szToken);            /* trim trailing white space but
                                       leave any embedded white*/
    return(p);
}

/*
** Name: getMgrInfo
**
** Description:
**      This function retrieves the alternate path info, if it exists,
**      from the $II_CONFIG/odbcmgr.dat file.
**
** Return value:
**      defaultInfo        Array of default configuration values.
**
** Re-entrancy:
**      This function does not update shared memory.
**
** Parameters:
**      none.
**
** History:
**      06-Sep-03 (loera01)
**          Created.
*/

char *getMgrInfo(bool *sysIniDefined)
{
    char *dirPath;
    char fullPath[OCFG_MAX_STRLEN];
    char buf[OCFG_MAX_STRLEN];           
    FILE *pfile;
    LOCATION loc;
    char *odbcsysini;
    static bool pathDefined = FALSE;
    static bool iniDef = FALSE;
    static char *path = NULL;
    i4 i;
    char Token[OCFG_MAX_STRLEN], *pToken = &Token[0];
    char *p;
  
    if (pathDefined)
    {
	*sysIniDefined = iniDef;
        return path;
    }

    /*
    **  See if ODBCSYSINI is supported.  Overrides other settings.
    */
    NMgtAt("ODBCSYSINI",&odbcsysini);
    if (odbcsysini  && *odbcsysini)
    {
	*sysIniDefined = iniDef = TRUE;
        path = STalloc(odbcsysini); 	
        pathDefined = TRUE;
    }
    /*
    ** Get the path of the odbcmgr.dat file and create the asscociated string.
    */
    NMgtAt(SYSTEM_LOCATION_VARIABLE,&dirPath);  /* usually II_SYSTEM */
    if (dirPath != NULL && *dirPath)
        STlcopy(dirPath,fullPath,sizeof(fullPath)-20-1);
    else
        return NULL;

# ifdef VMS
    STcat(fullPath,"[");
    STcat(fullPath,SYSTEM_LOCATION_SUBDIRECTORY);  /* usually "ingres"  */
    STcat(fullPath,".files]");
    STcat(fullPath,MGR_STR_FILE_NAME);
# else
    STcat(fullPath,"/");
    STcat(fullPath,SYSTEM_LOCATION_SUBDIRECTORY);  /* usually "ingres"  */
    STcat(fullPath,"/files/");
    STcat(fullPath,MGR_STR_FILE_NAME);
# endif /* VMS */

    if ( LOfroms(PATH & FILENAME, fullPath, &loc) != OK )
       return NULL;

    if (SIfopen(&loc, "r", (i4)SI_TXT, OCFG_MAX_STRLEN, &pfile) != OK)
        return NULL;

    while (SIgetrec (buf, (i4)sizeof(buf), pfile) == OK)
    {
        p = skipSpaces (buf);   /* skip whitespace */
        p = getFileToken(p, pToken,FALSE); /* get entry keyword */
        if (*pToken == EOS)      /* if missing entry keyword */
            continue;            /* then skip the record     */
        p = skipSpaces (p);  /* skip leading whitespace   */
        if (CMcmpcase(p,ERx("=")) != 0)
        {
            continue; /* if missing '=' in key = val, skip record */
        }

        /*
        ** Check for "DriverManagerPath".
        */
	if ( !STbcompare( pToken, 0, MGR_STR_PATH, 0, TRUE ) && 
            ! pathDefined )
        {
	    CMnext(p);   /* skip the '='              */
#ifdef VMS
	    p = getFileToken(p, pToken,TRUE); /* get entry value */
#else
	    p = getFileToken(p, pToken,FALSE); /* get entry value */
#endif /* VMS */
            if (*pToken == EOS)      /* if missing entry keyword */
                continue;            /* then skip the record     */
	    path = STalloc(pToken);
        }

        /*
        ** Check for "DriverManager".
        */
	if ( !STbcompare(pToken, 0, MGR_STR_MANAGER, 0, TRUE ) )
        {
	    CMnext(p);   /* skip the '='              */
	    p = getFileToken(p, pToken,FALSE); /* get entry value */
            if (*pToken == EOS)      /* if missing entry keyword */
                continue;            /* then skip the record     */
	    if ( !STbcompare(pToken, 0, MGR_STR_UNIX_ODBC, 0, TRUE ) )
                driverManager = MGR_VALUE_UNIX_ODBC;
            else
                driverManager = MGR_VALUE_CAI_PT;
        }
    }   /* end while loop processing entries */

    if (path)
        STtrmwhite(path);

    pathDefined = TRUE;
    SIclose(pfile);
    return path;
}
