/* sched.c */

#include <kernel.h>
#include <q.h>
#include <proc.h>
#include "sched.h"

int expdistsched_nextproc(int randomValue)
{
	int process, prev_process;

	process = q[rdytail].qprev;
	prev_process = q[process].qprev;

	while((randomValue < q[prev_process].qkey) && (prev_process < NPROC) ) {
		if(q[process].qkey != q[prev_process].qkey){
			process = prev_process;
		}
		prev_process = q[prev_process].qprev;
	}
	if(process >= NPROC)
		return (NULLPROC);
	else
		return (process);
}
