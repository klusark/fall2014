	#! /usr/bin/make

# Generic Makefile that should work with any program you're going to compile.
# Any complaints should be directed at honghual@sfu.ca
#
# To compile and link your program all you have to do is run 'make' in the
#    current directory.
# To clean up object files run 'make clean_object'.
# To delete any compiled files run 'make clean'.
# Originated in 2001 by Haris Teguh
# modified May-2012 by Honghua Li

# If you have more source files add them here 
SOURCE= scene.cpp image_util.cpp sphere.cpp vector.cpp trace.cpp raycast.cpp model.cpp plane.cpp include/InitShader.cpp

# The compiler we are using 
CXX= g++

# The flags that will be used to compile the object file.
# If you want to debug your program,
# you can add '-g' on the following line
CFLAGS= -O3 -g -Wall -pedantic -DGL_GLEXT_PROTOTYPES -std=c++11

# The name of the final executable 
EXECUTABLE= raycast

# The basic library we are using add the other libraries you want to link
# to your program here 

# Linux (default)
LDFLAGS = -lGL -lglut -lGLEW -lXext -lX11 -lm -std=c++11 -pthread

# If you have other library files in a different directory add them here 
INCLUDEFLAG= -I. -Iinclude/

# Don't touch this one if you don't know what you're doing 
OBJECT= $(SOURCE:.cpp=.o)

# Don't touch any of these either if you don't know what you're doing 
all: $(OBJECT) depend
	$(CXX) $(CFLAGS) $(INCLUDEFLAG) $(OBJECT) -o $(EXECUTABLE) $(LDFLAGS)

depend:
	$(CXX) -M $(SOURCE) -std=c++11 > depend

$(OBJECT):
	$(CXX) $(CFLAGS) $(INCLUDEFLAG) -c -o $@ $(@:.o=.cpp)

clean_object:
	rm -f $(OBJECT)

clean:
	rm -f $(OBJECT) depend $(EXECUTABLE)

include depend
