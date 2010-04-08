$! Copyright (c) 2005, 2009  Ingres Corporation
$!
$! Name: bldenv
$!
$! Purpose: Setup environment for building Ingres.
$!
$! Requirements:
$!      The following variable must be set before running this script:
$!
$!      ING_ROOT -      Root directory of source code. Must point
$!                      to the directory created when the source
$!                      archive is unpacked or download.
$!
$!      The Ingres r3 build procedure is dependent on a number or third party
$!      libraries and headers. The following variables are used to point to
$!      their locations.
$!
$!      XERCESCROOT -   Location XERCES source code root directory
$!      XERCESLOC - Location of Apache Xerces C++ XML parser libraries.
$!      KRB5HDR - Location of Kerberos headers.
$!
$!!     History:
$!!        17-Mar-2005 (nansa02)
$!!               Created from bldenv.
$!!        29-apr-2005 (loera01)
$!!               Added II_ODBC_CLILIB and II_ODBCROLIB.
$!!	19-may-2005 (abbjo03)
$!!	    Add set proc/parse=ext.
$!!     17-oct-2004 (loera01) Bug 115408
$!!         Added include logical override for the gssapi subdirectory.
$!!         Changed search for Kerberos items to reference Kerberos 2.0.
$!!	18-apr-2007 (abbjo03)
$!!	    Allow ING_ROOT to be specified interactively using a concealed
$!!	    device.  Simplify the logic.
$!!     08-Jan-2008 (bolke01)  119936
$!!         Update build env to align with Unix to allow building directly
$!!         into an installation
$!!         Define JAM_SYMBOLS to include the common environment variables
$!!         that may be used to change the way JAM builds files
$!!         Define the CD search paths if the utility can be found.
$!!     2-Dec-2008 (stegr01)
$!!         ARCH_NAME for Itanium is IA64
$!!	15-jun-2009 (joea)
$!!	    Add define for BUILD_WITH_POSIX_THREADS for Alpha.
$!---------------------------------------------------------------
$!
$!
$ echo := write sys$output
$ if f$getjpi("", "parse_style_perm") .nes. "EXTENDED" then -
	set process /parse_style=extended
$! 
$!Xerces version
$ xvers = 25
$!
$!
$  config_string :==
$  unames = F$GETSYI("ARCH_NAME")
$  unameri = F$GETSYI("VERSION")
$  host = F$GETSYI("NODENAME")
$  user = F$USER()
$  if unames .EQS. "Alpha"
$   then config_string :== axm_vms
$	if f$trnlnm("BUILD_WITH_POSIX_THREADS") .eqs. ""
$	then
$	    define /job BUILD_WITH_POSIX_THREADS "N"
$	endif
$   else if unames .EQS. "IA64"
$      then config_string :== i64_vms
$      else if config_string .EQS. ""
$           then echo "This is an unrecognised platform."
$                echo "Please edit bldenv to add the apropriate platform information."
$           endif
$      endif
$ endif
$!
$!  Root of build/src tree. Must be set by hand
$!
$ ingroot_value = f$trnlnm("ING_ROOT")
$
$ while:
$    if ingroot_value .eqs. ""
$        then
$            READ SYS$COMMAND ing_roottmp /end_of_file=quit /PROMPT= - 
                 "Please specify disk and directory for ING_ROOT, e.g., disk1:[r904]: "
$        else
$            ing_roottmp = ingroot_value - ".]" + "]"
$    endif
$    tmp_path = f$parse(ing_roottmp, "no log:[no dir]", , , "no_conceal") - "][" - "].;" - ".000000"
$    if  tmp_path .eqs. ""
$        then
$            echo "The specified path is invalid"
$            ingroot_value = ""
$            goto while
$    endif
$    dsk = f$element(0, "[", tmp_path)
$    root = f$element(1, "[", tmp_path) + "."
$!
$! Ingres build area
$!     
$         define /nolog /trans=conc ING_ROOT 'dsk'['root']
$         define /nolog /trans=conc ING_SRC 'dsk'['root'src.]
$         define /nolog /trans=conc ING_BUILD 'dsk'['root'install.build.ingres.]
$         define /nolog /trans=conc II_SYSTEM 'dsk'['root'install.build.]
$         define /nolog II_MANIFEST_DIR ING_BUILD:[manifest]
$!
$! ingtest area
$!
$         define /nolog /trans=conc ING_TST   'dsk'['root'tst.] 
$         define /nolog /trans=conc ING_TOOLS 'dsk'['root'tools.]
$         define /nolog ING_TOOLSD 'dsk'['root'tools]
$! 
$!   Kerberos headers
$!
$     if (f$search("krb$root:[include]gssapi_generic.h") .eqs "") - 
       .AND. (f$search("krb$root:[include]gssapi.h").eqs."" )
       .AND. (f$search("sys$library:gss$rtl32.exe").eqs."" )
$        then
$        echo "Warning !! Files needed for Kerberos driver are missing"
$     endif
$
$! include override for gssapi/gssapi.h in gssapi_generic.h
$!
$ define /nolog gssapi KRB$ROOT:[INCLUDE]
$!
$!
$!   Xerces code and library
$!
$    xercesroot_value="''f$trnlnm("XERCESCROOT")'"
$!
$ while_xerces:
$    if "''xercesroot_value'" .eqs. ""
$        then
$          if f$parse("''dsk'[''root'xerces-c-src_2_5_0]") .NES. "" 
$            then 
$               define /nolog /trans=conc XERCESCROOT -
               'dsk'['root'xerces-c-src_2_5_0.]             
$               define /nolog XERCESLOC XERCESCROOT:[lib]
$          else 
$            READ SYS$COMMAND xercesroot_tmp /PROMPT= -
"Please specify disk and directory for XERCESCROOT, e.g., disk1:[dir] :"
$            dsk_xer = f$parse("''xercesroot_tmp'",,,"device")
$              if  F$GETDVI ("''dsk_xer'","EXISTS") .EQS."FALSE"
$                  then
$                     echo "Disk does not exist"
$                     GOTO while_xerces
$              endif
$           root_xer = f$parse("''xercesroot_tmp'",,,"directory")
$           root_xer = F$EXTRACT(1,(F$LOCATE("]","''root_xer'")-1),"''root_xer'")
$           root_dir = "''dsk_xer'[''root_xer'.xerces-c-src_2_5_0]"
$              if f$parse("''root_dir'") .EQS. ""
$                 then
$                     echo "''root_dir' does not exist"
$                     GOTO while_xerces
$              else
$                     define /nolog /trans=conc XERCESCROOT -
                      'dsk_xer'['root_xer'.xerces-c-src_2_5_0.]
$                     define /nolog XERCESLOC XERCESCROOT:[lib]
$              endif
$          endif 
$    else
$           xercesroot_value = F$EXTRACT(0,(F$LOCATE("]",- 
                   "''xercesroot_value'")-1),"''xercesroot_value'") 
$               if f$parse("''xercesroot_value']") .EQS. ""
$                     then    
$                        echo "XERCESCROOT is set to incorrect location"
$                        xercesroot_Value = ""               
$                        GOTO while_xerces 
$               else 
$!                        echo "XERCESCROOT is set to ''xercesroot_value']"
$               endif
$ 
$   endif
$!
$!   Jam variables
$!
$     jam_defined = "''f$type(jam)'"
$     if "''jam_defined'" .EQS. "STRING"
$        then
$!          echo "Jam is defined "
$     else
$        jam_input:
$         READ SYS$COMMAND jam /PROMPT= - 
"Please specify disk and directory where Jam executable is located, e.g., disk1:[jam] :"
$         if f$search("''jam'jam.exe") .EQS. ""
$           then
$            echo "Jam executable not found"
$            GOTO jam_input
$          else 
$            jam = "$''jam'jam"
$         endif
$     endif
$       jamrules="-sING_SRCRULES=ING_SRC:[tools.port.jam]Jamrules"
$       jam :== "''jam'" -sconfig_string="''config_string'" """''jamrules'"""
$!
$!
$       define /nolog DECC$TEXT_LIBRARY SYS$SHARE:SYS$LIB_C.TLB
$!
$!  Setting DCL$PATH here
$! 
$!  
$  i=0
$  dclpath_value=""
$  while:
$       comp = f$trnlnm("DCL$PATH",,i)
$       if comp .eqs. "" then goto end
$       dclpath_value="''dclpath_value', ''comp'"
$       i=i+1
$       goto while
$  end:
$      ptools_bin = ""
$      if f$trnlnm( "PTOOLSBIN" ) .nes. "" then ptools_bin = ",PTOOLSBIN"
$       define /nolog DCL$PATH ING_ROOT:[tools.bin], - 
       ING_BUILD:[bin],ING_BUILD:[utility]'ptools_bin''dclpath_Value'
$!
$!
$!--- To allow a build env to have an  installation
$!
$       define /nolog II_COMPATLIB II_SYSTEM:[ingres.library]clfelib.exe
$       define /nolog II_LIBQLIB II_SYSTEM:[ingres.library]libqfelib.exe
$       define /nolog II_FRAMELIB II_SYSTEM:[ingres.library]framefelib.exe
$       define /nolog II_INTERPLIB II_SYSTEM:[ingres.library]interpfelib.exe
$       define /nolog II_APILIB II_SYSTEM:[ingres.library]apifelib.exe
$       define /nolog II_KERBEROSLIB II_SYSTEM:[ingres.library]libgcskrb.exe
$       define /nolog II_ODBCLIB II_SYSTEM:[ingres.library]odbcfelib.exe
$       define /nolog II_ODBCROLIB II_SYSTEM:[ingres.library]odbcrofelib.exe
$       define /nolog II_ODBC_CLILIB II_SYSTEM:[ingres.library]odbcclifelib.exe
$       define /nolog II_USERADT II_SYSTEM:[ingres.library]iiuseradt.exe
$
$! Define Build Environment symbols
$!
$! Check to see if the JAM_SYMBOLS.COM file exists and was created after the JAMDEFS files.
$  mk_jam_sym = 0
$  jam_sym_file = "ING_SRC:[tools.port.jam]jam_symbols.com"
$  if f$search( "''jam_sym_file'") .eqs. ""
$  then
$     mk_jam_sym = 1
$  else
$     jam_sym_date  = f$cvtime(f$file_attribute( "''jam_sym_file'", "CDT"))
$
$     jam_def_date  = f$cvtime(f$file_attribute( "ING_SRC:[tools.port.jam]Jamdefs.", "CDT"))
$     jam_defa_date = f$cvtime(f$file_attribute( "ING_SRC:[tools.port.jam]Jamdefs.axm_vms", "CDT"))
$     bldenv_date   = f$cvtime(f$file_attribute( "ING_SRC:[tools.port.jam]bldenv.com", "CDT"))
$
$     if (jam_sym_date .lts. jam_def_date) .or. (jam_sym_date .lts. jam_defa_date) .or. -
         (jam_sym_date .lts. bldenv_date)
$     then
$        mk_jam_sym = 1
$     endif
$  endif
$
$  if mk_jam_sym .eq. 1
$  then
$     ! Need to rebuild the list of JAM symbols which can be imported from the environment
$     ! i.e Jam symbols assigned using "?=".
$
$     ! As different versions of AWK behave differently, find out if the version we're
$     ! about to use requires the SYS$PIPE as the input file.
$
$     set noon
$     pipe write sys$output "sys$pipe" | awk "{print $1}" sys$pipe 2>&1 | -
           search/nowarn/noout sys$pipe "sys$pipe"
$     res = $STATUS
$
$     if 'res .eq. %X10000001
$     then
$        awk_file = "sys$pipe"
$     else
$        awk_file = ""
$     endif
$
$     set on
$
$     ! First create a list of Environment Symbols used by JAM that have no default value
$
$     type/out=tmp.syms sys$input
CONFIG_STRING ?=
$
$     type/out=tmp.awk sys$input
BEGIN{ sym="JAM_SYMBOLS"}

{
   if (NR==1)
   {
      printf( "$ %s == \"%s", sym, $1);
   }
   else
   {
      if (NR%10==0)
      {
         printf( "\"\n$ %s == %s + \",%s", sym, sym, $1);
      }
      else
      {
         printf( ",%s", $1);
      }
   }
}

END{ printf( "\"")}
$
$     pipe search/nohead tmp.syms,ING_SRC:[tools.port.jam]Jamdefs.,Jamdefs.axm_vms "?=" | -
           awk "{print $1}" 'awk_file' | -
           sort/nodup sys$pipe sys$output | -
           awk -f tmp.awk 'awk_file' >'jam_sym_file'
$     delete tmp.awk;*,tmp.syms;*
$     purge 'jam_sym_file'
$  endif
$
$  @'jam_sym_file'
$
$  if f$search( "PTOOLSBIN:CD_SEARCH_PATHS.COM") .nes. "" then -
           @PTOOLSBIN:CD_SEARCH_PATHS.COM
$!
$       echo "Ingres build environment is now set"
$quit:
$ exit
