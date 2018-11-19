#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "min_heap.h"

#define INIT_CAPACITY 16
#define HEAP_SINKABLE_LEFT -1
#define HEAP_SINKABLE_RIGHT 1
#define HEAP_NOT_SINKABLE 0

static int sinkable(struct min_heap * h, int index, int index_left,
		int index_right);

struct event * create_event(int time, int job, int type)
{
	struct event * e = malloc(sizeof(struct event));
	e->time = time;
	e->job = job;
	e->type = type;

	return e;
}

/* Function to create and initialize a new heap */
struct min_heap * init_heap()
{
	struct min_heap * new_heap = malloc(sizeof(struct min_heap));
	new_heap->arr = malloc(sizeof(struct event *) * INIT_CAPACITY);
	new_heap->size = 0;
	new_heap->capacity = INIT_CAPACITY;

	for (int i = 0; i < new_heap->capacity; i++) {
		*(new_heap->arr + i) = NULL;
	}

	return new_heap;
}

/* Function to free a heap */
void kill_heap(struct min_heap * heap)
{
	/* Frees the events held in the array */
	for (int i = 0; i < heap->capacity; i++) {
		free(*(heap->arr + i));
	}

	/* Frees the array */
	free(heap->arr);

	/* Frees the heap */
	free(heap);
}

/* Function used to grow our array when it fills its capacity */
static void grow_array(struct min_heap * heap)
{
	/* Calculates new capacity and creates new array of this capacity */
	int new_capacity = heap->capacity * 2;
	struct event ** new_arr = malloc(sizeof(struct event *)
			* new_capacity);

	/* Copies data from old array to new array */
	for (int i = 0; i < heap->size; i++) {
		*(new_arr + i) = *(heap->arr + i);
	}

	/* Frees old array */
	free(heap->arr);

	/* Sets heaps array to the new array and updates to new capacity */
	heap->arr = new_arr;
	heap->capacity = new_capacity;
}

/* Utility function to swap two pointers in the pointer array, effectively
 * swapping the position of the structs they are pointing to */
static void swap_event(struct event ** a, struct event ** b)
{
	struct event * temp = *a;
	*a = *b;
	*b = temp;
}

/* Pushes data onto the heap */
void heap_push(struct min_heap * heap, struct event * e)
{
	/* Grow Array if needed */
	if (heap->size >= heap->capacity) {
		grow_array(heap);
	}

	/* Adds data entry to end of heap */
	*(heap->arr + heap->size) = e;

	/* Reorganizes heap */
	int index = heap->size; // Points to new item, very last in array
	int index_parent = (index - 1) / 2;
	struct event ** current = (heap->arr + index);
	struct event ** parent = (heap->arr + index_parent);
	/* CHECK'S FOR TOPS PARENT WHEN IT SHOULDN'T */
	while ((*current)->time < (*parent)->time) {
		swap_event(current, parent);

		index = (index - 1) / 2;
		index_parent = (index -1) / 2;
		current = (heap->arr + index);
		parent = (heap->arr + index_parent);
	}

	heap->size++;
}

struct event * heap_pop(struct min_heap * heap)
{
	/* Stores top value to be returned as retval */
	struct event * retval = *heap->arr;

	/* Swaps head and tail and deletes data in tail */
	struct event ** head = heap->arr;
	struct event ** tail = (heap->arr + (heap->size - 1));
	swap_event(head, tail);
	*tail = NULL;
	heap->size--;

	if (heap->size == 0) {
		return retval;
	}

	/* Reorganizes heap by sinking head */
	int index = 0;
	int index_left = (index * 2) + 1;
	int index_right = (index * 2) + 2;
	struct event ** current = (heap->arr + index);
	struct event ** left = (heap->arr + index_left);
	struct event ** right = (heap->arr + index_right);
	int x;
	while (x = sinkable(heap, index, index_left, index_right),
			x != HEAP_NOT_SINKABLE) {
		switch (x) {
		case HEAP_SINKABLE_LEFT :
			swap_event(left, current);
			index = (index * 2) + 1;
			break;
		case HEAP_SINKABLE_RIGHT :
			swap_event(right, current);
			index = (index * 2) + 2;
			break;
		default: 
			fprintf(stderr, "Error: Unknown Heap Sink code %d", x);
			exit(1);
		}

		index_left = (index * 2) + 1;
		index_right = (index * 2) + 2;
		current = (heap->arr + index);
		left = (heap->arr + index_left);
		right = (heap->arr + index_right);
	}

	return retval;
}

/* Function to determine whether or not a node in the heap can sink down, and if
 * so, which side it can sink down to. Will return HEAP_NOT_SINKABLE, if the
 * node should not sink. Will return HEAP_SINKABLE_LEFT, if it should sink left.
 * Will return HEAP_SINKABLE_RIGHT, if it should sink right.
 */
static int sinkable(struct min_heap * h, int index, int index_left,
		int index_right)
{
	struct event * current = *(h->arr + index);
	struct event * left = *(h->arr + index_left);
	struct event * right = *(h->arr + index_right);
	int retval = 0;
	int c_t = current->time;
	int l_t;
	int r_t;

	if ((h->arr + index_left) >= (h->arr + h->size)) {
		retval = HEAP_NOT_SINKABLE;
	} else if ((h->arr + index_right) >= (h->arr + h->size)) {
		l_t = left->time;
		if (l_t < c_t) {
			retval = HEAP_SINKABLE_LEFT;
		} else {
			retval =  HEAP_NOT_SINKABLE;
		}
	} else { // Neither left nor right are null
		l_t = left->time;
		r_t = right->time;
		if (l_t <= r_t && l_t < c_t) {
			retval = HEAP_SINKABLE_LEFT;
		} else if (r_t <= l_t && r_t < c_t) {
			retval = HEAP_SINKABLE_RIGHT;
		} else {
			retval = HEAP_NOT_SINKABLE;
		}
	}

	return retval;
}


bool heap_is_empty(struct min_heap * heap)
{
	if (heap->size == 0) {
		return true;
	}
	return false;
}

struct event * heap_peek(struct min_heap * heap)
{
	if (heap->size <= 0) {
		return NULL;
	}
	return *(heap->arr);
}

void print_event(struct event * e)
{
	if (e == NULL) {
		printf("<NULL>\n");
	} else {
		printf("time: %d, job%d, type: %d\n", e->time, e->job, e->type);
	}
}

void print_heap(struct min_heap * heap)
{
	for (int i = 0; i < heap->capacity; i++) {
		struct event * p = *(heap->arr + i);
		print_event(p);
	}
}
