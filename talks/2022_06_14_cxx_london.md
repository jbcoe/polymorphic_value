---
marp: true
theme: default
paginate: true
footer: 'Vocabulary types for composite class design: https://github.com/jbcoe/polymorphic_value/pull/105'
---

# Vocabulary types for composite class design

Jonathan B. Coe & Antony Peacock

<https://github.com/jbcoe/indirect_value>

<https://github.com/jbcoe/polymorphic_value>

---

* We propose the addition of two new class templates, `indirect_value<T>` and `polymorphic_value<T>` to make designing composite classes simple and correct.


* We'll look into some of the challenges of composite class design and see what problems are unsolved by current vocabulary types.


* We'll then run through our reference implementations of the proposed new types.

---

Encapsulation with classes (and structs)

---

The need for incomplete types

---

The need for polymorphism

--- 

Closed and open-set polymorphism

---

Compiler generated functions

---

`const` in C++

---

Reference and value types

---

`const` propagation and reference types

---

class-instance members

---

template-type-instance members

---

`std::variant<Ts...>` members

---

pointer (and reference) members

---

`std::unique_ptr<T>` members

---

`std::shared_ptr<T>` members

---

`std::experimental<propagate_const<T>` members

---

`indirect_value<T>` members

---

`polymorphic_value<T>` members

---

Implementing `indirect_value<T>`

---

Implementing `polymorphic_value<T>`

---

Using `indirect_value<T>` in your code

---

Using `polymorphic_value<T>` in your code

---

Standardisation efforts for addition to the C++ standard library

---

Acknowledgements