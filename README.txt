Ingres is a feature rich and robust database (RDBMS)

It is licensed under the GPL license, version 2.

Supported Platforms
===================

For more information on building Ingres, including supported platforms see:
http://community.ingres.com/wiki/Building_Ingres_FAQ

If it is a new issue, please raise a ticket. Thank you.


To get the latest version of Ingres server source code:
=======================================================

This git repository is a clone of subversion currently.

To get the latest code:

svn co http://code.ingres.com/ingres/main ~/ingres/server
(this will create a copy of the latest code in ~/ingres/server)

or

git clone http://github.com/ingres/Ingres.git ~/ingres/server

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

3) source buildtools/test_env.sh <same installation ID>

ingstart (will start the dbms)
ingstop (will stop the dbms)

To build, install and run an automated test suite:
==================================================
Become root.
buildtools/buildAndTest.sh <2 character installation ID>

To run the build (the less easy way):
=====================================

Visit: http://community.ingres.com/wiki/Category:DBMS
(see the Building Ingres on _____ under B in the directory)

Where to get help/contact us:
=============================

comp.databases.ingres
(http://groups.google.com/group/comp.databases.ingres/topics)

IRC channel #ingres in irc.freenode.net
