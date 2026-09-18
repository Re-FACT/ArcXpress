# 
# Licensce Generation Makefile
# ----------------------------
# -s : Suppresss makefile output (e.g. entering/leaving directories)
# --output-sync target : For parallel compilation ensure output for each target is synchronized (make version >= 4.0)
MAKEFLAGS := -s
# Define executables
PYTHON_EXEC ?= python3
VERSION_FILE = VERSION.md
# License generation options
TMP_LCCGEN = projects
MD5_BIN = md5sum
LCCGEN_BIN = ${PWD}/bin/lccgen
LCC_PROJ_NAME = AAEE
LIC_DURATION=30
LIC_FEATURES=AAEE,EVAL
ifeq ($(LIC_TYPE),eval)
LIC_FEATURES = AAEE,EVAL
endif
ifeq ($(LIC_TYPE),prototype)
LIC_FEATURES =AAEE,EVAL,PROTOTYPE
endif
ifeq ($(LIC_TYPE),full)
LIC_FEATURES =AAEE,EVAL,PROTOTYPE,FULL
endif
LIC_FNAME=aaee_${LIC_TYPE}${LIC_DURATION}days.lic
LIC_FPATH=${PWD}/lic_files/${LIC_FNAME}

LCC_OPTIONS = -b
# Windows require exe in executable
# base64 option of license generator is not working on windows
ifeq ($(OS),Windows_NT)
MD5_BIN = md5sum.exe
LCC_OPTIONS=
endif

# Put it first so that "make" without argument is like "make help".
export COMMENT_EXTRACT

# Put it first so that "make" without argument is like "make help".
help:
	@echo "License generator"
	@echo "Available commands:"
	@${PYTHON_EXEC} -c "$$COMMENT_EXTRACT"

.PHONY: help

license:
# Generate license with option, LIC_DURATION=<num_of_days>, LIC_TYPE=[eval|prototype|full]
	echo "======== Clean up previous run ========";
	if [ -e ${LIC_FPATH} ]; then \
	    echo "Removed file: ${LIC_FPATH}" && \
		rm ${LIC_FPATH}; \
	fi
	echo "======== Running license generator ========";
	${LCCGEN_BIN} license issue ${LCC_OPTIONS} -o ${LIC_FPATH} -p ${TMP_LCCGEN}/${LCC_PROJ_NAME} --valid-from `date +%Y%m%d` --valid-to `date -d "+${LIC_DURATION} days" +"%Y%m%d"` -f ${LIC_FEATURES} -x 2025R1-20250618
	echo "License file outputted to ${LIC_FPATH}"
	echo "======== Generate MD5 for license ========";
	${MD5_BIN} ${LIC_FPATH} > ${LIC_FPATH}.md5
	echo "License MD5 file outputted to ${LIC_FPATH}.md5"
	echo "======== Done ========";

clean:
# Clean all generated license files
	echo "======== Clean up generated license files ========"
	rm -f ${LIC_FPATH} ${LIC_FPATH}.md5
	echo "======== Done ========";

# Functions to extract comments from Makefiles
define COMMENT_EXTRACT
import re
with open ('Makefile', 'r' ) as f:
    matches = re.finditer('^([a-zA-Z-_]*):.*\n#(.*)', f.read(), flags=re.M)
    for _, match in enumerate(matches, start=1):
        header, content = match[1], match[2]
        print(f"  {header:10} {content}")
endef
