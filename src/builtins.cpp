#include "builtins.h"

/*
 * NUMBER FUNCTIONS
 */

void op_add (scm_env*e, scm*params)
{
	number*res = new_scm (e, number, 0);
	if (!res) return;
	pair*p = pair_p (params);
	e->val = res->collectable<number>();
	while (p) {
		if (number_p (p->a) )
			res->add (number_p (p->a) );
		//else cast error
		p = pair_p (p->d);
	}
	e->pop_cont();
}

void op_sub (scm_env*e, scm*params)
{
	number*res = new_scm (e, number, 0);
	if (!res) return;
	pair*p = pair_p (params);
	e->val = res->collectable<number>();
	if (pair_p (p) ) {
		if (number_p (p->a) ) res->set (number_p (p->a) );
		p = pair_p (p->d);
		while (p) {
			if (number_p (p->a) )
				res->sub (number_p (p->a) );
			//else cast error
			p = pair_p (p->d);
		}
	}
	e->pop_cont();
}

void op_mul (scm_env*e, scm*params)
{
	number*res = new_scm (e, number, 1);
	if (!res) return;
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
	if (!res) return;
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
				return;
			}
		} else {
			if (f) {
				res->div (f);
				return;
			}
		}
	}
	e->pop_cont();
}

#define two_number_func(name,func) \
void name(scm_env*e, scm*params) \
{ \
	number*res=new_scm(e,number,0); \
	if(!res)return; \
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
	if (!res) return;
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
	if (!res) return;
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
	if (!p) return;
	name = symbol_p (p->a);
	if (!name) return;
	p = pair_p (p->d);
	if (!p) return;
	e->val = p->a;
	e->lexdef (name);
	e->ret (name);
}

static void op_define (scm_env*e, pair*code)
{
	code = pair_p (code->d);
	symbol*name;
	scm*def;
	if (!code) return;
	if (pair_p (code->a) ) { //defining a lambda, shortened syntax
		pair*l = (pair*) code->a;
		name = symbol_p (l->a);
		scm*lam = new_scm (e, symbol, "LAMBDA");
		scm*temp = new_scm (e, pair, l->d, code->d);
		def = new_scm (e, pair, lam, temp);
		//generates (lambda params . code)
	} else if (symbol_p (code->a) ) {
		name = (symbol*) code->a;
		if (pair_p (code->d) ) def = pair_p (code->d)->a;
		else def = code->d;
	} else {
		e->val = 0;
		e->pop_cont();
		return; //error
	}

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
	code = pair_p (code->d);
	e->ret (new_scm (e, closure, code->a, pair_p (code->d), e->cont->env)
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
	e->ret (0);
}

void op_cdr (scm_env*e, scm*arglist)
{
	if (pair_p (arglist) ) arglist = ( (pair*) arglist)->a;
	if (pair_p (arglist) ) {
		e->ret ( ( (pair*) arglist)->d);
		return;
	}
	e->ret (0);
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
	e->ret (0);
}

/*
 * DISPLAY (subject to remove)
 */

#include "display.h"

static void op_display (scm_env*e, scm*arglist)
{
	if (pair_p (arglist) ) 
		escm_display_to_stdout ( ( (pair*) arglist)->a, true);
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

//TODO

/*
 * GENERAL
 */

static void add_global (scm_env*e, char*name, scm*data)
{
	scm*t = e->val;
	symbol*s = new_scm (e, symbol, name);
	if (!s) return;
	e->val = data;
	e->globdef (s);
	s->mark_collectable();
	data->mark_collectable();
	e->val = t;
}

#define add_syntax_handler(name,h)\
add_global(e,name,new_scm(e,extern_syntax,h))

#define add_func_handler(name,h)\
add_global(e,name,new_scm(e,extern_func,h))

void escm_add_scheme_builtins (scm_env*e)
{
	add_func_handler ("+", op_add);
	add_func_handler ("-", op_sub);
	add_func_handler ("*", op_mul);
	add_func_handler ("/", op_div);
	add_func_handler ("modulo", op_mod);
	add_func_handler ("expt", op_pow);
	add_func_handler ("log", op_log);
	add_func_handler ("pow-e", op_exp);

	add_syntax_handler ("quote", op_quote);
	add_func_handler ("eval", op_eval);

	add_func_handler ("*do-define*", op_actual_define);
	add_syntax_handler ("define", op_define);

	add_syntax_handler ("lambda", op_lambda);
	add_syntax_handler ("macro", op_macro);

	add_func_handler ("list", op_list);
	add_func_handler ("cons", op_cons);
	add_func_handler ("car", op_car);
	add_func_handler ("cdr", op_cdr);

	add_func_handler ("display", op_display);
	add_func_handler ("newline", op_newline);
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

