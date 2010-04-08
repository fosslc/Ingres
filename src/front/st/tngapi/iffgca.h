/*
** Copyright (c) 2004 Ingres Corporation
*/

/*
** Name: iffgca
**
** Description:
**      Structures, types and definitions for the IFF communications
**      interface.
**
** History:
**      18-Feb-2004 (fanra01)
**          Created.
**      26-Mar-2004 (fanra01)
**          Add prototypes.
**      22-Apr-2004 (fanra01)
**          Add prototype for iff_load_instances.
**          Modify iff_dbms_server prototype to remove protocol.
**      07-Feb-2005 (fanra01)
**          Sir 113881
**          Merge Windows and UNIX sources.
*/
# ifndef __IFFGCA_INCLUDED
# define __IFFGCA_INCLUDED
typedef	struct
{

    QUEUE	q;
    char	id[ GCN_VAL_MAX_LEN + 1 ];
    char	addr[ GCN_VAL_MAX_LEN + 1 ];

} INST_DATA;

typedef struct
{

    QUEUE	q;
    char	*data[1];	/* Variable length */

} GCM_DATA;

typedef struct
{

    char	*classid;
    char	value[ GCN_VAL_MAX_LEN + 1 ];

} GCM_INFO;


#define ARRAY_SIZE( array )	(sizeof( array ) / sizeof( (array)[0] ))

FUNC_EXTERN VOID
iff_display( ING_INS_CTX* ins_ctx, ING_INS_DATA* server, ING_INS_DATA* proto,
    ING_INS_DATA* instance, ING_INS_DATA* config );

FUNC_EXTERN STATUS
iff_load_install( ING_INS_CTX* ins_ctx, ING_CONNECT *tgt, ING_INS_DATA *instance );

FUNC_EXTERN STATUS
iff_load_instances( ING_INS_CTX* ins_ctx, ING_CONNECT *tgt,
    ING_INS_DATA *instance );

FUNC_EXTERN VOID
iff_unload_install( ING_INS_DATA *install );

FUNC_EXTERN VOID
iff_unload_values( ING_INS_DATA* value );

FUNC_EXTERN STATUS
iff_dbms_server( ING_INS_CTX* ins_ctx, ING_CONNECT* tgt, char* addr,
    ING_INS_DATA* server, i4* dbms );

FUNC_EXTERN STATUS
iff_load_instance( ING_INS_CTX* ins_ctx, ING_CONNECT *tgt, char *addr,
    ING_INS_DATA* instance );

FUNC_EXTERN STATUS
iff_load_config( ING_INS_CTX* ins_ctx, ING_CONNECT *tgt, char *addr,
    ING_INS_DATA* config );

FUNC_EXTERN i4 iff_msg_max( VOID );

FUNC_EXTERN STATUS iff_gca_init( VOID );

FUNC_EXTERN STATUS iff_gca_term( VOID );

FUNC_EXTERN STATUS
iff_gca_ping( ING_CONNECT* tgt, char* msg_buff, BOOL* active );

# endif /* __IFFGCA_INCLUDED */
