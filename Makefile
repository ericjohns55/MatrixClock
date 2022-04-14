CXXFLAGS=-Wall -O3 -g
OBJECTS=matrix_clock.cpp matrix_clock.h matrix_color.cpp matrix_font.cpp text_line.cpp time_period.cpp variable_utility.cpp telegram_handler.cpp matrix_data.cpp
BINARIES=matrix_clock

RGB_INCDIR=../include
RGB_LIBDIR=../lib
RGB_LIBRARY_NAME=rgbmatrix
RGB_LIBRARY=$(RGB_LIBDIR)/lib$(RGB_LIBRARY_NAME).a
LDFLAGS+=-L$(RGB_LIBDIR) -l$(RGB_LIBRARY_NAME) -lrt -lm -lpthread -lcurl -ljsoncpp -lTgBot -lboost_system -lssl -lcrypto -lpthread


all : matrix_clock

matrix_clock : $(OBJECTS) $(RGB_LIBRARY)
	$(CXX) $(CXXFLAGS) $(OBJECTS) -I$(RGB_INCDIR) -o $@ $(LDFLAGS)

$(RGB_LIBRARY): FORCE
	$(MAKE) -C $(RGB_LIBDIR)

%.o : %.cpp
	$(CXX) -I$(RGB_INCDIR) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJECTS) $(BINARIES)
	$(MAKE) -C $(RGB_LIBDIR) clean

FORCE:
.PHONY: FORCE