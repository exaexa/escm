
#include "parser.h"

/*
 * Oh hai hell! this parser is a little complicated...so no one's
 * gonna shit bricks when we use C++ exception catching. kthxbai.
 */

enum { //token type
	tok_paren_open,
	tok_paren_open_vector,
	tok_paren_close,
	tok_dot,
	tok_number_decimal,
	tok_number_binary,
	tok_number_octal,
	tok_number_hex,
	tok_symbol,
	tok_string,
	tok_char,
	tok_bool_true,
	tok_bool_false,
	tok_quote,
	tok_quasiquote,
	tok_unquote,
	tok_unquote_splice
};


enum { //tokenizer state
	ts_normal,
	ts_in_string,
	ts_in_sharp,
	ts_in_char,
	ts_after_unquote,
	ts_in_atom,
	ts_in_comment
};

int scm_classical_parser::parse_string (const char*str)
{
	try {
		while (*str) parse_char (* (str++) );
	} catch (int i) {
		reset_current_cont();
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
	if (stack.size() > 1) throw 1; //not terminated
	return 0;
}

void scm_classical_parser::reset()
{
	stack = List<parser_cont>();
	stack.push_back (parser_cont() );
	pos.col = pos.row = 0;
	t_state = ts_normal;
}

void scm_classical_parser::reset_current_cont()
{
	List<parser_cont> newstack;
	newstack.push_front (stack.back() );
	stack = newstack;
	pos.col = pos.row = 0;
	t_state = ts_normal;
}

void scm_classical_parser::push()
{
	if (cont().must_pop) throw 2;
	stack.push_front (parser_cont() );
}

void scm_classical_parser::pop()
{
	/*
	 * this throw is needed just because of cleaniness
	 * ( 1 2 3 . ) should cause an error
	 * if this is (simply) commented out, such expression
	 * behaves just like (1 2 3) or (1 2 3 . ()).
	 * kthxbai.
	 */
	if (cont().tail_next) throw 3;
	if (stack.size() <= 1) throw 4;

	scm*result = cont().result;
	stack.pop_front();
	append (result);
}

void scm_classical_parser::append (scm*s)
{
	if (cont().must_pop) throw 2;
	//do appendation
	if (cont().tail_next) {
		if (!cont().result) throw 3;
		* (cont().result_tail) = s;
		cont().must_pop = 1;
	} else {
		pair* tmp = new_scm (env, pair, s, 0);

		if (cont().result)
			* (cont().result_tail) = tmp;
		else
			cont().result = tmp;

		cont().result_tail = & (tmp->d);
	}

	if (cont().pop_after_next) pop();
}

void process_token (int type, const String* tok)
{}

static bool is_white (char c)
{
	if (c == ' ') return true;
	if (c == '\t') return true;
	if (c == '\n') return true;
	return false;
}

void scm_classical_parser::parse_char (char c)
{
#define pt process_token
	switch (t_state) {
	case ts_in_string:
		break;

	case ts_in_sharp:
		switch (c) {
		case '(':
			pt (tok_paren_open_vector);
			t_state = ts_normal;
			break;
		case 't':
			pt (tok_bool_true);
			t_state = ts_normal;
			break;

		case 'f':
			pt (tok_bool_false);
			t_state = ts_normal;
			break;

		case '\\':
			t_state = ts_in_char;
			token_rest = "";
			break;

			/*
			 * NOTE. R5RS here also defines #e and #i prefixes for
			 * exact and inexact numbers. This sux, because all
			 * numbers that can possibly e entered via this parser
			 * are exact. omitted, conversion e<->i shall be
			 * done by some functions.
			 */

		case 'b': //binary
		case 'o': //octal
		case 'd': //decimal
			t_state = ts_in_atom;
			token_rest = "#";
			token_rest.append (1, c);
			break;
		default:
			throw 5; //undefined sharp expression
		}
		break;

	case ts_in_char:
		if (is_white (c) ) {
			t_state = ts_normal;
			pt (tok_char, &token_rest);
		} else
			token_rest.append (1, c);
		break;

	case ts_after_unquote:

		if (c == '@') {
			pt (tok_unquote_splice);
			t_state = ts_normal;
		} else {
			pt (tok_unquote);
			t_state = ts_normal;
			parse_char (c);
		}
		break;

	case ts_in_atom:
		if (is_white (c) || (c == '(') || (c == ')') ) {
			//TODO!!PROCESS THE TOKEN + guess the type of it.
			//TODO TODO
			t_state = ts_normal;
			parse_char (c);
		} else
			token_rest.append (1, c);
		break;

	case ts_in_comment:
		if (c == '\n') t_state = ts_normal;
		break;

	case ts_normal:
	default:
		if (is_white (c) ) return;
		switch (c) {
		case '(':
			pt (tok_paren_open);
			break;
		case ')':
			pt (tok_paren_close);
			break;
		case '\'':
			pt (tok_quote);
			break;
		case '`':
			pt (tok_quasiquote);
			break;
		case ',':
			t_state = ts_after_unquote;
			break;
		case '#':
			t_state = ts_in_sharp;
			break;
		case '"':
			t_state = ts_in_string;
			token_rest = "";
			break;
		case ';':
			t_state = ts_in_comment;
			break;
		default:
			t_state = ts_in_atom;
			token_rest = "";
			token_rest.append (1, c);
			break;
		}
		break;
	}

	//process_token(...) here!
}

scm_classical_parser::scm_classical_parser (scm_env*e) : scm_parser (e)
{
	reset();
}

const char* scm_classical_parser::get_parse_error (int i)
{
	switch (i) {
	case 1:
		return "unexpected stream termination";
	case 2:
		return "list termination expected";
	case 3:
		return "illegal dot expression";
	case 4:
		return "unexpected list termination";
	case 5:
		return "illegal sharp expression";
	default:
		return 0;
	}
}

