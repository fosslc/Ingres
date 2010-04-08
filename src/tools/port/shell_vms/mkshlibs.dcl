$! Copyright (c) 2005 Ingres Corporation

$!
$! Name:	
$!	mkshlibs.com
$!
$! Usage:
$!	     mkshlibs [compat] [libq] [frame] [interp] [api] 
$!		      [kerberos] [odbc] [odbcro] [odbccli] [iiuseradt] 
$!
$! Description:
$!
$!	Creates the shared libraries for the front ends. Creation of libq needs
$!	compat, frame needs libq and compat, interp needs frame, libq and 
$!	compat; and api, kerberos need compat. 
$!
$!	Before running this script, you need to set up various variables such
$!	as shlink_cmd and shlink_opts, krblink_cmd and krblink_opts
$!	in shlibinfo.sh.
$!
$! DEST = utility
$!
$! VMS Notes:
$!      This does not yet support the creation of multiple shared libraries
$!      in one go as the Unix mkshlibs does.
$!
$!
$!! History
$!!	04-jan-2005 (abbjo03)
$!!	    Created.
$!!	23-feb-2005 (abbjo03)
$!!	    Add ODBC and UDT shared libraries. Create symbol table files in
$!!	    ING_BUILD:[debug].
$!!	26-apr-2005 (abbjo03)
$!!	    Change location for Kerberos library.
$!!     29-apr-2005 (loera01)
$!!         Added ODBC CLI and read-only ODBC driver.
$!!	16-may-2005 (abbjo03)
$!!	    Add LINKFLAGS to link command. Reset working directory.

$!!     08-Jan-2008 (bolke01) Bug 117648
$!!         Enabled use of mkshlibs from any directory
$!!     18-mar-2008 (bolke01)
$!!         The Shared libraries are now built into the "library" directory.
$!!         Also the built files will be purged if BLD_PURGE defined.
$!!     14-jan-2010 (joea)
$!!         Use /sysexe=selective on IA64 or if requested via the logical
$!!         BUILD_WITH_KPS.
$!!
$ shlib = p1
$ if shlib .eqs. "COMPAT"
$ then	dir = "cl.clf.specials_vms"
$	libname = "clfelib"
$	exec = "II_COMPATLIB"
$ else if shlib .eqs. "LIBQ"
$ then	dir = "front.embed.specials_vms"
$	libname = "libqfelib"
$	exec = "II_LIBQLIB"
$ else if shlib .eqs. "FRAME"
$ then	dir = "front.frame.specials_vms"
$	libname = "framefelib"
$	exec = "II_FRAMELIB"
$ else if shlib .eqs. "INTERP"
$ then	dir = "front.abf.specials_vms"
$	libname = "interpfelib"
$	exec = "II_INTERPLIB"
$ else if shlib .eqs. "API"
$ then	dir = "common.aif.specials_vms"
$	libname = "apifelib"
$	exec = "II_APILIB"
$ else if shlib .eqs. "KERBEROS"
$ then	dir = "common.gcf.drivers.krb5"
$	libname = "libgcskrb"
$	exec = "II_KERBEROSLIB"
$ else if shlib .eqs. "ODBC"
$ then	dir = "common.odbc.driver"
$	libname = "odbcfelib"
$	exec = "II_ODBCLIB"
$ else if shlib .eqs. "ODBCRO"
$ then	dir = "common.odbc.driver"
$	libname = "odbcrofelib"
$	exec = "II_ODBCROLIB"
$ else if shlib .eqs. "ODBCCLI"
$ then	dir = "common.odbc.manager"
$	libname = "odbcclifelib"
$	exec = "II_ODBC_CLILIB"
$ else if shlib .eqs. "IIUSERADT"
$ then	dir = "back.scf.udt"
$	libname = "iiuseradt"
$	exec = "II_USERADT"
$ endif
$ endif
$ endif
$ endif
$ endif
$ endif
$ endif
$ endif
$ endif
$ endif
$ orig_wd = f$env("default")                 
$ set def ING_SRC:['dir']
$ pipe sed -e "s/INGLIB:/ING_BUILD:[library]/g" 'libname'.lot > 'libname'.opt
$ arch_name   = f$edit(f$getsyi("ARCH_NAME"),"upcase")
$ kps = f$trnlnm("BUILD_WITH_KPS")
$ sysexe_flag = ""
$ if kps .or. arch_name .eqs. "IA64"
$ then
$    sysexe_flag = "/sysexe=selective"
$ else
$    sysexe_flag = ""
$ endif
$ on warning then goto err_exit
$ link 'LINKFLAGS' 'sysexe_flag' /share/section_binding/replace/notraceback -
	/map='libname'.map/full /symb=ING_BUILD:[debug]'libname'.stb -
	/exec='exec' 'libname'.opt/options
$
$ backup/new 'libname'.map; ING_BUILD:[debug]'libname'.map
$ if f$type( BLD_PURGE ) .nes. ""
$ then
$    purge 'libname'.map
$    purge ING_BUILD:[debug]'libname'.stb
$    purge ING_BUILD:[debug]'libname'.map
$    purge 'exec'
$ endif
$ del 'libname'.opt;*
$ set def 'orig_wd'
$ exit
$err_exit:
$ set def 'orig_wd'
$ exit %x2c
