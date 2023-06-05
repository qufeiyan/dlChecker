PREFIX =

CC := ${PREFIX}gcc
AR := $(PREFIX)ar

ECHO_COLOR=\033[4;96m
ECHO_COLOR_END=\033[0;m

# use libtool to generate lib.
USE_LIBTOOL = no

$(warning "gcc: $(CC)")
$(warning "ar: $(AR)")

CFLAGS += -g -fno-omit-frame-pointer -O0 -fdiagnostics-color=always 
LFLAGS += -lpthread -ldl -L.
IFLAGS += -I./include/
# DFLAGS += -DUSER_BACKTRACE
CFLAGS += -funwind-tables 
# DFLAGS += -DDLC_TEST
LSCRIPT += -Tmem.lds

CFLAGS += -fsanitize=address -Wall -Werror

# define the directory where the source codes are located.
SRCDIR = ./src
VPATH = $(SRCDIR)

ALL_SRCS = $(wildcard $(SRCDIR)/*.c)
NODIR_SRCS = $(notdir $(ALL_SRCS))

# Temporarily remove some source code files that
# do not participate in compliation.
EXCLUDE = test.c demo.c
# SRC = *.c

SRCS = $(filter-out $(EXCLUDE), $(NODIR_SRCS))
OBJS= $(patsubst %.c, %.o, $(SRCS))

LIB_NAME = dlChecker
LIB := lib${LIB_NAME}.a

TARGET = demo
UNITTEST = unit

.PHONY : clean 
all : desc $(OBJS) $(LIB) move $(TARGET)

desc :
	@echo "$(ECHO_COLOR)""lib name:" $(LIB) "$(ECHO_COLOR_END)"
	@echo "$(ECHO_COLOR)""src :" ${SRCS} "$(ECHO_COLOR_END)"
	@echo "$(ECHO_COLOR)""obj :" $(OBJS) "$(ECHO_COLOR_END)"

clean : 
	$(shell if [ -d objs ];then rm -rf ./objs; fi)
	$(shell if [ `ls *.o 2>/dev/null | wc -l` != 0 ]; then rm *o; fi)
	$(shell if [ `(ls *.a 2>/dev/null | wc -l)` != 0 ]; then rm *a; fi)
	$(shell if [ `ls *.dSYM 2>/dev/null | wc -l` != 0 ]; then rm -rf *.dSYM/; fi)
	$(shell if [ -e ${TARGET} ];then rm ${TARGET}; fi)
	$(shell if [ -e *.out ];then rm *.out; fi)

$(shell if [ ! -d ./objs ];then mkdir -p ./objs; fi)  

$(OBJS) : $(SRCS)
	$(CC) -c $^ $(IFLAGS) $(CFLAGS) $(DFLAGS)                      

move : $(OBJS)
	mv $^ ./objs/    

$(LIB) : $(OBJS) move
ifeq ($(USE_LIBTOOL), yes) 
	$(warning "use libtool to generate a static lib...")         
	libtool -static -o $@ $(addprefix ./objs/, $(OBJS)) 
else
	$(warning "use ar to generate a static lib...")
	$(AR) rcsv $@ $(addprefix ./objs/, $(OBJS)) 
endif

${TARGET} : demo.c libdlChecker.a 
	$(CC) $(LSCRIPT) $^ $(IFLAGS) $(LFLAGS) -l$(LIB_NAME) $(CFLAGS) $(DFLAGS) -o $@

test : test.c libdlChecker.a 
	$(CC) $^ $(IFLAGS) $(LFLAGS) -l$(LIB_NAME) $(CFLAGS) $(DFLAGS) -o $@

${UNITTEST} : unitTest.c libdlChecker.a 
	$(CC) $(LSCRIPT) $^ $(IFLAGS) $(LFLAGS) -l$(LIB_NAME) $(CFLAGS) $(DFLAGS) -o $@