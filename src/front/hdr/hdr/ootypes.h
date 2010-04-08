/*
**	Copyright (c) 2004 Ingres Corporation
*/
# ifndef SYSTYPES_INCLUDE
# define SYSTYPES_INCLUDE
# ifdef major
# undef	major
# undef	minor
# endif
typedef	struct {
    u_i4	levelbits:29;
    u_i4	dirty:1;
    u_i4	inDatabase:1;
    u_i4	dbObject:1;
} dataWord;
/*
**	Object.h -- instance C structure declaration 
*/
  typedef struct OBJECT {
    i4  numid;
	dataWord	data;
    struct TYPE {
      i4  numid;
	dataWord	data;
      struct TYPE *type;
      char *name;
      i4  env;
      char *owner;
      i4  iscurrent;
      char *remarks;
      char *table;
      char *surrogate;
      struct TYPE *super;
      i4  level;
      i4  keepstatus;
      i4  hasarray;
      i4  memsize;
      struct COLLECTION {
        i4  numid;
	dataWord	data;
        struct TYPE *type;
        char *name;
        i4  env;
        char *owner;
        i4  iscurrent;
        char *remarks;
        i4  size;
        i4  current;
        i4  atend;
        struct OBJECT *array[1];
      } *subTypes;
      struct COLLECTION *instances;
      struct COLLECTION *masterRefs;
      struct COLLECTION *detailRefs;
      struct COLLECTION *operations;
      struct COLLECTION *attributes;
    } *type;
    char *name;
    i4  env;
    char *owner;
    i4  iscurrent;
    char *remarks;
  } *OBJECT;
/*
**	Type.h -- instance C structure declaration 
*/
  typedef struct TYPE *TYPE;
/*
**	Collection.h -- instance C structure declaration 
*/
  typedef struct COLLECTION *COLLECTION;
/*
**	Reference.h -- instance C structure declaration 
*/
  typedef struct REFERENCE {
    i4  numid;
	dataWord	data;
    struct TYPE *type;
    char *name;
    i4  env;
    char *owner;
    i4  iscurrent;
    char *remarks;
    struct TYPE *master;
    i4  mdelete;
    i4  moffset;
    char *connector;
    struct TYPE *detail;
    i4  ddelete;
    i4  doffset;
  } *REFERENCE;
/*
**	Operation.h -- instance C structure declaration 
*/
  typedef struct OPERATION {
    i4  numid;
	dataWord	data;
    struct TYPE *type;
    char *name;
    i4  env;
    char *owner;
    i4  iscurrent;
    char *remarks;
    struct TYPE *optype;
    char *procname;
    i4  entrypt;
    i4  argcount;
    i4  defaultperm;
    i4  keepstatus;
    i4  fetchlevel;
  } *OPERATION;
/*
**	CompObj.h -- instance C structure declaration 
*/
  typedef struct COMPOBJ {
    i4  numid;
 	dataWord	data;
    struct TYPE *type;
    char *name;
    i4  env;
    char *owner;
    i4  iscurrent;
    char *remarks;
    i4  object;
    i4  seq;
    char *cstring;
  } *COMPOBJ;
/*
**	UnknownRef.h -- instance C structure declaration 
*/
  typedef struct UNKNOWNREF {
    i4  numid;
 	dataWord	data;
  } *UNKNOWNREF;
  typedef struct ATTRIB {
    i4  attnum;
    char *name;
    i4  frmt;
    i4  frml;
  } *ATTRIB;
/*
**	externs -- type objects declared extern
*/
  GLOBALREF struct OBJECT nil[];
  GLOBALREF struct TYPE Object[];
  GLOBALREF struct TYPE Type[];
  GLOBALREF struct TYPE Reference[];
  GLOBALREF struct TYPE Operation[];
  GLOBALREF struct TYPE Collection[];
  GLOBALREF struct TYPE CompObj[];
  GLOBALREF struct TYPE UnknownRef[];
# endif /* SYSTYPES_INCLUDE */
