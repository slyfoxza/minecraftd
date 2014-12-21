AC_INIT([minecraftd], [1.0], [syke107@gmail.com])
AC_CONFIG_MACRO_DIRS([m4])
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

MCD_JNI_INCLUDE_DIR
for JNI_INCLUDE_DIR in $JNI_INCLUDE_DIRS; do
	CPPFLAGS="$CPPFLAGS -I$JNI_INCLUDE_DIR"
done

AC_ARG_WITH(jvmlibpath,
		AS_HELP_STRING([--with-jvmlibpath=FILE],
			[JVM library path @<:@default=/etc/alternatives/jre/lib/amd64/server/libjvm.so@:>@]))
if ! test -z "$with_jvmlibpath"; then
	JVM_LIB_PATH=$with_jvmlibpath
else
	JVM_LIB_PATH=/etc/alternatives/jre/lib/amd64/server/libjvm.so
fi
AC_SUBST(jvmlibpath, $JVM_LIB_PATH)

AC_ARG_WITH(logdir, AS_HELP_STRING([--with-logdir=DIR], [log directory @<:@default=LOCALSTATEDIR/log/minecraftd@:>@]))
if ! test -z "$with_logdir"; then
	LOG_DIR=$with_logdir
else
	LOG_DIR=$localstatedir/log/minecraftd
fi
AC_SUBST(logdir, $LOG_DIR)

AC_ARG_WITH(minecraftdconfdir,
		AS_HELP_STRING([--with-minecraftdconfdir=DIR],
				[minecraftd configuration @<:@default=SYSCONFDIR/minecraftd@:>@]))
if ! test -z "$with_minecraftdconfdir"; then
	MINECRAFTD_CONF_DIR=$with_minecraftdconfdir
else
	MINECRAFTD_CONF_DIR=$sysconfdir/minecraftd
fi
AC_SUBST(minecraftdconfdir, $MINECRAFTD_CONF_DIR)

AC_ARG_WITH(minecraftjardir,
		AS_HELP_STRING([--with-minecraftjardir=DIR], [Minecraft JARs @<:@default=DATADIR/games/minecraft@:>@]))
if ! test -z "$with_minecraftjardir"; then
	MINECRAFT_JAR_DIR=$with_minecraftjardir
else
	MINECRAFT_JAR_DIR=$datadir/games/minecraft
fi
AC_SUBST(minecraftjardir, $MINECRAFT_JAR_DIR)

AC_ARG_WITH(minecraftserverdir,
		AS_HELP_STRING([--with-minecraftserverdir=DIR],
			[Minecraft server state/storage @<:@default=LOCALSTATEDIR/games/minecraft@:>@]))
if ! test -z "$with_minecraftserverdir"; then
	MINECRAFT_SERVER_DIR=$with_minecraftserverdir
else
	MINECRAFT_SERVER_DIR=$localstatedir/games/minecraft
fi
AC_SUBST(minecraftserverdir, $MINECRAFT_SERVER_DIR)

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile data/Makefile src/Makefile])
AC_OUTPUT
