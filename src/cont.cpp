
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

	eval_continuation*c=0;

	if (pair_p (ip) ) {
		c = new_scm (e, eval_continuation, ip->a)
		    ->collectable<eval_continuation>();
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
		c = new_scm (e, eval_continuation, ip)
		    ->collectable<eval_continuation>();
		e->replace_cont (c);
	}
}

void eval_continuation::eval_step (scm_env*e)
{
	if (pair_p (object) ) {
		e->replace_cont (new_scm
				 (e, pair_continuation, (pair*) object)
				 ->collectable<pair_continuation>() );

		return;
	}
	if (symbol_p (object) ) {
		if(! e->lexget ( (symbol*) object))
			e->throw_desc_exception("unbound variable",object);
	} else //tis just an atom.
		e->val = object;
	e->pop_cont();
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
					  (lambda*) selector, list)
					 ->collectable<continuation>() );
		} else if (syntax_p (selector) ) {
			e->replace_cont (new_scm
					 (e, syntax_continuation,
					  (syntax*) selector, list)
					 ->collectable<continuation>() );
		} else if (cont_p (selector) ) {
			e->cont = (continuation*) selector;
			if (pair_p (list->d) )
				e->val = ( (pair*) (list->d) )->a; //cadr list
			/*
			 * ^^^TODO, how should the other
			 * elemets of list be treated? correct implementation
			 * should trigger an error.
			 */
			else
				e->val = list->d; //just rest of list
		} else e->throw_desc_exception("bad selector",selector);
	} else {
		e->push_cont (new_scm (e, eval_continuation, list->a)
			      ->collectable<continuation>() );
		selector_evaluated = true;
	}
}

void syntax_continuation::eval_step (scm_env*e)
{
	if (code) {
		//this should push evaluation of code
		//some syntaxes may replace this continuation
		syn->apply (e, code);
		code = 0;
	} else {
		//replace with eval of val
		e->replace_cont (new_scm
				 (e, eval_continuation, e->val)
				 ->collectable<continuation>() );
	}

	/*
	 * NOTE to self. (just to be damn sure)
	 * Some macros don't even vant the second phase to occur
	 * (e.g. quote, which we mentioned elsewhere). Then apply
	 * doesn't push the evaluation, but uses pop_cont 
	 * (or replace. or whatever.)
	 */
}

void lambda_continuation::eval_step (scm_env*e)
{
	switch (arglist_pos) {
	case 1: //we have evaluated list arg
		arglist = (pair*) (arglist->d);
		* (pair**) evaluated_args_d = new_scm (e, pair, e->val, 0)
					      ->collectable<pair>();
		{
			scm**temp = & ( (*evaluated_args_d)->d);
			evaluated_args_d = (pair**) temp;
		}
	case 0: //initial (note break is missing here with a reason!)
		if (pair_p (arglist) ) {
			arglist_pos = 1;
			e->push_cont (new_scm (e, eval_continuation, arglist->a)
				      ->collectable<continuation>() );
		} else if (arglist) {
			arglist_pos = 2;
			e->push_cont (new_scm (e, eval_continuation, arglist)
				      ->collectable<continuation>() );
		} else {
			l->apply (e, evaluated_args);
		}
		break;
	default: //we have evaluated a non-null list-termination
		* (pair**) evaluated_args_d = (pair*) (e->val);
		l->apply (e, evaluated_args);
	}
}

