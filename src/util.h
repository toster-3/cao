#ifndef UTIL_H_
#define UTIL_H_

#include <stdio.h>
#include <unistd.h>

void *emalloc(size_t);
void *erealloc(void *, size_t);
void *estrdup(char *);
void *strdupn(char *, size_t);
void *estrdupn(char *, size_t);

ssize_t writeall(const void *, size_t, FILE *);
int concat(FILE *, const char *, FILE *, const char *);
int cp(char *, char *);
char *pathprefix(char *, char *);

#endif
