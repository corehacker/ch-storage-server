AC_INIT([ch-storage-server], [0.0.1])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([subdir-objects foreign -Wall -Werror])
AM_SILENT_RULES([yes])
AC_HEADER_STDC
AC_TYPE_SIZE_T
AC_SUBST([AM_CXXFLAGS])
AC_PROG_CC([gcc])
AC_PROG_CXX([g++])
AM_PROG_AR
AC_CHECK_TOOLS(CXX, [$CCC c++ g++ gcc CC cxx cc++ cl], gcc)
AC_LANG([C])
# AC_CHECK_HEADER_STDBOOL

CXXFLAGS="$CXXFLAGS -Wall -std=c++0x"
LIBS="-lpthread -lrt -lm $LIBS"

trylibeventdir=""
AC_ARG_WITH(libevent,
       [  --with-libevent=PATH     Specify path to libevent installation ],
       [
                if test "x$withval" != "xno" ; then
                        trylibeventdir=$withval
                fi
       ]
)

dnl ------------------------------------------------------
dnl libevent detection.  swiped from Tor.  modified a bit.

LIBEVENT_URL=http://www.monkey.org/~provos/libevent/

AC_CACHE_CHECK([for libevent directory], ac_cv_libevent_dir, [
  saved_LIBS="$LIBS"
  saved_LDFLAGS="$LDFLAGS"
  saved_CPPFLAGS="$CPPFLAGS"
  le_found=no
  for ledir in $trylibeventdir "" $prefix /usr/local ; do
    LDFLAGS="$saved_LDFLAGS"
    LIBS="-levent $saved_LIBS"

    # Skip the directory if it isn't there.
    if test ! -z "$ledir" -a ! -d "$ledir" ; then
       continue;
    fi
    if test ! -z "$ledir" ; then
      if test -d "$ledir/lib" ; then
        LDFLAGS="-L$ledir/lib $LDFLAGS"
      else
        LDFLAGS="-L$ledir $LDFLAGS"
      fi
      if test -d "$ledir/include" ; then
        CPPFLAGS="-I$ledir/include $CPPFLAGS"
      else
        CPPFLAGS="-I$ledir $CPPFLAGS"
      fi
    fi
    # Can I compile and link it?
    AC_TRY_LINK([#include <sys/time.h>
#include <sys/types.h>
#include <event.h>], [ event_init(); ],
       [ libevent_linked=yes ], [ libevent_linked=no ])
    if test $libevent_linked = yes; then
       if test ! -z "$ledir" ; then
         ac_cv_libevent_dir=$ledir
         _myos=`echo $target_os | cut -f 1 -d .`
         AS_IF(test "$SUNCC" = "yes" -o "x$_myos" = "xsolaris2",
               [saved_LDFLAGS="$saved_LDFLAGS -Wl,-R$ledir/lib"],
               [AS_IF(test "$GCC" = "yes",
                     [saved_LDFLAGS="$saved_LDFLAGS -Wl,-rpath,$ledir/lib"])])
       else
         ac_cv_libevent_dir="(system)"
       fi
       le_found=yes
       break
    fi
  done
  LIBS="$saved_LIBS"
  LDFLAGS="$saved_LDFLAGS"
  CPPFLAGS="$saved_CPPFLAGS"
  if test $le_found = no ; then
    AC_MSG_ERROR([libevent is required.  You can get it from $LIBEVENT_URL

      If it's already installed, specify its path using --with-libevent=/dir/
])
  fi
])
LIBS="-levent $LIBS"
if test $ac_cv_libevent_dir != "(system)"; then
  if test -d "$ac_cv_libevent_dir/lib" ; then
    LDFLAGS="-L$ac_cv_libevent_dir/lib $LDFLAGS"
    le_libdir="$ac_cv_libevent_dir/lib"
  else
    LDFLAGS="-L$ac_cv_libevent_dir $LDFLAGS"
    le_libdir="$ac_cv_libevent_dir"
  fi
  if test -d "$ac_cv_libevent_dir/include" ; then
    CPPFLAGS="-I$ac_cv_libevent_dir/include $CPPFLAGS"
  else
    CPPFLAGS="-I$ac_cv_libevent_dir $CPPFLAGS"
  fi
fi

dnl ----------------------------------------------------------------------------


##########################################################################
# check for glog library
##########################################################################
#dnl add the external library to the list of libraries which will be used for our test program
LIBS="-lglog $LIBS"

AC_LANG_PUSH([C++])
#dnl check for the presence of 'libtotest'
AC_MSG_CHECKING([for the glog availability])
AC_LINK_IFELSE([
                AC_LANG_PROGRAM([#include <glog/logging.h>],
                                [google::InitGoogleLogging("test")])
                ],
                [LIBCH_GLOG_PRESENCE=1], [LIBCH_GLOG_PRESENCE=0])

if test "$LIBCH_GLOG_PRESENCE" = "1"; then
    AC_MSG_RESULT([found])
else
    AC_MSG_ERROR([not found !])
fi
AC_LANG_POP

##########################################################################
# check for opencv library
##########################################################################
dnl check opencv
PKG_CHECK_MODULES([opencv], [opencv >= 3.3.0],
          [AC_DEFINE([HAVE_OPENCV], [1], [Using OpenCV])]
          [OPENCV_LIBS="-lopencv_core -lopencv_imgproc -lopencv_calib3d"],
          [PKG_CHECK_MODULES([opencv], [opencv >= 2.0],
          [AC_DEFINE([HAVE_OPENCV], [1], [Using OpenCV])]
          [OPENCV_LIBS="-lopencv_core -lopencv_imgproc"],
          AC_MSG_WARN([opencv not found. Not using some image processing.]))]
          )

AM_CONDITIONAL(HAVE_OPENCV, [test "x$OPENCV_LIBS" != "x"])
LIBS="-lopencv_core -lopencv_imgcodecs -lopencv_videoio -lopencv_ccalib -lopencv_highgui -lopencv_imgproc -lopencv_video -lopencv_bgsegm $LIBS"

##########################################################################
# check for ffmpeg libraries
##########################################################################
dnl checking several ffmpeg libraries
PKG_CHECK_MODULES(FFMPEG, [libavformat libavutil >= 55.20 libavcodec libswscale libswresample],
          [AC_DEFINE([HAVE_FFMPEG], [1], [film export supported])],
          AC_MSG_WARN([ffmpeg libav libraries not found or too old. Not using film export formats.]))

LIBS="-lavformat -lavcodec -lavutil -lswscale -lswresample $LIBS"


##########################################################################
# check for curl libraries
##########################################################################

AC_MSG_CHECKING([for libcurl cflags])
CURLCFLAGS=`pkg-config libcurl --cflags || curl-config --cflags`
AS_IF([ test "$?" -eq 0 ],,[AC_MSG_ERROR([libcurl was not found])] )
AC_MSG_RESULT([$CURLCFLAGS])

AC_MSG_CHECKING([for libcurl libs])
CURLLIBS=`pkg-config libcurl --libs || curl-config --libs`
AS_IF([ test "$?" -eq 0 ],,[AC_MSG_ERROR([libcurl was not found])] )
AC_MSG_RESULT([$CURLLIBS])

AC_MSG_CHECKING([for libcurl version >= $MIN_LIBCURL_VERSION])
LIBCURL_VERSION=`pkg-config libcurl --modversion || curl-config --version`
(pkg-config libcurl && pkg-config libcurl --atleast-version=${MIN_LIBCURL_VERSION}) || curl-config --checkfor ${MIN_LIBCURL_VERSION}
AS_IF([ test "$?" -eq 0 ],,[AC_MSG_ERROR([${LIBCURL_VERSION} is too old])])
AC_MSG_RESULT([${LIBCURL_VERSION}])

LIBS="-lcurl $LIBS"

##########################################################################
# check for librdkafka library
##########################################################################
#dnl add the external library to the list of libraries which will be used for our test program
LIBS="-lrdkafka++ $LIBS"

AC_LANG_PUSH([C++])
#dnl check for the presence of 'libtotest'
AC_MSG_CHECKING([for the rdkafka++ availability])
AC_LINK_IFELSE([
                AC_LANG_PROGRAM([#include <librdkafka/rdkafkacpp.h>],
                                [RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL)])
                ],
                [LIBCH_LIB_RD_KAFKA_PRESENCE=1], [LIBCH_LIB_RD_KAFKA_PRESENCE=0])

if test "$LIBCH_LIB_RD_KAFKA_PRESENCE" = "1"; then
    AC_MSG_RESULT([found])
else
    AC_MSG_ERROR([not found !])
fi
AC_LANG_POP

##########################################################################
# check for ch-cpp-utils library
##########################################################################
#dnl add the external library to the list of libraries which will be used for our test program
LIBS="-lch-cpp-utils $LIBS"

AC_LANG_PUSH([C++])
#dnl check for the presence of 'libtotest'
AC_MSG_CHECKING([for the ch-cpp-utils availability])
AC_LINK_IFELSE([
                AC_LANG_PROGRAM([#include <ch-cpp-utils/http-server.hpp>],
                                [ChCppUtils::Http::Server::HttpServer pool(8888,1)])
                ],
                [LIBCH_CH_CPP_UTILS_PRESENCE=1], [LIBCH_CH_CPP_UTILS_PRESENCE=0])

if test "$LIBCH_CH_CPP_UTILS_PRESENCE" = "1"; then
    AC_MSG_RESULT([found])
else
    AC_MSG_ERROR([not found !])
fi
AC_LANG_POP



AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES(Makefile \
                src/Makefile \
                manual-classify/Makefile)
AC_OUTPUT
