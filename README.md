# bibdb

A command-line tool for managing a BibTeX database file.

`bibdb` treats a `.bib` file as a tiny database: each entry is identified by its
citation key (the text between the entry's opening brace and the first comma)
and can be added, printed, listed or deleted. Keys must be unique; when the
database is modified it is rewritten atomically through a temporary file in the
same directory, and the original file permissions are preserved.

## Build and install

```
$ make
# make install      # installs to /usr/local/bin and /usr/local/man/man1
```

To uninstall:

```
# make remove
$ make clean        # deletes the build artifact
```

## Usage

```
bibdb -f dbfile (-c file | -d key | -l | -r key)
```

- `-f dbfile` — database file (required)
- `-c file` — add every entry from `file` (`-` reads stdin)
- `-r key` — print the entry for `key` (`-` prints all entries)
- `-d key` — delete the entry with this key
- `-l` — list all keys

The program exits 0 on success and nonzero on error (bad usage, unknown key,
malformed database, ...).

## Examples

Add entries from a file, then list the keys:

```
$ bibdb -f refs.bib -c new.bib
Add knuth1984
$ bibdb -f refs.bib -l
knuth1984
```

Add an entry from standard input, read it back, then delete it:

```
$ echo '@misc{torvalds1991, title={Linux}}' | bibdb -f refs.bib -c -
Add torvalds1991
$ bibdb -f refs.bib -r torvalds1991
@misc{torvalds1991, title={Linux}}
$ bibdb -f refs.bib -d torvalds1991
Deleted torvalds1991
```

## Notes

- Whitespace around keys is ignored (`@book{ key ,` is stored as `key`).
- Duplicate keys are not added again; a warning is printed for each one.
- Entries longer than 4096 bytes are rejected.
- Plain entries only: `@string` and `@comment` are not supported.
