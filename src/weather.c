/*
	weather.c ~ RL
	Simulates weather found in-game.
	TO DO: This is not fully accurate, this merely recreates what it might look like.
*/

#include "weather.h"
#include "screen.h"

/* this is the count in the real Dig-N-Rig */
#define WEATHER_PARTICLE_COUNT 12000

struct weather_particle
{
	float x, y;
	CHAR_INFO visual;
	float x_vel, y_vel;
	bool exists;
} particles[WEATHER_PARTICLE_COUNT];

static dnr_weather_type_t type;
static int spawn_rate;
static float speed;
static bool started;
static int scroll;

void weather_start(dnr_weather_type_t _type, int _spawn_rate, float _speed)
{
	if (started)
	{
		weather_end();
	}
	if (_type == WEATHER_NONE || _spawn_rate == 0)
	{
		return;
	}
	started = true;
	type = _type;
	spawn_rate = _spawn_rate;
	speed = _speed;
}

void weather_end(void)
{
	started = false;
	memset(particles, 0, sizeof particles);
}

void weather_set_scroll(int _scroll)
{
	scroll = _scroll;
}

static inline int weather_random(int min, int max)
{
	return min + rand() % (max - min + 1);
}

static void weather_create_particle(int x, int y, dnr_weather_type_t type)
{
	for (int i = 0; i < WEATHER_PARTICLE_COUNT; i++)
	{
		if (!particles[i].exists)
		{
			particles[i].x = (float)x;
			particles[i].y = (float)y;
			if (type == WEATHER_RAIN)
			{
				particles[i].visual = (CHAR_INFO){ .Char = '|', .Attributes = CREATE_ATTRIBUTE(LIGHT_BLUE, DARK_BLACK) };
				particles[i].x_vel = weather_random(5, 100) * speed;
				particles[i].y_vel = weather_random(150, 220);
			}
			else if (type == WEATHER_LAVA)
			{
				particles[i].visual = (CHAR_INFO){ .Char = 0x0F, .Attributes = CREATE_ATTRIBUTE(LIGHT_RED, DARK_BLACK) };
				particles[i].x_vel = weather_random(5, 30) * speed;
				particles[i].y_vel = weather_random(25, 60);
			}
			particles[i].exists = true;
			break;
		}
	}
}

static void weather_spawn_more_particles(void)
{
	int count = 0;
	for (int i = 0; i < WEATHER_PARTICLE_COUNT; i++)
	{
		if (particles[i].exists)
		{
			count++;
		}
	}
	if (count >= WEATHER_PARTICLE_COUNT)
	{
		return;
	}
	dnr_weather_type_t particle_type = type;
	if (type == WEATHER_RAIN_AND_LAVA)
	{
		particle_type = weather_random(0, 1) == 0 ? WEATHER_RAIN : WEATHER_LAVA;
	}
	for (int i = 0; i < spawn_rate; i++)
	{
		weather_create_particle(weather_random(-50, WORLD_WIDTH), weather_random(scroll - 10, 0), particle_type);
	}
}

static void weather_update_and_render_particles(float delta_time)
{
	for (int i = 0; i < WEATHER_PARTICLE_COUNT; i++)
	{
		if (!particles[i].exists)
		{
			continue;
		}
		particles[i].x += particles[i].x_vel * delta_time;
		particles[i].y += particles[i].y_vel * delta_time;
		if (particles[i].y >= WORLD_HEIGHT || particles[i].x >= WORLD_WIDTH)
		{
			particles[i].exists = false;
		}
		int px = (int)particles[i].x;
		int py = (int)particles[i].y;
		screen_set_char_region(&particles[i].visual.Char.AsciiChar, (region_t) { px, py, px, py });
		screen_set_attrib_region(&particles[i].visual.Attributes, (region_t) { px, py, px, py });
	}
}

void weather_simulate(float delta_time)
{
	if (!started)
	{
		return;
	}

	weather_spawn_more_particles();
	weather_update_and_render_particles(delta_time);
}