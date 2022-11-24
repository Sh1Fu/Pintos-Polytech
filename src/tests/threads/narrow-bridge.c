
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "tests/threads/tests.h"
#include "threads/thread.h"
#include "threads/synch.h"
#include "narrow-bridge.h"

struct semaphore directions_sema[2][2];		  // Matrix with all semaphores
struct semaphore bridge_sema;				  // Semaphore of bridge
uint16_t cars_counts[2][2];					  // Number of all types of cars
uint16_t occupied_places = 0;				  // Occupied places on bridge
uint16_t current_cars_count, all_active_cars; // Number of current all cars and non blocked cars
uint16_t current_dir;						  // Current bridge direction
uint16_t type;								  // Type of car
bool is_moving_started;

void narrow_bridge_init(void)
{
	is_moving_started = false;
	type = 0;
	sema_init(&bridge_sema, 1);
	for (int i = 0; i < 2; i++)
	{
		sema_init(&directions_sema[i][0], 0);
		sema_init(&directions_sema[i][1], 0);
	}
}

/*
Set two cars of one type if bridge if full emty
*/
void _up_two_to_bridge()
{
	for (uint8_t i = 0; i < 2; i++)
	{
		sema_up(&directions_sema[type][current_dir]);
		sema_down(&bridge_sema);
	}
	occupied_places += 2;
	type = 0;
}

void _up_solo_to_bridge()
{
	sema_down(&bridge_sema);
	sema_up(&directions_sema[type][current_dir]);
	sema_down(&bridge_sema);
	occupied_places++;
	type = 0;
}

/*
Move normal car with emergency if emergency is last on current direction
*/
void _last_emer_w_normal()
{
	sema_up(&directions_sema[1][current_dir]);
	sema_up(&directions_sema[0][current_dir]);
	sema_down(&bridge_sema);
	sema_down(&bridge_sema);
	occupied_places += 2;
}

/*
Choose correct cars to move
*/
void move_to_bridge()
{
	if (cars_counts[1][current_dir] >= 2) // We have two blocked emergency
	{
		type = 1;
		_up_two_to_bridge();
	}
	else if (cars_counts[1][current_dir] == 1) // Last emergency on current direction
	{
		if (cars_counts[0][current_dir] >= 1)
		{
			_last_emer_w_normal();
		}
		else
		{
			type = 1;
			_up_solo_to_bridge();
		}
	}
	else if (cars_counts[0][current_dir] >= 2) // We have two normal blocked cars and empty bridge
	{
		_up_two_to_bridge();
	}
	else if (cars_counts[0][current_dir] == 1) // We have the last car on current direction
	{
		_up_solo_to_bridge();
	}
	else
	{
		for (uint8_t i = 0; i < 2; i++)
		{
			sema_down(&bridge_sema);
		}
	}
}

void arrive_bridge(enum car_priority prio, enum car_direction dir)
{
	cars_counts[prio][dir]++;
	thread_yield(); // Count all types of cars
	if (!is_moving_started)
	{
		current_dir = ((cars_counts[1][0] > cars_counts[1][1] && cars_counts[1][0] != cars_counts[1][1]) ? dir_left : dir_right); // Choose start direction
		if (cars_counts[1][0] == cars_counts[1][1])
			current_dir = ((cars_counts[0][0] >= cars_counts[0][1]) ? dir_left : dir_right);
		all_active_cars = cars_counts[0][0] + cars_counts[0][1] + cars_counts[1][0] + cars_counts[1][1];
		is_moving_started = true;
	}
	if (all_active_cars > 1) // Block all cars and move them to their semaphores
	{
		all_active_cars--;
		sema_down(&directions_sema[prio][dir]);
	}

	if (all_active_cars == 1) // If last non blocked car then we should start "list"
	{
		all_active_cars--;
		for (uint8_t i = 0; i < 2; i++)
		{
			sema_up(&bridge_sema);
		}
		move_to_bridge();
		sema_down(&directions_sema[prio][dir]);
	}
}

void exit_bridge(enum car_priority prio, enum car_direction dir)
{
	cars_counts[prio][dir]--;
	current_cars_count = cars_counts[0][0] + cars_counts[0][1] + cars_counts[1][0] + cars_counts[1][1]; // Count all cars
	occupied_places--;
	if (current_cars_count && occupied_places % 2 == 0) // If we have cars and our bridge is not empty
	{
		if (!cars_counts[1][current_dir ^ 1] && cars_counts[1][current_dir])
			current_dir ^= 1;
		if ((current_dir == dir_right || current_dir == dir_left) && !(cars_counts[0][current_dir ^ 1] + cars_counts[1][current_dir ^ 1]))
			current_dir ^= 1;
		current_dir ^= 1;
		for (uint8_t i = 0; i < 2; i++) // Move cars from bridge
		{
			sema_up(&bridge_sema);
		}
		move_to_bridge();
	}
}
