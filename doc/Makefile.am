DOC_MODULE="liberasurecode"
HTML_DIR=$(datadir)/$(DOC_MODULE)/html
TARGET_DIR=$(HTML_DIR)/$(DOC_MODULE)

all-local:	html/index.html

if ENABLE_DOXYGEN
html/index.html: doxygen.cfg
	doxygen doxygen.cfg
else
html/index.html:
endif

clean-local:
	rm -f *~ *.bak
	rm -rf html

install-data-local: html
	../install-sh -d $(DESTDIR)$(TARGET_DIR)
	(installfiles=`echo html/*`; \
	if test "$$installfiles" = 'html/*'; \
	then echo '-- Nothing to install' ; \
	else \
		for i in $$installfiles; do \
			echo '-- Installing '$$i ; \
			../install-sh $$i $(DESTDIR)$(TARGET_DIR); \
		done; \
	fi)

dist-hook:
	mkdir $(distdir)/html
	mkdir $(distdir)/man
	-cp html/* $(distdir)/html
	-cp man/* $(distdir)/man

.PHONY : html latex man
