

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#include "nowide/convert.hpp"
#else
#include <unistd.h>
#endif

#include <type_traits>
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include <glob.h>

#include "nowide/iostream.hpp"
#include "nowide/args.hpp"

#include "container_util.hpp"

#include "raii_util.hpp"

// element_type 可以是 std::string, std::filesystem::path, boost::filesystem::path
template<typename element_type = std::filesystem::path>
static auto glob(std::string pattern)
{
	raii_pod<glob_t, &globfree> glob_result;

	::glob(reinterpret_cast<const char*>(pattern.c_str()), GLOB_ERR | GLOB_MARK | GLOB_NOSORT | GLOB_NOESCAPE, nullptr, &glob_result);

	return map<std::vector>(as_container(glob_result->gl_pathv, glob_result->gl_pathc), [](const auto& gl_path)
	{
		return element_type{std::string{gl_path}};
	});
}

template<template<typename...> typename  Container = std::vector>
Container<int> find_digi_for_two_string(const std::string& a, const std::string& b)
{
	Container<int> ret;
	for (auto i = 0; (i < a.size()) && (i < b.size()); i++)
	{
		if (std::isdigit(a[i]) && std::isdigit(b[i]))
		{
			char* remain = nullptr;
			auto episode_a = std::strtol(&(a[i]), &remain, 10);
			ret.push_back(i);
			if (a[i] != b[i])
			{
				ret.push_back(i);
			}
			if (remain)
			{
				i = remain - a.c_str() - 1;
			}
		}
	}
	return ret;
}

template<ContainerType Container>
auto find_digi_for_episode(Container&& list)
{
	// 其实就是输出第几个 数字序列，表示 第几集 的意思.
	auto list_size = std::distance(std::begin(list), std::end(list));
	if (list_size < 2)
	{
		return 0;
	}

	std::vector<int> diff_idx_array;

	// 方法就是查找文件名的差异部分的最大值
	// 进行 两两 比对
	for (int a = 0; a < list_size; a++)
	{
		for (int b = 0; b < list_size; b++)
		{
			if (a != b)
			{
				auto list_iter_a = std::begin(list);
				auto list_iter_b = std::begin(list);
				std::advance(list_iter_a, a);
				std::advance(list_iter_b, b);

				auto file1 = *list_iter_a;
				auto file2 = *list_iter_b;

				auto diff_idx = find_digi_for_two_string(file1.string(), file2.string());

				diff_idx_array = concat(diff_idx_array, diff_idx);
			}
		}
	}

	std::map<int, int> idx_count_map;

	// 统计下标和次数

	for (auto diff_idx : diff_idx_array)
	{
		idx_count_map[diff_idx] += idx_count_map.count(diff_idx);
	}

	std::vector<std::pair<int, int>> idx_counts;
	for (auto& pair : idx_count_map)
	{
		idx_counts.push_back(pair);
	}

	std::sort(idx_counts.begin(), idx_counts.end(), [](auto& a, auto& b) { return a.second < b.second; });
	// 选择 出现次数最最最多的数字
	return idx_counts.rbegin()->first;
}

// 将 文件名 给拆分成一个一个的“段落”
// 每个段落进行单独的比较大小
// 比如 ABC-第2集.mp4
// 拆分成 'ABC', '-', '第', 2, '集','.', 'mp4'
// 比如 ABC-第11集.mp4
// 拆分成 'ABC', '-', '第', 11, '集','.', 'mp4'
// 那么，进行比较的时候， 依次对每个分段进行比较
// 自然比较到 11 > 2 的时候，比较就结束了。
// 这样，比纯 ascii 码，第2集 就一定会排在 11 集的前面。
template<bool reverse = false>
struct filename_human_compare
{
	bool operator()(const std::string& a, const std::string& b)
	{
		int idx = 0;
		do
		{
			// 接下来是数字，直接比较数字大小
			if (std::isdigit(a[idx]) && std::isdigit(b[idx]))
			{

				auto digit_a = std::strtol(&(a[idx]), nullptr, 10);
				auto digit_b = std::strtol(&(b[idx]), nullptr, 10);
				if (digit_a != digit_b)
				{
					if constexpr (reverse)
						return digit_a >= digit_b;
					else
						return digit_a < digit_b;
				}
			}

			if (a[idx] != b[idx])
			{
				if constexpr (reverse)
					return a[idx] >= b[idx];
				else
					return a[idx] < b[idx];
			}

			idx ++;

		}while( idx < a.length() && idx < b.length() );

		if constexpr (reverse)
			return a.length() >= b.length();
		else
			return a.length() < b.length();
	}

	bool operator()(const std::filesystem::path& a, const std::filesystem::path& b)
	{
		return(*this)(a.string(), b.string());
	}

};

template<ContainerType Container> auto get_base_names(Container&& files)
{
	std::decay_t<Container> ret;

	for (auto f : files)
	{
		ret.push_back(std::filesystem::path(f).stem().string());
	}

	return ret;
}

struct output
{
	std::ostream& outstream;
	bool is_tty;
};

template<ContainerType Container>
void do_outputs(Container&& files)
{

	std::vector<output> outputs;

	std::ostream* output = nullptr;

	std::ofstream m3u8;

	bool is_tty = isatty(1);


	if (!is_tty)
	{
		outputs.push_back({std::cout, is_tty});
		nowide::cout << "#EXTM3U" << std::endl;
	}
	else
	{
		outputs.push_back({nowide::cout, is_tty});
		m3u8.open("000-playlist.m3u8");
		m3u8 << "#EXTM3U" << std::endl;
		m3u8 << "#EXT-X-TITLE: auto-play-all" << std::endl;
		outputs.push_back({m3u8, false});
	}

	// 寻找表征 第几集 的数字所在的位置，用来进行变色打印
	auto file_names = get_base_names(files);
	auto digi_for_episode = find_digi_for_episode(file_names);

	for (const auto& file : files)
	{
		std::string f = file.string();

		for (auto out : outputs)
		{
			if (out.is_tty)
			{
				if (digi_for_episode < f.size())
				{
					// output color full digit
					out.outstream << f.substr(0, digi_for_episode);

					auto remain_pos	= digi_for_episode;
					out.outstream << "\033[0;35m" << "\u001b[1m";
					while( std::isdigit(f[remain_pos]))
					{
						out.outstream << f[remain_pos];
						remain_pos++;
					}

					out.outstream << "\033[0m";
					out.outstream << f.substr(remain_pos);
				}
				else
				{
					out.outstream << f;
				}
				out.outstream << std::endl;
			}
			else
			{
				out.outstream << f << std::endl;
			}
		}
	}
}

#ifdef _WIN32
int chdir(const char* newdir)
{
	return SetCurrentDirectoryW(nowide::widen(newdir).c_str()) ? 0 : 1;
}
#endif

int main(int argc, char** argv, char** env)
{
	bool is_tty = isatty(1);

	nowide::args _args{argc, argv, env};
	// 首先进入到目标目录. 然后列举出所有的视频文件
	if (argc == 2)
	{
		if (is_tty)
		{
			if (chdir(argv[1]) != 0)
			{
				perror("failed to chdir");
				return 2;
			}
		}
	}

	std::string glob_pattern_prefix;

	if (!is_tty)
	{
		glob_pattern_prefix = argv[1];
		glob_pattern_prefix += std::filesystem::path::preferred_separator;
	}

	auto mkvfiles = glob(glob_pattern_prefix + "*.mkv");
	auto mp4files = glob(glob_pattern_prefix + "*.mp4");
	auto files = concat(mkvfiles, mp4files);


	// 进行根据文件名里的自然阿拉伯数字进行排序
	std::sort(std::begin(files), std::end(files), filename_human_compare{});

	// 最后输出 m3u8 格式

	if (files.empty())
	{
		nowide::cerr << "no videos found" << std::endl;
		return 1;
	}

	do_outputs(files);

	return 0;
}
