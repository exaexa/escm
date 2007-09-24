
#include "env.h"


#include <stdlib.h>
#include <stdint.h>
#define byte uint8_t

#include <queue>
using std::queue;


scm_env::scm_env (size_t heap_size, size_t alignment)
{
	heap = malloc (heap_size);
	hs = heap_size;
	align = alignment;

	ip = val = cont = 0;
	global_frame = env = new_scm (*this, hashed_frame);
}

scm_env::~scm_env()
{
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

			he = *i;

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

	return NULL;  //too bad, none found. we shall collect.
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
	else return NULL;
}

void scm_env::deallocate (void* p)
{
	set<gc_heap_entry>::iterator i;
	i = allocated_space.find
	    (gc_heap_entry ( (byte*) p - (byte*) heap, 0) );
	//note. heap entries are indexed by start, not by size,
	//so the zero here doesn't harm anything.
	if (i != allocated_space.end() ) free_space.insert (*i);
	allocated_space.erase (i);
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
	set<scm*> active, unused;
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

	processing.push (ip);
	processing.push (val);
	processing.push (env);
	processing.push (cont);

	for (k = collector.begin();k != collector.end();++k)
		if ( is_scm_protected (*k) ) processing.push (*k);

	while (!processing.empty() ) {

		v = processing.front();
		processing.pop();

		if (!v) continue;

		active.insert (v);

		for (a = 0;
		        (t = v->get_child (a++) ) != scm_no_more_children ;) {
			if (t) processing.push (t);
		}
	}

	k = active.begin();
	l = collector.begin();
	while (l != collector.end() ) {
		while ( (*k) > (*l) ) {
			if ( is_scm_protected (*l) );
			else deallocate (*l);
			++l;
		}
		++k;
		++l;
	}

	sort_out_free_space();
}


/*
 * SCHEME MACHINE
 */

scm* scm_env::push_frame (size_t s)
{
	frame*new_frame = new_scm (*this, local_frame, s);
	if (!new_frame) return NULL;
	new_frame->parent = env;
	env = new_frame;
	return env;
}

scm* scm_env::frame_set (symbol*sym)
{
	return env->define (this, sym, val);
}

void scm_env::call()
{
	/*
	 * 'call' the list in val
	 */
	if (val) {
		pair*l = dynamic_cast<pair*> (val);
		if (!l) return;
		lambda*p = dynamic_cast<lambda*> (l);
		if (!p) {
			//exception, bad type!
			return;
		}
		push_env();
		p->call (this);
	}
}

/*
 * about tailcalls. Please note that we can't use them for evaluating things
 * like function arguments and such, it's just one-purpose optimization.
 * use with caution. ;)
 */

void scm_env::call_tail()
{
	call();
	/*
	 * what we need now is to get rid of the continuation of function that
	 * already finished (processing it's tail)
	 */
	if (!cont) {
		//internal error, call failed.
		return;
	}
	if (!cont->parent) {
		//internal error, tailcall invoked on global scope
		return;
	}
	cont->val_save = cont->parent->val_save;
	cont->parent = cont->parent->parent;
}

scm* scm_env::ret() //seems more like an alias, maybe we could get rid of it?
{
	pop_env();
	return NULL;
}

scm* scm_env::push_env (scm**result_save)
{
	continuation*c = new_scm (*this, continuation, ip, env, cont,
	                          result_save);
	if (!c) return NULL;
	return cont = c;
}

scm* scm_env::pop_env()
{
	if (cont->val_save) * (cont->val_save) = val;
	ip = cont->ip;
	env = cont->env;
	cont = cont->parent;

	return NULL;
}

scm* scm_env::jump (scm*new_ip)
{
	ip = new_ip;
	return ip;
}

scm* scm_env::jump_false (scm*new_ip)
{
	if (is_type (*val, boolean) )
		if (! ( ( (boolean*) val) -> b) )
			ip = new_ip;
	return ip;
}

scm* scm_env::make_closure (scm*cl_ip)
{
	closure*c = new_scm (*this, closure, (pair*) val, cl_ip, env);
	val = c;
	return val;
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
	frame*i = env;
	while (i && d) {
		i = i->parent;
		--d;
	}
	return val = env->define (this, scm, val);
}

bool scm_env::lexset (symbol*scm, int d)
{
	frame*i = env;
	while (i && d) {
		i = i->parent;
		--d;
	}
	while (i) {
		if (i->set (scm, val) ) return true;
		i = i->parent;
	}
	return false;
}

bool scm_env::lexget (symbol*sym, int d)
{
	frame*i = env;
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
	return false;
}

