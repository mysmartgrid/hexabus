#ifndef LIBHBC_COMMON_HPP
#define LIBHBC_COMMON_HPP

#include <cstddef>
#ifdef __GLIBCXX__
#  include <tr1/memory>
#else
#  ifdef __IBMCPP__
#    define __IBMCPP_TR1__
#  endif
#  include <memory>
#endif

#include <config.h>

#include <stdint.h>
#include <string>

namespace hexabus {
  class VersionInfo {
    public:
      // typedef std::tr1::shared_ptr<VersionInfo> Ptr;
      typedef VersionInfo* Ptr;
      VersionInfo () {};
      virtual ~VersionInfo() {};
      const std::string getVersion();

    private:
      VersionInfo (const VersionInfo& original);
      VersionInfo& operator= (const VersionInfo& rhs);
  };

  class GlobalOptions {
    public:
      static GlobalOptions* getInstance();

      bool getVerbose();
      void setVerbose(bool v);

    private:
      GlobalOptions();
      ~GlobalOptions() { }
      static void deleteInstance();
      static GlobalOptions* instance;

      bool verbose;
  };
};


#endif // LIBHBC_COMMON_HPP
