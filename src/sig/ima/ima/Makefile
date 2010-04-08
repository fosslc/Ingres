.c.o:
	cc -c $(CFLAGS) $*.c

.sc.c:
	esqlc $<

.SUFFIXES: .o  .c  .sc  


DATABASE=   imadb

OBJS=	iomon.o

FORMS=	ioform.o

HDRS=	

LIBS = -lcurses -lgen -lm -lnsl -lsocket -lelf

DEFN=	iomon.sql

iomon: $(OBJS) $(FORMS) Makefile
	cc $(OBJS) $(FORMS) \
	$(II_SYSTEM)/ingres/lib/libingres.a $(LIBS) -o iomon

all:	register copyform compform iomon

compform:
	compform $(DATABASE) dio_top ioform.c

register:
	sql $(DATABASE) < iomon.sql

copyform:
	copyform -i $(DATABASE) iomon.form

saveforms:
	copyform $(DATABASE) iomon.form dio_top

iomon.c : iomon.sc
