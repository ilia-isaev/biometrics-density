#include "config.h"

#include "lib378.h"
#include "lodepng.h"
#include "indicators.hpp"

#include <random>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <iostream>
#include <vector>

#include <cstddef>
#include <cstdio>
#include <cstring>
#include <climits>


using namespace std;
using namespace std::filesystem;
using namespace indicators;

struct point_counter
{
	int x   = 0;
	int y   = 0;
	int cnt = 0;

	point_counter() {};
	point_counter(int _x, int _y) : x(_x), y(_y), cnt(1) {};
	void reset() {
		cnt = 0;
	}
	bool operator == (const point_counter& obj) const {
		return obj.x == x && obj.y == y;
	}
	bool operator != (const point_counter& obj) const {
		return obj.x != x || obj.y != y;
	}
	bool operator <  (const point_counter& obj) {
		if (x == obj.x)
			return y < obj.y;
		return x < obj.x;
	}
	void operator ++ () {
		++cnt;
	}
};

std::ostream& operator<<(std::ostream& o, point_counter const& p)
{
	o << std::to_string(p.x) << "," << std::to_string(p.y) << "," << std::to_string(p.cnt);
	return o;
}

void
scan_entry(std::vector<std::filesystem::path>& dst, std::filesystem::directory_iterator di, std::string extension)
{
	for (auto const& entry : di)
	{
		if (entry.is_directory())
			scan_entry(dst, directory_iterator(entry), extension);
		else if (entry.exists() && entry.path().extension() == extension)
			dst.push_back(entry.path());
	}
}

int
check_file_extension(const char* fileext, const char* extension)
{
    return strncmp(fileext, extension, 3);
}

int main(int argc, char* argv[])
{
	int help_opt = 0;
	int vers_opt = 0;
	int vrbs_opt = 0;

	char *file_png = NULL;
	char *file_csv = NULL;
	path dir;

	/* Promgramm option check */
	for (int i = 1; i < argc; ++i)
	{
		const size_t len = strlen (argv[i]);
		if (argv[i][0] == '-')
		{
			for (size_t n = 1; n < len; ++n)
			{
				const char opt = argv[i][n];
				if ('V' == opt)
					vrbs_opt = 1;
				else if ('h' == opt)
					help_opt = 1;
				else if ('v' == opt)
					vers_opt = 1;
				else
				{
					fprintf(stdout, "Bad option:'%s'", argv[i]);
					return 1;
				}
			}
		}
		else if (len > 4 && check_file_extension(argv[i] + len - 3, "png") == 0) {
			file_png = argv[i];
		}
		else if (len > 4 && check_file_extension(argv[i] + len - 3, "csv") == 0) {
			file_csv = argv[i];
		}
		else {
			path root = argv[i];
			if (dir.empty() && is_directory(root))
				dir = root;
		}
	}

	/* Standart imformation output */ 
	if (help_opt)
		fprintf (stdout,
			"Usage: %s [-Vvh] directory density.csv density.png\n"
			"Minutia density tool\n"
			"Create minexiii minutia density .csv file from .incits378 template's\n"
			"   -V      verbose output\n"
			"   -v      show current version\n"
			"   -h      show this help\n"
			, PACKAGE_NAME);
	if (vers_opt)
		fprintf (stdout,
			"   %s tool\n"
			"version:   %s\n"
			"date:      15 Jan 2024\n", PACKAGE_NAME, PACKAGE_VERSION);
	if (help_opt || vers_opt)
		return 0;
	
	if (vrbs_opt)
		cout << dir << endl;


	if (!dir.empty())
	{
		indicators::ProgressBar bar {
			option::BarWidth{50},
			option::Start{"["},
			option::Fill{"="},
			option::Lead{">"},
			option::Remainder{" "},
			option::End{"]"},
			option::ForegroundColor{Color::green},
			option::FontStyles{std::vector<FontStyle>{FontStyle::bold}}
		};

		vector<path> tmpl;
		scan_entry(tmpl, directory_iterator(dir), ".incits378");

		//const int grid_size = 9;

		int min_cnt = INT_MAX;
		int max_cnt = 0;
		int max_w = 0;
		int max_h = 0;

		vector<point_counter> pcnt_acl;

		size_t tic_cnt     = 0;
		size_t tic_acl     = 0;
		size_t one_tic_num = tmpl.size()/100;

		cout << "handle template..." << endl;
		show_console_cursor(false);
		for (auto const& file_path : tmpl)
		{
			if (tic_cnt++ == one_tic_num)
			{
				bar.set_option(option::PostfixText{
					to_string(tic_acl) + "/" + to_string(tmpl.size())
				});
				bar.tick();
				tic_acl += tic_cnt;
				tic_cnt  = 0;
			}

			if (vrbs_opt)
				cout << file_path << endl;

			ifstream              ifbuff(file_path.c_str(), std::ios::binary);
			vector<unsigned char> templ (istreambuf_iterator<char>(ifbuff), {});

			lib378_blk_s b378 = incits378_dimension_block(&templ[0]);
			if (b378.err)
				continue;

			incits378_image_sz_s img_sz = incits378_image_size(b378.blk);

			if (max_w < img_sz.w)
				max_w = img_sz.w;
			if (max_h < img_sz.h)
				max_h = img_sz.h;

			b378 = incits378_minutiae_block(&templ[0]);
			if (b378.err)
				continue;

			// --- pseudo random move each minutia to remove grid ---
			//std::uniform_int_distribution<int> distribution(0, grid_size*grid_size-1);
			// --- ---

			for (int i = 0; i < b378.blk.num; ++i)
			{
				incits378_minutiae_s m = incits378_minutiae(b378.blk, i);
				//cout << to_string(m.x) << "," << to_string(m.y) << endl;

				// --- pseudo random move each minutia to remove grid ---
				/*mt19937 gen(i);
				const auto val = distribution(gen);

				int xx = int(val / grid_size) -(grid_size-1)/2;
				int yy = int(val % grid_size) -(grid_size-1)/2;

				if (!(m.x + xx < 0 || m.y + yy < 0 || m.x + xx >= img_sz.w || m.y + yy >= img_sz.h))
				{
					m.x += xx;
					m.y += yy;
				}*/
				// --- ---

				pcnt_acl.push_back(point_counter(m.x, m.y));
			}
		}
		bar.set_option(option::PostfixText{
					to_string(tmpl.size()) + "/" + to_string(tmpl.size())
				});
		bar.tick();
		//bar.mark_as_completed();
		show_console_cursor(true);

		cout << "calculate density..." << endl;
		sort(pcnt_acl.begin(), pcnt_acl.end());

		vector<point_counter> density;

		auto cur = pcnt_acl.begin();
		for (auto it = pcnt_acl.begin()+1; it != pcnt_acl.end(); ++it)
		{
			if (*cur == *it)
				++(*cur);
			else
			{
				density.push_back(*cur);
				cur = it;
			}
		}
		if (density.back() != *cur)
			density.push_back(*cur);

		cout << "all tempalte: " << tmpl.size() << " density sz: " << density.size() << endl;
		cout << "area sz: " << to_string(max_w) << "x" << to_string(max_h) << endl;

		ofstream dout(file_csv);
		dout << "x,y,count" << endl;
		for (auto &d : density)
		{
			dout << d << endl;

			if (min_cnt > d.cnt)
				min_cnt = d.cnt;
			if (max_cnt < d.cnt)
				max_cnt = d.cnt;
		}

		vector<unsigned char> raw;
		raw.resize(max_w*max_h);

		memset(&raw[0], 0, max_w*max_h);

		const double scale = 255.0 / (max_cnt - min_cnt);
		for (auto &d : density)
		{
			size_t px  = d.y*max_w+d.x;
			double val = (d.cnt-min_cnt)*scale;

			raw[px] = val;
		}

		unsigned error = lodepng_encode_file(file_png, &raw[0], max_w, max_h, LCT_GREY, 8);
		if (error)
			cerr << file_png << " error:" << lodepng_error_text(error) << endl;
	}

	return 0;
}

