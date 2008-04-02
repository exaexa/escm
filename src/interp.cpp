
#include "env.h"
#include "builtins.h"

#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>

/*
 * if we want to build lib, leave this alone, maybe call it.
 * -Drun_interpreter=main to use this as a parser main.
 * Maybe just don't compile this file at all.
 */

int run_interpreter (int argc, char**argv)
{
	scm_env e;
	e.init (0, 32768, 4);

	if (!escm_add_scheme_builtins (&e) ) return 1;

	char* c;
	bool firstline = true;
	int err;

	printf (" -- escm "ESCM_VERSION_STR" --\n");

	while (1) {
		firstline = true;
		do {
			c = readline (firstline ? "> " : " | ");
			firstline = false;

			if (!c) {
				printf ("\n -- escm terminating. --\n");
				e.release ();
				return 0;
			}

			add_history (c);

			err = e.eval_string (c);
			if (err) {
				printf ("** parse error: %s\n",
					e.parser->get_parse_error (err) );
				break;
			}
			e.eval_string ("\n"); //because it's really typed

		} while (!e.evaluating() ) ;

		e.run_eval_loop();

		printf (" => ");
		printf (e.get_last_result()->display().c_str() );

		printf ("\n");
	}
}
