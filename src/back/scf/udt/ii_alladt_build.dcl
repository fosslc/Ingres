$ inst := 'f$trnlnm("II_INSTALLATION")
$ if inst .eqs. ""
$ then
$   inst_code := <production>
$ else
$   inst_code = inst
$ endif
$ Write sys$output "Building Sharable Image for INGRES Spatial Objects and User Defined Datatypes for ''inst_code' installation"
$!
$ link/share/notrace/nodebug/ -
 exe=ii_system:[ingres.library]iiuseradt'inst' SYS$INPUT/OPTION
II_SYSTEM:[ingres.library]spat.olb/library, -
ii_system:[ingres.demo.udadts]op.obj,-
ii_system:[ingres.demo.udadts]common.obj,-
ii_system:[ingres.demo.udadts]cpx.obj,-
ii_system:[ingres.demo.udadts]intlist.obj,-
ii_system:[ingres.demo.udadts]zchar.obj,-
ii_system:[ingres.demo.udadts]numeric.obj,-
ii_system:[ingres.demo.udadts]nvarchar.obj,-
ii_system:[ingres.demo.udadts]iicvpk.obj,-
ii_system:[ingres.demo.udadts]iimhpk.obj
psect_attr = _HUGE, NOWRT
NAME = IIUSERADT
IDENTIFICATION = "V2-000"
GSMATCH=LEQUAL, 2, 0
SYMBOL_VECTOR = (iiudadt_register = PROCEDURE)
SYMBOL_VECTOR = (iiclsadt_register = PROCEDURE)
$ exit
