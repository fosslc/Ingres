/*
**      Copyright (c) 1983 Ingres Corporation
*/

# include       <compat.h>
# include       <ex.h>
# include       <te.h>
# include       <si.h>
# include       "telocal.h"

# ifdef         MSDOS
# define        USE_DEFAULT
# endif

/*{
** Name:        TEopen  - Initialize the terminal.
**
** Description:
**      Open up the terminal associated with this user's process.  Get the
**      speed of the terminal and use this to return a delay constant.
**      Initialize the buffering structures for terminal I/O.  TEopen()
**      saves the state of the user's terminal for possible later use
**      by TErestore(TE_NORMAL).  Additionally, return the number of
**      rows and columns on the terminal.  For all environments but DG,
**      these sizes will presently be 0, indicating that the values
**      obtained from the termcap file are to be used.  Returning a
**      negative row or column value also indicates use of the values
**      from the termcap file, but additionally causes use of the
**      returned size's absolute value as the number of rows (or columns)
**      to be made unavailable to the forms runtime system.  Both the
**      delay_constant and the number of rows and columns are members
**      of a TEENV_INFO structure.
**
** Inputs:
**      None.
**
** Outputs:
**      envinfo         Must point to a TEENV_INFO structure.
**              .delay_constant Constant value used to determine delays on
**                              terminal updates, based on transmission speed.
**              .rows           Indicate number of rows on the terminal screen;
**                              or no information if 0; or number of rows to
**                              reserve from the forms system if negative.
**              .cols           Indicate number of cols on the terminal screen;
**                              or no information if 0; or number of cols to
**                              reserve from the forms system if negative.
**
**      Returns:
**              STATUS
**
**      Exceptions:
**              None.
**
** Side Effects:
**      None.
**
**  History:
**
**      10/83 - (mmm)
**              written
**      85/10/02  17:08:36  daveb
**              Don't set GT here; should be done by termdr.
**      85/12/12  15:54:29  shelby
**              Fixed delay constant so that it defaults to DE_B9600.
**              This fixes Gould's 19.2 baud rate problem.
**      86/03/26  22:59:45  bruceb
**              enable the EXTA baud rate--usable provided it is >=9600 baud.
**      86/06/30  15:51:15  boba
**              Change WECO to TERMIO.
**      14-oct-87 (bab)
**              Changed parameter to TEopen to be a pointer to a TEENV_INFO
**              struct rather than a nat *.  Code changed to match, including
**              setting the row, column members of the struct to 0.
**
**      29-Oct-86 nancy
**              enable the EXTB baud rate
**
**      06-03-89 leighb
**              added separate section for MSDOS
**
**      1-apr-91 (fraser)
**          Removed reference to UPCSTERM and definition of function TEupcase.
**	08-aug-1999 (mcgem01)
**	    Replace nat with i4.
*/


static STATUS   TEdelay(i4 *);

STATUS
TEopen(
TEENV_INFO      *envinfo)
{
        STATUS  ret_val;

        TEok2write = OK;

        /* disable interrupts to make TEopen() and TEclose()
        ** atomic actions.
        */
        EXinterrupt(EX_OFF);

        envinfo->rows = 0;
        envinfo->cols = 0;

        for (;;)
        {
                /* this loop is only executed once */

                if (!TEisa(stderr))
                {
                        ret_val = OK;
                        envinfo->delay_constant = DE_B9600;
                        break;
                }
#ifdef	TESAVE
                if (ret_val = TEsave())
                {
                        break;
                }
#endif
                if (ret_val = TEdelay(&(envinfo->delay_constant)))
                {
                        break;
                }

                if (ret_val = TErestore(TE_FORMS))
                {
                        break;
                }

                ret_val = OK;

                break;
        }

        /* allow interrupts */
        EXinterrupt(EX_ON);

        return(ret_val);
}


/*
** Tests whether I/O stream 'stream' is a tty or not.
** We first find the file descriptor of the stream
** and use this value to check if 'stream' is a tty or not.
** Will return TRUE if 'stream' is a tty, otherwise
** FALSE will be returned.
*/

bool
TEisa(
FILE    *stream)
{
	return(((stream == stdout)
	     || (stream == stdin)
	     || (stream == stderr) 
	     || (isatty(fileno(stream)) == 1)) ? TRUE : FALSE);
}

/* IBM/PC Terminal I/O compatibility routines */

/*
** Terminal I/O Compatibility library for IBM/PC .
**
** Created 8-2-85 (jen)
**
*/

/*
** TEdelay
**
** Returns:
**      OK - if things were successful.
**
** Note:
**      We will have more speeds defined than what UNIX normally
**      supports because VMS supports more speeds.
*/

static
STATUS
TEdelay(
i4     *delay)
{
        *delay = DE_B9600;
        return(OK);
}
