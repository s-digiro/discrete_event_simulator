#ifndef MIN_HEAP_C
#define MIN_HEAP_C

struct event
{
	int time;
	int job;
	int type;
};

struct min_heap
{
	struct event ** arr;
	int size;
	int capacity;
};

struct event * create_event(int time, int job, int type);
struct min_heap * init_heap();
void kill_heap(struct min_heap * heap);
void heap_push(struct min_heap * heap, struct event * e);
struct event * heap_pop(struct min_heap * heap);
bool heap_is_empty(struct min_heap * heap);
struct event * heap_peek(struct min_heap * heap);
void print_event(struct event * e);
void print_heap(struct min_heap * heap);

#endif
