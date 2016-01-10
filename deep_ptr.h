#include <type_traits>
#include <cassert>

struct deleter {
  virtual ~deleter() = default;
  virtual void do_delete(const void *ptr) const = 0;
  virtual std::unique_ptr<deleter> clone_self() const = 0;
};

template <typename U> struct default_deleter : deleter {
  void do_delete(const void *ptr) const override {
    delete static_cast<const U *>(ptr);
  }
  std::unique_ptr<deleter> clone_self() const override {
    return std::make_unique<default_deleter<U>>(*this);
  }
};

struct cloner {
  virtual ~cloner() = default;
  virtual void* clone(const void *ptr) const = 0;
  virtual std::unique_ptr<cloner> clone_self() const = 0;
};

template <typename U> struct default_cloner : cloner {
  void* clone(const void *ptr) const override {
    return new U(*static_cast<const U*>(ptr));
  }
  std::unique_ptr<cloner> clone_self() const override {
    return std::make_unique<default_cloner<U>>(*this);
  }
};

template <typename T> class deep_ptr {

  template <typename U> friend class deep_ptr;
  
  T* ptr_ = nullptr;
  std::unique_ptr<deleter> deleter_;
  std::unique_ptr<cloner> cloner_;

  template <typename U,
            typename V = std::enable_if_t<std::is_same<T, U>::value ||
                                          std::is_base_of<T, U>::value>>
  void do_copy_construct(const deep_ptr<U> &p) {
    if (!p.ptr_) {
      ptr_ = nullptr;
      return;
    }

    deleter_ = p.deleter_->clone_self();
    cloner_ = p.cloner_->clone_self();

    assert(p.cloner_);
    ptr_ = static_cast<T*>(p.cloner_->clone(p.ptr_));
  }

  template <typename U,
            typename V = std::enable_if_t<std::is_same<T, U>::value ||
                                          std::is_base_of<T, U>::value>>
  deep_ptr &do_assign(const deep_ptr<U> &p) {

    if (ptr_) {
      assert(deleter_);
      deleter_->do_delete(ptr_);
    }

    do_copy_construct(p);

    return *this;
  }

  template <typename U,
            typename V = std::enable_if_t<std::is_same<T, U>::value ||
                                          std::is_base_of<T, U>::value>>
  void do_move_construct(deep_ptr<U> &&p) {
    if (ptr_) {
      assert(deleter_);
      deleter_->do_delete(ptr_);
    }

    deleter_ = std::move(p.deleter_);
    cloner_ = std::move(p.cloner_);
    ptr_ = p.ptr_;

    p.ptr_ = nullptr;
  }

  template <typename U,
            typename V = std::enable_if_t<std::is_same<T, U>::value ||
                                          std::is_base_of<T, U>::value>>
  deep_ptr &do_move_assign(deep_ptr<U> &&u) {
    do_move_construct(std::move(u));
    return *this;
  }

public:
  ~deep_ptr() {
    if (!ptr_) {
      return;
    }
    assert(deleter_);
    deleter_->do_delete(ptr_);
  }

  //
  // Constructors
  //

  deep_ptr() {}

  deep_ptr(std::nullptr_t) : deep_ptr() {}

  template <typename U,
            typename V = std::enable_if_t<std::is_same<T, U>::value ||
                                          std::is_base_of<T, U>::value>>
  explicit deep_ptr(U *u) {
    if (!u) {
      return;
    }

    deleter_ = std::make_unique<default_deleter<U>>();
    cloner_ = std::make_unique<default_cloner<U>>();
    ptr_ = u;
  }
  
  //
  // Copy-constructors
  //

  deep_ptr(const deep_ptr &p) {
    do_copy_construct(p);
  }

  template <typename U,
            typename V = std::enable_if_t<!std::is_same<T, U>::value &&
                                          std::is_base_of<T, U>::value>>
  deep_ptr(const deep_ptr<U> &p) {
    do_copy_construct(p);
  }
  
  //
  // Move-constructors
  //

  deep_ptr(deep_ptr &&p) { do_move_construct(std::move(p)); }

  template <typename U,
            typename V = std::enable_if_t<!std::is_same<T, U>::value &&
                                          std::is_base_of<T, U>::value>>
  deep_ptr(deep_ptr<U> &&p) { do_move_construct(std::move(p)); }

  //
  // Assignment
  //

  deep_ptr &operator=(const deep_ptr &p) {
    if (&p == this) {
      return *this;
    }
    return do_assign(p);
  }

  template <typename U,
            typename V = std::enable_if_t<!std::is_same<T, U>::value &&
                                          std::is_base_of<T, U>::value>>
  deep_ptr &operator=(const deep_ptr<U> &p) {
    return do_assign(p);
  }
  
  //
  // Move-assignment
  //

  deep_ptr &operator=(deep_ptr &&p) { return do_move_assign(std::move(p)); }

  template <typename U,
            typename V = std::enable_if_t<!std::is_same<T, U>::value &&
                                          std::is_base_of<T, U>::value>>
  deep_ptr &operator=(deep_ptr<U> &&p) { return do_move_assign(std::move(p)); }

  //
  // Accessors
  //

  const operator bool() const { return ptr_; }

  const T *operator->() const { return get(); }

  const T *get() const {
    return ptr_;
  }

  const T &operator*() const {
    assert(ptr_);
    return *get();
  }

  //
  // Non-const accessors
  //

  T *operator->() { return get(); }

  T *get() {
    return ptr_;
  }

  T &operator*() {
    assert(ptr_);
    return *get();
  }
};

template <typename T, typename ...Ts>
deep_ptr<T> make_deep_ptr(Ts&& ...ts)
{
  return deep_ptr<T>(new T(std::forward<Ts>(ts)...));
}

