localhost: app.localhost
localhost: export TARGET=localhost

ar7: app.ar7
ar7: export TARGET=ar7

wp7: app.wp7
wp7: export TARGET=wp7

app.%:
	mkapp helloWorld.adef -t $(TARGET)

clean:
	rm -rf _build_* *.localhost *.ar7 *.wp7
