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

    checked_iterator checked_begin() { return checked_iterator(this,elem); }
    checked_iterator checked_end() { return checked_iterator(this,elem+sz); }

    const_checked_iterator checked_cbegin() { return checked_iterator(this,elem); }
    const_checked_iterator checked_cend() { return checked_iterator(this,elem+sz); }

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

template<class T, class A> class vector<T,A>::checked_iterator {   // with pointer semantics
public:
    typedef typename vector<T,A>::iterator_category iterator_category;
    typedef typename vector<T,A>::value_type        value_type;
    typedef typename vector<T,A>::difference_type   difference_type;
    typedef typename vector<T,A>::pointer           pointer;
    typedef typename vector<T,A>::reference         reference;
    friend class vector<T,A>::const_checked_iterator;

public:
    checked_iterator() :current(0), vec_obj(0) {}

    explicit checked_iterator(const vector<T,A>* v)
        :current(v->elem), vec_obj(v) { }

    checked_iterator(const vector<T,A>* v, iterator p) throw(iterator_range_error)
        :current(p), vec_obj(v) { check_position(p,"checked_iterator(const vector<T,A>*, iterator)"); }

    T& operator*() throw(iterator_range_error)
    {
        if(current==vec_obj->elem+vec_obj->sz) throw iterator_range_error("T& operator*() derefrence end()");
        return *current;
    }
    T& operator[](size_type n) { return *(*this+n); }
    const T& operator[](size_type n) const { return *(*this+n); }

    const T& operator*() const throw(iterator_range_error)
    {
        if(current==vec_obj->elem+vec_obj->sz) throw iterator_range_error("const T& operator*() derefrence end()");
        return *current;
    }
    T* operator->() { return current; }

    checked_iterator& operator++() throw(iterator_range_error);
    checked_iterator operator++(int) throw(iterator_range_error);

    checked_iterator& operator--() throw(iterator_range_error);
    checked_iterator operator--(int) throw(iterator_range_error);

    checked_iterator& operator+=(size_type n) throw(iterator_range_error);
    checked_iterator& operator-=(size_type n) throw(iterator_range_error);

    checked_iterator operator+(size_type n) throw(iterator_range_error);
    checked_iterator operator-(size_type n) throw(iterator_range_error);
    difference_type operator-(const checked_iterator& other) { return current-other.current; }
    difference_type operator-(typename vector<T,A>::const_checked_iterator& other) { return current - other.current; }

    bool operator==(const checked_iterator& other) const
    {
        return current==other.current;
    }

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

    bool operator!=(const checked_iterator& other) const
    {
        return current!=other.current;
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

    bool operator<(const checked_iterator& other) const
    {
        return current<other.current;
    }

    bool operator<(const const_checked_iterator& other) const
    {
        return current<other.current;
    }

    bool operator<(const iterator& other) const
    {
        return current<&*other;
    }

    bool operator<(const const_iterator& other) const
    {
        return current<&*other;
    }

    bool operator>(const checked_iterator& other) const
    {
        return current>other.current;
    }

    bool operator>(const const_checked_iterator& other) const
    {
        return current>other.current;
    }

    bool operator>(const iterator& other) const
    {
        return current>&*other;
    }

    bool operator>(const const_iterator& other) const
    {
        return current>&*other;
    }

    bool operator<=(const checked_iterator& other) const
    {
        return current<=other.current;
    }

    bool operator<=(const const_checked_iterator& other) const
    {
        return current<=other.current;
    }

    bool operator<=(const iterator& other) const
    {
        return current<=&*other;
    }

    bool operator<=(const const_iterator& other) const
    {
        return current<=&*other;
    }

    bool operator>=(const checked_iterator& other) const
    {
        return current>=other.current;
    }

    bool operator>=(const const_checked_iterator& other) const
    {
        return current>=other.current;
    }

    bool operator>=(const iterator& other) const
    {
        return current>=&*other;
    }

    bool operator>=(const const_iterator& other) const
    {
        return current>=&*other;
    }

    iterator plain_iterator() { return current; }

private:
    void check_position(const T* p, const std::string& s) throw(iterator_range_error)
    {
        if(vec_obj->elem+vec_obj->sz<p)throw iterator_range_error(" " + s + " passed end()");
        if(p<vec_obj->elem) throw iterator_range_error(" " + s + " before begin()");
    }

private:
    T* current;
    const vector<T,A>* vec_obj;
};

template<class T, class A>
typename vector<T,A>::checked_iterator& vector<T,A>::checked_iterator::operator++() throw(iterator_range_error)
{
    if(current==(vec_obj->elem+vec_obj->sz))
        throw iterator_range_error("checked_iterator::operator++() surpasses end()");
    ++current;
    return *this;
}

template<class T, class A>
typename vector<T,A>::checked_iterator vector<T,A>::checked_iterator::operator++(int) throw(iterator_range_error)
{
    if(current==(vec_obj->elem+vec_obj->sz))
        throw iterator_range_error("checked_iterator::operator++(int) surpasses end()");
    T* temp = current;
    ++current;
    return checked_iterator(vec_obj,temp);
}

template<class T, class A>
typename vector<T,A>::checked_iterator& vector<T,A>::checked_iterator::operator--() throw(iterator_range_error)
{
    if(current==vec_obj->elem)
        throw iterator_range_error("checked_iterator::operator--() precedes begin()");
    --current;
    return *this;
}

template<class T, class A>
typename vector<T,A>::checked_iterator vector<T,A>::checked_iterator::operator--(int) throw(iterator_range_error)
{
    if(current==vec_obj->elem)
        throw iterator_range_error("checked_iterator::operator--(int) precedes begin()");
    T* temp = this->current;
    --current;
    return checked_iterator(vec_obj,temp);
}

template<class T, class A>
typename vector<T,A>::checked_iterator& vector<T,A>::checked_iterator::operator+=(size_type n) throw(iterator_range_error)
{
    check_position(current+n,"checked_iterator::operator+=(size_type)/(+)");
    current += n;
    return *this;
}

template<class T, class A>
typename vector<T,A>::checked_iterator& vector<T,A>::checked_iterator::operator-=(size_type n) throw(iterator_range_error)
{
    check_position(current-n,"checked_iterator::operator-=(size_type)/(-)");
    current -= n;
    return *this;
}

template<class T, class A>
typename vector<T,A>::checked_iterator vector<T,A>::checked_iterator::operator+(size_type n) throw(iterator_range_error)
{
    checked_iterator temp(*this);
    return temp+=n;
}

template<class T, class A>
typename vector<T,A>::checked_iterator vector<T,A>::checked_iterator::operator-(size_type n) throw(iterator_range_error)
{
    checked_iterator temp(*this);
    return temp-=n;
}

template<class T, class A>
class vector<T,A>::const_checked_iterator {   // with pointer semantics
public:
    typedef typename vector<T,A>::iterator_category iterator_category;
    typedef typename vector<T,A>::value_type        value_type;
    typedef typename vector<T,A>::difference_type   difference_type;
    typedef typename vector<T,A>::pointer           pointer;
    typedef typename vector<T,A>::reference         reference;
    friend class vector<T,A>::checked_iterator;

public:
    const_checked_iterator() :current(0), vec_obj(0) {}

    const_checked_iterator(const checked_iterator& p)   // no need to check, since checked_iterator
        :current(p.current), vec_obj(p.vec_obj) {}                      // does the check for us

    const_checked_iterator(const vector<T,A>* v, const_iterator p) throw(iterator_range_error)
        :current(p), vec_obj(v) { check_position(p,"const_checked_iterator(const vector<T,A>*, const_iterator)"); }

    const_checked_iterator(const vector<T,A>* v, iterator p) throw(iterator_range_error)
        :current(p), vec_obj(v) { check_position(p,"const_checked_iterator(const vector<T,A>*, iterator)"); }

    const T& operator*() const throw(iterator_range_error)
    {
        if(vec_obj->elem+vec_obj->sz==current)
            throw iterator_range_error("const T& operator*() derefrence end()");
        return *current;
    }
    const T& operator[](size_type n) { return *(*this+n); }
    const T* operator->() const { return current; }

    const_checked_iterator& operator++() throw(iterator_range_error);
    const_checked_iterator operator++(int) throw(iterator_range_error);

    const_checked_iterator& operator--() throw(iterator_range_error);
    const_checked_iterator operator--(int) throw(iterator_range_error);

    const_checked_iterator& operator+=(size_type) throw(iterator_range_error);
    const_checked_iterator& operator-=(size_type) throw(iterator_range_error);

    const_checked_iterator operator+(size_type) throw(iterator_range_error);
    const_checked_iterator operator-(size_type) throw(iterator_range_error);
    difference_type operator-(typename vector<T,A>::const_checked_iterator& other) { return current - other.current; }
    difference_type operator-(typename vector<T,A>::checked_iterator& other) { return current - other.current; }

    bool operator==(const const_checked_iterator& other) const
    {
        return current==other.current;
    }

    bool operator==(const checked_iterator& other) const
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

    bool operator!=(const checked_iterator& other) const
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

    bool operator<(const const_checked_iterator& other) const
    {
        return current<other.current;
    }

    bool operator<(const checked_iterator& other) const
    {
        return current<other.current;
    }

    bool operator<(const iterator& other) const
    {
        return current<&*other;
    }

    bool operator<(const const_iterator& other) const
    {
        return current<&*other;
    }

    bool operator>(const const_checked_iterator& other) const
    {
        return current>other.current;
    }

    bool operator>(const checked_iterator& other) const
    {
        return current>other.current;
    }

    bool operator>(const iterator& other) const
    {
        return current>&*other;
    }

    bool operator>(const const_iterator& other) const
    {
        return current>&*other;
    }

    bool operator<=(const const_checked_iterator& other) const
    {
        return current<=other.current;
    }

    bool operator<=(const checked_iterator& other) const
    {
        return current<=other.current;
    }

    bool operator<=(const iterator& other) const
    {
        return current<=&*other;
    }

    bool operator<=(const const_iterator& other) const
    {
        return current<=&*other;
    }

    bool operator>=(const const_checked_iterator& other) const
    {
        return current>=other.current;
    }

    bool operator>=(const checked_iterator& other) const
    {
        return current>=other.current;
    }

    bool operator>=(const iterator& other) const
    {
        return current>=&*other;
    }

    bool operator>=(const const_iterator& other) const
    {
        return current>=&*other;
    }

private:
    void check_position(const T* p, const std::string& s) throw(iterator_range_error)
    {
        if(vec_obj->elem+vec_obj->sz<p)throw iterator_range_error(" " + s + " passed end()");
        if(p<vec_obj->elem) throw iterator_range_error(" " + s + " before begin()");
    }

private:
    const T* current;
    const vector<T,A>* vec_obj;
};

template<class T, class A>
typename vector<T,A>::const_checked_iterator& vector<T,A>::const_checked_iterator::operator++() throw(iterator_range_error)
{
    if(current==(vec_obj->elem+vec_obj->sz))
        throw iterator_range_error("const_checked_iterator::operator++() surpasses end()");
    ++current;
    return *this;
}

template<class T, class A>
typename vector<T,A>::const_checked_iterator vector<T,A>::const_checked_iterator::operator++(int) throw(iterator_range_error)
{
    if(current==(vec_obj->elem+vec_obj->sz))
        throw iterator_range_error("const_checked_iterator::operator++(int) surpasses end()");
    const T* temp = this->current;
    ++current;
    return const_checked_iterator(vec_obj,temp);
}

template<class T, class A>
typename vector<T,A>::const_checked_iterator& vector<T,A>::const_checked_iterator::operator--() throw(iterator_range_error)
{
    if(current==vec_obj->elem)
        throw iterator_range_error("const_checked_iterator::operator--() precedes begin()");
    --current;
    return *this;
}

template<class T, class A>
typename vector<T,A>::const_checked_iterator vector<T,A>::const_checked_iterator::operator--(int) throw(iterator_range_error)
{
    if(current==vec_obj->elem)
        throw iterator_range_error("const_checked_iterator::operator--(int) precedes begin()");
    const T* temp = this->current;
    --current;
    return const_checked_iterator(vec_obj,temp);
}

template<class T, class A>
typename vector<T,A>::const_checked_iterator& vector<T,A>::const_checked_iterator::operator+=(size_type n) throw(iterator_range_error)
{
    check_position(current+n,"const_checked_iterator::operator+=(size_type)/(+)");
    current += n;
    return *this;
}

template<class T, class A>
typename vector<T,A>::const_checked_iterator& vector<T,A>::const_checked_iterator::operator-=(size_type n) throw(iterator_range_error)
{
    check_position(current-n,"const_checked_iterator::operator-=(size_type)/(-)");
    current -= n;
    return *this;
}

template<class T, class A>
typename vector<T,A>::const_checked_iterator vector<T,A>::const_checked_iterator::operator+(size_type n) throw(iterator_range_error)
{
    const_checked_iterator temp(*this);
    return temp+=n;
}

template<class T, class A>
typename vector<T,A>::const_checked_iterator vector<T,A>::const_checked_iterator::operator-(size_type n) throw(iterator_range_error)
{
    const_checked_iterator temp(*this);
    return temp-=n;
}

//!-----------------------------------------------------------------------------------------------------------------------------------!//

template<class T, class A>
vector<T,A>& vector<T,A>::operator=(const vector<T,A>& v)
{
    if(this==&v) return *this;

    if(v.sz<=space){
        for(int i=0 ; i<sz && i<v.sz ; ++i) elem[i] = v.elem[i];   // for already constructed space
        for(int i=sz ; i<v.sz ; ++i) alloc.construct(&elem[i],v.elem[i]);  // for exeeding elements
        for(int i=v.sz ; i<sz ; ++i) alloc.destroy(&elem[i]);   // in case the new vector has fewer elements
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
/*
template<class T, class A>
void vector<T,A>::move_back(const T& d)
{
    for(int i=sz-1 ; i>0 ; --i)
        elem[i] = elem[i-1];
    elem[0] = d;
}*/

template<class T, class A>
void vector<T,A>::move_back(const typename vector<T,A>::iterator& p, const T& d)
{
    if(sz==0) return;
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

    if(sz!=0) alloc.construct(elem+sz,back());
    ++sz;
    p = begin() + index;
    move_back(p,val);
    return p;
}

template<class T, class A>
typename vector<T,A>::iterator vector<T,A>::erase(typename vector<T,A>::iterator p)
{
    if(p==end()) return p;

    for( iterator pos=p+1 ; pos!=end() ; ++pos)
        *(pos-1) = *pos;
    alloc.destroy(&*(end()-1));

    --sz;
    return p;
}

template<class T, class A>
void vector<T,A>::resize(int newsize, T val)
{
    if(newsize<0) return;
    reserve(newsize);       // handle newsize<=space and space<newsize
    for(int i=sz ; i<newsize ; ++i) alloc.construct(&elem[i],val);    // implicitely handle newsize<=size && size<newsize
    for(int i=newsize ; i<sz ; ++i) alloc.destroy(&elem[i]);
    sz = newsize;   // handle newsize<size
}

template<typename T>
void print(const vector<T>& v)
{
    std::cout << "v.size() == " << v.size() << " v.capacity() == " << v.capacity() << std::endl;
    for(int i=0 ; i<v.size() ; ++i)
        std::cout << "v[" << i << "] == "<< v[i] << '\n';
    std::cout << "\n";
}

template<typename Iter>
void print(Iter s, Iter e)
{
    std::cout << "{\n";
    for(; s!=e ; ++s)
        std::cout << *s << '\n';
    std::cout << "}";
}

int main()
try{
    vector<std::string> v;
    std::string x;v.push_back("first");

    while(std::cin>>x) { v.insert(v.end(),x);}

    /*
    for(vector<std::string>::checked_iterator i(&v,v.begin()) ; i!=v.cend() ; ++i)
        std::cout << "v[" << &*i << "] == "<< *i << '\n';*/
    print(vector<std::string>::checked_iterator(&v,v.begin()),vector<std::string>::checked_iterator(&v,v.end()));

    std::cerr << "\nFine\n";

    typedef vector<std::string>::checked_iterator vec_string_checked_iter;
    std::sort(vec_string_checked_iter(&v,v.begin()),
                           vec_string_checked_iter(&v,v.end()));

    v.erase(v.end()-1);//std::cerr << "*" << *(v.end()-1) << "*" << std::endl;

    print(v.checked_cbegin(),v.checked_cend());
/*

    try{
        for(vector<int>::checked_iterator i(&v,v.end()-1) ; ; i=i-1  --i i--)
            std::cout << "v[" << &*i << "] == "<< *i << '\n';
    }catch(iterator_range_error& ierr){
        std::cerr << ierr.what() << std::endl;
    }
    x=0;
    for(vector<int>::checked_iterator i(&v,v.begin()) ; x<v.size() ; x++)
        std::cout << "v[" << &i[x] << "] == "<< i[x] << '\n';

    std::cerr << &*std::find(vector<int>::const_checked_iterator(&v,v.cbegin()),
                           vector<int>::const_checked_iterator(&v,v.cend()),1) << std::endl;

    std::sort(vector<int>::checked_iterator(&v,v.begin()),
                           vector<int>::checked_iterator(&v,v.end()));

    for(vector<int>::checked_iterator i(&v,v.begin()) ; i!=v.cend() ; i=i+1)
        std::cout << "v[" << &*i << "] == "<< *i << '\n';*/
/*
    for(vector<int>::const_checked_iterator i(&v,v.begin()) ; i!=v.cend() ; i=i+1, ++i, i++)
        std::cout << "v[" << &*i << "] == "<< *i << '\n';


    try{
        for(vector<int>::const_checked_iterator i(&v,v.end()-1) ; ; i=i-1, --i, i--)
            std::cout << "v[" << &*i << "] == "<< *i << '\n';
    }catch(iterator_range_error& ierr){
        std::cerr << ierr.what() << std::endl;
    }
    x=0;
    for(vector<int>::const_checked_iterator i(&v,v.begin()) ; x<v.size() ; x++)
        std::cout << "v[" << &i[x] << "] == "<< i[x] << '\n';

    std::cerr << &*std::find(vector<int>::checked_iterator(&v,v.begin()),
                           vector<int>::checked_iterator(&v,v.end()),1) << std::endl;

//    std::sort(vector<int>::const_checked_iterator(&v,v.begin()),
//                           vector<int>::const_checked_iterator(&v,v.end()));

    for(vector<int>::const_checked_iterator i(&v,v.begin()) ; i!=v.cend() ; i=i+1)
        std::cout << "v[" << &*i << "] == "<< *i << '\n';*/


    return 0;
}catch(Range_error& err){
    std::cerr << err.what() << ", at : " << err.index << std::endl;
    return 1;
}
catch(std::exception& err){
    std::cerr << err.what() << std::endl;
    return 2;
}

