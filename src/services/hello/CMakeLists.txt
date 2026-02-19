file(GLOB sources *.bpf.c)
foreach(src ${sources})
  ebpf_copile(${src})
endforeach()
