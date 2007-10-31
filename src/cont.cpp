
#include "cont.h"
#include "env.h"

void codevector_continuation::eval_step (scm_env*e)
{
	/*
	 * If we come to list, we evaluate car and move ip to cdr.
	 * If cdr is null, or ip is not a list, we replace ourselves 
	 * with evaluation.
	 * if ip is null, we pop ourselves.
	 */

	eval_continuation*c;
	if (pair_p (ip) ) {
		if (ip->d) {
			//push eval
		} else {
			//replace with eval
		}
	} else if (ip) {
		//list terminated by value (replace with eval)
	} else {
		//no list
	}
}

void eval_continuation::eval_step (scm_env*e)
{
	if(pair_p(object)) {
		//replace with pair continuation
	} if(symbol_p(object)){
		//lexget
	} else
		e->val=object;
}

void pair_continuation::eval_step(scm_env*e)
{
	if(selector_evaluated){
		//replace with lambda or syntax, or cause a bad-selector error
	} else {
		//push eval of first frame
	}
}

void syntax_continuation::eval_step(scm_env*e)
{
	syn->apply(e,code);
	//replace continuation with eval of val
}

void lambda_continuation::eval_step(scm_env*e)
{

}
