#include <asm-generic/errno-base.h>
#define _DEFAULT_SOURCE
#include <dirent.h>
#include <errno.h>
#include <libgen.h>
#include <linux/limits.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "base.h"
#define OPTPARSE_IMPLEMENTATION
#include "optparse.h"

#define assert(x) do { if (!(x)) __builtin_trap(); } while(0)

char *argv0;

bool eq(char *s1, char *s2)
{
	return !strcmp(s1, s2);
}

typedef struct StrNode StrNode;
struct StrNode {
	Str s;
	StrNode *next;
};

typedef struct {
	StrNode *first;
	StrNode *last;
	Size len;
	Size total_size;
} StrList;

typedef struct {
	Arena perm, scratch;
} Context;

bool file_exists(char *fname)
{
	return !access(fname, F_OK);
}

Str path_basename(Str s)
{
	char *p = s.v;
	Usize i;
	if (!p || !*p)
		return S(".");
	i = s.len - 1;
	for (; i && p[i] == '/'; i--) p[i] = 0;
	for (; i && p[i - 1] != '/'; i--);
	return (Str){p + i, s.len - i};
}

char *signal_str(int sig)
{
	// clang-format off
	switch (sig) {
	case SIGABRT:   return "SIGABRT";
	case SIGALRM:   return "SIGALRM";
	case SIGBUS:    return "SIGBUS";
	case SIGCHLD:   return "SIGCHLD";
	case SIGCONT:   return "SIGCONT";
	case SIGFPE:    return "SIGFPE";
	case SIGHUP:    return "SIGHUP";
	case SIGILL:    return "SIGILL";
	case SIGINT:    return "SIGINT";
	case SIGIO:     return "SIGIO";
	case SIGKILL:   return "SIGKILL";
	case SIGPIPE:   return "SIGPIPE";
	case SIGPROF:   return "SIGPROF";
	case SIGPWR:    return "SIGPWR";
	case SIGQUIT:   return "SIGQUIT";
	case SIGSEGV:   return "SIGSEGV";
	case SIGSTKFLT: return "SIGSTKFLT";
	case SIGSTOP:   return "SIGSTOP";
	case SIGTSTP:   return "SIGTSTP";
	case SIGSYS:    return "SIGSYS";
	case SIGTERM:   return "SIGTERM";
	case SIGTRAP:   return "SIGTRAP";
	case SIGTTIN:   return "SIGTTIN";
	case SIGTTOU:   return "SIGTTOU";
	case SIGURG:    return "SIGURG";
	case SIGUSR1:   return "SIGUSR1";
	case SIGUSR2:   return "SIGUSR2";
	case SIGVTALRM: return "SIGVTALRM";
	case SIGXCPU:   return "SIGXCPU";
	case SIGXFSZ:   return "SIGXFSZ";
	case SIGWINCH:  return "SIGWINCH";
	}
	// clang-format on
	return 0;
}

int os_command(char **argv, Arena scratch, char *argv0)
{
	Str cmd = {0};
	pid_t child_pid;
	int status;
	for (char **arg = argv; *arg; arg++) {
		cmd = str_concat(cmd, S(" "), &scratch);
		cmd = str_concat(cmd, STR(*arg), &scratch);
	}
	cmd.v[0] = '>';
	cmd = str_concat(cmd, S("\n"), &scratch);
	fwrite(cmd.v, sizeof(*cmd.v), cmd.len, stdout);
	switch ((child_pid = fork())) {
	case -1:
		perror("fork");
		exit(1);
	case 0:
		execvp(argv[0], argv);
		perror("execvp");
		_exit(127);
	default:
		break;
	}
	waitpid(child_pid, &status, 0);
	if (WIFSIGNALED(status)) {
		char *sigstr = signal_str(WSTOPSIG(status));
		cmd = str_skip(cmd, 1);
		fprintf(stderr, "%s: process '%.*s' terminated by signal %s\n", argv0, (int)cmd.len, cmd.v, sigstr);
	}

	return WEXITSTATUS(status);
}

Str path_concat(Str a, Str b, Arena *perm)
{
	if (str_eq(a, S(".")))
		return b;
	if (str_eq(b, S(".")))
		return a;
	Str s = a.v[a.len - 1] == '/' ? a : str_concat(a, S("/"), perm);
	return str_concat(s, b, perm);
}

typedef struct DirEnt DirEnt;
struct DirEnt {
	DirEnt *next;
	bool is_dir;
	Str fname;
	Str bname;
};

Str str_null_terminate(Str s, Arena *perm)
{
	return str_concat(s, (Str){"\0", 1}, perm);
}

// NOTE: doesnt handle symlink collisions or wtv its called
DirEnt *all_files(Str dir, DirEnt *last, Str cwd, Arena *perm)
{
	DirEnt *node = last;
	DIR *dr;
	struct dirent *de;
	Str realdir = path_concat(dir, cwd, perm);
	dr = opendir(str_null_terminate(realdir, perm).v);
	if (!dr) {
		fprintf(stderr, "%s: opendir '%.*s': %s\n", argv0, (int)realdir.len, realdir.v, strerror(errno));
		exit(1);
	}
	while ((de = readdir(dr))) {
		DirEnt *nnode;
		if (de->d_type == DT_DIR) {
			if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, "..")) {
				continue;
			}
			nnode = new(DirEnt, perm);
			node = all_files(dir, node, path_concat(cwd, STR(de->d_name), perm), perm);
			nnode->is_dir = true;
		} else {
			nnode = new(DirEnt, perm);
			nnode->is_dir = false;
		}
		nnode->next = node;
		nnode->bname = path_concat(cwd, str_copy_cstr(de->d_name, perm), perm);
		nnode->fname = path_concat(dir, nnode->bname, perm);
		node = nnode;
	}
	closedir(dr);
	return node;
}

bool str_has_prefix(Str s, Str pref)
{
	return str_eq(prefix(s, pref.len), pref);
}

Usize fputstr(Str s, FILE *fp)
{
	return fwrite(s.v, sizeof *s.v, s.len, fp);
}

#define STR_FMT(s) (int)(s).len, s.v
bool copy_over_and_subst(Str dest, Str src, Str substitution, Arena scratch)
{
	FILE *in = fopen(str_null_terminate(src, &scratch).v, "rb");
	if (!in) {
		fprintf(stderr, "%s: fopen '%.*s': %s\n", argv0, (int)src.len, src.v, strerror(errno));
		return false;
	}
	FILE *out = fopen(str_null_terminate(dest, &scratch).v, "wb");
	if (!out) {
		fclose(in);
		fprintf(stderr, "%s: fopen '%.*s': %s\n", argv0, (int)dest.len, dest.v, strerror(errno));
		return false;
	}
	Str s = slurp_file(in, &scratch);
	while (s.len) {
		if (str_has_prefix(s, S("{{PROJECTNAME}}"))) {
			fputstr(substitution, out);
			s = str_skip(s, sizeof("{{PROJECTNAME}}"));
		} else {
			fputc(*s.v, out);
			s = str_skip(s, 1);
		}
	}
	fclose(in);
	fclose(out);
	return true;
}

Str str_cwd(Arena *perm)
{
	Size amount = 4096;
	char *s = make(char, amount, perm);
	while (!getcwd(s, amount) && errno == ERANGE) {
		// we know that this memory is right after the previous allocation
		make(char, amount, perm);
		amount *= 2;
	}
	return STR(s);
}

// cao new <template> <project>
int cmd_new(char **argv, Arena *perm, Arena scratch)
{
	struct optparse_long longopts[] = {
	    {"force", 'f', OPTPARSE_NONE},
	    {"help", 'h', OPTPARSE_NONE}};
	int option;
	bool force = false;
	struct optparse options;
	optparse_init(&options, argv);

	while ((option = optparse_long(&options, longopts, NULL)) != -1) {
		switch (option) {
		case 'f':
			force = true;
			break;
		case 'h':
			return 123123;
		case '?':
			fprintf(stderr, "%s: %s\nusage: %s new <template> <project>", argv[0], options.errmsg, argv0);
			return 1;
		}
	}

	if (!argv[1] || !argv[2]) {
		fprintf(stderr, "%s: not enough arguments\nUsage: %s new <template> <project>\n", argv0, argv0);
		return 1;
	}
	Str home = str_copy_cstr(getenv("HOME"), perm);
	if (!home.v) {
		fprintf(stderr, "%s: HOME environment variable not set.\n", argv0);
		return 1;
	}
	Str template = path_concat(home, S(".config/cao/templates"), perm);
	template = path_concat(template, STR(argv[1]), perm);
	char *ntermed = str_null_terminate(template, perm).v;
	if (!file_exists(ntermed)) {
		fprintf(stderr, "%s: template '%s' ('%s') doesnt exist\n", argv0, basename(ntermed), ntermed);
		return 1;
	}
	if (mkdir(argv[2], 0755) == -1) {
		if (errno == EEXIST) {
			if (force)
				goto forced;
			fprintf(stderr, "%s: Directory '%s' already exists, use -f to create project anyway.\n", argv0, argv[2]);
		} else {
			fprintf(stderr, "%s: mkdir: %s\n", argv0, strerror(errno));
		}
		return 1;
	}
forced:
	if (chdir(argv[2]) == -1) {
		fprintf(stderr, "%s: chdir: %s\n", argv0, strerror(errno));
		return 1;
	}

	DirEnt *files = all_files(template, 0, S("."), perm);
	Str subst = path_basename(str_cwd(perm));
	for (DirEnt *node = files; node; node = node->next) {
		if (node->is_dir) {
			if (mkdir(str_null_terminate(node->bname, perm).v, 0755) != -1) {
				fprintf(stderr, "%s: created directory '%.*s'\n", argv0, (int)node->bname.len, node->bname.v);
			} else {
				fprintf(stderr, "%s: mkdir: %s\n", argv0, strerror(errno));
			}
		} else {
			if (copy_over_and_subst(node->bname, node->fname, subst, scratch)) {
				fprintf(stderr, "%s: created file '%.*s'\n", argv0, (int)node->bname.len, node->bname.v);
			}
		}
	}
	return 0;
}

int cmd_build(char **argv, Arena *perm, Arena scratch)
{
	int ret = 0;
	char *make_cmd[] = {"make", 0};
	if (argv[1]) {
		fprintf(stderr, "cao: build: invalid option -- '%s'", argv[1]);
	}
	if (file_exists("Makefile") || file_exists("makefile")) {
		ret = os_command(make_cmd, scratch, argv0);
	} else {
		fprintf(stderr, "%s: no build system found\n", argv0);
	}
	return ret;
}

int cmd_run(char **argv, Arena *perm, Arena scratch)
{
	int ret = cmd_build((char *[]){"build", 0}, perm, scratch);
	if (ret)
		return ret;

	Str path = str_concat(S("./build/"), path_basename(str_cwd(perm)), perm);
	path = str_null_terminate(path, perm);
	if (file_exists(path.v)) {
		char *old = argv[0];
		argv[0] = path.v;
		ret = os_command(argv, scratch, argv0);
		argv[0] = old;
	}

	return ret;
}

int cmd_version(char **argv, Arena *perm, Arena scratch)
{
	(void)argv;
	(void)perm;
	(void)scratch;
	printf("cao version 0.1\n");
	return 0;
}

int cmd_help(char **argv, Arena *perm, Arena scratch)
{
	struct {
		char *name, *help;
	} help_msgs[] = {
	    {"help",
	     "usage: %s help <subcommand>\n\n"
	     "displays help for <subcommand>, or for the program if no subcommand was specified.\n"},
	    {"version",
	     "usage: %s version\n\n"
	     "prints version information and exits.\n"},
	    {"build",
	     "usage: %s build\n\n"
	     "builds the current project.\n"},
	    {"run",
	     "usage: %s run\n\n"
	     "builds and runs the current project.\n"},
	    {"new",
	     "usage: %s new <template> <project>\n\n"
	     "creates a new directory called <project> and copies the contents of the template <template>\n"
	     "which is located at ~/.config/cao.\n\n"
	     "Options:\n"
	     "  -f, --force : create project even if <project> is non-empty\n"}};
	int i, nmsgs = sizeof(help_msgs) / sizeof(*help_msgs);
	(void)perm;
	(void)scratch;
	if (!argv[1]) {
		printf("usage: %s [OPTIONS] [COMMAND] ...\n\n"
		       "Options:\n"
		       "  -C, --directory <DIR> : change directory to <DIR>\n"
		       "  -h, --help            : print this help and exit\n"
		       "  -V, --version         : print version information and exit\n\n"
		       "Commands:\n"
		       "  build   : build the current project\n"
		       "  run     : build and run the current project\n"
		       "  new     : create a new project\n"
		       "  help    : print help for a subcommand\n"
		       "  version : print version information and exit\n\n"
		       "Try '%s help <subcommand>' for help with a specific subcommand.\n",
		       argv0, argv0);
		return 0;
	}
	if (argv[2]) {
		fprintf(stderr, "%s: unexpected argument -- '%s'\n", argv0, argv[2]);
		return 1;
	}
	for (i = 0; i < nmsgs; i++) {
		if (!strcmp(help_msgs[i].name, argv[1])) {
			printf(help_msgs[i].help, argv0);
			break;
		}
	}
	if (i == nmsgs) {
		fprintf(stderr, "%s: invalid subcommand -- '%s'\n", argv0, argv[1]);
		fprintf(stderr, "usage: %s help <subcommand>\n", argv0);
		return 1;
	}
	return 0;
}

void try_help(char *argv0)
{
	fprintf(stderr, "Try %s help for more information.\n", argv0);
}

int main(int argc, char **argv)
{
	Arena perm = arena_create(GB(1));
	Arena scratch = arena_create(GB(1));
	struct optparse options;
	char **subargv;
	int option, ret = 0, i;
	struct optparse_long longopts[] = {
	    {"directory", 'C', OPTPARSE_REQUIRED},
	    {"help", 'h', OPTPARSE_NONE},
	    {"version", 'V', OPTPARSE_NONE},
	    {0}};
	static const struct {
		char name[8];
		int (*cmd)(char **, Arena *, Arena);
	} cmds[] = {
	    {"help", cmd_help},
	    {"version", cmd_version},
	    {"build", cmd_build},
	    {"run", cmd_run},
	    {"new", cmd_new}};
	int ncmds = sizeof(cmds) / sizeof(*cmds);
	(void)argc;
	argv0 = argv[0];

	optparse_init(&options, argv);
	options.permute = 0;
	while ((option = optparse_long(&options, longopts, NULL)) != -1) {
		switch (option) {
		case 'C':
			chdir(options.optarg);
			break;
		case 'h':
			return cmd_help(argv, &perm, scratch);
		case 'V':
			return cmd_version(argv, &perm, scratch);
		case '?':
			fprintf(stderr, "%s: %s\n", argv[0], options.errmsg);
			try_help(argv[0]);
			return 1;
		}
	}

	subargv = argv + options.optind;
	if (!*subargv) {
		fprintf(stderr, "%s: missing subcommand\n", argv[0]);
		try_help(argv[0]);
		return 1;
	}
	for (i = 0; i < ncmds; i++) {
		if (!strcmp(cmds[i].name, subargv[0])) {
			ret = cmds[i].cmd(subargv, &perm, scratch);
			break;
		}
	}
	if (i == ncmds) {
		fprintf(stderr, "%s: invalid subcommand -- '%s'\n", argv[0], subargv[0]);
		try_help(argv[0]);
		return 1;
	}

	arena_destroy(&perm);
	arena_destroy(&scratch);
	return ret;
}
