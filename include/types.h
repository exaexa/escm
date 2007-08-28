
#ifndef _ESCM_TYPES_
#define _ESCM_TYPES_

#include <stddef.h>

class scm_env;

#define scmf_nocollect 0x01

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
		return 0;
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
		if (i > 1) return 0;
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
	//nothing? ooh! really? (make up something!)
};

#define dataof(p) ((p)+1) //+ sizeof(p). hackish. TODO solve the type of this.
#define new_data_scm(env, size) \
	(new ((env).allocate(sizeof(data_placeholder)+(size)))\
		data_placeholder)

class symbol : public scm
{
public:
	data_placeholder * d;
	inline scm* get_child (int i)
	{
		if (i) return 0 ;
		else return d;
	}
	symbol (scm_env*, const char*); //TODO

	//(note: symbols are case-insensitive, so downcase them on init.)
};

class frame : public scm
{
public:
	frame* parent;

	virtual scm* lookup (symbol*) = 0;
	virtual scm* set (symbol*, scm*) = 0;
	virtual scm* define (symbol*, scm*) = 0;
};

class hashed_frame : public frame
{
public:
	data_placeholder* table;

	hashed_frame (scm_env*);

	virtual scm* lookup (symbol*);
	virtual scm* set (symbol*, scm*);
	virtual scm* define (symbol*, scm*);
};

class local_frame : public frame
{
public:
	data_placeholder* table; //binary lookup in the small table
	size_t size, used;

	hashed_frame (scm_env*, size_t);

	virtual scm* lookup (symbol*);
	virtual scm* set (symbol*, scm*);
	virtual scm* define (symbol*, scm*);
	/*
	 * TODO, decide, whether define should make a new size-1 frame, or
	 * try to modify the table somehow.
	 */
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
		if (i) return 0 ;
		else return d;
	}

	text (scm_env*, const char*); //TODO
};

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
			return 0;
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
			return 0;
		}
	}
};

#endif
