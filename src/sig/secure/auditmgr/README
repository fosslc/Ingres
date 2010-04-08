AUDITMGR
--------
This directory contains ABF/VISION source code for the AUDITMGR utility.

This utility provides a convenient method for:

- Maintaining the INGRES security audit state.

- Updating the INGRES security audit mask.

- Registering and Removing Security audit logs

- Viewing, Selecting and Extracting data from audit logs.

The application should be built and run against the master database, iidbdb
by a privileged user or application. It performs INGRES privilege bracketing
as necessary to gain and release privileges.

To load the application into iidbdb use the following command:

    UNIX:
    copyapp in -a$II_SYSTEM/ingres/sig/auditmgr -nauditmgr iidbdb iicopyapp.tmp

    WINDOWS:
    copyapp in -a%II_SYSTEM%\ingres\sig\auditmgr -nauditmgr iidbdb iicopyapp.tmp

If you wish to image the application use:

    imageapp iidbdb auditmgr -oauditmgr 

