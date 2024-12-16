#include <libgen.h>
#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "arg.h"
#include "util.h"

enum buildmode { DEV, RELEASE };

void cao_build(int argc, char **argv)
{
	char buf[PATH_MAX];
	enum buildmode bm = DEV;
	ARGSTRUCT{{"release", 'r'}, {"dev", 'd'}, {0, 0}};

	LONG_ARGBEGIN
	{
	case 'r':
		bm = RELEASE;
		break;
	default:
		exit(1);
	}
	ARGEND;

	getcwd(buf, PATH_MAX);
	fprintf(stderr, "\x1b[32;1mBuilding\x1b[0m %s (%s) with make\n",
	        basename(buf), buf);
	switch (bm) {
	case DEV:
		system("make");
		break;
	case RELEASE:
		system("make OFLAGS=\"-O2\"");
		break;
	}
}

// NOTE: very messy
void cao_run(int argc, char **argv)
{
	char buf[PATH_MAX];
	enum buildmode bm = DEV;
	ARGSTRUCT{{"release", 'r'}, {"dev", 'd'}, {0, 0}};

	LONG_ARGBEGIN
	{
	case 'r':
		bm = RELEASE;
		break;
	default:
		exit(1);
	}
	ARGEND;

	getcwd(buf, PATH_MAX);
	fprintf(stderr, "\x1b[32;1mBuilding\x1b[0m %s (%s) with make\n",
	        basename(buf), buf);

	memmove(buf, basename(buf), strlen(buf));
	pathprefix("./build", buf);
	switch (bm) {
	case DEV:
		system("make");
		break;
	case RELEASE:
		system("make OFLAGS=\"-O2\"");
		break;
	}
	fprintf(stderr,
	        "\x1b[1;32mFinished\x1b[0m `%s` profile\n"
	        "\x1b[1;32mRunning\x1b[0m  `%s`\n",
	        (bm == DEV) ? "dev" : "release", buf);
	system(buf);
}
