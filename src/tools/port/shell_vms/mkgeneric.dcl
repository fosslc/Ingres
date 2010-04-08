$! Copyright (c) 2005 Ingres Corporation
$!
$!
$! construct generic.h file
$!
$!! History:
$!!	28-jan-2005 (abbjo03)
$!!		Created from mkgeneric.sh.
$!!	02-feb-2005 (abbjo03)
$!!	    Add define for TYPESIG.
$ create generic.h
/*
 * generic.h -- allows portable use of the following functions:
 *
 *	dup2(cur,new)		dups fd cur into fd new
 *	killpg(pg,sig)		sends sig to process group pg
 *	signal(s,f)		set function f to catch signal s -- return old f
 *	freesignal(s)		unblock signal s
 *	memcpy(b1,b2,n)		copy n bytes from b2 to b1 -- return b1
 *	memzero(b,n)		zero n bytes starting at b -- return b
 *	strchr(s,c)		return ptr to c in s
 *	strrchr(s,c)		return ptr to last c in s
 *	vfork()			data share version of fork()
 */

union				/* handles side effects in macros */
{
	char	*ustring;
} __GeN__;

$ @ING_TOOLS:[bin]readvers
$ vers=IIVERS_config
$ open /app out generic.h
$ write out "#define ''vers'"
$!
$! 	Define DOUBLEBYTE if this is a DBCS enabled port.
$!
$ if "''IIVERS_conf_DBL'" .nes. ""
$	then write out "# define DOUBLEBYTE"
$ endif
$!
$! 	Define INGRESII if this is Ingres II
$!
$ write out "# define INGRESII"
$!
$! Determine TYPESIG setting. TYPESIG is also associated with sigvec in the CL.
$! Some systems define *signal differently.
$!
$ write out "#define TYPESIG void"
$ close out
$ exit
