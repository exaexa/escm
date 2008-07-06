
#include "env.h"


#include <stdlib.h>
#include <stdint.h>

#include <queue>
using std::queue;

bool scm_env::init (scm_parser*par, size_t heap_size, size_t alignment)
{
	val = 0;
	cont = 0;

	eval_cont_factory = 0;
	codevector_cont_factory = 0;

	protected_exception = 0;

	fatal_error = 0;

	if (par) parser = par;
	else parser = new scm_classical_parser (this);

	global_frame = 0;
	t_true = t_false = 0;
	t_errorhook = 0;
	t_memoryerror = 0;

	try {
		global_frame = new_scm (this, hashed_frame)->collectable<frame>();
		t_true = new_scm (this, boolean, true)->collectable<boolean>();
		t_false = new_scm (this, boolean, false)->collectable<boolean>();
		t_errorhook = new_scm (this, symbol, "*error-hook*")
			      ->collectable<symbol>();
		t_memoryerror = new_scm (this, string, "out of memory")
				->collectable<string>();
	} catch (scm*) {
		return false;
	}
	return is_init = true;
}

void scm_env::release()
{
	if (!is_init) return;
	collect_garbage();
	for (set<scm*>::iterator i = collector.begin();
			i != collector.end(); ++i) deallocate (*i);
	if (parser) free (parser);
	is_init = false;
}

void * scm_env::allocate (size_t size)
{
	static int counter = 0;
	if((counter++)>1000){
		counter = 0;
		collect_garbage();
	}
	void*d;
	d = malloc (size);
	if (d) return d;
	collect_garbage();
	d = malloc (size);
	if (d) return d;
	throw_exception (t_memoryerror);
	return 0;
}

void scm_env::deallocate (void* p)
{
	free (p);
}

#include <stdio.h>

void scm_env::collect_garbage ()
{
	set<scm*> blacklist;
	queue<scm*> processing;
	list<scm*>::iterator i;
	set<scm*>::iterator k, l;
	scm *v, *t;
	int a;

	//TODO, IMPROVE THE COLLECTOR! OOH PLZ SOMEONE!
	//For example we don't need to use "active", but way better try to
	//delete used entries from a tree which after that gets collected.

	for (i = collector_queue.begin();i != collector_queue.end();++i)
		collector.insert (*i);

	collector_queue.clear();

	blacklist = collector;

	processing.push (val);
	processing.push (global_frame);
	processing.push (cont);
	processing.push (t_true);
	processing.push (t_false);
	processing.push (t_errorhook);
	processing.push (t_memoryerror);
	processing.push (protected_exception);

	for (k = collector.begin();k != collector.end();++k)
		if ( is_scm_protected (*k) )
			processing.push (*k);

	while (!processing.empty() ) {

		v = processing.front();
		processing.pop();

		if (!v) continue;

		if ( (l = blacklist.find (v) ) != blacklist.end() ) {
			blacklist.erase (l);

			for (a = 0; (t = v->get_child (a++) )
					!= escm_no_more_children ;) {
				if (t) processing.push (t);
			}
		}
	}

	for (k = blacklist.begin();k != blacklist.end();++k)
		if (!is_scm_protected (*k) ) {
			deallocate (*k);
			collector.erase (*k);
		}
}


/*
 * SCHEME HELPERS
 */

frame* scm_env::push_frame (size_t s)
{
	if (!cont) return global_frame; //global frame is infinately extensible

	frame*new_frame = new_scm (this, local_frame, s)
			  ->collectable<local_frame>();
	new_frame->parent = cont->env;
	cont->env = new_frame;
	return new_frame;
}

scm* scm_env::globdef (symbol*sym)
{
	return val = global_frame->define (this, sym, val);
}

bool scm_env::globset (symbol*sym)
{
	return global_frame->set (sym, val);
}

bool scm_env::globget (symbol*sym)
{
	if (global_frame->lookup (sym, &val) ) return true;
	val = 0;
	return false;
}

scm* scm_env::lexdef (symbol*scm, int d)
{
	frame*i = cont ? cont->env : global_frame;
	while (i && d) {
		i = i->parent;
		--d;
	}
	return val = (i ? i : global_frame)->define (this, scm, val);
}

bool scm_env::lexset (symbol*scm, int d)
{
	frame*i = cont ? cont->env : global_frame;
	while (i && d) {
		i = i->parent;
		--d;
	}
	while (i) {
		if (i->set (scm, val) ) {
			val = scm;
			return true;
		}
		i = i->parent;
	}
	return false;
}

bool scm_env::lexget (symbol*sym, int d)
{
	frame*i = cont ? cont->env : global_frame;
	scm*r;
	while (i && d) {
		i = i->parent;
		--d;
	}
	while (i) {
		if (i->lookup (sym, &r) ) {
			val = r;
			return true;
		}
		i = i->parent;
	}
	val = 0;
	return false;
}

/*
 * EVAL helpers
 */

void scm_env::eval_code (pair*s)
{
	push_cont (codevector_cont_factory (this , s) );
}

void scm_env::eval_expr (scm*s)
{
	push_cont (eval_cont_factory (this, s) );
}

#include "parser.h"

int scm_env::eval_string (const char*s, char term_char)
{
	int err = parser->parse_string (s);
	if (err) return err;
	if (term_char) {
		char str[2] = {term_char, 0};
		err = parser->parse_string (str);
		if (err) return err;
	}

	pair*code = parser->get_result (false);
	if (code) eval_code (code->collectable<pair>() );
	return 0;
}

void scm_env::add_global (const char*name, scm*data)
{
	symbol*s = new_scm (this, symbol, name);
	global_frame->define (this, s, data);
	s->mark_collectable();
	data->mark_collectable();
}



