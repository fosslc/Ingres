## Copyright (c) 2004 Ingres Corporation 
## Makefile for plays extension
#
# Name: makefile
#
# Description:
#       Build the extension server function shared object for the
#       play tutorial and copy it the the directory:
#           $II_SYSTEM/ingres/files/dynamic
#
#       To enable debugging the environment variable PLAY_DEBUG to
#       the value of your compiler debug switch.
#
## History
##    12-Mar-1999 (peeje01)
##        Created
##

CC_CMD=cc
CFLAGS=-c -G -K pic $(PLAY_DEBUG)
SH_LINK=ld -G -i 
SH_LINK_OPTS=-lm -lc 

all: play_NewOrder.1.so

play_NewOrder.1.so: play_NewOrder.o
	$(SH_LINK) $(LFLAGS) -o $@ play_NewOrder.o $(SH_LINK_OPTS) \
	$(II_SYSTEM)/ingres/lib/libingres.a

play_NewOrder.o: play_NewOrder.c play_NewOrder.h
	$(CC_CMD) $(CFLAGS) play_NewOrder.c

play_NewOrder.c: play_NewOrder.sc
	esqlc play_NewOrder.sc

install: play_NewOrder.1.so
	rm $(II_SYSTEM)/ingres/files/dynamic/libplay_NewOrder.so.2.0
	cp play_NewOrder.1.so $(II_SYSTEM)/ingres/files/dynamic/libplay_NewOrder.so.2.0
