/*
 *  network-loop.c
 *  atomic-libc
 *
 *  Created by Magnus Deininger on 06/08/2008.
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

#include "atomic/network.h"
#include "atomic/sexpr.h"

#define TEST_STRING_1 "hello-world!"
#define TEST_STRING_2 "hello \"world\"!"

int a_main(void) {
    struct io *in, *out;
    struct sexpr_io *io, *stdio;
    struct sexpr *i, *t1, *t2;
    int rv = 0;

    net_open_loop(&in, &out);

    io = sx_open_io(in, out);
    stdio = sx_open_stdio();

    t1 = make_symbol(TEST_STRING_1);
    t2 = make_string(TEST_STRING_2);

    sx_write (io, t1);
    sx_write (io, t2);
    while (io_commit (io->out) != io_complete);

    /* test transmission of a single symbol*/
    do {
        i = sx_read (io);
    } while (i == sx_nonexistent);

    rv |= ((t1 == i) ? 0 : 1) << 1;

    sx_destroy (i);

    /* test transmission of a single string */

    while (io_commit (io->out) == io_incomplete);
    do {
        i = sx_read (io);
    } while (i == sx_nonexistent);

    rv |= ((i == t2) ? 0 : 1) << 2;

    sx_destroy (i);

    i = sx_read (io);
    rv |= ((i == sx_nonexistent) ? 0 : 1) << 3;

    if (rv != 0) {
        sx_write (stdio, (i = make_symbol("failure")));
    } else {
        sx_write (stdio, (i = make_symbol("success")));
    }

    sx_destroy (i);
    sx_destroy (t1);
    sx_destroy (t2);

    sx_close_io (io);
    sx_close_io (stdio);

    return rv;
}
