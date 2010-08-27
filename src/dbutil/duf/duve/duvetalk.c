/*
** Copyright (c) 1988, 2010 Ingres Corporation
*/
#include	<compat.h>
#include	<gl.h>
#include	<me.h>
#include	<sl.h>
#include	<iicommon.h>
#include	<dbdbms.h>
#include	<er.h>
#include	<lo.h>
#include	<qu.h>
#include	<si.h>
#include	<st.h>
#include	<nm.h>
#include	<cv.h>

#include	<duerr.h>

#include	<duustrings.h>
#include	<duvstrings.h>
#include	<duvfiles.h>
#include	<duve.h>
/**
**
**  Name: DUVETALK.C - Collection of routines that operate on the
**		       verifydb log file and communication to/from
**		       stdout.
**
**  Description:
**        This module contains a collection of routines that operate
**	on the verifydb log and communication to user.
**
**	    duve_fopen	    -	    Open the verifydb log file.
**	    duve_fclose	    -	    Close verifydbv log file.
**	    duve_talk	    -	    handle verifydb I/O to file and/or stdout,
**				    also accept interactive user inputs, mode
**				    permitting.
**	    duve_log	    -	    log string to verifydb log file.
**	    duve_banner	    -	    make duve test banner
**
**
**  History:    
**      14-Jul-1988 (teg)
**          Initial creation for verifydb
**	8-Mar-89 (teg)
**	    initialize DU_ERROR member of DUVE_CB before each call to du_error()
**	8-aug-93 (ed)
**	    unnest dbms.h
**	10-jan-94 (andre)
**	    add a forward declaration for duve_suppress() and make it static
**      28-sep-96 (mcgem01)
**              Global data moved to duvedata.c
**	25-feb-1998 (somsa01)
**	    In duve_talk(), if SIgetrec() returns ENDFILE, keep trying
**	    until it's not. This means that stdin is a file.
**      21-apr-1999 (hanch04)
**          replace STindex with STchr
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      29-Nov-1999 (hanch04)
**          First stage of updating ME calls to use OS memory management.
**          Make sure me.h is included.  Use SIZE_TYPE for lengths.
**	14-Sep-2000 (hanje04)
**	    Removed 'dummy routine to make included header file duvfiles.h
**	    happy', because it was causing problems building iimerge libraries
**	    as shared objects. No longer seems to effect build process.
**	19-jan-2005 (abbjo03)
**	    Remove Duv_statf, Duv_dbf and Duv_conv_status.
**      18-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
**     16-Feb-2010 (hanje04)
**         SIR 123296
**         Add LSB option, writable files are stored under ADMIN, logs under
**         LOG and read-only under FILES location.
**	13-Jan-2010 (wanfr01) Bug 123139
**	    Include cv.h for function defintions
**/

/*
**  Forward and/or External function references.
*/

FUNC_EXTERN DU_STATUS duve_fopen();     /* open verifydb log file */
FUNC_EXTERN DU_STATUS duve_fclose();    /* close verifydb log file */
FUNC_EXTERN DU_STATUS duve_talk();      /* communicate to user and/or log */
FUNC_EXTERN DU_STATUS duve_log();       /* write entry to log file */
static DU_STATUS duve_error(DU_ERROR	    *du_errcb,
			    bool	    du_printmsg,
			    i4	    du_errno,
			    i4		    pcnt,
			    ER_ARGUMENT	    *param);	/* format messages */
static VOID duve_reset_err(DU_ERROR *du_errcb); /* clear message buffer */
static bool duve_suppress(DU_STATUS message_id, DUVE_CB *duve_cb);

/*
**  Defines of other constants.
*/

/*
**      Strings used in error messages to specify what routine the error
**	occurred in.
*/

#define			DUVE_FOPEN	    "duve_fopen()"
#define			DUVE_FCLOSE	    "duve_fclose()"
#define			DUVE_TALK	    "duve_talk()"


/*{
** Name: duve_fopen - open the verifydb log file.
**
** Description:
**        This routine opens the database specific log file so the
**	events that occur during the verification of a database can be
**	recorded.  The log file will be placed into
**	    ii_config:iivdb.log
**
**	or
**	    ii_system:[ingres.files]iivdb.log
**
**
** Inputs:
**	duve_cb				Verifydb Control Block.  Contains:
**	    duve_msg			    Message control block.
**	    duve_log			    File Control Block for log file.
** Outputs:
**	duve_cb				Verifydb Control Block:
**	    *duv_errcb                      If an error occurs, this struct is
**					    set by calling duve_error().
**	Returns:
**	    E_DU_OK			Completed successfully.
**	    E_DU2410_GEN_CL_FAIL	A CL routine failed.
**	    E_DU3030_NO_LOGNAME		No translation exists for the Ingres
**					environment variable 'II_CONFIG'.
**	Exceptions:
**	    none
**
** Side Effects:
**	      Opens the verifydb log file.
**	      If it encounters an error, it formats the error msg, but does
**		not output it.  The formatted error msg is returned in the
**		duve_cb->duve_msg.
**
** History:
**      14-Jul-1988 (teg)
**          Created for verifydb
**	25-apr-1990 (teg)
**	    use NMloc instead of NMgtat
**	28-jan-1990 (teresa)
**	    set world read/write access to iivdb.log to fix bug 35248
**	08-jun-93 (teresa)
**	    add support for user specified log file name.
**	15-jul-93 (teresa)
**	    fixed unix compiler warnings.
**	06-aug-07 (smeke01) b118877
**	    Remove world rw permission setting on log file. Ensure 
**	    that if the path references something that already exists 
**	    then that something must be a file and not (eg) a link.
**      20-aug-2007 (horda03) Bug 118877
**          FIx of 06-aug-07 accessed fields within a LOCATION. This is
**          not portable, as different OS use different field names.
**          Corrected to use LOtos() function.
**	31-jan-07 (smeke01) b118877
**	    Improved error message for log file open error. Now 
**	    the full path to the file is displayed to the user.
*/
DU_STATUS
duve_fopen(duve_cb)
DUVE_CB		*duve_cb;
{
    char	    fil_nam[DUV_FMAXNAME+1];
    char	    *cp;
    DU_FILE	    *fp;
    ER_ARGUMENT	    err_param[2];
    LOINFORMATION loinf;
    i4 flagword;


    if (duve_cb->duve_skiplog)
	return( (DU_STATUS) E_DU_OK);

    fp = &duve_cb->duve_log;

    /* Initialize the file location if necessary. */
    if (duve_cb->duve_log.du_flocbuf[0] == EOS)
    {
	char	*cp;
        /* Make a location to the user codename mapping file. */
        if (duve_cb->duve_altlog)
	{
	    /* special log file name was specified, so use it */
	    duve_cb->duve_msg->du_clerr = NMloc(LOG, FILENAME,
			    		duve_cb->duve_lgfile, &(fp->du_floc) );
	}
	else
	{
	    /* use default name */
	    duve_cb->duve_msg->du_clerr = NMloc(LOG, FILENAME,
						"iivdb.log",&(fp->du_floc) );
	}
						
	if (duve_cb->duve_msg->du_clerr != OK )
	{
	    err_param[0].er_size  = (i4) 0;
	    err_param[0].er_value = (PTR) ERx("NMloc()");
	    err_param[1].er_size  = (i4) 0;
	    err_param[1].er_value = (PTR) ERx("DUVE_FOPEN");
            return(duve_error(duve_cb->duve_msg, TRUE, 
			      E_DU2410_GEN_CL_FAIL, 2, err_param));
	}

        LOtos(&(fp->du_floc), &cp);
        STcopy(cp,fp->du_flocbuf);
    }

    /*
    ** Check whether the path is a file & not (eg) a link. 
    ** If it's not a file, bail out.
    */
    flagword = (LO_I_XTYPE | LO_I_LSTAT);
    if (LOinfo(&(fp->du_floc), &flagword, &loinf) == OK)
    {
	if (!(loinf.li_type & LO_IS_FILE))
	{
	    char temppath[MAX_LOC +1];
            char *path;

	    /* 
	    ** need to use temppath because duve_error() 
	    ** overwrites duve_cb->duve_log.du_floc.path
	    */
            LOtos( &duve_cb->duve_log.du_floc, &path);

	    STlcopy(path, temppath, MAX_LOC);
	    err_param[0].er_size  = (i4) 0; 
	    err_param[0].er_value = (PTR) temppath;
	    return(duve_error(duve_cb->duve_msg, TRUE, E_DU503E_INVALID_LOG_FILE, 1, err_param));
	}
    }

    /* 
    ** Now try to open the file.  This open will create the file if it 
    ** does not exist and append to it if it does exist 
    */
    if ( (duve_cb->duve_msg->du_clerr = SIopen(&duve_cb->duve_log.du_floc, "a", &duve_cb->duve_log.du_file)) != OK )
    {
	char temppath[MAX_LOC +1];
	char *path;

	/* 
	** need to use temppath because duve_error() 
	** overwrites duve_cb->duve_log.du_floc.path
	*/
	LOtos( &duve_cb->duve_log.du_floc, &path);

	STlcopy(path, temppath, MAX_LOC);
	err_param[0].er_size  = (i4) 0; 
	err_param[0].er_value = (PTR) temppath;
	return(duve_error(duve_cb->duve_msg, TRUE, E_DU5023_CANT_OPEN_LOG_FILE, 1, err_param));
    }

    return( (DU_STATUS) E_DU_OK);
}

/*{
** Name: duve_fclose	- close the given file.
**
** Description:
**        This routine is called to close the verifydb log file.  Note, if 
**	  the file is not open, this routine is a no-op.
**
** Inputs:
**	duve_cb			    Verifydb control block.  Contains:
**	    duve_msg			Message control block.
**	    duve_loc			Verifydb log file descriptor.
**		.du_file		Pointer for generic file descriptor.
**					If NULL, then the given file is not
**					open.
**
** Outputs:
**	duve_cb			    Verifydb control block:
**	    duv_errcb			If an error occurs, this struct is
**					set by calling du_error().
**	    duv_f
**		.du_file		Upon successfully closing the given file
**					this field is set to NULL.
**	Returns:
**	    E_DU_OK			Completed successfully.
**	    E_DU2410_GEN_CL_FAIL	A CL routine failed.
**	Exceptions:
**	    none
**
** Side Effects:
**	      Close the given file.
**
** History:
**      14-Jul-1988
**          Created for verifydb
**	15-jul-93 (teresa)
**	    fixed unix compiler warnings.
*/
DU_STATUS
duve_fclose(duve_cb)
DUVE_CB		*duve_cb;
{
    ER_ARGUMENT	    err_param[2];

    if (duve_cb->duve_log.du_file != NULL)
    {
	/* The file should be open. */
	if ((duve_cb->duve_msg->du_clerr=SIclose(duve_cb->duve_log.du_file))!=OK)
	{
	    err_param[0].er_size  = (i4) 0;
	    err_param[0].er_value = (PTR) DUV_SICLOSE;
	    err_param[1].er_size  = (i4) 0;
	    err_param[1].er_value = (PTR) DUVE_FCLOSE;
            return(duve_error(duve_cb->duve_msg, TRUE,
			      E_DU2410_GEN_CL_FAIL, 2, err_param));
	}
	else
	    /* Record that the file has been closed. */
	    duve_cb->duve_log.du_file  = NULL;
    }

    return( (DU_STATUS) E_DU_OK);
}

/*{
** Name: duve_talk	- handle verifydb I/O
**
** Description
**
**	This routine handles output for verifydb.  It examines message type
**	and mode to determine what I/O actions to take.  Possible I/O actions
**	are:
**
**	    FORMAT: format msg from parameters
**	    PROMPT: Create/format a prompt msg, prompt user and accept input
**	    IOMSG:  Create/format a msg that the fix not permitted this mode
**	    STDOUT: output formatted msg to stdout
**	    LOG:    output formatted msg to verifydb log file
**	    RETURN: status returned from (DUVE_BAD, DUVE_NO, DUVE_YES)
**
**	Message types are:
**	    DUVE_ALWAYS:	always output to stdout and verifydb log
**	    DUVE_DEBUG:		output only to stdout
**	    DUVE_MODESENS:	output to stdout if mode permits, always to log
**	    DUVE_IO:		output to stdout if mode = RUNINTERACTIVE then
**				 prompt for yes/no.
**	    DUVE_ASK:	        output to stdout and prompt for yes/no
**
**	This routine handles the mode-dependent prompting the user.  Possible
**	modes are:
**		run	      -	Inform user of suggested action, then take it.
**		runsilent     -	do not inform user of suggested action, but
**				take it.
**		runinteractive- Inform user of suggested action, then prompt
**				the user for yes/no permission.  If permission
**				is "yes", take action.
**		report	      -	Inform user of suggested action, but do not take
**				it.
**	The way this all fits together is:
**  
**  :-MSG TYPE---:  :------------------- VERIFYDB MODE ------------------------:
**		    DUVE_REPORT:    DUVE_RUN:	    DUVE_IRUN:	    DUVE_SILENT:
**  
**  DUVE_ALWAYS:    format	    format	    format	    format
**  (err msgs)	    stdout	    stdout	    stdout	    stdout
**  (legal warning) log		    log		    log		    log
**		    return(no,err)  return(no,err)  return(no,err)  return(no,err)
**
**  DUVE_DEBUG:	    format	    format	    format	    format
**  (-d flag)	    stdout	    stdout	    stdout	    stdout
**		    return(no,err)  return(no,err)  return(no,err)  return(no,err)
**
**  DUVE_MODESENS:  format	    format	    format	    format
**  (report warn    stdout	    stdout	    stdout	    log
**   or inform)	    log		    log		    log		    return(no,err)
**		    return(no,err)  return(no,err)  return(no,err)  
**
**  DUVE_ASK:	    format	    return(yes,err) format	    return(yes,err)
**  (ask regular    stdout	    	            stdout
**  repairs)	    log				    prompt
**		    return(no,err)		    return(y/n/err)
**
**  DUVE_IO:	    format	    format	    format	    format
**  (ask critical   stdout	    stdout	    stdout	    log
**  repairs)	    log		    log		    prompt	    io_msg
**		    io_msg	    io_msg	    stdout	    log
**		    stdout	    stdout	    return(y/n/err) return(no,err)
**		    log		    log
**		    return(no,err)  return(no,err)
**
**  DUVE_LEGAL	    format	    format	    format	    format
**		    stdout	    stdout	    stdout	    log
**		    log		    log		    log		    return yes
**		    return(yes,err) return(yes,err) special prompt
**						    return(y/n/err)
**
**  NOTES:  This routine assumes that the log file is open before it is called.
**
**
** Inputs:
**	duve_mtype			verifydb message type:
**					    DUVE_ALWAYS
**					    DUVE_DEBUG
**					    DUVE_MODESENS
**					    DUVE_IO
**					    DUVE_ASK
**	duve_cb				verifydb control block.  Contains
**					 mode and verifydb error control block
**					 and flag indicating if logging is
**					 bypassed
**      du_errno                        error code number indicating which
**					 error message to format
**	p_cnt				#parameters (5 max)
**	(note: parameters optional, but must match p_cnt)
**	p1_len				#bytes length of 1st parameter --
**					 null terminated strings should have
**					 a length of zero
**	p1_ptr				address of 1st parameter
**	p2_len				#bytes length of parameter #2 --
**					 null terminated strings should have
**					 a length of zero
**	p2_ptr				address of 2nd parameter
**	p3_len				#bytes length of parameter #3 --
**					 null terminated strings should have
**					 a length of zero
**	p3_ptr				address of 3rd parameter
**	p4_len				#bytes length of parameter #4 --
**					 null terminated strings should have
**					 a length of zero
**	p4_ptr				address of 4th parameter
**	p5_len				#bytes length of parameter #5 --
**					 null terminated strings should have
**					 a length of zero
**	p5_ptr				address of 5th parameter
**
**
** Outputs:
**      duve_dbv                        Verifydb control block.  Contains:
**	    duve_msg->du_errmsg		    error message that was output
**	    duve_msg->du_utilerr	    error message id that was output
**
**	Returns:
**	    DUVE_BAD
**	    DUVE_NO
**	    DUVE_YES
**	Exceptions:
**	    none
**
** Side Effects:
**	    msg may be redefined to error message
**	    disables du_error from automatically outputting warning/error
**	      msgs and resetting control block.
**
** History:
**      27-Jun-1988 (teg)
**          initial creation for verifydb
**	27-Sep-1988 (teg)
**	    changed design for outputting suggested repair and info that
**	    the repair is only permitted in DUVE_IO mode.
**	19-Jan-1989 (teg)
**	    added support for new DUVE_LEGAL mode.
**      23-jan-1989 (roger)
**          It is illegal to peer inside the CL error descriptor.
**	8-Mar-89 (teg)
**	    initialize DU_ERROR member of DUVE_CB before each call to du_error()
**	09-jun-93 (teresa)
**	    add support for -test flag.  If this flag is specified, then supress
**	    output to stdout.
**	25-feb-1998 (somsa01)
**	    If SIgetrec() returns ENDFILE, keep trying until it's not.
**	    This means that stdin is a file.
**      22-Jul-2010 (coomi01) b124111
**          Use lower case buffer in test for a 'No' reply.
*/

/* VARARGS4 */
DU_STATUS
duve_talk(duve_mtype, duve_cb, du_errno, pcnt, p1_len, p1_ptr, 
	     p2_len, p2_ptr, p3_len, p3_ptr, p4_len, p4_ptr, p5_len, p5_ptr)
i4		    duve_mtype;	    
DUVE_CB		    *duve_cb;
i4		    du_errno;
i4		    pcnt;
i4		    p1_len;
i4		    *p1_ptr;
i4		    p2_len;
i4		    *p2_ptr;
i4		    p3_len;
i4		    *p3_ptr;
i4		    p4_len;
i4		    *p4_ptr;
i4		    p5_len;
i4		    *p5_ptr;

{

    ER_ARGUMENT		param[DUVE_MX_MSG_PARAMS];
    i4             retstat;    /* return status: DUVE_BAD, DUVE_NO,
					DUVE_YES */
    STATUS		status;

    /*
    *************************************
    ** dont format certain msg_types   **
    *************************************
    */

    if (duve_mtype == DUVE_ASK)
    {	
	if ( (duve_cb->duve_mode == DUVE_SILENT) ||
	     (duve_cb->duve_mode == DUVE_RUN) )
	    return ((DU_STATUS) DUVE_YES);
    }


    /*
    **************************************
    **  format msg text			**
    **************************************
    */

    retstat = DU_INVALID;

    /*
    ** call duve_error() to format message and put into message buffer. --
    **  1st - initialize message buffer
    **	2nd - verify we have a reasonable number of arguments, since arguments
    **	      come in pairs (a length and a data pointer), assure there are an
    **	      even number of arguments.
    **  3rd - Stuff the arguments into the appropriate structure
    **  4th - call duve_error() to format message and to handle any obscure
    **	      formatting errors.
    */

    duve_reset_err(duve_cb->duve_msg);    

    if ((pcnt % 2) == 1 || pcnt > (DUVE_MX_MSG_PARAMS*2) )
    {
	du_errno    = E_DU2001_BAD_ERROR_PARAMS;
	pcnt	    = 0;
    }
    else
	pcnt = pcnt/2;

    param[0].er_size  = p1_len;
    param[0].er_value = (PTR) p1_ptr;
    param[1].er_size  = p2_len;
    param[1].er_value = (PTR) p2_ptr;
    param[2].er_size  = p3_len;
    param[2].er_value = (PTR) p3_ptr;
    param[3].er_size  = p4_len;
    param[3].er_value = (PTR) p4_ptr;
    param[4].er_size  = p5_len;
    param[4].er_value = (PTR) p5_ptr;

    (VOID) duve_error(duve_cb->duve_msg, FALSE, du_errno, pcnt, param);

    /* now figure out if additional message formatting may be required.
    ** the criteria is as follows:
    **	    none - if there is an error formatting a message
    **	    none - if msg type is DEBUG, ERROR or MODESENSITIVE
    **	    none - if msg type is always and mode is NOT RUNINTERACTIVE
    **		   (note, duve_talk has already exited with DUVE_YES if mode
    **		    was RUN or RUNSILENT)
    **	    prompt - if msg type is DUVE_ASK or DUVE_IO & mode is RUNINTERACTIVE
    **	    special
    **	     prompt- if msg type is DUVE_LEGAL and mode is RUNINTERACTIVE
    ** retstat is set for any of the no additional message formatting cases
    */
    if (duve_cb->duve_msg->du_utilerr != du_errno)
	retstat  = DUVE_BAD;
    else if (duve_cb->duve_test)
    {
	if ( (duve_cb->duve_mode == DUVE_IRUN) || (duve_mtype == DUVE_LEGAL) )
	    retstat = DUVE_YES;
	else
	    retstat = DUVE_NO;
    }
    else if (duve_mtype <= DUVE_MODESENS)
	retstat = DUVE_NO;
    else if ( (duve_cb->duve_mode == DUVE_REPORT) && (duve_mtype == DUVE_ASK))
	retstat = DUVE_NO;
    else if ( (duve_mtype == DUVE_LEGAL) && (duve_cb->duve_mode != DUVE_IRUN) )
	retstat = DUVE_YES;
    /*
    **************************************
    **  output msg text			**
    **************************************
    */

    /*
    **	 if msg type is DUVE_ALWAYS (reports errors), DUVE_LEGAL or DUVE_DEBUG,
    **	 always output msg to stdout.  Otherwise, output msg to stdout if
    **	 mode is not RUNSILENT.  Also, if duve_cb->duve_test (ie, -test flag
    **   was specified, the surpress output to stdout.
    */
    if ( (duve_cb->duve_mode != DUVE_SILENT) || (duve_mtype == DUVE_DEBUG) ||
	 (duve_mtype == DUVE_ALWAYS) || (duve_mtype == DUVE_LEGAL) )
    {
	if ( ! duve_cb->duve_test)
	{
	    SIprintf("%s\n",duve_cb->duve_msg->du_errmsg);
	    SIflush(stdout);
	}
    }

    /*
    ** Determine whether or not to output msg to log file.  The criteria
    ** is as follows:
    **	  if mode is DUVE_DEBUG do not log
    **	  if msg type is DUVE_ASK and mode is not REPORT do not log
    **    (if msg type was DUVE_ASK and mode was RUN or RUNSILENT, duve_talk
    **		returned a DUVE_YES before it got here)
    **	  if msg type is DUVE_IO and mode is RUNINTERACTIVE, do not log
    **    ALL OTHER conditions are logged.
    **
    **	By the way, if the log file doesn't exist, we skip logging regardless of
    **	any of these conditions.
    */
    if (
	 (duve_cb->duve_log.du_file != NULL) /* see if log file exists */
	||
	 (duve_cb->duve_sep)		     /* or we're running in SEP mode */
       )
    {
	if ( (duve_mtype == DUVE_ALWAYS) || (duve_mtype == DUVE_MODESENS) ||
	     (duve_mtype == DUVE_LEGAL) ||
	    ((duve_mtype==DUVE_ASK) && (duve_cb->duve_mode==DUVE_REPORT)) ||
	    ((duve_mtype==DUVE_IO) && (duve_cb->duve_mode != DUVE_IRUN)) )
	{
	    duve_log(duve_cb, du_errno, duve_cb->duve_msg->du_errmsg);
	}
    }

    if (retstat == DU_INVALID)
    {
	/* either
	**  1.  mode is RUNINTERACTIVE and user must be prompted or
	**  2.	the message type requires RUNINTERACTIVE mode for a corrective
	**	action and the mode is not RUNINTERACTIVE
	**  3.  we are outputting a legal warning in RUNINTERACTIVE mode and
	**	require an input from the user before we can continue
	*/

	/* check for runinteractive required and not available */
	
	if ( (duve_cb->duve_mode != DUVE_IRUN) && (duve_mtype == DUVE_IO))
	{
	    duve_error( duve_cb->duve_msg, FALSE,
			S_DU17FF_FIX_INTERACTIVELY,  0, param);
	    /* output to stdout unless runsilent */
	    if (duve_cb->duve_mode != DUVE_SILENT)
	    {
		SIprintf ("%s\n",duve_cb->duve_msg->du_errmsg);
	    }
	    duve_log (duve_cb, du_errno, duve_cb->duve_msg->du_errmsg);
	    retstat = DUVE_NO;
	}

        /*
        *****************************************
        **  prompt for yes/no permission and   **
        **  determine return status of:        **
        **  DUVE_YES, DUVE_NO, DUVE_BAD	       **
        *****************************************
        */

	else if ( ( ((duve_mtype == DUVE_ASK) || (duve_mtype == DUVE_IO)) &&
		  (duve_cb->duve_mode == DUVE_IRUN) ) ||
		  ( (duve_mtype == DUVE_LEGAL) &&
		    (duve_cb->duve_mode == DUVE_IRUN) ) )
		  
	{   
#define	MAXLINE		128
	    char                reply[MAXLINE],
				reply1[MAXLINE];
	    if (duve_mtype == DUVE_LEGAL)
	    {
		if (duve_error( duve_cb->duve_msg, TRUE,
				S_DU04FF_CONTINUE_PROMPT, 0, param))
		    return ( (DU_STATUS) DUVE_BAD);
	    }
	    else
	    {
		if (duve_error( duve_cb->duve_msg, TRUE, 
				S_DU0300_PROMPT, 0, param))
		    return ( (DU_STATUS) DUVE_BAD);
	    }
	    retstat = DUVE_NO;

	    for(;;)
	    {
		SIprintf ("%s\n",duve_cb->duve_msg->du_errmsg);
		if (duve_cb->duve_simulate == TRUE)
		{
		    /* simulate a YES answer from user */
		    retstat = DUVE_YES;
		    break;
		}
		while ( (status = SIgetrec(reply, MAXLINE, stdin)) == ENDFILE );
		if (status != OK)
		    break;
		STzapblank(reply, reply1);
		CVlower(reply1);

		if ( !STcompare("yes", reply1) ||
		    (reply1[0] == 'y' && reply1[1] == '\0') )
		{
		    retstat = DUVE_YES;
		    break;
		}

		if ( !STcompare("no", reply1) || 
		    (reply1[0] == 'n' && reply1[1] == '\0') )
		{
		    break;
		}

	    }/*_for */
	} /* _if in an interactive mode for an interactive msg */
	else
	    retstat = DUVE_NO;
    }

    return ( (DU_STATUS) retstat);
}

/*{
** Name: duve_log	- Log string to verifydb log file
**
** Description
**	This routine outputs a null terminated string to the verifydb log
**	file.  The file must be open before this routine is called.  The
**	character string must be null terminated.
**
**	WARNING:  this routine assumes message length is not too long for
**		  a message.  It also assumes (does not take the time to
**		  verify) that the msg buffer is null terminated.  However,
**		  it does verify that the log file is open.
**
**
** Inputs:
**	duve_cb				verifydb control block.  Contains
**	    duve_log			    verifydb log file descriptor
**	msg_id				message id
**	msg_buffer			contains null terminated msg to output
**
** Outputs:
**	none
**	Returns:
**	    DUVE_BAD	    (log file was not open)
**	    DUVE_YES
**	Exceptions:
**	    none
**
** Side Effects:
**	      msg is output to buffer
**
** History:
**      27-Jun-1988 (teg)
**          initial creation for verifydb
**	14-Jul-1993 (teresa)
**	    Add support for SEP testing to redirect log file output to STDOUT
**	10-dec-93 (teresa)
**	    added msg_id parameter and support to call duve_suppress.
*/

DU_STATUS
duve_log(duve_cb, msg_id, msg_buffer)
DUVE_CB	    *duve_cb;
i4	    msg_id;
char	    *msg_buffer;
{
    /* this is a no-op if the skip logging feature is selected via -n flag */
    if (duve_cb->duve_skiplog)
	return( (DU_STATUS) E_DU_OK);

    if (duve_suppress( (DU_STATUS) msg_id,duve_cb) )
	return ( (DU_STATUS) E_DU_OK);


    if (duve_cb->duve_sep)
    {
	/* redirect log file output to STDOUT for SEP testing */
	SIprintf(" %s\n", msg_buffer);
	SIflush(stdout);
    }
    else
    {
	/* output to log file, if it is already opened */

	if (duve_cb->duve_log.du_file == NULL)  /* no open log file */
	    return ( (DU_STATUS) DUVE_BAD);

	SIfprintf(duve_cb->duve_log.du_file, "    %s\n", msg_buffer);
	SIflush(duve_cb->duve_log.du_file);
    }

    return ( (DU_STATUS) DUVE_YES);
}

/*{
** Name: duve_banner	- Make banner for verifydb test
**
** Description
**	This routine outputs a null terminated string to the verifydb log
**	file.  That string indentifies a verifydb test.  (The file must be 
**	open before this routine is called.
**
**
** Inputs:
**	test_id				test_identifier
**	duve_cb				verifydb control block.  Contains
**	    duve_log			    verifydb log file descriptor
**
** Outputs:
**	none
**	Returns:
**	    DUVE_BAD	    (could not make banner, usually cuz log file was 
**			     not open)
**	    DUVE_YES
**	Exceptions:
**	    none
**
** Side Effects:
**	      banner is output to log file
**
** History:
**      07-Jun-1989 (teg)
**          initial creation for verifydb
**	06-may-1993 (teresa)
**	    Changed banner to indicate catalog name as well as test number;
**	    changed to input test number as an integer rather than as an
**	    ASCII constant string.
*/

DU_STATUS
duve_banner( char *cat_id, i4  test_id, DUVE_CB *duve_cb)
{
    char	buf[MAXLINE];
    char	*ptr;
    char	id_text[8];

    /* convert test id to ascii */
    MEfill( (u_i2) sizeof(id_text), ' ', id_text);
    id_text[7]='\0';	/* null terminate */
    CVna (test_id, id_text);
    (VOID) STtrmwhite(id_text);

    /* build verifydb banner */
    ptr = STpolycat(5, DUVE_0BANNER, cat_id, DUVE_1BANNER, id_text, 
		    DUVE_2BANNER, buf);

    /* now write banner to verifydb log file */
    if (duve_log ( duve_cb, 0, ptr) == DUVE_BAD) 
	return ( (DU_STATUS) DUVE_BAD);
    else
        return ( (DU_STATUS) DUVE_YES);
}

/*{
** Name: duve_error() -   format an error message and translate to proper
**			  DU_STATUS for return.
**
** Description:
**        This routine formats an error message using ERslookup().  It
**	also translates it to the proper DU_STATUS for return.
**
** Inputs:
**      du_errcb                        Ptr to an DU_ERROR struct.
**	du_printmsg			Flag.  If TRUE, format & print to stdout
**					       If FALSE, format only
**      du_errno                        An error number (defined in duerr.h)
**	pcnt				Argument count.  This is the number
**					of elements in the param input.
**	param				array error message arguments.
**
** Outputs:
**      *du_errcb
**	    .du_errmsg			Buffer containing null-terminated
**					formatted error	message.
**	Returns:
**	    E_DU_OK			Completed successfully.
**	    E_DU_IERROR			The error number passed in was
**					generated by an internal error.
**	    E_DU_UERROR			The error number passed in was
**					generated by an user error.
**
** Side Effects:
**	     none.
**
** History:
**      07-may-93 (teresa)
**	    Created as lift and tailor from DU_ERROR()
*/
DU_STATUS
duve_error(
	    DU_ERROR	    *du_errcb,
	    bool	    du_printmsg,
	    i4	    du_errno,
	    i4		    pcnt,
	    ER_ARGUMENT	    *param)
{
    char		*msg_ptr;
    DU_STATUS           ret_status;
    CL_ERR_DESC		clerror;
    i4			rslt_msglen=0;
    i4			save_msglen;
    i4			buf_len	    = ER_MAX_LEN;
    ER_ARGUMENT		loc_param;
    i4			tmp_status;
    i4			lang_code;


    /* First get the language code. */
    ERlangcode((char *) NULL, &lang_code);

    /* Classify the severity of the error */
    if (du_errno < DU_OK_BOUNDARY)
	ret_status    = E_DU_OK;
    else if (du_errno < DU_INFO_BOUNDARY)
	ret_status    = E_DU_INFO;
    else if (du_errno < DU_WARN_BOUNDARY)
	ret_status    = E_DU_WARN;
    else if (du_errno < DU_IERROR_BOUNDARY)
	ret_status    = E_DU_IERROR;
    else
	ret_status    = E_DU_UERROR;

    du_errcb->du_status	= ret_status;

    /* Format the database utility error message */
    du_errcb->du_utilerr    = du_errno;

    tmp_status  = ERslookup(du_errno, (CL_ERR_DESC *) 0, 0, (char *) NULL,
			    du_errcb->du_errmsg, buf_len, lang_code,
			    &rslt_msglen, &clerror, pcnt, param);


    if (tmp_status != OK)
    {
	loc_param.er_size   = sizeof(du_errno);
	loc_param.er_value  = (PTR)&du_errno;

	tmp_status = ERslookup((i4) E_DU2000_BAD_ERLOOKUP,
	    (CL_ERR_DESC *)0, 0, (char *) NULL, du_errcb->du_errmsg, buf_len,
	    lang_code, &rslt_msglen, &clerror, 1, &loc_param);
	if (tmp_status != OK)
	{
	    /* err_lookup is not working.  Put something in the buffer to
	    ** avoid a silent exit 
	    */
	    STcopy ( DUU_ERRLOOKUP_FAIL, du_errcb->du_errmsg);
	    SIprintf("%s\n", du_errcb->du_errmsg);
	    SIflush(stdout);

	}
	else
	{
	    du_errcb->du_errmsg[rslt_msglen]    = EOS;
	    ret_status		= E_DU_IERROR;
	    du_errcb->du_status	= ret_status;
	    du_errcb->du_utilerr	= E_DU2000_BAD_ERLOOKUP;
	}
    }
    else
    {
	/* Format CL error (internal CL failure and/or OS error) */
	du_errcb->du_errmsg[rslt_msglen++]    = '\n';
	save_msglen  = rslt_msglen;
	buf_len	    -= rslt_msglen;
	tmp_status = ERslookup((i4) 0, &du_errcb->du_clsyserr, 0,
			       (char *) NULL, &du_errcb->du_errmsg[rslt_msglen],
			       buf_len, lang_code,
			       &rslt_msglen, &clerror, 0,(ER_ARGUMENT *) NULL);

	if (!rslt_msglen)
	    du_errcb->du_errmsg[save_msglen - 1] = ' ';    /* delete newline */

	if (tmp_status != OK)
	{
	    /* Report that the operating system error couldn't be found */
	    ERslookup((i4) E_DU2002_BAD_SYS_ERLOOKUP, (CL_ERR_DESC *) 0, 0,
		     (char *) NULL, &du_errcb->du_errmsg[save_msglen], buf_len,
		     lang_code, &rslt_msglen, &clerror, 0,
		     (ER_ARGUMENT *) NULL);
	    ret_status	= E_DU_IERROR;
	    du_errcb->du_status	= ret_status;
	    du_errcb->du_utilerr    = E_DU2002_BAD_SYS_ERLOOKUP;
	}
	du_errcb->du_errmsg[save_msglen + rslt_msglen]    = EOS;
    }

    if (du_printmsg)
    {
	/* Print the msg and reset the error control block.
	*/

	msg_ptr = du_errcb->du_errmsg;
	SIprintf("%s\n", msg_ptr);
	SIflush(stdout);
	duve_reset_err(du_errcb);
	ret_status  = E_DU_OK;
    }
    else if (ret_status == E_DU_WARN || ret_status == E_DU_INFO)
    {
	/* 
	** This is an informational message or warning, so
	** return an OK status.
	*/
	ret_status  = E_DU_OK;
    }
    else
    {
	/* Replace the TAB after the DUF facility code id with a NEWLINE. */
	msg_ptr = STchr(du_errcb->du_errmsg, '\t');
	if (msg_ptr)
	    *msg_ptr = '\n';
    }

    return(ret_status);
}

/*{
** Name: duve_reset_err()   -   reset the error-handling control block.
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
**      07-may-1993 (teresa)
**	    copied from du_reset_err
*/
VOID
duve_reset_err(DU_ERROR *du_errcb)
{
    /* Reset the system error control block */
    MEfill( (u_i2) sizeof(DU_ERROR), 0, (PTR) du_errcb);
    du_errcb->du_ingerr	    = DU_NONEXIST_ERROR;
    du_errcb->du_utilerr    = DU_NONEXIST_ERROR;

    return;
}

/*{
** Name: duve_suppress()   -   suppress specific messages 
**
** Description:
**
**      This routine determines if a specific message should be suppressed.  It
**	does this to eliminiate messages that generate generate spurious diffs 
**	during dbms testing.  It works like this  --
**	if dbms_test flag is set (nonzero), then it will supress any messages
**	in the target list except for a message with the same value as the
**	the flag.  Example:
**
**	    Message S_DU167A_WRONG_CDB_ID is an annoying spurious diff that
**	    occurs every time that someone creates a distributed DB in the
**	    installation and is unrelated to the particular verifydb test
**	    that is being run, except when the full verifydb test suite is
**	    running test iistar_cdbs test 5.  In that particular case, we do 
**	    not want this message suppressed, nor do we want the corrective
**	    action messages that accompany it (S_DU0340_UPDATE_STARCDBS,
**	    S_DU0390_UPDATE_STARCDBS) suppressed.  So the verifydb test driver
**	    would set the undocumented -dbms_test flag to 167A:
**		    -dbms_test167A
**	    this would cause duve_suppress to skip suppression of the following
**	    three messages:  S_DU0340_UPDATE_STARCDBS, S_DU0390_UPDATE_STARCDBS,
**			     S_DU167A_WRONG_CDB_ID.
**
**	If we were running verifydb apart from the test driver, then the 
**	duve_test flag would not be set and no messages would be suppressed.  
**	If someone were to run verifydb through the verifydb test driver and 
**	wanted to suppress all suprious messages (i.e., messages that cause 
**	non-significant difs), the driver should set the undocumented  
**	dbms_test flag to 1:
**                  -dbms_test1
**
**	duve_suppress returns a value of TRUE when a message is to be suppressed
**	and returns a value of FALSE when it should not be suppressed.  It uses
**	two lookup tables to make it's decisions:
**	    suppress_list - a list of warning messages to suppress
**	    suppress_map  - a map linking info messages to their corresponding
**			    warning messages.
**
**	The simplified logic is as follows:
**	    if the dbms_test flag was not specified then
**		return FALSE (we dont suppress any msgs unless flag is set)
**	    else if message_id is an error message then
**		return FALSE (we never suppress error messages)
**	    else if message_id is an info message then
**		search for message_id in suppress_map[]
**		if found then
**		    if suppress_map.warn = dbms_test flag value then 
**			return FALSE (user doesnot want this message suppressed)
**		    else
**			return TRUE (this message qualifies for suppression)
**		else
**		    return FALSE (this message does not qualify for suppression)
**	    else (this is a warning message)
**		if message_id = duve_dbms_test then
**		    return FALSE (user does not want this message suppressed)
**		else
**		    look up message in suppress_list
**		    if found then
**			return TRUE (this message qualifies for suppression)
**		    else
**			return FALSE (this msg does not qualify for suppression)
**
**  ASSUMPTIONS:
**	This routine is written with the assumption that the list of messages
**	to suppress is relatively small and therefore linear searches will
**	be performed.
**
** Inputs:
**	message_id		    id of message (that we are trying to 
**				    determine whether or not to suppress)
**	duve_cb			    VERIFYDB control block
**	    duve_dbms_test	    flag / datavalue.  
**				    - zero indicates that no messages are to be
**				      suppressed
**				    - when nonzero, suppress all messages in
**				      list unless the message has the same
**				      value as this flag variable.
** Outputs:
**	    None.
**
**	Returns:
**	    TRUE    - suppress this message
**	    FALSE   - do not suppress this message
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      10-dec-1993 (teresa)
**	    created as project to increase verifydb performance and also reduce
**	    suprious test diffs.
*/
static bool
duve_suppress(DU_STATUS message_id, DUVE_CB *duve_cb)
{
    bool	ret_val = FALSE;
    i4		i;
    DU_STATUS	warn;
    static DU_STATUS suppress_list [] = 
    {
	S_DU1611_NO_PROTECTS,
	S_DU1612_NO_INTEGS,
	S_DU1619_NO_VIEW,
	S_DU161E_NO_STATISTICS,
	S_DU167D_NO_RULES,
	S_DU165E_BAD_DB,
	S_DU167A_WRONG_CDB_ID
    };
#define	    LIST_CNT	    (sizeof(suppress_list)/sizeof(suppress_list[0]))

    static const struct 
    {   
	DU_STATUS	info;
	DU_STATUS	warn;
    } suppress_map [] = 
    {
	{S_DU0305_CLEAR_PRTUPS, S_DU1611_NO_PROTECTS},
	{S_DU0355_CLEAR_PRTUPS, S_DU1611_NO_PROTECTS},
	{S_DU0306_CLEAR_INTEGS, S_DU1612_NO_INTEGS},
	{S_DU0356_CLEAR_INTEGS, S_DU1612_NO_INTEGS},
	{S_DU030C_CLEAR_VBASE, S_DU1619_NO_VIEW},
	{S_DU035C_CLEAR_VBASE, S_DU1619_NO_VIEW},
	{S_DU0310_CLEAR_OPTSTAT, S_DU161E_NO_STATISTICS},
	{S_DU0360_CLEAR_OPTSTAT, S_DU161E_NO_STATISTICS},
	{S_DU0342_CLEAR_RULE, S_DU167D_NO_RULES},
	{S_DU0392_CLEAR_RULE, S_DU167D_NO_RULES},
	{S_DU0340_UPDATE_STARCDBS, S_DU167A_WRONG_CDB_ID},
	{S_DU0390_UPDATE_STARCDBS, S_DU167A_WRONG_CDB_ID} 
    };
#define	    MAP_CNT	    (sizeof(suppress_map)/sizeof(suppress_map[0]))

    do	/* control loop */
    {
	if ( ! duve_cb->duve_dbms_test)
	    /* duve_test flag is not set */
	    break;
	if (message_id > DU_WARN_BOUNDARY)
	    /* this is an error message */
	    break;
	if (message_id <= DU_OK_BOUNDARY)
	    /* we dont suppress this type of message, so */
	    break;
	if (message_id <= DU_INFO_BOUNDARY)
	{
	    /* this is an info message */
	    warn = 0;
	    for (i=0; i < MAP_CNT; i ++)
	    {
		if (suppress_map[i].info == message_id)
		{
		    warn = suppress_map[i].warn;
		    break;
		}
	    }
	    if ( (warn) && (warn != duve_cb->duve_dbms_test) )
		/* info message is in suppress list */
		ret_val = TRUE;
	}
	else
	{
	    /* we are processing a warning message, so look for it in
	    ** the suppress_list unless it's explicitely referenced by
	    ** the dbms_test flag.
	    */
	    if (message_id == duve_cb->duve_dbms_test )
		break;
	    for (i = 0; i < LIST_CNT; i++)
	    {
		if (suppress_list[i] == message_id)
		{
		    ret_val = TRUE;
		    break;
		}   /* end if found */
	    }	/* end of search loop */
	}

    } while (0);    /* end of error control loop */
    return (ret_val);
}
