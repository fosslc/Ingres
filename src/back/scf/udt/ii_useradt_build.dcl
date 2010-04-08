$ inst := 'f$trnlnm("II_INSTALLATION")
$ if inst .eqs. ""
$ then
$   inst_code := <production>
$ else
$   inst_code = inst
$ endif
$ Write sys$output "Building Sharable Image for User Defined Datatypes for ''inst_code' installation"
$ if f$getsyi("hw_model") .lt. 1024
$ then
$!
$!	VAX version
$!
$ macro/object=ii_system:[ingres.library]ii_useradt_xfer.obj -
	ii_system:[ingres.library]ii_useradt_xfer.mar
$ link/share=ii_system:[ingres.library]iiuseradt'inst'/sysshr -
	sys$input/opt /nodebug	'p1'
!
! This CLUSTER statement forces the transfer vector to the beginning of
! the shared image.  It should not be removed.
!
Cluster = TRANSFER_VECTOR,,,ii_system:[ingres.library]ii_useradt_xfer.obj
ii_system:[ingres.library]iiclsadt.obj
!
! Replace the object module below with the object modules 
! defining Installation Datatypes
!
ii_system:[ingres.library]iiuseradt.obj
!
! End of object modules defining datatypes.
!
NAME = IIUSERADT
!
! Note that the shared image id should not be changed.  INGRES expects this
! level.  The shared image ID can be changed ONLY by the product vendor.
!
IDENTIFICATION = "v2-000"
GSMATCH=LEQUAL, 2, 0
$ exit
$!
$ else
$!
$!	Alpha version
$!
$ link/share/notrace/nodebug/ -
exe=ii_system:[ingres.library]iiuseradt'inst' SYS$INPUT/OPTION
ii_system:[ingres.library]iiclsadt.obj -
,ii_system:[ingres.library]iiuseradt.obj 	
NAME = IIUSERADT 
IDENTIFICATION = "V2-000" 
GSMATCH=LEQUAL, 2, 0
SYMBOL_VECTOR = (iiudadt_register = PROCEDURE)
SYMBOL_VECTOR = (iiclsadt_register = PROCEDURE)
$ exit
$ endif
