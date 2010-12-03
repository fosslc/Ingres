Ingres is a feature rich and robust database (RDBMS)

It is licensed under the GPL license, version 2.

Supported Platforms
===================

For more information on building Ingres, including supported platforms see:
http://community.ingres.com/wiki/Building_Ingres_FAQ

If you need help, visit:
http://webchat.freenode.net/?channels=#ingres
(tip: make up a unique nickname)


To get the latest version of Ingres server source code:
=======================================================

To get the latest code:

svn co http://code.ingres.com/ingres/main ~/ingres/server
OR, alternatively if you prefer git:
git clone http://github.com/ingres/Ingres.git ~/ingres/server

(either of these will create a copy of the latest code in ~/ingres/server)

Prerequisites:
==============

This version of Ingres includes geospatial types and functions built-in.
As a result, it requires two additional libraries. They are:
1) GEOS which is acquired here: http://trac.osgeo.org/geos/
   GEOS version 3.3.0 is the target version that includes Ingres required 
   changes.  As of Nov, 2010 an official 3.3.0 verision has not been released.
   The HEAD revision from svn is expected to work.  The last known tested
   revision is 3117.
2) Proj.4 which is acquired here: http://trac.osgeo.org/proj/
   Proj.4 has been packaged for most systems and those packages can be used
   as is.

Other dependencies will be double-checked by the runbuild script.
It can usually tell you what you're missing and where to get it.


To run the build (the easy way):
================================

1) cd ~/ingres/server
2) ./runbuild.sh

The runbuild.sh script will set up your environment, execute the build
and provide detailed logs.

To set up your environment to develop:
======================================

cd ~/ingres/server
source buildtools/set_env.sh

You can now "cd ~/ingres/server/src" and run jam to build by hand.

To install and use an instance:
==============================-

1) Run the build as above.

2) Run buildtools/createdbms <2 character installation ID>

You can install many instances of Ingres on the same machine provided
the installation ID's do not conflict.

for example:
buildtools/createdbms A1

3) source buildtools/test_env.sh <same installation ID>

for example:
buildtools/test_env.sh A1

ingstart (will start the dbms)
ingstop (will stop the dbms)

To build, install and run an automated test suite:
==================================================
Become root.
buildtools/buildAndTest.sh <2 character installation ID>

for example:

buildtools/buildAndTest.sh A1

To run the build (the less easy way):
=====================================

Visit: http://community.ingres.com/wiki/Category:DBMS
(see the Building Ingres on _____ under B in the directory)

Where to get help/contact us:
=============================

Internet Relay Chat (IRC):
http://webchat.freenode.net/?channels=#ingres

comp.databases.ingres
http://groups.google.com/group/comp.databases.ingres/topics

