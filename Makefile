NOX=$PWD


configure: db
	./boot.sh; mkdir -p build; cd build; ../configure

db:
	./dbScript.sh

compile:
	make -C build


start:
	cd build/src/;./nox_core -v -i ptcp:6633 discoveryws snmp_stats_ws bod_routing_ws db_manager acl&
	python src/nox/netapps/netic/snmp-walker.py &

stop:	
	-killall lt-nox_core
	killall python #to kill snmp-walker.py

.PHONY: configure
