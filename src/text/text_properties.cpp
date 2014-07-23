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
#include <mapnik/text/text_properties.hpp>
#include <mapnik/text/layout.hpp>
#include <mapnik/debug.hpp>
#include <mapnik/feature.hpp>
#include <mapnik/ptree_helpers.hpp>
#include <mapnik/expression_string.hpp>
#include <mapnik/text/formatting/text.hpp>
#include <mapnik/xml_node.hpp>
#include <mapnik/config_error.hpp>
#include <mapnik/text/properties_util.hpp>
#include <mapnik/boolean.hpp>

// boost
#include <boost/property_tree/ptree.hpp>

namespace mapnik
{
using boost::optional;

text_symbolizer_properties::text_symbolizer_properties()
    : label_placement(POINT_PLACEMENT),
      label_spacing(0.0),
      label_position_tolerance(0.0),
      avoid_edges(false),
      minimum_distance(0.0),
      minimum_padding(0.0),
      minimum_path_length(0.0),
      max_char_angle_delta(22.5 * M_PI/180.0),
      force_odd_labels(false),
      allow_overlap(false),
      largest_bbox_only(true),
      upright(UPRIGHT_AUTO),
      layout_defaults(),
      format_properties(),//std::make_shared<format_properties>()),
      tree_() {}

void text_symbolizer_properties::process(text_layout & output, feature_impl const& feature, attributes const& attrs) const
{
    output.clear();
    if (tree_)
    {
        //evaluate format properties
        char_properties_ptr format = std::make_shared<char_properties>();
        format->face_name = format_properties.face_name;
        format->fontset = format_properties.fontset;
        format->text_size = boost::apply_visitor(extract_value<value_double>(feature,attrs), format_properties.text_size);
        format->character_spacing = boost::apply_visitor(extract_value<value_double>(feature,attrs), format_properties.character_spacing);
        format->line_spacing = format_properties.line_spacing;
        format->text_opacity = format_properties.text_opacity;
        format->halo_opacity = format_properties.halo_opacity;
        format->wrap_char = format_properties.wrap_char;
        format->text_transform = format_properties.text_transform;
        format->fill = format_properties.fill;
        format->halo_fill = format_properties.halo_fill;
        format->halo_radius = format_properties.halo_radius;

        tree_->apply(format, feature, attrs, output);
    }
    else MAPNIK_LOG_WARN(text_properties) << "text_symbolizer_properties can't produce text: No formatting tree!";
}

void text_symbolizer_properties::set_format_tree(formatting::node_ptr tree)
{
    tree_ = tree;
}

formatting::node_ptr text_symbolizer_properties::format_tree() const
{
    return tree_;
}

void text_symbolizer_properties::placement_properties_from_xml(xml_node const& sym)
{
    optional<label_placement_e> placement_ = sym.get_opt_attr<label_placement_e>("placement");
    if (placement_) label_placement = *placement_;
    optional<double> label_position_tolerance_ = sym.get_opt_attr<double>("label-position-tolerance");
    if (label_position_tolerance_) label_position_tolerance = *label_position_tolerance_;
    optional<double> spacing_ = sym.get_opt_attr<double>("spacing");
    if (spacing_) label_spacing = *spacing_;
    else {
        // https://github.com/mapnik/mapnik/issues/1427
        spacing_ = sym.get_opt_attr<double>("label-spacing");
        if (spacing_) label_spacing = *spacing_;
    }
    optional<double> minimum_distance_ = sym.get_opt_attr<double>("minimum-distance");
    if (minimum_distance_) minimum_distance = *minimum_distance_;
    optional<double> min_padding_ = sym.get_opt_attr<double>("minimum-padding");
    if (min_padding_) minimum_padding = *min_padding_;
    optional<double> min_path_length_ = sym.get_opt_attr<double>("minimum-path-length");
    if (min_path_length_) minimum_path_length = *min_path_length_;
    optional<mapnik::boolean> avoid_edges_ = sym.get_opt_attr<mapnik::boolean>("avoid-edges");
    if (avoid_edges_) avoid_edges = *avoid_edges_;
    optional<mapnik::boolean> allow_overlap_ = sym.get_opt_attr<mapnik::boolean>("allow-overlap");
    if (allow_overlap_) allow_overlap = *allow_overlap_;
    optional<mapnik::boolean> largest_bbox_only_ = sym.get_opt_attr<mapnik::boolean>("largest-bbox-only");
    if (largest_bbox_only_) largest_bbox_only = *largest_bbox_only_;
}

void text_symbolizer_properties::from_xml(xml_node const& node, fontset_map const& fontsets)
{
    placement_properties_from_xml(node);

    optional<double> max_char_angle_delta_ = node.get_opt_attr<double>("max-char-angle-delta");
    if (max_char_angle_delta_) max_char_angle_delta=(*max_char_angle_delta_)*(M_PI/180);
    optional<text_upright_e> upright_ = node.get_opt_attr<text_upright_e>("upright");
    if (upright_) upright = *upright_;

    layout_defaults.from_xml(node);

    optional<expression_ptr> name_ = node.get_opt_attr<expression_ptr>("name");
    if (name_)
    {
        MAPNIK_LOG_WARN(text_placements) << "Using 'name' in TextSymbolizer/ShieldSymbolizer is deprecated!";

        set_old_style_expression(*name_);
    }

    format_properties.from_xml(node, fontsets);
    formatting::node_ptr n(formatting::node::from_xml(node));
    if (n) set_format_tree(n);
}

void text_symbolizer_properties::to_xml(boost::property_tree::ptree &node,
                                        bool explicit_defaults,
                                        text_symbolizer_properties const& dfl) const
{
    if (label_placement != dfl.label_placement || explicit_defaults)
    {
        set_attr(node, "placement", label_placement);
    }
    if (label_position_tolerance != dfl.label_position_tolerance || explicit_defaults)
    {
        set_attr(node, "label-position-tolerance", label_position_tolerance);
    }
    if (label_spacing != dfl.label_spacing || explicit_defaults)
    {
        set_attr(node, "spacing", label_spacing);
    }
    if (minimum_distance != dfl.minimum_distance || explicit_defaults)
    {
        set_attr(node, "minimum-distance", minimum_distance);
    }
    if (minimum_padding != dfl.minimum_padding || explicit_defaults)
    {
        set_attr(node, "minimum-padding", minimum_padding);
    }
    if (minimum_path_length != dfl.minimum_path_length || explicit_defaults)
    {
        set_attr(node, "minimum-path-length", minimum_path_length);
    }
    if (avoid_edges != dfl.avoid_edges || explicit_defaults)
    {
        set_attr(node, "avoid-edges", avoid_edges);
    }
    if (allow_overlap != dfl.allow_overlap || explicit_defaults)
    {
        set_attr(node, "allow-overlap", allow_overlap);
    }
    if (largest_bbox_only != dfl.largest_bbox_only || explicit_defaults)
    {
        set_attr(node, "largest-bbox-only", largest_bbox_only);
    }
    if (max_char_angle_delta != dfl.max_char_angle_delta || explicit_defaults)
    {
        set_attr(node, "max-char-angle-delta", max_char_angle_delta/(M_PI/180));
    }
    if (upright != dfl.upright || explicit_defaults)
    {
        set_attr(node, "upright", upright);
    }

    layout_defaults.to_xml(node, explicit_defaults, dfl.layout_defaults);
    format_properties.to_xml(node, explicit_defaults, dfl.format_properties);
    if (tree_) tree_->to_xml(node);
}


void text_symbolizer_properties::add_expressions(expression_set & output) const
{
    layout_defaults.add_expressions(output);
    if (tree_) tree_->add_expressions(output);
}

void text_symbolizer_properties::set_old_style_expression(expression_ptr expr)
{
    tree_ = std::make_shared<formatting::text_node>(expr);
}

text_layout_properties::text_layout_properties()
    : halign(H_AUTO),
      jalign(J_AUTO),
      valign(V_AUTO)
{}

void text_layout_properties::from_xml(xml_node const &node)
{
    set_property_from_xml<double>(dx, "dx", node);
    set_property_from_xml<double>(dy, "dy", node);
    set_property_from_xml<double>(text_ratio, "text-ratio", node);
    set_property_from_xml<double>(wrap_width, "wrap-width", node);
    set_property_from_xml<mapnik::boolean>(wrap_before, "wrap-before", node);
    set_property_from_xml<mapnik::boolean>(rotate_displacement, "rotate-displacement", node);
    set_property_from_xml<double>(orientation, "orientation", node);
    //
    optional<vertical_alignment_e> valign_ = node.get_opt_attr<vertical_alignment_e>("vertical-alignment");
    if (valign_) valign = *valign_;
    optional<horizontal_alignment_e> halign_ = node.get_opt_attr<horizontal_alignment_e>("horizontal-alignment");
    if (halign_) halign = *halign_;
    optional<justify_alignment_e> jalign_ = node.get_opt_attr<justify_alignment_e>("justify-alignment");
    if (jalign_) jalign = *jalign_;
}

void text_layout_properties::to_xml(boost::property_tree::ptree & node,
                                    bool explicit_defaults,
                                    text_layout_properties const& dfl) const
{
    if (!(dx == dfl.dx) || explicit_defaults) serialize_property("dx", dx, node);
    if (!(dy == dfl.dy) || explicit_defaults) serialize_property("dy", dy, node);
    if (valign != dfl.valign || explicit_defaults) set_attr(node, "vertical-alignment", valign);
    if (halign != dfl.halign || explicit_defaults) set_attr(node, "horizontal-alignment", halign);
    if (jalign != dfl.jalign || explicit_defaults) set_attr(node, "justify-alignment", jalign);
    if (!(text_ratio == dfl.text_ratio) || explicit_defaults) serialize_property("text-ratio", text_ratio, node);
    if (!(wrap_width == dfl.wrap_width) || explicit_defaults) serialize_property("wrap-width", wrap_width, node);
    if (!(wrap_before == dfl.wrap_before) || explicit_defaults) serialize_property("wrap-before", wrap_before, node);
    if (!(rotate_displacement == dfl.rotate_displacement) || explicit_defaults)
        serialize_property("rotate-displacement", rotate_displacement, node);
    if (!(orientation == dfl.orientation) || explicit_defaults) serialize_property("orientation", orientation, node);
}

void text_layout_properties::add_expressions(expression_set& output) const
{
    if (is_expression(dx)) output.insert(boost::get<expression_ptr>(dx));
    if (is_expression(dy)) output.insert(boost::get<expression_ptr>(dy));
    if (is_expression(orientation)) output.insert(boost::get<expression_ptr>(orientation));
    if (is_expression(wrap_width)) output.insert(boost::get<expression_ptr>(wrap_width));
    if (is_expression(wrap_before)) output.insert(boost::get<expression_ptr>(wrap_before));
    if (is_expression(rotate_displacement)) output.insert(boost::get<expression_ptr>(rotate_displacement));
    if (is_expression(text_ratio)) output.insert(boost::get<expression_ptr>(text_ratio));
}

// text format properties

format_properties::format_properties()
    : face_name(),
      fontset(),
      text_size(10.0),
      character_spacing(0.0),
      line_spacing(0),
      text_opacity(1.0),
      halo_opacity(1.0),
      wrap_char(' '),
      text_transform(NONE),
      fill(color(0,0,0)),
      halo_fill(color(255,255,255)),
      halo_radius(0) {}

void format_properties::from_xml(xml_node const& node, fontset_map const& fontsets)
{
    set_property_from_xml<double>(text_size, "size", node);
    set_property_from_xml<double>(character_spacing, "character-spacing", node);

    //optional<double> character_spacing_ = node.get_opt_attr<double>("character-spacing");
    //if (character_spacing_) character_spacing = *character_spacing_;
    optional<color> fill_ = node.get_opt_attr<color>("fill");
    if (fill_) fill = *fill_;
    optional<color> halo_fill_ = node.get_opt_attr<color>("halo-fill");
    if (halo_fill_) halo_fill = *halo_fill_;
    optional<double> halo_radius_ = node.get_opt_attr<double>("halo-radius");
    if (halo_radius_) halo_radius = *halo_radius_;
    optional<text_transform_e> tconvert_ = node.get_opt_attr<text_transform_e>("text-transform");
    if (tconvert_) text_transform = *tconvert_;
    optional<double> line_spacing_ = node.get_opt_attr<double>("line-spacing");
    if (line_spacing_) line_spacing = *line_spacing_;
    optional<double> opacity_ = node.get_opt_attr<double>("opacity");
    if (opacity_) text_opacity = *opacity_;
    optional<double> halo_opacity_ = node.get_opt_attr<double>("halo-opacity");
    if (halo_opacity_) halo_opacity = *halo_opacity_;
    optional<std::string> wrap_char_ = node.get_opt_attr<std::string>("wrap-character");
    if (wrap_char_ && (*wrap_char_).size() > 0) wrap_char = ((*wrap_char_)[0]);
    optional<std::string> face_name_ = node.get_opt_attr<std::string>("face-name");
    if (face_name_)
    {
        face_name = *face_name_;
    }
    optional<std::string> fontset_name_ = node.get_opt_attr<std::string>("fontset-name");
    if (fontset_name_) {
        std::map<std::string,font_set>::const_iterator itr = fontsets.find(*fontset_name_);
        if (itr != fontsets.end())
        {
            fontset = itr->second;
        }
        else
        {
            throw config_error("Unable to find any fontset named '" + *fontset_name_ + "'", node);
        }
    }
    if (!face_name.empty() && fontset)
    {
        throw config_error("Can't have both face-name and fontset-name", node);
    }
    if (face_name.empty() && !fontset)
    {
        throw config_error("Must have face-name or fontset-name", node);
    }
}

void format_properties::to_xml(boost::property_tree::ptree & node, bool explicit_defaults, format_properties const& dfl) const
{
    if (fontset)
    {
        set_attr(node, "fontset-name", fontset->get_name());
    }

    if (face_name != dfl.face_name || explicit_defaults)
    {
        set_attr(node, "face-name", face_name);
    }

    if (!(text_size == dfl.text_size) || explicit_defaults) serialize_property("size", text_size, node);

    if (fill != dfl.fill || explicit_defaults)
    {
        set_attr(node, "fill", fill);
    }
    if (halo_radius != dfl.halo_radius || explicit_defaults)
    {
        set_attr(node, "halo-radius", halo_radius);
    }
    if (halo_fill != dfl.halo_fill || explicit_defaults)
    {
        set_attr(node, "halo-fill", halo_fill);
    }
    if (wrap_char != dfl.wrap_char || explicit_defaults)
    {
        set_attr(node, "wrap-character", std::string(1, wrap_char));
    }
    if (text_transform != dfl.text_transform || explicit_defaults)
    {
        set_attr(node, "text-transform", text_transform);
    }
    if (line_spacing != dfl.line_spacing || explicit_defaults)
    {
        set_attr(node, "line-spacing", line_spacing);
    }
    if (!(character_spacing == dfl.character_spacing) || explicit_defaults)
        serialize_property("character-spacing", character_spacing, node);

    // for shield_symbolizer this is later overridden
    if (text_opacity != dfl.text_opacity || explicit_defaults)
    {
        set_attr(node, "opacity", text_opacity);
    }
    // for shield_symbolizer this is later overridden
    if (halo_opacity != dfl.halo_opacity || explicit_defaults)
    {
        set_attr(node, "halo-opacity", halo_opacity);
    }
}

} //ns mapnik
