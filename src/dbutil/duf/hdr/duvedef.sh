/*
**Copyright (c) 2004 Ingres Corporation
*/
/* Description of table iidefault from database iidbdb */
  EXEC SQL DECLARE iidefault TABLE
	(defid1	  integer not null,
	 defid2	  integer not null,	
	 defvalue varchar(1501));

  struct default_tbl_ {
	i4	defid1;
	i4	defid2;
	char	defvalue[1502];
   } default_tbl;		
