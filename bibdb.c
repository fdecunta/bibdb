#include <sys/stat.h>

#include <err.h>
#include <errno.h>
#include <limits.h>
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
	char text[MAX_BUF + 1];
	int pos;
};

void 		  	 usage(void);
struct reference 	*parse_bib(char *);
struct reference 	*new_ref(char *);
void 			 free_list(struct reference *);
int 			 create_key(struct reference *, struct reference *);
void 			 read_key(struct reference *, char *);
int 			 delete_key(struct reference **, char *);
void 			 list_keys(struct reference *);
int 			 write_list(char *, struct reference *);

int
main(int argc, char *argv[])
{
	int c;
	int modified;
	int ret;
	char *arg;

	int cflag, dflag, lflag, rflag, uflag;
	char *dbpath, *inpath;
	struct reference *dblist, *inlist;

	ret = EXIT_SUCCESS;
	modified = 0;
	cflag = dflag = lflag = rflag = uflag = 0;
	dbpath = inpath = NULL;
	dblist = inlist = NULL;
	
	while ((c = getopt(argc, argv, "c:d:f:hlr:u:")) != -1) {
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
		case 'l':
			lflag = 1;
			break;
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

	if (cflag + dflag + lflag + rflag + uflag != 1) {
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
			goto done;
		}
		if (create_key(dblist, inlist) == 0) {
			warnx("no references added");
			ret = EXIT_FAILURE;
			goto done;
		}
		modified += 1;
	} else if (dflag) {
		if ((delete_key(&dblist, arg)) == -1) {
			fprintf(stderr, "key not found: %s\n", arg);
			ret = EXIT_FAILURE;
			goto done;
		}
		modified += 1;
	} else if (lflag) {
		list_keys(dblist);
	} else if (rflag) {
		read_key(dblist, arg);
	} else if (uflag) {
		return 0;
	}

	if (modified) {
		if ((write_list(dbpath, dblist)) == -1) {
			warnx("failed during writing");
			ret = EXIT_FAILURE;
		} 
	}

done:
	free_list(dblist);
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

	line = depth = inflag = 0;
	rhead = rlast = NULL;
	while ((c = fgetc(fp)) != EOF) {
		if (c == '\n')
			line += 1;

		if (!inflag && c == '@')
			inflag = 1;

		if (inflag) {
			if (buf.pos < MAX_BUF)
				buf.text[buf.pos++] = (char)c;
			else {
				warnx("line too long for buffer:\n\n%s...", buf.text);
				goto fail;
			}

			if (c == '{')
				depth++;

			if (c == '}') {
				if (depth == 0) {
					warnx("unbalanced curly braces at line: %d", line);
					goto fail;
				}
				depth--;
			}
		
			if (!depth && c == '}')  {
				buf.text[buf.pos] = '\0';
				struct reference *r = new_ref(buf.text);
				if (r == NULL) {
					warnx("failed to create reference");
					goto fail;
				}

				if (!rhead)
					rhead = r;
				if (rlast)
					rlast->next = r;
				rlast = r;

				buf.pos = 0;
				inflag = 0;
			}
		}
	} 

	fclose(fp);
	return rhead;

fail:
	free_list(rhead);
	fclose(fp);
	return NULL;
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
				if (!dbp->next) 
					warnx("cannot create new key: %s", inp->key);
				else {
					changes += 1;
					printf("Add %s\n", inp->key);
				}
				break;
			}
		}
	}
	return changes;
}

void
list_keys(struct reference *dblist)
{
	struct reference *p;
	for (p = dblist; p; p = p->next)
		printf("%s\n", p->key);
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
	int fd;
	struct reference *p;
	struct stat st;
	char *slash;
	char tmp[PATH_MAX];

	/* save the original permissions */
	if (stat(path, &st) == -1) {
		if (errno != ENOENT)
			warn("stat %s", path);
		mode_t mask = umask(0);
		umask(mask);
		st.st_mode = 0666 & ~mask;
	}

	/* tmp in the same directory as the db so rename() stays on one fs */
	slash = strrchr(path, '/');
	if (slash)
		snprintf(tmp, sizeof(tmp), "%.*s/.bibdb.XXXXXX", 
			(int)(slash - path), path);
	else
		snprintf(tmp, sizeof(tmp), ".bibdb.XXXXXX");

	if ((fd = mkstemp(tmp)) == -1) {
		warn("cannot create tmp file");
		return -1;
	}

	for (p = dblist; p; p = p->next) {
		if (dprintf(fd, "%s\n", p->content) < 0) {
			warn("cannot write into %s", tmp);
			close(fd);
			unlink(tmp);
			return -1;
		}
	}

	if (fchmod(fd, st.st_mode & 07777) == -1) {
		warn("cannot set permissions");
		close(fd);
		unlink(tmp);
		return -1;
	}

	if (close(fd) == -1) {
		warn("close %s", tmp);
		unlink(tmp);
		return -1;
	}
	if (rename(tmp, path) == -1) {
		warn("cannot overwrite %s", path);
		unlink(tmp);
		return -1;
	}

	return 0;
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
    fprintf(stderr, "  -l          list: list all keys\n");
}
