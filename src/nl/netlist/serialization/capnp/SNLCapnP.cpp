// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLCapnP.h"
#include "SNLDumpManifest.h"
#include "NLDB.h"
#include "NLDB0.h"
#include "SNLDump.h"
#include "SNLDumpException.h"
#include "NajaVersion.h"

#include <sstream>

//using boost::asio::ip::tcp;

namespace naja::NL {

namespace {

void checkManifestCompatibility(const SNLDumpManifest& manifest) {
  const auto manifestVersion = manifest.getSchemaVersion();
  const auto expectedVersion = SNLDump::getVersion();
  if (manifestVersion == expectedVersion) {
    return;
  }
  std::ostringstream reason;
  reason << "Incompatible SNL snapshot schema version: manifest has "
    << manifestVersion.getMajor() << "." << manifestVersion.getMinor()
    << "." << manifestVersion.getRevision()
    << ", reader expects "
    << expectedVersion.getMajor() << "." << expectedVersion.getMinor()
    << "." << expectedVersion.getRevision();
  if (not manifest.getProducerVersion().empty()) {
    reason << " (producer naja version " << manifest.getProducerVersion();
    if (not manifest.getProducerGitHash().empty()) {
      reason << ", git hash " << manifest.getProducerGitHash();
    }
    reason << ")";
  }
  throw SNLDumpException(reason.str());
}

void checkManifestProducer(const SNLDumpManifest& manifest) {
  if (manifest.getProducerVersion() == naja::NAJA_VERSION
      and manifest.getProducerGitHash() == naja::NAJA_GIT_HASH) {
    return;
  }
  std::ostringstream reason;
  reason << "Incompatible SNL snapshot producer: snapshot was built by naja "
    << "version "
    << (manifest.getProducerVersion().empty()
          ? "<missing>" : manifest.getProducerVersion())
    << ", git hash "
    << (manifest.getProducerGitHash().empty()
          ? "<missing>" : manifest.getProducerGitHash())
    << "; reader is naja version " << naja::NAJA_VERSION
    << ", git hash " << naja::NAJA_GIT_HASH;
  throw SNLDumpException(reason.str());
}

}

void SNLCapnP::dump(const NLDB* db, const std::filesystem::path& path) {
  std::filesystem::create_directory(path);
  SNLDumpManifest::dump(path);
  // Preserve all canonical models, including unused predecessors: compound
  // families allocate IDs in creation order. This also preserves the initial
  // DB0 state for subsequent creation after a fresh-universe reload.
  dumpInterface(NLDB0::getDB0(), path/DB0InterfaceName);
  dumpInterface(db, path/InterfaceName);
  dumpImplementation(db, path/ImplementationName);
}

//Need to find a proper way to test serialization on the wire
//LCOV_EXCL_START
//void SNLCapnP::send(const NLDB* db, const std::string& ipAddress, uint16_t port) {
//  send(db, ipAddress, port, db->getID());
//}
//
//void SNLCapnP::send(const NLDB* db, const std::string& ipAddress, uint16_t port, NLID::DBID forceDBID) {
//  boost::asio::io_context ioContext;
//  //socket creation
//  tcp::socket socket(ioContext);
//  socket.connect(tcp::endpoint(boost::asio::ip::make_address(ipAddress), port));
//  sendInterface(db, socket, forceDBID);
//  sendImplementation(db, socket, forceDBID);
//}
//LCOV_EXCL_STOP

NLDB* SNLCapnP::load(
  const std::filesystem::path& path,
  LoadingConfiguration loadingConfiguration) {
  NajaProgressReporter progress;
  progress.setCallback(loadingConfiguration.progress);
  progress.begin("parse");
  loadingConfiguration.nested_ = true;
  const auto manifest = SNLDumpManifest::load(path);
  checkManifestCompatibility(manifest);
  checkManifestProducer(manifest);
  if (manifest.getDB0LibraryIDVersion() != NLDB0::LibraryIDVersion) {
    std::ostringstream reason;
    reason << "Incompatible SNL snapshot DB0 library ID version: manifest has "
      << manifest.getDB0LibraryIDVersion() << ", reader expects "
      << NLDB0::LibraryIDVersion << ". Regenerate the snapshot.";
    throw SNLDumpException(reason.str());
  }
  if (not std::filesystem::is_regular_file(path/DB0InterfaceName)) {
    throw SNLDumpException("Missing snapshot DB0 model definitions (db0_interface.snl). Regenerate the snapshot.");
  }
  if (not NLDB0::isDB0(loadInterface(path/DB0InterfaceName, loadingConfiguration))) {
    throw SNLDumpException("Invalid snapshot DB0 model definitions: expected database ID 0");
  }
  loadInterface(path/InterfaceName, loadingConfiguration);
  NLDB* db = loadImplementation(path/ImplementationName, loadingConfiguration);
  progress.begin("done");
  return db;
}

//Need to find a proper way to test serialization on the wire
//LCOV_EXCL_START
//boost::asio::ip::tcp::socket SNLCapnP::getSocket(uint16_t port) {
//  boost::asio::io_context ioContext;
//  //listen for new connection
//  tcp::acceptor acceptor(ioContext, tcp::endpoint(tcp::v4(), port));
//  //socket creation 
//  tcp::socket socket(ioContext);
//  //waiting for connection
//  acceptor.accept(socket);
//  return std::move(socket);
//}
//
//NLDB* SNLCapnP::receive(boost::asio::ip::tcp::socket& socket) {
//  receiveInterface(socket);
//  NLDB* db = receiveImplementation(socket);
//  return db;
//}
//
//NLDB* SNLCapnP::receive(uint16_t port) {
//  auto socket = getSocket(port);
//  return receive(socket);
//}
//LCOV_EXCL_STOP

}  // namespace naja::NL
