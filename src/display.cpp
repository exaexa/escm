
#include "display.h"
#include <stack>

#define display escm_display_to_stdout
void display (scm*a, bool output)
{

	if (pair_p (a) ) {
		pair*p = pair_p (a);
		printf ("(");
		while (1) {
			display (p->a, false);
			if (!p->d) {
				printf (")");
				break;
			}
			printf (" ");
			if (pair_p (p->d) ) {
				p = pair_p (p->d);
				continue;
			} else {
				printf (". ");
				display (p->d, false);
				printf (")");
				break;
			}
		}
	} else if (number_p (a) )
		printf ("%g", number_p (a)->n);
	else if (symbol_p (a) )
		printf ("%s", (const char*) *symbol_p (a) );
	else if (string_p (a) )
		printf (output ? "%s" : "\"%s\"", (const char*) *string_p (a) );
	else if (boolean_p (a) )
		printf ( ( (boolean*) a)->b ? "#T" : "#F");
	else if (dynamic_cast<extern_func*> (a) )
		printf ("#<extern_func h=%p>", ( (extern_func*) a) -> handler);
	else if (dynamic_cast<closure*> (a) ) {
		closure*c = (closure*) a;
		printf ("#<closure args[%zd] ", c->paramsize);
		display (c->arglist);
		printf (" code ");
		display (c->ip);
		printf (" env ");
		display (c->env);
		printf (">");
	} else if (dynamic_cast<extern_syntax*> (a) )
		printf ("#<extern_syntax h=%p>",
			( (extern_syntax*) a) -> handler);
	else if (dynamic_cast<macro*> (a) ) {
		macro*c = (macro*) a;
		printf ("#<macro argname ");
		display (c->argname);
		printf (" code ");
		display (c->code);
		printf (">");
	} else if (!a)
		printf ("()");
	else printf ("#<scm %p>", a);
}
