cmake_minimum_required (VERSION 3.11)

find_package(Boost REQUIRED)
find_package(sisl REQUIRED)

add_executable(asio_client)
target_sources(asio_client PRIVATE
      rpc_client.cpp
)
target_link_libraries (asio_client
   Boost::Boost
   sisl::sisl
)

add_executable(asio_server)
target_sources(asio_server PRIVATE
      rpc_server.cpp
)
target_link_libraries (asio_server
   Boost::Boost
   sisl::sisl
)
