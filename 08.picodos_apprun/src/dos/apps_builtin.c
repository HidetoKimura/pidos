// apps_builtin.c
#include "dos/apps_builtin.h"
#include "dos/dos.h"
#include "dos/dos_sys.h"
#include "util/strutil.h"
#include <string.h>

static int app_hello(int argc, char** argv) {
	(void)argc; (void)argv;
	dos_printf("HELLO from PicoDOS app!\n");
	return 0;
}

static int app_hexdump(int argc, char** argv) {
	dos_printf("HEXDUMP (demo) argc=%d\n", argc);
	for (int i=0;i<argc;i++) dos_printf("  argv[%d]=%s\n", i, argv[i]);
	return 0;
}

typedef int (*app_main_t)(int argc, char** argv);
typedef struct { const char* name; app_main_t fn; } app_ent_t;

static const app_ent_t g_apps[] = {
	{ "HELLO",   app_hello },
	{ "HEXDUMP", app_hexdump },
};

bool apps_builtin_run(const char* name, int argc, char** argv) {
	char tmp[32];
	strncpy(tmp, name, sizeof(tmp)-1);
	tmp[sizeof(tmp)-1] = '\0';
	str_to_upper(tmp);

	for (unsigned i=0; i<sizeof(g_apps)/sizeof(g_apps[0]); i++) {
		if (str_eq_nocase(tmp, g_apps[i].name)) {
			g_apps[i].fn(argc, argv);
			return true;
		}
	}
	return false;
}

