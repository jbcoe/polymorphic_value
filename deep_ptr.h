#include <type_traits>
#include <cassert>

template <typename T>
struct deleter {
  virtual ~deleter() = default;
  virtual void do_delete(const T *ptr) const = 0;
  virtual std::unique_ptr<deleter<T>> clone_self() const = 0;
};

template <typename T, typename U> struct default_deleter : deleter<T> {
  void do_delete(const T *ptr) const override {
    delete static_cast<const U *>(ptr);
  }
  std::unique_ptr<deleter<T>> clone_self() const override {
    return std::make_unique<default_deleter<T,U>>(*this);
  }
};

template <typename T>
struct cloner {
  virtual ~cloner() = default;
  virtual T* clone(const T *ptr) const = 0;
  virtual std::unique_ptr<cloner<T>> clone_self() const = 0;
};

template <typename T, typename U> struct default_cloner : cloner<T> {
  T* clone(const T *ptr) const override {
    return new U(*static_cast<const U*>(ptr));
  }
  std::unique_ptr<cloner<T>> clone_self() const override {
    return std::make_unique<default_cloner<T,U>>(*this);
  }
};

template <typename T> class deep_ptr {

  T* ptr_ = nullptr;
  std::unique_ptr<deleter<T>> deleter_;
  std::unique_ptr<cloner<T>> cloner_;

  void do_copy_construct(const deep_ptr &p) {
    if (!p.ptr_) {
      ptr_ = nullptr;
      return;  
    }

    deleter_ = p.deleter_->clone_self();
    cloner_ = p.cloner_->clone_self();
    
    assert(p.cloner_);
    ptr_ = p.cloner_->clone(p.ptr_);
  }

  deep_ptr &do_assign(const deep_ptr &p) {
    if (&p == this) {
      return *this;
    }

    if (ptr_) {
      assert(deleter_);
      deleter_->do_delete(ptr_);
    }

    do_copy_construct(p);
    
    return *this;
  }

  void do_move_construct(deep_ptr &&p) {
    if (ptr_) {
      assert(deleter_);
      deleter_->do_delete(ptr_);
    }
    
    deleter_ = std::move(p.deleter_);
    cloner_ = std::move(p.cloner_);
    ptr_ = p.ptr_;

    p.ptr_ = nullptr;
  }

  deep_ptr &do_move_assign(deep_ptr &&u) {
    do_move_construct(std::move(u));
    return *this;
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

  template <typename U,
            typename V = std::enable_if_t<std::is_same<T, U>::value ||
                                          std::is_base_of<T, U>::value>>
  explicit deep_ptr(U *u) {
    if (!u) {
      return;
    }

    deleter_ = std::make_unique<default_deleter<T, U>>();
    cloner_ = std::make_unique<default_cloner<T, U>>();
    ptr_ = u;
  }
  //
  // Copy-constructors
  //

  deep_ptr(const deep_ptr &p) { do_copy_construct(p); }


  //
  // Move-constructors
  //

  deep_ptr(deep_ptr &&p) { do_move_construct(std::move(p)); }
  

  //
  // Assignment
  //

  deep_ptr &operator=(const deep_ptr &p) { return do_assign(p); }


  //
  // Move-assignment
  //

  deep_ptr &operator=(deep_ptr &&p) { return do_move_assign(std::move(p)); }


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

template <typename T, typename U, typename ...Ts>
deep_ptr<T> make_deep_ptr(Ts&& ...ts)
{
  return deep_ptr<T>(new U(std::forward<Ts>(ts)...));
}

