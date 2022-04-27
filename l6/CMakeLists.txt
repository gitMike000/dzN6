cmake_minimum_required(VERSION 3.10)

project(cpp-network-lesson-6)


if (UNIX)
# To build this lesson code on Windows, libraries must be installed manually.

    add_subdirectory("qt_tcpclient")
    add_subdirectory("qt_tcpserver")

endif()
