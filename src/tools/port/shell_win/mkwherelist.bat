REM
REM  mkwherelist - generate list for the where command
REM
REM  History:
REM
REM	14-jul-95 (tutto01)
REM	    Created.
REM

rm %ING_SRC%\wherelist
REM for %%I in (admin back cl common dbutil front gl gateway sig testtool tools)            do find %ING_SRC%\%%I -name '*' -print >> %ING_SRC%\wherelist.tmp
for %%I in (admin back cl common dbutil front gl gateway sig testtool tools ing_tst)            do find %ING_SRC%\%%I -name '*' -print >> %ING_SRC%\wherelist
REM cat %ING_SRC%\wherelist.tmp | egrep -v "MANIFEST|exp$|pdb$|err$|obj$|srclist|dirlist|makefile" > %ING_SRC%\wherelist
rm %ING_SRC%\wherelist.tmp
