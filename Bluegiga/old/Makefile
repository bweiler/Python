CC = gcc

TARGET = thermometer-demo

MACHINE = $(shell $(CC) -dumpmachine)
# Windows
ifneq (,$(or $(findstring mingw, $(MACHINE)), $(findstring cygwin, $(MACHINE))))
	PLATFORM = WIN
	LIBS = -lm -lsetupapi
# POSIX
else
	PLATFORM = POSIX
	LIBS = -lm
endif

SRCS := $(wildcard *.c)
OBJS := $(SRCS:.c=.o)
DEPS := $(SRCS:.c=.d)
INCLUDES := -IC:\Users\billb\AppData\Local\Programs\Python\Python37\include -lpython3 -lpython37 -LC:\Users\billb\AppData\Local\Programs\Python\Python37\libs

all: $(TARGET)

%.o: %.c
	$(CC) -O3 -Wall -c -fmessage-length=0 -DPLATFORM_$(PLATFORM) $(INCLUDES) -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"

$(TARGET): $(OBJS)
	@echo 'Building target: $@'
	$(CC) -o"$(TARGET)" $(OBJS) $(LIBS)
	@echo 'Finished building target: $@'

clean:
	-$(RM) $(OBJS) $(DEPS) $(TARGET)

.PHONY: all clean
