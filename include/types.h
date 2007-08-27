
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
	data_placeholder* table;
	frame* parent;
	size_t size;

	frame (scm_env*, int size);
	void sort(); //do the sort, so da lookup is fasta!
	void load_symbols (pair* list); //or somehow...
	scm* lookup (symbol*); //TODO  (!!! - don't do the lookup recursively!)

	inline scm* get_child (int i)
	{
		switch (i) {
		case 0:
			return table;
		case 1:
			return parent;
		default:
			return 0;
		}
	}
};

class number : public scm
{
	int n; //TODO if someone would... OMG FIX IT! UNLIMITED SIZE!
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
};

class continuation : public scm
{
public:
	scm*ip;
	frame*env;
	continuation*next;
	inline scm* get_child (int i)
	{
		switch (i) {
		case 0:
			return ip;
		case 1:
			return env;
		case 2:
			return next;
		default:
			return 0;
		}
	}
};

#endif
