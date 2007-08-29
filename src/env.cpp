
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

void scm_env::mark_collectable (scm* p)
{
	scm*t = 0, *v;
	queue<scm*>q;
	int i;

	q.push (p);

	while (! (q.empty() ) ) {
		v = q.front();
		q.pop();

		if (v->flags & scmf_nocollect) { //avoids looping in loops :D
			v->flags &= (~scmf_nocollect);
			for (i = 0; (t = v->get_child (i++) );) q.push (t);
		}
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

	processing.push (cv);
	processing.push (ip);
	processing.push (val);
	processing.push (env);
	processing.push (cont);

	for (k = collector.begin();k != collector.end();++k)
		if ((*k)->flags & scmf_nocollect) processing.push (*k);

	while (!processing.empty() ) {

		v = processing.front();
		processing.pop();

		active.insert (v);

		for (a = 0; (t = v->get_child (a++) );) {
			processing.push (t);
		}
	}

	k = active.begin();
	l = collector.begin();
	while (l != collector.end() ) {
		while ( (*k) > (*l) ) {
			if ( (*l)->flags & scmf_nocollect);
			else deallocate (*l);
			++l;
		}
		++k;
		++l;
	}

	sort_out_free_space();
}

