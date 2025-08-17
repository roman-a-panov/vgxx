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

#ifndef VGXX_CELLPROCESSOR_HH
#define VGXX_CELLPROCESSOR_HH

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <vector>

#include <vgxx/fill_rule.hh>
#include <vgxx/util.hh>

namespace vgxx
{

struct Cell_processor
{
  using Int_32 = ::std::int32_t;
  using Unt_16 = ::std::uint16_t;

  Cell_processor(Cell_processor&&) = delete;
  Cell_processor(Cell_processor const&) = delete;

  explicit Cell_processor(Unt_16 const width, Unt_16 const height) :
    rows_(nullptr),
    width_(static_cast<Int_32>(width)) ,
    height_(static_cast<Int_32>(height)),
    x_(0),
    y_(0)
  {
    if(0u < width && 0u < height)
    {
      rows_ = new Row_[static_cast<Size_>(height)];
    }
  }

  ~Cell_processor()
  {
    if(rows_)
    {
      delete[] rows_;
    }
  }

  Cell_processor& operator =(Cell_processor&&) = delete;
  Cell_processor& operator =(Cell_processor const&) = delete;

  void inc_x() noexcept
  {
    ++x_;
  }

  void set_x(Int_32 const x) noexcept
  {
    x_ = x;
  }

  void set_y(Int_32 const y)
  {
    y_ = y;
  }

  void set_cell(Int_32 const cover, Int_32 const area) noexcept
  {
    if(height_ > y_ && 0 <= y_)
    {
      Row_& row = rows_[y_];
      if(0 <= x_)
      {
        if(width_ > x_)
        {
          auto& cell_idx = row.first_cell_idx;
          if(invalid_cell_index_ != cell_idx)
          {
            auto& cell = cell_stash_[cell_idx];
            if(cell.x == x_)
            {
              cell.cover += cover;
              cell.area += area;
              return;
            }
          }

          Cell_index_ new_cell_idx;
          auto& cell = cell_stash_.acquire(new_cell_idx);
          cell.cover = cover;
          cell.area = area;
          cell.next_cell_idx = cell_idx;
          cell.x = x_;
          cell_idx = new_cell_idx;
          row.x_range.update(x_);
        }
        else // width_ <= x
        {
          if(0 < width_)
          {
            row.x_range.update(static_cast<Unt_16>(width_ - 1));
          }
        }
      }
      else // 0 > x_
      {
        row.left_cover += cover;
        row.x_range.update(0u);
      }

      y_range_.update(y_);
    }
  }

  template<class Blender>
  void swipe(Blender&& blender, Fill_rule const fill_rule)
  {
    switch(fill_rule)
    {
    case Fill_rule::non_zero:
      swipe<Fill_rule::non_zero>(static_cast<Blender&&>(blender));
      break;
    case Fill_rule::even_odd:
      swipe<Fill_rule::even_odd>(static_cast<Blender&&>(blender));
      break;
    default:
      assert(false);
      break;
    }
  }

  template<Fill_rule fill_rule, class Blender>
  void swipe(Blender&& blender)
  {
    static_assert(
      Fill_rule::non_zero == fill_rule ||
      Fill_rule::even_odd == fill_rule);

    if(y_range_)
    {
      auto* row = rows_;
      Cell_* cell;
      Int_32 cover, mid_cover;
      auto y = y_range_.min;
      Unt_8_ coverage, mid_coverage;
      row += y;
      static_cast<Blender&&>(blender).set_y(y);

      for(;;)
      {
        auto& x_range = row->x_range;
        if(x_range)
        {
          auto const& x_min = x_range.min;
          auto const& x_max = x_range.max;
          Size_ const x_range_size = static_cast<Size_>(x_max - x_min) + 1u;
          if(cells_.size() < x_range_size)
          {
            cells_.resize(x_range_size);
          }

          auto cell_idx = row->first_cell_idx;
          cell = cells_.data();
          while(invalid_cell_index_ != cell_idx)
          {
            auto const& src_cell = cell_stash_[cell_idx];
            auto& dst_cell = cell[src_cell.x - x_min];
            dst_cell.cover += src_cell.cover;
            dst_cell.area += src_cell.area;
            cell_idx = src_cell.next_cell_idx;
          }

          auto x = x_min;
          cover = row->left_cover;
          mid_coverage = 0u;
          static_cast<Blender&&>(blender).set_x(x);

          for(;;)
          {
            if(*cell)
            {
              cover += cell->cover;
              coverage =
                Util::compute_cell_coverage<fill_rule>(cover, cell->area);
              mid_coverage = 0u;
              cell->reset();
            }
            else
            {
              if(1u > mid_coverage && 0 != cover)
              {
                mid_cover = cover;
                if(0 > mid_cover)
                {
                  mid_cover = -mid_cover;
                }

                if constexpr(Fill_rule::non_zero == fill_rule)
                {
                  if (0x100 < mid_cover)
                  {
                    mid_cover = 0x100;
                  }
                }
                else // Fill_rule::non_zero == fill_rule
                {
                  if(((mid_cover >> 8u) & 1) == 0)
                  {
                    // Even.
                    mid_cover &= 0xff;
                  }
                  else
                  {
                    // Odd.
                    mid_cover = 0x100 - (mid_cover & 0xff);
                  }
                }

                // * 255 / 256
                mid_coverage =
                  static_cast<Unt_8_>(((mid_cover << 8u) - mid_cover) >> 8u);
              }

              coverage = mid_coverage;
            }

            if(0u < coverage)
            {
              static_cast<Blender&&>(blender).blend(coverage);
            }

            if(x_max > x)
            {
              ++cell;
              ++x;
              static_cast<Blender&&>(blender).inc_x();
            }
            else
            {
              break;
            }
          }

          row->reset();
        }

        if(y_range_.max > y)
        {
          ++row;
          ++y;
          static_cast<Blender&&>(blender).inc_y();
        }
        else
        {
          break;
        }
      }

      y_range_.reset();
    }

    cell_stash_.reset();
  }

private:
  using Size_ = ::std::size_t;
  using Unt_8_ = ::std::uint8_t;
  using Unt_32_ = ::std::uint32_t;
  using Cell_index_ = Unt_32_;
  using Overflow_error_ = ::std::overflow_error;

  template<class T>
  using Decay = typename ::std::decay<T>::type;

  template<class T>
  using Numeric_limits_ = ::std::numeric_limits<T>;

  template<class T>
  using Vector_ = ::std::vector<T>;

  struct Pixel_range_
  {
    Pixel_range_() noexcept :
      min(unt_16_max_),
      max(unt_16_min_)
    {}

    [[nodiscard]] explicit operator bool() const noexcept
    {
      return min <= max;
    }

    void reset() noexcept
    {
      min = unt_16_max_;
      max = unt_16_min_;
    }

    void update(Unt_16 const val) noexcept
    {
      if(val < min) {
        min = val;
      }
      if(val > max) {
        max = val;
      }
    }

    Unt_16 min;
    Unt_16 max;

  private:
    using Unt_16_limits_ = Numeric_limits_<Unt_16>;

    static Unt_16 constexpr unt_16_min_ = Unt_16_limits_::min();
    static Unt_16 constexpr unt_16_max_ = Unt_16_limits_::max();
  };

  struct Cell_
  {
    explicit operator bool() const noexcept
    {
      return 0 != cover || 0 != area;
    }

    void reset() noexcept
    {
      cover = 0;
      area = 0;
    }

    Int_32 cover;
    Int_32 area;
  };

  struct Cell_ex_ : Cell_
  {
    Cell_index_ next_cell_idx;
    Unt_16 x;
  };

  struct Cell_stash_
  {
    template<class I>
    [[nodiscard]] Cell_ex_& operator [](I const& i) noexcept
    {
      return cells_[i];
    }

    void reset() noexcept
    {
      cells_in_use_ = 0u;
    }

    [[nodiscard]] Cell_ex_& acquire(Cell_index_& idx)
    {
      using Vec_size = Decay<decltype(cells_.size())>;

      auto constexpr max_size = Numeric_limits_<Cell_index_>::max();

      if(max_size > cells_in_use_)
      {
        Cell_ex_* cell;
        auto const vec_size = cells_.size();

        if(vec_size > cells_in_use_)
        {
          cell = cells_.data() + cells_in_use_;
        }
        else
        {
          if(cells_.capacity() <= vec_size)
          {
            Vec_size new_capacity;
            if(20u > vec_size)
            {
              new_capacity = vec_size + 4u;
            }
            else
            {
              new_capacity = vec_size + vec_size / 4u;
            }

            if(max_size < new_capacity)
            {
              new_capacity = static_cast<Vec_size>(max_size);
            }

            cells_.reserve(new_capacity);
          }

          cell = std::addressof(cells_.emplace_back());
        }

        idx = cells_in_use_++;
        return *cell;
      }

      throw Overflow_error_("Too many cells");
    }

  private:
    Vector_<Cell_ex_> cells_;
    Cell_index_ cells_in_use_ = 0u;
  };

  struct Row_
  {
    Row_() noexcept :
      first_cell_idx(invalid_cell_index_),
      left_cover(0)
    {}

    void reset() noexcept
    {
      first_cell_idx = invalid_cell_index_;
      left_cover = 0;
      x_range.reset();
    }

    Cell_index_ first_cell_idx;
    Int_32 left_cover;
    Pixel_range_ x_range;
  };

  static auto constexpr invalid_cell_index_ =
    Numeric_limits_<Cell_index_>::max();

  Row_* rows_;
  Vector_<Cell_> cells_;
  Cell_stash_ cell_stash_;
  Int_32 const width_;
  Int_32 const height_;
  Int_32 x_;
  Int_32 y_;
  Pixel_range_ y_range_;
};

} // namespace vgxx

#endif // VGXX_CELLPROCESSOR_HH
