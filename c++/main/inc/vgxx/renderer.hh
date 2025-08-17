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

#ifndef VGXX_RENDERER_HH
#define VGXX_RENDERER_HH

#include <cassert>
#include <cstdint>
#include <type_traits>

#include <vgxx/fill_rule.hh>
#include <vgxx/rasterizer.hh>
#include <vgxx/cell_processor.hh>
#include <vgxx/util.hh>

namespace vgxx
{

template<class B>
struct Renderer
{
  using Blender = B;
  using Unt_16 = ::std::uint16_t;

private:
  using Clip_flags_ = Unt_16;

  template<class T, class... Args>
  using Is_constructible_ = typename ::std::is_constructible<T, Args...>::type;

  template<bool, class = void>
  struct Enable_if_
  {};

  template<class T>
  struct Enable_if_<true, T>
  {
    using Type = T;
  };

public:
  template<
    class... Blebder_args,
    bool e = Is_constructible_<Blender, Blebder_args...>::value,
    class = typename Enable_if_<e>::Type>
  explicit Renderer(
    Unt_16 const width,
    Unt_16 const height,
    Blebder_args&&... blender_args) :
    cell_proc_(width, height),
    blender_(static_cast<Blebder_args&&>(blender_args)...),
    //height_(static_cast<float>(height)),
    //width_(static_cast<float>(width)),
    x_0_(0.f),
    y_0_(0.f),
    x_(0.f),
    y_(0.f)
    //clip_flags_0_(0u),
    //clip_flags_(0u)
  {
    assert(0u < width);
    assert(0u < height);
  }

  [[nodiscard]] Blender& blender() noexcept
  {
    return blender_;
  }

  [[nodiscard]] Blender const& blender() const noexcept
  {
    return blender_;
  }

  void move_to(float const x, float const y) noexcept
  {
    rasterizer_.move_to(cell_proc_, x, y);
    x_0_ = x;
    y_0_ = y;
    x_ = x;
    y_ = y;
    //clip_flags_0_ = compute_clip_flags_(x, y);
    //clip_flags_ = clip_flags_0_;
  }

  void line_to(float const x, float const y) noexcept
  {
    line_to_(x, y);
    x_ = x;
    y_ = y;
    //clip_flags_ = compute_clip_flags_(x, y);
  }

  void bezier_to(
    float const x_1,
    float const y_1,
    float const x_2,
    float const y_2,
    float const x_3,
    float const y_3) noexcept
  {
    //auto clip_flags = clip_flags_;
    //Clip_flags_ next_clip_flags;
    //bool out_of_viewport = false;

    /*if(clip_flags)
    {
      clip_flags &= compute_clip_flags_(x_1, y_1);
      if(clip_flags)
      {
        clip_flags &= compute_clip_flags_(x_2, y_2);
        if(clip_flags)
        {
          next_clip_flags = compute_clip_flags_(x_3, y_3);
          clip_flags &= next_clip_flags;
          if(clip_flags)
          {
            out_of_viewport = true;
          }
        }
      }
    }*/

    /*if(out_of_viewport)
    {
      if(x_3 != x_ || y_3 != y_)
      {
        line_to_(x_3, y_3);
      }
    }
    else*/
    {
      Util::subdivide_bezier(
        [this](auto const& x, auto const& y) noexcept
        {
          line_to_(x, y);
        },
        x_, y_, x_1, y_1, x_2, y_2, x_3, y_3);
      //next_clip_flags = compute_clip_flags_(x_3, y_3);
    }

    x_ = x_3;
    y_ = y_3;
    //clip_flags_ = next_clip_flags;
  }

  void close_outline() noexcept
  {
    rasterizer_.close(cell_proc_);
    x_ = x_0_;
    y_ = y_0_;
    //clip_flags_ = clip_flags_0_;
  }

  template<Fill_rule fill_rule>
  void fill()
  {
    close_outline();
    cell_proc_.swipe<fill_rule>(blender_);
  }

  void fill(Fill_rule const fill_rule)
  {
    close_outline();
    cell_proc_.swipe(blender_, fill_rule);
  }

private:
  template<class T>
  void line_to_(T const& x, T const& y) noexcept
  {
    rasterizer_.line_to(cell_proc_, x, y);
  }

  /*
  template<class T>
  [[nodiscard]] Clip_flags_ compute_clip_flags_(
    T const& x, T const& y) const noexcept
  {
    auto constexpr val_0 = static_cast<T>(0);

    Clip_flags_ flags = 0u;
    if(val_0 >= x)
    {
      flags = 0b0001u;
    }
    if(val_0 >= y)
    {
      flags |= 0b0010u;
    }
    if(width_ <= x)
    {
      flags |= 0b0100u;
    }
    if(height_ <= y)
    {
      flags |= 0b1000u;
    }

    return flags;
  }
   */

  Rasterizer rasterizer_;
  Cell_processor cell_proc_;
  Blender blender_;
  //float height_;
  //float width_;
  float x_0_;
  float y_0_;
  float x_;
  float y_;
  //Clip_flags_ clip_flags_0_;
  //Clip_flags_ clip_flags_;
};

} // namespace vgxx

#endif // VGXX_RENDERER_HH
