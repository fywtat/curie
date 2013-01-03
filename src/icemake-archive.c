/*
 * This file is part of the becquerel.org Curie project.
 * See the appropriate repository at http://git.becquerel.org/ for exact file
 * modification records.
*/

/*
 * Copyright (c) 2008-2013, Kyuba Project Members
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
*/

#include <icemake/icemake.h>

#include <curie/multiplex.h>
#include <curie/directory.h>
#include <curie/network.h>
#include <curie/memory.h>

#include <sievert/cpio.h>
#include <sievert/filesystem.h>

struct archive_metadata
{
    struct io *out;
    sexpr name;
    sexpr source;
    unsigned int length;
    struct cpio *cpio;
};

static int open_archive_files = 0;

static void on_read_cpio_archive_source_file (struct io *io, void *aux)
{
}

static void on_close_cpio_archive_source_file (struct io *io, void *aux)
{
    struct archive_metadata *ad = (struct archive_metadata *)aux;
    if (consp (ad->source))
    {
        sexpr ca = car (ad->source), caa = car (ca), cad = cdr (ca);
        struct io *i = io_open_read (sx_string (caa));

        cpio_next_file (ad->cpio, sx_string (cad), (struct metadata *)0, i);

        multiplex_add_io
            (i, on_read_cpio_archive_source_file,
             on_close_cpio_archive_source_file, ad);

        ad->source = cdr (ad->source);
    }
    else
    {
        cpio_close (ad->cpio);
    }
}

static void on_read_archive_source_file (struct io *io, void *aux)
{
    struct archive_metadata *ad = (struct archive_metadata *)aux;
    char s[] = "0xxx,";
    int i = 0;
    const char *b = io->buffer;

    for (i = io->position; i < io->length; i++)
    {
        unsigned char t = (unsigned char)b[i];

        s[3] =  (t % 16);
        s[2] = ((t - s[3]) / 16);

        if (s[2] < 10) { s[2] = '0' + s[2]; }
        else           { s[2] = 'a' + s[2] - 10; }
        if (s[3] < 10) { s[3] = '0' + s[3]; }
        else           { s[3] = 'a' + s[3] - 10; }

        io_collect (ad->out, s, 5);
    }

    io_commit (ad->out);

    ad->length   += io->length - io->position;

    io->position  = io->length;
}

static void on_close_archive_source_file (struct io *io, void *aux)
{
    struct archive_metadata *ad = (struct archive_metadata *)aux;
    const char *s = sx_string (ad->name);
    char x[8] = "00000000";
    int j = 0;
    signed int i;
    char c;

    io_write (ad->out, "0x00};\nconst unsigned long ", 27);
    for (j = 0; s[j] != (char)0; j++);
    io_write (ad->out, s, j);
    io_write (ad->out, "_length=0x", 10);

    for (i = 7; (i >= 0) && (ad->length >= 0); i--)
    {
        c = ad->length % 16;
        if (c < 10)
        {
            x[i] = '0' + c;
        }
        else
        {
            x[i] = 'a' + (c - 10);
        }

        ad->length = (ad->length - c) / 16;
    }

    io_write (ad->out, x, 8);
    io_write (ad->out, ";\n", 2);
    io_close (ad->out);

    open_archive_files--;
    free_pool_mem (aux);
}

static void target_map_prepare_archives (struct tree_node *node, void *u)
{
    struct target *context = (struct target *)node_get_value(node);
    sexpr c = context->code, d, da, dd, name, source, out, df, ca, caa, cad, q;
    struct memory_pool pool =
        MEMORY_POOL_INITIALISER(sizeof(struct archive_metadata));
    struct archive_metadata *ad;
    struct io *o, *i, *co, *ci;
    const char *s;
    int j = 0;
    struct io *header;

    sexpr outx = path_normalise
        (get_path_d (context, context->toolchain->toolchain_sym, sym_build,
                     sym_C_Header, str_data, sx_nonexistent));
    while (consp(outx))
    {
        out = car(outx);
        outx = cdr(outx);

        header = io_open_write (sx_string(out));
        io_collect (header, "/* this file was generated by icemake */\n\n",
                    42);

        context->headers = cons (cons(str_data, cons (out, sx_end_of_list)),
                                 context->headers);

        for (c; consp (c); c = cdr (c))
        {
            d  = car (c);
            da = car (d);

            if (truep (equalp (da, sym_raw_c)) ||
                truep (equalp (da, sym_cpio_c)))
            {
                dd     = cdr (d);
                name   = car (dd);
                df     = sx_join (str_datas, name, sx_nil);

                s = sx_string (name);
                for (j = 0; s[j] != (char)0; j++);

                io_collect (header, "extern const unsigned char ", 27);
                io_collect (header, s, j);
                io_collect (header, "[];\n", 4);

                io_collect (header, "extern const unsigned long int ", 31);
                io_collect (header, s, j);
                io_collect (header, "_length;\n", 9);

                if (truep (equalp (da, sym_raw_c)))
                {
                    source = sx_join (context->base, car(cdr (dd)), sx_nil);

                    out = get_path (context, context->toolchain->toolchain_sym,
                                    sym_c, df, sx_nonexistent);
                    out = get_path (context, sym_build, sym_c, out,
                                    sx_nonexistent);
                    out = car (out);

                  open_archive_files++;

                    i = io_open_read  (sx_string (source));
                    o = io_open_write (sx_string (out));

                    io_collect (o,      "const unsigned char ",        20);

                    io_collect (o,      s, j);
                    io_collect (o,      "[]={", 4);

                    ad = (struct archive_metadata *)get_pool_mem (&pool);
                    ad->out    = o;
                    ad->name   = name;
                    ad->source = sx_end_of_list;
                    ad->length = 0;
                    ad->cpio   = (struct cpio *)0;

                    multiplex_add_io
                        (i, on_read_archive_source_file,
                         on_close_archive_source_file, ad);

                    context->code = sx_set_remove (context->code, d);

                    q = sx_alist_get (context->code, df);
                    context->code = sx_alist_remove (context->code, df);

                    if (nexp (q))
                    {
                        q = sx_end_of_list;
                    }

                    q = sx_alist_add (q, sym_c,
                                      sx_list2 (out, context->name));
                    q = sx_alist_add (q, sym_c_pic,
                                      sx_list2 (out, context->name));
                    context->code = sx_alist_add (context->code, df, q);
                }
                else
                {
                    dd     = cdr (dd);
                    source = sx_end_of_list;

                    while (consp (dd))
                    {
                        source = sx_set_merge
                            (source, read_directory_sx
                                       (sx_join (context->base, car (dd),
                                                 sx_nil)));

                        dd = cdr (dd);
                    }

                    source = path_normalise_prefix (source);

                    context->code = sx_set_remove (context->code, d);

                    if (consp (source))
                    {
                        out = get_path (context,
                                        context->toolchain->toolchain_sym,
                                        sym_c, df, sx_nonexistent);
                        out = get_path (context, sym_build, sym_c, out,
                                        sx_nonexistent);
                        out = car (out);
                        open_archive_files++;

                        o  = io_open_write (sx_string (out));

                        io_collect (o,      "const unsigned char ", 20);

                        io_collect (o,      s, j);
                        io_collect (o,      "[]={", 4);

                        net_open_loop (&ci, &co);

                        ad = (struct archive_metadata *)get_pool_mem (&pool);
                        ad->out    = o;
                        ad->name   = name;
                        ad->source = cdr(source);
                        ad->length = 0;
                        ad->cpio   = cpio_create_archive (co);

                        ca  = car (source);
                        caa = car (ca);
                        cad = cdr (ca);

                        i = io_open_read (sx_string (caa));

                        cpio_next_file
                            (ad->cpio, sx_string (cad), (struct metadata *)0,
                             i);

                        multiplex_add_io
                            (i, on_read_cpio_archive_source_file,
                             on_close_cpio_archive_source_file, ad);

                        multiplex_add_io
                            (ci, on_read_archive_source_file,
                             on_close_archive_source_file, ad);

                        q = sx_alist_get (context->code, df);
                        context->code = sx_alist_remove (context->code, df);

                        if (nexp (q))
                        {
                            q = sx_end_of_list;
                        }

                        q = sx_alist_add (q, sym_c,
                                          sx_list2 (out, context->name));
                        q = sx_alist_add (q, sym_c_pic,
                                          sx_list2 (out, context->name));
                        context->code = sx_alist_add (context->code, df, q);
                    }

                    context->code = sx_set_remove (context->code, d);
                }
            }
        }
    }

    io_close (header);
}

void icemake_prepare_archives (struct icemake *im)
{
    multiplex_cpio();

    tree_map (&(im->targets), target_map_prepare_archives, (void *)im);

    while (open_archive_files > 0)
    {
        multiplex();
    }
}

