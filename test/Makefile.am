
bin_PROGRAMS = mouse unicode

mouse_SOURCES = mouse.c
mouse_LDADD = $(top_builddir)/src/libfoils_hid.a $(RUDP_LIBS) -lm
mouse_CFLAGS = -I$(top_srcdir)/include $(RUDP_CFLAGS) $(GCC_CFLAGS)

unicode_SOURCES = unicode.c term_input.c term_input.h mapping.c mapping.h
unicode_LDADD = $(top_builddir)/src/libfoils_hid.a $(RUDP_LIBS) -lm
unicode_CFLAGS = -I$(top_srcdir)/include $(RUDP_CFLAGS) $(GCC_CFLAGS)
