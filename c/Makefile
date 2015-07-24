all : acowc utest

common_SOURCES := acow_file.c acow_data.c acow_calc.c

acowc_SOURCES := acow.c $(common_SOURCES)
acowc_OBJS := $(patsubst %.c, %.o, $(acowc_SOURCES))
acowc : ${acowc_OBJS} Makefile
	gcc -o $@ ${acowc_OBJS}

utest_SOURCES := utest.c $(common_SOURCES)
utest_OBJS := $(patsubst %.c, %.o, $(utest_SOURCES))
utest : ${utest_OBJS} Makefile
	gcc -o $@ ${utest_OBJS}

clean :
	rm -f ${acowc_OBJS} ${utest_OBJS} acowc utest

%.o : %.c acow.h
	gcc -o $@ -c $< 
