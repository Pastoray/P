#include <benchmark/benchmark.h>
#include "../src/tokenizer.hpp"
#include "../src/parser.hpp"

static void BM_Parser_ParsePrintProg(benchmark::State& state)
{
  const char* input = R"(
    fn printi32(x) -> ;
    fn main() ->
    {
      print(67);
    }
  )";

  Tokenizer tokenizer(input);
  auto tokens = tokenizer.tokenize();
  for (auto _ : state)
  {
    Parser parser(tokens);
    auto prog = parser.parse_prog();
    benchmark::DoNotOptimize(prog);
  }
}

BENCHMARK(BM_Parser_ParsePrintProg);
