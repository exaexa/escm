
#include "env.h"
#include "builtins.h"
#include "display.h"
#include <stdio.h>


//if we want to build lib, leave this alone, maybe call it.
//If we don't want a lib, then -Drun_interpreter=main
int run_interpreter (int argc, char**argv)
{
	scm_env e;

	escm_add_scheme_builtins (&e);

	char cmdbuffer[513];
	int n;
	while (1) {
		n = fread (cmdbuffer, 1, 512, stdin);
		cmdbuffer[n] = 0;
		e.eval_string (cmdbuffer);
		e.run_eval_loop();
		printf ("=> ");
		escm_display_to_stdout (e.get_last_result(), false);
		printf ("\n");
	}

	return 0;
}
