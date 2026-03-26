// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef THIRD_PARTY_PY_GOOGLE_PROTOBUF_PYEXT_LAZY_UNIQUE_PTR_H_
#define THIRD_PARTY_PY_GOOGLE_PROTOBUF_PYEXT_LAZY_UNIQUE_PTR_H_

#define PY_SSIZE_T_CLEAN
#include <Python.h>

#ifdef Py_GIL_DISABLED
#include <atomic>
#include <memory>
#endif  // Py_GIL_DISABLED

namespace google {
namespace protobuf {
namespace python {

template <class T>
class LazyUniquePtr {
 public:
  LazyUniquePtr() = default;
  ~LazyUniquePtr();

  // Returns the pointer to the object if it has been initialized, otherwise
  // returns nullptr.
  //
  // Exercise caution when testing the return value against nullptr. Unless we
  // are being called from a mutating method (eg. msg.Clear()), this is a race
  // condition and the pointer could become non-null at any time.
  T* TryGet();

  // Returns the pointer to the object, initializing it if necessary.
  T* Get();

 private:
#ifdef Py_GIL_DISABLED
  std::atomic<T*> ptr_ = nullptr;
#else
  T* ptr_ = nullptr;
#endif
};

#ifdef Py_GIL_DISABLED

template <class T>
T* LazyUniquePtr<T>::TryGet() {
  return ptr_.load(std::memory_order_acquire);
}

// Returns the pointer to the object, initializing it if necessary.
template <class T>
T* LazyUniquePtr<T>::Get() {
  T* instance = ptr_.load(std::memory_order_acquire);
  if (instance != nullptr) return instance;

  std::unique_ptr<T> obj(new T());
  T* expected = nullptr;
  return ptr_.compare_exchange_strong(expected, obj.get(),
                                      std::memory_order_release,
                                      std::memory_order_acquire)
             ? obj.release()
             : expected;
}

template <class T>
LazyUniquePtr<T>::~LazyUniquePtr() {
  // Relaxed memory order is sufficient because the object is require to be
  // quiescent before destruction.
  delete ptr_.load(std::memory_order_relaxed);
}

#else

template <class T>
LazyUniquePtr<T>::~LazyUniquePtr() {
  delete ptr_;
}

template <class T>
T* LazyUniquePtr<T>::TryGet() {
  return ptr_;
}

template <class T>
T* LazyUniquePtr<T>::Get() {
  if (ptr_ == nullptr) ptr_ = new T();
  return ptr_;
}

#endif  // Py_GIL_DISABLED

}  // namespace python
}  // namespace protobuf
}  // namespace google

#endif  // THIRD_PARTY_PY_GOOGLE_PROTOBUF_PYEXT_LAZY_UNIQUE_PTR_H_
