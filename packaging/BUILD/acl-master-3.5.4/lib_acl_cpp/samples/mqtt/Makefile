all:
	@(cd mqtt_server; make)
	@(cd mqtt_client; make)
	@(cd mqtt_pub; make)
	@(cd mqtt_aserver; make)
	@(cd mqtt_aclient; make)
clean cl:
	@(cd mqtt_server; make clean)
	@(cd mqtt_client; make clean)
	@(cd mqtt_pub; make clean)
	@(cd mqtt_aserver; make clean)
	@(cd mqtt_aclient; make clean)

rebuild rb: cl all
