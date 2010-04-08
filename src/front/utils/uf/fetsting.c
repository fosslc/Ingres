/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<st.h>
#include	<si.h>
#include	<lo.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>

/**
** Name:    fetsting.c	- Front-End Utility Test Files Module.
**
** Description:
**	This file defines the routines used in starting up the testing
**	environment for forms-based programs.  By passing in the
**	parameters given on the command line which control testing
**	(the -I, -Z and -D flags), these routines control the
**	details of the testing implementation.
**
**	This file defines:
**
**	FEtstbegin()	Start the testing files.
**	FEtstend()	Close down the testing files.
**	FEtstrun()	Boolean function to test if running tests.
**	FEtstmake()	Boolean function to test if making tests.
**
** History:
**	Revision 6.0  86/12/15  peter
**	Initial revision.
**/

static FILE	*tst_fddfile = NULL;
static bool	tst_ifile_open = FALSE;
static bool	tst_dfile_open = FALSE;
static bool	tst_zfile_open = FALSE;


/*{
** Name:    FEtstbegin() - Open testing files for forms program.
**
** Description:
**	This opens up all files associated with testing, and sets the
**	global variables needed by the lower levels of the system
**	in performing the tests.
**
**	The three relevant flags are:
**
**	() -Dfilename	for collecting the .scr files for
**				testing.
**	() -Ifilename	for collecting the keystroke file.
**	() -Zfilename	for running tests.  This argument
**				contains the input keystroke file,
**				followed by the output 'stdout'
**				file for the program.
** Inputs:
**	dfile		pointer to the -D parameter.
**	ifile		pointer to the -I parameter.
**	zfile		pointer to the -Z parameter.
**
** Returns:
**	STATUS  OK	if everything worked.
**		FAIL	if anything failed.
**
** Side Effects:
**	Sets static variables tst_fddfile, tst_ifile_open, tst_dfile_open
**	and tst_zfile_open.
**
** History:
**	15-dec-1986 (peter)	Written.
*/

STATUS
FEtstbegin (dfile, ifile, zfile)
char	*dfile;
char	*ifile;
char	*zfile;
{
    tst_ifile_open = FALSE;		/* TRUE if -i file open */
    tst_dfile_open = FALSE;		/* TRUE if -d file open */
    tst_zfile_open = FALSE;		/* TRUE if -d file open */

    if (ifile != NULL)
    {
	FDrcvalloc(ifile);
	tst_ifile_open = TRUE;
    }

    if (dfile != NULL)
    {
	LOCATION	file;			/* file for .scr file */
	char	buf[MAX_LOC];

	STcopy(dfile, buf);
	LOfroms(FILENAME, buf, &file);
	if (SIopen(&file, ERx("w"), &tst_fddfile) != OK)
	    return FAIL;
	FDdmpinit(tst_fddfile);
	if (FEinqferr() != 0)
	    return FAIL;
    }

    if (zfile != NULL)
    {
	FEtest(zfile);
	if (FEinqferr() != 0)
	    return FAIL;
	tst_zfile_open = TRUE;
    }

    return OK;
}


/*{
** Name:    FEtstend() - Close testing files.
**
** Description:
**	Close testing files opened by FEtstbegin.
**
** Returns:
**	STATUS  OK if everything ok.
**		error status from SIclose if FAIL.
**
** History:
**	15-dec-1986 (peter)	Written.
*/

STATUS
FEtstend()
{
    if (tst_ifile_open == TRUE)
	FDrcvclose(FALSE);
    if (tst_dfile_open == TRUE)
	return SIclose(tst_fddfile);

    return OK;
}


/*{
** Name:    FEtstrun() - Test if running tests.
**
** Description:
**	This function returns TRUE if the tests are being run through
**	use of the -Z flag option.
**
** Returns:
**	STATUS OK if -Z flag specified in this run.
**
** History:
**	15-dec-1986 (peter)	Written.
*/

STATUS
FEtstrun()
{
    return (tst_zfile_open ? OK : FAIL);
}


/*{
** Name:    FEtstmake() - Test if creating test keystroke file.
**
** Description:
**	This function returns TRUE if keystroke files are being created through
**	use of the -I flag option.
**
** Returns:
**	STATUS OK if -I flag specified in this run.
**
** History:
**	15-dec-1986 (peter)	Written.
*/

STATUS
FEtstmake()
{
    return(tst_ifile_open ? OK : FAIL);
}
