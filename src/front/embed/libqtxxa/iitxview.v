##
##  Copyright (c) 2004 Ingres Corporation
##
##  Name:  iitxview.v
##
##  Description: 	
##	This file contains the TUXEDO view definitions 
##	required to support the INGRES/OpenDTP TUXEDO
##	BRIDGE product. This file must be preprocessed by
##	the TUXEDO command viewc.  It generates two files.
##	One,iitxview.V, is placed in ingres/files 
##	directory, the other,iitxview.h is placed here
##	in the hdr directory.
##
##  Notes:
##	See file iitxgbl.h.  There are two defines in that file
##	that reflect the array sizes of the IITUX_AS_XID_LIST_VIEW
##	and the IITUX_ICAS_SVN_LIST_VIEW.  If you change the array
##	sizes of these views, you must also update the 
##	IITUX_AS_XID_LIST_ARRAY_SIZE and IITUX_ICAS_SVN_LIST_ARRAY_SIZE
##	defines in front!embed!hdr iitxgbl.h.
##
##  History:
##		14-Sep-1993 (mhughes,larrym)
##		    written.
##		25-Nov-1993 (mhughes)
##		    Shorten view names to <16 chars to avoid
##		    runtime errors.
##		18-mar-1994 (larrym)
##		    added field to IITUX_SVN_LIST to mark the 
##		    xid as a 2PC xid.
##
VIEW		IITUX_XID_SEQNO
$	/* View for invoking a transaction demarcation routine */
##
## type	cname		fbname	count	flag	size	null
##
int	xa_call		-	1	-	-	-
long	xa_flags	-	1	-	-	-
long	formatID	-	1	-	-	-
long	gtrid_length	-	1	-	-	-
long	bqual_length	-	1	-	-	-
string	data		-	1	-	128	-
long	rmid		-	1	-	-	-
int	seqnum		-	1	-	-	-
int	tp_flags	-	1	-	-	-
int	xa_ret_value	-	1	-	-	-
END
#
VIEW		IITUX_XID_LIST
$	/* view for refreshing an ICAS's list of XID's after crash */
int	SVN		-	1	-	-	-
int	no_xids		-	1	-	-	-
int	call_again	-	1	-	-	-
long	formatID	-	10	-	-	-
long	gtrid_length	-	10	-	-	-
long	bqual_length	-	10	-	-	-
string	data		-	10	-	128	-
long	rmid		-	10	-	-	-
int	seqnums		-	10	-	-	-
int	tux_ret_value	-	1	-	-	-
END
#
VIEW		IITUX_SVN_XID
$	/* view for registering an SVN with an XID */
long	formatID	-	1	-	-	-
long	gtrid_length	-	1	-	-	-
long	bqual_length	-	1	-	-	-
string	data		-	1	-	128	-
long	rmid		-	1	-	-	-
int	xa_call		-	1	-	-	-
int	SVN		-	1	-	-	-
int	tux_ret_value	-	1	-	-	-
END
#
VIEW		IITUX_REG_AS
$	/* view for registering an SVN with the ICAS */
int	SVN		-	1	-	-	-
int	tux_ret_value	-	1	-	-	-
END
#
VIEW		IITUX_PURGE_XID
$	/* view for requesting deletion of XID from ICAS */
long	formatID	-	1	-	-	-
long	gtrid_length	-	1	-	-	-
long	bqual_length	-	1	-	-	-
string	data		-	1	-	128	-
long	rmid		-	1	-	-	-
int	tux_ret_value	-	1	-	-	-
END
#
VIEW		IITUX_SVN_LIST
$	/* view for getting a list of SVN's involved in an XID */
long	formatID	-	1	-	-	-
long	gtrid_length	-	1	-	-	-
long	bqual_length	-	1	-	-	-
string	data		-	1	-	128	-
long	rmid		-	1	-	-	-
int	mark_2pc	-	1	-	-	-
int	no_SVNs		-	1	-	-	-
int	call_again	-	1	-	-	-
int	SVNs		-	10	-	-	-
int	seqnums		-	10	-	-	-
int	tux_ret_value	-	1	-	-	-
END
