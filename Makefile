PROG = myd
CC = gcc
#CFLAGS = -g
#CFLAGS = -O2
LINK = -lncurses
OBJS = myd.o
PREFIX = /usr/local
BIN = ${PREFIX}/bin


${PROG}:${OBJS}
	${CC} -o ${PROG} ${OBJS} ${LINK}

.c.o:
	${CC} -c ${CFLAGS} $<
clean:
	rm -f ${PROG} ${OBJS}
install:
	install ${PROG} ${BIN}
