#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cts/util.h>
#include <cts/parray.h>

#define CTS_SYMBOL "@"
#define CTS_TYPEDEF CTS_SYMBOL "typedef"

struct Symbol {
	char *name_tm; // T, U, ...
	char *name_tp; // int, long, ...
};

struct Marker {
	char *begin;
	size_t len;
};

static void usage(void);

struct Symbol *symbol_new(char *tm, size_t tmlen, char *tp, size_t tplen);
void symbol_free(void *sym);
void symbol_print(void *sym, FILE *fp);

struct Marker *marker_new(char *begin, size_t len);
void marker_print(struct Marker *marker, FILE *fp);

int save_symbol_definition(char *begin, struct parray *table);

static char *program_name;
static char *ifile;
static struct parray *symbol_table;
static struct parray *marker_list;

int main(int argc, char **argv)
{
	struct Marker *marker;
	size_t len;
	char *text;
	char *p;
	char *mark;
	char *ofile = NULL;
	char *match;
	int rv;

	program_name = argv[0];
	if (argc < 2) {
		usage();
		exit(1);
	}

	ifile = argv[1];
	mark = p = text = cts_file_read(ifile, &len);
	if (!text) {
		fprintf(stderr, "%s: Could not open %s\n", program_name, ifile);
		fflush(stderr);
		exit(2);
	}

	cts_strcatf(&ofile, "%s.c", ifile);
	symbol_table = parray_new(symbol_free);
	marker_list = parray_new(free);

	// go thru file
	while (p < text + len) {
		match = strchr(p, '@');
		if (!match) {
			break;
		}

		marker = marker_new(p, match - p);
		parray_push(marker_list, marker);

		p = match;

		/*fprintf(stdout, "%.*s", p - mark, mark);
		fflush(stdout);*/

		rv = save_symbol_definition(p, symbol_table);
		if (rv < 0) {
			fprintf(stderr, "%s: Unknown expression `%.*s'\n", program_name, -rv, p);
			fflush(stderr);
			exit(3);
		}
		// not a typedef but a replacement! Save the typename
		else if (rv == 0) {
			size_t tlen = 0;
			while (*p && !isspace(*p)) {
				tlen++;
				p++;
			}
			marker = marker_new(match, tlen);
			parray_push(marker_list, marker);
		}
		else {
			mark = p;
			p += rv + 1; // pass length + ';'
		}
	}
	marker = marker_new(p, len - (p - text));
	parray_push(marker_list, marker);

	// go through the file for each template type
	for (size_t i = 0; i < symbol_table->size; i++) {
		for (size_t j = 0; j < marker_list->size; j++) {
			if (((struct Marker *)marker_list->buf[j])->begin[0] == '@') {
				fprintf(stdout, "%s", ((struct Symbol *)symbol_table->buf[i])->name_tp);
				fflush(stdout);
			}
			else {
				marker_print(marker_list->buf[j], stdout);
			}
		}
	}

	return 0;
}

static void usage(void)
{
	assert(program_name);
	fflush(stdout);
}

struct Symbol *symbol_new(char *tm, size_t tmlen, char *tp, size_t tplen)
{
	struct Symbol *sym;

	assert(tm);
	assert(tp);

	sym = malloc(sizeof(*sym));
	assert(sym);

	sym->name_tm = strndup(tm, tmlen);
	assert(sym->name_tm);
	sym->name_tp = strndup(tp, tplen);
	assert(sym->name_tp);

	return sym;
}

void symbol_free(void *sym)
{
	struct Symbol *symbol = sym;

	assert(symbol);

	assert(symbol->name_tm);
	free(symbol->name_tm);
	assert(symbol->name_tp);
	free(symbol->name_tp);

	free(symbol);
}

void symbol_print(void *sym, FILE *fp)
{
	struct Symbol *symbol = sym;
	assert(fp);
	assert(sym);

	fprintf(fp, "%s: %s,\n", symbol->name_tm, symbol->name_tp);
	fflush(fp);
}

struct Marker *marker_new(char *begin, size_t len)
{
	struct Marker *marker;

	assert(begin);

	marker = malloc(sizeof(*marker));
	assert(marker);
	marker->begin = begin;
	marker->len = len;
	return marker;
}

void marker_print(struct Marker *marker, FILE *fp)
{
	assert(marker);
	fprintf(fp, "%.*s", marker->len, marker->begin);
	fflush(fp);
}

/**
 * \param begin
 *  The start of a symbol definition
 * \param table
 *  Where to put the symbols
 * \return
 *  The offset after \a begin to the end of the typedef region
 * 
 * \example
 *  begin -> "@typedef T <typename> [, <typename>]*;<ch>"
 *                                                  ^ retval
 */
int save_symbol_definition(char *begin, struct parray *table)
{
	struct Symbol *sym;
	char *tm;
	size_t tmlen;
	char *tp;
	size_t tplen;
	char *p = begin;

	assert(begin);
	assert(table);

	// found none
	if (strncmp(CTS_TYPEDEF, p, sizeof(CTS_TYPEDEF) - 1) != 0) {
		return 0;
	}

	p += sizeof(CTS_TYPEDEF) - 1;
	while (isspace(*p)) {
		p++;
	}
	if (!*p) {
		return -(p - begin);
	}

	// template type must start with a letter or underscore
	tm = p;
	if (!isalpha(*tm) && (*tm != '_')) {
		return -(p - begin);
	}

	for (tmlen = 0; *p && (isalnum(*p) || (*p == '_')); p++, tmlen++) {
		;
	}
	if (!*p) {
		return -(p - begin);
	}

	// skip space
	while (isspace(*p)) {
		p++;
	}
	if (!*p) {
		return -(p - begin);
	}

	tp = p;
	for (tplen = 0; *p && (isalnum(*p) || (*p == '_')); p++, tplen++) {
		;
	}
	sym = symbol_new(tm, tmlen, tp, tplen);
	assert(sym);
	parray_push(symbol_table, sym);

	// skip space
	while (isspace(*p)) {
		p++;
	}
	if (!*p) {
		return -(p - begin);
	}

	if (*p && *p != ';') {
		return -(p - begin);
	}

	return p - begin;
}
