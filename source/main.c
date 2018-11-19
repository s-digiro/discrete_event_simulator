#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>
#include "min_heap.h"
#include "queue.h"

/* These definitions are used to define what type of job an event is and used in
 * a case statement to decide how the simulation should handle each particular
 * event.
 */
#define SIM_FIN -1
#define JOB_ARRIVES 0
#define CPU_FINISHED 1
#define DISK1_FINISHED 2
#define DISK2_FINISHED 3

/* Structure to hold config values. */
struct config
{
	int seed;
	int init_time;
	int fin_time;
	int arrive_min;
	int arrive_max;
	double quit_prob;
	int cpu_min;
	int cpu_max;
	int disk1_min;
	int disk1_max;
	int disk2_min;
	int disk2_max;
};

/* Structure to hold statistic values. */
struct statistics
{
	int cpu_max;		// Largest size reached by cpu queue
	int d1_max;		// Largest size reached by disk1 queue
	int d2_max;		// Largest size reached by disk2 queue
	int cpu_cumul_len;	// Sum of all lengths of cpu queue
	int d1_cumul_len;	// Sum of all lengths of disk1 queue
	int d2_cumul_len;	// Sum of all lengths of disk2 queue
	int cpu_num_lens;	// Number of lengths summed for cpu queue
	int d1_num_lens;	// Number of lengths summed for disk1 queue
	int d2_num_lens;	// Number of lengths summed for disk2 queue
	int cpu_tot_busy_t;	// How much time the cpu was busy
	int d1_tot_busy_t;	// How much time disk1 was busy
	int d2_tot_busy_t;	// How much time disk2 was busy
	int sim_tot_t;		// Total time of simulation
	int cpu_tot_resp_t;	// Total response time of cpu
	int d1_tot_resp_t;	// Total response time of disk1
	int d2_tot_resp_t;	// Total response time of disk2
	int cpu_max_resp_t;	// Maximum response time from cpu
	int d1_max_resp_t;	// Maximum response time from disk1
	int d2_max_resp_t;	// Maximum response time from disk2
	int cpu_comp_jobs;	// Total number of completed jobs by cpu
	int d1_comp_jobs;	// Total number of completed jobs by disk1
	int d2_comp_jobs;	// Total number of completed jobs by disk2
};

/* Gets config values from config file and records these values to log file */
void parse_config(struct config * conf, FILE * log_file, FILE * stats_file);
/* Calcs when job time occurs given configuration, current time, and job type */
int calc_job_time(struct config * conf, int t, int x);
/* Calcs whether or not job should quit, given configuarion */
bool quit_job(struct config * conf);
/* Prints statistics to stats file */
void record_stats(struct statistics * stats, FILE * stats_file);
/* Creates config, and parses it, returns pointer to conf structure */
struct config * init_conf(FILE * log_file, FILE * stats_file);
/* Creates stats, and inits values properly, returns pointer to stats struct */
struct statistics * init_stats(struct config * conf);

/* Driver Method */
int main()
{
	/* SETUP */
	FILE * log_file;
	log_file = fopen("log", "a");
	fprintf(log_file, "STARTING NEW SIMULATION\n");
	fprintf(log_file, "~~~~~~~~~~~~~~~~~~~~~~~\n");

	FILE * stats_file;
	stats_file = fopen("stats", "a");
	fprintf(stats_file, "STARTING NEW SIMULATION\n");
	fprintf(stats_file, "~~~~~~~~~~~~~~~~~~~~~~~\n");

	struct config * conf = init_conf(log_file, stats_file);

	struct statistics * stats = init_stats(conf);

	struct queue * cpu = init_queue();
	struct queue * disk1 = init_queue();
	struct queue * disk2 = init_queue();
	struct min_heap * to_do = init_heap();
	srand(conf->seed);

	/* START SIMULATION */
	struct event * new_e;
	new_e = create_event(conf->fin_time, -1, SIM_FIN);
	heap_push(to_do, new_e);
	new_e = create_event(conf->init_time, 1, JOB_ARRIVES);
	heap_push(to_do, new_e);

	struct event * curr_e;	// Current event (changes with each pass)
	struct node * comp_job;	// Completed jobs when queues are popped
	int t;			// Current time (changes with each pass)
	int job;		// Current job number (Changes with each pass)
	int type;		// Current job type (Changes with each pass)
	int job_count = 1;	// Total number of jobs created (excluding end)
	int fin_t;		// Used to calculate fin times for certain jobs
	int cpu_start_work;	// Tracks start time of latest cpu job
	int d1_start_work;	// Tracks start time of latest disk1 job
	int d2_start_work;	// Tracks start timr of latest disk2 job
	while (!heap_is_empty(to_do)) {
		/* Get current event */
		curr_e = heap_pop(to_do);
		t = curr_e->time;
		job = curr_e->job;
		type = curr_e->type;

		/* Handle event */
		switch (type) {
		case JOB_ARRIVES :
			/* Current event is a job arrival event, so it
			 * must now be sent to the cpu or quit.
			 * Determining next job arrival is also handled here so
			 * that jobs arrive at more regular intervals. */

			/* Determing next job arrival and add the event to
			 * heap */
			fin_t = calc_job_time(conf, t, JOB_ARRIVES);
			new_e = create_event(fin_t, job_count + 1, JOB_ARRIVES);
			heap_push(to_do, new_e);

			fprintf(log_file, "%d: Job%d arrives\n", t, job);

			/* If cpu is idle, job can be handled immediately. If
			 * not, add to cpu queue and handle it later */
			if (queue_is_empty(cpu)) {
				fin_t = calc_job_time(conf, t, CPU_FINISHED);
				new_e = create_event(fin_t, job, CPU_FINISHED);
				heap_push(to_do, new_e);
				queue_push(cpu, t, job);

				cpu_start_work = t;
			} else {
				queue_push(cpu, t, job);
			}

			break;
		case CPU_FINISHED :
			/* Current event is a cpu finished event, so it must be
			 * sent to a disk, or abandoned if the job is finished
			 * (Whether it is finished or not is determined by
			 * quit_prob) */

			fprintf(log_file, "%d: Job%d finishes at CPU\n", t,
					job);

			comp_job = queue_pop(cpu);

			/* Some stat handling */
			stats->cpu_tot_busy_t += t - cpu_start_work;
			stats->cpu_tot_resp_t += t - comp_job->time;
			stats->cpu_comp_jobs++;
			if (stats->cpu_max_resp_t < t - comp_job->time) {
				stats->cpu_max_resp_t = t - comp_job->time;
			}

			/* Either quits job, or sends to disk1 or disk2
			 * (Whichever has less jobs queued)
			 */
			if (quit_job(conf)) {
				fprintf(log_file, "%d: Job%d quitting\n", t,
						job);
			} else if (disk1->size < disk2->size) {
				if (queue_is_empty(disk1)) {
					/* Create new event for heap */
					fin_t = calc_job_time(conf, t,
							DISK1_FINISHED);
					new_e = create_event(fin_t, job,
							DISK1_FINISHED);
					heap_push(to_do, new_e);
					queue_push(disk1, t, job);

					d1_start_work = t;
				} else {
					queue_push(disk1, t, job);
				}
			} else { // disk2 < disk1
				if (queue_is_empty(disk2)) {
					/* Create new event for heap */
					fin_t = calc_job_time(conf, t,
							DISK2_FINISHED);
					new_e = create_event(fin_t, job,
							DISK2_FINISHED);
					heap_push(to_do, new_e);
					queue_push(disk2, t, job);

					d2_start_work = t;
				} else {
					queue_push(disk2, t, job);
				}
			}

			/* Calculates finish time for new job at cpu, now that
			 * the cpu is finished with the old job
			 */
			if (!queue_is_empty(cpu)) {
				fin_t = calc_job_time(conf, t, CPU_FINISHED);
				new_e = create_event(fin_t,
						queue_peek(cpu)->job,
						CPU_FINISHED);
				heap_push(to_do, new_e);

				cpu_start_work = t;
			}

			break;
		case DISK1_FINISHED :
			/* Current event is a disk1 finished event, so it must
			 * be sent to the cpu
			 */

			fprintf(log_file, "%d: Job%d finishes at disk1\n", t,
					job);
			comp_job = queue_pop(disk1);

			/* Some stat handling */
			stats->d1_tot_busy_t += t - d1_start_work;
			stats->d1_tot_resp_t += t - comp_job->time;
			stats->d1_comp_jobs++;
			if (stats->d1_max_resp_t < t - comp_job->time) {
				stats->d1_max_resp_t = t - comp_job->time;
			}

			if (queue_is_empty(cpu)) {
				fin_t = calc_job_time(conf, t, CPU_FINISHED);
				new_e = create_event(fin_t, job, CPU_FINISHED);
				heap_push(to_do, new_e);
				queue_push(cpu, t, job);

				cpu_start_work = t;
			} else {
				queue_push(cpu, t, job);
			}

			/* Calculates finish time for new job at disk1 if it
			 * exists, now that disk1 is finished with the old job
			 */
			if (!queue_is_empty(disk1)) {
				fin_t = calc_job_time(conf, t, DISK1_FINISHED);
				new_e = create_event(fin_t,
						queue_peek(disk1)->job,
						DISK1_FINISHED);
				heap_push(to_do, new_e);

				d1_start_work = t;
			}

			free(comp_job);

			break;
		case DISK2_FINISHED :
			/* Current event is a disk2 finished event, so it must
			 * be sent to the cpu
			 */

			fprintf(log_file, "%d: Job%d finishes at disk2\n", t,
					job);
			comp_job = queue_pop(disk2);

			/* Some stat handling */
			stats->d2_tot_busy_t += t - d2_start_work;
			stats->d2_tot_resp_t += t - comp_job->time;
			stats->d2_comp_jobs++;
			if (stats->d2_max_resp_t < t - comp_job->time) {
				stats->d2_max_resp_t = t - comp_job->time;
			}

			if (queue_is_empty(cpu)) {
				fin_t = calc_job_time(conf, t, CPU_FINISHED);
				new_e = create_event(fin_t, job, CPU_FINISHED);
				heap_push(to_do, new_e);
				queue_push(cpu, t, job);

				cpu_start_work = t;
			} else {
				queue_push(cpu, t, job);
			}

			/* Calculates finish time for new job at disk2 if it
			 * exists, now that disk2 is finished with the old job
			 */
			if (!queue_is_empty(disk2)) {
				fin_t = calc_job_time(conf, t, DISK2_FINISHED);
				new_e = create_event(fin_t,
						queue_peek(disk2)->job,
						DISK2_FINISHED);
				heap_push(to_do, new_e);

				d2_start_work = t;
			}

			free(comp_job);

			break;
		case SIM_FIN :
			fprintf(log_file, "%d: Simulation Finished\n", t);
			goto EXIT_LOOP;
		default :
			fprintf(stderr, "unknown job type code\n");
			exit(1);
		}

		/* Statistics handling */
		if (stats->cpu_max < cpu->size) {
			stats->cpu_max = cpu->size;
		}
		stats->cpu_cumul_len += cpu->size;
		stats->cpu_num_lens ++;

		if (stats->d1_max < disk1->size) {
			stats->d1_max = disk1->size;
		}
		stats->d1_cumul_len += disk1->size;
		stats->d1_num_lens ++;

		if (stats->d2_max < disk2->size) {
			stats->d2_max = disk2->size;
		}
		stats->d2_cumul_len += disk2->size;
		stats->d2_num_lens ++;

		job_count++;
	}

EXIT_LOOP:

	/* END SIMULATION
	 * Add a new line to the log and stats to separate simulations and close
	 * the files
	 */
	fprintf(log_file, "\n\n\n");
	record_stats(stats, stats_file);
	fprintf(stats_file, "\n\n\n");
	fclose(log_file);
	fclose(stats_file);

	/* Free any malloced data */
	free(conf);
	free(stats);
	kill_heap(to_do);
	kill_queue(cpu);
	kill_queue(disk1);
	kill_queue(disk2);

	return 0;
}

/* Calcs when job time occurs given configuration, current time, and job type */
void parse_config(struct config * conf, FILE * log_file, FILE * stats_file)
{
	FILE * config_file = fopen("config", "r");

	if (config_file == NULL) {
		fprintf(stderr, "Error: Config file not found, exiting\n");
		exit(1);
	}

	char option[30];
	char value[10];
	while (fscanf(config_file, "%s %s", &option, &value) == 2) {
		if (strcmp(option, "SEED") == 0) {
			conf->seed = atoi(value);
		} else if (strcmp(option, "INIT_TIME") == 0) {
			conf->init_time = atoi(value);
		} else if (strcmp(option, "FIN_TIME") == 0) {
			conf->fin_time = atoi(value);
		} else if (strcmp(option, "ARRIVE_MIN") == 0) {
			conf->arrive_min = atoi(value);
		} else if (strcmp(option, "ARRIVE_MAX") == 0) {
			conf->arrive_max = atoi(value);
		} else if (strcmp(option, "QUIT_PROB") == 0) {
			conf->quit_prob = atof(value);
		} else if (strcmp(option, "CPU_MIN") == 0) {
			conf->cpu_min = atoi(value);
		} else if (strcmp(option, "CPU_MAX") == 0) {
			conf->cpu_max = atoi(value);
		} else if (strcmp(option, "DISK1_MIN") == 0) {
			conf->disk1_min = atoi(value);
		} else if (strcmp(option, "DISK1_MAX") == 0) {
			conf->disk1_max = atoi(value);
		} else if (strcmp(option, "DISK2_MIN") == 0) {
			conf->disk2_min = atoi(value);
		} else if (strcmp(option, "DISK2_MAX") == 0) {
			conf->disk2_max = atoi(value);
		}
		fprintf(log_file, "%s = %s\n", option, value);
		fprintf(stats_file, "%s = %s\n", option, value);
	}

	/* Checks to make sure that config values are valid */
	if (conf->init_time >= conf->fin_time) {
		fprintf(stderr, "Error: INIT_TIME must be less than FIN_TIME\n");
		exit(1);
	} else if (conf->arrive_min >= conf->arrive_max) {
		fprintf(stderr, "Error: ARRIVE_MIN must be less than ARRIVE_MAX\n");
		exit(1);
	} else if (conf->quit_prob < 0 || conf->quit_prob > 1) {
		fprintf(stderr, "Error: QUIT_PROB must be >= 0 and <= 1\n");
		exit(1);
	} else if (conf->cpu_min >= conf->cpu_max) {
		fprintf(stderr, "Error: CPU_MIN must be less than CPU_MAX\n");
		exit(1);
	} else if (conf->disk1_min >= conf->disk1_max) {
		fprintf(stderr, "Error: DISK1_MIN must be less than DISK1_MAX\n");
		exit(1);
	} else if (conf->disk2_min >= conf->disk2_max) {
		fprintf(stderr, "Error: DISK2_MIN must be less than DISK2_MAX\n");
		exit(1);
	}

	fprintf(stats_file, "\n");

	fclose(config_file);
}

/* Calcs whether or not job should quit, given configuarion */
int calc_job_time(struct config * conf, int t, int x)
{
	int fin_t= rand();

	switch (x) {
	case JOB_ARRIVES :
		fin_t %= (conf->arrive_max - conf->arrive_min);
		fin_t += (conf->arrive_min + t);
		break;
	case CPU_FINISHED :
		fin_t %= (conf->cpu_max - conf->cpu_min);
		fin_t += (conf->cpu_min + t);
		break;
	case DISK1_FINISHED :
		fin_t %= (conf->disk1_max - conf->disk1_min);
		fin_t += (conf->disk2_min + t);
		break;
	case DISK2_FINISHED :
		fin_t %= (conf->disk2_max - conf->disk2_min);
		fin_t += (conf->disk2_min + t);
		break;
	default :
		fprintf(stderr, "Error: Invalid job code for calc_job_time\n");
		exit(1);
		break;
	}

	return fin_t;
}

bool quit_job(struct config * conf)
{
	int x = rand() % 1000;

	double temp = conf->quit_prob * 1000;
	int quit_prob_i = (int) round(temp);
	if (x < quit_prob_i) {
		return true;
	}
	return false;
}

/* Prints statistics to stats file */
void record_stats(struct statistics * stats, FILE * stats_file)
{
	fprintf(stats_file, "CPU avg queue size = %lf\n",
			stats->cpu_cumul_len / (double) stats->cpu_num_lens);
	fprintf(stats_file, "CPU max queue size = %d\n", stats->cpu_max);
	fprintf(stats_file, "CPU utilization = %lf%%\n",
			(stats->cpu_tot_busy_t * 100)
			/ (double) stats->sim_tot_t);
	fprintf(stats_file, "CPU avg response time = %lf\n",
			stats->cpu_tot_resp_t / (double) stats->cpu_comp_jobs);
	fprintf(stats_file, "CPU max response time = %d\n",
			stats->cpu_max_resp_t);
	fprintf(stats_file, "CPU throughput = %lf per 100 units of time\n",
			(stats->cpu_comp_jobs * 100)
			/ (double) stats->sim_tot_t);
	fprintf(stats_file, "\n");

	fprintf(stats_file, "Disk1 avg queue size = %lf\n",
			stats->d1_cumul_len / (double) stats->d1_num_lens);
	fprintf(stats_file, "Disk1 max queue size = %d\n", stats->d1_max);
	fprintf(stats_file, "Disk1 utilization = %lf%%\n",
			(stats->d1_tot_busy_t * 100)
			/ (double) stats->sim_tot_t);
	fprintf(stats_file, "Disk1 avg response time = %lf\n",
			stats->d1_tot_resp_t / (double) stats->d1_comp_jobs);
	fprintf(stats_file, "Disk1 max response time = %d\n",
			stats->d1_max_resp_t);
	fprintf(stats_file, "Disk1 throughput = %lf per 100 units of time\n",
			(stats->d1_comp_jobs * 100)
			/ (double) stats->sim_tot_t);
	fprintf(stats_file, "\n");

	fprintf(stats_file, "Disk2 avg queue size = %lf\n",
			stats->d2_cumul_len / (double) stats->d2_num_lens);
	fprintf(stats_file, "Disk2 max queue size = %d\n", stats->d2_max);
	fprintf(stats_file, "Disk2 utilization = %lf%%\n",
			(stats->d2_tot_busy_t * 100)
			/ (double) stats->sim_tot_t);
	fprintf(stats_file, "Disk2 avg response time = %lf\n",
			stats->d2_tot_resp_t / (double) stats->d2_comp_jobs);
	fprintf(stats_file, "Disk2 max response time = %d\n",
			stats->d2_max_resp_t);
	fprintf(stats_file, "Disk2 throughput = %lf per 100 units of time\n",
			(stats->d2_comp_jobs * 100)
			/ (double) stats->sim_tot_t);
}

/* Creates config, and parses it, returns pointer to conf structure */
struct config * init_conf(FILE * log_file, FILE * stats_file)
{
	struct config * conf = malloc(sizeof(struct config));

	/* Default values in case they are undefined in file */
	conf->seed = 1234;
	conf->init_time = 0;
	conf->fin_time = 10000;
	conf->arrive_min = 1;
	conf->arrive_max = 7;
	conf->quit_prob = .2;
	conf->cpu_min = 1;
	conf->cpu_max = 5;
	conf->disk1_min = 50;
	conf->disk1_max = 500;
	conf->disk2_min = 50;
	conf->disk2_max = 500;

	parse_config(conf, log_file, stats_file);
	fprintf(log_file, "\n");

	return conf;
}

/* Creates stats, and inits values properly, returns pointer to stats struct */
struct statistics * init_stats(struct config * conf)
{
	struct statistics * stats = malloc(sizeof(struct statistics));
	stats->cpu_max = 0;
	stats->d1_max = 0;
	stats->d2_max = 0;
	stats->cpu_cumul_len = 0;
	stats->d1_cumul_len = 0;
	stats->d2_cumul_len = 0;
	stats->cpu_num_lens = 0;
	stats->d1_num_lens = 0;
	stats->d2_num_lens = 0;
	stats->cpu_tot_busy_t = 0;
	stats->d1_tot_busy_t = 0;
	stats->d2_tot_busy_t = 0;
	stats->sim_tot_t = conf->fin_time - conf->init_time;
	stats->cpu_tot_resp_t = 0;
	stats->d1_tot_resp_t = 0;
	stats->d2_tot_resp_t = 0;
	stats->cpu_max_resp_t = 0;
	stats->d1_max_resp_t = 0;
	stats->d2_max_resp_t = 0;
	stats->cpu_comp_jobs = 0;
	stats->d1_comp_jobs = 0;
	stats->d2_comp_jobs = 0;

	return stats;
}
