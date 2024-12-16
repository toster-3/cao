#include <ctype.h>
#ifdef __linux
#include <linux/limits.h>
#endif
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "cao.h"
#include "logging.h"
#include "util.h"
#include "vec.h"

#define TRY(fn_name, ...)                                                      \
	do {                                                                       \
		if ((fn_name)(__VA_ARGS__) < 0) {                                      \
			log_error(#fn_name ":");                                           \
			die = 1;                                                           \
			goto cleanup;                                                      \
		}                                                                      \
	} while (0)

#define TRY_MSG(msg, fn_name, ...)                                             \
	do {                                                                       \
		if ((fn_name)(__VA_ARGS__) < 0) {                                      \
			log_error(msg);                                                    \
			die = 1;                                                           \
			goto cleanup;                                                      \
		}                                                                      \
	} while (0)

enum template_type {
	TPLT_FILE,
	TPLT_DIR,
	TPLT_COMMENT,
};

static int dir_exists(const char *path)
{
	struct stat st;
	if (stat(path, &st) == 0) {
		return S_ISDIR(st.st_mode);
	}
	return 0;
}

static void trim_upper(char *buf)
{
	// p should be just before null byte
	char *p = buf + strlen(buf) - 1;
	while (1) {
		if (!isspace(*p))
			break;
		*p-- = '\0';
	}
}

static void parse_cao_template(const char *path, vec_str_t *files,
                               vec_str_t *dirs)
{
	FILE *fp = fopen(path, "rb");
	char buf[PATH_MAX];
	enum template_type type;

	vec_init(files);
	vec_init(dirs);
	while (fgets(buf, PATH_MAX, fp) != NULL) {
		trim_upper(buf);
		if (buf[0] == '#') {
			type = TPLT_COMMENT;
		} else if (buf[strlen(buf) - 1] == '/') {
			buf[strlen(buf) - 1] = '\0';
			type = TPLT_DIR;
		} else {
			type = TPLT_FILE;
		}

		switch (type) {
		case TPLT_FILE:
			vec_push(files, estrdupn(buf, PATH_MAX));
			break;
		case TPLT_DIR:
			vec_push(dirs, estrdupn(buf, PATH_MAX));
			break;
		case TPLT_COMMENT:
		default:
			break;
		}
	}
	fclose(fp);
}

static void cao_copy_files(char *cao_template, char *src, char *dest)
{
	vec_str_t files = {0}, dirs = {0};
	char *s, destfile[PATH_MAX], cmd[PATH_MAX + 32];
	size_t i;
	int die = 0;
	mode_t mode, mask;

	mask = umask(0);
	mode = 0777 & ~mask;

	parse_cao_template(cao_template, &files, &dirs);

	log_debug("src = `%s`, dest = `%s`", src, dest);

	TRY_MSG("could not make project directory:", mkdir, dest, mode);
	TRY(chdir, src);
	vec_foreach (&dirs, s, i) {
		pathprefix(dest, s);
		TRY(mkdir, s, mode);
	}

	vec_foreach (&files, s, i) {
		strcpy(destfile, s);
		pathprefix(dest, destfile);
		TRY(cp, s, destfile);
		/*
		 * FIXME: using system() is very yucky.
		 * We now depend on a shell, sed and the
		 * '-i' option which is non-posix
		 */
		sprintf(cmd, "sed -i s/PROJECT_NAME/%s/g %s", basename(dest), destfile);
		system(cmd);
	}

cleanup:
	vec_foreach (&files, s, i) {
		free(s);
	}
	vec_deinit(&files);
	vec_foreach (&dirs, s, i) {
		free(s);
	}
	vec_deinit(&dirs);
	if (die)
		exit(die);
}

void cao_new(int argc, char **argv)
{
	char *home;
	char thedir[PATH_MAX];
	char cao_template[PATH_MAX];
	char projdir[PATH_MAX];

	if (!argv[1]) {
		log_error("project name not given");
		exit(1);
	}

	home = getenv("HOME");
	if (!home) {
		log_error("HOME environment variable not set");
		exit(1);
	}

	strcpy(thedir, home);
	strcat(thedir, "/.config/cao");

	if (!dir_exists(thedir)) {
		log_error("directory '%s' which should have the project template "
		          "doesnt exist",
		          thedir);
		exit(1);
	}

	strcpy(cao_template, thedir);
	strcat(cao_template, "/cao_template");

	if (access(cao_template, F_OK) != 0) {
		log_error("project template at `%s` is missing", cao_template);
		exit(1);
	}

	getcwd(projdir, PATH_MAX);
	strcat(projdir, "/");
	strcat(projdir, argv[1]);

	cao_copy_files(cao_template, thedir, projdir);
	fprintf(stderr, "\x1b[32;1mCreated\x1b[0m `%s` project\n", argv[1]);
}
