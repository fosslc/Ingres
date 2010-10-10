

Supported Platforms
===================

We intend to support the build on all major distributions and operating systems.

Please note: As of April 29, 2008 only Fedora Core 8 - 32 bit builds and runs
cleanly with no issues. 

Other platforms including Ubuntu (32 bit/64 bit), Fedora (64 bit), Suse,
Windows, and MacOS are almost clean. If you run into an issue, please check
http://bugs.ingres.com to see if the issue is an existing one.

If it is a new issue, please raise a ticket. Thank you.


To get the latest version of Ingres server source code:
=======================================================

svn co http://code.ingres.com/ingres/main ~/ingres/server
(this will create a copy of the latest code in ~/ingres/server)

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
