/**
** Name: GCBLSIO.H - Header for BLAST async protocol serial i/o driver
**
** Description:
**	This header contains definitions in the BLAST sio.h header
**	and Ingres-specific structures for the BLAST serial I/O driver.
**
** History:
**      30-jan-92 (alan)
**          Renamed as per Ingres standards
**/

/*
	sio.h - roussel 10-29-86
*/

/*
**	siopn() return codes 
*/
#define	SIO_TIMO	1	/* timeout */
#define SIO_COMRD	2	/* comm port read complete */
#define SIO_COMWR	3	/* comm port write complete */
#define SIO_BLOCKED 	4   	/* blocked waiting for i/o */
#define SIO_ERROR       5   	/* i/o error */

/*
**	comst( ) arguments
*/
/* baud rate */
#define SIO_B300	0
#define SIO_B600	1
#define SIO_B1200	2
#define SIO_B2400	3
#define SIO_B4800	4
#define SIO_B9600	5
#define SIO_B19200	6
#define SIO_B38400	7

/* parity */
#define SIO_PNONE	0
#define SIO_PODD	1
#define SIO_PEVEN	2
#define SIO_PMARK	3
#define SIO_PSPACE	4


/*}
** Name :
**	SIOCB - Serial I/O control block
**
** Description: 
**	This structure is the control block used and managed by the serial
**      I/O interface routines. It is either dynamically or statically
**      allocated by the SIO caller, one for each IPC channel.
**
**  History:
**	20-Feb-91 (cmorris)
**		Initial version
*/

typedef	struct _SIO_CB
{
	/*
	** Externally visible data
	*/
	STATUS		status;		/* generic status */
    	CL_ERR_DESC 	*syserr;    	/* system status */
} SIO_CB;
