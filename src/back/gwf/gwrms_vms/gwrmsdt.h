/*
** Copyright (c) 1990, Ingres Corporation
** All Rights Reserved
*/

/**
** Name: GWRMSDT.H  - Header for datatype definitions of RMS gateway server
**
** Description:
**	This header includes definitions of all RMS datatypes used by the
**	datatype module of the RMS gateway server.
**
** History:
**	29-jan-90 (linda)
**	    Created.
**      07-jun-92 (schang)
**          add RMSGW_IMAGINE
**/

/*
[@type_reference@]...
[@forward_function_references@]
*/


/*
**  Defines of other constants.
*/

/*
**  This section defines all of the VAX-specific datatypes which are supported
**  by the RMS Gateway.  The complete list of type names and modifiers is:
**
**	byte
**	word
**	longword
**	quadword
**	octaword
**	    (the preceeding five can be signed or unsigned)
**	d_floating
**	f_floating
**	    (above are identical to INGRES float types)--but they get their own
**	    RMS datatypes to support coercions that Ingres won't perform
**	g_floating
**	h_floating
**	    (VAX extended range float types)
**	fixed string
**	varying string
**	    (above may be modified by 'blankpad' (default) or 'nullpad').
**	vax date
**	packed decimal
**	leading numeric string
**	zoned numeric string
**	unsigned numeric string
**	overpunched numeric string
**	trailing separate numeric string
**
**  History:
**	21-mar-90 (jrb)
**	    Combined G-float and H-float into a single type.
**	14-jun-90 (jrb)
**	    Added RMSGW_BL_FIXSTR as an RMS type.
**	21-jun-90 (edwin, linda)
**	    Added RMSGW_NATFLOAT, same as Ingres float types and d_floating,
**	    f_floating RMS float types.  Used to support coercions that Ingres
**	    won't allow even though the conversion functions exist.
**	26-jun-90 (linda)
**	    Added back RMSGW_BL_VARSTR and RMSGW_NL_VARSTR types, because even
**	    though they are exactly the same as the fixed types for purposes of
**	    conversion, we need to remember what the user specified when doing
**	    output to a varying-record-length file.
**	11-sep-90 (linda)
**	    Added trailing separate numeric string datatype, RMSGW_TRL_NUM.
**	9-nov-91 (rickh)
**	    Added RMS_MAX_FLOAT, the maximum number that will fit into
**	    an INGRES f8.
**      17-jun-93 (schang)
**          change RMS_MAX_FLOAT from 1e+38 to 1.7e+38
**      26-jul-94 (schang)
**          Intel likes to push things to its limit, thus we have to give
**          exact maximum value for float8 as
**          #define	    RMS_MAX_FLOAT	( 1.70141183460469229e+38 )
**      26-feb-97 (schang)
**          add f-,d-,h-,g-,s-,t- floating data type, retire natfloat and
**          extfloat, but not remove them.
*/

#define	    RMSGW_INT		((DB_DT_ID) ADD_HIGH_USER)	/* 1,2,4,8 & 16
								   byte ints */
#define	    RMSGW_UNSINT	((DB_DT_ID) ADD_HIGH_USER - 1)	/* unsigned
								   integers of
								   any length */
/* feb-26-97 (schang) retire NATFLOAT AND EXTFLOAT but not remove them */
#define	    RMSGW_NATFLOAT	((DB_DT_ID) ADD_HIGH_USER - 2)	/* "natural"
								   floating
								   types */
#define	    RMSGW_EXTFLOAT	((DB_DT_ID) ADD_HIGH_USER - 3)	/* extended
								   floating
								   types */
#define	    RMSGW_BL_FIXSTR	((DB_DT_ID) ADD_HIGH_USER - 4)	/* blankpad
								   fixed
								   string */
#define	    RMSGW_NL_FIXSTR	((DB_DT_ID) ADD_HIGH_USER - 5)	/* nullpad
								   fixed
								   string */
#define	    RMSGW_BL_VARSTR	((DB_DT_ID) ADD_HIGH_USER - 6)	/* blankpad
								   varying
								   string */
#define	    RMSGW_NL_VARSTR	((DB_DT_ID) ADD_HIGH_USER - 7)	/* nullpad
								   varying
								   string */
#define	    RMSGW_VAXDATE	((DB_DT_ID) ADD_HIGH_USER - 8)	/* vax date
								   type */
#define	    RMSGW_PACKED_DEC	((DB_DT_ID) ADD_HIGH_USER - 9)	/* packed
								   decimal */
#define	    RMSGW_LEADING_NUM	((DB_DT_ID) ADD_HIGH_USER - 10)	/* leading
								   numeric
								   string */
#define	    RMSGW_ZONED_NUM	((DB_DT_ID) ADD_HIGH_USER - 11) /* zoned
								   numeric
								   string */
#define	    RMSGW_UNS_NUM	((DB_DT_ID) ADD_HIGH_USER - 12) /* unsigned
								   numeric
								   string */
#define	    RMSGW_OVR_NUM	((DB_DT_ID) ADD_HIGH_USER - 13) /* overpunched
								   numeric */
#define	    RMSGW_TRL_NUM	((DB_DT_ID) ADD_HIGH_USER - 14)	/* trailing
								   separate
								   numeric */
#define	    RMSGW_IMAGINE	((DB_DT_ID) ADD_HIGH_USER - 15)	/* repeating
								  group phantom
                                                                   type */
#define     RMSGW_FFLOAT	((DB_DT_ID) ADD_HIGH_USER - 16)	/* f_floating
								   types */
#define     RMSGW_DFLOAT	((DB_DT_ID) ADD_HIGH_USER - 17)	/* d_floating
								   types */
#define     RMSGW_GFLOAT	((DB_DT_ID) ADD_HIGH_USER - 18)	/* g_floating
								   types */
#define     RMSGW_HFLOAT	((DB_DT_ID) ADD_HIGH_USER - 19)	/* h_floating
								   types */
#define     RMSGW_SFLOAT	((DB_DT_ID) ADD_HIGH_USER - 20)	/* s_floating
								   types */
#define     RMSGW_TFLOAT	((DB_DT_ID) ADD_HIGH_USER - 21)	/* t_floating
								   types */
#if 0
#define     RMSGW_LVCHR		((DB_DT_ID) ADD_HIGH_USER - 16)	/* longvarchar
								   type */
#define     RMSGW_LBYTE		((DB_DT_ID) ADD_HIGH_USER - 17)	/* longbyte
								   type */
#endif


#define	    RMS_MAX_FLOAT	( 1.7014118346046923e+38 )
