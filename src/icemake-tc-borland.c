/*
 * This file is part of the kyuba.org Curie project.
 * See the appropriate repository at http://git.kyuba.org/ for exact file
 * modification records.
*/

/*
 * Copyright (c) 2008-2010, Kyuba Project Members
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

static sexpr prepend_includes_common (struct target *t, sexpr x)
{
    sexpr include_paths = icemake_permutate_paths (t->toolchain, str_include);
    sexpr cur = include_paths;

    if (stringp (i_destdir))
    {
        x = cons (sx_join (str_dI,
                           get_install_file (t, str_include), sx_nil), x);
    }

    while (consp (cur))
    {
        sexpr sxcar = car(cur);

        x = cons (sx_join (str_dI, sxcar, sx_nil), x);

        cur = cdr (cur);
    }
        
    return cons (sx_join (str_dIbuilds, architecture, str_sinclude), x);
}

static sexpr prepend_includes_borland (struct target *t, sexpr x)
{
    return prepend_includes_common (t, x);
}

static sexpr prepend_cflags_borland (sexpr x)
{
    return prepend_flags_from_environment (x, "CFLAGS");
}

static sexpr prepend_cxxflags_borland (sexpr x)
{
    return prepend_flags_from_environment (x, "CXXFLAGS");
}

static void build_object_borland_generic
    (const char *source, const char *target, struct target *t)
{
    t->icemake->workstack =
        sx_set_add (t->icemake->workstack,
            cons (t->toolchain->meta_toolchain.borland.bcc32,
                  cons (str_dAT,
                  cons (str_dq,
                    cons (str_dw,
                      prepend_cflags_borland (
                      prepend_includes_borland (t,
                        cons (str_do,
                          cons (make_string (target),
                            cons (str_dc,
                              cons (make_string(source), sx_end_of_list)))))))))));
}

static void build_object_borland_cpp
    (const char *source, const char *target, struct target *t)
{
    t->icemake->workstack =
        sx_set_add (t->icemake->workstack,
            cons (t->toolchain->meta_toolchain.borland.bcc32,
                  cons (str_dAT,
                  cons (str_dq,
                  cons (str_dP,
                    cons (str_dw,
                      prepend_cxxflags_borland (
                      prepend_includes_borland (t,
                        cons (str_do,
                          cons (make_string (target),
                            cons (str_dc,
                              cons (make_string(source), sx_end_of_list))))))))))));
}

static void build_object_borland
    (struct target *t, sexpr type, sexpr source, sexpr target)
{
    if (truep(equalp(type, sym_link))) return;

    if (truep(equalp(type, sym_resource)))
    {
        /* STUB */
    }
    else if (truep(equalp(type, sym_cpp)))
    {
        build_object_borland_cpp (sx_string(source), sx_string(target), t);
    }
    else
    {
        build_object_borland_generic (sx_string(source), sx_string(target), t);
    }
}

static void map_includes (struct tree_node *node, void *psx)
{
    sexpr *sx = (sexpr *)psx;
    struct target *t = node_get_value (node);

    *sx = cons (sx_join (str_dL, get_build_file (t, sx_nil), sx_nil), *sx);
}

static sexpr get_special_linker_options (struct target *t, sexpr sx)
{
    define_string (str_multiply_defined, "-multiply_defined");
    define_string (str_warning,          "warning");
 
    if (stringp (i_destdir))
    {
        switch (t->icemake->filesystem_layout)
        {
            case fs_fhs:
            case fs_fhs_binlib:
                sx = cons (sx_join (str_dL, i_destdir, str_slib), sx);
                break;
            case fs_afsl:
                sx = cons (sx_join (str_dL, i_destdir,
                           sx_join (str_slash,
                                    make_string (t->toolchain->uname_os),
                           sx_join (str_slash, make_string (uname_arch),
                           str_slib))), sx);
                break;
        }
    }

    tree_map (&(t->icemake->targets), map_includes, (void *)&sx);

    if (i_os == os_darwin)
    {
        sx = cons (str_multiply_defined, cons (str_warning, sx));
    }

    return prepend_flags_from_environment (sx, "LDFLAGS");
}

static sexpr collect_code (sexpr sx, sexpr code)
{
    while (consp (code))
    {
        sexpr txn = car (code);
        sexpr ttype = car (txn);
        sexpr objectfile = car (cdr (cdr (txn)));

        if (truep(equalp(ttype, sym_assembly)) ||
            truep(equalp(ttype, sym_preproc_assembly)) ||
            truep(equalp(ttype, sym_c)) ||
            truep(equalp(ttype, sym_cpp)) ||
            truep(equalp(ttype, sym_resource)))
        {
            sx = cons (objectfile, sx);
        }

        code = cdr (code);
    }

    return sx;
}

static sexpr collect_library_link_flags (sexpr sx, struct target *t)
{
    char buffer[BUFFERSIZE];
    sexpr cur = t->libraries;

    while (consp (cur))
    {
        sexpr libname = car (cur);

        sx = cons (sx_join (str_lib, libname, str_dot_lib), sx);
        mangle_path_borland (buffer);

        cur = cdr (cur);
    }

    return sx;
}

static void link_programme_borland_filename
    (sexpr ofile, sexpr name, sexpr code, struct target *t)
{
    sexpr sx = sx_end_of_list;

    sx = collect_library_link_flags (sx, t);

    sx = cons (str_dq,
               get_special_linker_options (t,
                    cons (str_do,
                        cons (ofile,
                              collect_code (sx, code)))));

    t->icemake->workstack = sx_set_add (t->icemake->workstack,
                      cons (t->toolchain->meta_toolchain.borland.bcc32, sx));
}

static void write_curie_sx (sexpr name, struct target *t)
{
    if (truep(equalp(name, str_curie)))
    {
        sexpr b = get_build_file (t, sx_join (str_lib, name, str_dot_sx));
        struct sexpr_io *io;

        io = sx_open_o (io_open_create (sx_string (b), 0644));

        if (t->toolchain->options & ICEMAKE_OPTION_FREESTANDING)
        {
            sx_write (io, sym_freestanding);
        }
        else
        {
            sx_write (io, sym_hosted);
        }

        multiplex_add_sexpr (io, (void *)0, (void *)0);
    }
}


static void link_test_cases
    (sexpr name, sexpr code, struct target *t,
     void (*f) (sexpr, sexpr, sexpr, struct target *))
{
    if (truep(do_tests))
    {
        sexpr s = t->test_cases;

        while (consp (s))
        {
            sexpr s1 = car(s);
            sexpr s2 = cdr(cdr(s1));
            sexpr s3 = car(s2);
            sexpr s4 = car(cdr(s2));
            sexpr s5 = cons(cons (car (s1),
                            cons (s3, cons(s3, sx_end_of_list))),
                                 sx_end_of_list);

            f (s4, name, s5, t);

            s = cdr (s);
        }
    }
}

static void link_library_borland (sexpr name, sexpr code, struct target *t)
{
    sexpr sx = sx_end_of_list, sxx = sx_end_of_list,
          b = get_build_file (t, sx_join (str_lib, name, str_dot_lib));

    write_curie_sx (name, t);

    sx = collect_code (sx, code);

    while (consp (sx))
    {
        sxx = cons (str_plus, cons (car (sx), sxx));
        sx = cdr (sx);
    }

    t->icemake->workstack = sx_set_add (t->icemake->workstack,
                    cons (t->toolchain->meta_toolchain.borland.tlib,
                          cons (b, sxx)));
}

static void link_library_borland_dynamic
    (sexpr name, sexpr code, struct target *t)
{
    sexpr sx = sx_end_of_list, cur, b;

    write_curie_sx (name, t);

    cur = t->libraries;
    while (consp (cur))
    {
        sexpr libname = car (cur);

        if (falsep(equalp(name, libname)))
        {
            sx = cons (mangle_path_borland_sx (sx_join (str_lib, libname,
                                                        str_dot_lib)), sx);
        }

        cur = cdr (cur);
    }

    sx = get_special_linker_options (t, sx);

    b = get_build_file (t, sx_join (str_lib, name,
                             sx_join (str_dot, t->dversion, str_dot_dll)));

    /* just collect_code(), because bcc doesn't use extra PIC objects */
    sx = collect_code (sx, code);

    t->icemake->workstack = sx_set_add (t->icemake->workstack,
                    cons (t->toolchain->meta_toolchain.borland.bcc32,
                          cons (str_dq, cons (str_dWD,
                                get_special_linker_options (t,
                                    cons (str_do, cons (b, sx)))))));

    b = get_build_file (t, sx_join (str_lib, name, str_dot_dll));

    t->icemake->workstack = sx_set_add (t->icemake->workstack,
                    cons (t->toolchain->meta_toolchain.borland.bcc32,
                          cons (str_dq, cons (str_dWD,
                                get_special_linker_options (t,
                                    cons (str_do, cons (b, sx)))))));
}

static int do_link (struct target *t)
{
    link_test_cases (t->name, t->code, t,
                     link_programme_borland_filename);

    if (t->options & ICEMAKE_LIBRARY)
    {
        link_library_borland (t->name, t->code, t);

        if (truep(i_dynamic_libraries) &&
            !(t->options & ICEMAKE_NO_SHARED_LIBRARY))
        {
           link_library_borland_dynamic (t->name, t->code, t);
        }
    }
    else if (t->options & ICEMAKE_PROGRAMME)
    {
        sexpr b = get_build_file
            (t, (i_os == os_windows) ? sx_join (t->name, str_dot_exe, sx_nil)
                                     : t->name);

        link_programme_borland_filename (b, t->name, t->code, t);
    }

    return 0;
}

int icemake_prepare_toolchain_borland (struct toolchain_descriptor *td)
{
    td->meta_toolchain.borland.bcc32 = icemake_which (td, "bcc32");
    td->meta_toolchain.borland.tlib  = icemake_which (td, "tlib");

    td->build_object = build_object_borland;
    td->link         = do_link;

    return (falsep (td->meta_toolchain.borland.bcc32) ? 1 : 0) + 
           (falsep (td->meta_toolchain.borland.tlib)  ? 1 : 0);
}
