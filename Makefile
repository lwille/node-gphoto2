all:
	node-waf configure build
clean:
	rm -rf build
	rm .lock-wscript