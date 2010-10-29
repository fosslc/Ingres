all : ogrTest

ogrTest : 
	gcc -Wall -g -L. -Wl,-rpath,. ogrTest.c -liiapi.1 -lcompat.1 -lframe.1 -lq.1 -lgdal -o ogrTest

clean:
	rm -f ogrTest
