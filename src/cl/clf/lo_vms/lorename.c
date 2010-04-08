/*
**    Copyright (c) 1983, 2000 Ingres Corporation
*/
# include	<compat.h>  
# include	<gl.h>
# include	<rms.h>
# include	<descrip.h>
# include	<lib$routines.h>
# include	<lo.h>  
# include	<er.h>  
# include	<st.h>  
# include	"lolocal.h"

/*LOrename
**
**	LOrename changes the name of the file specified by "old_loc" to
**	that specified by "new_loc"
**
**	Parameters:
**		old_loc		location of file 
**		new_loc		location containing new name for the file
**
**	Side Effects:
**		The directories which hold and will hold the files will be
**		modified.
**
**	History:
**		Written for VMS CL 9/26/83 (dd)
**		10/89 (Mike S)  Explicit check for directory rename.
**
**      16-jul-93 (ed)
**	    added gl.h
**	19-jul-2000 (kinte01)
**	   Correct prototype definitions by adding missing includes
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
*/

STATUS
LOrename(old_loc, new_loc)
LOCATION	*old_loc;
LOCATION	*new_loc;
{
	char old_string[MAX_LOC+1];
	char new_string[MAX_LOC+1];
	$DESCRIPTOR(old_desc, old_string);
	$DESCRIPTOR(new_desc, new_string);
	STATUS	rmsstatus;
	i4	num_renamed;

	/*
	** Directory renaming is explicitly verboten, even though it might be
	** useful.  If we ever do it, the steps will be:
	**
	** 1. Use LOdir_to_file to get the "dirname.dir;1" syntax for the
	**    directories.
	** 2. Be sure we have "delete" access to the exiting directory.
	**    This is needed to remove the directory's old name.
	** 3. Call lib$rename_file the usual way.
        */
	if ((old_loc->desc | FILENAME) != FILENAME)
	{
		return (LO_NOT_FILE);
	}
	if ((new_loc->desc | FILENAME) != FILENAME)
	{
		return (LO_NOT_FILE);
	}

	/* We don't allow renaming of anything less than all versions */
	if (old_loc->verlen != 0 || new_loc->verlen != 0)
		return (FAIL);

	/* 
	** Now we go through this silly loop which is the only good way
	** to rename all versions.  
	**
	**	RENAME old.file;* new.file;0 
	**	
	** makes the oldest versions of old.file into the newest versions
	** of new.file.  Instead, we rename old.file;-0 to new.file;0
	** until nothing's left.
	*/
  	STpolycat(2, old_loc->string, ERx(";-0"), old_string);
  	STpolycat(2, new_loc->string, ERx(";0"), new_string);
	old_desc.dsc$w_length = STlength(old_string);
	new_desc.dsc$w_length = STlength(new_string);
	
	for (num_renamed = 0; ; num_renamed++)
	{
		rmsstatus = lib$rename_file(&old_desc, &new_desc);
		if(( rmsstatus & 1) != 1)
			break;
	}

	/* If we renamed at least one, we'll call it success */
	if (num_renamed > 0)
		return(OK);

	/* Otherwise, we'll return the error status */
	if(rmsstatus == RMS$_PRV)
		return(LO_NO_PERM);
	if((rmsstatus == RMS$_FNF) || (rmsstatus == RMS$_DNF))
		return(LO_NO_SUCH);
	return(FAIL);
}
