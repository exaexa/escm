
#include "env.h"

#ifndef _ESCM_PARSER_
#define _ESCM_PARSER_

class scm_parser {
	scm_env*env;
public:
	inline scm_parser(scm_env*e):env(e){}
	inline virtual ~scm_parser(){}

	virtual pair* parse_string(const char* str)=0;
	virtual void reset()=0;
};

#include <string>
#include <list>
#define String std::string
#define List std::list

class scm_classical_parser:public scm_parser {

	/*
	 * how does the parser work.
	 * 1] we have one parser_cont pushed everytime, tis the cont with
	 * 	results. Shall it get popped, we create an error.
	 * 2] we parse tokens and append them, sometimes we modify flags.
	 * 3] on pop, we append the res of popped continuation to the
	 * 	older continuation
	 */

	class parser_cont{
	public:
		pair *res,*res_tail;
		int flags;
	};

	List<parser_cont> stack;

	parser_cont& cont();

	void push();
	void pop();
	void append(scm*);
	
	void process_token(const String&);

	/*
	 * tokenizer
	 * we push chars, then we get tokens (sometimes)
	 */

	String token_rest;

	bool parse_char(char c, String&output_token);

	/*
	 * This is the environment we work with
	 */

public:	
	inline scm_classical_parser(scm_env*e):scm_parser(e){}
	virtual pair* parse_string(const char* str);
	virtual void reset();
};

#endif

