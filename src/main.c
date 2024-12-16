#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "arg.h"
#include "cao.h"
#include "logging.h"
#include "util.h"

char *argv0;
int tty_switch = 1;

int str_eq(const char *s1, const char *s2)
{
	return !strncmp(s1, s2, strlen(s1));
}

void usage(int exit_query)
{
	fprintf(stderr,
	        "usage: %s -hV <COMMAND> ...\n\n"
	        "for more information run `%s help`",
	        argv0, argv0);
	if (exit_query)
		exit(exit_query);
}

void cao_version(void)
{
	printf("version info\n");
}

int main(int argc, char *argv[])
{
	ARGSTRUCT{
	    {"help", 'h'},
	    {"version", 'V'},
	};

	if (argc < 2) {
		log_error("not enough args");
		exit(1);
	}
	log_set_level(3);

	argv0 = argv[0];
	ELONG_ARGBEGIN(usage(1))
	{
	case 'h':
		cao_help(argc, argv);
		exit(0);
	case 'V':
		cao_version();
		exit(0);
	default:
		usage(1);
	}
	ARGEND;

	if (str_eq("help", argv[0])) {
		cao_help(argc, argv);
	} else if (str_eq("version", argv[0])) {
		cao_version();
	} else if (str_eq("build", argv[0])) {
		cao_build(argc, argv);
	} else if (str_eq("new", argv[0])) {
		cao_new(argc, argv);
	} else if (str_eq("run", argv[0])) {
		cao_run(argc, argv);
	}

	return 0;
}
