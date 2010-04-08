/*
** a.800.h
**
** History:
**	17-dec-91 (francel/kirke/terjeb)
**		Created this file to add information missing from system
**		header files.
*/


/* N_BADTYPE, N_BADMACH, and N_BADMAG are defined here as a temporary
   fix until HP puts them into the a.out.h supplied with the OS. This
   will hopefully happen in HP-UX 8.0. Note, however, that the 800
   series uses the "header" struct (see filehdr.h) rather than the
   "exec" struct.  --Ack  9/6/90   */

/* 17-Jan-92: At this time this bug has not been fixed in HP-UX 8.0
*/

#ifdef hp8_us5
#define N_BADTYPE(x) (((x).a_magic)!=EXEC_MAGIC&&\
        ((x).a_magic)!=SHARE_MAGIC&&\
        ((x).a_magic)!=DEMAND_MAGIC&&\
        ((x).a_magic)!=RELOC_MAGIC)

#define N_BADMACH(x) (((x).system_id)!=HP9000S800_ID)

#define N_BADMAG(x)  (N_BADTYPE(x) || N_BADMACH(x))
#endif /* hp8_us5 */
