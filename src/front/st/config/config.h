/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
**
**  Name: config.h -- header for config.sc
**
**  Description:
**	This file contains prototypes for functions defined in
**	front!st!config!config.sc and should be included by files
**	which references these functions.
**
**  History:
**	5-oct-92 (tyler)
**		Created.
**	14-dec-92 (tyler)
**		Improved.
**	18-jan-93 (tyler)
**		Added this comment header, including past history.  Also
**		added argument declarations to functions prototypes.
**	16-jul-93 (tyler)
**		Added PMerror() prototype.
**	23-jul-93 (tyler)
**		Added prototype for write_transaction_log().
**	19-oct-93 (tyler)
**		Added definitions for PRIMARY_LOG_ENABLED and
**		SECONDARY_LOG_ENABLED.
*/

# define PRIMARY_LOG_ENABLED	0x01
# define SECONDARY_LOG_ENABLED	0x02

FUNC_EXTERN STATUS	browse_file( LOCATION *, char *, bool );
FUNC_EXTERN void	display_error( STATUS, i4, ER_ARGUMENT * );
FUNC_EXTERN bool	get_help( char * );
FUNC_EXTERN bool	reset_resource( char *, char **, bool );
FUNC_EXTERN bool	set_resource( char *, char * );

