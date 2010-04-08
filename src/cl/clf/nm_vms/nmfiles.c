# include	  <compat.h>
# include	  <gl.h>
# include	  <er.h>  
# include	  <lo.h>  
# include	  <nm.h>  
# include	  <st.h>  
# include	  <si.h>
# include	  "nmerr.h"

static	char		glob_buf[MAX_LOC];

/*
	NMfiles.c
		contains the following routines
			NMsyspath()
			NMloc()
			NMf_open()
			NMt_open()
		History
			10/13/83 (dd)	VMS CL
			6/7/85 (ac)	ING_TEMP in VMS should looks for
					the logical of "TEMP_INGRES" instead of
					"ING_TEMP"
**	03-aug-1985 (greg)
**	    zap obsoleted symbols from NMloc(), zap NMIngAt()
**	    NMsyspath
**	        STcopy is a VOID so don't set length to it's return
**			which was garbage ???  Call STlength().
**		don't implicitly VOID LOfroms and STpolycat; explicitly
**			VOID later, check status of former
**	02-16-90 (Mike S)
**		Use LOfaddpath in NMsyspath to construct subdirectory name.
**      16-jul-93 (ed)
**	    added gl.h
**	19-oct-99 (kinte01)
**	    added dummy NMflushIng() routine introduced into generic code
**	    by fix for bug 98648
**	22-Mar-2010 (hanje04)
**	    SIR 123296
**	    Define ADMIN & LOG to be the same as FILES for VMS as ADMIN is used
**	    as the 'writable' location for LSB builds. Change should have no
**	    net effect for VMS.
*/


/*
	NMsyspath()
		set LOsys to contain the path to the 'subdir' under the
			ingres home directory.  'subdir' may be NULL.

		As a global buffer is associated with 'LOsys' it may
			be advisable to do an LOcopy() upon return.
*/

static STATUS
NMsyspath(subdir, LOsys)
char		*subdir;
LOCATION	*LOsys;
{
	LOCATION	t_loc;
	LOCATION	*pt_loc;
	STATUS		CLerror;
	char		*dir_name;

	pt_loc = &t_loc;

	if ((CLerror = NMpathIng(&dir_name)) == OK)
	{
		STcopy(dir_name, glob_buf);

		if (((CLerror = LOfroms(PATH, glob_buf, LOsys)) == OK) &&
		    (subdir && *subdir))
		{
			CLerror = LOfaddpath(LOsys, subdir, LOsys);
		}
	}

	return(CLerror);
}

/*
** NMIngAt
**	dummy routine until CLF_XFER_VECTOR can be changed
*/

STATUS
NMIngAt()
{
    return(OK);
}

/*
	NMloc()
		set 'ploc1' to point to LOCATION whose buffer contains the
			path to 'fname' (which depending on the value of
			'what' is some combination of FILENAME, PATH, DEV)
			and which is located in a sub-directory of
			II_SYSTEM:[ingres]. 'which' is one of the following:
			    BIN	     for executables
			    FILES    for error files
			    ADMIN    same as files, needed for Linux
			    LOG	     same as files, needed for Linux
			    UTILITY  for users, [ext,loc,acc].bin
			    LIB	     for INGRES libraries
			    TEMP     for temporary files
			    DBTMPLT  database template directory
			    SUBDIR   generic subdirectory of II_SYSTEM:[ingres]

		NOTE:
			TEMP is a special case in that if ING_TEMP is not
				defined the path chosen is the directory
				the process is running in.

			As a global buffer is associated with 'ploc1' it may
				be advisable to do an LOcopy() upon return.
*/

STATUS
NMloc(
char		which,
LOCTYPE		what,
char		*fname,
LOCATION	*ploc1)
{
	STATUS		CLerror = OK;
	char		*fname_ptr;
	char		ii_symbol[16];
	char		dname[16];


	switch (which)
	{
		case BIN:
			STcopy("II_BINARY", ii_symbol);
			STcopy("bin", dname);
			break;

	        case UTILITY:
			STcopy("II_DBMS_CONFIG", ii_symbol);
			STcopy("config", dname);
			break;

		case FILES:
		case ADMIN:
		case LOG:
			STcopy("II_CONFIG", ii_symbol);
			STcopy("files", dname);
			break;

		case DBTMPLT:
			STcopy("II_TEMPLATE", ii_symbol);
			STcopy("dbtmplt", dname);
			break;

		case LIB:
			STcopy("II_LIBRARY", ii_symbol);
			STcopy("library", dname);
			break;

		case SUBDIR:
			CLerror = (what & PATH) ? NMsyspath(fname, ploc1) :
						  NM_LOC;
			return(CLerror);
			break;

		case TEMP:
			STcopy("II_TEMPORARY", ii_symbol);
			break;

		default:
			return(NM_LOC);
	}


	NMgtAt(ii_symbol, &fname_ptr);

	if (fname_ptr && *fname_ptr)
	{
		STcopy(fname_ptr, glob_buf);
		CLerror = LOfroms(PATH,     glob_buf, ploc1);
	}
	else if (which == TEMP)
	{
		CLerror = LOgt(glob_buf, ploc1);
	}
	else
	{
		CLerror = NMsyspath(dname, ploc1);
	}

	if ((CLerror == OK) && (fname))
	{
		if (what == FILENAME)
		{
			CLerror = LOfstfile(fname, ploc1);
		}
		else if (what & PATH)
		{
			CLerror = LOfaddpath(ploc1, fname, ploc1);
		}
	}

	return(CLerror);
}


/*
	NMf_open()
		open 'fname' which is in the directory sys_ingres:[ingres.files] with 'mode' of
			RD, WT or AP.  Set 'fptr' to the file pointer of the
			opened file, type is TXT, BIN, VAR
*/

STATUS
NMf_open(mode, fname, fptr)
char	*mode;
char	*fname;
FILE	**fptr;
{
	LOCATION	loc;
	STATUS		CLerror;


	if ((CLerror = NMloc(FILES, FILENAME, fname, &loc)) == OK)
		CLerror = SIopen(&loc, mode, fptr);

	return(CLerror);
}

/*
	NMt_open()
		create and open with 'mode' &  'type', a uniquly named file (prefix = 'fname'
			and suffix = 'suffix') in the ingres temp directory.
		set 'fptr' to be FILE * associated with 'fname' and fill in 't_loc'
			so it may be used in subsequent call to LOdelete().

		As a static buffer is associated with 't_loc' it may
			be advisable to do an LOcopy() upon return.
*/

STATUS
NMt_open(mode, fname, suffix, t_loc, fptr)
char			*mode;
char			*fname;
char			*suffix;
LOCATION		*t_loc;
FILE			**fptr;
{
	LOCATION	loc1;
	STATUS		CLerror;
	static char	loc_buf[MAX_LOC];


	LOfroms(PATH & FILENAME, loc_buf, &loc1);
	LOuniq(fname, suffix, &loc1);

	if ((CLerror = NMloc(TEMP, FILENAME, loc_buf, t_loc)) == OK)
		CLerror = SIopen(t_loc, mode, fptr);

	return(CLerror);
}

/*{
** Name: NMflushIng - flush the internal cache
**
** Description:
**
** Inputs:
**
** Output:
**
**      Returns:
**
** History:
**      10-Mar-89 (GordonW)
**	   created on Unix
**	19-Oct-99 (kinte01)
**	   added to VMS CL as a dummy routine
*/
STATUS
NMflushIng()
{
        return(OK);
}

