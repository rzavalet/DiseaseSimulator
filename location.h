/*
 * location.h
 * Copyright (C) 2023 rzavalet <rzavalet@noemail.com>
 *
 * Distributed under terms of the MIT license.
 */

#ifndef LOCATION_H
#define LOCATION_H

typedef struct
{
  int x;
  int y;

} location_t;

double distance(location_t a, location_t b);

#endif /* !LOCATION_H */
