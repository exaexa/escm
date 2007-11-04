
#include "cont.h"
#include "env.h"

/*
 * NOTICE
 * 	for future generations and port writers.
 *
 * hereby continuations do bad stuff when are replaced by some other,
 * just because the function here still runs when its continuation
 * might already be garbage collected (see, say, lambda_cont). This, if
 * our C++ implementation is clean and we don't use any local variables
 * whose destructors mess with *this, should never hurt anything.
 * If C++ evolved further (and this eventually would have fucked up),
 * this is the first thing we should search the bugs in.
 */

void codevector_continuation::eval_step (scm_env*e)
{
	/*
	 * If we come to list, we evaluate car and move ip to cdr.
	 * If cdr is null, or ip is not a list, we replace ourselves 
	 * with evaluation.
	 * if ip is null, we pop ourselves.
	 */

	if (!ip) {
		//(), means "return null"
		e->val = 0;
		e->pop_cont();
	}

	eval_continuation*c;

	if (pair_p (ip) ) {
		c = new_scm (e, eval_continuation, ip->a);
		c->mark_collectable();
		if (ip->d) {
			//push eval of car, move ip
			e->push_cont (c);
			ip = (pair*) (ip->d);
		} else {
			//replace with eval
			e->replace_cont (c);
		}
	} else {
		//list seems to be terminated by something else
		//than null sentinel -> return it!
		c = new_scm (e, eval_continuation, ip);
		c->mark_collectable();
		e->replace_cont (c);
	}
}

void eval_continuation::eval_step (scm_env*e)
{
	if (pair_p (object) ) {
		e->replace_cont (new_scm
		                 (e, pair_continuation, (pair*) object) );
	}
	if (symbol_p (object) ) {
		e->lexget ( (symbol*) object);
		e->pop_cont();
	} else //is just an atom.
		e->val = object;
}

void pair_continuation::eval_step (scm_env*e)
{
	//note: we are sure that pair_p(list) is true.
	if (selector_evaluated) {
		scm*selector = e->val;
		//replace with lambda or syntax, or cause bad selector err

		if (lambda_p (selector) ) {
			e->replace_cont (new_scm
			                 (e, lambda_continuation,
					 (lambda*) selector, list) );
		} else if (syntax_p (selector) ) {
			e->replace_cont (new_scm
			                 (e, syntax_continuation,
					 (syntax*) selector, list) );
		} else if (cont_p (selector) ) {
			e->cont = (continuation*) selector;
			if (pair_p (list->d) )
				e->val = ( (pair*) (list->d) )->a; //cadr list
			/*
			 * ^^^TODO, how should the other
			 * elemets of list be treated?
			 */
			else
				e->val = list->d; //just rest of list
		} else {
			//error, bad selector.
			//TODO trigger an error, now we just ignore it
			e->val = NULL;
			e->pop_cont();
		}
	} else {
		e->push_cont (new_scm (e, eval_continuation, list->a) );
		selector_evaluated = true;
	}
}

void syntax_continuation::eval_step (scm_env*e)
{
	if (code) {
		//push evaluation of code
		syn->apply (e, code);
		code = 0;
	} else {
		//replace with eval of val
	}
}

void lambda_continuation::eval_step (scm_env*e)
{
	if (arglist) {
		//eval next argument
	} else {
		//apply a lambda
		l->apply (e, evaluated_args);
	}
}

