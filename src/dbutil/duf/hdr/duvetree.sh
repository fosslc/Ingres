/*
**Copyright (c) 2004 Ingres Corporation
*/
/* Description of table iitree from database iidbdb */
  EXEC SQL DECLARE iitree TABLE
	(treetabbase	integer not null,
	 treetabidx	integer not null,
	 treeid1	integer not null,
	 treeid2	integer not null,
	 treeseq	smallint not null,
	 treemode	smallint not null,
	 treevers	smallint not null,
	 treetree	varchar(1024) not null);

  struct tree_tbl_ {
	i4	treetabbase;
	i4	treetabidx;
	i4	treeid1;
	i4	treeid2;
	short	treeseq;
	short	treemode;
	short	treevers;
	char	treetree[1025];
  } tree_tbl;
