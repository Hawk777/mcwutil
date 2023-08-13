#ifndef NBT_INTEGER_SPAN_H
#define NBT_INTEGER_SPAN_H

#include <mcwutil/nbt/tags.hpp>
#include <mcwutil/util/codec.hpp>
#include <concepts>
#include <cstdint>
#include <iterator>
#include <ranges>
#include <span>

namespace mcwutil {
namespace nbt {
/**
 * \brief An iterator over a range of integers decoded from underlying bytes.
 *
 * \tparam T the type of integer in the range.
 */
template<std::signed_integral T>
class integer_span_iterator final {
	public:
	/**
	 * \brief The type of iterator this is.
	 */
	using iterator_category = std::random_access_iterator_tag;

	/**
	 * \brief The type of value returned by dereferencing this iterator.
	 */
	using value_type = T;

	/**
	 * \brief The type of integer used to measure distances between instances
	 * of this iterator.
	 */
	using difference_type = std::iterator_traits<std::span<const uint8_t>::iterator>::difference_type;

	/**
	 * \brief A pointer to this iterator’s @c value_type.
	 */
	using pointer = const T *;

	/**
	 * \brief A reference to this iterator’s @c value_type.
	 */
	using reference = const T &;

	explicit integer_span_iterator() = default;
	explicit integer_span_iterator(std::span<const uint8_t>::iterator bytes);
	T operator*() const;
	T operator[](difference_type n) const;
	integer_span_iterator &operator++();
	integer_span_iterator operator++(int);
	integer_span_iterator &operator+=(difference_type n);
	integer_span_iterator operator+(difference_type n) const;
	integer_span_iterator &operator--();
	integer_span_iterator operator--(int);
	integer_span_iterator &operator-=(difference_type n);
	integer_span_iterator operator-(difference_type n) const;
	difference_type operator-(integer_span_iterator other) const;
	bool operator==(integer_span_iterator other) const;
	bool operator!=(integer_span_iterator other) const;
	bool operator<(integer_span_iterator other) const;
	bool operator<=(integer_span_iterator other) const;
	bool operator>(integer_span_iterator other) const;
	bool operator>=(integer_span_iterator other) const;

	private:
	/**
	 * \brief The iterator over the underlying bytes.
	 */
	std::span<const uint8_t>::iterator bytes;
};

template<std::signed_integral T>
integer_span_iterator<T> operator+(typename integer_span_iterator<T>::difference_type n, integer_span_iterator<T> i);

/**
 * \brief A range of integers decoded from an underlying span of bytes.
 *
 * \tparam T the type of integer in the range.
 */
template<std::signed_integral T>
class integer_span final : public std::ranges::view_base {
	public:
	/**
	 * \brief An iterator over an integer range.
	 */
	using iterator = integer_span_iterator<T>;

	explicit integer_span(std::span<const uint8_t> bytes);
	iterator begin() const;
	iterator end() const;

	private:
	/**
	 * \brief The underlying bytes.
	 */
	std::span<const uint8_t> bytes;
};
}
}

extern template class mcwutil::nbt::integer_span_iterator<int32_t>;
extern template class mcwutil::nbt::integer_span_iterator<int64_t>;

extern template mcwutil::nbt::integer_span_iterator<int32_t> mcwutil::nbt::operator+(typename integer_span_iterator<int32_t>::difference_type n, integer_span_iterator<int32_t> i);
extern template mcwutil::nbt::integer_span_iterator<int64_t> mcwutil::nbt::operator+(typename integer_span_iterator<int64_t>::difference_type n, integer_span_iterator<int64_t> i);

extern template class mcwutil::nbt::integer_span<int32_t>;
extern template class mcwutil::nbt::integer_span<int64_t>;

#endif
