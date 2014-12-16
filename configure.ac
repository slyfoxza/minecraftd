AC_INIT([minecraftd], [1.0], [syke107@gmail.com])
AM_INIT_AUTOMAKE([foreign -Wall -Werror])

AC_CANONICAL_HOST

AC_PROG_CXX
AC_PROG_SED

AC_ARG_WITH(systemd, AS_HELP_STRING([--with-systemd], [Add systemd support @<:@default=yes@:>@]),
		[with_systemd=$withval], [with_systemd=yes])
AM_CONDITIONAL(HAVE_SYSTEMD, [test "x$with_systemd" = "xyes"])

PKG_CHECK_MODULES(glibmm, [glibmm-2.4 giomm-2.4])
PKG_CHECK_MODULES(libconfig, [libconfig++])
PKG_CHECK_MODULES(libzip, [libzip])

AX_JNI_INCLUDE_DIR
for JNI_INCLUDE_DIR in $JNI_INCLUDE_DIRS; do
	CPPFLAGS="$CPPFLAGS -I$JNI_INCLUDE_DIR"
done

AC_ARG_WITH(minecraftjardir, AS_HELP_STRING([--with-minecraftjardir=DIR], [Minecraft JARs @<:@default=DATADIR/minecraft@:>@]))
if ! test -z "$with_minecraftjardir"; then
	MINECRAFT_JAR_DIR=$with_minecraftjardir
else
	MINECRAFT_JAR_DIR=$datadir/minecraft
fi
AC_SUBST(minecraftjardir, $MINECRAFT_JAR_DIR)

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile data/Makefile src/Makefile])
AC_OUTPUT
