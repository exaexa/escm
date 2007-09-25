
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
				for (i = 0; (t = v->get_child (i++) ) != scm_no_more_children;)
					if (t) q.push (t);
			}
	}
}

/*
 * STRINGS AND SYMBOLS
 */

symbol::symbol (scm_env* e, const char* c) : text (e, c)
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

text::text (scm_env*e, const char*c) : scm (e)
{
	char*p;
	d = new_data_scm (*e, strlen (c) + 1);

	if (!d) return;

	p = (char*) dataof (d);
	while (*c) {
		*p = *c;
		++p;
		++c;
	}
	*p = 0;
}

/*
 * TODO
 * just create some better hash function. This one most probably sux.
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

	table = new_data_scm (*e,
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

	f = new_scm (*e, chained_frame_entry, s, d,
	             ( (chained_frame_entry**) dataof (table) ) [hash]);
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
	table = new_data_scm (*e, 2 * sizeof (scm*) * s);
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

		local_frame* f = new_scm (*e, local_frame, new_size);
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

#define pair_p(x) (typeid(x)==typeid(pair))

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

void closure::call (scm_env*e)
{
	pair *argdata = (pair*) (e->val), *argname = arglist;
	frame*f;
	f = new_scm (*e, local_frame, paramcount);
	if (!f) return;
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

	//create new environment
	e->ip = ip;
	e->et = et_closure;
	f->parent = env;
	e->env = f;
}

