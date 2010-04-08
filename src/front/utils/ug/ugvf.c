/*
**	Copyright (c) 2004 Ingres Corporation
*/


# include       <compat.h>
# include       <lo.h>
# include       <st.h>
# include	<er.h>
# include	<cm.h>
# include	<si.h>
# include       <gl.h>
# include       <sl.h>
# include       <iicommon.h>
# include       <fe.h>
# include       <ug.h>
# include	<erug.h>

# define	IsExistingDir		1
# define	IsWrtExistingDir	2
# define	IsNonExistingFile	3
# define	IsExistingFile		4
# define	IsWrtExistingFile	5


/*{
** Name:	IIUGvfVerifyFile
**
** Description:
**	This utility make it easy to check variety of file/directory name
**	and permission validation. You'll tell the utility what you expect
**	by supplying UG_CheckType. An OK return value tells you that there
**	is a match between your expectation and what the string represents.
**	
**
** Inputs:
**	str		char *		string containing the file/dir name
**	UG_MonS		bool		TRUE or FALSE  - message on success
**	UG_MonF		bool		TRUE or FALSE  - message on failure
**	UG_CheckType	int		One of: 
**
**		UG_IsExistingDir		
**		UG_IsWrtExistingDir	
**		UG_IsNonExistingFile		
**		UG_IsExistingFile		
**		UG_IsWrtExistingFile	
**
**	UG_FailType	int		UG_ERR_ERROR or UG_ERR_FATAL
**		UG_ERR_ERROR is always used for successes meaning that
**		a successful call never terminates.
**
** Outputs:
**	none	
**
**	Returns:
**		OK
**		FAIL
**		
**	Exceptions:
**		none
**
** Side Effects:
**		none
** Example:
**
** 	IIUGvfVerifyFile(str, UG_IsExistingFile, TRUE, FALSE, UG_ERR_FATAL)
**	                 |    |                  |     |      |
**	                 |    |                  |     |      Terminate
**	                 |    |                  |     No msg on failure
**			 |    |                  Display msg on success
**	                 |    Return OK if str is an existing file
**	                 File or directory to work on
**
**	This call returns OK if the passed string is an existing file and
**	FAIL otherwise. The third argument tells the routine to display a
**	message stating the existence of the file. The fourth argument tells
**	not to display a message if the passed string is not an existing file.
**	The fifth argument tells the routine to terminate and exit if the
**	condition is not met. Note that a UG_ERR_ERROR is used for all 
**	successes.
**
** History:
**	10-feb-93 (essi)
**		First written.
**	11-feb-93 (essi)
**		Changed the UG_ERR_ERROR to UG_FailType for E_UG00C4_IsDir
**		messages.
**	18-feb-93 (sandyd)
**		<si.h> is needed on VMS to pick up "FILE" typedef.
**	30-jun-93 (essi)
**		Fixed call to LOdetail.
**	18-nov-93 (mgw) Bug #56714
**		Use LOcompose() to re-compose LOdetail() decomposition. Also
**		insulate the str input arg from LOfroms() changes for accurate
**		error reporting.
**	14-dec-93 (mgw) Bug #56714
**		Botched last fix. This corrects that. When checking a path
**		for existence, be sure we're really working with a path.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

STATUS
IIUGvfVerifyFile (char *str, i4  UG_CheckType, bool UG_MonS, bool UG_MonF, i4  UG_FailType)
{
	i4		flagword;
	LOINFORMATION	loinf;
	LOCATION	loc;
	FILE		*tmpfp;
	i4		LocType;

	if ( str == NULL || *str == EOS )
	{
		if ( UG_MonF )
			IIUGerr( E_UG00C1_IsNull, UG_FailType, 0);
		return ( FAIL );
	}
	if ( LOfroms( PATH&FILENAME, str, &loc ) != OK )
	{
		if ( UG_MonF )
			IIUGerr( E_UG00C3_BadPath, UG_FailType, 1, str );
		return ( FAIL );
	}
	flagword = ( LO_I_TYPE | LO_I_PERMS );

	/* 
	** Failure of LOinfo could suggest a new file/directory spec.
	*/
	if ( LOinfo( &loc, &flagword, &loinf ) != OK )
	{
		/*
		** Since LOinfo FAILs on new files/directories (because it
		** does 'stat') we need to find a way to find out about new
		** file/directory specs. To accomplish this we strip off the
		** last part of the path and do another LOinfo.
		*/
		char	fdev[LO_DEVNAME_MAX+1], path[LO_PATH_MAX+1];
		char	fpre[LO_FPREFIX_MAX+1], fsuf[LO_FSUFFIX_MAX+1];
		char	fver[LO_FVERSION_MAX+1];
		LOCATION	newloc;
		char		newstr[MAX_LOC+1];

		STlcopy(str, newstr, MAX_LOC);
		if ( LOfroms( PATH&FILENAME, newstr, &newloc ) != OK )
		{
			if ( UG_MonF )
			{
				IIUGerr( E_UG00C3_BadPath, 
				    UG_FailType, 1, str );
			}
			return ( FAIL );
		}
		fdev[0] = EOS;
		path[0] = EOS;
		fpre[0] = EOS;
		fsuf[0] = EOS;
		if ( LOdetail(&newloc, fdev, path, fpre, fsuf, fver) != OK )
		{
			if ( UG_MonF )
			{
				IIUGerr( E_UG00CC_CannotParse, 
				    UG_FailType, 1, str );
			}
			return ( FAIL );
		}
		else
		{
			if (path[0] == EOS)
			{
				if ( LOgt(newstr, &newloc) != OK )
				{
				    if ( UG_MonF )
				    {
					IIUGerr( E_UG00C3_BadPath,
					    UG_FailType, 1, str );
				    }
				}
			}
			else
			{
				fpre[0] = EOS;
				fsuf[0] = EOS;
				fver[0] = EOS;
				if ( LOcompose(fdev, path, fpre,
				               fsuf, fver, &newloc)
				     != OK )
				{
				    if ( UG_MonF )
				    {
					IIUGerr( E_UG00C3_BadPath,
					    UG_FailType, 1, str );
				    }
				}
			}
			if ( LOfroms(PATH, newstr, &newloc) != OK )
			{
				if ( UG_MonF )
				{
					IIUGerr( E_UG00C3_BadPath, 
					    UG_FailType, 1, str );
				}
				return ( FAIL );
			}
			else
			{
				/*
				** If LOinfo fails it means the path does 
				** not exist. We take the success to mean
				** that the str is a new file spec. The
				** sure way is to try to open the file
				** subsequently, but that will be slow and
				** this test is sufficient for our purpose.
				*/
				if (LOinfo(&newloc, &flagword, &loinf) != OK)
				{
					if ( UG_MonF )
					{
						IIUGerr(E_UG00C2_PathTrap,
						    UG_FailType, 1, str);
					}
					return ( FAIL );
				}
				else
					LocType = IsNonExistingFile;
			}
		}

	}
	else
	{
		if ((flagword & LO_I_TYPE) && (loinf.li_type == LO_IS_DIR))
		{
			if ((flagword & LO_I_PERMS) &&
			    (loinf.li_perms & LO_P_WRITE))
			{
				LocType = IsWrtExistingDir;
			}
			else
			{
				LocType = IsExistingDir;
			}
		}
		if ((flagword & LO_I_TYPE) && (loinf.li_type == LO_IS_FILE))
		{
			if ((flagword & LO_I_PERMS) &&
			    (loinf.li_perms & LO_P_WRITE))
			{
				LocType = IsWrtExistingFile;
			}
			else
			{
				LocType = IsExistingFile;
			}
		}
	}

	switch ( UG_CheckType )
	{
	case	UG_IsExistingFile:
		switch ( LocType )
		{
		case	IsExistingFile:
		case	IsWrtExistingFile:
			if ( UG_MonS )
			{
				IIUGerr( E_UG00C8_IsExistingFile, 
				    UG_ERR_ERROR, 1, str);
			}
			return ( OK );
			break;

		case	IsExistingDir:
		case	IsWrtExistingDir:
			if ( UG_MonF )
			{
				IIUGerr( E_UG00C4_IsDir, 
				    UG_FailType, 1, str );
			}
			return ( FAIL );
			break;

		default:
			if ( UG_MonF )
			{
				IIUGerr( E_UG00C9_IsNotExistingFile, 
				    UG_FailType, 1, str );
			}
			return ( FAIL );
		}

	case	UG_IsWrtExistingFile:
		switch ( LocType )
		{
		case	IsWrtExistingFile:
			if ( UG_MonS )
			{
				IIUGerr( E_UG00CB_IsWrtExistingFile, 
				    UG_ERR_ERROR, 1, str);
			}
			return ( OK );
			break;

		case	IsExistingDir:
		case	IsWrtExistingDir:
			if ( UG_MonF )
			{
				IIUGerr( E_UG00C4_IsDir, 
				    UG_FailType, 1, str );
			}
			return ( FAIL );
			break;

		default:
			if ( UG_MonF )
			{
				IIUGerr( E_UG00CA_IsNotWrtExistingFile, 
				    UG_FailType, 1, str );
			}
			return ( FAIL );
		}

	case	UG_IsExistingDir:
		switch ( LocType )
		{
		case	IsExistingDir:
		case	IsWrtExistingDir:
			if ( UG_MonS )
			{
				IIUGerr( E_UG00C4_IsDir, 
				    UG_FailType, 1, str );
			}
			return ( OK );
			break;

		default:
			if ( UG_MonF )
			{
				IIUGerr( E_UG00C5_IsNotDir, 
				    UG_FailType, 1, str );
			}
			return ( FAIL );
			break;
		}
	case	UG_IsWrtExistingDir:
		switch ( LocType )
		{
		case	IsWrtExistingDir:
			if ( UG_MonS )
			{
				IIUGerr( E_UG00C6_IsWrtDir, 
				    UG_ERR_ERROR, 1, str );
			}
			return ( OK );
			break;

		default:
			if ( UG_MonF )
			{
				IIUGerr( E_UG00C7_IsNotWrtDir, 
				    UG_FailType, 1, str );
			}
			return ( FAIL );
		}

	case	UG_IsNonExistingFile:
		switch ( LocType )
		{
		case	IsNonExistingFile:
			if ( UG_MonS )
			{
				IIUGerr( E_UG00CD_IsNonExistingFile, 
				    UG_ERR_ERROR, 1, str);
			}
			return ( OK );
			break;

		case	IsExistingDir:
		case	IsWrtExistingDir:
			if ( UG_MonF )
			{
				IIUGerr( E_UG00C4_IsDir, 
				    UG_FailType, 1, str );
			}
			return ( FAIL );
			break;

		case	IsExistingFile:
		case	IsWrtExistingFile:
			if ( UG_MonF )
			{
				IIUGerr( E_UG00C8_IsExistingFile, 
				    UG_FailType, 1, str );
			}
			return ( FAIL );
			break;
		}
	default:

		IIUGerr( E_UG00C0_BadParameter, UG_ERR_ERROR, 0 );
	}
}
