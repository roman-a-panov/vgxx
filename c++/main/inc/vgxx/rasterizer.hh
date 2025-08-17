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

#ifndef VGXX_RASTERIZER_HH
#define VGXX_RASTERIZER_HH

#include <cstdint>
#include <type_traits>

#include <vgxx/util.hh>

namespace vgxx
{

class Rasterizer
{
  using Unt_32_ = ::std::uint32_t;
  using Unt_64_ = ::std::uint64_t;

  template<class T>
  using Is_floating_point_ = typename ::std::is_floating_point<T>::type;

  template<bool, class = void>
  struct Enable_if_
  {};

  template<class T>
  struct Enable_if_<true, T>
  {
    using Type = T;
  };

public:
  using Int_32 = ::std::int32_t;

  template<
    class Float,
    class Cell_processor,
    bool e = Is_floating_point_<Float>::value>
  auto move_to(
    Cell_processor&& cell_proc,
    Float const& x,
    Float const& y) -> typename Enable_if_<e>::Type
  {
    move_to_fixed_24_dot_8(
      static_cast<Cell_processor&&>(cell_proc),
      Util::to_fixed_24_dot_8(x),
      Util::to_fixed_24_dot_8(y));
  }

  template<
    class Float,
    class Cell_processor,
    bool e = Is_floating_point_<Float>::value>
  auto line_to(
    Cell_processor&& cell_proc,
    Float const& x,
    Float const& y) -> typename Enable_if_<e>::Type
  {
    line_to_fixed_24_dot_8(
      static_cast<Cell_processor&&>(cell_proc),
      Util::to_fixed_24_dot_8(x),
      Util::to_fixed_24_dot_8(y));
  }

  template<class Cell_processor>
  void move_to_fixed_24_dot_8(
    Cell_processor&& cell_proc,
    Int_32 const x,
    Int_32 const y)
  {
    // Close the previous contour.
    add_line_(static_cast<Cell_processor&&>(cell_proc), x_, y_, x_0_, y_0_);
    x_0_ = x;
    y_0_ = y;
    x_ = x;
    y_ = y;
  }

  template<class Cell_processor>
  void line_to_fixed_24_dot_8(
    Cell_processor&& cell_proc,
    Int_32 const x,
    Int_32 const y)
  {
    add_line_(static_cast<Cell_processor&&>(cell_proc), x_, y_, x, y);
    x_ = x;
    y_ = y;
  }

  template<typename Cell_processor>
  void close(Cell_processor&& cell_proc)
  {
    line_to_fixed_24_dot_8(
      static_cast<Cell_processor&&>(cell_proc),
      x_0_, y_0_);
  }

  void reset() noexcept
  {
    x_0_ = 0;
    y_0_ = 0;
    x_ = 0;
    y_ = 0;
  }

private:
  enum class Direction_
  {
    positive,
    negative
  };

  template<class Cell_processor>
  void add_line_(
    Cell_processor&& cell_proc,
    Int_32 const x_0,
    Int_32 const y_0,
    Int_32 const x_1,
    Int_32 const y_1)
  {
    if(y_0 == y_1)
    {
      return;
    }

    if(x_0 == x_1)
    {
      // Vertical line.
      Int_32 const int_x = x_0 >> 8u;
      Int_32 const frac_x = x_0 & 0xff;
      Int_32 int_y_0 = y_0 >> 8u;
      Int_32 int_y_1 = y_1 >> 8u;
      Int_32 const frac_y_0 = y_0 & 0xff;
      Int_32 const frac_y_1 = y_1 & 0xff;
      Int_32 cover;
      Int_32 area;

      if(int_y_0 == int_y_1)
      {
        cover = frac_y_1 - frac_y_0;
        area = (cover * frac_x) << 1;
        static_cast<Cell_processor&&>(cell_proc).set_x(int_x);
        static_cast<Cell_processor&&>(cell_proc).set_y(int_y_0);
        static_cast<Cell_processor&&>(cell_proc).set_cell(cover, area);
        return;
      }

      if(y_0 < y_1)
      {
        if(frac_y_0)
        {
          cover = 0x100 - frac_y_0;
          area = (cover * frac_x) << 1u;
          static_cast<Cell_processor&&>(cell_proc).set_x(int_x);
          static_cast<Cell_processor&&>(cell_proc).set_y(int_y_0);
          static_cast<Cell_processor&&>(cell_proc).set_cell(cover, area);
          ++int_y_0;
        }

        if(frac_y_1)
        {
          cover = frac_y_1;
          area = (cover * frac_x) << 1u;
          static_cast<Cell_processor&&>(cell_proc).set_x(int_x);
          static_cast<Cell_processor&&>(cell_proc).set_y(int_y_1);
          static_cast<Cell_processor&&>(cell_proc).set_cell(cover, area);
        }

        cover = 0x100;
        area = frac_x << 9u;  // (cover * fracX) * 2
      }
      else
      {
        if(frac_y_0)
        {
          cover = -frac_y_0;
          area = (cover * frac_x) << 1u;
          static_cast<Cell_processor&&>(cell_proc).set_x(int_x);
          static_cast<Cell_processor&&>(cell_proc).set_y(int_y_0);
          static_cast<Cell_processor&&>(cell_proc).set_cell(cover, area);
        }

        if(frac_y_1)
        {
          cover = frac_y_1 - 0x100;
          area = (cover * frac_x) << 1u;
          static_cast<Cell_processor&&>(cell_proc).set_x(int_x);
          static_cast<Cell_processor&&>(cell_proc).set_y(int_y_1);
          static_cast<Cell_processor&&>(cell_proc).set_cell(cover, area);
          ++int_y_1;
        }

        Int_32 const tmp = int_y_0;
        int_y_0 = int_y_1;
        int_y_1 = tmp;

        cover = -0x100;
        area = -(frac_x << 9u);  // (cover * fracX) * 2
      }

      while(int_y_0 < int_y_1)
      {
        static_cast<Cell_processor&&>(cell_proc).set_x(int_x);
        static_cast<Cell_processor&&>(cell_proc).set_y(int_y_0);
        static_cast<Cell_processor&&>(cell_proc).set_cell(cover, area);
        ++int_y_0;
      }

      return;
    }

    if(x_1 > x_0)
    {
      if(y_1 > y_0)
      {
        add_line_<
          Direction_::positive,
          Direction_::positive,
          Direction_::positive>(
          static_cast<Cell_processor&&>(cell_proc),
          x_0, y_0, x_1, y_1);
      }
      else
      {
        add_line_<
          Direction_::positive,
          Direction_::negative,
          Direction_::negative>(
          static_cast<Cell_processor&&>(cell_proc),
          x_1, y_1, x_0, y_0);
      }
    }
    else
    {
      if(y_1 > y_0)
      {
        add_line_<
          Direction_::negative,
          Direction_::positive,
          Direction_::negative>(
          static_cast<Cell_processor&&>(cell_proc),
          x_0, y_0, x_1, y_1);
      }
      else
      {
        add_line_<
          Direction_::negative,
          Direction_::negative,
          Direction_::positive>(
          static_cast<Cell_processor&&>(cell_proc),
          x_1, y_1, x_0, y_0);
      }
    }
  }

  template<
    Direction_ x_direction,
    Direction_ y_direction,
    Direction_ x_y_direction,
    class Cell_processor>
  void add_line_(
    Cell_processor&& cell_proc,
    Int_32 const x_0,
    Int_32 const y_0,
    Int_32 const x_1,
    Int_32 const y_1)
  {
    Int_32 int_x_0 = x_0 >> 8u;
    Int_32 int_x_1 = x_1 >> 8u;
    Int_32 int_y_0 = y_0 >> 8u;
    Int_32 int_y_1 = y_1 >> 8u;
    Int_32 frac_x_0 = x_0 & 0xff;
    Int_32 frac_x_1 = x_1 & 0xff;

    if(int_y_0 == int_y_1)
    {
      // Only one scanline is involved.

      if constexpr(Direction_::positive == x_y_direction)
      {
        add_scanline_<y_direction>(
          static_cast<Cell_processor&&>(cell_proc),
          int_y_0, int_x_0, int_x_1, frac_x_0, frac_x_1, x_1 - x_0, y_1 - y_0);
      }
      else
      {
        add_scanline_<y_direction>(
          static_cast<Cell_processor&&>(cell_proc),
          int_y_0, int_x_1, int_x_0, frac_x_1, frac_x_0, x_0 - x_1, y_1 - y_0);
      }

      return;
    }

    Unt_32_ d_x;
    auto d_y = static_cast<Unt_32_>(y_1 - y_0);
    Int_32 frac_y_0 = y_0 & 0xff;
    Int_32 frac_y_1 = y_1 & 0xff;

    if constexpr(Direction_::positive == x_y_direction)
    {
      d_x = static_cast<Unt_32_>(x_1 - x_0);
    }
    else
    {
      d_x = static_cast<Unt_32_>(x_0 - x_1);
    }

    Int_32 int_y = int_y_0;
    Int_32 x = x_0;
    Int_32 int_x, frac_x;
    Int_32 delta_x;
    Unt_32_ rem;

    if(frac_y_0)
    {
      Int_32 const delta_y = 0x100 - frac_y_0;

      if(0x1000000u > d_x)
      {
        Unt_32_ const p = d_x * static_cast<Unt_32_>(delta_y);
        delta_x = static_cast<Int_32>(p / d_y);
        rem = p % d_y;
      }
      else
      {
        Unt_64_ const p =
          static_cast<Unt_64_>(d_x) * static_cast<Unt_32_>(delta_y);
        delta_x = static_cast<Int_32>(p / d_y);
        rem = static_cast<Unt_32_>(p % d_y);
      }

      if constexpr(Direction_::negative == x_direction)
      {
        if(rem)
        {
          ++delta_x;
          rem = d_y - rem;
        }
      }

      if constexpr(Direction_::positive == x_y_direction)
      {
        x += delta_x;
      }
      else
      {
        x -= delta_x;
      }

      int_x = x >> 8u;
      frac_x = x & 0xff;

      if constexpr(Direction_::positive == x_y_direction)
      {
        add_scanline_<y_direction>(
          static_cast<Cell_processor&&>(cell_proc),
          int_y_0, int_x_0, int_x, frac_x_0, frac_x, x - x_0, delta_y);
      }
      else
      {
        add_scanline_<y_direction>(
          static_cast<Cell_processor&&>(cell_proc),
          int_y_0, int_x, int_x_0, frac_x, frac_x_0, x_0 - x, delta_y);
      }

      ++int_y;
    }
    else
    {
      int_x = int_x_0;
      frac_x = frac_x_0;
      rem = 0u;
    }

    if(int_y < int_y_1)
    {
      Unt_32_ mod;
      Int_32 inc_x;
      Int_32 annex;

      if(0x1000000u > d_x)
      {
        Unt_32_ const p = d_x << 8u;
        inc_x = static_cast<Int_32>(p / d_y);
        mod = p % d_y;
      }
      else
      {
        Unt_64_ const p = static_cast<Unt_64_>(d_x) << 8u;
        inc_x = static_cast<Int_32>(p / d_y);
        mod = p % d_y;
      }

      if constexpr(Direction_::positive == x_direction)
      {
        annex = 1;
      }
      else
      {
        if(mod)
        {
          ++inc_x;
          mod = d_y - mod;
        }

        annex = -1;
      }

      Int_32 cover, area;

      do
      {
        delta_x = inc_x;
        rem += mod;

        if(rem >= d_y)
        {
          delta_x += annex;
          rem -= d_y;
        }

        static_cast<Cell_processor&&>(cell_proc).set_y(int_y);
        Int_32 next_x, int_next_x, frac_next_x;
        Int_32 left_x, right_x;
        Int_32 int_left_x, int_right_x;
        Int_32 frac_left_x, frac_right_x;

        if constexpr(Direction_::positive == x_y_direction)
        {
          next_x = x + delta_x;
          int_next_x = next_x >> 8u;
          frac_next_x = next_x & 0xff;
          left_x = x;
          right_x = next_x;
          int_left_x = int_x;
          int_right_x = int_next_x;
          frac_left_x = frac_x;
          frac_right_x = frac_next_x;
        }
        else
        {
          next_x = x - delta_x;
          int_next_x = next_x >> 8u;
          frac_next_x = next_x & 0xff;
          left_x = next_x;
          right_x = x;
          int_left_x = int_next_x;
          int_right_x = int_x;
          frac_left_x = frac_next_x;
          frac_right_x = frac_x;
        }

        if(int_left_x == int_right_x)
        {
          // Inside one cell.

          if constexpr(Direction_::positive == y_direction)
          {
            cover = 0x100;
            area = (frac_left_x + frac_right_x) << 8u;
          }
          else
          {
            cover = -0x100;
            area = -((frac_left_x + frac_right_x) << 8u);
          }

          static_cast<Cell_processor&&>(cell_proc).set_x(int_left_x);
          static_cast<Cell_processor&&>(cell_proc).set_cell(cover, area);
        }
        else
        {
          int_x = int_left_x;
          Int_32 y = 0;
          Int_32 delta_y;
          Int_32 rem_1;
          Int_32 cell_x = 0;
          bool cell_x_valid;

          if(frac_left_x)
          {
            Int_32 const p_1 = (0x100 - frac_left_x) << 8u;
            delta_y = p_1 / delta_x;
            rem_1 = p_1 % delta_x;

            if constexpr(Direction_::positive == y_direction)
            {
              cover = delta_y;
            }
            else
            {
              if(rem_1)
              {
                ++delta_y;
                rem_1 = delta_x - rem_1;
              }

              cover = -delta_y;
            }

            area = cover * (frac_left_x + 0x100);
            cell_x = int_left_x;
            static_cast<Cell_processor&&>(cell_proc).set_x(cell_x);
            static_cast<Cell_processor&&>(cell_proc).set_cell(cover, area);
            ++int_x;
            y += delta_y;
            cell_x_valid = true;
          }
          else
          {
            rem_1 = 0;
            cell_x_valid = false;
          }

          if(int_x < int_right_x)
          {
            Int_32 inc_y = 0x10000 / delta_x;
            Int_32 mod_1 = 0x10000 % delta_x;
            Int_32 annex_1;

            if constexpr(Direction_::positive == y_direction)
            {
              annex_1 = 1;
            }
            else
            {
              if(mod_1)
              {
                ++inc_y;
                mod_1 = delta_x - mod_1;
              }

              annex_1 = -1;
            }

            do
            {
              delta_y = inc_y;
              rem_1 += mod_1;

              if(rem_1 >= delta_x)
              {
                delta_y += annex_1;
                rem_1 -= delta_x;
              }

              if constexpr(Direction_::positive == y_direction)
              {
                cover = delta_y;
              }
              else
              {
                cover = -delta_y;
              }

              area = cover << 8u;

              if(cell_x_valid)
              {
                ++cell_x;
                static_cast<Cell_processor&&>(cell_proc).inc_x();
              }
              else
              {
                cell_x = int_x;
                cell_x_valid = true;
                static_cast<Cell_processor&&>(cell_proc).set_x(cell_x);
              }

              static_cast<Cell_processor&&>(cell_proc).set_cell(cover, area);
              y += delta_y;
              ++int_x;
            }
            while(int_x < int_right_x);
          }

          if(frac_right_x)
          {
            delta_y = 0x100 - y;

            if(delta_y)
            {
              if constexpr(Direction_::positive == y_direction)
              {
                cover = delta_y;
              }
              else
              {
                cover = -delta_y;
              }

              area = cover * frac_right_x;
              ++cell_x;
              static_cast<Cell_processor&&>(cell_proc).inc_x();
              static_cast<Cell_processor&&>(cell_proc).set_cell(cover, area);
            }
          }
        }

        x = next_x;
        int_x = int_next_x;
        frac_x = frac_next_x;
        ++int_y;
      }
      while(int_y < int_y_1);
    }

    if(frac_y_1)
    {
      if constexpr(Direction_::positive == x_y_direction)
      {
        add_scanline_<y_direction>(
          static_cast<Cell_processor&&>(cell_proc),
          int_y_1, int_x, int_x_1, frac_x, frac_x_1, x_1 - x, frac_y_1);
      }
      else
      {
        add_scanline_<y_direction>(
          static_cast<Cell_processor&&>(cell_proc),
          int_y_1, int_x_1, int_x, frac_x_1, frac_x, x - x_1, frac_y_1);
      }
    }
  }

  template<Direction_ y_direction, class Cell_processor>
  void add_scanline_(
    Cell_processor&& cell_proc,
    Int_32 const int_y,
    Int_32 const int_x_0,
    Int_32 const int_x_1,
    Int_32 const frac_x_0,
    Int_32 const frac_x_1,
    Int_32 const d_x,
    Int_32 const d_y)
  {
    static_cast<Cell_processor&&>(cell_proc).set_y(int_y);

    if(int_x_0 == int_x_1)
    {
      // Inside one cell.
      Int_32 cover;

      if constexpr(Direction_::positive == y_direction)
      {
        cover = d_y;
      }
      else
      {
        cover = -d_y;
      }

      Int_32 const area = cover * (frac_x_0 + frac_x_1);
      static_cast<Cell_processor&&>(cell_proc).set_x(int_x_0);
      static_cast<Cell_processor&&>(cell_proc).set_cell(cover, area);
      return;
    }

    Int_32 int_x = int_x_0;
    Int_32 y = 0;
    Int_32 delta_y;
    Int_32 cover, area;
    Int_32 p, rem;
    Int_32 cell_x = 0;
    bool cell_x_valid;

    if(frac_x_0)
    {
      p = (0x100 - frac_x_0) * d_y;
      delta_y = p / d_x;
      rem = p % d_x;

      if constexpr(Direction_::positive == y_direction)
      {
        cover = delta_y;
      }
      else
      {
        if(rem)
        {
          ++delta_y;
          rem = d_x - rem;
        }

        cover = -delta_y;
      }

      area = cover * (frac_x_0 + 0x100);
      cell_x = int_x_0;
      static_cast<Cell_processor&&>(cell_proc).set_x(cell_x);
      static_cast<Cell_processor&&>(cell_proc).set_cell(cover, area);
      ++int_x;
      y += delta_y;
      cell_x_valid = true;
    }
    else
    {
      rem = 0;
      cell_x_valid = false;
    }

    if(int_x < int_x_1)
    {
      p = d_y << 8u;
      Int_32 inc_y = p / d_x;
      Int_32 mod = p % d_x;
      Int_32 annex;

      if constexpr(Direction_::positive == y_direction)
      {
        annex = 1;
      }
      else
      {
        if(mod)
        {
          ++inc_y;
          mod = d_x - mod;
        }

        annex = -1;
      }

      do
      {
        delta_y = inc_y;
        rem += mod;

        if(rem >= d_x)
        {
          delta_y += annex;
          rem -= d_x;
        }

        if constexpr(Direction_::positive == y_direction)
        {
          cover = delta_y;
        }
        else
        {
          cover = -delta_y;
        }

        area = cover << 8u;

        if(cell_x_valid)
        {
          ++cell_x;
          static_cast<Cell_processor&&>(cell_proc).inc_x();
        }
        else
        {
          cell_x = int_x;
          static_cast<Cell_processor&&>(cell_proc).set_x(cell_x);
          cell_x_valid = true;
        }

        static_cast<Cell_processor&&>(cell_proc).set_cell(cover, area);
        y += delta_y;
        ++int_x;
      }
      while(int_x < int_x_1);
    }

    if(frac_x_1)
    {
      delta_y = d_y - y;

      if(delta_y)
      {
        if constexpr(Direction_::positive == y_direction)
        {
          cover = delta_y;
        }
        else
        {
          cover = -delta_y;
        }

        area = cover * frac_x_1;
        static_cast<Cell_processor&&>(cell_proc).inc_x();
        static_cast<Cell_processor&&>(cell_proc).set_cell(cover, area);
      }
    }
  }

  Int_32 x_0_ = 0;
  Int_32 y_0_ = 0;
  Int_32 x_ = 0;
  Int_32 y_ = 0;
};

} // namespace vgxx

#endif // VGXX_RASTERIZER_HH
