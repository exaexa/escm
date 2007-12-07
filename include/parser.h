
#include "env.h"

#ifndef _ESCM_PARSER_
#define _ESCM_PARSER_

class scm_parser {
public:
	inline scm_parser(){}
	inline virtual ~scm_parser(){}


	virtual pair* parse_string(scm_env*e, const char* str)=0;
	virtual void reset()=0;
};

class scm_classical_parser:public scm_parser {
public:	
	virtual pair* parse_string(scm_env*e, const char* str);
	virtual void reset();
};

#endif

