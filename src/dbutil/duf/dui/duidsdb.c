/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <pc.h>
#include    <gl.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <duf.h>
#include    <er.h>
#include    <duerr.h>

#include    <cs.h>		/* required by lk.h */
#include    <lk.h>
#include    <duenv.h>

#include    <lo.h>
#include    <me.h>
#include    <nm.h>
#include    <st.h>

/**
**
**  Name: DUIDSDB.C -	Destroydb C support routines that are owned by
**			frontends group.
**
**  Description:
**        The routines in this module are owned by the FE group.
**	They are used exclusively by the database utility, destroydb.
**
**          dui_delabfpath()	 -  destroy all of the ABF directories and
**				    contents associated with a specific db.
**
**
**  History:
**      17-Apr-87 (ericj)
**          Initial creation.
**	05-jun-87 (sandyd)
**	    Fixed case where no ABF dir exists for this database.
**	12-Oct-1988 (teg)
**	    Changed return from error to ok if no ABF dir exists for this db.
**	07-Mar-89 (teg)
**	    fixed bug 4901 (ABFDIR not deleted on UNIX).  
**      17-Jan-1990 (alexc)
**          #ifndef VMS added to ifdef out dir.fp reference to make this file
**          compilable on VMS.  Change is made in dui_rm_element().
**      01-Feb-1990 (alexc)
**          change a LOaddpath() call to LOfaddpath().
**	24-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up PID definition.
**	8-aug-93 (ed)
**	    unnest dbms.h
**      31-jan-94 (mikem)
**          Added include of <cs.h> before <lk.h>.  <lk.h> now references
**          a CS_SID type.
**      24-jul-1998 (rigka01)
**          bug 89550 - for star databases, use dud_dbenv->du_ddbname as
**          part of abf path rather than dud_dbenv->du_dbname.   For non-star
**          databases, dud_dbenv->du_ddbname is NULL.  For star databases,
**          dud_dbenv->du_ddbname is the name of the database minus the
**          ii prefix.  This change is in dui_delabfpath().
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**	13-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**/

/*{
** Name: dui_delabfpath() -	delete files associated with abf directories
**				for a given database.
**
** Description:
**        This routine deletes any files and directories associated with
**	abf directories for a given database.  Note, if this routine cannot
**	destroy one or more of the database's ABF application directories,
**	it only prints an error message and continues.
**
**
** Inputs:
**      dud_dbenv                        Ptr to a DU_ENV struct which describes
**					the environment of the database being
**					destroyed.
**	    .du_dbname			Name of db to be destroyed.
**	dud_errcb			Ptr to DU_ERROR struct for error
**					handling.
**
** Outputs:
**	Returns:
**	    E_DU_OK			Completed successfully.
**	    E_DU_UERROR			The ABF directories for this database
**					couldn't be destroyed.
**
** Side Effects:
**	      Destroys all of the ABF directories and files associated with
**	    database being destroyed.  Also destroys anything that happens to
**	    be in these directories that is not ABF related.
**
** History:
**      17-Jan-86 (annie)
**          created to fix bugs #5673 and #7098.
**      04-Sep-86 (ericj)
**          Converted for Jupiter.  Added warning message and termination of
**	    procedure if the enviroment variable, ING_ABFDIR, isn't defined.
**      17-Apr-87 (ericj)
**	    Extracted from dudutil.c and placed in this module.
**	05-jun-87 (sandyd)
**	    Fixed case where no ABF dir exists for this database.
**      12-oct-88  (teg)
**	    Changed return from error to ok if no ABF dir exists for this db.
**	07-Mar-89 (teg)
**	    fixed bug 4901 (ABFDIR not deleted on UNIX).
**	26-Jul-89 (teg)
**	    made changes to "work around" CL bugs where 1) LOlist treats
**	    subdirectories as PATH on UNIX but as file on VMS, 2) LOisdir
**	    requires I2, not i4  (as documented in CL spec)
**	17-apr-92 (teresa)
**	    rewrote to stop using obsolete LO calls.
**      24-jul-1998 (rigka01)
**          bug 89550 - for star databases, use dud_dbenv->du_ddbname as
**          part of abf path rather than dud_dbenv->du_dbname.   For non-star
**          databases, dud_dbenv->du_ddbname is NULL.  For star databases,
**          dud_dbenv->du_ddbname is the name of the database minus the
**          ii prefix.  This change is in dui_delabfpath().
**	17-Jun-2004 (schka24)
**	    Safe env var handling.
*/

DU_STATUS
dui_delabfpath(dud_dbenv, dud_errcb)
DU_ENV	    *dud_dbenv;
DU_ERROR    *dud_errcb;
{
    LO_DIR_CONTEXT  lc;
    LOCATION	    loc;
    LOCATION	    loc_file;
    LOCATION	    loc_temp;
    LOCTYPE	    flag;
    LOINFORMATION   locinfo;
    i4		    flagword;
    STATUS	    dir_stat;

    char        *cp;
    char	*path_spec;
    char        tbuf[MAX_LOC];
    char        lbuf[MAX_LOC];
    char        file_buf[MAX_LOC];


    NMgtAt("ING_ABFDIR", &cp);
    if ( cp != NULL && *cp != '\0')
    {
        STlcopy(cp, lbuf, sizeof(lbuf)-1);
        LOfroms(PATH, lbuf, &loc);
        if (*dud_dbenv->du_ddbname)
          STlcopy(dud_dbenv->du_ddbname, tbuf, sizeof(tbuf)-1);
        else
          STlcopy(dud_dbenv->du_dbname, tbuf, sizeof(tbuf)-1);
        LOfaddpath(&loc, tbuf, &loc);
    }
    else
    {
	/* the code used to detect an error if the ing_abfdir logical was
	**  not defined, cuz destroydb could not delete the temp files in
	**  this directory.  However, there is no requirement that the
	**  user use abf in their database, so it is not reasonable to
	**  require this logical before destroying a database.  If the
	**  user really did have abf, then these files wont be cleaned up
	**  where destroydb deletes the other disk files.  Anyhow, destroydb
	**  used to generate this error here:
	**   return(du_error(dud_errcb, E_DU3081_NO_ABFLOG_DS, 0))
	*/
	
	return(E_DU_OK);
    }

    /*
    ** If no ABF directory exists for this database, exit now
    ** since there is nothing to delete.
    */
    flagword = LO_I_TYPE;
    LOinfo(&loc, &flagword, &locinfo);
    if (  (flagword & LO_I_TYPE) == 0 || locinfo.li_type != LO_IS_DIR ) 
    {
	/* the ABF directory does not exist */
	return(E_DU_OK);
    }

    /*
    **  associate a buffer with loc_file which will be filled by LOlist
    */
    file_buf[0] = EOS;
    LOcopy (&loc, file_buf, &loc_file);

    if (LOwcard (&loc_file, NULL, NULL, NULL, &lc) != OK)
    {
	/* the directory is probably empty, so abort the search */
	_VOID_ LOwend(&lc);
        /* and delete the empty abf directory */
        if (LOdelete(&loc) != OK)
        {
	    LOtos(&loc, &path_spec);
	    du_error(dud_errcb, E_DU3080_DELETE_DIR_DS, 2, 0, path_spec);
	    SIprintf("%s\n", dud_errcb->du_errmsg);
	}
	return (E_DU_OK);
    }
    else
    {
	/* copy the result to another location */
	LOcopy(&loc_file, tbuf, &loc_temp);

	/* now delete file, or subdirectory */
	if (dui_rm_element( &loc_temp, dud_errcb) != E_DU_OK)
	{
	    /* add some type of error handling here?? */
	    return (E_DU_UERROR);
	}
	file_buf[0] = EOS;
	LOcopy (&loc, file_buf, &loc_file);
    }
    while ((dir_stat = LOwnext(&lc, &loc_file)) == OK)
    {
	/* copy the result to another location */
	LOcopy(&loc_file, tbuf, &loc_temp);

	/* now delete file, or subdirectory */
	if (dui_rm_element( &loc_temp, dud_errcb) != E_DU_OK)
	    break;	

	file_buf[0] = EOS;
	LOcopy (&loc, file_buf, &loc_file);

    }
    if (dir_stat == ENDFILE)
    {	
	/* terminate the search, since EOF was reached */
	_VOID_ LOwend(&lc);
    }
    else
    {
	/* report error here */
	LOtos(&loc_temp, &path_spec);
	du_error(dud_errcb, E_DU3080_DELETE_DIR_DS, 2, 0, path_spec);
	SIprintf("%s\n", dud_errcb->du_errmsg);
    }

    
    /* finally, delete the abf directory itself */
    if (LOdelete(&loc) != OK)
    {
	LOtos(&loc, &path_spec);
	du_error(dud_errcb, E_DU3080_DELETE_DIR_DS, 2, 0, path_spec);
	SIprintf("%s\n", dud_errcb->du_errmsg);
    }

    return(E_DU_OK);    
}

/*{
** Name: dui_rm_element() -	delete specified file/directory
**
** Description:
**        This routine recursively deletes any files and directories associated
**	  with abf directories for a given database.  If the specified location
**	  was a file, the routine deletes it.  If it was a directory, it
**	  translates the name from node:[path.subpath]directory.dir to
**	  node:[path.subpath.directory] and deletes all members in that
**	  diretory (or any subdirectories).  This is managed recursively.
**
**	  NOTE:  THERE IS A KLUDGE IN THIS ROUTINE BECAUSE LO DOES NOT SUPPORT
**		 ALL OPERATIONS THAT ARE NEEDED.
**
**
** Inputs:
**      loc				LOCATION descriptor containing a file
**					    or directory name.
**	dud_errcb			Ptr to DU_ERROR struct for error
**					handling.
**
** Outputs:
**	Returns:
**	    E_DU_OK			Completed successfully.
**	    E_DU_UERROR			The ABF directories for this database
**					couldn't be destroyed.
**
** Side Effects:
**	      Destroys all of the ABF directories and files associated with
**	    database being destroyed.  Also destroys anything that happens to
**	    be in these directories that is not ABF related.
**
** History:
**	07-Mar-89 (teg)
**	    created to fix bug 4901 (ABF directorys not correctly destroyed when
**	    ING_ABFDIR is set.)
**	26-Jul-89 (teg)
**	    made changes to "work around" CL bugs where 1) LOlist treats
**	    subdirectories as PATH on UNIX but as file on VMS, 2) LOisdir
**	    requires I2, not i4  (as documented in CL spec)
**      17-jan-1990 (alexc)
**          #ifndef VMS to make this compilable.
**      01-feb-1990 (alexc)
**          Change a call from LOaddpath() to LOfaddpath().
**	19-sep-92 (teresa)   [from 6.4 line]
**	    Fix portability bug by redoing a portion of the LO calls to stop
**	    doing an LOdetail and LOcopy (there is a VMS LOcopy bug where
**	    portions of the LOC structure are uninitialized.  Previously,
**	    this routine was setting LOC.fp to NULL to work around that bug,
**	    bug that is not legal for portability reasons.  So replaced
**	    a set of LO calls with another set that does not require any coding
**	    standards violations.
**	17-apr-92 (teresa)
**	    rewrote to stop using obsolete LO calls.
*/
DU_STATUS
dui_rm_element(loc, du_errcb)
LOCATION    *loc;
DU_ERROR    *du_errcb;
{
    LO_DIR_CONTEXT  lc;
    LOCATION    member,dir;
    LOCATION    loc_temp;
    char        tbuf[MAX_LOC];
    char	*path_spec;
    STATUS	rc;

    LOINFORMATION   locinfo;
    i4		    flagword;
    char	dir_string[MAX_LOC];
    char	member_buf[MAX_LOC];
    char	*str_ptr;



	/* Convert the full path into a result directory location.
	** i.e. convert from [.dbname]app.dir to [.dbname.app] (or the
	** equvalent for other platforms.
	*/
	MEfill (sizeof(dir),'\0',(PTR) &dir);
	LOcopy(loc, tbuf, &loc_temp);
	LOtos(&loc_temp,&str_ptr);
	LOfroms(PATH,str_ptr,&dir);

	flagword = LO_I_TYPE;
	LOinfo(&dir, &flagword, &locinfo);
	if ( (flagword & LO_I_TYPE) && (locinfo.li_type == LO_IS_DIR) )
	{
	    /*
	    **  associate a buffer with loc_file which will be filled by LOlist
	    */
	    member_buf[0] = EOS;
	    LOcopy(&dir, member_buf, &member);

	    if ( (rc=LOwcard (&member, NULL, NULL, NULL, &lc)) == OK)
	    {
		/* now delete file, or subdirectory */
		if (dui_rm_element( &member, du_errcb) != E_DU_OK)
		{	
		    return (E_DU_UERROR);
		}
		/* reset the member location for the next iteration */
		member_buf[0] = EOS;
		LOcopy(&dir, member_buf, &member);

		while ((rc = LOwnext(&lc, &member))==OK)
		{

		    /* now delete file, or subdirectory */
		    if (dui_rm_element( &member, du_errcb) != E_DU_OK)
			break;

		    /* reset the member location for the next iteration */
		    member_buf[0] = EOS;
		    LOcopy(&dir, member_buf, &member);
		}
	    }
	    /* when we get here, the directory tree is empty, so close off
	    ** the search */
	    _VOID_ LOwend(&lc);
	    if (rc != ENDFILE)
	    {
		/* report error here */
		LOtos(&dir, &path_spec);
		du_error(du_errcb, E_DU3080_DELETE_DIR_DS, 2, 0, path_spec);
		SIprintf("%s\n", du_errcb->du_errmsg);
	    }

	}

	/* delete file or directory */
        if (LOdelete(loc) != OK)
	{
	    LOtos(loc, &path_spec);
	    du_error(du_errcb, E_DU3080_DELETE_DIR_DS, 2, 0, path_spec);
	    SIprintf("%s\n", du_errcb->du_errmsg);
	}

	return ((DU_STATUS) E_DU_OK);
}
