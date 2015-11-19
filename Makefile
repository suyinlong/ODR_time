CC = gcc

USR = $(shell logname)
UNP_DIR = /users/cse533/Stevens/unpv13e

LIBS = -lpthread ${UNP_DIR}/libunp.a

FLAGS = -g -O2

CFLAGS = ${FLAGS} -I${UNP_DIR}/lib

all: ODR_${USR} server_${USR} client_${USR}

utils.o: utils.c
	${CC} ${CFLAGS} -c utils.c

ODR_${USR}: odr.o odr_frame.o odr_handler.o utils.o get_hw_addrs.o odr_rrep.o
	${CC} ${CFLAGS} -o ODR_${USR} odr.o odr_frame.o odr_handler.o utils.o get_hw_addrs.o odr_rrep.o ${LIBS}

odr.o: odr.c
	${CC} ${CFLAGS} -c odr.c

odr_frame.o: odr_frame.c
	${CC} ${CFLAGS} -c odr_frame.c

odr_handler.o: odr_handler.c
	${CC} ${CFLAGS} -c odr_handler.c

odr_api.o: odr_api.c
	${CC} ${CFLAGS} -c odr_api.c

odr_rrep.o: odr_rrep.c
	${CC} ${CFLAGS} -c odr_rrep.c

server_${USR}: server.o get_hw_addrs.o utils.o odr_api.o
	${CC} ${CFLAGS} -o server_${USR} server.o get_hw_addrs.o utils.o odr_api.o ${LIBS}

server.o: server.c
	${CC} ${CFLAGS} -c server.c

client_${USR}: client.o get_hw_addrs.o utils.o odr_api.o
	${CC} ${CFLAGS} -o client_${USR} client.o get_hw_addrs.o utils.o odr_api.o ${LIBS}

client.o: client.c
	${CC} ${CFLAGS} -c client.c

get_hw_addrs.o: get_hw_addrs.c
	${CC} ${CFLAGS} -c get_hw_addrs.c

clean:
	rm -f ODR_${USR} server_${USR} client_${USR} *.o
	
install:
	~/cse533/deploy_app ODR_${USR} server_${USR} client_${USR}	

