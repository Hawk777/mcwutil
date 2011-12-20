#ifndef UTIL_NON_COPYABLE_H
#define UTIL_NON_COPYABLE_H

/**
 * An object that should not be copied or assigned.
 */
class NonCopyable {
	protected:
		/**
		 * Noncopyable objects can still be constructed.
		 */
		NonCopyable();

		/**
		 * Prevents objects from being copied.
		 */
		NonCopyable(const NonCopyable &) = delete;

		/**
		 * Prevents objects from being assigned to one another.
		 */
		NonCopyable &operator=(const NonCopyable &) = delete;
};

inline NonCopyable::NonCopyable() = default;

#endif

