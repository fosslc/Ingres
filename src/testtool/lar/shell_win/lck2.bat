@echo off
REM
REM History
REM	19-sep-1995 (somsa01)
REM		Created.
REM
REM
REM %1 = dbname
REM %2 = append_loops
REM %3 = cpdbmask
REM %4 = df

PCdate > CKPDB%4_LCK2
ckpdb_append %1 %2 |sed -f %3 >ckpdb_ap%4.out1 2>&1
del CKPDB%4_LCK2
