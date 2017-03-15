#ifndef COUNTER_H
#define COUNTER_H

typedef struct counter_ *counter;

struct counter_ {
    int num;
};

counter Counter(int);

#endif
