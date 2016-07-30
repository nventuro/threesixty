# Processor
PART=LM4F120H5QR

# The base directory for StellarisWare
STELLARISWARE_DIR=stellarisware

COMPILER=gcc

OUT_DIR=out

# The prefix for the tools to use
PREFIX=arm-none-eabi

# The command for calling the compiler
CC=${PREFIX}-gcc

# The compiler CPU/FPU options
CPU=-mcpu=cortex-m4
FPU=-mfpu=fpv4-sp-d16 -mfloat-abi=softfp

# The flags passed to the assembler
AFLAGS=-mthumb \
       ${CPU}  \
       ${FPU}  \
       -MD

# The flags passed to the compiler
CFLAGS=-mthumb             \
       ${CPU}              \
       ${FPU}              \
       -Os                 \
       -ffunction-sections \
       -fdata-sections     \
       -MD                 \
       -std=c99            \
       -Wall               \
       -pedantic           \
       -DPART_${PART}      \
       -c


# Tell the compiler to include debugging information if the DEBUG environment variable is set
ifdef DEBUG
CFLAGS+=-g -D DEBUG
endif

# The command for calling the library archiver
AR=${PREFIX}-ar

# The command for calling the linker
LD=${PREFIX}-ld

# The flags passed to the linker
LDFLAGS=--gc-sections

# Get the location of libgcc.a from the GCC front-end
LIBGCC=${shell ${CC} ${CFLAGS} -print-libgcc-file-name}

# Get the location of libc.a from the GCC front-end
LIBC=${shell ${CC} ${CFLAGS} -print-file-name=libc.a}

# Get the location of libm.a from the GCC front-end
LIBM=${shell ${CC} ${CFLAGS} -print-file-name=libm.a}

# The command for extracting images from the linked executables
OBJCOPY=${PREFIX}-objcopy

# Add the tool specific CFLAGS
CFLAGS+=${CFLAGSgcc}

# Add the include file paths to AFLAGS and CFLAGS
AFLAGS+=${patsubst %,-I%,${subst :, ,${IPATH}}}
CFLAGS+=${patsubst %,-I%,${subst :, ,${IPATH}}}

# Where to find source files that do not live in this directory.
VPATH=${STELLARISWARE_DIR}/boards/ek-lm4f120xl/drivers
VPATH+=${STELLARISWARE_DIR}/utils

# Where to find header files that do not live in the source directory.
IPATH=${STELLARISWARE_DIR}/boards/ek-lm4f120xl
IPATH+=${STELLARISWARE_DIR}

SCATTERgcc_threesixty-display=src/threesixty-display.ld
ENTRY_threesixty-display=ResetISR
CFLAGSgcc=-DTARGET_IS_BLIZZARD_RA2

# The rule for building the object file from each C source file
${OUT_DIR}/%.o: src/%.c
	${CC} ${CFLAGS} -D${COMPILER} -o ${@} ${<}

# The rule for building the object file from each assembly source file
${OUT_DIR}/%.o: src/%.S
	${CC} ${AFLAGS} -D${COMPILER} -o ${@} -c ${<}

# The rule for creating an object library
${OUT_DIR}/%.a:
	${AR} -cr ${@} ${^}

# The rule for linking the application
${OUT_DIR}/%.axf:
	${LD} -T ${SCATTERgcc_${notdir ${@:.axf=}}} --entry ${ENTRY_${notdir ${@:.axf=}}} ${LDFLAGSgcc_${notdir ${@:.axf=}}} ${LDFLAGS} -o ${@} $(filter %.o %.a, ${^}) '${LIBM}' '${LIBC}' '${LIBGCC}'
	${OBJCOPY} -O binary ${@} ${@:.axf=.bin}

# The default rule, which causes the project be built
all: ${OUT_DIR}
all: ${OUT_DIR}/threesixty-display.axf

# The rule to create the output directory
${OUT_DIR}:
	mkdir -p ${OUT_DIR}

# The rule to clean out all the build products
clean:
	@rm -rf ${OUT_DIR} ${wildcard *~}

# Rules for building the ThreeSixty Display subproject
${OUT_DIR}/threesixty-display.axf: ${OUT_DIR}/main.o
${OUT_DIR}/threesixty-display.axf: ${OUT_DIR}/startup_${COMPILER}.o
${OUT_DIR}/threesixty-display.axf: ${STELLARISWARE_DIR}/driverlib/${COMPILER}-cm4f/libdriver-cm4f.a

# Include the automatically generated dependency files
ifneq (${MAKECMDGOALS},clean)
-include ${wildcard ${OUT_DIR}/*.d} __dummy__
endif
