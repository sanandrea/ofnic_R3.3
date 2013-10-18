include ../../../Make.vars 

EXTRA_DIST = \
	meta.json\
	__init__.py


NOX_RUNTIMEFILES = meta.json	\
	__init__.py\
	snmp_stats.py\
	open_stats.py\
	snmp_stats_ws.py\
	bod_routing.py\
	bod_routing_ws.py

all-local: nox-all-local
clean-local: nox-clean-local 
install-exec-hook: nox-install-local
