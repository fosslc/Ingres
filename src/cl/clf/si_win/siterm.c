# include	<compat.h>
# include	<gl.h>
# include	<si.h>

# ifdef NT_GENERIC
#include <sys/types.h>
#include <sys/stat.h>
# endif /* NT_GENERIC */

/*
** Tests whether I/O stream 'stream' is a tty or not.
** We first find the file descriptor of the stream
** and use this value to check if 'stream' is a tty or not.
** Will return TRUE if 'stream' is a tty, otherwise
** FALSE will be returned.
**
** History:
**	19-jun-95 (emmag)
**          Should always return TRUE on NT.
**	18-jul-95 (tutto01)
**	    Now use fstat to determine whether stream is a tty or not on NT.
**	20-Jul-2004 (lakvi01)
**		SIR #112703, cleaned-up warnings.
**      26-Jul-2004 (lakvi01)
**          Backed-out the above change to keep the open-source stable.
**          Will be revisited and submitted at a later date. 
*/

bool
SIterminal(stream)
FILE	*stream;
{
# ifndef NT_GENERIC
	if (isatty(fileno(stream)) == 1)
	{
		return(TRUE);
	}
	else
	{
		return(FALSE);
	}
# else
int result;
struct stat buf;

      result=_fstat(_fileno(stream), &buf);
      if ( result !=0 ) return(TRUE);

      if (buf.st_mode & _S_IFREG) return(FALSE);
      else return(TRUE);
# endif
}

