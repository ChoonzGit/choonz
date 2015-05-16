# $Id: $
# 28 Oct 2012
# Simplify choonz to have a single multicast player and remove local playback
# Use libffmpeg for decoding so remove livemedia
# Get rid of stufff rarely/never used e.g. CD ripper, MusicBrainz
# Keep the ability to scan locations
# Add support for Echo Nest, so include FFMPEG resampling

.PHONY: all clean

POSTGRES =	/usr/local/pgsql
JSON_PATH = /home/adrian/choonz2/libjson
CC =		gcc
CXX =		g++
CXXFLAGS =	-Wall -O2 -pipe -std=c++11 -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -D__STDC_CONSTANT_MACROS -D_REENTRANT -pthread \
			-I/usr/local/include/taglib \
			-I$(POSTGRES)/include


TARGET =	server

LIBS =		-lboost_filesystem -lboost_thread -lboost_system -lpq \
			-lavutil -lavformat -lavcodec -lavdevice -lavfilter -lavutil -lswresample \
			-ltag -lpthread -lrt -lz -lbz2 ../library/library.a


LIBFLAGS =	-L$(POSTGRES)/lib \
			-L/usr/local/lib \
			-L$(JSON_PATH)


LINK =		g++
INCLUDES =	-I/usr/local/include/boost

MAIN_OBJ = 	server.o

OBJECTS	=	command_library.o \
		command_location.o \
		track_manager.o

all: $(TARGET)

clean:
	rm -f $(OBJECTS)
	rm -f $(TARGET)

.cpp.o:
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c -o $@ $<

$(TARGET): $(MAIN_OBJ) $(OBJECTS)
	$(LINK) -o $(TARGET) $(MAIN_OBJ) $(OBJECTS) $(LIBFLAGS) $(LIBS)
