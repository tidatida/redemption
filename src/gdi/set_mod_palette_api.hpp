/*
*   This program is free software; you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation; either version 2 of the License, or
*   (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this program; if not, write to the Free Software
*   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*
*   Product name: redemption, a FLOSS RDP proxy
*   Copyright (C) Wallix 2010-2015
*   Author(s): Jonathan Poelen
*/

#ifndef REDEMPTION_GDI_SET_MOD_PALETTE_API_HPP
#define REDEMPTION_GDI_SET_MOD_PALETTE_API_HPP

#include "utils/virtual_deleter.hpp"

#include "noncopyable.hpp"

class Pointer;

namespace gdi {

struct SetModPaletteApi : private noncopyable
{
    virtual ~SetModPaletteApi() = default;

    virtual void set_mod_palette(const BGRPalette & palette) {};
};

using SetModPaletteApiDeleterBase = utils::virtual_deleter_base<SetModPaletteApi>;
using SetModPaletteApiPtr = utils::unique_ptr_with_virtual_deleter<SetModPaletteApi>;

using utils::default_delete;
using utils::no_delete;

template<class SetModPaletteApi, class... Args>
SetModPaletteApiPtr make_set_mod_pallete_ptr(Args && ... args) {
    return SetModPaletteApiPtr(new SetModPaletteApi(std::forward<Args>(args)...), default_delete);
}

template<class SetModPaletteApi>
SetModPaletteApiPtr make_set_mod_pallete_ref(SetModPaletteApi & gd) {
    return SetModPaletteApiPtr(&gd, no_delete);
}

}

#endif
