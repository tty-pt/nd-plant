all := libnd-plant

LDLIBS-libnd-plant := -lxylem

# Sibling -I for a dev build. In CI the siblings do not exist and all headers
# come from the installed packages named in .github/workflows/ci.yml.
CFLAGS += -I$(shell cd .. && pwd)/axil-nd/include
CFLAGS += -I$(shell cd .. && pwd)/axil-nd-drink/include
CFLAGS += -I$(shell cd .. && pwd)/axil-nd-core/include

FOLDER := nd


# macOS ld rejects undefined symbols in shared libs, but WARN needs
# qsyslog: an engine-provided function pointer resolved at dlopen (Linux
# allows this by default). dynamic_lookup is the Darwin equivalent.
LDFLAGS-libnd-plant-Darwin += -undefined dynamic_lookup
-include ./../mk/include.mk
