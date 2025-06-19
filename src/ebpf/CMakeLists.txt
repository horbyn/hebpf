find_package(BpfObject REQUIRED)
file(GLOB source "*.bpf.c")
foreach(src ${source})
  get_filename_component(src_stem ${src} NAME_WE)

  bpf_object(${src_stem} ${src_stem}.bpf.c)
  add_dependencies(${src_stem}_skel libbpf-build bpftool-build)

  add_executable(${src_stem} ${src_stem}.c)
  target_link_libraries(${src_stem} ${src_stem}_skel)
endforeach()
