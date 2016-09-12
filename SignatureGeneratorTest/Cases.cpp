#define BOOST_TEST_MODULE FileSignatureTestModule

#include <chrono>
#include <boost/test/unit_test.hpp>
#include <boost/filesystem/operations.hpp>

#include <Generator.h>
#include <boost/crc.hpp>

using namespace boost::filesystem;
using namespace boost::unit_test;
using namespace NFileSignature;

BOOST_AUTO_TEST_SUITE(FileSignatureTestSuite)

BOOST_AUTO_TEST_CASE(BadSourcePaths)
{
   path srcPath(current_path() /= "testfile"), stub;
   if(exists(srcPath))
      remove(srcPath);
   
   BOOST_CHECK_THROW(CreateSignature(srcPath, stub), std::runtime_error); //doesn't exists
   BOOST_CHECK_THROW(CreateSignature(srcPath.parent_path(), stub), std::runtime_error); //is not a file
}

template <uintmax_t fileSize = 1ull << 30>
struct Fixture
{
   static uintmax_t Size() { return fileSize; }
   
   static const path& Path()
   {
      static auto filePath(current_path() /= "testfile");
      return filePath;
   }

   Fixture()
   {
      {
         std::ofstream file(Path().string());
      }

      resize_file(Path(), fileSize);
   }

   ~Fixture()
   {
      if (exists(Path())) remove(Path());
   }
};

BOOST_FIXTURE_TEST_CASE(BadResultPaths, Fixture<4>)
{
   auto resPath = Path().parent_path();
   BOOST_CHECK_THROW(CreateSignature(Path(), resPath, Size()), std::runtime_error); //is not a file

   resPath /= "abracadabra";
   if (exists(resPath)) remove(resPath);
   CreateSignature(Path(), resPath);

   //create with correct size
   BOOST_REQUIRE(exists(resPath));
   BOOST_CHECK(file_size(resPath) == sizeof(boost::crc_32_type::value_type));
}

BOOST_FIXTURE_TEST_CASE(CRCForSingleBlock, Fixture<512>)
{
   std::ofstream ostream(Path().string(), std::ios_base::binary);
   auto buffer = std::unique_ptr<char[]>(new char[Size()]);
   for (auto i = 0u; i < Size(); ++i)
      buffer.get()[i] = i;

   ostream.write(buffer.get(), Size());
   ostream.close();

   boost::crc_32_type crc;
   crc.process_bytes(buffer.get(), Size());

   auto resPath = Path().parent_path() /= "result";
   CreateSignature(Path(), resPath, Size());

   std::ifstream istream(resPath.string());
   uint32_t tested = 0;
   istream.read(reinterpret_cast<char*>(&tested), sizeof(tested));
   istream.close();

   remove(resPath);

   BOOST_CHECK_EQUAL(crc.checksum(), tested);
}

BOOST_FIXTURE_TEST_CASE(SingleBlockSingleThreadedBenchmark, Fixture<1ull << 20>)
{
   auto resPath = Path().parent_path() /= "stream_result";

   auto start = std::chrono::steady_clock::now();

   auto buffer = std::unique_ptr<char[]>(new char[Size()]);

   std::ifstream istream(Path().string());
   istream.read(buffer.get(), Size());
   istream.close();

   boost::crc_32_type crc;
   crc.process_bytes(buffer.get(), Size());

   std::ofstream ostream(resPath.string(), std::ios_base::binary);
   auto value = crc.checksum();
   ostream.write(reinterpret_cast<const char*>(&value), sizeof(boost::crc_32_type::value_type));
   ostream.close();

   auto streamDuration = std::chrono::steady_clock::now() - start;

   remove(resPath);
   resPath = Path().parent_path() /= "mmf_result";

   start = std::chrono::steady_clock::now();
   CreateSignature(Path(), resPath, Size());
   auto mmfDuration = std::chrono::steady_clock::now() - start;

   remove(resPath);
   BOOST_CHECK_LT( //fails on small sizes (~less than 200KB)
      std::chrono::duration_cast<std::chrono::milliseconds>(mmfDuration).count(),
      std::chrono::duration_cast<std::chrono::milliseconds>(streamDuration).count());
}

BOOST_FIXTURE_TEST_CASE(Profiling, Fixture<10ull<<30>) //10GB source file
{
   auto resPath = Path().parent_path() /= "result";

   auto start = std::chrono::steady_clock::now();
   CreateSignature(Path(), resPath);
   auto duration = std::chrono::steady_clock::now() - start;

   std::cout << "\n\"CreateSignature\" for file of" << Size() << "bytes takes:\n\t"
      << std::chrono::duration_cast<std::chrono::minutes>(duration).count() << ":"
      << std::chrono::duration_cast<std::chrono::seconds>(duration).count() % 60 << ":"
      << std::chrono::duration_cast<std::chrono::milliseconds>(duration).count() % 1000;

   remove(resPath);
}

BOOST_AUTO_TEST_SUITE_END()