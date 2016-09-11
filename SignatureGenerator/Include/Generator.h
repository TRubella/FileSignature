#ifndef __fs__Generator_h__
#define __fs__Generator_h__

#include <cstdint>

namespace boost
{
   namespace filesystem
   {
      class path;
   }
}

namespace NFileSignature
{
   void CreateSignature(
      const boost::filesystem::path& inFilePath,
      const boost::filesystem::path& outFilePath,
      uintmax_t blockSize = 1ull << 10);

} // NFileSignature

#endif // __fs__Generator_h__
