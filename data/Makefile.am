conf_files = log4j2.xml minecraftd.conf

CLEANFILES = $(conf_files)
EXTRA_DIST = log4j2.xml.in minecraftd.conf.in
systemdsystemunit =

log4j2.xml: log4j2.xml.in
	$(SED) -e 's,[@]logdir[@],$(logdir),g' <$< >$@

minecraftd.conf: minecraftd.conf.in
	$(SED) \
		-e 's,[@]jvmlibpath[@],$(jvmlibpath),g' \
		-e 's,[@]minecraftdconfdir[@],$(minecraftdconfdir),g' \
		-e 's,[@]minecraftjardir[@],$(minecraftjardir),g' \
		-e 's,[@]minecraftserverdir[@],$(minecraftserverdir),g' \
		<$< >$@

if HAVE_SYSTEMD
minecraftd.service: minecraftd.service.in
	$(SED) -e 's,[@]bindir[@],$(bindir),g' <$< >$@
systemdsystemunit += minecraftd.service
CLEANFILES += minecraftd.service
endif
EXTRA_DIST += minecraftd.service.in

uninstall-hook:
	$(RM) \
		$(DESTDIR)$(minecraftdconfdir)/log4j2.xml
		$(DESTDIR)$(minecraftdconfdir)/minecraftd.conf

install-data-hook: $(conf_files) $(systemdsystemunit)
	if test '!' -d $(DESTDIR)$(minecraftdconfdir); then \
		$(mkinstalldirs) $(DESTDIR)$(minecraftdconfdir); \
		chmod 755 $(DESTDIR)$(minecraftdconfdir); \
	fi
	for conf_file in $(conf_files); do \
		$(INSTALL_DATA) $$conf_file $(DESTDIR)$(minecraftdconfdir)/$$conf_file; \
	done
	if test -n "$(systemdsystemunit)" -a '!' -d $(DESTDIR)$(SYSTEMD_SYSTEM_UNIT_DIR); then \
		$(mkinstalldirs) $(DESTDIR)$(SYSTEMD_SYSTEM_UNIT_DIR); \
		chmod 755 $(DESTDIR)$(SYSTEMD_SYSTEM_UNIT_DIR); \
		$(INSTALL_DATA) $(systemdsystemunit) $(DESTDIR)$(SYSTEMD_SYSTEM_UNIT_DIR)/$(systemdsystemunit); \
	fi
