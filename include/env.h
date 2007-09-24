

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
	(new ((env).allocate(sizeof(type))) type (&(env), ##params))

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

	pair *ip, *cv;
	scm *val;
	frame *env, *global_frame;
	continuation *cont;

	/*
	 * "general-purpose" frontends
	 */

	scm_env (size_t heap_size = 65536, size_t alignment = 4);

	~scm_env();

	void eval (scm*);
	void eval_string (const char* str);

	inline void run_eval_loop() {while(ip)eval_step();}
	inline bool evaluating(){return ip?true:false;}

	void eval_step();

	void collect_garbage ();

	/*
	 * scheme internal handlers
	 *
	 * NOTE: I don't think returning scm* is really necessary
	 * (well we have val and others..) so go check it out.
	 */

	scm* push_frame (size_t size);
	scm* frame_set (symbol*s);

	void call();
	void call_tail();
	scm* ret(); // consider forced frame recycling, it would save mem ;)
	scm* push_env (scm**result_save = NULL); // frame magic (let)
	scm* pop_env(); //same recycling problem asi with ret()

	scm* make_closure (pair* ip); //args are in val, code on ip

	/*
	 * TODO, think about -arity of closures - it would be really nice
	 * if it was defined at make_closure, but, well, how?
	 * would it be possible to avoid any *arity prechecking?
	 */

	scm* globdef (symbol* sym);
	bool globset (symbol* sym);
	bool globget (symbol* sym);

	scm* lexdef (symbol* sym, int d = 0);
	bool lexset (symbol* sym, int d = 0);
	bool lexget (symbol* sym, int d = 0);
};

#endif

