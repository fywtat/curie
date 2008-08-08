/*
 *  multiplex-io.c
 *  atomic-libc
 *
 *  Created by Magnus Deininger on 07/08/2008.
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

#include <atomic/multiplex.h>
#include <atomic/memory.h>

static void mx_f_count(int *r, int *w);
static void mx_f_augment(int *rs, int *r, int *ws, int *w);
static void mx_f_callback(int *rs, int r, int *ws, int w);

static struct multiplex_functions mx_functions = {
    .count = mx_f_count,
    .augment = mx_f_augment,
    .callback = mx_f_callback
};

struct io_list {
    struct io *io;
    void (*on_read)(struct io *);
    struct io_list *next;
};

static struct memory_pool list_pool = MEMORY_POOL_INITIALISER(sizeof (struct io_list));
static struct io_list *list = (struct io_list *)0;

static void mx_f_count(int *r, int *w) {
    struct io_list *l = list;

    while (l != (struct io_list *)0) {
        if ((l->io->fd != -1) &&
             (l->io->status != io_end_of_file) &&
             (l->io->status != io_unrecoverable_error))
        {
            switch (l->io->type) {
                case iot_read:
                    (*r) += 1;
                    break;
                case iot_write:
                    if (l->io->length == 0) goto next;

                    (*w) += 1;
                    break;
                default:
                    break;
            }
        }

        next:
        l = l->next;
    }
}

static void mx_f_augment(int *rs, int *r, int *ws, int *w) {
    struct io_list *l = list;

    while (l != (struct io_list *)0) {
        if ((l->io->fd != -1) &&
            (l->io->status != io_end_of_file) &&
            (l->io->status != io_unrecoverable_error))
        {
            int i, t, fd = l->io->fd;

            switch (l->io->type) {
                case iot_read:
                    t = *r;
                    for (i = 0; i < t; i++) {
                        if (rs[i] == fd) {
                            goto next;
                        }
                    }
                    rs[i] = fd;
                    (*r) += 1;
                    break;
                case iot_write:
                    if (l->io->length == 0) goto next;

                    t = *w;
                    for (i = 0; i < t; i++) {
                        if (ws[i] == fd) {
                            goto next;
                        }
                    }
                    ws[i] = fd;
                    (*w) += 1;
                    break;
                default:
                    break;
            }
        }

        next:
        l = l->next;
    }
}

static void mx_f_callback(int *rs, int r, int *ws, int w) {
    struct io_list *l = list;

    while (l != (struct io_list *)0) {
        if ((l->io->fd != -1) &&
            (l->io->status != io_end_of_file) &&
            (l->io->status != io_unrecoverable_error))
        {
            int i, fd = l->io->fd;

            switch (l->io->type) {
                case iot_read:
                    for (i = 0; i < r; i++) {
                        if (rs[i] == fd) {
                            l->on_read (l->io);
                        }
                    }
                    break;
                case iot_write:
                    if (l->io->length == 0) goto next;

                    for (i = 0; i < w; i++) {
                        if (ws[i] == fd) {
                            io_commit (l->io);
                        }
                    }
                    break;
                default:
                    break;
            }
        }

        next:
        l = l->next;
    }
}

void multiplex_io () {
    static char installed = (char)0;

    if (installed == (char)0) {
        multiplex_add (&mx_functions);
        installed = (char)1;
    }
}

void multiplex_add_io (struct io *io, void (*on_read)(struct io *)) {
    struct io_list *list_element = get_pool_mem (&list_pool);
    list_element->next = list;
    list = list_element;

    list_element->io = io;
    list_element->on_read = on_read;
}