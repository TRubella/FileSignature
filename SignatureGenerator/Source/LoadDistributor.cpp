#include <vector>
#include <future>

#include "LoadDistributor.h"

namespace NFileSignature
{
   const LoadDistributor& LoadDistributor::Create(size_t threadsCount)
   {
      static LoadDistributor distributor(threadsCount);
      return distributor;
   }

   LoadDistributor::LoadDistributor(size_t threadsCount)
      : m_threadsCount(threadsCount)
   {}

   void LoadDistributor::Execute
   (
      const std::function<void(uintmax_t)>& task,
      const std::function<boost::optional<uintmax_t>()>& predicate
   ) const
   {
      auto Execute = [&task, &predicate]()
      {
         boost::optional<uintmax_t> predResult;
         while ((predResult = predicate()))
            task(predResult.value());
      };

      std::vector<std::future<void>> futures;
      futures.push_back(std::async(std::launch::deferred, Execute));
      
      auto threadsSupported = m_threadsCount;

      for (auto i = futures.size(); i < threadsSupported; ++i)
         futures.push_back(std::async(std::launch::async, Execute));

      for (auto& item : futures)
         item.wait();
   }
} // NFileSignature