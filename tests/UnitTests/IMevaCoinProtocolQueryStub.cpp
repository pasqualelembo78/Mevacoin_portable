// Copyright (c) 2011-2016 The Mevacoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "IMevaCoinProtocolQueryStub.h"

bool IMevaCoinProtocolQueryStub::addObserver(MevaCoin::IMevaCoinProtocolObserver* observer) {
  return false;
}

bool IMevaCoinProtocolQueryStub::removeObserver(MevaCoin::IMevaCoinProtocolObserver* observer) {
  return false;
}

uint32_t IMevaCoinProtocolQueryStub::getObservedHeight() const {
  return observedHeight;
}

size_t IMevaCoinProtocolQueryStub::getPeerCount() const {
  return peers;
}

bool IMevaCoinProtocolQueryStub::isSynchronized() const {
  return synchronized;
}

void IMevaCoinProtocolQueryStub::setPeerCount(uint32_t count) {
  peers = count;
}

void IMevaCoinProtocolQueryStub::setObservedHeight(uint32_t height) {
  observedHeight = height;
}

void IMevaCoinProtocolQueryStub::setSynchronizedStatus(bool status) {
    synchronized = status;
}
