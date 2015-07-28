CC = gcc
CFLAGS = 
#LIBS = -lsocket -lnsl

PROGS = asincrono cliente

all: ${PROGS}

transporte:
	${CC} ${CFLAGS} -c transporte.c ${LIBS}

asincrono: transporte.o
	${CC} ${CFLAGS} -o $@ asincrono.c transporte.o ${LIBS}

cliente: transporte.o
	${CC} ${CFLAGS} -o $@ cliente.c transporte.o ${LIBS}


clean:
	rm *.o

	

