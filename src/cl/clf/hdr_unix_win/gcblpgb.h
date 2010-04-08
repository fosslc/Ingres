/*
** Copyright (c) 2004 Ingres Corporation
*/
/**
** Name: GCBLPGB.H - BLAST async protocol error codes
**
** Description:
**	This header contains the BLAST 'pgb.h' protocol generator
**	header file, along with Ingres-specific structures for
**	support of the BLAST asynchronous serial I/O interface
**	through the Ingres Protocol Bridge.
**
** History:
**      30-jan-92 (alan)
**          Renamed as per Ingres standards
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	26-Aug-2009 (kschendel) b121804
**	    Bool prototypes to keep gcc 4.3 happy.
**/


/*
 * BLAST 'B' Protocol Generator
 * (c) 1985, Communications Research Group, Inc.
 * Definitions
 */

/* -- pgpnd() return codes -- */
#define PG_TIMO		0		/* timeout */
#define PG_SND		1		/* send done */
#define PG_RD		2		/* recv done */
#define PG_UISND	3		/* unumbered info send */
#define PG_UIRD		4		/* unumbered info read */
#define PG_DISC		5		/* disconnect received */
#define PG_BLOCKED  	6   	    	/* blocked waiting i/o */
#define PG_LOGON	10		/* logon accomplished */
#define PG_CONN         11              /* connect accomplished */
#define PG_ERROR	99		/* protocol error */

/* -- sbksts[] - status values -- */
#define STS_SEND	1		/* needs sending (unsent or known bad) */
#define STS_ACKED	-1		/* acknowledged */
#define STS_UNKNOWN	0 		/* sent, status unknown */
#define STS_EOD		2		/* end of data flag */

/* -- protocol parameters -- */
/* the next three parameters define the maximum size of packet transfers */
#define DBLK_IMAX	4096		/* maximum info bytes per data blk */
#define DBLK_BSZ	(2+DBLK_IMAX+5+2) /* address, id, info, ack, crc */
#define DBLK_PSZ	(1+((4*DBLK_BSZ)/3)+3+1)	/* after packing */

#define DBLK_ISZ	84		/* num of info bytes in a data blk */

#define CBLK_ISZ	1		/* info bytes in a control blk */
#define CBLK_SZ		2		/* total size w/o crc */
#define CBLK_CSZ	4		/* total size w/ crc */

#define ACK_ISZ		5		/* size of ack info */
#define ACK_SZ		7		/* ack with address and id field */
#define ACK_CSZ		9		/* total ack with CRC */

#define SASM_SZ		6		/* size of sasam blk wo/crc */
#define SASM_CSZ 	8		/* size of sasm blk w/crc */
#define SASMA_SZ 	2		/* size of sasma blk wo/crc */
#define SASMA_CSZ 	4		/* size of sasma blk w/crc */

#define SWDWSZ		16		/* send window size */
#define SWDWMK		017		/* mask for valid wdw index */
#define RWDWSZ		16		/* receive window size */
#define RWDWMK		017		/* and a mask of possible values */
#define SEQ_MAX		32		/* max block sequence number */


/*  Packets consist of a flag byte followed by the packet contents which
 *  are encoded on 3/4th's encoding, and a carriage return.
 *
 *  < flag >< packet contents .. 3/4 encoded >< tail >< return >
 *
 *  The 3/4th's encoding guarantees that only the printable ASCII 
 *  character set is used to transmit data.  The FLAG character,
 *  the TAIL character, and the RETURN character must be outside 
 *  this set.  The TAIL character is the only indication of the 
 *  size of the packet. The RETURN character is only present to make
 *  the blocks appear as normal typed input, and to insure a 
 *  minimum line length.
 *
 *  Optionally, if an 8-bit channel is present, the contents of
 *  data blocks are encoded in 7/8th's encoding which yields a
 *  higher throughput.  These blocks are marked with the FLAG8
 *  character.
 */

#define FLAG		';'		/* packet start character */
#define FLAG8		','		/* start char for 8-bit packets */
#define DCE_ADD		03		/* address of DCE */
#define DTE_ADD		01		/* address of DTE */
#define TAIL		'.'		/* end of packet character */
#define RETURN  	015		/* launch character */

/*  Once the packet contents have been decoded from 3/4th's
 *  (or 7/8th's) format then the packet contents can be analyzed.  
 *  The first byte of the packet is the header and describes what 
 *  the packet contains.  If the NUM_INFO (numbered information) 
 *  bit is set, then the packet is a numbered information frame, 
 *  else the packet type can be determined by examining the header 
 *  bits under the BLK_ID mask.
 */ 

#define NUM_INFO	0200	/* if set in header, packet is a numbered */
							/* info packet, else use BLK_ID bits */
#define BLK_ID  	017		/* packet id mask */
#define	SASM		000		/* set asynchronous response mode */
#define	SASMA		001		/* sasm acknowledge */
#define DISC		002		/* disconnect */
#define DISCI		012		/* disconnect intr */
#define ACK			003		/* acknowledge only block */
#define UNUM_INFO	004		/* unnumberred information */

/*  SASM and SASMA may also have bits set to show 
	or acknowledge limitations */

#define BIT7		040		/* 7 bit (3/4th's) operation only */
#define HDX			0100	/* Half duplex only */
#define HDX_T1		16		/* t1 delay in hdx mode */

/*  The DISC packet may also have an interrupt bit set 
	to imply an immediated disconnect */

#define DISC_INTR	010		/* disconnect interrupt */

/*  If the packet is a NUM_INFO packet or an UNUM_INFO packet,
 *  the ACK_PRES bit indicates that an ACK frame is present the 
 *  last ACKSZ bytes of the information packet
 */

#define ACK_PRES	0100	/* ack present in information frame */

/*  In a NUM_INFO packet the sequence number of the packet and the
 *  ACK_REQ bit are also present in the header
 */

#define ACK_REQ		040		/* ack request */
#define SEQ_NUM		037		/* sequence number mask */

/*  In an ACK packet the byte following the header byte is used
 *  to report the received line quality of the receiving station
 *  and the UI_FF bit of the last unnumbered information frame
 *  received
 */

#define UI_FF		040		/* flip/flop bit of last ui packet */
#define LINE_QUAL	037		/* line quality .. # of errors in last */
							/* 20 possible occurences */

#ifdef AandB
#define R5DBSTC		0xf0	/* rev 5 data blk start code */
#define R4DBSTC		0xb7	/* rev 4 ack blk start code */
#endif


/*}
** Name :
**	PGCB - Protocol Generator control block
**
** Description: 
**	This structure is the control block used and managed by the protocol
**      generator interface routines. It is either dynamically or statically
**      allocated by the PG caller, one for each IPC channel.
**
**  History:
**	09-May-90 (cmorris)
**		Initial version
**  	02-Feb-91 (cmorris)
**  	    	Got rid of separate structures for each event
**  	20-Feb-91 (cmorris)
**  	    	Added SIO_CB.
*/

typedef	struct _PG_CB
{
	/*
	** Externally visible data
	*/
	STATUS		status;		/* generic status */
    	CL_ERR_DESC 	syserr;    	/* system status */
	char		*buf;		/* input string */
	i4		len;		/* length of "buf" */
    	bool	    	intr;	    	/* TRUE if interrupt disconnect */

    	/*
    	** Serial I/O Driver control block
    	*/
    	SIO_CB	    	siocb;	    	/* serial i/o control block */
} PG_CB;

/* Function prototypes */

FUNC_EXTERN bool	pgini(void);

/* end of gcblpgb.h */
