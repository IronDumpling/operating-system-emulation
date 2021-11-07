#include <assert.h>
#include <stdlib.h>
#include <math.h>
#include "common.h"
#include "point.h"

void
point_translate(struct point *p, double x, double y)
{
    point_set(p, x, y);
}

double
point_distance(const struct point *p1, const struct point *p2)
{
    double distance = sqrt(pow(point_X(p1) - point_X(p2), 2) + pow(point_Y(p1) - point_Y(p2), 2));
	return distance;
}

int
point_compare(const struct point *p1, const struct point *p2)
{
    double euclidean1 = sqrt(pow(point_X(p1), 2) + pow(point_Y(p1), 2));
    double euclidean2 = sqrt(pow(point_X(p2), 2) + pow(point_Y(p2), 2));
    if(euclidean1 < euclidean2) return -1;
    else if(euclidean1 == euclidean2) return 0;
    else return 1;
}
