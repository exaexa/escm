#include "builtins.h"
#include "macros.h"

/*
 * NUMBER FUNCTIONS
 */

void op_add (scm_env*e, scm*params)
{
	number*res = new_scm (e, number, 0);
	pair*p = pair_p (params);
	e->val = res->collectable<number>();
	while (p) {
		if (number_p (p->a) )
			res->add (number_p (p->a) );
		else e->throw_desc_exception ("not a number", p->a);
		p = pair_p (p->d);
	}
	e->pop_cont();
}

void op_sub (scm_env*e, scm*params)
{
	number*res = new_scm (e, number, 0);
	pair*p = pair_p (params);
	e->val = res->collectable<number>();
	if (pair_p (p) ) {
		if (number_p (p->a) ) res->set (number_p (p->a) );
		p = pair_p (p->d);
		while (p) {
			if (number_p (p->a) )
				res->sub (number_p (p->a) );
			else e->throw_desc_exception ("not a number", p->a);
			p = pair_p (p->d);
		}
	}
	e->pop_cont();
}

void op_mul (scm_env*e, scm*params)
{
	number*res = new_scm (e, number, 1);
	pair*p = pair_p (params);
	e->val = res->collectable<number>();
	while (p) {
		if (number_p (p->a) )
			res->mul (number_p (p->a) );
		//else cast error
		p = pair_p (p->d);
	}
	e->pop_cont();
}

void op_div (scm_env*e, scm*params)
{
	number*res = new_scm (e, number, 1);
	e->val = res->collectable<number>();
	number*f = 0;
	pair*p = pair_p (params);
	if (p) {
		f = number_p (p->a);

		p = pair_p (p->d);
		if (p) {
			if (f) {
				res->set (f);
				if (number_p (p->a) ) res->div (number_p (p->a) );
			}
		} else if (f) res->div (f);
	}
	e->pop_cont();
}

#define two_number_func(name,func) \
void name(scm_env*e, scm*params) \
{ \
	number*res=new_scm(e,number,0); \
	e->val=res->collectable<number>(); \
 \
	pair*l=pair_p(params); \
	number*a=0,*b=0; \
	if(l){ \
		a=number_p(l->a); \
		l=pair_p(l->d); \
		if(l) b=number_p(l->a); \
	} \
	if(!(a&&b))return; \
	res->set(a); \
	res->func(b); \
	e->pop_cont(); \
}

two_number_func (op_mod, mod)
two_number_func (op_pow, pow)

void op_log (scm_env*e, scm*params)
{
	number*res = new_scm (e, number, 0);
	e->val = res->collectable<number>();

	pair*l = pair_p (params);
	number*a = 0, *b = 0;
	if (l) {
		a = number_p (l->a);
		l = pair_p (l->d);
		if (l) b = number_p (l->a);
	}

	if (a && b) {
		res->set (a);
		res->log (b);
	} else if (a) {
		res->set (a);
		res->log (0);
	}
	e->pop_cont();
}

void op_exp (scm_env*e, scm*params)
{
	number*res = new_scm (e, number, 1);
	e->val = res->collectable<number>();

	pair*p = pair_p (params);
	if (p) if (number_p (p->a) ) res->set (number_p (p->a) );
	res->exp();
	e->pop_cont();
}

/*
 * QUOTE, EVAL and friends
 */

static void op_quote (scm_env*e, pair*code)
{
	if (pair_p (code->d) ) e->val = pair_p (code->d)->a;
	e->pop_cont();
}

static void op_eval (scm_env*e, scm*args)
{
	if (pair_p (args) ) e->replace_cont
		(new_scm (e, eval_continuation,
			  ( (pair*) args)->a)->collectable<continuation>() );
}


/*
 * DEFINEs, SETs
 */

static void op_actual_define (scm_env*e, scm*params)
{
	e->val = 0;
	symbol*name;
	pair*p = pair_p (params);
	if (!p) goto except;
	name = symbol_p (p->a);
	if (!name) goto except;
	p = pair_p (p->d);
	if (!p) goto except;
	e->val = p->a;
	e->lexdef (name);
	e->ret (name);
	return;
except:
	e->throw_desc_exception ("bad define:", params);
}

static void op_define (scm_env*e, pair*code)
{
	code = pair_p (code->d);
	symbol*name;
	scm*def;
	if (!code) e->throw_string_exception ("invalid define");
	if (pair_p (code->a) ) { //defining a lambda, shortened syntax
		pair*l = (pair*) code->a;
		name = symbol_p (l->a);
		scm*lam = new_scm (e, symbol, "LAMBDA");
		scm*temp = new_scm (e, pair, l->d, code->d);
		def = new_scm (e, pair, lam, temp);
		//generates (lambda params . code)
	} else {
		name = symbol_p (code->a);
		if (pair_p (code->d) ) def = pair_p (code->d)->a;
		else def = code->d;
	}

	if (!name) e->throw_desc_exception
		("invalid define symbol format", code->a);

	//generates (#<op_actual_define> (QUOTE name) def)
	lambda*func = new_scm (e, extern_func, op_actual_define);
	scm*quoted_name = new_scm (e, pair, name, 0);
	scm*quote = new_scm (e, symbol, "QUOTE");
	quoted_name = new_scm (e, pair, quote, quoted_name);
	pair*params = new_scm (e, pair, def, 0);
	params = new_scm (e, pair, quoted_name, params);
	continuation*cont = new_scm (e, lambda_continuation,
				     func, params, false);
	e->replace_cont (cont);
	cont->mark_collectable();
}

/*
 * LAMBDA
 */

void op_lambda (scm_env*e, pair*code)
{
	pair* c = pair_p (code->d);
	if (!c) e->throw_exception (code);
	e->ret (new_scm (e, closure, c->a, pair_p (c->d), e->cont->env)
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
		}
	}
}

/*
 * LISTs
 */

void op_list (scm_env*e, scm*arglist)
{
	e->ret (arglist);
}

void op_car (scm_env*e, scm*arglist)
{
	if (pair_p (arglist) ) arglist = ( (pair*) arglist)->a;
	if (pair_p (arglist) ) {
		e->ret ( ( (pair*) arglist)->a);
		return;
	}
	e->throw_desc_exception ("not a pair", arglist);
}

void op_cdr (scm_env*e, scm*arglist)
{
	if (pair_p (arglist) ) arglist = ( (pair*) arglist)->a;
	if (pair_p (arglist) ) {
		e->ret ( ( (pair*) arglist)->d);
		return;
	}
	e->throw_desc_exception ("not a pair", arglist);
}

void op_cons (scm_env*e, scm*arglist)
{
	scm*a = 0;
	if (pair_p (arglist) ) {
		a = ( (pair*) arglist)->a;
		arglist = ( (pair*) arglist)->d;
		if (pair_p (arglist) ) {
			e->ret (new_scm (e, pair, a, ( (pair*) arglist)->a)
				->collectable<pair>() );
			return;
		}
	}
	e->throw_string_exception ("cons needs 2 arguments");
}

/*
 * DISPLAY (subject to remove)
 */

static void op_display (scm_env*e, scm*arglist)
{
	if (pair_p (arglist) )
		printf ( ( (pair*) arglist)->a->display (1).c_str() );
	e->ret (0);
}

static void op_newline (scm_env*e, scm*arglist)
{
	printf ("\n");
	e->ret (0);
}

/*
 * TYPE PREDICATES
 */

static void op_null_p (scm_env*e, scm*arglist)
{
	if (!pair_p (arglist) ) e->ret (0);
	else if ( ( (pair*) arglist)->a) e->ret (e->t_false);
	else e->ret (e->t_true);
}

/*static void op_pair_p (scm_env*e, scm*arglist)
{
	if (!pair_p (arglist) ) e->ret (0);
	else if (pair_p ( ( (pair*) arglist)->a) ) e->ret (e->t_true);
	else e->ret (e->t_false);
}*/

#define create_predicate_function(type) \
static void op_##type##_p (scm_env*e, scm*arglist) \
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

static void op_true_p (scm_env*e, scm*arglist)
{
	if (!pair_p (arglist) ) e->ret (0);
	else if (!boolean_p ( ( (pair*) arglist)->a) ) e->ret (0);
	else if ( ( (boolean*) ( ( (pair*) arglist)->a) )->b)
		e->ret (e->t_true);
	else e->ret (e->t_false);
}

static void op_false_p (scm_env*e, scm*arglist)
{
	if (!pair_p (arglist) ) e->ret (0);
	else if (!boolean_p ( ( (pair*) arglist)->a) ) e->ret (0);
	else if ( ( (boolean*) ( ( (pair*) arglist)->a) )->b)
		e->ret (e->t_false);
	else e->ret (e->t_true);
}


/*
 * PROGRAM FLOW CONTROL
 */

static void op_begin (scm_env*e, pair*code)
{
	e->replace_cont (new_scm (e, codevector_continuation, pair_p (code->d) )
			 ->collectable<continuation>() );
}

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
		e->val = 0;
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
			return scm_no_more_children;
		}
	}

	virtual void eval_step (scm_env*e);
};

#include <stdio.h>

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

static void op_if (scm_env*e, pair*code)
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

static void op_error (scm_env*e, scm*arglist)
{
	e->throw_exception (arglist);
}

/*
 * General add-all-the-shaite function
 */

bool escm_add_scheme_builtins (scm_env*e)
{
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

		//basic scheme
		escm_add_syntax_handler (e, "quote", op_quote);
		escm_add_func_handler (e, "eval", op_eval);

		escm_add_func_handler (e, "*do-define*", op_actual_define);
		escm_add_syntax_handler (e, "define", op_define);

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

		//I/O
		escm_add_func_handler (e, "display", op_display);
		escm_add_func_handler (e, "newline", op_newline);

		//errors
		escm_add_func_handler (e, "error", op_error);
	} catch (scm*) {
		return false;
	}
	return true;
}

/*
 * TODO, specific continuations for all 'special forms':
 * define set!
 * if cond case (and,or)
 * map foreach do
 * let let* letrec
 *
 * (lambda is a syntax!)
 */

