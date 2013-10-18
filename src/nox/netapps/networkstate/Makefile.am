include ../../../Make.vars 

EXTRA_DIST =\
	__init__.py\
	meta.json\
	statisticsws.py\
	linkload.i

if PY_ENABLED
AM_CPPFLAGS += $(PYTHON_CPPFLAGS)
endif # PY_ENABLED 

pkglib_LTLIBRARIES =\
	linkload.la \
	switchrtt.la \
	datapathmem.la
	
NOX_RUNTIMEFILES = meta.json	

if PY_ENABLED
NOX_RUNTIMEFILES += \
	__init__.py		\
	statisticsws.py  

pylinkload_wrap.cc pylinkload.py: linkload.i 
	$(SWIG) $(SWIG_PYTHON_OPT) -I$(top_srcdir)/src/nox/lib/ -I$(top_srcdir)/src/nox/coreapps/ -outdir $(builddir)/. -o pylinkload_wrap.cc  -module pylinkload $(srcdir)/linkload.i

pkglib_LTLIBRARIES += _pylinkload.la
_pylinkload_la_CPPFLAGS = $(AM_CPPFLAGS) -I$(top_srcdir)/src/nox -I$(srcdir)/../ -I$(top_builddir)/src/nox/ -I$(top_srcdir)/src/nox/coreapps/
_pylinkload_la_SOURCES = pylinkload.cc pylinkload.hh linkload.i
nodist__pylinkload_la_SOURCES = pylinkload_wrap.cc
_pylinkload_la_LDFLAGS = -module

NOX_PYLIBFILES = _pylinkload.so
NOX_PYBUILDFILES = pylinkload_wrap.cc pylinkload.py
	
endif # PY_ENABLED


datapathmem_la_CPPFLAGS = $(AM_CPPFLAGS) -I $(top_srcdir)/src/nox -I $(top_srcdir)/src/nox/netapps/
datapathmem_la_SOURCES = datapathmem.hh datapathmem.cc
datapathmem_la_LDFLAGS = -module -export-dynamic

switchrtt_la_CPPFLAGS = $(AM_CPPFLAGS) -I $(top_srcdir)/src/nox -I $(top_srcdir)/src/nox/netapps/
switchrtt_la_SOURCES = switchrtt.hh switchrtt.cc
switchrtt_la_LDFLAGS = -module -export-dynamic

linkload_la_CPPFLAGS = $(AM_CPPFLAGS) -I $(top_srcdir)/src/nox -I $(top_srcdir)/src/nox/netapps/
linkload_la_SOURCES = linkload.hh linkload.cc
linkload_la_LDFLAGS = -module -export-dynamic	



all-local: nox-all-local
clean-local: nox-clean-local 
install-exec-hook: nox-install-local
