// main.cpp : Defines the entry point for the console application.
//
#include <iostream>
#include <future>
#include <assert.h>

#include <boost/optional.hpp>
#include <boost/filesystem.hpp>

#include <Generator.h>

namespace
{
	boost::optional<boost::filesystem::path> ReadPath()
	{
	   boost::filesystem::path path;
      try
      {
         std::cin >> path;
      }
      catch(...)
      {
         std::cout << "\nThis is not a valid file path\n";
         return boost::none;
      }
		
      return boost::make_optional(path);
	}
}

int main()
{
   while ('q' != std::getchar())
   {
      try
      {
         std::cout << "Enter source file path\n\t";

         auto optSrcPath = ReadPath();
         if (!optSrcPath)
            continue;

         boost::filesystem::resize_file(optSrcPath.value(), 2ull << 30);

         std::cout << "Enter result file path\n\t";

         auto optResultPath = ReadPath();
         if (!optResultPath)
            continue;

         NFileSignature::CreateSignature(optSrcPath.value(), optResultPath.value());
      }
      catch (const std::future_error& error)
      {
         std::cout << "Future throws" << error.what() << '\n';
      }
      catch (const std::exception& error)
      {
         std::cout << "Generator throws" << error.what() << '\n';
      }
      catch (...)
      {
         std::cout << "Undefined error occurred\n";
      }
   }

   return 0;
}

