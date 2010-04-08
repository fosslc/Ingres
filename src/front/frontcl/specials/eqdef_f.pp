C
C Copyright (c) 2005 Ingres Corporation
C
C	eqdef.f
C		-- External INGRES integer functions for UNIX F77 FORTRAN
C
C	Because of APOLLO Fortran, tabs are no longer legal.
C
C History:
C	01-Nov-2005 (hanje04)
C	    Added history.
C	    BUGS 113212, 114839 & 115285.
C	    Make IIsd, IIsdno, IIps, IIstr, IInum & IIsadr INTEGER*8 on
C	    64bit Linux.
C
      external IInexe, IInext, IItupg, IIerrt, IIcsRe, IILprs
      external IIsd, IIsdno, IIps
      integer  IInexe, IInext, IItupg, IIerrt, IIcsRe, IILprs
#if defined(i64_hpu) || defined(axp_osf) || (defined(LNX) && defined(LP64))
      integer*8  IIsd, IIsdno, IIps
#else
      integer    IIsd, IIsdno, IIps
#endif
C
C -- Utility calls for user
      external IIstr, IInum, IIslen, IIsadr
      integer  IIslen
#if defined(i64_hpu) || defined(axp_osf) || (defined(LNX) && defined(LP64))
      integer*8  IIstr, IInum, IIsadr
#else
      integer    IIstr, IInum, IIsadr
#endif
