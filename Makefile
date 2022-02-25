ryanj_proj1.exe: ryanj_proj1.o is_prime.o
	gcc ryanj_proj1.o is_prime.o -o ryanj_proj1.exe -lrt

ryanj_proj1.o: ryanj_proj1.c is_prime.h
	gcc -c ryanj_proj1.c -o ryanj_proj1.o

is_prime.o: is_prime.c is_prime.h
	gcc -c is_prime.c -o is_prime.o

clean:
	rm *.o

all: ryanj_proj1.exe
