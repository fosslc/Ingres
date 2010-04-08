$goto skip
$! Copyright (c) 2004, 2009  Ingres Corporation
$!
$!
$! inglink.dcl - Links Ingres executables
$!
$! Supports the linking of Ingres executables based on a linker options
$! template (.lot) file.
$!
$! Parameters:
$!	p1	path to executable to be linked, e.g.
$!		ING_BUILD:[bin]ercompile.exe
$!	p2	path to main object, usually in relative form, e.g.,
$!		[.cl.clf.er_vms]ercomp.obj
$!
$! A search is made in the main object directory, e.g., ING_SRC:[cl.clf.er_vms],
$! for a .lot file with the name of the executable, e.g., ercompile.lot. If
$! no .lot file is found, a basic link is made. If a .lot file is found, it
$! it preprocessed into a .opt file using sed. The following substitutions are
$! made:
$!
$!	INGLIB:		replaced by the path to the main "library" directory,
$!			e.g., ING_BUILD:[lib]
$!	INGTOOLSLIB:	replaced by the path the tools library directory, e.g.,
$!			ING_TOOLS:[lib]
$!	MAINOBJ		replaced by the path to the main object, e.g.,
$!			ING_SRC:[cl.clf.er_vms]
$!
$! The substitutions do not include the suffixes (e.g., .obj, .olb) or any
$! applicable linker options (e.g., /library, /include=(gv)). A sample
$! linker options template file looks as follows:
$!
$!	INGLIB:frontmain.obj, -
$!	MAINOBJ.obj, -
$!	INGLIB:libcompat.olb/library
$!
$!!  History:
$!!	24-nov-2004 (abbjo03)
$!!	    Created.
$!!	16-dec-2004 (abbjo03)
$!!	    Create a linker map file when using linker options templates.
$!!	10-jan-2005 (abbjo03)
$!!	    Exit with an error if linker gives warnings in ALL cases.
$!!	08-feb-2005 (abbjo03)
$!!	    Link septool and setuser with /SYSEXE.
$!!	17-feb-2005 (abbjo03)
$!!	    Create symbol table files for bin and utility executables.
$!!     26-apr-2004 (loera01)
$!!          Broke sed command into three separate commands, because
$!!          long directory paths exceed the 256-character command limit.
$!!          This change should be removed when the build environment
$!!          is upgraded to VMS 7.3-2 or later.
$!!	22-jul-2005 (abbjo03)
$!!	     Unless LINKFLAGS is defined, set it to /NOTRACEBACK so that
$!!	     privileged images can be installed.
$!!	09-apr-2007 (abbjo03)
$!!	     Support ING_SRC defined in terms of a concealed logical.
$!!	06-nov-2007 (bolke01) Bug 119592
$!!	     Support use of inglink for more than ING_SRC 
$!!          Correct error from day 1 where the quotes on the sed string are
$!!          wrongly placed.
$!!          As all file names are now set to lowercase, change SEPTOOL and SETUSER 
$!!          to include SYSEXE ( to satisfy CTL$xxx )
$!!          Enabled inglink to build into an installation.
$!!          Changed the location for .MAP files from the option directory to 
$!!          the more central II_SYSTEM:[debug] directory together with the
$!!          .STB files
$!!          Add the creation of the DSF files for debug assistance.
$!!          Initialise the mapf, dsff and stb variables.
$!!          Copy map file to debug area and purge ING_BUILD:[debug] for current
$!!          executeable.
$!!     29-mar-2008 (bolke01) Bug 119592
$!!          Fails to build the sig executables due to no map file being created
$!!     02-may-2008 (horda03)
$!!          The .MAP file (if produced) should be in the same directory as the
$!!          main .OBJ file.
$!!     08-may-2008 (horda03)
$!!          The optpath is really the same as the objpath. Needed to change as not
$!!          all EXE's are built using a .LOT file.
$!!          Only create the .DSF file if requested by user; by creating the variable
$!!          dsf_required.
$!!     22-Dec-2008 (stegr01)
$!!          Itanium VMS port.
$!!          If II_THREAD_TYPE defined (on Alpha) then the compilation should build
$!!          OS threads support - the setting of this logical to INTERNAL (on Alpha)
$!!          indicates that Ingres threads are to be used.
$!!     12-mar-2009 (stegr01)
$!!          replace II_THREAD_TYPE with BUILD_WITH_INTERNAL_THREADS
$!!	11-jun-2009 (joea)
$!!	    Replace BUILD_WITH_INTERNAL_THREADS with BUILD_WITH_POSIX_THREADS.
$!!     14-jan-2010 (joea)
$!!         On IA64, the new default is to build with KPS.
$!!
$skip:
$ exe := 'p1
$ mainobj := 'p2
$ mapf = ""
$ mapf_copy = ""
$ dsff = ""
$ stb  = ""
$ te_flags = ""
$ exename = f$edit(f$parse(exe,,,"name"),"lowercase")
$ exepath = f$parse(exe,,,"device")+f$edit(f$parse(exe,,,"directory"),"lowercase")
$ objpath = F$ELEMENT(0, "]", f$parse(mainobj,,,,"no_conceal") - "][") + "]"
$ objname = f$parse(mainobj,,,"name")
$ opttempl = f$search("''objpath'''exename'.lot")
$ optpath = objpath
$ define/nolog optpath 'optpath'  
$
$! Is the application threaded ?
$! On Itanium we'll use KPS as the default, but allow OS/POSIX on request
$ if f$edit(f$getsyi("ARCH_NAME"),"upcase") .eqs. "IA64"
$ then
$     te_flags = ""
$     sysexe_flag = "/sysexe=selective"
$! On Alpha  the default remains internal threads, but allows KPS or POSIX
$ else ! ARCH_NAME .eqs. "ALPHA"
$     te_flags = ""
$     sysexe_flag = ""
$ endif
$! The logicals BUILD_WITH_KPS and BUILD_WITH_POSIX_THREADS may be either
$! undefined (== No) or defined as a DCL boolean True (a string starting with
$! tTyY or non-0 number)
$ if f$trnlnm("BUILD_WITH_KPS")
$ then
$     te_flags = ""
$     sysexe_flag = "/sysexe=selective"
$ endif
$ if f$trnlnm("BUILD_WITH_POSIX_THREADS")
$ then
$     te_flags = "/THREADS_ENABLE=(MULTIPLE_KERNEL_THREADS,UPCALLS)"
$     sysexe_flag = ""
$ endif
$! Always create the STB and MAP files for ING_BUILD based targets
$! Note:  Not all STB files are deliverables.
$ if exepath .eqs. "ING_BUILD:[bin]"      .or. - 
     exepath .eqs. "ING_BUILD:[utility]"  .or. - 
     exepath .eqs. "ING_BUILD:[sig]"      .or. - 
     exepath .eqs. "ING_BUILD:[library]"
$ then 
$	stb = "/SYMB=ING_BUILD:[debug]''exename'.stb"
$       map_file="optpath:''exename'.map"
$       mapf = "/MAP=''map_file'"
$       mapf_copy = "ING_BUILD:[debug]''exename'.map"
$       if f$type( dsf_required ) .nes. "" then -
              dsff = "/DSF=ING_BUILD:[debug]''exename'.dsf"
$ endif
$ if opttempl .eqs. ""
$ then	on warning then goto err_exit
$	link /exec='exe' 'mapf''stb''dsff''te_flags' 'mainobj'
$
$       if mapf .nes. "" then backup/new 'map_file'; ING_BUILD:[debug]'exename'.map
$
$       if f$type( BLD_PURGE ) .nes. ""
$       then
$          if stb .nes. "" then purge ING_BUILD:[debug]'exename'.stb
$          if mapf .nes. "" 
$          then 
$             purge 'map_file'
$             purge ING_BUILD:[debug]'exename'.map
$          endif
$          if dsff .nes. "" then purge ING_BUILD:[debug]'exename'.dsf
$          purge 'exe'
$       endif
$	exit
$ endif
$ define/nolog opttempl 'opttempl'
$ if f$type( LINKFLAGS_'exename' ) .NES. ""
$ then
$    linkflags = LINKFLAGS_'exename'
$ else
$    if "''LINKFLAGS'" .eqs. "" then LINKFLAGS = "/NOTRACE"
$    if exename .eqs. "septool" .or. exename .eqs. "setuser" then -
	LINKFLAGS = "/SYSEXE /NOTRACE"
$ endif
$ mainobj=objpath+objname
$ pipe sed -e "s/INGLIB:/ING_BUILD:[library]/g" -e "s/INGTOOLSLIB:/ING_TOOLS:[lib]/g" -
           -e "s/MAINOBJ/''mainobj'/" opttempl > optpath:'exename'.opt
$ on warning then goto err_exit
$ link 'LINKFLAGS' 'sysexe_flag' /exec='exe' 'stb' 'mapf' 'dsff' 'te_flags' -
	optpath:'exename'.opt/options
$ if mapf_copy .nes. ""
$ then
$     backup /nolog /new_version /ignor=inter 'map_file'; 'mapf_copy'
$ endif
$ delete /noconfirm /nolog optpath:'exename'.opt;*
$ set noon
$ if f$type( BLD_PURGE ) .nes. ""
$ then
$    if stb .nes. "" then purge ING_BUILD:[debug]'exename'.stb
$    if mapf .nes. ""
$    then 
$       purge 'map_file'
$       purge ING_BUILD:[debug]'exename'.map
$    endif
$    if dsff .nes. "" then purge ING_BUILD:[debug]'exename'.dsf
$    purge 'exe'
$ endif
$ deassign optpath
$ deassign opttempl
$ exit
$err_exit:
$ exit %x2c
