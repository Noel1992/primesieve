//
// Copyright (c) 2012 Kim Walisch, <kim.walisch@gmail.com>.
// All rights reserved.
//
// This file is part of primesieve.
// Homepage: http://primesieve.googlecode.com
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//
//   * Redistributions of source code must retain the above copyright
//     notice, this list of conditions and the following disclaimer.
//   * Redistributions in binary form must reproduce the above
//     copyright notice, this list of conditions and the following
//     disclaimer in the documentation and/or other materials provided
//     with the distribution.
//   * Neither the name of the author nor the names of its
//     contributors may be used to endorse or promote products derived
//     from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.

#include "EratBig.h"
#include "SieveOfEratosthenes.h"
#include "SieveOfEratosthenes-inline.h"
#include "WheelFactorization.h"
#include "imath.h"

#include <stdint.h>
#include <stdexcept>
#include <cstdlib>
#include <algorithm>
#include <vector>
#include <list>

namespace soe {

EratBig::EratBig(const SieveOfEratosthenes& soe) :
  Modulo210Wheel_t(soe),
  stock_(NULL),
  log2SieveSize_(ilog2(soe.getSieveSize())),
  moduloSieveSize_(soe.getSieveSize() - 1)
{
  // EratBig uses bitwise operations that require a power of 2 sieve size
  if (!isPowerOf2(soe.getSieveSize()))
    throw std::invalid_argument("EratBig: sieveSize must be a power of 2 (2^n).");
  setListsSize(soe);
  init();
}

EratBig::~EratBig() {
  for (std::list<Bucket*>::iterator it = pointers_.begin();
      it != pointers_.end(); ++it)
    delete[] *it;
}

void EratBig::setListsSize(const SieveOfEratosthenes& soe) {
  // max values in crossOff()
  uint_t maxSievingPrime  = soe.getSqrtStop() / SieveOfEratosthenes::NUMBERS_PER_BYTE;
  uint_t maxNextMultiple  = maxSievingPrime * getMaxFactor() + getMaxFactor();
  uint_t maxMultipleIndex = soe.getSieveSize() - 1 + maxNextMultiple;
  uint_t maxSegmentCount  = maxMultipleIndex >> log2SieveSize_;
  uint_t size = maxSegmentCount + 1;
  lists_.resize(size, NULL);
}

void EratBig::init() {
  // initialize each bucket list with an empty bucket
  for (uint_t i = 0; i < lists_.size(); i++)
    pushBucket(i);
}

/// Add an empty bucket from the bucket stock_ to the
/// front of lists_[segment], if the bucket stock_ is
/// empty new buckets are allocated first.
///
void EratBig::pushBucket(uint_t segment) {
  if (stock_ == NULL) {
    Bucket* more = new Bucket[BUCKETS_PER_ALLOC];
    stock_ = &more[0];
    pointers_.push_back(more);
    for(int i = 0; i < BUCKETS_PER_ALLOC - 1; i++)
      more[i].setNext(&more[i + 1]);
    more[BUCKETS_PER_ALLOC - 1].setNext(NULL);
  }
  Bucket* bucket = stock_;
  stock_ = stock_->next();
  bucket->setNext(lists_[segment]);
  lists_[segment] = bucket;
}

/// Add a new sieving prime
/// @see addSievingPrime() in WheelFactorization.h
///
void EratBig::storeSievingPrime(uint_t sievingPrime, uint_t multipleIndex, uint_t wheelIndex) {
  // indicates in how many segments the next multiple
  // of sievingPrime must be crossed-off
  uint_t segment = multipleIndex >> log2SieveSize_;
  multipleIndex &= moduloSieveSize_;
  // store sievingPrime in the bucket list related to
  // its next multiple's segment
  if (!lists_[segment]->store(sievingPrime, multipleIndex, wheelIndex))
    pushBucket(segment);
}

/// This algorithm is used to cross-off the multiples of big sieving
/// primes that have very few multiples per segment.
/// @see crossOffMultiples() in SieveOfEratosthenes.cpp
///
void EratBig::crossOff(uint8_t* sieve)
{
  // lists_[0] contains the buckets related to the current segment
  // i.e. its buckets contain all the sieving primes that have
  // multiples in the current segment
  Bucket*& list = lists_[0];

  while (list->hasNext() || !list->empty()) {
    Bucket* bucket = list;
    list = NULL;
    pushBucket(0);
    do {
      crossOff(*bucket, sieve);
      // reset the processed bucket and move it to the bucket stock_
      Bucket* old = bucket;
      bucket = bucket->next();
      old->reset();
      old->setNext(stock_);
      stock_ = old;
    } while (bucket != NULL);
  }

  // lists_[0] has been processed, thus the list related to the
  // next segment lists_[1] moves to lists_[0]
  Bucket* tmp = list;
  std::copy(lists_.begin() + 1, lists_.end(), lists_.begin());
  lists_.back() = tmp;
}

/// Cross-off the next multiple of each sieving prime within the
/// current bucket. This algorithm uses a modulo 210 wheel that skips
/// multiples of 2, 3, 5 and 7.
/// This is an optimized implementation of Tomas Oliveira e Silva's
/// cache-friendly segmented sieve of Eratosthenes algorithm:
/// http://www.ieeta.pt/~tos/software/prime_sieve.html
///
void EratBig::crossOff(Bucket& bucket, uint8_t* sieve)
{
  WheelPrime* wPrime = bucket.begin();
  WheelPrime* end    = bucket.end();

  // 2 sieving primes are processed per loop iteration to break
  // the dependency chain and reduce pipeline stalls
  for (; wPrime + 2 <= end; wPrime += 2) {
    uint_t multipleIndex0 = wPrime[0].getMultipleIndex();
    uint_t wheelIndex0    = wPrime[0].getWheelIndex();
    uint_t sievingPrime0  = wPrime[0].getSievingPrime();
    uint_t multipleIndex1 = wPrime[1].getMultipleIndex();
    uint_t wheelIndex1    = wPrime[1].getWheelIndex();
    uint_t sievingPrime1  = wPrime[1].getSievingPrime();
    // cross-off the current multiple (unset corresponding bit) of
    // sievingPrime(0|1) and calculate the next multiple
    // @see unsetBit() in WheelFactorization.h
    unsetBit(sieve, sievingPrime0, &multipleIndex0, &wheelIndex0);
    unsetBit(sieve, sievingPrime1, &multipleIndex1, &wheelIndex1);
    uint_t segment0 = multipleIndex0 >> log2SieveSize_;
    uint_t segment1 = multipleIndex1 >> log2SieveSize_;
    multipleIndex0 &= moduloSieveSize_;
    multipleIndex1 &= moduloSieveSize_;
    // move sievingPrime(0|1) to the bucket list
    // related to its next multiple
    if (!lists_[segment0]->store(sievingPrime0, multipleIndex0, wheelIndex0))
      pushBucket(segment0);
    if (!lists_[segment1]->store(sievingPrime1, multipleIndex1, wheelIndex1))
      pushBucket(segment1);
  }

  if (wPrime != end) {
    uint_t multipleIndex = wPrime->getMultipleIndex();
    uint_t wheelIndex    = wPrime->getWheelIndex();
    uint_t sievingPrime  = wPrime->getSievingPrime();
    unsetBit(sieve, sievingPrime, &multipleIndex, &wheelIndex);
    uint_t segment = multipleIndex >> log2SieveSize_;
    multipleIndex &= moduloSieveSize_;
    if (!lists_[segment]->store(sievingPrime, multipleIndex, wheelIndex))
      pushBucket(segment);
  }
}

} // namespace soe
