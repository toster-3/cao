#include <stdio.h>

#include <arg.h>

extern char *argv0;

static void main_help(void)
{
	fprintf(stderr,
	        "Simple project manager for C code\n\n"
	        "Usage: %s <COMMAND> ...\n\n"
	        "Commands:\n"
	        "  build  build the current project\n"
	        "  new    create a new project\n"
	        "  run    run the current project\n"
	        "  help   display help\n\n"
	        "see `%s help <command|topic>` to learn more about a specific "
	        "command or topic (WIP)\n",
	        argv0, argv0);
}

void cao_help(int argc, char **argv)
{
	if (argc == 1) {
		main_help();
		return;
	}
}
