$! recompile all UDT source
$!##   	16-may-1995 (wolf) 
$!##   	    Oddly enough, op.obj is put in [ingres.demo] by abf.ccpp, so
$!##	    that's where a newly-compiled object file should go.
$!##	25-sep-95 (dougb)
$!##	    Now compiling with /member_alignment on Alpha.
$!##	21-dec-95 (dougb)
$!##	    Undo previous change and prevent any affect on user's environment.
$!##    19-sep-2000 (horda03)
$!##        AXM_VMS build is with /MEMBER_ALIGNMENT
$!##	19-oct-2001 (kinte01)
$!##	    add nvarchar
$!##	10-jan-2003 (abbjo03)
$!##	    Move op.obj to udadts subdirectory.
$!
$ if f$getsyi("hw_model") .lt. 1024
$ then
$!
$!  VAX version
$!
$ if f$trnlnm("ii_c_compiler") .eqs. "VAX11" 
$ then
$	CFLAGS := ""
$ else 
$	CFLAGS := "/DECC/STANDARD=VAXC"
$ endif
$ else
$!
$!  Alpha version
$!
$ CFLAGS := "/STANDARD=VAXC/FLOAT=IEEE_FLOAT"
$ endif 
$
$ set verify
$ cc'CFLAGS'/include=(ii_system:[ingres.files], sys$library) zchar
$ cc'CFLAGS'/include=(ii_system:[ingres.files], sys$library) cpx
$ cc'CFLAGS'/include=(ii_system:[ingres.files], sys$library) intlist
$ cc'CFLAGS'/include=(ii_system:[ingres.files], sys$library) op
$ cc'CFLAGS'/include=(ii_system:[ingres.files], sys$library) common
$ cc'CFLAGS'/include=(ii_system:[ingres.files], sys$library) numeric
$ cc'CFLAGS'/include=(ii_system:[ingres.files], sys$library) nvarchar
$ set noverify
