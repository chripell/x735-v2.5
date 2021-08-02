
all: supervisor

supervisor: supervisor.c
	gcc -Wall -g -O2 -o supervisor supervisor.c

clean:
	rm -f *~ supervisor compile_commands.json

compile_commands.json:
	compiledb make
