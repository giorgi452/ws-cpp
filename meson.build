project(
  'ws-cpp', 
  'cpp',
  version : '1.0.0',
  default_options : ['cpp_std=c++17']
)

inc = include_directories(['include', 'framework/include'])

executable(
  'ws-cpp', 
  [
    'src/main.cpp', 
    'src/routes.cpp', 
    'framework/http_request.cpp', 
    'framework/socket.cpp',
    'framework/not_found.cpp'
  ],
  include_directories: inc
)

