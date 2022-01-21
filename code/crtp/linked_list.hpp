#pragma once

// Adapted from brpc-LinkedList
// https://github.com/apache/incubator-brpc/blob/master/src/butil/containers/linked_list.h

// Simple LinkedList type. (See the Q&A section to understand how this
// differs from std::list).
//
// To use, start by declaring the class which will be contained in the linked
// list, as extending LinkNode (this gives it next/previous pointers).
//
//   class MyNodeType : public LinkNode<MyNoeType> {
//     ...d
//   };
//
// Next, to keep track of the list's Head/Tail, use a LinkedList instance:
//
//   LinkedList<MyNodeType> list;
//
// To add elements to the list, use any of LinkedList::Append,
// LinkNode::InsertBefore, or LinkNode::InsertAfter:
//
//   LinkNode<MyNodeType>* n1 = ...;
//   LinkNode<MyNodeType>* n2 = ...;
//   LinkNode<MyNodeType>* n3 = ...;
//
//   list.Append(n1);
//   list.Append(n3);
//   n2->InsertBefore(n3);
//
// Lastly, to iterate through the linked list forwards:
//
//   for (LinkNode<MyNodeType>* node = list.Head();
//        node != list.End();
//        node = node->Next()) {
//     MyNodeType* value = node->Value();
//     ...
//   }
//
// Or to iterate the linked list backwards:
//
//   for (LinkNode<MyNodeType>* node = list.Tail();
//        node != list.End();
//        node = node->Previous()) {
//     MyNodeType* value = node->Value();
//     ...
//   }
//
// Questions and Answers:
//
// Q. Should I use std::list or butil::LinkedList?
//
// A. The main reason to use butil::LinkedList over std::list is
//    performance. If you don't care about the performance differences
//    then use an STL container, as it makes for better code readability.
//
//    Comparing the performance of butil::LinkedList<T> to std::list<T*>:
//
//    * Erasing an element of type T* from butil::LinkedList<T> is
//      an O(1) operation. Whereas for std::list<T*> it is O(n).
//      That is because with std::list<T*> you must obtain an
//      iterator to the T* element before you can call erase(iterator).
//
//    * Insertion operations with butil::LinkedList<T> never require
//      heap allocations.
//
// Q. How does butil::LinkedList implementation differ from std::list?
//
// A. Doubly-linked lists are made up of nodes that contain "next" and
//    "previous" pointers that reference other nodes in the list.
//
//    With butil::LinkedList<T>, the type being inserted already reserves
//    space for the "next" and "previous" pointers (butil::LinkNode<T>*).
//    Whereas with std::list<T> the type can be anything, so the implementation
//    needs to glue on the "next" and "previous" pointers using
//    some internal node type.

namespace cpp_idioms {

template <typename T>
class LinkNode {
 public:
  // LinkNode are self-referential as default.
  LinkNode() : previous_(this), next_(this) {}

  LinkNode(LinkNode<T>* previous, LinkNode<T>* next)
      : previous_(previous), next_(next) {}

  LinkNode(const LinkNode&) = delete;
  LinkNode& operator=(const LinkNode&) = delete;

  // Insert |this| into the linked list, before |e|.
  void InsertBefore(LinkNode<T>* e) {
    this->next_ = e;
    this->previous_ = e->previous_;
    e->previous_->next_ = this;
    e->previous_ = this;
  }

  // Insert |this| as a circular linked list into the linked list, before |e|.
  void InsertBeforeAsList(LinkNode<T>* e) {
    LinkNode<T>* prev = this->previous_;
    prev->next_ = e;
    this->previous_ = e->previous_;
    e->previous_->next_ = this;
    e->previous_ = prev;
  }

  // Insert |this| into the linked list, after |e|.
  void InsertAfter(LinkNode<T>* e) {
    this->next_ = e->next_;
    this->previous_ = e;
    e->next_->previous_ = this;
    e->next_ = this;
  }

  // Insert |this| as a circular list into the linked list, after |e|.
  void InsertAfterAsList(LinkNode<T>* e) {
    LinkNode<T>* prev = this->previous_;
    prev->next_ = e->next_;
    this->previous_ = e;
    e->next_->previous_ = prev;
    e->next_ = this;
  }

  // Remove |this| from the linked list.
  void RemoveFromList() {
    this->previous_->next_ = this->next_;
    this->next_->previous_ = this->previous_;
    // Next() and Previous() return non-NULL if and only this node is not in any
    // list.
    this->next_ = this;
    this->previous_ = this;
  }

  LinkNode<T>* Previous() const { return previous_; }

  LinkNode<T>* Next() const { return next_; }

  // Cast from the node-type to the value type.
  const T* Value() const { return static_cast<const T*>(this); }

  T* Value() { return static_cast<T*>(this); }

 private:
  LinkNode<T>* previous_;
  LinkNode<T>* next_;
};

template <typename T>
class LinkedList {
 public:
  // The "root" node is self-referential, and forms the basis of a circular
  // list (root_.Next() will point back to the start of the list,
  // and root_->Previous() wraps around to the end of the list).
  LinkedList() {}
  LinkedList(const LinkedList&) = delete;
  LinkedList& operator=(LinkedList&) = delete;

  // Appends |e| to the end of the linked list.
  void Append(LinkNode<T>* e) { e->InsertBefore(&root_); }

  LinkNode<T>* Head() const { return root_.Next(); }

  LinkNode<T>* Tail() const { return root_.Previous(); }

  const LinkNode<T>* End() const { return &root_; }

  bool Empty() const { return Head() == End(); }

 private:
  LinkNode<T> root_;
};

}  // namespace cpp_idioms
