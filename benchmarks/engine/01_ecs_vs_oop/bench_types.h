#ifndef BENCH_TYPES_H
#define BENCH_TYPES_H

// Simple POD used for serialization benchmarks to a contiguous buffer.
struct SerializedEntity {
  float x;
  float y;
  float vx;
  float vy;
  int hp;
};

#endif
