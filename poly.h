#include <type_traits>
#include <cassert>

template <typename T>
struct default_copier
{
  T* operator()(const T& t) const
  {
    return new T(t);
  }
};

template <typename T>
struct default_deleter
{
  void operator()(const T* t) const
  {
    delete t;
  }
};

template <typename T>
struct control_block
{
  virtual ~control_block() = default;
  virtual std::unique_ptr<control_block> clone() const = 0;
  virtual T* ptr() = 0;
};

template <typename T, typename U, typename C = default_copier<U>,
          typename D = default_deleter<U>>
class control_block_impl : public control_block<T>
{
  std::unique_ptr<U, D> p_;
  C c_;

public:
  explicit control_block_impl(U* u, C c = C{}, D d = D{})
      : c_(std::move(c)), p_(u, std::move(d))
  {
  }

  std::unique_ptr<control_block<T>> clone() const override
  {
    assert(p_);
    return std::make_unique<control_block_impl>(c_(*p_), c_, p_.get_deleter());
  }

  T* ptr() override
  {
    return p_.get();
  }
};

template <typename T, typename U = T>
class direct_control_block_impl : public control_block<U>
{
  U u_;

public:
  template <typename... Ts>
  explicit direct_control_block_impl(Ts&&... ts)
      : u_(U(std::forward<Ts>(ts)...))
  {
  }

  std::unique_ptr<control_block<U>> clone() const override
  {
    return std::make_unique<direct_control_block_impl>(*this);
  }

  T* ptr() override
  {
    return &u_;
  }
};

template <typename T, typename U>
class delegating_control_block : public control_block<T>
{

  std::unique_ptr<control_block<U>> delegate_;

public:
  explicit delegating_control_block(std::unique_ptr<control_block<U>> b)
      : delegate_(std::move(b))
  {
  }

  std::unique_ptr<control_block<T>> clone() const override
  {
    return std::make_unique<delegating_control_block>(delegate_->clone());
  }

  T* ptr() override
  {
    return delegate_->ptr();
  }
};

template <typename T>
class poly
{

  template <typename U>
  friend class poly;
  template <typename T_, typename... Ts>
  friend poly<T_> make_poly(Ts&&... ts);

  T* ptr_ = nullptr;
  std::unique_ptr<control_block<T>> cb_;

  void init(std::unique_ptr<control_block<T>> p)
  {
    cb_ = std::move(p);
    ptr_ = cb_->ptr();
  }

public:
  //
  // Constructors
  //

  poly()
  {
  }

  template <typename U, typename C = default_copier<U>,
            typename D = default_deleter<U>,
            typename V = std::enable_if_t<std::is_convertible<U*, T*>::value>>
  explicit poly(U* u, C copier = C{}, D deleter = D{})
  {
    if (!u)
    {
      return;
    }

    cb_ = std::make_unique<control_block_impl<T, U, C, D>>(u, std::move(copier),
                                                           std::move(deleter));
    ptr_ = u;
  }

  //
  // Copy-constructors
  //

  poly(const poly& p)
  {
    if (!p)
    {
      return;
    }
    auto tmp_cb = p.cb_->clone();
    ptr_ = tmp_cb->ptr();
    cb_ = std::move(tmp_cb);
  }

  template <typename U,
            typename V = std::enable_if_t<!std::is_same<T, U>::value &&
                                          std::is_convertible<U*, T*>::value>>
  poly(const poly<U>& p)
  {
    poly<U> tmp(p);
    ptr_ = tmp.ptr_;
    cb_ = std::make_unique<delegating_control_block<T, U>>(std::move(tmp.cb_));
  }

  //
  // Move-constructors
  //

  poly(poly&& p)
  {
    ptr_ = p.ptr_;
    cb_ = std::move(p.cb_);
    p.ptr_ = nullptr;
  }

  template <typename U,
            typename V = std::enable_if_t<!std::is_same<T, U>::value &&
                                          std::is_convertible<U*, T*>::value>>
  poly(poly<U>&& p)
  {
    ptr_ = p.ptr_;
    cb_ = std::make_unique<delegating_control_block<T, U>>(std::move(p.cb_));
    p.ptr_ = nullptr;
  }

  //
  // Assignment
  //

  poly& operator=(const poly& p)
  {
    if (&p == this)
    {
      return *this;
    }

    if (!p)
    {
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
                                          std::is_convertible<U*, T*>::value>>
  poly& operator=(const poly<U>& p)
  {
    poly<U> tmp(p);
    *this = std::move(tmp);
    return *this;
  }

  //
  // Move-assignment
  //

  poly& operator=(poly&& p)
  {
    if (&p == this)
    {
      return *this;
    }

    cb_ = std::move(p.cb_);
    ptr_ = p.ptr_;
    p.ptr_ = nullptr;
    return *this;
  }

  template <typename U,
            typename V = std::enable_if_t<!std::is_same<T, U>::value &&
                                          std::is_convertible<U*, T*>::value>>
  poly& operator=(poly<U>&& p)
  {
    cb_ = std::make_unique<delegating_control_block<T, U>>(std::move(p.cb_));
    ptr_ = p.ptr_;
    p.ptr_ = nullptr;
    return *this;
  }

  void swap(poly& p)
  {
    using std::swap;
    swap(ptr_, p.ptr_);
    swap(cb_, p.cb_);
  }

  //
  // Modifiers
  //

  template <typename U, typename... Ts>
  void emplace(Ts&&... ts)
  {
    auto p = std::make_unique<direct_control_block_impl<T, U>>(
        std::forward<Ts>(ts)...);
    ptr_ = p->ptr();
    cb_ = std::move(p);
  }

  //
  // Accessors
  //

  bool empty() const
  {
    return ptr_ == nullptr;
  }
  explicit operator bool() const
  {
    return ptr_ != nullptr;
  }

  const T* operator->() const
  {
    return ptr_;
  }

  const T& value() const &
  {
    return *ptr_;
  }
  const T&& value() const &&
  {
    return *ptr_;
  }

  const T& operator*() const &
  {
    return *ptr_;
  }
  const T&& operator*() const &&
  {
    return *ptr_;
  }

  T* operator->()
  {
    return ptr_;
  }

  T& value() &
  {
    return *ptr_;
  }
  T&& value() &&
  {
    return *ptr_;
  }

  T& operator*() &
  {
    return *ptr_;
  }
  T&& operator*() &&
  {
    return *ptr_;
  }
};

//
// poly creation
//
template <typename T, typename... Ts>
poly<T> make_poly(Ts&&... ts)
{
  poly<T> p;
  p.cb_ =
      std::make_unique<direct_control_block_impl<T>>(std::forward<Ts>(ts)...);
  p.ptr_ = p.cb_->ptr();
  return std::move(p);
}
