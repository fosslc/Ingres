****************** Build Environment *****************************
***     2.0/0001/00
***     MARK 2250
***     Fixes in this build area on top of the mark are: None
****************** Build Procedure *******************************
***   1. pb_setenv DEEDXID pxxxx
***            where xxxx is your patch number
***   2. pb_setenv PMFKEY yyyyyyy
***            where yyyyyyy is your pmf key
***   3. pb_setup SFP $DEEDXID mzzzz
***            where zzzz is current mark (e.g. 2225)
***   4. pb_ckbase
***   5. Open and modify piccolo files containing your changes
***   6. Issue "nmake" or "nmake -a" in directories affected by
***      your source code change(s)
***   7. If this is an SFP, copy front\st\patch\patch.txt to
***      patch_0011.txt.  If this is a service pack, update
***      front\st\patch\patch.txt.servicepack and patch.txt and then
***      copy patch.txt.servicepack to patch_0011.txt.
***      Add Release Notes for bug(s) you've fixed to patch_0011.txt.
***      Also update the date and patch number.
***   8. pb_pack
***   9. Use Winzip to create pxxxx.zip file in II_RELEASE_DIR\pack
****************************************************************
****************************************************************
Setting environment for using Microsoft Visual C++ tools.
Setting Ingres Development environment variables.
