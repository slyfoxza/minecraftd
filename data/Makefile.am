CLEANFILES =
EXTRA_DIST =
systemdsystemunit =

if HAVE_SYSTEMD
minecraftd.service: minecraftd.service.in
	$(SED) -e 's,[@]bindir[@],$(bindir),g' <$< >$@
systemdsystemunit += minecraftd.service
CLEANFILES += minecraftd.service
endif
EXTRA_DIST += minecraftd.service.in

