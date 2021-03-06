/*******************************************************************************
* Author    :  Angus Johnson                                                   *
* Version   :  10.0 (beta)                                                     *
* Date      :  24 March 2019                                                   *
* Website   :  http://www.angusj.com                                           *
* Copyright :  Angus Johnson 2010-2019                                         *
* Purpose   :  Core Clipper Library module                                     *
*              Contains structures and functions used throughout the library   *
* License   :  http://www.boost.org/LICENSE_1_0.txt                            *
*******************************************************************************/

#include <clipper2/clipper_core.h>

namespace clipperlib {

//------------------------------------------------------------------------------
// Template specialization declarations ...
//------------------------------------------------------------------------------

template struct Point<cInt>;
template struct Point<double>;
template struct Rect<cInt>;
template struct Rect<double>;
template struct Path<cInt>;
template struct Path<double>;
template struct Paths<cInt>;
template struct Paths<double>;
template struct PathsArray<cInt>;
template struct PathsArray<double>;

//------------------------------------------------------------------------------
// Point
//------------------------------------------------------------------------------

template<>
void PointI::Rotate(const PointD &center, double angle_rad) {
	double tmp_x = x - center.x;
	double tmp_y = y - center.y;
	double cos_a = cos(angle_rad);
	double sin_a = sin(angle_rad);

	x = (cInt)round(tmp_x * cos_a - tmp_y * sin_a + center.x);
	y = (cInt)round(tmp_x * sin_a - tmp_y * cos_a + center.y);
}
//------------------------------------------------------------------------------

template<>
void PointD::Rotate(const PointD &center, double angle_rad) {
	double tmp_x = x - center.x;
	double tmp_y = y - center.y;
	double cos_a = cos(angle_rad);
	double sin_a = sin(angle_rad);

	x = tmp_x * cos_a - tmp_y * sin_a + center.x;
	y = tmp_x * sin_a - tmp_y * cos_a + center.y;
}
//------------------------------------------------------------------------------

template<>
void PointI::Rotate(const PointD &center, double sin_a, double cos_a) {
	double tmp_x = x - center.x;
	double tmp_y = y - center.y;

	x = (cInt)round(tmp_x * cos_a - tmp_y * sin_a + center.x);
	y = (cInt)round(tmp_x * sin_a - tmp_y * cos_a + center.y);
}
//------------------------------------------------------------------------------

template<>
void PointD::Rotate(const PointD &center, double sin_a, double cos_a) {
	double tmp_x = x - center.x;
	double tmp_y = y - center.y;

	x = tmp_x * cos_a - tmp_y * sin_a + center.x;
	y = tmp_x * sin_a - tmp_y * cos_a + center.y;
}
//------------------------------------------------------------------------------

template<typename T>
inline void Point<T>::Rotate(const PointD & center, double angle_rad){}
//------------------------------------------------------------------------------

template<typename T>
void clipperlib::Point<T>::Rotate(const PointD & center,
  double sin_a, double cos_a){}

//------------------------------------------------------------------------------
// Rect
//------------------------------------------------------------------------------

template <typename T>
void Rect<T>::Intersect(const Rect<T> &rect) {
	if (IsEmpty())
		return;
	else if (rect.IsEmpty()) {
		*this = Rect();
	} else {
		left = std::max(rect.left, left);
		right = std::min(rect.right, right);
		top = std::max(rect.top, top);
		bottom = std::min(rect.bottom, bottom);
		if (IsEmpty())
			*this = Rect();
	}
}
//------------------------------------------------------------------------------

template <>
void RectI::Rotate(double angle_rad) {
	PointD cp;
	PathD pts;

	cp.x = double(right + left) / 2;
	cp.y = double(bottom + top) / 2;

	pts.resize(4);
	pts[0] = PointD((double)left, (double)top);
	pts[1] = PointD((double)right, (double)top);
	pts[2] = PointD((double)right, (double)bottom);
	pts[3] = PointD((double)left, (double)bottom);
	pts.Rotate(cp, angle_rad);

	left = (cInt)floor(pts[0].x);
	top = (cInt)floor(pts[0].y);
	right = (cInt)ceil(pts[0].x);
	bottom = (cInt)ceil(pts[0].y);

	for (const auto &p : pts.data) {
		if (p.x < left) left = (cInt)floor(p.x);
		if (p.y < top) top = (cInt)floor(p.y);
		if (p.x > right) right = (cInt)ceil(p.x);
		if (p.y > bottom) bottom = (cInt)ceil(p.y);
	}
}
//------------------------------------------------------------------------------

template <>
void RectD::Rotate(double angle_rad) {
	PointD cp;
	PathD pts;

	cp.x = double(right + left) / 2;
	cp.y = double(bottom + top) / 2;

	pts.resize(4);
	pts[0] = PointD(left, top);
	pts[1] = PointD(right, top);
	pts[2] = PointD(right, bottom);
	pts[3] = PointD(left, bottom);

	pts.Rotate(cp, angle_rad);
	left = pts[0].x;
	top = pts[0].y;
	right = pts[0].x;
	bottom = pts[0].y;

	for (const auto &point : pts.data) {
		if (point.x < left) left = point.x;
		if (point.y < top) top = point.y;
		if (point.x > right) right = point.x;
		if (point.y > bottom) bottom = point.y;
	}
}
//------------------------------------------------------------------------------

template<typename T>
inline void Rect<T>::Rotate(double angle_rad) {}
//------------------------------------------------------------------------------

template <typename T>
void Rect<T>::Union(const Rect<T> &rect) {
	if (rect.IsEmpty())
		return;
	else if (IsEmpty()) {
		*this = rect;
		return;
	}
	left = std::min(rect.left, left);
	right = std::max(rect.right, right);
	top = std::min(rect.top, top);
	bottom = std::max(rect.bottom, bottom);
}

//------------------------------------------------------------------------------
// Path
//------------------------------------------------------------------------------

template <>
PathI::Path(const PathI &other, double scale) {
	if (scale == 0) scale = 1;
	if (scale == 1) {
		Append(other);
	} else {
		data.reserve(other.size());
		for (const auto &p : other.data)
			data.push_back(PointI((cInt)round(p.x * scale), (cInt)round(p.y * scale)));
	}
}
//------------------------------------------------------------------------------

template <>
PathI::Path(const PathD &other, double scale) {
	if (scale == 0) scale = 1;
	data.reserve(other.size());
	for (const auto &p : other.data)
		data.push_back(PointI((cInt)round(p.x * scale), (cInt)round(p.y * scale)));
}
//------------------------------------------------------------------------------

template <>
PathD::Path(const PathI &other, double scale) {
	if (scale == 0) scale = 1;
	data.reserve(other.size());
	for (const auto &p : other.data)
		data.push_back(PointD(p.x * scale, p.y * scale));
}
//------------------------------------------------------------------------------

template <>
PathD::Path(const PathD &other, double scale) {
	if (scale == 0) scale = 1;
	if (scale == 1) {
		Append(other);
	} else {
		data.reserve(other.size());
		for (const auto &p : other.data)
			data.push_back(PointD(p.x * scale, p.y * scale));
	}
}
//------------------------------------------------------------------------------

template<typename T>
clipperlib::Path<T>::Path(const PathI & other, double scale){}
//------------------------------------------------------------------------------

template<typename T>
clipperlib::Path<T>::Path(const PathD & other, double scale){}
//------------------------------------------------------------------------------

template <typename T>
void Path<T>::Append(const Path<T> &extra) {
  if (extra.size() > 0)
    data.insert(end(data), begin(extra.data), end(extra.data));
}
//------------------------------------------------------------------------------

template <>
void PathI::Assign(const PathI &other, double scale) {
  if (&other == this)
    throw ClipperLibException("Can't assign self to self in Path<T>::Assign.");
  data.clear();
	if (scale == 0.0 || scale == 1.0) {
    Append(other);
  } else {
		data.reserve(other.size());
		for (const auto &p : other.data)
			data.push_back(PointI((cInt)round(p.x * scale), (cInt)round(p.y * scale)));
	}
}
//------------------------------------------------------------------------------

template <>
void PathD::Assign(const PathI &other, double scale) {
	data.clear();
	if (scale == 0.0 || scale == 1.0) {
    Append(other);
  } else {
		data.reserve(other.size());
		for (const auto &p : other.data)
			data.push_back(PointD((double)p.x * scale, (double)p.y * scale));
	}
}
//------------------------------------------------------------------------------

template <>
void PathI::Assign(const PathD &other, double scale) {
	data.clear();
	if (scale == 0.0 || scale == 1.0) {
    Append(other);
  } else {
		data.reserve(other.size());
		for (const auto &p : other.data)
			data.push_back(PointI((cInt)round(p.x * scale), (cInt)round(p.y * scale)));
	}
}
//------------------------------------------------------------------------------

template <>
void PathD::Assign(const PathD &other, double scale) {
  if (&other == this)
    throw ClipperLibException("Can't assign self to self in Path<T>::Assign.");
  data.clear();
	if (scale == 0.0 || scale == 1.0) {
    Append(other);
  } else {
		data.reserve(other.size());
		for (const auto &p : other.data)
			data.push_back(PointD(p.x * scale, p.y * scale));
	}
}
//------------------------------------------------------------------------------

template<typename T>
void clipperlib::Path<T>::Assign(const PathI & other, double scale){}
//------------------------------------------------------------------------------

template<typename T>
void clipperlib::Path<T>::Assign(const PathD & other, double scale){}
//------------------------------------------------------------------------------

template <typename T>
double Path<T>::Area() const {
	double area = 0.0;
	auto len = data.size() - 1;
	if (len < 3) return area;
	auto j = len - 1;
	for (decltype(len) i = 0; i < len; ++i) {
		double d = (double)(data[j].x + data[i].x);
		area += d * (data[j].y - data[i].y);
		j = i;
	}
	return -area * 0.5;
}
//------------------------------------------------------------------------------

template <typename T>
Rect<T> Path<T>::Bounds() const {
	const T _MAX = std::numeric_limits<T>::max();
	const T _MIN = -_MAX;

	Rect<T> bounds(_MAX, _MAX, _MIN, _MIN);

	for (const auto &point : data) {
		if (point.x < bounds.left) bounds.left = point.x;
		if (point.x > bounds.right) bounds.right = point.x;
		if (point.y < bounds.top) bounds.top = point.y;
		if (point.y > bounds.bottom) bounds.bottom = point.y;
	}

	if (bounds.left >= bounds.right)
		return Rect<T>();
	else
		return bounds;
}
//------------------------------------------------------------------------------

template <typename T>
void Path<T>::Offset(T dx, T dy) {
	if (dx == 0 && dy == 0) return;
	for (auto &point : data) {
		point.x += dx;
		point.y += dy;
	}
}
//------------------------------------------------------------------------------

template <typename T>
bool Path<T>::Orientation() const {
	return Area() >= 0;
}
//------------------------------------------------------------------------------

template <typename T>
void Path<T>::Reverse() {
	std::reverse(begin(data), end(data));
}
//------------------------------------------------------------------------------

template <typename T>
void Path<T>::Rotate(const PointD &center, double angle_rad) {
	double cos_a = cos(angle_rad);
	double sin_a = sin(angle_rad);

	for (auto &point : data)
		point.Rotate(center, sin_a, cos_a);
}
//------------------------------------------------------------------------------

template <typename T>
void Path<T>::Scale(T sx, T sy) {
	if (sx == 0) sx = 1;
	if (sy == 0) sy = 1;
	if (sx == 1 && sy == 1) return;

	for (auto &point : data) {
		point.x *= sx;
		point.y *= sy;
	}
	StripDuplicates();
}
//------------------------------------------------------------------------------

template <typename T>
void Path<T>::StripDuplicates() {
	data.erase(unique(begin(data), end(data)), end(data));
}

//------------------------------------------------------------------------------
// Paths
//------------------------------------------------------------------------------

template <>
PathsI::Paths(const PathsI &other, double scale) {
  Assign(other, scale);
}
//------------------------------------------------------------------------------

template <>
PathsD::Paths(const PathsI &other, double scale) {
  Assign(other, scale);
}
//------------------------------------------------------------------------------

template <>
PathsI::Paths(const PathsD &other, double scale) {
  Assign(other, scale);
}
//------------------------------------------------------------------------------

template <>
PathsD::Paths(const PathsD &other, double scale) {
  Assign(other, scale);
}
//------------------------------------------------------------------------------

template<typename T>
clipperlib::Paths<T>::Paths(const PathsI & other, double scale){}
//------------------------------------------------------------------------------

template<typename T>
clipperlib::Paths<T>::Paths(const PathsD & other, double scale){}
//------------------------------------------------------------------------------

template <typename T>
void Paths<T>::Append(const Paths<T> &extra) {
  if (extra.size() > 0)
    data.insert(end(data), begin(extra.data), end(extra.data));
}
//------------------------------------------------------------------------------

template <>
void PathsI::Assign(const PathsI &other, double scale) {
	using namespace std;
	data.clear();
	data.resize(other.data.size());
	typename vector<PathI>::iterator it1;
	typename vector<PathI>::const_iterator it2;
	for (it1 = data.begin(), it2 = other.data.begin(); it1 != data.end(); it1++, it2++)
		it1->Assign(*it2, scale);
}
//------------------------------------------------------------------------------

template <>
void PathsD::Assign(const PathsI &other, double scale) {
	using namespace std;
	data.clear();
	data.resize(other.data.size());
	typename vector<PathD>::iterator it1;
	typename vector<PathI>::const_iterator it2;
	for (it1 = data.begin(), it2 = other.data.begin(); it1 != data.end(); it1++, it2++)
		it1->Assign(*it2, scale);
}
//------------------------------------------------------------------------------

template <>
void PathsI::Assign(const PathsD &other, double scale) {
	using namespace std;
	data.clear();
	data.resize(other.data.size());
	typename vector<PathI>::iterator it1;
	typename vector<PathD>::const_iterator it2;
	for (it1 = data.begin(), it2 = other.data.begin(); it1 != data.end(); it1++, it2++)
		it1->Assign(*it2, scale);
}
//------------------------------------------------------------------------------

template <>
void PathsD::Assign(const PathsD &other, double scale) {
	using namespace std;
	data.clear();
	data.resize(other.data.size());
	typename vector<PathD>::iterator it1;
	typename vector<PathD>::const_iterator it2;
	for (it1 = data.begin(), it2 = other.data.begin(); it1 != data.end(); it1++, it2++)
		it1->Assign(*it2, scale);
}
//------------------------------------------------------------------------------

template<typename T>
void clipperlib::Paths<T>::Assign(const PathsI & other, double scale){}
//------------------------------------------------------------------------------

template<typename T>
void clipperlib::Paths<T>::Assign(const PathsD & other, double scale){}
//------------------------------------------------------------------------------

template <typename T>
Rect<T> Paths<T>::Bounds() const {
	const T _MAX = std::numeric_limits<T>::max();
	const T _MIN = -_MAX;

	Rect<T> bounds(_MAX, _MAX, _MIN, _MIN);

	for (const auto &path : data) {
		for (const auto &point : path.data) {
			if (point.x < bounds.left) bounds.left = point.x;
			if (point.x > bounds.right) bounds.right = point.x;
			if (point.y < bounds.top) bounds.top = point.y;
			if (point.y > bounds.bottom) bounds.bottom = point.y;
		}
	}

	if (bounds.left >= bounds.right)
		return Rect<T>();
	else
		return bounds;
}
//------------------------------------------------------------------------------

template <typename T>
void Paths<T>::Rotate(const PointD &center, double angle_rad) {
	double cos_a = cos(angle_rad);
	double sin_a = sin(angle_rad);

	for (auto &path : data)
		for (auto &point : path.data)
			point.Rotate(center, sin_a, cos_a);
}
//------------------------------------------------------------------------------

template <typename T>
void Paths<T>::Scale(T scale_x, T scale_y) {
	for (auto &path : data)
		path.Scale(scale_x, scale_y);
}
//------------------------------------------------------------------------------

template <typename T>
void Paths<T>::Offset(T dx, T dy) {
	if (dx == 0 && dy == 0) return;
	for (auto &path : data)
		for (auto &point : path.data) {
			point.x += dx;
			point.y += dy;
		}
}
//------------------------------------------------------------------------------

template <typename T>
void Paths<T>::Reverse() {
	for (auto &path : data)
		path.Reverse();
}

//------------------------------------------------------------------------------
// PathsArray
//------------------------------------------------------------------------------

template <typename T>
Rect<T> PathsArray<T>::Bounds() const {
	const T _MAX = std::numeric_limits<T>::max();
	const T _MIN = -_MAX;

	Rect<T> bounds(_MAX, _MAX, _MIN, _MIN);

	for (const auto &paths : data) {
		for (const auto &path : paths.data) {
			for (const auto &point : path.data) {
				if (point.x < bounds.left) bounds.left = point.x;
				if (point.x > bounds.right) bounds.right = point.x;
				if (point.y < bounds.top) bounds.top = point.y;
				if (point.y > bounds.bottom) bounds.bottom = point.y;
			}
		}
	}

	if (bounds.left >= bounds.right)
		return Rect<T>();
	else
		return bounds;
}

//------------------------------------------------------------------------------
// Miscellaneous
//------------------------------------------------------------------------------

PipResult PointInPolygon(const PointI &pt, const PathI &path) {
	int val = 0;
	auto cnt = path.size();
	double d, d2, d3;  // using doubles to avoid possible integer overflow
	PointI ip, ip_next;
	PipResult result = pipOnEdge;

	if (cnt < 3) {
		result = pipOutside;
		return result;
	}
	ip = path[0];
	for (decltype(cnt) i = 1; i < cnt; ++i) {
		if (i < cnt)
			ip_next = path[i];
		else
			ip_next = path[0];

		if (ip_next.y == pt.y) {
			if ((ip_next.x == pt.x) || ((ip.y == pt.y) && ((ip_next.x > pt.x) == (ip.x < pt.x)))) {
				return result;
			}
		}
		if ((ip.y < pt.y) != (ip_next.y < pt.y)) {
			if (ip.x >= pt.x) {
				if (ip_next.x > pt.x) {
					val = 1 - val;
				} else {
					d2 = (double)(ip.x - pt.x);
					d3 = (double)(ip_next.x - pt.x);
					d = d2 * (double)(ip_next.y - pt.y) - d3 * (double)(ip.y - pt.y);
					if (d == 0)
						return result;
					if ((d > 0) == (ip_next.y > ip.y))
						val = 1 - val;
				}
			} else {
				if (ip_next.x > pt.x) {
					d2 = (double)(ip.x - pt.x);
					d3 = (double)(ip_next.x - pt.x);
					d = d2 * (double)(ip_next.y - pt.y) - d3 * (double)(ip.y - pt.y);
					if (d == 0)
						return result;
					if ((d > 0) == (ip_next.y > ip.y))
						val = 1 - val;
				}
			}
		}
		ip = ip_next;
	}
	switch (val) {
		case -1: result = pipOnEdge; break;
		case 1: result = pipInside; break;
		default: result = pipOutside; break;
	}
	return result;
}
//------------------------------------------------------------------------------

double CrossProduct(const PointI &pt1, const PointI &pt2, const PointI &pt3) {
	double x1 = double(pt2.x - pt1.x);
	double y1 = double(pt2.y - pt1.y);
	double x2 = double(pt3.x - pt2.x);
	double y2 = double(pt3.y - pt2.y);
	return (x1 * y2 - y1 * x2);
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

}  // namespace clipperlib
