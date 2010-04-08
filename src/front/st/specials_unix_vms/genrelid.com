$!------------------------------------------------------------------------
$!
$!  Copyright (c) 2004 Ingres Corporation 
$!
$!  Name: genrelid.com -- displays INGRES version string
$!
$!  Usage:
$!      @genrelid package output_file
$!
$!  Description:
$!      This script looks up the INGRES version number in the 
$!      Compatibility Library (gv.c) source and writes it to the
$!      output_file designated on the command line.
$!
$!      This script assumes the Version declaration has the following form:
$!
$!         globaldef {"iicl_gvar$readonly"}    readonly
$!         	char	Version[] = "x.x/yy (vax.vms/zz)";
$!
$!      An attempt is made to locate the 2 words "char" and "Version" 
$!      as the first two words of the line.  If this succeeds the string
$!      within the double quotes is extracted from the remaining portion of
$!      the same line.
$!
$!  History:
$!      05-aug-93 (joplin)
$!              Created.
$!
$!------------------------------------------------------------------------
$!
$ set noon
$!
$ on error then goto FAILED
$!
$ package        = p1                     ! Get the command line parameters
$ output_file    = p2
$!
$! Make sure a package and an output file were specified.
$!
$ if output_file.eqs."" .or. p3.nes."" then goto USAGE_FAIL
$!
$! Define local symbols.
$!
$ src_file       = "jpt_clf_src:[gv]gv.c" ! Name of the CL source file
$ ver_str        = ""                     ! INGRES version number string
$ key_wrds       = "char Version"         ! version string identifier key words
$!
$! The source file is opened for reading, and records are read until
$! the Version string is located.  Open up the output file for writing.
$!
$ open/read infile 'src_file'
$ open/write outfile 'output_file' 
$!
$ READ_LOOP:
$ read/end=READ_LOOP_END infile rec
$ if f$extract(0,f$length(key_wrds),f$edit(rec,"COMPRESS,TRIM")).nes.key_wrds -
  then goto READ_LOOP
$!
$! It looks as if we have located the version declaration.  Parse the
$! initializer string from the current line.
$!
$ sloc = f$locate("""",rec)                               ! Look for start quote
$ if sloc .ge. f$length(rec) then goto SRC_FORMAT_FAIL
$ estring = f$extract(sloc + 1, f$length(rec), rec) 
$ eloc = f$locate("""",estring)                           ! Look for end quote
$ if eloc .ge. f$length(estring) then goto SRC_FORMAT_FAIL
$ ver_str = f$extract(sloc + 1,eloc, rec)                 ! Extract the string
$!
$ READ_LOOP_END: 
$!
$! If the Version lookup failed, an error occured parsing the declaration.
$!
$ if ver_str .eqs."" then goto LOOKUP_FAIL
$ write outfile ver_str
$ close infile
$ close outfile
$ exit
$!
$ USAGE_FAIL:
$!
$ write sys$error "Usage: GENRELID package_name output_file_name"  
$ goto FAILED
$!
$ LOOKUP_FAIL:
$!
$ write sys$error "Version string declaration not found in " + src_file
$ goto FAILED
$!
$ SRC_FORMAT_FAIL:
$!
$ write sys$error "Error parsing Version string declaration in " + src_file
$ goto FAILED
$!
$ FAILED:
$!
$ if f$trnlnm("infile", "LNM$PROCESS_TABLE") .nes."" then close infile
$ if f$trnlnm("outfile", "LNM$PROCESS_TABLE") .nes."" then close outfile
$ exit -1
