$!
$!
$!  GETTMPLT.COM    -	Copy the 6.0 or 6.1a templates from the templates
$!			reserve directory to the directory that the dbms
$!			server uses the templates in.
$!
$!  Description:
$!
$!	The following history info is relevent:
$!
$!	    "6.0 "  - origional 6.x template.  Created from 5.x installation
$!		      by the command file  DBCNF.COM (which uses DBCNF.QRY).
$!		      Later, after 6.0 instalations existed, created by
$!		      DBCNF60.COM (which uses DBCNF60.QRY).
$!	    "6.0a"  - There was never a template for this level.  The level
$!		      was created because a new (better) hash algorythm was
$!		      implemented.
$!	    "6.1a"  - This is the current (FCS) version.  The 6.1a server is
$!		      distinguished from the 6.0a server because of a new
$!		      compression algorythm that allows more fields to be
$!		      compressed.
$!		      The new templates for 6.1a take advantage of the
$!		      compression algorythm from "6.0a" that the "6.0 " ones
$!		      did not.  This should enhance performance a little.
$!		      The "6.1a" templates are created from the command file
$!		      DBCNF61A.COM (which uses DBCNF61A.QRY).
$!
$!	The 6.0 templates were preserved when the 6.1a templates were created.
$!	Copies of both templates live under symbolic names of:
$!	    iirelation:	    rel---.tem
$!	    iiattribute:    att---.tem
$!	    iirel_idx:	    ridx---.tem
$!	    iiindex:	    idx---.tem
$!
$!	where --- is the version level (60 or 61a)
$!
$!	This command file copies the templates from the newtmplt directory
$!	to the directory where ingres actually uses them.   It gives the
$!	templates the names that ingres expects.  Also, it purges the directory
$!	because createdb cannot handle multiple versions of the files.
$!	    
$!  Assumptions:
$!	the installation is jupiter's rplus installation.  The dbtmplt
$!	directory is on develop15:[jup_rplus], and so is the dnewtmplt
$!	directory.  IF YOU ARE RUNNING ON A REGULAR INSTALLATION, JUST COMMENT
$!	OUT THE LINES THAT DEF/TRANS=CONSEALED II_SYSTEM, AND THE LINE THAT
$!	DEASSIGNS THIS II_SYSTEM DEFINITION.
$!
$!  Inputs:
$!	Version = 60	(gets origional 6.0 templates)
$!	Version = 61a	(gets new 6.1a templates)
$!
$!  History:
$!	3-Dec-1988 (teg)
$!	    Initial Creation.
$!
$!
$ if P1 .nes. "" then goto CONT
$ write sys$output " "
$ write sys$output " gettmplt.com - copies specified verision of templates"
$ write sys$output "		    from master to tablenames used by DBMS"
$ write sys$output " "
$ write sys$output "		@gettmplt <version>"
$ write sys$output " where "
$ write sys$output "		version = 60"
$ write sys$output "		or"
$ write sys$output "		version = 61a"
$ write sys$output " "
$ exit
$CONT:
$ set proc/priv = sysprv
$ def/trans=concealed ii_system develop15:[jup_rplus.]
$ copy rel'P1'.tem ii_system:[ingres.dbtmplt]aaaaaaab.t00
$ copy ridx'P1'.tem ii_system:[ingres.dbtmplt]aaaaaaac.t00
$ copy att'P1'.tem ii_system:[ingres.dbtmplt]aaaaaaad.t00
$ copy idx'P1'.tem ii_system:[ingres.dbtmplt]aaaaaaae.t00
$ purge ii_system:[ingres.dbtmplt]aaaaaa*.*
$ deas/process ii_system
$ set proc/priv = nosysprv
$exit
