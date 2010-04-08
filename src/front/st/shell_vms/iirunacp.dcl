$!------------------------------------------------------------------------
$!
$!  Copyright (c) 2004 Ingres Corporation
$!
$!  Name: iirunacp.com -- called by ingstart to launch the INGRES
$!	archiver process. 
$!
$!  Usage:
$!      @iirunacp uic ii_installation
$!
$!  Description:
$!	runs loginout.exe which launchs dmfacp.exe via iidmfacp.com
$!
$!! History:
$!!	16-may-1995 (wolf)
$!!	    Raise page_file quota to 15000.
$!!	28-may-1998 (kinte01)
$!!	    Modified the command files placed in ingres.utility so that
$!!	    they no longer include history comments.
$!!	14-dec-2004 (abbjo03)
$!!	    Replace history comments.
$!!	15-nov-2007 (joea)
$!!	    Increase page_file quota to 150000.
$!------------------------------------------------------------------------
$!
$ on error then exit $status
$ on warning then exit $status
$ run -
	/proc = dmfacp'p2' -
	/priv = (sysprv,exquota,oper) -
	/page = 150000 -
	/buffer_limit = 40000 -
	/file_limit = 40 -
	/ast_limit = 60 -
	/io_buffered = 20 -
	/io_direct = 20 -
	/extent = 2000 -
	/queue_limit = 30 -
	/input = ii_system:[ingres.utility]iidmfacp.com -
	/output = ii_system:[ingres.files]iirunacp.log -
	/prio = 4 -
	/uic = 'p1'-
	sys$system:loginout.exe
