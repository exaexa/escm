

#ifndef _ESCM_ENV_
#define _ESCM_ENV_

/*
 * This class contains all the things needed to run lisp. One scm_env is equal
 * to one execution environment with separate memory/procs/data/everything.
 */

#include <map>
using std::map;
#include <vector>
using std::vector;
#include <set>
using std::set;
#include <list>
using std::list;
#include <string>
using std::string;

class scm_env;

class scm
{

};

typedef map<string, scm*> frame;

typedef scm* (*scm_c_handler) (scm*, scm_env*);

class scm_env
{
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

	frame globals;
	vector<frame> stack;

	// Memory nanagment (garbage collector)

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

	void* allocate (size_t size);
	void deallocate (void*);

	scm * new_scm (); //TODO
	void free_scm (scm*);

	void mark_collectable (scm*);

	void sort_out_free_space();

public:
	scm_env (size_t heap_size = 65536, size_t alignment = 4);

	~scm_env();

	int register_c_func (const string& name, scm_c_handler);

	scm* eval (scm*);

	void collect_garbage ();
};

#endif

