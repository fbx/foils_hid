
bin_PROGRAMS = mouse remote

mouse_SOURCES = mouse.c
mouse_LDADD = $(top_builddir)/src/libfoils_hid.a $(RUDP_LIBS) -lm
mouse_CFLAGS = -I$(top_srcdir)/include $(RUDP_CFLAGS) $(GCC_CFLAGS)

remote_SOURCES = remote.c term_input.c term_input.h mapping.c mapping.h
remote_LDADD = $(top_builddir)/src/libfoils_hid.a $(RUDP_LIBS) -lm
remote_CFLAGS = -I$(top_srcdir)/include $(RUDP_CFLAGS) $(GCC_CFLAGS)
