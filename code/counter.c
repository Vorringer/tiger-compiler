#include "counter.h"

counter Counter(int num) {
    counter c = (counter) checked_malloc(sizeof(*c));
    c->num = num;
    return c;
}