#include <iostream>
#include <stdexcept>
#include <memory>
#include <algorithm>

template<class T>//, class A = std::allocator<T>
class Auto_array_adapter { // to be used with auto_ptr
    T* ptr;
    bool safe;
public:
    Auto_array_adapter(T* p) : ptr(p), safe(false) {}

    T& operator[](int n) { return ptr[n]; }
    operator T*() { safe = true; return ptr;}
    ~Auto_array_adapter() { if(!safe) delete[] ptr;}
private:
    Auto_array_adapter(const Auto_array_adapter&);
    Auto_array_adapter& operator=(const Auto_array_adapter&);
};

struct Range_error : std::out_of_range {
    int index;
    Range_error(int i) : out_of_range("Range error"), index(i) {}
};

template<class T, class A = std::allocator<T> >
class vector {
    A alloc;
    T* elem;
    int sz;
    int space;

public:
    typedef std::random_access_iterator_tag iterator_category;
    typedef T value_type;
    typedef unsigned long size_type;
    typedef std::ptrdiff_t difference_type;
    typedef T* pointer;
    typedef T& reference;
    typedef T* iterator;
    typedef const T* const_iterator;
    class checked_iterator;
    class const_checked_iterator;

public:

    vector() : sz(0), elem(0), space(0) {}

    explicit vector(int n, T def = T())
        : elem(alloc.allocate(n)), sz(n), space(n)
    {
        for(int i=0 ; i<n ; ++i) alloc.construct(&elem[i],def);
    }

    vector(const vector& v)
        : sz(v.sz), elem(alloc.allocate(v.sz))
    {
        for(int i=0 ; i<v.sz ; ++i) alloc.construct(&elem[i],v.elem[i]);
    }

    vector& operator=(const vector&);

    ~vector()
    {
        for(int i=0 ; i<sz ; ++i) alloc.destroy(&elem[i]);
        alloc.deallocate(elem,space);
    }

    T& at(int n)
    {
        if(n<0 || sz<=n) throw Range_error(n);
        return elem[n];
    }

    const T& at(int n) const
    {
        if(n<0 || sz<=n) throw Range_error(n);
        return elem[n];
    }

    T& operator[](int i) { return elem[i]; }
    const T& operator[](int i) const { return elem[i]; }

    void reserve(int newalloc);
    void resize(int newsize, T def = T());

    void push_back(const T&);
    void push_front(const T&);
    T& back() { return *(end()-1); }
    T& front() { return *begin(); }
    const T& back() const { return *(end()-1); }
    const T& front() const { return *begin(); }

    iterator begin() { return elem; }
    iterator end() { return elem+sz; }
    const_iterator begin() const { return elem; }
    const_iterator end() const { return elem+sz; }
    const_iterator cbegin() const { return begin(); }
    const_iterator cend() const { return end(); }
    iterator insert(iterator p, const T& val);
    iterator erase(iterator p);

    int size() const { return sz; }
    int capacity() const { return space; }

private:
    void move_back(const iterator&,const T&);
};

struct iterator_range_error : std::out_of_range {
    std::string where;
    iterator_range_error(const std::string& s) : out_of_range("iterator_range error"), where(s) {}
    const char* what() const throw() { return (std::string(out_of_range::what()) + " : " + where).c_str(); }
    ~iterator_range_error() throw() {}      // where need to be destroyed ( by the compiler generated destructor)
                                        // which is doesn't include throw(), so we need to rewrite it explicitly
                                        // whenever we add members that have destructors
};

template<class T, class A>
class vector<T,A>::const_checked_iterator {   // with pointer semantics
protected:
    const T* current;
    const vector<T,A>* vec_obj;
    /**
    * used a pointer (const vector<T,A>*) instead of a refrence to allow
    * assignment with the default meaning (shallow copy), and const is
    * there to prohibit modifing the data.
    */
public:
    const_checked_iterator(const vector<T,A>* v, T* p)
        :current(p), vec_obj(v) { check_position(p); }

    const_checked_iterator(const vector<T,A>* v, const T* p)
        :current(p), vec_obj(v) { check_position(p); }

    const T& operator*() const
    {
        if(vec_obj->elem+vec_obj->sz==current)
            throw iterator_range_error("derefrencing end()");
        return *current;
    }

    const_checked_iterator& operator++();
    const_checked_iterator operator++(int);

    const_checked_iterator& operator--();
    const_checked_iterator operator--(int);

    const_checked_iterator operator+(size_type);
    const_checked_iterator operator-(size_type);
    size_type operator-(typename vector<T,A>::const_checked_iterator);

    const_checked_iterator& operator+=(size_type);
    const_checked_iterator& operator-=(size_type);

    bool operator==(const const_checked_iterator& other) const
    {
        return current==other.current;
    }

    bool operator==(const iterator& other) const
    {
        return current==&*other;
    }

    bool operator==(const const_iterator& other) const
    {
        return current==&*other;
    }

    bool operator!=(const const_checked_iterator& other) const
    {
        return current!=other.current;
    }

    bool operator!=(const iterator& other) const
    {
        return current!=&*other;
    }

    bool operator!=(const const_iterator& other) const
    {
        return current!=&*other;
    }

private:

    void check_position(const T* p) throw()
    {
        if(vec_obj->elem+vec_obj->sz<p)throw iterator_range_error("intitializing checked_iterator() passed end()");
        if(p<vec_obj->elem) throw iterator_range_error("intitializing checked_iterator() before begin()");
    }

public:
    typedef typename vector<T,A>::iterator_category iterator_category;
    typedef typename vector<T,A>::value_type        value_type;
    typedef typename vector<T,A>::difference_type   difference_type;
    typedef typename vector<T,A>::pointer           pointer;
    typedef typename vector<T,A>::reference         reference;
};

template<class T, class A>
typename vector<T,A>::const_checked_iterator& vector<T,A>::const_checked_iterator::operator++()
{
    if(current==(vec_obj->elem+vec_obj->sz))
        throw iterator_range_error("surpasses end()");
    ++current;
    return *this;
}

template<class T, class A>
typename vector<T,A>::const_checked_iterator vector<T,A>::const_checked_iterator::operator++(int)
{
    if(current==(vec_obj->elem+vec_obj->sz))
        throw iterator_range_error("surpasses end()");
    const T* temp = this->current;
    ++current;
    return const_checked_iterator(vec_obj,temp);
}

template<class T, class A>
typename vector<T,A>::const_checked_iterator& vector<T,A>::const_checked_iterator::operator--()
{
    if(current==vec_obj->elem)
        throw iterator_range_error("precedes begin()");
    --current;
    return *this;
}

template<class T, class A>
typename vector<T,A>::const_checked_iterator vector<T,A>::const_checked_iterator::operator--(int)
{
    if(current==vec_obj->elem)
        throw iterator_range_error("precedes begin()");
    const T* temp = this->current;
    --current;
    return const_checked_iterator(vec_obj,temp);
}

template<class T, class A>
typename vector<T,A>::const_checked_iterator vector<T,A>::const_checked_iterator::operator+(size_type n)
{
    if((vec_obj->elem+vec_obj->sz)<(current+n))
        throw iterator_range_error("surpasses end()");
    return const_checked_iterator(vec_obj,current+n);
}

template<class T, class A>
typename vector<T,A>::size_type vector<T,A>::const_checked_iterator::operator-(typename vector<T,A>::const_checked_iterator other)
{
    return current - other.current;
}

template<class T, class A>
typename vector<T,A>::const_checked_iterator vector<T,A>::const_checked_iterator::operator-(size_type n)
{
    if((current-n)<vec_obj->elem)
        throw iterator_range_error("precedes begin()");
    return const_checked_iterator(vec_obj,current-n);
}

template<class T, class A>
typename vector<T,A>::const_checked_iterator& vector<T,A>::const_checked_iterator::operator+=(size_type n)
{
    if((vec_obj->elem+vec_obj->sz)<(current+n))
        throw iterator_range_error("surpasses end()");
    current += n;
    return *this;
}

template<class T, class A>
typename vector<T,A>::const_checked_iterator& vector<T,A>::const_checked_iterator::operator-=(size_type n)
{
    if((current-n)<vec_obj->elem)
        throw iterator_range_error("precedes begin()");
    current -= n;
    return *this;
}

template<class T, class A>
class vector<T,A>::checked_iterator : public const_checked_iterator {   // with pointer semantics
public:

    //! ommit explicit to allow assignment of base operations to a
    //! checked_iterator object (eg. checked_iterator i = p + 5)
    checked_iterator(const const_checked_iterator& p)
        :const_checked_iterator(p) {}

    checked_iterator(const vector<T,A>* v, T* p)
        :const_checked_iterator(v,p) {}

    checked_iterator(const vector<T,A>* v, const T* p)
        :const_checked_iterator(v,p) {}

    T& operator*() { return *(const_cast<T*>(this->current)); }
    //! the alternative to const_cast<T*>() would be to ommit the inheritance
    //! and implement each iterator as a separate class see exo18.2

    const T& operator*() const { return *(this->current); }

    //! not needed, since we have explicit checked_iterator(const const_checked_iterator& p)
    /*checked_iterator& operator++() { return const_checked_iterator::operator++(); }
    checked_iterator operator++(int) { return const_checked_iterator::operator++(int()); }

    checked_iterator& operator--() { return const_checked_iterator::operator--(); }
    checked_iterator operator--(int) { return const_checked_iterator::operator--(int()); }

    checked_iterator operator+(size_type n) { return const_checked_iterator::operator+(n); }
    checked_iterator operator-(size_type n) { return const_checked_iterator::operator-(n); }

    checked_iterator& operator+=(size_type n) { return const_checked_iterator::operator+=(n); }
    checked_iterator& operator-=(size_type n) { return const_checked_iterator::operator-=(n); }

    bool operator==(const const_checked_iterator& other) const
    {
        return current==other.current;
    }

    bool operator==(const iterator& other) const
    {
        return current==&*other;
    }

    bool operator!=(const const_checked_iterator& other) const
    {
        return current!=other.current;
    }

    bool operator!=(const iterator& other) const
    {
        return current!=&*other;
    }*/
};

template<class T, class A>
vector<T,A>& vector<T,A>::operator=(const vector<T,A>& v)
{
    if(this==&v) return *this;

    if(v.sz<=space){
        for(int i=0 ; i<sz && i<v.sz ; ++i) elem[i] = v.elem[i];   
        for(int i=sz ; i<v.sz ; ++i) alloc.construct(&elem[i],v.elem[i]);  // for exeeding elements
        for(int i=v.sz ; i<sz ; ++i) alloc.destroy(&elem[i]); 
        sz = v.sz;
        return *this;
    }

    std::auto_ptr<Auto_array_adapter<T> > p(new Auto_array_adapter<T>(alloc.allocate(v.sz)));
    for(int i=0 ; i<v.sz ; ++i) alloc.construct(&(*p)[i],v.elem[i]);
    for(int i=0 ; i<sz ; ++i) alloc.destroy(&elem[i]);
    alloc.deallocate(elem,space);
    space = sz = v.sz;
    elem = p.release()->operator T*();
    return *this;
}

template<class T, class A>
void vector<T,A>::reserve(int newalloc)
{
    if(newalloc<=space) return; // never decrease allocation
    std::auto_ptr<Auto_array_adapter<T> > p(new Auto_array_adapter<T>(alloc.allocate(newalloc)));

    for(int i=0 ; i<sz ; ++i) alloc.construct(&((*p)[i]),elem[i]);
    for(int i=0 ; i<sz ; ++i) alloc.destroy(&elem[i]);

    alloc.deallocate(elem,space);
    elem = p.release()->operator T*();
    space = newalloc;
}

template<class T, class A>
void vector<T,A>::push_back(const T& d)
{
    if(space==0) reserve(8);
    else if(sz==space) reserve(2*space);
    alloc.construct(&elem[sz],d);
    ++sz;
}

template<class T, class A>
void vector<T,A>::move_back(const typename vector<T,A>::iterator& p, const T& d)
{
    for(iterator pos=end()-1 ; pos!=p ; --pos)
        *pos = *(pos-1);
    *p = d;
}

template<class T, class A>
void vector<T,A>::push_front(const T& d)
{
    if(space==0){
        push_back(d);
        return;
    }
    else if(sz==space) reserve(2*space);
    alloc.construct(&elem[sz],elem[sz-1]);
    move_back(begin(),d);
    ++sz;
}

template<class T, class A>
typename vector<T,A>::iterator vector<T,A>::insert(typename vector<T,A>::iterator p, const T& val)
{
    size_type index = p - begin();
    if(space==0) reserve(8);
    else if(sz==space) reserve(2*space);

    alloc.construct(elem+sz,back());
    ++sz;
    p = begin() + index;
    move_back(p,val);
    return p;
}

template<class T, class A>
typename vector<T,A>::iterator vector<T,A>::erase(typename vector<T,A>::iterator p)
{
    if(p==end()) return p;

    {
        iterator pos=p+1;
        for( ; pos!=end() ; ++pos)
            *(pos-1) = *pos;
        alloc.destroy(&*pos);
    }
    --sz;
    return p;
}

template<class T, class A>
void vector<T,A>::resize(int newsize, T val)
{
    if(newsize<0) return;
    reserve(newsize);      
    for(int i=sz ; i<newsize ; ++i) alloc.construct(&elem[i],val);   
    for(int i=newsize ; i<sz ; ++i) alloc.destroy(&elem[i]);
    sz = newsize;
}

template<typename T>
void print(const vector<T>& v)
{
    std::cout << "v.size() == " << v.size() << " v.capacity() == " << v.capacity() << std::endl;
    for(int i=0 ; i<v.size() ; ++i)
        std::cout << "v[" << i << "] == "<< v[i] << '\n';
    std::cout << "\n";
}

int main()
try{
    vector<int> v;
    int x;
    //while(std::cin>>x) v.insert(v.end(),x); v.insert(v.begin()+v.size()/2,666);v.erase(v.begin());v.erase(v.end()-1);
    //print(v);

    while(std::cin>>x) { v.insert(v.end(),x); v.insert(v.begin(),x); }
    /*
    for(vector<int>::checked_iterator i(&v,v.cbegin()) ; i!=v.cend() ; i++){
        //*i = 5;
        std::cout << "v[" << &*i << "] == "<< *i << '\n';
    }*/
    for(vector<int>::checked_iterator i(&v,v.end()-1) ; i!=v.cbegin() ; i = i-1){
        //*i = 5;
        std::cout << "v[" << &*i << "] == "<< *i << '\n';
    }
    std::cout << "-------------------------------------------------\n";
    int n=0;/*
    for(vector<int>::const_checked_iterator i(&v,v.cend()-1) ; (i-n)!=v.cbegin() ; ){
        std::cout << "v[" << &*(i-n) << "] == "<< *(i-n) << ", n == " << n << '\n';
        ++n;
    }*/
    for(vector<int>::const_checked_iterator i(&v,v.cend()-1) ; i!=v.cbegin() ; i=i-1){
        //*i = 5;
        std::cout << "v[" << &*i << "] == "<< *i << ", n == " << n << '\n';
        ++n;
    }
    std::cerr << &*std::find(vector<int>::const_checked_iterator(&v,v.cbegin()),
                           vector<int>::const_checked_iterator(&v,v.cend()),1);


    return 0;
}catch(Range_error& err){
    std::cerr << err.what() << ", at : " << err.index << std::endl;
    return 1;
}
catch(std::exception& err){
    std::cerr << err.what() << std::endl;
    return 2;
}

