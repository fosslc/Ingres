#undef INCLUDE_ALL_CL_PROTOS
# include       <ctype.h>
# include       "telocal.h"
# include       <te.h>

/*
** Copyright (c) 1985 Ingres Corporation
**
**      Name:
**              TEwrite.c
**
**      Function:
**              Write buffer to terminal
**
**
**      Arguments:
**              char    *buffer;
**              i4      length;
**
**      Result:
**              When the application knows that it is writing more than
**              one character it should call this routine.  TEwrite
**              efficiently moves many characters to the bufferand then
**              to the terminal.
**
**      Side Effects:
**              None
**
**      History:
**              10/83 - (mmm)
**                      written
**	25-Oct-93	(marc_b)
**			Undefed INCLUDE_ALL_CL_PROTOS
**      12-Dec-95 (fanra01)
**          Added definitions for extracting data for DLLs on windows NT
**	26-jun097 (kitch01)
**		Bug 79761/83247. Implemented function TEcmdwrite to allow MWS messages
**		to be sent to CACI-Upfront.
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
*/

/*
** Update Entire Screen Flag.
*/
GLOBALREF  i2   TEwriteall;
GLOBALREF  bool TEcmdEnabled; 


VOID
TEwrite(
char    *buffer,
i4      length)
{
	i4      dummy_count;

	if      (TEok2write == OK)
		SIwrite(length, buffer, &dummy_count, stdout);
	else if (memcmp(buffer, "[24;", 5) == 0)
	{
		buffer += 4;
		while   (isdigit(*++buffer))
			;
		if      (memcmp(buffer, "H\n\n", 3) == 0)
			TEwriteall = 1;
	}
}

/*
**  TEcmdwrite - not used in PMFE DOS
**
**  History:
**	26-jun097 (kitch01)
**		Bug 79761/83247. Implemented this function to allow MWS messages
**		to be sent to CACI-Upfront.
*/

VOID
TEcmdwrite(
char *  buffer,
i4      length)
{
    i4      dummy_count;

    if (TEcmdEnabled)
    {
            SIwrite(length, buffer, &dummy_count, stdout);
            SIflush(stdout);
    }

}

/*
** TEsetWriteall - set TEwriteall to the value given.
*/

VOID
TEsetWriteall(
i2      i)
{
	TEwriteall = i;
}
