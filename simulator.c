/*
 * simulator.c
 * Copyright (C) 2023 rzavalet <rzavalet@noemail.com>
 *
 * Distributed under terms of the MIT license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <raylib.h>
#include <math.h>
#include <assert.h>

#include "location.h"

#define WIDTH   900
#define HEIGHT  600

#define POPULATION_SIZE     500
#define THINK_TIME          1

#define FOR_STATUS_LIST(DO)   \
  DO(0, VULNERABLE)  \
  DO(1, INFECTED)    \
  DO(2, IMMUNE)      \
  DO(3, DEAD)

typedef enum
{
#define STATUS_ITEMS(id, name)  name = id,
  FOR_STATUS_LIST(STATUS_ITEMS)
#undef STATUS_ITEMS
  DISEASE_STATUS_COUNT
} disease_status;

const char *status_label[DISEASE_STATUS_COUNT] = {
#define STATUS_ITEMS(id, name)  #name,
  FOR_STATUS_LIST(STATUS_ITEMS)
#undef STATUS_ITEMS
};

disease_status disease_table[DISEASE_STATUS_COUNT] =
{ 
#define STATUS_ITEMS(id, name)  name,
  FOR_STATUS_LIST(STATUS_ITEMS)
#undef STATUS_ITEMS
};

typedef struct
{
  location_t      current_location;
  disease_status  status;
  int             remaining_infection_days;
} person_t;

#define INFECTION_DURATION    (200)
#define INFECTION_PROBABILITY (0.5f)

/* Disease dynamics */
#define NORMAL_FATALITY_RATE    (10)
#define INFECTION_PROXIMITY     (50.0)

#define SATURATION_THRESHOLD  (POPULATION_SIZE / 5)

person_t  people[POPULATION_SIZE];
int num_susceptible = 0;

location_t random_location(void)
{
  location_t location;

  location.x = rand() % WIDTH;
  location.y = rand() % HEIGHT;

  return location;
}

void init_people(void)
{
  for (size_t i = 0; i < sizeof(people)/sizeof(people[0]); i++) {
    people[i].current_location = random_location();
    disease_status tmp = rand() % 100 < 5 ? INFECTED : VULNERABLE;
    people[i].status = tmp;
    people[i].remaining_infection_days = INFECTION_DURATION;
    if (people[i].status == VULNERABLE || people[i].status == INFECTED) num_susceptible ++;
  }
}

void print_people_status(void)
{
  for (size_t i = 0; i < sizeof(people)/sizeof(people[0]); i++) {
    printf("(%d, %d) - %s\n", 
      people[i].current_location.x,
      people[i].current_location.y,
      status_label[people[i].status]);
  }
}

void draw_people(void)
{
  for (size_t i = 0; i < sizeof(people)/sizeof(people[0]); i++) {
    Color color;
    switch(people[i].status) {
      case VULNERABLE:
        color = WHITE;
        break;
      case INFECTED:
        color = RED;
        break;
      case IMMUNE:
        color = GREEN;
        break;
      case DEAD:
        color = BLACK;
        break;
      default:
        assert("Invalid status" == NULL);
    }


    DrawRectangle(people[i].current_location.x, 
                  people[i].current_location.y, 
                  5,
                  5,
                  color);
  }
}

double distance(location_t a, location_t b)
{
  return sqrt((a.x-b.x)*(a.x-b.x) + (a.y-b.y)*(a.y-b.y));
}

#define CONSTRAINT(x, min, max)   (x = (x < min ? min : (x > max ? max : x)))

void simulation_step()
{
  if (num_susceptible == 0) {
    const char *msg = "Simulation has finished";
    int text_size = MeasureText(msg, 30);
    DrawText(msg, WIDTH/2 - text_size/2, HEIGHT/2 - 30/2, 30, RED);
    return;
  }

  for (size_t i = 0; i < sizeof(people)/sizeof(people[0]) - 1; i++) {
    for (size_t j = i + 1; j < sizeof(people)/sizeof(people[0]); j++) {

      double d = distance(people[i].current_location, people[j].current_location);

      if (d <= INFECTION_PROXIMITY) {
        if (people[i].status == INFECTED) {
          if (-- people[i].remaining_infection_days <= 0) {
            if (rand() % 100 <= NORMAL_FATALITY_RATE) people[i].status = DEAD;
            else people[i].status = IMMUNE;
          }
        }
        if (people[j].status == INFECTED) {
          if (-- people[j].remaining_infection_days <= 0) {
            if (rand() % 100 <= NORMAL_FATALITY_RATE) people[j].status = DEAD;
            else people[j].status = IMMUNE;
          }
        }

        if (people[i].status == INFECTED && people[j].status == VULNERABLE) {
          people[j].status = INFECTED;
          people[j].remaining_infection_days = INFECTION_DURATION;
        }
        else if (people[j].status == INFECTED && people[i].status == VULNERABLE) {
          people[i].status = INFECTED;
          people[i].remaining_infection_days = INFECTION_DURATION;
        }

      }
    }
  }

  num_susceptible = 0;
  for (size_t i = 0; i < sizeof(people)/sizeof(people[0]); i++) {
    if (people[i].status == DEAD) continue;
    if (people[i].status == VULNERABLE || people[i].status == INFECTED) num_susceptible ++;

    people[i].current_location.x += (int)(rand() % 11) - 5;
    CONSTRAINT(people[i].current_location.x, 0, WIDTH);

    people[i].current_location.y += (int)(rand() % 11) - 5;
    CONSTRAINT(people[i].current_location.y, 0, HEIGHT);
  }
}

int main(void)
{
  srand(time(NULL));
  init_people();
  //print_people_status();

  InitWindow(WIDTH, HEIGHT, "Covid simulation");
  SetTargetFPS(60);                                 // Set target FPS (maximum)

  while (!WindowShouldClose()) {
    BeginDrawing();
      ClearBackground(BLUE);
      draw_people();
      simulation_step();
    EndDrawing();
  }

  CloseWindow();

  return 0;
}
