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
#include <string.h>


/* Disease dynamics */
#define POPULATION_SIZE         (1000)
#define DAY_LENGTH              (24)
#define INFECTION_DURATION      (15 * DAY_LENGTH)
#define INFECTION_PROBABILITY   (1)
#define NORMAL_FATALITY_RATE    (10)
#define SATURATED_FATALITY_RATE (50)
#define INFECTION_PROXIMITY     (10.0f)
#define SATURATION_THRESHOLD    (POPULATION_SIZE / 5)


/* Health system policies */
#define SOCIAL_DISTANCING (0x01)
#define ISOLATION         (0x02)


/* Display settings */
#define SCALE             100
#define WIDTH             (16 * SCALE)
#define HEIGHT            (9 * SCALE)
#define SIMULATION_WIDTH  (WIDTH/2)
#define SIMULATION_HEIGHT (HEIGHT)
#define GRAPH_WIDTH       (WIDTH - SIMULATION_WIDTH)
#define GRAPH_HEIGHT      (HEIGHT)
#define FONT_SIZE         (30)
#define LINE_SPACING      (10)
#define COLUMN_SPACING    (20)



#define CONSTRAINT(x, min, max)   (x = (x < min ? min : (x > max ? max : x)))

#define FOR_STATUS_LIST(DO)   \
  DO(0, VULNERABLE, WHITE)    \
  DO(1, INFECTED, RED)        \
  DO(2, IMMUNE, GREEN)        \
  DO(3, DEAD, BLACK)

typedef enum
{
#define STATUS_ITEMS(id, name, color) name = id,
  FOR_STATUS_LIST(STATUS_ITEMS)
#undef STATUS_ITEMS
  DISEASE_STATUS_COUNT
} disease_status;

const char *status_label[DISEASE_STATUS_COUNT] = {
#define STATUS_ITEMS(id, name, color)  #name,
  FOR_STATUS_LIST(STATUS_ITEMS)
#undef STATUS_ITEMS
};

Color color_map[DISEASE_STATUS_COUNT] =
{
#define STATUS_ITEMS(id, name, color)  color,
  FOR_STATUS_LIST(STATUS_ITEMS)
#undef STATUS_ITEMS
};


typedef struct
{
  int x;
  int y;

} location_t;

typedef struct
{
  location_t      current_location;
  disease_status  status;
  int             remaining_infection_days;
} person_t;

typedef struct 
{
  unsigned int *infected; 
  size_t  count;
  size_t  capacity;
} disease_history_t;



unsigned int counters[DISEASE_STATUS_COUNT] = {0};
person_t  people[POPULATION_SIZE];
disease_history_t history = {0};
bool paused = false;
bool system_saturated = false;
unsigned int health_system_policy = 0;



void history_append(unsigned int e, disease_history_t *history)
{
  if (history->count >= history->capacity) {
    history->capacity = (history->capacity <= 0) ? 1024 : history->capacity << 1;

    history->infected = realloc(history->infected, history->capacity * sizeof(*history->infected));
    assert(history->infected);
  }

  history->infected[history->count++] = e;
}

location_t random_location(void)
{
  location_t location;

  location.x = rand() % SIMULATION_WIDTH;
  location.y = rand() % SIMULATION_HEIGHT;

  return location;
}

void init_people(void)
{
  for (size_t i = 0; i < POPULATION_SIZE; i++) {
    people[i].current_location = random_location();
    people[i].status = rand() % 100 < 5 ? INFECTED : VULNERABLE;
    people[i].remaining_infection_days = INFECTION_DURATION;
    counters[people[i].status] ++;
  }
}

void draw_people()
{
  for (size_t i = 0; i < POPULATION_SIZE; i++) {
    DrawRectangle(people[i].current_location.x, 
                  people[i].current_location.y, 
                  5,
                  5,
                  color_map[people[i].status]);
  }
}

void plot_graph(disease_history_t *history)
{
  char msg[128];
  int text_size;

  float x1, y1, x2, y2;
  Color color;
  float max_y = history->infected[0];

  for (size_t i = 1; i < history->count; i++)
    if (history->infected[i] > max_y) 
      max_y = history->infected[i]; 

  for (size_t i = 0; i < history->count - 1; i++) {

    x1 = 5 + SIMULATION_WIDTH + i * (GRAPH_WIDTH / (float) history->count);
    y1 = GRAPH_HEIGHT - history->infected[i] * (GRAPH_HEIGHT / max_y);

    x2 = 5 + SIMULATION_WIDTH + (i+1) * (GRAPH_WIDTH / (float) history->count);
    y2 = GRAPH_HEIGHT - history->infected[i+1] * (GRAPH_HEIGHT / max_y);

    color = history->infected[i] > SATURATION_THRESHOLD ? RED : GREEN;
    DrawLineEx((Vector2){x1, y1}, (Vector2){x2, y2}, 2.0, color);

  }

  snprintf(msg, sizeof(msg), "Days elapsed: %lu", history->count / DAY_LENGTH);
  text_size = MeasureText(msg, FONT_SIZE);
  DrawText(msg, WIDTH - text_size - COLUMN_SPACING, LINE_SPACING + FONT_SIZE, FONT_SIZE, WHITE);

  snprintf(msg, sizeof(msg), "Infected: %d / %d", counters[INFECTED], POPULATION_SIZE);
  text_size = MeasureText(msg, FONT_SIZE);
  DrawText(msg, WIDTH - text_size - COLUMN_SPACING, 2 * (LINE_SPACING + FONT_SIZE), FONT_SIZE, RED);

  snprintf(msg, sizeof(msg), "Dead: %d / %d", counters[DEAD], POPULATION_SIZE);
  text_size = MeasureText(msg, FONT_SIZE);
  DrawText(msg, WIDTH - text_size - COLUMN_SPACING, 3 * (LINE_SPACING + FONT_SIZE), FONT_SIZE, BLACK);
}


double distance(location_t a, location_t b)
{
  return sqrt((a.x-b.x)*(a.x-b.x) + (a.y-b.y)*(a.y-b.y));
}

void move_people(void)
{
  memset(counters, 0, sizeof(counters));

  for (size_t i = 0; i < POPULATION_SIZE; i++) {

    counters[people[i].status] ++;

    if (people[i].status == DEAD) continue;

    if ((health_system_policy & ISOLATION) && people[i].status == INFECTED) continue;

    people[i].current_location.x += (int)(rand() % 11) - 5;
    CONSTRAINT(people[i].current_location.x, 0, SIMULATION_WIDTH);

    people[i].current_location.y += (int)(rand() % 11) - 5;
    CONSTRAINT(people[i].current_location.y, 0, SIMULATION_HEIGHT);
  }

  history_append(counters[INFECTED], &history);
}

void kill_or_recover(person_t *target)
{
  if (target->status != INFECTED) return;

  if (-- target->remaining_infection_days <= 0)
    target->status = (rand() % 100 <= (system_saturated ? SATURATED_FATALITY_RATE : NORMAL_FATALITY_RATE)) ? DEAD : IMMUNE;
}

void infect_people(person_t *a, person_t *b)
{
  if (a->status == INFECTED && b->status == VULNERABLE && (rand() % 100 <= INFECTION_PROBABILITY)) {
    b->status = INFECTED;
    b->remaining_infection_days = INFECTION_DURATION;
  }
  else if (b->status == INFECTED && a->status == VULNERABLE && (rand() % 100 <= INFECTION_PROBABILITY)) {
    a->status = INFECTED;
    a->remaining_infection_days = INFECTION_DURATION;
  }
}

void scan_people(void)
{
  for (size_t i = 0; i < POPULATION_SIZE - 1; i++) {
    for (size_t j = i + 1; j < POPULATION_SIZE; j++) {

      double d = distance(people[i].current_location, people[j].current_location);

      if (d <= INFECTION_PROXIMITY) {

        kill_or_recover(&people[i]);
        kill_or_recover(&people[j]);

        infect_people(&people[i], &people[j]);
      }
    }
  }

}

void simulation_step()
{
  if (counters[INFECTED] == 0) {
    const char *msg = "Simulation has finished";
    int text_size = MeasureText(msg, FONT_SIZE);
    DrawText(msg, WIDTH/2 - text_size/2, HEIGHT/2 - FONT_SIZE/2, FONT_SIZE, RED);
    return;
  }

  scan_people();
  move_people();

  system_saturated = (counters[INFECTED] >= SATURATION_THRESHOLD) ? true : false;
}

int main(void)
{
  srand(time(NULL));
  init_people();
  history_append(0, &history);

  InitWindow(WIDTH, HEIGHT, "Covid simulation");
  SetTargetFPS(60);

  while (!WindowShouldClose()) {
    if (IsKeyPressed(KEY_SPACE)) {
      paused = !paused;
    }

    if (IsKeyPressed(KEY_ENTER)) {
      init_people();
      history.count = 0;
      history_append(0, &history);
    }

    BeginDrawing();
      ClearBackground(BLUE);
      if (!paused) simulation_step();
      draw_people();
      plot_graph(&history);
    EndDrawing();
  }

  CloseWindow();

  return 0;
}
