
#include "env.h"
#include <queue>
using std::queue;

/*
 * note. because of compatibility with embedded systems, we'll later
 * try not to use as much library functions as we'll be able to;)
 *
 * (and someone code it)
 */

void scm::mark_collectable()
{
	scm*t = 0, *v;
	queue<scm*>q;
	int i;
	q.push (this);
	while (!q.empty() ) {
		v = q.front();
		q.pop();
		if (v) if (is_scm_protected (v) ) {
				mark_scm_collectable (v);
				for (i = 0;
				        (t = v->get_child (i++) )
				        != scm_no_more_children
				        ;)
					if (t) q.push (t);
			}
	}
}

/*
 * PAIR
 */

int pair::list_length()
{
	pair *a = this, *b = this; //a moves 2x faster than b, we detect "overtakes"
	int size = 0;
	while (a) {
		a = pair_p (a->d); //pair_p may be used as dynamic_cast;) useful!
		if (a == b) return -1; // cycle detected
		if (size&1)
			b = pair_p (b->d);
		++size;
	}
	return size;
}

/*
 * list_loop_position
 *
 * this one has terrible time (O(n^2)), use it only for lists you are sure
 * they are loops.
 */
int pair::list_loop_position()
{
	pair*a = this, *b;
	int i = 0;
	while (a) {
		b = a;
		while (b && (b != a) ) b = pair_p (b->d);
		if (b) return i;
		++i;
		a = pair_p (a->d);
	}
	return -1;
}

/*
 * same problem, terrible time on cyclic lists
 */
int pair::list_size()
{
	int i = list_length(), j = 0;
	if (i >= 0) return i;
	j = i = list_loop_position();

	pair*a = this, *b;

	while (i >= 0) {
		--i;
		a = pair_p (a->d);
	}

	b = a;

	while (b && (b->d != a) ) {
		++j;
		b = pair_p (b->d);
	}

	return j;
}

/*
 * STRINGS AND SYMBOLS
 */

symbol::symbol (scm_env* e, const char* c, int len) : text (e, c, len)
{
	char*p;
	if (!d) return;
	p = (char*) dataof (d);
	while (*p) {
		if ( (*p >= 'a') && (*p <= 'z') ) *p -= 0x20;
		++p;
	}
}

int symbol::cmp (symbol* s)
{
	const char*a, *b;
	a = (const char*) dataof (d);
	b = (const char*) dataof (s->d);
	while (*a && *b && (*a == *b) ) {
		++a;
		++b;
	}
	return (int) *b - (int) *a;
}

text::text (scm_env*e, const char*c, int len) : scm (e)
{
	char*p;
	int datasize = (len < 0) ? (strlen (c) + 1) : (len + 1);
	d = new_data_scm (e, datasize);

	if (!d) return;

	p = (char*) dataof (d);

	datasize--; //we don't copy the finishing zero

	while (--datasize) {
		*p = *c;
		++p;
		++c;
	}
	*p = 0;
}

/*
 * VECTOR
 */

bool vector::alloc (scm_env*e, size_t size)
{
	d = new_data_scm (e, sizeof (scm*) * size);
	if (!d) {
		this->size = 0;
		return false;
	}
	this->size = size;
	return true;
}

vector::vector (scm_env*e, size_t size) : scm (e)
{
	if (!alloc (e, size) ) return;
	for (size_t i = 0;i < size;++i)  ( (scm**) dataof (d) ) [i] = 0;
}

vector::vector (scm_env*e, pair* l) : scm (e)
{
	if (!l) {
		d = 0;
		size = 0;
		return;
	}
	size = l->list_size();
	for (int i = 0;i < (int) size;++i) {
		( (scm**) dataof (d) ) [i] = l->a;
		l = pair_p (l->d);
	}
}

scm* vector::ref (size_t i)
{
	if (i < size) return ( (scm**) dataof (d) ) [i];
	return 0;
}

void vector::set (size_t i, scm*a)
{
	if (i <= size) ( (scm**) dataof (d) ) [i] = a;
}

scm* vector::get_child (int i)
{
	if (i < (int) size) return ref (i);
	if (i == (int) size) return d;
	return scm_no_more_children;
}

/*
 * TODO
 * just create some better hash function. This one most probably sux.
 *
 * (or prove that it doesn't sux)
 */

#define hash_table_size 256

static char hash_string (const char*s)
{
	char t = 0;
	while (*s) {
		t += *s;
		t = (t << 1) + (t & 0x80);
		++s;
	}
	return t;
}

/*
 * HASHED FRAMES
 */

hashed_frame::hashed_frame (scm_env*e) : frame (e)
{
	int i;
	chained_frame_entry**t;

	table = new_data_scm (e,
	                      sizeof (chained_frame_entry*) * hash_table_size);
	if (!table) return;

	t = (chained_frame_entry**) dataof (table);

	for (i = 0;i < hash_table_size;++i) t[i] = 0;
}

bool hashed_frame::lookup_frame (symbol*s, chained_frame_entry**result,
                                 int hash)
{
	chained_frame_entry*p =
	    ( (chained_frame_entry**) dataof (table) )
	    [ (hash>=0) ?hash:hash_string ( (const char*) dataof (s->d) ) ];
	while (p) {
		if (!s->cmp (p->name) ) {
			*result = p;
			return true;
		}
		p = p->next;
	}
	return false;
}

bool hashed_frame::lookup (symbol*s, scm**result)
{
	chained_frame_entry*f;
	if (lookup_frame (s, &f) ) {
		*result = f->content;
		return true;
	}
	return false;
}

bool hashed_frame::set (symbol*s, scm*d)
{
	chained_frame_entry*f;
	if (lookup_frame (s, &f) ) {
		f->content = d;
		return true;
	}
	return false;
}

scm* hashed_frame::define (scm_env*e, symbol*s, scm*d)
{
	int hash = hash_string ( (const char*) dataof (s->d) );
	chained_frame_entry*f;

	if (lookup_frame (s, &f) ) {
		f->content = d;
		return s;
	}

	f = new_scm (e, chained_frame_entry, s, d,
	             ( (chained_frame_entry**) dataof (table) ) [hash])
	    ->collectable<chained_frame_entry>();
	if (!f) return 0;

	( (chained_frame_entry**) dataof (table) ) [hash] = f;

	return s;
}

scm* hashed_frame::get_child (int i)
{
	if (i < hash_table_size)
		return ( (chained_frame_entry**) dataof (table) ) [i];
	if (i == hash_table_size) return table;
	return scm_no_more_children;
}

/*
 * LOCAL FRAMES
 */

/*
 * about:
 * If we decide the frame is full, we'll chain it to parent to
 * add another binding. Good thing is that we can preallocate some
 * memory (so it runs better later), bad thing is that we should know how
 * much of it we shall use. Maybe add some destructor-triggered statistic
 * collector of "ooh how many free positions/overflows we had this time"
 * and compute the value after it. Algorithmization is unknown yet:D
 */

local_frame::local_frame (scm_env*e, size_t s) : frame (e)
{
	table = new_data_scm (e, 2 * sizeof (scm*) * s);
	if (!table) return;
	size = s;
	used = 0;
}

int local_frame::get_index (symbol*name)
{
	int s = 0, e = size, i, t;
	scm**p = (scm**) dataof (table);
	while (s <= e) {
		i = (s + e) / 2;
		t = name->cmp ( (symbol*) p[i*2]);
		if (!t) return i;
		else if (s == e) return - (s + 1);
		else if (t < 0) e = i;
		else s = i + 1;
	}

	/*
	 * program should never come here, if it does it's
	 * seriously broken. returning 'failsafe' value.
	 */

	printf ("!!! bad lookup in local_frame at %p, data at %p\n",
	        this, table);
	return -1;
}

bool local_frame::lookup (symbol*name, scm**result)
{
	int t = get_index (name);
	if (t < 0) return false;
	*result = ( (scm**) dataof (table) ) [ (t*2) +1];
	return true;
}

bool local_frame::set (symbol* name, scm*data)
{
	int t = get_index (name);
	if (t < 0) return false;
	( (scm**) dataof (table) ) [ (t*2) +1] = data;
	return true;
}

scm* local_frame::get_child (int i)
{
	if ( (unsigned int) i < size*2)
		return ( (scm**) dataof (table) ) [i];
	if ( (unsigned int) i == size*2) return table;
	return scm_no_more_children;
}

scm* local_frame::define (scm_env*e, symbol*name, scm*content)
{
	int t = get_index (name);
	if (t > 0) {
		( (scm**) dataof (table) ) [ (t*2) +1] = content;
		return name;
	}

	t = (-t) - 1;

	if (used < size) { //insert into vector

		scm**p = (scm**) dataof (table);
		int i;

		for (i = size - 1;i > t;--i) {
			p[2*i] = p[2* (i-1) ];
			p[1+2*i] = p[1+2* (i-1) ];
		}

		p[2*i] = name;
		p[2*i+1] = content;

	} else { //chain the frames

		size_t new_size = size + 1; //compute new frame size

		local_frame* f = new_scm (e, local_frame, new_size)
		                 ->collectable<local_frame>();
		if (!f) return 0;

		/*
		 * because we can't push the newly allocated frame on top
		 * of frame stack, we'll push it below the top and then switch
		 * data, so the empty frame stays on top
		 */

		data_placeholder*new_table = f->table;

		f->used = used;
		f->size = size;
		f->table = table;

		f->parent = parent;
		parent = f;

		table = new_table;
		used = 0;
		size = new_size;
	}

	return name;
}


/*
 * FUNCTIONS
 */

closure::closure (scm_env*e, pair*Arglist,
                  pair*Ip, frame*Env) : lambda (e)
{
	int i;
	arglist = Arglist;
	ip = Ip;
	env = Env;

	i = 0;
	while (Arglist) {
		++i;
		if (!pair_p (Arglist) ) break;
	}
	paramcount = i;
}

/* FIXME FIXME FIXME
void closure::apply (scm_env*e)
{
	//TODO this should move to lambda_continuation
	pair *argdata = (pair*) (e->val), *argname = arglist;
	frame*f;
	f = new_scm (e, local_frame, paramcount);
	if (!f) return;
	//FIXME. improve this so it really supports "rest" arguments
	while (argdata && argname && pair_p (argdata) ) {
		if (pair_p (argname) ) {
			f->define (e, (symbol*) (argname->a), argdata->a);
			argname = (pair*) (argname->d);
			argdata = (pair*) (argdata->d);
		} else {
			f->define (e, (symbol*) argname, argdata);
			argname = argdata = 0;
			break;
		}
	}

	if (argname) if (pair_p (argname) ) {
			//exception, not'nuff arguments
		}

	if (argdata) if (pair_p (argdata) ) {
			//exception, too many args passed
		}

	//TODO create new environment
} FIXME FIXME FIXME*/

