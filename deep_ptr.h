#include <type_traits>
#include <cassert>

template <typename T> class deep_ptr;
template <typename T, typename ...Ts> deep_ptr<T> make_deep_ptr(Ts&& ...ts);

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

  struct inner {
    virtual void copy(void *buffer) const = 0;

    virtual const T *get() const = 0;

    virtual T *get() = 0;

    virtual ~inner() = default;
  };

  template <typename U> struct inner_impl : inner {
    inner_impl(const inner_impl &) = delete;
    inner_impl &operator=(const inner_impl &) = delete;

    inner_impl(U *u) : u_(u) {}

    void copy(void *buffer) const override {
      new (buffer) inner_impl(new U(*u_));
    }

    const T *get() const override { return u_; }

    T *get() override { return u_; }

    ~inner_impl() { delete u_; }

    U *u_;
  };

  std::aligned_storage_t<sizeof(inner_impl<void>), alignof(inner_impl<void>)>
      buffer_;
  bool engaged_ = false;

  template <typename U,
            typename V = std::enable_if_t<std::is_same<T, U>::value ||
                                          std::is_base_of<T, U>::value>>
  void do_copy_construct(const deep_ptr<U> &u) {
    if (!u.engaged_)
      return;
    reinterpret_cast<const typename deep_ptr<U>::inner *>(&u.buffer_)
        ->copy(&buffer_);
    engaged_ = true;
  }

  template <typename U,
            typename V = std::enable_if_t<std::is_same<T, U>::value ||
                                          std::is_base_of<T, U>::value>>
  void do_move_construct(deep_ptr<U> &&u) {
    if (u.engaged_) {
      buffer_ = u.buffer_;
    }

    engaged_ = u.engaged_;
    u.engaged_ = false;
  }

  template <typename U,
            typename V = std::enable_if_t<std::is_same<T, U>::value ||
                                          std::is_base_of<T, U>::value>>
  deep_ptr &do_assign(const deep_ptr<U> &u) {
    if (reinterpret_cast<const void *>(&u) ==
        reinterpret_cast<const void *>(this))
      return *this;

    if (engaged_) {
      reinterpret_cast<const inner *>(&buffer_)->~inner();
    }

    if (!u.engaged_) {
      engaged_ = false;
    } else {
      reinterpret_cast<const typename deep_ptr<U>::inner *>(&u.buffer_)
          ->copy(&buffer_);
      engaged_ = true;
    }

    return *this;
  }

  template <typename U,
            typename V = std::enable_if_t<std::is_same<T, U>::value ||
                                          std::is_base_of<T, U>::value>>
  deep_ptr &do_move_assign(deep_ptr<U> &&u) {
    if (engaged_) {
      reinterpret_cast<const inner *>(&buffer_)->~inner();
    }

    engaged_ = u.engaged_;

    if (u.engaged_) {
      buffer_ = u.buffer_;
    }

    u.engaged_ = false;
    return *this;
  }

  template <typename U,
            typename V = std::enable_if_t<std::is_same<T, U>::value ||
                                          std::is_base_of<T, U>::value>>
  deep_ptr(U *u) {
    if (!u) {
      engaged_ = false;
      return;
    }

    new (&buffer_) inner_impl<U>(u);
    engaged_ = true;
  }

public:
  ~deep_ptr() {
    if (engaged_) {
      reinterpret_cast<inner *>(&buffer_)->~inner();
    }
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

  const operator bool() const { return engaged_; }

  const T *operator->() const { return get(); }

  const T *get() const {
    if (!engaged_) {
      return nullptr;
    }
    return reinterpret_cast<const inner *>(&buffer_)->get();
  }

  const T &operator*() const {
    assert(engaged_);
    return *get();
  }

  //
  // Non-const accessors
  //

  T *operator->() { return get(); }

  T *get() {
    if (!engaged_) {
      return nullptr;
    }
    return reinterpret_cast<inner *>(&buffer_)->get();
  }

  T &operator*() {
    assert(engaged_);
    return *get();
  }
};

template <typename T, typename ...Ts>
deep_ptr<T> make_deep_ptr(Ts&& ...ts)
{
  return deep_ptr<T>(new T(std::forward<Ts>(ts)...));
}
