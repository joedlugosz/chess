
#ifndef COMPILER_H
# define COMPILER_H

# define GCC 0
# define MSVC 1
# if defined (__GNUC__)
#  define COMP GCC
# elif defined (_MSC_VER)
#  define COMP MSVC
# else
#  error Compiler not supported
# endif

# define _POSIX_C_SOURCE 200809L
# if __STDC_VERSION__ >= 199901L
#  define _XOPEN_SOURCE 600
# else
#  define _XOPEN_SOURCE 500
# endif /* __STDC_VERSION__ */
#endif /* COMPILER_H */
