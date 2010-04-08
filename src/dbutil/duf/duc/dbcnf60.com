$!
$!
$!  DBCNF60.COM	- Create 6.0 database core system catalogs (Version 2)
$!		  using an existing 6.0 INGRES installation.
$!
$!  Description:
$!	  This script makes the 6.0 database core system catalog
$!	templates.  The catalogs built include iirelation, iiattribute,
$!	ii_index, and ii_relidx.  These templates are used by createdb
$!	to build the core relations for a newly created database.  The
$!	6.0 DBMS can run on a database iff these templates exist.
$!
$!  Assumptions:
$!	  You are running in a 6.0 installation.
$!
$!  Side Effects:
$!	The database 'crdb' is destroyed and recreated.
$!
$!  History:
$!	11-Feb-1988 (ericj)
$!	    Updated to reference dbcnf60.qry which is a portable query
$!	    script.
$!	25-Feb-1988 (ericj)
$!	    Changed source file names from *.tab to *.t00.  This version
$!	    should be used with Ingres 6.0/06 and later releases.
$!
$!
$ destroydb crdb
$ createdb crdb
$ ingres -s crdb <dbcnf60.qry >sys$login:dbcnf60.log
$!
$!
$! Copy the newly created files to a directory.
$! NOTE, the actual mappings listed below could change over time.
$!
$ def/trans=concealed ii_system develop2:[jupiter.]
$ cre/dir  ii_system:[ingres.newdbtmplt]
$!  Copy the iirelation template
$ copy ii_database:[ingres.data.crdb]aaaaaake.t00 -
	    ii_system:[ingres.newdbtmplt]aaaaaaab.t00
$!  Copy the iiattribute template
$ copy ii_database:[ingres.data.crdb]aaaaaakf.t00 -
	    ii_system:[ingres.newdbtmplt]aaaaaaad.t00
$!  Copy the iiindex template
$ copy ii_database:[ingres.data.crdb]aaaaaakg.t00 -
	    ii_system:[ingres.newdbtmplt]aaaaaaae.t00
$!  Copy the iirel_idx template
$ copy ii_database:[ingres.data.crdb]aaaaaakh.t00 -
	    ii_system:[ingres.newdbtmplt]aaaaaaac.t00
$ purge ii_system:[ingres.newdbtmplt]
$ exit
