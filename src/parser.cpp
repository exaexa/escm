
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
	tok_symbol_or_number,
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
	ts_in_string_backslashed,
	ts_in_sharp,
	ts_in_char,
	ts_in_char_letters,
	ts_after_unquote,
	ts_in_atom,
	ts_in_comment
};

int scm_classical_parser::parse_string (const char*str)
{
	try {
		while (*str) parse_char (* (str++) );
	} catch (int i) {
		reset();
		return i;
	} catch (scm* s) {
		reset();
		return 7; //special scm-OOM error.
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
	t_state = ts_normal;
}

void scm_classical_parser::reset_current_cont()
{
	List<parser_cont> newstack;
	newstack.push_front (stack.back() );
	stack = newstack;
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

	pair*result = cont().result;
	bool vectorize = cont().return_vector;
	stack.pop_front();
	if (vectorize) {
		vector*v = new_scm (env, vector, result);
		result->mark_collectable();
		append (v);
	} else append (result);
}

void scm_classical_parser::append (scm*s)
{
	if (cont().must_pop) throw 2;
	//do appendation
	if (cont().tail_next) {
		if (!cont().result) throw 3;
		* (cont().result_tail) = s;
		cont().must_pop = 1;
		cont().tail_next = 0;
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

enum { //symbol or number type
	type_symbol,
	type_number_decimal,
	type_number_decimal_prefix,
	type_number_octal_prefix,
	type_number_binary_prefix,
	type_number_hex_prefix
};

static bool is_binary_digit (char c)
{
	return (c == '0') || (c == '1');
}

static bool is_octal_digit (char c)
{
	return (c >= '0') && (c <= '7');
}

static bool is_decimal_digit (char c)
{
	return (c >= '0') && (c <= '9');
}

static bool is_hex_digit (char c)
{
	return ( (c >= '0') && (c <= '9') )
	       || ( (c >= 'a') && (c <= 'f') )
	       || ( (c >= 'A') && (c <= 'F') );
}

static bool is_alnum (char c)
{
	if ( (c >= 'a') && (c <= 'z') ) return true;
	if ( (c >= 'A') && (c <= 'Z') ) return true;
	if ( (c >= '0') && (c <= '9') ) return true;
	return false;
}

static int guess_token_type (const String& s)
{
	//first, check if we have a prefix
	int i, len = s.length();
	bool (*digit_check) (char);
	if ( (len > 2) && (s[0] == '#') ) {

		switch (s[1]) {
		case 'b':
			digit_check = is_binary_digit;
			break;
		case 'o':
			digit_check = is_octal_digit;
			break;
		case 'd':
			digit_check = is_decimal_digit;
			break;
		case 'x':
			digit_check = is_hex_digit;
			break;
		default:
			digit_check = 0;
		}
	}

	int cc = 0; //comma count. shloud be <=1
	int dc = 0; //digit count. shloud be >0. also, no digits before minus.
	int minus = 0; //minus unacceptable?
	if (!digit_check) {
		i = 0;
		digit_check = is_decimal_digit;
	} else i = 2;
	for (;i < len;++i)
		if (s[i] == '.') {
			++cc;
			minus = 1;
		} else if (s[i] == '-') {
			if (minus) break;
			minus = 1;
			if (dc) break;
		} else if (!digit_check (s[i]) ) break;
		else ++dc;

	if ( (i == len) && dc) {
		if (s[0] == '#') {
			switch (s[1]) {
			case 'b':
				return type_number_binary_prefix;
			case 'o':
				return type_number_octal_prefix;
			case 'd':
				return type_number_decimal_prefix;
			case 'x':
				return type_number_hex_prefix;
			default:
				throw 6;
			}
		} else return type_number_decimal;
	}

	return type_symbol;
}

void scm_classical_parser::process_token (int type, const String* tok)
{
	switch (type) {

	case tok_paren_open:
		push();
		break;

	case tok_paren_open_vector:
		push();
		cont().return_vector = 1;
		break;

	case tok_paren_close:
		pop();
		break;

	case tok_dot:
		cont().tail_next = 1;
		break;

	case tok_symbol_or_number:
		if (guess_token_type (*tok) == type_symbol)
			append (new_scm (env, symbol, tok->c_str() ) );
		else
			append (new_scm (env, number, tok->c_str() ) );
		break;

	case tok_string:
		append (new_scm (env, string, tok->c_str() ) );
		break;

	case tok_char:
		append (new_scm (env, character, tok->c_str() [0]) );
		break;

	case tok_bool_true:
		append (env->t_true);
		break;

	case tok_bool_false:
		append (env->t_false);
		break;

	case tok_quote:
	case tok_quasiquote:
	case tok_unquote:
	case tok_unquote_splice: {
		const char*t;
		switch (type) {
		case tok_quote:
			t = "quote";
			break;
		case tok_quasiquote:
			t = "quasiquote";
			break;
		case tok_unquote:
			t = "unquote";
			break;
		case tok_unquote_splice:
			t = "unquote-splicing";
			break;
		default:
			throw 6;
		}
		push();
		append (new_scm (env, symbol, t) );
		cont().pop_after_next = 1;
	}
	break;

	default:
		throw 6;
	}
}

static bool is_white (char c)
{
	if (!c) return true;
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
		switch (c) {
		case '"':
			pt (tok_string, &token_rest);
			token_rest.clear();
			t_state = ts_normal;
			break;
		case '\\':
			t_state = ts_in_string_backslashed;
			break;
		default:
			token_rest.append (1, c);
		}
		break;

	case ts_in_string_backslashed:
		switch (c) {
		case '\\':
		case '"':
			token_rest.append (1, c);
			break;

			/*
			 * TODO
			 * someone could invent better "conversion" mechanism ;D
			 */
		case 'a':
			token_rest.append (1, '\a');
			break;

		case 'b':
			token_rest.append (1, '\b');
			break;

		case 'f':
			token_rest.append (1, '\f');
			break;

		case 'n':
			token_rest.append (1, '\n');
			break;

		case 'r':
			token_rest.append (1, '\r');
			break;

		case 't':
			token_rest.append (1, '\t');
			break;

		case 'v':
			token_rest.append (1, '\v');
			break;
		}

		t_state = ts_in_string;

		break;

	case ts_in_sharp:
		switch (c) {
		case '(':
			pt (tok_paren_open_vector);
			t_state = ts_normal;
			break;
		case 't':
		case 'T':
			pt (tok_bool_true);
			t_state = ts_normal;
			break;

		case 'f':
		case 'F':
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
			 * numbers that can possibly be entered via this parser
			 * are exact -> omitted; conversion e<->i shall be
			 * done by some function.
			 */

		case 'b': //binary
		case 'o': //octal
		case 'd': //decimal
		case 'x': //hexadecimal
			t_state = ts_in_atom;
			token_rest = "#";
			token_rest.append (1, c);
			break;
		default:
			throw 5; //undefined sharp expression
		}
		break;

	case ts_in_char:
		token_rest.append (1, c);
		if (is_alnum (c) ) t_state = ts_in_char_letters;
		else {
			t_state = ts_normal;
			pt (tok_char, &token_rest);
			token_rest.clear();
		}
		break;

	case ts_in_char_letters:
		if (!is_alnum (c) ) {
			t_state = ts_normal;
			pt (tok_char, &token_rest);
			token_rest.clear();
			parse_char (c);
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
			if (token_rest == ".") pt (tok_dot);
			else pt (tok_symbol_or_number, &token_rest);
			token_rest.clear();
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
	case 6:
		return "internal parser error";
	case 7:
		return "scheme machine error, most probably out of memory";
	default:
		return 0;
	}
}

