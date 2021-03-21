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

static void usage(void);

struct Symbol *symbol_new(char *tm, size_t tmlen, char *tp, size_t tplen);
void symbol_free(void *sym);
void symbol_print(void *sym);

int save_symbol_definition(char *begin, struct parray *table);

static char *program_name;
static char *ifile;
static struct parray *symbol_table;

int main(int argc, char **argv)
{
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

	// go thru file
	while (*p) {
		match = strchr(p, '@');
		if (!match) {
			break;
		}
		p = match;

		fprintf(stdout, "%.*s", p - mark, mark);
		fflush(stdout);

		rv = save_symbol_definition(p, symbol_table);
		if (rv < 0) {
			fprintf(stderr, "%s: Unknown expression `%.*s'\n", program_name, -rv, p);
			fflush(stderr);
			exit(3);
		}
		// not a typedef but a replacement!
		else if (rv == 0) {
			p++;
		}
		else {
			mark = p;
			p += rv;
		}
	}
	fprintf(stdout, "%.*s", p - mark, mark);
	fflush(stdout);

	/*for (int i = 0; i < symbol_table->size; i++) {
		symbol_print(symbol_table->buf[i]);
	}*/

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

void symbol_print(void *sym)
{
	struct Symbol *symbol = sym;

	assert(sym);

	fprintf(stdout, "%s: %s,\n", symbol->name_tm, symbol->name_tp);
	fflush(stdout);
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
