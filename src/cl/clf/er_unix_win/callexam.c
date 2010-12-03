/*
**Copyright (c) 2004 Ingres Corporation
*/
# include   <compat.h>
# include   <gl.h>
# include   <st.h>
# include   <er.h>
# include   <si.h>
# include   <me.h>
# include   <cs.h>   /* Needed for "erloc.h" */
# include   <errno.h>

/*
PROGRAM =   ercallexam
**
NEEDLIBS =  COMPAT MALLOCLIB
*/

/*
** History:
**	25-Oct-2005 (hanje04)
**	    Added history(!?)
**	    Add prototype for CLcall() to prevent compiler
**          errors with GCC 4.0 due to conflict with implicit declaration.
**	11-Nov-2010 (kschendel) SIR 124685
**	    Prototype fixes.
*/

/*
**  Example program showing how to call ERslookup() with CL_ERR_DESC.
**  The example is a generic error message reporting function, geterrmsg().
*/

/* local prototypes */
static i4 CLcall(CL_ERR_DESC *errdesc);
static i4 client(void);
static void geterrmsg(STATUS errorcode, CL_ERR_DESC *clerror,
	ER_ARGUMENT *msgparms, i4 n, bool want_grisly_details,
	char *message, i4 *length);

int
main(int argc, char **argv)
{
	MEadvise(ME_INGRES_ALLOC);

	if(client())
		PCexit(FAIL);

	PCexit(OK);
	/* NOTREACHED */
	return (0);
}


static i4
client(void)
{
	char msg[ER_MAX_LEN];
	int msglen;
	int status = OK;
	CL_ERR_DESC clerror;

	if ((status = CLcall(&clerror)) != OK)
	{
		geterrmsg(status, &clerror, NULL, 0, TRUE, msg, &msglen);
		SIfprintf(stderr, "%s\n", msg);
		return(OK);
	}
	return(FAIL);
}


static void
geterrmsg(STATUS errorcode, CL_ERR_DESC *clerror,
	ER_ARGUMENT *msgparms, i4 n, bool want_grisly_details,
	char *message, i4 *length)
{
	char		text[ER_MAX_LEN];
	i4			textlen;
	CL_ERR_DESC	er_err;

	*length = 0;

	if (clerror)
	{
		/*
		** Look up external (documented in CL specification) CL error.
		** Always pass the CL_ERR_DESC set by a failed CL call to ERslookup()
		** when looking up an external CL error.  Last argument to
		** ERslookup() will be ignored, so simply pass null pointer.
		*/

		(VOID) ERslookup(errorcode, clerror, 0, (char *) NULL, text, 
			sizeof(text), -1, &textlen, &er_err, n, NULL);

		(*length) += textlen;

		if (want_grisly_details)
		{
			/*
			** Look up internal (system-dependent) CL error, and append
			** text to the message just retrieved.  Pass message number
			** argument of zero to get this information.  Again, last
			** argument is ignored.
			*/
			text[(*length)++] = '\n';

			(VOID) ERslookup(0, clerror, 0, (char *) NULL, 
				&text[*length], sizeof(text) - *length, -1, 
				&textlen, &er_err, n, NULL);

			(*length) += textlen;;
		}
	}
	else
	{
		/*
		** Look up external or internal non-CL error.  Don't pass
		** clerror. There may be parameters supplied in `msgparms'.
		** (This call to ERslookup is not executed in this example.)
		*/

		(VOID) ERslookup(errorcode, NULL, 0, (char *) NULL, text, 
			sizeof(text), -1, &textlen, &er_err, n, msgparms);

		(*length) += textlen;
	}

	text[*length] = '\0';

	STcopy(text, message);
}


/*
** A phony "CL call" for illustrative purposes.
** Some VMS-knowledgeable person should change "1" to a real VMS error
** in the VMS implementation.
*/

# include   "erloc.h"

static i4
CLcall(CL_ERR_DESC *errdesc)
# if !defined (VMS)
{
	i4 status = OK;
	char *filename = "/some/unix/file";
	if (open(filename, 0) == -1)

	{
		SETCLERR(errdesc, ER_INTERR_TESTMSG, ER_open);
		errdesc->moreinfo[0].size = STlength(filename);
		STcopy(filename, errdesc->moreinfo[0].data.string);
		status = ER_BADREAD;
	}

	return (status);
}
#else
{
	i4 status = OK;

	if (1)
	{
		errdesc->error = 1;
		status = ER_BADREAD;
	}

	return (status);
}
# endif /* VMS */
