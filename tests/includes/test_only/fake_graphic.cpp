/*
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

   Product name: redemption, a FLOSS RDP proxy
   Copyright (C) Wallix 2010-2013
   Author(s): Christophe Grosjean

   Fake Graphic class for Unit Testing
*/

#include "test_only/fake_graphic.hpp"
#include "utils/log.hpp"
#include "utils/png.hpp"
#include "core/client_info.hpp"
#include "core/RDP/RDPDrawable.hpp"
#include "core/RDP/orders/RDPOrdersSecondaryBrushCache.hpp"
#include "core/RDP/orders/RDPOrdersSecondaryColorCache.hpp"
#include "core/RDP/orders/AlternateSecondaryWindowing.hpp"
#include "gdi/graphic_api.hpp"

#include <cstdlib>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

namespace
{
    void draw_impl(RDPDrawable & gd, int verbose, const RDPBitmapData & cmd, const Bitmap & bmp)
    {
        if (verbose > 10) {
            cmd.log(LOG_INFO);
        }

        gd.draw(cmd, bmp);
    }

    template<class Cmd, class... Ts>
    void draw_impl(RDPDrawable & gd, int verbose, Cmd const & cmd, Rect clip, Ts const & ... args) {
        if (verbose > 10) {
            cmd.log(LOG_INFO, clip);
        }

        gd.draw(cmd, clip, args...);
    }

    template<class Cmd, class... Ts>
    void draw_impl(RDPDrawable & gd, int verbose, Cmd const & cmd) {
        if (verbose > 10) {
            cmd.log(LOG_INFO);
        }

        gd.draw(cmd);
    }
}

void FakeGraphic::draw(RDP::FrameMarker    const & cmd)
{
    draw_impl(this->gd, this->verbose, cmd);
}

void FakeGraphic::draw(RDPDestBlt          const & cmd, Rect clip)
{
    draw_impl(this->gd, this->verbose, cmd, clip);
}

void FakeGraphic::draw(RDPMultiDstBlt      const & cmd, Rect clip)
{
    draw_impl(this->gd, this->verbose, cmd, clip);
}

void FakeGraphic::draw(RDPPatBlt           const & cmd, Rect clip, gdi::ColorCtx color_ctx)
{
    draw_impl(this->gd, this->verbose, cmd, clip, color_ctx);
}

void FakeGraphic::draw(RDP::RDPMultiPatBlt const & cmd, Rect clip, gdi::ColorCtx color_ctx)
{
    draw_impl(this->gd, this->verbose, cmd, clip, color_ctx);
}

void FakeGraphic::draw(RDPOpaqueRect       const & cmd, Rect clip, gdi::ColorCtx color_ctx)
{
    draw_impl(this->gd, this->verbose, cmd, clip, color_ctx);
}

void FakeGraphic::draw(RDPMultiOpaqueRect  const & cmd, Rect clip, gdi::ColorCtx color_ctx)
{
    draw_impl(this->gd, this->verbose, cmd, clip, color_ctx);
}

void FakeGraphic::draw(RDPScrBlt           const & cmd, Rect clip)
{
    draw_impl(this->gd, this->verbose, cmd, clip);
}

void FakeGraphic::draw(RDP::RDPMultiScrBlt const & cmd, Rect clip)
{
    draw_impl(this->gd, this->verbose, cmd, clip);
}

void FakeGraphic::draw(RDPLineTo           const & cmd, Rect clip, gdi::ColorCtx color_ctx)
{
    draw_impl(this->gd, this->verbose, cmd, clip, color_ctx);
}

void FakeGraphic::draw(RDPPolygonSC        const & cmd, Rect clip, gdi::ColorCtx color_ctx)
{
    draw_impl(this->gd, this->verbose, cmd, clip, color_ctx);
}

void FakeGraphic::draw(RDPPolygonCB        const & cmd, Rect clip, gdi::ColorCtx color_ctx)
{
    draw_impl(this->gd, this->verbose, cmd, clip, color_ctx);
}

void FakeGraphic::draw(RDPPolyline         const & cmd, Rect clip, gdi::ColorCtx color_ctx)
{
    draw_impl(this->gd, this->verbose, cmd, clip, color_ctx);
}

void FakeGraphic::draw(RDPEllipseSC        const & cmd, Rect clip, gdi::ColorCtx color_ctx)
{
    draw_impl(this->gd, this->verbose, cmd, clip, color_ctx);
}

void FakeGraphic::draw(RDPEllipseCB        const & cmd, Rect clip, gdi::ColorCtx color_ctx)
{
    draw_impl(this->gd, this->verbose, cmd, clip, color_ctx);
}

void FakeGraphic::draw(RDPBitmapData       const & cmd, Bitmap const & bmp)
{
    draw_impl(this->gd, this->verbose, cmd, bmp);
}

void FakeGraphic::draw(RDPMemBlt           const & cmd, Rect clip, Bitmap const & bmp)
{
    draw_impl(this->gd, this->verbose, cmd, clip, bmp);
}

void FakeGraphic::draw(RDPMem3Blt          const & cmd, Rect clip, gdi::ColorCtx color_ctx, Bitmap const & bmp)
{
    draw_impl(this->gd, this->verbose, cmd, clip, color_ctx, bmp);
}

void FakeGraphic::draw(RDPGlyphIndex       const & cmd, Rect clip, gdi::ColorCtx color_ctx, GlyphCache const & gly_cache)
{
    draw_impl(this->gd, this->verbose, cmd, clip, color_ctx, gly_cache);
}

void FakeGraphic::draw(const RDP::RAIL::NewOrExistingWindow            & cmd)
{
    draw_impl(this->gd, this->verbose, cmd);
}
void FakeGraphic::draw(const RDP::RAIL::WindowIcon                     & cmd)
{
    draw_impl(this->gd, this->verbose, cmd);
}

void FakeGraphic::draw(const RDP::RAIL::CachedIcon                     & cmd)
{
    draw_impl(this->gd, this->verbose, cmd);
}

void FakeGraphic::draw(const RDP::RAIL::DeletedWindow                  & cmd)
{
    draw_impl(this->gd, this->verbose, cmd);
}

void FakeGraphic::draw(const RDP::RAIL::NewOrExistingNotificationIcons & cmd)
{
    draw_impl(this->gd, this->verbose, cmd);
}

void FakeGraphic::draw(const RDP::RAIL::DeletedNotificationIcons       & cmd)
{
    draw_impl(this->gd, this->verbose, cmd);
}

void FakeGraphic::draw(const RDP::RAIL::ActivelyMonitoredDesktop       & cmd)
{
    draw_impl(this->gd, this->verbose, cmd);
}

void FakeGraphic::draw(const RDP::RAIL::NonMonitoredDesktop            & cmd)
{
    draw_impl(this->gd, this->verbose, cmd);
}

void FakeGraphic::draw(RDPColCache   const & cmd)
{
    draw_impl(this->gd, this->verbose, cmd);
}

void FakeGraphic::draw(RDPBrushCache const & cmd)
{
    draw_impl(this->gd, this->verbose, cmd);
}

void FakeGraphic::set_palette(const BGRPalette &)
{
    if (this->verbose > 10) {
        LOG(LOG_INFO, "--------- FRONT ------------------------");
        LOG(LOG_INFO, "set_palette");
        LOG(LOG_INFO, "========================================\n");
    }
}

void FakeGraphic::set_pointer(const Pointer & cursor)
{
    if (this->verbose > 10) {
        LOG(LOG_INFO, "--------- FRONT ------------------------");
        LOG(LOG_INFO, "set_pointer");
        LOG(LOG_INFO, "========================================\n");
    }

    this->gd.set_pointer(cursor);
}

void FakeGraphic::sync()
{
    if (this->verbose > 10) {
            LOG(LOG_INFO, "--------- FRONT ------------------------");
            LOG(LOG_INFO, "sync()");
            LOG(LOG_INFO, "========================================\n");
    }
}

void FakeGraphic::begin_update()
{
    //if (this->verbose > 10) {
    //    LOG(LOG_INFO, "--------- FRONT ------------------------");
    //    LOG(LOG_INFO, "begin_update");
    //    LOG(LOG_INFO, "========================================\n");
    //}
}

void FakeGraphic::end_update()
{
    //if (this->verbose > 10) {
    //    LOG(LOG_INFO, "--------- FRONT ------------------------");
    //    LOG(LOG_INFO, "end_update");
    //    LOG(LOG_INFO, "========================================\n");
    //}
}

void FakeGraphic::dump_png(const char * prefix)
{
    char tmpname[128];
    sprintf(tmpname, "%sXXXXXX.png", prefix);
    int fd = ::mkostemps(tmpname, 4, O_WRONLY | O_CREAT);
    FILE * f = fdopen(fd, "wb");
    ::dump_png24(f, this->gd, true);
    ::fclose(f);
}

void FakeGraphic::save_to_png(const char * filename)
{
    std::FILE * file = fopen(filename, "w+");
    dump_png24(file, this->gd, true);
    fclose(file);
}

FakeGraphic::FakeGraphic(uint8_t bpp, size_t width, size_t height, uint32_t verbose)
: verbose(verbose)
, mod_bpp(bpp)
, mod_palette(BGRPalette::classic_332())
, gd(width, height)
{
    if (width % 4) {
       LOG(LOG_ERR, "FakeGraphic only support image width that are multiple of 4 (drawable limitation)");
    }
}

FakeGraphic::operator ConstImageDataView () const
{
    return this->gd;
}
