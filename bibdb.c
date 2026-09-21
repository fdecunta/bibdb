#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct reference {
	char *type;
	char *key;
	char *content;
};

void 	usage(void);
void 	parse_bib(FILE *);

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

	parse_bib(fp);
	
	fclose(fp);
	exit(EXIT_SUCCESS);
}

void
parse_bib(FILE *fp)
{
	ssize_t len;
	size_t linesz = 0;
	char *line = NULL;

	/* 
	 * TODO: cannot use getline because stuff can come in multiple lines. 
	 * need to do something with {} and depth levels.
	 */

	char *sp, *ep;
	while ((len = getline(&line, &linesz, fp)) != -1) {
		if (line[len - 1] == '\n')
			line[--len] = '\0';

		struct reference *r;
		r = (struct reference *) malloc(sizeof(struct reference));

		if ((r->content = strdup(line)) == NULL) 
			// here it leaks r, maybe free
			err(EXIT_FAILURE, "cannot allocate space for strdup");
		
		sp = strchr(line, '@');
		ep = strchr(line, '{');
		if (sp == NULL || ep == NULL) {
			warnx("cannot parse line: \n%s", line);
			free(r);
			continue;
		}
		sp++; 	/* pass the at */
		r->type = strndup(sp, (size_t)(ep - sp)); 

		sp = ep;
		ep = strchr(sp, ',');
		if (ep == NULL) {
			warnx("cannot parse line: \n%s", line);
			free(r);
			continue;
		}
		sp++; 	/* pass the '{' */
		r->key = strndup(sp, (size_t)(ep - sp)); 

		printf("%s\n", r->type);
		printf("%s\n", r->key);

		free(r);
	} 

	free(line);
}

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
