#include <benchmark/benchmark.h>
#include "../src/tokenizer.hpp"
#include "../src/parser.hpp"
#include "../src/sema.hpp"

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

  Sema sema({});
  for (auto _ : state)
  {
    Parser parser(tokens, sema);
    auto prog = parser.parse_prog();
    benchmark::DoNotOptimize(prog);
  }
}

BENCHMARK(BM_Parser_ParsePrintProg);
