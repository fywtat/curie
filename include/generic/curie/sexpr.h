/*
 *  sexpr.h
 *  libcurie
 *
 *  Created by Magnus Deininger on 01/06/2008.
 *  Copyright 2008 Magnus Deininger. All rights reserved.
 *
 */

/*
 * Copyright (c) 2008, Magnus Deininger All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer. *
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution. *
 * Neither the name of the project nor the names of its contributors may
 * be used to endorse or promote products derived from this software
 * without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*! \file
 *  \brief S-expressions
 *
 *  Functions to parse, write and otherwise handle symbolic expressions, as used
 *  in lisp-like programming languages.
 */

#ifndef LIBCURIE_SEXPR_H
#define LIBCURIE_SEXPR_H

#include <curie/io.h>

enum sx_type {
    sxt_nil = 0,
    sxt_false = 1,
    sxt_true = 2,
    sxt_integer = 3,
    sxt_string = 4,
    sxt_symbol = 5,
    sxt_cons = 6,
    sxt_empty_list = 7,
    sxt_end_of_list = 8,
    sxt_end_of_file = 9,
    sxt_not_a_number = 10,
    sxt_nonexistent = 11,
    sxt_dot = 12
};

/*! \brief Generic Sexprs
 */
struct sexpr {
    enum sx_type type;
    signed int references;
};

/*! \brief Sexpr with an Integer Payload
 *  \internal
 */
struct sexpr_integer {
    enum sx_type type;
    signed int references;

    signed long integer;
};

/*! \brief A Pair of Sexprs
 *  \internal
 */
struct sexpr_cons {
    enum sx_type type;
    signed int references;

    /*@shared@*/ struct sexpr *car;
    /*@shared@*/ struct sexpr *cdr;
};

/*! \brief Sexpr with a String or Symbol Payload
 *  \internal
 */
struct sexpr_string_or_symbol {
    enum sx_type type;
    signed int references;

    char character_data[];
};

struct sexpr_io {
  struct io *in, *out;
};

/*@notnull@*/ /*@only@*/ struct sexpr_io *sx_open_io(/*@notnull@*/ /*@only@*/ struct io *, /*@notnull@*/ /*@only@*/ struct io *);

#define sx_open_io_fd(in,out) sx_open_io (io_open ((in)), io_open((out)))
#define sx_open_stdio() sx_open_io_fd(0, 1)

void sx_close_io (/*@notnull@*/ /*@only@*/ struct sexpr_io *);

/*@notnull@*/ /*@shared@*/ struct sexpr *sx_read(/*@notnull@*/ struct sexpr_io *);
void sx_write(/*@notnull@*/ struct sexpr_io *, /*@notnull@*/ /*@shared@*/ struct sexpr *);

/*@notnull@*/ /*@shared@*/ struct sexpr *cons(/*@shared@*/ struct sexpr *, /*@shared@*/ struct sexpr *);
/*@notnull@*/ /*@shared@*/ struct sexpr *make_integer(signed long);
/*@notnull@*/ /*@shared@*/ struct sexpr *make_string(/*@notnull@*/ const char *);
/*@notnull@*/ /*@shared@*/ struct sexpr *make_symbol(/*@notnull@*/ const char *);

void sx_destroy(/*@notnull@*/ /*@shared@*/ struct sexpr *);
void sx_xref(/*@notnull@*/ /*@shared@*/ struct sexpr *);

void *sx_list_map (struct sexpr *, void (*)(struct sexpr *, void *), void *);
struct sexpr *sx_list_fold (struct sexpr *, void (*)(struct sexpr *));

struct sexpr *equalp (struct sexpr *, struct sexpr *);

extern struct sexpr * const sx_nil;
extern struct sexpr * const sx_false;
extern struct sexpr * const sx_true;
extern struct sexpr * const sx_empty_list;
extern struct sexpr * const sx_end_of_list;
extern struct sexpr * const sx_end_of_file;
extern struct sexpr * const sx_not_a_number;
extern struct sexpr * const sx_nonexistent;
extern struct sexpr * const sx_dot;

#define nilp(sx)   (((struct sexpr *)(sx) == sx_nil)          || ((sx)->type == sxt_nil))
#define truep(sx)  (((struct sexpr *)(sx) == sx_true)         || ((sx)->type == sxt_true))
#define falsep(sx) (((struct sexpr *)(sx) == sx_false)        || ((sx)->type == sxt_false))
#define emptyp(sx) (((struct sexpr *)(sx) == sx_empty_list)   || ((sx)->type == sxt_empty_list))
#define eolp(sx)   (((struct sexpr *)(sx) == sx_end_of_list)  || ((sx)->type == sxt_end_of_list))
#define eofp(sx)   (((struct sexpr *)(sx) == sx_end_of_file)  || ((sx)->type == sxt_end_of_file))
#define nanp(sx)   (((struct sexpr *)(sx) == sx_not_a_number) || ((sx)->type == sxt_not_a_number))
#define nexp(sx)   (((struct sexpr *)(sx) == sx_nonexistent)  || ((sx)->type == sxt_nonexistent))
#define dotp(sx)   (((struct sexpr *)(sx) == sx_dot)          || ((sx)->type == sxt_dot))

#define consp(sx)    ((sx)->type == sxt_cons)
#define stringp(sx)  ((sx)->type == sxt_string)
#define symbolp(sx)  ((sx)->type == sxt_symbol)
#define integerp(sx) ((sx)->type == sxt_integer)

#define car(sx)        (((sx)->type == sxt_cons) ? (((struct sexpr_cons *)(sx))->car) : sx_nonexistent)
#define cdr(sx)        (((sx)->type == sxt_cons) ? (((struct sexpr_cons *)(sx))->cdr) : sx_nonexistent)

#define sx_integer(sx) (((sx)->type == sxt_integer) ? (((struct sexpr_integer *)(sx))->integer) : -1)
#define sx_string(sx)  (const char *)(((sx)->type == sxt_string) ? (((struct sexpr_string_or_symbol *)(sx))->character_data) : "#nonexistent")
#define sx_symbol(sx)  (const char *)(((sx)->type == sxt_symbol) ? (((struct sexpr_string_or_symbol *)(sx))->character_data) : "#nonexistent")

#endif
