/*
  pygame - Python Game Library
  Copyright (C) 2000-2001  Pete Shinners
  Copyright (C) 2007  Rene Dudfield, Richard Goedeken

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public
  License along with this library; if not, write to the Free
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

  Pete Shinners
  pete@shinners.org
*/

/*
 *  surface transformations for pygame
 */
#include "pygame.h"
#include "pgcompat.h"
#include "doc/transform_doc.h"
#include <math.h>
#include <string.h>
#include "scale.h"


typedef void (* SMOOTHSCALE_FILTER_P)(Uint8 *, Uint8 *, int, int, int, int, int);
struct _module_state {
    const char *filter_type;
    SMOOTHSCALE_FILTER_P filter_shrink_X;
    SMOOTHSCALE_FILTER_P filter_shrink_Y;
    SMOOTHSCALE_FILTER_P filter_expand_X;
    SMOOTHSCALE_FILTER_P filter_expand_Y;
};

#if defined(SCALE_MMX_SUPPORT)

#include <SDL_cpuinfo.h>

#if PY3
#define GETSTATE(m) PY3_GETSTATE (_module_state, m)
#else
static struct _module_state _state = {0, 0, 0, 0, 0};
#define GETSTATE(m) PY2_GETSTATE (_state)
#endif

#else /* if defined(SCALE_MMX_SUPPORT) */

static void filter_shrink_X_ONLYC(Uint8 *, Uint8 *, int, int, int, int, int);
static void filter_shrink_Y_ONLYC(Uint8 *, Uint8 *, int, int, int, int, int);
static void filter_expand_X_ONLYC(Uint8 *, Uint8 *, int, int, int, int, int);
static void filter_expand_Y_ONLYC(Uint8 *, Uint8 *, int, int, int, int, int);

static struct _module_state _state = {
    "GENERIC",
    filter_shrink_X_ONLYC,
    filter_shrink_Y_ONLYC,
    filter_expand_X_ONLYC,
    filter_expand_Y_ONLYC};
#define GETSTATE(m) PY2_GETSTATE (_state)
#define smoothscale_init(st)

#endif /* if defined(SCALE_MMX_SUPPORT) */

void scale2x (SDL_Surface *src, SDL_Surface *dst);
extern SDL_Surface* rotozoomSurface (SDL_Surface *src, double angle,
                                     double zoom, int smooth);

static SDL_Surface*
newsurf_fromsurf (SDL_Surface* surf, int width, int height)
{
    SDL_Surface* newsurf;
    int result;

    if (surf->format->BytesPerPixel <= 0 || surf->format->BytesPerPixel > 4)
        return (SDL_Surface*)
            (RAISE (PyExc_ValueError,
                    "unsupport Surface bit depth for transform"));

    newsurf = SDL_CreateRGBSurface (surf->flags, width, height,
                                    surf->format->BitsPerPixel,
                                    surf->format->Rmask, surf->format->Gmask,
                                    surf->format->Bmask, surf->format->Amask);
    if (!newsurf)
        return (SDL_Surface*) (RAISE (PyExc_SDLError, SDL_GetError ()));

    /* Copy palette, colorkey, etc info */
    if (surf->format->BytesPerPixel==1 && surf->format->palette)
        SDL_SetColors (newsurf, surf->format->palette->colors, 0,
                       surf->format->palette->ncolors);
    if (surf->flags & SDL_SRCCOLORKEY)
        SDL_SetColorKey (newsurf, (surf->flags&SDL_RLEACCEL) | SDL_SRCCOLORKEY,
                         surf->format->colorkey);

    if (surf->flags&SDL_SRCALPHA)
    {
        result = SDL_SetAlpha (newsurf, surf->flags, surf->format->alpha);
        if (result == -1)
            return (SDL_Surface*) (RAISE (PyExc_SDLError, SDL_GetError ()));
    }
    return newsurf;
}

static SDL_Surface*
rotate90 (SDL_Surface *src, int angle)
{
    int numturns = (angle / 90) % 4;
    int dstwidth, dstheight;
    SDL_Surface* dst;
    char *srcpix, *dstpix, *srcrow, *dstrow;
    int srcstepx, srcstepy, dststepx, dststepy;
    int loopx, loopy;

    if (numturns < 0)
        numturns = 4 + numturns;
    if (!(numturns % 2))
    {
        dstwidth = src->w;
        dstheight = src->h;
    }
    else
    {
        dstwidth = src->h;
        dstheight = src->w;
    }

    dst = newsurf_fromsurf (src, dstwidth, dstheight);
    if (!dst)
        return NULL;
    SDL_LockSurface (dst);
    srcrow = (char*) src->pixels;
    dstrow = (char*) dst->pixels;
    srcstepx = dststepx = src->format->BytesPerPixel;
    srcstepy = src->pitch;
    dststepy = dst->pitch;

    switch (numturns)
    {
        /*case 0: we don't need to change anything*/
    case 1:
        srcrow += ((src->w - 1) * srcstepx);
        srcstepy = -srcstepx;
        srcstepx = src->pitch;
        break;
    case 2:
        srcrow += ((src->h - 1) * srcstepy) + ((src->w - 1) * srcstepx);
        srcstepx = -srcstepx;
        srcstepy = -srcstepy;
        break;
    case 3:
        srcrow += ((src->h - 1) * srcstepy);
        srcstepx = -srcstepy;
        srcstepy = src->format->BytesPerPixel;
        break;
    }

    switch (src->format->BytesPerPixel)
    {
    case 1:
        for (loopy = 0; loopy < dstheight; ++loopy)
        {
            dstpix = dstrow;
            srcpix = srcrow;
            for (loopx = 0; loopx < dstwidth; ++loopx)
            {
                *dstpix = *srcpix;
                srcpix += srcstepx;
                dstpix += dststepx;
            }
            dstrow += dststepy;
            srcrow += srcstepy;
        }
        break;
    case 2:
        for (loopy = 0; loopy < dstheight; ++loopy)
        {
            dstpix = dstrow;
            srcpix = srcrow;
            for (loopx = 0; loopx < dstwidth; ++loopx)
            {
                *(Uint16*)dstpix = *(Uint16*)srcpix;
                srcpix += srcstepx;
                dstpix += dststepx;
            }
            dstrow += dststepy;
            srcrow += srcstepy;
        }
        break;
    case 3:
        for (loopy = 0; loopy < dstheight; ++loopy)
        {
            dstpix = dstrow;
            srcpix = srcrow;
            for (loopx = 0; loopx < dstwidth; ++loopx)
            {
                dstpix[0] = srcpix[0];
                dstpix[1] = srcpix[1];
                dstpix[2] = srcpix[2];
                srcpix += srcstepx;
                dstpix += dststepx;
            }
            dstrow += dststepy;
            srcrow += srcstepy;
        }
        break;
    case 4:
        for (loopy = 0; loopy < dstheight; ++loopy)
        {
            dstpix = dstrow;
            srcpix = srcrow;
            for (loopx = 0; loopx < dstwidth; ++loopx)
            {
                *(Uint32*)dstpix = *(Uint32*)srcpix;
                srcpix += srcstepx;
                dstpix += dststepx;
            }
            dstrow += dststepy;
            srcrow += srcstepy;
        }
        break;
    }
    SDL_UnlockSurface (dst);
    return dst;
}


static void
rotate (SDL_Surface *src, SDL_Surface *dst, Uint32 bgcolor, double sangle,
        double cangle)
{
    int x, y, dx, dy;

    Uint8 *srcpix = (Uint8*) src->pixels;
    Uint8 *dstrow = (Uint8*) dst->pixels;
    int srcpitch = src->pitch;
    int dstpitch = dst->pitch;

    int cy = dst->h / 2;
    int xd = ((src->w - dst->w) << 15);
    int yd = ((src->h - dst->h) << 15);

    int isin = (int)(sangle * 65536);
    int icos = (int)(cangle * 65536);

    int ax = ((dst->w) << 15) - (int)(cangle * ((dst->w - 1) << 15));
    int ay = ((dst->h) << 15) - (int)(sangle * ((dst->w - 1) << 15));

    int xmaxval = ((src->w) << 16) - 1;
    int ymaxval = ((src->h) << 16) - 1;

    switch (src->format->BytesPerPixel)
    {
    case 1:
        for (y = 0; y < dst->h; y++)
        {
            Uint8 *dstpos = (Uint8*)dstrow;
            dx = (ax + (isin * (cy - y))) + xd;
            dy = (ay - (icos * (cy - y))) + yd;
            for (x = 0; x < dst->w; x++)
            {
                if(dx < 0 || dy < 0 || dx > xmaxval || dy > ymaxval)
                    *dstpos++ = bgcolor;
                else
                    *dstpos++ = *(Uint8*)
                        (srcpix + ((dy >> 16) * srcpitch) + (dx >> 16));
                dx += icos;
                dy += isin;
            }
            dstrow += dstpitch;
        }
        break;
    case 2:
        for (y = 0; y < dst->h; y++)
        {
            Uint16 *dstpos = (Uint16*)dstrow;
            dx = (ax + (isin * (cy - y))) + xd;
            dy = (ay - (icos * (cy - y))) + yd;
            for (x = 0; x < dst->w; x++)
            {
                if (dx < 0 || dy < 0 || dx > xmaxval || dy > ymaxval)
                    *dstpos++ = bgcolor;
                else
                    *dstpos++ = *(Uint16*)
                        (srcpix + ((dy >> 16) * srcpitch) + (dx >> 16 << 1));
                dx += icos;
                dy += isin;
            }
            dstrow += dstpitch;
        }
        break;
    case 4:
        for (y = 0; y < dst->h; y++)
        {
            Uint32 *dstpos = (Uint32*)dstrow;
            dx = (ax + (isin * (cy - y))) + xd;
            dy = (ay - (icos * (cy - y))) + yd;
            for (x = 0; x < dst->w; x++)
            {
                if (dx < 0 || dy < 0 || dx > xmaxval || dy > ymaxval)
                    *dstpos++ = bgcolor;
                else
                    *dstpos++ = *(Uint32*)
                        (srcpix + ((dy >> 16) * srcpitch) + (dx >> 16 << 2));
                dx += icos;
                dy += isin;
            }
            dstrow += dstpitch;
        }
        break;
    default: /*case 3:*/
        for (y = 0; y < dst->h; y++)
        {
            Uint8 *dstpos = (Uint8*)dstrow;
            dx = (ax + (isin * (cy - y))) + xd;
            dy = (ay - (icos * (cy - y))) + yd;
            for (x = 0; x < dst->w; x++)
            {
                if (dx < 0 || dy < 0 || dx > xmaxval || dy > ymaxval)
                {
                    dstpos[0] = ((Uint8*) &bgcolor)[0];
                    dstpos[1] = ((Uint8*) &bgcolor)[1];
                    dstpos[2] = ((Uint8*) &bgcolor)[2];
                    dstpos += 3;
                }
                else
                {
                    Uint8* srcpos = (Uint8*)
                        (srcpix + ((dy >> 16) * srcpitch) + ((dx >> 16) * 3));
                    dstpos[0] = srcpos[0];
                    dstpos[1] = srcpos[1];
                    dstpos[2] = srcpos[2];
                    dstpos += 3;
                }
                dx += icos; dy += isin;
            }
            dstrow += dstpitch;
        }
        break;
    }
}

static void
stretch (SDL_Surface *src, SDL_Surface *dst)
{
    int looph, loopw;

    Uint8* srcrow = (Uint8*) src->pixels;
    Uint8* dstrow = (Uint8*) dst->pixels;

    int srcpitch = src->pitch;
    int dstpitch = dst->pitch;

    int dstwidth = dst->w;
    int dstheight = dst->h;
    int dstwidth2 = dst->w << 1;
    int dstheight2 = dst->h << 1;

    int srcwidth2 = src->w << 1;
    int srcheight2 = src->h << 1;

    int w_err, h_err = srcheight2 - dstheight2;


    switch (src->format->BytesPerPixel)
    {
    case 1:
        for (looph = 0; looph < dstheight; ++looph)
        {
            Uint8 *srcpix = (Uint8*)srcrow, *dstpix = (Uint8*)dstrow;
            w_err = srcwidth2 - dstwidth2;
            for (loopw = 0; loopw < dstwidth; ++ loopw)
            {
                *dstpix++ = *srcpix;
                while (w_err >= 0)
                {
                    ++srcpix;
                    w_err -= dstwidth2;
                }
                w_err += srcwidth2;
            }
            while (h_err >= 0)
            {
                srcrow += srcpitch;
                h_err -= dstheight2;
            }
            dstrow += dstpitch;
            h_err += srcheight2;
        }
        break;
    case 2:
        for (looph = 0; looph < dstheight; ++looph)
        {
            Uint16 *srcpix = (Uint16*)srcrow, *dstpix = (Uint16*)dstrow;
            w_err = srcwidth2 - dstwidth2;
            for (loopw = 0; loopw < dstwidth; ++ loopw)
            {
                *dstpix++ = *srcpix;
                while (w_err >= 0)
                {
                    ++srcpix;
                    w_err -= dstwidth2;
                }
                w_err += srcwidth2;
            }
            while (h_err >= 0)
            {
                srcrow += srcpitch;
                h_err -= dstheight2;
            }
            dstrow += dstpitch;
            h_err += srcheight2;
        }
        break;
    case 3:
        for (looph = 0; looph < dstheight; ++looph)
        {
            Uint8 *srcpix = (Uint8*)srcrow, *dstpix = (Uint8*)dstrow;
            w_err = srcwidth2 - dstwidth2;
            for (loopw = 0; loopw < dstwidth; ++ loopw)
            {
                dstpix[0] = srcpix[0];
                dstpix[1] = srcpix[1];
                dstpix[2] = srcpix[2];
                dstpix += 3;
                while (w_err >= 0)
                {
                    srcpix+=3;
                    w_err -= dstwidth2;
                }
                w_err += srcwidth2;
            }
            while (h_err >= 0)
            {
                srcrow += srcpitch;
                h_err -= dstheight2;
            }
            dstrow += dstpitch;
            h_err += srcheight2;
        }
        break;
    default: /*case 4:*/
        for (looph = 0; looph < dstheight; ++looph)
        {
            Uint32 *srcpix = (Uint32*)srcrow, *dstpix = (Uint32*)dstrow;
            w_err = srcwidth2 - dstwidth2;
            for (loopw = 0; loopw < dstwidth; ++ loopw)
            {
                *dstpix++ = *srcpix;
                while (w_err >= 0)
                {
                    ++srcpix;
                    w_err -= dstwidth2;
                }
                w_err += srcwidth2;
            }
            while (h_err >= 0)
            {
                srcrow += srcpitch;
                h_err -= dstheight2;
            }
            dstrow += dstpitch;
            h_err += srcheight2;
        }
        break;
    }
}

static PyObject*
surf_scale (PyObject* self, PyObject* arg)
{
    PyObject *surfobj, *surfobj2;
    SDL_Surface* surf, *newsurf;
    int width, height;
    surfobj2 = NULL;

    /*get all the arguments*/
    if (!PyArg_ParseTuple (arg, "O!(ii)|O!", &PySurface_Type, &surfobj,
                           &width, &height, &PySurface_Type, &surfobj2))
        return NULL;

    if (width < 0 || height < 0)
        return RAISE (PyExc_ValueError, "Cannot scale to negative size");

    surf = PySurface_AsSurface (surfobj);

    if (!surfobj2)
    {
        newsurf = newsurf_fromsurf (surf, width, height);
        if (!newsurf)
            return NULL;
    }
    else
        newsurf = PySurface_AsSurface (surfobj2);

    /* check to see if the size is twice as big. */
    if (newsurf->w != width || newsurf->h != height)
        return RAISE (PyExc_ValueError,
                      "Destination surface not the given width or height.");

    /* check to see if the format of the surface is the same. */
    if (surf->format->BytesPerPixel != newsurf->format->BytesPerPixel)
        return RAISE (PyExc_ValueError,
                      "Source and destination surfaces need the same format.");

    if (width && height)
    {
        SDL_LockSurface (newsurf);
        PySurface_Lock (surfobj);

        Py_BEGIN_ALLOW_THREADS;
        stretch (surf, newsurf);
        Py_END_ALLOW_THREADS;

        PySurface_Unlock (surfobj);
        SDL_UnlockSurface (newsurf);
    }

    if (surfobj2)
    {
        Py_INCREF (surfobj2);
        return surfobj2;
    }
    else
        return PySurface_New (newsurf);
}

static PyObject*
surf_scale2x (PyObject* self, PyObject* arg)
{
    PyObject *surfobj, *surfobj2;
    SDL_Surface *surf;
    SDL_Surface *newsurf;
    int width, height;
    surfobj2 = NULL;

    /*get all the arguments*/
    if (!PyArg_ParseTuple (arg, "O!|O!", &PySurface_Type, &surfobj,
                           &PySurface_Type, &surfobj2))
        return NULL;

    surf = PySurface_AsSurface (surfobj);

    /* if the second surface is not there, then make a new one. */

    if (!surfobj2)
    {
        width = surf->w * 2;
        height = surf->h * 2;

        newsurf = newsurf_fromsurf (surf, width, height);

        if (!newsurf)
            return NULL;
    }
    else
        newsurf = PySurface_AsSurface (surfobj2);

    /* check to see if the size is twice as big. */
    if (newsurf->w != (surf->w * 2) || newsurf->h != (surf->h * 2))
        return RAISE (PyExc_ValueError, "Destination surface not 2x bigger.");

    /* check to see if the format of the surface is the same. */
    if (surf->format->BytesPerPixel != newsurf->format->BytesPerPixel)
        return RAISE (PyExc_ValueError,
                      "Source and destination surfaces need the same format.");

    SDL_LockSurface (newsurf);
    SDL_LockSurface (surf);

    Py_BEGIN_ALLOW_THREADS;
    scale2x (surf, newsurf);
    Py_END_ALLOW_THREADS;

    SDL_UnlockSurface (surf);
    SDL_UnlockSurface (newsurf);

    if (surfobj2)
    {
        Py_INCREF (surfobj2);
        return surfobj2;
    }
    else
        return PySurface_New (newsurf);
}

static PyObject*
surf_rotate (PyObject* self, PyObject* arg)
{
    PyObject *surfobj;
    SDL_Surface* surf, *newsurf;
    float angle;

    double radangle, sangle, cangle;
    double x, y, cx, cy, sx, sy;
    int nxmax,nymax;
    Uint32 bgcolor;

    /*get all the arguments*/
    if (!PyArg_ParseTuple (arg, "O!f", &PySurface_Type, &surfobj, &angle))
        return NULL;
    surf = PySurface_AsSurface (surfobj);

    if (surf->format->BytesPerPixel <= 0 || surf->format->BytesPerPixel > 4)
        return RAISE (PyExc_ValueError,
                      "unsupport Surface bit depth for transform");



    if ( !( fmod((double)angle, (double)90.0f) ) ) {
        PySurface_Lock (surfobj);

        Py_BEGIN_ALLOW_THREADS;
        newsurf = rotate90 (surf, (int) angle);
        Py_END_ALLOW_THREADS;

        PySurface_Unlock (surfobj);
        if (!newsurf)
            return NULL;
        return PySurface_New (newsurf);
    }

    radangle = angle*.01745329251994329;
    sangle = sin (radangle);
    cangle = cos (radangle);

    x = surf->w;
    y = surf->h;
    cx = cangle*x;
    cy = cangle*y;
    sx = sangle*x;
    sy = sangle*y;
    nxmax = (int) (MAX (MAX (MAX (fabs (cx + sy), fabs (cx - sy)),
                             fabs (-cx + sy)), fabs (-cx - sy)));
    nymax = (int) (MAX (MAX (MAX (fabs (sx + cy), fabs (sx - cy)),
                             fabs (-sx + cy)), fabs (-sx - cy)));

    newsurf = newsurf_fromsurf (surf, nxmax, nymax);
    if (!newsurf)
        return NULL;

    /* get the background color */
    if (surf->flags & SDL_SRCCOLORKEY)
        bgcolor = surf->format->colorkey;
    else
    {
        SDL_LockSurface (surf);
        switch (surf->format->BytesPerPixel)
        {
        case 1:
            bgcolor = *(Uint8*) surf->pixels;
            break;
        case 2:
            bgcolor = *(Uint16*) surf->pixels;
            break;
        case 4:
            bgcolor = *(Uint32*) surf->pixels;
            break;
        default: /*case 3:*/
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
            bgcolor = (((Uint8*) surf->pixels)[0]) +
                (((Uint8*) surf->pixels)[1] << 8) +
                (((Uint8*) surf->pixels)[2] << 16);
#else
            bgcolor = (((Uint8*) surf->pixels)[2]) +
                (((Uint8*) surf->pixels)[1] << 8) +
                (((Uint8*) surf->pixels)[0] << 16);
#endif
        }
        SDL_UnlockSurface (surf);
        bgcolor &= ~surf->format->Amask;
    }

    SDL_LockSurface (newsurf);
    PySurface_Lock (surfobj);

    Py_BEGIN_ALLOW_THREADS;
    rotate (surf, newsurf, bgcolor, sangle, cangle);
    Py_END_ALLOW_THREADS;

    PySurface_Unlock (surfobj);
    SDL_UnlockSurface (newsurf);

    return PySurface_New (newsurf);
}

static PyObject*
surf_flip (PyObject* self, PyObject* arg)
{
    PyObject *surfobj;
    SDL_Surface* surf, *newsurf;
    int xaxis, yaxis;
    int loopx, loopy;
    int pixsize, srcpitch, dstpitch;
    Uint8 *srcpix, *dstpix;

    /*get all the arguments*/
    if (!PyArg_ParseTuple (arg, "O!ii", &PySurface_Type, &surfobj,
                           &xaxis, &yaxis))
        return NULL;
    surf = PySurface_AsSurface (surfobj);

    newsurf = newsurf_fromsurf (surf, surf->w, surf->h);
    if (!newsurf)
        return NULL;

    pixsize = surf->format->BytesPerPixel;
    srcpitch = surf->pitch;
    dstpitch = newsurf->pitch;

    SDL_LockSurface (newsurf);
    PySurface_Lock (surfobj);

    srcpix = (Uint8*) surf->pixels;
    dstpix = (Uint8*) newsurf->pixels;


    Py_BEGIN_ALLOW_THREADS;

    if (!xaxis)
    {
        if (!yaxis)
        {
            for (loopy = 0; loopy < surf->h; ++loopy)
                memcpy (dstpix + loopy * dstpitch, srcpix + loopy * srcpitch,
                        surf->w * surf->format->BytesPerPixel);
        }
        else
        {
            for (loopy = 0; loopy < surf->h; ++loopy)
                memcpy (dstpix + loopy * dstpitch,
                        srcpix + (surf->h - 1 - loopy) * srcpitch,
                        surf->w * surf->format->BytesPerPixel);
        }
    }
    else /*if (xaxis)*/
    {
        if (yaxis)
        {
            switch (surf->format->BytesPerPixel)
            {
            case 1:
                for (loopy = 0; loopy < surf->h; ++loopy)
                {
                    Uint8* dst = (Uint8*) (dstpix + loopy * dstpitch);
                    Uint8* src = ((Uint8*) (srcpix + (surf->h - 1 - loopy)
                                            * srcpitch)) + surf->w - 1;
                    for (loopx = 0; loopx < surf->w; ++loopx)
                        *dst++ = *src--;
                }
                break;
            case 2:
                for (loopy = 0; loopy < surf->h; ++loopy)
                {
                    Uint16* dst = (Uint16*) (dstpix + loopy * dstpitch);
                    Uint16* src = ((Uint16*)
                                   (srcpix + (surf->h - 1 - loopy)
                                    * srcpitch)) + surf->w - 1;
                    for (loopx = 0; loopx < surf->w; ++loopx)
                        *dst++ = *src--;
                }
                break;
            case 4:
                for (loopy = 0; loopy < surf->h; ++loopy)
                {
                    Uint32* dst = (Uint32*) (dstpix + loopy * dstpitch);
                    Uint32* src = ((Uint32*)
                                   (srcpix + (surf->h - 1 - loopy)
                                    * srcpitch)) + surf->w - 1;
                    for (loopx = 0; loopx < surf->w; ++loopx)
                        *dst++ = *src--;
                }
                break;
            case 3:
                for (loopy = 0; loopy < surf->h; ++loopy)
                {
                    Uint8* dst = (Uint8*) (dstpix + loopy * dstpitch);
                    Uint8* src = ((Uint8*) (srcpix + (surf->h - 1 - loopy)
                                            * srcpitch)) + surf->w * 3 - 3;
                    for (loopx = 0; loopx < surf->w; ++loopx)
                    {
                        dst[0] = src[0];
                        dst[1] = src[1];
                        dst[2] = src[2];
                        dst += 3;
                        src -= 3;
                    }
                }
                break;
            }
        }
        else
        {
            switch (surf->format->BytesPerPixel)
            {
            case 1:
                for (loopy = 0; loopy < surf->h; ++loopy)
                {
                    Uint8* dst = (Uint8*) (dstpix + loopy * dstpitch);
                    Uint8* src = ((Uint8*) (srcpix + loopy * srcpitch)) +
                        surf->w - 1;
                    for (loopx = 0; loopx < surf->w; ++loopx)
                        *dst++ = *src--;
                }
                break;
            case 2:
                for (loopy = 0; loopy < surf->h; ++loopy)
                {
                    Uint16* dst = (Uint16*) (dstpix + loopy * dstpitch);
                    Uint16* src = ((Uint16*) (srcpix + loopy * srcpitch))
                        + surf->w - 1;
                    for (loopx = 0; loopx < surf->w; ++loopx)
                        *dst++ = *src--;
                }
                break;
            case 4:
                for (loopy = 0; loopy < surf->h; ++loopy)
                {
                    Uint32* dst = (Uint32*) (dstpix + loopy * dstpitch);
                    Uint32* src = ((Uint32*) (srcpix + loopy * srcpitch))
                        + surf->w - 1;
                    for (loopx = 0; loopx < surf->w; ++loopx)
                        *dst++ = *src--;
                }
                break;
            case 3:
                for (loopy = 0; loopy < surf->h; ++loopy)
                {
                    Uint8* dst = (Uint8*) (dstpix + loopy * dstpitch);
                    Uint8* src = ((Uint8*) (srcpix + loopy * srcpitch))
                        + surf->w * 3 - 3;
                    for (loopx = 0; loopx < surf->w; ++loopx)
                    {
                        dst[0] = src[0];
                        dst[1] = src[1];
                        dst[2] = src[2];
                        dst += 3;
                        src -= 3;
                    }
                }
                break;
            }
        }
    }
    Py_END_ALLOW_THREADS;

    PySurface_Unlock (surfobj);
    SDL_UnlockSurface (newsurf);
    return PySurface_New (newsurf);
}

static PyObject*
surf_rotozoom (PyObject* self, PyObject* arg)
{
    PyObject *surfobj;
    SDL_Surface *surf, *newsurf, *surf32;
    float scale, angle;

    /*get all the arguments*/
    if (!PyArg_ParseTuple (arg, "O!ff", &PySurface_Type, &surfobj, &angle,
                           &scale))
        return NULL;
    surf = PySurface_AsSurface (surfobj);
    if (scale == 0.0)
    {
        newsurf = newsurf_fromsurf (surf, surf->w, surf->h);
        return PySurface_New (newsurf);
    }

    if (surf->format->BitsPerPixel == 32)
    {
        surf32 = surf;
        PySurface_Lock (surfobj);
    }
    else
    {
        Py_BEGIN_ALLOW_THREADS;
        surf32 = SDL_CreateRGBSurface (SDL_SWSURFACE, surf->w, surf->h, 32,
                                       0x000000ff, 0x0000ff00, 0x00ff0000,
                                       0xff000000);
        SDL_BlitSurface (surf, NULL, surf32, NULL);
        Py_END_ALLOW_THREADS;
    }

    Py_BEGIN_ALLOW_THREADS;
    newsurf = rotozoomSurface (surf32, angle, scale, 1);
    Py_END_ALLOW_THREADS;

    if (surf32 == surf)
        PySurface_Unlock (surfobj);
    else
        SDL_FreeSurface (surf32);
    return PySurface_New (newsurf);
}

static SDL_Surface*
chop (SDL_Surface *src, int x, int y, int width, int height)
{
    SDL_Surface* dst;
    int dstwidth,dstheight;
    char *srcpix, *dstpix, *srcrow, *dstrow;
    int srcstepx, srcstepy, dststepx, dststepy;
    int loopx,loopy;

    if ((x + width) > src->w)
        width = src->w - x;
    if ((y + height) > src->h)
        height=src->h - y;
    if (x < 0)
    {
        width -= (-x);
        x = 0;
    }
    if (y < 0)
    {
        height -= (-y);
        y = 0;
    }

    dstwidth = src->w-width;
    dstheight = src->h-height;

    dst = newsurf_fromsurf (src,dstwidth,dstheight);
    if (!dst)
        return NULL;
    SDL_LockSurface (dst);
    srcrow = (char*) src->pixels;
    dstrow = (char*) dst->pixels;
    srcstepx = dststepx = src->format->BytesPerPixel;
    srcstepy = src->pitch;
    dststepy = dst->pitch;

    for (loopy = 0; loopy < src->h; loopy++)
    {
        if ((loopy < y) || (loopy >= (y + height)))
    {
            dstpix = dstrow;
            srcpix = srcrow;
            for (loopx = 0; loopx < src->w; loopx++)
        {
                if ((loopx < x) || (loopx >= (x + width)))
        {
                    switch (src->format->BytesPerPixel)
            {
            case 1:
                        *dstpix = *srcpix;
                        break;
            case 2:
                        *(Uint16*) dstpix = *(Uint16*) srcpix;
                        break;
            case 3:
                        dstpix[0] = srcpix[0];
                        dstpix[1] = srcpix[1];
                        dstpix[2] = srcpix[2];
                        break;
            case 4:
                        *(Uint32*) dstpix = *(Uint32*) srcpix;
                        break;
            }
                    dstpix += dststepx;
        }
                srcpix += srcstepx;
        }
            dstrow += dststepy;
    }
        srcrow += srcstepy;
    }
    SDL_UnlockSurface (dst);
    return dst;
}

static PyObject*
surf_chop (PyObject* self, PyObject* arg)
{
    PyObject *surfobj, *rectobj;
    SDL_Surface* surf, *newsurf;
    GAME_Rect* rect, temp;

    if (!PyArg_ParseTuple (arg, "O!O", &PySurface_Type, &surfobj, &rectobj))
        return NULL;
    if (!(rect = GameRect_FromObject (rectobj, &temp)))
        return RAISE (PyExc_TypeError, "Rect argument is invalid");

    surf=PySurface_AsSurface (surfobj);
    Py_BEGIN_ALLOW_THREADS;
    newsurf = chop (surf, rect->x, rect->y, rect->w, rect->h);
    Py_END_ALLOW_THREADS;

    return PySurface_New (newsurf);
}




/*
 * smooth scale functions.
 */


/* this function implements an area-averaging shrinking filter in the X-dimension */
static void filter_shrink_X_ONLYC(Uint8 *srcpix, Uint8 *dstpix, int height, int srcpitch, int dstpitch, int srcwidth, int dstwidth)
{
    int srcdiff = srcpitch - (srcwidth * 4);
    int dstdiff = dstpitch - (dstwidth * 4);
    int x, y;

    int xspace = 0x10000 * srcwidth / dstwidth; /* must be > 1 */
    int xrecip = (int) (0x100000000LL / xspace);
    for (y = 0; y < height; y++)
    {
        Uint16 accumulate[4] = {0,0,0,0};
        int xcounter = xspace;
        for (x = 0; x < srcwidth; x++)
        {
            if (xcounter > 0x10000)
            {
                accumulate[0] += (Uint16) *srcpix++;
                accumulate[1] += (Uint16) *srcpix++;
                accumulate[2] += (Uint16) *srcpix++;
                accumulate[3] += (Uint16) *srcpix++;
                xcounter -= 0x10000;
            }
            else
            {
                int xfrac = 0x10000 - xcounter;
                /* write out a destination pixel */
                *dstpix++ = (Uint8) (((accumulate[0] + ((srcpix[0] * xcounter) >> 16)) * xrecip) >> 16);
                *dstpix++ = (Uint8) (((accumulate[1] + ((srcpix[1] * xcounter) >> 16)) * xrecip) >> 16);
                *dstpix++ = (Uint8) (((accumulate[2] + ((srcpix[2] * xcounter) >> 16)) * xrecip) >> 16);
                *dstpix++ = (Uint8) (((accumulate[3] + ((srcpix[3] * xcounter) >> 16)) * xrecip) >> 16);
                /* reload the accumulator with the remainder of this pixel */
                accumulate[0] = (Uint16) ((*srcpix++ * xfrac) >> 16);
                accumulate[1] = (Uint16) ((*srcpix++ * xfrac) >> 16);
                accumulate[2] = (Uint16) ((*srcpix++ * xfrac) >> 16);
                accumulate[3] = (Uint16) ((*srcpix++ * xfrac) >> 16);
                xcounter = xspace - xfrac;
            }
        }
        srcpix += srcdiff;
        dstpix += dstdiff;
    }
}

/* this function implements an area-averaging shrinking filter in the Y-dimension */
static void filter_shrink_Y_ONLYC(Uint8 *srcpix, Uint8 *dstpix, int width, int srcpitch, int dstpitch, int srcheight, int dstheight)
{
    Uint16 *templine;
    int srcdiff = srcpitch - (width * 4);
    int dstdiff = dstpitch - (width * 4);
    int x, y;
    int yspace = 0x10000 * srcheight / dstheight; /* must be > 1 */
    int yrecip = (int) (0x100000000LL / yspace);
    int ycounter = yspace;

    /* allocate and clear a memory area for storing the accumulator line */
    templine = (Uint16 *) malloc(dstpitch * 2);
    if (templine == NULL) return;
    memset(templine, 0, dstpitch * 2);

    for (y = 0; y < srcheight; y++)
    {
        Uint16 *accumulate = templine;
        if (ycounter > 0x10000)
        {
            for (x = 0; x < width; x++)
            {
                *accumulate++ += (Uint16) *srcpix++;
                *accumulate++ += (Uint16) *srcpix++;
                *accumulate++ += (Uint16) *srcpix++;
                *accumulate++ += (Uint16) *srcpix++;
            }
            ycounter -= 0x10000;
        }
        else
        {
            int yfrac = 0x10000 - ycounter;
            /* write out a destination line */
            for (x = 0; x < width; x++)
            {
                *dstpix++ = (Uint8) (((*accumulate++ + ((*srcpix++ * ycounter) >> 16)) * yrecip) >> 16);
                *dstpix++ = (Uint8) (((*accumulate++ + ((*srcpix++ * ycounter) >> 16)) * yrecip) >> 16);
                *dstpix++ = (Uint8) (((*accumulate++ + ((*srcpix++ * ycounter) >> 16)) * yrecip) >> 16);
                *dstpix++ = (Uint8) (((*accumulate++ + ((*srcpix++ * ycounter) >> 16)) * yrecip) >> 16);
            }
            dstpix += dstdiff;
            /* reload the accumulator with the remainder of this line */
            accumulate = templine;
            srcpix -= 4 * width;
            for (x = 0; x < width; x++)
            {
                *accumulate++ = (Uint16) ((*srcpix++ * yfrac) >> 16);
                *accumulate++ = (Uint16) ((*srcpix++ * yfrac) >> 16);
                *accumulate++ = (Uint16) ((*srcpix++ * yfrac) >> 16);
                *accumulate++ = (Uint16) ((*srcpix++ * yfrac) >> 16);
            }
            ycounter = yspace - yfrac;
        }
        srcpix += srcdiff;
    } /* for (int y = 0; y < srcheight; y++) */

    /* free the temporary memory */
    free(templine);
}

/* this function implements a bilinear filter in the X-dimension */
static void filter_expand_X_ONLYC(Uint8 *srcpix, Uint8 *dstpix, int height, int srcpitch, int dstpitch, int srcwidth, int dstwidth)
{
    int dstdiff = dstpitch - (dstwidth * 4);
    int *xidx0, *xmult0, *xmult1;
    int x, y;
    int factorwidth = 4;

    /* Allocate memory for factors */
    xidx0 = malloc(dstwidth * 4);
    if (xidx0 == NULL) return;
    xmult0 = (int *) malloc(dstwidth * factorwidth);
    xmult1 = (int *) malloc(dstwidth * factorwidth);
    if (xmult0 == NULL || xmult1 == NULL)
    {
        free(xidx0);
        if (xmult0) free(xmult0);
        if (xmult1) free(xmult1);
    }

    /* Create multiplier factors and starting indices and put them in arrays */
    for (x = 0; x < dstwidth; x++)
    {
        xidx0[x] = x * (srcwidth - 1) / dstwidth;
        xmult1[x] = 0x10000 * ((x * (srcwidth - 1)) % dstwidth) / dstwidth;
        xmult0[x] = 0x10000 - xmult1[x];
    }

    /* Do the scaling in raster order so we don't trash the cache */
    for (y = 0; y < height; y++)
    {
        Uint8 *srcrow0 = srcpix + y * srcpitch;
        for (x = 0; x < dstwidth; x++)
        {
            Uint8 *src = srcrow0 + xidx0[x] * 4;
            int xm0 = xmult0[x];
            int xm1 = xmult1[x];
            *dstpix++ = (Uint8) (((src[0] * xm0) + (src[4] * xm1)) >> 16);
            *dstpix++ = (Uint8) (((src[1] * xm0) + (src[5] * xm1)) >> 16);
            *dstpix++ = (Uint8) (((src[2] * xm0) + (src[6] * xm1)) >> 16);
            *dstpix++ = (Uint8) (((src[3] * xm0) + (src[7] * xm1)) >> 16);
        }
        dstpix += dstdiff;
    }

    /* free memory */
    free(xidx0);
    free(xmult0);
    free(xmult1);
}

/* this function implements a bilinear filter in the Y-dimension */
static void filter_expand_Y_ONLYC(Uint8 *srcpix, Uint8 *dstpix, int width, int srcpitch, int dstpitch, int srcheight, int dstheight)
{
    int x, y;

    for (y = 0; y < dstheight; y++)
    {
        int yidx0 = y * (srcheight - 1) / dstheight;
        Uint8 *srcrow0 = srcpix + yidx0 * srcpitch;
        Uint8 *srcrow1 = srcrow0 + srcpitch;
        int ymult1 = 0x10000 * ((y * (srcheight - 1)) % dstheight) / dstheight;
        int ymult0 = 0x10000 - ymult1;
        for (x = 0; x < width; x++)
        {
            *dstpix++ = (Uint8) (((*srcrow0++ * ymult0) + (*srcrow1++ * ymult1)) >> 16);
            *dstpix++ = (Uint8) (((*srcrow0++ * ymult0) + (*srcrow1++ * ymult1)) >> 16);
            *dstpix++ = (Uint8) (((*srcrow0++ * ymult0) + (*srcrow1++ * ymult1)) >> 16);
            *dstpix++ = (Uint8) (((*srcrow0++ * ymult0) + (*srcrow1++ * ymult1)) >> 16);
        }
    }
}

#if defined(SCALE_MMX_SUPPORT)
static void
smoothscale_init (struct _module_state *st)
{
    if (st->filter_shrink_X == 0)
    {
    if (SDL_HasSSE ())
    {
        st->filter_type = "SSE";
        st->filter_shrink_X = filter_shrink_X_SSE;
        st->filter_shrink_Y = filter_shrink_Y_SSE;
        st->filter_expand_X = filter_expand_X_SSE;
        st->filter_expand_Y = filter_expand_Y_SSE;
    }
    else if (SDL_HasMMX ())
    {
        st->filter_type = "MMX";
        st->filter_shrink_X = filter_shrink_X_MMX;
        st->filter_shrink_Y = filter_shrink_Y_MMX;
        st->filter_expand_X = filter_expand_X_MMX;
        st->filter_expand_Y = filter_expand_Y_MMX;
    }
    else
    {
        st->filter_type = "GENERIC";
        st->filter_shrink_X = filter_shrink_X_ONLYC;
        st->filter_shrink_Y = filter_shrink_Y_ONLYC;
        st->filter_expand_X = filter_expand_X_ONLYC;
        st->filter_expand_Y = filter_expand_Y_ONLYC;
    }
    }
}
#endif

static void convert_24_32(Uint8 *srcpix, int srcpitch, Uint8 *dstpix, int dstpitch, int width, int height)
{
    int srcdiff = srcpitch - (width * 3);
    int dstdiff = dstpitch - (width * 4);
    int x, y;

    for (y = 0; y < height; y++)
    {
        for (x = 0; x < width; x++)
        {
            *dstpix++ = *srcpix++;
            *dstpix++ = *srcpix++;
            *dstpix++ = *srcpix++;
            *dstpix++ = 0xff;
        }
        srcpix += srcdiff;
        dstpix += dstdiff;
    }
}

static void convert_32_24(Uint8 *srcpix, int srcpitch, Uint8 *dstpix, int dstpitch, int width, int height)
{
    int srcdiff = srcpitch - (width * 4);
    int dstdiff = dstpitch - (width * 3);
    int x, y;

    for (y = 0; y < height; y++)
    {
        for (x = 0; x < width; x++)
        {
            *dstpix++ = *srcpix++;
            *dstpix++ = *srcpix++;
            *dstpix++ = *srcpix++;
            srcpix++;
        }
        srcpix += srcdiff;
        dstpix += dstdiff;
    }
}

static void
scalesmooth(SDL_Surface *src, SDL_Surface *dst,
            struct _module_state *st)
{
    Uint8* srcpix = (Uint8*)src->pixels;
    Uint8* dstpix = (Uint8*)dst->pixels;
    Uint8* dst32 = NULL;
    int srcpitch = src->pitch;
    int dstpitch = dst->pitch;

    int srcwidth = src->w;
    int srcheight = src->h;
    int dstwidth = dst->w;
    int dstheight = dst->h;

    int bpp = src->format->BytesPerPixel;

    Uint8 *temppix = NULL;
    int tempwidth=0, temppitch=0, tempheight=0;

    /* convert to 32-bit if necessary */
    if (bpp == 3)
    {
        int newpitch = srcwidth * 4;
        Uint8 *newsrc = (Uint8 *) malloc(newpitch * srcheight);
        if (!newsrc)
            return;
        convert_24_32(srcpix, srcpitch, newsrc, newpitch, srcwidth, srcheight);
        srcpix = newsrc;
        srcpitch = newpitch;
        /* create a destination buffer for the 32-bit result */
        dstpitch = dstwidth << 2;
        dst32 = (Uint8 *) malloc(dstpitch * dstheight);
        if (dst32 == NULL)
        {
            free(srcpix);
            return;
        }
        dstpix = dst32;
    }

    /* Create a temporary processing buffer if we will be scaling both X and Y */
    if (srcwidth != dstwidth && srcheight != dstheight)
    {
        tempwidth = dstwidth;
        temppitch = tempwidth << 2;
        tempheight = srcheight;
        temppix = (Uint8 *) malloc(temppitch * tempheight);
        if (temppix == NULL)
        {
            if (bpp == 3)
            {
                free(srcpix);
                free(dstpix);
            }
            return;
        }
    }

    /* Start the filter by doing X-scaling */
    if (dstwidth < srcwidth) /* shrink */
    {
        if (srcheight != dstheight)
            st->filter_shrink_X(srcpix, temppix, srcheight, srcpitch, temppitch, srcwidth, dstwidth);
        else
            st->filter_shrink_X(srcpix, dstpix, srcheight, srcpitch, dstpitch, srcwidth, dstwidth);
    }
    else if (dstwidth > srcwidth) /* expand */
    {
        if (srcheight != dstheight)
            st->filter_expand_X(srcpix, temppix, srcheight, srcpitch, temppitch, srcwidth, dstwidth);
        else
            st->filter_expand_X(srcpix, dstpix, srcheight, srcpitch, dstpitch, srcwidth, dstwidth);
    }
    /* Now do the Y scale */
    if (dstheight < srcheight) /* shrink */
    {
        if (srcwidth != dstwidth)
            st->filter_shrink_Y(temppix, dstpix, tempwidth, temppitch, dstpitch, srcheight, dstheight);
        else
            st->filter_shrink_Y(srcpix, dstpix, srcwidth, srcpitch, dstpitch, srcheight, dstheight);
    }
    else if (dstheight > srcheight)  /* expand */
    {
        if (srcwidth != dstwidth)
            st->filter_expand_Y(temppix, dstpix, tempwidth, temppitch, dstpitch, srcheight, dstheight);
        else
            st->filter_expand_Y(srcpix, dstpix, srcwidth, srcpitch, dstpitch, srcheight, dstheight);
    }

    /* Convert back to 24-bit if necessary */
    if (bpp == 3)
    {
        convert_32_24(dst32, dstpitch, (Uint8*)dst->pixels, dst->pitch, dstwidth, dstheight);
        free(dst32);
        dst32 = NULL;
        free(srcpix);
        srcpix = NULL;
    }
    /* free temporary buffer if necessary */
    if (temppix != NULL)
        free(temppix);

}


static PyObject* surf_scalesmooth(PyObject* self, PyObject* arg)
{
    PyObject *surfobj, *surfobj2;
    SDL_Surface* surf, *newsurf;
    int width, height, bpp;
    surfobj2 = NULL;

    /*get all the arguments*/
    if (!PyArg_ParseTuple (arg, "O!(ii)|O!", &PySurface_Type, &surfobj,
                           &width, &height, &PySurface_Type, &surfobj2))
        return NULL;

    if (width < 0 || height < 0)
        return RAISE (PyExc_ValueError, "Cannot scale to negative size");

    surf = PySurface_AsSurface (surfobj);

    bpp = surf->format->BytesPerPixel;
    if(bpp < 3 || bpp > 4)
        return RAISE(PyExc_ValueError, "Only 24-bit or 32-bit surfaces can be smoothly scaled");


    if (!surfobj2)
    {
        newsurf = newsurf_fromsurf (surf, width, height);
        if (!newsurf)
            return NULL;
    }
    else
        newsurf = PySurface_AsSurface (surfobj2);

    /* check to see if the size is twice as big. */
    if (newsurf->w != width || newsurf->h != height)
        return RAISE (PyExc_ValueError,
                      "Destination surface not the given width or height.");


    if(((width * bpp + 3) >> 2) > newsurf->pitch)
        return RAISE(PyExc_ValueError, "SDL Error: destination surface pitch not 4-byte aligned.");


    if(width && height)
    {
        SDL_LockSurface(newsurf);
        PySurface_Lock(surfobj);
        Py_BEGIN_ALLOW_THREADS;

        /* handle trivial case */
        if (surf->w == width && surf->h == height) {
            int y;
            for (y = 0; y < height; y++) {
                memcpy((Uint8*)newsurf->pixels + y * newsurf->pitch,
                       (Uint8*)surf->pixels + y * surf->pitch, width * bpp);
            }
        }
        else {
            scalesmooth(surf, newsurf, GETSTATE (self));
        }
        Py_END_ALLOW_THREADS;

        PySurface_Unlock(surfobj);
        SDL_UnlockSurface(newsurf);
    }

    if (surfobj2)
    {
        Py_INCREF (surfobj2);
        return surfobj2;
    }
    else
        return PySurface_New (newsurf);

}

static PyObject *
surf_get_smoothscale_backend (PyObject *self)
{
    return Text_FromUTF8 (GETSTATE (self)->filter_type);
}

static  PyObject *
surf_set_smoothscale_backend (PyObject *self, PyObject *args, PyObject *kwds)
{
    struct _module_state *st = GETSTATE (self);
    char *keywords[] = {"type", NULL};
    const char *type;

    if (!PyArg_ParseTupleAndKeywords (args, kwds, "s:set_smoothscale_backend",
                                      keywords, &type))
    {
        return NULL;
    }

#if defined(SCALE_MMX_SUPPORT)
    if (strcmp (type, "GENERIC") == 0)
    {
        st->filter_type = "GENERIC";
        st->filter_shrink_X = filter_shrink_X_ONLYC;
        st->filter_shrink_Y = filter_shrink_Y_ONLYC;
        st->filter_expand_X = filter_expand_X_ONLYC;
        st->filter_expand_Y = filter_expand_Y_ONLYC;
    }
    else if (strcmp (type, "MMX") == 0)
    {
        if (!SDL_HasMMX ())
        {
            return RAISE (PyExc_ValueError,
                          "MMX not supported on this machine");
        }
        st->filter_type = "MMX";
        st->filter_shrink_X = filter_shrink_X_MMX;
        st->filter_shrink_Y = filter_shrink_Y_MMX;
        st->filter_expand_X = filter_expand_X_MMX;
        st->filter_expand_Y = filter_expand_Y_MMX;
    }
    else if (strcmp (type, "SSE") == 0)
    {
        if (!SDL_HasSSE ())
        {
            return RAISE (PyExc_ValueError,
                          "SSE not supported on this machine");
        }
        st->filter_type = "SSE";
        st->filter_shrink_X = filter_shrink_X_SSE;
        st->filter_shrink_Y = filter_shrink_Y_SSE;
        st->filter_expand_X = filter_expand_X_SSE;
        st->filter_expand_Y = filter_expand_Y_SSE;
    }
    else
    {
        return PyErr_Format (PyExc_ValueError,
                             "Unknown backend type %s", type);
    }
    Py_RETURN_NONE;
#else /* Not an x86 processor */
    if (strcmp (type, "GENERIC") != 0)
    {
        if (strcmp (st->filter_type, "MMX") == 0 ||
            strcmp (st->filter_type, "SSE") == 0    )
        {
            return PyErr_Format (PyExc_ValueError,
                                 "%s not supported on this machine", type);
        }
        return PyErr_Format (PyExc_ValueError,
                             "Unknown backend type %s", type);
    }
    Py_RETURN_NONE;
#endif /* defined(SCALE_MMX_SUPPORT) */
}


static int get_threshold (SDL_Surface *destsurf, SDL_Surface *surf,
                          SDL_Surface *surf2, Uint32 color,  Uint32 threshold,
                          Uint32 diff_color, int change_return, int inverse)
{
    int x, y, result, similar, rshift, gshift, bshift, rshift2, gshift2, bshift2;
    int rloss, gloss, bloss, rloss2, gloss2, bloss2;
    Uint8 *pixels, *destpixels, *pixels2;
    SDL_Rect sdlrect;
    SDL_PixelFormat *format, *destformat, *format2;
    Uint32 the_color, the_color2, rmask, gmask, bmask, rmask2, gmask2, bmask2;
    Uint8 *pix, *byte_buf;
    Uint8 r, g, b, a;
    Uint8 dr, dg, db, da;
    Uint8 tr, tg, tb, ta;

    similar = 0;
    pixels = (Uint8 *) surf->pixels;
    format = surf->format;
    rmask = format->Rmask;
    gmask = format->Gmask;
    bmask = format->Bmask;
    rshift = format->Rshift;
    gshift = format->Gshift;
    bshift = format->Bshift;
    rloss = format->Rloss;
    gloss = format->Gloss;
    bloss = format->Bloss;

    if(change_return) {
        sdlrect.x = sdlrect.y = 0;
        sdlrect.w = destsurf->w;
        sdlrect.h = destsurf->h;
        destpixels = (Uint8 *) destsurf->pixels;
        destformat = destsurf->format;
        result = SDL_FillRect (destsurf, &sdlrect, diff_color);
    } else { /* make gcc stop complaining */
        destpixels = NULL;
        destformat = NULL;
    }

    if(surf2) {
        format2 = surf2->format;
        rmask2 = format2->Rmask;
        gmask2 = format2->Gmask;
        bmask2 = format2->Bmask;
        rshift2 = format2->Rshift;
        gshift2 = format2->Gshift;
        bshift2 = format2->Bshift;
        rloss2 = format2->Rloss;
        gloss2 = format2->Gloss;
        bloss2 = format2->Bloss;
        pixels2 = (Uint8 *) surf2->pixels;
    } else { /* make gcc stop complaining */
        rmask2 = gmask2 = bmask2 = 0;
        rshift2 = gshift2 = bshift2 = 0;
        rloss2 = gloss2 = bloss2 = 0;
        format2 = NULL;
        pixels2 = NULL;
    }

    SDL_GetRGBA (color, format, &r, &g, &b, &a);
    SDL_GetRGBA (threshold, format, &tr, &tg, &tb, &ta);
    SDL_GetRGBA (diff_color, format, &dr, &dg, &db, &da);

    for(y=0; y < surf->h; y++) {
        pixels = (Uint8 *) surf->pixels + y*surf->pitch;
        if (surf2) {
            pixels2 = (Uint8 *) surf2->pixels + y*surf2->pitch;
        }
        for(x=0; x < surf->w; x++) {
            /* the_color = surf->get_at(x,y) */
            switch (format->BytesPerPixel)
            {
            case 1:
                the_color = (Uint32)*((Uint8 *) pixels);
                pixels++;
                break;
            case 2:
                the_color = (Uint32)*((Uint16 *) pixels);
                pixels += 2;
                break;
            case 3:
                pix = ((Uint8 *) pixels);
                pixels += 3;
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
                the_color = (pix[0]) + (pix[1] << 8) + (pix[2] << 16);
#else
                the_color = (pix[2]) + (pix[1] << 8) + (pix[0] << 16);
#endif
                break;
            default:                  /* case 4: */
                the_color = *((Uint32 *) pixels);
                pixels += 4;
                break;
            }

            if (surf2) {
                switch (format2->BytesPerPixel) {
                case 1:
                    the_color2 = (Uint32)*((Uint8 *) pixels2);
                    pixels2++;
                    break;
                case 2:
                    the_color2 = (Uint32)*((Uint16 *) pixels2);
                    pixels2 += 2;
                    break;
                case 3:
                    pix = ((Uint8 *) pixels2);
                    pixels2 += 3;
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
                    the_color2 = (pix[0]) + (pix[1] << 8) + (pix[2] << 16);
#else
                    the_color2 = (pix[2]) + (pix[1] << 8) + (pix[0] << 16);
#endif
                    break;
                default:                  /* case 4: */
                    the_color2 = *((Uint32 *) pixels2);
                    pixels2 += 4;
                    break;
                }

                if (((abs((((the_color2 & rmask2) >> rshift2) << rloss2) - (((the_color & rmask) >> rshift) << rloss)) <= tr) &
                     (abs((((the_color2 & gmask2) >> gshift2) << gloss2) - (((the_color & gmask) >> gshift) << gloss)) <= tg) &
                     (abs((((the_color2 & bmask2) >> bshift2) << bloss2) - (((the_color & bmask) >> bshift) << bloss)) <= tb))
                    ^ inverse) {
                    /* this pixel is within the threshold of the pixel in the
                       other surface. */
                    if (change_return == 2) {
                        /* change the pixel to the color from the first surface.  */

                        /* destsurf->set_at((x,y), the_color) */
                        switch (destformat->BytesPerPixel) {
                        case 1:
                            *((Uint8 *) destpixels + y * destsurf->pitch + x) = (Uint8) the_color;
                            break;
                        case 2:
                            *((Uint16 *) (destpixels + y * destsurf->pitch) + x) = (Uint16) the_color;
                            break;
                        case 3:
                            byte_buf = (Uint8 *) (destpixels + y * destsurf->pitch) + x * 3;
#if (SDL_BYTEORDER == SDL_LIL_ENDIAN)
                            *(byte_buf + (destformat->Rshift >> 3)) = (Uint8) (the_color >> 16);
                            *(byte_buf + (destformat->Gshift >> 3)) = (Uint8) (the_color >> 8);
                            *(byte_buf + (destformat->Bshift >> 3)) = (Uint8) the_color;
#else
                            *(byte_buf + 2 - (destformat->Rshift >> 3)) = (Uint8) (the_color >> 16);
                            *(byte_buf + 2 - (destformat->Gshift >> 3)) = (Uint8) (the_color >> 8);
                            *(byte_buf + 2 - (destformat->Bshift >> 3)) = (Uint8) the_color;
#endif
                            break;
                        default:                  /* case 4: */
                            *((Uint32 *) (destpixels + y * destsurf->pitch) + x) = the_color;
                            break;
                        }
                    }
                    else if (change_return == 1) {
                        /* change the pixel to color.  */
                        /* destsurf->set_at((x,y), color) */
                        switch (destformat->BytesPerPixel) {
                        case 1:
                            *((Uint8 *) destpixels + y * destsurf->pitch + x) = (Uint8) color;
                            break;
                        case 2:
                            *((Uint16 *) (destpixels + y * destsurf->pitch) + x) = (Uint16) color;
                            break;
                        case 3:
                            byte_buf = (Uint8 *) (destpixels + y * destsurf->pitch) + x * 3;
#if (SDL_BYTEORDER == SDL_LIL_ENDIAN)
                            *(byte_buf + (destformat->Rshift >> 3)) = (Uint8) (color >> 16);
                            *(byte_buf + (destformat->Gshift >> 3)) = (Uint8) (color >> 8);
                            *(byte_buf + (destformat->Bshift >> 3)) = (Uint8) color;
#else
                            *(byte_buf + 2 - (destformat->Rshift >> 3)) = (Uint8) (color >> 16);
                            *(byte_buf + 2 - (destformat->Gshift >> 3)) = (Uint8) (color >> 8);
                            *(byte_buf + 2 - (destformat->Bshift >> 3)) = (Uint8) color;
#endif
                            break;
                        default:                  /* case 4: */
                            *((Uint32 *) (destpixels + y * destsurf->pitch) + x) = color;
                            break;
                        }
                    }

                    similar++;
                }

            } else if (((abs((((the_color & rmask) >> rshift) << rloss) - r) <= tr) &
                        (abs((((the_color & gmask) >> gshift) << gloss) - g) <= tg) &
                        (abs((((the_color & bmask) >> bshift) << bloss) - b) <= tb))
                       ^ inverse) {

                /* Comparing the threshold against the color. */

                /* this pixel is within the threshold. */
                if (change_return == 2) {
                    /* destsurf->set_at((x,y), the_color) */
                    switch (destformat->BytesPerPixel)
                    {
                    case 1:
                        *((Uint8 *) destpixels + y * destsurf->pitch + x) = (Uint8) the_color;
                        break;
                    case 2:
                        *((Uint16 *) (destpixels + y * destsurf->pitch) + x) = (Uint16) the_color;
                        break;
                    case 3:
                        byte_buf = (Uint8 *) (destpixels + y * destsurf->pitch) + x * 3;
#if (SDL_BYTEORDER == SDL_LIL_ENDIAN)
                        *(byte_buf + (destformat->Rshift >> 3)) = (Uint8) (the_color >> 16);
                        *(byte_buf + (destformat->Gshift >> 3)) = (Uint8) (the_color >> 8);
                        *(byte_buf + (destformat->Bshift >> 3)) = (Uint8) the_color;
#else
                        *(byte_buf + 2 - (destformat->Rshift >> 3)) = (Uint8) (the_color >> 16);
                        *(byte_buf + 2 - (destformat->Gshift >> 3)) = (Uint8) (the_color >> 8);
                        *(byte_buf + 2 - (destformat->Bshift >> 3)) = (Uint8) the_color;
#endif
                        break;
                    default:                  /* case 4: */
                        *((Uint32 *) (destpixels + y * destsurf->pitch) + x) = the_color;
                        break;
                    }
                }
                else if (change_return == 1) {

                    /* change the pixel to color.  */
                    /* destsurf->set_at((x,y), color) */
                    switch (destformat->BytesPerPixel)
                    {
                    case 1:
                        *((Uint8 *) destpixels + y * destsurf->pitch + x) = (Uint8) color;
                        break;
                    case 2:
                        *((Uint16 *) (destpixels + y * destsurf->pitch) + x) = (Uint16) color;
                        break;
                    case 3:
                        byte_buf = (Uint8 *) (destpixels + y * destsurf->pitch) + x * 3;
#if (SDL_BYTEORDER == SDL_LIL_ENDIAN)
                        *(byte_buf + (destformat->Rshift >> 3)) = (Uint8) (color >> 16);
                        *(byte_buf + (destformat->Gshift >> 3)) = (Uint8) (color >> 8);
                        *(byte_buf + (destformat->Bshift >> 3)) = (Uint8) color;
#else
                        *(byte_buf + 2 - (destformat->Rshift >> 3)) = (Uint8) (color >> 16);
                        *(byte_buf + 2 - (destformat->Gshift >> 3)) = (Uint8) (color >> 8);
                        *(byte_buf + 2 - (destformat->Bshift >> 3)) = (Uint8) color;
#endif
                        break;
                    default:                  /* case 4: */
                        *((Uint32 *) (destpixels + y * destsurf->pitch) + x) = color;
                        break;
                    }
                }

                similar++;
            }
        }
    }
    return similar;
}




static PyObject* surf_threshold(PyObject* self, PyObject* arg)
{
    PyObject *surfobj, *surfobj2 = NULL, *surfobj3 = NULL;
    SDL_Surface* surf = NULL, *destsurf = NULL, *surf2 = NULL;
    int bpp, change_return = 1, inverse = 0;
    int num_threshold_pixels = 0;

    PyObject *rgba_obj_color;
    PyObject *rgba_obj_threshold = NULL;
    PyObject *rgba_obj_diff_color = NULL;
    Uint8 rgba_color[4];
    Uint8 rgba_threshold[4] = {0, 0, 0, 255};
    Uint8 rgba_diff_color[4] = {0, 0, 0, 255};

    Uint32 color;
    Uint32 color_threshold;
    Uint32 color_diff_color;

    /*get all the arguments*/
    if (!PyArg_ParseTuple (arg, "O!O!O|OOiO!i", &PySurface_Type, &surfobj,
                           &PySurface_Type, &surfobj2,
                           &rgba_obj_color,  &rgba_obj_threshold, &rgba_obj_diff_color,
                           &change_return,
                           &PySurface_Type, &surfobj3, &inverse))
        return NULL;


    destsurf = PySurface_AsSurface (surfobj);
    surf = PySurface_AsSurface (surfobj2);
    if(surfobj3) {
        surf2 = PySurface_AsSurface (surfobj3);
    }



    if (PyInt_Check (rgba_obj_color)) {
        color = (Uint32) PyInt_AsLong (rgba_obj_color);
    } else if (PyLong_Check (rgba_obj_color)) {
        color = (Uint32) PyLong_AsUnsignedLong (rgba_obj_color);
    }
    else if (RGBAFromColorObj (rgba_obj_color, rgba_color)) {
        color = SDL_MapRGBA (surf->format, rgba_color[0], rgba_color[1],
            rgba_color[2], rgba_color[3]);
    } else {
        return RAISE (PyExc_TypeError, "invalid color argument");
    }

    if(rgba_obj_threshold) {

        if (PyInt_Check (rgba_obj_threshold))
            color_threshold = (Uint32) PyInt_AsLong (rgba_obj_threshold);
        else if (PyLong_Check (rgba_obj_threshold))
            color_threshold = (Uint32) PyLong_AsUnsignedLong
                (rgba_obj_threshold);
        else if (RGBAFromColorObj (rgba_obj_threshold, rgba_threshold))
            color_threshold = SDL_MapRGBA (surf->format,
                                           rgba_threshold[0],
                                           rgba_threshold[1],
                                           rgba_threshold[2],
                                           rgba_threshold[3]);
        else
            return RAISE (PyExc_TypeError, "invalid threshold argument");

    } else {
        color_threshold = SDL_MapRGBA (surf->format,
                                       rgba_threshold[0],
                                       rgba_threshold[1],
                                       rgba_threshold[2],
                                       rgba_threshold[3]);
    }

    if(rgba_obj_diff_color) {

        if (PyInt_Check (rgba_obj_diff_color))
            color_diff_color = (Uint32) PyInt_AsLong (rgba_obj_diff_color);
        else if (PyLong_Check (rgba_obj_diff_color))
            color_diff_color = (Uint32) PyLong_AsUnsignedLong
                (rgba_obj_diff_color);
        else if (RGBAFromColorObj (rgba_obj_diff_color, rgba_diff_color))
            color_diff_color = SDL_MapRGBA (surf->format,
                                            rgba_diff_color[0],
                                            rgba_diff_color[1],
                                            rgba_diff_color[2],
                                            rgba_diff_color[3]);
        else
            return RAISE (PyExc_TypeError, "invalid diff_color argument");
    } else {
        color_diff_color = SDL_MapRGBA (surf->format,
                                        rgba_diff_color[0],
                                        rgba_diff_color[1],
                                        rgba_diff_color[2],
                                        rgba_diff_color[3]);
    }

    bpp = surf->format->BytesPerPixel;

    PySurface_Lock(surfobj);
    PySurface_Lock(surfobj2);

    if(surfobj3) {
        PySurface_Lock(surfobj3);
    }
    Py_BEGIN_ALLOW_THREADS;


    num_threshold_pixels = get_threshold (destsurf,
                                          surf,
                                          surf2,
                                          color,
                                          color_threshold,
                                          color_diff_color,
                                          change_return,
                                          inverse);


    Py_END_ALLOW_THREADS;

    PySurface_Unlock(surfobj);
    PySurface_Unlock(surfobj2);
    if(surfobj3) {
        PySurface_Unlock(surfobj3);
    }

    return PyInt_FromLong (num_threshold_pixels);
}



/*

TODO:
add_4
sub_4
mul_4
clamp_4

*/


#define SURF_GET_AT(p_color, p_surf, p_x, p_y, p_pixels, p_format, p_pix) \
        switch (p_format->BytesPerPixel) \
        { \
        case 1: \
            p_color = (Uint32)*((Uint8 *) (p_pixels) + (p_y) * p_surf->pitch + (p_x)); \
            break; \
        case 2: \
            p_color = (Uint32)*((Uint16 *) ((p_pixels) + (p_y) * p_surf->pitch) + (p_x)); \
            break; \
        case 3: \
            p_pix = ((Uint8 *) (p_pixels + (p_y) * p_surf->pitch) + (p_x) * 3); \
            p_color = (SDL_BYTEORDER == SDL_LIL_ENDIAN) ? \
                      (p_pix[0]) + (p_pix[1] << 8) + (p_pix[2] << 16) : \
                      (p_pix[2]) + (p_pix[1] << 8) + (p_pix[0] << 16); \
            break; \
        default:                  /* case 4: */ \
            p_color = *((Uint32 *) (p_pixels + (p_y) * p_surf->pitch) + (p_x)); \
            break; \
        } \



#if (SDL_BYTEORDER == SDL_LIL_ENDIAN)

#define SURF_SET_AT(p_color, p_surf, p_x, p_y, p_pixels, p_format, p_byte_buf) \
        switch (p_format->BytesPerPixel) \
        { \
        case 1: \
            *((Uint8 *) p_pixels + (p_y) * p_surf->pitch + (p_x)) = (Uint8) p_color; \
            break; \
        case 2: \
            *((Uint16 *) (p_pixels + (p_y) * p_surf->pitch) + (p_x)) = (Uint16) p_color; \
            break; \
        case 3: \
            p_byte_buf = (Uint8 *) (p_pixels + (p_y) * p_surf->pitch) + (p_x) * 3; \
            *(p_byte_buf + (p_format->Rshift >> 3)) = (Uint8) (p_color >> 16); \
            *(p_byte_buf + (p_format->Gshift >> 3)) = (Uint8) (p_color >> 8); \
            *(p_byte_buf + (p_format->Bshift >> 3)) = (Uint8) p_color; \
            break; \
        default:                   \
            *((Uint32 *) (p_pixels + (p_y) * p_surf->pitch) + (p_x)) = p_color; \
            break; \
        } \

#else

#define SURF_SET_AT(p_color, p_surf, p_x, p_y, p_pixels, p_format, p_byte_buf) \
        switch (p_format->BytesPerPixel) \
        { \
        case 1: \
            *((Uint8 *) p_pixels + (p_y) * p_surf->pitch + (p_x)) = (Uint8) p_color; \
            break; \
        case 2: \
            *((Uint16 *) (p_pixels + (p_y) * p_surf->pitch) + (p_x)) = (Uint16) p_color; \
            break; \
        case 3: \
            p_byte_buf = (Uint8 *) (p_pixels + (p_y) * p_surf->pitch) + (p_x) * 3; \
            *(p_byte_buf + 2 - (p_format->Rshift >> 3)) = (Uint8) (p_color >> 16); \
            *(p_byte_buf + 2 - (p_format->Gshift >> 3)) = (Uint8) (p_color >> 8); \
            *(p_byte_buf + 2 - (p_format->Bshift >> 3)) = (Uint8) p_color; \
            break; \
        default:                   \
            *((Uint32 *) (p_pixels + (p_y) * p_surf->pitch) + (p_x)) = p_color; \
            break; \
        } \

#endif








/*
number to use for missing samples
*/
#define LAPLACIAN_NUM 0xFFFFFFFF



void laplacian(SDL_Surface *surf, SDL_Surface *destsurf) {

    int ii;
    int x,y,height,width;

    Uint32 sample[9];
    //Uint32 total[4];
    int total[4];

    Uint8 c1r, c1g, c1b, c1a;
    //Uint32 c1r, c1g, c1b, c1a;
    Uint8 acolor[4];

    Uint32 the_color;

    int atmp0;
    int atmp1;
    int atmp2;
    int atmp3;

    SDL_PixelFormat *format, *destformat;
    Uint8 *pixels, *destpixels;
    Uint8 *pix;

    Uint8 *byte_buf;


    height = surf->h;
    width = surf->w;


    pixels = (Uint8 *) surf->pixels;
    format = surf->format;

    destpixels = (Uint8 *) destsurf->pixels;
    destformat = destsurf->format;

/*
    -1 -1 -1
    -1  8 -1
    -1 -1 -1

    col = (sample[4] * 8) - (sample[0] + sample[1] + sample[2] +
                             sample[3] +             sample[5] +
                             sample[6] + sample[7] + sample[8])

    [(-1,-1), (0,-1), (1,-1),     (-1,0), (0,0), (1,0),     (-1,1), (0,1), (1,1)]

*/

    for(y=0;y<height;y++) {
        for(x=0;x<width;x++) {

            // Need to bounds check these accesses.

            if(y > 0) {
                if(x>0) {
                    SURF_GET_AT(sample[0], surf, x+ -1, y+ -1, pixels, format, pix);
                }

                SURF_GET_AT(sample[1], surf, x+  0, y+ -1, pixels, format, pix);

                if(x+1<width) {
                    SURF_GET_AT(sample[2], surf, x+  1, y+ -1, pixels, format, pix);
                }
            } else {
                sample[0] = LAPLACIAN_NUM;
                sample[1] = LAPLACIAN_NUM;
                sample[2] = LAPLACIAN_NUM;
            }

            if(x>0) {
                SURF_GET_AT(sample[3], surf, x+ -1, y+  0, pixels, format, pix);
            } else {
                sample[3] = LAPLACIAN_NUM;
            }


            //SURF_GET_AT(sample[4], surf, x+0 , y+0);
            sample[4] = 0;

            if(x+1<width) {
                SURF_GET_AT(sample[5], surf, x+  1, y+  0, pixels, format, pix);
            } else {
                sample[5] = LAPLACIAN_NUM;
            }

            if(y+1 < height) {

                if(x>0) {
                    SURF_GET_AT(sample[6], surf, x+ -1, y+  1, pixels, format, pix);
                }

                SURF_GET_AT(sample[7], surf, x+  0, y+  1, pixels, format, pix);

                if(x+1<width) {
                    SURF_GET_AT(sample[8], surf, x+  1, y+  1, pixels, format, pix);
                }
            } else {
                sample[6] = LAPLACIAN_NUM;
                sample[7] = LAPLACIAN_NUM;
                sample[8] = LAPLACIAN_NUM;
            }


            total[0] = 0; total[1] = 0; total[2] = 0; total[3] = 0;

            for(ii =0; ii<9; ii++) {
                SDL_GetRGBA (sample[ii], format, &c1r, &c1g, &c1b, &c1a);
                total[0] += c1r; total[1] += c1g; total[2] += c1b; total[3] += c1a;
            }


            SURF_GET_AT(sample[4], surf, x, y, pixels, format, pix);

            SDL_GetRGBA (sample[4], format, &c1r, &c1g, &c1b, &c1a);

            // cast on the right to a signed int, and then clamp to 0-255.

            //atmp = c1r * 8


            atmp0 = c1r * 8;
            acolor[0] = MIN(MAX(atmp0 - total[0], 0), 255);
            atmp1 = c1g * 8;
            acolor[1] = MIN(MAX(atmp1 - total[1], 0), 255);
            atmp2 = c1b * 8;
            acolor[2] = MIN(MAX(atmp2 - total[2], 0), 255);
            atmp3 = c1a * 8;
            acolor[3] = MIN(MAX(atmp3 - total[3], 0), 255);

            //printf("%d;;%d;;%d;;  ", atmp0, acolor[0],total[0]);


            //printf("%d,%d,%d,%d;;  \n", acolor[0], acolor[1], acolor[2], acolor[3]);

            //the_color = (Uint32)acolor;
            //the_color = 0x00000000;



            // cast on the right to Uint32, and then clamp to 255.

            the_color = SDL_MapRGBA (surf->format, acolor[0], acolor[1], acolor[2], acolor[3]);




            // set_at(destsurf, color, x,y);

            switch (destformat->BytesPerPixel)
            {
            case 1:
                *((Uint8 *) destpixels + y * destsurf->pitch + x) = (Uint8) the_color;
                break;
            case 2:
                *((Uint16 *) (destpixels + y * destsurf->pitch) + x) = (Uint16) the_color;
                break;
            case 3:
                byte_buf = (Uint8 *) (destpixels + y * destsurf->pitch) + x * 3;
#if (SDL_BYTEORDER == SDL_LIL_ENDIAN)
                *(byte_buf + (destformat->Rshift >> 3)) = (Uint8) (the_color >> 16);
                *(byte_buf + (destformat->Gshift >> 3)) = (Uint8) (the_color >> 8);
                *(byte_buf + (destformat->Bshift >> 3)) = (Uint8) the_color;
#else
                *(byte_buf + 2 - (destformat->Rshift >> 3)) = (Uint8) (the_color >> 16);
                *(byte_buf + 2 - (destformat->Gshift >> 3)) = (Uint8) (the_color >> 8);
                *(byte_buf + 2 - (destformat->Bshift >> 3)) = (Uint8) the_color;
#endif
                break;
            default:
                *((Uint32 *) (destpixels + y * destsurf->pitch) + x) = the_color;
                break;
            }



        }
    }



}











static PyObject*
surf_laplacian (PyObject* self, PyObject* arg)
{
    PyObject *surfobj, *surfobj2;
    SDL_Surface *surf;
    SDL_Surface *newsurf;
    int width, height;
    surfobj2 = NULL;

    /*get all the arguments*/
    if (!PyArg_ParseTuple (arg, "O!|O!", &PySurface_Type, &surfobj,
                           &PySurface_Type, &surfobj2))
        return NULL;

    surf = PySurface_AsSurface (surfobj);

    /* if the second surface is not there, then make a new one. */

    if (!surfobj2)
    {
        width = surf->w;
        height = surf->h;

        newsurf = newsurf_fromsurf (surf, width, height);

        if (!newsurf)
            return NULL;
    }
    else
        newsurf = PySurface_AsSurface (surfobj2);

    /* check to see if the size is the correct size. */
    if (newsurf->w != (surf->w) || newsurf->h != (surf->h))
        return RAISE (PyExc_ValueError, "Destination surface not the same size.");

    /* check to see if the format of the surface is the same. */
    if (surf->format->BytesPerPixel != newsurf->format->BytesPerPixel)
        return RAISE (PyExc_ValueError,
                      "Source and destination surfaces need the same format.");

    SDL_LockSurface (newsurf);
    SDL_LockSurface (surf);

    Py_BEGIN_ALLOW_THREADS;
    laplacian (surf, newsurf);
    Py_END_ALLOW_THREADS;

    SDL_UnlockSurface (surf);
    SDL_UnlockSurface (newsurf);

    if (surfobj2)
    {
        Py_INCREF (surfobj2);
        return surfobj2;
    }
    else
        return PySurface_New (newsurf);
}




int average_surfaces(SDL_Surface **surfaces,
                     int num_surfaces,
                     SDL_Surface *destsurf,
                     int palette_colors) {
    /*
        returns the average surface from the ones given.

        All surfaces need to be the same size.

        palette_colors - if true we average the colors in palette, otherwise we
            average the pixel values.  This is useful if the surface is
            actually greyscale colors, and not palette colors.

    */


    Uint32 *accumulate;
    Uint32 *the_idx;
    Uint32 the_color;
    SDL_Surface *surf;
    int height, width, x, y, surf_idx;

    float div_inv;





    SDL_PixelFormat *format, *destformat;
    Uint8 *pixels, *destpixels;
    Uint8 *pix;
    Uint8 *byte_buf;

    Uint32 rmask, gmask, bmask;
    int rshift, gshift, bshift, rloss, gloss, bloss;
    int num_elements;

    if(!num_surfaces) { return 0; }

    height = surfaces[0]->h;
    width = surfaces[0]->w;

    destpixels = (Uint8 *) destsurf->pixels;
    destformat = destsurf->format;


    /* allocate an array to accumulate them all.

    If we're using 1 byte per pixel, then only need to average on that much.
    */

    if((destformat->BytesPerPixel == 1) &&
       (destformat->palette) &&
       (!palette_colors)) {
        num_elements = 1;
    } else {
        num_elements = 3;
    }

    accumulate = (Uint32 *) calloc(1, sizeof(Uint32) * height * width * num_elements );



    if(!accumulate) { return -1; }


    /* add up the r,g,b from all the surfaces. */

    for(surf_idx=0;surf_idx < num_surfaces;surf_idx++) {
        surf = surfaces[surf_idx];

        pixels = (Uint8 *) surf->pixels;
        format = surf->format;
        rmask = format->Rmask;
        gmask = format->Gmask;
        bmask = format->Bmask;
        rshift = format->Rshift;
        gshift = format->Gshift;
        bshift = format->Bshift;
        rloss = format->Rloss;
        gloss = format->Gloss;
        bloss = format->Bloss;

        the_idx = accumulate;
        /* If palette surface, we use a different code path... */

        if((format->BytesPerPixel == 1 && destformat->BytesPerPixel == 1)
            && (format->palette) && (destformat->palette) && (!palette_colors)
          ) {
            /*
            This is useful if the surface is actually greyscale colors,
            and not palette colors.
            */
            for(y=0;y<height;y++) {
                for(x=0;x<width;x++) {
                    SURF_GET_AT(the_color, surf, x, y, pixels, format, pix);
                    *(the_idx) += the_color;
                    the_idx++;
                }
            }


        } else {
            /* TODO: This doesn't work correctly for palette surfaces yet, when the
                    source is paletted.  Probably need to use something
                    like GET_PIXELVALS_1 from surface.h
            */


            /* for non palette surfaces, we do this... */
            for(y=0;y<height;y++) {
                for(x=0;x<width;x++) {
                    SURF_GET_AT(the_color, surf, x, y, pixels, format, pix);

                    *(the_idx) += ((the_color & rmask) >> rshift) << rloss;
                    *(the_idx + 1) += ((the_color & gmask) >> gshift) << gloss;
                    *(the_idx + 2) += ((the_color & bmask) >> bshift) << bloss;
                    the_idx += 3;
                }
            }
        }



    }


    /* blit the accumulated array back to the destination surface. */


    div_inv = (float) (1.0L / (num_surfaces));

    the_idx = accumulate;

    if(num_elements == 1 && (!palette_colors)) {
        /* this is where we are using the palette surface without using its
        colors from the palette.
        */
        for(y=0;y<height;y++) {
            for(x=0;x<width;x++) {
                the_color = (*(the_idx) * div_inv + .5f);
                SURF_SET_AT(the_color, destsurf, x, y, destpixels, destformat, byte_buf);
                the_idx++;
            }
        }
    /* TODO: will need to handle palette colors.
    */

    } else if (num_elements == 3) {
        for(y=0;y<height;y++) {
            for(x=0;x<width;x++) {

                the_color = SDL_MapRGB (destformat,
                                        (Uint8) (*(the_idx) * div_inv + .5f),
                                        (Uint8) (*(the_idx + 1) * div_inv + .5f),
                                        (Uint8) (*(the_idx + 2) * div_inv + .5f));

                /* TODO: should it take into consideration the output
                    shifts/masks/losses?  Or does SDL_MapRGB do that correctly?

                    *(the_idx) += ((the_color & rmask) >> rshift) << rloss;
                    *(the_idx + 1) += ((the_color & gmask) >> gshift) << gloss;
                    *(the_idx + 2) += ((the_color & bmask) >> bshift) << bloss;
                */

                SURF_SET_AT(the_color, destsurf, x, y, destpixels, destformat, byte_buf);

                the_idx += 3;
            }
        }
    } else {
        free(accumulate);
        return -4;
    }

    free(accumulate);

    return 1;
}










/*
    returns the average surface from the ones given.

    All surfaces need to be the same size.

    palette_colors - if true we average the colors in palette, otherwise we
        average the pixel values.  This is useful if the surface is
        actually greyscale colors, and not palette colors.

*/
static PyObject*
surf_average_surfaces (PyObject* self, PyObject* arg)
{
    PyObject *surfobj2;
    SDL_Surface *surf;
    SDL_Surface *newsurf;
    SDL_Surface **surfaces;
    int width, height;
    int an_error;
    size_t size, loop, loop_up_to;
    int palette_colors = 1;

    PyObject* list, *obj;
    PyObject* ret = NULL;

    an_error = 0;


    surfobj2 = NULL;
    newsurf = NULL;

    if (!PyArg_ParseTuple (arg, "O|O!i", &list, &PySurface_Type, &surfobj2, &palette_colors ))
        return NULL;

    if (!PySequence_Check (list))
        return RAISE (PyExc_TypeError, "Argument must be a sequence of surface objects.");


    size = PySequence_Length (list); /*warning, size could be -1 on error?*/

    if(size < 1)
        return RAISE (PyExc_TypeError, "Needs to be given at least one surface.");


    /* Allocate an array of surface pointers. */

    surfaces = (SDL_Surface **) calloc(1, sizeof(SDL_Surface *) * size);

    if(!surfaces) {
        return RAISE (PyExc_MemoryError, "Not enough memory to store surfaces.\n");
    }


    /* Iterate over 'surfaces' passed in. */



    /* need to get the first surface to see how big it is */

    loop = 0;


    for (loop = 0; loop < size; ++loop)
    {

        obj = PySequence_GetItem (list, loop);

        if(!obj) {
            Py_XDECREF (obj);
            ret = RAISE (PyExc_TypeError, "Needs to be a surface object.");
            an_error = 1;
            break;
        }


        if (!PySurface_Check (obj)) {
            Py_XDECREF (obj);
            ret = RAISE (PyExc_TypeError, "Needs to be a surface object.");
            an_error = 1;
            break;
        }


        surf = PySurface_AsSurface (obj);

        if(!surf) {
            Py_XDECREF (obj);
            ret = RAISE (PyExc_TypeError, "Needs to be a surface object.");
            an_error = 1;
            break;
        }


        if(loop == 0) {
            /* if the second surface is not there, then make a new one. */
            if (!surfobj2) {

                width = surf->w;
                height = surf->h;

                newsurf = newsurf_fromsurf (surf, width, height);

                if (!newsurf) {
                    Py_XDECREF (obj);
                    ret = RAISE (PyExc_ValueError, "Could not create new surface.");
                    an_error = 1;
                    break;
                }
            }
            else
                newsurf = PySurface_AsSurface (surfobj2);


            /* check to see if the size is the correct size. */
            if (newsurf->w != (surf->w) || newsurf->h != (surf->h)) {
                Py_XDECREF (obj);
                ret = RAISE (PyExc_ValueError, "Destination surface not the same size.");
                an_error = 1;
                break;
            }

            /* check to see if the format of the surface is the same. */
            if (surf->format->BytesPerPixel != newsurf->format->BytesPerPixel) {
                Py_XDECREF (obj);
                ret = RAISE (PyExc_ValueError, "Source and destination surfaces need the same format.");
                an_error = 1;
                break;
            }
        }



        /* Copy surface pointer, and also lock surface. */
        SDL_LockSurface (surf);
        surfaces[loop] = surf;

        Py_DECREF (obj);
    }


    loop_up_to = loop;

    if(!an_error) {

        /* Process images, get average surface. */

        SDL_LockSurface (newsurf);

        Py_BEGIN_ALLOW_THREADS;
        average_surfaces (surfaces, size, newsurf, palette_colors);
        Py_END_ALLOW_THREADS;

        SDL_UnlockSurface (newsurf);

        if (surfobj2)
        {
            Py_INCREF (surfobj2);
            ret = surfobj2;
        }
        else {
            ret = PySurface_New (newsurf);
        }
    } else {

    }



    /* cleanup */

    /* unlock the surfaces we got up to. */

    for (loop = 0; loop < loop_up_to; loop++) {
        if(surfaces[loop]) {
            SDL_UnlockSurface (surfaces[loop]);
        }
    }

    free(surfaces);

    return ret;
}


void average_color(SDL_Surface* surf, int x, int y, int width, int height, Uint8* r, Uint8* g, Uint8* b, Uint8* a) {
    Uint32 color, rmask, gmask, bmask, amask;
    Uint8 *pixels, *pix;
    unsigned int rtot, gtot, btot, atot, size, rshift, gshift, bshift, ashift;
    unsigned int rloss, gloss, bloss, aloss;
    int row, col;

    SDL_PixelFormat *format;

    format = surf->format;
    rmask = format->Rmask;
    gmask = format->Gmask;
    bmask = format->Bmask;
    amask = format->Amask;
    rshift = format->Rshift;
    gshift = format->Gshift;
    bshift = format->Bshift;
    ashift = format->Ashift;
    rloss = format->Rloss;
    gloss = format->Gloss;
    bloss = format->Bloss;
    aloss = format->Aloss;
    rtot = gtot = btot = atot = 0;

    /* make sure the area specified is within the Surface */
    if ((x + width) > surf->w)
        width = surf->w - x;
    if ((y + height) > surf->h)
        height = surf->h - y;
    if (x < 0)
    {
        width -= (-x);
        x = 0;
    }
    if (y < 0)
    {
        height -= (-y);
        y = 0;
    }

    size = width*height;

    switch (format->BytesPerPixel) {
        case 1:
            for (row = y; row < y+height; row++) {
                pixels = (Uint8 *) surf->pixels + row*surf->pitch + x;
                for (col = x; col < x+width; col++) {
                    color = (Uint32)*((Uint8 *) pixels);
                    rtot += ((color & rmask) >> rshift) << rloss;
                    gtot += ((color & gmask) >> gshift) << gloss;
                    btot += ((color & bmask) >> bshift) << bloss;
                    atot += ((color & amask) >> ashift) << aloss;
                    pixels++;
                }
            }
            break;
        case 2:
            for (row = y; row < y+height; row++) {
                pixels = (Uint8 *) surf->pixels + row*surf->pitch + x*2;
                for (col = x; col < x+width; col++) {
                    color = (Uint32)*((Uint16 *) pixels);
                    rtot += ((color & rmask) >> rshift) << rloss;
                    gtot += ((color & gmask) >> gshift) << gloss;
                    btot += ((color & bmask) >> bshift) << bloss;
                    atot += ((color & amask) >> ashift) << aloss;
                    pixels += 2;
                }
            }
            break;
        case 3:
            for (row = y; row < y+height; row++) {
                pixels = (Uint8 *) surf->pixels + row*surf->pitch + x*3;
                for (col = x; col < x+width; col++) {
                    pix = pixels;
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
                    color = (pix[0]) + (pix[1] << 8) + (pix[2] << 16);
#else
                    color = (pix[2]) + (pix[1] << 8) + (pix[0] << 16);
#endif
                    rtot += ((color & rmask) >> rshift) << rloss;
                    gtot += ((color & gmask) >> gshift) << gloss;
                    btot += ((color & bmask) >> bshift) << bloss;
                    atot += ((color & amask) >> ashift) << aloss;
                    pixels += 3;
                }
            }
            break;
        default:                  /* case 4: */
            for (row = y; row < y+height; row++) {
                pixels = (Uint8 *) surf->pixels + row*surf->pitch + x*4;
                for (col = x; col < x+width; col++) {
                    color = *(Uint32 *)pixels;
                    rtot += ((color & rmask) >> rshift) << rloss;
                    gtot += ((color & gmask) >> gshift) << gloss;
                    btot += ((color & bmask) >> bshift) << bloss;
                    atot += ((color & amask) >> ashift) << aloss;
                    pixels += 4;
                }
            }
            break;
    }
    *r = rtot/size;
    *g = gtot/size;
    *b = btot/size;
    *a = atot/size;
}

static PyObject* surf_average_color(PyObject* self, PyObject* arg)
{
    PyObject *surfobj, *rectobj = NULL;
    SDL_Surface* surf;
    GAME_Rect* rect, temp;
    Uint8 r, g, b, a;
    int x, y, w, h;

    if (!PyArg_ParseTuple (arg, "O!|O", &PySurface_Type, &surfobj, &rectobj))
        return NULL;

    surf = PySurface_AsSurface (surfobj);
    PySurface_Lock (surfobj);

    if (!rectobj) {
        x = 0;
        y = 0;
        w = surf->w;
        h = surf->h;
    } else {
        if (!(rect = GameRect_FromObject (rectobj, &temp)))
            return RAISE (PyExc_TypeError, "Rect argument is invalid");
        x = rect->x;
        y = rect->y;
        w = rect->w;
        h = rect->h;
    }

    Py_BEGIN_ALLOW_THREADS;
    average_color(surf, x, y, w, h, &r, &g, &b, &a);
    Py_END_ALLOW_THREADS;

    PySurface_Unlock (surfobj);
    return Py_BuildValue ("(bbbb)", r, g, b, a);
}

static PyMethodDef _transform_methods[] =
{
    { "scale", surf_scale, METH_VARARGS, DOC_PYGAMETRANSFORMSCALE },
    { "rotate", surf_rotate, METH_VARARGS, DOC_PYGAMETRANSFORMROTATE },
    { "flip", surf_flip, METH_VARARGS, DOC_PYGAMETRANSFORMFLIP },
    { "rotozoom", surf_rotozoom, METH_VARARGS, DOC_PYGAMETRANSFORMROTOZOOM},
    { "chop", surf_chop, METH_VARARGS, DOC_PYGAMETRANSFORMCHOP },
    { "scale2x", surf_scale2x, METH_VARARGS, DOC_PYGAMETRANSFORMSCALE2X },
    { "smoothscale", surf_scalesmooth, METH_VARARGS, DOC_PYGAMETRANSFORMSMOOTHSCALE },
    { "get_smoothscale_backend", (PyCFunction) surf_get_smoothscale_backend, METH_NOARGS,
          DOC_PYGAMETRANSFORMGETSMOOTHSCALEBACKEND },
    { "set_smoothscale_backend", (PyCFunction) surf_set_smoothscale_backend,
          METH_VARARGS | METH_KEYWORDS,
          DOC_PYGAMETRANSFORMSETSMOOTHSCALEBACKEND },
    { "threshold", surf_threshold, METH_VARARGS, DOC_PYGAMETRANSFORMTHRESHOLD },
    { "laplacian", surf_laplacian, METH_VARARGS, DOC_PYGAMETRANSFORMTHRESHOLD },
    { "average_surfaces", surf_average_surfaces, METH_VARARGS, DOC_PYGAMETRANSFORMAVERAGESURFACES },
    { "average_color", surf_average_color, METH_VARARGS, DOC_PYGAMETRANSFORMAVERAGECOLOR },

    { NULL, NULL, 0, NULL }
};


MODINIT_DEFINE (transform)
{
    PyObject *module;
    struct _module_state *st;

#if PY3
    static struct PyModuleDef _module = {
        PyModuleDef_HEAD_INIT,
        "transform",
        DOC_PYGAMETRANSFORM,
        sizeof (struct _module_state),
        _transform_methods,
        NULL, NULL, NULL, NULL
    };
#endif

    /* imported needed apis; Do this first so if there is an error
       the module is not loaded.
    */
    import_pygame_base ();
    if (PyErr_Occurred ()) {
        MODINIT_ERROR;
    }
    import_pygame_color ();
    if (PyErr_Occurred ()) {
        MODINIT_ERROR;
    }
    import_pygame_rect ();
    if (PyErr_Occurred ()) {
        MODINIT_ERROR;
    }
    import_pygame_surface ();
    if (PyErr_Occurred ()) {
        MODINIT_ERROR;
    }

    /* create the module */
#if PY3
    module = PyModule_Create (&_module);
#else
    module = Py_InitModule3 (MODPREFIX "transform",
                             _transform_methods,
                             DOC_PYGAMETRANSFORM);
#endif

    if (module == 0) {
        MODINIT_ERROR;
    }

    st = GETSTATE (module);
    if (st->filter_type == 0) {
        smoothscale_init (st);
    }
    MODINIT_RETURN (module);
}
