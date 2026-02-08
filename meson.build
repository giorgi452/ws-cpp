project(
  'ws-cpp',
  'cpp',
  version : '1.0.0',
  default_options : ['cpp_std=c++17']
)

inc = include_directories(['include', 'framework/include'])

json_dep = dependency('nlohmann_json', fallback: ['nlohmann_json', 'nlohmann_json_dep'])

executable(
  'ws-cpp',
  [
    'src/main.cpp',
    'src/routes.cpp',
  ],
  include_directories: inc,
  dependencies: json_dep
)

