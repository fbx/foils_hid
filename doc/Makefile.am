
if HAVE_MKDOC

html-local:
	$(mkdoc) \
		--conf-file $(srcdir)/mkdoc.conf \
		--code-path $(top_srcdir)/test \
		--doc-path $(srcdir)/. \
		-I $(top_srcdir)/include \
		foils/hid.h foils/hid_device.h foils/rudp_hid_client.h


clean-local:
	-rm -r html

endif
