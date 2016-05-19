#include <cassert>
#include <memory>
#include <type_traits>

template <typename T>
struct default_copier
{
  T* operator()(const T& t) const
  {
    return new T(t);
  }

  T* operator()(T&& t) const
  {
    return new T(std::move(t));
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
  virtual std::unique_ptr<control_block> move() = 0;
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

  std::unique_ptr<control_block<T>> move() override
  {
    assert(p_);
    return std::make_unique<control_block_impl>(c_(std::move(*p_)), c_,
                                                p_.get_deleter());
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
    return std::make_unique<direct_control_block_impl>(u_);
  }

  std::unique_ptr<control_block<U>> move() override
  {
    return std::make_unique<direct_control_block_impl>(std::move(u_));
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

  std::unique_ptr<control_block<T>> move() override
  {
    return std::make_unique<delegating_control_block>(delegate_->move());
  }

  T* ptr() override
  {
    return delegate_->ptr();
  }
};

template <typename T>
class indirect
{

  template <typename U>
  friend class indirect;

  template <typename T_, typename... Ts>
  friend indirect<T_> make_indirect(Ts&&... ts);

  T* ptr_ = nullptr;
  std::unique_ptr<control_block<T>> cb_;

public:
  //
  // Constructors
  //

  template <typename T_ = T,
            typename = std::enable_if_t<!std::is_abstract<T_>::value>>
  indirect() : cb_(std::make_unique<control_block_impl<T, T>>())
  {
    ptr_ = cb_->ptr();
  }

  template <typename U, typename C = default_copier<U>,
            typename D = default_deleter<U>,
            typename V = std::enable_if_t<std::is_convertible<U*, T*>::value>>
  explicit indirect(U* u, C copier = C{}, D deleter = D{})
  {
    assert(u);
    assert(typeid(*u) == typeid(U));

    cb_ = std::make_unique<control_block_impl<T, U, C, D>>(u, std::move(copier),
                                                           std::move(deleter));
    ptr_ = u;
  }

  //
  // Copy-constructors
  //

  indirect(const indirect& p)
  {
    cb_ = p.cb_->clone();
    ptr_ = cb_->ptr();
  }

  template <typename U,
            typename V = std::enable_if_t<!std::is_same<T, U>::value &&
                                          std::is_convertible<U*, T*>::value>>
  indirect(const indirect<U>& p)
  {
    cb_ = std::make_unique<delegating_control_block<T, U>>(p.cb_->clone());
    ptr_ = cb_->ptr();
  }

  //
  // Move-constructors
  //

  indirect(indirect&& p)
  {
    cb_ = p.cb_->move();
    ptr_ = cb_->ptr();
  }

  template <typename U,
            typename V = std::enable_if_t<!std::is_same<T, U>::value &&
                                          std::is_convertible<U*, T*>::value>>
  indirect(indirect<U>&& p)
  {
    cb_ = std::make_unique<delegating_control_block<T, U>>(p.cb_->move());
    ptr_ = cb_->ptr();
  }

  //
  // Assignment
  //

  indirect& operator=(const indirect& p)
  {
    if (&p == this)
    {
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
  indirect& operator=(const indirect<U>& p)
  {
    indirect<U> tmp(p);
    *this = std::move(tmp);
    return *this;
  }

  //
  // Move-assignment
  //

  indirect& operator=(indirect&& p)
  {
    cb_ = p.cb_->move();
    ptr_ = cb_->ptr();
    return *this;
  }

  template <typename U,
            typename V = std::enable_if_t<!std::is_same<T, U>::value &&
                                          std::is_convertible<U*, T*>::value>>
  indirect& operator=(indirect<U>&& p)
  {
    cb_ = std::make_unique<delegating_control_block<T, U>>(p.cb_->move());
    ptr_ = cb_.ptr();
    return *this;
  }

  void swap(indirect& p)
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

  const T* operator->() const
  {
    return ptr_;
  }

  const T& value() const
  {
    return *ptr_;
  }

  const T& operator*() const
  {
    return *ptr_;
  }

  T* operator->()
  {
    return ptr_;
  }

  T& value()
  {
    return *ptr_;
  }

  T& operator*()
  {
    return *ptr_;
  }
};

//
// indirect creation
//
template <typename T, typename... Ts>
indirect<T> make_indirect(Ts&&... ts)
{
  indirect<T> p;
  p.cb_ =
      std::make_unique<direct_control_block_impl<T>>(std::forward<Ts>(ts)...);
  p.ptr_ = p.cb_->ptr();
  return std::move(p);
}
