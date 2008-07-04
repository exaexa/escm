
#include "env.h"


#include <stdlib.h>
#include <stdint.h>

#include <queue>
using std::queue;

bool scm_env::init (scm_parser*par, size_t heap_size, size_t alignment)
{
	min_heap_part_size = heap_size;
	align = alignment;

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
	if (parser) free (parser);
	for (set<gc_heap_entry>::iterator i = allocated_heap.begin();
			i != allocated_heap.end();++i) free (i->start);
	allocated_heap.clear();
	is_init = false;
}

void scm_env::add_heap_part (size_t minsize)
{
	size_t s = (minsize > min_heap_part_size) ? minsize : min_heap_part_size;
	void*t = malloc (s);
	if (!t) return;
	gc_heap_entry he (t, s);
	allocated_heap.insert (he);
	free_space.insert (he);
}

void scm_env::free_heap_parts()
{
	set<gc_heap_entry>::iterator i, j;
	queue<gc_heap_entry> to_delete;

	for (i = allocated_heap.begin();
			i != allocated_heap.end(); ++i) {
		j = free_space.find (*i);
		if (j == free_space.end() ) continue;
		if (i->size == j->size) to_delete.push (*i);
	}

	while (!to_delete.empty() ) {
		free (to_delete.front().start);
		allocated_heap.erase (to_delete.front() );
		free_space.erase (to_delete.front() );
		to_delete.pop();
	}
}

void * scm_env::new_heap_object (size_t size)
{
	set<gc_heap_entry>::iterator i;
	gc_heap_entry he;
	void* t;

	//do some alignment
	if (size % align) {
		size /= align;
		++size;
		size *= align;
	}

	for (i = free_space.begin();i != free_space.end();++i)
		if (i->size >= size) {

			he = *i;

			allocated_space.insert (gc_heap_entry (he.start, size) );

			//remove old free space entry
			free_space.erase (i);

			//calculate the pointer
			t = he.start;

			//decrease free space size
			he.size -= size;
			if (he.size) { //if something remains
				he.start = (char*) (he.start) + size;
				//move it and push it back
				free_space.insert (he);
			}

			return t; //finish searching
		}
	return 0;  //too bad, none found. we shall collect.
}

void * scm_env::allocate (size_t size)
{
	//Try to Allocate, then try to sweep and allocate, then die.
	void*d;
	d = new_heap_object (size);
	if (d) return d;
	collect_garbage();
	d = new_heap_object (size);
	if (d) return d;
	add_heap_part (size);
	d = new_heap_object (size);
	if (d) return d;
	throw_exception (t_memoryerror);
	return 0;
}

void scm_env::deallocate (void* p)
{
	set<gc_heap_entry>::iterator i;
	i = allocated_space.find (gc_heap_entry (p, 0) );
	if (i != allocated_space.end() ) {
		free_space.insert (*i);
		allocated_space.erase (i);
	} else {
		dprint ("deallocating unknown block %p !!!\n", p);
	}
}

void scm_env::sort_out_free_space()
{
	/*
	 * This "defragments" free space list by joining more gapless free
	 * spaces into bigger ones, allowing us to choose better pieces.
	 */

	set<gc_heap_entry>::iterator i, j;
	gc_heap_entry eh;

	//free_space should always have at least 1 entry.
	if (!free_space.size() ) return;

	/*
	 * An extremely ugly hack follows. (we would need operator+ in 
	 * std::set iterators to get rid of it)
	 * I hope that gcc's -O2 is able to simplify those j=++i=j...
	 */

	i = free_space.begin();
	j = i;
	++j;
	while (1) {
		if (j == free_space.end() ) break;

		//if the space touches the following, join them
		//(we also count overlapping, but this should never happen.
		if ( ( (char*) (i->start) + i->size) >= (j->start) ) {
			eh = *i; //compute a new field
			eh.start = i->start;
			eh.size = (char*) (j->start) + j->size
				  - (char*) (i->start);
			free_space.erase (i); //erase old
			free_space.erase (j);
			i = free_space.insert (eh).first; //insert the new one
			j = i;
			++j;  //^^ oh das stinx!
		} else {
			i = j;
			++j;
		}
	}
}

void scm_env::collect_garbage ()
{
	set<scm*> active;
	queue<scm*> processing;
	list<scm*>::iterator i;
	set<scm*>::iterator k, l;
	scm *v, *t;
	int a;

	//TODO, IMPROVE THE COLLECTOR! OOH PLZ SOMEONE!
	//TODO2, find out how?!

	for (i = collector_queue.begin();i != collector_queue.end();++i)
		collector.insert (*i);

	collector_queue.clear();

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

		if (active.find (v) == active.end() ) {
			active.insert (v);

			for (a = 0; (t = v->get_child (a++) )
					!= escm_no_more_children ;) {
				if (t) processing.push (t);
			}
		}
	}

	k = active.begin();
	l = collector.begin();
	while (l != collector.end() ) {
		while ( (*k) > (*l) ) {
			if ( !is_scm_protected (*l) )
				collector_queue.push_front (*l);
			++l;
		}
		++k;
		++l;
	}

	for (i = collector_queue.begin();i != collector_queue.end();++i) {
		deallocate (*i);
		collector.erase (*i);
	}
	collector_queue.clear();

	sort_out_free_space();
	free_heap_parts();
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



