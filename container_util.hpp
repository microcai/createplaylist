
#pragma once

#include <type_traits>
#include <utility>
#include <iterator>
#include <algorithm>

template<typename T>
concept ContainerType = requires(T a) {
	std::begin(a);
	std::end(a);
};

template<typename T>
concept STLContainerType = requires(T a) {
	a.begin();
	a.end();
	a.size();
	std::back_inserter(a);
};


template<STLContainerType Container>
auto concat(Container&& list1, Container&& list2)
{
	std::decay_t<Container> ret(list1.get_allocator());
	ret.reserve(list1.size() + list2.size());
	std::copy(std::begin(list1), std::end(list1), std::back_inserter(ret));
	std::copy(std::begin(list2), std::end(list2), std::back_inserter(ret));
	return ret;
}

template<template<typename...> typename ResultContainer, ContainerType Container, typename Transformer>
auto map(Container&& list1, Transformer&& transformer)
{
	ResultContainer<std::invoke_result_t<Transformer, typename std::decay_t<Container>::value_type> > ret;
	std::transform(std::begin(list1), std::end(list1), std::back_inserter(ret), std::forward<Transformer>(transformer));
	return ret;
}

template<STLContainerType Container>
auto as_container(Container&& c, int size)
{
	return std::forward<Container>(c);
}

template<typename ElementType>
auto as_container(ElementType * c, int size)
{
	struct array_wrap
	{
		ElementType* _array;
		int _size;
		using value_type = ElementType;

		struct iterator
		{
			array_wrap& parent;
			int cur_pos;

			ElementType& operator * ()
			{
				return parent._array[cur_pos];
			}

			bool operator == (const iterator& other)
			{
				return cur_pos == other.cur_pos;
			}

			iterator& operator +(int inc) {
				cur_pos += inc;
				return *this;
			}

			iterator& operator ++() {
				cur_pos ++;
				return *this;
			}

		};

		iterator begin()
		{
			return iterator{*this, 0};
		}

		iterator end()
		{
			return iterator{*this, _size};
		}

		int size() const { return _size;}

	};
	return array_wrap{c, size};
}
