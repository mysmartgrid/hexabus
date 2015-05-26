#ifndef INCLUDE_UTIL_RANGE_HPP_4229B6E0C2C8659C
#define INCLUDE_UTIL_RANGE_HPP_4229B6E0C2C8659C

namespace hbt {
namespace util {

template<typename It>
class range {
	private:
		It _begin, _end;

	public:
		typedef It iterator;

		range(It begin, It end)
			: _begin(begin), _end(end)
		{}

		template<typename Range>
		range(Range& r)
			: _begin(r.begin()), _end(r.end())
		{}

		template<typename Range>
		range(const Range& r)
			: _begin(r.begin()), _end(r.end())
		{}

		It begin() const { return _begin; }
		It end() const { return _end; }
};

template<typename It>
range<It> make_range(It begin, It end)
{
	return { begin, end };
}

template<typename Container>
auto reverse_range(const Container& c)
	-> range<decltype(c.rbegin())>
{
	return { c.rbegin(), c.rend() };
}

template<typename Container>
auto reverse_range(Container& c)
	-> range<decltype(c.rbegin())>
{
	return { c.rbegin(), c.rend() };
}

}
}

#endif
