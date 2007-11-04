

#ifndef _ESCM_ENV_
#define _ESCM_ENV_

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

class scm_env
{
	/*
	 * memory allocator
	 */

	class gc_heap_entry
	{
	public:
		size_t start;
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
		inline gc_heap_entry (size_t st, size_t si)
		{
			start = st;
			size = si;
		}
	};

	void* heap;
	size_t hs, align;

	/*
	 * NOTE. (quite important)
	 * align shouldn't be non-power of 2, but also should not be higher
	 * that sizeof(scm) - this could lead to significant memory overhead.
	 */

	set<gc_heap_entry> allocated_space;
	set<gc_heap_entry> free_space;
	set<scm*> collector;
	list<scm*> collector_queue;

	void* new_heap_object (size_t size);

public:

	void* allocate (size_t size);
	void deallocate (void*);

	void sort_out_free_space();

	/*
	 * scheme machine registers
	 *
	 * see this:
	 * http://www.mazama.net/scheme/devlog/2006/11/14
	 */

	scm *val;
	frame *global_frame;
	continuation *cont;

	/*
	 * "general-purpose" frontends
	 */

	scm_env (size_t heap_size = 65536, size_t alignment = 4);

	~scm_env();

	void eval_code (scm*);
	void eval_expr (scm*);
	void eval_string (const char* str);

	inline void run_eval_loop()
	{
		while (eval_step() );
	}

	inline bool evaluating()
	{
		return cont ? true : false;
	}

	inline bool eval_step()
	{
		if (cont)
			cont->eval_step (this);
		return cont ? true : false;
	}

	inline scm* get_last_result()
	{
		return val;
	}

	void collect_garbage ();

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
		cont = c;
	}

	inline void replace_cont (continuation*c)
	{
		if (cont) {
			c->parent = cont->parent;
			cont = c;
		} else printf ("!!! replace_cont misused, tried to replace with %p\n", c);
	}

	inline void pop_cont()
	{
		if (cont) cont = cont->parent;
	}

	scm* globdef (symbol* sym);
	bool globset (symbol* sym);
	bool globget (symbol* sym);

	scm* lexdef (symbol* sym, int d = 0);
	bool lexset (symbol* sym, int d = 0);
	bool lexget (symbol* sym, int d = 0);
};

#endif

