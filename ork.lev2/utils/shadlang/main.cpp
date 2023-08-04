////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2023, Michael T. Mayers.
// Distributed under the MIT License.
// see license-mit.txt in the root of the repo, and/or https://opensource.org/license/mit/
////////////////////////////////////////////////////////////////

#include <ork/file/path.h>
#include <ork/kernel/spawner.h>
#include <ork/application/application.h>
#include <ork/lev2/gfx/shadlang.h>
#include <boost/program_options.hpp>

using namespace ork;
using namespace ork::lev2;
namespace po = ::boost::program_options;

int main(int argc, char** argv, char** envp) {

  auto init_data  = std::make_shared<AppInitData>(argc, argv, envp);
  auto desc       = init_data->commandLineOptions("Orkid Shader Language Frontend");
  auto rval       = desc->add_options() //
      ("help", "produce help message") //
      ("in", po::value<std::string>()->default_value(""), "input shader file path") //
      ("astout", po::value<std::string>()->default_value(""), "output shader AST file path")
      ("dotout", po::value<std::string>()->default_value(""), "output shader DOT file path");
  auto opts = init_data->parse();

  auto incdir     = file::Path::orkroot_dir() / "ork.data" / "platform_lev2" / "shaders" / "glfx";
  auto fdevctx    = FileEnv::createContextForUriBase("shaders://", incdir);
  auto input_path = init_data->commandLineOption("in").as<std::string>();
  auto ast_output_path = init_data->commandLineOption("astout").as<std::string>();
  auto dot_output_path = init_data->commandLineOption("dotout").as<std::string>();
  printf( "input_path<%s>\n", input_path.c_str());
  printf( "ast_output_path<%s>\n", ast_output_path.c_str());
  printf( "dot_output_path<%s>\n", dot_output_path.c_str());
  auto shader_data = File::readAsString(input_path);
  auto tunit      = shadlang::parse(shader_data->_data);
  if(tunit){
    if( ast_output_path.length() ){
        auto ast = shadlang::toASTstring(tunit);
        printf( "AST<%s>\n", ast.c_str());
        bool OK = File::writeString(ast_output_path, ast);
        OrkAssert(OK);
    }
    if( dot_output_path.length() ){
        auto dot = shadlang::toDotFile(tunit);
        bool OK = File::writeString(dot_output_path, dot);
        OrkAssert(OK);
        auto spawner = std::make_shared<Spawner>();
        spawner->mCommandLine = std::string("dot -Tpng -o ") + dot_output_path + ".png " + dot_output_path;
        spawner->spawnSynchronous();

    }
  }
  return (tunit!=nullptr) ? 0 : -1;
}