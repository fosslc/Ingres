/*
**    Copyright (c) 2008 Ingres Corporation
**
*/

/*
** Name: queuemgmt.h - VMS specific queue management stuff.
**
**
** History:
**  18-Jun-2007 (stegr01)
*/

#ifndef QUEUE_MGMT_H_INCLUDED
#define QUEUE_MGMT_H_INCLUDED

#include <compat.h>

#include <builtins.h>

typedef struct _srelqhdr
{
  u_i4      hdr[2];
} SRELQHDR, *LPSRELQHDR;

typedef struct _srelqentry
{
  u_i4        entry[2];
} SRELQENTRY, *LPSRELQENTRY;

typedef struct _absqhdr
{
  struct   _absqhdr* flink;
  struct   _absqhdr* blink;
} ABSQHDR, *LPABSQHDR;

typedef struct _absqentry
{
    struct _absqentry* flink;
    struct _absqentry* blink;
} ABSQENTRY, *LPABSQENTRY;



/* longword queue instructions */

#define INSQHI __PAL_INSQHIL
#define INSQTI __PAL_INSQTIL
#define INSQUE __PAL_INSQUEL_D
#define REMQHI __PAL_REMQHIL
#define REMQTI __PAL_REMQTIL
#define REMQUE __PAL_REMQUEL_D

/*
** ins?i returns the following possible sts values ...
**
** -1 the entry was not inserted because the secondary interlock failed
**  0 the entry was inserted but it was not the only entry in the list
**  1 the entry was inserted and it was the only entry in the list
*/

#define INSQI_STS_INTERLOCK -1
#define INSQI_STS_MORE       0
#define INSQI_STS_NOMORE     1


/*
** remq?i returns the following statuses
**
** -1: the entry cannot be removed because the secondary interlock failed
**  0: the queue was empty
**  1: the entry was removed and the queue has remaining entries
**  2: the entry was removed and the queue is now empty
*/

#define REMQI_STS_INTERLOCK -1
#define REMQI_STS_EMPTY      0
#define REMQI_STS_MORE       1
#define REMQI_STS_NOMORE     2


/*
** INSQUE returns the following statuses
**
** 0 if the entry was not the only entry in the queue
** 1 if the entry was the only entry in the queue
*/

#define INSQUE_STS_MORE      0
#define INSQUE_STS_NOMORE    1

/*
** REMQUE returns the following statuses
**
**  -1 if the queue was empty
**   0 if the entry was removed and the queue is now empty
**   1 if the entry was removed and the queue has remaining entries
**/

#define REMQUE_STS_EMPTY    -1
#define REMQUE_STS_NOMORE    0
#define REMQUE_STS_MORE      1

typedef i4 QUEUE_STS;


#endif /* QUEUE_MGMT_H_INCLUDED */
