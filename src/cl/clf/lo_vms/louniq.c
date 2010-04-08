#include	<compat.h>  
#include    <gl.h>
#include	<st.h> 
#include	<pc.h> 
#include	<lo.h>  
# include 	  <er.h>  
#include	"lolocal.h"

/*LOuniq
**
**	Make unique location. It just appends process id for now.
**
**	The string pat is prepended to a alphanumeric pattern guaranteed
**	(with respect to other calls of LOuniq) to be unique.  The re-
**	sulting filename is located in directory path.  The file is not created.
**
**		Copyright (c) 1983 Ingres Corporation
**		History
**			03/09/83 -- (mmm)
**				written
**			09/22/83 -- (dd) VMS CL
**			1/17/86 -- (ac) bug # 4660. STpolycat expect a null
**					terminate string. Changed the data type 
**					of templetter from char to *char and 
**					its data from 'A' to "A".
**			23-sep-1987 (Joe)
**				Increased the size of the buffer 'string'
**				to 6 since it holds the decimal ascii version
**				of a  4 digit hex number.  Since the maximum
**				4 digit hex number (0xffff) is 65535, the old
**				buffer size of 5 was 1 too short.  Also, changed
**				the a STcopy to a STlcopy since the STcopy was
**				done and then a check was made for an overrun.
**	06-jan-89 (greg)
**		Integrated MGW's fix for PID required for VMS 5.0
**			22-sep-88 (mgw) Bug #15658
**				CL SPEC says that pat is prepended to a 6
**				character alpha numeric guaranteed to be
**				unique... This fix guarantees that.
**	11/05/92 (dkh) - Changed templetter to point to a real buffer instead
**			 a string literal so that  updates to it won't cause
**			 an AV on alpha.
**				
**      16-jul-93 (ed)
**	    added gl.h
**      11-aug-93 (ed)
**          added missing includes
**	24-may-95 (albany)
**	    Removed unused function declarations; they were causing build
**	    failures with prototyping turned on.
*/

# define	EXTENSION_LENGTH	6
# define	SUFFIX_LENGTH		4

static 	char	tletterbuf[1] = "A";

STATUS
LOuniq(pat, suffix, pathloc)
char		*pat;
char		*suffix;
LOCATION	*pathloc;

{
	char		buf[MAXFNAMELEN + SUFFIX_LENGTH];
	PID		pid;
	LOCATION	loc;
	char		string[12];      /* Used to hold a 4 digit hex number */
	char		*templetter = tletterbuf;

	if (pat != NULL)
		STlcopy(pat, buf, sizeof(buf) - 1);

	/* don't over run file name space */
	if (STlength(buf) > (MAXFNAMELEN - EXTENSION_LENGTH))
	{
		buf[MAXFNAMELEN - EXTENSION_LENGTH] = NULL;
	}
	/* don't exceed suffix space */

	if(suffix)
	{
		if (STlength(suffix) >= SUFFIX_LENGTH)
			suffix[SUFFIX_LENGTH - 1] = NULL;	/* SUFFIX_LENGTH includes '.' */
	}

	PCpid(&pid);
   	/*
   	** Fix for bug 15658:
   	** Guarantee that the 6 char alpha numeric pattern has the 5 most
   	** unique digits in the pid, plus the templetter.
   	*/
   	STprintf(string, "%05x", pid & 0xfffff);
   	STpolycat(3, buf, string, templetter, buf);

	if (*templetter == 'Z')
		*templetter = 'A';
	else
		(*templetter)++;
	/* add suffix if there is one */

	if (suffix != NULL)
		STpolycat(3, buf, ".", suffix, buf);

	/* allocate space in location and store new filename */

	return(LOfstfile(buf, pathloc));
}
