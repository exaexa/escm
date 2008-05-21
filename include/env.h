

#ifndef _ESCM_ENV_
#define _ESCM_ENV_

#define ESCM_VERSION_STR "0.0.2"

#include "debug.h"

/*
 * This class contains all the things needed to run scheme. One scm_env is equal
 * to one execution environment with separate memory/procs/data/everything.
 */

#include <set>
using std::set;
#include <list>
using std::list;

class scm_env;

#include "types.h"
#include "builtins.h"
#include "parser.h"

#define new_scm(env, type, params...) \
	(new ((env)->allocate(sizeof(type))) type ((env), ##params))

typedef continuation* (*scm_eval_cont_factory) (scm_env*, scm*);
typedef continuation* (*scm_code_cont_factory) (scm_env*, pair*);
typedef void (*scm_fatal_error_callback) (scm_env*, scm*);

class scm_env
{
public:

	/*
	 * memory allocator
	 */

	class gc_heap_entry
	{
	public:
		void* start;
		size_t size;
		inline bool operator< (const gc_heap_entry& a) const
		{
			return start < a.start;
		}
		inline bool operator== (const gc_heap_entry& a) const
		{
			return start == a.start;
		}
		inline gc_heap_entry()
		{
			start = 0;
			size = 0;
		}
		inline gc_heap_entry (void* st, size_t si)
		{
			start = st;
			size = si;
		}
	};

	size_t align;

	/*
	 * NOTE. (quite important)
	 * align shouldn't be non-power of 2, but also should not be higher
	 * that sizeof(scm) - this could lead to significant memory overhead.
	 */

	set<gc_heap_entry> allocated_space;
	set<gc_heap_entry> free_space;
	set<scm*> collector;
	list<scm*> collector_queue;

	set<gc_heap_entry> allocated_heap;
	size_t min_heap_part_size;

	void add_heap_part (size_t minsize = 0);
	void free_heap_parts();

	void* new_heap_object (size_t size);

	/*
	 * memory management
	 */

	void* allocate (size_t size);
	void deallocate (void*);

	void sort_out_free_space();

	void collect_garbage ();

	/*
	 * scheme machine registers
	 */

	scm *val;
	frame *global_frame;
	continuation *cont;

	/*
	 * Exception handling
	 */


	inline void throw_exception (scm* s)
	{
		throw s;
	}

	inline void throw_string_exception (char* c)
	{
		throw_exception (new_scm (this, string, c)
				 ->collectable<scm>() );
	}

	inline void throw_desc_exception (char*c, scm*s)
	{
		scm*t = new_scm (this, string, c);
		scm*t2 = new_scm (this, pair, s, 0);
		throw_exception (new_scm (this, pair, t, t2)
				 ->collectable<scm>() );
	}

	/*
	 * env uses those functions as an interface to external parts.
	 * Use builtins to set them to "scheme" values.
	 *
	 * NOTE: continuation factories should return pointers to conts
	 * which are marked as collectable. It's guaranteed they will be
	 * "indexed" before gc occurs. Tree generated by parser will be
	 * marked collectable by env.
	 */

	scm_parser* parser;

	scm_eval_cont_factory eval_cont_factory;
	scm_code_cont_factory codevector_cont_factory;
	scm_fatal_error_callback fatal_error;

	/*
	 * "general-purpose" frontends
	 */

	inline scm_env ()
	{
		is_init = false;
	}

	inline ~scm_env()
	{
		if (is_init) release();
	}

	bool is_init;
	bool init (scm_parser* defaultparser = 0,
		   size_t heap_size = 65536,
		   size_t alignment = 4);

	void release ();

	void eval_code (pair*);
	void eval_expr (scm*);
	int eval_string (const char* str, char term_char = 0);

	inline void run_eval_loop()
	{
		while (eval_step() );
	}

	inline bool evaluating()
	{
		return cont ? true : false;
	}

	void add_global (const char*name, scm*data);

	scm* protected_exception;

	inline bool eval_step()
	{
		try {
			if (cont)
				cont->eval_step (this);
		} catch (scm* e) {
			protected_exception = e;
			cont = 0;
			val = 0;
			try {
				globget (t_errorhook);
				if (!val) throw e;
				if (!lambda_p (val) )
					throw_desc_exception ("invalid *error-hook*", e);

				( (lambda*) val)->apply (this, e);

			} catch (scm* e) {
				/*
				 * if even this goes wrong, let's die terribly.
				 * what about passing info to the caller?
				 * or could he set error hook himself?
				 */
				cont = 0;
				val = 0;
				if (fatal_error) fatal_error (this, e);
			}
			protected_exception = 0;
		}
		return cont ? true : false;
	}

	inline scm* get_last_result()
	{
		return val;
	}

	/*
	 * scheme internal handlers
	 *
	 * NOTE: I don't think returning scm* is really necessary
	 * (well we have val and others..) so go check it out.
	 *
	 * TODO, check if continuations need some common operations
	 * (stack push/pop/replace) and possibly code it.
	 */

	frame* push_frame (size_t size);

	inline void push_cont (continuation*c)
	{
		c->parent = cont;
		if (!c->env) c->env = cont ? (cont->env) : global_frame;
		cont = c;
	}

	inline void replace_cont (continuation*c)
	{
		if (cont) {
			c->parent = cont->parent;
			if (!c->env) c->env = cont->env;
			cont = c;
		} else push_cont (c);
	}

	inline void pop_cont()
	{
		if (cont) cont = cont->parent;
	}

	inline void ret (scm*v)
	{
		val = v;
		pop_cont();
	}

	scm* globdef (symbol* sym);
	bool globset (symbol* sym);
	bool globget (symbol* sym);

	scm* lexdef (symbol* sym, int d = 0);
	bool lexset (symbol* sym, int d = 0);
	bool lexget (symbol* sym, int d = 0);

	/*
	 * SCHEME HELPERS
	 *
	 * booleans are guaranteed to have true and false meaning,
	 * so we don't need to alloc new booleans everytime we create them.
	 *
	 * So we need the error hook symbol.
	 */

	boolean *t_true, *t_false;
	symbol *t_errorhook;
	string *t_memoryerror;
};

#endif

