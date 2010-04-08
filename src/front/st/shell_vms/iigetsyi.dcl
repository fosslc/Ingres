$!------------------------------------------------------------------------
$!
$!  Copyright (c) 2004 Ingres Corporation
$!
$!  Name: iigetsyi.com -- returns f$getsyi() value to the configuration
$!	rules system.
$!
$!  Usage:
$!      @iigetsyi item out_file [ node ]
$!
$!  Description:
$!	Writes the value returned by f$getsyi( item [, node ] ) to the
$!	out_file.
$!
$!------------------------------------------------------------------------
$!
$ open /write outfile 'p2
$ local_node = f$getsyi( "NODENAME" )
$ if "''p3'" .nes. "" .and. "''local_node'" .nes. "''p3'"
$ then
$    write outfile f$getsyi( "''p1'", "''p3'" )
$ else
$    write outfile f$getsyi( "''p1'" )
$ endif
$ close outfile
