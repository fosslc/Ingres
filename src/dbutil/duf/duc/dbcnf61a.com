$!
$!
$!  DBCNF61A.COM- Create 6.1a database core system catalogs
$!		  using an existing 6.1a INGRES installation.
$!
$!  Description:
$!	  This script makes the 6.0 database core system catalog
$!	templates.  The catalogs built include iirelation, iiattribute,
$!	ii_index, and ii_relidx.  These templates are used by createdb
$!	to build the core relations for a newly created database.  The
$!	6.0 DBMS can run on a database iff these templates exist.
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
$!
$!  Assumptions:
$!  1.	  You are running in a 6.1a or greater installation.  If you are
$!	  running with an earlier version of the DBMS server, use command
$!	  file DBCNF60.COM.  Also, the dbms server is up.
$!  2.    Assumes that you are running in the jupiter installation, where the
$!	  special symbols (jptcrdb, jptdsdb) are defined.  If you wish to run
$!	  in a non-jupiter installation, then you may comment out the jptxxxx
$!	  lines and remove the comment from the destroydb, createdb lines.
$!  3.    dbcnf61a.qry script is in the same directory that this script is
$!	  evoked from.
$!  4.	  The directory ii_system:[ingres.newtmplt] has already been created.
$!  5.    ii_system for newtmplt and dbtmplt directories resides on
$!	  develop15:[jup_rplus.].  If this changes for the installation, then
$!	  this command file script will need to be modified.
$!
$!  Side Effects:
$!	The database 'crdb' is destroyed (if exists) and recreated.  Then it is
$!	destroyed again to conserve on disk space.
$!
$!  History:
$!	11-Feb-1988 (ericj)
$!	    Updated to reference dbcnf60.qry which is a portable query
$!	    script.
$!	25-Feb-1988 (ericj)
$!	    Changed source file names from *.tab to *.t00.  This version
$!	    should be used with Ingres 6.0/06 and later releases.
$!	3-Dec-1988 (teg)
$!	    created dbcnf61a.com from dbcnf60.com and dbcnf61a.qry from
$!	    dbcnf60.qry.  The only notable changes are 1) relcmptlvl set to
$!	    "6.1a" instead of "6.0 ", 2) templates put in current template
$!	    directory, using the correct file name.
$!
$!
$ on error then continue
$! jptdsdb crdb
$ destroydb crdb
$ on error then stop
$! jptcrdb crdb
$ createdb crdb
$ ingres -s crdb <dbcnf61a.qry >dbcnf61a.log
$!
$!
$! Copy the newly created files to a directory.
$! NOTE, the actual mappings listed below could change over time.
$!
$ set proc/priv = sysprv
$ def/trans=concealed ii_system develop15:[jup_rplus.]
$!  Copy the iirelation template
$ copy ii_database:[ingres.data.crdb]aaaaaakg.t00 -
	    ii_system:[ingres.newtmplt]rel61a.tem
$!  Copy the iiattribute template
$ copy ii_database:[ingres.data.crdb]aaaaaakh.t00 -
	    ii_system:[ingres.newtmplt]att61a.tem
$!  Copy the iiindex template
$ copy ii_database:[ingres.data.crdb]aaaaaaki.t00 -
	    ii_system:[ingres.newtmplt]idx61a.tem
$!  Copy the iirel_idx template
$ copy ii_database:[ingres.data.crdb]aaaaaakj.t00 -
	    ii_system:[ingres.newtmplt]ridx61a.tem
$ purge ii_system:[ingres.dbtmplt]
$ copy ii_system:[ingres.newtmplt]rel61a.tem -
	    ii_system:[ingres.dbtmplt]aaaaaaab.t00
$ copy ii_system:[ingres.newtmplt]ridx61a.tem -
	    ii_system:[ingres.dbtmplt]aaaaaaac.t00
$ copy ii_system:[ingres.newtmplt]att61a.tem -
	    ii_system:[ingres.dbtmplt]aaaaaaad.t00
$ copy ii_system:[ingres.newtmplt]idx61a.tem -
	    ii_system:[ingres.dbtmplt]aaaaaaae.t00
$ purge ii_system:[ingres.dbtmplt]
$ deas/process ii_system
$ set proc/priv = nosysprv
$! jptdsdb crdb
$ destroydb crdb
$ exit
