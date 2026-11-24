// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0


#pragma once
#include <filesystem>
//#include <boost/asio.hpp>
#include "NLID.h"
#include "NajaProgress.h"

namespace naja::NL {

class NLDB;

class SNLCapnP {
  public:
    struct LoadingConfiguration {
      enum class PrimitiveConflictPolicy {
        PreferExisting,
        ForbidConflicts
      };
      /// Synchronous callback on the loading thread; false throws NajaCancelled.
      NajaProgressCallback progress;
      PrimitiveConflictPolicy primitiveConflictPolicy_;
      LoadingConfiguration(): primitiveConflictPolicy_(PrimitiveConflictPolicy::PreferExisting) {}
     private:
      friend class SNLCapnP;
      bool nested_ = false;
    };

    //static boost::asio::ip::tcp::socket getSocket(uint16_t port=0); 
    static constexpr std::string_view InterfaceName = "db_interface.snl";
    // Canonical DB0 interfaces, encoded with the existing DBInterface schema.
    static constexpr std::string_view DB0InterfaceName = "db0_interface.snl";
    static constexpr std::string_view ImplementationName = "db_implementation.snl";
    static void dump(const NLDB* db, const std::filesystem::path& dumpPath);
    //static void send(const NLDB* db, const std::string& ipAddress, uint16_t port);
    //static void send(const NLDB* db, const std::string& ipAddress, uint16_t port, NLID::DBID forceDBID);
    static NLDB* load(const std::filesystem::path& dumpPath, LoadingConfiguration loadingConfiguration=LoadingConfiguration());  
    //static NLDB* receive(boost::asio::ip::tcp::socket& socket);
    //static NLDB* receive(uint16_t port);

    static void dumpInterface(const NLDB* db, int fileDescriptor);
    static void dumpInterface(const NLDB* db, int fileDescriptor, NLID::DBID forceDBID);
    static void dumpInterface(const NLDB* db, const std::filesystem::path& interfacePath);
    //static void sendInterface(const NLDB* db, const std::string& ipAddress, uint16_t port);
    //static void sendInterface(const NLDB* db, boost::asio::ip::tcp::socket& socket); 
    //static void sendInterface(const NLDB* db, boost::asio::ip::tcp::socket& socket, NLID::DBID forceDBID); 

    static NLDB* loadInterface(int fileDescriptor, LoadingConfiguration loadingConfiguration);
    static NLDB* loadInterface(const std::filesystem::path& interfacePath, LoadingConfiguration loadingConfiguration);
    //static NLDB* receiveInterface(uint16_t port);
    //static NLDB* receiveInterface(boost::asio::ip::tcp::socket& socket); 

    static void dumpImplementation(const NLDB* db, int fileDescriptor);
    static void dumpImplementation(const NLDB* db, int fileDescriptor, NLID::DBID forceDBID);
    static void dumpImplementation(const NLDB* db, const std::filesystem::path& implementationPath);
    //static void sendImplementation(const NLDB* db, const std::string& ipAddress, uint16_t port);
    //static void sendImplementation(const NLDB* db, boost::asio::ip::tcp::socket& socket);
    //static void sendImplementation(const NLDB* db, boost::asio::ip::tcp::socket& socket, NLID::DBID forceDBID);

    static NLDB* loadImplementation(int fileDescriptor, LoadingConfiguration loadingConfiguration);
    static NLDB* loadImplementation(const std::filesystem::path& implementationPath, LoadingConfiguration loadingConfiguration);
    //static NLDB* receiveImplementation(uint16_t port);
    //static NLDB* receiveImplementation(boost::asio::ip::tcp::socket& socket); 
};

}  // namespace naja::NL
