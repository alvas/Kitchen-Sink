/*
 * mac_dump - dump memory from a target process
 *
 * Written by Dean Pucsek <dean@lightbulbone.com>
 * Copyright 2011, All rights reserved.
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <mach/mach.h>
#include <mach/mach_traps.h>
#include <mach/mach_vm.h>
#include <mach/kern_return.h>



int fork_calculator() {
  int pid = fork();
  if(pid < 0) {
    fprintf(stderr, "fork error\n");
    return -1;
  } else if(pid == 0) { /* child */
    execl("/Applications/Calculator.app/Contents/MacOS/Calculator", 
          "/Applications/Calculator.app/Contents/MacOS/Calculator", 
          NULL);
    perror("execl");
  } else { /* parent */
    sleep(1); /* this is necessary */
    return pid;
  }

  return -1;
}

int mem_dump(mach_port_name_t tport) {
  kern_return_t ret = 0;
  
  /* This is the starting address of text section of Calculator in Mac OS X 10.7.4 */
  mach_vm_address_t address = 0x100001c84;
  mach_vm_size_t size = 0;
  vm_region_basic_info_data_64_t info;
  mach_msg_type_number_t info_cnt = sizeof(vm_region_basic_info_data_64_t);
  mach_port_t object_name;

  /* User call to obtain information about a region in 1334  *  a task's address map. */
  ret = mach_vm_region(tport, &address, &size, VM_REGION_BASIC_INFO_64, (vm_region_info_t) & info, &info_cnt, &object_name);

  if (ret != KERN_SUCCESS)
  {
    fprintf(stderr, "mem_dump: failed to region data at 0x%lx (0x%x)\n", (long)address, ret);
    return -1;
  }

  pointer_t data;
  mach_msg_type_number_t data_size;

  ret = mach_vm_read(tport, address, size, &data, &data_size);

  if (ret != KERN_SUCCESS) {
    fprintf(stderr, "mem_dump: failed to read data at 0x%lx (0x%x)\n", (long)address, ret);
    return -1;
  }

  int fd;
  fd = open("mem.dump", O_CREAT|O_RDWR, S_IRWXU);
  if (fd == -1) {
    perror("open");
    return -1;
  }

  if (write(fd, (const void *)data, (size_t)data_size) == -1) {
    perror("write");
    return -1;
  }

  return 0;
}

int main(int argc, char **argv) {
  kern_return_t ret = -1;
  mach_port_name_t tport = -1;
  int tpid = -1;

  tpid = fork_calculator();
  
  ret = task_for_pid(mach_task_self(), tpid, &tport);
  if(ret != KERN_SUCCESS) {
    fprintf(stderr, "main: failed to get task for pid %d (%d)\n", tpid, ret);
    return -1;
  }

  if(mem_dump(tport) < 0) {
    fprintf(stderr, "main: failed to dump memory\n");
    return -1;
  }

  kill(tpid, SIGTERM);

  return 0;
}
