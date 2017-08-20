/* setschedclass.c - set scheduler class */

#include <kernel.h>
#include "sched.h"
#include "lab1.h"
void  setschedclass(int sched_class) {
	scheduler_class = sched_class;
}
