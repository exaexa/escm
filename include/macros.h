
/*
 * This is here because we need a simple method how to create
 * C++ selectors/lambdas. Basic problems are:
 * 1] evaluation of parameters. Repeating code should be standartized
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

#define escm_macro_handler(name,code) \
void name (scm_env* escm_environment, pair* code)

#define arg_count (pair_p(escm_arglist)?((pair*)escm_arglist)->list_length():0)

#define has_arg (pair_p(escm_arglist)?true:false)
#define has_tail_arg (escm_arglist&&(!pair_p(escm_arglist)))

#define pop_arg() \
({scm* var=escm_arglist?((pair*)escm_arglist)->a:0; if(escm_arglist) escm_arglist=((pair*)escm_arglist)->d;var;})
#define pop_tail_arg() \
({scm* var=escm_arglist; escm_arglist=0;var;})

#define pop_arg_type(type) \
({type* var=escm_arglist?dynamic_cast<type*>(((pair*)escm_arglist)->a):0;\
escm_arglist=((pair*)escm_arglist)->d;var;})
#define pop_tail_arg_type(type) \
({type* var=escm_arglist?dynamic_cast<type*>(escm_arglist):0;\
escm_arglist=0;var;})

#define return_scm(x) escm_environment->ret(x)
#define return_macro_code(x) (escm_environment->val=(x))

#define set_return(x) (escm_environment->val=(x))
#define do_return escm_environment->pop_cont()

#define throw_scm(x) escm_environment->throw_exception(x)
#define throw_string(x) escm_environment->throw_string_exception(x)
#define throw_str_scm(st,sc) escm_environment->throw_desc_exception(st,sc)

#define create_scm(type,params...) new_scm(escm_environment,type,##params)

#define escm_add_syntax_handler(e,name,h)\
(e)->add_global((name),new_scm((e),extern_syntax,(h)))

#define escm_add_func_handler(e,name,h)\
(e)->add_global((name),new_scm((e),extern_func,(h)))

#endif

