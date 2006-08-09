#ifndef INCLUDED_GLOBAL_H
#define INCLUDED_GLOBAL_H

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <assert.h>
#include <glib.h>

#include "../config.h"


/**********************************
  Macros to display debugging data
***********************************/
#ifdef _DEBUG

#define MALLOC_CHECK_ 2

/* X is a string */
#define DEBUG(X) fprintf(stderr,"DEBUG in %s at line %d: %s\n",__FILE__,__LINE__,X)
/* Just use like printf */
#define DEBUGf(X, args...) fprintf(stderr,"DEBUG in %s at line %d: " X "\n",__FILE__,__LINE__, args)

/* assert that expression is true. If it fails then execution is halted. */
#define ASSERT(X) if(!(X)){\
    fprintf(stderr,"ASSERT(" #X ") failed! in %s at line %d\n",__FILE__,__LINE__);\
}

#else

#define DEBUG(X)
#define DEBUGf(X, args...)

#define ASSERT(X)

#endif

/****************************
  Macros to display warnings
*****************************/
#define WARN(X) fprintf(stderr, PACKAGE ": %s\n", X)
#define WARNf(X, args...) fprintf(stderr , PACKAGE ": " X "\n", args)

/*******************************
  Macros to terminate execution
          with an error
*******************************/
#define DIE(X) {DEBUG(X);fprintf(stderr,#X "\n");gswat_deinit();exit(EXIT_FAILURE);}
#define DIEf(X, args...) {DEBUGf(X, args);fprintf(stderr, PACKAGE ": " X "\n", args);gswat_deinit();exit(EXIT_FAILURE);}

#define CASE(X) case X: DEBUG("CASE: " #X);


void gswat_deinit(void);

#define False 0
#define True 1


#endif /* INCLUDED_GLOBAL_H */

