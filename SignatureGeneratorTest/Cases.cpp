#define BOOST_TEST_MODULE FileSignatureTestModule
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

BOOST_AUTO_TEST_SUITE_END()