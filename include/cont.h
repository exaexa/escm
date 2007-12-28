
#ifndef _ESCM_CONT_
#define _ESCM_CONT_

#include "types.h"


//evaluates list of expressions
class codevector_continuation : public continuation
{
public:
	pair* ip;
	inline codevector_continuation (scm_env*e, pair*code)
			: continuation (e)
	{
		ip = code;
	}

	virtual scm*get_child (int i)
	{
		switch (i) {
		case 0:
			return ip;
		case 1:
			return parent;
		case 2:
			return env;
		default:
			return scm_no_more_children;
		}
	}

	virtual void eval_step (scm_env*);
};

//evaluates a non-pair (very simple), or transforms itself into pair_cont
class eval_continuation : public continuation
{
public:
	scm* object;
	inline eval_continuation (scm_env*e, scm*o)
			: continuation (e)
	{
		object = o;
	}

	virtual scm*get_child (int i)
	{
		switch (i) {
		case 0:
			return object;
		case 1:
			return parent;
		case 2:
			return env;
		default:
			return scm_no_more_children;
		}
	}

	virtual void eval_step (scm_env*);
};

//evaluates a list as a function call/syntax. Evaluates params.
//May trigger some error like 'invalid selector type'
class pair_continuation : public continuation
{
public:
	pair* list;

	bool selector_evaluated;

	inline pair_continuation (scm_env*e, pair*l)
			: continuation (e)
	{
		list = l;
		selector_evaluated = false;
	}

	//phases: if s_e==0, we assume that we need to evaluate the selector
	//else val==evaluated selector;)

	virtual scm* get_child (int i)
	{
		switch (i) {
		case 0:
			return list;
		case 1:
			return parent;
		case 2:
			return env;
		default:
			return scm_no_more_children;
		}
	}

	virtual void eval_step (scm_env*);
};

//for syntax. evaluates the syntax, replaces itself with standart eval.
class syntax_continuation : public continuation
{
public:
	syntax*syn; //pre-evaluated syntax pointer
	pair*code; //code to transform

	inline syntax_continuation (scm_env*e, syntax*s, pair*c)
			: continuation (e)
	{
		syn = s;
		code = c;
	}

	virtual scm* get_child (int i)
	{
		switch (i) {
		case 0:
			return syn;
		case 1:
			return code;
		case 2:
			return parent;
		case 3:
			return env;
		default:
			return scm_no_more_children;
		}
	}

	virtual void eval_step (scm_env*);
};

/*
 * for function calls, evaluates all arguments,
 * then replaces itself with a call
 *
 * Problem is the whole argument list evaluation thing,
 * solved by the arglist_pos flag
 * 0] beginning, don't save val
 * 1] middle, save val to the next list element
 * 2] end, save val to cdr(tail)
 *
 * if arglist is empty after this, we can call.
 */
class lambda_continuation : public continuation
{
public:
	lambda *l;
	pair *arglist;
	int arglist_pos;
	pair *evaluated_args;
	pair **evaluated_args_d;

	inline lambda_continuation (scm_env*e,
				    lambda*lam, pair*code,
				    bool with_selector = true)
			: continuation (e)
	{
		l = lam;
		evaluated_args = 0;
		evaluated_args_d = &evaluated_args;

		if (with_selector) arglist = (pair*) (code->d);
		else arglist = code;
		arglist_pos = 0;
		//we should examine real type of arglist later,
		//but we can be sure that pair_p(code)
	}

	virtual scm* get_child (int i)
	{
		switch (i) {
		case 0:
			return l;
		case 1:
			return arglist;
		case 2:
			return evaluated_args;
		case 3:
			return parent;
		case 4:
			return env;

			/*NOTE - no need to mark *evaluated_args_d */

		default:
			return scm_no_more_children;
		}
	}

	virtual void eval_step (scm_env*);
};

/*
 * TODO, specific continuations for all 'special forms':
 * define set!
 * if cond case and or
 * map foreach do
 * let let* letrec
 *
 * (lambda is a syntax!)
 */

#endif

