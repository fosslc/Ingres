@echo off
call "%II_SYSTEM%\ingres\bin\setingenvs.bat"
unlodctr oipfctrs
regedit /s "%II_SYSTEM%\ingres\files\OIPFCTRS.REG"
lodctr "%II_SYSTEM%\ingres\files\OIPFCTRS.INI"
perfmon %1
