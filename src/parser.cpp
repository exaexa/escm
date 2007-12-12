
#include "parser.h"

/*
 * Oh hai hell! this parser is a little complicated...so no one's
 * gonna shit bricks when we use C++ exception catching. kthxbai.
 */

int scm_classical_parser::parse_string (const char*str)
{
	String token;
	try {
		while (*str) if (parse_char (* (str++), token) )
				process_token (token);
	} catch (int i) {
		return i;
	}
	return 0;
}

pair* scm_classical_parser::get_result (bool keep)
{
	pair*t = stack.back().result;
	if (!keep) {
		stack.back().result = 0;
		stack.back().result_tail = 0;
	}
	return t;
}

int scm_classical_parser::end_stream()
{

	return 0;
}

void scm_classical_parser::reset()
{
}

void scm_classical_parser::push()
{
}

void scm_classical_parser::pop()
{
}

void scm_classical_parser::append (scm*s)
{
}

void process_token (const String& tok)
{
}

bool scm_classical_parser::parse_char (char c, String&out)
{

	return false;
}

scm_classical_parser::scm_classical_parser (scm_env*e) : scm_parser (e)
{
}

const char* scm_classical_parser::get_parse_error (int i)
{
	switch (i) {
	default:
		return 0;
	}
}
