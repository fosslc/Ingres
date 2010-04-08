/*
** Copyright (c) 1988, 2005 Ingres Corporation
**
*/

#ifdef	    PHASE50
#include	compat.h
#include	er.h

#include	lo.h
#include	qu.h
#include	si.h
#include	nm.h
#include	st.h

#include	"pvhack.h"
#include	"duerr50.h"

#include	"duvstrings.h"
#include	"duvcommon.h"
#include	"duvfiles.h"

#else

#include	<compat.h>
#include	<er.h>

#include	<lo.h>
#include	<qu.h>
#include	<si.h>
#include	<nm.h>
#include	<st.h>

#include	<duerr.h>

#include	<duvstrings.h>
#include	<duvcommon.h>
#include	<duvfiles.h>
#endif	   /* PHASE50 */

/**
**
**  Name: DUVFILES.C - Collection of routines that operate on the
**		       CONVTO60 database log file and audit/status
**		       files.
**
**  Description:
**        This module contains a collection of routines that operate
**	on the CONVTO60 database log and audit/status files.  This module
**	is sprinkled with "#ifdef PHASE50" statements.  This is because
**	these routines must be called from the 5.0 phase of CONVTO60 as
**	well as the 6.0 DBMS and FECVT60 phases of CONVTO60.  This module
**	provides a common interface for operating on these files across
**	all three executables.  The only difference between the 6.0 and
**	5.0 versions of this file should be a "#define PHASE50" preceeding
**	the header inclusion section.
**
**          duv_0statf_open -	    Open the audit/status file.  If the
**				    file doesn't exist, this routine
**				    creates it.
**          duv_finit -		    Create a path and file specification to
**				    the database log file or audit/status file.
**	    duv_2statf_read -	    Read the contents of the audit/status file.
**	    duv_3statf_write -	    Write the contents of the CONVTO60 status
**				    buffer to the audit/status file.
**          duv_determine_status -  Read the contents of the audit/status file
**				    into the CONVTO60 status buffer.
**	    duv_dbf_open -	    Open the database specific log file.
**	    duv_f_close -	    Close a given file.
**	    duv_prnt_msg -	    Print a message to the specified files.
**	    duv_iw_msg -	    Format an informational or warning message
**				    and print to specified files.
**
**  History:    $Log-for RCS$
**      23-Mar-88 (ericj)
**          Initially commented.
**      15-Jul-88 (ericj)
**          Include st.h.
**	24-mar-93 (smc)
**	    Commented out text following endifs.
**      28-sep-96 (mcgem01)
**          Global data moved to duvdata.c
**      21-apr-1999 (hanch04)
**          replace STindex with STchr
**	07-sep-2005 (abbjo03)
**	    Replace xxx_FILE constants by SI_xxx.
**/


/*
**      Strings used in error messages to specify what routine the error
**	occurred in.
*/

#define                 DUV_0STATF_OPEN	    "duv_0statf_open()"
#define			DUV_FINIT	    "duv_finit()"
#define                 DUV_2STATF_READ	    "duv_2statf_read()"
#define                 DUV_3STATF_WRITE    "duv_3statf_write()"

#define			DUV_DBF_OPEN	    "duv_dbf_open()"
#define			DUV_F_CLOSE	    "duv_f_close()"

/*
[@group_of_defined_constants@]...
*/
/*
[@type_definitions@]
*/

/*
** Definition of all global variables owned by this file.
*/

GLOBALREF DU_FILE             Duv_statf;  /* CONVTO60 file descriptor
					    ** used for the audit/status
					    ** file.
					    */
GLOBALREF DU_FILE             Duv_dbf;    /* CONVTO60 file descriptor
					    ** used for the db specific
					    ** log file.
					    */
GLOBALREF DUV_CONV_STAT	      Duv_conv_status; /* Buffer for the
						** conversion status
						** and log file.
						*/
/*
[@global_variable_definition@]...
*/
/*
[@static_variable_or_function_definitions@]
[@function_definition@]...
*/


/*{
** Name: duv_0statf_open - Open the audit/status file.
**
** Description:
**        This routine opens the audit/status file.  If the file doesn't
**	exists, it opens the file in 'w' mode to create it.  It then
**	closes.  At the end of successful completion, this file (which
**	is an SI_RACC file) is opened in 'rw' mode.  This allows random
**	access to the data in the file.
**
** Inputs:
**      duv_errcb                       Error control block.
**
** Outputs:
**      *duv_errcb                      If an error occurs, this struct is
**					set by calling du_error().
**	Returns:
**	    E_DU_OK			Completed successfully.
**	  5.0 Errors:
**	    E_DU410_GEN_CL_FAIL		A CL routine failed.
**	  6.0 Errors:
**	    E_DU2410_GEN_CL_FAIL	A CL routine failed.
**	Exceptions:
**	    none
**
** Side Effects:
**	      If the file doesn't exist, it is created.  Upon successful
**	    completion the audit/status file is opened in 'rw' mode.
**
** History:
**      25-Mar-88 (ericj)
**          Initially commented.
[@history_template@]...
*/
DU_STATUS
duv_0statf_open(duv_errcb)
DU_ERROR	    *duv_errcb;
{

    if (LOexist(&Duv_statf.du_floc))
    {
	/* Open the file in write mode to guarantee it exists. */
	if ((duv_errcb->du_clerr = SIfopen(&Duv_statf.du_floc, "w", SI_RACC,
					   (i4) sizeof (DUV_CONV_STAT),
					   &Duv_statf.du_file))
	    != OK
	   )
#ifdef PHASE50
	    return(duv_err50(duv_errcb, E_DU410_GEN_CL_FAIL, duv_errcb->du_clerr,
			     2, PV_STR, DUV_SIFOPEN, PV_STR, DUV_0STATF_OPEN));
#else
	    return(du_error(duv_errcb, E_DU2410_GEN_CL_FAIL, 4,
			    0, DUV_SIFOPEN, 0, DUV_0STATF_OPEN));
#endif /* PHASE50 */

	/* Close the file. */
	if ((duv_errcb->du_clerr = SIclose(Duv_statf.du_file)) != OK)
#ifdef	PHASE50
	    return(duv_err50(duv_errcb, E_DU410_GEN_CL_FAIL, duv_errcb->du_clerr,
		   2, PV_STR, DUV_SICLOSE, PV_STR, DUV_0STATF_OPEN));
#else
	    return(du_error(duv_errcb, E_DU2410_GEN_CL_FAIL,
		   4, 0, DUV_SICLOSE, 0, DUV_0STATF_OPEN));
#endif	/* PHASE50 */
    }
	

    /* Open the file in the appropriate mode. */
    if ((duv_errcb->du_clerr = SIfopen(&Duv_statf.du_floc, "rw", SI_RACC,
				       (i4) sizeof (DUV_CONV_STAT),
				       &Duv_statf.du_file))
	!= OK
       )
#ifdef	PHASE50
	return(duv_err50(duv_errcb, E_DU410_GEN_CL_FAIL, duv_errcb->du_clerr,
			 2, PV_STR, DUV_SIFOPEN, PV_STR, DUV_0STATF_OPEN));
#else
	return(du_error(duv_errcb, E_DU2410_GEN_CL_FAIL,
			 4, 0, DUV_SIFOPEN, 0, DUV_0STATF_OPEN));
#endif	/* PHASE50 */

    return(OK);
}


/*{
** Name: duv_finit - Create a full path file specification for either the
**		     database audit/status file or log file.
**
** Description:
**        This routine creates a full path file specification for either
**	the database audit/status file or the log file.  Both of these files
**	are located in the directory II_SYSTEM~INGRES~CONVTO60~dbname where
**	~ is an abstraction for path specifications that are actually specific
**	to various operating systems.  Note, if the database name directory
**	does not exist and the build_path parameter is set, this routine
**	creates it.
**
** Inputs:
**	fp				Status or Log file descriptor.
**	dbname				Name of database to convert path for.
**	filename			Name of file to be accessed.  The
**					audit/status file has the name
**					"dbname.sts".  The log file has the
**					name "dbname.log".
**	build_path			If set and the dbname directory doesn't
**					exist, create it.
**      duv_errcb                       Error control block.
**
** Outputs:
**      *duv_errcb                      If an error occurs, this struct is
**					set by calling du_error().
**	Returns:
**	    E_DU_OK			Completed successfully.
**	  5.0 Errors:
**	    E_DU410_GEN_CL_FAIL		A CL routine failed.
**	    E_DU550_NO_LOGNAME		No translation exists for the Ingres
**					environment variable 'II_SYSTEM'.
**	  6.0 Errors:
**	    E_DU2410_GEN_CL_FAIL	A CL routine failed.
**	    E_DU3030_NO_LOGNAME		No translation exists for the Ingres
**					environment variable 'II_SYSTEM'.
**	Exceptions:
**	    none
**
** Side Effects:
**	      If the CONVTO60 'dbname' directory doesn't exist and the
**	    build_path parameter is set, then the directory is created.
**
** History:
**      25-Mar-88 (ericj)
**          Initially commented.
[@history_template@]...
*/
DU_STATUS
duv_finit(fp, dbname, filename, build_path, duv_errcb)
DU_FILE		    *fp;
char		    *dbname;
char		    *filename;
i4		    build_path;
DU_ERROR	    *duv_errcb;
{
    char		*cp;

    /* Initialize the location of the file. */

    /* Create a path to the directory that is suppossed to contain the
    ** audit/status file.
    */
    NMgtAt(DUV_60_SYSNAME, &cp);
    if (cp == NULL || *cp == EOS)
#ifdef	PHASE50
	return(duv_err50(duv_errcb, E_DU550_NO_LOGNAME, DU_NOCLERR, 1,
			 PV_STR, DUV_60_SYSNAME));
#else
	return(du_error(duv_errcb, E_DU3030_NO_LOGNAME, 2, 0, DUV_60_SYSNAME));
#endif	/* PHASE50 */

    STcopy(cp, fp->du_flocbuf);
    if ((duv_errcb->du_clerr = LOfroms(PATH, fp->du_flocbuf,
				       &(fp->du_floc))
	) != OK
       )
#ifdef	PHASE50
	return(duv_err50(duv_errcb, E_DU410_GEN_CL_FAIL, duv_errcb->du_clerr,
			 2, PV_STR, DUV_LOFROMS, PV_STR, DUV_FINIT));
#else
	return(du_error(duv_errcb, E_DU2410_GEN_CL_FAIL,
			 4, 0, DUV_LOFROMS, 0, DUV_FINIT));
#endif	/* PHASE50 */

    for (;;)
    {
	if ((duv_errcb->du_clerr = LOfaddpath(&fp->du_floc, "ingres",
					      &fp->du_floc)) != OK
	   )
	    break;
	if ((duv_errcb->du_clerr = LOfaddpath(&fp->du_floc, DUV_0CONVTO60_DIR,
					      &fp->du_floc)) != OK
	   )
	    break;
	if (dbname != NULL)
	    duv_errcb->du_clerr = LOfaddpath(&fp->du_floc, dbname,
					     &fp->du_floc);
	break;
    }
    if (duv_errcb->du_clerr != OK)
#ifdef	PHASE50
	return(duv_err50(duv_errcb, E_DU410_GEN_CL_FAIL, duv_errcb->du_clerr, 2,
			 PV_STR, DUV_LOFADDPATH, PV_STR, DUV_FINIT));
#else
	return(du_error(duv_errcb, E_DU2410_GEN_CL_FAIL, 4,
			 0, DUV_LOFADDPATH, 0, DUV_FINIT));
#endif	/* PHASE50 */

    /* Check to see if the utility directory for this database exists. 
    ** If not, create it.
    */
    if (build_path && LOexist(&fp->du_floc))
    {
	if ((duv_errcb->du_clerr = LOcreate(&fp->du_floc)) != OK)
#ifdef	PHASE50
	    return(duv_err50(duv_errcb, E_DU410_GEN_CL_FAIL,
		   duv_errcb->du_clerr, 2,
		   PV_STR, DUV_LOCREATE, PV_STR, DUV_FINIT));
#else
	    return(du_error(duv_errcb, E_DU2410_GEN_CL_FAIL, 4,
		   0, DUV_LOCREATE, 0, DUV_FINIT));
#endif	/* PHASE50 */
    }


    /* Create a location that contains either the audit/status file
    ** specification, global log file specification, or the db specific
    ** log file.
    */
    if ((duv_errcb->du_clerr = LOfstfile(filename, &fp->du_floc)) != OK)
#ifdef	PHASE50
	return(duv_err50(duv_errcb, E_DU410_GEN_CL_FAIL, duv_errcb->du_clerr, 2,
			 PV_STR, DUV_LOFSTFILE, PV_STR, DUV_FINIT));
#else
	return(du_error(duv_errcb, E_DU2410_GEN_CL_FAIL, 4,
			 0, DUV_LOFSTFILE, 0, DUV_FINIT));
#endif	/* PHASE50 */

    return(E_DU_OK);
}

    

/*{
** Name: duv_2statf_read - Read the contents of the audit/status file.
**
** Description:
**        This routine reads the contents of the audit/status file.
**	First, it seeks to the beginning of this file and then
**	it reads the contents of the file into the global audit/status
**	buffer 'Duv_statf'.
**
** Inputs:
**      duv_errcb                       Error control block.
**
** Outputs:
**      *duv_errcb                      If an error occurs, this struct is
**					set by calling du_error().
**	Returns:
**	    E_DU_OK			Completed successfully.
**	  5.0 Errors:
**	    E_DU410_GEN_CL_FAIL		A CL routine failed.
**	    E_DU411_BAD_STATF_READ	Read incorrect number of bytes.
**	  6.0 Errors:
**	    E_DU2410_GEN_CL_FAIL	A CL routine failed.
**	    E_DU4000_BAD_STATF_READ	Read incorrect number of bytes.
**	Exceptions:
**	    none
**
** Side Effects:
**	      Seeks to the beginning of the audit/status file and
**	    reads contents into global audit/status buffer 'Duv_statf'.
**
** History:
**      25-Mar-88 (ericj)
**          Initially commented.
[@history_template@]...
*/
DU_STATUS
duv_2statf_read(duv_errcb)
DU_ERROR	    *duv_errcb;
{
    i4		bcnt;

    /* Seek to the front of the status file. */
    if ((duv_errcb->du_clerr = SIfseek(Duv_statf.du_file, (i4) 0, SI_P_START))
	!= OK
       )
#ifdef	PHASE50
	return(duv_err50(duv_errcb, E_DU410_GEN_CL_FAIL, duv_errcb->du_clerr,
			 2, PV_STR, DUV_SIFSEEK, PV_STR, DUV_2STATF_READ));
#else
	return(du_error(duv_errcb, E_DU2410_GEN_CL_FAIL,
			 4, 0, DUV_SIFSEEK, 0, DUV_2STATF_READ));
#endif	/* PHASE50 */


    /* Read the status file into the appropriate buffer. */
    if ((duv_errcb->du_clerr = SIread(Duv_statf.du_file,
				      (i4) sizeof (DUV_CONV_STAT), &bcnt,
				      (char *) &Duv_conv_status)) != OK)
#ifdef	PHASE50
	return(duv_err50(duv_errcb, E_DU410_GEN_CL_FAIL, duv_errcb->du_clerr,
			 2, PV_STR, DUV_SIREAD, PV_STR, DUV_2STATF_READ));
#else
	return(du_error(duv_errcb, E_DU2410_GEN_CL_FAIL,
			 4, 0, DUV_SIREAD, 0, DUV_2STATF_READ));
#endif	/* PHASE50 */


    if (bcnt != sizeof (DUV_CONV_STAT))
#ifdef	PHASE50
	return(duv_err50(duv_errcb, E_DU411_BAD_STATF_READ, DU_NOCLERR, 0));
#else
	return(du_error(duv_errcb, E_DU4000_BAD_STATF_READ, 0));
#endif	/* PHASE50 */

    return(E_DU_OK);
}



/*{
** Name: duv_3statf_write - Update the contents of the audit/status file.
**
** Description:
**        This routine writes the contents of the global audit/status
**	buffer 'Duv_statf' to the audit/status file.  The write is then
**	flushed to disk.
**
** Inputs:
**      duv_errcb                       Error control block.
**
** Outputs:
**      *duv_errcb                      If an error occurs, this struct is
**					set by calling du_error().
**	Returns:
**	    E_DU_OK			Completed successfully.
**	  5.0 Errors:
**	    E_DU410_GEN_CL_FAIL		A CL routine failed.
**	  6.0 Errors:
**	    E_DU2410_GEN_CL_FAIL	A CL routine failed.
**	Exceptions:
**	    none
**
** Side Effects:
**	      The contents of the audit/status file is updated.
**
** History:
**      25-Mar-88 (ericj)
**          Initially commented.
[@history_template@]...
*/
DU_STATUS
duv_3statf_write(duv_errcb)
DU_ERROR	    *duv_errcb;
{
    i4		useless_cnt;

    /* Seek to the front of the status file. */
    if ((duv_errcb->du_clerr = SIfseek(Duv_statf.du_file, (i4) 0, SI_P_START))
	!= OK
       )
#ifdef	PHASE50
	return(duv_err50(duv_errcb, E_DU410_GEN_CL_FAIL, duv_errcb->du_clerr,
			 2, PV_STR, DUV_SIFSEEK, PV_STR, DUV_3STATF_WRITE));
#else
	return(du_error(duv_errcb, E_DU2410_GEN_CL_FAIL,
			 4, 0, DUV_SIFSEEK, 0, DUV_3STATF_WRITE));
#endif	/* PHASE50 */

    /* Write the conversion status buf into the status file. */
    if ((duv_errcb->du_clerr = SIwrite((i4) sizeof (DUV_CONV_STAT),
				       (char *) &Duv_conv_status, &useless_cnt,
				       Duv_statf.du_file)) != OK
       )
#ifdef	PHASE50
	return(duv_err50(duv_errcb, E_DU410_GEN_CL_FAIL, duv_errcb->du_clerr,
			 2, PV_STR, DUV_SIWRITE, PV_STR, DUV_3STATF_WRITE));
#else
	return(du_error(duv_errcb, E_DU2410_GEN_CL_FAIL,
			 4, 0, DUV_SIWRITE, 0, DUV_3STATF_WRITE));
#endif	/* PHASE50 */

    /* This write implicitly does a flush. */
    if ((duv_errcb->du_clerr = SIflush(Duv_statf.du_file)) != OK)
#ifdef	PHASE50
	return(duv_err50(duv_errcb, E_DU410_GEN_CL_FAIL, duv_errcb->du_clerr,
			 2, PV_STR, DUV_SIFLUSH, PV_STR, DUV_3STATF_WRITE));
#else
	return(du_error(duv_errcb, E_DU2410_GEN_CL_FAIL,
			 4, 0, DUV_SIFLUSH, 0, DUV_3STATF_WRITE));
#endif	/* PHASE50 */

    return(E_DU_OK);
}



/*{
** Name: duv_determine_status()	-   get the status of the database being
**				    converted.
**
** Description:
**        This routine gets the status of the database being converted.
**	This status is kept in a binary file in a directory with the
**	name of the database being converted.  This directory is located
**	under the CONVTO60 utility directory which is located under
**	the Ingres directory of the default system installation.
**	  This routine is idempotent.
**	  This routine assumes that database log file has been opened.
**
** Inputs:
**      dbname				Database name.
**	intro_msg			If TRUE, print an introduction message
**					for this database.
**	duv_errcb			CONVTO60 error control block.
**
** Outputs:
**      *conv_status                    The status of the database being
**					converted.
**	    .du_audit_stat		If this is the first time
**					CONVTO60 has been called on this
**					database, this routine will return
**					DUV_STATUS_FILE_EXISTS, otherwise it
**					will contain the status of the last
**					idempotent action to be successfully
**					completed.
**	    .du_prog_stat		The exit status of a program.  This
**					is used to communicate the exit status
**					of the 6.0 DBMS phase to the 6.0 FE
**					phase and then the exit status of the
**					6.0 FE phase to this program.
**				    
**	*duv_errcb			If an error occurs, this struct will be
**					set by a call to duv_err50().
**	Returns:
**	    E_DU_OK			Completed successfully.
**	Exceptions:
**	    none
**
** Side Effects:
**	    If the audit/status file doesn't exist it is created and
**	    initialized.  It also updates the global buffer Duv_conv_status.
**
** History:
**      29-Jun-87 (ericj)
**          Initial creation.
[@history_template@]...
*/
DU_STATUS
duv_determine_status(dbname, intro_msg, duv_errcb)
char		    *dbname;
i4		    intro_msg;
DU_ERROR	    *duv_errcb;
{
    i4		    exists;
    char	    tmp_buf[DUV_FMAXNAME];

    /* If necessary, initialize the location of the status/audit file. */
    if (Duv_statf.du_flocbuf[0] == EOS)
    {
	STprintf(tmp_buf, "%s.sts", dbname);
	if (duv_finit(&Duv_statf, dbname, tmp_buf, FALSE, duv_errcb)
	    != E_DU_OK
	   )
	    return(duv_errcb->du_status);
    }

    /* Determine if the file already exists. */
    exists  = !LOexist(&Duv_statf.du_floc);

    /* Open the file for random access. */
    if (duv_0statf_open(duv_errcb) != E_DU_OK)
	return(duv_errcb->du_status);


    if (exists)
    {
	/* The file exists, so read the current status of the
	** database conversion.
	*/
	if (duv_2statf_read(duv_errcb) != OK)
	    return(duv_errcb->du_status);
    }

    if (intro_msg)
    {
#ifdef	PHASE50
	if (exists
	    && (Duv_conv_status.du_audit_stat >= DUV_1STATUS_FILE_EXISTS)
	   )
	{
	    /* The file exists and has been initialized,
	    ** so print a "resuming conversion" info message.
	    */
	    if (duv_iw_msg(duv_errcb, DUV_ALLF, I_DU061_CONVDB_INTRO, 2,
			   PV_STR, DUV_RESUMING, PV_STR, dbname)
		!= E_DU_OK
	       )
		return(duv_errcb->du_status);
	}
	else
	{
	    /* The file doesn't exist, so print a "starting conversion" info
	    ** message.
	    */
	    if (duv_iw_msg(duv_errcb, DUV_ALLF, I_DU061_CONVDB_INTRO, 2,
			   PV_STR, DUV_STARTING, PV_STR, dbname)
		!= E_DU_OK
	       )
		return(duv_errcb->du_status);

	    /* The file didn't exist, so initialize appropriately. */
	    Duv_conv_status.du_audit_stat	= DUV_1STATUS_FILE_EXISTS;
	    Duv_conv_status.du_prog_stat	= DUV_NOPROG_CALL;
	    if (duv_3statf_write(duv_errcb) != OK)
		return(duv_errcb->du_status);

	}
#else
	if (Duv_conv_status.du_audit_stat == DUV_50PASSOFF)
	{
	    if (duv_iw_msg(duv_errcb, DUV_ALLF, I_DU0061_CONVDB_INTRO, 4,
			   0, DUV_STARTING, 0, dbname)
		!= E_DU_OK
	       )
		return(duv_errcb->du_status);
	}
	else if (Duv_conv_status.du_audit_stat < DUV_LAST_DBMS_ACTION)
	{
	    if (duv_iw_msg(duv_errcb, DUV_ALLF, I_DU0061_CONVDB_INTRO, 4,
			   0, DUV_RESUMING, 0, dbname)
		!= E_DU_OK
	       )
	    return(duv_errcb->du_status);
	}
#endif	/* PHASE50 */
    }   /* end of if (intro_msg) */

    return(E_DU_OK);
}


/*{
** Name: duv_dbf_open - open the CONVTO60 database log file.
**
** Description:
**        This routine opens the database specific log file so the
**	events that occur during the conversion of a database can be
**	recorded.
**
** Inputs:
**      duv_errcb                       Error control block.
**	dbname				Name of database to open the
**					log file for.
** Outputs:
**      *duv_errcb                      If an error occurs, this struct is
**					set by calling du_error().
**	Returns:
**	    E_DU_OK			Completed successfully.
**	  5.0 Errors:
**	    E_DU410_GEN_CL_FAIL		A CL routine failed.
**	  6.0 Errors:
**	    E_DU2410_GEN_CL_FAIL	A CL routine failed.
**	Exceptions:
**	    none
**
** Side Effects:
**	      Opens the CONVTO60 database log file.
**
** History:
**      25-Mar-88 (ericj)
**          Initially commented.
[@history_template@]...
*/
DU_STATUS
duv_dbf_open(dbname, duv_errcb)
char		*dbname;
DU_ERROR	*duv_errcb;
{
    char	    tmp_buf[DUV_FMAXNAME];

    /* Open the database specific conversion log file. */

    /* Initialize the file location if necessary. */
    if (Duv_dbf.du_flocbuf[0] == EOS)
    {
	STprintf(tmp_buf, "%s.log", dbname);
	if (duv_finit(&Duv_dbf, dbname, tmp_buf, TRUE, duv_errcb) != E_DU_OK)
	    return(duv_errcb->du_status);
    }

    if ((duv_errcb->du_clerr = SIopen(&Duv_dbf.du_floc, "a", &Duv_dbf.du_file)
	) != OK
       )
#ifdef	PHASE50
	return(duv_err50(duv_errcb, E_DU410_GEN_CL_FAIL, duv_errcb->du_clerr,
			 2,PV_STR, DUV_SIOPEN, PV_STR, DUV_DBF_OPEN));
#else
	return(du_error(duv_errcb, E_DU2410_GEN_CL_FAIL,
			 4, 0, DUV_SIOPEN, 0, DUV_DBF_OPEN));
#endif	/* PHASE50 */

    return(E_DU_OK);
}


/*{
** Name: duv_f_close	- close the given file.
**
** Description:
**        This routine is called to close the CONVTO60 audit/status file
**	or the CONVTO60 log file for a given database.  Note, if the file
**	is not open, this routine is a no-op.
**
** Inputs:
**      duv_errcb                       Error control block.
**	duv_f				CONVTO60 file descriptor.
**	    .du_file			Pointer for generic file descriptor.
**					If NULL, then the given file is not
**					open.
**
** Outputs:
**      *duv_errcb                      If an error occurs, this struct is
**					set by calling du_error().
**	*duv_f
**	    .du_file			Upon successfully closing the given file
**					this field is set to NULL.
**	Returns:
**	    E_DU_OK			Completed successfully.
**	  5.0 Errors:
**	    E_DU410_GEN_CL_FAIL		A CL routine failed.
**	  6.0 Errors:
**	    E_DU2410_GEN_CL_FAIL	A CL routine failed.
**	Exceptions:
**	    none
**
** Side Effects:
**	      Close the given file.
**
** History:
**      25-Mar-88 (ericj)
**          Initially commented.
[@history_template@]...
*/
DU_STATUS
duv_f_close(duv_f, duv_errcb)
DU_FILE		*duv_f;
DU_ERROR	*duv_errcb;
{
    if (duv_f->du_file != NULL)
    {
	/* The file should be open. */
	if ((duv_errcb->du_clerr = SIclose(duv_f->du_file)) != OK)
#ifdef	PHASE50
	    return(duv_err50(duv_errcb, E_DU410_GEN_CL_FAIL, duv_errcb->du_clerr,
			    2, PV_STR, DUV_SICLOSE, PV_STR, DUV_F_CLOSE));
#else
	    return(du_error(duv_errcb, E_DU2410_GEN_CL_FAIL,
			    4, 0, DUV_SICLOSE, 0, DUV_F_CLOSE));
#endif	/* PHASE50 */
	else
	    /* Record that the file has been closed. */
	    duv_f->du_file  = NULL;
    }

    return(E_DU_OK);
}



/*{
** Name: duv_prnt_msg	- Print message to specified files.
**
** Description:
**        This routine prints the given message to the specified files.
**
** Inputs:
**	files_flag			Bit map of which files the message is
**					to be printed to.  Currently, the
**					files that a message can be printed
**					to are STDOUT and the database specific
**					log file.
**	msg				Message to be printed.
**
** Outputs:
**	none
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	      The message is printed to the specified files.
**
** History:
**      25-Mar-88 (ericj)
**          Initially commented.
[@history_template@]...
*/
VOID
duv_prnt_msg(files_flag, msg)
i4	    files_flag;
char	    *msg;
{
    /* Print to standard output? */
    if (files_flag & DUV_STDOUT)
    {
	SIprintf("%s", msg);
	SIflush(stdout);
    }

    if ((files_flag & DUV_DBF) && (Duv_dbf.du_file != NULL))
    {
	SIfprintf(Duv_dbf.du_file, "%s", msg);
	SIflush(Duv_dbf.du_file);
    }

    return;
}



#ifdef	PHASE50
/*{
** Name: duv_iw_msg - Format an information or warning message and print
**		      to specified files. (5.0 version)
**
** Description:
**        This routine formats an information or warning message and
**	prints it to the specified files.  Note, a version of this
**	routine is required for both 5.0 and 6.0 because of different
**	error-handling interfaces between the two systems.
**
** Inputs:
**      duv_errcb                       Error control block.
**      files_flag                      Bit mask specifying what files the
**					message is to be written to.
**      msg_no                          Number associated with informational
**					or warning message.
**      arg_cnt                         Count of the number of argument pairs
**					that follow.
**	arg_t1, arg1			Variable number of message parameters.
**	  . . .				Up to 5 can be given.  Arguments come
**	arg_t5, arg5			in pairs where the first part specifies
**					the argument type and the second part
**					the address of the parameter.
**
** Outputs:
**      *duv_errcb                      If and error occurs, this is struct is
**					set by calling du_error().
**	Returns:
**	    E_DU_OK			Completed successfully.
**	    E_DU414_BAD_CASE_LABEL	An incorrect number of parameters
**					were passed into this routine.
**	Exceptions:
**	    none
**
** Side Effects:
**	      A message is printed to the specified files.
**
** History:
**      25-Mar-88 (ericj)
**          Initially commented.
[@history_template@]...
*/
DU_STATUS
duv_iw_msg(duv_errcb, files_flag, msg_no, arg_cnt,
	   arg_t1, arg1, arg_t2, arg2, arg_t3, arg3, arg_t4, arg4, arg_t5, arg5)
DU_ERROR	*duv_errcb;
i4		files_flag;
DU_STATUS	msg_no;
i4		arg_cnt;
i2		arg_t1;
char		*arg1;    
i2		arg_t2;
char		*arg2;    
i2		arg_t3;
char		*arg3;    
i2		arg_t4;
char		*arg4;    
i2		arg_t5;
char		*arg5;    
{
    char	    *msg_ptr;

    switch(arg_cnt)
    {
	case 0:
	    duv_err50(duv_errcb, msg_no, DU_NOCLERR, arg_cnt);
	    break;

	case 1:
	    duv_err50(duv_errcb, msg_no, DU_NOCLERR, arg_cnt,
		      arg_t1, arg1);
	    break;

	case 2:
	    duv_err50(duv_errcb, msg_no, DU_NOCLERR, arg_cnt,
		      arg_t1, arg1, arg_t2, arg2);
	    break;

	case 3:
	    duv_err50(duv_errcb, msg_no, DU_NOCLERR, arg_cnt,
		      arg_t1, arg1, arg_t2, arg2, arg_t3, arg3);
	    break;

	case 4:
	    duv_err50(duv_errcb, msg_no, DU_NOCLERR, arg_cnt,
		      arg_t1, arg1, arg_t2, arg2, arg_t3, arg3, arg_t4, arg4);
	    break;

	case 5:
	    duv_err50(duv_errcb, msg_no, DU_NOCLERR, arg_cnt,
		      arg_t1, arg1, arg_t2, arg2, arg_t3, arg3,
		      arg_t4, arg4, arg_t5, arg5);
	    break;

	default:
	    return(duv_err50(duv_errcb, E_DU414_BAD_CASE_LABEL, DU_NOCLERR, 2,
			     PV_INT, arg_cnt,
			     PV_STR, "duv_iw_msg()"));

    }

    if (duv_errcb->du_status == E_DU_IERROR
	|| duv_errcb->du_status == E_DU_UERROR
       )
	return(duv_errcb->du_status);

    msg_ptr = STchr(duv_errcb->du_errmsg, '\n');
    if (msg_ptr != NULL)
	msg_ptr++;
    else
	msg_ptr = duv_errcb->du_errmsg;

    duv_prnt_msg(files_flag, msg_ptr);
    duv_reset_err(duv_errcb);

    return(E_DU_OK);
}

#else
/*{
** Name: duv_iw_msg - Format an information or warning message and print
**		      to specified files. (6.0 version)
**
** Description:
**        This routine formats an information or warning message and
**	prints it to the specified files.  Note, a version of this
**	routine is required for both 5.0 and 6.0 because of different
**	error-handling interfaces between the two systems.
**
** Inputs:
**      duv_errcb                       Error control block.
**      files_flag                      Bit mask specifying what files the
**					message is to be written to.
**      msg_no                          Number associated with informational
**					or warning message.
**      arg_cnt                         Count of the number of argument pairs
**					that follow.
**	arg_l1, arg1			Variable number of message parameters.
**	  . . .				Up to 5 can be given.  Arguments come
**	arg_l5, arg5			in pairs where the first part specifies
**					the argument length and the second part
**					the address of the parameter.  Note,
**					an argument length of 0 associated with
**					a string type indicates variable
**					length string that is null-terminated.
**
** Outputs:
**      *duv_errcb                      If and error occurs, this is struct is
**					set by calling du_error().
**	Returns:
**	    E_DU_OK			Completed successfully.
**	    E_DU414_BAD_CASE_LABEL	An incorrect number of parameters
**					were passed into this routine.
**	Exceptions:
**	    none
**
** Side Effects:
**	      A message is printed to the specified files.
**
** History:
**      25-Mar-88 (ericj)
**          Initially commented.
[@history_template@]...
*/
/*ARGSUSED*/
/*VARARGS2*/
DU_STATUS
duv_iw_msg(duv_errcb, files_flag, msg_no, arg_cnt,
	   arg_l1, arg1, arg_l2, arg2, arg_l3, arg3, arg_l4, arg4, arg_l5, arg5)
DU_ERROR	*duv_errcb;
i4		files_flag;
DU_STATUS	msg_no;
i4		arg_cnt;
i2		arg_l1;
char		*arg1;    
i2		arg_l2;
char		*arg2;    
i2		arg_l3;
char		*arg3;    
i2		arg_l4;
char		*arg4;    
i2		arg_l5;
char		*arg5;    
{
    char	    *msg_ptr;

    /* Prevent the error handler from automatically printing the warning or
    ** information message and from resetting the error control block.  This
    ** routine will print the message to the appropriate files and then
    ** reset the error control block itself.
    */
    duv_errcb->du_flags |= DU_SAVEMSG;

    switch(arg_cnt/2)
    {
	case 0:
	    du_error(duv_errcb, msg_no, arg_cnt);
	    break;

	case 1:
	    du_error(duv_errcb, msg_no, arg_cnt,
		      arg_l1, arg1);
	    break;

	case 2:
	    du_error(duv_errcb, msg_no, arg_cnt,
		      arg_l1, arg1, arg_l2, arg2);
	    break;

	case 3:
	    du_error(duv_errcb, msg_no, arg_cnt,
		      arg_l1, arg1, arg_l2, arg2, arg_l3, arg3);
	    break;

	case 4:
	    du_error(duv_errcb, msg_no, arg_cnt,
		      arg_l1, arg1, arg_l2, arg2, arg_l3, arg3, arg_l4, arg4);
	    break;

	case 5:
	    du_error(duv_errcb, msg_no, arg_cnt,
		      arg_l1, arg1, arg_l2, arg2, arg_l3, arg3,
		      arg_l4, arg4, arg_l5, arg5);
	    break;

	default:
	    return(du_error(duv_errcb, E_DU2414_BAD_CASE_LABEL, 4,
			     sizeof (arg_cnt), &arg_cnt,
			     0, "duv_iw_msg()"));

    }

    if (duv_errcb->du_status == E_DU_IERROR
	|| duv_errcb->du_status == E_DU_UERROR
       )
	return(duv_errcb->du_status);

    msg_ptr = STchr(duv_errcb->du_errmsg, '\t');
    if (msg_ptr != NULL)
	msg_ptr++;
    else
	msg_ptr = duv_errcb->du_errmsg;

    duv_prnt_msg(files_flag, msg_ptr);
    du_reset_err(duv_errcb);

    return(E_DU_OK);
}
#endif	/* PHASE50 */
