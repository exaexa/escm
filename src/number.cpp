
#include "types.h"
#include <stdio.h>
#include <math.h>

number::number (scm_env*e, int i)
		: scm (e)
{
	n = i;
}



number::number (scm_env*e, const char*str)
		: scm (e)
{
	float f;
	sscanf (str, "%f", &f);
	n = f;
}

number::number (scm_env*e, double d)
		: scm (e)
{
	n = d;
}

void number::zero()
{
	n = 0;
}

void number::neg()
{
	n *= -1;
}

void number::add (number*a)
{
	n = n + a->n;
}

void number::sub (number*a)
{
	n = n - a->n;
}

void number::mul (number*a)
{
	n = n * a->n;
}

void number::div (number*a)
{
	n = n / a->n;
}

void number::mod (number*a)
{
	n = fmod (n, a->n);
}

void number::pow (number*a)
{
	n =::pow (n, a->n);
}

void number::log (number*a)
{
	if (!a) n =::log (n);
	else n =::log (n) /::log (a->n);
}

void number::exp()
{
	n =::exp (n);
}

