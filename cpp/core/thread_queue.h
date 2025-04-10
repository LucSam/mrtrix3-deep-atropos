/* Copyright (c) 2008-2024 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */

#pragma once

#include <condition_variable>
#include <stack>

#include "exception.h"
#include "memory.h"
#include "thread.h"

#define MRTRIX_QUEUE_DEFAULT_CAPACITY 128
#define MRTRIX_QUEUE_DEFAULT_BATCH_SIZE 128

namespace MR::Thread {

//* \cond skip
namespace {

// to get multi/single job/functor seamlessly:
template <class X> class __job {
public:
  using type = typename std::remove_reference<X>::type;
  using member_type = typename std::remove_reference<X>::type &;
  static X &functor(X &job) { return job; }

  template <class SingleFunctor> static SingleFunctor &get(X & /*f*/, SingleFunctor &functor) { return functor; }
};

template <class X> class __job<__Multi<X>> {
public:
  using type = typename std::remove_reference<X>::type;
  using member_type = typename std::remove_reference<X>::type;
  static X &functor(__Multi<X> &job) { return job.functor; }

  template <class SingleFunctor> static __Multi<SingleFunctor> get(__Multi<X> &f, SingleFunctor &functor) {
    return __Multi<SingleFunctor>(functor, f.num);
  }
};

} // namespace

//! \endcond

/** \addtogroup thread_classes
 * @{ */

/** \defgroup thread_queue Thread-safe queue
 * \brief Functionality for thread-safe parallel processing of queued items
 *
 * These functions and classes provide functionality for one or more \e
 * source threads to feed items into a first-in first-out queue, and one or
 * more \e sink threads to consume items. This pipeline can also extend
 * over two queues, with one or more \e pipe threads consuming items of one
 * type from the first queue, and feeding items of another type onto the
 * second queue.
 *
 * As a graphical representation of the pipeline, the following workflows
 * can be achieved:
 *
 * \code
 *     [source] \               / [sink]
 *     [source] -- queue<item> -- [sink]
 *     [source] /               \ [sink]
 *        ..                        ..
 *     N_source                   N_sink
 * \endcode
 *
 * or for a deeper pipeline:
 *
 * \code
 *     [source] \                / [pipe]  \                 / [sink]
 *     [source] -- queue<item1> -- [pipe]  -- queue<item2>  -- [sink]
 *     [source] /                \ [pipe]  /                 \ [sink]
 *        ..                         ..                          ..
 *     N_source                    N_pipe                      N_sink
 * \endcode
 *
 * By default, items are push to and pulled from the queue one by one. In
 * situations where the amount of processing per item is small, items can
 * be sent in batches to reduce the overhead of thread management (mutex
 * locking/unlocking, etc).
 *
 * The simplest way to use this functionality is via the
 * Thread::run_queue() and associated Thread::multi() and Thread::batch()
 * functions. In complex situations, it may be necessary to use the
 * Thread::Queue class directly, although that should very rarely (if ever)
 * be needed.
 *
 * \sa Thread::run_queue()
 * \sa Thread::Queue
 *
 * @{ */

//! A first-in first-out thread-safe item queue
/*! This class implements a thread-safe means of pushing data items into a
 * queue, so that they can each be processed in one or more separate
 * threads.
 *
 * \note In practice, it is almost always simpler to use the convenience
 * function Thread::run_queue(). You should never need to use the
 * Thread::Queue directly unless you have a very unusual situation.
 *
 * \section thread_queue_usage Usage overview
 *
 * Thread::Queue has somewhat unusual usage, which consists of the following
 * steps:
 * - Create an instance of a Thread::Queue
 * - Create one or more instances of the corresponding
 * Thread::Queue::Writer class, each constructed with a reference to the
 * queue. Each of these instances will automatically notify the queue that
 * its corresponding thread will be writing to the queue.
 * - Create one or more instances of the corresponding
 * Thread::Queue::Reader class, each constructed with a reference to the
 * queue. Each of these instances will automatically notify the queue that
 * its corresponding thread will be reading from the queue.
 * - Launch all threads, one per instance of Thread::Queue::Writer or
 *   Thread::Queue::Reader. Note that one of these threads can be the
 *   current thread - simply invoke the respective functor's execute()
 *   method directly once all other threads have been launched.
 * - Within the execute() method of each thread with a
 * Thread::Queue::Writer:
 *   - create an instance of Thread::Queue::Writer::Item, constructed from
 *   the corresponding Thread::Queue::Writer;
 *   - perform processing in a loop:
 *     - prepare the item using pointer semantics (i.e. *item or
 *     item->method());
 *     - use the write() method of this class to write to the queue;
 *     - break out of loop if write() returns \c false.
 *   - when the execute() method returns, the destructor of the
 *   Thread::Queue::Writer::Item class will notify the queue that its
 *   thread has finished writing to the queue.
 * - Within the execute() method of each thread with a
 * Thread::Queue::Reader:
 *   - create an instance of Thread::Queue::Reader::Item, constructed from
 *   the corresponding Thread::Queue::Reader;
 *   - perform processing in a loop:
 *     - use the read() method of this class to read the next item from the
 *     queue;
 *     - break out of the loop if read() returns \c false;
 *     - process the item using pointer semantics (i.e. *item or
 *     item->method()).
 *   - when the execute() method returns, the destructor of the
 *   Thread::Queue::Reader::Item class will notify the queue that its
 *   thread has finished reading from the queue.
 * - If all reader threads have returned, the queue will notify all writer
 * threads that processing should stop, by returning \c false from the next
 * write attempt.
 * - If all writer threads have returned and no items remain in the queue,
 * the queue will notify all reader threads that processing should stop, by
 * returning \c false from the next read attempt.
 *
 * The additional member classes are designed to be used in conjunction
 * with the MRtrix multi-threading interface. In this system, each thread
 * corresponds to an instance of a functor class, and its execute() method
 * is the function that will be run within the thread (see Thread::Exec for
 * details). For this reason:
 * - The Thread::Queue instance is designed to be created before any of the
 *   threads.
 * - The Thread::Queue::Writer and Thread::Queue::Reader classes are
 *   designed to be used as members of each functor, so that each functor
 *   must construct these classes from a reference to the queue within
 *   their own constructor. This ensures each thread registers their
 *   intention to read or write with the queue \e before their thread is
 *   launched.
 * - The Thread::Queue::Writer::Item and Thread::Queue::Reader::Item
 *   classes are designed to be instantiated within each functor's
 *   execute() method. They must be constructed from a reference to a
 *   Thread::Queue::Writer or Thread::Queue::Reader respectively, ensuring
 *   no reads or write can take place without having registered with the
 *   queue. Their destructors will also unregister from the queue, ensuring
 *   that each thread unregisters as soon as the execute() method returns,
 *   and hence \e before the thread exits.
 *
 * The Queue class performs all memory management for the items in the
 * queue. For this reason, the items are accessed via the Writer::Item &
 * Reader::Item classes. This allows items to be recycled once they have
 * been processed, reducing overheads associated with memory
 * allocation/deallocation.
 *
 * \note It is important that all instances of Thread::Queue::Writer and
 * Thread::Queue::Reader are created \e before any of the threads are
 * launched, to avoid any race conditions at startup.
 *
 * The use of Thread::Queue is best illustrated with an example:
 * \code
 * // the type of objects that will be sent through the queue:
 * class Item {
 *   public:
 *     ...
 *     // data members
 *     ...
 * };
 *
 *
 * // The use a typedef is recommended to help with readability (and typing!):
 * typedef Thread::Queue<Item> MyQueue;
 *
 *
 * // this class will write to the queue:
 * class Sender {
 *   public:
 *     // construct the 'writer' member in the constructor:
 *     Sender (MyQueue& queue) : writer (queue) { }
 *
 *     void execute () {
 *       // use a local instance of Thread::Queue<Item>::Writer::Item to write to the queue:
 *       MyQueue::Writer::Item item (writer);
 *       while (need_more_items()) {
 *         ...
 *         // prepare item
 *         *item = something();
 *         item->set (something_else);
 *         ...
 *         if (!item.write()) break; // break if write() returns false
 *       }
 *     }
 *
 *   private:
 *     MyQueue::Writer writer;
 * };
 *
 *
 * // this class will read from the queue:
 * class Receiver {
 *   public:
 *     // construct the 'reader' member in the constructor:
 *     Receiver (MyQueue& queue) : reader (queue) { }
 *
 *     void execute () {
 *       // use a local instance of Thread::Queue<Item>::Reader::Item to read from the queue:
 *       MyQueue::Reader::Item item (reader);
 *       while ((item.read())) { // break when read() returns false
 *         ...
 *         // process item
 *         do_something (*item);
 *         if (item->status()) report_error();
 *         ...
 *         if (enough_items()) return;
 *       }
 *     }
 *
 *   private:
 *     MyQueue::Reader reader;
 * };
 *
 *
 * // this is where the queue and threads are created:
 * void my_function () {
 *   // create an instance of the queue:
 *   MyQueue queue;
 *
 *   // create all functors from a reference to the queue:
 *   Sender sender (queue);
 *   Receiver receiver (queue);
 *
 *   // once all functors are created, launch their corresponding threads:
 *   Thread::Exec sender_thread (sender);
 *   Thread::Exec receiver_thread (receiver);
 * }
 * \endcode
 *
 * \section thread_queue_rationale Rationale for the Writer, Reader, and Item member classes
 *
 * The motivation for the use of additional member classes to perform the
 * actual process of writing and reading to and from the queue is related
 * to the need to keep track of the number of processes currently using the
 * queue. This is essential to ensure that threads are notified when the
 * queue is closed. This happens either when all readers have finished
 * reading; or when all writers have finished writing and no items are left
 * in the queue. This is complicated by the need to ensure that the various
 * operations are called in the right order to avoid deadlocks.
 *
 * There are essentially 4 operations that need to take place:
 * - registering an intention to read/write from/to the queue
 * - launching the corresponding thread
 * - unregistering from the queue
 * - terminating the thread
 *
 * For proper multi-threaded operations, these operations must take place
 * in the order above. Moreover, each operation must be completed for
 * all users of the queue before any of them can perform the next
 * operation. The use of additional member classes ensures that threads
 * have to register their intention to read or write from the queue, and
 * that they unregister from the queue once their processing is done.
 *
 * While this could have been achieved simply with the appropriate member
 * functions (i.e. register(), unregister(), %read() & %write() methods in
 * the main Queue class), this places a huge burden on the developer to get
 * it right. Using these member functions reduces the chance of coding
 * errors, and in fact reduces the total amount of code that needs to be
 * written to use the Queue in a safe manner.
 *
 * The Item classes additionally simplify the memory management of the
 * items in the queue, by preventing direct access to the underlying
 * pointers, and ensuring the Queue itself is responsible for all
 * allocation and deallocation of items as needed.
 *
 * \sa Thread::run_queue()
 */
template <class T> class Queue {
public:
  //! Construct a Queue of items of type \c T
  /*! \param description a string identifying the queue for degugging purposes
   * \param buffer_size the maximum number of items that can be pushed onto the queue before
   * blocking. If a thread attempts to push more data onto the queue when the
   * queue already contains this number of items, the thread will block until
   * at least one item has been popped.  By default, the buffer size is
   * MRTRIX_QUEUE_DEFAULT_CAPACITY items.
   */
  Queue(const std::string &description = "unnamed", size_t buffer_size = MRTRIX_QUEUE_DEFAULT_CAPACITY)
      : buffer(new T *[buffer_size]),
        front(buffer),
        back(buffer),
        capacity(buffer_size),
        writer_count(0),
        reader_count(0),
        name(description) {
    assert(capacity > 0);
  }

  Queue(const Queue &) = delete;
  Queue(Queue &&) = default;
  Queue &operator=(const Queue &) = delete;
  Queue &operator=(Queue &&) = default;

  ~Queue() { delete[] buffer; }

  //! This class is used to register a writer with the queue
  /*! Items cannot be written directly onto a Thread::Queue queue. An
   * object of this class must first be instanciated to notify the queue
   * that a section of code will be writing to the queue. The actual
   * process of writing items to the queue is done via the Writer::Item
   * class.
   *
   * \sa Thread::Queue for more detailed information and examples.
   * \sa Thread::run_queue() for a much more user-friendly way of setting
   * up a queue.  */
  class Writer {
  public:
    //! Register a Writer object with the queue
    /*! The Writer object will register itself with the queue as a
     * writer. */
    Writer(Queue<T> &queue) : Q(queue) { Q.register_writer(); }
    Writer(const Writer &W) : Q(W.Q) { Q.register_writer(); }

    //! This class is used to write items to the queue
    /*! Items cannot be written directly onto a Thread::Queue queue. An
     * object of this class must be instantiated and used to write to the
     * queue.
     *
     * \sa Thread::Queue for more detailed information and examples.
     * \sa Thread::run_queue() for a much more user-friendly way of setting
     * up a queue.  */
    class Item {
    public:
      //! Construct a Writer::Item object
      /*! The Writer::Item object can only be instantiated from a
       * Writer object, ensuring that the corresponding section of code
       * has already registered as a writer with the queue. The
       * destructor for this object will unregister from the queue.
       *
       * \note There should only be one Writer::Item object per Writer.
       * */
      Item(const Writer &writer) : Q(writer.Q), p(Q.get_item()) {}
      //! Unregister the parent Writer from the queue
      ~Item() { Q.unregister_writer(); }

      using item_type = T;

      //! Push the item onto the queue
      FORCE_INLINE bool write() { return Q.push(p); }
      FORCE_INLINE T &operator*() const throw() { return *p; }
      FORCE_INLINE T *operator->() const throw() { return p; }

    private:
      Queue<T> &Q;
      T *p;
    };

    Item placeholder() const { return Item(*this); }

  private:
    Queue<T> &Q;
  };

  //! This class is used to register a reader with the queue
  /*! Items cannot be read directly from a Thread::Queue queue. An
   * object of this class must be instanciated to notify the queue
   * that a section of code will be reading from the queue. The actual
   * process of reading items from the queue is done via the Reader::Item
   * class.
   *
   * \sa Thread::Queue for more detailed information and examples.
   * \sa Thread::run_queue() for a much more user-friendly way of setting
   * up a queue.  */
  class Reader {
  public:
    //! Register a Reader object with the queue.
    /*! The Reader object will register itself with the queue as a
     * reader. */
    Reader(Queue<T> &queue) : Q(queue) { Q.register_reader(); }
    Reader(const Reader &reader) : Q(reader.Q) { Q.register_reader(); }

    //! This class is used to read items from the queue
    /*! Items cannot be read directly from a Thread::Queue queue. An
     * object of this class must be instanciated and used to read from the
     * queue.
     *
     * \sa Thread::Queue for more detailed information and examples.
     * \sa Thread::run_queue() for a much more user-friendly way of setting
     * up a queue.  */
    class Item {
    public:
      //! Construct a Reader::Item object
      /*! The Reader::Item object can only be instantiated from a
       * Reader object, ensuring that the corresponding section of code
       * has already registered as a reader with the queue. The
       * destructor for this object will unregister from the queue.
       *
       * \note There should only be one Reader::Item object per
       * Reader. */
      Item(const Reader &reader) : Q(reader.Q), p(nullptr) {}
      //! Unregister the parent Reader from the queue
      ~Item() { Q.unregister_reader(); }

      using item_type = T;

      //! Get next item from the queue
      FORCE_INLINE bool read() { return Q.pop(p); }
      FORCE_INLINE T *stash() throw() {
        T *item = p;
        p = nullptr;
        return item;
      }
      FORCE_INLINE void recycle(T *item) const throw() { Q.recycle(item); }
      FORCE_INLINE T &operator*() const throw() { return *p; }
      FORCE_INLINE T *operator->() const throw() { return p; }
      FORCE_INLINE bool operator!() const throw() { return !p; }

    private:
      Queue<T> &Q;
      T *p;
    };

    Item placeholder() const { return Item(*this); }

  private:
    Queue<T> &Q;
  };

  //! Print out a status report for debugging purposes
  void status() {
    std::lock_guard<std::mutex> lock(mutex);
    std::cerr << "Thread::Queue \"" + name + "\": " << writer_count << " writer" << (writer_count > 1 ? "s" : "")
              << ", " << reader_count << " reader" << (reader_count > 1 ? "s" : "") << ", items waiting: " << size()
              << "\n";
  }

private:
  std::mutex mutex;
  std::condition_variable more_data, more_space;
  T **buffer;
  T **front;
  T **back;
  size_t capacity;
  size_t writer_count, reader_count;
  std::stack<T *, std::vector<T *>> item_stack;
  std::vector<std::unique_ptr<T>> items;
  std::string name;

  void register_writer() {
    std::lock_guard<std::mutex> lock(mutex);
    ++writer_count;
  }
  void unregister_writer() {
    std::lock_guard<std::mutex> lock(mutex);
    assert(writer_count);
    --writer_count;
    if (!writer_count) {
      DEBUG("no writers left on queue \"" + name + "\"");
      more_data.notify_all();
    }
  }
  void register_reader() {
    std::lock_guard<std::mutex> lock(mutex);
    ++reader_count;
  }
  void unregister_reader() {
    std::lock_guard<std::mutex> lock(mutex);
    assert(reader_count);
    --reader_count;
    if (!reader_count) {
      DEBUG("no readers left on queue \"" + name + "\"");
      more_space.notify_all();
    }
  }

  FORCE_INLINE bool empty() const { return (front == back); }
  FORCE_INLINE bool full() const { return (inc(back) == front); }
  FORCE_INLINE size_t size() const { return ((back < front ? back + capacity : back) - front); }

  FORCE_INLINE T *get_item() {
    std::lock_guard<std::mutex> lock(mutex);
    T *item(new T);
    items.push_back(std::unique_ptr<T>(item));
    return item;
  }

  FORCE_INLINE bool push(T *&item) {
    std::unique_lock<std::mutex> lock(mutex);
    more_space.wait(lock, [this] { return !(full() && reader_count); });
    if (!reader_count)
      return false;
    *back = item;
    back = inc(back);
    if (item_stack.empty()) {
      item = new T;
      items.push_back(std::unique_ptr<T>(item));
    } else {
      item = item_stack.top();
      item_stack.pop();
    }
    more_data.notify_one();
    return true;
  }

  FORCE_INLINE bool pop(T *&item) {
    std::unique_lock<std::mutex> lock(mutex);
    if (item)
      item_stack.push(item);
    item = nullptr;
    more_data.wait(lock, [this] { return !(empty() && writer_count); });
    if (empty() && !writer_count)
      return false;
    item = *front;
    front = inc(front);
    more_space.notify_one();
    return true;
  }

  FORCE_INLINE void recycle(T *&item) {
    std::unique_lock<std::mutex> lock(mutex);
    if (item)
      item_stack.push(item);
  }

  FORCE_INLINE T **inc(T **p) const {
    ++p;
    if (p >= buffer + capacity)
      p = buffer;
    return p;
  }
};

//* \cond skip

namespace {
/********************************************************************
 * convenience Functor classes for use in Thread::run_queue()
 ********************************************************************/
template <class Item> struct __Batch {
  __Batch(size_t number) : num(number) {}
  size_t num;
};

template <class Item> struct __batch_size {
  __batch_size(const Item &) {}
  operator size_t() const { return 0; }
};
template <class Item> struct __batch_size<__Batch<Item>> {
  __batch_size(const __Batch<Item> &item) : n(item.num) {}
  operator size_t() const { return n; }
  const size_t n;
};

/*! wrapper classes to extend simple functors designed for use with
 * Thread::run_queue with functionality needed for use with Thread::Queue */

template <class Item> struct Type {
  using item = Item;
  using queue = Queue<Item>;
  using reader = typename queue::Reader;
  using writer = typename queue::Writer;
  using read_item = typename reader::Item;
  using write_item = typename writer::Item;
};

template <class Item> struct Type<__Batch<Item>> {
  using item = Item;
  using queue = Queue<std::vector<Item>>;
  using reader = typename queue::Reader;
  using writer = typename queue::Writer;
  using read_item = typename reader::Item;
  using write_item = typename writer::Item;
};

template <class Item> struct FetchItem {
  FetchItem(typename Type<Item>::reader &item) : in(item.placeholder()) {}
  bool read() { return in.read(); }
  Item &value() { return (*in); }
  typename Type<Item>::read_item in;
};

template <class Item> struct FetchItem<__Batch<Item>> {
  FetchItem(typename Type<__Batch<Item>>::reader &in) : in(in.placeholder()), n(0) {}
  bool read() {
    if (!in)
      return in.read();
    ++n;
    if (n >= in->size()) {
      if (!in.read())
        return false;
      n = 0;
    }
    return true;
  }
  Item &value() { return (*in)[n]; }
  typename Type<__Batch<Item>>::read_item in;
  size_t n;
};

template <class Item> struct StoreItem {
  StoreItem(size_t, typename Type<Item>::writer &item) : out(item.placeholder()) {}
  bool write() { return out.write(); }
  Item &value() { return (*out); }
  bool flush() { return true; }
  typename Type<Item>::write_item out;
};

template <class Item> struct StoreItem<__Batch<Item>> {
  StoreItem(size_t batch_size, typename Type<__Batch<Item>>::writer &item)
      : out(item.placeholder()), batch_size(batch_size), n(0) {
    out->resize(batch_size);
  }
  bool write() {
    ++n;
    if (n >= batch_size) {
      n = 0;
      if (!out.write())
        return false;
      out->resize(batch_size);
    }
    return true;
  }
  Item &value() { return (*out)[n]; }
  void flush() {
    if (n) {
      out->resize(n);
      out.write();
    }
  }
  typename Type<__Batch<Item>>::write_item out;
  const size_t batch_size;
  size_t n;
};

template <class Item, class Functor> struct __Source {
  using item_t = typename Type<Item>::item;
  using queue_t = typename Type<Item>::queue;
  using writer_t = typename Type<Item>::writer;
  using functor_t = typename __job<Functor>::member_type;

  writer_t writer;
  functor_t func;
  size_t batch_size;

  __Source(queue_t &queue, Functor &functor, const Item &item)
      : writer(queue), func(__job<Functor>::functor(functor)), batch_size(__batch_size<Item>(item)) {}

  void execute() {
    auto out = StoreItem<Item>(batch_size, writer);
    do {
      if (!func(out.value()))
        break;
    } while (out.write());
    out.flush();
  }
};

template <class Item1, class Functor, class Item2> struct __Pipe {
  using item1_t = typename Type<Item1>::item;
  using item2_t = typename Type<Item2>::item;
  using queue1_t = typename Type<Item1>::queue;
  using queue2_t = typename Type<Item2>::queue;
  using reader_t = typename Type<Item1>::reader;
  using writer_t = typename Type<Item2>::writer;
  using functor_t = typename __job<Functor>::member_type;

  reader_t reader;
  writer_t writer;
  functor_t func;
  const size_t batch_size;

  __Pipe(queue1_t &queue_in, Functor &functor, queue2_t &queue_out, const Item2 &item2)
      : reader(queue_in),
        writer(queue_out),
        func(__job<Functor>::functor(functor)),
        batch_size(__batch_size<Item2>(item2)) {}

  void execute() {
    auto in = FetchItem<Item1>(reader);
    auto out = StoreItem<Item2>(batch_size, writer);
    while (in.read()) {
      if (func(in.value(), out.value())) {
        if (!out.write())
          break;
      }
    }
    out.flush();
  }
};

template <class Item, class Functor> struct __Sink {
  using item_t = typename Type<Item>::item;
  using queue_t = typename Type<Item>::queue;
  using reader_t = typename Type<Item>::reader;
  using functor_t = typename __job<Functor>::member_type;

  reader_t reader;
  functor_t func;

  __Sink(queue_t &queue, Functor &functor) : reader(queue), func(__job<Functor>::functor(functor)) {}

  void execute() {
    auto in = FetchItem<Item>(reader);
    while (in.read()) {
      if (!func(in.value()))
        return;
    }
  }
};

} // namespace

//! \endcond

//! used to request batched processing of items
/*! This function is used in combination with Thread::run_queue to request
 * that the items \a object be processed in batches of \a number items
 * (defaults to MRTRIX_QUEUE_DEFAULT_BATCH_SIZE).
 * \sa Thread::run_queue() */
template <class Item> inline __Batch<Item> batch(const Item &, size_t number = MRTRIX_QUEUE_DEFAULT_BATCH_SIZE) {
  return __Batch<Item>(number);
}

//! convenience function to set up and run a 2-stage multi-threaded pipeline.
/*! This function (and its 3-stage equivalent
 * Thread::run_queue(const Source&, const Item1&, const Pipe&, const Item2&, const Sink&, size_t))
 * simplify the process of setting up a multi-threaded processing chain
 * that should meet most users' needs.
 *
 * The arguments to this function correspond to an instance of the Source,
 * the Sink, and optionally the Pipe functors, in addition to an instance
 * of the Items to be passed through each stage of the pipeline - these are
 * provided purely to specify the type of object to pass through the
 * queue(s).
 *
 * \section thread_run_queue_functors Functors
 *
 * The 3 types of functors each have a specific purpose, and corresponding
 * requirements as described below:
 *
 * \par Source: the input functor
 * The Source class must at least provide the method:
 * \code
 * bool operator() (Item& item);
 * \endcode
 * This function prepares the \a item passed to it, and should return \c
 * true if further items need to be processed, or \c false to signal that
 * no further items are to be sent through the queue (at which point the
 * corresponding thread(s) will exit).
 *
 * \par Sink: the output functor
 * The Sink class must at least provide the method:
 * \code
 * bool operator() (const Item& item);
 * \endcode
 * This function processes the \a item passed to it, and should return \c
 * true when ready to process further items, or \c false to signal the end
 * of processing (at which point the corresponding thread(s) will exit).
 *
 * \par Pipe: the processing functor (for 3-stage pipeline only)
 * The Pipe class must at least provide the method:
 * \code
 * bool operator() (const Item1& item_in, Item2& item_out);
 * \endcode
 * This function processes the \a item_in passed to it, and prepares
 * \a item_out for the next stage of the pipeline. It should return \c
 * true if the item processed is to be sent to the next stage in the
 * pipeline, and false if it is to be discarded - note that this is
 * very different from the other functors, where returning false signals
 * end of processing.
 *
 * \section thread_run_queue_example Simple example
 *
 * This is a simple demo application that generates a linear sequence of
 * numbers and sums them up:
 *
 * \code
 * const size_t max_count;
 *
 * // the functor that will generate the items:
 * class Source {
 *   public:
 *     Source () : count (0) { }
 *     bool operator() (size_t& item) {
 *       item = count++;
 *       return count < max_count; // stop when max_count is reached
 *     }
 * };
 *
 * // the functor that will consume the items:
 * class Sink {
 *   public:
 *     Sink (size_t& total) :
 *       grand_total (grand_total),
 *       total (0) { }
 *     ~Sink () { // update grand_total in destructor
 *       grand_total += total;
 *     }
 *     bool operator() (const size_t& item) {
 *       total += item;
 *       return true;
 *    }
 *  protected:
 *    size_t& grand_total;
 * };
 *
 * void run ()
 * {
 *   size_t grand_total = 0;
 *   Source source;
 *   Sink sink (grand_total);
 *
 *   // run a single-source => single-sink pipeline:
 *   Thread::run_queue (source, size_t(), sink);
 * }
 * \endcode
 *
 * \section thread_run_queue_multi Parallel execution of functors
 *
 * If a functor is to be run over multiple parallel threads of execution,
 * it should be wrapped in a call to Thread::multi() before being passed
 * to the Thread::run_queue() functions.  The Thread::run_queue() functions
 * will then create additional instances of the relevant functor using its
 * copy constructor; care should therefore be taken to ensure that the
 * functor's copy constructor behaves appropriately.
 *
 * For example, using the code above:
 *
 * \code
 * ...
 *
 * void run ()
 * {
 *   ...
 *
 *   // run a single-source => multi-sink pipeline:
 *   Thread::run_queue (source, size_t(), Thread::multi (sink));
 * }
 * \endcode
 *
 * For the functor that is being multi-threaded, the default number of
 * threads instantiated will depend on the "NumberOfThreads" entry in the
 * MRtrix confugration file, or can be set at the command-line using the
 * -nthreads option. This number can also be set as additional optional
 * argument to Thread::multi().
 *
 * Note that any functor can be parallelised in this way. In the example
 * above, the Source functor could have been wrapped in Thread::multi()
 * instead if this was the behaviour required:
 *
 * \code
 * ...
 *
 * void run ()
 * {
 *   ...
 *
 *   // run a multi-source => single-sink pipeline:
 *   Thread::run_queue (Thread::multi (source), size_t(), sink);
 * }
 * \endcode
 *
 *
 * \section thread_run_queue_batch Batching items
 *
 * In cases where the amount of processing per item is small, the overhead
 * of managing the concurrent access to the various queues from all the
 * threads may become prohibitive (see \ref multithreading for details). In
 * this case, it is a good idea to process the items in batches, which
 * drastically reduces the number of accesses to the queue. This can be
 * done by wrapping the items in a call to Thread::batch():
 *
 * \code
 * ...
 *
 * void run ()
 * {
 *   ...
 *
 *   // run a single-source => multi-sink pipeline on batches of size_t items:
 *   Thread::run_queue (source, Thread::batch (size_t()), Thread::multi (sink));
 * }
 * \endcode
 *
 * By default, batches consist of MRTRIX_QUEUE_DEFAULT_BATCH_SIZE items
 * (defined as 128). This can be set explicitly by providing the desired
 * size as an additional argument to Thread::batch():
 *
 * \code
 * ...
 *
 * void run ()
 * {
 *   ...
 *
 *   // run a single-source => multi-sink pipeline on batches of 1024 size_t items:
 *   Thread::run_queue (source, Thread::batch (size_t(), 1024), Thread::multi (sink));
 * }
 * \endcode
 *
 * Obviously, Thread::multi() and Thread::batch() can be used in any
 * combination to perform the operations required.
 */

template <class Source, class Item, class Sink>
inline void run_queue(Source &&source, const Item &item, Sink &&sink, size_t capacity = MRTRIX_QUEUE_DEFAULT_CAPACITY) {
  if (threads_to_execute() == 0) {
    typename Type<Item>::item item;
    while (__job<Source>::functor(source)(item))
      if (!__job<Sink>::functor(sink)(item))
        return;
    return;
  }

  typename Type<Item>::queue queue("source->sink", capacity);
  __Source<Item, Source> source_functor(queue, source, item);
  __Sink<Item, Sink> sink_functor(queue, sink);

  auto t1 = run(__job<Source>::get(source, source_functor), "source");
  auto t2 = run(__job<Sink>::get(sink, sink_functor), "sink");

  t1.wait();
  t2.wait();

  check_app_exit_code();
}

//! convenience functions to set up and run a 3-stage multi-threaded pipeline.
/*! This function extends the 2-stage Thread::run_queue() function to allow
 * a 3-stage pipeline. For example, using the example from
 * Thread::run_queue(), the following would add an additional stage to the
 * pipeline to double the numbers as they come through:
 *
 * \code
 *
 * ...
 *
 * class Pipe {
 *   public:
 *     bool operator() (const size_t& item_in, size_t& item_out) {
 *       item_out = 2 * item_in;
 *       return true;
 *     }
 * };
 *
 * ...
 *
 * void run ()
 * {
 *   ...
 *
 *   // run a single-source => multi-pipe => single-sink pipeline on batches of size_t items:
 *   Thread::run_queue (
 *       source,
 *       Thread::batch (size_t()),
 *       Thread::multi (pipe)
 *       Thread::batch (size_t()),
 *       sink);
 * }
 * \endcode
 *
 * Note that the return value of the Pipe functor's operator() method is
 * used in this case to signal whether or not the corresponding item should
 * be sent through to the next stage (true) or discarded (false). This
 * differs from the Source & Sink functors where the corresponding return
 * value is used to signal end of processing.
 *
 * As with the 2-stage pipeline, any functor can be executed in parallel
 * (i.e. wrapped in Thread::multi()), Items do not need to be of the same
 * type, and can be batched independently with any desired size.
 * */
template <class Source, class Item1, class Pipe, class Item2, class Sink>
inline void run_queue(Source &&source,
                      const Item1 &item1,
                      Pipe &&pipe,
                      const Item2 &item2,
                      Sink &&sink,
                      size_t capacity = MRTRIX_QUEUE_DEFAULT_CAPACITY) {
  if (threads_to_execute() == 0) {
    typename Type<Item1>::item item1;
    typename Type<Item2>::item item2;
    while (__job<Source>::functor(source)(item1)) {
      if (__job<Pipe>::functor(pipe)(item1, item2))
        if (!__job<Sink>::functor(sink)(item2))
          return;
    }
    return;
  }

  typename Type<Item1>::queue queue1("source->pipe", capacity);
  typename Type<Item2>::queue queue2("pipe->sink", capacity);

  __Source<Item1, Source> source_functor(queue1, source, item1);
  __Pipe<Item1, Pipe, Item2> pipe_functor(queue1, pipe, queue2, item2);
  __Sink<Item2, Sink> sink_functor(queue2, sink);

  auto t1 = run(__job<Source>::get(source, source_functor), "source");
  auto t2 = run(__job<Pipe>::get(pipe, pipe_functor), "pipe");
  auto t3 = run(__job<Sink>::get(sink, sink_functor), "sink");

  t1.wait();
  t2.wait();
  t3.wait();

  check_app_exit_code();
}

//! convenience functions to set up and run a 4-stage multi-threaded pipeline.
/*! This function extends the 2-stage Thread::run_queue() function to allow
 * a 3-stage pipeline.  */
template <class Source, class Item1, class Pipe1, class Item2, class Pipe2, class Item3, class Sink>
inline void run_queue(Source &&source,
                      const Item1 &item1,
                      Pipe1 &&pipe1,
                      const Item2 &item2,
                      Pipe2 &&pipe2,
                      const Item3 &item3,
                      Sink &&sink,
                      size_t capacity = MRTRIX_QUEUE_DEFAULT_CAPACITY) {
  if (threads_to_execute() == 0) {
    typename Type<Item1>::item item1;
    typename Type<Item2>::item item2;
    typename Type<Item3>::item item3;
    while (__job<Source>::functor(source)(item1)) {
      if (__job<Pipe1>::functor(pipe1)(item1, item2))
        if (__job<Pipe2>::functor(pipe2)(item2, item3))
          if (!__job<Sink>::functor(sink)(item3))
            return;
    }
    return;
  }

  typename Type<Item1>::queue queue1("source->pipe", capacity);
  typename Type<Item2>::queue queue2("pipe->pipe", capacity);
  typename Type<Item3>::queue queue3("pipe->sink", capacity);

  __Source<Item1, Source> source_functor(queue1, source, item1);
  __Pipe<Item1, Pipe1, Item2> pipe1_functor(queue1, pipe1, queue2, item2);
  __Pipe<Item2, Pipe2, Item3> pipe2_functor(queue2, pipe2, queue3, item3);
  __Sink<Item3, Sink> sink_functor(queue3, sink);

  auto t1 = run(__job<Source>::get(source, source_functor), "source");
  auto t2 = run(__job<Pipe1>::get(pipe1, pipe1_functor), "pipe1");
  auto t3 = run(__job<Pipe2>::get(pipe2, pipe2_functor), "pipe2");
  auto t4 = run(__job<Sink>::get(sink, sink_functor), "sink");

  t1.wait();
  t2.wait();
  t3.wait();
  t4.wait();

  check_app_exit_code();
}

/** @} */
/** @} */
} // namespace MR::Thread
