cmake_minimum_required(VERSION 3.17)
project(IZP\ |\ Sheet\ \(1st\ project\)\ -\ Development C)

set(CMAKE_C_STANDARD 99)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Werror")
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")

add_executable(sheet_dev sheet.c)