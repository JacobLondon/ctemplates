#include <assert.h>
#include <ctype.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <wchar.h>

int cts_arg_check(int argc, char **argv, const char *da, const char *ddarg)
{
	int i;
	for (i = 0; i < argc; i++) {
		if ((da && strcmp(argv[i], da) == 0) && (ddarg && strcmp(argv[1], ddarg) == 0)) {
			return 1;
		}
	}
	return 0;
}

char *cts_arg_get(int argc, char **argv, const char *da, const char *ddarg)
{
	int i;
	for (i = 0; i < argc; i++) {
		if ((da && strcmp(argv[i], da) == 0) && (ddarg && strcmp(argv[1], ddarg) == 0) && (i + 1 < argc)) {
			return argv[i + 1];
		}
	}
	return NULL;
}

/**
 * Only works with the following formats:
 * 
 * %d   %i   %u   %o   %x   %X   %e   %E   %a   %A   %f   %F   %c   %n   %s   %p   %%
 * %ld  %li  %lu  %lo  %lx  %lX  %le  %lE  %la  %lA  %lf  %lF  %lc  %ln  %ls
 * %lld %lli %llu %llo %llx %llX                                    %lln
 * %zd  %zi  %zu  %zo  %zx  %zX                                     %zn
 * %jd  %ji  %ju  %jo  %jx  %jX                                     %jn
 * %td  %ti  %tu  %to  %tx  %tX                                     %tn
 * 
 * Sizing, time, colors, etc... are NOT supported
 */
int cts_strcatf(char **buffer, const char *format, ...)
{
	char buf[350];
	long long int buffer_size;
	size_t format_size = strlen(format);
	va_list ap;
	long long int bytes;
	char *p, *tmp;

	enum {
		X_NONE = 0x00,
		X_L    = 0x01,
		X_LL   = 0x02,
		X_Z    = 0x04,
		X_J    = 0x08,
		X_T    = 0x10
	} type_info;

	if (format == NULL || buffer == NULL) {
		return 0;
	}

	/* allow for NULL buffer, create a new string */
	if (*buffer != NULL) {
		buffer_size = strlen(*buffer);
	}
	else {
		buffer_size = 0;
	}

	va_start(ap, format);
	for (p = (char *)format, bytes = buffer_size + format_size; p != format + format_size; ++p) {
		if (*p != '%') continue;
		if (++p == format + format_size) break;
		
		for (tmp = buf; tmp != buf + sizeof(buf) && *tmp != '\0'; *tmp++ = '\0');
		type_info = X_NONE;
		again:
		switch (*p) {
			case '%': // special case, just go to entirely next item
				bytes--;
				continue;
			case 'l':
				if (type_info & X_L) type_info = X_LL;
				else type_info = X_L;
				goto next;
			case 'z':
				type_info = X_Z;
				goto next;
			case 'j':
				type_info = X_J;
				goto next;
			case 't':
				type_info = X_T;
			next:
				if (++p == format + format_size) goto done;
				goto again;
			default:
				break;
		}

		#define X_FORMAT(Fmt, Type) \
			snprintf(buf, sizeof(buf), (Fmt), va_arg(ap, Type)); \
			bytes -= (sizeof(Fmt) - 1); break
		
		switch (*p) {
			case 'd': switch (type_info) {
				case X_NONE: X_FORMAT(  "%d", int);
				case X_L:    X_FORMAT( "%ld", long int);
				case X_LL:   X_FORMAT("%lld", long long int);
				case X_Z:    X_FORMAT( "%zd", size_t);
				case X_J:    X_FORMAT( "%jd", intmax_t);
				case X_T:    X_FORMAT( "%td", ptrdiff_t);
				default:     return 0;
			}
			break;
			case 'i': switch (type_info) {
				case X_NONE: X_FORMAT(  "%i", int);
				case X_L:    X_FORMAT( "%li", long int);
				case X_LL:   X_FORMAT("%lli", long long int);
				case X_Z:    X_FORMAT( "%zi", size_t);
				case X_J:    X_FORMAT( "%ji", intmax_t);
				case X_T:    X_FORMAT( "%ti", ptrdiff_t);
				default:     return 0;
			}
			break;
			case 'x': /* fallthrough */
			case 'X': switch (type_info) {
				case X_NONE: X_FORMAT(  "%x", unsigned int);
				case X_L:    X_FORMAT( "%lx", unsigned long int);
				case X_LL:   X_FORMAT("%llx", unsigned long long int);
				case X_Z:    X_FORMAT( "%zx", size_t);
				case X_J:    X_FORMAT( "%jx", uintmax_t);
				case X_T:    X_FORMAT( "%tx", ptrdiff_t);
				default:     return 0;
			}
			break;
			case 'u': switch (type_info) {
				case X_NONE: X_FORMAT(  "%u", unsigned int);
				case X_L:    X_FORMAT( "%lu", unsigned long int);
				case X_LL:   X_FORMAT("%llu", unsigned long long int);
				case X_Z:    X_FORMAT( "%zu", size_t);
				case X_J:    X_FORMAT( "%ju", uintmax_t);
				case X_T:    X_FORMAT( "%tu", ptrdiff_t);
				default:     return 0;
			}
			break;
			case 'e': /* fallthrough */
			case 'E': switch (type_info) {
				case X_NONE: X_FORMAT(  "%e", double);
				case X_L:    X_FORMAT( "%le", double);
				default:     return 0;
			}
			break;
			case 'a': /* fallthrough */
			case 'A': switch (type_info) {
				case X_NONE: X_FORMAT(  "%a", double);
				case X_L:    X_FORMAT( "%la", double);
				default:     return 0;
			}
			break;
			case 'f': /* fallthrough */
			case 'F': switch (type_info) {
				case X_NONE: X_FORMAT(  "%f", double);
				case X_L:    X_FORMAT( "%lf", double);
				default:     return 0;
			}
			break;
			case 'c': switch (type_info) {
				case X_NONE: X_FORMAT(  "%c", int);
				case X_L:    X_FORMAT( "%lc", wint_t);
				default:     return 0;
			}
			break;
			case 'n': switch (type_info) {
				case X_NONE: X_FORMAT(  "%n", int*);
				case X_L:    X_FORMAT( "%ln", long int*);
				case X_LL:   X_FORMAT("%lln", long long int*);
				case X_Z:    X_FORMAT( "%zn", size_t*);
				case X_J:    X_FORMAT( "%jn", intmax_t*);
				case X_T:    X_FORMAT( "%tn", ptrdiff_t*);
				default:     return 0;
			}
			break;
			case 's': switch (type_info) {
				case X_NONE:
					tmp = va_arg(ap, char*);
					assert(tmp);
					bytes -= sizeof("%s") - 1;
					bytes += strlen(tmp);
					continue;
				case X_L:
					tmp = (char *)va_arg(ap, wchar_t*);
					assert(tmp);
					bytes -= sizeof("%ls") - 1;
					bytes += wcslen((wchar_t *)tmp) * sizeof(wchar_t);
					continue;
				default:     assert(0);
			}
			break;
			case 'p': switch (type_info) {
				case X_NONE: X_FORMAT(  "%p", void*);
				default:     return 0;
			}
			break;
		}
		bytes += strlen(buf);
	}
done:
	va_end(ap);
	bytes++; /* NUL term */

	if (*buffer != NULL) {
		tmp = realloc(*buffer, bytes + 1);
		if (!tmp) return 0;
		tmp[bytes] = '\0';
		*buffer = tmp;
	}
	else {
		tmp = calloc(bytes + 1, sizeof(char));
		if (!tmp) return 0;
		*buffer = tmp;
	}
	va_start(ap, format);
	vsnprintf(&tmp[buffer_size], bytes - buffer_size, format, ap);
	va_end(ap);
	return 1;
	#undef X_FORMAT
}

#ifndef strdup
char *strdup(const char *str)
{
	size_t len = strlen(str);
	char *buf = malloc(len + 1);
	if (!buf) return NULL;
	(void)memcpy(buf, str, len + 1); // copy NUL
	return buf;
}
#endif /* strdup */

#ifndef strndup
char *strndup(const char *str, size_t n)
{
	size_t size = strlen(str);
	size = size > n ? n : size;
	char *buf = malloc(size + 1);
	if (!buf) return NULL;
	(void)memcpy(buf, str, size);
	buf[size] = 0; // manual NUL
	return buf;
}
#endif

char **cts_strsplit(char *s, const char *fmt)
{
	if (!s || !fmt) {
		return NULL;
	}

	size_t word_count = 0;
	size_t size = strlen(s);
	char **buf;
	char *tmp;
	size_t i, j;

	/* count words */
	for (i = 0; i < size - 1; i++) {
		/* skip any invalid characters */
		if (strchr(fmt, s[i])) {
			continue;
		}

		/* word was found, traverse passed it */
		for (; !strchr(fmt, s[i]) && s[i]; i++)
			;
		word_count++;
	}

	/* hold each string and NULL terminate */
	buf = calloc(word_count + 1, sizeof(char *));
	if (!buf) {
		return NULL;
	}

	/* dup each string */
	for (i = 0, j = 0; i < word_count; i++) {
		/* probe forward for length */
		for (; j < size; j++) {
			if (strchr(fmt, s[j])) {
				continue;
			}
			
			/* word was found, traverse passed it */
			for (tmp = &s[j]; !strchr(fmt, s[j]) && s[j]; j++)
				;

			/* buf[i] might be NULL, no guarantees */
			buf[i] = strndup(tmp, (size_t)(&s[j] - tmp));
			break;
		}
	}
	return buf;
}

void cts_strsplit_free(char **buf)
{
	size_t i = 0;
	for (; buf[i]; i++) {
		free(buf[i]);
		buf[i] = NULL;
	}
	free(buf);
}

int cts_streplace(char **s, const char *old, const char *new)
{
	char *result;
	char *sub;
	char *p;
	size_t len;
	size_t old_len;
	size_t new_len;

	if (s == NULL || *s == NULL || old == NULL || new == NULL) {
		return 0;
	}

	p = *s;
	len = strlen(*s);
	old_len = strlen(old);
	new_len = strlen(new);

	/* no replacements found */
	if ((sub = strstr(p, old)) == NULL) {
		return 0;
	}

	/* at least one replacement found */
	do {
		len = len - old_len + new_len;
		p = (char *)((size_t)sub + old_len);
	} while ((sub = strstr(p, old)));
	
	result = calloc(len + 1, sizeof(char));
	if (!result) {
		return 0;
	}
	
	/* re-add the bytes back into the result */
	for (p = *s; *p != '\0'; p++) {
		if (p == strstr(p, old)) {
			strcat(result, new);
			p += old_len - 1;
		}
		else {
			strncat(result, p, 1);
		}
	}

	free(*s);
	*s = result;

	return 1;
}

char *cts_file_read(const char *fname, size_t *size)
{
	FILE *f;
	char *buf;
	size_t bytes;
	long length;

	if (!fname) {
		return NULL;
	}

	f = fopen(fname, "rb");
	if (!f) {
		return NULL;
	}

	fseek(f, 0, SEEK_END);
	length = ftell(f);
	fseek(f, 0, SEEK_SET);

	buf = malloc(length + 1);
	if (!buf) {
		fclose(f);
		return NULL;
	}

	bytes = fread(buf, 1, length, f);
	buf[bytes] = '\0';
	if (size) {
		*size = bytes;
	}

	fclose(f);
	return buf;
}
