@echo off
SET II_SYSTEM=%programfiles%\CA\Ingres
SET PATH=%II_SYSTEM%\ingres\bin;%II_SYSTEM%\ingres\utility;%PATH%
SET LIB=%II_SYSTEM%\ingres\lib;%LIB%
SET INCLUDE=%II_SYSTEM%\ingres\files;%INCLUDE%
SET TERM_INGRES=IBMPCD
MODE CON CP SELECT=1252 > C:\NUL
TITLE Ingres
