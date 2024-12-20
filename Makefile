C:: 
	gcc -c devices.c; 
	gcc -c threads.c; 
	gcc -o threads threads.o devices.o -lrt -lwiringPi -lpthread -lm

clean::
	rm *.o threads

all:: C
