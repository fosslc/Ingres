/*
** Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <sl.h>
#include    <lo.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <ulf.h>
#include    <adfint.h>

/*
**  Definitions of minimum and maximum values for string types.
*/

GLOBALREF   u_char      *Chr_min;
GLOBALREF   u_char      *Chr_max;
GLOBALREF   u_char      *Cha_min;
GLOBALREF   u_char      *Cha_max;
GLOBALREF   u_char      *Txt_max;
GLOBALREF   u_char      *Vch_min;
GLOBALREF   u_char      *Vch_max;
GLOBALREF   u_char      *Lke_min;
GLOBALREF   u_char      *Lke_max;
GLOBALREF   u_char      *Bit_min;
GLOBALREF   u_char      *Bit_max;

/**
**
**  Name: ADGSHUTDOWN.C - Shut down ADF for the server.
**
**  Description:
**	This file contains all of the routines necessary to perform the 
**	external ADF call "adg_shutdown()" which shuts down ADF for the
**	server.
**
**	This file defines:
**
**	    adg_shutdown() - Shut down ADF for the server.
**
**
**  History:	$Log-for RCS$
**	23-jul-86 (thurston)
**	    Initial creation.
**	26-feb-87 (thurston)
**	    Made changes to use server CB instead of GLOBALDEFs.
**	17-jul-2000 (rucmi01) Part of fix for bug 101768.
**	    Add code to deal with special case where adg_startup is called 
**      more than once. See adg_startup() code for more details.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	29-mar-2002 (gupsh01)
**	    Free the allocated memory for the Adu_maptbl 
**	    (unicode coercion table), if it was previously allocated.
**  18-Jan-2005 (fanra01)
**      Bug 113770
**      Multiple calls to adg_shutdown causes an exception when the memory
**      allocated for the string types max. and min. values are freed
**      more than once.
**/


GLOBALREF     i4                             adg_startup_instances;

/*{
** Name: adg_shutdown()	- Shut down ADF for the server.
**
** Description:
**	This function is the external ADF call "adg_shutdown()" which
**	shuts down ADF for the server.  It does whatever cleanup and
**	freeing of resources ADF had allocated for the server as a whole.
**
**	Currently, this is a no-op since ADF does not yet allocate
**	any resources for the server.  However, it should be called
**	when terminating the server, since it might someday do this.
**
**	In the INGRES backend server, this will be called once when bringing
**	the server down.  In a non-server process such as an INGRES frontend
**	it should be called when terminating the process.
**
** Inputs:
**	none
**
** Outputs:
**	none
**
**	Returns:
**	    E_DB_OK			Operation succeeded.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	23-jul-86 (thurston)
**	    Initial creation and coding.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
**	17-jul-2000 (rucmi01) Part of fix for bug 101768.
**	    Add code to deal with special case where adg_startup is called 
**      more than once. See adg_startup() code for more details.
**	23-Jan-2004 (gupsh01)
**	    Clean up the memory allocated for mapping table during shutdown.
**	02-Sep-2004 (gupsh01)
**	    Clean up memory allocated for global min/max adf data.
**  18-Jan-2005 (fanra01)
**      Add re-initialization of global pointers to prevent multiple attempts
**      to free memory.
**	16-Jun-2005 (gupsh01)
**	    Moved adg_clean_unimap to adgstartup.c.
*/

DB_STATUS
adg_shutdown()
{
    STATUS stat = OK;
	adg_startup_instances--;
    if (Adf_globs != NULL  &&  Adf_globs->Adi_inited)
	Adf_globs->Adi_inited = adg_startup_instances;

    if (Adf_globs != NULL && Adf_globs->Adu_maptbl != NULL)
    {
	stat = adu_deletemap();
        MEfree (Adf_globs->Adu_maptbl);
    }

    /* clean up memory allocated for max min global data */
    if (Chr_min) 
    {
        MEfree ((char *)Chr_min);
        Chr_min = NULL;
    }
    if (Chr_max)
    {
        MEfree ((char *)Chr_max);
        Chr_max = NULL;
    }
    if (Cha_min)
    {
        MEfree ((char *)Cha_min);
        Cha_min = NULL;
    }
    if (Cha_max)
    {
        MEfree ((char *)Cha_max);
        Cha_max = NULL;
    }
    if (Txt_max)
    {
        MEfree ((char *)Txt_max);
        Txt_max = NULL;
    }
    if (Vch_min)
    {
        MEfree ((char *)Vch_min);
        Vch_min = NULL;
    }
    if (Vch_max)
    {
        MEfree ((char *)Vch_max);
        Vch_max = NULL;
    }
    if (Lke_min)
    {
        MEfree ((char *)Lke_min);
        Lke_min = NULL;
    }
    if (Lke_max)
    {
        MEfree ((char *)Lke_max);
        Lke_max = NULL;
    }
    if (Bit_min)
    {
        MEfree ((char *)Bit_min);
        Bit_min = NULL;
    }
    if (Bit_max)
    {
        MEfree ((char *)Bit_max);
        Bit_max = NULL;
    }
    return(E_DB_OK);
}


/*{
** Name: adg_clean_unimap() 	- Cleanup memory allocated by ADF
**				  for unicode mapping files.
**
** Description:
**	This function is the external ADF call "adg_cleanunimap()"
**	This is used by libq presently to release the mapping files
**	initiated if unicode coercion was required.
**
** Inputs:
**	none
**
** Outputs:
**	none	Memory allocated to Adf_globs->Adu_maptbl is released
**
**	Returns:
**	    E_DB_OK			Operation succeeded.
**
** History:
**	03-Feb-2005 (gupsh01)
**	    Added
*/

DB_STATUS
adg_clean_unimap()
{
    STATUS stat = OK;

    if (Adf_globs != NULL && Adf_globs->Adu_maptbl != NULL)
    {
	stat = adu_deletemap();
        MEfree (Adf_globs->Adu_maptbl);
        Adf_globs->Adu_maptbl = NULL;
    }
    return(E_DB_OK);
}
