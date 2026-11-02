all := libnd-plant

LDLIBS-libnd-plant := -lxylem -laxil-nd

FOLDER := nd


# macOS ld rejects undefined symbols in shared libs, but WARN needs
# stllog: an engine-provided function pointer resolved at dlopen (Linux
# allows this by default). dynamic_lookup is the Darwin equivalent.
LDFLAGS-libnd-plant-Darwin += -undefined dynamic_lookup

-include ./../mk/include.mk
