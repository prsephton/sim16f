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
    SmartPtr() : pData(0), reference(0) {
         reference = new RC();
         reference->AddRef();
     }
    SmartPtr(T* pValue) : pData(pValue), reference(0) {
         reference = new RC();
         reference->AddRef();
     }
    SmartPtr(const SmartPtr<T>& sp) : pData(sp.pData), reference(sp.reference) {
         reference->AddRef();
     }
    ~SmartPtr() {
    	if(reference && reference->Release() == 0) {
    		delete pData; delete reference; reference = NULL;
        }
    }


    void incRef() { reference->AddRef(); }   // prevent disposal when out of scope
    const T& operator* () const { return *pData; }
    T& operator* () { return *pData; }
    T* operator-> () { return pData; }
    operator bool() const { return (pData != 0); }
    bool operator < (SmartPtr<T> &other) const {
    	return (long)(void *)pData < (long)(void *)(other.operator ->());
    }
    bool operator == (SmartPtr<T> &other) const {
    	return (long)(void *)pData == (long)(void *)(other.operator ->());
    }

    SmartPtr<T>& operator = (const SmartPtr<T>& sp)
    {
        // Assignment operator
        if (this != &sp) // Avoid self assignment
        {
            if(reference && reference->Release() == 0) {
                delete pData; delete reference;
            }

            pData = sp.pData;
            reference = sp.reference;
            reference->AddRef();
        }
        return *this;
    }
};

#endif






