

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#else
#include <unistd.h>
#endif

#include <algorithm>
#include <cctype>
#include <concepts>
#include <codecvt>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include <glob.h>

#include "nowide/convert.hpp"
#include "nowide/iostream.hpp"
#include "nowide/args.hpp"

template<typename T>
concept ContainerType = requires(T a) {
	std::begin(a);
	std::end(a);
};

template<ContainerType Container>
auto merge(Container&& list1, Container&& list2)
{
	std::decay_t<Container> ret = list1;
	std::copy(std::begin(list2), std::end(list2), std::back_inserter(ret));
	return ret;
}

template<typename resourcetype, typename cleanuper = decltype(&free)>
struct raii_resource
{
	resourcetype _managed_resource;
	cleanuper _cleanuper;

	~raii_resource()
	{
		if (_cleanuper)
		{
			_cleanuper(&_managed_resource);
		}
	}

	raii_resource()
	{
		_cleanuper = nullptr;
	}

	raii_resource(cleanuper cleanuper_)
	{
		_cleanuper = cleanuper_;
	}

	raii_resource(resourcetype _managed_resource) : _managed_resource(_managed_resource)
	{
		_cleanuper = &free;
	}

	raii_resource(resourcetype _managed_resource, cleanuper _cleanuper)
		: _managed_resource(_managed_resource), _cleanuper(_cleanuper)
	{
	}

	raii_resource(raii_resource&) = delete;
	raii_resource(raii_resource&& rvalue) : _managed_resource(rvalue._managed_resource), _cleanuper(rvalue._cleanuper)
	{
		rvalue._cleanuper = nullptr;
	}

	resourcetype* operator&()
	{
		return &_managed_resource;
	}

	const resourcetype* operator&() const
	{
		return &_managed_resource;
	}

	resourcetype* operator->()
	{
		return &_managed_resource;
	}
};

auto glob(std::string pattern)
{
	std::vector<std::filesystem::path> ret;

	raii_resource<glob_t, decltype(&globfree)> glob_result{&globfree};

	::glob(reinterpret_cast<const char*>(pattern.c_str()), GLOB_ERR | GLOB_MARK | GLOB_NOSORT | GLOB_NOESCAPE, nullptr, &glob_result);

	for (auto i = 0; i < glob_result->gl_pathc; i++)
	{
		std::string globed_file{glob_result->gl_pathv[i]};
		ret.push_back(std::filesystem::path{std::move(globed_file)});
	}

	return ret;
}

std::vector<int> find_digi_for_two_string(const std::string& a, const std::string& b)
{
	std::vector<int> ret;
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

				diff_idx_array = merge(diff_idx_array, diff_idx);
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
		m3u8 << "#EXT-X-START" << std::endl;

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

					char* r = nullptr;
					strtoll(&f[digi_for_episode], &r, 10);

					if (r)
					{
						auto remain_pos = r - f.c_str();
						if (out.is_tty)
						{
							out.outstream << "\033[0;35m" << "\u001b[1m";
						}

						out.outstream << f.substr(digi_for_episode, remain_pos - digi_for_episode);

						if (out.is_tty)
						{
							out.outstream << "\033[0m";
						}

						out.outstream << f.substr(remain_pos);
					}
					else
					{
						out.outstream << f.substr(digi_for_episode);
					}
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

	if (output)
	{
		*output << "#EXT-X-ENDLIST" << std::endl;
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
#ifdef _WIN32
		glob_pattern_prefix += '\\';
#else
		glob_pattern_prefix += '/';
#endif
	}

	auto mkvfiles = glob(glob_pattern_prefix + "*.mkv");
	auto mp4files = glob(glob_pattern_prefix + "*.mp4");
	auto files = merge(mkvfiles, mp4files);


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
