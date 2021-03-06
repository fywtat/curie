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

#include "sievert/sexpr.h"

static sexpr gtp (sexpr a, sexpr b, void *aux)
{
    if (integerp (a))
    {
        if (!integerp (b))
        {
            return sx_false;
        }

        return (sx_integer (a) > sx_integer (b)) ? sx_true : sx_false;
    }
    else if (symbolp (a))
    {
        const char *av, *bv;

        if (integerp (b))
        {
            return sx_true;
        }
        else if (stringp (b))
        {
            return sx_false;
        }

        av = sx_symbol (a);
        bv = sx_symbol (b);

        return (av[0] > bv[0]) ? sx_true : sx_false;
    }
    else
    {
        const char *av, *bv;

        if (integerp (b) || symbolp (b))
        {
            return sx_true;
        }

        av = sx_string (a);
        bv = sx_string (b);

        return (av[0] > bv[0]) ? sx_true : sx_false;
    }
}

int cmain(void)
{
    struct sexpr_io *o = sx_open_o (io_open_write ("to-sexpr-sort.sx"));
    struct sexpr_io *i = sx_open_i (io_open_read  ("sexpr-sort.sx"));
    sexpr a;

    while (nexp (a = sx_read (i)));

    sx_write (o, sx_set_sort_merge(a, gtp, (void *)0));

    return 0;
}
