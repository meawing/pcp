#pragma once

#include <unistd.h>
#include <syscall.h>

#include <linux/futex.h>

int FutexWait(void *addr, int val);

int FutexWake(void *addr, int count);
