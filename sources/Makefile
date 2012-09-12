
#
# NoSQL Benchmark Makefile.
#

CC = gcc
RM = rm

BUILD_CONFIG_CACHE = cache.mk

TARANTOOL_INCLUDE ?= ""
TARANTOOL_LIB ?= ""

-include ${BUILD_CONFIG_CACHE}

TARANTOOL_CFLAGS = -I${TARANTOOL_INCLUDE}
TARANTOOL_LIBS = -ltarantoolnet -ltarantoolsql -ltarantool
TARANTOOL_LFLAGS = -L${TARANTOOL_LIB} ${TARANTOOL_LIBS}

CFLAGS = -I. -Wall -g ${TARANTOOL_CFLAGS}
LFLAGS = ${TARANTOOL_LFLAGS} -pthread 

OBJECTS = nb_opt.o \
	  nb_config.o \
	  nb_db.o \
	  nb_db_tarantool.o \
	  nb_stat.o \
	  nb_workload.o \
	  nb_key.o \
	  nb_report.o \
	  nb_warmup.o \
	  nb_worker.o \
	  nb_engine.o \
	  nb.o

TARGET  = nb

all: check_env write_cache banner ${TARGET}
	@echo 
	@echo done

check_env:
	@if [ ${TARANTOOL_LIB} = "" ] || \
	    [ ${TARANTOOL_INCLUDE} = "" ] ; then \
		echo ; \
		echo "Please specify the following make options:" ; \
		echo TARANTOOL_LIB ; \
		echo TARANTOOL_INCLUDE ; \
		echo ; \
		exit 1 ; \
	fi

write_cache:
	@echo "TARANTOOL_LIB = ${TARANTOOL_LIB}" > ${BUILD_CONFIG_CACHE}
	@echo "TARANTOOL_INCLUDE = ${TARANTOOL_INCLUDE}" >> ${BUILD_CONFIG_CACHE}

banner:
	@echo 
	@echo NoSQL Benchmark build configuration.
	@echo	
	@echo TARANTOOL_LIB=${TARANTOOL_LIB}
	@echo TARANTOOL_INCLUDE=${TARANTOOL_INCLUDE}

$(TARGET): $(OBJECTS)
	@echo LINK $(TARGET)
	@$(CC) $(LFLAGS) $(OBJECTS) -o $(TARGET)

.c.o:
	@echo CC $<
	@$(CC) $(CFLAGS) -c $<

clean:
	$(RM) -f $(OBJECTS) $(TARGET)
