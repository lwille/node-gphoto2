srcfiles = src/autodetect.cc src/config.cc src/context.cc src/main.cc
all:
	g++ -lgphoto2 -lgphoto2_port $(srcfiles) -o node-gphoto2