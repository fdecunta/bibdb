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

.PHONY: install remove clean 
