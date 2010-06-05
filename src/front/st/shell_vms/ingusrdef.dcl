$! Copyright (c) 2004 Ingres Corporation
$!
$! INGUSRDEF.COM -- Define INGRES User Symbols.
$!
$! Define JOB logicals.
$!
$!! History:
$!!	28-may-1998	(kinte01)
$!!		Add replicator symbols
$!!	04-sep-1998	(kinte01)
$!!		Add missing replicator symbol for rpserver
$!!	11-feb-1998	(kinte01)
$!!		For SIR 78193 add a logical II_JOBDEF_OUTPUT_OFF that if
$!!		defined will not display the output from iijobdef.
$!!	04-apr-1999	(kinte01)
$!!		Update previous fix. The logic was reversed as the wrong
$!!		version of this file was submitted to Piccolo. Bug 96291
$!!	10-feb-2000	(kinte01)
$!!		add back in symbols for BASIC pre-compiler for AlphaVMS
$!!		(SIR 100394)
$!!	05-mar-2001 	(kinte01)
$!!		add in extenddb symbol
$!!	19-oct-2001 	(kinte01)
$!!		add in genxml, impxml, xmlimport, & usermod symbols
$!!		reorder symbols so they are alphabetical
$!!	20-nov-2002 (abbjo03)
$!!	    Remove codegen, iceinst, imagerep, rpcomp, rpmkserv. Add repcfg.
$!!	    Change rpgensql to a .com.
$!!	20-nov-2002 (abbjo03)
$!!	    Add esqlcc.
$!!	04-feb-2003 (abbjo03)
$!!	    Remove eqpl and esqlpl.
$!!	23-jan-2004 (abbjo03)
$!!	    Add blobstor.
$!!      28-jan-2004   (loera01)
$!!              Added iiodbcinst and iiodbcadmn.
$!!	14-dec-2004 (abbjo03)
$!!	    Replace history comments.
$!!	25-jan-2005 (abbjo03)
$!!	    Remove Vision tutorial.
$!!     12-oct-2007 (Ralph Loen) Bug 119296
$!!         Rename iiodbcadmn to iiodbcadmin.
$!!	26-Apr-2010 (hanje04)
$!!	    SIR 123608
$!!	    Make ii_system:[ingres.utility]ingres invoke SQL session instead of
$!!	    QUEL
$!!
$ iijobdef := $ii_system:[ingres.utility]iijobdef.exe
$ saved_message_format = f$environment( "MESSAGE" ) ! save message format
$ set message/notext/nofacility/noseverity/noidentification
$ define/job/super ii_config ii_system:[ingres.files]
$ set message 'saved_message_format'
$ set noon
$ if f$trnln("II_JOBDEF_OUTPUT_OFF") 
$ then
$       define/user sys$output nl:
$	iijobdef
$ else
$	iijobdef
$ endif
$!
$! Define user symbols.
$!
$ abf		:== "$ii_system:[ingres.bin]iiabf.exe abf"
$ abfdemo	:== @ii_system:[ingres.bin]abfdemo.com
$ abfimage	:== "$ii_system:[ingres.bin]iiabf.exe imageapp"
$ accessdb	:== "$ii_system:[ingres.bin]ingcntrl.exe -m -u$ingres"
$ arcclean      :== "$ii_system:[ingres.bin]arcclean.exe"
$ blobstor	:== "$ii_system:[ingres.bin]blobstor.exe"
$ catalogdb	:== "$ii_system:[ingres.bin]ingcntrl.exe"
$ compform	:== "$ii_system:[ingres.bin]compform.exe -m"
$ convrep       :== "$ii_system:[ingres.bin]convrep.exe"
$ copyapp	:== "$ii_system:[ingres.bin]copyapp.exe"
$ copydb	:== "$ii_system:[ingres.bin]copydb.exe"
$ copyform	:== "$ii_system:[ingres.bin]copyform.exe"
$ copyrep	:== "$ii_system:[ingres.bin]copyrep.exe"
$ dclgen	:== "$ii_system:[ingres.bin]dclgen.exe"
$ ddgenrul      :== "$ii_system:[ingres.bin]ddgenrul.exe"
$ ddgensup      :== "$ii_system:[ingres.bin]ddgensup.exe"
$ ddmovcfg      :== "$ii_system:[ingres.bin]ddmovcfg.exe"
$ deldemo	:== @ii_system:[ingres.bin]deldemo.com
$ delobj	:== "$ii_system:[ingres.bin]delobj.exe"
$ dereplic      :== "$ii_system:[ingres.bin]dereplic.exe"
$ extenddb	:== "$ii_system:[ingres.bin]extenddb.exe"
$ unextenddb:== "$ii_system:[ingres.bin]unextenddb.exe"
$ eqa		:== "$ii_system:[ingres.bin]eqa.exe"
$ eqc		:== "$ii_system:[ingres.bin]eqc.exe"
$ eqcbl		:== "$ii_system:[ingres.bin]eqcbl.exe"
$ eqf		:== "$ii_system:[ingres.bin]eqf.exe"
$ esqla		:== "$ii_system:[ingres.bin]esqla.exe"
$ esqlc		:== "$ii_system:[ingres.bin]esqlc.exe"
$ esqlcbl	:== "$ii_system:[ingres.bin]esqlcbl.exe"
$ esqlcc	:== "$ii_system:[ingres.bin]esqlc.exe -cplusplus -extension=cxx"
$ esqlf		:== "$ii_system:[ingres.bin]esqlf.exe"
$ eqb           :== "$ii_system:[ingres.bin]eqb.exe"
$ esqlb         :== "$ii_system:[ingres.bin]esqlb.exe"
$ eqp    	:== "$ii_system:[ingres.bin]eqp.exe"
$ esqlp         :== "$ii_system:[ingres.bin]esqlp.exe"
$ genxml 	:== "$ii_system:[ingres.bin]genxml.exe"
$ iiexport      :== "$ii_system:[ingres.bin]iiexport.exe"
$ iiimport      :== "$ii_system:[ingres.bin]iiimport.exe"
$ imageapp	:== "$ii_system:[ingres.bin]iiabf.exe imageapp"
$ impxml 	:== "$ii_system:[ingres.bin]impxml.exe"
$ ingmenu	:== "$ii_system:[ingres.bin]ingmenu.exe"
$ ingres	:== "$ii_system:[ingres.bin]tm.exe -qSQL"
$ iquel		:== "$ii_system:[ingres.bin]itm.exe"
$ isql		:== "$ii_system:[ingres.bin]itm.exe -sql"
$ modifyfe	:== "$ii_system:[ingres.bin]modifyfe.exe"
$ netutil	:== "$ii_system:[ingres.bin]netutil.exe"
$ iiodbcadmin	:== "$ii_system:[ingres.bin]iiodbcadmin.exe"
$ iiodbcinst	:== "$ii_system:[ingres.utility]iiodbcinst.exe"
$ printform	:== "$ii_system:[ingres.bin]printform.exe"
$ qbf		:== "$ii_system:[ingres.bin]qbf.exe"
$ quel		:== "$ii_system:[ingres.bin]tm.exe -qQUEL"
$ query		:== "$ii_system:[ingres.bin]qbf.exe -mall -lp"
$ report	:== "$ii_system:[ingres.bin]report.exe"
$ reconcil      :== "$ii_system:[ingres.bin]reconcil.exe"
$ repcat        :== "$ii_system:[ingres.bin]repcat.exe"
$ repdbcfg      :== "$ii_system:[ingres.bin]repdbcfg.exe"
$ repmgr        :== "$ii_system:[ingres.bin]repmgr.exe"
$ repmod        :== "$ii_system:[ingres.bin]repmod.exe"
$ rbf		:== "$ii_system:[ingres.bin]rbf.exe"
$ repcfg        :== "$ii_system:[ingres.bin]repcfg.exe"
$ rpgensql      :== "@ii_system:[ingres.bin]rpgensql.com"
$ rpserver      :== @ii_system:[ingres.bin]rpserver.com
$ sreport	:== "$ii_system:[ingres.bin]sreport.exe"
$ sql		:== "$ii_system:[ingres.bin]tm.exe -qSQL"
$ tables	:== "$ii_system:[ingres.bin]tables.exe"
$ unloaddb	:== "$ii_system:[ingres.bin]unloaddb.exe"
$ upgradefe	:== "$ii_system:[ingres.bin]upgradefe.exe"
$ usermod 	:== "$ii_system:[ingres.bin]usermod.exe"
$ vifred	:== "$ii_system:[ingres.bin]vifred.exe"
$ vision	:== "$ii_system:[ingres.bin]iiabf.exe vision"
$ xmlimport 	:== "$ii_system:[ingres.bin]xmlimport.exe"
$!
$ write sys$output ""
$ write sys$output "INGRES User Symbols Defined."
$ write sys$output ""
