
/*
 * This is here because we need a simple method how to create
 * C++ selectors/lambdas. Basic problems are:
 * 1] evaluation of parameters. Repeating code, should be standartized
 * 	in the best possible way (here)
 * 2] storing/retrieving global variables, adding functions to scheme machine
 * 3] some simple way to check argument types. Most people just want it.
 */

#ifndef _ESCM_MACROS_
#define _ESCM_MACROS_

#include "env.h"

void escm_add_global (scm_env*e, const char*name, scm*data);

#define escm_func_handler(name) \
void name (scm_env* escm_environment, scm* escm_arglist)

#define arg_count (pair_p(escm_arglist)?((pair*)escm_arglist)->list_length():0)

#define has_arg (pair_p(escm_arglist)?true:false)
#define has_tail_arg (escm_arglist&&(!pair_p(escm_arglist)))

#define pop_arg(var) \
var=((pair*)escm_arglist)->a; escm_arglist=((pair*)escm_arglist)->d;
#define pop_tail_arg(var) \
var=escm_arglist; escm_arglist=0;

#define pop_arg_type(var,type) \
var=dynamic_cast<type>(((pair*)escm_arglist)->a);\
escm_arglist=((pair*)escm_arglist)->d;
#define pop_tail_arg_type(var,type) \
var=dynamic_cast<type>(escm_arglist); escm_arglist=0;


#define escm_add_syntax_handler(e,name,h)\
escm_add_global(e,name,new_scm(e,extern_syntax,h))

#define escm_add_func_handler(e,name,h)\
escm_add_global(e,name,new_scm(e,extern_func,h))

#endif

