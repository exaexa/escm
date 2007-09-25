
#ifndef _ESCM_TYPES_
#define _ESCM_TYPES_

#include <stddef.h>

class scm_env;

/*
 * here we need multiplatform last-addressable-byte pointer (0xfff...fff)
 * TODO is: get it somewhere from library headers, or compute it somehow,
 * if it can be done. anyhow. whatever. plz.
 */

#ifdef __i386__
# define scm_no_more_children (scm*)0xFfffFfff
#endif

#ifdef __x86_64__
# define scm_no_more_children (scm*)0xFfffFfffFfffFfff
#endif

#ifndef scm_no_more_children //last chance, compute it haxor-way!
# define scm_no_more_children (scm*)(~(unsigned long)0)
#endif

#define scmf_nocollect 0x01

#define mark_scm_collectable(s) (s)->flags&=~scmf_nocollect
#define is_scm_protected(s) ((s)->flags&scmf_nocollect)

#include <typeinfo>
#define is_type(a,b) (typeid(a)==typeid(b))

/*
 * scm
 *
 * the generic class for storing any scheme object. This (and derivees) is
 * the only thing that will ever appear in garbage collector.
 */

class scm
{
protected:
	char flags;
public:

	inline scm (scm_env*)
	{
		flags = scmf_nocollect;
	}

	virtual scm* get_child (int)
	{
		return scm_no_more_children;
	}

	virtual ~scm()
	{}

	friend class scm_env;
	void mark_collectable();
};

/*
 * BASIC TYPES
 */

class pair : public scm
{
public:
	scm *a, *d;

	scm *get_child (int i)
	{
		if (!i) return a ;
		if (i > 1) return scm_no_more_children;
		return d;
	}

	pair (scm_env*e, scm*addr = 0, scm*dat = 0) : scm (e)
	{
		a = addr;
		d = dat;
	}
};

#define quote_type_quote 0
#define quote_type_backquote 1
#define quote_type_comma 2
#define quote_type_splice 3

class quote : public scm
{
public:
	char type;
	scm* child;

	inline quote (scm_env*e, scm*chld, char t) : scm (e)
	{
		child = chld;
		type = t;
	}

	inline bool is_quote()
	{
		return type == quote_type_quote;
	}

	inline bool is_backquote()
	{
		return type == quote_type_backquote;
	}

	inline bool is_comma()
	{
		return type == quote_type_comma;
	}

	inline bool is_splice()
	{
		return type == quote_type_splice;
	}
};


/*
 * Data placeholder is here for data:D
 * we need structure which is somehow scalable (by the means of size)
 * AND behaves well when garbage collected.
 *
 * Place for data is allocated BEHIND this class and recycled with it.
 *
 * please use macros following below
 */

class data_placeholder : public scm
{
public:
	data_placeholder (scm_env*e) : scm (e)
	{}
	//nothing? ooh! really? (make up something!)
};

#define dataof(p) ((p)+1) //+ sizeof(p). hackish. TODO solve the type of this.
#define new_data_scm(env, size) \
	(new ((env).allocate(sizeof(data_placeholder)+(size)))\
		data_placeholder(&(env)))


/*
 * ATOMS
 */

class boolean: public scm
{
public:
	bool b;
	inline boolean (scm_env*e, bool a) : scm (e)
	{
		b = a;
	}
};

class character: public scm
{
public:
	char c;
	inline character (scm_env*e, char a) : scm (e)
	{
		c = a;
	}
};


/*
 * symbols are case insensitive, so we store them in upper case.
 */

class number : public scm
{
	int n; //TODO if someone would... OMG FIX IT! UNLIMITED SIZE WE WANT!
};

class text : public scm
{
public:
	data_placeholder*d;

	inline scm* get_child (int i)
	{
		if (i) return scm_no_more_children ;
		else return d;
	}

	text (scm_env*, const char*);

	inline operator const char* ()
	{
		return (const char*) dataof (d);
	}
};

class symbol : public text
{
public:
	symbol (scm_env*, const char*);

	int cmp (symbol*);
};


/*
 * FRAMES
 */

class frame : public scm
{
public:
	frame* parent;

	inline frame (scm_env*e) : scm (e)
	{}

	virtual bool lookup (symbol*, scm**);
	virtual bool set (symbol*, scm*);
	virtual scm* define (scm_env*e, symbol*, scm*);

	virtual scm* get_child (int);

	/*
	 * think about: what if we also created a recursive
	 * lookup function just here?
	 */
};

//normal binding pair
class frame_entry : public scm
{
public:
	symbol*name;
	scm*content;

	inline frame_entry (scm_env*e, symbol*s, scm*c) : scm (e)
	{
		name = s;
		content = c;
	}

	virtual scm* get_child (int i)
	{
		switch (i) {
		case 0:
			return name;
		case 1:
			return content;
		default:
			return scm_no_more_children;
		}
	}
};

//for hash tables, with a pointer to
class chained_frame_entry : public frame_entry
{
public:
	chained_frame_entry*next;

	inline chained_frame_entry (scm_env*e,
	                            symbol*s, scm*c, chained_frame_entry*n)
			: frame_entry (e, s, c)
	{
		next = n;
	}

	virtual scm* get_child (int i)
	{
		switch (i) {
		case 0:
			return name;
		case 1:
			return content;
		case 2:
			return next;
		default:
			return scm_no_more_children;
		}
	}
};

class hashed_frame : public frame
{
public:
	data_placeholder* table;

	hashed_frame (scm_env*);

	virtual bool lookup (symbol*, scm**);
	virtual bool lookup_frame (symbol*, chained_frame_entry**, int hash = -1);
	virtual bool set (symbol*, scm*);
	virtual scm* define (scm_env*e, symbol*, scm*);

	virtual scm* get_child (int);
};

class local_frame : public frame
{
public:
	data_placeholder* table; //binary lookup in a small table
	size_t size, used;

	local_frame (scm_env*, size_t);

	virtual bool lookup (symbol*, scm**);
	virtual bool set (symbol*, scm*);
	virtual scm* define (scm_env*e, symbol*, scm*);

	virtual scm* get_child (int);
	/*
	 * TODO, decide, whether define should make a new size-1 frame 
	 * (which would result in slower lookups), or try to modify the
	 * actual table somehow (which might result in reallocs and possibly
	 * eat all our base (memory))
	 */
private:
	int get_index (symbol*);
};


/*
 * LAMBDAS
 */

class lambda : public scm
{
public:
	inline lambda (scm_env* e) : scm (e)
	{}

	virtual void call (scm_env* e);
	/*
	 * about calls.
	 * This function here should
	 * a] external functions are called normally, receive scm_env*
	 * b] closures come with arglist in var, so the
	 * 	frame with arguments mapped to correct names
	 * 	is pushed to env. ip has to be set.
	 */
};

typedef scm* (*scm_c_func) (scm_env*);

class extern_func : public lambda
{
public:
	scm_c_func handler;

	inline extern_func (scm_env*e, scm_c_func h = 0) : lambda (e)
	{
		handler = h;
	}

	inline virtual void call (scm_env* e)
	{
		handler (e);
	}
};

class closure : public lambda
{
public:
	pair *arglist;
	pair *ip;
	frame *env;
	size_t paramcount;

	closure (scm_env*e, pair*arglist, pair*ip, frame*env);

	virtual void call (scm_env* e);

	inline scm* get_child (int i)
	{
		switch (i) {
		case 0:
			return ip;
		case 1:
			return arglist;
		case 2:
			return env;
		default:
			return scm_no_more_children;
		}
	}
	/*
	 * NOTE. it would be really nice to save some information about
	 * how many args should be allocated in frame for the procedure,
	 * we would't need to count them everytime. think about it.
	 */
};


/*
 * CONTINUATION
 */

class continuation : public scm
{
public:
	pair *ip;
	int et;
	frame*env;
	continuation*parent;
	scm**val_save;

	inline continuation (scm_env*e, pair*i, int e_t, frame*en,
	                     continuation*p, scm**val_s) : scm (e)
	{
		ip = i;
		et=e_t;
		env = en;
		parent = p;
		val_save = val_s;
	}

	inline scm* get_child (int i)
	{
		switch (i) {
		case 0:
			return ip;
		case 1:
			return env;
		case 2:
			return parent;
		default:
			return scm_no_more_children;
		}
	}
};

#endif
