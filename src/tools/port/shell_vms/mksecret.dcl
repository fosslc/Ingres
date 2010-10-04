$! Copyright (c) 2005, 2008 Ingres Corporation
$!
$! This script builds UNIX versions of `clsecret.h,' a file containing
$! all configuration dependent definitions that are visible only to
$! the CL.  It extracts information that used to be kept in the
$! globally visible bzarch.h.  The earlier bzarch.h was written by mkhdr.
$! The new one is written by mkbzarch.
$!
$! The "clsecret.h" file written here is included by "cl/cl/hdr/clconfig.h",
$! which contains documentation about what each of the defines here means.
$!
$! Definitions like "BSD" or "WECOV" or "SYSV" or "SYS5" are NOT ALLOWED.
$! You must define a specific capability and create a test for it.
$!
$! BE SURE TO UPDATE CLCONFIG.H IF ANYTHING IS ADDED TO THIS PROGRAM.
$!
$!! History:
$!!	28-jan-2005 (abbjo03)
$!!	    Created from mksecret.sh
$!!	26-nov-2008 (joea)
$!!	    Define xCL_USE_ETIMEDOUT and xCL_014_FD_SET_TYPE_EXISTS.
$!!	05-dec-2008 (joea)
$!!	    Add xCL_094_TLS_EXISTS.
$!!     05-aug-2010 (joea)
$!!         Add TYPESIG, xCL_068_USE_SIGACTION, and invocation of mathsigs.
$!!         Add comments from Unix mksecret.sh.
$!!
$ echo := write out
$ define /nolog tmp SYS$SCRATCH
$ header="clsecret.h"
$ date=f$time()
$!
$ create 'header'
$ open /app out 'header'
$!
$ product="Ingres"
$!
$! create a new target:
$!
$ echo "/*"
$ echo "** ''product' ''header' created on ''date'"
$ echo "** by the ''f$env("procedure")' shell script"
$ echo "*/"
$!
$ echo "#define xCL_USE_ETIMEDOUT"
$!
$! By default, use sigaction only if sigvec is not available
$! These defines are mutually exclusive
$! NOTE from HP VMS documentation:
$!     The sigvec and signal functions are provided for compatibility to old
$!     UNIX systems; their function is a subset of that available with the
$!     sigaction  function. 
$!
$ echo "#define xCL_068_USE_SIGACTION"
$!
$! File inspection derived
$!
$! On VMS, fd_set is typedef'd in time.h 
$ echo "#define xCL_014_FD_SET_TYPE_EXISTS"
$!
$! SYSV <unistd.h>
$!
$ echo "#define xCL_069_UNISTD_H_EXISTS"
$!
$! SYSV <limits.h>
$!
$ echo "#define xCL_070_LIMITS_H_EXISTS"
$!
$! SYSV <siginfo.h>
$!
$ echo "#define xCL_072_SIGINFO_H_EXISTS"
$!
$! handling math exceptions via signals
$ close out
$ define /user sys$output tmp:mathsigs.out
$ mathsigs
$ append tmp:mathsigs.out 'header' 
$ delete tmp:mathsigs.out;*
$ open /app out 'header'
$!
$! Determine TYPESIG setting. TYPESIG is also associated with sigvec in the CL.
$! Some systems define *signal differently.
$!
$ echo "#define TYPESIG void"
$!
$! xCL_094_TLS_EXISTS -- For servers built with operating-system threads,
$! private storage for each thread is allocated using thread-local
$! storage (TLS).  For System V threads, these are the thr_getspecific(),
$! etc. functions.  For POSIX threads, they are pthread_getspecific(),
$! etc.  If a server cannot be built using the system TLS functions
$! (usually because they depend on the system malloc()), replacement
$! functions from the CL will be used unless xCL_094_TLS_EXISTS is defined.
$!
$ echo "#define xCL_094_TLS_EXISTS"
$ echo ""
$ echo "/* End of ''header' */"
$done:
$ close out
$ exit
