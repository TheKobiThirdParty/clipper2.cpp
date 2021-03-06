#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

#include <cassert>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#include "windows.h"
#include "clipper.h"
#include "clipper_core.h"
#include "svg.h"


const wchar_t filename[] = L".\\tests.txt";

using namespace std;
using namespace clipper2;

//------------------------------------------------------------------------------
// Timer
//------------------------------------------------------------------------------

struct Timer {
	_LARGE_INTEGER qpf, qpc1, qpc2;
	void Start() {
		QueryPerformanceFrequency(&qpf);
		QueryPerformanceCounter(&qpc1);
	}
	double Stop() {
		QueryPerformanceCounter(&qpc2);
		double elapsed = double(qpc2.QuadPart - qpc1.QuadPart) / qpf.QuadPart;
		return elapsed;
	}
};

//------------------------------------------------------------------------------
// Miscellaneous functions
//------------------------------------------------------------------------------

void MakeRandomPath(PathI &path, int width, int height, unsigned vertCnt) {
	//stress_factor > 1 causes more frequent complex intersections
	const int stress_val = 10;
	path.resize(vertCnt);
	int w = width / stress_val, h = height / stress_val;
	for (unsigned i = 0; i < vertCnt; ++i) {
		path[i].x = (rand() % w) * stress_val;
		path[i].y = (rand() % h) * stress_val;
	}
}
//---------------------------------------------------------------------------

void Trim(std::string &s) {
	s.erase(s.find_last_not_of(" \n\r\t") + 1);
	s.erase(0, s.find_first_not_of(" \n\r\t"));
}
//------------------------------------------------------------------------------

void LoadPaths(std::ifstream &file_stream, PathsI &paths) {
	//format assumes coordinates for each path are
	//on the same line and are space and or comma separated

	paths.clear();
	string line;
	PathI path;
	std::streamoff file_pos = file_stream.tellg();
	while (getline(file_stream, line)) {
		Trim(line);
		stringstream ss(line);
		for (;;) {
			double x = 0.0, y = 0.0;
			if (!(ss >> x)) break;
			char c = ss.peek();
			while (c == ' ') {
				ss.read(&c, 1);
				c = ss.peek();
			}  //gobble spaces before comma
			if (c == ',') {
				ss.read(&c, 1);
				c = ss.peek();
			}  //gobble comma
			while (c == ' ') {
				ss.read(&c, 1);
				c = ss.peek();
			}  //gobble spaces after comma
			if (!(ss >> y)) break;  //oops!
			path.push_back(PointI((cInt)(x), (cInt)(y)));
		}
		if (path.size() == 0) break;
		paths.push_back(path);
		path.clear();
		file_pos = file_stream.tellg();
	}
	file_stream.seekg(file_pos);
}
//------------------------------------------------------------------------------

std::ifstream *OpenFileStreamAtTestNum(const wchar_t* filename, int testNum = 1) {
	ifstream *file_stream = new ifstream(filename);
	if (!file_stream) return NULL;
	string line;
	std::streamoff file_pos = 0;
	while (getline(*file_stream, line)) {
		if (line.find("CAPTION:") != string::npos) --testNum;
		if (testNum <= 0) {
			file_stream->seekg(file_pos);
			return file_stream;
		}
		file_pos = file_stream->tellg();
	}
	file_stream->close();
	delete file_stream;
	return NULL;
}
//------------------------------------------------------------------------------

bool LoadFromStream(std::ifstream &file_stream,
		PathsI &subj, PathsI &clip, ClipType &ct, FillRule &fr) {
	subj.clear();
	clip.clear();
	string line;
	std::streamoff file_pos = file_stream.tellg();
	bool result = false;
	for (;;) {
		if (!getline(file_stream, line)) break;
		if (line.find("CAPTION") == string::npos) continue;
		if (!getline(file_stream, line) || line.find("CLIPTYPE") == string::npos) break;
		line.erase(0, line.find(":") + 1);
		Trim(line);
		switch (line[0]) {
			case 'I': ct = ctIntersection; break;
			case 'U': ct = ctUnion; break;
			case 'D': ct = ctDifference; break;
			case 'X': ct = ctXor; break;
			default: ct = ctNone;
		}
		if (!getline(file_stream, line) || line.find("FILLRULE") == string::npos) break;
		line.erase(0, line.find(":") + 1);
		Trim(line);
		switch (line[0]) {
			case 'E': fr = frEvenOdd; break;
			case 'N': fr = frNonZero; break;
			case 'P': fr = frPositive; break;
			default: fr = frEvenOdd;
		}
		result = true;
		file_pos = file_stream.tellg();
		if (!getline(file_stream, line) || line.find("SUBJECTS") == string::npos) break;
		LoadPaths(file_stream, subj);
		file_pos = file_stream.tellg();
		if (!getline(file_stream, line) || line.find("CLIPS") == string::npos) break;
		LoadPaths(file_stream, clip);
		file_pos = file_stream.tellg();
		break;
	}
	file_stream.seekg(file_pos);
	return result;
}
//------------------------------------------------------------------------------

bool LoadFromFile(const wchar_t* filename, int index,
		PathsI &subj, PathsI &clip, ClipType &ct, FillRule &fr) {
	subj.clear();

	ifstream *file_stream = OpenFileStreamAtTestNum(filename, index);
	if (!file_stream) return false;
	bool result = LoadFromStream(*file_stream, subj, clip, ct, fr);
	file_stream->close();
	delete file_stream;
	return result;
}
//------------------------------------------------------------------------------

void AddPolyNodeToPaths(PolyTreeI &pp, PathsI &paths) {
	if (pp.Path().size() > 2)
		paths.push_back(pp.Path());
	for (unsigned i = 0; i < pp.ChildCount(); ++i) {
		PolyTreeI &child = pp.Child(i);
		AddPolyNodeToPaths(child, paths);
	}
}
//------------------------------------------------------------------------------

void PolyTreeToPaths(PolyTreeI &pt, PathsI &paths) {
	paths.clear();
	AddPolyNodeToPaths(pt, paths);
}
//------------------------------------------------------------------------------

void AddPolyNodeToPathsD(PolyTreeD &pp, PathsD &paths) {
	if (pp.Path().size() > 2)
		paths.push_back(pp.Path());
	for (unsigned i = 0; i < pp.ChildCount(); ++i) {
		PolyTreeD &child = pp.Child(i);
		AddPolyNodeToPathsD(child, paths);
	}
}
//------------------------------------------------------------------------------

void PolyTreeToPathsD(PolyTreeD &pt, PathsD &paths) {
	paths.clear();
	AddPolyNodeToPathsD(pt, paths);
}

//------------------------------------------------------------------------------
//Clipper Test functions
//------------------------------------------------------------------------------

void test_path_i() {
	std::clog << "Testing PathI - Offset(), Reverse(), Scale()" << std::endl;

	PathI path_i;
	path_i << PointI(0, 0) << PointI(100, 0)
		   << PointI(100, 100) << PointI(0, 100);

	path_i.Offset(10, 10);

	assert(path_i[0] == PointI(10, 10));
	assert(path_i[1] == PointI(110, 10));
	assert(path_i[2] == PointI(110, 110));
	assert(path_i[3] == PointI(10, 110));

	path_i.Scale(2, 2);

	assert(path_i[0] == PointI(20, 20));
	assert(path_i[1] == PointI(220, 20));
	assert(path_i[2] == PointI(220, 220));
	assert(path_i[3] == PointI(20, 220));

	path_i.Reverse();

	assert(path_i[0] == PointI(20, 220));
	assert(path_i[1] == PointI(220, 220));
	assert(path_i[2] == PointI(220, 20));
	assert(path_i[3] == PointI(20, 20));
}
//------------------------------------------------------------------------------

void test_path_d() {
	std::clog << "Testing PathD - Offset(), Reverse(), Scale()" << std::endl;

	PathD path_d;
	path_d << PointD(0.5, 0.5) << PointD(100.5, 0.5)
		   << PointD(100.5, 100.5) << PointD(0.5, 100.5);

	path_d.Offset(10.0, 10.0);

	assert(path_d[0] == PointD(10.5, 10.5));
	assert(path_d[1] == PointD(110.5, 10.5));
	assert(path_d[2] == PointD(110.5, 110.5));
	assert(path_d[3] == PointD(10.5, 110.5));

	path_d.Reverse();

	assert(path_d[0] == PointD(10.5, 110.5));
	assert(path_d[1] == PointD(110.5, 110.5));
	assert(path_d[2] == PointD(110.5, 10.5));
	assert(path_d[3] == PointD(10.5, 10.5));

	path_d.Scale(2.0, 2.0);

	assert(path_d[0] == PointD(21, 221));
	assert(path_d[1] == PointD(221, 221));
	assert(path_d[2] == PointD(221, 21));
	assert(path_d[3] == PointD(21, 21));
}
//------------------------------------------------------------------------------

void test_path_conversions() {
	std::clog << "Testing PathD to PathI conversions" << std::endl;

	PathD path_d;
	path_d << PointD(0.5, 0.5) << PointD(100.5, 0.5)
		   << PointD(100.5, 100.5) << PointD(0.5, 100.5);

	PathI path_i;
	path_i = path_d;

	assert(path_i[3] == PointI(1, 101));

	PathI path_i2(path_d, 2);

	assert(path_i2[3] == PointI(1, 201));  //0.5 * 2, 100.5 * 2
}
//------------------------------------------------------------------------------

void test_point() {
	std::clog << "Testing PointI & PointD - Rotate()" << std::endl;

	// PointI
	PointI point_i(200, 0);
	point_i.Rotate(PointD(100.0, 0.0), PI);

	assert(point_i == PointI(0, 0));
	assert(point_i != PointI(200, 0));

	// PointD
	PointD point_d(200.0, 0.0);
	point_d.Rotate(PointD(100.0, 0.0), 0.0, -1.0);

	assert(point_d == PointD(0.0, 0.0));
	assert(point_d != PointD(200.0, 0.0));
}
//------------------------------------------------------------------------------

void test_rect_i() {
	std::clog << "Testing RectI - IsEmpty(), Inflated(), Union()" << std::endl;

	// RectI
	RectI rect_i(0, 0, 50, 100);
	cInt width = rect_i.Width();
	cInt height = rect_i.Height();

	assert(width == 50);
	assert(height == 100);

	bool empty = rect_i.IsEmpty();

	assert(!empty);

	rect_i.Inflate(10, 10);

	assert(rect_i.top == -10);
	assert(rect_i.left == -10);
	assert(rect_i.right == 60);
	assert(rect_i.bottom == 110);

	RectI rect_i2(100, 100, 200, 200);

	rect_i.Union(rect_i2);

	assert(rect_i.top == -10);
	assert(rect_i.left == -10);
	assert(rect_i.right == 200);
	assert(rect_i.bottom == 200);
}
//------------------------------------------------------------------------------

void test_rect_d() {
	std::clog << "Testing RectD - IsEmpty(), Inflated(), Union()" << std::endl;

	// RectI
	RectD rect_d(0.0, 0.0, 50.0, 100.0);
	double width = rect_d.Width();
	double height = rect_d.Height();

	assert(width == 50.0);
	assert(height == 100.0);

	bool empty = rect_d.IsEmpty();

	assert(!empty);

	rect_d.Inflate(10.0, 10.0);

	assert(rect_d.top == -10.0);
	assert(rect_d.left == -10.0);
	assert(rect_d.right == 60.0);
	assert(rect_d.bottom == 110.0);

	RectD rect_d2(100.0, 100.0, 200.0, 200.0);

	rect_d.Union(rect_d2);

	assert(rect_d.top == -10.0);
	assert(rect_d.left == -10.0);
	assert(rect_d.right == 200.0);
	assert(rect_d.bottom == 200.0);
}
//------------------------------------------------------------------------------

void test_get_paths_bounds() {
	std::clog << "Testing Path.Bounds() & PathsArray.Bounds()" << std::endl;

	// Bounds int
	PathI path_i;
	path_i << PointI(-100, -100) << PointI(100, -100)
		   << PointI(100, 100) << PointI(-100, 100);

	PathI path_i2;
	path_i2 << PointI(0, 0) << PointI(300, 0)
			<< PointI(300, 300) << PointI(0, 300);

	PathsI paths;
	paths << path_i;
	paths << path_i2;

	RectI bounds = paths.Bounds();

	assert(bounds.top == -100);
	assert(bounds.left == -100);
	assert(bounds.right == 300);
	assert(bounds.bottom == 300);

	PathsArrayI pa_i;
	pa_i.push_back(paths);
	bounds = pa_i.Bounds();

	assert(bounds.top == -100);
	assert(bounds.left == -100);
	assert(bounds.right == 300);
	assert(bounds.bottom == 300);

	// Bounds double
	PathD path_d;
	path_d << PointD(-100.5, -100.5) << PointD(100.5, -100.5)
		   << PointD(100.5, 100.5) << PointD(-100.5, 100.5);

	PathD path_d2;
	path_d2 << PointD(0.5, 0.5) << PointD(300.5, 0.5)
			<< PointD(300.5, 300.5) << PointD(0.5, 300.5);

	PathsD paths_d;
	paths_d << path_d;
	paths_d << path_d2;

	RectD bounds_d = paths_d.Bounds();

	assert(bounds_d.top == -100.5);  // floor
	assert(bounds_d.left == -100.5);  // floor
	assert(bounds_d.right == 300.5);  // ceil
	assert(bounds_d.bottom == 300.5);  // ceil
}
//------------------------------------------------------------------------------

void test_append_paths() {
	std::clog << "Testing Path.Append()" << std::endl;

	PathI path_i;
	path_i << PointI(0, 0) << PointI(100, 0)
		   << PointI(100, 100) << PointI(0, 100);

	PathI path_i2;
	path_i2 << PointI(0, 0) << PointI(200, 0)
			<< PointI(200, 200) << PointI(0, 200);

	PathsI paths;
	paths << path_i;

	PathsI extra;
	extra << path_i2;

	paths.Append(extra);

	assert(paths.size() == 2);
}
//------------------------------------------------------------------------------

void test_strip_duplicates() {
	std::clog << "Testing Path.StripDuplicates()" << std::endl;

	PathI path;
	path << PointI(0, 0) << PointI(100, 0) << PointI(100, 100)
		 << PointI(100, 100) << PointI(0, 100) << PointI(0, 100);

	path.StripDuplicates();

	assert(path.size() == 4);
}
//------------------------------------------------------------------------------

void test_point_in_polygon() {
	std::clog << "Testing PointInPolygon()" << std::endl;

	PathI path;
	path << PointI(0, 0) << PointI(100, 0)
		 << PointI(100, 100) << PointI(0, 100);

	PipResult pip;

	PointI point_in(50, 50);
	pip = PointInPolygon(point_in, path);
	assert(pip == pipInside);

	PointI point_out(150, 150);
	pip = PointInPolygon(point_out, path);
	assert(pip == pipOutside);

	PointI point_on(100, 0);
	pip = PointInPolygon(point_on, path);
	assert(pip == pipOnEdge);
}
//------------------------------------------------------------------------------


void test_clipper_paths_i() {
	std::clog << "Testing Clipper intersection with PathsI" << std::endl;

  const int test_num = 4;
	PathsI subj, clip, solution;
	ClipType ct;
	FillRule fr;
	LoadFromFile(filename, test_num, subj, clip, ct, fr);

	Clipper c;
	c.AddPaths(subj, ptSubject);
	c.AddPaths(clip, ptClip);
	c.Execute(ct, fr, solution);

	assert(solution.size() > 0);

	//std::wstringstream svgName;
	//svgName << ".\\sample" << test_num << ".svg";
	//svglib::SVGBuilder svg;
	//svg.fill_rule = fr;
	//svg.AddPaths(subj, false, 0x100000FF, 0xFF0000FF, 1, false);
	//svg.AddPaths(clip, false, 0x10FFAA00, 0xFF996600, 1, false);
	//svg.AddPaths(solution, false, 0x4000AA00, 0xFF006600, 1, false);
	//svg.SaveToFile(svgName.str(), 800, 600, 80);
	//ShellExecute(0, NULL, svgName.str().c_str(), NULL, NULL, SW_SHOW);
}
//------------------------------------------------------------------------------


void test_clipper_paths_d() {
	std::clog << "Testing Clipper intersection with PathsD" << std::endl;

  const int test_num = 5;
	PathsI subj, clip, solution;
	ClipType ct;
	FillRule fr;
	LoadFromFile(filename, test_num, subj, clip, ct, fr);

	//convert PathsI to PathsD (without any scaling) ...
	PathsD subj_d = PathsD(subj, 1);
	PathsD clip_d = PathsD(clip, 1);
	PathsD solution_d;

	ClipperD c;  //uses default scale (100)
	c.AddPaths(subj_d, ptSubject);
	c.AddPaths(subj_d, ptClip);
	c.Execute(ct, fr, solution_d);

	assert(solution_d.size() > 0);

	//solution.Assign(solution_d, 1.0);
	//std::wstringstream svgName;
	//svgName << ".\\sample" << test_num << ".svg";
	//svglib::SVGBuilder svg;
	//svg.fill_rule = fr;
	//svg.AddPaths(subj, false, 0x100000FF, 0xFF0000FF, 1, false);
	//svg.AddPaths(clip, false, 0x10FFAA00, 0xFF996600, 1, false);
	//svg.AddPaths(solution, false, 0x4000AA00, 0xFF006600, 1, false);
	//svg.SaveToFile(svgName.str(), 800, 600, 80);
	//ShellExecute(0, NULL, svgName.str().c_str(), NULL, NULL, SW_SHOW);
}
//------------------------------------------------------------------------------

void test_clipper_polytree_i() {
	std::clog << "Testing Clipper intersection with PolyTreeI" << std::endl;

  const int test_num = 6;
	PathsI subj, clip, solution;
	ClipType ct;
	FillRule fr;
	LoadFromFile(filename, test_num, subj, clip, ct, fr);

	PolyTreeI pt_i;
	Clipper c;
	c.AddPaths(subj, ptSubject);
	c.AddPaths(clip, ptClip);
	c.Execute(ct, fr, pt_i, solution);

	assert(pt_i.ChildCount() > 0);

	//PolyTreeToPaths(pt_i, solution);
	//std::wstringstream svgName;
	//svgName << ".\\sample" << test_num << ".svg";
	//svglib::SVGBuilder svg;
	//svg.fill_rule = fr;
	//svg.AddPaths(subj, false, 0x100000FF, 0xFF0000FF, 1, false);
	//svg.AddPaths(clip, false, 0x10FFAA00, 0xFF996600, 1, false);
	//svg.AddPaths(solution, false, 0x4000AA00, 0xFF006600, 1, false);
	//svg.SaveToFile(svgName.str(), 800, 600, 80);
	//ShellExecute(0, NULL, svgName.str().c_str(), NULL, NULL, SW_SHOW);
}
//------------------------------------------------------------------------------

void test_clipper_polytree_d() {
	std::clog << "Testing Clipper intersection with PolyTreeD" << std::endl;

	const int test_num = 7;
	PathsI subj, clip, solution;
	ClipType ct;
	FillRule fr;
	LoadFromFile(filename, test_num, subj, clip, ct, fr);

	std::wstringstream svgName;
	svgName << ".\\sample" << test_num << ".svg";

	//convert PathsI to PathsD (without any scaling) ...
	PathsD subj_d = PathsD(subj, 1);
	PathsD clip_d = PathsD(clip, 1);
	PathsD solution_d;

	PolyTreeD pt_d;

	ClipperD c;  //uses default scale (100)
	c.AddPaths(subj_d, ptSubject);
	c.AddPaths(subj_d, ptClip);
	c.Execute(ct, fr, pt_d, solution_d);

	assert(pt_d.ChildCount() > 0);

	//PolyTreeToPathsD(pt_d, solution_d);
	//solution.Assign(solution_d, 1.0);
	//std::wstringstream svgName;
	//svgName << ".\\sample" << test_num << ".svg";
	//svglib::SVGBuilder svg;
	//svg.fill_rule = fr;
	//svg.AddPaths(subj, false, 0x100000FF, 0xFF0000FF, 1, false);
	//svg.AddPaths(clip, false, 0x10FFAA00, 0xFF996600, 1, false);
	//svg.AddPaths(solution, false, 0x4000AA00, 0xFF006600, 1, false);
	//svg.SaveToFile(svgName.str(), 800, 600, 80);
	//ShellExecute(0, NULL, svgName.str().c_str(), NULL, NULL, SW_SHOW);
}
//------------------------------------------------------------------------------
void test_clipper_all() {
	std::clog << "Testing 213 sample clipping operations ... ";

  PathsI subj, clip, solution;
	ClipType ct;
	FillRule fr;

	ifstream *file_stream = OpenFileStreamAtTestNum(filename, 1);
	if (!file_stream) return;
	int cnt = 0;
	while (LoadFromStream(*file_stream, subj, clip, ct, fr)) {
		Clipper c;
		c.AddPaths(subj, ptSubject);
		c.AddPaths(clip, ptClip);
		if (!c.Execute(ct, fr, solution)) break;
		++cnt;
	}
	file_stream->close();
	delete file_stream;
	clog << std::endl
		 << "Finished. " << cnt << " passed." << std::endl;
}
//------------------------------------------------------------------------------

//The table below is an indication of Clipper's performance.
/*==================+==========+==========++==========+==========+==========+
|Edges (per poly.)  | Clipper2 | Clipper1 || Clipper2 | Clipper2 | Clipper1 |
|Language           |   Delphi |   Delphi ||      C++ |      C++ |      C++ |
|32 / 64 bit        |    32bit |    32bit ||    32bit |    64bit |    32bit |
+===================+==========+==========++==========+==========+==========+
| 500               |    0.05  |    0.078 ||    0.08  |    0.07  |    0.08  |
|1000               |    0.22  |    0.60  ||    0.28  |    0.25  |    0.47  |
|2000               |    1.0   |    8.15  ||    1.16  |    1.03  |    4.8   |
|2500               |    2.0   |   20.0   ||    1.98  |    1.76  |   11.0   |
|3000               |    3.6   |   48.8   ||    3.19  |    2.82  |   25.5   |
|3500               |    6.3   |   93.5   ||    5.07  |    4.65  |   47.4   |
|4000               |   11.2   |  173     ||    7.88  |    7.27  |   92.6   |
|4500               |   16.5   |  294     ||   12.0   |   11.7   |  156     |
|5000               |   27.1   |  497     ||   18.9   |   18.3   |  280     |
+===================+==========+==========++==========+==========+=========*/

void test_clipper_performance() {
	PathsI subj, clip, solution;
	Timer t;
  //////////////////////////////////////////
	const int edge_cnt = 100;
	//////////////////////////////////////////

	clog << "Testing time to intersect 2 polygons, each with " <<
    edge_cnt << " random vertices. ";
	if (edge_cnt >= 2000)
	  clog << endl << "Please wait a few seconds ... ";

	subj.resize(1);
	clip.resize(1);
	MakeRandomPath(subj[0], 800, 600, edge_cnt);
	MakeRandomPath(clip[0], 800, 600, edge_cnt);

	t.Start();
	Clipper c;
	c.AddPaths(subj, ptSubject);
	c.AddPaths(clip, ptClip);
	if (!c.Execute(ctIntersection, frEvenOdd, solution)){
		clog << endl << "Oops, an error occurred!" << endl;
		return;
	}

	double elapsed = t.Stop();
	clog << endl << "Finished. The clipping operation took " <<
    setprecision(0) << fixed << elapsed * 1000 << " msecs." << endl;

	if (edge_cnt >= 1000) return;
	wchar_t s[] = L".\\sample.svg";
	svglib::SVGBuilder svg;
	svg.fill_rule = frEvenOdd;
	svg.AddPaths(subj, false, 0x100000FF, 0xFF0000FF, 1, false);
	svg.AddPaths(clip, false, 0x10FFAA00, 0xFF996600, 1, false);
	svg.AddPaths(solution, false, 0x4000AA00, 0xFF006600, 1, false);
	svg.SaveToFile(s, 800, 600, 80);
	ShellExecute(0, NULL, s, NULL, NULL, SW_SHOW);
}
//------------------------------------------------------------------------------

typedef void (*TestFunc)(void);

TestFunc test_funcs[] = {
	test_path_i,
	test_path_d,
	test_point,
	test_rect_i,
	test_rect_d,
	test_get_paths_bounds,
	test_append_paths,
	test_strip_duplicates,
	test_point_in_polygon,
	test_path_conversions,
	test_clipper_paths_i,
	test_clipper_paths_d,
	test_clipper_polytree_i,
	test_clipper_polytree_d,
	test_clipper_all,
	test_clipper_performance,
	0
};
//------------------------------------------------------------------------------

bool CheckFilename(){
  ifstream *file_stream = new ifstream(filename);
	if (!file_stream) return false;

	string line;
	if (!file_stream->is_open()) {
		clog << "Error: unable to open " << filename << "." << endl;
		return false;
	}
	file_stream->close();
	delete file_stream;
	return true;
}
//------------------------------------------------------------------------------

int main() {

  srand((unsigned)time(NULL));
	if (!CheckFilename()) return 1;

	for (int i = 0; test_funcs[i]; ++i)
		test_funcs[i]();

  _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT);
	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDOUT);
	_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDOUT);
	_CrtDumpMemoryLeaks();

	std::clog << std::endl
			  << "Testing finished, press ENTER to close console ... ";
	getwchar();
}
