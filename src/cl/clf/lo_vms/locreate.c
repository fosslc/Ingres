/*
**		Copyright (c) 1983, 2000 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include 	<jpidef.h>
# include	<rms.h>
# include	<descrip.h>
# include	<starlet.h>
# include	<lib$routines.h>
# include	<lo.h>  
# include	<me.h>  
# include	<st.h>  
# include	<cm.h>  
# include	<er.h>  
# include	"lolocal.h"  

typedef struct NAM	_NAM;
typedef struct FAB	_FAB;


static STATUS mkdir();
static STATUS create();

/*LOcreate
**
**	Create location.
**	
**	The file or directory specified by lo is created if possible. The
**	file or directory is empty, but the exact meaning of empty is imple-
**	mentation-dependent.
**
**	success: LOcreate creates the file or directory and returns-OK.
**	failure: LOcreate creates no file or directory.
**		 LOcreate returns NO_PERM if the process does not have
**		 permissions to create file or directory. LOcreate returns
**		 NO_PERM if mkdir tries to create a directory under a non
**		 existant directory.  LOcreate will return
**		 NO_SPACE if not enough space to create. LOcreate returns
**		 NO_SUCH if path does not exist. LOcreate returns FAIL 
**		 if some other situation comes up.
**
**
**		History
**			03/09/83 -- (mmm)
**				written
**			09/15/83 -- (DD) VMS CL
**			08/11/85 -- (ac) 
**				Bug fix for VMS 4.* file ownership problem.
**				Directory created should be owned by UIC 
**				of the current running process rather than the
**				UIC of the parent directory.
**			04/19/89 -- (Mike S)
**				Use LOparse and LOcleanup
**			10/3/89 -- (Mike S)
**				Clean up
**      16-jul-93 (ed)
**	    added gl.h
**      11-aug-93 (ed)
**          added missing includes
**	19-jul-2000 (kinte01)
**	   Correct prototype definitions by adding missing includes
*/

/* static char	*Sccsid = "@(#)LOcreate.c	1.4  6/1/83"; */

STATUS
LOcreate(loc)
register LOCATION	*loc;

{
	STATUS 		rmsstatus;

	if ((loc->string == NULL) || (*(loc->string) == NULL))	
	{	
		/* no path in location */
		return(LO_NO_SUCH);
	}

	if (loc->desc == (PATH & NODE))
	{
		/* Don't create a direcotry across the net */
		return(LO_NO_PERM);
	}

	if (loc->desc == PATH)
	{
		/* then create a directory */
		return(mkdir(loc->string));
	}
	else
	{
		/* try to create a file */

		/* make sure file name begins with alphanumeric characters */
		if (loc->namelen == 0)
			return (FAIL);	/* No file name */		
		if(!CMalpha(loc->nameptr) && !CMdigit(loc->nameptr))
			return(FAIL);	/* illegal file name */
       		rmsstatus = create(loc->string);
		if ((rmsstatus & 1) == 1)
			return(OK);
		if (rmsstatus == RMS$_DNF)		
			return(LO_NO_SUCH);
		if (rmsstatus == RMS$_PRV)
			return(LO_NO_PERM);
		return(FAIL);
	}
}



/*
**	Create a directory using the rtl routine LIB$CEATE_DIR
*/

# define PROT_ENABLE 0xFFFF		/* Enable mask -- ignore parent */ 
# define PROT_VALUE  0xFF00		/* O:RWED, S:RWED */

static STATUS
mkdir(dir)
char	*dir;	/* Directory name */
{
	i4 uic;				/* User's UIC */
	i4 item = JPI$_UIC;		/* getjpi item */
	i4 penable = PROT_ENABLE;	/* protection enable mask */
	i4 pvalue = PROT_VALUE;		/* protection value mask */
	$DESCRIPTOR(desc, dir);

	/* Get UIC  */
	lib$getjpi(&item, 0, 0, &uic, 0, 0);

	/* Create directory */
	desc.dsc$w_length = STlength(dir);
	if ((lib$create_dir(&desc, &uic, &penable, &pvalue, 0, 0) & 1) == 1)
		return (OK);
	else
		return (FAIL);
}		

/*
**	CREATE	- create a file
**
**	Parameters:
**		name	- the name of the file
**		type	- the type of file
**
**	Returns:
**		RMS status
*/
static STATUS
create(name)
char	*name;
{
	_FAB	fab;
	_NAM	nam;
	char	esabuf[MAX_LOC+1];

	/*	Initialize the RMS fab		*/

	MEfill( sizeof(fab), NULL, (PTR)&fab);
	fab.fab$b_bid = FAB$C_BID;
	fab.fab$b_bln = FAB$C_BLN;
	fab.fab$b_fns = STlength(fab.fab$l_fna = name);
	fab.fab$l_fop = FAB$M_CBT;
	fab.fab$b_rfm = FAB$C_VAR;
	fab.fab$b_rat = FAB$M_CR;
	fab.fab$b_org = FAB$C_SEQ;

	/*	Initialize the RMS nam		*/

	MEfill( sizeof(nam), NULL, (PTR)&nam);
	nam.nam$b_bid = NAM$C_BID;
	nam.nam$b_bln = NAM$C_BLN;
	nam.nam$l_esa = esabuf;
	nam.nam$b_ess = sizeof(esabuf);
	fab.fab$l_nam = &nam;

	/*	create the file			*/

	if ((sys$create(&fab) & 1) == 0)
		return (fab.fab$l_sts);

	sys$close(&fab);
	return (fab.fab$l_sts);
}
