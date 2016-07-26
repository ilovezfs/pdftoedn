#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
m4_define([pdftoedn_version],[0.31.0])

AC_PREREQ([2.68])
AC_INIT([pdftoedn], [pdftoedn_version], [ed@motologic.com])
AM_INIT_AUTOMAKE

AC_CONFIG_SRCDIR([src/base_types.cc])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile src/Makefile])
AC_CONFIG_MACRO_DIR([m4])

# Checks for programs.
PKG_PROG_PKG_CONFIG
AC_PROG_CXX
AC_PROG_CC

# c++-11 is required
AX_CXX_COMPILE_STDCXX_11


AC_DEFINE_UNQUOTED([PDFTOEDN_VERSION], ["pdftoedn_version"], [Explicitly named version])

# Checks for libraries.
AC_LANG_PUSH(C++)
# poppler
PKG_CHECK_MODULES([poppler], [poppler >= 0.45.0])
PKG_CHECK_MODULES([poppler_cpp], [poppler-cpp >= 0.45.0])

dnl make sure poppler xpdf headers are installed
save_CPPFLAGS=$CPPFLAGS
CPPFLAGS="$CPPFLAGS $poppler_CFLAGS"
AC_CHECK_HEADER([GfxState.h],,[AC_MSG_ERROR([poppler headers not found. Please configure poppler with --enable-xpdf-headers flag])])
CPPFLAGS=$save_CPPFLAGS

dnl libpng
PKG_CHECK_MODULES([png], [libpng], libpng_ok=yes, libpng_ok=no)

if test "x$libpng_ok" = "xyes"; then
   LIBPNG_LDFLAGS="-L`pkg-config --variable=libdir libpng`"
else
   AC_MSG_ERROR([libpng was not found])
fi

dnl freetype
PKG_CHECK_MODULES([freetype2], freetype2,
                  [], [freetype2 was not found])

dnl openjpeg
PKG_CHECK_MODULES([freetype2], freetype2,
                  [], [freetype2 was not found])

dnl boost
AX_BOOST_BASE([1.60],[], [AC_MSG_ERROR([boost was not found in your system])])
AX_BOOST_SYSTEM
AX_BOOST_FILESYSTEM
AX_BOOST_PROGRAM_OPTIONS
AX_BOOST_REGEX
AX_BOOST_LOCALE

dnl rapidjson
save_CPPFLAGS=$CPPFLAGS
CPPFLAGS="$CPPFLAGS $rapidjson_CFLAGS"
AC_CHECK_HEADER([rapidjson/document.h],,[AC_MSG_ERROR([rapidjson not found])])
CPPFLAGS=$save_CPPFLAGS

AC_LANG_POP

AC_LANG_PUSH(C)
dnl leptonica
save_CPPFLAGS=$CPPFLAGS
CPPFLAGS="$CPPFLAGS $leptonica_CFLAGS"
AC_CHECK_HEADER([leptonica/allheaders.h],,[AC_MSG_ERROR([leptonica headers not found])])
AC_SEARCH_LIBS([getLeptonicaVersion], [lept],, [AC_MSG_ERROR([leptonica library not found])])
CPPFLAGS=$save_CPPFLAGS

dnl openssl
AX_CHECK_OPENSSL(openssl_ok=yes, openssl_ok=no)

if test "x$openssl_ok" = "xyes"; then
   AC_DEFINE([HAVE_LIBOPENSSL], [1], [using openssl md5])
else
   AC_MSG_NOTICE([openssl not found - using local md5 calculation code])
fi
AM_CONDITIONAL([LOCAL_MD5], [test x$openssl_ok = xno])


AC_LANG_POP

# Checks for header files.
AC_CHECK_HEADERS([unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_INT16_T
AC_TYPE_INT8_T
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_CHECK_FUNCS([memset setlocale strerror strstr])

AC_OUTPUT
