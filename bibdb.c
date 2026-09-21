#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_BUF 4096

struct reference {
	char *key;
	char *content;
	struct reference *next;
};

struct buffer {
	char text[MAX_BUF];
	int pos;
};

void 		  	 usage(void);
struct reference 	*parse_bib(char *);
struct reference 	*new_ref(char *);
void 			 free_list(struct reference *);
int 			 create_key(struct reference *, struct reference *);
void 			 read_key(struct reference *, char *);
int 			 delete_key(struct reference **, char *);

int 			 write_list(char *, struct reference *);

int
main(int argc, char *argv[])
{
	int c;
	int modified;
	int ret;
	char *arg;

	int cflag, dflag, rflag, uflag;
	char *dbpath, *inpath;
	struct reference *dblist, *inlist;

	ret = EXIT_SUCCESS;
	modified = 0;
	cflag = dflag = rflag = uflag = 0;
	dbpath = inpath = NULL;
	dblist = inlist = NULL;
	
	while ((c = getopt(argc, argv, "c:d:f:hr:u:")) != -1) {
		switch (c) {
		case 'c':
			cflag = 1;
			inpath = optarg;
			break;
		case 'd':
			dflag = 1;
			arg = optarg;
			break;
		case 'f':
			dbpath = optarg;
			break;
		case 'h':
			usage();
			exit(EXIT_SUCCESS);
		case 'r':
			rflag = 1;
			arg = optarg;
			break;
		case 'u':
			uflag = 1;
			arg = optarg;
			break;
		default:
			usage();
			exit(EXIT_FAILURE);
		}
	}
	argc -= optind;
	argv += optind;

	if (argc) {
		warnx("too many arguments");
		usage();
		exit(EXIT_FAILURE);
	}

	if (!dbpath) {
		warnx("file not specified");
		usage();
		exit(EXIT_FAILURE);
	}

	if (cflag + dflag + rflag + uflag != 1) {
		warnx("too many options");
		usage();
		exit(EXIT_FAILURE);
	}

	dblist = parse_bib(dbpath);
	if (!dblist) 
		err(EXIT_FAILURE, "failed to read references from %s", dbpath);

	if (cflag) {
		inlist = parse_bib(inpath);
		if (!inlist) {
			warnx("failed to read references from %s", inpath);
			ret = EXIT_FAILURE;
		}
		if (create_key(dblist, inlist) == 0) {
			warnx("no references added");
			ret = EXIT_FAILURE;
		}
		modified += 1;
	} else if (dflag) {
		if ((delete_key(&dblist, arg)) == -1) {
			fprintf(stderr, "key not found: %s\n", arg);
			ret = EXIT_FAILURE;
		}
		modified += 1;
	} else if (rflag) {
		read_key(dblist, arg);
	} else if (uflag) {
		return 0;
	}

	if (modified) {
		if ((write_list(dbpath, dblist)) < 1) {
			warnx("failed during writing");
			ret = EXIT_FAILURE;
		} 
	}

	free_list(dblist);
	if (inlist)
		free_list(inlist);

	exit(ret);
}

struct reference *
parse_bib(char *path)
{
	/* 
	 * read a full entry into a buffer.
	 * when if ends, find the key in the
	 * buffer and store it along with the 
	 * content into a list of references.
	 */

	FILE *fp;
	int c;
	int line;
	int depth; 	/* count {} depth */
	int inflag;	/* 1 if inside a reference */
	struct reference *rhead, *rlast;
	struct buffer buf = {0};

	fp = fopen(path, "r");
	if (!fp) {
		warn("cannot open %s", path);
		return NULL;
	}

	line = depth = 0;
	rhead = rlast = NULL;
	while ((c = fgetc(fp)) != EOF) {
		if (c == '\n')
			line += 1;
		else if (c == '{')
			depth++;
		else if (c == '}') {
			if (depth == 0) 
				errx(EXIT_FAILURE, "unbalanced curly braces at line: %d", line);
			depth--;
		}

		if (!inflag && c == '@')
			inflag = 1;

		if (inflag)  {
			if (buf.pos < MAX_BUF)
				buf.text[buf.pos++] = (char)c;
			else {
				warnx("line too long for buffer:\n\n%s...", buf.text);
				free_list(rhead);
				fclose(fp);
				return NULL;
			}
		}

		if (inflag && !depth && c == '\n')  {
			buf.text[--buf.pos] = '\0';
			struct reference *r = new_ref(buf.text);
			if (r == NULL) {
				warnx("failed to create reference");
				free_list(rhead);
				fclose(fp);
				return NULL;
			}

			if (!rhead)
				rhead = r;
			if (rlast)
				rlast->next = r;
			rlast = r;

			memset(buf.text, '\0', MAX_BUF);
			buf.pos = 0;
			inflag = 0;
		}
	} 

	fclose(fp);
	return rhead;
}

struct reference *
new_ref(char *buf)
{
	char *sp, *ep;
	struct reference *r;

	if ((sp = strchr(buf, '{')) == NULL) {
		warnx("bad formated reference, can't find key");
		return NULL;
	}
	sp++; 	/* set pointer past '{' */

	if ((ep = strchr(sp, ',')) == NULL) {
		warnx("bad formated reference, can't find key");
		return NULL;
	}

	r = (struct reference *) malloc(sizeof(struct reference));
	r->key = strndup(sp, (size_t)(ep - sp));
	r->content = strdup(buf);
	r->next = NULL;

	if (r->key == NULL || r->content == NULL) {
		warnx("cannot duplicate string");
		return NULL;
	}

	return r;
}

int
create_key(struct reference *dblist, struct reference *inlist)
{
	int changes;
	struct reference *dbp, *inp;

	changes = 0;
	for (inp = inlist; inp; inp = inp->next) {
		for (dbp = dblist; dbp; dbp = dbp->next) {
			if (!strcmp(inp->key, dbp->key)) {
				warnx("key already exists: %s", inp->key);
				break;
			}

			if (!dbp->next) {
				dbp->next = new_ref(inp->content);
				changes += 1;
				printf("Add %s\n", inp->key);
				break;
			}
		}
	}
	return changes;
}

void
read_key(struct reference *dblist, char *key)
{
	struct reference *p;

	for (p = dblist; p; p = p->next) {
		if (!strcmp(key, "-"))
			printf("%s\n", p->content);
		else if (!strcmp(key, p->key)) {
			printf("%s\n", p->content);
			break;
		}
	}
}

int
delete_key(struct reference **head, char *key)
{
	struct reference *p, *prev;

	prev = NULL;
	for (p = *head; p; p = p->next) {
		if (!strcmp(key, p->key)) {
			if (prev)
				prev->next = p->next;
			else
				*head = p->next;
			printf("Deleted %s\n", p->key);
			free(p->key);
			free(p->content);
			free(p);
			return 0;
		}
		prev = p;
	}
	return -1;
}

int
write_list(char *path, struct reference *dblist)
{
	int writes = 0;
	int fd; 	/* file descriptor for tmp file */
	struct reference *p;
	char tmp[] = "bibdb.XXXXXX";

	if ((fd = mkstemp(tmp)) == -1) {
		warn("cannot create tmp file");
		return -1;
	}

	for (p = dblist; p; p = p->next, writes++)
		dprintf(fd, "%s\n", p->content); 
	close(fd);
	
	if ((rename(tmp, path)) != 0) {
		warn("cannot overwrite %s", path);
		unlink(tmp);
		return -1;
	}

	return writes;
}

void
free_list(struct reference *head)
{
	if (!head)
		return;

	struct reference *p, *tmp;
	for (p = head; p != NULL; p = tmp) {
		tmp = p->next;

		free(p->key);
		free(p->content);
		free(p);
	}
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
    fprintf(stderr, "  -r key      read: print entry for this key ('-' prints all)\n");
    fprintf(stderr, "  -u key      update: replace entry 'key' with an entry read from stdin\n");
    fprintf(stderr, "  -d key      delete: remove entry with this key\n");
}
