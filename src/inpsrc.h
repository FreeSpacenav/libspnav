/*
This file is part of libspnav, part of the spacenav project (spacenav.sf.net)
Copyright (C) 2007-2018 John Tsiombikas <nuclear@member.fsf.org>

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
3. The name of the author may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
OF SUCH DAMAGE.
*/
/* input source abstraction */
#ifndef INPSRC_H_
#define INPSRC_H_

struct display;

struct input_src {
	const char *name;	/* module name */
	int fd;				/* file descriptor for select */
	void *data;			/* extra data */

	struct display *disp;	/* display callbacks */

	int (*open)(void);
	int (*close)(void);

	int (*pending)(void);
	int (*process)(void);
	int (*dropev)(int);

	int (*set_flt)(double);
	int (*set_int)(int);
	int (*set_str)(const char*);

	int (*get_flt)(double*);
	int (*get_int)(int*);
	int (*get_str)(char*, int);
};

struct display {
	struct input_src *inp;
	int width, height;
	int num_colors;

	void *(*map)(void);
	int (*unmap)(void);

	int (*color)(unsigned int, unsigned int);
	int (*cursor)(int, int);
	int (*print)(const char*);
	int (*clear)(void);
	int (*rect)(int, int, int, int);
	int (*fill)(int, int, int, int);
	int (*line)(int, int, int, int);
};


#endif	/* INPSRC_H_ */
