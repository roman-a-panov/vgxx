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

#ifndef VGXX_UTIL_HH
#define VGXX_UTIL_HH

#include <cassert>
#include <cmath>
#include <cstdint>
#include <type_traits>

#include <vgxx/fill_rule.hh>

namespace vgxx
{

class Util
{
  using Unt_32_ = ::std::uint32_t;

  template<class T>
  using Is_floating_point_ = ::std::is_floating_point<T>;

  template<bool, class = void>
  struct Enable_if_
  {};

  template<class T>
  struct Enable_if_<true, T>
  {
    using Type = T;
  };

  template<class T>
  static T&& ref_() noexcept;

public:
  using Unt_8 = ::std::uint8_t;
  using Int_32 = ::std::int32_t;

  template<class Float, bool e = Is_floating_point_<Float>::value>
  static auto to_fixed_24_dot_8(
    Float const& x) noexcept -> typename Enable_if_<e, Int_32>::Type
  {
    return to_fixed_<256u>(x);
  }

  template<class Float, bool e = Is_floating_point_<Float>::value>
  static auto to_fixed_26_dot_6(
    Float const& x) noexcept -> typename Enable_if_<e, Int_32>::Type
  {
    return to_fixed_<64u>(x);
  }

  [[nodiscard]] static Int_32 blend(
    Int_32 const src,
    Int_32 const dst,
    Int_32 const alpha) noexcept
  {
    // result = (src * alpha + dst * (255 - alpha)) / 255
    Int_32 val = (dst << 8u) - dst + alpha * (src - dst);
    return (val + 1 + (val >> 8u)) >> 8u; // val / 255
  }

  template<
    class Float,
    class Callback,
    bool e = Is_floating_point_<Float>::value>
  static auto subdivide_bezier(
    Callback&& callback,
    Float x_0,
    Float y_0,
    Float const& x_1,
    Float const& y_1,
    Float const& x_2,
    Float const& y_2,
    Float const& x_3,
    Float const& y_3)
      noexcept(noexcept(
        ref_<Callback>()(ref_<Float const&>(), ref_<Float const&>()))) ->
      typename Enable_if_<e>::Type
  {
    auto constexpr val_quarter = static_cast<Float>(0.25);
    auto constexpr val_one = static_cast<Float>(1);
    auto constexpr val_two = static_cast<Float>(2);
    auto constexpr val_three = static_cast<Float>(3);
    auto constexpr val_four = static_cast<Float>(4);
    auto constexpr val_six = static_cast<Float>(6);

    Float const dx_0 = ::std::fabs(x_1 - x_0);
    Float const dy_0 = ::std::fabs(y_1 - y_0);
    Float const dx_1 = ::std::fabs(x_2 - x_1);
    Float const dy_1 = ::std::fabs(y_2 - y_1);
    Float const dx_2 = ::std::fabs(x_3 - x_2);
    Float const dy_2 = ::std::fabs(y_3 - y_2);
    Float step_count_flt = ::std::ceil(
      (dx_0 + dy_0 + dx_1 + dy_1 + dx_2 + dy_2) * val_quarter);
    auto step_count = static_cast<Unt_32_>(step_count_flt);

    if(0u < step_count)
    {
      if(4u > step_count)
      {
        step_count = 4u;
        step_count_flt = val_four;
      }

      // B(t) = c_0 + c_1 * t + c_2 * t * t + c_3 * t * t * t
      // p_0, p_1, p_2, p_3 are control points.
      // c_0 = p_0
      // c_1 = 3 * (p_1 - p_0)
      // c_2 = 3 * p_0 - 6 * p_1 + 3 * p_2
      // c_3 = p_3 - 3 * p_2 + 3 * p_1 - p_0

      Float const x_0_x_3 = x_0 * val_three;
      Float const y_0_x_3 = y_0 * val_three;
      Float const x_1_x_3 = x_1 * val_three;
      Float const y_1_x_3 = y_1 * val_three;
      Float const x_2_x_3 = x_2 * val_three;
      Float const y_2_x_3 = y_2 * val_three;
      Float const c_1_x = x_1_x_3 - x_0_x_3;
      Float const c_1_y = y_1_x_3 - y_0_x_3;
      Float const c_2_x = x_0_x_3 + x_2_x_3 - x_1_x_3 * val_two;
      Float const c_2_y = y_0_x_3 + y_2_x_3 - y_1_x_3 * val_two;
      Float const c_3_x = x_3 - x_2_x_3 + x_1_x_3 - x_0;
      Float const c_3_y = y_3 - y_2_x_3 + y_1_x_3 - y_0;
      Float const d_t = val_one / step_count_flt;
      Float const d_t_sqr = d_t * d_t;
      Float const d_t_cub = d_t * d_t_sqr;
      Float d_x = c_3_x * d_t_cub + c_2_x * d_t_sqr + c_1_x * d_t;
      Float d_y = c_3_y * d_t_cub + c_2_y * d_t_sqr + c_1_y * d_t;
      Float d_d_x = c_2_x * d_t_sqr * val_two;
      Float d_d_y = c_2_y * d_t_sqr * val_two;
      Float const d_d_d_x = c_3_x * d_t_cub * val_six;
      Float const d_d_d_y = c_3_y * d_t_cub * val_six;
      Float& x = x_0;
      Float& y = y_0;

      for(;;)
      {
        x += d_x;
        y += d_y;
        static_cast<Callback&&>(callback)(
          static_cast<Float const&>(x), static_cast<Float const&>(y));

        if(0u < --step_count)
        {
          d_d_x += d_d_d_x;
          d_d_y += d_d_d_y;
          d_x += d_d_x;
          d_y += d_d_y;
        }
        else
        {
          break;
        }
      }
    }
  }

  template<Fill_rule fill_rule>
  [[nodiscard]] static Unt_8 compute_cell_coverage(
    Int_32 const cover,
    Int_32 const area) noexcept
  {
    static_assert(
      Fill_rule::non_zero == fill_rule ||
      Fill_rule::even_odd == fill_rule);

    Int_32 c = (cover << 9u) - area;
    if(0 > c)
    {
      c = -c;
    }

    if constexpr(Fill_rule::non_zero == fill_rule)
    {
      if(0x20000 < c)
      {
        c = 0x20000;
      }
    }
    else // (fill_rule == Fill_rule::eben_odd)
    {
      if(0 == ((c >> 17u) & 1))
      {
        // Even.
        c &= 0x1ffff;
      }
      else
      {
        // Odd.
        c = 0x20000 - (c & 0x1ffff);
      }
    }

    c >>= 9u;
    c = ((c << 8u) - c) >> 8u;  // c * 255 / 256
    return static_cast<Unt_8>(c);
  }

  [[nodiscard]] static Unt_8 compute_cell_coverage(
    Int_32 const cover,
    Int_32 const area,
    Fill_rule const fill_rule) noexcept
  {
    switch(fill_rule)
    {
    case Fill_rule::non_zero:
      return compute_cell_coverage<Fill_rule::non_zero>(cover, area);
    case Fill_rule::even_odd:
      return compute_cell_coverage<Fill_rule::even_odd>(cover, area);
    default:
      assert(false);
      return 0u;
    }
  }

private:
  template<
    Unt_32_ frac, class Float,
    bool e = Is_floating_point_<Float>::value>
  static auto to_fixed_(
    Float x) noexcept -> typename Enable_if_<e, Int_32>::Type
  {
    constexpr auto val_0 = static_cast<Float>(0);
    constexpr auto mul = static_cast<Float>(frac);
    constexpr auto val_half = static_cast<Float>(0.5);

    x *= mul;
    if(val_0 <= x)
    {
      x += val_half;
    }
    else
    {
      x -= val_half;
    }

    return static_cast<Int_32>(x);
  }
};

} // namespace vgxx

#endif // VGXX_UTIL_HH
