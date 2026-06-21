idf_component_register(
    SRCS
        "opsec.c"
        "mbedtls_host_mem.c"
    INCLUDE_DIRS "."
    REQUIRES mbedtls log
)

# On the linux host target the linker processes libopsec.a before
# libmbed-builtin.a.  Because mbedtls_host_mem.c.o is never *called*
# by anything early in the link, the archive extractor skips it.
# Later, platform.c in libmbed-builtin.a references esp_mbedtls_mem_*
# as data-section relocations, but libopsec.a has already been walked.
#
# -Wl,-u,<sym> tells the linker to treat the symbol as undefined from
# the very start of the link, which forces it to search all archives
# for a definition and pull in mbedtls_host_mem.c.o from libopsec.a.
if(IDF_TARGET STREQUAL "linux")
    target_link_options(${COMPONENT_LIB} INTERFACE
        -Wl,-u,esp_mbedtls_mem_calloc
        -Wl,-u,esp_mbedtls_mem_free
    )
endif()
