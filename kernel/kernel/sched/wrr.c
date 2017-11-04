#include <linux/latencytop.h>
#include <linux/sched.h>
#include <linux/cpumask.h>
#include <linux/slab.h>
#include <linux/profile.h>
#include <linux/interrupt.h>
#include <linux/mempolicy.h>
#include <linux/migrate.h>
#include <linux/task_work.h>

#include <trace/events/sched.h>

#include "sched.h"

#define QUANTUM 10


static void dequeue_task_wrr(struct rq *rq, struct task_struct *p, int flags)
{
    struct sched_wrr_entity *wrr_se = &p->wrr;
    list_del(&wrr_se->run_list);
    dec_nr_running(rq);
    // rq->wrr.nr_running--; ??
    wrr_se->wrr_rq->total_weight-=wrr_se->weight;
    /*To Do: SMP steal tasks from other cpu, if wrr_rt is empty*/

}

static void wrr_rq_weight(struct wrr_rq * wrr_rq){
	struct list_head *p;
	struct sched_wrr_entity * wrr_se;
	unsigned long sum = 0;

	list_for_each_entry(p,&wrr_rq->entity_list){
		wrr_se = list_entry(p,struct sched_wrr_entity,run_list);
		sum += wrr_se->weight;
	}

	wrr_rq -> total_weight = sum;
}


static void enqueue_wrr_entity(struct sched_wrr_entity *wrr_se, int HEAD){
	struct wrr_rq *wrr_rq;

	wrr_rq = wrr_rq_of(wrr_se);

	if (HEAD)
		list_add(&wrr_rq->run_list,&wrr_rq->entity_list);
	else
		list_add_tail(&wrr_rq->run_list,&wrr_rq->entity_list);

}

static void enqueue_task_wrr(struct rq * rq,struct task_struct *p,int flags){
	struct sched_wrr_entity *wrr_se = &p->wrr;

	enqueue_wrr_entity(wrr_se, flags & ENQUEUE_HEAD);

	inc_nr_running(rq);
}

static struct sched_wrr_entity *pick_next_wrr_entity(struct rq *rq,
						     struct wrr_rq *wrr_rq)
{
	struct sched_wrr_entity *next = NULL;
	next = list_entry(wrr_rq->entity_list.next, struct sched_wrr_entity, run_list);
	return next;
}

static struct task_struct *pick_next_task_wrr(struct rq *rq)
{
	struct wrr_rq *wrr_rq = &rq->wrr;
	struct sched_wrr_entity* wrr_se;
	struct task_struct *p;
	
	/* according to fair */
	wrr_se = pick_next_wrr_entity(rq, wrr_rq); /* do we need do while here */
	p = task_of(wrr_se);
	/* in fair */
	// if (hrtick_enabled(rq))
	// 	hrtick_start_wrr(rq, p);
	return p;
}

static inline struct wrr_rq *wrr_rq_of(struct sched_entity *se)
{
	return se->wrr_rq;
}

static void task_tick_wrr(struct rq *rq, struct task_struct *p, int queued)
{
	struct wrr_rq *wrr_rq;
	struct sched_wrr_entity *wrr_se = &p->wrr; /* or curr->wrr? */
	/* we don't need to deal with for each, it's for group */
	if (--p->wrr.time_slice)
		return;
	p->wrr.time_slice = p->wrr.weight * QUANTUM;

	/* when will this be false? */
	if (wrr_se->run_list.prev != wrr_se->run_list.next) {
		/* as in 'requeue_rt_entity' */
		wrr_rq = wrr_rq_of(p);
		list_move_tail(&wrr_se->run_list, &wrr_rq->entity_list); 
		// set_tsk_need_resched(p); /* ? */
		resched_task(p); /* here's locker things, maybe better */
	}
}

const struct sched_class wrr_sched_class = {
	/* most important */
	.next = &fair_sched_class,
	.enqueue_task           = enqueue_task_wrr,
	.dequeue_task		= dequeue_task_wrr,
	.task_tick		= task_tick_wrr,
	.pick_next_task		= pick_next_task_wrr,
	.yield_task		= yield_task_wrr,
#ifdef CONFIG_SMP
	.select_task_rq		= select_task_rq_wrr,
	.pre_schedule		= pre_schedule_wrr,
	.post_schedule		= post_schedule_wrr,
#endif
	/* need to define, maybe empty */
	.check_preempt_curr	= check_preempt_curr_wrr,
	.put_prev_task		= put_prev_task_wrr, /* do nothing */
	.set_curr_task		= set_curr_task_wrr,
	.get_rr_interval	= get_rr_interval_wrr,
	.prio_changed		= prio_changed_wrr,
	.switched_to		= switched_to_wrr,
};
