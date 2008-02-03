LIBS=`pkg-config --libs gswat-0.1`
INCS=`pkg-config --cflags gswat-0.1`

.c.o:
	$(CC) -g -Wall $(CFLAGS) $(INCS) -c $*.c

all: test0


test0: test0.o
	$(CC) -g -Wall $(CFLAGS) -o $@ test0.o $(LIBS)

clean:
	rm -fr *.o test0
