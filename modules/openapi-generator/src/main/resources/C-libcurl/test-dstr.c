#include <stdio.h>
#include <string.h>

#include "dstr.h"

int
main (int   argc,
      char *argv[])
{
	// Test concatenating all arguments
	struct dstr cmdline = dstr_init();

	// Test separate init + fini for each argument
	// while also appending arguments into cmdline
	for (int i = 0; i < argc; i++) {
		size_t len = strlen(argv[i]);
		struct dstr tmp1 = dstr_init();
		struct dstr tmp2 = dstr_init_reserve(len);

		dstr_set(&tmp1, argv[i], len);
		dstr_set(&tmp2, argv[i], len);

		printf("%s\033[m  %s\n", (const char [2][8]){
			"\033[31mEE",
			"\033[32mOK",
		}[!strcmp(dstr_get(&tmp1), dstr_get(&tmp2))], dstr_get(&tmp1));

		dstr_fini(&tmp1);
		dstr_fini(&tmp2);

		if (i > 0)
			dstr_cat(&cmdline, " ", 1U);

		dstr_cat(&cmdline, argv[i], len);
	}

	puts(dstr_get(&cmdline));
	dstr_fini(&cmdline);

	return 0;
}
