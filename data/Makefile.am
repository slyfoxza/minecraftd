CLEANFILES = minecraftd.conf
EXTRA_DIST = minecraftd.conf.in
systemdsystemunit =

minecraftd.conf: minecraftd.conf.in
	$(SED) -e 's,[@]datadir[@],$(datadir),g' <$< >$@

if HAVE_SYSTEMD
minecraftd.service: minecraftd.service.in
	$(SED) -e 's,[@]bindir[@],$(bindir),g' <$< >$@
systemdsystemunit += minecraftd.service
CLEANFILES += minecraftd.service
endif
EXTRA_DIST += minecraftd.service.in

