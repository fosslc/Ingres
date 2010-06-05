/*
**	Copyright (c) 1987, 2008 Ingres Corporation
*/
#include	<compat.h> 
#include	<descrip.h>
#include	<iledef.h>
#include	<lnmdef.h>
#include	<ssdef.h>
#include	<gl.h>
#include	<me.h>
#include	<st.h>
#include	<starlet.h>
#include	<cv.h>

/*NMgetenv - return a pointer to the value of a logical name.
**
**	success: return pointer to value of logical name.
**	failure: return NULL pointer.
**
**	history:
**		9/83 -- (dd) written
**      06-Apr-1987 (fred)
**          Updated to VMS v4 lnm standards
**	12-nov-1991 (bryanp)
**	    Fixed three bugs:
**		1) When circular name definition occurred, this routine would
**		   simply loop forever. Standard VMS utilities will typically
**		   translate no further than 9-11 iterations before ceasing
**		   logical name translation. I changed this routine to stop
**		   logical name translation after 100 iterations and return
**		   NULL (NULL seems as good an answer as any...)
**		2) The code which attempted to handle the special process
**		   permanent filename translations (sys$input, etc.) was
**		   broken. I fixed it and added some comments to it. I don't
**		   know if this code is ever used; it's apparently been doing
**		   this for years...
**		3) (while I was here) increased the internal buffer lengths
**		    from 66 bytes to 256 bytes to handle 255 character names.
**
**      16-jul-93 (ed)
**	    added gl.h
**      11-aug-93 (ed)
**          added missing includes
**	19-jul-2000 (kinte01)
**	   Correct prototype definitions by adding missing includes and
**	   external function references
**	26-jun-2001 (loera01)
**	   Update for multiple product builds
**	17-sep-2001 (kinte01)
**	   STlength now returns a size_t value
**	29-aug-2003 (abbjo03)
**	    Changes to use VMS headers provided by HP.
**	09-oct-2008 (stegr01/joea)
**	    Replace II_VMS_ITEM_LIST_3 by ILE3.
**	12-feb-2010 (abbjo03, wanfr01) Bug 123139
**	    cv.h shoudl be used for function definitions
*/

typedef struct dsc$descriptor STR_DESC;

char *
NMgetenv(name)
char *name;
{
	register int		count;
	STR_DESC		logdesc;
	short		    	length;
	size_t			lname_length;
	static	char		tname[256];
	static char		lname[256];
	static	STR_DESC namedesc = { sizeof(tname) - 1, 0, 0, tname };
	$DESCRIPTOR(file_dev_dsc, "LNM$FILE_DEV");
	ILE3			lnm_item_list[2];
	
	lname_length = STlength(name);
        if ( MEcmp( name, "II", 2 ) == OK )
            STpolycat( 2, SystemVarPrefix, name+2, lname );
        else if ( STcompare(name, "TERM_INGRES") == OK )
            STcopy( SystemTermType, lname );
        else if ( STcompare(name, "TERM_EDBC") == OK )
            STcopy( SystemTermType, lname );
        else
            STcopy( name, lname );
	CVupper(lname);

	count = 0;
	logdesc.dsc$w_length = STlength(logdesc.dsc$a_pointer = lname);

	/* First, pass in buffer to translate */

	lnm_item_list[0].ile3$w_code = LNM$_STRING;
	lnm_item_list[0].ile3$w_length = sizeof(tname) - 1;
	lnm_item_list[0].ile3$ps_retlen_addr = &length;
	lnm_item_list[0].ile3$ps_bufaddr = tname;
	lnm_item_list[1].ile3$w_code = 0;
	lnm_item_list[1].ile3$w_length = 0;

	while (sys$trnlnm(0,
				&file_dev_dsc,
				&logdesc,
				0,
				lnm_item_list) == SS$_NORMAL)
	{
		count++;
		if (count > 100)
		{
		    /*
		    ** We've been through this loop one hundred times and we
		    ** haven't finished the translation yet. This indicates that
		    ** we've encountered a circular definition. For example:
		    **
		    **	def/job a_log b_log
		    **	def/group b_log a_log
		    **
		    ** There's no "right" answer for the translation of a_log
		    ** in this case, so just return NULL.
		    */
		    return (NULL);
		}

		if (tname[0] == '\033')
		{
			/*
			** When you log in, the system creates default logical
			** name table entries for process permanent files. The
			** equivalence names for these entries (for example,
			** SYS$INPUT and SYS$OUTPUT) are preceded by a four-
			** byte header that contains the following information:
			**  Byte 0: \x1b (Escape character)
			**  Byte 1: \x00
			**  Byte 2-3: VMS RMS Internal File Identifier (IFI)
			**
			** This header is followed by the equivalence name
			** string. If any of your program applications must
			** translate system-assigned logical names, you must
			** prepare the program to check for the existence of
			** this header and then to use only the desired part
			** of the equivalence string (that is, to strip off
			** the first 4 bytes).
			**
			** I don't know if Ingres actually uses this
			** functionality; this code was using &tname[3] until
			** 1991 and there were apparently no ill effects...
			*/
			MEcopy(&tname[4], length-4, lname);
			lname[length-4] = '\0';
/*			CVlower(lname);*/
			return (lname);
		}
		if (tname[0] == '_' && tname[1] == '_')
		{
			/*
			** We assume that if a translated name begins with a double
			** underscore that it MUST be a concealed device and the the
			** user would like back a device name (INCLUDING the terminating
			** colon). The following code does just that.
			*/
			lname[lname_length] = ':';
			lname[lname_length+1] = '\0';
/*			CVlower(lname);*/
			return (lname);
			break;
		}
		logdesc.dsc$w_length = lname_length = length;
		logdesc.dsc$a_pointer = tname;
		MEcopy(tname, sizeof lname, lname);
	}
	if (count == 0)
		return (NULL);
	tname[length] = '\0';
/*	CVlower(tname);*/
	return (tname);
}
