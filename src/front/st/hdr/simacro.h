/*
** Copyright (c) 2004 Ingres Corporation
**
**  Name: simacro.h -- useful macro wrappers SIprintf(), SIfprintf(),
**	SIflush(), and PCexit().
**
**  Description:
**	These macros are simpler to use than SIprintf() and friends.
**
**  History:
**	6-oct-92 (tyler)
**		Created.
**	10-nov-92 (tyler)
**		Added P_ERROR().
**	15-dec-92 (tyler)
**		Added F_ERROR().  Modified P_ERROR() to display program
**		name.
**	11-feb-93 (tyler)
**		Removed P_ERROR().	
**	22-jun-93 (tyler)
**		Added FPRINT() and F_FPRINT().
**	23-jul-93 (tyler)
**		Removed SI_MAX_TXT_REC definition and removed ERx()
**		from string argument references.
**	24-may-95 (emmag)
**		Microsoft already have an ERROR macro which causes conflct.
**	04-feb-2000 (somsa01)
**		MainWin header file wingdi.h on HP has ERROR defined as well.
*/

# if defined(NT_GENERIC) || (defined(MAINWIN) && defined(hp8_us5))
# ifdef ERROR
# undef ERROR
# endif
# endif

# define F_ERROR( F_MSG, ARG ) \
	{ \
		SIfprintf( stderr, ERx( "\n" ) ); \
		SIfprintf( stderr, F_MSG, ARG ); \
		SIfprintf( stderr, ERx( "\n\n" ) ); \
		SIflush( stderr ); \
		PCexit( FAIL ); \
	}

# define ERROR( MSG ) \
	{ \
		SIfprintf( stderr, ERx( "\n%s\n\n" ), MSG ); \
		SIflush( stderr ); \
		PCexit( FAIL ); \
	}

# define F_PRINT( F_MSG, ARG ) \
	{ \
		SIprintf( F_MSG, ARG ); \
		SIflush( stdout ); \
	}

# define F_FPRINT( FP, F_MSG, ARG ) \
	{ \
		SIfprintf( FP, F_MSG, ARG ); \
		SIflush( FP ); \
	}

# define PRINT( MSG ) \
	{ \
		SIprintf( MSG ); \
		SIflush( stdout ); \
	}

# define FPRINT( FP, MSG ) \
	{ \
		SIfprintf( FP, MSG ); \
		SIflush( FP ); \
	}

