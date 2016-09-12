// main.cpp : Defines the entry point for the console application.
//
#include <iostream>
#include <future>

#include <boost/filesystem.hpp>

#include <Generator.h>

namespace
{
   void PrintUsageMsg(const char* progName)
   {
      assert(progName);
      std::cerr <<
         "Usage: " << progName << " source_file_path result_file_path [block_size in MByte]" << std::endl;
   }
}

int main(int argc, char* argv[])
{
   if (argc < 3)
   {
      PrintUsageMsg(argv[0]);
      return 1;
   }

   boost::filesystem::path srcPath, resultPath;
   try
   {
      srcPath = argv[1];
      resultPath = argv[2];
   }
   catch(...)
   {
      PrintUsageMsg(argv[0]);
      return 1;
   }

   try
   {
      if (argc >= 4)
         NFileSignature::CreateSignature(srcPath, resultPath, static_cast<uintmax_t>(atol(argv[3])) << 20);
      else
         NFileSignature::CreateSignature(srcPath, resultPath);
   }
   catch (const std::future_error& error)
   {
      std::cout << "Future throws: " << error.what() << '\n';
   }
   catch (const std::exception& error)
   {
      std::cout << "Generator throws: " << error.what() << '\n';
   }
   catch (...)
   {
      std::cout << "Undefined error occurred\n";
   }

   return 0;
}

