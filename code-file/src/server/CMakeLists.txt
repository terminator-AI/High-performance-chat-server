#定义SRC_LISTS变量，包含该目录下所有源文件
aux_source_directory(. SRC_LISTS)
aux_source_directory(./db DB_LISTS)
aux_source_directory(./model MODEL_LISTS)
aux_source_directory(./redis REDIS_LISTS)
#指定生产可执行文件
add_executable(ChatServer ${SRC_LISTS} ${DB_LISTS} ${MODEL_LISTS} ${REDIS_LISTS})

#指定可执行文件链接时需要的库文件
target_link_libraries(ChatServer muduo_net muduo_base mysqlclient hiredis pthread)