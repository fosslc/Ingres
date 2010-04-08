/*
**    Copyright (c) 1989, 2008 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<st.h>
# include	<lo.h>
# include	<er.h>
# include	<cv.h>
# include	<descrip.h>
# include	<iledef.h>
# include	<lnmdef.h>
# include	<starlet.h>
# include	"lolocal.h"

/**
** Name:	locvtvms.c - do string manipulations on VMS filenames
**
** Description:
**	This file contains all routines which do string manipulations
**	on filenames.  Elsewhere, filenames are processed by RMS routines.
**	Thus, these routines are an unfortunately required backdoor into
**	VMS filename processing.
**
**	In all cases, client code determines that the requested manipulation
**	is required; this module simply performs it.
**
**	This file defines:
**
**	LOdev_to_root	Add the MFD ([000000]) to a bare device name.
**	LOdir_to_file	Change a directory spec to a filename
**	LOfile_to_dir	Change a directory filename to a directory spec
**	LOcombine_paths	Combine two (possibly relative) paths
**
** History:
**	10/2/89 (Mike S) Initial version
**	7/91 (Mike S) Handle the MFD for a rooted directory in LOfile_to_dir.
**		      Bug 30647
**      16-jul-93 (ed)
**	    added gl.h
**	19-jul-2000 (kinte01)
**	   Correct prototype definitions by adding missing includes
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
**	15-apr-2005 (abbjo03)
**	    Update MAX_SUBDIRS to 255 (available since Alpha VMS 7.2).
**	09-oct-2008 (stegr01/joea)
**	    Replace ITEMLIST by ILE3.
**/

/* # define's */
# define MFD		ERx("000000")
# define DIR_EXT 	ERx(".DIR;1")

# define MAX_SUBDIRS	255		/* Subdirectories VMS allows */

/* GLOBALDEF's */
/* extern's */
/* static's */
static bool rooted_mfd();

/*{
** Name:	LOdev_to_root - Add MFD to a bare device name
**
** Description:
**	Add the MFD directory ([000000]) to a bare device name.
**
** Inputs:
**	in	char *	input string
**
** Outputs:
**	out	char *  output string
**
**		These may overlap.
**
**	Returns:
**		None
**
**	Exceptions:
**		None
**
** Side Effects:
**		none.
**
** History:
**	10/2/89 (Mike S)	Initial version
*/
VOID
LOdev_to_root(in, out)
char *in;
char *out;
{
	char buffer[MAX_LOC+1];
	i4 in_len = STlength(in);	/* Length of input */

	if (in_len == 0)
	{
		STcopy (MFD, out);		/* No input -- use bare MFD */
	}
	else if (in[in_len-1] == *COLON)
	{
		/* Colon already present */
		STcopy(in, buffer);
		STpolycat(4, buffer, OBRK, MFD, CBRK, out);	
	}
	else
	{
		STcopy(in, buffer);
		STpolycat(5, buffer, COLON, OBRK, MFD, CBRK, out); 
							/* Add colon */
	}
}

/*{
** Name:	LOdir_to_file - convert a directory spec to a directory
**
** Description:
**	For instance, convert
**		DUA2:[MAIN.FILES] 	to	DUA2:[MAIN]FILES.DIR;1
**		DUA2:[FILES]		to	DUA2:[000000]FILES.DIR;1
**		[FILES]			to	FILES.DIR;1
**		DUA2:			to	DUA2:[000000]000000.DIR;1
**		USER1:[000000]		to	$2$DUA32:[000000]USR.DIR;1
**					where USER1 is $2$DUA32:[USR.]
**	Note that "<>" are legal synonyms for "[]", and so are allowed in the
**	input string.  They cannot be mixed: i.e. [FILES>" is illegal.
**
** Inputs:
**	in	char *	input string
**
** Outputs:
**	out	char *  output string
**
**		These may overlap.
**
**	Returns:
**		None
**
**	Exceptions:
**		None
**
** Side Effects:
**		none.
**
** History:
**	10/2/89 (Mike S)	Initial version
*/
VOID
LOdir_to_file(in, out)
char	*in;
char	*out;
{
	char outbuffer[MAX_LOC+1];
	char tmpbuffer[MAX_LOC+1];
	char *obracket;		/* Open bracket */
	char *dot;		/* Dot preceding last subdirectory */
	char *file;		/* file name */
	i4 file_len;		/* Length of file name */
	bool angle = FALSE;	/* Use angle brackets */
	char cbracket;		/* Closing bracket */
	char *p;

	obracket = STindex(in, OBRK, 0);	/* Find bracket */
	if (obracket == NULL)
	{
		angle = TRUE;
		obracket = STindex(in, OABRK, 0);	/* Find angle bracket */
	}
	if (obracket == NULL)
	{
		/* It must be a bare device name. Add MFD */
    		LOdev_to_root(in, tmpbuffer);
		obracket = STindex(tmpbuffer, OBRK, 0);	/* Find bracket */
		angle = FALSE;
		in = tmpbuffer;
	}

	/* See if we're at the MFD */
	cbracket = (angle ? *CABRK : *CBRK);
	if (STindex(obracket, DOT, 0) == NULL &&
	    STbcompare(obracket+1, sizeof(MFD) - 1, MFD, 
			sizeof(MFD) - 1, FALSE) == 0 &&
	    obracket[sizeof(MFD)] == cbracket)
	{
		/* Yep. check for concealed device */
		if (rooted_mfd(in, obracket, tmpbuffer))
		{
			/* Much may have changed */
			angle = FALSE;
			obracket = STindex(tmpbuffer, OBRK, 0);
			if (obracket == NULL)
			{
				obracket = STindex(tmpbuffer, OABRK, 0);
				angle = TRUE;
			}
			in = tmpbuffer;
			cbracket = (angle ? *CABRK : *CBRK);
		}
	}			

	/* See if we have subdirectories */
	dot = STrindex(obracket, DOT, 0);
	if (dot == NULL)
	{
		/* 
		** MFD or first-level directory.  Copy the device name, if any, 
	  	** plus the MFD.
		*/
		file = obracket + 1;
		STlcopy(in, outbuffer, obracket - in + 1);
		p = outbuffer + (obracket - in) + 1;
		STcopy(MFD, p);
		p += STlength(MFD);
	}
	else
	{
		/* Yes.  Copy up to, but not including, the final dot */
		file = dot + 1;
		STlcopy(in, outbuffer, dot - in);
		p = outbuffer + (dot - in);
	}
	/* Close up directory part */
	*p++ = cbracket;

	/* Copy file part */
	file_len = STlength(file) - 1;
	STlcopy(file, p, file_len);	
    	p += file_len;

	/* Add directory extension */
	STcopy(DIR_EXT, p);

	STcopy(outbuffer, out);		/* Output result */
}

/*
** static: rooted_mfd
**
** Handle the case where we have the MFD of a rooted directory.
**
**	CDEV:[000000]	Result: DEV:[DIR1]
**				where CDEV = DEV:[DIR1.]
**	CDEV:[000000]	Result: DEV:[DIR1...DIRn]
**				where CDEV = DEV:[DIR1...DIRn.]
*/
static bool
rooted_mfd(in, obracket, out)
char *in;	/* Input directory name */
char *obracket;	/* Left bracket */
char *out;	/* Result string.  This may be the same as in */
{
	char devstring[MAX_LOC+1];
	$DESCRIPTOR(devdesc, devstring);
	char result[MAX_LOC+1];
	u_i2 reslength;
	static $DESCRIPTOR(filedev_tables, "LNM$FILE_DEV");
	STATUS status;
	ILE3	 itemlist[] = 
	{
		{sizeof(result), LNM$_STRING, result, &reslength},
		{0, 0, 0, 0}
	};

	/* Check for a device string which is a concealed device */
	if (in == obracket)
		return FALSE;

	STlcopy(in, devstring, obracket - in - 1);
	devdesc.dsc$w_length = STlength(devstring);
	CVupper(devstring);
	status = sys$trnlnm(0, &filedev_tables, &devdesc, 0, itemlist);
	if ((status & 1) != 1)
		return FALSE;	/* No logical name */
	if (result[reslength-2] != *DOT)
		return FALSE;	/* Not a concealed device */

	/* A concealed device; unconceal it */
	result[reslength-2] = result[reslength-1];
	result[reslength-1] = EOS;
	STcopy(result, out);
	return TRUE;
}
		
/*{
** Name:	LOfile_to_dir - Convert directory file to directory form
**
** Description:
**	For instance, convert
**		DUA2:[MAIN]FILES.DIR		to	DUA2:[MAIN.FILES]      
**		DUA2:[000000]FILES.DIR		to	DUA2:[FILES]	       
**		FILES.DIR			to	[FILES]
**
**	Note that "<>" are legal synonyms for "[]", and so are allowed in the
**	input string.  They cannot be mixed: i.e. [FILES>" is illegal.
**
** Inputs:
**	in	char *	input string
**
** Outputs:
**	out	char *  output string
**
**		These may overlap.
**
**	Returns:
**		None
**
**	Exceptions:
**		None
**
** Side Effects:
**		none.
**
** History:
**	10/2/89 (Mike S)	Initial version
**	02/1/93 (teresal)	Fix # of arguments in STlcopy(). 
*/
VOID
LOfile_to_dir(in, out)
char *in;
char *out;
{
	char buffer[MAX_LOC+1];
	char *obracket;		/* Opening bracket */
	char *cbracket;		/* Closing bracket */
	bool angle = FALSE;	/* Are we using angle brackets */
	char *file;		/* File name */
	i4  file_len;		/* Length of file name */
	i4  dir_len;		/* Length of directory name */
	char *dot;		/* dot preceding file type */
	char *colon;		/* colon from node or device */
	char *p;

	/* Find beginning and closing brackets, if any */
	obracket = STindex(in, OBRK, 0);
	if (obracket != NULL)
	{
		cbracket = STindex(obracket, CBRK, 0);
	}
	else
	{
		obracket = STindex(in, OABRK, 0);
		if (obracket != NULL)
		{
			angle = TRUE;
			cbracket = STindex(obracket, CABRK, 0);
		}
	}

	/* Find the length of the file part */
	
	if (obracket != NULL)
	{
		file = cbracket + 1;
	}
	else
	{
		colon = STrindex(in, COLON, 0);
		if (colon != NULL)
			file = colon + 1;
		else
			file = in;
	}
	dot = STindex(file, DOT, 0);
	if (dot == NULL)
	{
		*out = EOS;
		return;
	}				
	file_len = dot - file;

	if (obracket != NULL)
	{
		/* Copy everything up to and including the open bracket */
		STlcopy( in, buffer, obracket - in + 1);
		p = buffer + (obracket - in) + 1;

		/* 
	        ** If the directory is the MFD, make a top-level directory.  
		** Otherwise, make a subdirectory.
		*/
		dir_len = cbracket - obracket - 1;
		if ((STbcompare(obracket + 1, dir_len, MFD, dir_len , FALSE) 
			!= 0)
		     || (dir_len != STlength(MFD)))
		{
			/* Make a subdirectory */
			STlcopy(obracket + 1, p,
				cbracket - obracket - 1);
			p += cbracket - obracket - 1;
			*p++ = *DOT;
		}
	}	
	else if (colon != NULL)
	{
		/* Copy everything up to the colon, and add "[" */
		STlcopy(in, buffer, colon - in + 1);
		p = buffer + (colon - in) + 1;
		*p++ = *OBRK;
	}
	else
	{
		/* No directory, device, or node */
		*buffer = *OBRK;
		p = buffer + 1;
	}
	STlcopy(file, p, file_len);
	p += file_len;

	/* Close up directory */
	STcopy((angle ? CABRK : CBRK), p);
	STcopy(buffer, out);
}

/*{
** Name:	LOcombine_paths	- 	Combine two (possibly relative) paths
**
** Description:
**	The input arguments are two VMS pathnames.  We append the second to
**	the first.  The second must be a relative pathname, and cannot include
**	a device name.  It can include [-].  Some examples:
**
**	Combining	DEV:[DIR]	and [.SUB]	gives DEV:[DIR.SUB]
**			DEV:[DIR.SUB]	    [-.SUB2]	      DEV:[DIR.SUB2]
** 			DEV:[DIR]	    [-]		      DEV:[000000]
** 			DEV:[DIR]	    [--] 	      FAIL
**
**	Input and output arguments can overlap.
**	Logical names are assumed to be expanded already.
**
** Inputs:
**	head		char *	first string to combine	
**	tail		char *	second string to combine
**
** Outputs:
**	result		char *	combined string, if status = OK
**
**	Returns:
**		STATUS	OK
**			FAIL
**
**	Exceptions:
**		none
**
** Side Effects:
** 		none
**
** History:
**	10/16/89	(Mike S)	Initial version
*/
STATUS
LOcombine_paths(head, tail, result)
char	*head;
char	*tail;
char	*result;
{
	char	buffer[MAX_LOC+1];	/* Work buffers */
	char	*obracket = OBRK;	/* Directory delimiters */
	char	*cbracket = CBRK;
	char	*headp, *tailp, *p;	/* Utility pointers */
	i4	subdirs;		/* Subdirectory count */

	/* Check that the tail is a relative path */
	if (tail[1] != *DOT && tail[1] != *DASH)
		return FAIL;

	/* 
	** See if we're using normal or angle brackets.  If necessary, change 
	** a bare device name to include the MFD.
	*/
	if (STindex(head, OABRK, 0) != NULL)
	{
		obracket = OABRK;
		cbracket = CABRK;
		STcopy(head, buffer);
	}
	else if (STindex(head, OBRK, 0) != NULL)
	{
		STcopy(head, buffer);
	}
	else
	{
		LOdev_to_root(head, buffer);
	}

	/* 
	** Count subdirectories in our head path.  We will enforce the limit
	** of 255.  MFD alone will be called -1.
	*/		
	for (p = buffer, subdirs = 0; p != NULL; )
	{
		if ((p = STindex(p + 1, DOT, 0)) != NULL) 
			subdirs++;
	}
	if (subdirs == 0)
	{
		i4 MFDlen = STlength(MFD);

		p = STindex(buffer, obracket, 0);
		if (p == NULL) return(FAIL);
		if ((STbcompare(p+1, 0,  MFD, MFDlen, FALSE) == 0) &&
		    (p[MFDlen+1] == *cbracket))
			subdirs = -1;
	}

	/* Go through tail string, adjusting buffer as we go */
	for (tailp = tail+1, headp = buffer + STlength(buffer) - 1; ;)
	{
		if (*tailp == *DOT)
		{
			/* Check for too many subdirectories */
			if (subdirs++ >= MAX_SUBDIRS)
				return FAIL;
		
			/* Append next subdirectory */
			p = STindex(tailp + 1, DOT, 0);
			if (p == NULL) p = tailp + STlength(tailp) - 1;
			if (subdirs > 0)
			{
				STlcopy(tailp, headp, p - tailp);
			}
			else
			{
				headp = STindex(buffer, obracket, 0);
				if (headp == NULL) return (FAIL);
				STlcopy(tailp+1, headp + 1, p - tailp - 1);
			}			

			/* Adjust pointers */
			tailp = p;
			headp = headp + STlength(headp);
		}
		else if (*tailp == *CBRK || *tailp == *CABRK)
		{
		    	STcopy(cbracket, headp);
			STcopy(buffer, result);
			return (OK);
		}
		else if (*tailp == *DASH)
		{
			/* Remove one directory. Fail if we're already at MFD*/
			if (subdirs-- < 0)
			{
				return (FAIL);
			}
			if (subdirs == -1)
			{
				/* Go up to MFD */
				p = STindex(buffer, obracket, 0);
				if (obracket == NULL) return (FAIL);
				STcopy(MFD, p + 1);
				headp = p + STlength(p);
			}
			else
			{
				/* Remove one subdirectory. */
				headp = STrindex(buffer, DOT, 0);
				if (headp == NULL) return (FAIL);
				*headp = EOS;
			}
			tailp++;
		}
		else
		{
			/* Something's wrong here ... */
			return (FAIL);
		}
	}
} 
