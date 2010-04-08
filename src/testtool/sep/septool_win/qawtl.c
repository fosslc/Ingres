/*
**  Copyright (c) 1999 Ingres Corporation
**
**  Name: qawtl.c -- Write To Log utility
**
**  Usage:
**	qawtl <message>
**
**  Description:
**	This utility writes messages to the errlog.log, indicating a
**	note worthy testing event. This could include the beginning
**	or ending of running a test suite or to mark an event that
**	is supposed to write certain errors to the errlog.log.
**	Ultimately, these messages should aid the tester when he/she
**	is reviewing the errlog.log after several test suites have
**	been run. Furthermore, a tester can send qawtl messages to a
**	file other than errlog.log by setting the OS environment
**	variable QAWTL_LOG to the full path of the new file. To
**	disable these messages all together, set QAWTL_LOG to "c:\nul".
**
**  History:
**	30-dec-1999 (somsa01)
**	    Created based upon the UNIX version, which is a shell script.
**	31-dec-1999 (somsa01)
**	    A new line character must also be added when writing to the
**	    errlog.log .
*/

# include <compat.h>
# include <er.h>
# include <gl.h>
# include <me.h>
# include <nm.h>
# include <lo.h>
# include <pc.h>
# include <si.h>
# include <st.h>


int
main(int argc, char *argv[])
{
    bool	use_ingres_errlog=FALSE, create_flag=FALSE;
    char	*QAWTL_LOG, *QA_MESSAGE, chrtime[50];
    int		i, status=0;
    DWORD	bytes_written, buf_size;
    FILE	*hFile=NULL;
    LOCATION	loc;
    SYSTIME	stime;

    /* If an alternate log file is not set default to errlog.log */
    NMgtAt("QAWTL_LOG", &QAWTL_LOG);
    if (!*QAWTL_LOG)
    {
	char *sysloc;

	NMgtAt(SystemLocationVariable, &sysloc);
	QAWTL_LOG = (char *)MEreqmem(0, STlength(sysloc) +
				     STlength(SystemLocationSubdirectory) +
				     32, TRUE, NULL);
	STprintf(QAWTL_LOG, ERx("%s\\%s\\files\\errlog.log"), sysloc,
		 SystemLocationSubdirectory);
	use_ingres_errlog = TRUE;
    }

    /*
    ** If we can't locate the log and it is errlog.log
    ** echo the error and abort.
    */
    LOfroms(PATH & FILENAME, QAWTL_LOG, &loc);
    if (LOexist(&loc) != OK && use_ingres_errlog)
    {
	STprintf("QAWTL cannot locate %s\n", QAWTL_LOG);
	MEfree(QAWTL_LOG);
	PCexit(1);
    }
    else
    {
	/*
	** Using alternate log and if doesn't exist we will try to create
	** on our first write to the log.
	*/
	if (LOexist(&loc) != OK)
	{
	    STprintf("QAWTL cannot locate %s\n", QAWTL_LOG);
	    STprintf("QAWTL will attempt to create %s\n", QAWTL_LOG);
	    create_flag = TRUE;
	}
    }

    TMnow(&stime);
    TMstr(&stime, chrtime);

    /*
    ** Figure out the size of QA_MESSAGE.
    */
    buf_size = 18 + strlen(chrtime) + 1;
    for (i=1; i<argc; i++)
	buf_size += 1 + strlen(argv[i]);
    QA_MESSAGE = (char *)MEreqmem(0, buf_size, TRUE, NULL);

    STprintf(QA_MESSAGE, "QAWTL %s ====>", chrtime);

    /* Loop through passed arguments to build message */
    for (i=1; i<argc; i++)
    {
	STcat(QA_MESSAGE, " ");
	STcat(QA_MESSAGE, argv[i]);
    }

    STcat(QA_MESSAGE, " <====\n");

    /*
    ** Now, open the log file and append the message to it.
    */
    if (SIopen(&loc, "a", &hFile) == OK &&
	SIwrite(buf_size, QA_MESSAGE, &bytes_written, hFile) == OK &&
	SIclose(hFile) == OK)
    {
	/* Do nothing if successful. */
    }
    else
    {
	STprintf("QAWTL could not write to %s\n", QAWTL_LOG);
	status = FAIL;
    }

    MEfree(QA_MESSAGE);
    if (use_ingres_errlog)
	MEfree(QAWTL_LOG);
    PCexit(status);
}
