/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
** Name: iceutil.h
**
** Description:
**      File contains declarations for the setup utilities to abstract the
**      compatibility facilities.
**      Includes structure declarations for the lexical analyser.
**
## History:
##      21-Feb-2001 (fanra01)
##          Sir 103813
##          Created.
*/
# include <compat.h>
# include <cv.h>
# include <er.h>
# include <me.h>
# include <si.h>
# include <st.h>

# include <gl.h>
# include <iicommon.h>
# include <fe.h>
# include <ug.h>

# include <erwu.h>

# define MEMALLOC( size, type , clear, stat, mptr ) \
    { \
        mptr = (type)MEreqmem( 0, size, clear, &stat ); \
    }

# define MEMFREE( mptr ) MEfree( (PTR)mptr )

# define MEMCOPY( src, size, dst )  MEcopy( src, size, dst )

# define MEMFILL( size, filler, mptr )  MEfill( size, filler, mptr )

# define PRINTF             SIprintf

# define FPRINTF            SIfprintf

# define FFLUSH             SIflush

# define STLENGTH( mptr )   STlength( mptr )

# define STRCAT                 STcat

# define STRCMP( str, cstr ) STcompare( str, cstr )

# define STRCOPY( src, dest )   STcopy( src, dest )

# define STRPRINTF          STprintf

# define STRDUP( str )    STalloc( str )

# define ASCTOI( str, iptr ) \
    { \
        CVan( str, iptr ); \
    }

/*
** Name: REGPARAMS
**
** Description:
**      Registration/deregistration parameters as read from an input file.
**
**      owner       Owner of the document.
**      unit        Business unit the document belongs to.
**      location    Path of the document.
**      type        Document type.
**      flags       Document flags.
**      filename    Filename of the document.  This is decomposed into its
**                  component parts.
*/
typedef struct _tag_regparams
{
    II_CHAR*    owner;
    II_CHAR*    unit;
    II_CHAR*    location;
    II_INT      type;
    II_INT      flags;
    II_CHAR*    filename;
}REGPARAMS, *PREGPARAMS;

/*
** Name: readinpfile
**
** Description:
**      Function performs lexical anaylsis from the input file and returns
**      when a complete line of tokens has been parsed.  The values are
**      stored within the regparms structure.
**
** Inputs:
**      f               Input stream handle.
**      p               Pointer to regparams structure.
**
** Outputs:
**      None.
**
** Returns:
**      OK              Completed successfully.
**      FAIL            Lexer failed.
*/
II_EXTERN ICE_STATUS
readinpfile( FILE* f, PREGPARAMS p );
