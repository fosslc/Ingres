/*
**Copyright (c) 2004, 2010 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <pc.h>
#include    <tr.h>
#include    <me.h>
#include    <di.h>
#include    <sl.h>
#include    <lo.h>
#include    <cv.h>
#include    <nm.h>
#include    <si.h>
#include    <ck.h>
#include    <st.h>
#include    <id.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <lk.h>
#include    <lg.h>
#include    <scf.h>
#include    <adf.h>
#include    <ulf.h>
#include    <dmf.h>
#include    <sxf.h>
#include    <dm.h>
#include    <dmp.h>
#include    <dmpp.h>
#include    <dm1b.h>
#include    <dm0l.h>
#include    <dm0p.h>
#include    <dm0m.h>
#include    <dmpl.h>
#include    <dm1p.h>
#include    <dm1c.h>
#include    <dm1u.h>
#include    <dm2f.h>
#include    <dmckp.h>
#include    <dm0c.h>
#include    <dmfjsp.h>
#include    <dmftrace.h>

/**
**
**  Name: DMCKP.C - Checkpoint and Restore Routines.
**
**  Description:
**	The file contains all the routines need to
**	    - Checkpoint and Restore databases, locations, files
**	    - Create/destroy directories and files associated with
**	      the above operations.
**
**  History:
**	14-sep-1994 (andyw)
**	    Partial Backup/Recovery Project:
**		Created.
**	06-jan-1995 (shero03)
**	    Added Checkpoint support
**      18-jan-1995 (stial01)
**         dmckp_subst(): need to call dmckp_determine_cktmpl() before 
**         each open of cktmpl.def. (alternatively we could copy the name
**         into a new field in the dmckp structure).
**      06-apr-1995 (stial01)
**         BUG 67385: set err_code if problem doing command substitution.
**      12-jun-1995 (canor01)
**          semaphore protect memory allocation (MCT)
**      12-jun-1995 (canor01)
**          semaphore protect NMloc()'s static buffer
**	12-jul-1995 (canor01)
**	    replace slash with backslash for DESKTOP
**      24-july-1995 (thaju02)
**         bug #52556 - Modified dmckp_begin_restore_tbls_at_loc.
**         table level rollforward was always prompting
**         for tape device 0 to be mounted.  Modified such that
**         dmckp_file_num field of the DMCKP_CB specifies the tape
**         device count to be prompted for.
**	16-may-1996 (hanch04)
**	   added dmckp_spawnnow, pre_work
**      03-jun-1996 (canor01)
**          New ME for operating-system threads does not need external
**          semaphores. Removed ME_stream_sem.
**	    Remove NMloc() semaphore.
**      30-jan-1997 (hanch04)
**              Added dmckp_dev_cnt to be initialized
**	23-june-1998(angusm)
**	    bug 82736: check II_CKTMPL_FILE is owned by user 'ingres'.
**	27-aug-1998 (hanch04)
**	    must allocate memory used for LOgetowner call.
**      21-jan-1999 (hanch04)
**          replace nat and longnat with i4
**      19-feb-1999 (hanch04)
**	    save and restore of raw files
**	30-mar-1999 (hanch04)
**	    Added support for raw locations
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	18-Jan-2001 (jenjo02)
**	    Subvert calls to dma_write_audit() if not C2SECURE.
**      20-Sep-2001 (hweho01)
**          Call NMgtIngAt() to get the value of II_CKTMPL_FILE instead      
**          of NMgtAt();  ensure the user to use ingsetenv/ingunset
**          to change the ckpdb template file, and the local environment 
**          variable setting would not have any effect. 
**      14-Nov-2001 (hweho01)
**          Backed out the change on 20-Sep-2001.
**          For getting the value of II_CKTMPL_FILE, NMgtAt() should be
**          called, since it can be used by a particular user for testing
**          purpose. Use NMgtIngAt() to get the value of II_CKTMPL_CPIO
**          to ensure the environment variable only can be set by 
**          ingsetenv/ingunset.
**	18-Jul-2002 (hanch04)
**	    Fix bad cross integration.
**	14-May-2002 (hanje04)
**	    In dmckp_audit, cast NULL with (PTR) when calling dma_write_audit1
**	    to stop dbname in this function not being properly nulled on 
**	    Itanium Linux and causing a SEGV.
**      08-Aug-2002 (hweho01)
**          Use NMgtAt() to get the value of II_CKTMPL_FILE.
**	22-Apr-2004 (bonro01)
**	    Allow Ingres to be installed as a user other than 'ingres'
**	08-Sep-2004 (hanje04)
**	    BUG 112987
**	    Re-add code to check ownership of files specified by II_CKTMPL_FILE,
**	    previously removed by change 468275 to allow ingres to be installed
**	    as a user other than ingres.
**	    This time check that the file POINTED to by II_CKTMPL_FILE is 
**	    owned by the same user as the cktmplt.def
**    29-Mar-2005 (hanje04)
**       BUG 112987
**       Checkpoint template should be owned by EUID of server as
**       ownership of ckptmpl.def can be "changed" with missuse of II_CONFIG
**    15-Apr-2005 (hanje04)
**        BUG 112987
**        Use IDename instead of IDgeteuser to get effective user of process.
**	08-Dec-2008 (jonj)
**	    SIR 120874: dma_? auditing functions converted to DB_ERROR *
**	10-Dec-2008 (jonj)
**	    SIR 120874: Remove last vestiges of CL_SYS_ERR,
**	    old form uleFormat.
**     16-Feb-2010 (hanje04)
**         SIR 123296
**         Add LSB option, writable files are stored under ADMIN, logs under
**         LOG and read-only under FILES location.
**/



/*
**  Forward and/or External function references.
*/

static DB_STATUS dmckp_subst(
		    u_i4	op_type,
/* constants moved to generic!back!dmf!hdr!dmckp.h */
		    u_i4	direction,
/* constants moved to generic!back!dmf!hdr!dmckp.h */
		    u_i4	granularity,
/* constants moved to generic!back!dmf!hdr!dmckp.h */
		    DMCKP_CB	*dmckp,
		    char	*cline,
		    u_i4	cline_length,
		    i4	*err_code );

static VOID dmckp_lfill(
		    char    	**cp,
		    i4      	len,
		    char    	*str);

static DB_STATUS dmckp_determine_cktmpl(
		    DMCKP_CB	*dmckp,
		    u_i4	direction, 
		    i4	*err_code );

static DB_STATUS dmckp_spawn(
		    char	*command );
static DB_STATUS dmckp_spawnnow(
    		    char        *command,
    		    PID         *pid );


/*{
** Name: dmckp_subst - Build a command line from templates, requirements and
**		       the DMCKP_CB.
**
** Description:
**	This utility gets an operation from the rfp process and tries
**	to read the cktmpl.def definition file and try to understand
**	how this file is defined. When a match is made in the operation
**	a further process of inserting the required values from the
**	dmckp structure into a temporary command line
**	This is then spawned as a seperate process through CL.
**
** Inputs:
**	op_type			One of the following
**				    DMCKP_OP_BEGIN - Begin operation
**				    DMCKP_OP_END   - End operation
**				    DMCKP_OP_WORK  - Perform operation.
**	direction	 	One of the following
**				    DMCKP_SAVE	   - checkpoint
**				    DMCKP_RESTORE  - Rollforward.
**				    DMCKP_JOURNAL  - Journal file processing
**				    DMCKP_DELETE   - Delete processing.
**				    DMCKP_DUMP     - Dump processing.
**	Granularity		One of the following
**				    DMCKP_GRAN_DB  - Whole database
**				    DMCKP_GRAN_TBL - Table level.
**	dmckp			Pointer to a DMCKP_CB holding all the
**				information needed to build command
**				line.
**	cline_length		Length of cline.
**
**
** Outputs:
**	cline			Space for generated command.
**      err_code                Pointer to an area to return error code. May
**			        be set to one of the
**
**	Returns:
**
**	    E_DB_OK
**          E_DB_ERROR
**
**	Exceptions:
**	    none
**
** Side Effects:
**
** History:
**	14-sep-1994 (andyw)
**	    Partial Backup/Recovery Project:
**		Created.
**	16-nov-1994 (andyw)
**	    Changed position of temp from the command read from the
**	    checkpoint definition file cktmpl.def by one.
**	27-dec-1994 (shero03)
**	    Detect when an incorrect .def file is used
**	    Fix table name length in message
**      18-jan-1995 (stial01) call dmckp_determine_cktmpl() before opening
**         cktmpl.def file.
**	11-feb-1999 (hanch04)
**	    added raw type for cooked or raw data files.
**	22-Mar-2001 (jenjo02)
**	    Added special handling of RAW device link names.
*/
static DB_STATUS
dmckp_subst(
    u_i4	op_type,
    u_i4	direction,
    u_i4	granularity,
    DMCKP_CB	*dmckp,
    char	*cline,
    u_i4	cline_length,
    i4	*err_code )
{
    STATUS	status;
    FILE	*fp;
    char	*command;
    char	op, dir, dev, dev_num, gran, raw;
    char	*temp = 0;
    char	*cp;
    DMCKP_CB	*d = dmckp;
    bool	found = FALSE;


    /*
    ** Setup the charaters for operation, direction and type. Put this
    ** here so can change as needed.
    */
    switch( op_type )
    {
    case DMCKP_OP_BEGIN:
	op = 'B';
	break;

    case DMCKP_OP_END:
	op = 'E';
	break;

    case DMCKP_OP_PRE_WORK:
	op = 'P';
	break;

    case DMCKP_OP_WORK:
	op = 'W';
	break;

    case DMCKP_OP_END_TBL_WORK:
	op = 'F'; /* Finish */
	break;

    case DMCKP_OP_BEGIN_TBL_WORK:
	op = 'I';  /* Initialise */
	break;

    default:
	/*
	** FIXME, need an error
	*/
	return( E_DB_ERROR );
    }

    switch( direction )
    {
    case DMCKP_RESTORE:
	dir = 'R';
	break;

    case DMCKP_SAVE:
	dir = 'S';
	break;

    case DMCKP_JOURNAL:
	dir = 'J';
	break;

    case DMCKP_DELETE:
	dir = 'D';
	break;

    case DMCKP_CHECK_SNAPSHOT:
	dir = 'C';
	break;

    case DMCKP_DUMP:
	dir = 'U';
	break;

    default:
	/*
	** FIXME, need an error
	*/
	return( E_DB_ERROR );

    }

    switch ( granularity )
    {
    case DMCKP_GRAN_DB:
	gran     = 'D';
	break;

    case DMCKP_GRAN_TBL:
	gran     = 'T';
	break;

    case DMCKP_GRAN_ALL:
	gran     = 'A';
	break;

    case DMCKP_RAW_DEVICE:
	gran     = 'R';
	break;

    default:
	/*
	** FIXME, need an error
	*/
	return( E_DB_ERROR );
    }

    switch ( d->dmckp_device_type )
    {
    case DMCKP_DISK_FILE:
	dev     = 'D';
	dev_num = '1';
	break;

    case DMCKP_TAPE_FILE:
	dev     = 'T';
	dev_num = '0';
	break;

    default:
	/*
	** FIXME, need an error
	*/
	return( E_DB_ERROR );
    }

    do
    {
	/*
	** Allocate space to read commands into
	*/
	command = MEreqmem(0, cline_length, TRUE, &status);
	if ( status != OK )
	    break;

	/*
	** Determine the checkpoint file exists
	** We need to init dmckp_cktmpl_loc again because filename 
	** may have been overwritten with 'errlog.log'
	*/
	status = dmckp_determine_cktmpl(d, direction, err_code);
	if (status != OK)
	{
	    break;
	}

	/*
	** Open the template file
	*/
	status = SIopen( &dmckp->dmckp_cktmpl_loc, "r", &fp);
	if ( status != OK )
	    break;

	/*
	** Scan the template file looking for command, skip blank lines
	** and comments
	*/

	while ((status = SIgetrec(command, cline_length, fp)) == OK)
	{
	    if (*command == '\n')
	    {
		continue;
	    }

	    if (( granularity & DMCKP_GRAN_DB) && (d->dmckp_use_65_tmpl) )
	    {
		if (command[0] == op
		    && (command[1] == dir   || command[1] == 'E')
		    && (command[2] == dev   || command[2] == 'E')
		    && (command[3] == ':') )
	        {
		    temp  = command + 5;
		    found = TRUE;
		    break;
	        }
	    }
	    else
	    {
		if (command[0] == op
		    && (command[1] == dir   || command[1] == 'E')
		    && (command[2] == dev   || command[2] == 'E')
		    && (command[3] == gran  || command[3] == 'E')
		    && (command[4] == ':') )
	        {
		    temp  = command + 6;
		    found = TRUE;
		    break;
	        }
	    }

	}


	status = SIclose(fp);

	/*
	** If the line has not be found it may be ok to skip a command
	** so return E_DB_INFO and let the above layers decide
	*/
	if (!found)
	    return ( E_DB_INFO );

	/*
	** Found the command line so do the subsitution
	*/
	cp   = cline;

	while( *temp != '\n' )
	{
	    if (*temp == '%')
	    {
		/*
		** Substitution required
		*/
		switch (*++temp)
                {
                case '%':
                    *cp++ = '%';
                    break;

                case 'A':
                    dmckp_lfill(&cp, d->dmckp_ckp_l_path, d->dmckp_ckp_path);
# ifdef UNIX
                    *cp++ = '/';
# endif
# ifdef DESKTOP
                    *cp++ = '\\';
# endif
		    dmckp_lfill(&cp, d->dmckp_ckp_l_file, d->dmckp_ckp_file);
                    break;

		case 'B':    /* DI filename */
		    if ( d->dmckp_raw_blocks )
			dmckp_lfill(&cp, sizeof(DM2F_RAW_LINKNAME)-1, DM2F_RAW_LINKNAME);
		    else
			dmckp_lfill(&cp, d->dmckp_di_l_file, d->dmckp_di_file);
		    break;

                case 'C':
                    dmckp_lfill(&cp, d->dmckp_ckp_l_path, d->dmckp_ckp_path);
                    break;

                case 'D':
                    dmckp_lfill(&cp, d->dmckp_di_l_path, d->dmckp_di_path);
                    break;

	        case 'E':
		    dmckp_lfill(&cp, d->dmckp_di_l_path, d->dmckp_di_path);
# ifdef UNIX
                    *cp++ = '/';
# endif
# ifdef DESKTOP
                    *cp++ = '\\';
# endif
		    if ( d->dmckp_raw_blocks )
			dmckp_lfill(&cp, sizeof(DM2F_RAW_LINKNAME)-1, DM2F_RAW_LINKNAME);
		    else
			dmckp_lfill(&cp, d->dmckp_di_l_file, d->dmckp_di_file);
		    break;

                case 'F':
                    dmckp_lfill(&cp, d->dmckp_ckp_l_file, d->dmckp_ckp_file);
                    break;

                case 'G':
                    CVna(d->dmckp_num_files, cp);
                    while (*cp)
                        cp++;
                    break;

                case 'H':
                    CVna(d->dmckp_num_files_at_loc, cp);
                    while (*cp)
                        cp++;
                    break;

		case 'I':
		    dmckp_lfill(&cp, d->dmckp_l_user_string,d->dmckp_user_string);
		    break;

		case 'J':
		    switch (*++temp)
                    {
		    case '0':
			if ( d->dmckp_jnl_phase == DMCKP_JNL_DO )
			    dmckp_lfill( &cp, sizeof("Do") -1 , "Do");
			else
			    dmckp_lfill( &cp, sizeof("Undo") -1 , "Undo");
			break;

		    case '1':
			CVna(d->dmckp_first_jnl_file, cp);
			while (*cp)
			   cp++;
			break;

		    case '2' :
			CVna(d->dmckp_last_jnl_file, cp);
			while (*cp)
			   cp++;
			break;

		    case '3':
                        dmckp_lfill(&cp, d->dmckp_jnl_l_path, d->dmckp_jnl_path);
                        break;

		    case '4':
                        dmckp_lfill(&cp, d->dmckp_jnl_l_file, d->dmckp_jnl_file);
                        break;

                    default:
			*cp++ = '%';
			*cp++ = *temp;
                    }
		    break;

                case 'K':
                    CVna(d->dmckp_file_num, cp);
                    while (*cp)
                        cp++;
                    break;

                case 'N':
                    CVna(d->dmckp_num_locs, cp);
                    while (*cp)
                        cp++;
                    break;

                case 'P':
                    CVna(d->dmckp_raw_blocks, cp);
                    while (*cp)
                        cp++;
                    break;

                case 'M':
                    CVna(d->dmckp_loc_number, cp);
                    while (*cp)
                        cp++;
                    break;

                case 'S':
                    CVna(d->dmckp_raw_start, cp);
                    while (*cp)
                        cp++;
                    break;

                case 'Q':
                    CVna(DM2F_RAW_BLKSIZE, cp);
                    while (*cp)
                        cp++;
                    break;

                case 'T':
                    *cp++ = dev_num;
                    break;

		case 'U':
		    switch (*++temp)
                    {

		    case '0':
			CVna(d->dmckp_first_dmp_file, cp);
			while (*cp)
			   cp++;
			break;

		    case '1' :
			CVna(d->dmckp_last_dmp_file, cp);
			while (*cp)
			   cp++;
			break;

		    case '2':
                        dmckp_lfill(&cp, d->dmckp_dmp_l_path, d->dmckp_dmp_path);
                        break;

		    case '3':
                        dmckp_lfill(&cp, d->dmckp_dmp_l_file, d->dmckp_dmp_file);
                        break;

                    default:
			*cp++ = '%';
			*cp++ = *temp;
                    }
		    break;

                case 'X':
                    dmckp_lfill(&cp, d->dmckp_l_tab_name,
		    		d->dmckp_tab_name);
                    break;

                default:
                    *cp++ = '%';
                    *cp++ = *temp;
                }

		++temp;
	    }
	    else
	    {
		/*
		** Blindly copy the char
	        */
		*cp++ = *temp++;
	    }

	}

	*cp = EOS;

    } while (FALSE);


    return( E_DB_OK );
}

/*{
** Name: dmckp_determine_cktmpl	- Determine path to find the cktmpl file.
**
** Description:
**
** Inputs:
**	dmckp			Pointer to a DMCKP_CB holding all the
**				information needed to build command
**				line.
**	cline_length		Length of cline.
**
**
** Outputs:
**	dmckp			Pointer to a DMCKP_CB, the path to the
**				template file is returned in this
**				structure.
**      err_code                Pointer to an area to return error code. May
**			        be set to one of the
**
**	Returns:
**
** History:
**	14-sep-1994 (andyw)
**	    Partial Backup/Recovery Project:
**		Created.
*/
static DB_STATUS
dmckp_determine_cktmpl(
    DMCKP_CB	*dmckp,
    u_i4	direction,
    i4	*err_code )
{

    char	*s;
    STATUS	status = OK;
    FILE	*fp;
    char	username[DB_MAXNAME+1];
    char	instowner[DB_MAXNAME+1];
    char	*cp = &username[0];
    char	*def_cp = &instowner[0];

    /*
    ** Determine the location of the checkpoint template file.
    ** FIXME, this needs to be a PM thing
    */
    NMgtAt("II_CKTMPL_FILE", &s);

    if (s == NULL || *s == EOS)
    {
        status = NMloc(FILES, FILENAME, "cktmpl.def", 
			&dmckp->dmckp_cktmpl_loc);
    }
    else
    {
        status = LOfroms( PATH & FILENAME, s, &dmckp->dmckp_cktmpl_loc );
    }

    if ( status != OK )
    {
  	/*
	** FIXME, need an error message
	*/
	uleFormat(NULL, status, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, (char *)NULL, 0L,
		    (i4 *)NULL, err_code, 0);

	return( E_DB_ERROR );
    }

    /*
    ** Check that the template files exists
    */
    status = SIopen( &dmckp->dmckp_cktmpl_loc, "r", &fp);
    if ( status != OK )
    {
	uleFormat(NULL, status, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, NULL, 0, NULL,
	            err_code, 0);
        uleFormat(NULL, E_DM1155_CPP_TMPL_MISSING, (CL_ERR_DESC *)NULL,
                    ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
        *err_code = E_DM1155_CPP_TMPL_MISSING;

	return( E_DB_ERROR );
    }

    status = SIclose(fp);
    if ( status != OK )
    {
	uleFormat(NULL, status, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, NULL, 0, NULL,
	            err_code, 0);
	return( E_DB_ERROR );
    }

#ifdef CK_OWNNOTING
   /*
   ** check ownership matches that of the effective user ID
   **
   */
   (VOID)LOgetowner(&dmckp->dmckp_cktmpl_loc, &cp);
   (VOID)IDename(&def_cp);
   if ((STcompare(cp, def_cp) != 0)
# ifdef conf_LSB_BUILD
	&& (STcompare(cp, "root") != 0 ) /* allow root ownership too */
# endif
	)
   {
        if (direction == DMCKP_SAVE)
      {
            *err_code = I_DM116D_CPP_NOT_DEF_TEMPLATE;
          return(E_DB_ERROR);
      }
      else if (direction == DMCKP_RESTORE)
      {
            *err_code = I_DM1369_RFP_NOT_DEF_TEMPLATE;
          return(E_DB_ERROR);
      }
   }
#endif



    /*
    ** Backward compatibility determine if want to use the 6.5 style
    ** template files.
    ** FIXME, need to add a PM variable to do this.
    */
    dmckp->dmckp_use_65_tmpl = FALSE;

    return( E_DB_OK );
}


/*{
** Name: dmckp_lfill - fill in a substitute string
**
** Description:
**	This function simply copies a string
**      usually spans more than one line.
**
** Inputs:
**	cp				pointer to destination space
**					(also an output)
**	len				length of string to substitute
**	str				string to substitute
**
** Outputs:
**      cp				string pointer changed to point
**					to end of filled area.
**
**	Returns:
**	    none
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	14-sep-1994 (andyw)
**	    Partial Backup/Recovery Project:
**		Created.
*/
static
VOID dmckp_lfill(
    char	**cp,
    i4		len,
    char	*str)
{
    while (len--)
    {
	*(*cp)++ = *str++;
    }
}


/*{
** Name: dmckp_init - Initialise the DMCKP_CB structure that is passed
**		      in.
**
** Description:
**	This routine initialises a DMCKP_CB structure.
** Inputs:
**	dmckp			Pointer to the DMCKP_CB.
**
** Outputs:
**	err_code		Pointer to variable to return error.
**
**	Returns:
**	E_DB_OK
**	E_DB_ERROR
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	14-sep-1994 (andyw)
**	    Partial Backup/Recovery Project:
**		Created.
**	30-jan-1997 (hanch04)
**		Added dmckp_dev_cnt
**	23-June-1998(angusm)
**	        Add 'direction' to proto.
*/
DB_STATUS
dmckp_init(
    DMCKP_CB	*dmckp,
    u_i4	direction,
    i4	*err_code )
{
    DMCKP_CB	*d = dmckp;
    DB_STATUS	status;
    i4	temp = sizeof("Undefined")-1;

    d->dmckp_op_type	     = 0;
    d->dmckp_tab_name        = "Undefined";
    d->dmckp_l_tab_name      = temp;
    d->dmckp_ckp_path	     = "Undefined";
    d->dmckp_ckp_l_path	     = temp;
    d->dmckp_ckp_file	     = "Undefined";
    d->dmckp_ckp_l_file	     = temp;
    d->dmckp_di_path         = "Undefined";
    d->dmckp_di_l_path       = temp;
    d->dmckp_di_file         = "Undefined";
    d->dmckp_di_l_file       = temp;
    d->dmckp_jnl_path	     = "Undefined";
    d->dmckp_jnl_l_path	     = temp;
    d->dmckp_jnl_file	     = "Undefined";
    d->dmckp_jnl_l_file	     = temp;
    d->dmckp_dmp_path	     = "Undefined";
    d->dmckp_dmp_l_path	     = temp;
    d->dmckp_dmp_file	     = "Undefined";
    d->dmckp_dmp_l_file	     = temp;
    d->dmckp_device_type	     = 0;
    d->dmckp_num_locs         = 0;
    d->dmckp_loc_number       = 0;
    d->dmckp_num_files        = 0;
    d->dmckp_num_files_at_loc = 0;
    d->dmckp_file_num	      = 0;
    d->dmckp_first_jnl_file   = 0;
    d->dmckp_last_jnl_file    = 0;
    d->dmckp_jnl_phase	      = 0;
    d->dmckp_raw_start	      = 0;
    d->dmckp_raw_blocks       = 0;
    d->dmckp_raw_total_blocks = 0;
    d->dmckp_user_string      = "Undefined";
    d->dmckp_l_user_string    = temp;
    d->dmckp_dev_cnt          = 1;

    /*
    ** Setup the checkpoint template file
    */
    status = dmckp_determine_cktmpl( d, direction, err_code );

    return( status );

}

/*{
** Name: dmckp_pre_restore_loc - Do pre work restore a database location.
**
**
** Description:
**	This routine restores the specified  database location from
**	the specified checkpoint. All information is held in the
**	DMCKP_CB.
**
** Inputs:
**	dmckp			Pointer to the DMCKP_CB.
**
** Outputs:
**	err_code		Pointer to variable to return error.
**
**	Returns:
**	E_DB_OK
**	E_DB_ERROR
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	01-jun-1996 (hanch04)
**	    Parallel Backup/Recovery Project:
**		Created.
**	23-june-1998(angusm)
**	   Audit all Os commands issued (SIR 86561)
*/
DB_STATUS
dmckp_pre_restore_loc(
    DMCKP_CB	*dmckp,
    DMP_DCB     *dcb,
    i4	*err_code )
{
    DMCKP_CB		*d = dmckp;
    char		*command = dmckp->dmckp_command;
    DB_STATUS		status;

    /*
    ** Increase count of locations sofar restored
    */
    d->dmckp_loc_number++;

    /*

    */
    status = dmckp_subst( DMCKP_OP_PRE_WORK, DMCKP_RESTORE, DMCKP_GRAN_DB,
		    d, command , MAXCOMMAND, err_code );
    if ( status != E_DB_OK )
    {
	if (status == E_DB_INFO)
	    *err_code = E_DM1363_RFP_TMPL_CMD_MISSING;
	else
	    *err_code = E_DM1362_RFP_TMPL_ERROR;

        return( E_DB_ERROR );
    }

    /*
    ** Issue the command
    */
    status = dmckp_spawn( command );
    if ( status != E_DB_OK )
    {
	*err_code = E_DM1307_RFP_RESTORE;
	return (E_DB_ERROR);
    }

    _VOID_ dmckp_audit(dcb, DMCKP_RESTORE, DMCKP_GRAN_DB,
				status, command, err_code);
    return( E_DB_OK );

}

/*
** Name: dmckp_restore_loc - Restore a database location.
**
**
** Description:
**	This routine restores the specified  database location from
**	the specified checkpoint. All information is held in the
**	DMCKP_CB.
**
** Inputs:
**	dmckp			Pointer to the DMCKP_CB.
**
** Outputs:
**	err_code		Pointer to variable to return error.
**
**	Returns:
**	E_DB_OK
**	E_DB_ERROR
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	14-sep-1994 (andyw)
**	    Partial Backup/Recovery Project:
**		Created.
**	23-june-1998(angusm)
**	   Audit all Os commands issued (SIR 86561)
*/
DB_STATUS
dmckp_restore_loc(
    DMCKP_CB	*dmckp,
    DMP_DCB     *dcb,
    i4	*err_code )
{
    DMCKP_CB		*d = dmckp;
    char		*command = dmckp->dmckp_command;
    DB_STATUS		status;

    /*
    ** Build the underlying o/s command
    */
    if ( d->dmckp_raw_blocks )
	status = dmckp_subst( DMCKP_OP_WORK, DMCKP_RESTORE, DMCKP_RAW_DEVICE,
		d, command , MAXCOMMAND, err_code );
    else
	status = dmckp_subst( DMCKP_OP_WORK, DMCKP_RESTORE, DMCKP_GRAN_DB,
		d, command , MAXCOMMAND, err_code );
    if ( status != E_DB_OK )
    {
	if (status == E_DB_INFO)
	    *err_code = E_DM1363_RFP_TMPL_CMD_MISSING;
	else
	    *err_code = E_DM1362_RFP_TMPL_ERROR;

        return( E_DB_ERROR );
    }

    /*
    ** Issue the command, spawn with no wait if using 
    ** multiple devices.  If only one device, wait.
    */

    if ( d->dmckp_dev_cnt > 1 )
    {
        status = dmckp_spawnnow( command, d->dmckp_ckp_path_pid );
    }
    else
    {
        status = dmckp_spawn( command );
    }
    _VOID_ dmckp_audit(dcb, DMCKP_RESTORE, DMCKP_GRAN_DB,
			status, command, err_code);

    if ( status != E_DB_OK )
    {
	*err_code = E_DM1307_RFP_RESTORE;
	return (E_DB_ERROR);
    }

    return( E_DB_OK );

}

/*{
** Name: dmckp_begin_restore_db - Begin restoring the whole database.
**
**
** Description:
**	This routine is called at the nginning of a restore of the
**	whole database.
**
** Inputs:
**	dmckp			Pointer to the DMCKP_CB.
**
** Outputs:
**	err_code		Pointer to variable to return error.
**
**	Returns:
**	E_DB_OK
**	E_DB_ERROR
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	14-sep-1994 (andyw)
**	    Partial Backup/Recovery project:
**		Created.
**	23-june-1998(angusm)
**	   Audit all Os commands issued (SIR 86561)
*/
DB_STATUS
dmckp_begin_restore_db(
    DMCKP_CB	*dmckp,
    DMP_DCB     *dcb,
    i4	*err_code )
{
    DMCKP_CB		*d = dmckp;
    char		*command = dmckp->dmckp_command;
    DB_STATUS		status;

    /*
    ** Build the underlying o/s command
    */
    status = dmckp_subst( DMCKP_OP_BEGIN, DMCKP_RESTORE, DMCKP_GRAN_DB,
		    d, command, MAXCOMMAND, err_code );
    if ( status != E_DB_OK )
    {
	if (status == E_DB_INFO)
	    *err_code = E_DM1363_RFP_TMPL_CMD_MISSING;
	else
	    *err_code = E_DM1362_RFP_TMPL_ERROR;

        return( E_DB_ERROR );
    }

    /*
    ** Issue the command
    */
    status = dmckp_spawn( command );

    _VOID_ dmckp_audit(dcb, DMCKP_RESTORE, DMCKP_GRAN_DB,
			status, command, err_code);

    if ( status != E_DB_OK )
    {
	*err_code = E_DM1321_RFP_BEGIN_ERROR;
	return (E_DB_ERROR);
    }

    return( E_DB_OK );

}

/*{
** Name: dmckp_spawn - Call the CL to actually issue the command.
**
**
** Description:
**
** Inputs:
** 	command			Command to issue.
**
** Outputs:
**
**	Returns:
**	E_DB_OK
**	E_DB_ERROR
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	14-sep-1994 (andyw)
**	    Partial Backup/Recovery Project:
**		Created.
*/
static
DB_STATUS 
dmckp_spawn(
    char	*command )
{
    STATUS		cl_status;
    CL_ERR_DESC 	sys_err;
    i4		err_code;

    cl_status = CK_spawn(command, &sys_err);
    if ( cl_status != OK )
    {
	uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL, NULL, 0, NULL, &err_code, 0);
	return (E_DB_ERROR);
    }

    return( E_DB_OK );
}

/*{
** Name: dmckp_spawnnow - Call the CL to actually issue the command.
**
**
** Description:
**
** Inputs:
**      command                 Command to issue.
**	pid			Pointer to pid
**
** Outputs:
**
**      Returns:
**      E_DB_OK
**      E_DB_ERROR
**
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      16-may-1996 (hanch04)
**          Parallel Backup/Recovery Project:
**              Created from dmckp_spawn.
*/
static
DB_STATUS 
dmckp_spawnnow(
    char        *command,
    PID		*pid )
{
    STATUS              cl_status;
    CL_ERR_DESC         sys_err;
    i4             err_code;
 
    cl_status = CK_spawnnow(command, pid, &sys_err);
    if ( cl_status != OK )
    {
        uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL, NULL, 0, NULL, &err_code, 0);
        return (E_DB_ERROR);
    }
 
    return( E_DB_OK );
}

/*{
** Name: dmckp_end_restore_db -  End restoring the whole database.
**
**
** Description:
**	This routine is called at the end of a restore of the
**	whole database.
**
** Inputs:
**	dmckp			Pointer to the DMCKP_CB.
**
** Outputs:
**	err_code		Pointer to variable to return error.
**
**	Returns:
**	E_DB_OK
**	E_DB_ERROR
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	14-sep-1994 (andyw)
**	    Partial Backup/Recovery Project:
**		Created.
**	23-june-1998(angusm)
**	   Audit all Os commands issued (SIR 86561)
*/
DB_STATUS
dmckp_end_restore_db(
    DMCKP_CB	*dmckp,
    DMP_DCB     *dcb,
    i4	*err_code )
{
    DMCKP_CB		*d = dmckp;
    char		*command = dmckp->dmckp_command;
    DB_STATUS		status;

    /*
    ** Build the underlying o/s command
    */
    status = dmckp_subst( DMCKP_OP_END, DMCKP_RESTORE, DMCKP_GRAN_DB,
		    d, command, MAXCOMMAND, err_code );

    _VOID_ dmckp_audit(dcb, DMCKP_RESTORE, DMCKP_GRAN_DB,
			status, command, err_code);

    if ( status != E_DB_OK )
    {
	if (status == E_DB_INFO)
	    *err_code = E_DM1363_RFP_TMPL_CMD_MISSING;
	else
	    *err_code = E_DM1362_RFP_TMPL_ERROR;

        return( E_DB_ERROR );
    }

    /*
    ** Issue the command
    */
    status = dmckp_spawn( command );
    if ( status != E_DB_OK )
    {
	*err_code = E_DM1322_RFP_END_ERROR;
	return (E_DB_ERROR);
    }

    return( E_DB_OK );

}

/*{
** Name: dmckp_begin_restore_tbl - Begin restoring some tables
**
**
** Description:
**	This routine is called at the begining of table level restore.
**
** Inputs:
**	dmckp			Pointer to the DMCKP_CB.
**
** Outputs:
**	err_code		Pointer to variable to return error.
**
**	Returns:
**	E_DB_OK
**	E_DB_ERROR
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	14-sep-1994 (andyw)
**	    Partial Backup/Recovery Project:
**		Created.
**	23-june-1998(angusm)
**	   Audit all Os commands issued (SIR 86561)
*/
DB_STATUS
dmckp_begin_restore_tbl(
    DMCKP_CB	*dmckp,
    DMP_DCB     *dcb,
    i4	*err_code )
{
    DMCKP_CB		*d = dmckp;
    char		*command = dmckp->dmckp_command;
    DB_STATUS		status;

    /*
    ** Build the underlying o/s command
    */
    status = dmckp_subst( DMCKP_OP_BEGIN, DMCKP_RESTORE, DMCKP_GRAN_TBL,
		    d, command, MAXCOMMAND, err_code );
    if ( status != E_DB_OK )
    {
	if (status == E_DB_INFO)
	    *err_code = E_DM1363_RFP_TMPL_CMD_MISSING;
	else
	    *err_code = E_DM1362_RFP_TMPL_ERROR;

        return( E_DB_ERROR );
    }

    /*
    ** Issue the command
    */
    status = dmckp_spawn( command );

    _VOID_ dmckp_audit(dcb, DMCKP_RESTORE, DMCKP_GRAN_TBL,
				status, command, err_code);

    if ( status != E_DB_OK )
    {
	*err_code = E_DM1321_RFP_BEGIN_ERROR;
	return (E_DB_ERROR);
    }

    return( E_DB_OK );

}

/*{
** Name: dmckp_pre_restore_file - Pre work for restore a file for a table.
**
**
** Description:
**	This routine restores the specified file at the specified database
**	location from the specfied checkpoint.
**	All information is held in the DMCKP_CB.
**
** Inputs:
**	dmckp			Pointer to the DMCKP_CB.
**
** Outputs:
**	err_code		Pointer to variable to return error.
**
**	Returns:
**	E_DB_OK
**	E_DB_ERROR
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	01-jun-1996 (hanch04)
**	    Parallel Backup/Recovery project:
**		Created.
**	23-june-1998(angusm)
**	   Audit all Os commands issued (SIR 86561)
*/
DB_STATUS
dmckp_pre_restore_file(
    DMCKP_CB	*dmckp,
    DMP_DCB     *dcb,
    i4	*err_code )
{
    DMCKP_CB		*d = dmckp;
    char		*command = dmckp->dmckp_command;
    DB_STATUS		status;

    /*
    ** Increase the count of the number of files recovered at this location
    */
    d->dmckp_file_num++;

    /*
    ** Build the underlying o/s command
    */
    if ( d->dmckp_raw_blocks )
	status = dmckp_subst( DMCKP_OP_PRE_WORK, DMCKP_RESTORE, DMCKP_RAW_DEVICE,
		    d, command , MAXCOMMAND, err_code );
    else
	status = dmckp_subst( DMCKP_OP_PRE_WORK, DMCKP_RESTORE, DMCKP_GRAN_TBL,
		    d, command , MAXCOMMAND, err_code );
    if ( status != E_DB_OK )
    {
	if (status == E_DB_INFO)
	    *err_code = E_DM1363_RFP_TMPL_CMD_MISSING;
	else
	    *err_code = E_DM1362_RFP_TMPL_ERROR;

        return( E_DB_ERROR );
    }

    /*
    ** Issue the command
    */
    status = dmckp_spawn( command );

    _VOID_ dmckp_audit(dcb, DMCKP_RESTORE, DMCKP_GRAN_TBL,
			status, command,err_code);

    if ( status != E_DB_OK )
    {
	*err_code = E_DM1307_RFP_RESTORE;
	return (E_DB_ERROR);
    }

    return( E_DB_OK );

}

/*{
** Name: dmckp_restore_file - Restore a file for a table.
**
**
** Description:
**	This routine restores the specified file at the specified database
**	location from the specfied checkpoint.
**	All information is held in the DMCKP_CB.
**
** Inputs:
**	dmckp			Pointer to the DMCKP_CB.
**
** Outputs:
**	err_code		Pointer to variable to return error.
**
**	Returns:
**	E_DB_OK
**	E_DB_ERROR
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	14-sep-1994 (andyw)
**	    Partial Backup/Recovery project:
**		Created.
**	23-june-1998(angusm)
**	   Audit all Os commands issued (SIR 86561)
*/
DB_STATUS
dmckp_restore_file(
    DMCKP_CB	*dmckp,
    DMP_DCB     *dcb,
    i4	*err_code )
{
    DMCKP_CB		*d = dmckp;
    char		*command = dmckp->dmckp_command;
    DB_STATUS		status;

    /*
    ** Build the underlying o/s command
    */
    if ( d->dmckp_raw_blocks )
	status = dmckp_subst( DMCKP_OP_WORK, DMCKP_RESTORE, DMCKP_RAW_DEVICE,
		    d, command , MAXCOMMAND, err_code );
    else
	status = dmckp_subst( DMCKP_OP_WORK, DMCKP_RESTORE, DMCKP_GRAN_TBL,
		    d, command , MAXCOMMAND, err_code );
    if ( status != E_DB_OK )
    {
	if (status == E_DB_INFO)
	    *err_code = E_DM1363_RFP_TMPL_CMD_MISSING;
	else
	    *err_code = E_DM1362_RFP_TMPL_ERROR;

        return( E_DB_ERROR );
    }

    /*
    ** Issue the command
    */
    if ( d->dmckp_dev_cnt > 1 )
    {
        status = dmckp_spawnnow( command, d->dmckp_ckp_path_pid );
    }
    else
    {
        status = dmckp_spawn( command );
    }

    _VOID_ dmckp_audit(dcb, DMCKP_RESTORE, DMCKP_GRAN_TBL,
			status, command,err_code);

    if ( status != E_DB_OK )
    {
	*err_code = E_DM1307_RFP_RESTORE;
	return (E_DB_ERROR);
    }

    return( E_DB_OK );

}

/*{
** Name: dmckp_begin_restore_tbls_at_loc - Begin restoring tables at location
**
**
** Description:
**	This routineis called at the beginning of restoring tables at a
**	location. All required information is within the DMCKP_CB
**
** Inputs:
**	dmckp			Pointer to the DMCKP_CB.
**
** Outputs:
**	err_code		Pointer to variable to return error.
**
**	Returns:
**	E_DB_OK
**	E_DB_ERROR
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	14-sep-1994 (andyw)
**	    Partial Backup/Recovery project:
**		Created.
**      24-july-1995 (thaju02)
**          bug #52556 - table level rollforward always was prompting
**          for tape device 0 to be mounted.  Modified such that
**          dmckp_file_num field of the DMCKP_CB specifies the tape
**          device count to be prompted for.
**	23-june-1998(angusm)
**	   Audit all Os commands issued (SIR 86561)
*/
DB_STATUS
dmckp_begin_restore_tbls_at_loc(
    DMCKP_CB	*dmckp,
    DMP_DCB     *dcb,
    i4	*err_code )
{
    DMCKP_CB		*d = dmckp;
    char		*command = dmckp->dmckp_command;
    DB_STATUS		status;

    /*
    ** Build the underlying o/s command
    */
    status = dmckp_subst( DMCKP_OP_BEGIN_TBL_WORK, DMCKP_RESTORE, DMCKP_GRAN_TBL,
		    d, command , MAXCOMMAND, err_code );
    if ( status != E_DB_OK )
    {
	if (status == E_DB_INFO)
	    *err_code = E_DM1363_RFP_TMPL_CMD_MISSING;
	else
	    *err_code = E_DM1362_RFP_TMPL_ERROR;

        return( E_DB_ERROR );
    }

    /*
    ** Issue the command
    */
    status = dmckp_spawn( command );

    _VOID_ dmckp_audit(dcb, DMCKP_RESTORE, DMCKP_GRAN_TBL,
			status, command, err_code);

    if ( status != E_DB_OK )
    {
	*err_code = E_DM1307_RFP_RESTORE;
	return (E_DB_ERROR);
    }

    return( E_DB_OK );

}

/*{
** Name: dmckp_end_restore_tbls_at_loc - end restoring tables at location
**
**
** Description:
**	This routine is called at the end of restoring tables at a
**	location. All required information is within the DMCKP_CB
**
** Inputs:
**	dmckp			Pointer to the DMCKP_CB.
**
** Outputs:
**	err_code		Pointer to variable to return error.
**
**	Returns:
**	E_DB_OK
**	E_DB_ERROR
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	14-sep-1994 (andyw)
**	    Partial Backup/Recovery Project:
**		Created.
**	23-june-1998(angusm)
**	   Audit all Os commands issued (SIR 86561)
*/
DB_STATUS
dmckp_end_restore_tbls_at_loc(
    DMCKP_CB	*dmckp,
    DMP_DCB     *dcb,
    i4	*err_code )
{
    DMCKP_CB		*d = dmckp;
    char		*command = dmckp->dmckp_command;
    DB_STATUS		status;


    /*
    ** Build the underlying o/s command
    */
    status = dmckp_subst( DMCKP_OP_END_TBL_WORK, DMCKP_RESTORE, DMCKP_GRAN_TBL,
		   d, command , MAXCOMMAND, err_code );
    if ( status != E_DB_OK )
    {
	if (status == E_DB_INFO)
	    *err_code = E_DM1363_RFP_TMPL_CMD_MISSING;
	else
	    *err_code = E_DM1362_RFP_TMPL_ERROR;

        return( E_DB_ERROR );
    }

    /*
    ** Issue the command
    */
    status = dmckp_spawn( command );

    _VOID_ dmckp_audit(dcb, DMCKP_RESTORE, DMCKP_GRAN_TBL,
			status, command, err_code);

    if ( status != E_DB_OK )
    {
	*err_code = E_DM1307_RFP_RESTORE;
	return (E_DB_ERROR);
    }

    return( E_DB_OK );

}

/*{
** Name: dmckp_end_restore_tbl -  End restoring a set of tables.
**
**
** Description:
**	This routine is called at the end of a restore of a
**	set of tables.
**
** Inputs:
**	dmckp			Pointer to the DMCKP_CB.
**
** Outputs:
**	err_code		Pointer to variable to return error.
**
**	Returns:
**	E_DB_OK
**	E_DB_ERROR
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	14-sep-1994 (andyw)
**	    Partial Backup/Recovery Project:
**		Created.
**	23-june-1998(angusm)
**	   Audit all Os commands issued (SIR 86561)
*/
DB_STATUS
dmckp_end_restore_tbl(
    DMCKP_CB	*dmckp,
    DMP_DCB     *dcb,
    i4	*err_code )
{
    DMCKP_CB		*d = dmckp;
    char		*command = dmckp->dmckp_command;
    DB_STATUS		status;

    /*
    ** Build the underlying o/s command
    */
    status = dmckp_subst( DMCKP_OP_END, DMCKP_RESTORE, DMCKP_GRAN_TBL,
		    d, command, MAXCOMMAND, err_code );
    if ( status != E_DB_OK )
    {
	if (status == E_DB_INFO)
	    *err_code = E_DM1363_RFP_TMPL_CMD_MISSING;
	else
	    *err_code = E_DM1362_RFP_TMPL_ERROR;

        return( E_DB_ERROR );
    }

    /*
    ** Issue the command
    */
    status = dmckp_spawn( command );

    _VOID_ dmckp_audit(dcb, DMCKP_RESTORE, DMCKP_GRAN_TBL,
			status, command, err_code);

    if ( status != E_DB_OK )
    {
	*err_code = E_DM1322_RFP_END_ERROR;
	return (E_DB_ERROR);
    }

    return( E_DB_OK );
}

/*{
** Name: dmckp_begin_journal_phase - Begin journal phase of rollforward
**
**
** Description:
**	This routine is called at the beginning of the journal phase.
**
** Inputs:
**	dmckp			Pointer to the DMCKP_CB.
**
** Outputs:
**	err_code		Pointer to variable to return error.
**
**	Returns:
**	E_DB_OK
**	E_DB_ERROR
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	14-sep-1994 (andyw)
**	    Partial Backup/Recovery Project:
**		Created.
*/
DB_STATUS
dmckp_begin_journal_phase(
    DMCKP_CB	*dmckp,
    i4	*err_code )
{
    DMCKP_CB		*d = dmckp;
    char		*command = dmckp->dmckp_command;
    DB_STATUS		status;

    /*
    ** Build the underlying o/s command
    */
    status = dmckp_subst( DMCKP_OP_BEGIN, DMCKP_JOURNAL, DMCKP_GRAN_ALL,
		    d, command, MAXCOMMAND, err_code );
    if ( status != E_DB_OK )
    {
	if (status == E_DB_INFO)
	    *err_code = E_DM1363_RFP_TMPL_CMD_MISSING;
	else
	    *err_code = E_DM1362_RFP_TMPL_ERROR;

        return( E_DB_ERROR );
    }

    /*
    ** Issue the command
    */
    status = dmckp_spawn( command );
    if ( status != E_DB_OK )
    {
	*err_code = E_DM1321_RFP_BEGIN_ERROR;
	return (E_DB_ERROR);
    }

    return( E_DB_OK );

}

/*{
** Name: dmckp_end_journal_phase - End journal phase of rollforwarddb.
**
**
** Description:
**
** Inputs:
**	dmckp			Pointer to the DMCKP_CB.
**
** Outputs:
**	err_code		Pointer to variable to return error.
**
**	Returns:
**	E_DB_OK
**	E_DB_ERROR
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	14-sep-1994 (andyw)
**	    Partial Backup/Recovery Project:
**		Created.
*/
DB_STATUS
dmckp_end_journal_phase(
    DMCKP_CB	*dmckp,
    i4	*err_code )
{
    DMCKP_CB		*d = dmckp;
    char		*command = dmckp->dmckp_command;
    DB_STATUS		status;

    /*
    ** Build the underlying o/s command
    */
    status = dmckp_subst( DMCKP_OP_END, DMCKP_JOURNAL, DMCKP_GRAN_ALL,
		    d, command, MAXCOMMAND, err_code );
    if ( status != E_DB_OK )
    {
	if (status == E_DB_INFO)
	    *err_code = E_DM1363_RFP_TMPL_CMD_MISSING;
	else
	    *err_code = E_DM1362_RFP_TMPL_ERROR;

        return( E_DB_ERROR );
    }

    /*
    ** Issue the command
    */
    status = dmckp_spawn( command );
    if ( status != E_DB_OK )
    {
	*err_code = E_DM1321_RFP_BEGIN_ERROR;
	return (E_DB_ERROR);
    }

    return( E_DB_OK );

}

/*{
** Name: dmckp_work_journal_phase - Work journal phase of rollforwarddb
**
**
** Description:
**
** Inputs:
**	dmckp			Pointer to the DMCKP_CB.
**
** Outputs:
**	err_code		Pointer to variable to return error.
**
**	Returns:
**	E_DB_OK
**	E_DB_ERROR
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	14-sep-1994 (andyw)
**	    Partial Backup/Recovery Project:
**		Created.
*/
DB_STATUS
dmckp_work_journal_phase(
    DMCKP_CB	*dmckp,
    i4	*err_code )
{
    DMCKP_CB		*d = dmckp;
    char		*command = dmckp->dmckp_command;
    DB_STATUS		status;

    /*
    ** Build the underlying o/s command
    */
    status = dmckp_subst( DMCKP_OP_WORK, DMCKP_JOURNAL, DMCKP_GRAN_ALL,
		   d, command, MAXCOMMAND, err_code );
    if ( status != E_DB_OK )
    {
	if (status == E_DB_INFO)
	    *err_code = E_DM1363_RFP_TMPL_CMD_MISSING;
	else
	    *err_code = E_DM1362_RFP_TMPL_ERROR;

        return( E_DB_ERROR );
    }

    /*
    ** Issue the command
    */
    status = dmckp_spawn( command );
    if ( status != E_DB_OK )
    {
	*err_code = E_DM1321_RFP_BEGIN_ERROR;
	return (E_DB_ERROR);
    }

    return( E_DB_OK );

}

/*{
** Name: dmckp_delete_loc - Delete all files at a location, rollforwarddb.
**
**
** Description:
**
** Inputs:
**	dmckp			Pointer to the DMCKP_CB.
**
** Outputs:
**	err_code		Pointer to variable to return error.
**
**	Returns:
**	E_DB_OK
**	E_DB_ERROR
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	14-sep-1994 (andyw)
**	    Partial Backup/Recovery Project:
**		Created.
*/
DB_STATUS
dmckp_delete_loc(
    DMCKP_CB	*dmckp,
    i4	*err_code )
{
    DMCKP_CB		*d = dmckp;
    char		*command = dmckp->dmckp_command;
    DB_STATUS		status;

    /*
    ** Build the underlying o/s command
    */
    status = dmckp_subst( DMCKP_OP_WORK, DMCKP_DELETE, DMCKP_GRAN_DB,
		    d, command, MAXCOMMAND, err_code );
    if ( status != E_DB_OK )
    {
	if (status == E_DB_INFO)
	    *err_code = E_DM1363_RFP_TMPL_CMD_MISSING;
	else
	    *err_code = E_DM1362_RFP_TMPL_ERROR;

        return( E_DB_ERROR );
    }

    /*
    ** Issue the command
    */
    status = dmckp_spawn( command );
    if ( status != E_DB_OK )
    {
	*err_code = E_DM1321_RFP_BEGIN_ERROR;
	return (E_DB_ERROR);
    }

    return( E_DB_OK );
}

/*{
** Name: dmckp_check_snapshot - Check that the specified snapshot file exists.
**
**
** Description:
**
** Inputs:
**	dmckp			Pointer to the DMCKP_CB.
**
** Outputs:
**	err_code		Pointer to variable to return error.
**
**	Returns:
**	E_DB_OK
**	E_DB_ERROR
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	14-sep-1994 (andyw)
**	    Partial Backup/Recovery Project:
**		Created.
*/
DB_STATUS
dmckp_check_snapshot(
    DMCKP_CB	*dmckp,
    i4	*err_code )
{
    DMCKP_CB		*d = dmckp;
    char		*command = dmckp->dmckp_command;
    DB_STATUS		status;

    /*
    ** Build the underlying o/s command
    */
    status = dmckp_subst( DMCKP_OP_WORK, DMCKP_CHECK_SNAPSHOT, DMCKP_GRAN_ALL,
		    d, command, MAXCOMMAND, err_code );
    if ( status != E_DB_OK )
    {
	if (status == E_DB_INFO)
	    *err_code = E_DM1363_RFP_TMPL_CMD_MISSING;
	else
	    *err_code = E_DM1362_RFP_TMPL_ERROR;

        return( E_DB_ERROR );
    }

    /*
    ** Issue the command
    */
    status = dmckp_spawn( command );
    if ( status != E_DB_OK )
    {
	*err_code = E_DM1321_RFP_BEGIN_ERROR;
	return (E_DB_ERROR);
    }

    return( E_DB_OK );
}

/*{
** Name: dmckp_delete_file - Delete a file at a location, rollforwarddb.
**
**
** Description:
**
** Inputs:
**	dmckp			Pointer to the DMCKP_CB.
**
** Outputs:
**	err_code		Pointer to variable to return error.
**
**	Returns:
**	E_DB_OK
**	E_DB_ERROR
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	14-sep-1994 (andyw)
**	    Partial Backup/Recovery Project:
**		Created.
*/
DB_STATUS
dmckp_delete_file(
    DMCKP_CB	*dmckp,
    i4	*err_code )
{
    DMCKP_CB		*d = dmckp;
    char		*command = dmckp->dmckp_command;
    DB_STATUS		status;

    /*
    ** Build the underlying o/s command
    */
    status = dmckp_subst( DMCKP_OP_WORK, DMCKP_DELETE, DMCKP_GRAN_TBL,
		    d, command, MAXCOMMAND, err_code );
    if (( status != E_DB_OK ) && ( status != E_DB_INFO ))
        return( E_DB_ERROR );

    /*
    ** If E_DB_INFO, the line was not there. This is OK as allows the user
    ** to move the delete option into the restore to reduce the number of
    ** spawns for performance reasons.
    */
    if ( status == E_DB_INFO)
	return( E_DB_OK );

    /*
    ** Issue the command
    */
    status = dmckp_spawn( command );
    if ( status != E_DB_OK )
    {
	*err_code = E_DM1321_RFP_BEGIN_ERROR;
	return (E_DB_ERROR);
    }

    return( E_DB_OK );
}

/*{
** Name: dmckp_begin_dump_phase - Begin dump phase of rollforward
**
**
** Description:
**	This routine is called at the beginning of the dump phase.
**
** Inputs:
**	dmckp			Pointer to the DMCKP_CB.
**
** Outputs:
**	err_code		Pointer to variable to return error.
**
**	Returns:
**	E_DB_OK
**	E_DB_ERROR
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	14-sep-1994 (andyw)
**	    Partial Backup/Recovery Project:
**		Created.
*/
DB_STATUS
dmckp_begin_dump_phase(
    DMCKP_CB	*dmckp,
    i4	*err_code )
{
    DMCKP_CB		*d = dmckp;
    char		*command = dmckp->dmckp_command;
    DB_STATUS		status;

    /*
    ** Build the underlying o/s command
    */
    status = dmckp_subst( DMCKP_OP_BEGIN, DMCKP_DUMP, DMCKP_GRAN_ALL,
		   d, command, MAXCOMMAND, err_code );
    if ( status != E_DB_OK )
    {
	if (status == E_DB_INFO)
	    *err_code = E_DM1363_RFP_TMPL_CMD_MISSING;
	else
	    *err_code = E_DM1362_RFP_TMPL_ERROR;

        return( E_DB_ERROR );
    }

    /*
    ** Issue the command
    */
    status = dmckp_spawn( command );
    if ( status != E_DB_OK )
    {
	*err_code = E_DM1321_RFP_BEGIN_ERROR;
	return (E_DB_ERROR);
    }

    return( E_DB_OK );

}

/*{
** Name: dmckp_end_dump_phase - End dump phase of rollforwarddb.
**
**
** Description:
**
** Inputs:
**	dmckp			Pointer to the DMCKP_CB.
**
** Outputs:
**	err_code		Pointer to variable to return error.
**
**	Returns:
**	E_DB_OK
**	E_DB_ERROR
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	14-sep-1994 (andyw)
**	    Partial Backup/Recovery Project:
**		Created.
*/
DB_STATUS
dmckp_end_dump_phase(
    DMCKP_CB	*dmckp,
    i4	*err_code )
{
    DMCKP_CB		*d = dmckp;
    char		*command = dmckp->dmckp_command;
    DB_STATUS		status;

    /*
    ** Build the underlying o/s command
    */
    status = dmckp_subst( DMCKP_OP_END, DMCKP_DUMP, DMCKP_GRAN_ALL,
		    d, command, MAXCOMMAND, err_code );
    if ( status != E_DB_OK )
    {
	if (status == E_DB_INFO)
	    *err_code = E_DM1363_RFP_TMPL_CMD_MISSING;
	else
	    *err_code = E_DM1362_RFP_TMPL_ERROR;

        return( E_DB_ERROR );
    }

    /*
    ** Issue the command
    */
    status = dmckp_spawn( command );
    if ( status != E_DB_OK )
    {
	*err_code = E_DM1321_RFP_BEGIN_ERROR;
	return (E_DB_ERROR);
    }

    return( E_DB_OK );

}

/*{
** Name: dmckp_work_dump_phase - Work dump phase of rollforwarddb
**
**
** Description:
**
** Inputs:
**	dmckp			Pointer to the DMCKP_CB.
**
** Outputs:
**	err_code		Pointer to variable to return error.
**
**	Returns:
**	E_DB_OK
**	E_DB_ERROR
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	14-sep-1994 (andyw)
**	    Partial Backup/Recovery Project:
**		Created.
*/
DB_STATUS
dmckp_work_dump_phase(
    DMCKP_CB	*dmckp,
    i4	*err_code )
{
    DMCKP_CB		*d = dmckp;
    char		*command = dmckp->dmckp_command;
    DB_STATUS		status;

    /*
    ** Build the underlying o/s command
    */
    status = dmckp_subst( DMCKP_OP_WORK, DMCKP_DUMP, DMCKP_GRAN_ALL,
		    d, command, MAXCOMMAND, err_code );
    if ( status != E_DB_OK )
    {
	if (status == E_DB_INFO)
	    *err_code = E_DM1363_RFP_TMPL_CMD_MISSING;
	else
	    *err_code = E_DM1362_RFP_TMPL_ERROR;

        return( E_DB_ERROR );
    }

    /*
    ** Issue the command
    */
    status = dmckp_spawn( command );
    if ( status != E_DB_OK )
    {
	*err_code = E_DM1321_RFP_BEGIN_ERROR;
	return (E_DB_ERROR);
    }

    return( E_DB_OK );

}

/*{
** Name: dmckp_pre_save_loc - Save a database location.
**
**
** Description:
**	This routine saves the specified  database location.
**	All information is held in the DMCKP_CB.
**
** Inputs:
**	dmckp			Pointer to the DMCKP_CB.
**
** Outputs:
**	err_code		Pointer to variable to return error.
**
**	Returns:
**	E_DB_OK
**	E_DB_ERROR
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	21-may-1996 (hanch04)
**	    Added parallel Checkpoint support
**		Created.
**	23-june-1998(angusm)
**	   Audit all Os commands issued (SIR 86561)
*/
DB_STATUS
dmckp_pre_save_loc(
    DMCKP_CB	*dmckp,
    DMP_DCB     *dcb,
    i4	*err_code )
{
    DMCKP_CB		*d = dmckp;
    char		*command = dmckp->dmckp_command;
    DB_STATUS		status;

    /*
    ** Increase count of locations sofar restored
    */
    d->dmckp_loc_number++;

    /*
    ** Build the underlying o/s command
    */
    if ( d->dmckp_raw_blocks )
	status = dmckp_subst( DMCKP_OP_PRE_WORK, DMCKP_SAVE, DMCKP_RAW_DEVICE,
		   d, command , MAXCOMMAND, err_code );
    else
	status = dmckp_subst( DMCKP_OP_PRE_WORK, DMCKP_SAVE, DMCKP_GRAN_DB,
		   d, command , MAXCOMMAND, err_code );

    if ( status != E_DB_OK )
    {
	if (status == E_DB_INFO)
	    *err_code = E_DM1164_CPP_TMPL_CMD_MISSING;
	else
	    *err_code = E_DM1163_CPP_TMPL_ERROR;

        return( E_DB_ERROR );
    }

    /*
    ** Issue the command
    */
    
    status = dmckp_spawn( command );

    _VOID_ dmckp_audit(dcb, DMCKP_SAVE, DMCKP_GRAN_DB,
			status, command, err_code);

    if ( status != E_DB_OK )
    {
	*err_code = E_DM1160_CPP_SAVE;
	return (E_DB_ERROR);
    }

    return( E_DB_OK );

}

/*{
** Name: dmckp_save_loc - Save a database location.
**
**
** Description:
**	This routine saves the specified  database location.
**	All information is held in the DMCKP_CB.
**
** Inputs:
**	dmckp			Pointer to the DMCKP_CB.
**
** Outputs:
**	err_code		Pointer to variable to return error.
**
**	Returns:
**	E_DB_OK
**	E_DB_ERROR
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	06-jan-1995 (shero03)
**	    Added Checkpoint support
**		Created.
**	23-june-1998(angusm)
**	   Audit all Os commands issued (SIR 86561)
*/
DB_STATUS
dmckp_save_loc(
    DMCKP_CB	*dmckp,
    DMP_DCB     *dcb,
    i4	*err_code )
{
    DMCKP_CB		*d = dmckp;
    char		*command = dmckp->dmckp_command;
    DB_STATUS		status;

    /*
    ** Build the underlying o/s command
    */
    if ( d->dmckp_raw_blocks )
	status = dmckp_subst( DMCKP_OP_WORK, DMCKP_SAVE, DMCKP_RAW_DEVICE,
		    d, command , MAXCOMMAND, err_code );
    
    else
	status = dmckp_subst( DMCKP_OP_WORK, DMCKP_SAVE, DMCKP_GRAN_DB,
		    d, command , MAXCOMMAND, err_code );
    if ( status != E_DB_OK )
    {
	if (status == E_DB_INFO)
	    *err_code = E_DM1164_CPP_TMPL_CMD_MISSING;
	else
	    *err_code = E_DM1163_CPP_TMPL_ERROR;

        return( E_DB_ERROR );
    }

    /*
    ** Issue the command
    */
    if ( d->dmckp_dev_cnt > 1 ) 
    {
	status = dmckp_spawnnow( command, d->dmckp_ckp_path_pid );
    }
    else
    {
	status = dmckp_spawn( command );
    }

    _VOID_ dmckp_audit(dcb, DMCKP_SAVE, DMCKP_GRAN_DB,
			status, command, err_code);

    if ( status != E_DB_OK )
    {
	*err_code = E_DM1160_CPP_SAVE;
	return (E_DB_ERROR);
    }

    return( E_DB_OK );

}

/*{
** Name: dmckp_begin_save_db - Begin saving the whole database.
**
**
** Description:
**	This routine is called at the beginning of a restore of the
**	whole database.
**
** Inputs:
**	dmckp			Pointer to the DMCKP_CB.
**
** Outputs:
**	err_code		Pointer to variable to return error.
**
**	Returns:
**	E_DB_OK
**	E_DB_ERROR
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	06-jan-1995 (shero03)
**	    Added Checkpoint support
**		Created.
**	23-june-1998(angusm)
**	   Audit all Os commands issued (SIR 86561)
*/
DB_STATUS
dmckp_begin_save_db(
    DMCKP_CB	*dmckp,
    DMP_DCB     *dcb,
    i4	*err_code )
{
    DMCKP_CB		*d = dmckp;
    char		*command = dmckp->dmckp_command;
    DB_STATUS		status;

    /*
    ** Build the underlying o/s command
    */
    status = dmckp_subst( DMCKP_OP_BEGIN, DMCKP_SAVE, DMCKP_GRAN_DB,
		    d, command, MAXCOMMAND, err_code );
    if ( status != E_DB_OK )
    {
	if (status == E_DB_INFO)
	    *err_code = E_DM1164_CPP_TMPL_CMD_MISSING;
	else
	    *err_code = E_DM1163_CPP_TMPL_ERROR;

        return( E_DB_ERROR );
    }

    /*
    ** Issue the command
    */
    status = dmckp_spawn( command );

    _VOID_ dmckp_audit(dcb, DMCKP_SAVE, DMCKP_GRAN_DB,
			status, command, err_code);

    if ( status != E_DB_OK )
    {
	*err_code = E_DM1158_CPP_BEGIN_ERROR;
	return (E_DB_ERROR);
    }

    return( E_DB_OK );

}

/*{
** Name: dmckp_end_restore_db -  End saving the whole database.
**
**
** Description:
**	This routine is called at the end of a save of the
**	whole database.
**
** Inputs:
**	dmckp			Pointer to the DMCKP_CB.
**
** Outputs:
**	err_code		Pointer to variable to return error.
**
**	Returns:
**	E_DB_OK
**	E_DB_ERROR
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	06-jan-1995 (shero03)
**	    Added Checkpoint support
**		Created.
**	23-june-1998(angusm)
**	   Audit all Os commands issued (SIR 86561)
*/
DB_STATUS
dmckp_end_save_db(
    DMCKP_CB	*dmckp,
    DMP_DCB     *dcb,
    i4	*err_code )
{
    DMCKP_CB		*d = dmckp;
    char		*command = dmckp->dmckp_command;
    DB_STATUS		status;

    /*
    ** Build the underlying o/s command
    */
    status = dmckp_subst( DMCKP_OP_END, DMCKP_SAVE, DMCKP_GRAN_DB,
		    d, command, MAXCOMMAND, err_code );
    if ( status != E_DB_OK )
    {
	if (status == E_DB_INFO)
	    *err_code = E_DM1164_CPP_TMPL_CMD_MISSING;
	else
	    *err_code = E_DM1163_CPP_TMPL_ERROR;

        return( E_DB_ERROR );
    }

    /*
    ** Issue the command
    */
    status = dmckp_spawn( command );

    _VOID_ dmckp_audit(dcb, DMCKP_SAVE, DMCKP_GRAN_DB,
			status, command, err_code);

    if ( status != E_DB_OK )
    {
	*err_code = E_DM1159_CPP_END_ERROR;
	return (E_DB_ERROR);
    }

    return( E_DB_OK );

}

/*{
** Name: dmckp_begin_save_tbl - Begin saving some tables
**
**
** Description:
**	This routine is called at the begining of table level checkpoint.
**
** Inputs:
**	dmckp			Pointer to the DMCKP_CB.
**
** Outputs:
**	err_code		Pointer to variable to return error.
**
**	Returns:
**	E_DB_OK
**	E_DB_ERROR
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	06-jan-1995 (shero03)
**	    Added Checkpoint support
**		Created.
**	23-june-1998(angusm)
**	   Audit all Os commands issued (SIR 86561)
*/
DB_STATUS
dmckp_begin_save_tbl(
    DMCKP_CB	*dmckp,
    DMP_DCB     *dcb,
    i4	*err_code )
{
    DMCKP_CB		*d = dmckp;
    char		*command = dmckp->dmckp_command;
    DB_STATUS		status;

    /*
    ** Build the underlying o/s command
    */
    status = dmckp_subst( DMCKP_OP_BEGIN, DMCKP_SAVE, DMCKP_GRAN_TBL,
		    d, command, MAXCOMMAND, err_code );
    if ( status != E_DB_OK )
    {
	if (status == E_DB_INFO)
	    *err_code = E_DM1164_CPP_TMPL_CMD_MISSING;
	else
	    *err_code = E_DM1163_CPP_TMPL_ERROR;

        return( E_DB_ERROR );
    }

    /*
    ** Issue the command
    */
    status = dmckp_spawn( command );

    _VOID_ dmckp_audit(dcb, DMCKP_SAVE, DMCKP_GRAN_TBL,
			status, command, err_code);

    if ( status != E_DB_OK )
    {
	*err_code = E_DM1158_CPP_BEGIN_ERROR;
	return (E_DB_ERROR);
    }

    return( E_DB_OK );

}

/*{
** Name: dmckp_end_save_tbl -  End saving a set of tables.
**
**
** Description:
**	This routine is called at the end of a save of a
**	set of tables.
**
** Inputs:
**	dmckp			Pointer to the DMCKP_CB.
**
** Outputs:
**	err_code		Pointer to variable to return error.
**
**	Returns:
**	E_DB_OK
**	E_DB_ERROR
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	06-jan-1995 (shero03)
**	    Added Checkpoint support
**		Created.
**	23-june-1998(angusm)
**	   Audit all Os commands issued (SIR 86561)
*/
DB_STATUS
dmckp_end_save_tbl(
    DMCKP_CB	*dmckp,
    DMP_DCB     *dcb,
    i4	*err_code )
{
    DMCKP_CB		*d = dmckp;
    char		*command = dmckp->dmckp_command;
    DB_STATUS		status;

    /*
    ** Build the underlying o/s command
    */
    status = dmckp_subst( DMCKP_OP_END, DMCKP_SAVE, DMCKP_GRAN_TBL,
		    d, command, MAXCOMMAND, err_code );
    if ( status != E_DB_OK )
    {
	if (status == E_DB_INFO)
	    *err_code = E_DM1164_CPP_TMPL_CMD_MISSING;
	else
	    *err_code = E_DM1163_CPP_TMPL_ERROR;

        return( E_DB_ERROR );
    }

    /*
    ** Issue the command
    */
    status = dmckp_spawn( command );

    _VOID_ dmckp_audit(dcb, DMCKP_SAVE, DMCKP_GRAN_TBL,
			status, command, err_code);

    if ( status != E_DB_OK )
    {
	*err_code = E_DM1159_CPP_END_ERROR;
	return (E_DB_ERROR);
    }

    return( E_DB_OK );
}

/*{
** Name: dmckp_pre_save_file - Pre work for save a file for a table.
**
**
** Description:
**	This routine saves the specified file at the specified database
**	location.
**	All information is held in the DMCKP_CB.
**
** Inputs:
**	dmckp			Pointer to the DMCKP_CB.
**
** Outputs:
**	err_code		Pointer to variable to return error.
**
**	Returns:
**	E_DB_OK
**	E_DB_ERROR
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	01-june-1996 (hanch04)
**	    Added Parallel Checkpointing support
**		Created from dmckp_save_file.
**	23-june-1998(angusm)
**	   Audit all Os commands issued (SIR 86561)
*/
DB_STATUS
dmckp_pre_save_file(
    DMCKP_CB	*dmckp,
    DMP_DCB     *dcb,
    i4	*err_code )
{
    DMCKP_CB		*d = dmckp;
    char		*command = dmckp->dmckp_command;
    DB_STATUS		status;

    /*
    ** Increase the count of the number of files recovered at this location
    */
    d->dmckp_file_num++;

    /*
    ** Build the underlying o/s command
    */
    if ( d->dmckp_raw_blocks )
	status = dmckp_subst( DMCKP_OP_PRE_WORK, DMCKP_SAVE, DMCKP_RAW_DEVICE,
		    d, command , MAXCOMMAND, err_code );
    else
	status = dmckp_subst( DMCKP_OP_PRE_WORK, DMCKP_SAVE, DMCKP_GRAN_TBL,
		    d, command , MAXCOMMAND, err_code );
    if ( status != E_DB_OK )
    {
	if (status == E_DB_INFO)
	    *err_code = E_DM1164_CPP_TMPL_CMD_MISSING;
	else
	    *err_code = E_DM1163_CPP_TMPL_ERROR;

        return( E_DB_ERROR );
    }

    /*
    ** Issue the command
    */
    status = dmckp_spawn( command );

    _VOID_ dmckp_audit(dcb, DMCKP_SAVE, DMCKP_GRAN_TBL,
			status, command, err_code);

    if ( status != E_DB_OK )
    {
	*err_code = E_DM1160_CPP_SAVE;
	return (E_DB_ERROR);
    }

    return( E_DB_OK );

}

/*{
** Name: dmckp_save_file - Save a file for a table.
**
**
** Description:
**	This routine saves the specified file at the specified database
**	location.
**	All information is held in the DMCKP_CB.
**
** Inputs:
**	dmckp			Pointer to the DMCKP_CB.
**
** Outputs:
**	err_code		Pointer to variable to return error.
**
**	Returns:
**	E_DB_OK
**	E_DB_ERROR
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	06-jan-1995 (shero03)
**	    Added Checkpoint support
**		Created.
**	23-june-1998(angusm)
**	   Audit all Os commands issued (SIR 86561)
*/
DB_STATUS
dmckp_save_file(
    DMCKP_CB	*dmckp,
    DMP_DCB     *dcb,
    i4	*err_code )
{
    DMCKP_CB		*d = dmckp;
    char		*command = dmckp->dmckp_command;
    DB_STATUS		status;

    /*
    ** Build the underlying o/s command
    */
    if ( d->dmckp_raw_blocks )
	status = dmckp_subst( DMCKP_OP_WORK, DMCKP_SAVE, DMCKP_RAW_DEVICE,
		    d, command , MAXCOMMAND, err_code );
    else
	status = dmckp_subst( DMCKP_OP_WORK, DMCKP_SAVE, DMCKP_GRAN_TBL,
		    d, command , MAXCOMMAND, err_code );
    if ( status != E_DB_OK )
    {
	if (status == E_DB_INFO)
	    *err_code = E_DM1164_CPP_TMPL_CMD_MISSING;
	else
	    *err_code = E_DM1163_CPP_TMPL_ERROR;

        return( E_DB_ERROR );
    }

    /*
    ** Issue the command
    */
    if ( d->dmckp_dev_cnt > 1 )
    {
        status = dmckp_spawnnow( command, d->dmckp_ckp_path_pid );
    }
    else
    {
        status = dmckp_spawn( command );
    }

    _VOID_ dmckp_audit(dcb, DMCKP_SAVE, DMCKP_GRAN_TBL,
			status, command, err_code);

    if ( status != E_DB_OK )
    {
	*err_code = E_DM1160_CPP_SAVE;
	return (E_DB_ERROR);
    }

    return( E_DB_OK );

}

/*{
** Name: dmckp_audit - Audit OS action for ckpdb/rollforwarddb.
**
**
** Description:
**	This routine writes an audit record for OS actions
**	for ckpdb/rollforwarddb
**
** Inputs:
**	dmckp			Pointer to the DMCKP_CB.
**
** Outputs:
**	err_code		Pointer to variable to return error.
**
**	Returns:
**	E_DB_OK
**	E_DB_ERROR
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	21-oct-1997(angusm)
**	   Created.
**	   audit all OS command strings executed for checkpoint/
**	   rollforward. (SIR 86561)
*/
DB_STATUS
dmckp_audit( 
    DMP_DCB     *dcb,
    u_i4	direction,
    u_i4	granularity,
    DB_STATUS   status,
    char	*command,
    i4	*err_code )
{
    if ( dmf_svcb->svcb_status & SVCB_C2SECURE )
    {
	SXF_EVENT           type = SXF_E_DATABASE;
        SXF_ACCESS	    access=0;
	i4             msgid;
        i4		    len = STlength(command);
	DB_ERROR	    local_dberr;

	 if (direction == DMCKP_SAVE)
	    access |= SXF_A_SELECT;
	 else
	    access |= SXF_A_UPDATE;

	 if (status == E_DB_OK)
	    access |= SXF_A_SUCCESS;
	 else
	    access |= SXF_A_FAIL;

	 if (access & SXF_A_UPDATE)
	    msgid = I_SX270C_ROLLDB;
	 else
	    msgid = I_SX270B_CKPDB; 
	 

	_VOID_ dma_write_audit1(type,
		    access,
		    dcb->dcb_name.db_db_name, /* Object name (database) */
		    sizeof(dcb->dcb_name.db_db_name), /* Object name (database) */
		    &dcb->dcb_db_owner, /* Object owner */
		    msgid,     	    /*  Message */
		    TRUE,		    /* Force */
		    &local_dberr,
		    (PTR)NULL,
		    command, len, 0);
	*err_code = local_dberr.err_code;
    }

    return( E_DB_OK );

}
