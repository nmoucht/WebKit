/*
 * Copyright (C) 2005-2024 Apple Inc. All rights reserved.
 * Copyright (C) 2011, Benjamin Poulain <ikipou@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#pragma once

#include <wtf/Forward.h>
#include <wtf/HashSet.h>

#if CHECK_HASHTABLE_ITERATORS
#include <wtf/WeakPtr.h>
#endif

namespace WTF {
template<typename Value, typename HashFunctions> class ListHashSet;
template<typename ValueArg> struct ListHashSetNode;
template<typename ValueArg, typename HashArg> class ListHashSetConstIterator;
}

namespace WTF {
template<typename T> struct IsDeprecatedWeakRefSmartPointerException;
template<typename Value, typename HashFunctions> struct IsDeprecatedWeakRefSmartPointerException<WTF::ListHashSet<Value, HashFunctions>> : std::true_type { };
template<typename ValueArg> struct IsDeprecatedWeakRefSmartPointerException<WTF::ListHashSetNode<ValueArg>> : std::true_type { };
template<typename ValueArg, typename HashArg> struct IsDeprecatedWeakRefSmartPointerException<WTF::ListHashSetConstIterator<ValueArg, HashArg>> : std::true_type { };
}

namespace WTF {

// ListHashSet: Just like HashSet, this class provides a Set
// interface - a collection of unique objects with O(1) insertion,
// removal and test for containership. However, it also has an
// order - iterating it will always give back values in the order
// in which they are added.

// Unlike iteration of most WTF Hash data structures, iteration is
// guaranteed safe against mutation of the ListHashSet, except for
// removal of the item currently pointed to by a given iterator.

template<typename Value, typename HashFunctions> class ListHashSet;

template<typename ValueArg, typename HashArg> class ListHashSetIterator;
template<typename ValueArg, typename HashArg> class ListHashSetConstIterator;

template<typename ValueArg> struct ListHashSetNode;

template<typename HashArg> struct ListHashSetNodeHashFunctions;
template<typename HashArg> struct ListHashSetTranslator;

template<typename ValueArg, typename HashArg> class ListHashSet final
#if CHECK_HASHTABLE_ITERATORS
    : public CanMakeWeakPtr<ListHashSet<ValueArg, HashArg>, WeakPtrFactoryInitialization::Eager>
#endif
{
    WTF_DEPRECATED_MAKE_FAST_ALLOCATED(ListHashSet);
private:
    typedef ListHashSetNode<ValueArg> Node;

    typedef HashTraits<Node*> NodeTraits;
    typedef ListHashSetNodeHashFunctions<HashArg> NodeHash;
    typedef ListHashSetTranslator<HashArg> BaseTranslator;

    typedef HashArg HashFunctions;

public:
    typedef ValueArg ValueType;

    typedef ListHashSetIterator<ValueType, HashArg> iterator;
    typedef ListHashSetConstIterator<ValueType, HashArg> const_iterator;
    friend class ListHashSetConstIterator<ValueType, HashArg>;

    typedef std::reverse_iterator<iterator> reverse_iterator;
    typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

    typedef HashTableAddResult<iterator> AddResult;

    ListHashSet() = default;
    ListHashSet(std::initializer_list<ValueType>);
    ListHashSet(const ListHashSet&);
    ListHashSet(ListHashSet&&);
    ListHashSet& operator=(const ListHashSet&);
    ListHashSet& operator=(ListHashSet&&);
    ~ListHashSet();

    void swap(ListHashSet&);

    unsigned size() const;
    unsigned capacity() const;
    bool isEmpty() const;

    iterator begin() LIFETIME_BOUND { return makeIterator(m_head); }
    iterator end() LIFETIME_BOUND { return makeIterator(nullptr); }
    const_iterator begin() const LIFETIME_BOUND { return makeConstIterator(m_head); }
    const_iterator end() const LIFETIME_BOUND { return makeConstIterator(nullptr); }

    iterator random() LIFETIME_BOUND { return makeIterator(m_impl.random()); }
    const_iterator random() const LIFETIME_BOUND { return makeIterator(m_impl.random()); }

    reverse_iterator rbegin() LIFETIME_BOUND { return reverse_iterator(end()); }
    reverse_iterator rend() LIFETIME_BOUND { return reverse_iterator(begin()); }
    const_reverse_iterator rbegin() const LIFETIME_BOUND { return const_reverse_iterator(end()); }
    const_reverse_iterator rend() const LIFETIME_BOUND { return const_reverse_iterator(begin()); }

    ValueType& first() LIFETIME_BOUND;
    const ValueType& first() const LIFETIME_BOUND;
    void removeFirst();
    ValueType takeFirst();

    ValueType& last() LIFETIME_BOUND;
    const ValueType& last() const LIFETIME_BOUND;
    void removeLast();
    ValueType takeLast();

    iterator find(const ValueType&);
    const_iterator find(const ValueType&) const;
    bool contains(const ValueType&) const;

    // An alternate version of find() that finds the object by hashing and comparing
    // with some other type, to avoid the cost of type conversion.
    // The HashTranslator interface is defined in HashSet.
    template<typename HashTranslator, typename T> iterator find(const T&);
    template<typename HashTranslator, typename T> const_iterator find(const T&) const;
    template<typename HashTranslator, typename T> bool contains(const T&) const;

    // The return value of add is a pair of an iterator to the new value's location, 
    // and a bool that is true if an new entry was added.
    AddResult add(const ValueType&);
    AddResult add(ValueType&&);

    // Add the value to the end of the collection. If the value was already in
    // the list, it is moved to the end.
    AddResult appendOrMoveToLast(const ValueType&);
    AddResult appendOrMoveToLast(ValueType&&);
    bool moveToLastIfPresent(const ValueType&);

    // Add the value to the beginning of the collection. If the value was already in
    // the list, it is moved to the beginning.
    AddResult prependOrMoveToFirst(const ValueType&);
    AddResult prependOrMoveToFirst(ValueType&&);

    AddResult insertBefore(const ValueType& beforeValue, const ValueType& newValue);
    AddResult insertBefore(const ValueType& beforeValue, ValueType&& newValue);
    AddResult insertBefore(iterator, const ValueType&);
    AddResult insertBefore(iterator, ValueType&&);

    bool remove(const ValueType&);
    bool remove(iterator);
    void clear();

    // Overloads for smart pointer values that take the raw pointer type as the parameter.
    template<typename V = ValueType> typename std::enable_if<IsSmartPtr<V>::value, iterator>::type find(std::add_const_t<typename GetPtrHelper<V>::UnderlyingType>*);
    template<typename V = ValueType> typename std::enable_if<IsSmartPtr<V>::value, const_iterator>::type find(std::add_const_t<typename GetPtrHelper<V>::UnderlyingType>*) const;
    template<typename V = ValueType> typename std::enable_if<IsSmartPtr<V>::value, bool>::type contains(std::add_const_t<typename GetPtrHelper<V>::UnderlyingType>*) const;
    template<typename V = ValueType> typename std::enable_if<IsSmartPtr<V>::value, AddResult>::type insertBefore(std::add_const_t<typename GetPtrHelper<V>::UnderlyingType>*, const ValueType&);
    template<typename V = ValueType> typename std::enable_if<IsSmartPtr<V>::value, AddResult>::type insertBefore(std::add_const_t<typename GetPtrHelper<V>::UnderlyingType>*, ValueType&&);
    template<typename V = ValueType> typename std::enable_if<IsSmartPtr<V>::value, bool>::type remove(std::add_const_t<typename GetPtrHelper<V>::UnderlyingType>*);

    // Overloads for non-nullable smart pointer values that take the raw reference type as the parameter.
    template<typename V = ValueType> typename std::enable_if<IsSmartPtr<V>::value && !IsSmartPtr<V>::isNullable, iterator>::type find(std::add_const_t<typename GetPtrHelper<V>::UnderlyingType>&);
    template<typename V = ValueType> typename std::enable_if<IsSmartPtr<V>::value && !IsSmartPtr<V>::isNullable, const_iterator>::type find(std::add_const_t<typename GetPtrHelper<V>::UnderlyingType>&) const;
    template<typename V = ValueType> typename std::enable_if<IsSmartPtr<V>::value && !IsSmartPtr<V>::isNullable, bool>::type contains(std::add_const_t<typename GetPtrHelper<V>::UnderlyingType>&) const;
    template<typename V = ValueType> typename std::enable_if<IsSmartPtr<V>::value && !IsSmartPtr<V>::isNullable, AddResult>::type insertBefore(std::add_const_t<typename GetPtrHelper<V>::UnderlyingType>&, const ValueType&);
    template<typename V = ValueType> typename std::enable_if<IsSmartPtr<V>::value && !IsSmartPtr<V>::isNullable, AddResult>::type insertBefore(std::add_const_t<typename GetPtrHelper<V>::UnderlyingType>&, ValueType&&);
    template<typename V = ValueType> typename std::enable_if<IsSmartPtr<V>::value && !IsSmartPtr<V>::isNullable, bool>::type remove(std::add_const_t<typename GetPtrHelper<V>::UnderlyingType>&);

private:
    void unlink(Node*);
    void unlinkAndDelete(Node*);
    void appendNode(Node*);
    void prependNode(Node*);
    void insertNodeBefore(Node* beforeNode, Node* newNode);
    void deleteAllNodes();
    
    iterator makeIterator(Node*);
    const_iterator makeConstIterator(Node*) const;

    HashTable<Node*, Node*, IdentityExtractor, NodeHash, NodeTraits, NodeTraits> m_impl;
    Node* m_head { nullptr };
    Node* m_tail { nullptr };
};

template<typename ValueArg> struct ListHashSetNode
#if CHECK_HASHTABLE_ITERATORS
    : CanMakeWeakPtr<ListHashSetNode<ValueArg>, WeakPtrFactoryInitialization::Eager>
#endif
{
    WTF_DEPRECATED_MAKE_STRUCT_FAST_ALLOCATED(ListHashSetNode);

    template<typename T> ListHashSetNode(T&& value)
        : m_value(std::forward<T>(value))
    {
    }

    ValueArg m_value;
    ListHashSetNode* m_prev { nullptr };
    ListHashSetNode* m_next { nullptr };
};

template<typename HashArg> struct ListHashSetNodeHashFunctions {
    template<typename T> static unsigned hash(const T& key) { return HashArg::hash(key->m_value); }
    template<typename T> static bool equal(const T& a, const T& b) { return HashArg::equal(a->m_value, b->m_value); }
    static constexpr bool safeToCompareToEmptyOrDeleted = false;
};

template<typename ValueArg, typename HashArg> class ListHashSetIterator {
    WTF_DEPRECATED_MAKE_FAST_ALLOCATED(ListHashSetIterator);
private:
    typedef ListHashSet<ValueArg, HashArg> ListHashSetType;
    typedef ListHashSetIterator<ValueArg, HashArg> iterator;
    typedef ListHashSetConstIterator<ValueArg, HashArg> const_iterator;
    typedef ListHashSetNode<ValueArg> Node;
    typedef ValueArg ValueType;

    friend class ListHashSet<ValueArg, HashArg>;

    ListHashSetIterator(const ListHashSetType* set, Node* position) : m_iterator(set, position) { }

public:
    typedef ptrdiff_t difference_type;
    typedef ValueType value_type;
    typedef ValueType* pointer;
    typedef ValueType& reference;
    typedef std::bidirectional_iterator_tag iterator_category;

    ListHashSetIterator() = default;

    // default copy, assignment and destructor are OK

    ValueType* get() const { return const_cast<ValueType*>(m_iterator.get()); }
    ValueType& operator*() const { return *get(); }
    ValueType* operator->() const { return get(); }

    iterator& operator++() { ++m_iterator; return *this; }
    iterator operator++(int)
    {
        iterator temp = *this;
        ++(*this);
        return temp;
    }

    iterator& operator--() { --m_iterator; return *this; }

    // postfix -- intentionally omitted

    // Comparison.
    friend bool operator==(const iterator&, const iterator&) = default;

    operator const_iterator() const { return m_iterator; }

private:
    Node* node() { return m_iterator.node(); }

    const_iterator m_iterator;
};

template<typename ValueArg, typename HashArg> class ListHashSetConstIterator {
    WTF_DEPRECATED_MAKE_FAST_ALLOCATED(ListHashSetConstIterator);
private:
    typedef ListHashSet<ValueArg, HashArg> ListHashSetType;
    typedef ListHashSetIterator<ValueArg, HashArg> iterator;
    typedef ListHashSetConstIterator<ValueArg, HashArg> const_iterator;
    typedef ListHashSetNode<ValueArg> Node;
    typedef ValueArg ValueType;

    friend class ListHashSet<ValueArg, HashArg>;
    friend class ListHashSetIterator<ValueArg, HashArg>;

    ListHashSetConstIterator(const ListHashSetType* set, Node* position)
        : m_set(set)
        , m_position(position)
#if CHECK_HASHTABLE_ITERATORS
        , m_weakSet(set)
        , m_weakPosition(position)
#endif
    {
    }

public:
    typedef ptrdiff_t difference_type;
    typedef const ValueType value_type;
    typedef const ValueType* pointer;
    typedef const ValueType& reference;
    typedef std::bidirectional_iterator_tag iterator_category;

    ListHashSetConstIterator()
    {
    }

    const ValueType* get() const
    {
#if CHECK_HASHTABLE_ITERATORS
        ASSERT(m_weakPosition);
#endif
        return std::addressof(m_position->m_value);
    }

    const ValueType& operator*() const { return *get(); }
    const ValueType* operator->() const { return get(); }

    const_iterator& operator++()
    {
#if CHECK_HASHTABLE_ITERATORS
        ASSERT(m_weakPosition);
#endif
        ASSERT(m_position);
        m_position = m_position->m_next;
#if CHECK_HASHTABLE_ITERATORS
        m_weakPosition = m_position;
#endif
        return *this;
    }

    const_iterator operator++(int)
    {
        const_iterator temp = *this;
        ++(*this);
        return temp;
    }

    const_iterator& operator--()
    {
#if CHECK_HASHTABLE_ITERATORS
        ASSERT(m_weakSet);
        m_weakPosition.get();
#endif
        ASSERT(m_position != m_set->m_head);
        if (!m_position)
            m_position = m_set->m_tail;
        else
            m_position = m_position->m_prev;
#if CHECK_HASHTABLE_ITERATORS
        m_weakPosition = m_position;
#endif
        return *this;
    }

    // postfix -- intentionally omitted

    // Comparison.
    bool operator==(const const_iterator& other) const
    {
        return m_position == other.m_position;
    }

private:
    Node* node() { return m_position; }

    const ListHashSetType* m_set { nullptr };
    Node* m_position { nullptr };
#if CHECK_HASHTABLE_ITERATORS
    WeakPtr<const ListHashSetType> m_weakSet;
    WeakPtr<Node> m_weakPosition;
#endif
};

template<typename HashFunctions>
struct ListHashSetTranslator {
    template<typename T> static unsigned hash(const T& key) { return HashFunctions::hash(key); }
    template<typename T, typename U> static bool equal(const T& a, const U& b) { return HashFunctions::equal(a->m_value, b); }
    template<typename T, typename U, typename V> static void translate(T*& location, U&& key, V&&)
    {
        location = new T(std::forward<U>(key));
    }
};


template<typename T, typename U>
inline ListHashSet<T, U>::ListHashSet(std::initializer_list<T> initializerList)
{
    for (const auto& value : initializerList)
        add(value);
}

template<typename T, typename U>
inline ListHashSet<T, U>::ListHashSet(const ListHashSet& other)
    : ListHashSet<T, U>()
{
    for (auto& item : other)
        add(item);
}

template<typename T, typename U>
inline ListHashSet<T, U>& ListHashSet<T, U>::operator=(const ListHashSet& other)
{
    ListHashSet tmp(other);
    swap(tmp);
    return *this;
}

template<typename T, typename U>
inline ListHashSet<T, U>::ListHashSet(ListHashSet&& other)
    : m_impl(WTFMove(other.m_impl))
    , m_head(std::exchange(other.m_head, nullptr))
    , m_tail(std::exchange(other.m_tail, nullptr))
{
}

template<typename T, typename U>
inline ListHashSet<T, U>& ListHashSet<T, U>::operator=(ListHashSet&& other)
{
    ListHashSet movedSet(WTFMove(other));
    swap(movedSet);
    return *this;
}

template<typename T, typename U>
inline void ListHashSet<T, U>::swap(ListHashSet& other)
{
    m_impl.swap(other.m_impl);
    std::swap(m_head, other.m_head);
    std::swap(m_tail, other.m_tail);
}

template<typename T, typename U>
inline ListHashSet<T, U>::~ListHashSet()
{
    deleteAllNodes();
}

template<typename T, typename U>
inline unsigned ListHashSet<T, U>::size() const
{
    return m_impl.size(); 
}

template<typename T, typename U>
inline unsigned ListHashSet<T, U>::capacity() const
{
    return m_impl.capacity(); 
}

template<typename T, typename U>
inline bool ListHashSet<T, U>::isEmpty() const
{
    return m_impl.isEmpty(); 
}

template<typename T, typename U>
inline T& ListHashSet<T, U>::first()
{
    ASSERT(!isEmpty());
    return m_head->m_value;
}

template<typename T, typename U>
inline void ListHashSet<T, U>::removeFirst()
{
    takeFirst();
}

template<typename T, typename U>
inline T ListHashSet<T, U>::takeFirst()
{
    ASSERT(!isEmpty());
    auto it = m_impl.template find<ShouldValidateKey::Yes>(m_head);

    T result = WTFMove((*it)->m_value);
    m_impl.remove(it);
    unlinkAndDelete(m_head);

    return result;
}

template<typename T, typename U>
inline const T& ListHashSet<T, U>::first() const
{
    ASSERT(!isEmpty());
    return m_head->m_value;
}

template<typename T, typename U>
inline T& ListHashSet<T, U>::last()
{
    ASSERT(!isEmpty());
    return m_tail->m_value;
}

template<typename T, typename U>
inline const T& ListHashSet<T, U>::last() const
{
    ASSERT(!isEmpty());
    return m_tail->m_value;
}

template<typename T, typename U>
inline void ListHashSet<T, U>::removeLast()
{
    takeLast();
}

template<typename T, typename U>
inline T ListHashSet<T, U>::takeLast()
{
    ASSERT(!isEmpty());
    auto it = m_impl.template find<ShouldValidateKey::Yes>(m_tail);

    T result = WTFMove((*it)->m_value);
    m_impl.remove(it);
    unlinkAndDelete(m_tail);

    return result;
}

template<typename T, typename U>
inline auto ListHashSet<T, U>::find(const ValueType& value) -> iterator
{
    auto it = m_impl.template find<BaseTranslator, ShouldValidateKey::Yes>(value);
    if (it == m_impl.end())
        return end();
    return makeIterator(*it); 
}

template<typename T, typename U>
inline auto ListHashSet<T, U>::find(const ValueType& value) const -> const_iterator
{
    auto it = m_impl.template find<BaseTranslator, ShouldValidateKey::Yes>(value);
    if (it == m_impl.end())
        return end();
    return makeConstIterator(*it);
}

template<typename Translator>
struct ListHashSetTranslatorAdapter {
    template<typename T> static unsigned hash(const T& key) { return Translator::hash(key); }
    template<typename T, typename U> static bool equal(const T& a, const U& b) { return Translator::equal(a->m_value, b); }
};

template<typename ValueType, typename U>
template<typename HashTranslator, typename T>
inline auto ListHashSet<ValueType, U>::find(const T& value) -> iterator
{
    auto it = m_impl.template find<ListHashSetTranslatorAdapter<HashTranslator>, ShouldValidateKey::Yes>(value);
    if (it == m_impl.end())
        return end();
    return makeIterator(*it);
}

template<typename ValueType, typename U>
template<typename HashTranslator, typename T>
inline auto ListHashSet<ValueType, U>::find(const T& value) const -> const_iterator
{
    auto it = m_impl.template find<ListHashSetTranslatorAdapter<HashTranslator>, ShouldValidateKey::Yes>(value);
    if (it == m_impl.end())
        return end();
    return makeConstIterator(*it);
}

template<typename ValueType, typename U>
template<typename HashTranslator, typename T>
inline bool ListHashSet<ValueType, U>::contains(const T& value) const
{
    return m_impl.template contains<ListHashSetTranslatorAdapter<HashTranslator>, ShouldValidateKey::Yes>(value);
}

template<typename T, typename U>
inline bool ListHashSet<T, U>::contains(const ValueType& value) const
{
    return m_impl.template contains<BaseTranslator, ShouldValidateKey::Yes>(value);
}

template<typename T, typename U>
auto ListHashSet<T, U>::add(const ValueType& value) -> AddResult
{
    auto result = m_impl.template add<BaseTranslator, ShouldValidateKey::Yes>(value, [] { return nullptr; });
    if (result.isNewEntry)
        appendNode(*result.iterator);
    return AddResult(makeIterator(*result.iterator), result.isNewEntry);
}

template<typename T, typename U>
auto ListHashSet<T, U>::add(ValueType&& value) -> AddResult
{
    auto result = m_impl.template add<BaseTranslator, ShouldValidateKey::Yes>(WTFMove(value), [] { return nullptr; });
    if (result.isNewEntry)
        appendNode(*result.iterator);
    return AddResult(makeIterator(*result.iterator), result.isNewEntry);
}

template<typename T, typename U>
auto ListHashSet<T, U>::appendOrMoveToLast(const ValueType& value) -> AddResult
{
    auto result = m_impl.template add<BaseTranslator, ShouldValidateKey::Yes>(value, [] { return nullptr; });
    Node* node = *result.iterator;
    if (!result.isNewEntry)
        unlink(node);
    appendNode(node);

    return AddResult(makeIterator(*result.iterator), result.isNewEntry);
}

template<typename T, typename U>
auto ListHashSet<T, U>::appendOrMoveToLast(ValueType&& value) -> AddResult
{
    auto result = m_impl.template add<BaseTranslator, ShouldValidateKey::Yes>(WTFMove(value), [] { return nullptr; });
    Node* node = *result.iterator;
    if (!result.isNewEntry)
        unlink(node);
    appendNode(node);

    return AddResult(makeIterator(*result.iterator), result.isNewEntry);
}

template<typename T, typename U>
bool ListHashSet<T, U>::moveToLastIfPresent(const ValueType& value)
{
    auto iterator = m_impl.template find<BaseTranslator, ShouldValidateKey::Yes>(value);
    if (iterator == m_impl.end())
        return false;
    Node* node = *iterator;
    unlink(node);
    appendNode(node);
    return true;
}

template<typename T, typename U>
auto ListHashSet<T, U>::prependOrMoveToFirst(const ValueType& value) -> AddResult
{
    auto result = m_impl.template add<BaseTranslator, ShouldValidateKey::Yes>(value, [] { return nullptr; });
    Node* node = *result.iterator;
    if (!result.isNewEntry)
        unlink(node);
    prependNode(node);

    return AddResult(makeIterator(*result.iterator), result.isNewEntry);
}

template<typename T, typename U>
auto ListHashSet<T, U>::prependOrMoveToFirst(ValueType&& value) -> AddResult
{
    auto result = m_impl.template add<BaseTranslator, ShouldValidateKey::Yes>(WTFMove(value), [] { return nullptr; });
    Node* node = *result.iterator;
    if (!result.isNewEntry)
        unlink(node);
    prependNode(node);

    return AddResult(makeIterator(*result.iterator), result.isNewEntry);
}

template<typename T, typename U>
auto ListHashSet<T, U>::insertBefore(const ValueType& beforeValue, const ValueType& newValue) -> AddResult
{
    return insertBefore(find(beforeValue), newValue);
}

template<typename T, typename U>
auto ListHashSet<T, U>::insertBefore(const ValueType& beforeValue, ValueType&& newValue) -> AddResult
{
    return insertBefore(find(beforeValue), WTFMove(newValue));
}

template<typename T, typename U>
auto ListHashSet<T, U>::insertBefore(iterator it, const ValueType& newValue) -> AddResult
{
    auto result = m_impl.template add<BaseTranslator, ShouldValidateKey::Yes>(newValue, [] { return nullptr; });
    if (result.isNewEntry)
        insertNodeBefore(it.node(), *result.iterator);
    return AddResult(makeIterator(*result.iterator), result.isNewEntry);
}

template<typename T, typename U>
auto ListHashSet<T, U>::insertBefore(iterator it, ValueType&& newValue) -> AddResult
{
    auto result = m_impl.template add<BaseTranslator, ShouldValidateKey::Yes>(WTFMove(newValue), [] { return nullptr; });
    if (result.isNewEntry)
        insertNodeBefore(it.node(), *result.iterator);
    return AddResult(makeIterator(*result.iterator), result.isNewEntry);
}

template<typename T, typename U>
inline bool ListHashSet<T, U>::remove(iterator it)
{
    if (it == end())
        return false;
    m_impl.template remove<ShouldValidateKey::Yes>(it.node());
    unlinkAndDelete(it.node());
    return true;
}

template<typename T, typename U>
inline bool ListHashSet<T, U>::remove(const ValueType& value)
{
    return remove(find(value));
}

template<typename T, typename U>
inline void ListHashSet<T, U>::clear()
{
    deleteAllNodes();
    m_impl.clear(); 
    m_head = nullptr;
    m_tail = nullptr;
}

template<typename T, typename U>
template<typename V>
inline auto ListHashSet<T, U>::find(std::add_const_t<typename GetPtrHelper<V>::UnderlyingType>* value) -> typename std::enable_if<IsSmartPtr<V>::value, iterator>::type
{
    auto it = m_impl.template find<BaseTranslator, ShouldValidateKey::Yes>(value);
    if (it == m_impl.end())
        return end();
    return makeIterator(*it);
}

template<typename T, typename U>
template<typename V>
inline auto ListHashSet<T, U>::find(std::add_const_t<typename GetPtrHelper<V>::UnderlyingType>* value) const -> typename std::enable_if<IsSmartPtr<V>::value, const_iterator>::type
{
    auto it = m_impl.template find<BaseTranslator, ShouldValidateKey::Yes>(value);
    if (it == m_impl.end())
        return end();
    return makeConstIterator(*it);
}

template<typename T, typename U>
template<typename V>
inline auto ListHashSet<T, U>::contains(std::add_const_t<typename GetPtrHelper<V>::UnderlyingType>* value) const -> typename std::enable_if<IsSmartPtr<V>::value, bool>::type
{
    return m_impl.template contains<BaseTranslator, ShouldValidateKey::Yes>(value);
}

template<typename T, typename U>
template<typename V>
inline auto ListHashSet<T, U>::insertBefore(std::add_const_t<typename GetPtrHelper<V>::UnderlyingType>* beforeValue, const ValueType& newValue) -> typename std::enable_if<IsSmartPtr<V>::value, AddResult>::type
{
    return insertBefore(find(beforeValue), newValue);
}

template<typename T, typename U>
template<typename V>
inline auto ListHashSet<T, U>::insertBefore(std::add_const_t<typename GetPtrHelper<V>::UnderlyingType>* beforeValue, ValueType&& newValue) -> typename std::enable_if<IsSmartPtr<V>::value, AddResult>::type
{
    return insertBefore(find(beforeValue), WTFMove(newValue));
}

template<typename T, typename U>
template<typename V>
inline auto ListHashSet<T, U>::remove(std::add_const_t<typename GetPtrHelper<V>::UnderlyingType>* value) -> typename std::enable_if<IsSmartPtr<V>::value, bool>::type
{
    return remove(find(value));
}

template<typename T, typename U>
template<typename V>
inline auto ListHashSet<T, U>::find(std::add_const_t<typename GetPtrHelper<V>::UnderlyingType>& value) -> typename std::enable_if<IsSmartPtr<V>::value && !IsSmartPtr<V>::isNullable, iterator>::type
{
    return find(&value);
}

template<typename T, typename U>
template<typename V>
inline auto ListHashSet<T, U>::find(std::add_const_t<typename GetPtrHelper<V>::UnderlyingType>& value) const -> typename std::enable_if<IsSmartPtr<V>::value && !IsSmartPtr<V>::isNullable, const_iterator>::type
{
    return find(&value);
}

template<typename T, typename U>
template<typename V>
inline auto ListHashSet<T, U>::contains(std::add_const_t<typename GetPtrHelper<V>::UnderlyingType>& value) const -> typename std::enable_if<IsSmartPtr<V>::value && !IsSmartPtr<V>::isNullable, bool>::type
{
    return contains(&value);
}

template<typename T, typename U>
template<typename V>
inline auto ListHashSet<T, U>::insertBefore(std::add_const_t<typename GetPtrHelper<V>::UnderlyingType>& beforeValue, const ValueType& newValue) -> typename std::enable_if<IsSmartPtr<V>::value && !IsSmartPtr<V>::isNullable, AddResult>::type
{
    return insertBefore(&beforeValue, newValue);
}

template<typename T, typename U>
template<typename V>
inline auto ListHashSet<T, U>::insertBefore(std::add_const_t<typename GetPtrHelper<V>::UnderlyingType>& beforeValue, ValueType&& newValue) -> typename std::enable_if<IsSmartPtr<V>::value && !IsSmartPtr<V>::isNullable, AddResult>::type
{
    return insertBefore(&beforeValue, WTFMove(newValue));
}

template<typename T, typename U>
template<typename V>
inline auto ListHashSet<T, U>::remove(std::add_const_t<typename GetPtrHelper<V>::UnderlyingType>& value) -> typename std::enable_if<IsSmartPtr<V>::value && !IsSmartPtr<V>::isNullable, bool>::type
{
    return remove(&value);
}

template<typename T, typename U>
void ListHashSet<T, U>::unlink(Node* node)
{
    if (!node->m_prev) {
        ASSERT(node == m_head);
        m_head = node->m_next;
    } else {
        ASSERT(node != m_head);
        node->m_prev->m_next = node->m_next;
    }

    if (!node->m_next) {
        ASSERT(node == m_tail);
        m_tail = node->m_prev;
    } else {
        ASSERT(node != m_tail);
        node->m_next->m_prev = node->m_prev;
    }
}

template<typename T, typename U>
void ListHashSet<T, U>::unlinkAndDelete(Node* node)
{
    unlink(node);
    delete node;
}

template<typename T, typename U>
void ListHashSet<T, U>::appendNode(Node* node)
{
    node->m_prev = m_tail;
    node->m_next = nullptr;

    if (m_tail) {
        ASSERT(m_head);
        m_tail->m_next = node;
    } else {
        ASSERT(!m_head);
        m_head = node;
    }

    m_tail = node;
}

template<typename T, typename U>
void ListHashSet<T, U>::prependNode(Node* node)
{
    node->m_prev = nullptr;
    node->m_next = m_head;

    if (m_head)
        m_head->m_prev = node;
    else
        m_tail = node;

    m_head = node;
}

template<typename T, typename U>
void ListHashSet<T, U>::insertNodeBefore(Node* beforeNode, Node* newNode)
{
    if (!beforeNode)
        return appendNode(newNode);
    
    newNode->m_next = beforeNode;
    newNode->m_prev = beforeNode->m_prev;
    if (beforeNode->m_prev)
        beforeNode->m_prev->m_next = newNode;
    beforeNode->m_prev = newNode;

    if (!newNode->m_prev)
        m_head = newNode;
}

template<typename T, typename U>
void ListHashSet<T, U>::deleteAllNodes()
{
    if (!m_head)
        return;

    for (Node* node = m_head, *next = m_head->m_next; node; node = next, next = node ? node->m_next : nullptr)
        delete node;
}

template<typename T, typename U>
inline auto ListHashSet<T, U>::makeIterator(Node* position) -> iterator
{
    return iterator(this, position);
}

template<typename T, typename U>
inline auto ListHashSet<T, U>::makeConstIterator(Node* position) const -> const_iterator
{ 
    return const_iterator(this, position);
}

} // namespace WTF

using WTF::ListHashSet;
