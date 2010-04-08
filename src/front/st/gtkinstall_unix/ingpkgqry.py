#!/usr/bin/python
#
# Copyright Ingres Corporation 2009. All Rights Reserved
#

# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

# Name: ingpkgqry
#
# Description:
#       Utility for querying package management system (eg RPM) for
#       existing Ingres instances and writing out the information
#	as XML on a per instance basis.
#	The packages contained in the saveset are also queried.
#
#	Currently only supported for RPM packages but contains code
#	for querying DEBs (Debian package manager) too.
#
# Usage:
#    ingpkgqry -s <pkgdir> [-o <filename>][-p]
#	-s		Search <pkgdir> for Ingres packages to compare against
#			those installed
#	-o		Write output to <filename>
#			(default is to write to ./ingpkginfo.xml)
#	-p		Pretty print to stdout
#			(ingpkginfo.xml will still be written)
#
# History:
#	06-Aug-2009 (hanje04)
#	   BUG 122571
#	   Created from 2007 prototype


# load external modules
import sys
import os
import re
import getopt

# define globals
Debug = False
pprint = False
pkg_basename = 'ingres'
filesloc = 'ingres/files'
symtbl='symbol.tbl'
cfgdat='config.dat'
outfile='ingpkginfo.xml'
sslocation=None
instlist = []
ssinfo = { 'basename' : [ pkg_basename ], 'version' : '', 'arch' : '', \
	'format' : '', 'location' : '', 'package' : [] } 

# WARNING! Correct generation of XML is dependent on the order of ingpkgs
# entires matching EXACTLY those defined be PKGIDX in front!st!hdr gip.h
# DO NOT ALTER UNLESS PKGIDX unless has also been changed
ingpkgs = ( 'core', 'sup32', 'abf', 'bridge', 'c2', 'das', 'dbms', \
	'esql', 'i18n', 'ice', 'jdbc', 'net', 'odbc', 'ome', 'qr_run', 'rep', \
	'star', 'tux', 'vision', 'documentation' )
ingpkgsdict = dict( zip( ingpkgs, range( 0, len( ingpkgs ) ) ) )

# WARNING! Correct generation of XML is dependent on the order of dblocnames
# entires matching EXACTLY those defined be location_index in front!st!hdr gip.h
# DO NOT ALTER UNLESS location_index unless has also been changed
dblocnames = ( 'II_DATABASE', 'II_CHECKPOINT', 'II_JOURNAL', \
	'II_WORK', 'II_DUMP' ) 
dblocdict = dict( zip( dblocnames, range( 1, len( dblocnames ) ) ) )

# search masks
rnm = '[A-Z]([A-Z]|[0-9])' # rename mask
sm = ( 'ca-ingres$', 'ca-ingres-%s$' % rnm, 'ingres2006$', \
	'ingres2006-%s$' % rnm, 'ingres$', 'ingres-%s$' % rnm )

# error messages
FAIL_IMPORT_RPM = "ERROR: Failed to load Python module \"rpm\" \n\
The 'rpm-python' package must be installed to run this installer"
FAIL_IMPORT_APT = "ERROR: Failed to load Python module \"apt\" \n\
The 'python-apt' package must be installed to run this installer"
FAIL_IMPORT_XML = "ERROR: Failed to load Python module \"xml\" \n\
The 'python-xml' package must be installed to run this installer"
FAIL_OPEN_FILE = "ERROR: Failed to open file: %s"
FAIL_GET_DBLOCS = "ERROR: Failed to determine DB locations"
FAIL_GET_LOGLOCS = "ERROR: Failed to determine log locations"
FAIL_WRITE_XML="ERROR: Failed to write XML to: %s\n%s\n"
FAIL_SSET_QRY="ERROR: Failed to query %s saveset"
INVALID_PKG_NAME = "ERROR: %(name)s-%(version)s-%(release)s-%(arch)s.rpm is\n \
not a valid package.\nAborting"

def usage():
    print "Usage:\n    %s -s <pkgdir> [-o <filename>][-p]" % sys.argv[0].split('/')[-1]
    print "\t-s\t\tSearch <pkgdir> for Ingres packages to compare against\n\t\t\tthose installed"
    print "\t-o\t\tWrite output to <filename>\n\t\t\t(default is to write to ./ingpkginfo.xml)"
    print "\t-p\t\tPretty print to stdout\n\t\t\t(ingpkginfo.xml will still be written)"
    sys.exit(-1)

def debugPrint( message ):
    if Debug:
        print message

    return

def writeXML( filename, xmldoc ):
    """Write XML Document to file"""
    try:
        outf = file( filename, 'w' )
        outf.truncate()
        outf.writelines(xmldoc.toxml())
    except Exception, e:
	print FAIL_WRITE_XML % (outfile, str(e))
	return 1

    outf.close()
    return 0

def genXMLTagsFromDict( xmldoc, ele, dict ):
    """Creates Element-Textnode pairs from dictionary keys and data"""
    for tag in dict.keys():
	for ent in dict[ tag ]:
	    newele = xmldoc.createElement( tag ) # create new element
	    if tag == "package": # need to add attribute special cases
		newele.setAttribute( "idx", "%d" % ingpkgsdict[ ent ] ) 
	    elif re.match( "II_", tag ) != None:
		newele.setAttribute( "idx", "%d" % dblocdict[ tag ] ) 

	    ele.appendChild( newele ) # attach it to current element
	    tn = xmldoc.createTextNode( ent ) # create text node
	    newele.appendChild( tn ) # attch it to the new element

    return
	    
def genXML( instlist, ssinfo ):
    """Write Ingres instance data out in XML."""
    # Create document for instance data and base element
    instdoc = Document()
    pkginfo = instdoc.createElement( "IngresPackageInfo" )
    instdoc.appendChild( pkginfo )

    # Saveset element
    saveset = instdoc.createElement("saveset")
    pkginfo.appendChild( saveset )

    # Instances element
    instances = instdoc.createElement("instances")
    pkginfo.appendChild( instances )

    # write out saveset info first
    genXMLTagsFromDict( instdoc, saveset, ssinfo )

    # loop through the list of instances and add to DOM
    for inst in instlist:
	# Create instance
	instance = instdoc.createElement("instance")
	instances.appendChild( instance )

	# now iterate over all the keys using the keys as XML 
	genXMLTagsFromDict( instdoc, instance, inst )
	    
    if pprint:
	print instdoc.toprettyxml(indent="  ")

    return writeXML( outfile, instdoc )
   
  

def isRenamed( pkgname ):
    """Has package been renamed"""
    # Has pkg been renamed
    if re.search( rnm, pkgname ) != None:
	debugPrint( "%s is renamed" % pkgname )
	renamed = True
    else:
	renamed = False

    return renamed

def findAuxLoc( instloc ):
    """Get database and log locations for an Ingres instance at
	a given location"""
    symtblpath = "%s/%s/%s" % ( instloc, filesloc, symtbl )
    cfgdatpath = "%s/%s/%s" % ( instloc, filesloc, cfgdat )
    logparts = "ii\..*\.rcp.log.log_file_parts"
    primlog = "ii\..*\.rcp.log.log_file_[1-9]+:"
    duallog = "ii\..*\.rcp.log.dual_log_[1-9]+:"
    AuxLocs = { 'dblocs' : {}, 'loglocs' : {} }
    dblocs = AuxLocs[ 'dblocs' ]
    loglocs = AuxLocs[ 'loglocs' ]
    loglocs[ 'primarylog' ] = [] 
    loglocs[ 'duallog' ] = [] 

    # first look at the symbol table
    try:
	cfgfile = file( symtblpath, 'r' )
    except:
	print FAIL_OPEN_FILE % symtblpath
	print FAIL_GET_DBLOCS
    else:
	# start reading the file
	ent = cfgfile.readline()
	while ent != '':
	    # match each line against each location
	    for loc in dblocnames:
		if re.match( loc, ent ) != None:
		    # when we find a match, check it against II_SYSTEM
		    tmplst = re.split( '/', ent, maxsplit = 1 )
		    path = [ '/' + tmplst[1].strip() ]
		    pat = instloc + '$'
		    if re.match( pat, path[0] ) == None:
			# if it differs store it
			dblocs[ loc ] = path 
	
	    # and move on
	    ent = cfgfile.readline()

    # now config.dat
    try:
	cfgfile = file( cfgdatpath, 'r' )
    except:
	print FAIL_OPEN_FILE % cfgdatpath
	print FAIL_GET_LOGLOCS
    else:
	# start reading the file
	ent = cfgfile.readline()
	while ent != '':
	    # check for primary log
	    if re.match( primlog, ent ) != None:
		tmplst = re.split( ':', ent, maxsplit = 1 )
		path = [ tmplst[1].strip() ]
		pat = instloc + '$'
		if re.match( pat, path[0] ) == None:
		    loglocs[ 'primarylog' ].append( path[0] )
	    # then dual
	    elif re.match( duallog, ent ) != None:
		tmplst = re.split( ':', ent, maxsplit = 1 )
		path = [ tmplst[1].strip() ]
		pat = instloc + '$'
		if re.match( pat, path[0] ) == None:
		    loglocs[ 'duallog' ].append( path[0] )

	    ent = cfgfile.readline()

    
    return AuxLocs

def getRPMSaveSetInfo( location ):
    """Load RPM header info form saveset and store in ssinfo"""

    # Get a list of available packages
    rpmloc = location + '/rpm/'
    filelist = os.listdir( rpmloc )

    # check first file is valid, then use it to populate ssinfo
    ssts = rpm.ts()
    fdno = os.open( rpmloc + filelist[0], os.O_RDONLY )
    hdr = ssts.hdrFromFdno( fdno )
    if re.match( pkg_basename, hdr[ 'name' ] ) == None:
	print INVALID_PKG_NAME % hdr
	return None

    ssinfo[ 'version' ] = [ "%(version)s-%(release)s" % hdr  ]
    ssinfo[ 'arch' ] = [ "%(arch)s" % hdr ]
    ssinfo[ 'format' ] = [ "rpm" ]
    ssinfo[ 'location' ] = [ location ]

 
    # Done with file
    os.close( fdno )

    # now scroll through the entries, adding the valid packages
    for f in filelist:
	for pkg in ingpkgs:
	    if re.search( pkg, f ) != None:
		ssinfo[ 'package' ].append( pkg )

    return hdr
    
def searchRPMDB( sshdr ):
    """Search RPM database for Ingres packages"""
    # RPM db connection
    ts = rpm.TransactionSet() # handler for querying RPM database
    ss_ds = sshdr.dsOfHeader() # dependency set for saveset

    # now search results set using search masks
    for pat in sm :
	pkgs = ts.dbMatch() # RPM package list
	debugPrint( "Searching with %s" % pat )
	# Search list using mask
	pkgs.pattern( 'name', rpm.RPMMIRE_DEFAULT, pat )

	for inst in pkgs :
	    debugPrint( "Found %(name)s-%(version)s-%(release)s" % inst )
	    inst_ds = inst.dsOfHeader() # dependency set instance
	    pkgname = inst['name']
	    
	    # we have at least one package for this instance
	    # all need to be lists so we don't itterate over the characters
	    instpkgs = [ 'core' ]
	    instid = [ 'II' ] 
	    basename = [ inst['name'] ]
	    version = [ "%(version)s-%(release)s" % inst ]

	    # Has pkg been renamed
	    if isRenamed( pkgname ) == True:
		# pkg renamed, strip out inst ID
		tmplist = re.split( "-", inst['name'], 4 )
		instid[0] = tmplist[len(tmplist) - 1] 
		debugPrint( "%s is renamed, ID is %s" % ( inst['name'], instid[0] ) )
		renamed = True
	    else:
		instid[0]= "II"
		renamed = False

	    # What's version is the instance and what can we do to it
  	    debugPrint( 'Saveset version: %s\nInstalled version: %s' %  \
		( ss_ds.EVR(), inst_ds.EVR() ) )
	    res=rpm.versionCompare(sshdr, inst)
	    if res > 0:
		action = [ 'upgrade' ] # older so upgrade
	    elif res == 0:
	 	action = [ 'modify' ] # same version so we can only modify
	    else:
		action = [ 'ignore' ] # newer so just ignore it
  	    debugPrint( 'Install action should be: %s' % str(action) )

	    # Find where it's installed
	    instloc = inst[rpm.RPMTAG_INSTPREFIXES]
	    debugPrint( "installed under: %s" % instloc )
	    debugPrint( "Searching for other packages" )
	    allpkgs = ts.dbMatch()
	    for p in allpkgs :
		for ingpkg in ingpkgs:
		    if renamed == True :
			instpat = "%s-%s-%s" % \
				( ingpkg, instid[0], inst['version'] )
			string = "%(name)s-%(version)s" % p
		    else:
			instpat = "%s-%s-%s" % \
				( inst['name'], ingpkg, inst['version'] )
			string = "%(name)s-%(version)s" % p

		    if re.search( instpat, string, 0 ) != None:
			debugPrint( "\tFound %(name)s-%(version)s-%(release)s" % p )
			instpkgs.append( ingpkg )

	    # create dictionary to store info
	    instinfo = { 'ID' : instid,
	    		'basename' : basename,
			'version' : version,
	    		'package' : instpkgs,
	    		'location' : instloc,
			'action' : action,
			'renamed' : [str(renamed)] }

	    # Check for database and log locations and add them
	    auxlocs = findAuxLoc( instloc[0] )
	    for loctype in auxlocs.keys():
		locs = auxlocs[ loctype ]
		for loc in locs.keys():
		    path = ( locs[loc] )
		    instinfo[ loc ] = path

	    # finally add it to the list
	    instlist.append( instinfo ) 

def searchDEBDB():
    """Search DEB database for Ingres packages"""
    # Load package cache from database
    cache = apt.Cache()

    # scroll through cache looking for installed Ingres packages
    for pat in sm:
	debugPrint( "Searching with %s" % pat )
	for pkg in cache:
	    pkgname = pkg.name
	    if re.search( pat, pkgname, 0 ) != None and pkg.isInstalled :
		debugPrint( "Found %s" % pkgname )
		# we have at least one package for this instance
		instpkgs = [ 'core' ]

		# Has pkg been renamed
		if isRenamed( pkgname ) == True:
		    # pkg renamed, strip out inst ID 
		    tmplist = re.split( "-", pkgname, 4 )
		    instid = tmplist[len(tmplist) - 1]
		    debugPrint( "%s is renamed, ID is %s" % ( inst['name'], instid ) )
		    renamed = True
		else:
		    instid="II"
		    renamed = False

		debugPrint( "Searching for other packages" )
		for p in cache :
		    pname = p.name
		    for ingpkg in ingpkgs:
			if renamed == True :
			    instpat = "%s-%s$" % \
				( ingpkg, instid )
			    string = pname
			else:
			    instpat = "%s-%s$" % \
				( pkgname, ingpkg )
			    string = pname

			if re.search( instpat, string, 0 ) != None:
			    debugPrint( "\tFound %s" % pname )
			    instpkgs.append( ingpkg )

		# create dictionary to store info and add it to the list
		instinfo = { 'instID' : instid, 'pkgs' : instpkgs }; 
		instlist.append( instinfo )

# Main body of script
if len(sys.argv) < 3:
    usage()

try:
    opts, args = getopt.getopt(sys.argv[1:], "s:o:pd")
except getopt.GetoptError, e:
    print str(e)
    usage()

for o, a in opts:
    if o in ("-s"):
	sslocation=a
    elif o in ("-o"):
	outfile=a
    elif o in ("-p"):
	pprint=True
    elif o in ("-d"):
	Debug=True
	pprint=True
    else:
	usage()

if sslocation is None:
    usage()

# import xml module if we can
try:
    from xml.dom.minidom import Document
except ImportError:
    print FAIL_IMPORT_XML
    sys.exit(-2)

# Which Linux are we on?
if os.path.isfile('/etc/debian_version'):
    mode="DEB"
else:
    mode="RPM"

# Search for Ingres packages apropriately
if mode == "DEB":
    # Load apt module to query DEB repository
    try:
	import apt
    except ImportError:
	print FAIL_IMPORT_APT
	sys.exit(-2)

    searchDEBDB()
else:
    # Load rpm module to query the repository
    try:
	import rpm
	print "Querying saveset..."
        sshdr = getRPMSaveSetInfo( sslocation )
    except OSError, e:
	print FAIL_SSET_QRY % mode
	print str(e)
	sys.exit(-1)
    except ImportError:
	print FAIL_IMPORT_RPM
	sys.exit(-2)
   
    print "Querying %s database..." % mode
    searchRPMDB( sshdr )

sys.exit(genXML( instlist, ssinfo ))

