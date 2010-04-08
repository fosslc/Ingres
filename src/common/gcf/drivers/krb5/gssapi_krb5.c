/*
** Copyright (c) 2004 Ingres Corporation
*/

/*
** Name: gssapi_krb5.c
**
** Description:
**
** This file contains stubbed GSS API routines used in the Kerberos driver
** shared library. Its only purpose is to provide function entry points that
** the driver will resolve at runtime with the actual routines in the
** Kerberos installation.
**
** History:
**	04-Jan-02 loera01 SIR 106743
**	   Created.
**      20-Apr-05 (loera01) SIR 114358
**         Added Windows prototypes.
**	6-Feb-2007 (bonro01)
**	   Update Kerberos headers to v5 r1.6
**	06-mar-2007 (abbjo03)
**	    Include inttypes.h on VMS.
**	23-Mar-2007 (bonro01)
**	   Fix compile problem for HPUX.
*/
#include <bzarch.h>

#if defined(NT_GENERIC) || defined(thr_hpux)
/* No additional definitions required */
#elif defined(sparc_sol) || defined(VMS)
#include <inttypes.h>

#else
/* include stdint.h for uint32_t data type */
#include <stdint.h>
#endif

#include <gssapi/gssapi_generic.h>

OM_uint32 KRB5_CALLCONV gss_acquire_cred(OM_uint32 *a,
            gss_name_t b,
            OM_uint32 c,	
            gss_OID_set d,	
            gss_cred_usage_t e,
            gss_cred_id_t *f,
            gss_OID_set *g,
            OM_uint32 *h		
           )
{
	return 0;
}

OM_uint32 KRB5_CALLCONV gss_release_cred
(OM_uint32 *b,		/* minor_status */
            gss_cred_id_t *a		/* cred_handle */
           )
{
	return 0;
}

OM_uint32 KRB5_CALLCONV gss_init_sec_context
(OM_uint32 *z,		/* minor_status */
            gss_cred_id_t a,		/* claimant_cred_handle */
            gss_ctx_id_t *b,		/* context_handle */
            gss_name_t c,			/* target_name */
            gss_OID d,			/* mech_type (used to be const) */
            OM_uint32 e,			/* req_flags */
            OM_uint32 f,			/* time_req */
            gss_channel_bindings_t g,	/* input_chan_bindings */
            gss_buffer_t h,		/* input_token */
            gss_OID * i,		/* actual_mech_type */
            gss_buffer_t j,		/* output_token */
            OM_uint32 * k,		/* ret_flags */
            OM_uint32 *l		/* time_rec */
           )
{
	return 0;
}

OM_uint32 KRB5_CALLCONV gss_accept_sec_context
(OM_uint32 *a,		/* minor_status */
            gss_ctx_id_t *b,		/* context_handle */
            gss_cred_id_t c,		/* acceptor_cred_handle */
            gss_buffer_t d,		/* input_token_buffer */
            gss_channel_bindings_t e,	/* input_chan_bindings */
            gss_name_t *f,		/* src_name */
            gss_OID *g,		/* mech_type */
            gss_buffer_t h,		/* output_token */
            OM_uint32 *i,		/* ret_flags */
            OM_uint32 *j,		/* time_rec */
            gss_cred_id_t *k		/* delegated_cred_handle */
           )
{
	return 0;
}


OM_uint32 KRB5_CALLCONV gss_delete_sec_context
(OM_uint32 *a,		/* minor_status */
            gss_ctx_id_t *b,		/* context_handle */
            gss_buffer_t c		/* output_token */
           )
{
	return 0;
}

/* New for V2 */
OM_uint32 KRB5_CALLCONV gss_wrap
(OM_uint32 *a,		/* minor_status */
	    gss_ctx_id_t b,		/* context_handle */
	    int c,			/* conf_req_flag */
	    gss_qop_t d,			/* qop_req */
	    gss_buffer_t e,		/* input_message_buffer */
	    int *f,			/* conf_state */
	    gss_buffer_t g		/* output_message_buffer */
	   )
{
	return 0;
}

/* New for V2 */
OM_uint32 KRB5_CALLCONV gss_unwrap
(OM_uint32 *a,		/* minor_status */
	    gss_ctx_id_t b,		/* context_handle */
	    gss_buffer_t c,		/* input_message_buffer */
	    gss_buffer_t d,		/* output_message_buffer */
	    int *e,			/* conf_state */
	    gss_qop_t *g		/* qop_state */
	   )
{
	return 0;
}

OM_uint32 KRB5_CALLCONV gss_import_name
(OM_uint32 *a,		/* minor_status */
            gss_buffer_t b,		/* input_name_buffer */
            gss_OID c,			/* input_name_type(used to be const) */
            gss_name_t *d		/* output_name */
           )
{
	return 0;
}

OM_uint32 KRB5_CALLCONV gss_release_name
(OM_uint32 *a,		/* minor_status */
            gss_name_t *b		/* input_name */
           )
{
	return 0;
}

OM_uint32 KRB5_CALLCONV gss_release_buffer
(OM_uint32 *a,		/* minor_status */
            gss_buffer_t b		/* buffer */
           )
{
	return 0;
}

OM_uint32 KRB5_CALLCONV gss_release_oid_set
(OM_uint32 *a,		/* minor_status */
            gss_OID_set *b 		/* set */
           )
{
	return 0;
}

OM_uint32 KRB5_CALLCONV gss_inquire_cred
(OM_uint32 *a,		/* minor_status */
            gss_cred_id_t b,		/* cred_handle */
            gss_name_t *c,		/* name */
            OM_uint32 *d,		/* lifetime */
            gss_cred_usage_t *e,	/* cred_usage */
            gss_OID_set *f		/* mechanisms */
           )
{
	return 0;
}

/* Last argument new for V2 */
OM_uint32 KRB5_CALLCONV gss_inquire_context
(OM_uint32 *a,		/* minor_status */
	    gss_ctx_id_t b,		/* context_handle */
	    gss_name_t *c,		/* src_name */
	    gss_name_t *d,		/* targ_name */
	    OM_uint32 *e,		/* lifetime_rec */
	    gss_OID *f,		/* mech_type */
	    OM_uint32 *g,		/* ctx_flags */
	    int *h,           	/* locally_initiated */
	    int *i			/* open */
	   )
{
	return 0;
}

OM_uint32 KRB5_CALLCONV gss_display_status
(OM_uint32 *a,           /* minor_status */
            OM_uint32 b,                 /* status_value */
            int c,                       /* status_type */
            gss_OID d,                   /* mech_type (used to be const) */
            OM_uint32 *e,                /* message_context */
            gss_buffer_t f               /* status_string */
           )
{
    return 0;
}

OM_uint32 KRB5_CALLCONV gss_display_name
(OM_uint32 *a,           /* minor_status */
            gss_name_t b,                 /* input_name */
            gss_buffer_t c,               /* output_name_buffer */
            gss_OID *d           /* output_name_type */
           )
{
    return 0;
}

gss_OID GSS_C_NT_HOSTBASED_SERVICE;

