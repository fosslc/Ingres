$!
$!
$!  DBCNF.COM	- Create JUPITER database core system catalogs (Version 1)
$!		  using an existing 5.0 INGRES system.
$!
$!  Description:
$!	  This script makes the Jupiter database core system catalog
$!	templates (Version 1).  The catalogs built include iirelation,
$!	iiattributes, iiindex, and iirel_idx.  These templates are used 
$!	by CREATEDB (Version 1) to build core templates (Version 2).
$!	The second version of these templates are actually released with
$!	the product.
$!
$!  Assumptions:
$!	  You are running in a 5.0 Ingres installation.
$!	NOTE, You should not be running in an installation that already
$!	has a database 'crdb' which is used for anything other that template
$!	creation.
$!
$!
$!  Create the first version of the core catalog templates.  In this version
$!  character fields are implemented using the 'c' datatype (5.0 does not
$!  support the 'char' datatype).
$!
$ destroydb crdb
$ createdb crdb
$ ingres -s crdb <dbcnf.qry >dbcnf.log
$!
$! Copy the newly created files to a directory.
$!
$ def/trans=concealed ii_system develop4:[jupiter.]
$ cre/dir  ii_system:[ingres.dbtmplt]
$! Copy the iirelation template.
$copy db_db_dev:[ingres.data.crdb]rel.ea -
    ii_system:[ingres.dbtmplt]aaaaaaab.t00
$! Copy the iirel_idx template.
$copy db_db_dev:[ingres.data.crdb]relx.ea -
    ii_system:[ingres.dbtmplt]aaaaaaac.t00
$! Copy the iiattributes template.
$copy db_db_dev:[ingres.data.crdb]att.ea  -
    ii_system:[ingres.dbtmplt]aaaaaaad.t00
$! Copy the iiindex template.
$copy db_db_dev:[ingres.data.crdb]idx.ea  -
    ii_system:[ingres.dbtmplt]aaaaaaae.t00
$ exit
