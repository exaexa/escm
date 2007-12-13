
#include "env.h"

#ifndef _ESCM_PARSER_
#define _ESCM_PARSER_

class scm_parser
{
protected:
	scm_env*env;
public:
	inline scm_parser (scm_env*e) : env (e)
	{}
	inline virtual ~scm_parser()
	{}

	virtual int parse_string (const char* str) = 0;
	virtual pair* get_result (bool keep) = 0;
	virtual void reset() = 0;
	virtual int end_stream() = 0;
	virtual const char* get_parse_error (int);
};

#include <string>
#include <list>
#define String std::string
#define List std::list

class scm_classical_parser: public scm_parser
{

	/*
	 * how does the parser work.
	 * 1] we have one parser_cont pushed everytime, tis the cont with
	 * 	results. Shall it get popped, we create an error.
	 * 2] we parse tokens and append them, sometimes we modify flags.
	 * 3] on pop, we append the res of popped continuation to the
	 * 	older continuation
	 */

	class parser_cont
	{
	public:
		pair *result;
		scm **result_tail;

		union{
			unsigned int flags;
			struct {
			unsigned tail_next:
				1;
			unsigned must_pop:
				1;
			unsigned pop_after_next:
				1;
			unsigned return_vector:
					       1;
			};
		};

		inline parser_cont()
		{
			result = 0;
			result_tail = 0;
			flags = 0;
		}
	};

	List<parser_cont> stack;

	inline parser_cont& cont()
	{
		return stack.front();
	}

	void push();
	void pop();
	void append (scm*);

	void process_token (int type, const String&);

	/*
	 * tokenizer
	 * we push chars, then we get tokens (sometimes)
	 */

	String token_rest;
	int t_state;

	void parse_char (char c);

	/*
	 * interface
	 */

public:
	struct
	{
		int col, row;
	}
	pos;

	scm_classical_parser (scm_env*);
	virtual int parse_string (const char* str);
	virtual pair* get_result (bool);
	virtual void reset();
	void reset_current_cont();
	virtual int end_stream();
	virtual const char* get_parse_error (int);
};

#endif

