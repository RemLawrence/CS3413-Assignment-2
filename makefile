CC = gcc
CFLAGS = -Wall -I.
EXE=signalmakemepuke

hellomake:
	$(CC) $(CFLAGS) myshell.c -o $(EXE)
run:
	./$(EXE)
