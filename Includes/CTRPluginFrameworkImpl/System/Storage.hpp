#ifndef CTRPLUGINFRAMEWORKIMPL_STORAGE_HPP
#define CTRPLUGINFRAMEWORKIMPL_STORAGE_HPP

#include <cstring>
#include "types.h"
#include "ctrulib/allocator/linear.h"

namespace CTRPluginFramework
{
    // Simplistic container class, zero safety
    template <typename T>
    class Storage
    {
    public:

        /* Disabled so dev is forced to set the capacity
        Storage() :
            _itemCount(0)
        {
            _pool = new T[10];
            if (_pool != nullptr)
                _capacity = 10;
            else
                _capacity = 0;
        }*/

        Storage(u32 capacity) :
            _itemCount(0)
        {
            if (capacity * sizeof(T) < linearSpaceFree())
            {
                _pool = (T *)linearAlloc(capacity * sizeof(T));

                if (_pool != nullptr)
                {
                    _linear = true;
                    _capacity = capacity;
                    return;
                }
            }

            _pool = new T[capacity];
            if (_pool != nullptr)
                _capacity = capacity;
            else
                _capacity = 0;
        }

        ~Storage()
        {
            if (_pool != nullptr)
            {
                if (_linear)
                    linearFree(_pool);
                else
                    delete[] _pool;
            }
        }

        T     *begin(void)
        {
            if (_pool == nullptr)
                return (nullptr);

            return (&_pool[0]);
        }

        T     *end(void)
        {
            if (_pool == nullptr)
                return (nullptr);

            return (&_pool[_itemCount]);
        }

        void  *data(void) const
        {
            return ((void *)_pool);
        }

        // Force item count, memory isn't initilized
        void    resize(u32 size)
        {
            if (size > _capacity)
            {
                return;
                /*
                T   *newPool = new T[size];

                if (newPool != nullptr)
                {
                    std::memcpy(newPool, _pool, sizeof(T) * _itemCount);
                    delete[] _pool;
                    _pool = newPool;
                    _itemCount = size;
                    _capacity = size;
                }*/
            }
            else
            {
                _itemCount = size;
            }
        }

        u32     size(void) const
        {
            return (_itemCount);
        }

        u32     capacity(void) const
        {
            return (_capacity);
        }

        bool    empty(void) const
        {
            return (_itemCount == 0);
        }

        T       &operator [](u32 index)
        {
            return (_pool[index]);
        }

        void    push_back(T &&item)
        {
            if (_itemCount == _capacity)
                return;
            _pool[_itemCount] = item;
            _itemCount++;
        }

    private:
        bool    _linear;
        T       *_pool;
        u32     _capacity;
        u32     _itemCount;
    };
}

#endif