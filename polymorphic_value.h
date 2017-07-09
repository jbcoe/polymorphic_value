/*

Copyright (c) 2016-2017 Jonathan B. Coe

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#ifndef JBCOE_POLYMORPHIC_VALUE_H_INCLUDED
#define JBCOE_POLYMORPHIC_VALUE_H_INCLUDED

#include <cassert>
#include <memory>
#include <type_traits>

namespace jbcoe
{

  namespace detail
  {

    ////////////////////////////////////////////////////////////////////////////
    // Implementation detail classes
    ////////////////////////////////////////////////////////////////////////////

    template <class T>
    struct default_copy
    {
      T* operator()(const T& t) const
      {
        return new T(t);
      }
    };

    template <class T>
    struct default_delete
    {
      void operator()(const T* t) const
      {
        delete t;
      }
    };

    template <class T>
    struct control_block
    {
      virtual ~control_block() = default;

      virtual std::unique_ptr<control_block> clone() const = 0;

      virtual T* ptr() = 0;
    };

    template <class T, class U = T>
    class direct_control_block : public control_block<T>
    {
      U u_;

    public:
      template <class... Ts>
      explicit direct_control_block(Ts&&... ts) : u_(U(std::forward<Ts>(ts)...))
      {
      }

      std::unique_ptr<control_block<T>> clone() const override
      {
        return std::make_unique<direct_control_block>(*this);
      }

      T* ptr() override
      {
        return &u_;
      }
    };

    template <class T, class U, class C = default_copy<U>,
              class D = default_delete<U>>
    class pointer_control_block : public control_block<T>, public C
    {
      std::unique_ptr<U, D> p_;

    public:
      explicit pointer_control_block(U* u, C c = C{}, D d = D{})
          : C(std::move(c)), p_(u, std::move(d))
      {
      }

      explicit pointer_control_block(std::unique_ptr<U, D> p, C c = C{})
          : C(std::move(c)), p_(std::move(p))
      {
      }

      std::unique_ptr<control_block<T>> clone() const override
      {
        assert(p_);
        return std::make_unique<pointer_control_block>(
            C::operator()(*p_), static_cast<const C&>(*this), p_.get_deleter());
      }

      T* ptr() override
      {
        return p_.get();
      }
    };

    template <class T, class U>
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


  } // end namespace detail

  template <class T>
  class polymorphic_value;

  template <class T>
  struct is_polymorphic_value : std::false_type
  {
  };

  template <class T>
  struct is_polymorphic_value<polymorphic_value<T>> : std::true_type
  {
  };


  ////////////////////////////////////////////////////////////////////////////////
  // `polymorphic_value` class definition
  ////////////////////////////////////////////////////////////////////////////////

  template <class T>
  class polymorphic_value
  {

    template <class U>
    friend class polymorphic_value;

    template <class T_, class... Ts>
    friend polymorphic_value<T_> make_polymorphic_value(Ts&&... ts);

    T* ptr_ = nullptr;
    std::unique_ptr<detail::control_block<T>> cb_;

  public:
    //
    // Destructor
    //

    ~polymorphic_value() = default;


    //
    // Constructors
    //

    polymorphic_value()
    {
    }

    template <class U, class C = detail::default_copy<U>,
              class D = detail::default_delete<U>,
              class V = std::enable_if_t<std::is_convertible<U*, T*>::value>>
    explicit polymorphic_value(U* u, C copier = C{}, D deleter = D{})
    {
      if (!u)
      {
        return;
      }

      assert(typeid(*u) == typeid(U));
      std::unique_ptr<U, D> p(u, std::move(deleter));

      cb_ = std::make_unique<detail::pointer_control_block<T, U, C, D>>(
          std::move(p), std::move(copier));
      ptr_ = u;
    }


    //
    // Copy-constructors
    //

    polymorphic_value(const polymorphic_value& p)
    {
      if (!p)
      {
        return;
      }
      auto tmp_cb = p.cb_->clone();
      ptr_ = tmp_cb->ptr();
      cb_ = std::move(tmp_cb);
    }

    template <class U,
              class V = std::enable_if_t<!std::is_same<T, U>::value &&
                                         std::is_convertible<U*, T*>::value>>
    polymorphic_value(const polymorphic_value<U>& p)
    {
      polymorphic_value<U> tmp(p);
      ptr_ = tmp.ptr_;
      cb_ = std::make_unique<detail::delegating_control_block<T, U>>(
          std::move(tmp.cb_));
    }

    template <class U,
              class V = std::enable_if_t<std::is_convertible<U*, T*>::value &&
                                         !is_polymorphic_value<U>::value>>
    polymorphic_value(const U& u)
        : cb_(std::make_unique<detail::direct_control_block<T, U>>(u))
    {
      ptr_ = cb_->ptr();
    }


    //
    // Move-constructors
    //

    polymorphic_value(polymorphic_value&& p) noexcept
    {
      ptr_ = p.ptr_;
      cb_ = std::move(p.cb_);
      p.ptr_ = nullptr;
    }

    template <class U,
              class V = std::enable_if_t<!std::is_same<T, U>::value &&
                                         std::is_convertible<U*, T*>::value>>
    polymorphic_value(polymorphic_value<U>&& p)
    {
      ptr_ = p.ptr_;
      cb_ = std::make_unique<detail::delegating_control_block<T, U>>(
          std::move(p.cb_));
      p.ptr_ = nullptr;
    }

    template <class U,
              class V = std::enable_if_t<std::is_convertible<U*, T*>::value &&
                                         !is_polymorphic_value<U>::value>>
    polymorphic_value(U&& u)
        : cb_(std::make_unique<detail::direct_control_block<T, U>>(
              std::move(u)))
    {
      ptr_ = cb_->ptr();
    }


    //
    // Assignment
    //

    polymorphic_value& operator=(const polymorphic_value& p)
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

    template <class U,
              class V = std::enable_if_t<!std::is_same<T, U>::value &&
                                         std::is_convertible<U*, T*>::value>>
    polymorphic_value& operator=(const polymorphic_value<U>& p)
    {
      polymorphic_value<U> tmp(p);
      *this = std::move(tmp);
      return *this;
    }

    template <class U,
              class V = std::enable_if_t<std::is_convertible<U*, T*>::value &&
                                         !is_polymorphic_value<U>::value>>
    polymorphic_value& operator=(const U& u)
    {
      polymorphic_value tmp(u);
      *this = std::move(tmp);
      return *this;
    }


    //
    // Move-assignment
    //

    polymorphic_value& operator=(polymorphic_value&& p) noexcept
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

    template <class U,
              class V = std::enable_if_t<!std::is_same<T, U>::value &&
                                         std::is_convertible<U*, T*>::value>>
    polymorphic_value& operator=(polymorphic_value<U>&& p)
    {
      cb_ = std::make_unique<detail::delegating_control_block<T, U>>(
          std::move(p.cb_));
      ptr_ = p.ptr_;
      p.ptr_ = nullptr;
      return *this;
    }

    template <class U,
              class V = std::enable_if_t<std::is_convertible<U*, T*>::value &&
                                         !is_polymorphic_value<U>::value>>
    polymorphic_value& operator=(U&& u)
    {
      polymorphic_value tmp(std::move(u));
      *this = std::move(tmp);
      return *this;
    }


    //
    // Modifiers
    //

    void swap(polymorphic_value& p) noexcept
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
      assert(*this);
      return *ptr_;
    }

    const T& operator*() const
    {
      assert(*this);
      return *ptr_;
    }

    T* operator->()
    {
      assert(*this);
      return ptr_;
    }

    T& value()
    {
      assert(*this);
      return *ptr_;
    }

    T& operator*()
    {
      assert(*this);
      return *ptr_;
    }
  };

  //
  // polymorphic_value creation
  //
  template <class T, class... Ts>
  polymorphic_value<T> make_polymorphic_value(Ts&&... ts)
  {
    polymorphic_value<T> p;
    p.cb_ = std::make_unique<detail::direct_control_block<T>>(
        std::forward<Ts>(ts)...);
    p.ptr_ = p.cb_->ptr();
    return std::move(p);
  }

  //
  // non-member swap
  //
  template <class T>
  void swap(polymorphic_value<T>& t, polymorphic_value<T>& u) noexcept
  {
    t.swap(u);
  }

} // end namespace jbcoe

#endif
