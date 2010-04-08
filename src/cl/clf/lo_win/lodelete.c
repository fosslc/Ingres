# include	<compat.h>
# include	<gl.h>
# include	<clconfig.h>
# include	<systypes.h>
# include	<pc.h>
# include	<ex.h>
# include	<si.h>
# include	<lo.h>
# include	"lolocal.h"
# include	<sys/stat.h>

/*
** Copyright (c) 1985 Ingres Corporation
**
**
**  LOdelete -- Delete a location.
**
**	The file or directory specified by loc is destroyed if possible.
**
**	success:  LOdelete deletes the file or directory and returns-OK.
**		  LOdelete will not recursively delete directories.  If
**		  a directory exist within the directory then it will fail.
**	failure:  LOdelete deletes no file or directory.
**
**		For a file, returns NO_PERM if the process does not have
**		  permissions to delete the given file or directory.  LOdelete
**		  returns NO_SUCH if part of the path, the file, or the direc-
**		  tory does not exist.
**
**	History:
**	19-may-95 (emmag)
**	    Branch for NT.
**	06-may-1996 (canor01)
**	    Clean up compiler warnings.
**	05-aug-1997 (somsa01)
**	    Rather than reset a file's attributes to FILE_ATTRIBUTE_NORMAL,
**	    we now unset the appropriate attributes of a file/directory so
**	    that we can delete it.
*/

static STATUS LO_delete( char *name );

STATUS
LOdelete(register LOCATION *loc)
{

    if (loc->string == NULL) /* no path in location */
        return LO_NO_SUCH;
    return LO_delete(loc->string);
}

static
STATUS
LO_delete( char *name )
{
    ULONG status = OK;
    DWORD    attr;

    attr = GetFileAttributes(name);
    if (attr == -1)
    {
       return(LOerrno(GetLastError()));
    }


    if(attr & (FILE_ATTRIBUTE_READONLY|FILE_ATTRIBUTE_HIDDEN|\
             FILE_ATTRIBUTE_SYSTEM|FILE_ATTRIBUTE_ARCHIVE))
    {
	attr &= ~(FILE_ATTRIBUTE_READONLY|FILE_ATTRIBUTE_HIDDEN|
			FILE_ATTRIBUTE_SYSTEM|FILE_ATTRIBUTE_ARCHIVE);
	SetFileAttributes(name, attr);
    }

  if (attr & FILE_ATTRIBUTE_DIRECTORY)
  {
      char  dirname[MAX_LOC];
      char  fname[MAX_LOC];

      WIN32_FIND_DATA findbuf;
      HANDLE  hfind;

      strcpy(dirname, name);
      strcat(dirname, "\\*.*");

      status = OK;
      if((hfind = FindFirstFile(dirname, &findbuf)) != INVALID_HANDLE_VALUE)
      {
         do
         {
             if (findbuf.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                  continue;

             if ((findbuf.dwFileAttributes & 
			(FILE_ATTRIBUTE_READONLY |
			FILE_ATTRIBUTE_HIDDEN |
                        FILE_ATTRIBUTE_SYSTEM)) &&
                        (SetFileAttributes(dirname, FILE_ATTRIBUTE_NORMAL) 
			== FALSE))
                  break;
            strcpy(fname, name);
            strcat(fname,"\\");
            strcat(fname, findbuf.cFileName);
            status = (DeleteFile(fname) == FALSE) ? GetLastError() : OK;
            if(status != OK)
                break;
         } while (FindNextFile(hfind, &findbuf));
         FindClose(hfind);
      }
      if(status == OK)
         status = (RemoveDirectory(name) == FALSE) ? GetLastError() : OK;
  }
  else
     status = (DeleteFile(name) == FALSE) ? GetLastError() : OK;

    return (status!=OK) ? LOerrno(status) : OK;
}
