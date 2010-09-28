:
#
## Copyright Ingres Corporation 2010. All Rights Reserved.
##
##
##
##  Name: iiposttrans -- pre installation setup script usually run by
##			RPM %posttrans script
##
##  Usage:
##       iiposttrans
##
##  Description:
##      Runs pre-setup 
##      DEB or RPM packages
##
##  Exit Status:
##       0       setup procedure completed.
##       1       setup procedure did not complete.
##
##  DEST = utility
### History:
##      14-Jul-2010 (hanje04)
##	    BUG 124083
##          Created.

## Need II_SYSTEM to be set
[ -z "$II_SYSTEM" ] &&
{
    echo "II_SYSTEM is not set"
    exit 1
}

PATH=$II_SYSTEM/ingres/bin:$II_SYSTEM/ingres/utility:$PATH
LD_LIBRARY_PATH=/lib:/usr/lib:$II_SYSTEM/ingres/lib
export II_SYSTEM PATH LD_LIBRARY_PATH
CONFIG_HOST=`iipmhost`
instid=`ingprenv II_INSTALLATION`
ingvalidpw=`ingprenv II_SHADOW_PWD`
iiuid=`iigetres "ii.${CONFIG_HOST}.setup.owner.user"`
iigid=`iigetres "ii.${CONFIG_HOST}.setup.owner.group"`
II_USERID=${iiuid:-ingres}
II_GROUPID=${iigid:-ingres}

# correct dir ownership
chown $II_USERID:$II_GROUPID ${II_SYSTEM}/ingres/.
for dir in bin demo files ice install lib sig utility vdba
do
    [ -d "${II_SYSTEM}/ingres/${dir}/." ] && \
         chown -R $II_USERID:$II_GROUPID ${II_SYSTEM}/ingres/${dir}/.
done

chown $II_USERID:$II_GROUPID ${II_SYSTEM}/ingres/version.rel
[ -f "$ingvalidpw" ] &&
{
    chown root:root $ingvalidpw
    chmod 4755 $ingvalidpw
}

exit 0
