// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0

#include "VHDLIntent.h"
#include "NajaPrivateProperty.h"
#include "NLDB.h"
#include "NLObject.h"
#include "NLUniverse.h"
#include "NLLibrary.h"
#include "SNLDesign.h"
#include "SNLBusNetBit.h"
#include "SNLBusNet.h"
#include "SNLBusTerm.h"
#include "SNLBusTermBit.h"

namespace naja::NL {
namespace {
class SessionProperty final : public naja::NajaPrivateProperty {
 public:
  VHDLIntent::SessionPtr session;
  std::string getName() const override { return "VHDLIntentSession"; }
  std::string getString() const override { return getName(); }
  void preDestroy() override {
    if (session) session->valid = false;
    NajaPrivateProperty::preDestroy();
  }
  static SessionProperty* create(NLDB* db) {
    auto* result = new SessionProperty;
    result->postCreate(db);
    return result;
  }
};
class RecordProperty final : public naja::NajaPrivateProperty {
 public:
  std::weak_ptr<VHDLIntent::Session> session;
  VHDLIntentValue record;
  std::string getName() const override { return "VHDLIntentRecord"; }
  std::string getString() const override { return getName(); }
  static void create(NLObject* object, const VHDLIntent::SessionPtr& session, VHDLIntentValue record) {
    if (auto* old = object->getProperty("VHDLIntentRecord")) old->destroy();
    auto* result = new RecordProperty;
    result->session = session;
    result->record = std::move(record);
    result->postCreate(object);
  }
};
}
void VHDLIntent::invalidate(NLDB* db) {
  if (!db) return;
  if (auto* property = dynamic_cast<SessionProperty*>(db->getProperty("VHDLIntentSession"))) {
    if (property->session) property->session->valid = false;
    property->session.reset();
  }
}
VHDLIntent::SessionPtr VHDLIntent::begin(NLDB* db, bool retain) {
  invalidate(db);
  if (!retain) return {};
  auto* property = dynamic_cast<SessionProperty*>(db->getProperty("VHDLIntentSession"));
  if (!property) property = SessionProperty::create(db);
  property->session = std::make_shared<Session>();
  return property->session;
}
void VHDLIntent::attach(NLObject* object, const SessionPtr& session, VHDLIntentValue record) {
  if (object && session) RecordProperty::create(object, session, std::move(record));
}
const VHDLIntentValue* VHDLIntent::get(const NLObject* object) {
  if (!object) return nullptr;
  if (const auto* bit = dynamic_cast<const SNLBusNetBit*>(object)) return get(bit->getBus());
  if (const auto* bit = dynamic_cast<const SNLBusTermBit*>(object)) return get(bit->getBus());
  const auto* property = dynamic_cast<RecordProperty*>(object->getProperty("VHDLIntentRecord"));
  if (!property) return nullptr;
  const auto session = property->session.lock();
  return session && session->valid ? &property->record : nullptr;
}
bool VHDLIntent::available() {
  auto* universe = NLUniverse::get();
  if (!universe) return false;
  for (auto* db : universe->getDBs())
    for (auto* library : db->getLibraries()) {
      if (get(library)) return true;
      for (auto* design : library->getSNLDesigns()) if (get(design)) return true;
    }
  return false;
}
}
