CC = gcc
RM = rm
AR = ar

CFLAGS = -Wall -I./include
LFLAGS = -L./lib

CFLAGS += -g
LFLAGS += -g

LIBRARY = lib/libechonetll.a
SRCS = src/ell_packet.c src/ell_object.c src/connection.c src/ell_handle.c \
	src/ell_debug.c src/mwa_frame.c src/mwa_state.c src/mwa_uart.c \
	src/mwa_event.c src/mwa_echonet.c src/mwa_debug.c
OBJS = $(SRCS:.c=.o)
HEADERS = include/echonetlitelite.h include/connection.h \
	include/ell_config.h include/ell_depend.h include/ell_macros.h \
	include/ell_types.h include/ell_adapter.h

SAMPLE_DEV = ell_sample
SAMPLE_DEV_SRCS = app/ell_sample.c
SAMPLE_DEV_OBJS = $(SAMPLE_DEV_SRCS:.c=.o)

AIRCON = ell_aircon
AIRCON_SRCS = app/ell_aircon.c
AIRCON_OBJS = $(AIRCON_SRCS:.c=.o)

APP_REQ = ell_req
APP_REQ_SRCS = app/ell_req.c app/print_netif.c
APP_REQ_OBJS = $(APP_REQ_SRCS:.c=.o)

APP_SEARCH = ell_search
APP_SEARCH_SRCS = app/ell_search.c
APP_SEARCH_OBJS = $(APP_SEARCH_SRCS:.c=.o)

APP_SERVER = ell_frontend
APP_SERVER_SRCS = app/ell_frontend.c app/print_netif.c
APP_SERVER_OBJS = $(APP_SERVER_SRCS:.c=.o)

APP_ADAPTER = ell_adapter
APP_ADAPTER_SRCS = app/ell_adapter.c app/print_netif.c
APP_ADAPTER_OBJS = $(APP_ADAPTER_SRCS:.c=.o)

all:	$(LIBRARY) $(SAMPLE_DEV) $(AIRCON) $(APP_REQ) $(APP_SEARCH) $(APP_ADAPTER) $(APP_SERVER)

$(LIBRARY): $(OBJS)
	$(AR) r $@ $(OBJS)

$(SAMPLE_DEV):	$(SAMPLE_DEV_OBJS) $(LIBRARY)
	$(CC) $(LFLAGS) -o $@ $(SAMPLE_DEV_OBJS) -lechonetll

$(AIRCON):	$(AIRCON_OBJS) $(LIBRARY)
	$(CC) $(LFLAGS) -o $@ $(AIRCON_OBJS) -lechonetll

$(APP_REQ):	$(APP_REQ_OBJS) $(LIBRARY)
	$(CC) $(LFLAGS) -o $@ $(APP_REQ_OBJS) -lechonetll

$(APP_SEARCH):	$(APP_SEARCH_OBJS) $(LIBRARY)
	$(CC) $(LFLAGS) -o $@ $(APP_SEARCH_OBJS) -lechonetll

$(APP_SERVER):	$(APP_SERVER_OBJS) $(LIBRARY)
	$(CC) $(LFLAGS) -o $@ $(APP_SERVER_OBJS) -lechonetll

$(APP_ADAPTER):	$(APP_ADAPTER_OBJS) $(LIBRARY)
	$(CC) $(LFLAGS) -o $@ $(APP_ADAPTER_OBJS) -lechonetll -Wl,-Map=$(APP_ADAPTER).map

clean:
	$(RM) -f $(OBJS) $(SAMPLE_DEV_OBJS) $(AIRCON_OBJS) $(APP_REQ_OBJS) $(APP_SEARCH_OBJS) $(APP_SERVER_OBJS) $(APP_ADAPTER_OBJS)

cleanall:
	$(RM) -f $(SAMPLE_DEV) $(AIRCON) $(APP_REQ) $(APP_SEARCH) $(APP_SERVER) $(APP_ADAPTER)
	$(RM) -f $(LIBRARY)
	$(RM) -f $(OBJS) $(SAMPLE_DEV_OBJS) $(AIRCON_OBJS) $(APP_REQ_OBJS) $(APP_SEARCH_OBJS) $(APP_SERVER_OBJS) $(APP_ADAPTER_OBJS)

$(OBJS): $(HEADERS) makefile
$(SAMPLE_DEV_OBJS): $(HEADERS) makefile

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@
