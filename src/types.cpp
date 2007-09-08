
#include "env.h"

/*
 * note. because of compatibility with embedded systems, we'll later
 * try to replace as much library functions as we'll be able to;)
 *
 * (and someone code it)
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

/*
 * LOCAL FRAMES
 */

/*
 * about:
 * If we decide the frame is full, we'll chain it to parent to
 * add another binding. Good thing is that we can preallocate some
 * memory (so it runs better later), bad thing is that we should know how
 * much of it shall we use. Maybe add some destructor-triggered statistic
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

bool local_frame::lookup (symbol*name, scm**result)
{
	int s = 0, e = size, t, i;
	scm**p = (scm**) dataof (table);

	while (s <= e) {
		i = (s + e) / 2;
		t = name->cmp ( (symbol*) p[i*2]);
		if (!t) {
			*result = p[i*2];
			return true;
		} else 	if (t < 0) {
			e = i - 1;
		} else {
			s = i + 1;
		}
	}

	return false;
}
