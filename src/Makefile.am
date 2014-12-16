gamesdir=$(datadir)/games

AM_CPPFLAGS = -DMINECRAFTJARDIR='"$(minecraftjardir)"' -DSYSCONFDIR='"$(sysconfdir)"'
AM_CXXFLAGS = -std=c++11

bin_PROGRAMS = minecraftd
minecraftd_SOURCES = JarReader.cpp jvm.cpp minecraftd.cpp minecraftd-dbus.cpp
minecraftd_CPPFLAGS = $(AM_CPPFLAGS) $(AM_CXXFLAGS) $(glibmm_CFLAGS) $(libconfig_CFLAGS) $(libzip_CFLAGS)
minecraftd_LDFLAGS = -ldl -lpthread
minecraftd_LDADD = $(glibmm_LIBS) $(libconfig_LIBS) $(libzip_LIBS)
