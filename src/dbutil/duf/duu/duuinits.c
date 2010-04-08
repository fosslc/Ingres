/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <pc.h>
#include    <me.h>
#include    <gl.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <duf.h>
#include    <er.h>
#include    <duerr.h>

#include    <cs.h>		/* required by lk.h */
#include    <lk.h>
#include    <dudbms.h>
#include    <duenv.h>

#include    <st.h>
#include    <lo.h>
#include    <me.h>
#include    <nm.h>

/**
**
**  Name: DUUINITS.C -	routines to initialize structures that are common
**			to more than one system utility.
**
**  Description:
**        This file contains the routines that are necessary to initialize
**	system utility structures that are used by more than one system
**	utility.
**
**          du_reset_err()	- reset the system utility error-handling
**				  control block.
**	    du_envinit()	- initialize the system utility environment
**				  struct.
[@func_list@]...
**
**
**  History:    
**      11-Sep-86 (ericj)
**          Initial creation.
**	15-Apr-87 (ericj)
**	    Updated du_envinit to initialize newly added status field.
**      25-apr-89 (teg)
**	    Fixed bug 4994 (once a system error message is encounteres, it
**	    repeats for all susequent messages.)
**	5-may-90 (pete)
**	    Add new routine: duu_xflags() for callers of Utexe().
**	27-aug-90 (pete)
**	    Add new routine: duu_chkenv() to check environment.
**	    Called at startup by Createdb and Sysmod.
**	7-oct-90 (teresa)
**	    rewrote du_envinit().
**	23-Oct-1990 (jonb)
**	    Integrate ingres63 change 210308 (ericj):
**	    Null-terminate the du_ddbname field in du_envinit(); bug 33626.
**	24-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up PID definition.
**	8-aug-93 (ed)
**	    unnest dbms.h
**	24-jan-1994 (gmanning)
**	    Add #include <me.h>.
**      31-jan-94 (mikem)
**          Added include of <cs.h> before <lk.h>.  <lk.h> now references
**          a CS_SID type.
[@history_template@]...
**/

/*
[@forward_type_references@]
[@forward_function_references@]
[@#defines_of_other_constants@]
[@type_definitions@]
[@global_variable_definitions@]
[@static_variable_or_function_definitions@]
*/


/*{
** Name: du_envinit() -	    initialize the system utility environment struct.
**
** Description:
**        This routine is used to initialize the system utility environment
**	struct.  It MEfills the structure with ZEROS, which is the equivalent
**	of setting values to 0, flags to FALSE and strings to EOS ('\0')
**
**	NOTE: if future development requires an element to be initialized to
**	      any value other than 0,FALSE or EOS, then please document that in
**	      this description section.
**
** Inputs:
**      du_env                          Ptr to DU_ENV struct to be initialized.
**
** Outputs:
**      *du_env                         The initialized environment structure.
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      11-Sep-86 (ericj)
**          Initial creation.
**	15-Apr-87 (ericj)
**	    Updated du_envinit to initialize newly added status field.
**	7-oct-90 (teresa)
**	    rewrote to MEfill with zero instead of initializing each element.
**	    That old approach was faulty because when folks did new development,
**	    they often missed initializing the DU_ENV variable here.
[@history_template@]...
*/
VOID
du_envinit(du_env)
DU_ENV		    *du_env;
{
    MEfill( (u_i2) sizeof(DU_ENV), 0, (PTR) du_env);
    return;
}

/*{
** Name: du_reset_err()	-   reset the system utility error-handling control
**			    block.
**
** Description:
**        This routine is used to reset the system utility error-handling
**	control block.  After this routine is called, system utilities
**	will not know that an error, or warning has ever occurred.
**
** Inputs:
**      du_errcb                        Ptr to DU_ERROR struct to reset.
**
** Outputs:
**      *du_errcb                       The reset error-handling control
**					block.
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      11-Sep-86 (ericj)
**          Initial creation.
**      23-jan-1989 (roger)
**          It is illegal to peer inside the CL error descriptor.
**      25-apr-89 (teg)
**	    Fixed bug 4994 (once a system error message is encounteres, it
**	    repeats for all susequent messages.)
*/
VOID
du_reset_err(du_errcb)
DU_ERROR	    *du_errcb;
{
    /* Reset the system error control block */
    MEfill( (u_i2) sizeof(DU_ERROR), 0, (PTR) du_errcb);
    du_errcb->du_ingerr	    = DU_NONEXIST_ERROR;
    du_errcb->du_utilerr    = DU_NONEXIST_ERROR;

    return;
}

/*{
**  Name: duu_Xflag - Return value of the -X flags for GCA interface.
**
**  Description:
**	This routine is a copy of IIxflag(), which is in the front-end "ui"
**	library (DU_ executables don't link with "ui"). DU code that
**	need to call UTexe to build the front-end catalogs will call this.
**
**  Inputs:
**	Database name
**
**  Outputs:
**      Returns:
**          Pointer to static buffer with the -X flags as returned by IIx_flag.
**      Errors:
**          None
**
**  Side Effects:
**
**  History:
**      5-may-1990 (pete) - Initial version  (code cloned from IIxflag in 'ui')
*/
char *
duu_xflag(dbase)
char *dbase;
{
	static char x_flag_buf[MAX_LOC] ZERO_FILL;

	_VOID_ IIx_flag(dbase, x_flag_buf);
	return x_flag_buf;
}

/*{
** Name:    duu_chkenv() -        Check Environment Attributes.
**
** Description:
**      This routine is used to check attributes of a user environment.
**      It is called with a flag (or mask of flags) and returns TRUE
**      if all of the flags are TRUE.
**	(Currently it only checks if the current process can create
**	temporary files).
**
** Support flag masks:
**      DU_ENV_TWRITE     - writable ING_TEMP area.  This is used to
**                        check early to see if the user environment
**                        has a writable temp area so that programs
**                        make users fix it before going too far.
**
** Inputs:
**      mask            mask of flags in a i4  (ORed together).  ALL
**                      flags must be TRUE to pass.
**
** Returns:
**      {STATUS} OK if ALL flags are true.
**              FAIL otherwise.
**
** History:
**      27-aug-1990 (pete) Initial Version. Cloned from utils!ug!fechkenv.c
**		in the front-end code line.
*/

DU_STATUS
duu_chkenv (flags)
i4      flags;
{
    if ((flags & DU_ENV_TWRITE) != 0)
    {
        LOCATION        tloc;   /* Location to put temp file */
        LOINFORMATION   loinfo;
        i4              lflags = LO_I_PERMS;

        /* NMloc is strange, it doesn't take a buffer arg. */
        NMloc(TEMP, PATH, NULL, &tloc);

        if (LOinfo(&tloc, &lflags, &loinfo) != OK
          || (loinfo.li_perms & LO_P_WRITE) == 0)
        {
                return E_DU_UERROR;
        }
    }
    return E_DU_OK;
}

