CMAKE_COMMAND := cmake
SOURCE_DIR :=${PWD}
BUILD_DIR ?= build
BUILD_TYPE ?= release

ifeq (${BUILD_TYPE}, debug)
	CMAKE_FLAGS := -DCMAKE_BUILD_TYPE=debug
endif

build_example:
	@mkdir -p ${BUILD_DIR}
	echo "cd ${BUILD_DIR} && ${CMAKE_COMMAND} ${CMAKE_FLAGS} ${SOURCE_DIR}"
	cd ${BUILD_DIR} && ${CMAKE_COMMAND} ${CMAKE_FLAGS} ${SOURCE_DIR}
	@+${MAKE} -C ${BUILD_DIR}
clean:
	rm -rf ${BUILD_DIR}
run_example: build_example
	cd ${BUILD_DIR} && echo "============ Running the Example ============" && ./example && echo "============= Finished  Running ============="
