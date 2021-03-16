/**
 * Provide std::vector container with hashed index mapping.
 *
 * This file provides classes and methods for the handling of groups of mesh
 * subcomponents...
 *
 *@file
 */

//===============================================
// Include Guards
#ifndef ARRAYSTRUCTS_H_INCLUDED
#define ARRAYSTRUCTS_H_INCLUDED

//===============================================
// Levels of debuging Guards
#ifdef DEBUGLVL2 // All Debugging calls
#define DEBUGLVL1
#define TEST_RANGE // deprecated
#define TEST_ARRAYSTRUCT

#endif

#ifdef DEBUGLVL1 // Debugging of new features.
#ifndef SAFE_ACCESS
#define SAFE_ACCESS
#endif
#endif

//=================================
// forward declared dependencies
// 		class foo; //when you only need a pointer not the actual object
// 		and to avoid circular dependencies

//=================================
// included dependencies
#ifdef DBG_MEMLEAK
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#include <stdlib.h>
#endif // DBG_MEMLEAK

#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>
#if defined(USE_BOOST) && defined(USE_STACKTRACE)
#include <boost/stacktrace.hpp>
namespace stacktrace = boost::stacktrace;
#endif

#include "warning.hpp"

//==================================
// Code
// NOTE: function in a class definition are IMPLICITELY INLINED
//       ie replaced by their code at compile time

namespace rsvs3d
{
namespace constants
{
static const int __notfound = -1;
static const int __failure = -1;
static const int __success = 0;
} // namespace constants
namespace logicals
{
inline bool __isfound(int f)
{
    return f != constants::__notfound;
}
} // namespace logicals
} // namespace rsvs3d

template <class T> class ArrayStruct;
template <class T, class Q, class R = int> class HashedVector;
template <class T> class SnakStruct;

typedef unsigned int unsigned_int;

// Forward declared templated functions
template <class T> int TestTemplate_ArrayStruct();
bool CompareFuncOut(std::function<void()> func1, std::function<void()> func2);
template <typename T> inline void sort(std::vector<T> &vec);
template <typename T> inline void unique(std::vector<T> &vec);
// template <typename T> inline void set_intersection(std::vector<T>
// &targVec,std::vector<T> &vec1,std::vector<T> &vec2,bool isSort=true);
template <typename T>
inline void set_intersection(std::vector<T> &targVec, const std::vector<T> &vec1, const std::vector<T> &vec2,
                             bool isSort);
template <class T>
std::vector<int> FindSubList(const std::vector<T> &keyFind, const std::vector<T> &keyList,
                             std::unordered_multimap<T, int> &hashTable);
template <class T>
std::vector<int> FindSubList(const std::vector<T> &keyFind, const std::vector<T> &keyList,
                             const std::unordered_multimap<T, int> &hashTable);
template <class T, class Q>
void HashVector(const std::vector<T> &elems, std::unordered_multimap<T, Q> &hashTable,
                const std::vector<Q> &targElems = {});
template <class T> int FindSub(const T &key, const std::unordered_multimap<T, int> &hashTable);
template <class T> void ConcatenateVector(std::vector<T> &vecRoot, const std::vector<T> &vecConcat);
template <class T, class R> std::vector<R> ReturnDataEqualRange(T key, const std::unordered_multimap<T, R> &hashTable);
template <class T, class R>
void ReturnDataEqualRange(T key, const std::unordered_multimap<T, R> &hashTable, std::vector<R> &subList);

// Templates
template <class T> class ArrayStruct
{
  protected:
    int maxIndex;
    int isHash = 0;
    int isSetMI = 0;
    bool readyforuse = false;
    bool isInMesh = false; // used to change behaviour if not in a mesh.

    std::vector<T> elems;                        // std::vector of elements (structures)
    std::unordered_multimap<int, int> hashTable; // Hash Table of indexed elements
    void ForceArrayReady();
    void SetLastIndex()
    {
        isSetMI = 1;
        maxIndex = elems.back().index;
    };

  public:
    friend class mesh;
    friend class snake;
    friend class surf;
    friend int TestTemplate_ArrayStruct<T>();

    void disp() const;
    void disp(const std::vector<int> &subs) const;
    void disp(int iStart, int iEnd) const;
    int find(int key, bool noWarn = false) const;
    std::vector<int> find_list(const std::vector<int> &key, bool noWarn = false) const;
    inline int GetMaxIndex() const;
    inline void Init(int n);
    bool isready() const
    {
        return (readyforuse);
    };
    bool checkready();
    void Concatenate(const ArrayStruct<T> &other);
    void PopulateIndices();
    void SetMaxIndex();
    void HashArray();
    void PrepareForUse();
    void ChangeIndices(int nVert, int nEdge, int nSurf, int nVolu);
    void write(FILE *fid) const;
    void read(FILE *fid);
    void remove(std::vector<int> delInd);
    void TightenConnectivity();
    // methods needed from std::vector
    inline int size() const;
    inline int capacity() const;
    inline void assign(int n, T &newelem);
    inline void push_back(T &newelem);
    inline void reserve(int n);
    inline void clear();
    // Operators
    void issafeaccess(const int a)
    {
#ifdef SAFE_ACCESS // adds a check in debug mode
        if ((unsigned_int(a) >= elems.size()) || (0 > a))
        {
            std::cerr << "Error in " << __PRETTY_FUNCTION__ << std::endl;
#ifdef USE_STACKTRACE
            std::cerr << stacktrace::stacktrace() << std::endl;
#endif
            std::cerr << "Attempt to access position " << a << " in std::vector of size " << elems.size() << std::endl;
            // dbg::fail(__PRETTY_FUNCTION__,"index out of range");
            RSVS3D_ERROR_RANGE(" Index is out of range");
        }
#endif // SAFE_ACCESS
    }

    const T *operator()(const int a) const
    {
        // () Operator returns a constant pointer to the corresponding elems.
        // Cannot be used on the left hand side and can't be used to edit data in
        // elems
#ifdef SAFE_ACCESS // adds a check in debug mode
        if ((unsigned_int(a) >= elems.size()) || (0 > a))
        {
            std::cerr << "Error in " << __PRETTY_FUNCTION__ << std::endl;
#ifdef USE_STACKTRACE
            std::cerr << stacktrace::stacktrace() << std::endl;
#endif
            std::cerr << "Attempt to access position " << a << " in std::vector of size " << elems.size() << std::endl;
            // dbg::fail(__PRETTY_FUNCTION__,"index out of range");
            RSVS3D_ERROR_RANGE(" Index is out of range");
        }
#endif // SAFE_ACCESS
        return (&(elems[a]));
    }
    const T *isearch(const int b) const
    {
        // () Operator returns a constant pointer to the corresponding elems.
        // Cannot be used on the left hand side and can't be used to edit data in
        // elems
        int a = this->find(b);
#ifdef SAFE_ACCESS // adds a check in debug mode
        if ((unsigned_int(a) >= elems.size()) || (0 > a))
        {
            std::cerr << "Error in " << __PRETTY_FUNCTION__ << std::endl;
#ifdef USE_STACKTRACE
            std::cerr << stacktrace::stacktrace() << std::endl;
#endif
            std::cerr << "Attempt to access index " << b << " at position " << a << " in std::vector of size "
                      << elems.size() << std::endl;
            // dbg::fail(__PRETTY_FUNCTION__,"index out of range");
            RSVS3D_ERROR_RANGE(" Index is out of range");
        }
#endif // SAFE_ACCESS
        return (&(elems[a]));
    }

    T &operator[](const int a)
    {
        // [] Operator returns a reference to the corresponding elems.
#ifdef SAFE_ACCESS // adds a check in debug mode
        if ((unsigned_int(a) >= elems.size()) | (0 > a))
        {
            std::cerr << "Error in " << __PRETTY_FUNCTION__ << std::endl;
#ifdef USE_STACKTRACE
            std::cerr << stacktrace::stacktrace() << std::endl;
#endif
            std::cerr << "Attempt to access position " << a << " in std::vector of size " << elems.size() << std::endl;
            // dbg::fail(__PRETTY_FUNCTION__,"index out of range");
            RSVS3D_ERROR_RANGE("Index is out of range");
        }
#endif // SAFE_ACCESS
        isHash = 0;
        isSetMI = 0;
        readyforuse = false;
        return (elems[a]);
    }
};

template <class T> class SnakStruct : public ArrayStruct<T>
{
  protected:
    using ArrayStruct<T>::elems;
    using ArrayStruct<T>::readyforuse;

    std::unordered_multimap<int, int> hashParent;
    int isHashParent = 0;

  public:
    friend class snake;

    // inline int KeyParent(int a) const ;
    int findparent(int key) const;
    void findsiblings(int key, std::vector<int> &siblings) const;
    int countparent(int key) const
    {
        return (hashParent.count(key));
    };
    void HashParent();
    void DeHashParent(const int pos);
    bool memberIsHashParent(const int pos) const;
    inline void Init(int n);
    // Functions that need modification
    inline void push_back(T &newelem);
    inline void clear();
    bool checkready();
    void ForceArrayReady();
    void PrepareForUse();
    void Concatenate(const SnakStruct<T> &other);
    void remove(const std::vector<int> &sub);
    T &operator[](const int a)
    {
        isHashParent = 0;
        return (ArrayStruct<T>::operator[](a));
    }
};

template <class T> class ModiftrackArray : public ArrayStruct<T>
{
  protected:
    using ArrayStruct<T>::elems;
    friend class mesh;
    friend class snake;

  public:
    void SetNoModif();
    void ReturnModifInd(std::vector<int> &vecind);
    void ReturnModifLog(std::vector<bool> &modiflog);
    T &operator[](const int a)
    {
#ifdef SAFE_ACCESS
        ArrayStruct<T>::issafeaccess(a);
#endif
        elems[a].isModif = true;
        return (ArrayStruct<T>::operator[](a));
    }
};

template <class T, class Q, class R> class HashedVector
{ // container for
  public:
    std::vector<T> vec;
    std::unordered_multimap<T, R> hashTable;
    bool isHash = true;

    inline void GenerateHash();
    inline int find(const T key) const;
    inline std::vector<int> findall(const T key) const;
    inline int count(const T key) const;
    std::vector<int> count(const std::vector<T> &key) const;
    inline std::vector<int> find_list(const std::vector<T> &key) const;
    bool operator()(const Q &key) const;
    inline bool IsInVec(const Q &key) const;
    T &operator[](const int a)
    {
        return this->vec[a];
    }
    const T &operator[](const int a) const
    {
        return this->vec[a];
    }

    void reserve(const size_t a)
    {
        this->vec.reserve(a);
        this->hashTable.reserve(a);
    }
    void assign(const size_t a, const T &elm)
    {
        this->vec.assign(a, elm);
        this->isHash = false;
    }
    void push_back(const T &elm)
    {
        this->vec.push_back(elm);
        R t = this->vec.size() - 1;
        this->hashTable.emplace(elm, t);
    }
    void clear()
    {
        this->vec.clear();
        this->isHash = true;
        this->hashTable.clear();
    }
    size_t size() const
    {
        return this->vec.size();
    }
};

template <class T, class Q, class R> class HashedMap : public HashedVector<T, Q, R>
{
  public:
    using HashedVector<T, Q, R>::hashTable;
    using HashedVector<T, Q, R>::vec;
    using HashedVector<T, Q, R>::isHash;

    std::vector<R> targ;
    inline void GenerateHash();
};

template <class T, class Q, class R, class S> class HashedVectorPair : public HashedVector<T, Q, R>
{
    S defaultVal = S(0);

  public:
    using HashedVector<T, Q, R>::hashTable;
    using HashedVector<T, Q, R>::vec;
    using HashedVector<T, Q, R>::isHash;
    std::vector<S> targ;

    S &operator()(const T &elm)
    {
        int pos = this->HashedVector<T, Q, R>::find(elm);
        if (pos == rsvs3d::constants::__notfound)
        {
            this->push_back(elm, S(0));
            pos = this->size() - 1;
        }
        return this->targ[pos];
    }
    const S &operator()(const T &elm) const
    {
        int pos = this->HashedVector<T, Q, R>::find(elm);
        if (pos == rsvs3d::constants::__notfound)
        {
            return this->defaultVal;
        }
        return this->targ[pos];
    }
    void reserve(const size_t a)
    {
        this->HashedVector<T, Q, R>::reserve(a);
        this->vec.reserve(a);
    }
    void assign(const size_t a, const T &elmVec, const S &elmTarg)
    {
        this->HashedVector<T, Q, R>::assign(a, elmVec);
        this->targ.assign(a, elmTarg);
    }
    void push_back(const T &elmVec, const S &elmTarg)
    {
        this->HashedVector<T, Q, R>::push_back(elmVec);
        this->targ.push_back(elmTarg);
    }
    void clear()
    {
        this->HashedVector<T, Q, R>::clear();
        this->targ.clear();
    }
};

template <class T, class Q, class R = int> class HashedVectorSafe : protected HashedVector<T, Q, R>
{ // container for
  protected:
    using HashedVector<T, Q, R>::vec;
    using HashedVector<T, Q, R>::isHash;
    using HashedVector<T, Q, R>::hashTable;

  public:
    using HashedVector<T, Q, R>::GenerateHash;
    using HashedVector<T, Q, R>::find;
    using HashedVector<T, Q, R>::findall;
    using HashedVector<T, Q, R>::count;
    using HashedVector<T, Q, R>::find_list;
    using HashedVector<T, Q, R>::operator();
    using HashedVector<T, Q, R>::IsInVec;

    void operator=(const std::vector<T> &a)
    {
        vec = a;
        isHash = false;
    }
    void operator=(const HashedVector<T, Q> &a)
    {
        vec = a.vec;
        isHash = a.isHash;
        hashTable = a.hashTable;
    }
    T &operator[](const int a)
    {
        // [] Operator returns a reference to the corresponding elems.
#ifdef SAFE_ACCESS // adds a check in debug mode
        if ((unsigned_int(a) >= vec.size()) | (0 > a))
        {
            std::cerr << "Error in " << __PRETTY_FUNCTION__ << std::endl;
            RSVS3D_ERROR_RANGE(" : Index is out of range");
        }
#endif // SAFE_ACCESS
        isHash = 0;
        return (vec[a]);
    }
    const T &operator[](const int a) const
    {
        // [] Operator returns a reference to the corresponding elems.
#ifdef SAFE_ACCESS // adds a check in debug mode
        if ((unsigned_int(a) >= vec.size()) | (0 > a))
        {
            std::cerr << "Error in " << __PRETTY_FUNCTION__ << std::endl;
            RSVS3D_ERROR_RANGE(" : Index is out of range");
        }
#endif // SAFE_ACCESS
        return (vec[a]);
    }
    const T &isearch(const int b) const
    {
        // () Operator returns a constant pointer to the corresponding elems.
        // Cannot be used on the left hand side and can't be used to edit data in
        // elems
        int a = this->find(b);
#ifdef SAFE_ACCESS // adds a check in debug mode
        if ((unsigned_int(a) >= vec.size()) | (0 > a))
        {
            std::cerr << "Error in " << __PRETTY_FUNCTION__ << std::endl;
            RSVS3D_ERROR_RANGE(" : Index is out of range");
        }
#endif // SAFE_ACCESS
        return (&(vec[a]));
    }
};

// Base class

class ArrayStructpart
{ // Abstract class to ensure interface is correct
  public:
    int index = 0;
    bool isBorder = false;

    virtual void disp() const = 0;
    virtual int Key() const = 0;
    virtual void ChangeIndices(int nVert, int nEdge, int nSurf, int nVolu) = 0;
    virtual void PrepareForUse() = 0;
    virtual bool isready(bool isInMesh) const = 0;
    virtual void read(FILE *fid) = 0;
    virtual void write(FILE *fid) const = 0;
    virtual void TightenConnectivity() = 0;
    // virtual operator=( meshpart* other)=0 ;
};

class snakpart
{ // required functions for parts of snake
  public:
    virtual int KeyParent() const = 0;
};

class modiftrackpart
{ // required functions for parts of snake
  protected:
    bool isModif = true;

  public:
    bool returnIsModif() const
    {
        return (isModif);
    }
};

// functions
template <class T> bool CompareDisp(T *mesh1, T *mesh2);
template <class T> int TestReadiness(T &stackT, const char *txt, bool errTarg, bool errTargCheck);
template <class T> void DisplayVector(std::vector<T> vec);
template <class T> void DisplayVectorStatistics(std::vector<T> vec);
template <class T> void PrintVector(std::vector<T> vec, std::ostream &streamout);

template <class T, class R>
R ConcatenateVectorField(const ArrayStruct<T> &arrayIn, R T::*mp, const std::vector<int> &subList);
template <class T, class R>
std::vector<R> ConcatenateScalarField(const ArrayStruct<T> &arrayIn, R T::*mp, const std::vector<int> &subList);
template <class T, class R> R ConcatenateVectorField(const ArrayStruct<T> &arrayIn, R T::*mp, int rStart, int rEnd);

template <class T, class R>
std::vector<R> ConcatenateScalarField(const ArrayStruct<T> &arrayIn, R T::*mp, int rStart, int rEnd);

template <class T, class R, class U, class V>
void OperArrayStructMethod(const ArrayStruct<T> &arrayIn, const std::vector<int> &subList, R T::*mp, U &out, V oper);
template <template <class Q, class R> class T, class Q, class R> void EraseKeyPair(T<Q, R> hashTable, Q key, R pos);

// test functions

// template <class T> bool CompareDisp(T *mesh1,T *mesh2);
// bool CompareFuncOut(function<void()> mesh1, function<void()> mesh2);

#include "arraystructures_incl.cpp"
#include "snakstruct_incl.cpp"

#endif // ARRAYSTRUCTS_H_INCLUDED

// Regexp to add std everywhere
// ([^:])((exception|cerr|endl|cout|vector|unordered_multimap|string|stringstream)[^a-zA-Z0-9_])
// \1std::\2
