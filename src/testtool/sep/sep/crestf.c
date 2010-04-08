/*
**    Include file
*/

#include <compat.h>
#include <si.h>
#include <st.h>
#include <lo.h>
#include <er.h>
#include <termdr.h>

#define crestf_c

#include <sepdefs.h>
#include <fndefs.h>

/*
** History:
**	26-apr-1994 (donj)
**	    Created.
**    17-may-1994 (donj)
**      VMS needs <si.h> for FILE structure declaration.
**	24-may-1994 (donj)
**	    Took out tracing GLOBALREF's. Unused.
**	31-may-1994 (donj)
**	    Took out #include for sepparam.h. QA_C was complaining about all
**	    the unecessary GLOBALREF's.
*/

/*
    Reference global variables
*/

GLOBALREF    char         *ErrC ;
GLOBALREF    char         *creloc ;
GLOBALREF    char          SEPpidstr [] ;

GLOBALREF    bool          batchMode ;

STATUS
Create_stf_file( i2 ME_tag, char *prefix, char **fname, LOCATION **floc,
		 char *errmsg )
{
    STATUS                 ret_val = OK ;

    if (*fname == NULL)
    {
	*fname = (char *)SEP_MEalloc(ME_tag, MAX_LOC+1, TRUE, &ret_val);
    }

    if ((ret_val == OK)&&(floc != NULL)&&(*floc == NULL))
    {
	*floc  = (LOCATION *)SEP_MEalloc(ME_tag, sizeof(LOCATION), TRUE,
					 &ret_val);
    }

    if (ret_val != OK)
    {
	disp_line(STpolycat(2, ErrC, ERx("Allocate Memory in Create_stf_file."),
			       errmsg), 0, 0);
    }
    else
    {
	STpolycat(3, prefix, SEPpidstr, ERx(".stf"), *fname);

	/* create location for file */

	if (floc != NULL)
	{
	    if ((ret_val = LOfroms(FILENAME & PATH, *fname, *floc)) != OK)
	    {
		disp_line(STpolycat(4, ErrC,creloc, ERx(" stf file, "), *fname,
				       errmsg), 0, 0);
	    }
	}
    }

    return (ret_val);
}
