/*****************************************************************************/
/*                                                                           */
/*                                 signal.h                                  */
/*                                                                           */
/*                        Signal handling definitions                        */
/*                                                                           */
/*                                                                           */
/*                                                                           */
/* (C) 2002      Ullrich von Bassewitz                                       */
/*               Wacholderweg 14                                             */
/*               D-70597 Stuttgart                                           */
/* EMail:        uz@musoftware.de                                            */
/*                                                                           */
/*                                                                           */
/* This software is provided 'as-is', without any expressed or implied       */
/* warranty.  In no event will the authors be held liable for any damages    */
/* arising from the use of this software.                                    */
/*                                                                           */
/* Permission is granted to anyone to use this software for any purpose,     */
/* including commercial applications, and to alter it and redistribute it    */
/* freely, subject to the following restrictions:                            */
/*                                                                           */
/* 1. The origin of this software must not be misrepresented; you must not   */
/*    claim that you wrote the original software. If you use this software   */
/*    in a product, an acknowledgment in the product documentation would be  */
/*    appreciated but is not required.                                       */
/* 2. Altered source versions must be plainly marked as such, and must not   */
/*    be misrepresented as being the original software.                      */
/* 3. This notice may not be removed or altered from any source              */
/*    distribution.                                                          */
/*                                                                           */
/*****************************************************************************/



#ifndef _SIGNAL_H
#define _SIGNAL_H



/* sig_atomic_t */
typedef unsigned char sig_atomic_t;

/* Type of a signal handler */
typedef void (*__sigfunc) (int);

/* Standard signal handling functions */
#define SIG_IGN         ((__sigfunc) 0x0000)
#define SIG_ERR         ((__sigfunc) 0xFFFF)
#define SIG_DFL         ((__sigfunc) 0xFFFE)

/* Signal numbers */
#define SIGABRT         0
#define SIGFPE          1
#define SIGILL          2
#define SIGINT          3
#define SIGSEGV         4
#define SIGTERM         5

/* Function declarations */
__sigfunc signal (int sig, __sigfunc func);
int raise (int sig);



/* End of signal.h */
#endif



