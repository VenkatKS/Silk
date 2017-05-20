OUT_DIR = build/
MKDIR_P = mkdir -p

.PHONY: directories

all: directories *.c
	gcc -o build/silk.o *.c

directories: ${OUT_DIR}

${OUT_DIR}:
	${MKDIR_P} ${OUT_DIR}
