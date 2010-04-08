$ if f$trnlnm("ii_c_compiler").nes."VAX11" .and. f$trnlnm("ii_c_compiler").nes."DECC" then goto no_c
$ @ii_system:[ingres.files]dcc 'P1' 'P2' 'P3' 'P4' 'P5' 'P6' 'P7' 'P8'
$ exit
$ no_c:
$ set noon
$ open/write out tt
$ write out ""
$ write out "The logical 'II_C_COMPILER' indicates you do NOT have a C compiler."
$ write out "A C compiler is required to compile this frame or procedure."
$ write out "Make sure you have a HP/Compaq C compiler installed on"
$ write out "this machine, then rebuild INGRES by running the II_SYSTEM:[INGRES]IIBUILD.COM"
$ write out "command script."
$ close out
$ set message/nofac/noid/nosev/notext
$ exit 2			! error status
