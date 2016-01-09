#include <type_traits>
#include <cassert>

template <typename T> class deep_ptr;
template <typename T, typename ...Ts> deep_ptr<T> make_deep_ptr(Ts&& ...ts);
template <typename T, typename U> deep_ptr<T> deep_ptr_cast(U* u);

struct deleter {
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
  virtual void clone(const void *ptr, void** output) const = 0;
  virtual std::unique_ptr<cloner> clone_self() const = 0;
};

template <typename U> struct default_cloner : cloner {
  void clone(const void *ptr, void** output) const override {
    *output = new U(*static_cast<const U *>(ptr));
  }
  std::unique_ptr<cloner> clone_self() const override {
    return std::make_unique<default_cloner<U>>(*this);
  }
};

template <typename T> class deep_ptr {

  template <typename U> friend class deep_ptr;

  // make_deep_ptr friendship is more permissive than we would ideally like but
  // we cannot restrict make_deep_ptr by partial specialization.
  //
  // C++ Standard(§14.5.4/8): Friend declarations shall not declare partial
  // specializations.
  //
  template <typename T_, typename... Ts>
  friend deep_ptr<T_> make_deep_ptr(Ts &&... ts);

  template <typename T_, typename U>
  friend deep_ptr<T_> deep_ptr_cast(U* u);

  T* ptr_ = nullptr;
  std::unique_ptr<deleter> deleter_;
  std::unique_ptr<cloner> cloner_;

  template <typename U,
            typename V = std::enable_if_t<std::is_same<T, U>::value ||
                                          std::is_base_of<T, U>::value>>
  void do_copy_construct(const deep_ptr<U> &u) {
    if (!u.ptr_) {
      ptr_ = nullptr;
      return;  
    }

    deleter_ = u.deleter_->clone_self();
    cloner_ = u.cloner_->clone_self();
    
    assert(u.cloner_);
    u.cloner_->clone(u.ptr_, &ptr_);
  }

  template <typename U,
            typename V = std::enable_if_t<std::is_same<T, U>::value ||
                                          std::is_base_of<T, U>::value>>
  deep_ptr &do_assign(const deep_ptr<U> &u) {
    // protect against self-assignment
    //if (static_cast<const T*>(&u.ptr_) == static_cast<const T*>(ptr_)) {
    //  return *this;
    //}

    do_copy_construct(u);
    
    return *this;
  }

  template <typename U,
            typename V = std::enable_if_t<std::is_same<T, U>::value ||
                                          std::is_base_of<T, U>::value>>
  void do_move_construct(deep_ptr<U> &&u) {
    deleter_ = std::move(u.deleter_);
    cloner_ = std::move(u.cloner_);
    ptr_ = u.ptr_;
    u.ptr_ = nullptr;
  }

  template <typename U,
            typename V = std::enable_if_t<std::is_same<T, U>::value ||
                                          std::is_base_of<T, U>::value>>
  deep_ptr &do_move_assign(deep_ptr<U> &&u) {
    do_move_construct(std::move(u));
    return *this;
  }

  template <typename U,
            typename V = std::enable_if_t<std::is_same<T, U>::value ||
                                          std::is_base_of<T, U>::value>>
  deep_ptr(U *u) {
    if (!u) {
      return;
    }

    deleter_ = std::make_unique<default_deleter<U>>();
    cloner_ = std::make_unique<default_cloner<U>>();
    ptr_ = u;
  }

public:
  ~deep_ptr() {
    if (!ptr_) { return; }
    assert(deleter_);
    deleter_->do_delete(ptr_);
  }

  //
  // Constructors
  //

  deep_ptr() {}

  deep_ptr(std::nullptr_t) : deep_ptr() {}

  //
  // Copy-constructors
  //

  deep_ptr(const deep_ptr &p) { do_copy_construct(p); }

  template <typename U,
            typename V = std::enable_if_t<!std::is_same<T, U>::value &&
                                          std::is_base_of<T, U>::value>>
  deep_ptr(const deep_ptr<U> &u) {
    do_copy_construct(u);
  }

  //
  // Move-constructors
  //

  deep_ptr(deep_ptr &&p) { do_move_construct(std::move(p)); }

  template <typename U,
            typename V = std::enable_if_t<!std::is_same<T, U>::value &&
                                          std::is_base_of<T, U>::value>>
  deep_ptr(deep_ptr<U> &&u) {
    do_move_construct(std::move(u));
  }

  //
  // Assignment
  //

  deep_ptr &operator=(const deep_ptr &p) { return do_assign(p); }

  template <typename U,
            typename V = std::enable_if_t<!std::is_same<T, U>::value &&
                                          std::is_base_of<T, U>::value>>
  deep_ptr &operator=(const deep_ptr<U> &u) {
    return do_assign(u);
  }

  //
  // Move-assignment
  //

  deep_ptr &operator=(deep_ptr &&p) { return do_move_assign(std::move(p)); }

  template <typename U,
            typename V = std::enable_if_t<!std::is_same<T, U>::value &&
                                          std::is_base_of<T, U>::value>>
  deep_ptr &operator=(deep_ptr<U> &&u) {
    return do_move_assign(std::move(u));
  }

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

template <typename T, typename U>
deep_ptr<T> deep_ptr_cast(U* u)
{
  return deep_ptr<T>(u);
}

