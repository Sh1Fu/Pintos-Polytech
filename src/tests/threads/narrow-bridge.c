#include <stdio.h>
#include <string.h>
#include "tests/threads/tests.h"
#include "threads/thread.h"
#include "threads/synch.h"
#include "narrow-bridge.h"

int all_cars[2][2];						 // An array of all machine types and numbers
int current_dir;						 // Variable responsible for the current movement direction
bool is_moving_started;					 // A simple Boolean variable to determine the first direction
struct semaphore bridge;				 // Semaphore to control bridge car's count
struct semaphore emer_sema, normal_sema; // Semaphores responsible for the number of machines of a certain type that we can let in (queue of machines of a certain type
struct lock data_lock;					 // A lock that blocks access to a critical section for data integrity
int void_emer, void_places;				 // emer_sema and normal_sema value fields kinda

void narrow_bridge_init(void)
{
	memset(all_cars, 0, sizeof(all_cars));
	current_dir = 0;
	is_moving_started = false;
	void_places = 2;
	void_emer = 0;
	sema_init(&bridge, 2);
	sema_init(&emer_sema, 0);
	sema_init(&normal_sema, 0);
	lock_init(&data_lock);
}

void arrive_bridge(enum car_priority prio, enum car_direction dir)
{
	all_cars[prio][dir]++; // Count the number of all machines by moving around the ready_list
	thread_yield();
	struct semaphore *current_car = NULL; // Temporary semaphore
	if (is_moving_started == false)		  // Starting direction identification
	{
		current_dir = ((all_cars[1][0] >= all_cars[1][1]) ? dir_left : dir_right);
		is_moving_started = true;
	}
	while (all_cars[1][0] + all_cars[1][1] != 0) // Find the emergency cars amongst all the cars to let them go ahead of you
	{
		if (prio == car_emergency && current_dir == dir) // If it is an emergency with the right direction of travel - send it to the bridge
		{
			if (--all_cars[prio][dir] == 0) // Change direction if there are no ambulances left in the right direction(Last ambulance!!)
				current_dir ^= 1;
			current_car = &emer_sema; // Taking a queue where we will write an ambulance if there is not enough room on the bridge
			break;
		}
		else
		{
			thread_yield(); // Looking for threads further
		}
	}
	// printf("Current dir and counts: %d %d %d\n\n", current_dir, all_cars[1][0], all_cars[1][1]);
	while (prio == car_normal)
	{
		if (current_dir == dir || all_cars[prio][dir ^ 1] == 0)
		{
			current_car = &normal_sema;
			if (--all_cars[prio][dir] == 0)
				current_dir ^= 1;
			break;
		}
		else
		{
			thread_yield();
		}
	}
	sema_up(current_car);
	if (current_car == &emer_sema)
	{
		void_emer++;
	}
	while (true)
	{
		lock_acquire(&data_lock);
		/*
		Condition blocks. Check that:

		The desired direction of travel
		OR
		The last ambulance from the chosen direction is going
		OR
		Bridge is empty
		И
		It is an ambulance
		OR
		There are no more ambulances to send to the bridge
		*/
		if (((current_dir == dir || (current_dir == dir ^ 1 && prio == car_emergency && all_cars[1][0] + all_cars[1][1] != 0) || void_places == 2) && (prio == car_emergency || void_emer == 0)))
		{
			sema_down(&bridge); // Sending the car to the bridge
			void_places--;
			sema_down(current_car);
			if (current_car == &emer_sema)
			{
				void_emer--;
			}
			lock_release(&data_lock);
			break;
		}
		lock_release(&data_lock);
		thread_yield(); // Looking at a new car
	}
}

void exit_bridge(enum car_priority prio UNUSED, enum car_direction dir UNUSED)
{
	sema_up(&bridge);
	void_places++;
}
