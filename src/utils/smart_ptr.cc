#include "smart_ptr.h"

template < class T >
 SmartPtr<T>::SmartPtr() : pData(0), reference(0)
 {
     // Create a new reference
     reference = new RC();
     // Increment the reference count
     reference->AddRef();
 }

template < class T >
 SmartPtr<T>::SmartPtr(T* pValue) : pData(pValue), reference(0)
 {
     // Create a new reference
     reference = new RC();
     // Increment the reference count
     reference->AddRef();
 }

template < class T >
 SmartPtr<T>::SmartPtr(const SmartPtr<T>& sp) : pData(sp.pData), reference(sp.reference)
 {
     // Copy constructor
     // Copy the data and reference pointer
     // and increment the reference count
     reference->AddRef();
 }

template < class T >
 SmartPtr<T>::~SmartPtr()
  {
     // Destructor
     // Decrement the reference count
     // if reference become zero delete the data
     if(reference && reference->Release() == 0)
     {
         delete pData;
         delete reference;
     }
 }

template < class T >
 SmartPtr<T>& SmartPtr<T>::operator = (const SmartPtr<T>& sp)
 {
     // Assignment operator
     if (this != &sp) // Avoid self assignment
     {
         // Decrement the old reference count
         // if reference become zero delete the old data
         if(reference && reference->Release() == 0)
         {
             delete pData;
             delete reference;
         }

         // Copy the data and reference pointer
         // and increment the reference count
         pData = sp.pData;
         reference = sp.reference;
         reference->AddRef();
     }
     return *this;
 }
