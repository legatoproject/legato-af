
ar7: export TARGET = ar7
ar7: app.ar7

wp7: export TARGET = wp7
wp7: app.wp7

ar86: export TARGET = ar86
ar86: app.ar86

localhost: export TARGET = localhost
localhost: app.localhost

app.%:
	mkapp -v -t $(TARGET) -i $(LEGATO_ROOT)/build/$(TARGET)/airvantage/runtime/itf pollAvServer.adef

clean:
	rm -rf _build_* *.ar7 *.wp7 *.ar86 *.localhost
