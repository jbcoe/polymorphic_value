#include <type_traits>
#include <cassert>

template <typename T> struct default_copier {
  T *operator()(const T &t) const { return new T(t); }
};

template <typename T> struct default_deleter {
  void operator()(const T *t) const { delete t; }
};

template <typename T> struct control_block {
  virtual ~control_block() = default;
  virtual std::unique_ptr<control_block> clone() const = 0;
  virtual T *release() = 0;
  virtual T *ptr() = 0;
};

template <typename T, typename U, typename C = default_copier<U>,
          typename D = default_deleter<U>>
class control_block_impl : public control_block<T> {
  std::unique_ptr<U, D> p_;
  C c_;

public:
  explicit control_block_impl(U *u, C c = C{}, D d = D{})
      : c_(std::move(c)), p_(u, std::move(d)) {}

  std::unique_ptr<control_block<T>> clone() const override {
    assert(p_);
    return std::make_unique<control_block_impl>(c_(*p_), c_, p_.get_deleter());
  }

  T *release() override { return p_.release(); }

  T *ptr() override { return p_.get(); }
};

template <typename T, typename U>
class delegating_control_block : public control_block<T> {

  std::unique_ptr<control_block<U>> delegate_;

public:
  explicit delegating_control_block(std::unique_ptr<control_block<U>> b)
      : delegate_(std::move(b)) {}

  std::unique_ptr<control_block<T>> clone() const override {
    return std::make_unique<delegating_control_block>(delegate_->clone());
  }

  T *release() override { return delegate_->release(); }

  T *ptr() override { return delegate_->ptr(); }
};

template <typename T, typename U>
class downcasting_delegating_control_block : public control_block<T> {

  std::unique_ptr<control_block<U>> delegate_;

public:
  explicit downcasting_delegating_control_block(
      std::unique_ptr<control_block<U>> b)
      : delegate_(std::move(b)) {}

  std::unique_ptr<control_block<T>> clone() const override {
    return std::make_unique<downcasting_delegating_control_block>(
        delegate_->clone());
  }

  T *release() override { return static_cast<T *>(delegate_->release()); }

  T *ptr() override { return static_cast<T *>(delegate_->ptr()); }
};

template <typename T, typename U>
class dynamic_casting_delegating_control_block : public control_block<T> {

  std::unique_ptr<control_block<U>> delegate_;
  T *p_; // cache the pointer as dynamic_cast is slow

public:
  explicit dynamic_casting_delegating_control_block(
      std::unique_ptr<control_block<U>> b)
      : delegate_(std::move(b)) {
    p_ = dynamic_cast<T *>(delegate_->ptr());
  }

  std::unique_ptr<control_block<T>> clone() const override {
    return std::make_unique<dynamic_casting_delegating_control_block>(
        delegate_->clone());
  }

  T *release() override {
    delegate_->release();
    return p_;
  }

  T *ptr() override { return p_; }
};

template <typename T, typename U>
class const_casting_delegating_control_block : public control_block<T> {

  std::unique_ptr<control_block<U>> delegate_;

public:
  explicit const_casting_delegating_control_block(
      std::unique_ptr<control_block<U>> b)
      : delegate_(std::move(b)) {}

  std::unique_ptr<control_block<T>> clone() const override {
    return std::make_unique<const_casting_delegating_control_block>(
        delegate_->clone());
  }

  T *release() override { return const_cast<T *>(delegate_->release()); }

  T *ptr() override { return const_cast<T *>(delegate_->ptr()); }
};

template <typename T> class cloned_ptr {

  template <typename U> friend class cloned_ptr;
  template <typename T_, typename U>
  friend cloned_ptr<T_> std::const_pointer_cast(const cloned_ptr<U> &p);
  template <typename T_, typename U>
  friend cloned_ptr<T_> std::dynamic_pointer_cast(const cloned_ptr<U> &p);
  template <typename T_, typename U>
  friend cloned_ptr<T_> std::static_pointer_cast(const cloned_ptr<U> &p);

  T *ptr_ = nullptr;
  std::unique_ptr<control_block<T>> cb_;

public:
  ~cloned_ptr() = default;

  //
  // Constructors
  //

  cloned_ptr() {}

  cloned_ptr(std::nullptr_t) : cloned_ptr() {}

  template <typename U, typename C=default_copier<U>, typename D=default_deleter<U>,
            typename V = std::enable_if_t<std::is_base_of<T, U>::value>>
  explicit cloned_ptr(U *u, C copier=C{}, D deleter=D{}) {
    if (!u) {
      return;
    }

    cb_ = std::make_unique<control_block_impl<T, U, C, D>>(u, std::move(copier),
                                                           std::move(deleter));
    ptr_ = u;
  }

  //
  // Copy-constructors
  //

  cloned_ptr(const cloned_ptr &p) {
    if (!p) {
      return;
    }
    auto tmp_cb = p.cb_->clone();
    ptr_ = tmp_cb->ptr();
    cb_ = std::move(tmp_cb);
  }

  template <typename U,
            typename V = std::enable_if_t<!std::is_same<T, U>::value &&
                                          std::is_base_of<T, U>::value>>
  cloned_ptr(const cloned_ptr<U> &p) {
    cloned_ptr<U> tmp(p);
    ptr_ = tmp.ptr_;
    cb_ = std::make_unique<delegating_control_block<T, U>>(std::move(tmp.cb_));
  }

  //
  // Move-constructors
  //

  cloned_ptr(cloned_ptr &&p) {
    ptr_ = p.ptr_;
    cb_ = std::move(p.cb_);
    p.ptr_ = nullptr;
  }

  template <typename U,
            typename V = std::enable_if_t<!std::is_same<T, U>::value &&
                                          std::is_base_of<T, U>::value>>
  cloned_ptr(cloned_ptr<U> &&p) {
    ptr_ = p.ptr_;
    cb_ = std::make_unique<delegating_control_block<T, U>>(std::move(p.cb_));
    p.ptr_ = nullptr;
  }

  //
  // Assignment
  //

  cloned_ptr &operator=(const cloned_ptr &p) {
    if (&p == this) {
      return *this;
    }

    if (!p) {
      cb_.reset();
      ptr_ = nullptr;
      return *this;
    }

    auto tmp_cb = p.cb_->clone();
    ptr_ = tmp_cb->ptr();
    cb_ = std::move(tmp_cb);
    return *this;
  }

  template <typename U,
            typename V = std::enable_if_t<!std::is_same<T, U>::value &&
                                          std::is_base_of<T, U>::value>>
  cloned_ptr &operator=(const cloned_ptr<U> &p) {
    cloned_ptr<U> tmp(p);
    *this = std::move(tmp);
    return *this;
  }

  //
  // Move-assignment
  //

  cloned_ptr &operator=(cloned_ptr &&p) {
    if (&p == this) {
      return *this;
    }

    cb_ = std::move(p.cb_);
    ptr_ = p.ptr_;
    p.ptr_ = nullptr;
    return *this;
  }

  template <typename U,
            typename V = std::enable_if_t<!std::is_same<T, U>::value &&
                                          std::is_base_of<T, U>::value>>
  cloned_ptr &operator=(cloned_ptr<U> &&p) {
    cb_ = std::make_unique<delegating_control_block<T, U>>(std::move(p.cb_));
    ptr_ = p.ptr_;
    p.ptr_ = nullptr;
    return *this;
  }

  //
  // Modifiers
  //

  T *release() {
    if (!ptr_) {
      return nullptr;
    }
    ptr_ = nullptr;
    return cb_->release();
  }

  template <typename U = T,
            typename V = std::enable_if_t<std::is_base_of<T, U>::value>>
  void reset(U *u = nullptr) {
    if (static_cast<T *>(u) == ptr_) {
      return;
    }
    cloned_ptr<U> tmp(u);
    *this = std::move(tmp);
  }

  void swap(cloned_ptr &p) {
    using std::swap;
    swap(ptr_, p.ptr_);
    swap(cb_, p.cb_);
  }

  //
  // Accessors
  //

  operator bool() const { return ptr_; }

  T *operator->() const { return get(); }

  T *get() const { return ptr_; }

  T &operator*() const {
    assert(ptr_);
    return *get();
  }
};

//
// cloned_ptr creation
//
template <typename T, typename... Ts>
cloned_ptr<T> make_cloned_ptr(Ts &&... ts) {
  return cloned_ptr<T>(new T(std::forward<Ts>(ts)...));
}

//
// Casts
//

namespace std {

template <typename T, typename U>
cloned_ptr<T> static_pointer_cast(const cloned_ptr<U> &p) {
  cloned_ptr<U> tmp(p);
  cloned_ptr<T> t;

  t.ptr_ = static_cast<T *>(tmp.ptr_);
  t.cb_ = std::make_unique<downcasting_delegating_control_block<T, U>>(
      std::move(tmp.cb_));

  return t;
}

template <typename T, typename U>
cloned_ptr<T> dynamic_pointer_cast(const cloned_ptr<U> &p) {
  if (!dynamic_cast<T *>(p.get())) {
    return nullptr;
  }

  cloned_ptr<U> tmp(p);
  cloned_ptr<T> t;

  t.ptr_ = dynamic_cast<T *>(tmp.ptr_);
  t.cb_ = std::make_unique<dynamic_casting_delegating_control_block<T, U>>(
      std::move(tmp.cb_));

  return t;
}

template <typename T, typename U>
cloned_ptr<T> const_pointer_cast(const cloned_ptr<U> &p) {
  cloned_ptr<U> tmp(p);
  cloned_ptr<T> t;

  t.ptr_ = const_cast<T *>(tmp.ptr_);
  t.cb_ = std::make_unique<const_casting_delegating_control_block<T, U>>(
      std::move(tmp.cb_));

  return t;
}

} // end namespace std

