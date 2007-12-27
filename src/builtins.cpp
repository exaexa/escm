#include "builtins.h"
#include "debug.h"

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
				res->add (number_p (p->a) );
			//else cast error
			p = pair_p (p->d);
		}
	}
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
}

void op_exp (scm_env*e, scm*params)
{
	number*res = new_scm (e, number, 1);
	if (!res) return;
	e->val = res->collectable<number>();

	pair*p = pair_p (params);
	if (p) if (number_p (p->a) ) res->set (number_p (p->a) );
	res->exp();
}

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

