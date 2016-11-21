CFLAGS =  -Wall -g -pedantic -c

i-banco: i-banco.o commandlinereader.o contas.o
	gcc -pthread -o i-banco *.o

commandlinereader.o: commandlinereader.h commandlinereader.c
	gcc $(CFLAGS) commandlinereader.c

contas.o: contas.h contas.c
	gcc $(CFLAGS) contas.c

i-banco.o: contas.h commandlinereader.h i-banco.c
	gcc $(CFLAGS) i-banco.c

clean:
	rm -f *.o i-banco
