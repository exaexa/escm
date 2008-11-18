
#include <iostream>
#include <fstream>
using std::ofstream;

#include "env.h"
#include "builtins.h"

#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <readline/readline.h>
#include <readline/history.h>

scm_env*g_env;

static void fatal_err_handler (scm_env*e, scm*err)
{
	printf ("*** Fatal error in escm: ");
	printf (err->display().c_str() );
	printf ("\n");
}

/*
 * if we want to build lib, leave this alone, maybe call it.
 * -Drun_interpreter=main to use this as a parser main.
 * Maybe just don't compile this file at all.
 */

static void sigsegv_handler(int s, siginfo_t*info, void*d)
{
	printf("\n -- escm caught SIGSEGV --\n");
	printf("signo %d errno %d code %d pid %d uid %d status %d\n",
		info->si_signo, info->si_errno, info->si_code,
		info->si_pid, info->si_uid, info->si_status);
	printf("posix %d %p memory %p fd %d\n",
		info->si_int, info->si_ptr,
		info->si_addr, info->si_fd);
	
	printf("Scheme Heap is in ./faillog.txt\n");
	scm*t;
	ofstream f("./faillog.txt");
	for(set<scm*>::iterator i=g_env->collector.begin();
		i!=g_env->collector.end();i++) {
		t=dynamic_cast<scm*>(*i);

		f	<<(void*)*i
			<<"\t"
			<<(t?(t->display().c_str()):"*** CORRUPTED ***")
			<<std::endl;
	}

	f.close();
	
	_exit(1);
}

static void setup_sigsegv_handler()
{
	struct sigaction sa;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags=SA_SIGINFO;
	sa.sa_sigaction=sigsegv_handler;
	sigaction(SIGSEGV,&sa,0);
}

int run_interpreter (int argc, char**argv)
{
	scm_env e;

	g_env=&e;
	setup_sigsegv_handler();

	e.init ();

	if (!escm_add_scheme_builtins (&e) ) return 1;
	e.fatal_error = fatal_err_handler;

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

			err = e.eval_string (c, '\n');
			if (err) {
				printf ("** parse error: %s\n",
					e.parser->get_parse_error (err) );
				break;
			}

		} while (!e.evaluating() ) ;

		e.run_eval_loop();

		printf (" => ");
		printf (e.get_last_result()->display().c_str() );

		printf ("\n");
	}
}
