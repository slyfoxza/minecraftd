gamesdir=$(datadir)/games

AM_CPPFLAGS = -DGAMESDIR='"$(gamesdir)"' -DSYSCONFDIR='"$(sysconfdir)"'
AM_CXXFLAGS = -std=c++11

bin_PROGRAMS = minecraftd
minecraftd_SOURCES = minecraftd.cpp
minecraftd_CPPFLAGS = $(AM_CPPFLAGS) $(AM_CXXFLAGS) $(GLIBMM_CFLAGS) $(LIBCONFIG_CFLAGS)
minecraftd_LDFLAGS = -ldl -lpthread -lconfig -lzip
minecraftd_LDADD = $(GLIBMM_LIBS) $(LIBCONFIG_LIBS)
