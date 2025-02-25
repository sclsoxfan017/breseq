###
##
## AUTHORS
##
## Jeffrey E. Barrick <jeffrey.e.barrick@gmail.com>
## David B. Knoester
##
## LICENSE AND COPYRIGHT
##
## Copyright (c) 2010 Michigan State University
##
## breseq is free software; you can redistribute it and/or modify it under the terms the 
## GNU General Public License as published by the Free Software Foundation; either 
## version 1, or (at your option) any later version.
##
###


#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
AC_PREREQ([2.65])
AC_INIT([breseq], [0.25d], [jeffrey.e.barrick@gmail.com], [breseq], [http://barricklab.org/breseq])
AC_CONFIG_AUX_DIR(aux_build)
AC_CONFIG_MACRO_DIR([aux_build/m4])
AC_CONFIG_HEADERS([aux_build/config.h])
AC_CONFIG_SRCDIR([])

LT_INIT
AM_INIT_AUTOMAKE([subdir-objects])

AM_PROG_CC_C_O
AC_PROG_CXX
AC_PROG_AWK
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_MKDIR_P
AC_PROG_LIBTOOL

AX_COMPUTE_RELATIVE_PATHS([bindir:datadir:bin_to_data])
AC_MSG_NOTICE([Local path from bin to data...])
AC_MSG_NOTICE([$bin_to_data])
AC_SUBST([BIN_TO_DATA_RELATIVE_PATH], [$bin_to_data])


AC_CHECK_HEADERS([arpa/inet.h fcntl.h inttypes.h netdb.h stddef.h stdint.h stdlib.h string.h sys/socket.h unistd.h wchar.h])
AC_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_INT8_T
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T
AC_FUNC_FSEEKO
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([bzero gethostbyaddr gethostbyname memmove memset pow select socket sqrt strchr strdup strerror strstr strtol ftello])

AC_SEARCH_LIBS([wclear], [curses ncurses], [],
   [AC_MSG_ERROR(["lib(n)curses is required."])])

AC_SEARCH_LIBS([deflateSetHeader], [z], [], [AC_MSG_ERROR(["libz >=1.2.2.1 is required."])])
AC_SEARCH_LIBS([acosf], [m], [], [AC_MSG_ERROR(["libm is required."])])

# This code could be used to check for an existing samtools/libbam.a
# But unfortunately we have to compile them ourselves with a patch
#AC_CHECK_PROG([samtools],[samtools],[yes],[no])
#AM_CONDITIONAL([FOUND_SAMTOOLS], [test "x$samtools" = xyes])
#AM_COND_IF([FOUND_SAMTOOLS],,[AC_MSG_ERROR([required program 'samtools' not found.])])
#AC_SEARCH_LIBS([bam_fetch], [bam], [], [AC_MSG_ERROR(["libbam is required."])])


AC_SUBST([LIBBCF], [extern/samtools-0.1.18/bcftools])
AC_SUBST([LIBBAM], [extern/samtools-0.1.18])
AC_SUBST([BRESEQ], [src/c/breseq])

AC_CHECK_FILE([.hg/branchheads.cache], 
	AC_SUBST(HG_REVISION_STRING, [`eval head -n 1 .hg/branchheads.cache | sed 's/^\(.\{12\}\).*/\1/'`]),
	[ AC_CHECK_FILE([.hg/cache/branchheads], 
		AC_SUBST(HG_REVISION_STRING, [`eval head -n 1 .hg/cache/branchheads | sed 's/^\(.\{12\}\).*/\1/'`]), 
		AC_SUBST(HG_REVISION_STRING, [])
	) ]
)

AC_CHECK_FILE([.hg/branchheads.cache], 
	AC_SUBST(HG_REVISION_PREFIX, "revision "),
	[ AC_CHECK_FILE([.hg/cache/branchheads], 
		AC_SUBST(HG_REVISION_PREFIX, "revision "), 
		AC_SUBST(HG_REVISION_PREFIX, [])
	) ]
)

AC_DEFINE_UNQUOTED([HG_REVISION], ["${HG_REVISION_PREFIX}${HG_REVISION_STRING}"], [Defining HG repository version])


AC_CANONICAL_HOST
# Check for which host we are on and setup a few things
# specifically based on the host
case $host_os in
  cygwin* )
  		AC_MSG_NOTICE([Detected host 'cygwin'.])
  		AC_SUBST(SAMTOOLS_ADDITIONAL_CFLAGS, ["-Dexpl=exp -Dlogl=log"])
        ;;
  darwin* )
    	AC_MSG_NOTICE([Detected host 'darwin'.])
        ;;
esac


AC_CONFIG_FILES([
    extern/samtools-0.1.18/bcftools/Makefile
    extern/samtools-0.1.18/Makefile
    src/c/breseq/Makefile
    Makefile
])
AC_OUTPUT


