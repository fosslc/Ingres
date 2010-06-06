/* Description of table iirelation from database iidbdb */
  EXEC SQL DECLARE iirelation TABLE
	(reltid	integer not null,
	 reltidx	integer not null,
	 relatts	smallint not null,
	 reltcpri	smallint not null,
	 relkeys	smallint not null,
	 relspec	smallint not null,
	 relstat	integer not null,
	 relstat2	integer not null,
	 reltups	bigint not null,
	 relpages	bigint not null,
	 relprim	integer not null,
	 relmain	integer not null,
	 relsave	integer not null,
	 relstamp1	integer not null,
	 relstamp2	integer not null,
	 relcmptlvl	integer not null,
	 relcreate	integer not null,
	 relqid1	integer not null,
	 relqid2	integer not null,
	 relmoddate	integer not null,
	 relidxcount	smallint not null,
	 relifill	smallint not null,
	 reldfill	smallint not null,
	 rellfill	smallint not null,
	 relmin	integer not null,
	 relmax	integer not null,
	 relpgsize	integer not null,
	 relgwid	smallint not null,
	 relgwother	smallint not null,
	 relhigh_logkey	integer not null,
	 rellow_logkey	integer not null,
	 relfhdr	integer not null,
	 relallocation	integer not null,
	 relextend	integer not null,
	 relcomptype	smallint not null,
	 relpgtype	smallint not null,
	 relloccount	smallint not null,
	 relversion	smallint not null,
	 relwid	integer not null,
	 reltotwid	integer not null,
	 reldatawid	integer not null,
	 reltotdatawid	integer not null,
	 relattnametot	integer not null,
	 relnparts	smallint not null,
	 relnpartlevels	smallint not null,
	 relencflags	smallint not null,
	 relencver	smallint not null,
	 relenckey	byte(64) not null,
	 relfree	byte(16) not null,
	 relloc	char(32) not null,
	 relowner	char(32) not null,
	 relid	char(256) not null);

  struct rel_tbl_ {
	i4	reltid;
	i4	reltidx;
	i2	relatts;
	i2	reltcpri;
	i2	relkeys;
	i2	relspec;
	i4	relstat;
	i4	relstat2;
	i8	reltups;
	i8	relpages;
	i4	relprim;
	i4	relmain;
	i4	relsave;
	i4	relstamp1;
	i4	relstamp2;
	u_i4	relcmptlvl;
	i4	relcreate;
	i4	relqid1;
	i4	relqid2;
	i4	relmoddate;
	i2	relidxcount;
	i2	relifill;
	i2	reldfill;
	i2	rellfill;
	i4	relmin;
	i4	relmax;
	i4	relpgsize;
	i2	relgwid;
	i2	relgwother;
	i4	relhigh_logkey;
	i4	rellow_logkey;
	i4	relfhdr;
	i4	relallocation;
	i4	relextend;
	i2	relcomptype;
	i2	relpgtype;
	i2	relloccount;
	i2	relversion;
	i4	relwid;
	i4	reltotwid;
	i4	reldatawid;
	i4	reltotdatawid;
	i4	relattnametot;
	i2	relnparts;
	i2	relnpartlevels;
	u_i2	relencflags;
	i2	relencver;
	u_char	relenckey[64];
	char	relfree[17];
	char	relloc[33];
	char	relowner[33];
	char	relid[257];
  } rel_tbl;
