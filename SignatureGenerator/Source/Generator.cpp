#include <atomic>

#include <boost/crc.hpp>
#include <boost/filesystem.hpp>
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>

#include "Generator.h"
#include "LoadDistributor.h"

namespace NFileSignature
{
   namespace fs = boost::filesystem;
   namespace ip = boost::interprocess;

   namespace
   {
      void CheckPaths(const fs::path& inFilePath, const fs::path& outFilePath)
      {
         if (!fs::exists(inFilePath))
            throw std::runtime_error("source file does not exist");

         if (!fs::is_regular_file(inFilePath))
            throw std::runtime_error(inFilePath.string() + "is not a file");

         if (!fs::exists(outFilePath))
         {
            std::ofstream created(outFilePath.string().c_str());
            if (!created)
               throw std::runtime_error("cannot create output file");
         }
         else if (!fs::is_regular_file(outFilePath))
            throw std::runtime_error(outFilePath.string() + "is not a file");
      }
   }

   void ProcessBlock(
      const ip::file_mapping& srcFile, uintmax_t srcViewOffset, uintmax_t srcViewSize,
      const ip::mapped_region& view, uintmax_t viewBlockNumber)
   {
      ip::mapped_region srcView(srcFile, ip::read_only, srcViewOffset, srcViewSize);
      boost::crc_32_type  result;
      result.process_bytes(static_cast<const char*>(srcView.get_address()), srcView.get_size());

      using crc_value_type = boost::crc_32_type::value_type;
      assert(view.get_size() >= viewBlockNumber * sizeof(crc_value_type));
      *(static_cast<crc_value_type*>(view.get_address()) + viewBlockNumber) = result.checksum();
   }

   void CreateSignature(const fs::path& inFilePath, const fs::path& outFilePath, uintmax_t blockSize)
   {
      CheckPaths(inFilePath, outFilePath);

      auto blocksCount = static_cast<decltype(fs::file_size(inFilePath))>(
         std::ceil(static_cast<float>(fs::file_size(inFilePath)) / blockSize));

      constexpr auto crcValueSize = sizeof(boost::crc_32_type::value_type);
      fs::resize_file(outFilePath, blocksCount * crcValueSize);
      
      ip::file_mapping inFile(inFilePath.string().c_str(), ip::read_only);
      ip::file_mapping outFile(outFilePath.string().c_str(), ip::read_write);

      std::atomic_flag lock = ATOMIC_FLAG_INIT;
      decltype(blocksCount) processedCount(0u);
      decltype(blocksCount) viewOffset(0u);

      const auto& distributor = LoadDistributor::Create();

      while (processedCount < blocksCount)
      {
         auto viewSize = std::min(
            static_cast<decltype(processedCount)>(ip::mapped_region::get_page_size()),
            (blocksCount - processedCount) * crcValueSize);

         ip::mapped_region outView(outFile, ip::read_write, viewOffset, viewSize);

         assert(0 == viewSize % crcValueSize);


         auto portionSize = viewSize / crcValueSize;
         decltype(portionSize) blocksNumInPortion = 0u;

         distributor.Execute(
            [&inFile, &outView, processedCount, blockSize](uint32_t numberInPortion)
         {
            ProcessBlock(
               inFile,
               (processedCount + numberInPortion) * blockSize,
               blockSize,
               outView,
               numberInPortion);
         },
            [portionSize, &blocksNumInPortion, &lock]() -> boost::optional<uintmax_t>
         {
            while (lock.test_and_set(std::memory_order_acquire) == true) {}

            auto result = boost::make_optional(blocksNumInPortion);
            if (blocksNumInPortion < portionSize)
               blocksNumInPortion++;
            else
               result = boost::none;
           
            lock.clear(std::memory_order_release);
            return result;
         });

         outView.flush();
         viewOffset += viewSize;
         processedCount += portionSize;
      }
   }
} // NFileSignature