/*
	weather.h ~ RL
	Simulates weather found in-game
*/

#pragma once

#include "file.h"

void weather_start(dnr_weather_type_t type, int spawn_rate, float speed);
void weather_end(void);
void weather_simulate(float delta_time);

void weather_set_scroll(int scroll);
void weather_set_asset(const asset_t* asset);
void weather_set_state(const dnr_state_t* state);