/*
** INGRES 6.1+ clsecret.h created on Wed May 10 03:55:19 EDT 1995
** by the mkntsecret.sh shell script
**
** History:
**	06-aug-96 (mcgem01)
**	    Remove authorization strings support for Jasmine on NT.
**	03-mar-1997 (canor01)
**	    Add xCL_094_TLS_EXISTS
**	02-aug-97 (mcgem01)
**	    Switch off runtime auth string checking on NT.
**	12-dec-97 (mcgem01)
**	    Reinstate runtime auth string checking on NT.
**	06-may-1998 (canor01)
**	    Add xCL_CA_LICENSE_CHECK for new licensing API.
**	13-aug-1998 (canor01)
**	    Swith off license checking for Rosetta.
**	18-sep-98 (mcgem01)
**	    License checking flags are being set in makewnt.ini
**	    for added flexibility.
**	18-jul-2002 (somsa01)
**	    Added xCL_020_SELECT_EXISTS.
**	23-Jun-2008 (bonro01)
**	    Removed obsolete xCL_067_GETCWD_EXISTS
*/

# define TYPESIG int
# define xCL_006_FCNTL_H_EXISTS
# define xCL_008_MKDIR_EXISTS
# define xCL_009_RMDIR_EXISTS
# define xCL_012_DUP2_EXISTS
# define xCL_014_FD_SET_TYPE_EXISTS
# define xCL_015_SYS_TIME_H_EXISTS
# define xCL_029_EXECVP_EXISTS
# define xCL_035_RENAME_EXISTS
# define xCL_070_LIMITS_H_EXISTS
# define xCL_087_SETPGRP_2_ARGS
# define xCL_094_TLS_EXISTS
# define xCL_MLOCK_EXISTS
# define xCL_020_SELECT_EXISTS
#define PRINTER_CMD "print"
/* End of clsecret.h */
/* # define xCL_NO_AUTH_STRING_EXISTS - specified on command line */
/* # define xCL_CA_LICENSE_CHECK - specified on command line */
