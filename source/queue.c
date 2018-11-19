#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include "queue.h"

struct queue * init_queue()
{
	struct queue * retval = malloc(sizeof(struct queue));
	retval->head = NULL;
	retval->tail = NULL;
	retval->size = 0;
	return retval;
}

void kill_queue(struct queue * q)
{
	/* Catches case where q is empty */
	if (q->size == 0) {
		free(q);
		return;
	}

	/* Catches case where q is size 1 */
	if (q->size == 1) {
		free(q->head);
		free(q);
		return;
	}

	/* All other cases */
	struct node * current = q->head->next;
	while (current->next != NULL) {
		free(current->prev);
		current = current->next;
	}
	free(current->prev);
	free(q->tail);
	free(q);
}

void queue_push(struct queue * q, int t, int x)
{
	/* Creates new node and sets data */
	struct node * new_node = malloc(sizeof(struct node));
	new_node->job = x;
	new_node->time = t;
	new_node->next = NULL;
	new_node->prev = NULL;

	/* Sets as new head and tail if queue is empty */
	if (q->size <= 0) {
		q->head = new_node;
		q->tail = new_node;
		q->size = 1;
	} else {
		/* All other cases */
		q->tail->next = new_node;
		new_node->prev = q->tail;
		q->tail = q->tail->next;
		q->size++;
	}
}

struct node * queue_pop(struct queue * q)
{
	/* Returns error if queue is already empty */
	if (q->size == 0) {
		fprintf(stderr, "Attempted to pop from an empty queue\n");
		exit(1);
	}

	struct node * retval = q->head;

	/* Empties queue if queue only contains 1 job */
	if (q->size == 1) {
		q->head = NULL;
		q->tail = NULL;
		q->size = 0;
		return retval;
	}

	/* All other cases */
	q->head = q->head->next;
	q->head->prev = NULL;
	q->size--;

	retval->next = NULL;
	retval->prev = NULL;
	return retval;
}

struct node * queue_peek(struct queue * q)
{
	/* Returns error if queue is empty */
	if (q->size == 0) {
		fprintf(stderr, "Attempted to peek at an empty queue\n");
		exit(1);
	}
	struct node * retval = q->head;

	return retval;
}

bool queue_is_empty(struct queue * q)
{
	if (q->size == 0) {
		return true;
	}
	return false;
}

void queue_print(struct queue * q)
{
	if (q->size == 0) {
		return;
	}

	struct node * current = q->head;
	printf("[");
	while (current != NULL) {
		printf("%d, ", current->job);
		current = current->next;
	}
	printf("]\n");
}
