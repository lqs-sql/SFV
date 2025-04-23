cmake_minimum_required(VERSION 3.5)

#指定项目名（第一个参数名）
project(SFV VERSION 0.1 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
# 设置C++标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#相关组件查询
find_package(QT NAMES Qt6 Qt5 COMPONENTS Core Gui Widgets Network Sql PrintSupport REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core Gui Widgets Network Sql PrintSupport REQUIRED)

# 创建构建目录下的资源文件夹（可以给代码里随时使用资源文件夹里的文件，比如设置）
file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/resources")
# 复制资源文件到构建目录下的资源文件夹
file(GLOB_RECURSE RESOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/resources/*")#file(GLOB_RECURSE)命令递归地收集resources文件夹下的所有文件到RESOURCE_FILES变量
foreach(RESOURCE_FILE ${RESOURCE_FILES}) #遍历文件
    #使用get_filename_component获取文件名，最后用configure_file将文件复制到构建目录下的resources文件夹中，COPYONLY参数表示只复制文件，不进行变量替换等操作。
    get_filename_component(FILE_NAME ${RESOURCE_FILE} NAME)
    configure_file(${RESOURCE_FILE} "${CMAKE_CURRENT_SOURCE_DIR}/resources/${FILE_NAME}" COPYONLY)
endforeach()



#设置编译后的exe图标
# 设置图标RC资源文件（必须在RC资源文件里指定对应的ico文件，同时RC资源文件必须添加到编译资源里）
set(ICON_FILE "${CMAKE_CURRENT_SOURCE_DIR}/resources/logo.rc")
# 设置目标图标（必须指定为RC文件）
set_target_properties(${SFV} PROPERTIES WIN32_EXECUTABLE_ICON ${ICON_FILE})


#设置资源resources文件夹，存放qrc文件和对应资源文件
set(RESOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/resources/icon.qrc)
qt5_add_resources(RESOURCE_FILES_RCC ${RESOURCE_FILES})



#设置要编译进去资源的文件（先赋值给${PROJECT_SOURCES}变量，后使用qt_add_executable才是添加编译进去资源的方法）
set(PROJECT_SOURCES
        ${RESOURCE_FILES_RCC}
        main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
        CustomizeCmdTextEdit.h
        CustomizeCmdTextEdit.cpp
        commandwindow.h
        commandwindow.cpp
        commandwindow.ui
        notepad.h
        notepad.cpp
        notepad.ui
        notepad.qrc

        TreeNode.h
        TreeNode.cpp
        TreeUtil.cpp
        FileUtil.cpp
        Global.h
        Global.cpp
        StringUtil.h
        StringUtil.cpp
        UIUtil.cpp


        WorkThread.h
        ReadThread.h
        MonitorPathChangeThread.h
        WriteThread.h
        SftpConnectThread.h
        DownloadThread.h
        UploadThread.h

        LoginUser.h
        LoginUser.cpp
        LoginSession.h
        LoginSession.cpp
        SshCmd.h
        SshCmd.cpp
        SshCmdResult.h
        SshCmdResult.cpp
        Result.h
        Result.cpp
        ExecuteCommandResult.h
        ExecuteCommandResult.cpp
        SftpSession.h
        SftpSession.cpp


        VNCServerThread.h
        VNCClientThread.h

        vnc_server.h vnc_server.cpp
        vnc_client.h vnc_client.cpp
        VncWidget.h VncWidget.cpp


        videoserver.h videoserver.cpp

        QSshSocket.h
        QSshSocket.cpp
        MemoryModule.cpp
        MemoryModule.h
#        QProcessUtil.cpp
        ${ICON_FILE}
)


if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    #qt6添加可执行文件
    qt_add_executable(SFV
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET SFV APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        #android添加可执行文件
        add_library(SFV SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        #qt5添加可执行文件
        add_executable(SFV
            ${PROJECT_SOURCES}
        )
    endif()
endif()






# 设置编译后的Libssh2 库路径变量
set(Libssh2_ROOT "D:/software/profession/libssh2-master")
# 查找 libssh2 的导入库文件（通常是 .lib 文件）
find_library(LIBSSH2_LIBRARY
    NAMES libssh2
    PATHS ${Libssh2_ROOT}/build/src
)
# 查找 libssh2 的头文件目录
find_path(LIBSSH2_INCLUDE_DIR
    NAMES libssh2.h
    PATHS ${Libssh2_ROOT}/src
)
# 检查是否找到库和头文件
if(NOT LIBSSH2_LIBRARY)
    message(FATAL_ERROR "libssh2 library not found in ${Libssh2_ROOT}/build/src")
endif()
if(NOT LIBSSH2_INCLUDE_DIR)
    message(FATAL_ERROR "libssh2 headers not found in ${Libssh2_ROOT}/src")
endif()

# 包含 libssh2 的头文件目录
target_include_directories(SFV PRIVATE ${LIBSSH2_INCLUDE_DIR})





# 设置编译后的LibVNC库路径变量
set(LibVNC_ROOT "D:/software/profession/libvncserver-master")
# 查找libvncserver的导入库文件（通常是 .lib 或 .so 文件）
find_library(LIBVNC_SERVER_LIBRARY
    NAMES vncserver
    PATHS ${LibVNC_ROOT}/build
)
# 查找libvncclient的导入库文件（通常是 .lib 或 .so 文件）
find_library(LIBVNC_CLIENT_LIBRARY
    NAMES vncclient
    PATHS ${LibVNC_ROOT}/build
)
# 查找libvnc的头文件目录
find_path(LIBVNC_INCLUDE_DIR
    NAMES rfb/rfb.h
    PATHS ${LibVNC_ROOT}/include
)
# 检查是否找到库和头文件
if(NOT LIBVNC_SERVER_LIBRARY)
    message(FATAL_ERROR "libvncserver library not found in ${LibVNC_ROOT}/build")
endif()
if(NOT LIBVNC_CLIENT_LIBRARY)
    message(FATAL_ERROR "libvncclient library not found in ${LibVNC_ROOT}/build")
endif()
if(NOT LIBVNC_INCLUDE_DIR)
    message(FATAL_ERROR "libvnc headers not found in ${LibVNC_ROOT}/build/include/rfb")
endif()

# 包含libvnc的头文件目录
target_include_directories(SFV PRIVATE ${LIBVNC_INCLUDE_DIR})






# 设置编译后的 FFmpeg 库路径变量
set(FFMPEG_ROOT "D:/software/profession/ffmpeg")
##find_path(AVCODEC_INCLUDE_DIR
##    NAMES avcodec.h
##    PATHS ${FFMPEG_ROOT}/build/include/libavcodec
##    )
#find_library(AVCODEC_LIBRARY
#    NAMES avcodec-62
#    PATHS ${FFMPEG_ROOT}/build/libavcodec)

##find_path(AVFORMAT_INCLUDE_DIR
##    NAMES avformat.h
##    PATHS "D:/software/profession/ffmpeg/build/include/libavformat")
#find_library(AVFORMAT_LIBRARY
#    NAMES avformat-62
#    PATHS ${FFMPEG_ROOT}/build/libavformat)

##find_path(AVUTIL_INCLUDE_DIR
##    NAMES avutil.h
##    PATHS ${FFMPEG_ROOT}/build/include/libavutil)
#find_library(AVUTIL_LIBRARY
#    NAMES avutil-60
#    PATHS ${FFMPEG_ROOT}/build/libavutil
#)

##find_path(AVDEVICE_INCLUDE_DIR
##    NAMES avdevice.h
##    PATHS ${FFMPEG_ROOT}/build/include/libavdevice)
#find_library(AVDEVICE_LIBRARY
#    NAMES avdevice-62
#    PATHS ${FFMPEG_ROOT}/build/libavdevice
#)

##find_path(SWSCALE_INCLUDE_DIR
##    NAMES swscale.h
##    PATHS ${FFMPEG_ROOT}/build/include/libswscale
##)
#find_library(SWSCALE_LIBRARY
#    NAMES swscale-9
#    PATHS ${FFMPEG_ROOT}/build/libavformat
#)

#find_library(AVFORMAT_LIBRARY
#    NAMES avformat
#    PATHS "D:/software/profession/ffmpeg/build/lib")

#find_library(AVCODEC_LIBRARY
#    NAMES avcodec
#    PATHS "D:/software/profession/ffmpeg/build/lib")


#find_library(AVUTIL_LIBRARY
#    NAMES avutil
#    PATHS "D:/software/profession/ffmpeg/build/lib"
#)
#find_library(AVDEVICE_LIBRARY
#    NAMES avdevice
#    PATHS "D:/software/profession/ffmpeg/build/lib"
#)
#find_library(SWSCALE_LIBRARY
#    NAMES swscale
#    PATHS "D:/software/profession/ffmpeg/build/lib"
#)

#target_include_directories(SFV PRIVATE
#    ${FFMPEG_ROOT}/build/include) #${FFMPEG_ROOT}必须引入，否则依赖的avcodec.h里的libXXX.h会无法找到

#link_directories("D:/software/profession/ffmpeg/build/lib")

# 链接多个Qt组件
target_link_libraries(SFV PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Gui
     Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Sql Qt${QT_VERSION_MAJOR}::Network
     Qt${QT_VERSION_MAJOR}::PrintSupport
     ${LIBSSH2_LIBRARY}
     ${LIBVNC_SERVER_LIBRARY}
     ${LIBVNC_CLIENT_LIBRARY}

#     avcodec.dll		#音视频编码核心库 , 用于音视频或图片编解码
#             avdevice.dll	#硬件采集 , 加速 , 显示 库
#             avfilter.dll	#音视频滤波器
#             avformat.dll	#音视频封装格式生成或解析
#             avutil.dll		#该库中封装了一些公共的工具函数
#             postproc.dll	#封装了同步, 时间计算的简单算法
#             swresample.dll	#音视频编解码格式预设
#             swscale.dll		#原始视频格式转换库


     ws2_32 # 添加 Winsock 库链接（千万不能带后缀.dll）
)
# 打印包含目录
get_target_property(INCLUDE_DIRS SFV INCLUDE_DIRECTORIES)
message(STATUS "已经包含的.h目录路径，target_include_directories引入的路径: ${INCLUDE_DIRS}")

#MACOSX_BUNDLE_GUI_IDENTIFIER：用于指定 macOS 应用程序的唯一标识符。
#MACOSX_BUNDLE_BUNDLE_VERSION：用于指定 macOS 应用程序的版本号。
#MACOSX_BUNDLE_SHORT_VERSION_STRING：用于指定 macOS 应用程序的短版本号。
#MACOSX_BUNDLE：表示生成的目标是一个 macOS 应用程序捆绑包（bundle）。
#WIN32_EXECUTABLE：表示生成的目标是一个 Windows 可执行文件
#OUTPUT_NAME：指定编译后exe的中文名字
set_target_properties(SFV PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
    OUTPUT_NAME "Shell_File_Vnc工具"
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(SFV)
endif()
