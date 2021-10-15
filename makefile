CC = gcc
CFLAGS = -Wall -I.
EXE=ihatesignal

hellomake:
	$(CC) $(CFLAGS) myshell.c -o $(EXE)
run:
	./$(EXE)
clean:
	rm -f ${EXE}
