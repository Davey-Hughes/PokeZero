#ifndef DEBUG_HELPER_HH
#define DEBUG_HELPER_HH

// macros for debugging
#ifdef DEBUG
#define DEBUG_STDOUT(str)                 \
	do {                              \
		std::cout << str << '\n'; \
	} while (false)

#define DEBUG_STDERR(str)                 \
	do {                              \
		std::cerr << str << '\n'; \
	} while (false)
#else
#define DEBUG_STDOUT(str) \
	do {              \
	} while (false)

#define DEBUG_STDERR(str) \
	do {              \
	} while (false)
#endif

#endif /* DEBUG_HELPER_HH */
