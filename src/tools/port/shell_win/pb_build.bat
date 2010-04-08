@echo off
@echo After issuing purge/mypurge ...
@echo Copy license files to cl ...
cp %ING_SRC%\treasure\NT\CA_LIC\98\utils\VSC50\lic98mtdll.lib %ING_SRC%\cl\lib\.
cp %ING_SRC%\treasure\NT\CA_LIC\98\h\lic98.h %ING_SRC%\cl\clf\ci\.
attrib +r %ING_SRC%\cl\lib\lic98mtdll.lib
attrib +r %ING_SRC%\cl\clf\ci\lic98.h
@echo Remove new.h due to conflict when building VDBA (not implemented) 


