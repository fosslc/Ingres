:
#
# Copyright (c) 2004 Ingres Corporation
#
#	SCRIPT TO RESET THE TIME ZONE 
#
## History:
##	01-May-2000 (vande02) Created to SAVE the current Ingres installation's
##			      II_TIMEZONE_NAME setting and then RESTORE it at
##			      end.  Back-end qryproc test qp158.sep has always
##			      existed to test the timezone settings but never
##			      put the original timezone back to what it was
##			      set to during the Ingres installation.
##	19-May-2000 (hanch04)
##	    Scripts should not have the shell at the first line
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          then the copyright should be next.

if [ "$1" = "SAVE" ]; then

	qasetuser ingres ingsetenv SAVETZ `ingprenv II_TIMEZONE_NAME`

else

	qasetuser ingres ingsetenv II_TIMEZONE_NAME `ingprenv SAVETZ`

fi
