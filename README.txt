Sean DiGirolamo

Discrete Event Simulator


Background:
	This program simulated the discrete events that happen in a computer
	between a cpu, and 2 disks using queues and a min_heap. The events are
	stored on the min_heap and sorted by time, so that the earliest events
	happen first. The queues represent each node (cpu, disk1, disk2). In
	addition, numerous stats are recorded in a stat file about the average
	response time and such. A log file also records all notable events.
