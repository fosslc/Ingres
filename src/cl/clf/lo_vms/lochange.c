# include 	<compat.h> 
# include	<gl.h>
# include	<rms.h>
# include	<iledef.h>
# include	<lnmdef.h>
# include	<descrip.h>
# include	<lo.h> 
# include	<er.h> 
# include 	"lolocal.h"
# include 	"loparse.h"
# include	<starlet.h>

/*
**  LOchange -- change the default directory and device
**	LOchange moves the current image to the directory specified
**	as a string.
**
**	Parameters:
**		loc -- description of new directory string
**
**	Returns:
**		STATUS - OK if successful. NO_SUCH if location doesn't
**			 exist. Fail otherwise.
**	History:
**		9/13/83 -- written for VMS CL (dd)
**		4/19/89 -- use LOparse and LOclean_fab (Mike S)
**      16-jul-93 (ed)
**	    added gl.h
**	19-jul-2000 (kinte01)
**	   Correct prototype definitions by adding missing includes
**	09-oct-2008 (stegr01/joea)
**	    Replace ITEMLIST by ILE3.
*/
typedef struct dsc$descriptor DSC$DESC;
typedef struct FAB	_FAB;
typedef struct NAM	_NAM;

/* We set default device by defining logical SYS$DISK in the process table */
static	$DESCRIPTOR(sysdisk, "SYS$DISK");
static  $DESCRIPTOR(proc_table, "LNM$PROCESS_TABLE");

STATUS
LOchange(loc)
register LOCATION	*loc;
{
	LOCATION	rootloc;  	/* Used to add MFD to a bare device */
	char		rootstr[MAX_LOC+1];/* ditto */
	ILE3		itemlist[2];	/* Itemlist for logical name setting */
	DSC$DESC	dirdesc;	/* Directory descriptor */
	LOpstruct	pstruct;	/* Parse structure */
	STATUS		sts;		/* Status */
	STATUS		vmssts;		/* VMS service status */

	/* assume error */
	sts = LO_NO_SUCH;

	/* 
	** We accept DEV: as short for DEV:[000000].  If the location contains
	** no directory, assume that's what was meant.
	*/                                               
	if (loc->dirlen == 0)
	{
		LOdev_to_root(loc->string, rootstr);        	
		if (LOfroms(loc->desc, rootstr, &rootloc) != OK)
			return (FAIL);
		loc = &rootloc;
	}
	/* Check existence of location */
	vmssts = LOparse(loc->string, &pstruct, TRUE);

	if ((vmssts & 1) == 0) goto cleanup;	/* Not a legal directory */

	/* set device default by defining SYS$DISK */
	itemlist[0].ile3$w_length = pstruct.pnam.nam$b_dev;
	itemlist[0].ile3$w_code = LNM$_STRING;
	itemlist[0].ile3$ps_bufaddr = pstruct.pnam.nam$l_dev;
	itemlist[0].ile3$ps_retlen_addr = NULL;
	itemlist[1].ile3$w_length = itemlist[1].ile3$w_code = 0;

        vmssts = sys$crelnm(0, &proc_table, &sysdisk, 0, itemlist);
	if ((vmssts & 1) == 0)
	{
		sts = FAIL;
		goto cleanup;
	}

	/* set directory default */
	dirdesc.dsc$w_length = pstruct.pnam.nam$b_dir;
	dirdesc.dsc$a_pointer = pstruct.pnam.nam$l_dir;
	if((sys$setddir(&dirdesc, 0, 0) & 1) == 0)
	{
		sts = FAIL;
		goto cleanup;
	}
	
	/* if successful */
	sts = OK;		/* fall through to cleanup */

cleanup:
	LOclean_fab(&pstruct.pfab);
	return (sts);
}
