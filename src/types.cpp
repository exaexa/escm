
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
