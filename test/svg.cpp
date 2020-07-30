/*******************************************************************************
* Author    :  Angus Johnson                                                   *
* Date      :  27 March 2019                                                   *
* Website   :  http://www.angusj.com                                           *
* Copyright :  Angus Johnson 2010-2019                                         *
*                                                                              *
* License   : http://www.boost.org/LICENSE_1_0.txt                             *
*******************************************************************************/

#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include "svg.h"

namespace svglib {

  using namespace clipper2;

  const char svg_xml_header_0[] =
		  "<?xml version=\"1.0\" standalone=\"no\"?>\n"
		  "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.0//EN\"\n"
		  "\"http://www.w3.org/TR/2001/REC-SVG-20010904/DTD/svg10.dtd\">\n\n<svg width=\"";
  const char svg_xml_header_1[] = "\" height=\"";
  const char svg_xml_header_2[] = "\" viewBox=\"0 0 ";
  const char svg_xml_header_3[] = "\" version=\"1.1\" xmlns=\"http://www.w3.org/2000/svg\">\n\n";


  const char svg_xml_0[] = "\"\n    style=\"fill:";
  const char svg_xml_1[] = "; fill-opacity:";
  const char svg_xml_2[] = "; fill-rule:";
  const char svg_xml_3[] = "; stroke:";
  const char svg_xml_4[] = "; stroke-opacity:";
  const char svg_xml_5[] = "; stroke-width:";
  const char svg_xml_6[] = ";\"/>\n";

  inline std::string ColorToHtml(unsigned clr)
  {
    std::stringstream ss;
    ss << '#' << std::hex << std::setfill('0') << std::setw(6) << (clr & 0xFFFFFF);
    return ss.str();
  }
  //------------------------------------------------------------------------------

  inline float GetAlphaAsFrac(unsigned clr)
  {
    return ((float)(clr >> 24) / 255);
  }
  //------------------------------------------------------------------------------

  void SVGBuilder::Clear()
  {
    for (PathInfoList::iterator pi_iter = path_info_list_.begin();
      pi_iter != path_info_list_.end(); ++pi_iter) delete (*pi_iter);
    path_info_list_.resize(0);
  }
  //------------------------------------------------------------------------------

  void SVGBuilder::SetCoordsStyle(std::string font_name,
      unsigned font_color, unsigned font_size)
  {
    coords_style.font_name = font_name;
    coords_style.font_color = font_color;
    coords_style.font_size = font_size;
  }
  //------------------------------------------------------------------------------

  void SVGBuilder::AddCaption(const std::string caption,
    unsigned font_color, unsigned font_size, int x, int y)
  {
    caption_info_.caption = caption;
    caption_info_.font_color = font_color;
    caption_info_.font_size = font_size;
    caption_info_.x = x;
    caption_info_.y = y;
  }
  //------------------------------------------------------------------------------

  void SVGBuilder::AddPath(const PathI &path, bool is_open,
    unsigned brush_color, unsigned pen_color, double pen_width, bool show_coords)
  {
    if (path.size() == 0) return;
    PathsI p;
    p.push_back(path);
    path_info_list_.push_back(new PathInfo(p, is_open,
      brush_color, pen_color, pen_width, show_coords));
  }
  //------------------------------------------------------------------------------

  void SVGBuilder::AddPaths(const PathsI &paths, bool is_open,
    unsigned brush_color, unsigned pen_color, double pen_width, bool show_coords)
  {
    if (paths.size() == 0) return;
    path_info_list_.push_back(new PathInfo(paths, is_open,
      brush_color, pen_color, pen_width, show_coords));
  }
  //------------------------------------------------------------------------------

  bool SVGBuilder::SaveToFile(const std::wstring &filename, int max_width, int max_height, int margin)
  {
    RectI rec = RectI(INT64_MAX, INT64_MAX, INT64_MIN, INT64_MIN);
    for (size_t i = 0; i < path_info_list_.size(); ++i)
      for (size_t j = 0; j < path_info_list_[i]->paths_.size(); ++j)
        for (size_t  k = 0; k < path_info_list_[i]->paths_[j].size(); ++k) {
          PointI ip = path_info_list_[i]->paths_[j][k];
          if (ip.x < rec.left) rec.left = ip.x;
          if (ip.x > rec.right) rec.right = ip.x;
          if (ip.y < rec.top) rec.top = ip.y;
          if (ip.y > rec.bottom) rec.bottom = ip.y;
        }

    if (rec.right <= rec.left || rec.bottom <= rec.top) return false;
    if (margin < 20) margin = 20;
    if (max_width < 100) max_width = 100;
    if (max_height < 100) max_height = 100;
    int64_t rec_width = rec.right - rec.left, rec_height = rec.bottom - rec.top;
    max_width -= margin;  max_height -= margin;
    double  scale = (std::min)((double)max_height / rec_height, (double)max_width / rec_width);

    rec.left = (int64_t)((double)rec.left * scale);
    rec.top = (int64_t)((double)rec.top * scale);
    rec.right = (int64_t)((double)rec.right * scale);
    rec.bottom = (int64_t)((double)rec.bottom * scale);
    int64_t offsetX = -rec.left + margin;
    int64_t offsetY = -rec.top + margin;

    std::ofstream file;
    file.open(filename);
    if (!file.is_open()) return false;
    file.setf(std::ios::fixed);
    file.precision(0);
    file << svg_xml_header_0 <<
      ((rec.right - rec.left) + margin * 2) << "px" << svg_xml_header_1 <<
      ((rec.bottom - rec.top) + margin * 2) << "px" << svg_xml_header_2 <<
      ((rec.right - rec.left) + margin * 2) << " " <<
      ((rec.bottom - rec.top) + margin * 2) << svg_xml_header_3;
    setlocale(LC_NUMERIC, "C");
    file.precision(2);

    for (PathInfoList::size_type i = 0; i < path_info_list_.size(); ++i) {
      file << "  <path d=\"";
      for (size_t  j = 0; j < path_info_list_[i]->paths_.size(); ++j) {
        size_t path_len = path_info_list_[i]->paths_[j].size();
        if (path_len < 2 || (path_len == 2 && !path_info_list_[i]->is_open_path)) continue;
        file << " M " << ((double)path_info_list_[i]->paths_[j][0].x * scale + offsetX) <<
          " " << ((double)path_info_list_[i]->paths_[j][0].y * scale + offsetY);
        for (size_t  k = 1; k < path_info_list_[i]->paths_[j].size(); ++k) {
          PointI ip = path_info_list_[i]->paths_[j][k];
          double x = (double)ip.x * scale;
          double y = (double)ip.y * scale;
          file << " L " << (x + offsetX) << " " << (y + offsetY);
        }
        if(!path_info_list_[i]->is_open_path)  file << " z";
      }
      file << svg_xml_0 << ColorToHtml(path_info_list_[i]->brush_color_) <<
        svg_xml_1 << GetAlphaAsFrac(path_info_list_[i]->brush_color_) <<
        svg_xml_2 <<
        (fill_rule == frEvenOdd ? "evenodd" : "nonzero") <<
        svg_xml_3 << ColorToHtml(path_info_list_[i]->pen_color_) <<
        svg_xml_4 << GetAlphaAsFrac(path_info_list_[i]->pen_color_) <<
        svg_xml_5 << path_info_list_[i]->pen_width_ << svg_xml_6;

      if (path_info_list_[i]->show_coords_) {
        file << "  <g font-family=\"" << coords_style.font_name << "\" font-size=\"" <<
          coords_style.font_size  << "\" fill=\""<< ColorToHtml(coords_style.font_color) <<
          "\" fill-opacity=\"" << GetAlphaAsFrac(coords_style.font_color) << "\">\n";
        for (size_t  j = 0; j < path_info_list_[i]->paths_.size(); ++j) {
          size_t path_len = path_info_list_[i]->paths_[j].size();
          if (path_len < 2 || (path_len == 2 && !path_info_list_[i]->is_open_path)) continue;
          for (size_t  k = 0; k < path_info_list_[i]->paths_[j].size(); ++k) {
            PointI ip = path_info_list_[i]->paths_[j][k];
            file << "    <text x=\"" << (int)(ip.x * scale + offsetX) <<
              "\" y=\"" << (int)(ip.y * scale + offsetY) << "\">" <<
              ip.x << "," << ip.y << "</text>\n";
          }
        }
        file << "  </g>\n\n";
      }
    }

    if (!caption_info_.caption.empty() && caption_info_.font_size > 2) {
      file << "  <g font-family=\"" << coords_style.font_name << "\" font-size=\"" <<
        caption_info_.font_size << "\" fill=\"" << ColorToHtml(caption_info_.font_color) <<
        "\" fill-opacity=\"" << GetAlphaAsFrac(caption_info_.font_color) << "\">\n";
      file << "    <text x=\"" << caption_info_.x << "\" y=\"" << caption_info_.y << "\">" <<
        caption_info_.caption << "</text>\n  </g>\n\n";
    }

    file << "</svg>\n";
    file.close();
    setlocale(LC_NUMERIC, "");
    return true;
  }
  //------------------------------------------------------------------------------

} //svg namespace
