
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
}

scm_env::~scm_env()
{
	if (heap) free (heap);
}

int scm_env::register_c_func (const string& name, scm_c_handler func)
{

	return 0;
}

scm* scm_env::eval (scm* expression)
{

	return NULL;
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

scm * scm_env::new_scm () //TODO
{
}

void scm_env::free_scm (scm* p)
{
	//p->scm::~scm(); //destruct it (if possible)
	//we have no destructor there^ so it's not needed. Just to be clear.
}

void scm_env::mark_collectable (scm* p)
{
	scm*t = NULL, *v;
	queue<scm*>q;
	int i;

	q.push (p);

	while (! (q.empty() ) ) {
		v = q.front();
		q.pop();

		/* FIXME
		v->flags &= (~V_NOFREE);

		//recursively mark all children collectable.
		for (i = 0;t = type[v->type].get_children (v->data, i);++i)
			q.push (t);
		*/
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
			++j;  //^^ oh das iterator stinx!
		} else {
			i = j;
			++j;
		}
	}
}

void scm_env::collect_garbage ()
{
	set<scm*> unused, active;
	queue<scm*> processing;
	list<scm*>::iterator i;
	frame::iterator j;
	vector<frame>::iterator k;
	set<scm*>::iterator l;
	scm *v, *t;
	int a;

	//for multitasking: The critical section begins here.

	for (i = collector_queue.begin();i != collector_queue.end();++i)
		collector.insert (*i);

	collector_queue.clear();

	unused = collector;

	for (j = globals.begin();j != globals.end();++j) {
		processing.push (j->second);
		unused.erase (j->second);
	}

	for (k = stack.begin();k < stack.end();++k)
		for (j = k->begin();j != k->end();++j) {
			processing.push (j->second);
			unused.erase (j->second);
		}

	//end of critical section, program flow can continue

	while (!processing.empty() ) {

		v = processing.front();
		processing.pop();

		active.insert (v);

		/* FIXME
		for (a = 0; t = type[v->type].get_children (v->data, a);++a) {
			processing.push (t);
			unused.erase (t);
		}
		*/
	}

	/*for (l = unused.begin();l != unused.end();++l)
		if (! ( (*l)->flags & V_NOFREE) ) free_var (*l);
	*/

	sort_out_free_space();
}

