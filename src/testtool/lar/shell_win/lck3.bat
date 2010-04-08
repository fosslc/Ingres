@echo off
REM
REM History
REM	19-sep-1995 (somsa01)
REM		Created.
REM
REM
REM %1 = dbname
REM %2 = delapp_loops
REM %3 = cpdbmask
REM %4 = df

PCdate > CKPDB%4_LCK3
ckpdb_delapp %1 %2 |sed -f %3 >ckpdb_da%4.out1 2>&1
del CKPDB%4_LCK3
