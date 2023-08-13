#include <mcwutil/nbt/integer_span.hpp>
#include <mcwutil/util/codec.hpp>
#include <cassert>

using namespace mcwutil::nbt;

/**
 * \brief Constructs an iterator for a \ref integer_span.
 *
 * \param[in] bytes the iterator over the underlying bytes.
 */
template<std::signed_integral T>
integer_span_iterator<T>::integer_span_iterator(std::span<const uint8_t>::iterator bytes) :
		bytes(bytes) {
}

/**
 * \brief Decodes the integer at this iterator’s current position.
 *
 * \return the decoded integer.
 */
template<std::signed_integral T>
T integer_span_iterator<T>::operator*() const {
	return codec::decode_integer<std::make_unsigned_t<T>>(bytes);
}

/**
 * \brief Decodes the integer at an offset from this iterator’s current
 * position.
 *
 * \param[in] n the offset.
 *
 * \return the element at this iterator’s position plus \p n.
 */
template<std::signed_integral T>
T integer_span_iterator<T>::operator[](difference_type n) const {
	return *(*this + n);
}

/**
 * \brief Increments the iterator’s position.
 *
 * \return \p *this.
 */
template<std::signed_integral T>
auto integer_span_iterator<T>::operator++() -> integer_span_iterator & {
	bytes += sizeof(T);
	return *this;
}

/**
 * \brief Increments the iterator’s position.
 *
 * \return the iterator prior to the increment.
 */
template<std::signed_integral T>
auto integer_span_iterator<T>::operator++(int) -> integer_span_iterator {
	integer_span_iterator ret(*this);
	++*this;
	return ret;
}

/**
 * \brief Advances the iterator’s position.
 *
 * \param[in] n how many elements to advance over.
 *
 * \return \p *this.
 */
template<std::signed_integral T>
auto integer_span_iterator<T>::operator+=(difference_type n) -> integer_span_iterator & {
	bytes += n * sizeof(T);
	return *this;
}

/**
 * \brief Returns an advancement of the iterator.
 *
 * \param[in] n how many elements to advance over.
 *
 * \return this iterator, advanced \p n places.
 */
template<std::signed_integral T>
auto integer_span_iterator<T>::operator+(difference_type n) const -> integer_span_iterator {
	integer_span_iterator ret(*this);
	ret += n;
	return ret;
}

/**
 * \brief Decrements the iterator’s position.
 *
 * \return \p *this.
 */
template<std::signed_integral T>
auto integer_span_iterator<T>::operator--() -> integer_span_iterator & {
	bytes -= sizeof(T);
	return *this;
}

/**
 * \brief Decrements the iterator’s position.
 *
 * \return the iterator prior to the decrement.
 */
template<std::signed_integral T>
auto integer_span_iterator<T>::operator--(int) -> integer_span_iterator {
	integer_span_iterator ret(*this);
	--*this;
	return ret;
}

/**
 * \brief Advances the iterator’s position backwards.
 *
 * \param[in] n how many elements to advance over.
 *
 * \return \p *this.
 */
template<std::signed_integral T>
auto integer_span_iterator<T>::operator-=(difference_type n) -> integer_span_iterator & {
	return *this += -n;
}

/**
 * \brief Returns a backwards advancement of the iterator.
 *
 * \param[in] n how many elements to advance over.
 *
 * \return this iterator, advanced \p n places backwards.
 */
template<std::signed_integral T>
auto integer_span_iterator<T>::operator-(difference_type n) const -> integer_span_iterator {
	return *this + -n;
}

/**
 * \brief Calculates the distance between two iterators.
 *
 * \pre \p this and \p other point into the same \ref integer_span.
 *
 * \param[in] other the iterator to subtract.
 *
 * \return the distance from \p this to \p other.
 */
template<std::signed_integral T>
auto integer_span_iterator<T>::operator-(integer_span_iterator other) const -> difference_type {
	return (bytes - other.bytes) / sizeof(T);
}

/**
 * \brief Compares the positions of two iterators.
 *
 * \param[in] other the iterator to compare to.
 *
 * \retval true \p this is equal to \p other.
 * \retval false \p this is unequal to \p other.
 */
template<std::signed_integral T>
bool integer_span_iterator<T>::operator==(integer_span_iterator other) const {
	return bytes == other.bytes;
}

/**
 * \brief Compares the positions of two iterators.
 *
 * \param[in] other the iterator to compare to.
 *
 * \retval true \p this is unequal to \p other.
 * \retval false \p this is equal to \p other.
 */
template<std::signed_integral T>
bool integer_span_iterator<T>::operator!=(integer_span_iterator other) const {
	return bytes != other.bytes;
}

/**
 * \brief Compares the positions of two iterators.
 *
 * \pre \p this and \p other point within the same \ref integer_span.
 *
 * \param[in] other the iterator to compare to.
 *
 * \retval true \p this points before \p other.
 * \retval false \p this is equal to or points after \p other.
 */
template<std::signed_integral T>
bool integer_span_iterator<T>::operator<(integer_span_iterator other) const {
	return bytes < other.bytes;
}

/**
 * \brief Compares the positions of two iterators.
 *
 * \pre \p this and \p other point within the same \ref integer_span.
 *
 * \param[in] other the iterator to compare to.
 *
 * \retval true \p this is equal to or points before \p other.
 * \retval false \p this points after \p other.
 */
template<std::signed_integral T>
bool integer_span_iterator<T>::operator<=(integer_span_iterator other) const {
	return bytes <= other.bytes;
}

/**
 * \brief Compares the positions of two iterators.
 *
 * \pre \p this and \p other point within the same \ref integer_span.
 *
 * \param[in] other the iterator to compare to.
 *
 * \retval true \p this points after \p other.
 * \retval false \p this is equal to or points before \p other.
 */
template<std::signed_integral T>
bool integer_span_iterator<T>::operator>(integer_span_iterator other) const {
	return bytes > other.bytes;
}

/**
 * \brief Compares the positions of two iterators.
 *
 * \pre \p this and \p other point within the same \ref integer_span.
 *
 * \param[in] other the iterator to compare to.
 *
 * \retval true \p this is equal to or points after \p other.
 * \retval false \p this points before \p other.
 */
template<std::signed_integral T>
bool integer_span_iterator<T>::operator>=(integer_span_iterator other) const {
	return bytes >= other.bytes;
}

/**
 * \brief Returns an advancement of an iterator.
 *
 * \param[in] n how many elements to advance over.
 *
 * \param[in] i the iterator to advance.
 *
 * \return \p i, advanced \p n places.
 */
template<std::signed_integral T>
integer_span_iterator<T> mcwutil::nbt::operator+(typename integer_span_iterator<T>::difference_type n, integer_span_iterator<T> i) {
	return i + n;
}

template class integer_span_iterator<int32_t>;
template class integer_span_iterator<int64_t>;

namespace mcwutil::nbt {
/**
 * \brief Returns an advancement of an iterator.
 *
 * \param[in] n how many elements to advance over.
 *
 * \param[in] i the iterator to advance.
 *
 * \return \p i, advanced \p n places.
 */
template integer_span_iterator<int32_t> operator+(typename integer_span_iterator<int32_t>::difference_type n, integer_span_iterator<int32_t> i);

/**
 * \brief Returns an advancement of an iterator.
 *
 * \param[in] n how many elements to advance over.
 *
 * \param[in] i the iterator to advance.
 *
 * \return \p i, advanced \p n places.
 */
template integer_span_iterator<int64_t> operator+(typename integer_span_iterator<int64_t>::difference_type n, integer_span_iterator<int64_t> i);
}

static_assert(std::random_access_iterator<integer_span_iterator<int32_t>>);
static_assert(std::random_access_iterator<integer_span_iterator<int64_t>>);



/**
 * \brief Wraps a sequence of bytes in an \c integer_span.
 *
 * \pre the length of \p bytes is a multiple of the size of \p T.
 *
 * \param[in] bytes the bytes to wrap.
 */
template<std::signed_integral T>
integer_span<T>::integer_span(std::span<const uint8_t> bytes) :
		bytes(bytes) {
	assert(!(bytes.size() % sizeof(T)));
}

/**
 * \brief Returns an iterator to the beginning of the range.
 *
 * \return the start iterator.
 */
template<std::signed_integral T>
auto integer_span<T>::begin() const -> iterator {
	return iterator(bytes.begin());
}

/**
 * \brief Returns an iterator to the end of the range.
 *
 * \return the past-the-end iterator.
 */
template<std::signed_integral T>
auto integer_span<T>::end() const -> iterator {
	return iterator(bytes.end());
}

template class mcwutil::nbt::integer_span<int32_t>;
template class mcwutil::nbt::integer_span<int64_t>;

template<typename T>
concept expected_integer_span_properties = std::ranges::random_access_range<T> && std::ranges::sized_range<T> && std::ranges::view<T>;
static_assert(expected_integer_span_properties<integer_span<int32_t>>);
static_assert(expected_integer_span_properties<integer_span<int64_t>>);
