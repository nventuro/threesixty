# Processor
PART=LM4F120H5QR

# The base directory for StellarisWare
ROOT=../stellarisware

COMPILER=gcc

# The prefix for the tools to use
PREFIX=arm-none-eabi

# The command for calling the compiler.
CC=${PREFIX}-gcc

# Determine the compiler CPU/FPU options based on the processor variant.
CPU=-mcpu=cortex-m4
FPU=-mfpu=fpv4-sp-d16 -mfloat-abi=softfp

# The flags passed to the assembler.
AFLAGS=-mthumb \
       ${CPU}  \
       ${FPU}  \
       -MD

# The flags passed to the compiler.
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

# The command for calling the library archiver.
AR=${PREFIX}-ar

# The command for calling the linker.
LD=${PREFIX}-ld

# The flags passed to the linker.
LDFLAGS=--gc-sections

# Get the location of libgcc.a from the GCC front-end.
LIBGCC=${shell ${CC} ${CFLAGS} -print-libgcc-file-name}

# Get the location of libc.a from the GCC front-end.
LIBC=${shell ${CC} ${CFLAGS} -print-file-name=libc.a}

# Get the location of libm.a from the GCC front-end.
LIBM=${shell ${CC} ${CFLAGS} -print-file-name=libm.a}

# The command for extracting images from the linked executables.
OBJCOPY=${PREFIX}-objcopy

# Tell the compiler to include debugging information if the DEBUG environment
# variable is set.
ifdef DEBUG
CFLAGS+=-g -D DEBUG
endif

# Add the tool specific CFLAGS.
CFLAGS+=${CFLAGSgcc}

# Add the include file paths to AFLAGS and CFLAGS.
AFLAGS+=${patsubst %,-I%,${subst :, ,${IPATH}}}
CFLAGS+=${patsubst %,-I%,${subst :, ,${IPATH}}}

# The rule for building the object file from each C source file.
${COMPILER}${SUFFIX}/%.o: %.c
	${CC} ${CFLAGS} -D${COMPILER} -o ${@} ${<}

# The rule for building the object file from each assembly source file.
${COMPILER}${SUFFIX}/%.o: %.S
	${CC} ${AFLAGS} -D${COMPILER} -o ${@} -c ${<}

# The rule for creating an object library.
${COMPILER}${SUFFIX}/%.a:
	${AR} -cr ${@} ${^}

# The rule for linking the application.
${COMPILER}${SUFFIX}/%.axf:
	${LD} -T ${SCATTERgcc_${notdir ${@:.axf=}}} --entry ${ENTRY_${notdir ${@:.axf=}}} ${LDFLAGSgcc_${notdir ${@:.axf=}}} ${LDFLAGS} -o ${@} $(filter %.o %.a, ${^}) '${LIBM}' '${LIBC}' '${LIBGCC}'
	${OBJCOPY} -O binary ${@} ${@:.axf=.bin}


# Where to find source files that do not live in this directory.
VPATH=${ROOT}/boards/ek-lm4f120xl/drivers
VPATH+=${ROOT}/utils

# Where to find header files that do not live in the source directory.
IPATH=${ROOT}/boards/ek-lm4f120xl
IPATH+=${ROOT}

# The default rule, which causes the Project Zero Example to be built.
all: ${COMPILER}
all: ${COMPILER}/threesixty-display.axf

# The rule to clean out all the build products.
clean:
	@rm -rf ${COMPILER} ${wildcard *~}

# The rule to create the target directory.
${COMPILER}:
	@mkdir -p ${COMPILER}

# Rules for building the ThreeSixty Display.
${COMPILER}/threesixty-display.axf: ${COMPILER}/threesixty-display.o
${COMPILER}/threesixty-display.axf: ${COMPILER}/startup_${COMPILER}.o
${COMPILER}/threesixty-display.axf: ${ROOT}/driverlib/${COMPILER}-cm4f/libdriver-cm4f.a
${COMPILER}/threesixty-display.axf: threesixty-display.ld
SCATTERgcc_threesixty-display=threesixty-display.ld
ENTRY_threesixty-display=ResetISR
CFLAGSgcc=-DTARGET_IS_BLIZZARD_RA2

# Include the automatically generated dependency files.
ifneq (${MAKECMDGOALS},clean)
-include ${wildcard ${COMPILER}/*.d} __dummy__
endif
