// also available live: https://wandbox.org/permlink/80mBkv3UIj4S3kDb

// leak_detector.h
#ifndef LEAK_DETECTOR_H
#define LEAK_DETECTOR_H
#include <cstddef>
#include <atomic>
#include <new>
class Accountant {
   std::atomic<long long> cur;
   Accountant() : cur{ 0LL } { // note: private
   }
public:
   // deleted copy operations
   Accountant(const Accountant&) = delete;
   Accountant& operator=(const Accountant&) = delete;
   // to access the singleton object
   static auto& get() { // auto used for simplicity
      static Accountant singleton; // here it is
      return singleton;
   }
   // services offered by the object
   // n bytes were allocated
   void take(std::size_t n) { cur += n; }
   // n bytes were deallocated
   void give_back(std::size_t n) { cur -= n; }
   // number of bytes currently allocated
   std::size_t how_much() const { return cur.load(); }
};
// allocation operators (free functions)
void *operator new(std::size_t);
void *operator new[](std::size_t);
void operator delete(void*) noexcept;
void operator delete[](void*) noexcept;
#endif

// -----------------------------
// leak_detector.cpp
// -----------------------------
// #include "leak_detector.h"
#include <cstdlib>
void *operator new(std::size_t n) {
   // allocate n bytes plus enough space to hide n
   void *p = std::malloc(n + sizeof n); // to revisit
   // signal failure to meet postconditions if needed
   if(!p) throw std::bad_alloc{};
   // hide n at the beginning of the allocated block
   auto q = static_cast<std::size_t*>(p);
   *q = n; // to revisit
   // inform the Accountant of the allocation
   Accountant::get().take(n);
   // return the beginning of the requested block memory
   return q + 1; // to revisit
}
void *operator new[](std::size_t n) {
   // exactly the same as operator new above
   void *p = std::malloc(n + sizeof n); // to revisit
   if(!p) throw std::bad_alloc{};
   auto q = static_cast<std::size_t*>(p);
   *q = n; // to revisit
   Accountant::get().take(n);
   return q + 1; // to revisit
}

void operator delete(void *p) noexcept {
   // delete on a null pointer is a no-op
   if(!p) return;
   // find the beginning of the block that was allocated
   auto q = static_cast<std::size_t*>(p) - 1; // to revisit
   // inform the Accountant of the deallocation
   Accountant::get().give_back(*q);
   // free the memory
   std::free(q);
}
void operator delete[](void *p) noexcept {
   // exactly the same as operator delete above
   if(!p) return;
   auto q = static_cast<std::size_t*>(p) - 1; // to revisit
   Accountant::get().give_back(*q);
   std::free(q);
}

// -----------------------------
// main.cpp
// -----------------------------
// #include "leak_detector.h"
#include <iostream>
int main() {
   auto pre = Accountant::get().how_much();
   { // BEGIN
      int *p = new int{ 3 };
      int *q = new int[10]{ }; // initialized to zero
      delete p;
     // oops! Forgot to delete[] q
   } // END
   auto post = Accountant::get().how_much();
   // with this code, supposing sizeof(int)==4, we
   // expect to see "Leaked 40 bytes" printed
   if(post != pre)
      std::cout << "Leaked " << (post - pre) << " bytes\n";
}
