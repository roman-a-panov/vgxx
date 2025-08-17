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

#ifndef VGXX_BLENDERBASE_HH
#define VGXX_BLENDERBASE_HH

#include <cstddef>

namespace vgxx
{

template<class C>
struct Blender_base
{
  using Color = C;
  using Size = ::std::size_t;

  explicit Blender_base(
    Color* const img_data,
    Size const bytes_per_row) noexcept :
    img_data_(img_data),
    row_(nullptr),
    pixel_(nullptr),
    bytes_per_row_(bytes_per_row)
  {}

  [[nodiscard]] Color* pixel() const noexcept
  {
    return pixel_;
  }

  template<class X>
  void set_x(X const& x) noexcept
  {
    pixel_ = row_ + x;
  }

  template<class Y>
  void set_y(Y const& y) noexcept
  {
    static_assert(1u == sizeof(char));
    Size const row_offset = bytes_per_row_ * static_cast<Size>(y);
    row_ = reinterpret_cast<Color*>(
      reinterpret_cast<char*>(img_data_) + row_offset);
  }

  void inc_x() noexcept
  {
    ++pixel_;
  }

  void inc_y() noexcept
  {
    static_assert(1u == sizeof(char));
    row_ = reinterpret_cast<Color*>(
      reinterpret_cast<char*>(row_) + bytes_per_row_);
  }

private:
  Color* img_data_;
  Color* row_;
  Color* pixel_;
  Size bytes_per_row_;
};

} // namespace vgxx

#endif // VGXX_BLENDERBASE_HH
