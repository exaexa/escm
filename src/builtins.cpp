#include "builtins.h"
#include "macros.h"

#include <stdio.h>

/*
 * NUMBER FUNCTIONS
 */

escm_func_handler (op_add)
{
	scm*t;
	number*res = create_scm (number, 0);
	set_return (res->collectable<number>() );
	while (has_arg) {
		t = pop_arg();
		if (number_p (t) ) res->add ( (number*) t);
		else throw_str_scm ("not a number", t);
	}
	do_return;
}

escm_func_handler (op_sub)
{
	scm*t;
	number*res = create_scm (number, 0);
	set_return (res->collectable<number>() );

	if (has_arg) {
		t = pop_arg();
		if (!number_p (t) ) throw_str_scm ("not a number", t);
		else res->set ( (number*) t);
	}

	if (!has_arg) {
		res->neg();
		do_return;
		return;
	}

	while (has_arg) {
		t = pop_arg();
		if (!number_p (t) ) throw_str_scm ("not a number", t);
		else res->sub ( (number*) t);
	}

	do_return;
}

escm_func_handler (op_mul)
{
	scm*t;
	number*res = create_scm (number, 1);
	set_return (res->collectable<number>() );
	while (has_arg) {
		t = pop_arg();
		if (number_p (t) ) res->mul ( (number*) t);
		else throw_str_scm ("not a number", t);
	}
	do_return;
}

escm_func_handler (op_div)
{
	number*res = create_scm (number, 1);
	set_return (res->collectable<number>() );
	scm*t;

	if (!has_arg) throw_string ("invalid division");
	t = pop_arg();
	if (!number_p (t) ) throw_str_scm ("not a number", t);
	if (!has_arg) {
		res->div ( (number*) t);
		do_return;
		return;
	}
	res->set ( (number*) t);
	while (has_arg) {
		t = pop_arg();
		if (number_p (t) ) res->div ( (number*) t);
		else throw_str_scm ("not a number", t);
	}

	do_return;
}

escm_func_handler (op_mod)
{
	number*res = create_scm (number, 0);
	set_return (res->collectable<number>() );

	number*a = pop_arg_type (number);
	number*b = pop_arg_type (number);

	if (a && b) {
		res->set (a);
		res->mod (b);
	} else throw_string ("invalid mod params");

	do_return;
}

escm_func_handler (op_pow)
{
	number*res = create_scm (number, 0);
	set_return (res->collectable<number>() );
	number*a = pop_arg_type (number);
	number*b = pop_arg_type (number);
	if (a && b) {
		res->set (a);
		res->pow (b);
	} else throw_string ("invalid pow params");

	do_return;
}

escm_func_handler (op_less)
{
	number*a = pop_arg_type (number);
	number*b = pop_arg_type (number);
	if (a && b)
		return_scm (a->is_less (b) ?
			    escm_environment->t_true : escm_environment->t_false);
	else throw_string ("can't compare");
}

escm_func_handler (op_greater)
{
	number*a = pop_arg_type (number);
	number*b = pop_arg_type (number);
	if (a && b)
		return_scm (a->is_greater (b) ?
			    escm_environment->t_true : escm_environment->t_false);
	else throw_string ("can't compare");
}

escm_func_handler (op_equal)
{
	number*a = pop_arg_type (number);
	number*b = pop_arg_type (number);
	if (a && b)
		return_scm (a->is_equal (b) ?
			    escm_environment->t_true : escm_environment->t_false);
	else throw_string ("can't compare");
}

escm_func_handler (op_not_less)
{
	number*a = pop_arg_type (number);
	number*b = pop_arg_type (number);
	if (a && b)
		return_scm (a->is_less (b) ?
			    escm_environment->t_false : escm_environment->t_true);
	else throw_string ("can't compare");
}

escm_func_handler (op_not_greater)
{
	number*a = pop_arg_type (number);
	number*b = pop_arg_type (number);
	if (a && b)
		return_scm (a->is_greater (b) ?
			    escm_environment->t_false : escm_environment->t_true);
	else throw_string ("can't compare");
}

escm_func_handler (op_not_equal)
{
	number*a = pop_arg_type (number);
	number*b = pop_arg_type (number);
	if (a && b)
		return_scm (a->is_equal (b) ?
			    escm_environment->t_false : escm_environment->t_true);
	else throw_string ("can't compare");
}

escm_func_handler (op_log)
{
	number*res = create_scm (number, 0);
	set_return (res->collectable<number>() );

	number*a = pop_arg_type (number);
	number*b = pop_arg_type (number);

	if (a && b) {
		res->set (a);
		res->log (b);
	} else if (a) {
		res->set (a);
		res->log (0);
	} else throw_string ("invalid log params");
	do_return;
}

escm_func_handler (op_exp)
{
	number*res = create_scm (number, 1);
	set_return (res->collectable<number>() );

	scm*a = pop_arg();
	if (number_p (a) ) res->set ( (number*) a);
	else throw_str_scm ("not a number", a);
	res->exp();

	do_return;
}

/*
 * QUOTE, EVAL and friends
 */

void op_quote (scm_env*e, pair*code)
{
	if (pair_p (code->d) ) e->ret ( ( (pair*) (code->d) )->a );
}

void op_eval (scm_env*e, scm*args)
{
	if (pair_p (args) ) e->replace_cont
		(new_scm (e, eval_continuation,
			  ( (pair*) args)->a)->collectable<continuation>() );
}

void op_apply (scm_env*e, scm*args)
{
	scm* func;
	if (pair_p (args) ) {
		func = ( (pair*) args)->a;
		args = ( (pair*) args)->d;
		if (func && pair_p (args) ) {
			if (lambda_p (func) ) ( (lambda*) func)->
				apply (e, ( (pair*) args)->a);
			return;
		}
	}
	e->throw_string_exception ("bad apply syntax");
}

/*
 * DEFINEs, SETs
 */

class define_continuation : public continuation
{
public:
	enum {type_define, type_set};
	int type;
	scm* data;
	bool evaluated;

	define_continuation (scm_env*e, scm*d, int t) : continuation (e)
	{
		data = d;
		type = t;
		evaluated = false;
	}

	virtual scm* get_child (int i)
	{
		switch (i) {
		case 0:
			return data;
		case 1:
			return parent;
		case 2:
			return env;
		default:
			return escm_no_more_children;
		}
	}

	void eval_step (scm_env*e);
};

#include <stdio.h>

void define_continuation::eval_step (scm_env*e)
{
	if (evaluated) { //second stage, after eval.
		switch (type) {
		case type_define:
			e->lexdef ( (symbol*) data);
			break;
		case type_set:
			e->lexset ( (symbol*) data);
			break;
		default:
			e->throw_string_exception
			("internal error in define-cont");
			break;
		}
		e->val = data;
		e->pop_cont();
	} else { //first stage
		scm*name = 0;

		if (!pair_p (data) )
			e->throw_desc_exception ("invalid definition", data);

		name = ( (pair*) data)->a;
		data = ( (pair*) data)->d;

		if (symbol_p (name) ) { // (define symbol wut)

			if (!pair_p (data) )
				e->throw_desc_exception
				("invalid definition", data);

			e->push_cont (new_scm (e, eval_continuation,
					       ( (pair*) data)->a)
				      ->collectable<continuation>() );

			evaluated = true;
			data = name;

		} else if (pair_p (name) ) { // (define (sym lambda) wut wut wut)

			if (!symbol_p ( ( (pair*) name)->a) )
				e->throw_desc_exception
				("invalid definition", name);

			e->val = new_scm (e, closure, ( (pair*) name)->d,
					  pair_p (data), env)->collectable<closure>();
			data = ( (pair*) name)->a;
			evaluated = true;

		} else e->throw_desc_exception
			("invalid definition target", name);
	}
}

void op_define (scm_env*e, pair*code)
{
	e->replace_cont (new_scm (e, define_continuation, code->d,
				  define_continuation::type_define)
			 ->collectable<continuation>() );
}

void op_set (scm_env*e, pair*code)
{
	e->replace_cont (new_scm (e, define_continuation, code->d,
				  define_continuation::type_set)
			 ->collectable<continuation>() );
}

/*
 * LAMBDA
 */

void op_lambda (scm_env*e, pair*code)
{
	pair* c = pair_p (code->d);
	e->ret (new_scm (e, closure, c->a, (pair*)(c->d), e->cont->env)
		->collectable<scm>() );
}

/*
 * MACRO
 */

void op_macro (scm_env*e, pair*code)
{
	symbol*name = 0;
	symbol*argname = 0;
	pair*macro_code = 0;
	code = pair_p (code->d);
	if (!code) goto error;
	macro_code = pair_p (code->d);
	if (pair_p (code->a) ) {
		code = (pair*) (code->a);
		if (pair_p (code->d) ) {
			name = symbol_p (code->a);
			argname = symbol_p ( ( (pair*) (code->d) )->a);
		} else {
			argname = symbol_p (code->a);
		}
		if (argname) {
			macro*m = new_scm (e, macro, macro_code, argname);
			if (name) {
				e->val = m->collectable<scm>();
				e->lexdef (name);
				e->ret (name);
			} else {
				e->ret (m->collectable<scm>() );
			}
		} else goto error;
	}
	return;
error:
	e->throw_string_exception ("bad macro syntax");
}

/*
 * LISTs
 */

escm_func_handler (op_list)
{
	return_scm (escm_arglist);
}

escm_func_handler (op_car)
{
	scm*p = pop_arg();
	if (!pair_p (p) ) return throw_str_scm ("not a pair", p);
	return_scm ( ( (pair*) p)->a);
}

escm_func_handler (op_cdr)
{
	scm*p = pop_arg();
	if (!pair_p (p) ) return throw_str_scm ("not a pair", p);
	return_scm ( ( (pair*) p)->d);
}

escm_func_handler (op_cons)
{
	scm*a, *d;
	if (!has_arg) goto error;
	a = pop_arg();
	if (!has_arg) goto error;
	d = pop_arg();
	if (has_arg) goto error;
	return_scm (create_scm (pair, a, d)->collectable<pair>() );
	return;
error:
	throw_string ("cons needs 2 arguments");
}

/*
 * DISPLAY (subject to remove or change brutally)
 */

escm_func_handler (op_display)
{
	printf ( pop_arg()->display (1).c_str() );
	return_scm (0);
}

escm_func_handler (op_newline)
{
	printf ("\n");
	return_scm (0);
}

/*
 * TYPE PREDICATES
 */

void op_null_p (scm_env*e, scm*arglist)
{
	if (!pair_p (arglist) ) e->ret (0);
	else if ( ( (pair*) arglist)->a) e->ret (e->t_false);
	else e->ret (e->t_true);
}

/*void op_pair_p (scm_env*e, scm*arglist)
{
	if (!pair_p (arglist) ) e->ret (0);
	else if (pair_p ( ( (pair*) arglist)->a) ) e->ret (e->t_true);
	else e->ret (e->t_false);
}*/

#define create_predicate_function(type) \
void op_##type##_p (scm_env*e, scm*arglist) \
{ \
	if (!pair_p (arglist) ) e->ret (0); \
	else if (type##_p ( ( (pair*) arglist)->a) ) e->ret (e->t_true); \
	else e->ret (e->t_false); \
}

create_predicate_function (pair)
create_predicate_function (atom)
create_predicate_function (number)
create_predicate_function (character)
create_predicate_function (boolean)
create_predicate_function (string)
create_predicate_function (symbol)
create_predicate_function (lambda)
create_predicate_function (syntax)
create_predicate_function (continuation)

/*
 * BOOLEAN OPERATIONS
 */

static escm_func_handler (op_true_p)
{
	boolean*b = (boolean*) pop_arg();
	if (!boolean_p (b) ) throw_str_scm ("not a boolean", b);
	if (b->b) return_scm (escm_environment->t_true);
	else return_scm (escm_environment->t_false);
}

static escm_func_handler (op_false_p)
{
	boolean*b = (boolean*) pop_arg();
	if (!boolean_p (b) ) throw_str_scm ("not a boolean", b);
	if (b->b) return_scm (escm_environment->t_false);
	else return_scm (escm_environment->t_true);
}

static escm_func_handler (op_and_hard)
{
	boolean*a;
	while (has_arg) {
		a = (boolean*) pop_arg();
		if (!boolean_p (a) ) throw_str_scm ("not a boolean", a);
		if (! (a->b) ) {
			return_scm (escm_environment->t_false);
			return;
		}
	}
	return_scm (escm_environment->t_true);
}

static escm_func_handler (op_or_hard)
{
	boolean*a;
	while (has_arg) {
		a = (boolean*) pop_arg();
		if (!boolean_p (a) ) throw_str_scm ("not a boolean", a);
		if (a->b) {
			return_scm (escm_environment->t_true);
			return;
		}
	}
	return_scm (escm_environment->t_false);
}

static escm_func_handler (op_xor)
{
	boolean*a;
	bool result = false;
	while (has_arg) {
		a = (boolean*) pop_arg();
		if (!boolean_p (a) ) throw_str_scm ("not a boolean", a);
		if (a->b) result = !result;
	}
	return_scm (result ?
		    (escm_environment->t_true) : (escm_environment->t_false) );
}

/*
 * PROGRAM FLOW CONTROL
 */

void op_begin (scm_env*e, pair*code)
{
	e->replace_cont (new_scm (e, codevector_continuation, pair_p (code->d) )
			 ->collectable<continuation>() );
}

/* note to (begin)
 * if you (define a 1) (begin (define a 2)) then a is 2,
 * although it doesn't seem very logical. All major schemes
 * work this way. not a bug.
 */

/*
 * IF CONTINUATION
 * works this way:
 * 1] condition evaluation is pushed right in the constructor
 * 2] eval step is given a result of the cond, so therefore replaces
 * 	itself with true or false branch evaluation
 *
 * note: ANYTHING given IS true, unless it really IS false.
 */

class if_continuation : public continuation
{
public:
	scm *t, *f, *c;

	inline if_continuation (scm_env*e, scm*cond, scm*T, scm*F)
			: continuation (e)
	{
		t = T;
		f = F;
		c = cond;
	}

	virtual scm*get_child (int i)
	{
		switch (i) {
		case 0:
			return t;
		case 1:
			return f;
		case 2:
			return c;
		case 3:
			return parent;
		case 4:
			return env;
		default:
			return escm_no_more_children;
		}
	}

	virtual void eval_step (scm_env*e);
};

void if_continuation::eval_step (scm_env*e)
{
	if (c) {
		e->push_cont (new_scm (e, eval_continuation, c)
			      ->collectable<continuation>() );
		c = 0;
		return;
	}
	scm*res = t;
	if (boolean_p (e->val) ) if (! ( ( (boolean*) (e->val) )->b) ) res = f;
	e->replace_cont (new_scm (e, eval_continuation, res)
			 ->collectable<continuation>() );
}

void op_if (scm_env*e, pair*code)
{
	pair*p = pair_p (code->d);
	scm*cond = 0, *t = 0, *f = 0;
	if (p) {
		cond = p->a;
		p = pair_p (p->d);
	}
	if (p) {
		t = p->a;
		p = pair_p (p->d);
	}
	if (p) {
		f = p->a;
		p = pair_p (p->d);
	}
	e->replace_cont (new_scm (e, if_continuation, cond, t, f)
			 ->collectable<continuation>() );
}


/*
 * ERROR HANDLING
 *
 * not rly moar?
 */

escm_func_handler (op_error)
{
	throw_scm (escm_arglist);
}

/*
 * CONT factories for env
 */

static continuation* default_eval_factory (scm_env*e, scm*s)
{
	return new_scm (e, eval_continuation, s)
	       ->collectable<eval_continuation>();
}

static continuation* default_cv_factory (scm_env*e, pair*s)
{
	return new_scm (e, codevector_continuation, s)
	       ->collectable<codevector_continuation>();
}

/*
 * strings
 */

static escm_func_handler (op_str_compare)
{
	text*a, *b;
	a = pop_arg_type (text);
	b = pop_arg_type (text);
	if (a && b) return_scm (create_scm (number, a->cmp (b) )->collectable<scm>() );
	else throw_string ("need 2 text args to compare");
}

/*
 * internals
 */

static escm_func_handler (op_gc)
{
	escm_environment->collect_garbage();
	return_scm (0);
}

static escm_func_handler (op_cc)
{
	return_scm(escm_environment->cont);
}

static escm_func_handler (op_env)
{
	return_scm(escm_environment->cont->env);
}

/*
 * file loader
 */

static bool load_file (const char* fn, scm_env*e)
{
	FILE*f = fopen (fn, "r");
	if (!f) return false;
	char buf[65];
	int l;
	while ( (l = fread (buf, 1, 64, f) ) ) {
		buf[l] = 0;
		if (e->parser->parse_string (buf) ) {
			fclose (f);
			return false;
		}
	}
	fclose (f);
	pair*code = e->parser->get_result (false);
	if (code) e->eval_code (code->collectable<pair>() );
	e->run_eval_loop();
	return true;
}

/*
 * General add-all-the-shaite function
 */

static bool load_init_file (scm_env*e)
{
	char buffer[1024];
	strcpy (buffer, getenv ("HOME") );
	strcat (buffer, "/.escm-init");
	if (load_file (buffer, e) ) return true;
	return load_file ("/etc/escm-init", e);
}

bool escm_add_scheme_builtins (scm_env*e)
{
	e->eval_cont_factory = default_eval_factory;
	e->codevector_cont_factory = default_cv_factory;

	//arithmetics
	try {
		escm_add_func_handler (e, "+", op_add);
		escm_add_func_handler (e, "-", op_sub);
		escm_add_func_handler (e, "*", op_mul);
		escm_add_func_handler (e, "/", op_div);
		escm_add_func_handler (e, "modulo", op_mod);
		escm_add_func_handler (e, "%", op_mod);
		escm_add_func_handler (e, "expt", op_pow);
		escm_add_func_handler (e, "log", op_log);
		escm_add_func_handler (e, "pow-e", op_exp);

		escm_add_func_handler (e, "=", op_equal);
		escm_add_func_handler (e, "<", op_less);
		escm_add_func_handler (e, ">", op_greater);
		escm_add_func_handler (e, "!=", op_not_equal);
		escm_add_func_handler (e, ">=", op_not_less);
		escm_add_func_handler (e, "<=", op_not_greater);

		//basic scheme
		escm_add_syntax_handler (e, "quote", op_quote);
		escm_add_func_handler (e, "eval", op_eval);
		escm_add_func_handler (e, "apply", op_apply);

		escm_add_syntax_handler (e, "define", op_define);
		escm_add_syntax_handler (e, "set!", op_set);

		escm_add_syntax_handler (e, "lambda", op_lambda);
		escm_add_syntax_handler (e, "macro", op_macro);

		escm_add_syntax_handler (e, "if", op_if);

		escm_add_syntax_handler (e, "begin", op_begin);

		//lists
		escm_add_func_handler (e, "list", op_list);
		escm_add_func_handler (e, "cons", op_cons);
		escm_add_func_handler (e, "car", op_car);
		escm_add_func_handler (e, "cdr", op_cdr);

		//types
		escm_add_func_handler (e, "null?", op_null_p);
		escm_add_func_handler (e, "atom?", op_atom_p);
		escm_add_func_handler (e, "pair?", op_pair_p);
		escm_add_func_handler (e, "number?", op_number_p);
		escm_add_func_handler (e, "character?", op_character_p);
		escm_add_func_handler (e, "boolean?", op_boolean_p);
		escm_add_func_handler (e, "string?", op_string_p);
		escm_add_func_handler (e, "symbol?", op_symbol_p);
		escm_add_func_handler (e, "lambda?", op_lambda_p);
		escm_add_func_handler (e, "syntax?", op_syntax_p);
		escm_add_func_handler (e, "continuation?", op_continuation_p);

		//booleans
		escm_add_func_handler (e, "true?", op_true_p);
		escm_add_func_handler (e, "false?", op_false_p);
		escm_add_func_handler (e, "not", op_false_p);
		escm_add_func_handler (e, "and!", op_and_hard);
		escm_add_func_handler (e, "or!", op_or_hard);
		escm_add_func_handler (e, "xor", op_xor);

		//I/O
		escm_add_func_handler (e, "display", op_display);
		escm_add_func_handler (e, "newline", op_newline);

		//Strings
		escm_add_func_handler (e, "str-cmp", op_str_compare);
		escm_add_func_handler (e, "sym-cmp", op_str_compare);

		//errors
		escm_add_func_handler (e, "error", op_error);

		//internals
		escm_add_func_handler (e, "gc", op_gc);
		escm_add_func_handler (e, "cc", op_cc);
		escm_add_func_handler (e, "env", op_env);

		//init file
		load_init_file (e);
	} catch (scm*) {
		return false;
	}
	return true;
}

