/********************************************************************************************************************************/
/* Created 27-SEP-1989 09:30:36 by VAX-11 SDL V3.0-2      Source: 24-JUL-1989 18:26:51 WORK2$:[UCX.V12.BL15.][NET.SRC]INET_USER.S */
/********************************************************************************************************************************/
/*
**      HISTORY:
**      25-sep-95 (whitfield)
**              Turn off member_alignment.
**      05-dec-02 (loera01) SIR 109246
**              Added definition of INET$C_SOCK_NAME.
**      19-jul-2007 (stegr01)
**              No VAX support.
*/

#  pragma member_alignment __save
#  pragma nomember_alignment

 
/*** MODULE $ARPREQDEF ***/
#define ARP$M_IN_USE 1
#define ARP$M_COM 2
#define ARP$M_PERM 4
#define ARP$M_PUBL 8
#define ARP$M_USETRAILERS 16
#define ARP$C_LENGTH 34
#define ARP$K_LENGTH 34
struct ARPREQDEF {
    char ARP$T_PA [16];                 /* IP address                       */
/* $SOCKADDRINDEF defines offsets                                           */
    char ARP$T_HA [16];                 /* Ethernet hardware address        */
/* $SOCKADDRDEF defines offsets                                             */
    union  {                            /*                                  */
        unsigned short int ARP$W_FLAGS; /* flags                            */
        struct  {                       /*                                  */
            unsigned ARP$V_IN_USE : 1;  /* ARP entry is in use              */
            unsigned ARP$V_COM : 1;     /* ARP entry is complete            */
            unsigned ARP$V_PERM : 1;    /* ARP entry is pemanent            */
            unsigned ARP$V_PUBL : 1;    /* ARP entry is public              */
            unsigned ARP$V_USETRAILERS : 1; /* hosts uses trailers          */
            unsigned ARP$V_fill_0 : 3;
            } ARP$R_O_FLAGS;
        } ARP$R_OVLY;
    } ;
 
/*** MODULE $IFREQDEF ***/
#define IFR$M_IFF_UP 1
#define IFR$M_IFF_BROADCAST 2
#define IFR$M_IFF_DEBUG 4
#define IFR$M_IFF_LOOPBACK 8
#define IFR$M_IFF_POINTOPOINT 16
#define IFR$M_IFF_NOTRAILERS 32
#define IFR$M_IFF_RUNNING 64
#define IFR$M_IFF_NOARP 128
#define IFR$M_IFF_PROMISC 256
#define IFR$M_IFF_ALLMULTI 512
#define IFR$M_IFF_DYNPROTO 1024
#define IFR$M_IFF_MOP 2048
#define IFR$M_IFF_NONAME 16384
#define IFR$M_IFF_CLUSTER 32768
#define IFR$C_LENGTH 32
#define IFR$K_LENGTH 32
struct IFREQDEF {
    char IFR$T_NAME [16];               /* device name                      */
    union  {
        char IFR$T_ADDR [16];           /* SOCKADDRIN structure             */
        char IFR$T_DSTADDR [16];        /* SOCKADDRIN structure             */
        char IFR$T_BROADADDR [16];      /* SOCKADDRIN structure             */
        union  {
            unsigned short int IFR$W_FLAGS; /* flags                        */
            struct  {
                unsigned IFR$V_IFF_UP : 1; /* Interface is up               */
                unsigned IFR$V_IFF_BROADCAST : 1; /* Broadcast address valid */
                unsigned IFR$V_IFF_DEBUG : 1; /* Turn on tracing            */
                unsigned IFR$V_IFF_LOOPBACK : 1; /* Interface set to loopback */
                unsigned IFR$V_IFF_POINTOPOINT : 1; /* Interface is point-to-point link */
                unsigned IFR$V_IFF_NOTRAILERS : 1; /* Avoid use of trailers */
                unsigned IFR$V_IFF_RUNNING : 1; /* Resources are allocated  */
                unsigned IFR$V_IFF_NOARP : 1; /* No address resolution protocol */
                unsigned IFR$V_IFF_PROMISC : 1; /* Receive all packets      */
                unsigned IFR$V_IFF_ALLMULTI : 1; /* Receive all multicasting packets */
                unsigned IFR$V_IFF_DYNPROTO : 1; /* Support dynamic proto dispatching */
                unsigned IFR$V_IFF_MOP : 1; /* Device in MOP mode; not in use */
                unsigned IFR$V_IFF_RESERVE : 2; /* SPARE bits               */
                unsigned IFR$V_IFF_NONAME : 1; /* Interface cluster name flag */
                unsigned IFR$V_IFF_CLUSTER : 1; /* Interface is a cluster IFNET */
                } IFR$R_DUMMY_1_BITS;
            int *IFR$L_DATA;            /* pointer to data                  */
            } IFR$R_DUMMY_1_OVRL;
        } IFR$R_DUMMY;
    } ;
 
/*** MODULE $INETERRDEF ***/
#define EPERM 1                         /* Not owner                        */
#define ENOENT 2                        /* No such file or directory        */
#define ESRCH 3                         /* No such process                  */
#define EINTR 4                         /* Interrupted system call          */
#define EIO 5                           /* I/O error                        */
#define ENXIO 6                         /* No such device or address        */
#define E2BIG 7                         /* Arg list too long                */
#define ENOEXEC 8                       /* Exec format error                */
#define EBADF 9                         /* Bad file number                  */
#define ECHILD 10                       /* No children                      */
#define EAGAIN 11                       /* No more processes                */
#define ENOMEM 12                       /* Not enough core                  */
#define EACCES 13                       /* Permission denied                */
#define EFAULT 14                       /* Bad address                      */
#define ENOTBLK 15                      /* Block device required            */
#define EBUSY 16                        /* Mount device busy                */
#define EEXIST 17                       /* File exists                      */
#define EXDEV 18                        /* Cross-device link                */
#define ENODEV 19                       /* No such device                   */
#define ENOTDIR 20                      /* Not a directory                  */
#define EISDIR 21                       /* Is a directory                   */
#define EINVAL 22                       /* Invalid argument                 */
#define ENFILE 23                       /* File table overflow              */
#define EMFILE 24                       /* Too many open files              */
#define ENOTTY 25                       /* Not a typewriter                 */
#define ETXTBSY 26                      /* Text file busy                   */
#define EFBIG 27                        /* File too large                   */
#define ENOSPC 28                       /* No space left on device          */
#define ESPIPE 29                       /* Illegal seek                     */
#define EROFS 30                        /* Read-only file system            */
#define EMLINK 31                       /* Too many links                   */
#define EPIPE 32                        /* Broken pipe                      */
/* math software                                                            */
#define EDOM 33                         /* Argument too large               */
#define ERANGE 34                       /* Result too large                 */
/* non-blocking and interrupt i/o                                           */
#define EWOULDBLOCK 35                  /* Operation would block            */
#define EINPROGRESS 36                  /* Operation now in progress        */
#define EALREADY 37                     /* Operation already in progress    */
/* ipc/network software                                                     */
/* argument errors                                                          */
#define ENOTSOCK 38                     /* Socket operation on non-socket   */
#define EDESTADDRREQ 39                 /* Destination address required     */
#define EMSGSIZE 40                     /* Message too long                 */
#define EPROTOTYPE 41                   /* Protocol wrong type for socket   */
#define ENOPROTOOPT 42                  /* Protocol not available           */
#define EPROTONOSUPPORT 43              /* Protocol not supported           */
#define ESOCKTNOSUPPORT 44              /* Socket type not supported        */
#define EOPNOTSUPP 45                   /* Operation not supported on socket  */
#define EPFNOSUPPORT 46                 /* Protocol family not supported    */
#define EAFNOSUPPORT 47                 /* Address family not supported by protocol family  */
#define EADDRINUSE 48                   /* Address already in use           */
#define EADDRNOTAVAIL 49                /* Can't assign requested address   */
/* operational errors                                                       */
#define ENETDOWN 50                     /* Network is down                  */
#define ENETUNREACH 51                  /* Network is unreachable           */
#define ENETRESET 52                    /* Network dropped connection on reset  */
#define ECONNABORTED 53                 /* Software caused connection abort  */
#define ECONNRESET 54                   /* Connection reset by peer         */
#define ENOBUFS 55                      /* No buffer space available        */
#define EISCONN 56                      /* Socket is already connected      */
#define ENOTCONN 57                     /* Socket is not connected          */
#define ESHUTDOWN 58                    /* Can't send after socket shutdown  */
#define ETOOMANYREFS 59                 /* Too many references: can't splice  */
#define ETIMEDOUT 60                    /* Connection timed out             */
#define ECONNREFUSED 61                 /* Connection refused               */
#define ELOOP 62                        /* Too many levels of symbolic links  */
#define ENAMETOOLONG 63                 /* File name too long               */
/* should be rearranged                                                     */
#define EHOSTDOWN 64                    /* Host is down                     */
#define EHOSTUNREACH 65                 /* No route to host                 */
#define ENOTEMPTY 66                    /* Directory not empty              */
/* quotas & mush                                                            */
#define EPROCLIM 67                     /* Too many processes               */
#define EUSERS 68                       /* Too many users                   */
#define EDQUOT 69                       /* Disc quota exceeded              */
/* IPC errors                                                               */
#define ENOMSG 70                       /* No message of desired type       */
#define EIDRM 71                        /* Identifier removed               */
/* Alignment error of some type (i.e., cluster, page, block ...)            */
#define EALIGN 72                       /* alignment error                  */
 
/*** MODULE $INETSYMDEF ***/
#define INET$C_ICMP 1
#define INET$C_RAW_IP 255
#define INET$C_TCP 6
#define INET$C_UDP 17
#define IPPROTO$C_ICMP 1
#define IPPROTO$C_RAW_IP 255
#define IPPROTO$C_TCP 6
#define IPPROTO$C_UDP 17
/*                                                                          */
#define UCX$C_ICMP 1
#define UCX$C_RAW_IP 255
#define UCX$C_TCP 6
#define UCX$C_UDP 17
/*                                                                          */
/* Ports < IP_PROTO$C_RESERVED are reserved for                             */
/* privileged processes (e.g. root).                                        */
/*                                                                          */
#define IP_PROTO$C_RESERVED 1024
#define INET_PROTYP$C_STREAM 1          /* stream type                      */
#define INET_PROTYP$C_DGRAM 2           /* datagram type                    */
#define INET_PROTYP$C_RAW 3             /* raw type                         */
/*                                                                          */
#define UCX$C_STREAM 1
#define UCX$C_DGRAM 2
#define UCX$C_RAW 3
#define INET$C_IPOPT 0                  /* IP opt type parameter            */
#define INET$C_SOCKOPT 1                /* setsockopt type parameter        */
#define INET$C_IOCTL 2                  /* ioctl type parameter             */
#define INET$C_DATA 3                   /* data                             */
#define INET$C_SOCK_NAME 4              /* socket name type parameter       */
#define INET$C_RESERVE_1 4
#define INET$C_RESERVE_2 5
#define INET$C_TCPOPT 6                 /* TCP option type                  */
/*                                                                          */
#define UCX$C_IPOPT 0
#define UCX$C_SOCKOPT 1
#define UCX$C_TCPOPT 6
#define UCX$C_IOCTL 2
#define UCX$C_DATA 3
#define INET$C_DSC_RCV 0                /* discard received messages        */
#define INET$C_DSC_SND 1                /* discard sent messages            */
#define INET$C_DSC_ALL 2                /* discard all messages             */
#define UCX$C_DSC_RCV 0
#define UCX$C_DSC_SND 1
#define UCX$C_DSC_ALL 2
#define INET$C_TCPOPT_EOL 0
#define INET$C_TCPOPT_NOP 1
#define INET$C_TCPOPT_MAXSEG 2
#define INET$C_TCP_NODELAY 1            /* don't delay send to coalesce packets  */
#define INET$C_TCP_MAXSEG 2             /* set maximum segment size         */
#define UCX$C_TCPOPT_EOL 0
#define UCX$C_TCPOPT_NOP 1
#define UCX$C_TCPOPT_MAXSEG 2
#define UCX$C_TCP_NODELAY 1
#define UCX$C_TCP_MAXSEG 2
#define INET$C_AF_UNSPEC 0              /* unspecified                      */
#define INET$C_AF_UNIX 1                /* local to host (pipes, portals)   */
#define INET$C_AF_INET 2                /* internetwork: UDP, TCP, etc.     */
#define INET$C_AF_MAX 3                 /* maximum value                    */
#define INET$C_INADDR_ANY 0
#define INET$C_INADDR_BROADCAST -1
/*                                                                          */
#define UCX$C_AF_UNSPEC 0
#define UCX$C_AF_UNIX 1
#define UCX$C_AF_INET 2
#define UCX$C_AF_MAX 3
#define UCX$C_INADDR_ANY 0
#define UCX$C_INADDR_BROADCAST -1
/*                                                                          */
#define INET$C_MSG_OOB 1                /* process out-of-band data         */
#define INET$C_MSG_PEEK 2               /* peek at incoming message         */
#define INET$C_MSG_DONTROUTE 4          /* send without                     */
/* using routing tables                                                     */
#define INET$C_MSG_MAXIOVLEN 16
/*                                                                          */
#define UCX$C_MSG_OOB 1                 /* process out-of-band data         */
#define UCX$C_MSG_PEEK 2                /* peek at incoming message         */
#define UCX$C_MSG_DONTROUTE 4           /* send without                     */
/* using routing tables                                                     */
#define UCX$C_MSG_MAXIOVLEN 16
#define INET$M_DEBUGGING 1
#define INET$M_ACCEPTCONN 2
#define INET$M_REUSEADDR 4
#define INET$M_KEEPALIVE 8
#define INET$M_DONTROUTE 16
#define INET$M_BROADCAST 32
#define INET$M_USELOOPBACK 64
#define INET$M_LINGER 128
#define INET$M_OOBINLINE 256
#define INET$C_SNDBUF 4097              /* send buffer size                 */
#define INET$C_RCVBUF 4098              /* receive buffer size              */
#define INET$C_SNDLOWAT 4099            /* send low-water mark              */
#define INET$C_RCVLOWAT 4100            /* receive low-water mark           */
#define INET$C_SNDTIMEO 4101            /* send timeout                     */
#define INET$C_RCVTIMEO 4102            /* receive timeout                  */
#define INET$C_ERROR 4103               /* get error status and clear       */
#define INET$C_TYPE 4104                /* get socket type                  */
struct SOCKOPT_INET_DEF {
    union  {                            /*                                  */
        unsigned short int INET$W_OPTIONS; /* Socket options, see socket.h  */
        struct  {
/*                                                                          */
/* Socket options bits.                                                     */
/*                                                                          */
            unsigned INET$V_DEBUGGING : 1; /* turn on event logging, not used */
            unsigned INET$V_ACCEPTCONN : 1; /* socket has had LISTEN        */
            unsigned INET$V_REUSEADDR : 1; /* allow local address reuse     */
            unsigned INET$V_KEEPALIVE : 1; /* keep connection alive         */
            unsigned INET$V_DONTROUTE : 1; /* use only the interface addr   */
            unsigned INET$V_BROADCAST : 1; /* allow broadcasting            */
            unsigned INET$V_USELOOPBACK : 1; /* loopback interface, not used */
            unsigned INET$V_LINGER : 1; /* linger at close                  */
            unsigned INET$V_OOBINLINE : 1; /* leave received OOB data in line  */
/*                                                                          */
/* Additional options, not kept in so_options.                              */
/*                                                                          */
            unsigned INET$V_fill_1 : 7;
            } INET$R_OPT_BITS;
        } INET$R_OPT_OVRLY;
    } ;
#define UCX$M_DEBUGGING 1
#define UCX$M_ACCEPTCONN 2
#define UCX$M_REUSEADDR 4
#define UCX$M_KEEPALIVE 8
#define UCX$M_DONTROUTE 16
#define UCX$M_BROADCAST 32
#define UCX$M_USELOOPBACK 64
#define UCX$M_LINGER 128
#define UCX$M_OOBINLINE 256
#define UCX$C_SNDBUF 4097               /* send buffer size                 */
#define UCX$C_RCVBUF 4098               /* receive buffer size              */
#define UCX$C_SNDLOWAT 4099             /* send low-water mark              */
#define UCX$C_RCVLOWAT 4100             /* receive low-water mark           */
#define UCX$C_SNDTIMEO 4101             /* send timeout                     */
#define UCX$C_RCVTIMEO 4102             /* receive timeout                  */
#define UCX$C_ERROR 4103                /* get error status and clear       */
#define UCX$C_TYPE 4104                 /* get socket type                  */
struct SOCKOPTDEF {
    union  {                            /*                                  */
        unsigned short int UCX$W_OPTIONS; /* Socket options, see socket.h   */
        struct  {
/*                                                                          */
/* Socket options bits.                                                     */
/*                                                                          */
            unsigned UCX$V_DEBUGGING : 1; /* turn on event logging, not used */
            unsigned UCX$V_ACCEPTCONN : 1; /* socket has had LISTEN         */
            unsigned UCX$V_REUSEADDR : 1; /* allow local address reuse      */
            unsigned UCX$V_KEEPALIVE : 1; /* keep connection alive          */
            unsigned UCX$V_DONTROUTE : 1; /* use only the interface addr    */
            unsigned UCX$V_BROADCAST : 1; /* allow broadcasting             */
            unsigned UCX$V_USELOOPBACK : 1; /* loopback interface, not used */
            unsigned UCX$V_LINGER : 1;  /* linger at close                  */
            unsigned UCX$V_OOBINLINE : 1; /* leave received OOB data in line  */
/*                                                                          */
/* Additional options, not kept in so_options.                              */
/*                                                                          */
            unsigned UCX$V_fill_2 : 7;
            } UCX$R_OPT_BITS;
        } UCX$R_OPT_OVRLY;
    } ;
 
/*** MODULE $OPTDEF ***/
#define OPT$C_SET_LENGTH 8
#define OPT$K_SET_LENGTH 8
#define OPT$C_GET_LENGTH 12
#define OPT$K_GET_LENGTH 12
struct OPTDEF {
    unsigned short int OPT$W_LENGTH;    /* length                           */
    unsigned short int OPT$W_NAME;      /* name                             */
    int *OPT$L_ADDRESS;                 /* address                          */
    int *OPT$L_RET_LENGTH;              /* address                          */
    } ;
 
/*** MODULE $RTENTRYDEF ***/
/*                                                                          */
/* We distinguish between routes to hosts and routes to networks,           */
/* preferring the former if available.  For each route we infer             */
/* the interface to use from the gateway address supplied when              */
/* the route was entered.  Routes that forward packets through              */
/* gateways are marked so that the output routines know to address the      */
/* gateway rather than the ultimate destination.                            */
/*                                                                          */
#define RT$M_RTF_UP 1
#define RT$M_RTF_GATEWAY 2
#define RT$M_RTF_HOST 4
#define RT$M_RTF_DYNAMIC 8
#define RT$M_RTF_MODIFIED 16
#define RT$C_LENGTH 52
#define RT$K_LENGTH 52
struct RTENTRYDEF {
    unsigned long int RT$L_HASH;        /* Hash link                        */
    union  {
        struct  {
            unsigned short int RT$W_DST_SIN_FAMILY; /* Address type         */
            unsigned short int RT$W_DST_SIN_PORT; /* Port number            */
            unsigned long int RT$L_DST_SIN_ADDR; /* Internet address        */
            char RT$T_DST_SIN_ZERO [8]; /* Unused space                     */
            } RT$R_DST_FIELDS;
        char RT$T_DST [16];             /* Destination SOCKADDR structure   */
        } RT$R_DST_OVRLY;
    union  {
        struct  {
            unsigned short int RT$W_GATEWAY_SIN_FAMILY; /* Address type     */
            unsigned short int RT$W_GATEWAY_SIN_PORT; /* Port number        */
            unsigned long int RT$L_GATEWAY_SIN_ADDR; /* Internet address    */
            char RT$T_GATEWAY_SIN_ZERO [8]; /* Unused space                 */
            } RT$R_GATEWAY_FIELDS;
        char RT$T_GATEWAY [16];         /* Gateway SOCKADDR structure       */
        } RT$R_GATEWAY_OVRLY;
    union  {
        unsigned short int RT$W_FLAGS;  /* up/down?, host/net               */
        struct  {
            unsigned RT$V_RTF_UP : 1;   /* route useable                    */
            unsigned RT$V_RTF_GATEWAY : 1; /* destination is a gateway      */
            unsigned RT$V_RTF_HOST : 1; /* host entry (net otherwise)       */
            unsigned RT$V_RTF_DYNAMIC : 1; /* created dynamically (by redirect) */
            unsigned RT$V_RTF_MODIFIED : 1; /* changed by redirect          */
            unsigned RT$V_fill_3 : 3;
            } RT$R_FLAGS_BITS;
        } RT$R_FLAGS_OVRLY;
    unsigned short int RT$W_REFCNT;     /* # held references                */
    unsigned long int RT$L_USE;         /* raw # packets forwarded          */
    unsigned long int RT$L_IFP;         /* pointer to the IFNET interface to use */
    unsigned long int RT$L_NEXT;        /* pointer to the next RTENTRY      */
    } ;
 
/*** MODULE $SIOCDEF ***/
#define FIONREAD -2147195265            /* Get # bytes to read              */
#define FIONBIO -2147195266             /* non block I/O                    */
#define FIOASYNC -2147195267            /* asynch I/O                       */
#define SIOCSHIWAT -2147192064          /* high water mark                  */
#define SIOCGHIWAT 1074033409           /* high water mark                  */
#define SIOCSLOWAT -2147192062          /* low water mark                   */
#define SIOCGLOWAT 1074033411           /* low water mark                   */
#define SIOCATMARK 1074033415           /* at OOB mark                      */
#define SIOCSPGRP -2147192056           /* Process group                    */
#define SIOCGPGRP 1074033417            /* Process group                    */
#define SIOCADDRT -2144046582           /* add RT                           */
#define SIOCDELRT -2144046581           /* delete RT                        */
#define SIOCGETRT -1070304725           /* get RT                           */
#define SIOCSIFADDR -2145359604         /* set IF address                   */
#define SIOCGIFADDR -1071617779         /* Get IF address                   */
#define SIOCSIFDSTADDR -2145359602      /* Destination addr                 */
#define SIOCGIFDSTADDR -1071617777      /* BDestination addr                */
#define SIOCSIFFLAGS -2145359600        /* IF flags                         */
#define SIOCGIFFLAGS -1071617775        /* IF flags                         */
#define SIOCGIFBRDADDR -1071617774      /* Broadcast addr                   */
#define SIOCSIFBRDADDR -2145359597      /* Broadcats addr                   */
#define SIOCGIFCONF -1073190636         /* IF configuration                 */
#define SIOCGIFNETMASK -1071617771      /* Network mask                     */
#define SIOCSIFNETMASK -2145359594      /* Network mask                     */
#define SIOCSARP -2145097442            /* set ARP                          */
#define SIOCGARP -1071355617            /* get ARP                          */
#define SIOCDARP -2145097440            /* delete ARP                       */
#define SIOCARPREQ -2145097432          /* ARP request                      */
#define SIOCENABLBACK -2145359583       /* enable loopback                  */
#define SIOCDISABLBACK -2145359582      /* disable loopback                 */
#define SIOCSTATE -1072273117           /* state                            */
 
/*** MODULE $SOCKETOPTDEF ***/
/*                                                                          */
/* Socket options data structure.                                           */
/*                                                                          */
#define SOCKOPT$M_DEBUG 1
#define SOCKOPT$M_ACCEPTCONN 2
#define SOCKOPT$M_REUSEADDR 4
#define SOCKOPT$M_KEEPALIVE 8
#define SOCKOPT$M_DONTROUTE 16
#define SOCKOPT$M_BROADCAST 32
#define SOCKOPT$M_USELOOPBACK 64
#define SOCKOPT$M_LINGER 128
#define SOCKOPT$M_OOBINLINE 256
#define SOCKOPT$C_SNDBUF 4097           /* send buffer size                 */
#define SOCKOPT$C_RCVBUF 4098           /* receive buffer size              */
#define SOCKOPT$C_SNDLOWAT 4099         /* send low-water mark              */
#define SOCKOPT$C_RCVLOWAT 4100         /* receive low-water mark           */
#define SOCKOPT$C_SNDTIMEO 4101         /* send timeout                     */
#define SOCKOPT$C_RCVTIMEO 4102         /* receive timeout                  */
#define SOCKOPT$C_ERROR 4103            /* get error status and clear       */
#define SOCKOPT$C_TYPE 4104             /* get socket type                  */
#define SOCKOPT$C_LENGTH 2
#define SOCKOPT$K_LENGTH 2
struct SOCKETOPTDEF {
    union  {                            /*                                  */
        unsigned short int SOCKOPT$W_OPTIONS; /* Socket options, see socket.h  */
        struct  {
/*                                                                          */
/* Socket options bits.                                                     */
/*                                                                          */
            unsigned SOCKOPT$V_DEBUG : 1; /* turn on event logging, not used */
            unsigned SOCKOPT$V_ACCEPTCONN : 1; /* socket has had LISTEN     */
            unsigned SOCKOPT$V_REUSEADDR : 1; /* allow local address reuse  */
            unsigned SOCKOPT$V_KEEPALIVE : 1; /* keep connection alive      */
            unsigned SOCKOPT$V_DONTROUTE : 1; /* use only the interface addr */
            unsigned SOCKOPT$V_BROADCAST : 1; /* allow broadcasting         */
            unsigned SOCKOPT$V_USELOOPBACK : 1; /* loopback interface, not used */
            unsigned SOCKOPT$V_LINGER : 1; /* linger at close               */
            unsigned SOCKOPT$V_OOBINLINE : 1; /* leave received OOB data in line  */
/*                                                                          */
/* Additional options, not kept in so_options.                              */
/*                                                                          */
            unsigned SOCKOPT$V_fill_4 : 7;
            } SOCKOPT$R_OPT_BITS;
        } SOCKOPT$R_OPT_OVRLY;
    } ;
 
/*** MODULE $SOCKADDRDEF ***/
#define AF_UNSPEC 0                     /* unspecified socket family        */
#define AF_INET 2                       /* INET socket family               */
#define SA$C_LENGTH 16
#define SA$K_LENGTH 16
struct SOCKADDR {
    unsigned short int SA$W_FAMILY;     /* address family                   */
    char SA$T_DATA [14];                /* up to 14 bytes of address        */
    } ;
 
/*** MODULE $SOCKADDRINDEF ***/
#define SIN$C_LENGTH 16
#define SIN$K_LENGTH 16
struct SOCKADDRIN {
    unsigned short int SIN$W_FAMILY;    /* address family                   */
    unsigned short int SIN$W_PORT;      /* 2 bytes specifying a port        */
    unsigned long int SIN$L_ADDR;       /* 4 bytes specifying an IP address */
    char SIN$T_ZERO [8];                /* 8 bytes                          */
    } ;
 
/*** MODULE $INETACPSYMDEF ***/
/*+                                                                         */
/* Define ACP HOST/NET data base subroutine calls subfunction codes         */
/*                                                                          */
/*-                                                                         */
#define INETACP$C_ALIASES 1             /* aliases                          */
#define INETACP$C_TRANS 2               /* translate ASCII string in binary */
 
/*** MODULE $INETACPFSYMDEF ***/
/*+                                                                         */
/* Define ACP control subfunction codes                                     */
/*                                                                          */
/*-                                                                         */
#define INETACP_FUNC$C_GETHOSTBYNAME 1  /* Subroutine call of GET_HOST_BY_NAME */
#define INETACP_FUNC$C_GETHOSTBYADDR 2  /* Subroutine call of GET_HOST_BY_ADDR */
#define INETACP_FUNC$C_GETNETBYNAME 3   /* Subroutine call of GET_NET_BY_NAME */
#define INETACP_FUNC$C_GETNETBYADDR 4   /* Subroutine call of GET_NET_BY_ADDR  */

#  pragma member_alignment __restore

