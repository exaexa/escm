
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

#define scmf_nocollect 	0x01
#define scmf_quote 	0x02
#define scmf_backquote  0x04
#define scmf_comma	0x08
#define scmf_splice	0x10

#define mark_scm_collectable(s)	(s)->flags&=~scmf_nocollect
#define is_scm_protected(s)	((s)->flags&scmf_nocollect)

#define is_scm_quote(s) 	((s)->flags&scmf_quote)
#define is_scm_backquote(s) 	((s)->flags&scmf_backquote)
#define is_scm_comma(s) 	((s)->flags&scmf_comma)
#define is_scm_splice(s) 	((s)->flags&scmf_splice)

#define mark_scm_quote(s) 	(s)->flags&=~scmf_quote
#define mark_scm_backquote(s) 	(s)->flags&=~scmf_backquote
#define mark_scm_comma(s) 	(s)->flags&=~scmf_comma
#define mark_scm_splice(s) 	(s)->flags&=~scmf_splice

/*
 * scm
 *
 * the generic class for storing any scheme object.
 * This is the only thing that will appear in garbage collector.
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
};

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

typedef scm* (*scm_c_func) (scm*, scm_env*);

class extern_func : public scm
{
public:
	scm_c_func handler;

	extern_func (scm_env*e, scm_c_func h = 0) : scm (e)
	{
		handler = h;
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

class symbol : public scm
{
public:
	data_placeholder * d;

	inline scm* get_child (int i)
	{
		if (i) return scm_no_more_children ;
		else return d;
	}

	symbol (scm_env*, const char*);

	inline operator const char* ()
	{
		return (const char*) dataof (d);
	}

	int cmp (symbol*);
};

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

	text (scm_env*, const char*); //TODO

	inline operator const char* ()
	{
		return (const char*) dataof (d);
	}
};


/*
 * FRAMES
 */

class frame : public scm
{
public:
	frame* parent;

	virtual scm* lookup (symbol*) = 0;
	virtual scm* set (symbol*, scm*) = 0;
	virtual scm* define (symbol*, scm*) = 0;
	virtual scm* get_child (int) = 0;
};

class hashed_frame : public frame
{
public:
	data_placeholder* table;

	hashed_frame (scm_env*);

	virtual scm* lookup (symbol*);
	virtual scm* set (symbol*, scm*);
	virtual scm* define (symbol*, scm*);
	virtual scm* get_child (int);
};

class local_frame : public frame
{
public:
	data_placeholder* table; //binary lookup in the small table
	size_t size, used;

	local_frame (scm_env*, size_t);

	virtual scm* lookup (symbol*);
	virtual scm* set (symbol*, scm*);
	virtual scm* define (symbol*, scm*);
	virtual scm* get_child (int);
	/*
	 * TODO, decide, whether define should make a new size-1 frame 
	 * (which would result in slower lookups), or try to modify the
	 * actual table somehow (which might result in reallocs and possibly
	 * eat all our base (memory))
	 */
};


/*
 * INTERNAL STRUCTURES
 */

class closure : public scm
{
	pair *code, *arglist;

	inline scm* get_child (int i)
	{
		switch (i) {
		case 0:
			return code;
		case 1:
			return arglist;
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

class continuation : public scm
{
public:
	scm*ip;
	frame*env;
	continuation*parent;

	inline continuation (scm_env*e, scm*i, frame*en, continuation*p) : scm (e)
	{
		ip = i;
		env = en;
		parent = p;
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
