
#include "env.h"
#include "builtins.h"
#include <stdio.h>


//if we want to build lib, leave this alone, maybe call it.
//If we don't want a lib, then -Drun_interpreter=main
int run_interpreter (int argc, char**argv)
{
	scm_env e;

	escm_add_scheme_builtins (&e);

	return 0;
}
