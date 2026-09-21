CC    = cc
FLAGS = -Wall -Wextra -Werror -Wconversion -g

PROG = bibdb
SRC  = bibdb.c

BIN  = /usr/local/bin

$(PROG): $(SRC)
	$(CC) $(FLAGS) $(SRC) -o $(PROG)

clean:
	rm -f $(PROG)

install: $(PROG)
	cp $(PROG) $(BIN)

remove:
	rm -f $(BIN)/$(PROG)

run: $(PROG)
	./$(PROG) -f bla.bib -c vks.bib

mem: $(PROG)
	valgrind --leak-check=yes ./$(PROG) -f foo.bib -r -

.PHONY: install remove clean 
