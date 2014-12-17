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

uninstall-hook:
	$(RM) \
		$(DESTDIR)$(minecraftdconfdir)/minecraftd.conf

install-data-hook: minecraftd.conf
	if test '!' -d $(DESTDIR)$(minecraftdconfdir); then \
		$(mkinstalldirs) $(DESTDIR)$(minecraftdconfdir); \
		chmod 755 $(DESTDIR)$(minecraftdconfdir); \
	fi
	$(INSTALL_SCRIPT) minecraftd.conf $(DESTDIR)$(minecraftdconfdir)/minecraftd.conf
	chmod 644 $(DESTDIR)$(minecraftdconfdir)/minecraftd.conf
