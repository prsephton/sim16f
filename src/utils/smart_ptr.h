#ifndef __smart_ptr_h__
#define __smart_ptr_h__


class RC
{
    private:
    int count;   // Reference count

    public:
    void AddRef() {
    	count++;
    }

    int Release() {
    	return --count;
    }
};

template < class T > class SmartPtr
{
  private:
    T*  pData;       // pointer
    RC* reference;   // Reference count

  public:
    SmartPtr();
    SmartPtr(T* pValue);
    SmartPtr(const SmartPtr<T>& sp);
    ~SmartPtr();


    const T& operator* () const { return *pData; }
    T& operator* () { return *pData; }
    T* operator-> () { return pData; }
    operator bool() const { return (pData != 0); }

    SmartPtr<T>& operator = (const SmartPtr<T>& sp);
};

#endif
