
# include	"LOerr.h"

/*
**    History:
**
**	19-may-95 (emmag)
**	    Branched for NT.
**  01-Jul-2005 (fanra01)
**      Sir 114805
**      Add string definitions of colon and UNC path introducer.
*/

# ifndef CLCONFIG_H_INCLUDED
        # error "didn't include clconfig.h before lolocal.h (in cl/clf/lo)"
# endif

/* Constants */

# define        MAXSTRINGBUF    64      /* max length of a string buffer */
# define        COLON           ':'
# define        STAR            '*'
# define        PERIOD          '.'
# define        SLASH           '\\'
# define        BACKSLASH       '\\'
# define        SLASHSTRING     "\\"
# define        COLONSTRING     ":"
# define        UNCSTRING       "\\\\"

# define	BYTEMASK	0177

/* used to tell whether to open a file on successive calls to LOlist */

# define	FILECLOSED	-1
