default: ar7 wp7

ar7: export TARGET=ar7
wp7: export TARGET=wp7
localhost: export TARGET=locahost

ar7 wp7 localhost: modemDemo

modemDemo:
	mkapp -v -t $(TARGET) -i $(LEGATO_ROOT)/interfaces/modemServices/c/ -i $(LEGATO_ROOT)/interfaces/dataConnectionService/c/ modemDemo.adef

clean:
	rm -rf _build_* *.ar7 *.wp7 *.localhost
