#ifndef __fs__LoadDistributor_h__
#define __fs__LoadDistributor_h__

#include <cstdint>
#include <functional>
#include <thread>
#include <boost/optional/optional.hpp>

namespace NFileSignature
{
   class LoadDistributor final
   {
   public:
      static const LoadDistributor& Create(size_t threadsCount = std::thread::hardware_concurrency());

      void Execute(
         const std::function<void(uintmax_t)>& task,
         const std::function<boost::optional<uintmax_t>()>& predicate
      ) const;

      LoadDistributor(const LoadDistributor&) = delete;
      void operator=(const LoadDistributor&) = delete;

      LoadDistributor(LoadDistributor&&) = delete;
      LoadDistributor& operator=(LoadDistributor&&) = delete;

   private:
      explicit LoadDistributor(size_t threadsCount);

      size_t m_threadsCount;
   };

} // NFileSignature

#endif // __fs__LoadDistributor_h__
