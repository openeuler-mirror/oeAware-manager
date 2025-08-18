# set module name
project(smc_acc)

# set kernerl dir
set(KDIR /lib/modules/${KERNEL_VERSION}/build)


function(compile_module obj)
    set(TARGET_NAME "${obj}")
    add_custom_target(${TARGET_NAME} ALL cp -f ${CMAKE_CURRENT_SOURCE_DIR}/*.c ${CMAKE_CURRENT_BINARY_DIR}/
        COMMAND cp -f ${CMAKE_CURRENT_SOURCE_DIR}/*.c ${CMAKE_CURRENT_BINARY_DIR}/
        COMMENT "compiling module ${obj}.ko..."
        )
    add_custom_command(TARGET ${TARGET_NAME}
        POST_BUILD
        COMMAND echo "obj-m := ${obj}.o" > ${CMAKE_CURRENT_BINARY_DIR}/Makefile
        COMMAND make -C ${KDIR} M=${CMAKE_CURRENT_BINARY_DIR} modules
        COMMAND cp -f ${CMAKE_CURRENT_BINARY_DIR}/${obj}.ko ${PLUGIN_OBJECT_OUTPUT_DIRECTORY}/${obj}.ko
        )
endfunction()

compile_module(smc_acc)
