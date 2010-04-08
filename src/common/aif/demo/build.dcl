$ set noon
$!
$! compile the OpenAPI demo program
$!
$ CFLAGS := "/FLOAT=IEEE_FLOAT/NOWARNINGS"
$ cc'CFLAGS'/include=(ii_system:[ingres.files], -
	ii_system:[ingres.demo.api],sys$library) -
	apiademo.c, -
	apiaclnt.c, -
	apiasvr.c, -
	apiautil.c
$!
$! Build the application:
$!	
$ link	apiademo.obj, -
	apiaclnt.obj, -
	apiasvr.obj, -
	apiautil.obj, -
  ii_system:[ingres.files]iiapi.opt/opt
$!
$! purge the area
$!
$ purge/nolog ii_system:[ingres.demo.api]
$!
$ set on
$ exit
