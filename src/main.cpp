#include "codegen.hpp"
#include "ir.hpp"
#include "ir_optimizer.hpp"

#include "parser.hpp"
#include "sema.hpp"
#include "tokenizer.hpp"
#include "utils.hpp"

#include <fstream>
#include <sstream>
#include <string>

int main(int argc, char* argv[])
{
  std::vector<std::string> args(argv, argv + argc);
  std::string output_file_path;
  bool disable_debug = false;
  if (argc < 2)
    Utils::panic("Usage: ./P <source.p> [OPTIONS]");

  for (int i = 2; i < argc; i++)
  {
    if (args[i] == "-o")
    {
      output_file_path = args[i + 1];
      i++;
    }
    if (args[i] == "-dd")
    {
      disable_debug = true;
    }
  }

  std::stringstream src;
  {
    std::ifstream file(argv[1]);
    if (!file || !file.is_open())
    {
      Utils::panic("Could not open file: ", argv[1]);
      return 1;
    }

    src << file.rdbuf();
    Logger::info("Source: \n", src.str(), '\n');
  }

  Logger::debug("Tokenization started\n");
  Tokenizer tokenizer(src.str());
  std::vector<Token> tokens = tokenizer.tokenize();
  Logger::debug("Tokenization finished\n");
  Logger::debug("TOKENS:", '\n');
  for (auto& token : tokens) // Debug: Tokens
    Logger::debug(token, '\n');

  Logger::debug("Parsing started\n");
  Parser parser(tokens);
  std::vector<Node::Node> nodes = parser.parse_prog();

  Logger::debug("Parsing finished\n");

  std::cout << "--- AST Dump ---" << std::endl;
  std::cout << "Node count: " << nodes.size() << std::endl;
  for (const auto& node : nodes)
    std::visit([](auto&& arg) { if (arg) arg->dump(0); }, node);
  std::cout << std::flush;

  Logger::debug("Analyzing started\n");
  Sema analyzer(nodes);
  Sema::Analysis anl = analyzer.analyze();
  Logger::debug("Analyzing finished\n");

  Logger::debug("IR Gen started\n");
  IRGen ir_gen(nodes, anl);
  auto ir_instructs = ir_gen.gen();
  Logger::debug("IR Gen finished\n");

  Logger::debug("IR:", '\n');
  for (auto& instruct : ir_instructs) // Debug: Instructions
    std::cout << instruct << '\n';

  std::cout.flush();
  // Logger::log(Logger::Level::DEBUG, instruct, '\n');

  /*
  IROptimizer optimizer(ir_instructs);
  optimizer.optimize();


  Logger::log(Logger::Level::DEBUG, "OPTIMIZED IR:", '\n');
  for (auto& instruct : ir_instructs) // Debug: Instructions
    std::cout << instruct << '\n';
  */

  Logger::debug("Code Gen started\n");
  CodeGen code_gen(ir_instructs);
  auto cg_iss = std::istringstream(code_gen.gen_code());
  Logger::debug("Code Gen finished\n");

  std::string asm_str;
  if (disable_debug)
  {
    std::string line;
    while (std::getline(cg_iss, line))
    {
      size_t first = line.find_first_not_of(" \t");
      if (first != std::string::npos && line[first] == '#')
        continue;
      asm_str += line + '\n';
    }
  }
  else
    asm_str = cg_iss.str();
  auto iss = std::istringstream(asm_str);

  Logger::debug("ASM:", '\n');
  std::string line;
  while (std::getline(iss, line))
    Logger::debug(line, '\n');


  std::ofstream outfile(output_file_path);
  if (outfile.is_open())
    outfile << iss.str();

  outfile.close();

  return EXIT_SUCCESS;
}
