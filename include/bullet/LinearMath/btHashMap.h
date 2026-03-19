

#ifndef BT_HASH_MAP_H
#define BT_HASH_MAP_H

#include <string>
#include "btAlignedObjectArray.h"


struct btHashString
{
	std::string m_string1;
	unsigned int m_hash;

	SIMD_FORCE_INLINE unsigned int getHash() const
	{
		return m_hash;
	}

	btHashString()
	{
		m_string1 = "";
		m_hash = 0;
	}
	btHashString(const char* name)
		: m_string1(name)
	{
		
		static const unsigned int InitialFNV = 2166136261u;
		static const unsigned int FNVMultiple = 16777619u;

		
		unsigned int hash = InitialFNV;

		for (int i = 0; m_string1.c_str()[i]; i++)
		{
			hash = hash ^ (m_string1.c_str()[i]); 
			hash = hash * FNVMultiple;            
		}
		m_hash = hash;
	}

	bool equals(const btHashString& other) const
	{
		return (m_string1 == other.m_string1);
	}
};

const int BT_HASH_NULL = 0xffffffff;

class btHashInt
{
	int m_uid;

public:
	btHashInt()
	{
	}

	btHashInt(int uid) : m_uid(uid)
	{
	}

	int getUid1() const
	{
		return m_uid;
	}

	void setUid1(int uid)
	{
		m_uid = uid;
	}

	bool equals(const btHashInt& other) const
	{
		return getUid1() == other.getUid1();
	}
	
	SIMD_FORCE_INLINE unsigned int getHash() const
	{
		unsigned int key = m_uid;
		
		key += ~(key << 15);
		key ^= (key >> 10);
		key += (key << 3);
		key ^= (key >> 6);
		key += ~(key << 11);
		key ^= (key >> 16);

		return key;
	}
};

class btHashPtr
{
	union {
		const void* m_pointer;
		unsigned int m_hashValues[2];
	};

public:
	btHashPtr(const void* ptr)
		: m_pointer(ptr)
	{
	}

	const void* getPointer() const
	{
		return m_pointer;
	}

	bool equals(const btHashPtr& other) const
	{
		return getPointer() == other.getPointer();
	}

	
	SIMD_FORCE_INLINE unsigned int getHash() const
	{
		const bool VOID_IS_8 = ((sizeof(void*) == 8));

		unsigned int key = VOID_IS_8 ? m_hashValues[0] + m_hashValues[1] : m_hashValues[0];
		
		key += ~(key << 15);
		key ^= (key >> 10);
		key += (key << 3);
		key ^= (key >> 6);
		key += ~(key << 11);
		key ^= (key >> 16);
		return key;
	}
};

template <class Value>
class btHashKeyPtr
{
	int m_uid;

public:
	btHashKeyPtr(int uid) : m_uid(uid)
	{
	}

	int getUid1() const
	{
		return m_uid;
	}

	bool equals(const btHashKeyPtr<Value>& other) const
	{
		return getUid1() == other.getUid1();
	}

	
	SIMD_FORCE_INLINE unsigned int getHash() const
	{
		unsigned int key = m_uid;
		
		key += ~(key << 15);
		key ^= (key >> 10);
		key += (key << 3);
		key ^= (key >> 6);
		key += ~(key << 11);
		key ^= (key >> 16);
		return key;
	}
};

template <class Value>
class btHashKey
{
	int m_uid;

public:
	btHashKey(int uid) : m_uid(uid)
	{
	}

	int getUid1() const
	{
		return m_uid;
	}

	bool equals(const btHashKey<Value>& other) const
	{
		return getUid1() == other.getUid1();
	}
	
	SIMD_FORCE_INLINE unsigned int getHash() const
	{
		unsigned int key = m_uid;
		
		key += ~(key << 15);
		key ^= (key >> 10);
		key += (key << 3);
		key ^= (key >> 6);
		key += ~(key << 11);
		key ^= (key >> 16);
		return key;
	}
};



template <class Key, class Value>
class btHashMap
{
protected:
	btAlignedObjectArray<int> m_hashTable;
	btAlignedObjectArray<int> m_next;

	btAlignedObjectArray<Value> m_valueArray;
	btAlignedObjectArray<Key> m_keyArray;

	void growTables(const Key& )
	{
		int newCapacity = m_valueArray.capacity();

		if (m_hashTable.size() < newCapacity)
		{
			
			int curHashtableSize = m_hashTable.size();

			m_hashTable.resize(newCapacity);
			m_next.resize(newCapacity);

			int i;

			for (i = 0; i < newCapacity; ++i)
			{
				m_hashTable[i] = BT_HASH_NULL;
			}
			for (i = 0; i < newCapacity; ++i)
			{
				m_next[i] = BT_HASH_NULL;
			}

			for (i = 0; i < curHashtableSize; i++)
			{
				
				

				int hashValue = m_keyArray[i].getHash() & (m_valueArray.capacity() - 1);  
				m_next[i] = m_hashTable[hashValue];
				m_hashTable[hashValue] = i;
			}
		}
	}

public:
	void insert(const Key& key, const Value& value)
	{
		int hash = key.getHash() & (m_valueArray.capacity() - 1);

		
		int index = findIndex(key);
		if (index != BT_HASH_NULL)
		{
			m_valueArray[index] = value;
			return;
		}

		int count = m_valueArray.size();
		int oldCapacity = m_valueArray.capacity();
		m_valueArray.push_back(value);
		m_keyArray.push_back(key);

		int newCapacity = m_valueArray.capacity();
		if (oldCapacity < newCapacity)
		{
			growTables(key);
			
			hash = key.getHash() & (m_valueArray.capacity() - 1);
		}
		m_next[count] = m_hashTable[hash];
		m_hashTable[hash] = count;
	}

	void remove(const Key& key)
	{
		int hash = key.getHash() & (m_valueArray.capacity() - 1);

		int pairIndex = findIndex(key);

		if (pairIndex == BT_HASH_NULL)
		{
			return;
		}

		
		int index = m_hashTable[hash];
		btAssert(index != BT_HASH_NULL);

		int previous = BT_HASH_NULL;
		while (index != pairIndex)
		{
			previous = index;
			index = m_next[index];
		}

		if (previous != BT_HASH_NULL)
		{
			btAssert(m_next[previous] == pairIndex);
			m_next[previous] = m_next[pairIndex];
		}
		else
		{
			m_hashTable[hash] = m_next[pairIndex];
		}

		
		
		

		int lastPairIndex = m_valueArray.size() - 1;

		
		if (lastPairIndex == pairIndex)
		{
			m_valueArray.pop_back();
			m_keyArray.pop_back();
			return;
		}

		
		int lastHash = m_keyArray[lastPairIndex].getHash() & (m_valueArray.capacity() - 1);

		index = m_hashTable[lastHash];
		btAssert(index != BT_HASH_NULL);

		previous = BT_HASH_NULL;
		while (index != lastPairIndex)
		{
			previous = index;
			index = m_next[index];
		}

		if (previous != BT_HASH_NULL)
		{
			btAssert(m_next[previous] == lastPairIndex);
			m_next[previous] = m_next[lastPairIndex];
		}
		else
		{
			m_hashTable[lastHash] = m_next[lastPairIndex];
		}

		
		m_valueArray[pairIndex] = m_valueArray[lastPairIndex];
		m_keyArray[pairIndex] = m_keyArray[lastPairIndex];

		
		m_next[pairIndex] = m_hashTable[lastHash];
		m_hashTable[lastHash] = pairIndex;

		m_valueArray.pop_back();
		m_keyArray.pop_back();
	}

	int size() const
	{
		return m_valueArray.size();
	}

	const Value* getAtIndex(int index) const
	{
		btAssert(index < m_valueArray.size());
		btAssert(index >= 0);
		if (index >= 0 && index < m_valueArray.size())
		{
			return &m_valueArray[index];
		}
		return 0;
	}

	Value* getAtIndex(int index)
	{
		btAssert(index < m_valueArray.size());
		btAssert(index >= 0);
		if (index >= 0 && index < m_valueArray.size())
		{
			return &m_valueArray[index];
		}
		return 0;
	}

	Key getKeyAtIndex(int index)
	{
		btAssert(index < m_keyArray.size());
		btAssert(index >= 0);
		return m_keyArray[index];
	}

	const Key getKeyAtIndex(int index) const
	{
		btAssert(index < m_keyArray.size());
		btAssert(index >= 0);
		return m_keyArray[index];
	}

	Value* operator[](const Key& key)
	{
		return find(key);
	}

	const Value* operator[](const Key& key) const
	{
		return find(key);
	}

	const Value* find(const Key& key) const
	{
		int index = findIndex(key);
		if (index == BT_HASH_NULL)
		{
			return NULL;
		}
		return &m_valueArray[index];
	}

	Value* find(const Key& key)
	{
		int index = findIndex(key);
		if (index == BT_HASH_NULL)
		{
			return NULL;
		}
		return &m_valueArray[index];
	}

	int findIndex(const Key& key) const
	{
		unsigned int hash = key.getHash() & (m_valueArray.capacity() - 1);

		if (hash >= (unsigned int)m_hashTable.size())
		{
			return BT_HASH_NULL;
		}

		int index = m_hashTable[hash];
		while ((index != BT_HASH_NULL) && key.equals(m_keyArray[index]) == false)
		{
			index = m_next[index];
		}
		return index;
	}

	void clear()
	{
		m_hashTable.clear();
		m_next.clear();
		m_valueArray.clear();
		m_keyArray.clear();
	}
};

#endif  
