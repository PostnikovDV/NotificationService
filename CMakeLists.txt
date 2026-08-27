cmake_minimum_required(VERSION 3.15)
project(NotificationService)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(WIN32)
    add_compile_definitions(_WIN32_WINNT=0x0A00)
    add_compile_definitions(WINVER=0x0A00)
endif()

# Проверка существования файлов
set(SOURCE_FILES
    src/main.cpp
    src/Session/SessionManager.cpp
    src/Session/SessionManager.h
    src/Session/WebSocketSession.h
    src/Session/WebSocketSessionImpl.cpp
    src/Session/WebSocketSessionImpl.h
    src/Server/NotificationServer.cpp
    src/Server/NotificationServer.h
    src/Kafka/KafkaConsumer.cpp
    src/Kafka/KafkaConsumer.h
)

# Проверяем существование файлов
foreach(FILE ${SOURCE_FILES})
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${FILE}")
        message(WARNING "File not found: ${FILE}")
    endif()
endforeach()

set(PCH_HEADER src/stdafx.h)

add_executable(NotificationService ${SOURCE_FILES})

source_group("Main" FILES src/main.cpp)

source_group("Session" FILES 
    src/Session/SessionManager.cpp
    src/Session/SessionManager.h
    src/Session/WebSocketSession.h
    src/Session/WebSocketSessionImpl.cpp
    src/Session/WebSocketSessionImpl.h
)

source_group("Server" FILES 
   src/Server/NotificationServer.cpp
    src/Server/NotificationServer.h
)

source_group("Kafka" FILES 
  src/Kafka/KafkaConsumer.cpp
    src/Kafka/KafkaConsumer.h
)


source_group("PCH" FILES ${PCH_HEADER})

target_precompile_headers(NotificationService PRIVATE ${PCH_HEADER})

# ===== BOOST =====
# Для vcpkg используем find_package с CONFIG
find_package(Boost CONFIG REQUIRED COMPONENTS system asio beast uuid)
if(Boost_FOUND)
    message(STATUS "Boost found: ${Boost_VERSION}")
else()
    message(FATAL_ERROR "Boost not found!")
endif()

# ===== nlohmann_json =====
find_package(nlohmann_json 3.2.0 CONFIG REQUIRED)
if(TARGET nlohmann_json::nlohmann_json)
    message(STATUS "nlohmann_json found")
else()
    message(FATAL_ERROR "nlohmann_json not found!")
endif()

find_package(RdKafka CONFIG QUIET)

if(RdKafka_FOUND)
    message(STATUS "librdkafka found!")
    set(HAVE_KAFKA ON)
else()
    message(WARNING "librdkafka not found! Kafka support disabled.")
    message(WARNING "Install with: vcpkg install librdkafka:x64-windows")
    set(HAVE_KAFKA OFF)
endif()

# ===== Линковка =====
target_link_libraries(NotificationService PRIVATE
    Boost::system
    Boost::asio
    Boost::beast
    nlohmann_json::nlohmann_json
    RdKafka::rdkafka    
    RdKafka::rdkafka++  
)

target_compile_options(NotificationService PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/bigobj>)

message(STATUS "Build configuration complete!")