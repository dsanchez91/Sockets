CC = gcc
CFLAGS = 
#LIBS = -lsocket -lnsl

PROGS = sincrono cliente

all: ${PROGS}

transporte:
	${CC} ${CFLAGS} -c transporte.c ${LIBS}

sincrono: transporte.o
	${CC} ${CFLAGS} -o $@ sincrono.c transporte.o ${LIBS}

cliente: transporte.o
	${CC} ${CFLAGS} -o $@ cliente.c transporte.o ${LIBS}


clean:
	rm *.o

	

