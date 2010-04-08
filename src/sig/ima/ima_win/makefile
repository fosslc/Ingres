TARGETEXE   = iomon.exe
CC          = cl
CCFLAGS     = -G4 -Di386=1 -D_X86_ -Od -D_DLL -D_MT /nologo -MD -I.\ -I$(INC)
LINK        = link
LINKFLAGS   = /NOLOGO /DEF:$(DEF) /MACHINE:i386 -entry:mainCRTStartup
INC         = $(II_SYSTEM)\ingres\files
LIBS        = msvcrt.lib kernel32.lib user32.lib advapi32.lib \
              $(II_SYSTEM)\ingres\lib\ingres.lib
OBJS        = iomon.obj
FORMS       = ioform.obj
DEFN        = iomon.sql

!IF "$(DATABASE)" == ""
DATABASE    = imadb
!ENDIF

!MESSAGE DATABASE   : $(DATABASE)

.c.obj:
	$(CC) -c $(CCFLAGS) $*.c

.sc.c:
	esqlc $<

.SUFFIXES: .o  .c  .sc  


all:	register copyform $(TARGETEXE)

register:
	sql $(DATABASE) < iomon.sql

copyform:
	copyform -i $(DATABASE) iomon.form

saveforms:
	copyform $(DATABASE) iomon.form dio_top

iomon.c : iomon.sc

ioform.c:
	compform $(DATABASE) dio_top $@

$(TARGETEXE): $(OBJS) $(FORMS)
    $(LINK) $(LINKFLAGS) /OUT:$@ $** $(LIBS)
