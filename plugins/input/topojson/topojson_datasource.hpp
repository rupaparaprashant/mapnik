/*****************************************************************************
 *
 * This file is part of Mapnik (c++ mapping toolkit)
 *
 * Copyright (C) 2012 Artem Pavlenko
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

#ifndef TOPOJSON_DATASOURCE_HPP
#define TOPOJSON_DATASOURCE_HPP

// mapnik
#include <mapnik/datasource.hpp>
#include <mapnik/params.hpp>
#include <mapnik/query.hpp>
#include <mapnik/feature.hpp>
#include <mapnik/box2d.hpp>
#include <mapnik/coord.hpp>
#include <mapnik/feature_layer_desc.hpp>
#include <mapnik/unicode.hpp>
#include <mapnik/json/topology.hpp>
// boost
#include <boost/optional.hpp>

#include <boost/geometry/geometries/box.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/algorithms/area.hpp>
#include <boost/geometry/geometries/geometries.hpp>
#include <boost/geometry/extensions/index/rtree/rtree.hpp>
// stl
#include <vector>
#include <string>
#include <map>
#include <deque>
#include <memory>

class topojson_datasource : public mapnik::datasource
{
public:
    typedef boost::geometry::model::d2::point_xy<double> point_type;
    typedef boost::geometry::model::box<point_type> box_type;
    typedef boost::geometry::index::rtree<box_type,std::size_t> spatial_index_type;

    // constructor
    topojson_datasource(mapnik::parameters const& params);
    virtual ~topojson_datasource ();
    mapnik::datasource::datasource_t type() const;
    static const char * name();
    mapnik::featureset_ptr features(mapnik::query const& q) const;
    mapnik::featureset_ptr features_at_point(mapnik::coord2d const& pt, double tol = 0) const;
    mapnik::box2d<double> envelope() const;
    mapnik::layer_descriptor get_descriptor() const;
    boost::optional<mapnik::datasource::geometry_t> get_geometry_type() const;
private:
    mapnik::datasource::datasource_t type_;
    std::map<std::string, mapnik::parameters> statistics_;
    mapnik::layer_descriptor desc_;
    std::string file_;
    mapnik::box2d<double> extent_;
    std::shared_ptr<mapnik::transcoder> tr_;
    mapnik::topojson::topology topo_;
    spatial_index_type tree_;
};


#endif // FILE_DATASOURCE_HPP
