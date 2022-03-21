/*
* If not stated otherwise in this file or this component's LICENSE file the
* following copyright and licenses apply:
*
* Copyright 2011 RDK Management
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/ 

 

#ifndef __VL_VECTOR_H
#define __VL_VECTOR_H

#define VECTOR_CAPACITY     50
#define VECTOR_INCREMENT    10


//generic Vector class
class vlVector
{
public:
    typedef unsigned long* iterator;
    typedef unsigned long element;

    vlVector(int capacity = VECTOR_CAPACITY, int increment = VECTOR_INCREMENT);
    virtual ~vlVector();

    iterator begin() const;
    iterator end() const;
    bool remove(int index);
    bool removeElement(element ele);
    iterator erase(iterator it);
    iterator erase(iterator first, iterator last);

    int size() const;
    int capacity() const;
    void clear();
    bool empty() const;
    int search(element ele) const;
    bool operator==(const vlVector& rhs) const;
    bool operator!=(const vlVector& rhs) const;

#define push_back   add             //for backward compatibility

    void add(element ele);

    element& operator[](int pos) const;
    element& operator[](int pos);

protected:
    int m_capacity;
    int m_increment;
    int m_size;

protected:
    bool allocMemory(const int nCapacity);
    void delMemory();
    element *elements;
};


//byteVector class
class vlByteVector
{
public:
    typedef unsigned char* iterator;
    typedef unsigned char element;

    vlByteVector(int capacity = VECTOR_CAPACITY, int increment = VECTOR_INCREMENT);
    virtual ~vlByteVector();
    void setArray(const unsigned char *buf, const int nBytes);

    iterator begin() const;
    iterator end() const;
    bool remove(int index);
    bool removeElement(element ele);
    iterator erase(iterator it);
    iterator erase(iterator first, iterator last);

    int size() const;
    int capacity() const;
    void clear();
    bool empty() const;
    int search(element ele) const;
    bool operator==(const vlByteVector& rhs) const;
    bool operator!=(const vlByteVector& rhs) const;

#define push_back   add             //for backward compatibility

    void add(element ele);

    element operator[](int pos) const;

protected:
    int m_capacity;
    int m_increment;
    int m_size;

protected:
    bool allocMemory(const int nCapacity);
    void delMemory();
    element *elements;
};


//shortVector class
class vlShortVector
{
public:
    typedef unsigned short* iterator;
    typedef unsigned short element;

    vlShortVector(int capacity = VECTOR_CAPACITY, int increment = VECTOR_INCREMENT);
    virtual ~vlShortVector();

    iterator begin() const;
    iterator end() const;
    bool remove(int index);
    bool removeElement(element ele);
    iterator erase(iterator it);
    iterator erase(iterator first, iterator last);

    int size() const;
    int capacity() const;
    void clear();
    bool empty() const;
    int search(element ele) const;
    bool operator==(const vlShortVector& rhs) const;
    bool operator!=(const vlShortVector& rhs) const;

#define push_back   add             //for backward compatibility

    void add(element ele);

    element operator[](int pos) const;

private:
    int m_capacity;
    int m_increment;
    int m_size;

protected:
    bool allocMemory(const int nCapacity);
    void delMemory();
    element *elements;
};


//Other vector classes

//boolVector class
#define vlBoolVector    vlByteVector
//intVector class
#define vlIntVector     vlVector
//longVector class
#define vlLongVector    vlVector

// vlGuid
struct vlGuid
{
    vlGuid(const unsigned char * _guid = NULL);
    unsigned char m_guid[8];
    bool operator == (const vlGuid& rhs) const;
    bool operator != (const vlGuid& rhs) const;
};

// guid vector
class vlGuidVector : public vlVector
{
private:
    unsigned long mdebugSize; //for debugging purpose only
public:
    vlGuidVector( int initialCapacity );
    vlGuidVector( );
    vlGuidVector(const vlGuidVector& rhs);
    ~vlGuidVector( );

    void clear();
    vlGuidVector& operator = (const vlGuidVector& rhs);
    bool operator == (const vlGuidVector& rhs) const;
    bool operator != (const vlGuidVector& rhs) const;
private:
    void copyValue(const vlGuidVector& rhs);
};

//ProtectedVector class
// forward decl
struct _cVector;
typedef struct _cVector cVector;

class  vlProtectedVector
{
public:
    // protected vector should NOT have iterator
    typedef unsigned long element;

    vlProtectedVector(int capacity = VECTOR_CAPACITY, int increment = VECTOR_INCREMENT);
    virtual ~vlProtectedVector();

    bool remove(int index);
    bool removeElement(element ele);

    int size() const;
    int capacity() const;
    void clear();
    bool empty() const;
    int search(element ele) const;
    bool operator==(const vlVector& rhs) const;
    bool operator!=(const vlVector& rhs) const;

    void add(element ele);
    void setElementAt(int index, element ele);

    // elements cannot be changed
    // objects (if any) pointed to by element may be changed
    // use setElementAt() to change elements
    element operator[](int pos) const;
    element operator[](int pos);

    void lock() const;
    void unlock() const;

private:
    int m_capacity;
    int m_increment;
    int m_size;

protected:
    cVector * m_pVector;
};

#endif //__VL_VECTOR_H
