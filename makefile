
ADDITIONAL_CFLAGS = -I/usr/include/curl -lpthread -lcurl -I/usr/include/libxml2/ -lxml2
LIBPURPLE_CFLAGS = -I/usr/include/libpurple -I/usr/local/include/libpurple -DPURPLE_PLUGINS -DENABLE_NLS -lglib-2.0
DEBUG_CFLAGS = -DSPINPLOG -g
RELEASE_CFLAGS = -O2

PRPL_NAME	= libspinp.so

SPINP_SOURCES = \
	common.cpp \
	connector.cpp \
	cookie.cpp \
	convert.cpp \
	logger.cpp \
	job.cpp \
	message.cpp \
	http_request.cpp \
	base_connector.cpp \
	purple_connector.cpp \
	libspinp.cpp \

#SPINP_OBJ = $(SPINP_SOURCES:.c=.o)

$(PRPL_NAME): $(SPINP_SOURCES)
	g++ $(SPINP_SOURCES) -o libspinp.so `pkg-config --cflags --libs gtk+-2.0` -shared ${ADDITIONAL_CFLAGS} ${LIBPURPLE_CFLAGS} -std=c++11 -pipe -Wall -Wpedantic -fPIC -DPIC $(DEBUG_CFLAGS)

install:
	cp libspinp.so ~/.purple/plugins/libspinp.so

