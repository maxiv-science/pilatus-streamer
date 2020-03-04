.SUFFIXES:

CC = gcc
CFLAGS = -Wall -std=gnu99 -fopenmp -Ibitshuffle/src -Ibitshuffle/lz4 -O2 -march=native -DNDEBUG
LIBS = -lzmq -lm

OBJECTS =  tiff.o queue.o bitshuffle/src/iochain.o bitshuffle/src/bitshuffle_core.o bitshuffle/src/bitshuffle.o bitshuffle/lz4/lz4.o
	
pilatus:	${OBJECTS} pilatus.c
	$(CC) $(CFLAGS) pilatus.c ${OBJECTS} $(LIBS) -o pilatus
	
test:	${OBJECTS} test.c
	$(CC) $(CFLAGS) test.c ${OBJECTS} $(LIBS) -o test
		
%.o:	%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm *.o bitshuffle/src/*.o bitshuffle/lz4/*.o pilatus
	


