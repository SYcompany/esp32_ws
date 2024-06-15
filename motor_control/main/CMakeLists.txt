set(Src_DIR "src")
set(Header_DIR "include")


idf_component_register(SRCS "${Src_DIR}/main.c"
                    INCLUDE_DIRS "${Header_DIR}/.")