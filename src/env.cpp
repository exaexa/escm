
#include "env.h"


#include <stdlib.h>
#include <stdint.h>
#define byte uint8_t

#include <queue>
using std::queue;


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


scm_env::scm_env (scm_parser*par, size_t heap_size, size_t alignment)
{
	heap = malloc (heap_size);
	hs = heap_size;
	align = alignment;

	{
		gc_heap_entry h;
		h.size = heap_size;
		h.start = 0;
		free_space.insert (h);
	}

	val = 0;
	cont = 0;
	global_frame = new_scm (this, hashed_frame)->collectable<frame>();
	eval_cont_factory = default_eval_factory;
	codevector_cont_factory = default_cv_factory;

	if (par) parser = par;
	else parser = new scm_classical_parser (this);
}

scm_env::~scm_env()
{
	if (parser) free (parser);
	if (heap) free (heap);
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

			dprint ("free block: %ld\n", i->size);

			he = *i;

			allocated_space.insert (gc_heap_entry (he.start, size) );

			//remove old free space entry
			free_space.erase (i);

			//calculate the pointer
			t = (byte*) heap + he.start;

			//decrease free space size
			he.size -= size;
			if (he.size) { //if something remains
				he.start += size; //move it and push it back
				free_space.insert (he);
			}

			return t; //finish searching
		}
	dprint ("out of memory\n");
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
	dprint ("HARD out of memory\n");
	return 0;
}

void scm_env::deallocate (void* p)
{
	set<gc_heap_entry>::iterator i;
	i = allocated_space.find
	    (gc_heap_entry ( (byte*) p - (byte*) heap, 0) );
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
		if ( (i->start + i->size) >= (j->start) ) {
			eh = *i; //compute a new field
			eh.start = i->start;
			eh.size = j->start + j->size - i->start;
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

	dprint ("----- GC\n");

	for (i = collector_queue.begin();i != collector_queue.end();++i)
		collector.insert (*i);

	collector_queue.clear();

	processing.push (val);
	processing.push (global_frame);
	processing.push (cont);

	for (k = collector.begin();k != collector.end();++k)
		if ( is_scm_protected (*k) ) {
			dprint ("adding protected %p\n", *k);
			processing.push (*k);
		}

	while (!processing.empty() ) {

		v = processing.front();
		processing.pop();

		if (!v) continue;

		active.insert (v);

		for (a = 0; (t = v->get_child (a++) )
				!= scm_no_more_children ;) {
			if (t) processing.push (t);
		}
	}

	k = active.begin();
	l = collector.begin();
	while (l != collector.end() ) {
		while ( (*k) > (*l) ) {
			if ( is_scm_protected (*l) )
				dprint ("%p is protected\n", *l);
			else {
				dprint ("deallocating %p\n", *l);
				collector_queue.push_front (*l);
			}
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
	dprint ("----- GC done\n");
}


/*
 * SCHEME HELPERS
 */

frame* scm_env::push_frame (size_t s)
{
	if (!cont) return global_frame; //global frame is infinately extensible

	frame*new_frame = new_scm (this, local_frame, s)->collectable<frame>();
	if (!new_frame) return 0;
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
		if (i->set (scm, val) ) return true;
		i = i->parent;
	}
	return global_frame->set (scm, val);
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

void scm_env::eval_string (const char*s)
{
	if (parser->parse_string (s) ) return;

	/*
	 * what to do on errors?? seems like this function should not
	 * be used at all. Marked as future subject of removal.
	 */

	pair*code = parser->get_result (false);
	if (code) eval_code (code->collectable<pair>() );
}

