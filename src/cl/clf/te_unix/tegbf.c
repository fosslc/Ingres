/*
**Copyright (c) 2004 Ingres Corporation
*/
# include	<compat.h>
# include	<gl.h>
# include	<te.h>
# include	<lo.h>
# include	<si.h>
# include	<sgtty.h>

/*
** TEgbf
**	Special open call for GBF to open a device the user inputs.
**	On Unix, using the TEtest mechanism for redirection along with
**	the normal TEopen will suffice to open either a device or a file
**	correctly.
**
** History:
**	14-oct-87 (bab)
**		Modified to call TEopen with a TEENV_INFO struct
**		(all of whose members are ignored.)
**	24-may-89 (mca)
**		Modified TEtest call in response to addition of new TEtest
**		parameters. (No change to the function of TEgbf.)
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
*/

static  i4		loctype;	/* Type of loc (device or file) */ 
static	FILE		*tmpfp;		/* Pointer to opened loc */
static	bool		isopen = FALSE;	/* TRUE if a loc is open */

extern		bool TEisa();

STATUS
TEgbf(loc, type)
char		*loc;
i4		*type;
{
	TEENV_INFO		open_dummy;
	STATUS			retval;

	FILE			*fopen();


	/*
	**	1) isopen == FALSE, 
	**		open the specified device,
	**		set RAW, ~CRMOD, ~ECHO (if device is a real device),
	**		return OK.
	**	2) isopen == TRUE, signal FAIL and return (at most 1 call
	**	   to TEGBF is allowed).
	*/

	/*	Check if multiple "devices" are to be open */

	if (isopen == TRUE)
	{
		retval = FAIL;
	}
	else
	{

		/*
		** Temporarily open the "device" to determine if 
		** it is a device or a file.  
		*/

		if ((tmpfp = fopen(loc, "w")) == NULL)
		{
			/*	Couldn't open the device */

			isopen = FALSE;
			return (FAIL);
		}
		
		/*	Is loc a device or a file */

		if (TEisa(tmpfp) == TRUE)
		{
			/*	loc is a device. */

			loctype = TE_ISA;
			*type = TE_ISA;
		}
		else
		{
			/*	loc is a file */

			loctype = TE_FILE;
			*type = TE_FILE;
		}

		/*	Do the redirection */

		if (retval = TEtest("", TE_NO_FILE, loc, TE_SI_FILE))
			return retval;

		/*	Open the device for TE like output */

		if (TEopen(&open_dummy) != OK)
		{
			isopen = FALSE;
			retval = FAIL;
		}
		else
		{
			isopen = TRUE;
			retval = OK;
		}

	}	
	
	return retval;
}

STATUS
TEgbfclose()
{
	STATUS			retval;


	/*
	**	1) isopen == FALSE, do nothing, signal OK and return.
	**	2) isopen == TRUE, 
	**		flush and close the "device" (one call to TEclose), 
	**		return OK.
	*/

	if (isopen != TRUE)
	{
		retval = OK;
	}
	else
	{
		TEclose();

		isopen = FALSE;

		retval = OK;
	}

	return retval;
}
