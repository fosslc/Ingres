all : ogrTest ingresTest

ogrTest : 
	gcc -Wall -g -L. -L $(II_SYSTEM)/ingres/lib -Wl,-rpath,. ogrTest.c -liiapi.1 -lcompat.1 -lframe.1 -lq.1 -lgdal -o ogrTest
ingresTest : 
	gcc -Wall -g -L. -L $(II_SYSTEM)/ingres/lib -I $(II_SYSTEM)/ingres/files ingresTest.c -liiapi.1 -lcompat.1 -lframe.1 -lq.1 -lgdal -o ingresTest
clean:
	rm -f ogrTest
