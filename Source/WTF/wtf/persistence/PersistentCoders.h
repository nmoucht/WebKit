/*
 * Copyright (C) 2010-2020 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include <utility>
#include <wtf/CheckedArithmetic.h>
#include <wtf/Forward.h>
#include <wtf/HashMap.h>
#include <wtf/HashSet.h>
#include <wtf/StdLibExtras.h>
#include <wtf/Vector.h>
#include <wtf/persistence/PersistentDecoder.h>
#include <wtf/persistence/PersistentEncoder.h>

namespace WTF::Persistence {

template<typename> struct Coder;
class Decoder;
class Encoder;

template<typename T, typename U> struct Coder<std::pair<T, U>> {
    template<typename Encoder>
    static void encodeForPersistence(Encoder& encoder, const std::pair<T, U>& pair)
    {
        encoder << pair.first << pair.second;
    }

    template<typename Decoder>
    static std::optional<std::pair<T, U>> decodeForPersistence(Decoder& decoder)
    {
        std::optional<T> first;
        decoder >> first;
        if (!first)
            return std::nullopt;

        std::optional<U> second;
        decoder >> second;
        if (!second)
            return std::nullopt;

        return {{ WTFMove(*first), WTFMove(*second) }};
    }
};

template<typename T> struct Coder<std::optional<T>> {
    template<typename Encoder>
    static void encodeForPersistence(Encoder& encoder, const std::optional<T>& optional)
    {
        if (!optional) {
            encoder << false;
            return;
        }
        
        encoder << true;
        encoder << optional.value();
    }
    
    template<typename Decoder>
    static std::optional<std::optional<T>> decodeForPersistence(Decoder& decoder)
    {
        std::optional<bool> isEngaged;
        decoder >> isEngaged;
        if (!isEngaged)
            return std::nullopt;
        if (!*isEngaged)
            return std::optional<std::optional<T>> { std::optional<T> { std::nullopt } };
        
        std::optional<T> value;
        decoder >> value;
        if (!value)
            return std::nullopt;
        
        return std::optional<std::optional<T>> { std::optional<T> { WTFMove(*value) } };
    }
};

template<typename KeyType, typename ValueType> struct Coder<WTF::KeyValuePair<KeyType, ValueType>> {
    template<typename Encoder>
    static void encodeForPersistence(Encoder& encoder, const WTF::KeyValuePair<KeyType, ValueType>& pair)
    {
        encoder << pair.key << pair.value;
    }

    template<typename Decoder>
    static std::optional<WTF::KeyValuePair<KeyType, ValueType>> decodeForPersistence(Decoder& decoder)
    {
        std::optional<KeyType> key;
        decoder >> key;
        if (!key)
            return std::nullopt;

        std::optional<ValueType> value;
        decoder >>value;
        if (!value)
            return std::nullopt;

        return {{ WTFMove(*key), WTFMove(*value) }};
    }
};

template<bool fixedSizeElements, typename T, size_t inlineCapacity> struct VectorCoder;

template<typename T, size_t inlineCapacity> struct VectorCoder<false, T, inlineCapacity> {
    template<typename Encoder>
    static void encodeForPersistence(Encoder& encoder, const Vector<T, inlineCapacity>& vector)
    {
        encoder << static_cast<uint64_t>(vector.size());
        for (size_t i = 0; i < vector.size(); ++i)
            encoder << vector[i];
    }

    template<typename Decoder>
    static std::optional<Vector<T, inlineCapacity>> decodeForPersistence(Decoder& decoder)
    {
        std::optional<uint64_t> size;
        decoder >> size;
        if (!size)
            return std::nullopt;

        Vector<T, inlineCapacity> tmp;
        for (size_t i = 0; i < *size; ++i) {
            std::optional<T> element;
            decoder >> element;
            if (!element)
                return std::nullopt;
            tmp.append(WTFMove(*element));
        }

        tmp.shrinkToFit();
        return tmp;
    }
};

template<typename T, size_t inlineCapacity> struct VectorCoder<true, T, inlineCapacity> {
    template<typename Encoder>
    static void encodeForPersistence(Encoder& encoder, const Vector<T, inlineCapacity>& vector)
    {
        encoder << static_cast<uint64_t>(vector.size());
        encoder.encodeFixedLengthData(asBytes(vector.span()));
    }
    
    template<typename Decoder>
    static std::optional<Vector<T, inlineCapacity>> decodeForPersistence(Decoder& decoder)
    {
        std::optional<uint64_t> decodedSize;
        decoder >> decodedSize;
        if (!decodedSize)
            return std::nullopt;

        if (!isInBounds<size_t>(*decodedSize))
            return std::nullopt;

        auto size = static_cast<size_t>(*decodedSize);

        // Since we know the total size of the elements, we can allocate the vector in
        // one fell swoop. Before allocating we must however make sure that the decoder buffer
        // is big enough.
        if (!decoder.template bufferIsLargeEnoughToContain<T>(size))
            return std::nullopt;

        Vector<T, inlineCapacity> temp;
        temp.grow(size);

        if (!decoder.decodeFixedLengthData(temp.mutableSpan()))
            return std::nullopt;

        return temp;
    }
};

template<typename T, size_t inlineCapacity> struct Coder<Vector<T, inlineCapacity>> : VectorCoder<std::is_arithmetic<T>::value, T, inlineCapacity> { };

template<typename KeyArg, typename MappedArg, typename HashArg, typename KeyTraitsArg, typename MappedTraitsArg> struct Coder<HashMap<KeyArg, MappedArg, HashArg, KeyTraitsArg, MappedTraitsArg>> {
    typedef HashMap<KeyArg, MappedArg, HashArg, KeyTraitsArg, MappedTraitsArg> HashMapType;

    template<typename Encoder>
    static void encodeForPersistence(Encoder& encoder, const HashMapType& hashMap)
    {
        encoder << static_cast<uint64_t>(hashMap.size());
        for (typename HashMapType::const_iterator it = hashMap.begin(), end = hashMap.end(); it != end; ++it)
            encoder << *it;
    }

    template<typename Decoder>
    static std::optional<HashMapType> decodeForPersistence(Decoder& decoder)
    {
        std::optional<uint64_t> hashMapSize;
        decoder >> hashMapSize;
        if (!hashMapSize)
            return std::nullopt;

        HashMapType tempHashMap;
        tempHashMap.reserveInitialCapacity(static_cast<unsigned>(*hashMapSize));
        for (uint64_t i = 0; i < *hashMapSize; ++i) {
            std::optional<KeyArg> key;
            decoder >> key;
            if (!key)
                return std::nullopt;
            std::optional<MappedArg> value;
            decoder >> value;
            if (!value)
                return std::nullopt;

            if (!tempHashMap.add(WTFMove(*key), WTFMove(*value)).isNewEntry) {
                // The hash map already has the specified key, bail.
                return std::nullopt;
            }
        }

        return tempHashMap;
    }
};

template<typename KeyArg, typename HashArg, typename KeyTraitsArg> struct Coder<HashSet<KeyArg, HashArg, KeyTraitsArg>> {
    typedef HashSet<KeyArg, HashArg, KeyTraitsArg> HashSetType;

    template<typename Encoder>
    static void encodeForPersistence(Encoder& encoder, const HashSetType& hashSet)
    {
        encoder << static_cast<uint64_t>(hashSet.size());
        for (typename HashSetType::const_iterator it = hashSet.begin(), end = hashSet.end(); it != end; ++it)
            encoder << *it;
    }

    template<typename Decoder>
    static std::optional<HashSetType> decodeForPersistence(Decoder& decoder)
    {
        std::optional<uint64_t> hashSetSize;
        decoder >> hashSetSize;
        if (!hashSetSize)
            return std::nullopt;

        HashSetType tempHashSet;
        for (uint64_t i = 0; i < *hashSetSize; ++i) {
            std::optional<KeyArg> key;
            decoder >> key;
            if (!key)
                return std::nullopt;

            if (!tempHashSet.add(WTFMove(*key)).isNewEntry) {
                // The hash map already has the specified key, bail.
                return std::nullopt;
            }
        }

        return tempHashSet;
    }
};

#define DECLARE_CODER(class) \
template<> struct Coder<class> { \
    WTF_EXPORT_PRIVATE static void encodeForPersistence(Encoder&, const class&); \
    WTF_EXPORT_PRIVATE static std::optional<class> decodeForPersistence(Decoder&); \
}

DECLARE_CODER(AtomString);
DECLARE_CODER(CString);
DECLARE_CODER(Seconds);
DECLARE_CODER(String);
DECLARE_CODER(SHA1::Digest);
DECLARE_CODER(URL);
DECLARE_CODER(WallTime);

#undef DECLARE_CODER

}
