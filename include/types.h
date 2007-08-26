
#ifndef _ESCM_TYPES_
#define _ESCM_TYPES_

class scm_env;

class scm
{
	bool collectable;

public:

	scm (scm_env*)
	{};

	virtual scm* get_child (int) = 0;
	virtual ~scm() = 0;

	friend class scm_env;
};

class continuation : scm
	{};

class frame : scm
	{};

class number : scm
	{};

class text : scm
	{};

class symbol : scm
	{};

class pair : scm
	{};

class closure : scm
	{};

#endif
