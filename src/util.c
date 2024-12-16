#include <stddef.h>
#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "logging.h"

void *emalloc(size_t size)
{
	void *p;

	p = malloc(size);
	if (!p) {
		log_fatal("malloc: out of memory");
		exit(3);
	}
	return p;
}

void *erealloc(void *p, size_t size)
{
	// log_debug("erealloc called with params size = `%zu`", size);
	p = realloc(p, size);
	if (!p) {
		log_fatal("realloc: out of memory");
		exit(3);
	}
	return p;
}

char *estrdup(char *s)
{
	char *p = strdup(s);
	if (!p) {
		log_fatal("strdup: out of memory");
		exit(3);
	}
	return p;
}

char *strdupn(char *s, size_t size)
{
	char *p = malloc(sizeof(char) * size);
	if (!p) {
		log_fatal("strdupsz: out of memory");
		exit(3);
	}
	strcpy(p, s);
	return p;
}

char *estrdupn(char *s, size_t size)
{
	char *p;
	if (strlen(s) > size) {
		log_fatal("size of '%s' is greater than %uz", s, size);
		exit(2);
	}

	p = strdupn(s, size);
	if (!p) {
		log_fatal("strdup: out of memory");
		exit(3);
	}
	return p;
}

ssize_t writeall(const void *buf, size_t len, FILE *fp)
{
	const char *p = buf;
	ssize_t n;

	while (len) {
		n = fwrite(p, sizeof(*p), len, fp);
		if (n <= 0)
			return n;
		p += n;
		len -= n;
	}

	return p - (const char *)buf;
}

int concat(FILE *f1, const char *s1, FILE *f2, const char *s2)
{
	char buf[BUFSIZ];
	ssize_t n;

	while ((n = fread(buf, sizeof(*buf), BUFSIZ, f1)) > 0) {
		if (writeall(buf, n, f2) < 0) {
			log_warn("write %s:", s2);
			return -2;
		}
	}
	if (n < 0) {
		log_warn("read %s:", s1);
		return -1;
	}
	return 0;
}

char *pathprefix(const char *pre, char *s)
{
	size_t len = strlen(pre);

	memmove(s + len + 1, s, strlen(s) + 1);
	strncpy(s, pre, len + 1);
	s[len] = '/';

	return s;
}

int cp(char *src, char *dest)
{
	FILE *fsrc, *fdest;
	int ret = 0;

	if ((fsrc = fopen(src, "r")) == NULL)
		return -2;

	if ((fdest = fopen(dest, "w+")) == NULL) {
		fclose(fsrc);
		return -1;
	}

	ret = concat(fsrc, src, fdest, dest);
	fclose(fsrc);
	fclose(fdest);
	return ret;
}
