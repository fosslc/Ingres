/*
**
**  Copyright (c) 2004 Ingres Corporation
**  Name: util.h -- header for front!st!util!util.c
**
**  Description:
**	This file contains prototypes for functions declared in
**	front!st!util!util.c and definitions used throughout front!st.
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
**	26-jul-93 (tyler)
**		Added prototypes for lock_config_data() and
**		unlock_config_data().
**	27-Jan-1998 (jenjo02)
**	   Partitioned log file project:
**	   Modified arguments to write_transaction_log() prototype.
**	13-Aug-1998 (hanch04)
**	   Partitioned log file project:
**         Modified arguments to write_transaction_log() prototype.
**	24-Aug-2009 (kschendel) 121804
**	   Define more bool functions for gcc 4.3.
*/

# define II_PM_IDX		0
# define HOST_PM_IDX		1
# define OWNER_PM_IDX		2

# define BIG_ENOUGH		1024	

FUNC_EXTERN bool	ValueIsBool( char * );
FUNC_EXTERN bool	ValueIsBoolTrue( char * );
FUNC_EXTERN bool	ValueIsInteger( char * );
FUNC_EXTERN bool	ValueIsFloat( char * );
FUNC_EXTERN bool	ValueIsSignedInt( char * );
FUNC_EXTERN void	PMerror( STATUS, i4, ER_ARGUMENT * );
FUNC_EXTERN i4	write_transaction_log( bool, PM_CONTEXT *,
				char *logdir[], char *log_file[],
				void message( char * ),
				void init_graph( bool, i4 ),
				i4 , void update_graph( void ) );
FUNC_EXTERN STATUS	lock_config_data( char * );
FUNC_EXTERN void	unlock_config_data( void );
