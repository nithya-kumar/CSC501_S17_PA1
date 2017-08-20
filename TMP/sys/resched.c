/* resched.c  -  resched */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include "lab1.h"
#include "math.h"
#include "sched.h"

unsigned long currSP;	/* REAL sp of current process */
extern int ctxsw(int, int, int, int);
/*-----------------------------------------------------------------------
 * resched  --  reschedule processor to highest priority ready process
 *
 * Notes:	Upon entry, currpid gives current process id.
 *		Proctab[currpid].pstate gives correct NEXT state for
 *			current process if other than PRREADY.
 *------------------------------------------------------------------------
 */
int resched()
{
	register struct	pentry	*optr;	/* pointer to old process entry */
	register struct	pentry	*nptr;	/* pointer to new process entry */
	int randomValue;
	int next_process;
	if (scheduler_class == EXPDISTSCHED) { 
		double lambda = 0.1; /* exponential distribution value */
		randomValue = (int) expdev(lambda);
		next_process = expdistsched_nextproc(randomValue); //fetch the next process based on the specified conditions

		/* no switch needed if current process priority is lower than all processes in the ready queue and greater than randomNumber*/
		if ( ( (optr= &proctab[currpid])->pstate == PRCURR) &&
		   ( ( (q[next_process].qkey > optr->pprio) && (optr->pprio > randomValue) ) || (next_process == NULLPROC) ) ) {
		#ifdef	RTCLOCK
			preempt = QUANTUM;		/* reset preemption counter	*/
		#endif
		return (OK);
		}
	} else if (scheduler_class == LINUXSCHED) {
		int epoch = FALSE;
		optr = &proctab[currpid];
		int optrPrio = optr->goodness - optr->counter;
		optr->goodness = optrPrio + preempt;
		optr->counter = preempt;
		if (optr->counter <= 0 || currpid == NULLPROC) {
			optr->counter = 0; //No CPU ticks left
			optr->goodness = 0; //Used up all quantum time
		}

		//Find the process which has maximum goodness from all runnable processes
		int max_goodness = 0;
		int i = 0;
		int new_process = 0;
		for (i = q[rdytail].qprev; i != rdyhead; i = q[i].qprev) {
			if (proctab[i].goodness > max_goodness) {
				new_process = i;
				max_goodness = proctab[i].goodness;
			}
		}
		if ((optr->pstate != PRCURR || optr->counter == 0) && max_goodness == 0) {
			if (epoch == FALSE) {
				epoch = TRUE;
				int k;
				struct pentry *p;
				for (k = 0; k < NPROC; k++) {
					p = &proctab[k];
					if (p->pstate == PRFREE)
						continue;
					else if (p->counter == 0 || p->counter == p->time_quantum) { // Process either exhausted its time quantum or not run at all;
						p->time_quantum = p->pprio;
					} else {
						p->time_quantum = p->pprio + (p->counter) / 2; // Base priority + 0.5 * unused quantum tick from prev epoch
					}
					p->counter = p->time_quantum;
					p->goodness =  p->pprio + p->counter; // Base priority + unused quantum tick from curr epoch
				}
				preempt = optr->counter;
				optr = &proctab[currpid];
			}
			/*
			 * Time quantum, counter and goodness are identified, now choose process with maximum goodness.
			 * optr is the current process, preempt is set to optr->counter
			 */
			if (max_goodness == 0) {
				if (currpid == NULLPROC) {
					return OK;
				} else {
					new_process = NULLPROC;
					if (optr->pstate == PRCURR) {
						optr->pstate = PRREADY;
						insert(currpid, rdyhead, optr->pprio);
					}
					nptr = &proctab[new_process];
					nptr->pstate = PRCURR;
					dequeue(new_process);
					currpid = new_process;
			
			#ifdef	RTCLOCK
					preempt = QUANTUM;
			#endif
					ctxsw((int) &optr->pesp, (int) optr->pirmask, (int) &nptr->pesp, (int) nptr->pirmask);
					return OK;
				}
			}
		} else if (optr->goodness > 0 && optr->goodness >= max_goodness && optr->pstate == PRCURR) {
			/*
			 * Current process has goodness to be maximum and its goodness is greater than 0.
			 * If goodness is 0, all processes have same goodness and they have to be scheduled
			 * in Round-Robin manner. Also, it can mean the process has used up its time quantum.
			 * So, the process need not be scheduled in this epoch.
			 */
			preempt = optr->counter;
			return(OK);
		}
		/*
		 * For processes with goodness greater than zero, get last one from ready list and run it.
		 * Then run the next process from the last of ready list with same max_goodness. If any process enters the ready list
		 * it will not be executed until the next epoch as it has goodness value 0.
		 */
		else if (max_goodness > 0 && (optr->pstate != PRCURR || optr->counter == 0 || optr->goodness < max_goodness)) {
			if (optr->pstate == PRCURR) {
				optr->pstate = PRREADY;
				insert(currpid, rdyhead, optr->pprio);
			}
			nptr = &proctab[new_process];
			nptr->pstate = PRCURR;
			dequeue(new_process);
			currpid = new_process;
			preempt = nptr->counter;
			ctxsw((int) &optr->pesp, (int) optr->pirmask, (int) &nptr->pesp, (int) nptr->pirmask);
			return(OK);
		} else {
			return SYSERR;
		}
	} else {
		if ( ( (optr= &proctab[currpid])->pstate == PRCURR) && (lastkey(rdytail) < optr->pprio) ) {
			return(OK);
		}
	}

	/* force context switch */
	if (optr->pstate == PRCURR) {
		optr->pstate = PRREADY;
		insert(currpid,rdyhead,optr->pprio);
	}

	if (scheduler_class == EXPDISTSCHED) {
		/* remove the next highest priority process from remaining of ready list */
		nptr = &proctab[(currpid = expdistsched_last(next_process)) ];
	} else {
		/* remove highest priority process at end of ready list */
		nptr = &proctab[ (currpid = getlast(rdytail)) ];
	}
	nptr->pstate = PRCURR;		/* mark it currently running	*/
	#ifdef	RTCLOCK
		preempt = QUANTUM;		/* reset preemption counter	*/
	#endif
	
	ctxsw((int)&optr->pesp, (int)optr->pirmask, (int)&nptr->pesp, (int)nptr->pirmask);
	
	/* The OLD process returns here when resumed. */
	return OK;
}
