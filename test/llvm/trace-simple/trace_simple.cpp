#include <cstdlib>

constexpr int kNLoops = 128;

__attribute__((noinline)) int loop(int n_loops) {
  int sum = 0;
#pragma nounroll
  for (int i = 0; i < n_loops; i++) {
    if (i % 2) {
      sum += i;
    } else {
      sum *= i;
    }
  }
  return sum;
}

int main() {
  [[maybe_unused]] volatile int val = loop(kNLoops);
  return EXIT_SUCCESS;
}
