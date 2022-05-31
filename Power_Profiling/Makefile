CC=gcc 
LDFLAGS=-lm

SOURCES=cpuLogToFile.c
OBJS= $(SOURCES:.c=)

cpuLogToFile:
	${CC} ${SOURCES} -o ${OBJS} ${LDFLAGS}


clean: 
	-rm -f cpuLogToFile 

