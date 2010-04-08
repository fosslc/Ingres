/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: ASCF.H - Function prototypes for ascf. 
**
** Description:
**      This fuile contains the ascf specific function prototypes.
**
** History:
**      18-Jan-1999 (fanra01)
**          Created.
**      12-Feb-99 (fanra01)
**          Correct prototype for ascs_dispatch_query.
**      17-May-2000 (fanra01)
**          Bug 101345
**          Add func prototypes ascs_format_ice_header,
**          ascs_format_ice_response and defines for response indicators.
**          Add func ascs_tuple_len for current message length.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

# ifndef ASCF_INCLUDED
# define ASCF_INCLUDED

# define ICE_RESP_HEADER    0x0002
# define ICE_RESP_FINAL     0x0001

/*
**  Forward and/or External typedef/struct references.
*/

/* Function externs */
FUNC_EXTERN DB_STATUS ascs_gca_send(SCD_SCB *scb);
FUNC_EXTERN DB_STATUS ascs_gca_recv(SCD_SCB *scb);
FUNC_EXTERN DB_STATUS ascs_gca_flush(SCD_SCB *scb);
FUNC_EXTERN i4        ascs_gca_data_available(SCD_SCB *scb);
FUNC_EXTERN DB_STATUS ascs_gca_get(SCD_SCB *scb, PTR get_ptr, i4 get_len);
FUNC_EXTERN DB_STATUS ascs_gca_flush_outbuf(SCD_SCB *scb);

FUNC_EXTERN DB_STATUS ascs_desc_send(SCD_SCB *scb, SCC_GCMSG **pmessage );
FUNC_EXTERN DB_STATUS ascs_dispatch_query(SCD_SCB *scb);
FUNC_EXTERN DB_STATUS ascs_get_next_token(char **stmt_pos, char *stmt_end,
                             i4  *token_type, i4  *keyword_type, char *word);
FUNC_EXTERN DB_STATUS ascs_sequencer(i4 op_code, SCD_SCB *scb, i4  *next_op );
FUNC_EXTERN DB_STATUS ascs_process_close(SCD_SCB *scb);
FUNC_EXTERN DB_STATUS ascs_process_fetch(SCD_SCB *scb);
FUNC_EXTERN DB_STATUS ascs_process_interrupt(SCD_SCB *scb);
FUNC_EXTERN DB_STATUS ascs_process_query(SCD_SCB *scb);
FUNC_EXTERN DB_STATUS ascs_process_procedure(SCD_SCB *scb);
FUNC_EXTERN DB_STATUS ascs_process_request(SCD_SCB *scb);
FUNC_EXTERN DB_STATUS ascs_format_response(SCD_SCB *scb, i4  resptype, i4  qrystatus , i4  rowcount);
FUNC_EXTERN DB_STATUS ascs_format_ice_header( SCD_SCB *scb, u_i4 resp_type,
    i4 hlength, char *header );
FUNC_EXTERN DB_STATUS ascs_format_ice_response( SCD_SCB *scb, i4 pglen,
        char* response, i4 msgind, i4* sent );
FUNC_EXTERN PTR ascs_save_tdesc( SCD_SCB *scb, PTR desc );
FUNC_EXTERN i4 ascs_tuple_len( SCD_SCB *scb );
# endif /* ASCF_INCLUDED */
