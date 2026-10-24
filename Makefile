CC    = cc
FLAGS = -Wall -Wextra -Werror -Wconversion -g

PROG = bibdb
SRC  = bibdb.c
MAN  = bibdb.1

BINDIR  = /usr/local/bin
MANDIR  = /usr/local/man/man1

$(PROG): $(SRC)
	$(CC) $(FLAGS) $(SRC) -o $(PROG)

clean:
	rm -f $(PROG)

install: $(PROG)
	cp $(PROG) $(BINDIR)
	mkdir -p $(MANDIR)
	cp $(MAN) $(MANDIR)/$(MAN)

remove:
	rm -f $(BINDIR)/$(PROG)
	rm -f $(MANDIR)/$(MAN)

.PHONY: install remove clean 
