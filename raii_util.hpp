
#pragma once

#include <cstdlib>
#include <utility>

template <typename ArgType>
struct __freeer_traits
{
    typedef void (*pod_free)(ArgType*);
};

// NOTE: pod_free 不是一个类型，它就是一个函数本身！
template<typename PODType, __freeer_traits<PODType>::pod_free pod_free>
struct raii_pod
{
	raii_pod(const raii_pod&) = delete;
	raii_pod(raii_pod&&) = delete;

	PODType _managed_resource;

	~raii_pod()
	{
        pod_free(&_managed_resource);
	}

	template<typename Initializer>
	raii_pod(Initializer&& initdata)
		: _managed_resource(std::forward<Initializer>(initdata))
	{
	}

	raii_pod()
	{
	}

	PODType* operator&()
	{
		return &_managed_resource;
	}

	const PODType* operator&() const
	{
		return &_managed_resource;
	}

	PODType* operator->()
	{
		return &_managed_resource;
	}
};
