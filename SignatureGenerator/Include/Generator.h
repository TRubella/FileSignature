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
      uint32_t blockSize = 2ull << 10);

} // NFileSignature

#endif // __fs__Generator_h__
