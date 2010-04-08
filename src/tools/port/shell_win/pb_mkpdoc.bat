@echo off
echo Have you created curbugrns.txt in %ING_SRC%\front\st\patch?
echo If not, press Ctrl-C now, create the file, and re-run this program.
pause
cd %ING_SRC%\front\st\patch
p open patch.txt 
cp %PATCHTMP%\m%MARK%\%MARKPATCH%\ingres\patch.txt patch_0011.txt
echo Editing patch_0011.txt ... 
echo Fixes added >>patch_0011.txt
cat %1 >> patch_0011.txt
echo You must modify patch.txt. Move the added lines to the appropriate section.
