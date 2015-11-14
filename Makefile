CC = gcc

UNP_DIR = /users/cse533/Stevens/unpv13e

LIBS = -lpthread ${UNP_DIR}/libunp.a

FLAGS = -g -O2

CFLAGS = ${FLAGS} -I${UNP_DIR}/lib

all: ODR_yinlsu server_yinlsu client_yinlsu

utils.o: utils.c
	${CC} ${CFLAGS} -c utils.c

ODR_yinlsu: odr.o odr_frame.o utils.o get_hw_addrs.o
	${CC} ${CFLAGS} -o ODR_yinlsu odr.o odr_frame.o utils.o get_hw_addrs.o ${LIBS}

odr.o: odr.c
	${CC} ${CFLAGS} -c odr.c

odr_frame.o: odr_frame.c
	${CC} ${CFLAGS} -c odr_frame.c

odr_api.o: odr_api.c
	${CC} ${CFLAGS} -c odr_api.c

server_yinlsu: server.o get_hw_addrs.o odr_api.o
	${CC} ${CFLAGS} -o server_yinlsu server.o get_hw_addrs.o odr_api.o ${LIBS}

server.o: server.c
	${CC} ${CFLAGS} -c server.c

client_yinlsu: client.o get_hw_addrs.o odr_api.o
	${CC} ${CFLAGS} -o client_yinlsu client.o get_hw_addrs.o odr_api.o ${LIBS}

client.o: client.c
	${CC} ${CFLAGS} -c client.c

get_hw_addrs.o: get_hw_addrs.c
	${CC} ${CFLAGS} -c get_hw_addrs.c

clean:
	rm -f ODR_yinlsu server_yinlsu client_yinlsu *.o

