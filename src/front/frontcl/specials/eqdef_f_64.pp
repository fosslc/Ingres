C
C Copyright (c) 2008 Ingres Corporation
C
C	eqdef.f
C		-- External INGRES integer functions for UNIX F77 FORTRAN
C
C	Because of APOLLO Fortran, tabs are no longer legal.
C
C History:
C	20-Jun-2008 (hweho1)
C	    Added for 64-bit support on Unix hybrid platforms. 
C	    The file is made from eqdef_f.pp rev. 11 file.
C
      external IInexe, IInext, IItupg, IIerrt, IIcsRe, IILprs
      external IIsd, IIsdno, IIps
      integer  IInexe, IInext, IItupg, IIerrt, IIcsRe, IILprs
      integer*8  IIsd, IIsdno, IIps
C
C -- Utility calls for user
      external IIstr, IInum, IIslen, IIsadr
      integer  IIslen
      integer*8  IIstr, IInum, IIsadr
