
#include <concepts>
#include <algorithm>
#include <vector>
#include <string>
#include <map>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <fstream>

#include <glob.h>
#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

template<typename T>
concept ContainerType = requires (T a)
{
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
            _cleanuper(&_managed_resource);
    }

    raii_resource()
    {
        _cleanuper = nullptr;
    }

    raii_resource(cleanuper cleanuper_)
    {
        _cleanuper = cleanuper_;
    }

    raii_resource(resourcetype _managed_resource)
        : _managed_resource(_managed_resource)
    {
        _cleanuper = &free;
    }

    raii_resource(resourcetype _managed_resource, cleanuper _cleanuper)
        : _managed_resource(_managed_resource)
        , _cleanuper(_cleanuper)
    {}

    raii_resource(raii_resource&) = delete;
    raii_resource(raii_resource&& rvalue)
        : _managed_resource(rvalue._managed_resource)
        , _cleanuper(rvalue._cleanuper)
    {
        rvalue._cleanuper = nullptr;
    }

    resourcetype * operator & ()
    {
        return &_managed_resource;
    }

    const resourcetype * operator & () const
    {
        return &_managed_resource;
    }

    resourcetype* operator -> ()
    {
        return &_managed_resource;
    }

};

auto glob(std::string pattern)
{
    std::vector<std::string> ret;

    raii_resource<glob_t, decltype(&globfree)> glob_result { &globfree };

    ::glob(pattern.c_str(), GLOB_ERR|GLOB_MARK|GLOB_NOSORT|GLOB_NOESCAPE, nullptr, &glob_result);

    for (auto i = 0; i < glob_result->gl_pathc; i ++)
    {
        ret.push_back( std::string { glob_result->gl_pathv[i] } );
    }

    return ret;
}


std::vector<int> find_digi_for_two_string(const std::string& a, const std::string& b)
{
    std::vector<int> ret;
    for (auto i = 0; (i < a.size()) && (i < b.size()); i++)
    {
        if ( std::isdigit(a[i]) && std::isdigit(b[i]))
        {
            char *remain = nullptr;
            auto episode_a = std::strtol( & ( a[i]), &remain, 10);
            ret.push_back(i);
            if ( a[i] != b[i])
                ret.push_back(i);
            if (remain)
                i = remain - a.c_str() - 1;

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
    for (int a = 0; a < list_size; a ++)
    {
        for (int b = 0; b < list_size; b ++)
        {
            if (a != b)
            {
                auto list_iter_a = std::begin(list);
                auto list_iter_b = std::begin(list);
                std::advance(list_iter_a, a);
                std::advance(list_iter_b, b);

                auto file1 = * list_iter_a;
                auto file2 = * list_iter_b;

                auto diff_idx = find_digi_for_two_string(file1, file2);

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
    for (auto & pair : idx_count_map)
    {
        idx_counts.push_back(pair);
    }

    std::sort(idx_counts.begin(), idx_counts.end(), [](auto& a, auto& b){ return a.second < b.second ;});
    // 选择 出现次数最最最多的数字
    return idx_counts.rbegin()->first;
}

struct compare_on
{
    int _idx;
    compare_on(int idx): _idx(idx) {}

    bool operator () (auto a , auto b)
    {
        unsigned cond = 0;
        cond |= std::isdigit(a[_idx]) ? 1 : 0;
        cond |= std::isdigit(b[_idx]) ? 2 : 0;
        switch (cond)
        {
        case 0:
            return a < b;
        case 1:
            return true;
        case 2:
            return false;
        case 3:
            {
                auto episode_a = std::strtol( & ( a[_idx]), nullptr, 10);
                auto episode_b = std::strtol( & ( b[_idx]), nullptr, 10);

                return episode_a < episode_b;
            }
        default:
            return true;
        }

        return false;
    }
};

template<ContainerType Container>
auto get_base_names(Container&& files)
{
    std::decay_t<Container> ret;

    for (auto f : files)
    {
        ret.push_back(std::filesystem::path(f).stem().string());
    }

    return ret;
}

int main(int argc, char* argv[])
{
    // 首先进入到目标目录. 然后列举出所有的视频文件
    auto mkvfiles = glob("*.mkv");
    auto mp4files = glob("*.mp4");
    auto files = merge(mkvfiles, mp4files);

    auto file_names = get_base_names(files);

    // 接着寻找表征 第几集 的数字所在的位置
    auto digi_for_episode = find_digi_for_episode(file_names);
    // 然后根据这个数字排序
    std::sort(std::begin(files), std::end(files), compare_on(digi_for_episode));

    // 最后输出 m3u8 格式

    if (files.empty())
    {
        std::cerr << "no videos found" << std::endl;
        return 1;
    }

    std::ostream * output = nullptr;

    std::ofstream m3u8;

    auto is_pipe = !isatty(1);
    if (is_pipe)
    {
        std::cout << "#EXTM3U" << std::endl;
    }
    else
    {
        m3u8.open("000-playlist.m3u8");

        output = & m3u8;

        m3u8 << "#EXTM3U" << std::endl;
        m3u8 << "#EXT-X-TITLE: auto-play-all" << std::endl;
        m3u8 << "#EXT-X-START" << std::endl;
    }

    for (auto f : files)
    {
        std::cout << f.substr(0, digi_for_episode);

        char * r = nullptr;
        strtoll(& f[digi_for_episode], &r, 10);

        if (r)
        {
            auto remain_pos = r - f.c_str();
            if (!is_pipe)
               std::cout << "\033[0;35m" << "\u001b[1m";

            std::cout << f.substr(digi_for_episode, remain_pos - digi_for_episode);

            if (!is_pipe)
                std::cout << "\033[0m";

            std::cout << f.substr(remain_pos);
        }
        else
        {
            std::cout << f.substr(digi_for_episode);
        }

        std::cout << std::endl;
        if (output)
            *output << f << std::endl;
    }

    if (output)
        *output << "#EXT-X-ENDLIST" << std::endl;

    return 0;
}
