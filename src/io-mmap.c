/**\file
 *
 * \copyright
 * Copyright (c) 2008-2014, Kyuba Project Members
 * \copyright
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * \copyright
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * \copyright
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * \see Project Documentation: http://ef.gy/documentation/curie
 * \see Project Source Code: http://git.becquerel.org/kyuba/curie.git
*/

#include <curie/memory.h>
#include <curie/multiplex.h>
#include <sievert/io.h>

struct io_get_file_data
{
    void (*on_file_read)(void *, unsigned int, void *);
    void  *aux;
};

static void mx_read (struct io *io, void *aux)
{
}

static void mx_close (struct io *io, void *aux)
{
    struct io_get_file_data *d = (struct io_get_file_data *)aux;
    void *buffer = io->buffer;

    buffer = resize_mem (io->buffersize, buffer, io->length);

    io->buffersize = 0;
    io->buffer     = (void *)0;

    d->on_file_read (buffer, io->length, d->aux);

    free_pool_mem (aux);
}

void io_get_file_contents
    (const char *path, void (*on_file_read)(void *, unsigned int, void *),
     void *aux)
{
    struct io *io = io_open_read (path);
    struct memory_pool pool =
        MEMORY_POOL_INITIALISER (sizeof(struct io_get_file_data));
    struct io_get_file_data *d = get_pool_mem (&pool);

    d->on_file_read = on_file_read;
    d->aux          = aux;

    multiplex_add_io (io, mx_read, mx_close, (void *)d);
}
