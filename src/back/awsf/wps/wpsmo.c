/*
**Copyright (c) 2004 Ingres Corporation
*/

/*
** wpsmo.c -- MO module for wps
**
** Description
**
**      MO functions used for WPS methods.
**
**
**
** Functions:
**
**
** History:
**      02-Mar-98 (fanra01)
**          Created.
**      08-Sep-1998 (fanra01)
**          Removed erroneous semicolon.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/
# include <compat.h>
# include <sp.h>
# include <gl.h>
# include <erglf.h>
# include <mo.h>
# include <wpsfile.h>

STATUS wps_cache_define (void);
STATUS wps_cache_attach (WPS_PFILE wps_file);
VOID wps_cache_detach (WPS_PFILE wps_file);

MO_GET_METHOD cache_timeout_get;
MO_GET_METHOD cache_wps_name_get;
MO_GET_METHOD cache_wps_loc_name_get;
MO_GET_METHOD cache_wps_status_get;
MO_GET_METHOD cache_wps_counter_get;
MO_GET_METHOD cache_wps_exist_get;

GLOBALREF MO_CLASS_DEF wps_cache[];
GLOBALREF char wps_cache_index[];

/*
** Name:    wps_cache_define
**
** Description:
**      Define the MO class for wps.
**
**
** Inputs:
**      None.
**
** Outputs:
**      None.
**
** Returns:
**      OK                  successfully completed
**      MO_NO_MEMORY        MO_MEM_LIMIT exceeded
**      MO_NULL_METHOD      null get/set methods supplied when permission state
**                          they are allowed
**      !OK                 system specific error
**
** History:
**      02-Mar-98 (fanra01)
**          Created.
**
*/
STATUS
wps_cache_define (void)
{
    return (MOclassdef (MAXI2, wps_cache));
}

/*
** Name: wps_cache_attach - make an instance based on the filename.
**
** Description:
**
**      Wrapper for MOattach.  Create an instnace of the WPCS_FILE
**
** Inputs:
**
**      name            name of file
**      wps_file	    pointer to the file information structure
**
** Outputs:
**
**      none
**
** Returns:
**      OK              function completed successfully
**
** History:
**      02-Jul-98 (fanra01)
**          Created.
**
*/
STATUS
wps_cache_attach (WPS_PFILE wps_file)
{
    char buf[80];

    STATUS status = MOptrout (0, (PTR)wps_file, sizeof (buf), buf);

    if (status == OK)
    {
        MOattach (MO_INSTANCE_VAR, wps_cache_index, buf, (PTR)wps_file);
    }
return (status);
}

/*
** Name: wps_cache_detach - remove a wps_cache instance
**
** Description:
**
**      Wrapper for MOdetach.
**
** Inputs:
**
**      name            name for file
**
** Outputs:
**
**      None
**
** Returns:
**      None.
**
** History:
**      02-Jul-98 (fanra01)
**          Created.
**
*/
VOID
wps_cache_detach (WPS_PFILE wps_file)
{
    char buf[80];

    STATUS status = MOptrout (0, (PTR)wps_file, sizeof (buf), buf);

    if (status == OK)
    {
        MOdetach (wps_cache_index, buf);
    }
}

/*
** Name: cache_timeout_get
**
** Description:
**
**
** Inputs:
**
**      offset          offset to add to base object pointer.
**      objsize         size of object (ignored)
**      obj             pointer to bas object
**      userbuflen      size of the user buffer
**
** Outputs:
**
**      userbuf         updated user buffer with the cursor owner name.
**
** Returns:
**      OK                  successfully completed
**      MO_VALUE_TRUNCATED  buffer too short
**
** History:
**      02-Jul-98 (fanra01)
**          Created.
**
*/
STATUS
cache_timeout_get (i4 offset, i4  objsize, PTR obj, i4  userbuflen,
                  char * userbuf)
{
	STATUS status = OK;
	WPS_PFILE file= (WPS_PFILE)obj;

	status = MOlongout (MO_VALUE_TRUNCATED, file->timeout_end - TMsecs(), userbuflen, userbuf);
return (status);
}

/*
** Name: cache_wps_name_get
**
** Description:
**
**
** Inputs:
**
**      offset          offset to add to base object pointer.
**      objsize         size of object (ignored)
**      obj             pointer to bas object
**      userbuflen      size of the user buffer
**
** Outputs:
**
**      userbuf         updated user buffer with the cursor owner name.
**
** Returns:
**      OK                  successfully completed
**      MO_VALUE_TRUNCATED  buffer too short
**
** History:
**      02-Jul-98 (fanra01)
**          Created.
**
*/
STATUS
cache_wps_name_get (i4 offset, i4  objsize, PTR obj, i4  userbuflen,
                  char * userbuf)
{
	STATUS status = OK;
	WPS_PFILE cache = (WPS_PFILE)obj;

	status = MOstrout (MO_VALUE_TRUNCATED, cache->file->info->name, userbuflen, userbuf);
return (status);
}

/*
** Name: cache_wps_loc_name_get
**
** Description:
**
**
** Inputs:
**
**      offset          offset to add to base object pointer.
**      objsize         size of object (ignored)
**      obj             pointer to bas object
**      userbuflen      size of the user buffer
**
** Outputs:
**
**      userbuf         updated user buffer with the cursor owner name.
**
** Returns:
**      OK                  successfully completed
**      MO_VALUE_TRUNCATED  buffer too short
**
** History:
**      02-Jul-98 (fanra01)
**          Created.
**
*/
STATUS
cache_wps_loc_name_get (i4 offset, i4  objsize, PTR obj, i4  userbuflen,
                  char * userbuf)
{
	STATUS status = OK;
	WPS_PFILE cache = (WPS_PFILE)obj;

	status = MOstrout (MO_VALUE_TRUNCATED, cache->file->info->location->name, userbuflen, userbuf);
return (status);
}

/*
** Name: cache_wps_status_get
**
** Description:
**
**
** Inputs:
**
**      offset          offset to add to base object pointer.
**      objsize         size of object (ignored)
**      obj             pointer to bas object
**      userbuflen      size of the user buffer
**
** Outputs:
**
**      userbuf         updated user buffer with the cursor owner name.
**
** Returns:
**      OK                  successfully completed
**      MO_VALUE_TRUNCATED  buffer too short
**
** History:
**      02-Jul-98 (fanra01)
**          Created.
**
*/
STATUS
cache_wps_status_get (i4 offset, i4  objsize, PTR obj, i4  userbuflen,
                  char * userbuf)
{
	STATUS status = OK;
	WPS_PFILE cache = (WPS_PFILE)obj;

	status = MOlongout (MO_VALUE_TRUNCATED, cache->file->info->status, userbuflen, userbuf);
return (status);
}

/*
** Name: cache_wps_counter_get
**
** Description:
**
**
** Inputs:
**
**      offset          offset to add to base object pointer.
**      objsize         size of object (ignored)
**      obj             pointer to bas object
**      userbuflen      size of the user buffer
**
** Outputs:
**
**      userbuf         updated user buffer with the cursor owner name.
**
** Returns:
**      OK                  successfully completed
**      MO_VALUE_TRUNCATED  buffer too short
**
** History:
**      02-Jul-98 (fanra01)
**          Created.
**
*/
STATUS
cache_wps_counter_get (i4 offset, i4  objsize, PTR obj, i4  userbuflen,
                  char * userbuf)
{
	STATUS status = OK;
	WPS_PFILE cache = (WPS_PFILE)obj;

	status = MOlongout (MO_VALUE_TRUNCATED, cache->file->info->counter, userbuflen, userbuf);
return (status);
}

/*
** Name: cache_wps_exist_get
**
** Description:
**
**
** Inputs:
**
**      offset          offset to add to base object pointer.
**      objsize         size of object (ignored)
**      obj             pointer to bas object
**      userbuflen      size of the user buffer
**
** Outputs:
**
**      userbuf         updated user buffer with the cursor owner name.
**
** Returns:
**      OK                  successfully completed
**      MO_VALUE_TRUNCATED  buffer too short
**
** History:
**      02-Jul-98 (fanra01)
**          Created.
**
*/
STATUS
cache_wps_exist_get (i4 offset, i4  objsize, PTR obj, i4  userbuflen,
                  char * userbuf)
{
	STATUS status = OK;
	WPS_PFILE cache = (WPS_PFILE)obj;

	status = MOlongout (MO_VALUE_TRUNCATED, cache->file->info->exist, userbuflen, userbuf);
return (status);
}

