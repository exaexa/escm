
#include "types.h"
#include <sstream>

std::string pair::display_internal (int style)
{
	pair*p = this;
	std::string buf = "(";
	while (1) {
		buf += p->a->display (0);
		if (!p->d) {
			buf += ")";
			break;
		}
		buf += " ";
		if (pair_p (p->d) ) {
			p = pair_p (p->d);
			continue;
		} else {
			buf += ". ";
			buf += p->d->display (0);
			buf += ")";
			break;
		}
	}
	return buf;
}

std::string number::display_internal (int style)
{
	std::ostringstream os;
	os << n;
	return os.str();
}

std::string symbol::display_internal (int style)
{
	return (const char*) (*this);
}

std::string string::display_internal (int style)
{
	if (!style) return std::string ("\"") + (const char*) (*this) + "\"";
	return (const char*) (*this);
}

std::string boolean::display_internal (int style)
{
	return ( b ? "#T" : "#F");
}

std::string character::display_internal (int style)
{
	if (style) return std::string (1, c);
	return std::string ("#\\").append (1, c);
}

std::string extern_func::display_internal (int style)
{
	std::ostringstream os;
	os << "#<extern_func@" << handler << ">";
	return os.str();
}

std::string closure::display_internal (int style)
{
	std::ostringstream os;
	os << "#<closure args[" << paramsize << "] "
	<< arglist->display()
	<< " code "
	<< ip->display()
	<< " env "
	<< env->display() << ">";
	return os.str();
}

std::string extern_syntax::display_internal (int style)
{
	std::ostringstream os;
	os << "#<extern_syntax h=" << handler << ">";
	return os.str();
}

std::string macro::display_internal (int style)
{
	std::ostringstream os;
	os << "#<macro argname "
	<< argname->display()
	<< " code "
	<< code->display()
	<< ">";
	return os.str();
}

std::string vector::display_internal (int style)
{
	std::ostringstream os;
	os << "#(";
	for (size_t i = 0;i < size;++i) {
		if (i) os << " ";
		os << ref (i)->display();
	}
	os << ")";
	return os.str();
}

std::string scm::display_internal (int style)
{
	std::ostringstream os;
	os << "#<scm " << typeid (*this).name() << "@" << this << ">";
	return os.str();
}

