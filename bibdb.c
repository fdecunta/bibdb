#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void 	usage(void);

char *dbpath = "/home/fd/bibdb/foo.bib";

int cflag;
int dflag;
int rflag;
int uflag;

int
main(int argc, char *argv[])
{
	int c;
	FILE *fp;

	cflag = dflag = rflag = uflag = 0;
	
	while ((c = getopt(argc, argv, "c:d:f:hr:u:")) != -1) {
		switch (c) {
		case 'c':
			cflag = 1;
			break;
		case 'd':
			dflag = 1;
			break;
		case 'f':
			dbpath = optarg;
			break;
		case 'h':
			usage();
			exit(EXIT_SUCCESS);
		case 'r':
			rflag = 1;
			break;
		case 'u':
			uflag = 1;
			break;
		default:
			usage();
			exit(EXIT_FAILURE);
		}
	}
	argc -= optind;
	argv += optind;

	if (cflag + dflag + rflag + uflag != 1) {
		warnx("too many options");
		usage();
		exit(EXIT_FAILURE);
	}

	// TODO: for the moment only 'r' mode, but need 'w' for the rest.
	if ((fp = fopen(dbpath, "r")) == NULL)
		err(EXIT_FAILURE, "cannot open %s", dbpath);
	
	ssize_t nread;
	size_t len = 0;
	char *line = NULL;
 
	while ((nread = getline(&line, &len, fp)) != -1) {
		puts(line);
	} 

	free(line);
	fclose(fp);
	exit(EXIT_SUCCESS);
}

// TODO: parse this stuff!

void
usage(void)
{
    fprintf(stderr,
        "usage: bibdb -f dbfile (-c file | -r key | -u key | -d key)\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Create, read, update and delete entries in a bibtex database.\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "  -f dbfile   database file (required)\n");
    fprintf(stderr, "  -c file     create: add entries from file ('-' reads stdin)\n");
    fprintf(stderr, "  -r key      read: print entry for this key\n");
    fprintf(stderr, "  -u key      update: replace entry 'key' with an entry read from stdin\n");
    fprintf(stderr, "  -d key      delete: remove entry with this key\n");
}
