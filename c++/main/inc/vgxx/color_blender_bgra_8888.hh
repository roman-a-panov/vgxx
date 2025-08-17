/*
The MIT License (MIT) https://opensource.org/license/mit

Copyright (c) 2013-2024 Roman Panov roman.a.panov@gmail.com

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the “Software”), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef VGXX_COLORBLENDERBGRA8888_HH
#define VGXX_COLORBLENDERBGRA8888_HH

#include <cassert>
#include <cstdint>

#include <vgxx/blender_base.hh>
#include <vgxx/util.hh>

namespace vgxx
{

struct Color_blender_bgra_8888 : Blender_base<::std::uint32_t>
{
private:
  using Int_32_ = ::std::int32_t;
  using Base_ = Blender_base<Color>;

public:
  using Base_::Base_;

  [[nodiscard]] Color color() const noexcept
  {
    return color_;
  }

  void set_color(Color const c) noexcept
  {
    if(c != color_)
    {
      color_ = c;
      alpha_ = get_alpha_(c);
      red_ = get_red_(c);
      green_ = get_green_(c);
      blue_ = get_blue_(c);
    }
  }

  void set_r_g_b_a(Color const r_g_b_a) noexcept
  {
    Color const b_mask = (r_g_b_a >> 16u) & 0x000000ffu;
    Color const g_mask = r_g_b_a & 0x0000ff00u;
    Color const r_mask = (r_g_b_a << 16u) & 0x00ff0000u;
    Color const a_mask = r_g_b_a & 0xff000000u;
    set_color(b_mask | g_mask | r_mask | a_mask);
  }

  void blend(Color alpha) const noexcept
  {
    Color* const dst_color = pixel();
    assert(dst_color);

    if(0xffu > alpha || 0xffu > alpha_)
    {
      alpha *= alpha_;
      if(0u < alpha)
      {
        alpha = (alpha + 1u + (alpha >> 8u)) >> 8u; // alpha /= 255
        auto const src_a = static_cast<Int_32_>(alpha);
        auto const r = static_cast<Color>(Util::blend(
          red_,
          get_red_(*dst_color),
          src_a));
        auto const g = static_cast<Color>(Util::blend(
          green_,
          get_green_(*dst_color),
          src_a));
        auto const b = static_cast<Color>(Util::blend(
          blue_,
          get_blue_(*dst_color),
          src_a));

        *dst_color = Color{0xff000000u} | (r << 16u) | (g << 8u) | b;
      }
    }
    else
    {
      *dst_color = color_;
    }
  }

private:
  [[nodiscard]] static Color get_alpha_(Color const color) noexcept
  {
    return color >> 24u;
  }

  [[nodiscard]] static Int_32_ get_red_(Color const color) noexcept
  {
    return static_cast<Int_32_>((color >> 16u) & 0xffu);
  }

  [[nodiscard]] static Int_32_ get_green_(Color const color) noexcept
  {
    return static_cast<Int_32_>((color >> 8u) & 0xffu);
  }

  [[nodiscard]] static Int_32_ get_blue_(Color const color) noexcept
  {
    return static_cast<Int_32_>(color & 0xffu);
  }

  Color color_ = 0u;
  Color alpha_ = 0u;
  Int_32_ red_ = 0u;
  Int_32_ green_ = 0u;
  Int_32_ blue_ = 0u;
};

} // namespace vgxx

#endif // VGXX_COLORBLENDERBGRA8888_HH
