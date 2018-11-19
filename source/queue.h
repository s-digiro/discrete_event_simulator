#ifndef QUEUE_H
#define QUEUE_H

struct node
{
	int job;
	int time;
	struct node * next;
	struct node * prev;
};

struct queue
{
	struct node * head;
	struct node * tail;
	int size;
};

struct queue * init_queue();
void kill_queue(struct queue * q);
void queue_push(struct queue * q, int t, int x);
struct node * queue_pop(struct queue * q);
struct node * queue_peek(struct queue * q);
bool queue_is_empty(struct queue * q);
void print_queue(struct queue * q);

#endif /* not defined QUEUE_H */
