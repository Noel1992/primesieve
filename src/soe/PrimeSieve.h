///
/// @file   PrimeSieve.h
/// @brief  The PrimeSieve class provides an easy API for prime
///         sieving (single-threaded).
///
/// Copyright (C) 2013 Kim Walisch, <kim.walisch@gmail.com>
///
/// This file is distributed under the New BSD License. See the
/// LICENSE file in the top level directory.
///

#ifndef PRIMESIEVE_H
#define PRIMESIEVE_H

#define PRIMESIEVE_VERSION "4.2"
#define PRIMESIEVE_MAJOR_VERSION 4
#define PRIMESIEVE_MINOR_VERSION 2
#define PRIMESIEVE_YEAR "2013"

#include "PrimeSieveCallback.h"

#include <stdint.h>
#include <string>
#include <vector>

namespace soe {
  class PrimeFinder;
  class LockGuard;
}

/// PrimeSieve is a highly optimized C++ implementation of the
/// segmented sieve of Eratosthenes that generates primes and prime
/// k-tuplets (twin primes, prime triplets, ...) in order up to 2^64
/// maximum. The README file describes the algorithms used in more
/// detail and doc/EXAMPLES contains source code examples.
///
class PrimeSieve {
  friend class soe::PrimeFinder;
  friend class soe::LockGuard;
public:
  /// Public flags for use with setFlags(int)
  /// @pre flag < (1 << 20)
  enum {
    COUNT_PRIMES      = 1 << 0,
    COUNT_TWINS       = 1 << 1,
    COUNT_TRIPLETS    = 1 << 2,
    COUNT_QUADRUPLETS = 1 << 3,
    COUNT_QUINTUPLETS = 1 << 4,
    COUNT_SEXTUPLETS  = 1 << 5,
    COUNT_SEPTUPLETS  = 1 << 6,
    PRINT_PRIMES      = 1 << 7,
    PRINT_TWINS       = 1 << 8,
    PRINT_TRIPLETS    = 1 << 9,
    PRINT_QUADRUPLETS = 1 << 10,
    PRINT_QUINTUPLETS = 1 << 11,
    PRINT_SEXTUPLETS  = 1 << 12,
    PRINT_SEPTUPLETS  = 1 << 13,
    PRINT_STATUS      = 1 << 14,
    CALCULATE_STATUS  = 1 << 15
  };
  PrimeSieve();
  PrimeSieve(PrimeSieve&, int);
  virtual ~PrimeSieve();
  // Getters
  uint64_t getStart() const;
  uint64_t getStop() const;
  int getSieveSize() const;
  int getPreSieve() const;
  int getFlags() const;
  double getStatus() const;
  double getSeconds() const;
  // Setters
  void setStart(uint64_t);
  void setStop(uint64_t);
  void setSieveSize(int);
  void setPreSieve(int);
  void setFlags(int);
  void addFlags(int);
  // Bool is*
  bool isFlag(int) const;
  bool isGenerate() const;
  bool isCount() const;
  bool isCount(int) const;
  bool isPrint() const;
  bool isPrint(int) const;
  // Sieve
  virtual void sieve();
  void sieve(uint64_t, uint64_t);
  void sieve(uint64_t, uint64_t, int);
  // Generate
  void generatePrimes(uint32_t, uint32_t, void (*)(uint32_t));
  void generatePrimes(uint64_t, uint64_t, void (*)(uint64_t));
  void generatePrimes(uint64_t, uint64_t, void (*)(uint64_t, int));
  void generatePrimes(uint32_t, uint32_t, PrimeSieveCallback<uint32_t>*);
  void generatePrimes(uint64_t, uint64_t, PrimeSieveCallback<uint64_t>*);
  void generatePrimes(uint64_t, uint64_t, PrimeSieveCallback<uint64_t, int>*);
  // Print
  void printPrimes(uint64_t, uint64_t);
  void printTwins(uint64_t, uint64_t);
  void printTriplets(uint64_t, uint64_t);
  void printQuadruplets(uint64_t, uint64_t);
  void printQuintuplets(uint64_t, uint64_t);
  void printSextuplets(uint64_t, uint64_t);
  void printSeptuplets(uint64_t, uint64_t);
  // Count
  uint64_t countPrimes(uint64_t, uint64_t);
  uint64_t countTwins(uint64_t, uint64_t);
  uint64_t countTriplets(uint64_t, uint64_t);
  uint64_t countQuadruplets(uint64_t, uint64_t);
  uint64_t countQuintuplets(uint64_t, uint64_t);
  uint64_t countSextuplets(uint64_t, uint64_t);
  uint64_t countSeptuplets(uint64_t, uint64_t);
  // Count getters
  uint64_t getPrimeCount() const;
  uint64_t getTwinCount() const;
  uint64_t getTripletCount() const;
  uint64_t getQuadrupletCount() const;
  uint64_t getQuintupletCount() const;
  uint64_t getSextupletCount() const;
  uint64_t getSeptupletCount() const;
  uint64_t getCount(int) const;
  // Version
  static std::string getVersion();
  static int getMajorVersion();
  static int getMinorVersion();
protected:
  /// Sieve primes >= start_
  uint64_t start_;
  /// Sieve primes <= stop_
  uint64_t stop_;
  /// Prime number and prime k-tuplet counts
  std::vector<uint64_t> counts_;
  /// Time elapsed of sieve()
  double seconds_;
  uint64_t getInterval() const;
  void reset();
  virtual bool updateStatus(uint64_t, bool);
  virtual void setLock();
  virtual void unsetLock();
private:
  /// Private flags
  /// @pre flag >= (1 << 20)
  enum {
    CALLBACK32        = 1 << 20,
    CALLBACK64        = 1 << 21,
    CALLBACK64_TN     = 1 << 22,
    CALLBACK32_OBJ    = 1 << 23,
    CALLBACK64_OBJ    = 1 << 24,
    CALLBACK64_OBJ_TN = 1 << 25
  };
  enum {
      INIT_STATUS = 0,
    FINISH_STATUS = 10
  };
  struct SmallPrime
  {
    uint32_t firstPrime;
    uint32_t lastPrime;
    int index;
    std::string str;
  };
  static const SmallPrime smallPrimes_[8];
  /// Sum of all processed segments
  uint64_t processed_;
  /// Sum of processed segments that hasn't been updated yet
  uint64_t toUpdate_;
  /// Status of sieve() in percent
  double percent_;
  /// Pre-sieve multiples of small primes <= preSieve_
  int preSieve_;
  /// Sieve size in kilobytes
  int sieveSize_;
  /// Flags (settings) for PrimeSieve e.g. COUNT_PRIMES, PRINT_TWINS, ...
  int flags_;
  /// ParallelPrimeSieve thread number
  int threadNum_;
  /// Pointer to the parent ParallelPrimeSieve object
  PrimeSieve* parent_;
  /// Callbacks for use with generatePrimes()
  void (*callback32_)(uint32_t);
  void (*callback64_)(uint64_t);
  void (*callback64_tn_)(uint64_t, int);
  PrimeSieveCallback<uint32_t>* psc32_;
  PrimeSieveCallback<uint64_t>* psc64_;
  PrimeSieveCallback<uint64_t, int>* psc64_tn_;
  static void printStatus(double, double);
  bool isFlag(int, int) const;
  bool isPublicFlags(int) const;
  bool isStatus() const;
  bool isParallelPrimeSieveChild() const;
  void doSmallPrime(const SmallPrime&);
};

#endif
