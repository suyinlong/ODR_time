CC = gcc

UNP_DIR = /users/cse533/Stevens/unpv13e

LIBS = -lpthread ${UNP_DIR}/libunp.a

FLAGS = -g -O2

CFLAGS = ${FLAGS} -I${UNP_DIR}/lib

all: ODR_yinlsu server_yinlsu client_yinlsu

ODR_yinlsu: odr.o
	${CC} ${CFLAGS} -o ODR_yinlsu odr.o ${LIBS}

odr.o: odr.c
	${CC} ${CFLAGS} -c odr.c

server_yinlsu: server.o get_hw_addrs.o
	${CC} ${CFLAGS} -o server_yinlsu server.o get_hw_addrs.o ${LIBS}

server.o: server.c
	${CC} ${CFLAGS} -c server.c

client_yinlsu: client.o
	${CC} ${CFLAGS} -o client_yinlsu client.o ${LIBS}

client.o: client.c
	${CC} ${CFLAGS} -c client.c

get_hw_addrs.o: get_hw_addrs.c
	${CC} ${CFLAGS} -c get_hw_addrs.c

clean:
	rm -f ODR_yinlsu server_yinlsu client_yinlsu *.o

