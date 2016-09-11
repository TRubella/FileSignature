#include <atomic>
#include <future>
#include <thread>
#include <functional>
#include <vector>

#include <boost/crc.hpp>
#include <boost/filesystem.hpp>
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/optional/optional.hpp>
//#include <boost/thread/shared_mutex.hpp>
//#include <boost/thread/locks.hpp>

#include "Generator.h"

namespace NFileSignature
{
   void DistributeLoad(
      const std::function<void(uintmax_t)>& task,
      const std::function<boost::optional<uintmax_t>()>& predicate
   )
   {
      auto Execute = [&task, &predicate]()
      {
         boost::optional<uintmax_t> predValue;
         while ((predValue = predicate()))
            task(predValue.value());
      };

      std::vector<std::future<void>> futures;
      futures.push_back(std::async(std::launch::deferred, Execute));
      
      auto threadsSupported = std::thread::hardware_concurrency();

      for (auto i = futures.size(); i < threadsSupported; ++i)
         futures.push_back(std::async(std::launch::async, Execute));

      for (auto& item : futures)
         item.wait();
   }

   namespace fs = boost::filesystem;
   namespace ip = boost::interprocess;

   void ProcessBlock(
      const ip::file_mapping& srcFile, uintmax_t srcBlockNumber,
      const ip::mapped_region& view, uint32_t viewBlockNumber)//, const boost::shared_mutex& mutex)
   {
      //boost::upgrade_lock<boost::shared_mutex> lock(*mutex);
      boost::crc_32_type  result;
      // read block from view and compute crc

      //boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(lock);
      using crc_value_type = boost::crc_32_type::value_type;
      assert(view.get_size() >= viewBlockNumber * sizeof(crc_value_type));
      *(static_cast<crc_value_type*>(view.get_address()) + viewBlockNumber) = srcBlockNumber;
   }

   void CheckPaths(const fs::path& inFilePath, const fs::path& outFilePath);

   void CreateSignature(const fs::path& inFilePath, const fs::path& outFilePath, uint32_t blockSize)
   {
      CheckPaths(inFilePath, outFilePath);

      auto blocksCount = static_cast<decltype(fs::file_size(inFilePath))>(
         std::ceil(static_cast<float>(fs::file_size(inFilePath)) / blockSize));

      constexpr auto crcValueSize = sizeof(boost::crc_32_type::value_type);
      fs::resize_file(outFilePath, blocksCount * crcValueSize);
      
      ip::file_mapping inFile(inFilePath.string().c_str(), ip::read_only);
      ip::file_mapping outFile(outFilePath.string().c_str(), ip::read_write);

      //boost::shared_mutex mutex;
      std::atomic_flag lock = ATOMIC_FLAG_INIT;
      decltype(blocksCount) processedCount(0u);
      decltype(blocksCount) viewOffset(0u);

      while (processedCount < blocksCount)
      {
         auto viewSize = std::min(
            static_cast<decltype(processedCount)>(ip::mapped_region::get_page_size()),
            (blocksCount - processedCount) * crcValueSize);

         ip::mapped_region outView(outFile, ip::read_write, viewOffset, viewSize);

         assert(0 == viewSize % crcValueSize);


         auto portionSize = viewSize / crcValueSize;
         decltype(portionSize) blocksNumInPortion = 0u;

         DistributeLoad(
            [&inFile, &outView, processedCount/*, &mutex*/](uint32_t numberInPortion)
         {
            ProcessBlock(inFile, processedCount + numberInPortion, outView, numberInPortion);// , mutex);
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
} // NFileSignature