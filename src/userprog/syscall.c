#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

static void syscall_handler(struct intr_frame *);

void syscall_init(void)
{
  intr_register_int(0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler(struct intr_frame *f UNUSED)
{
  if (*(int *)f->esp == SYS_WRITE)
  {
    putbuf(((const char **)f->esp)[2], ((size_t *)f->esp)[3]);
    return;
  }
  else if (*(int *)f->esp == SYS_EXIT)
  {
    int exit_status = ((size_t *)f->esp)[1];
    thread_exit(); // вызов exit() должен завершать программу
  }
  else
  {
    printf("system call!\n");
    thread_exit();
  }
}
