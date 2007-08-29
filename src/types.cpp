
#include "env.h"

/*
 * note. because of compatibility with embedded systems, we'll later
 * try to replace as much library functions as we'll be able to;)
 *
 * (and someone code it)
 */

symbol::symbol (scm_env* e, const char* c) : scm (e)
{
	char*p;
	d = new_data_scm (*e, strlen (c) + 1);

	if (!d) return;

	p = (char*) dataof (d);
	while (*c) {
		*p = *c;
		if ( (*p >= 'a') && (*p <= 'z') ) *p -= 0x20;
		++p;
		++c;
	}
	*p = 0; //terminate da string
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

