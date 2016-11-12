#pragma once

#include <cassert>
#include <memory>
#include <type_traits>

////////////////////////////////////////////////////////////////////////////////
// Implementation detail classes
////////////////////////////////////////////////////////////////////////////////

template <typename T>
struct default_copy
{
  T* operator()(const T& t) const
  {
    return new T(t);
  }
};

template <typename T>
struct default_delete
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

template <typename T, typename U, typename C = default_copy<U>,
          typename D = default_delete<U>>
class pointer_control_block : public control_block<T>
{
  std::unique_ptr<U, D> p_;
  C c_;

public:
  explicit pointer_control_block(U* u, C c = C{}, D d = D{})
      : c_(std::move(c)), p_(u, std::move(d))
  {
  }

  std::unique_ptr<control_block<T>> clone() const override
  {
    assert(p_);
    return std::make_unique<pointer_control_block>(c_(*p_), c_,
                                                   p_.get_deleter());
  }

  T* ptr() override
  {
    return p_.get();
  }
};

template <typename T, typename U = T>
class direct_control_block : public control_block<U>
{
  U u_;

public:
  template <typename... Ts>
  explicit direct_control_block(Ts&&... ts) : u_(U(std::forward<Ts>(ts)...))
  {
  }

  std::unique_ptr<control_block<U>> clone() const override
  {
    return std::make_unique<direct_control_block>(*this);
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
class indirect;

template <typename T>
struct is_indirect : std::false_type
{
};

template <typename T>
struct is_indirect<indirect<T>> : std::true_type
{
};

////////////////////////////////////////////////////////////////////////////////
// `indirect` class definition
////////////////////////////////////////////////////////////////////////////////

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
  // Destructor
  //

  ~indirect() = default;


  //
  // Constructors
  //

  indirect()
  {
  }

  template <typename U, typename C = default_copy<U>,
            typename D = default_delete<U>,
            typename V = std::enable_if_t<std::is_convertible<U*, T*>::value>>
  explicit indirect(U* u, C copier = C{}, D deleter = D{})
  {
    if (!u)
    {
      return;
    }

    assert(typeid(*u) == typeid(U));

    cb_ = std::make_unique<pointer_control_block<T, U, C, D>>(
        u, std::move(copier), std::move(deleter));
    ptr_ = u;
  }


  //
  // Copy-constructors
  //

  indirect(const indirect& p)
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
  indirect(const indirect<U>& p)
  {
    indirect<U> tmp(p);
    ptr_ = tmp.ptr_;
    cb_ = std::make_unique<delegating_control_block<T, U>>(std::move(tmp.cb_));
  }

  template <typename U,
            typename V = std::enable_if_t<std::is_convertible<U*, T*>::value &&
                                          !is_indirect<U>::value>>
  indirect(const U& u) : indirect(new U(u))
  {
  }


  //
  // Move-constructors
  //

  indirect(indirect&& p) noexcept
  {
    ptr_ = p.ptr_;
    cb_ = std::move(p.cb_);
    p.ptr_ = nullptr;
  }

  template <typename U,
            typename V = std::enable_if_t<!std::is_same<T, U>::value &&
                                          std::is_convertible<U*, T*>::value>>
  indirect(indirect<U>&& p)
  {
    ptr_ = p.ptr_;
    cb_ = std::make_unique<delegating_control_block<T, U>>(std::move(p.cb_));
    p.ptr_ = nullptr;
  }

  template <typename U,
            typename V = std::enable_if_t<std::is_convertible<U*, T*>::value &&
                                          !is_indirect<U>::value>>
  indirect(U&& u) : indirect(new U(std::move(u)))
  {
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
  indirect& operator=(const indirect<U>& p)
  {
    indirect<U> tmp(p);
    *this = std::move(tmp);
    return *this;
  }

  template <typename U,
            typename V = std::enable_if_t<std::is_convertible<U*, T*>::value &&
                                          !is_indirect<U>::value>>
  indirect& operator=(const U& u)
  {
    indirect tmp(u);
    *this = std::move(tmp);
    return *this;
  }


  //
  // Move-assignment
  //

  indirect& operator=(indirect&& p) noexcept
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
  indirect& operator=(indirect<U>&& p)
  {
    cb_ = std::make_unique<delegating_control_block<T, U>>(std::move(p.cb_));
    ptr_ = p.ptr_;
    p.ptr_ = nullptr;
    return *this;
  }

  template <typename U,
            typename V = std::enable_if_t<std::is_convertible<U*, T*>::value &&
                                          !is_indirect<U>::value>>
  indirect& operator=(U&& u)
  {
    indirect tmp(std::move(u));
    *this = std::move(tmp);
    return *this;
  }


  //
  // Modifiers
  //

  void swap(indirect& p) noexcept
  {
    using std::swap;
    swap(ptr_, p.ptr_);
    swap(cb_, p.cb_);
  }


  //
  // Observers
  //

  explicit operator bool() const
  {
    return (bool)cb_;
  }

  const T* operator->() const
  {
    assert(ptr_);
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
  p.cb_ = std::make_unique<direct_control_block<T>>(std::forward<Ts>(ts)...);
  p.ptr_ = p.cb_->ptr();
  return std::move(p);
}

//
// non-member swap
//
template <typename T>
void swap(indirect<T>& t, indirect<T>& u) noexcept
{
  t.swap(u);
}

