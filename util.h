/* ***** BEGIN LICENSE BLOCK *****
 * Version: BSD License
 * 
 * Copyright (c) 2006, Diego Casorran <dcasorran@gmail.com>
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 ** Redistributions of source code must retain the above copyright notice,
 *  this list of conditions and the following disclaimer.
 *  
 ** Redistributions in binary form must reproduce the above copyright notice,
 *  this list of conditions and the following disclaimer in the documentation
 *  and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * ***** END LICENSE BLOCK ***** */


#include <string.h>
#include <stdarg.h>
#include <exec/types.h>
#include <dos/exall.h>
#include <dos/dosextens.h>

/****************************************************************************/

/* Address is neither aligned to a word or long word boundary. */
#define IS_UNALIGNED(a) ((((unsigned long)(a)) & 1) != 0)

/* Address is aligned to a word boundary, but not to a long
   word boundary. */
#define IS_SHORT_ALIGNED(a) ((((unsigned long)(a)) & 3) == 2)

/* Address is aligned to a long word boundary. For an 68030 and beyond the
   alignment does not matter. */
#if defined(M68020) || defined(_M68020) || defined(mc68020) || defined(__mc68020)
#define IS_LONG_ALIGNED(a) (1)
#else
#define IS_LONG_ALIGNED(a) ((((unsigned long)(a)) & 3) == 0)
#endif /* M68020 */

/****************************************************************************/
#if 0
/* Types for fib_DirEntryType.	NOTE that both USERDIR and ROOT are	 */
/* directories, and that directory/file checks should use <0 and >=0.	 */
/* This is not necessarily exhaustive!	Some handlers may use other	 */
/* values as needed, though <0 and >=0 should remain as supported as	 */
/* possible.								 */
#define ST_ROOT		1
#define ST_USERDIR	2
#define ST_SOFTLINK	3	/* looks like dir, but may point to a file! */
#define ST_LINKDIR	4	/* hard link to dir */
#define ST_FILE		-3	/* must be negative for FIB! */
#define ST_LINKFILE	-4	/* hard link to file */
#define ST_PIPEFILE	-5	/* for pipes that support ExamineFH */
#endif

/* Handy macros for checking what kind of object a ExAllData's 
   ed_Type describes;  ExAll() */

#ifndef EAD_IS_FILE
# define EAD_IS_FILE(ead)    ((ead)->ed_Type <  0)
#endif
#ifndef EAD_IS_DRAWER
# define EAD_IS_DRAWER(ead)  ((ead)->ed_Type >= 0 && \
                             (ead)->ed_Type != ST_SOFTLINK)
#endif
#ifndef EAD_IS_LINK
# define EAD_IS_LINK(ead)   ((ead)->ed_Type == ST_SOFTLINK || \
                             (ead)->ed_Type == ST_LINKDIR || \
                             (ead)->ed_Type == ST_LINKFILE)
#endif
#ifndef EAD_IS_SOFTLINK
# define EAD_IS_SOFTLINK(ead) ((ead)->ed_Type == ST_SOFTLINK)
#endif
#ifndef EAD_IS_LINKDIR
# define EAD_IS_LINKDIR(ead)  ((ead)->ed_Type == ST_LINKDIR)
#endif

#define EAD_IS_SOMEDRAWER(ead)	\
	(EAD_IS_DRAWER(ead)  ||	\
	 EAD_IS_LINKDIR(ead) ||	\
	(EAD_IS_SOFTLINK(ead) && !(ead)->ed_Size))

/****************************************************************************/

GLOBAL LONG SNPrintf( STRPTR outbuf, LONG size, CONST_STRPTR fmt, ... );

/****************************************************************************/


