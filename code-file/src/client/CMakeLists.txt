
aux_source_directory(. CLIENT_LISTS)

add_executable(Client ${CLIENT_LISTS})

target_link_libraries(Client pthread)