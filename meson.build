project('mangohud-server', 'cpp')

cpp = meson.get_compiler('cpp')

spdlog_options = [
  'default_library=static',
  'compile_library=true',
  'werror=false',
  'tests=disabled',
  'external_fmt=disabled',
  'std_format=disabled'
]

spdlog_sp = subproject('spdlog', default_options: spdlog_options)
spdlog_dep = spdlog_sp.get_variable('spdlog_dep')

json_dep = dependency('nlohmann_json')

add_project_arguments('-DSPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE', language: 'cpp')

subdir('server')
