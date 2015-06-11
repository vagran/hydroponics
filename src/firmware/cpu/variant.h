/* This file is a part of 'hydroponics' project.
 * Copyright (c) 2012-2015, Artyom Lebedev <artyom.lebedev@gmail.com>
 * All rights reserved.
 * See COPYING file for copyright details.
 */

/** @file variant.h
 * TODO insert description here.
 */

#ifndef VARIANT_H_
#define VARIANT_H_

/** Placement new definition. */
//XXX move to std replacement library
inline void *
operator new(size_t, void *p)
{
    return p;
}

namespace internal {

/** Base class for Variant. */
template <size_t size>
class VariantBase {
protected:
    template<typename... Types> friend class Variant;

    /** Currently stored type. Corresponds to VariantMem<Type>::typeCode. 0 is
     * none.
     */
    u8 curType;
    /** Buffer for stored object instance. */
    u8 buf[size];

    void
    DisengageImpl()
    {}

} __PACKED;

/** Each member of variant is represented by this class in the inheritance
 * hierarchy.
 */
template <typename Type, u8 code, class Base>
class VariantMem: public Base {
public:
    ~VariantMem()
    {
        if (Base::curType == typeCode) {
            GetPtr()->~Type();
        }
    }
protected:
    static const u8 typeCode = code;

    Type *
    GetPtr()
    {
        //XXX assert curType == typeCode
        return reinterpret_cast<Type *>(Base::buf);
    }

    template <typename... Args>
    void
    EngageImpl(Args... args)
    {
        new(GetPtr()) Type(args...);
    }

    void
    DisengageImpl()
    {
        if (Base::curType == typeCode) {
            GetPtr()->~Type();
            Base::curType = 0;
        } else {
            Base::DisengageImpl();
        }
    }

} __PACKED;

/** Select base class for Variant. */
template <size_t size, u8 code, typename... Types>
struct VariantBaseHelper;

template <size_t size, u8 code, typename Type, typename... Types>
struct VariantBaseHelper<size, code, Type, Types...> {
    using BaseType = VariantMem<Type, code,
        typename VariantBaseHelper<size, code + 1, Types...>::BaseType>;
};

template <size_t size, u8 code, typename Type>
struct VariantBaseHelper<size, code, Type> {
    using BaseType = VariantMem<Type, code, VariantBase<size>>;
};

/** Determine buffer size (maximal size of the provided types). */
template <typename... Types>
struct VariantSizeHelper;

template <typename Type, typename... Types>
struct VariantSizeHelper<Type, Types...> {
    static const size_t size = adk::Max(sizeof(Type), VariantSizeHelper<Types...>::size);
};

template <typename Type>
struct VariantSizeHelper<Type> {
    static const size_t size = sizeof(Type);
};

/** Helper for selecting base class type corresponding to the provided wrapped
 * type.
 */
template <size_t size, u8 code, typename SelType, typename... Types>
struct VariantMemSelector;

template <size_t size, u8 code, typename SelType, typename Type, typename... Types>
struct VariantMemSelector<size, code, SelType, Type, Types...> {
    using BaseType =
        typename VariantMemSelector<size, code + 1, SelType, Types...>::BaseType;
};

template <size_t size, u8 code, typename SelType, typename... Types>
struct VariantMemSelector<size, code, SelType, SelType, Types...> {
    using BaseType =
        typename VariantBaseHelper<size, code, SelType, Types...>::BaseType;
};

} /* namespace internal */

template <typename... Types>
class Variant: public internal::VariantBaseHelper<
    internal::VariantSizeHelper<Types...>::size, 1, Types...>::BaseType {

public:

    /** Get reference to the specified stored type. */
    template <typename Type>
    Type &
    Get()
    {
        return *SelBase<Type>::GetPtr();
    }

    /** Construct value of the specified type. */
    template <typename Type, typename... Args>
    void
    Engage(Args ...args)
    {
        Disengage();
        //XXX forward
        SelBase<Type>::EngageImpl(args...);
    }

    /** Destruct stored value. */
    void
    Disengage()
    {
        if (BaseType::curType) {
            BaseType::DisengageImpl();
        }
    }

private:
    using BaseType =
        typename internal::VariantBaseHelper<
            internal::VariantSizeHelper<Types...>::size, 1, Types...>::BaseType;

    /** Get base type corresponding to the provided wrapped type. */
    template <typename Type>
    using SelBase = typename internal::VariantMemSelector<
        sizeof(BaseType::buf), 1, Type, Types...>::BaseType;
} __PACKED;


#endif /* VARIANT_H_ */
