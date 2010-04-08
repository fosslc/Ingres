/*
**Copyright (c) 2004 Ingres Corporation
*/

/*
** Name: fileprep
**
** Description:
**      Prepare a directory and its contents for distribution on CD.
**
**      Utility to scan a directory tree outputting to stdout a rename command
**      of the files found to uppercase.
**      If a file is found which does not follow the 8.3 naming convention the
**      command is output to stderr.
**
** Usage:
**      fileprep [path]      assume current directory if no path supplied.
**
PROGRAM=fileprep
NEEDLIBS=COMPATLIB
**
** History:
**      08-May-98 (fanra01)
**          Created.
**      01-Jun-98 (fanra01)
**          Check status from LOwcard as empty directories cause infinite
**          recursion resulting in a stack overflow.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/
# include <compat.h>
# include <cm.h>
# include <lo.h>
# include <si.h>
# include <st.h>

static char*    pszType[] ={ "Unknown", "Dir", "File", "Mem" };

static char     szFile[MAX_LOC+1] = { "\0" };

/*
** Name: dirscan
**
** Description:
**      Function given the path will find the name of entries in the path.
**      If an entry is a directory a rescan is issued with the new path.
**
**      If a file is found a rename command to uppercase is displayed.
**      If the filename does not conform to 8.3 naming the command is
**      displayed to stderr.
**
** Inputs:
**      pszPath     path to scan
**
** Outputs:
**      none.
**
** Returns:
**      OK          completed successfully.
**      !OK         failed.
**
** History:
**      01-Jun-98 (fanra01)
**          Check status from LOwcard as empty directories cause infinite
**          recursion resulting in a stack overflow.
*/
STATUS
dirscan (char* pszPath)
{
    char     szPath[MAX_LOC+1] = { "\0" };
    char     szDev[MAX_LOC+1] = { "\0" };
    char     szFilePath[MAX_LOC+1] = { "\0" };
    char     szPrefix[MAX_LOC+1] = { "\0" };
    char     szSuffix[MAX_LOC+1] = { "\0" };
    char     szVersion[MAX_LOC+1] = { "\0" };

    char     szTarget[MAX_LOC+1] = { "\0" };

    LOCATION        sLoc;
    LOCATION        srLoc;
    LO_DIR_CONTEXT  sLdc;
    LOINFORMATION   sLocinfo;
    i4              nLt = LO_I_TYPE;
    STATUS          status = 0;
    char*           pszFile;

    STcopy (pszPath, szPath);
    if ((status = LOfroms (PATH, szPath, &sLoc)) == OK)
    {
        if ((status = LOwcard (&sLoc, NULL, NULL, NULL, &sLdc)) == OK)
        {
            do
            {
                LOtos (&sLoc, &pszFile);
                if ((status = LOinfo (&sLoc, &nLt, &sLocinfo)) == OK)
                {
                    switch (sLocinfo.li_type)
                    {
                        case LO_IS_DIR:
                            status = dirscan(pszFile);
                            break;
                        case LO_IS_FILE:
                        default:
                        {
                            char *pszTmp;

                            status = LOdetail (&sLoc, szDev, szFilePath,
                                               szPrefix, szSuffix, szVersion);
                            STprintf (szTarget,"%s.%s",szPrefix, szSuffix);
                            for (pszTmp=szTarget; *pszTmp != EOS;
                                CMnext(pszTmp))
                                CMtoupper (pszTmp, pszTmp);
                            if ((STlength (szPrefix) > 8) ||
                                (STlength (szSuffix) > 3) )
                            {
                                SIfprintf (stderr, "rename %s %s\n", pszFile,
                                           szTarget);
                            }
                            else
                            {
# ifdef ALLOW_RENAME_COMMAND
                                Stcopy (pszFile, szTarget);
                                LOfroms (PATH & FILENAME, szTarget, &srLoc);
                                status = LOrename(&sLoc, &srLoc);
# endif
                                SIprintf ("rename %s %s\n", pszFile, szTarget);
                            }
                            break;
                        }
                    }
                }
                status = LOwnext (&sLdc, &sLoc);
            }
            while (status == OK);
            status = LOwend (&sLdc);
        }
    }
    return (status);
}

/*
**  Name: fileprep
**
**  Description:
**      Utility to scan a directory tree outputting to stdout a rename command
**      of the files found to uppercase.
**      If a file is found which does not follow the 8.3 naming convention the
**      command is output to stderr.
**
**  Usage:
**      fileprep [path]      assume current directory if no path supplied.
**	18-May-1998 (merja01)
**		Removed pszPath.  This char* was setup to receive the
**		return value from STcopy, but STcopy has no return 
**		value and pszpath is never used in this function.
*/
STATUS
main (int nArgc, char** pArgv)
{
    char     szPath[MAX_LOC+1] = { "\0" };
    STATUS   status = 0;

    /*
    **  if no path supplied assume current directory.
    */
    (nArgc == 1) ? STcopy (".", szPath) : STcopy(pArgv[1], szPath);
    status = dirscan (szPath);
    return (status);
}
