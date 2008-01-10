
#ifndef _ESCM_DEBUG_
#define _ESCM_DEBUG_

#include <stdio.h>

#ifdef DEBUG
# define dprint(params...) fprintf(stderr,params)
#else
# define dprint(params...)
#endif

#endif

