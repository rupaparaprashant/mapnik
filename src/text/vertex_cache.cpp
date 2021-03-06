/*****************************************************************************
 *
 * This file is part of Mapnik (c++ mapping toolkit)
 *
 * Copyright (C) 2013 Artem Pavlenko
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *****************************************************************************/
// mapnik
#include <mapnik/text/vertex_cache.hpp>
#include <mapnik/offset_converter.hpp>

// boost


namespace mapnik
{

double vertex_cache::angle(double width)
{
 /* IMPORTANT NOTE: See note about coordinate systems in placement_finder::find_point_placement()
  * for imformation about why the y axis is inverted! */
    double tmp = width + position_in_segment_;
    if ((tmp <= current_segment_->length) && (tmp >= 0))
    {
        //Only calculate angle on request as it is expensive
        if (!angle_valid_)
        {
            angle_ = atan2(-(current_segment_->pos.y - segment_starting_point_.y),
                           current_segment_->pos.x - segment_starting_point_.x);
        }
        return width >= 0 ? angle_ : angle_ + M_PI;
    } else
    {
        scoped_state s(*this);
        pixel_position const& old_pos = s.get_state().position();
        move(width);
        double angle = atan2(-(current_position_.y - old_pos.y),
                             current_position_.x - old_pos.x);
        return angle;
    }
}

bool vertex_cache::next_subpath()
{
    if (!initialized_)
    {
        current_subpath_ = subpaths_.begin();
        initialized_ = true;
    } else
    {
        current_subpath_++;
    }
    if (current_subpath_ == subpaths_.end()) return false;
    rewind_subpath(); //Initialize position values
    return true;
}

void vertex_cache::rewind_subpath()
{
    current_segment_ = current_subpath_->vector.begin();
    //All subpaths contain at least one segment (i.e. the starting point)
    segment_starting_point_ = current_position_ = current_segment_->pos;
    position_in_segment_ = 0;
    angle_valid_ = false;
    position_ = 0;
}

void vertex_cache::reset()
{
    initialized_ = false;
}

bool vertex_cache::next_segment()
{
    segment_starting_point_ = current_segment_->pos; //Next segments starts at the end of the current one
    if (current_segment_ == current_subpath_->vector.end()) return false;
    current_segment_++;
    angle_valid_ = false;
    if (current_segment_ == current_subpath_->vector.end()) return false;
    return true;
}

bool vertex_cache::previous_segment()
{
    if (current_segment_ == current_subpath_->vector.begin()) return false;
    current_segment_--;
    angle_valid_ = false;
    if (current_segment_ == current_subpath_->vector.begin())
    {
        //First segment is special
        segment_starting_point_ = current_segment_->pos;
        return true;
    }
    segment_starting_point_ = (current_segment_-1)->pos;
    return true;
}

vertex_cache & vertex_cache::get_offseted(double offset, double region_width)
{
    if (fabs(offset) < 0.01)
    {
        return *this;
    }

    vertex_cache_ptr offseted_line;
    offseted_lines_map::iterator pos = offseted_lines_.find(offset);
    if (pos != offseted_lines_.end())
    {
        offseted_line = pos->second;
    } else {
        offset_converter<vertex_cache> converter(*this);
        converter.set_offset(offset);
        offseted_line = vertex_cache_ptr(new vertex_cache(converter));
    }
    offseted_line->reset();
    offseted_line->next_subpath(); //TODO: Multiple subpath support
    double seek = (position_ + region_width/2.) * offseted_line->length() / length() - region_width/2.;
    if (seek < 0) seek = 0;
    if (seek > offseted_line->length()) seek = offseted_line->length();
    offseted_line->move(seek);
    offseted_lines_[offset] = offseted_line;
    return *offseted_line;
}

bool vertex_cache::forward(double length)
{
    if (length < 0)
    {
        MAPNIK_LOG_ERROR(vertex_cache) << "vertex_cache::forward() called with negative argument!\n";
        return false;
    }
    return move(length);
}

bool vertex_cache::backward(double length)
{
    if (length < 0)
    {
        MAPNIK_LOG_ERROR(vertex_cache) << "vertex_cache::backward() called with negative argument!\n";
        return false;
    }
    return move(-length);
}

bool vertex_cache::move(double length)
{
    position_ += length;
    length += position_in_segment_;
    while (length >= current_segment_->length)
    {
        length -= current_segment_->length;
        if (!next_segment()) return false; //Skip all complete segments
    }
    while (length < 0)
    {
        if (!previous_segment()) return false;
        length += current_segment_->length;
    }
    double factor = length / current_segment_->length;
    position_in_segment_ = length;
    current_position_ = segment_starting_point_ + (current_segment_->pos - segment_starting_point_) * factor;
    return true;
}

void vertex_cache::rewind(unsigned)
{
    vertex_subpath_ = subpaths_.begin();
    vertex_segment_ = vertex_subpath_->vector.begin();
}

unsigned vertex_cache::vertex(double *x, double *y)
{
    if (vertex_segment_ == vertex_subpath_->vector.end())
    {
        vertex_subpath_++;
        if (vertex_subpath_ == subpaths_.end()) return agg::path_cmd_stop;
        vertex_segment_ = vertex_subpath_->vector.begin();
    }
    *x = vertex_segment_->pos.x;
    *y = vertex_segment_->pos.y;
    unsigned cmd = (vertex_segment_ == vertex_subpath_->vector.begin()) ? agg::path_cmd_move_to : agg::path_cmd_line_to;
    vertex_segment_++;
    return cmd;
}


vertex_cache::state vertex_cache::save_state() const
{
    state s;
    s.current_segment = current_segment_;
    s.position_in_segment = position_in_segment_;
    s.current_position = current_position_;
    s.segment_starting_point = segment_starting_point_;
    s.position_ = position_;
    return s;
}

void vertex_cache::restore_state(state const& s)
{
    current_segment_ = s.current_segment;
    position_in_segment_ = s.position_in_segment;
    current_position_ = s.current_position;
    segment_starting_point_ = s.segment_starting_point;
    position_ = s.position_;
    angle_valid_ = false;
}

} //ns mapnik
