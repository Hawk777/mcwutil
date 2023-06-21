#ifndef UTIL_NON_COPYABLE_H
#define UTIL_NON_COPYABLE_H

/**
 * \brief An object that should not be copied or assigned.
 */
class NonCopyable {
	protected:
	/**
	 * \brief Noncopyable objects can still be constructed.
	 */
	NonCopyable();

	/**
	 * \brief Prevents objects from being copied.
	 */
	NonCopyable(const NonCopyable &) = delete;

	/**
	 * \brief Prevents objects from being assigned to one another.
	 */
	NonCopyable &operator=(const NonCopyable &) = delete;
};

inline NonCopyable::NonCopyable() = default;

#endif
