.SUFFIXES:

CC = gcc
CFLAGS = -Wall -std=gnu99 -fopenmp -O2 -march=native -DNDEBUG
LIBS = -lzmq -lm

OBJECTS =  tiff.o queue.o
	
pilatus:	${OBJECTS} pilatus.c
	$(CC) $(CFLAGS) pilatus.c ${OBJECTS} $(LIBS) -o pilatus
	
test:	${OBJECTS} test.c
	$(CC) $(CFLAGS) test.c ${OBJECTS} $(LIBS) -o test
		
%.o:	%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm *.o pilatus
	


