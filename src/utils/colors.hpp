/*
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

   Product name: redemption, a FLOSS RDP proxy
   Copyright (C) Wallix 2010
   Author(s): Christophe Grosjean, Javier Caverni, Dominique Lafages
   Based on xrdp Copyright (C) Jay Sorg 2004-2010

   Colors object. Contains generic colors
*/


#pragma once

#include <iterator>

#include <cstdint>
#include <cassert>
#include <cstdlib>
#include <cstddef>
#include <cstring> // memcpy

// Those are in BGR
enum NamedBGRColor {
    BLACK                     = 0x000000,
    GREY                      = 0xc0c0c0,
    MEDIUM_GREY               = 0xa0a0a0,
    DARK_GREY                 = 0x8c8a8c,
    ANTHRACITE                = 0x808080,
    WHITE                     = 0xffffff,

    BLUE                      = 0xff0000,
    DARK_BLUE                 = 0x7f0000,
    CYAN                      = 0xffff00,
    DARK_BLUE_WIN             = 0x602000,
    DARK_BLUE_BIS             = 0x601f08,
    MEDIUM_BLUE               = 0xC47244,
    PALE_BLUE                 = 0xf6ece9,
    LIGHT_BLUE                = 0xebd5cf,
    WINBLUE                   = 0x9C4D00,

    RED                       = 0x0000ff,
    DARK_RED                  = 0x221CAD,
    MEDIUM_RED                = 0x302DB7,
    PINK                      = 0xff00ff,

    GREEN                     = 0x00ff00,
    WABGREEN                  = 0x2BBE91,
    WABGREEN_BIS              = 0x08ff7b,
    DARK_WABGREEN             = 0x91BE2B,
    INV_DARK_WABGREEN         = 0x2BBE91,
    DARK_GREEN                = 0x499F74,
    INV_DARK_GREEN            = 0x749F49,
    LIGHT_GREEN               = 0x90ffe0,
    INV_LIGHT_GREEN           = 0x8EE537,
    PALE_GREEN                = 0xE1FAF0,
    INV_PALE_GREEN            = 0xF0FAE1,
    MEDIUM_GREEN              = 0xACE4C8,
    INV_MEDIUM_GREEN          = 0xC8E4AC,

    YELLOW                    = 0x00ffff,
    LIGHT_YELLOW              = 0x9fffff,

    ORANGE                    = 0x1580DD,
    LIGHT_ORANGE              = 0x64BFFF,
    PALE_ORANGE               = 0x9AD5FF,
    BROWN                     = 0x006AC5
};


struct BGRasRGBColor_;

struct BGRColor_
{
    constexpr BGRColor_(NamedBGRColor color) noexcept
    : color_(color)
    {}

    constexpr BGRColor_(BGRasRGBColor_ const & color) noexcept;

    constexpr explicit BGRColor_(uint32_t color = 0) noexcept
    : color_(color/* & 0xFFFFFF*/)
    {}

    constexpr explicit BGRColor_(uint8_t blue, uint8_t green, uint8_t red) noexcept
    : color_((blue << 16) | (green << 8) | red)
    {}

    constexpr uint32_t to_u32() const noexcept { return this->color_; }

    constexpr uint8_t red() const noexcept { return this->color_; }
    constexpr uint8_t green() const noexcept { return this->color_ >> 8; }
    constexpr uint8_t blue() const noexcept { return this->color_ >> 16; }

private:
    uint32_t color_;
};

struct BGRasRGBColor_
{
    constexpr BGRasRGBColor_(NamedBGRColor color) noexcept
    : color_(color)
    {}

    constexpr BGRasRGBColor_(BGRColor_ color) noexcept
    : color_(color)
    {}

    constexpr uint8_t red() const noexcept { return this->color_.blue(); }
    constexpr uint8_t green() const noexcept { return this->color_.green(); }
    constexpr uint8_t blue() const noexcept { return this->color_.red(); }

private:
    BGRColor_ color_;
};

constexpr BGRColor_::BGRColor_(BGRasRGBColor_ const & color) noexcept
: BGRColor_(color.blue(), color.green(), color.red())
{}

constexpr bool operator == (BGRColor_ const & lhs, BGRColor_ const & rhs) { return lhs.to_u32() == rhs.to_u32(); }
constexpr bool operator != (BGRColor_ const & lhs, BGRColor_ const & rhs) { return !(lhs == rhs); }

template<class Ch, class Tr>
std::basic_ostream<Ch, Tr> & operator<<(std::basic_ostream<Ch, Tr> & out, BGRColor_ const & c)
{
    char const * thex = "0123456789ABCDEF";
    char s[]{
        thex[(c.to_u32() >> 20) & 0xf],
        thex[(c.to_u32() >> 16) & 0xf],
        thex[(c.to_u32() >> 12) & 0xf],
        thex[(c.to_u32() >> 8) & 0xf],
        thex[(c.to_u32() >> 4) & 0xf],
        thex[(c.to_u32() >> 0) & 0xf],
        '\0'
    };
    return out << s;
}

constexpr uint32_t log_value(BGRColor_ const & c) noexcept { return c.to_u32(); }


struct RDPColor
{
    constexpr RDPColor() noexcept
    : color_(0)
    {}

    constexpr BGRColor_ as_bgr() const noexcept { return BGRColor_(this->color_); }
    constexpr BGRasRGBColor_ as_rgb() const noexcept { return BGRasRGBColor_(this->as_bgr()); }

    constexpr static RDPColor from(uint32_t c) noexcept
    { return RDPColor(nullptr, c); }

private:
    constexpr RDPColor(std::nullptr_t, uint32_t c) noexcept
    : color_(c)
    {}

    uint32_t color_;
};

constexpr bool operator == (RDPColor const & lhs, RDPColor const & rhs)
{ return lhs.as_bgr().to_u32() == rhs.as_bgr().to_u32(); }
constexpr bool operator != (RDPColor const & lhs, RDPColor const & rhs)
{ return !(lhs == rhs); }

template<class Ch, class Tr>
std::basic_ostream<Ch, Tr> & operator<<(std::basic_ostream<Ch, Tr> & out, RDPColor const & c)
{ return out << c.as_bgr(); }

constexpr uint32_t log_value(RDPColor const & c) noexcept { return c.as_bgr().to_u32(); }


class BGRPalette
{
public:
    BGRPalette() = delete;

    struct no_init {};
    explicit BGRPalette(no_init) noexcept
    : BGRPalette(nullptr)
    {}

    explicit BGRPalette(std::nullptr_t) noexcept
    : BGRPalette(std::integral_constant<std::size_t, 4>{}, 0, 0, 0, 0)
    {}

    template<class BGRValue>
    explicit BGRPalette(BGRValue const (&a)[256]) noexcept
    : BGRPalette(a, std::integral_constant<std::size_t, 4>{}, 0, 1, 2, 3)
    {}

    template<class... BGRValues, typename std::enable_if<sizeof...(BGRValues) == 256, int>::type = 1>
    explicit BGRPalette(BGRValues ... bgr_values) noexcept
    : palette{BGRColor_(bgr_values)...}
    {}

    static const BGRPalette & classic_332_rgb() noexcept
    {
        static const BGRPalette palette([](BGRColor_ c) { return BGRasRGBColor_(c); }, 1);
        return palette;
    }

    static const BGRPalette & classic_332() noexcept
    {
        static const BGRPalette palette([](BGRColor_ c) { return c; }, 1);
        return palette;
    }

    BGRColor_ operator[](std::size_t i) const noexcept
    { return this->palette[i]; }

    BGRColor_ const * begin() const
    { using std::begin; return begin(this->palette); }

    BGRColor_ const * end() const
    { using std::end; return end(this->palette); }

    void set_color(std::size_t i, BGRColor_ c) noexcept
    { this->palette[i] = c; }

    const char * data() const noexcept
    { return reinterpret_cast<char const*>(this->palette); }

    static constexpr std::size_t data_size() noexcept
    { return sizeof(palette); }

private:
    BGRColor_ palette[256];

    #ifndef IN_IDE_PARSER
    template<std::size_t n, class... Ts>
    explicit BGRPalette(std::integral_constant<std::size_t, n>, Ts... ints) noexcept
    : BGRPalette(std::integral_constant<std::size_t, n*4>{}, ints..., ints..., ints..., ints...)
    {}
    #endif

    template<class... Ts>
    explicit BGRPalette(std::integral_constant<std::size_t, 256>, Ts... ints) noexcept
    : palette{BGRColor_(ints)...}
    {}

    #ifndef IN_IDE_PARSER
    template<class BGRValue, std::size_t n, class... Ts>
    explicit BGRPalette(BGRValue const (&a)[256], std::integral_constant<std::size_t, n>, Ts... ints) noexcept
    : BGRPalette(a, std::integral_constant<std::size_t, n*4>{},
        ints...,
        (ints + sizeof...(ints))...,
        (ints + sizeof...(ints) * 2)...,
        (ints + sizeof...(ints) * 3)...
    )
    {}
    #endif

    template<class BGRValue, class... Ts>
    explicit BGRPalette(BGRValue const (&a)[256], std::integral_constant<std::size_t, 256>, Ts... ints) noexcept
    : palette{BGRColor_(a[ints])...}
    {}

    template<class Transform>
    /*constexpr*/ BGRPalette(Transform trans, int) noexcept
    {
        /* rgb332 palette */
        for (int bindex = 0; bindex < 4; bindex++) {
            for (int gindex = 0; gindex < 8; gindex++) {
                for (int rindex = 0; rindex < 8; rindex++) {
                    this->palette[(rindex << 5) | (gindex << 2) | bindex] =
                    trans(BGRColor_(
                        // r1 r2 r2 r1 r2 r3 r1 r2 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
                        (((rindex<<5)|(rindex<<2)|(rindex>>1))<<16)
                        // 0 0 0 0 0 0 0 0 g1 g2 g3 g1 g2 g3 g1 g2 0 0 0 0 0 0 0 0
                      | (((gindex<<5)|(gindex<<2)|(gindex>>1))<< 8)
                        // 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 b1 b2 b1 b2 b1 b2 b1 b2
                      | ((bindex<<6)|(bindex<<4)|(bindex<<2)|(bindex))
                    ));
                }
            }
        }
    }
};

template<class... BGRValues>
BGRPalette make_rgb_palette(BGRValues... bgr_values) noexcept
{
    static_assert(sizeof...(bgr_values) == 256, "");
    return BGRPalette(BGRasRGBColor_(BGRColor_(bgr_values))...);
}

inline BGRPalette make_bgr_palette_from_bgrx_array(uint8_t const (&a)[256 * 4]) noexcept
{
    uint32_t bgr_array[256];
    for (int i = 0; i < 256; ++i) {
        bgr_array[i] = (a[i * 4 + 2] << 16) | (a[i * 4 + 1] << 8) | (a[i * 4 + 0] << 0);
    }
    return BGRPalette(bgr_array);
}


inline BGRColor_ color_from_cstr(const char * str)
{
    BGRColor_ bgr;

    if (0 == strcasecmp("BLACK", str))                  { bgr = BLACK; }
    else if (0 == strcasecmp("GREY", str))              { bgr = GREY; }
    else if (0 == strcasecmp("DARK_GREY", str))         { bgr = DARK_GREY; }
    else if (0 == strcasecmp("ANTHRACITE", str))        { bgr = ANTHRACITE; }
    else if (0 == strcasecmp("BLUE", str))              { bgr = BLUE; }
    else if (0 == strcasecmp("DARK_BLUE", str))         { bgr = DARK_BLUE; }
    else if (0 == strcasecmp("WHITE", str))             { bgr = WHITE; }
    else if (0 == strcasecmp("RED", str))               { bgr = RED; }
    else if (0 == strcasecmp("PINK", str))              { bgr = PINK; }
    else if (0 == strcasecmp("GREEN", str))             { bgr = GREEN; }
    else if (0 == strcasecmp("YELLOW", str))            { bgr = YELLOW; }
    else if (0 == strcasecmp("LIGHT_YELLOW", str))      { bgr = LIGHT_YELLOW; }
    else if (0 == strcasecmp("CYAN", str))              { bgr = CYAN; }
    else if (0 == strcasecmp("WABGREEN", str))          { bgr = WABGREEN; }
    else if (0 == strcasecmp("WABGREEN_BIS", str))      { bgr = WABGREEN_BIS; }
    else if (0 == strcasecmp("DARK_WABGREEN", str))     { bgr = DARK_WABGREEN; }
    else if (0 == strcasecmp("INV_DARK_WABGREEN", str)) { bgr = INV_DARK_WABGREEN; }
    else if (0 == strcasecmp("DARK_GREEN", str))        { bgr = DARK_GREEN; }
    else if (0 == strcasecmp("INV_DARK_GREEN", str))    { bgr = INV_DARK_GREEN; }
    else if (0 == strcasecmp("LIGHT_GREEN", str))       { bgr = LIGHT_GREEN; }
    else if (0 == strcasecmp("INV_LIGHT_GREEN", str))   { bgr = INV_LIGHT_GREEN; }
    else if (0 == strcasecmp("PALE_GREEN", str))        { bgr = PALE_GREEN; }
    else if (0 == strcasecmp("INV_PALE_GREEN", str))    { bgr = INV_PALE_GREEN; }
    else if (0 == strcasecmp("MEDIUM_GREEN", str))      { bgr = MEDIUM_GREEN; }
    else if (0 == strcasecmp("INV_MEDIUM_GREEN", str))  { bgr = INV_MEDIUM_GREEN; }
    else if (0 == strcasecmp("DARK_BLUE_WIN", str))     { bgr = DARK_BLUE_WIN; }
    else if (0 == strcasecmp("DARK_BLUE_BIS", str))     { bgr = DARK_BLUE_BIS; }
    else if (0 == strcasecmp("MEDIUM_BLUE", str))       { bgr = MEDIUM_BLUE; }
    else if (0 == strcasecmp("PALE_BLUE", str))         { bgr = PALE_BLUE; }
    else if (0 == strcasecmp("LIGHT_BLUE", str))        { bgr = LIGHT_BLUE; }
    else if (0 == strcasecmp("WINBLUE", str))           { bgr = WINBLUE; }
    else if (0 == strcasecmp("ORANGE", str))            { bgr = ORANGE; }
    else if (0 == strcasecmp("DARK_RED", str))          { bgr = DARK_RED; }
    else if (0 == strcasecmp("BROWN", str))             { bgr = BROWN; }
    else if (0 == strcasecmp("LIGHT_ORANGE", str))      { bgr = LIGHT_ORANGE; }
    else if (0 == strcasecmp("PALE_ORANGE", str))       { bgr = PALE_ORANGE; }
    else if (0 == strcasecmp("MEDIUM_RED", str))        { bgr = MEDIUM_RED; }
    else if ((*str == '0') && (*(str + 1) == 'x')){
        bgr = BGRasRGBColor_(BGRColor_(strtol(str + 2, nullptr, 16)));
    }
    else { bgr = BGRasRGBColor_(BGRColor_(atol(str))); }

    return bgr;
}


struct decode_color8
{
    static constexpr const uint8_t bpp = 8;

    constexpr decode_color8() noexcept {}

    BGRColor_ operator()(RDPColor c, BGRPalette const & palette) const noexcept
    {
        // assert(c.as_bgr().to_u32() <= 255);
        return BGRColor_(palette[static_cast<uint8_t>(c.as_bgr().to_u32())]);
    }
};

struct decode_color15
{
    static constexpr const uint8_t bpp = 15;

    constexpr decode_color15() noexcept {}

    constexpr BGRColor_ operator()(RDPColor c) const noexcept
    {
        // b1 b2 b3 b4 b5 g1 g2 g3 g4 g5 r1 r2 r3 r4 r5
        return BGRColor_(
            ((c.as_bgr().to_u32() << 3) & 0xf8) | ((c.as_bgr().to_u32() >>  2) & 0x7), // r1 r2 r3 r4 r5 r1 r2 r3
            ((c.as_bgr().to_u32() >> 2) & 0xf8) | ((c.as_bgr().to_u32() >>  7) & 0x7), // g1 g2 g3 g4 g5 g1 g2 g3
            ((c.as_bgr().to_u32() >> 7) & 0xf8) | ((c.as_bgr().to_u32() >> 12) & 0x7)  // b1 b2 b3 b4 b5 b1 b2 b3
        );
    }
};

struct decode_color16
{
    static constexpr const uint8_t bpp = 16;

    constexpr decode_color16() noexcept {}

    constexpr BGRColor_ operator()(RDPColor c) const noexcept
    {
        // b1 b2 b3 b4 b5 g1 g2 g3 g4 g5 g6 r1 r2 r3 r4 r5
        return BGRColor_(
            ((c.as_bgr().to_u32() << 3) & 0xf8) | ((c.as_bgr().to_u32() >>  2) & 0x7), // r1 r2 r3 r4 r5 r6 r7 r8
            ((c.as_bgr().to_u32() >> 3) & 0xfc) | ((c.as_bgr().to_u32() >>  9) & 0x3), // g1 g2 g3 g4 g5 g6 g1 g2
            ((c.as_bgr().to_u32() >> 8) & 0xf8) | ((c.as_bgr().to_u32() >> 13) & 0x7)  // b1 b2 b3 b4 b5 b1 b2 b3
        );
    }
};

struct decode_color24
{
    static constexpr const uint8_t bpp = 24;

    constexpr decode_color24() noexcept {}

    constexpr BGRColor_ operator()(RDPColor c) const noexcept
    {
        return c.as_bgr();
    }
};


// colorN (variable): an index into the current palette or an RGB triplet
//                    value; the actual interpretation depends on the color
//                    depth of the bitmap data.
// +-------------+------------+------------------------------------------------+
// | Color depth | Field size |                Meaning                         |
// +-------------+------------+------------------------------------------------+
// |       8 bpp |     1 byte |     Index into the current color palette.      |
// +-------------+------------+------------------------------------------------+
// |      15 bpp |    2 bytes | RGB color triplet expressed in 5-5-5 format    |
// |             |            | (5 bits for red, 5 bits for green, and 5 bits  |
// |             |            | for blue).                                     |
// +-------------+------------+------------------------------------------------+
// |      16 bpp |    2 bytes | RGB color triplet expressed in 5-6-5 format    |
// |             |            | (5 bits for red, 6 bits for green, and 5 bits  |
// |             |            | for blue).                                     |
// +-------------+------------+------------------------------------------------+
// |    24 bpp   |    3 bytes |     RGB color triplet (1 byte per component).  |
// +-------------+------------+------------------------------------------------+

inline BGRColor_ color_decode(const RDPColor c, const uint8_t in_bpp, const BGRPalette & palette) noexcept
{
    switch (in_bpp){
        case 8:  return decode_color8()(c, palette);
        case 15: return decode_color15()(c);
        case 16: return decode_color16()(c);
        case 24:
        case 32: return decode_color24()(c);
        default: assert(!"unknown bpp");
    }
    return BGRColor_{0};
}

template<class Converter>
struct with_color8_palette
{
    static constexpr const uint8_t bpp = Converter::bpp;

    BGRColor_ operator()(RDPColor c) const noexcept
    {
        return Converter()(c, this->palette);
    }

    BGRPalette const & palette;
};
using decode_color8_with_palette = with_color8_palette<decode_color8>;


struct encode_color8
{
    static constexpr const uint8_t bpp = 8;

    constexpr encode_color8() noexcept {}

    constexpr RDPColor operator()(BGRasRGBColor_ c) const noexcept {
        // bbbgggrr
        return RDPColor::from(
            ((BGRColor_(c).to_u32() >> 16) & 0xE0)
          | ((BGRColor_(c).to_u32() >> 11) & 0x1C)
          | ((BGRColor_(c).to_u32() >> 6 ) & 0x03)
        );
    }
};

struct encode_color15
{
    static constexpr const uint8_t bpp = 15;

    constexpr encode_color15() noexcept {}

    // rgb555
    RDPColor operator()(BGRasRGBColor_ c) const noexcept
    {
        // 0 b1 b2 b3 b4 b5 g1 g2 g3 g4 g5 r1 r2 r3 r4 r5
        return RDPColor::from(
            // r1 r2 r3 r4 r5 r6 r7 r8 --> 0 0 0 0 0 0 0 0 0 0 0 r1 r2 r3 r4 r5
            ((c.red())  >> 3)
            // g1 g2 g3 g4 g5 g6 g7 g8 --> 0 0 0 0 0 0 g1 g2 g3 g4 g5 0 0 0 0 0
          | ((c.green() << 2) & 0x03E0)
            // b1 b2 b3 b4 b5 b6 b7 b8 --> 0 b1 b2 b3 b4 b5 0 0 0 0 0 0 0 0 0 0
          | ((c.blue()  << 7) & 0x7C00)
        );
    }
};

struct encode_color16
{
    static constexpr const uint8_t bpp = 16;

    constexpr encode_color16() noexcept {}

    // rgb565
    RDPColor operator()(BGRasRGBColor_ c) const noexcept
    {
        // b1 b2 b3 b4 b5 g1 g2 g3 g4 g5 g6 r1 r2 r3 r4 r5
        return RDPColor::from(
            // r1 r2 r3 r4 r5 r6 r7 r8 --> 0 0 0 0 0 0 0 0 0 0 0 r1 r2 r3 r4 r5
            ((c.red())  >> 3)
            // g1 g2 g3 g4 g5 g6 g7 g8 --> 0 0 0 0 0 g1 g2 g3 g4 g5 g6 0 0 0 0 0
          | ((c.green() << 3) & 0x07E0)
            // b1 b2 b3 b4 b5 b6 b7 b8 --> b1 b2 b3 b4 b5 0 0 0 0 0 0 0 0 0 0 0
          | ((c.blue()  << 8) & 0xF800)
        );
    }
};

struct encode_color24
{
    static constexpr const uint8_t bpp = 24;

    constexpr encode_color24() noexcept {}

    RDPColor operator()(BGRColor_ c) const noexcept
    {
        return RDPColor::from(c.to_u32());
    }
};

inline RDPColor color_encode(const BGRColor_ c, const uint8_t out_bpp) noexcept
{
    switch (out_bpp){
        case 8:  return encode_color8()(c);
        case 15: return encode_color15()(c);
        case 16: return encode_color16()(c);
        case 32:
        case 24: return encode_color24()(c);
        default:
            assert(!"unknown bpp");
        break;
    }
    return RDPColor{};
}


namespace shortcut_encode
{
    using enc8 = encode_color8;
    using enc15 = encode_color15;
    using enc16 = encode_color16;
    using enc24 = encode_color24;
}

namespace shortcut_decode_with_palette
{
    using dec8 = decode_color8_with_palette;
    using dec15 = decode_color15;
    using dec16 = decode_color16;
    using dec24 = decode_color24;
}
