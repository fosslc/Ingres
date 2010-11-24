/*
** Copyright (c) 2004 Ingres Corporation
*/

# include <compat.h>
# include <gl.h>
# include <cs.h>
# include <er.h>
# include <lo.h>
# include <si.h>
# include <st.h>
# include <cm.h>
# include <lo.h>
# include <nm.h>
# include <si.h>
# include <sp.h>
# include <tm.h>
# include <mo.h>
# include <mu.h>
# include "moint.h"

/*
** Name:	modata.c
**
** Description:	Global data for mo facility.
**
**
LIBRARY = IMPCOMPATLIBDATA
**
**
** History:
**
**	24-sep-96 (mcgem01)
**	    Created.
**	18-dec-1996 (canor01)
**	    MO_GET_METHOD, MO_SET_METHOD and MO_INDEX_METHOD are
**	    typedefs that hide function declarations and should not
**	    be GLOBALDEFs.
**      21-jan-1999 (canor01)
**          Remove erglf.h.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      03-nov-2010 (joea)
**          Remove GLDLLdummy.  Move momon.c declarations to moint.h.
*/

/* momem.c */
GLOBALDEF i4 MO_nalloc        ZERO_FILL;
GLOBALDEF i4 MO_bytes_alloc   ZERO_FILL;
GLOBALDEF i4 MO_nfree         ZERO_FILL;
GLOBALDEF i4 MO_mem_limit = MO_DEF_MEM_LIMIT;

GLOBALDEF MO_CLASS_DEF MO_mem_classes[] =
{
    { 0, MO_NUM_ALLOC, sizeof(MO_nalloc), MO_READ, 0,
          0, MOintget, MOnoset, (PTR)&MO_nalloc, MOcdata_index },

    { 0, MO_NUM_FREE, sizeof(MO_nfree), MO_READ, 0,
          0, MOintget, MOnoset, (PTR)&MO_nfree, MOcdata_index },

    { 0, MO_MEM_LIMIT, sizeof(MO_mem_limit), MO_READ, 0,
          0, MOintget, MOintset, (PTR)&MO_mem_limit, MOcdata_index },

    { 0, MO_MEM_BYTES, sizeof(MO_bytes_alloc), MO_READ, 0,
          0, MOintget, MOnoset, (PTR)&MO_bytes_alloc, MOcdata_index },

    { 0 }
};

/* mometa.c */
MO_GET_METHOD MO_classid_get;
MO_GET_METHOD MO_oid_get;
MO_GET_METHOD MO_class_get;
MO_SET_METHOD MO_oid_set;
MO_SET_METHOD MO_oidmap_set;
MO_INDEX_METHOD MO_classid_index;

GLOBALDEF char MO_oid_map[ MAX_LOC ] 	ZERO_FILL;
GLOBALDEF SYSTIME MO_map_time 		ZERO_FILL;
static char mometa_index_class[] = MO_META_CLASSID_CLASS;

GLOBALDEF MO_CLASS_DEF MO_meta_classes[] =
{
    /* control objects */

    { 0, "exp.glf.mo.oid_map.file_name", sizeof( MO_oid_map ),
	  MO_READ|MO_SERVER_WRITE, 0,
	  0, MOstrget, MO_oidmap_set, (PTR)&MO_oid_map, MOcdata_index },

    { 0, "exp.glf.mo.oid_map.file_time", sizeof( MO_map_time ),
	  MO_READ, 0,
	  0, MOintget, MOnoset, (PTR)&MO_map_time, MOcdata_index },

    /* index class */

   { MO_INDEX_CLASSID, mometa_index_class,
	 0, MO_READ, 0,
	 CL_OFFSETOF( MO_CLASS, node.key ), MOstrpget, MOnoset,
	 0, MO_classid_index },

    /* special methods for handling twins */

   { 0, MO_META_CLASS_CLASS,
	 0, MO_READ, mometa_index_class,
	 0, MO_class_get, MOnoset,
	 0, MO_classid_index },

   { 0, MO_META_OID_CLASS,
	 0, MO_READ|MO_WRITE, mometa_index_class,
	 0, MO_oid_get, MO_oid_set,
	 0, MO_classid_index },

   /* vanilla methods */

   { 0, MO_META_SIZE_CLASS,
	 MO_SIZEOF_MEMBER( MO_CLASS, size ), MO_READ, mometa_index_class,
	 CL_OFFSETOF( MO_CLASS, size ), MOintget, MOnoset,
	 0, MO_classid_index },

   { 0, MO_META_OFFSET_CLASS,
	 MO_SIZEOF_MEMBER( MO_CLASS, offset ), MO_READ, mometa_index_class,
	 CL_OFFSETOF( MO_CLASS, offset ), MOintget, MOnoset,
	 0, MO_classid_index },

   { 0, MO_META_PERMS_CLASS,
	 MO_SIZEOF_MEMBER( MO_CLASS, perms ), MO_READ, mometa_index_class,
	 CL_OFFSETOF( MO_CLASS, perms ), MOintget, MOnoset,
	 0, MO_classid_index },

   { 0, MO_META_INDEX_CLASS,
	 0, MO_READ, mometa_index_class,
	 CL_OFFSETOF( MO_CLASS, index ), MOstrpget, MOnoset,
	 0, MO_classid_index },

   { 0 }
};

/* momon.c */

MO_GET_METHOD MO_mon_id_get;

static char momon_index_class[] = "exp.glf.mo.monitors.mon_id";

GLOBALDEF MO_CLASS_DEF MO_mon_classes[] =
{
    {  MO_INDEX_CLASSID, momon_index_class, 0, MO_READ, 0,
	   0, MO_mon_id_get, MOnoset, 0, MO_mon_index },

    {  0, "exp.glf.mo.monitors.classid", 0, MO_READ, momon_index_class,
	   0, MO_mon_class_get, MOnoset, 0, MO_mon_index },

    {  0, "exp.glf.mo.monitors.mon_block", 0, MO_READ, momon_index_class,
	   0, MOuivget, MOnoset, 0, MO_mon_index },

    {  0, "exp.glf.mo.monitors.mon_data",
	   MO_SIZEOF_MEMBER(MO_MON_BLOCK, mon_data), MO_READ, momon_index_class,
	   CL_OFFSETOF(MO_MON_BLOCK, mon_data), MOuintget, MOnoset,
	   0, MO_mon_index },

    {  0, "exp.glf.mo.monitors.monitor",
	   MO_SIZEOF_MEMBER(MO_MON_BLOCK, monitor), MO_READ, momon_index_class,
	   CL_OFFSETOF(MO_MON_BLOCK, monitor), MOuintget, MOnoset,
	   0, MO_mon_index },

    {  0, "exp.glf.mo.monitors.qual_regexp",
	   MO_SIZEOF_MEMBER(MO_MON_BLOCK, qual_regexp), MO_READ, momon_index_class,
	   CL_OFFSETOF(MO_MON_BLOCK, qual_regexp), MO_nullstr_get, MOnoset,
	   0, MO_mon_index },

    { 0 }
};

/* mostr.c */
MO_INDEX_METHOD MO_strindex;

GLOBALDEF i4 MO_max_str_bytes = MO_DEF_STR_LIMIT;
GLOBALDEF i4 MO_str_defines ZERO_FILL;
GLOBALDEF i4 MO_str_deletes ZERO_FILL;
GLOBALDEF i4 MO_str_bytes ZERO_FILL;
GLOBALDEF i4 MO_str_freed ZERO_FILL;
static char mostr_index_class[] = MO_STRING_CLASS;

GLOBALDEF MO_CLASS_DEF MO_str_classes[] =
{
    /* The strings table */

    { MO_INDEX_CLASSID, mostr_index_class,
	  0, MO_READ, 0,
	  CL_OFFSETOF( MO_STRING, buf[0] ), MOstrget, MOnoset,
	  NULL, MO_strindex },

    { 0, MO_STRING_REF_CLASS,
	  MO_SIZEOF_MEMBER( MO_STRING, refs ), MO_READ, mostr_index_class,
	  CL_OFFSETOF( MO_STRING, refs ), MOintget, MOnoset,
	  NULL, MO_strindex },

    /* Single instance global stuff */

    { 0, MO_STR_LIMIT, sizeof(MO_max_str_bytes), MO_READ, 0,
	  0, MOintget, MOnoset, (PTR)&MO_max_str_bytes, MOcdata_index },

    { 0, MO_STR_BYTES, sizeof(MO_str_bytes), MO_READ, 0,
	  0, MOintget, MOnoset, (PTR)&MO_str_bytes, MOcdata_index },

    { 0, MO_STR_DEFINED, sizeof(MO_str_defines), MO_READ, 0,
	  0, MOintget, MOnoset, (PTR)&MO_str_defines, MOcdata_index },

    { 0, MO_STR_DELETED, sizeof(MO_str_deletes), MO_READ, 0,
	  0, MOintget, MOnoset, (PTR)&MO_str_deletes, MOcdata_index },

    { 0, MO_STR_FREED, sizeof(MO_str_freed), MO_READ, 0,
	  0, MOintget, MOnoset, (PTR)&MO_str_freed, MOcdata_index },

    { 0 }
};

/* motrees.c */

GLOBALDEF char index_class[] = "exp.glf.mo.sptrees.name";

GLOBALDEF MO_CLASS_DEF MO_tree_classes[] = 
{
    /* this one is really attached */

    {  MO_INDEX_CLASSID|MO_CDATA_INDEX, index_class,
	   MO_SIZEOF_MEMBER(SPTREE, name), MO_READ, 0,
	   CL_OFFSETOF(SPTREE, name), MOstrpget, MOnoset,
	   0, MOidata_index },

    /* The rest are virtual, created with builtin methods */

    {  MO_CDATA_INDEX, "exp.glf.mo.sptrees.lookups",
	   MO_SIZEOF_MEMBER(SPTREE, lookups), MO_READ, index_class,
	   CL_OFFSETOF(SPTREE, lookups), MOintget, MOnoset,
	   0, MOidata_index },

    {  MO_CDATA_INDEX, "exp.glf.mo.sptrees.lkpcmps",
	   MO_SIZEOF_MEMBER(SPTREE, lkpcmps), MO_READ, index_class,
	   CL_OFFSETOF(SPTREE, lkpcmps), MOintget, MOnoset,
	   0, MOidata_index },

    {  MO_CDATA_INDEX, "exp.glf.mo.sptrees.enqs",
	   MO_SIZEOF_MEMBER(SPTREE, enqs), MO_READ, index_class,
	   CL_OFFSETOF(SPTREE, enqs), MOintget, MOnoset,
	   0, MOidata_index },

    {  MO_CDATA_INDEX, "exp.glf.mo.sptrees.enqcmps",
	   MO_SIZEOF_MEMBER(SPTREE, enqcmps), MO_READ, index_class,
	   CL_OFFSETOF(SPTREE, enqcmps), MOintget, MOnoset,
	   0, MOidata_index },

    {  MO_CDATA_INDEX, "exp.glf.mo.sptrees.splays",
	   MO_SIZEOF_MEMBER(SPTREE, splays), MO_READ, index_class,
	   CL_OFFSETOF(SPTREE, splays), MOintget, MOnoset,
	   0, MOidata_index },

    {  MO_CDATA_INDEX, "exp.glf.mo.sptrees.splayloops",
	   MO_SIZEOF_MEMBER(SPTREE, splayloops), MO_READ, index_class,
	   CL_OFFSETOF(SPTREE, splayloops ), MOintget, MOnoset,
	   0, MOidata_index },

    {  MO_CDATA_INDEX, "exp.glf.mo.sptrees.prevnexts",
	   MO_SIZEOF_MEMBER(SPTREE, prevnexts ), MO_READ, index_class,
	   CL_OFFSETOF(SPTREE, prevnexts), MOintget, MOnoset,
	   0, MOidata_index },

    { 0 }
};

/* moutil.c */

GLOBALDEF SPTREE *MO_instances ZERO_FILL;
GLOBALDEF SPTREE *MO_classes ZERO_FILL;
GLOBALDEF SPTREE *MO_strings ZERO_FILL;
GLOBALDEF SPTREE *MO_monitors ZERO_FILL;

GLOBALDEF MU_SEMAPHORE MO_sem ZERO_FILL;
GLOBALDEF i4  MO_semcnt ZERO_FILL;
GLOBALDEF bool MO_disabled = FALSE;

GLOBALDEF i4 MO_nattach    ZERO_FILL;
GLOBALDEF i4 MO_nclassdef  ZERO_FILL;
GLOBALDEF i4 MO_ndetach    ZERO_FILL;
GLOBALDEF i4 MO_nget       ZERO_FILL;
GLOBALDEF i4 MO_ngetnext   ZERO_FILL;
GLOBALDEF i4 MO_nset       ZERO_FILL;

GLOBALDEF i4 MO_ndel_monitor       ZERO_FILL;
GLOBALDEF i4 MO_nset_monitor       ZERO_FILL;
GLOBALDEF i4 MO_ntell_monitor      ZERO_FILL;

GLOBALDEF i4 MO_nmutex     ZERO_FILL;
GLOBALDEF i4 MO_nunmutex   ZERO_FILL;

GLOBALDEF MO_CLASS_DEF MO_cdefs[] =
{
    { 0, MO_NUM_ATTACH, sizeof( MO_nattach ), MO_READ, 0,
	  0, MOintget, MOnoset, (PTR)&MO_nattach , MOcdata_index },

    { 0, MO_NUM_CLASSDEF, sizeof( MO_nclassdef ), MO_READ, 0,
	  0, MOintget, MOnoset, (PTR)&MO_nclassdef , MOcdata_index },

    { 0, MO_NUM_DETACH, sizeof( MO_ndetach ), MO_READ, 0,
	  0, MOintget, MOnoset, (PTR)&MO_ndetach , MOcdata_index },

    { 0, MO_NUM_GET, sizeof( MO_nget ), MO_READ, 0,
	  0, MOintget, MOnoset, (PTR)&MO_nget , MOcdata_index },

    { 0, MO_NUM_GETNEXT, sizeof( MO_ngetnext ), MO_READ, 0,
	  0, MOintget, MOnoset, (PTR)&MO_ngetnext , MOcdata_index },

    { 0, MO_NUM_DEL_MONITOR, sizeof( MO_ndel_monitor ), MO_READ, 0,
	  0, MOintget, MOnoset, (PTR)&MO_ndel_monitor , MOcdata_index },

    { 0, MO_NUM_SET_MONITOR, sizeof( MO_nset_monitor ), MO_READ, 0,
	  0, MOintget, MOnoset, (PTR)&MO_nset_monitor , MOcdata_index },

    { 0, MO_NUM_TELL_MONITOR, sizeof( MO_ntell_monitor ), MO_READ, 0,
	  0, MOintget, MOnoset, (PTR)&MO_ntell_monitor , MOcdata_index },

    { 0, MO_NUM_MUTEX, sizeof( MO_nmutex ), MO_READ, 0,
	  0, MOintget, MOnoset, (PTR)&MO_nmutex , MOcdata_index },

    { 0, MO_NUM_UNMUTEX, sizeof( MO_nunmutex ), MO_READ, 0,
	  0, MOintget, MOnoset, (PTR)&MO_nunmutex , MOcdata_index },

    { 0 }
};
