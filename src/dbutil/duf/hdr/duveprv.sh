/*
**Copyright (c) 2004 Ingres Corporation
*/
/* Description of table iipriv from database iidbdb */
  EXEC SQL DECLARE iipriv TABLE
	(d_obj_id	integer not null,
	 d_obj_idx	integer not null,
	 d_priv_number	smallint not null,
	 d_obj_type	smallint not null,
	 i_obj_id	integer not null,
	 i_obj_idx	integer not null,
	 i_priv	integer not null,
	 i_priv_grantee	char(32) not null,
	 prv_flags	integer not null,
	 i_priv_map1	integer not null,
	 i_priv_map2	integer not null,
	 i_priv_map3	integer not null,
	 i_priv_map4	integer not null,
	 i_priv_map5	integer not null,
	 i_priv_map6	integer not null,
	 i_priv_map7	integer not null,
	 i_priv_map8	integer not null,
	 i_priv_map9	integer not null,
	 i_priv_mapa	integer not null,
	 i_priv_map11	integer not null,
	 i_priv_map12	integer not null,
	 i_priv_map13	integer not null,
	 i_priv_map14	integer not null,
	 i_priv_map15	integer not null,
	 i_priv_map16	integer not null,
	 i_priv_map17	integer not null,
	 i_priv_map18	integer not null,
	 i_priv_map19	integer not null,
	 i_priv_map20	integer not null,
	 i_priv_map21	integer not null,
	 i_priv_map22	integer not null,
	 i_priv_map23	integer not null,
	 i_priv_map24	integer not null,
	 i_priv_map25	integer not null,
	 i_priv_map26	integer not null,
	 i_priv_map27	integer not null,
	 i_priv_map28	integer not null,
	 i_priv_map29	integer not null,
	 i_priv_map30	integer not null,
	 i_priv_map31	integer not null,
	 i_priv_map32	integer not null,
	 i_priv_map33	integer not null
);

  struct prv_tbl_ {
	i4	d_obj_id;
	i4	d_obj_idx;
	short	d_priv_number;
	short	d_obj_type;
	i4	i_obj_id;
	i4	i_obj_idx;
	i4	i_priv;
	char	i_priv_grantee[33];
	i4	prv_flags;
	i4	i_priv_map1;
	i4	i_priv_map2;
	i4	i_priv_map3;
	i4	i_priv_map4;
	i4	i_priv_map5;
	i4	i_priv_map6;
	i4	i_priv_map7;
	i4	i_priv_map8;
	i4	i_priv_map9;
	i4	i_priv_mapa;
	i4	i_priv_map11;
	i4	i_priv_map12;
	i4	i_priv_map13;
	i4	i_priv_map14;
	i4	i_priv_map15;
	i4	i_priv_map16;
	i4	i_priv_map17;
	i4	i_priv_map18;
	i4	i_priv_map19;
	i4	i_priv_map20;
	i4	i_priv_map21;
	i4	i_priv_map22;
	i4	i_priv_map23;
	i4	i_priv_map24;
	i4	i_priv_map25;
	i4	i_priv_map26;
	i4	i_priv_map27;
	i4	i_priv_map28;
	i4	i_priv_map29;
	i4	i_priv_map30;
	i4	i_priv_map31;
	i4	i_priv_map32;
	i4	i_priv_map33;
  } prv_tbl;
