#include <benchmark/benchmark.h>
#include "../src/tokenizer.hpp"

static void BM_Tokenizer_TokenizePrintProg(benchmark::State& state)
{
  const char* input = R"(
    fn printi32(x) -> ;
    fn main() ->
    {
      print(67);
    }
  )";

  for (auto _ : state)
  {
    Tokenizer tokenizer(input);
    auto tokens = tokenizer.tokenize();
    benchmark::DoNotOptimize(tokens);
  }
}

BENCHMARK(BM_Tokenizer_TokenizePrintProg);
