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


void init_wrr_rq(struct wrr_rq *wrr_rq, int cpu){
	struct rq *rq = cpu_rq(cpu);

	wrr_rq -> rq = rq;
	raw_spin_lock_init(&wrr_rq->wrr_lock);
	wrr_rq -> total_weight = 0;
	wrr_rq -> wrr_nr_running = 0;
	INIT_LIST_HEAD(&wrr_rq->entity_list);
}


static inline struct task_struct *wrr_task_of(struct sched_wrr_entity *wrr_se)
{
	return container_of(wrr_se, struct task_struct, wrr);
}

static void enqueue_wrr_entity(struct sched_wrr_entity * wrr_se,struct rq *rq, bool HEAD){
	struct wrr_rq *wrr_rq;

	wrr_rq = &rq->wrr;

	if (HEAD)
		list_add(&wrr_se->run_list,&wrr_rq->entity_list);
	else
		list_add_tail(&wrr_se->run_list,&wrr_rq->entity_list);

}



static void dequeue_task_wrr(struct rq *rq, struct task_struct *p, int flags)
{
	struct wrr_rq *wrr_rq;
    struct sched_wrr_entity *wrr_se = &p->wrr;
    if (wrr_se == NULL)
	    return;
    wrr_rq = &rq->wrr;

    raw_spin_lock(&wrr_rq->wrr_lock);
    list_del(&wrr_se->run_list);
    dec_nr_running(rq);
    wrr_rq->wrr_nr_running--;
    wrr_rq->total_weight -= wrr_se->weight;
    // wrr_rq_weight(&rq->wrr);
    raw_spin_unlock(&wrr_rq->wrr_lock);

    // rq->wrr.nr_running--; ??
    /*To Do: SMP steal tasks from other cpu, if wrr_rt is empty*/

}

static void enqueue_task_wrr(struct rq * rq,struct task_struct *p,int flags){
	struct wrr_rq *wrr_rq;
	struct sched_wrr_entity *wrr_se = &p->wrr;
	if (wrr_se == NULL)
		return;
	wrr_rq = &rq->wrr;

	// printk("Second in\n");

	raw_spin_lock(&wrr_rq->wrr_lock);
	enqueue_wrr_entity(wrr_se, rq, flags & ENQUEUE_HEAD);
	// printk("Third in\n");
	wrr_rq->wrr_nr_running++;
	wrr_rq->total_weight += wrr_se->weight;
	wrr_se->time_slice = wrr_se->weight * QUANTUM;
	raw_spin_unlock(&wrr_rq->wrr_lock);
	inc_nr_running(rq);

	// printk("4th in\n");
}

static struct sched_wrr_entity *pick_next_wrr_entity(struct rq *rq,
						     struct wrr_rq *wrr_rq)
{
	struct sched_wrr_entity *next = NULL;
	next = list_entry(wrr_rq->entity_list.next, struct sched_wrr_entity, run_list);
	WARN_ON_ONCE(wrr_rq->entity_list.next == &wrr_rq->entity_list);
	return next;
}

static struct task_struct *pick_next_task_wrr(struct rq *rq)
{
	
	struct wrr_rq *wrr_rq = &rq->wrr;
	struct sched_wrr_entity* wrr_se;
	struct task_struct *p;
	
	/* according to fair */
	if (wrr_rq->wrr_nr_running == 0) {
		return NULL;
	}
	wrr_se = pick_next_wrr_entity(rq, wrr_rq); /* do we need do while here */
	p = wrr_task_of(wrr_se);
	/* in fair */
	// if (hrtick_enabled(rq))
	// 	hrtick_start_wrr(rq, p);
	return p;
}


static void task_tick_wrr(struct rq *rq, struct task_struct *p, int queued)
{
	struct wrr_rq *wrr_rq;
	struct sched_wrr_entity *wrr_se = &p->wrr; /* or curr->wrr? */
	/* we don't need to deal with for each, it's for group */


	if (--p->wrr.time_slice)
		return;
	//printk("%d\n",p->wrr.time_slice);
	wrr_rq = &rq->wrr;
	if (wrr_se->weight > 1) {/* ? */
		--wrr_se->weight;
		raw_spin_lock(&wrr_rq->wrr_lock);
		//printk("kkkkkkkkkkkk\n");
		wrr_rq->total_weight--;
		raw_spin_unlock(&wrr_rq->wrr_lock);
	}
	p->wrr.time_slice = p->wrr.weight * QUANTUM;
	// wrr_rq_weight(wrr_rq);

	printk("Weight %lu\n",wrr_se->weight);

	/* when will this be false? */
	if (wrr_se->run_list.prev != wrr_se->run_list.next) {
		/* as in 'requeue_rt_entity' */
		list_move_tail(&wrr_se->run_list, &wrr_rq->entity_list); 
		set_tsk_need_resched(p); /* ? */
		//resched_task(p); /* here's locker things, maybe better */
	}

}

static void task_fork_wrr(struct task_struct *p){
	struct rq *rq = this_rq();
	unsigned long flags;
	// int cpu = smp_processor_id();

	raw_spin_lock_irqsave(&rq->lock,flags);

	p->wrr.wrr_rq = &rq->wrr;
	// rcu_read_lock();
	// __set_task_cpu(p, this_cpu);
	// rcu_read_unlock();

	raw_spin_unlock_irqrestore(&rq->lock, flags);
}


static void yield_task_wrr(struct rq *rq)
{
	struct task_struct *p = rq->curr;
	struct sched_wrr_entity *wrr_se = &p->wrr;
	struct wrr_rq *wrr_rq = &rq->wrr;
	if (!list_empty(&wrr_rq->entity_list))
		list_move_tail(&wrr_se->run_list, &wrr_rq->entity_list);
}

void idle_balance_wrr(int this_cpu, struct rq *this_rq)
{
	struct task_struct *tsk = NULL;
	struct rq *remote_rq;
	struct sched_wrr_entity *wrr_se;
	int each_cpu;

	for_each_possible_cpu(each_cpu) {

		if (each_cpu == this_cpu)
			continue;
		remote_rq = cpu_rq(each_cpu);

		double_lock_balance(this_rq, remote_rq);
		if(remote_rq->wrr.wrr_nr_running<2) {
			double_unlock_balance(this_rq,remote_rq);
			continue;
		}

		list_for_each_entry(wrr_se, &remote_rq->wrr.entity_list, run_list) {
			tsk = wrr_task_of(wrr_se);
			if(!task_running(remote_rq, tsk) && cpumask_test_cpu(this_cpu, &tsk->cpus_allowed))
			//if(tsk != remote_rq->curr && cpumask_test_cpu(this_cpu, &tsk->cpus_allowed))
				break;
			else {
				
				tsk = NULL;
				continue;
			}

		}

		if(tsk == NULL) {
			double_unlock_balance(this_rq,remote_rq);
			continue;
		}

		deactivate_task(remote_rq, tsk, 0);
		set_task_cpu(tsk, this_cpu);
		activate_task(this_rq, tsk, 0);

		double_unlock_balance(this_rq,remote_rq);
		break;
	}

}


static void check_preempt_curr_wrr(struct rq *rq, struct task_struct *p, int flags)
{

}
static void put_prev_task_wrr(struct rq *rq, struct task_struct *p)
{

}
static void set_curr_task_wrr(struct rq *rq)
{

}

static unsigned int get_rr_interval_wrr(struct rq *rq, struct task_struct *task)
{

        return 0;
}

static void prio_changed_wrr(struct rq *rq, struct task_struct *p, int oldprio)
{

}

static void switched_to_wrr(struct rq *rq, struct task_struct *p)
{

}

static void pre_schedule_wrr(struct rq *rq, struct task_struct *prev)
{

}

static void post_schedule_wrr(struct rq *rq)
{

}

#ifdef CONFIG_SMP

static int select_task_rq_wrr(struct task_struct *p, int sd_flag, int flags)
{
	struct rq *rq;
	int cpu;
	int new_cpu;
	int min_weight;

	cpu = task_cpu(p);

	if (p->nr_cpus_allowed == 1)
		goto out;

	/* For anything but wake ups, just return the task_cpu */
	if (sd_flag != SD_BALANCE_WAKE && sd_flag != SD_BALANCE_FORK)
		goto out;
	WARN_ON_ONCE(p->policy != SCHED_WRR);

	rq = cpu_rq(cpu);

	rcu_read_lock();

	min_weight = rq->wrr.total_weight;

	for_each_possible_cpu(new_cpu){
		rq = cpu_rq(new_cpu);
		if (rq->wrr.total_weight < min_weight)
		{
			min_weight = rq->wrr.total_weight;
			cpu = new_cpu;
		}


	}
	rcu_read_unlock();

out:
	return cpu;
}

#endif

const struct sched_class wrr_sched_class = {
	/* most important */
	.next 			= &fair_sched_class,
	.enqueue_task           = enqueue_task_wrr,
	.dequeue_task		= dequeue_task_wrr,
	.task_tick		= task_tick_wrr,
	.task_fork		= task_fork_wrr,
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
