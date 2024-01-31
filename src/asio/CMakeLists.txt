cmake_minimum_required (VERSION 3.11)

find_package(Boost REQUIRED)
find_package(sisl REQUIRED)

add_executable(test_main)
target_sources(test_main PRIVATE
      test_main.cpp
      rpc_client.cpp
      session.cpp
      workloads.cpp
)
target_link_libraries (test_main
   Boost::Boost
   sisl::sisl
)

add_executable(asio_server)
target_sources(asio_server PRIVATE
      server_main.cpp
      rpc_server.cpp
      session.cpp
)
target_link_libraries (asio_server
   Boost::Boost
   sisl::sisl
)
