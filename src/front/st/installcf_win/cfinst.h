/*
**  Name:   cfinst.h
**
**  Description:
**      types
**
**  History:
**      11-Feb-2000 (mcgem01)
**          Created.
*/
# ifndef CFINST_H_INCLUDED
# define CFINST_H_INCLUDED

# include <cv.h>
# include <er.h>
# include <lo.h>
# include <me.h>
# include <nm.h>
# include <pc.h>
# include <si.h>
# include <st.h>

# include <gl.h>
# include <pm.h>

# ifndef CFCONF_H_INCLUDED
# include "cfconf.h"
# endif

# ifndef CFREG_H_INCLUDED
# include "cfreg.h"
# endif

# define MAX_PARAMS     256
# define MAX_LINE_LEN   256
# define MAX_COMP_LEN   256
# define MAX_REQUEST_LEN 256

# define MAX_READ_LINE  32000

#define HALLOC( W, X, Y ,Z )    (W = (X*)MEreqmem( 0, sizeof(X) * Y, TRUE, Z))

#define HFREE( p )              MEfree( p )
#define STRCAT( x, y )          STcat( x, y )
#define STRCMP( x, y )          STcompare( x, y )
#define STRCOPY( s, d )         STcopy( s, d )
#define STRALLOC( x )           STalloc( x )
#define STRLENGTH( x )          STlength( x )
#define STRSTRINDEX( x, y )     STstrindex( x, y , 0, TRUE)
#define STRRCHR( x, y )         STrindex( x, y )
#define SPRINTF                 STprintf
#define PRINTF                  SIprintf

# define CFTAGS     "MarkupTags.vtm"
# define ICFTAGS    "IngresMarkup.vtm"
# define STRTTAG    "<!-- -BEGIN- INGRES ICE MACRO TAGS -->"
# define ENDTAG     "<!-- -END- INGRES ICE MACRO TAGS -->"

FUNC_EXTERN STATUS
RegisterColdFusionICEExtensions( char* cfpath, char* icfpath );

# endif
