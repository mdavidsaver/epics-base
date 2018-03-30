/*************************************************************************\
* Copyright (c) 2018 Michael Davidsaver
* EPICS BASE is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
#ifndef EPICSJTREE_H
#define EPICSJTREE_H
/** @page jtree Parsing and storage of generic JSON tree
 *
 * Parsing of arbitrary JSON trees.  Each tree contains, at least, a root node (cf. epicsJTreeRoot() ).
 * Trees much be freed with epicsJTreeFree(), which also invalidates all nodes and char* .
 *
 * All functions will correctly error when called with NULL tree or node pointers.
 *
 * Nodes remain valid until the tree from which the were obtained is free()'d.
 * 'const char*' obtain from nodes are also invalidated when the tree is free()'d.
 */

#include <epicsString.h>
#include <shareLib.h>
#include <compilerDependencies.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct epicsJTree epicsJTree;
typedef struct epicsJNode epicsJNode;
typedef struct epicsJNodeClass epicsJNodeClass;

enum epicsJNodeType {
    epicsJNodeTypeNull,
    epicsJNodeTypeBool,
    epicsJNodeTypeInteger,
    epicsJNodeTypeReal,
    epicsJNodeTypeString,
    epicsJNodeTypeMap,
    epicsJNodeTypeArray,
};

typedef enum epicsJNodeType epicsJNodeType;

/** Parse provided string slice as JSON and return tree handle.
 *
 @param inp Input JSON string buffer
 @param inplen Number of charactors to read from string buffer
 @param errfn Called with information on any error encountered during parsing.
 @param errpvt Opaque pointer passed through to errfn
 @param flags Optional flags.  Default to zero (0).
 @returns A new tree pointer, or NULL on error.
 */
epicsShareFunc
const epicsJTree* ejtParse(const char *inp, size_t inplen,
                           void (*errfn)(void *errpvt, size_t pos, const char *msg),
                           void *errpvt,
                           unsigned flags);
/** Free memory associated with the tree. */
epicsShareFunc
void ejtFree(const epicsJTree* tree);

/** Helper for use in epicsJTreeParse() errfn to translate buffer position into line and column */
epicsShareFunc
void ejtPos2Line(const char *inp, size_t inplen, size_t pos, size_t *line, size_t *col);

/** Return root node of the tree (or NULL) */
epicsShareFunc
const epicsJNode* ejtRoot(const epicsJTree* tree);

/** Return the value type of a node.  NULL node is considered tp be epicsJNodeTypeNull */
epicsShareFunc
epicsJNodeType ejtType(const epicsJNode* node);
/** Return number of child nodes for an array or map node, and (size_t)-1 otherwise */
epicsShareFunc
size_t ejtCount(const epicsJNode* node);

/* options for map lookup */
#define EJT_KEY_RECURSE (0)
#define EJT_KEY_EXACT (1)
/* options to control implicit value type conversion */
#define EJT_VAL_CONVERT (0)
#define EJT_VAL_ERROR (2)

/** Lookup in map node by key name.
 *
 * Returns NULL if not such key.
 *
 @warning This function has O(n) runtime complexity.
 */
epicsShareFunc
const epicsJNode* ejtLookup(const epicsJNode* node, const char *key, unsigned flags);

/** Lookup in array or map node by index.
 *
 * Returns NULL for index our of range
 *
 @warning This function has O(n) runtime complexity.
 */
epicsShareFunc
const epicsJNode* ejtIndex(const epicsJNode* node, size_t index);

/** Fetch node value as boolean.  On success, return 1 and update 'result'.  Otherwise return 0 and leave 'result' unchanged. */
epicsShareFunc
int ejtAsBool(const epicsJNode* node, unsigned flags, int *result);
/** Fetch node value as integer.  On success, return 1 and update 'result'.  Otherwise return 0 and leave 'result' unchanged. */
epicsShareFunc
int ejtAsInteger(const epicsJNode* node, unsigned flags, long long *result);
/** Fetch node value as real number.  On success, return 1 and update 'result'.  Otherwise return 0 and leave 'result' unchanged. */
epicsShareFunc
int ejtAsReal(const epicsJNode* node, unsigned flags, double *result);
/** Fetch node value as string.  On success, return 1 and update 'result'.  Otherwise return 0 and leave 'result' unchanged. */
epicsShareFunc
int ejtAsString(const epicsJNode* node, unsigned flags, const char* *result);
/** Fetch node value as string (or NULL).  Returned string must be free()d */
epicsShareFunc
char* ejtAsStringAlloc(const epicsJNode* node, unsigned flags);

/** Lookup node in map and fetch value.  On success, return 1 and update 'result'.  Otherwise return 0 and leave 'result' unchanged. */
static EPICS_ALWAYS_INLINE
int ejtLookupAsBool(const epicsJNode* node, const char *key, unsigned flags, int *result) {
    return ejtAsBool(ejtLookup(node, key, flags), flags, result);
}

/** Lookup node in map and fetch value.  On success, return 1 and update 'result'.  Otherwise return 0 and leave 'result' unchanged. */
static EPICS_ALWAYS_INLINE
int ejtLookupAsInteger(const epicsJNode* node, const char *key, unsigned flags, long long *result) {
    return ejtAsInteger(ejtLookup(node, key, flags), flags, result);
}

/** Lookup node in map and fetch value.  On success, return 1 and update 'result'.  Otherwise return 0 and leave 'result' unchanged. */
static EPICS_ALWAYS_INLINE
int ejtLookupAsReal(const epicsJNode* node, const char *key, unsigned flags, double *result) {
    return ejtAsReal(ejtLookup(node, key, flags), flags, result);
}

/** Lookup node in map and fetch value.  On success, return 1 and update 'result'.  Otherwise return 0 and leave 'result' unchanged. */
static EPICS_ALWAYS_INLINE
int ejtLookupAsString(const epicsJNode* node, const char *key, unsigned flags, const char* *result) {
    return ejtAsString(ejtLookup(node, key, flags), flags, result);
}

/** Lookup node in map and fetch value as string (or NULL).  Returned string must be free()d */
static EPICS_ALWAYS_INLINE
char* ejtLookupAsStringAlloc(const epicsJNode* node, const char *key, unsigned flags) {
    return ejtAsStringAlloc(ejtLookup(node, key, flags), flags);
}

#undef EJT_MAKE_GET_KEY
#undef EJT_MAKE_GET_KEY2

struct ELLNODE;
typedef struct ELLNODE* epicsJNodeIterator;

/** Iterate through array or map.
 *
 @code
 const epicsJNode* node = ...;
 epicsJNodeIterator iter = NULL;
 const char *key;
 const epicsJNode *child;
 while(epicsJNodeIterate(node, &iter, &key, &child)) {
    // do something with 'key' and 'child'.
    // key==NULL when iterating an array.
 }
 @endcode
 */
epicsShareFunc
int ejtIterate(const epicsJNode* node, epicsJNodeIterator* iter, const char** key, const epicsJNode** child);

#ifdef __cplusplus
} // extern "C"

#include <stdexcept>
#include <algorithm>
#include <iterator>
#include <string>
#include <utility> // for std::pair

namespace epics{namespace jtree{

struct epicsShareClass ParseError : public std::runtime_error {
    ParseError(size_t pos, size_t line, size_t col, const std::string& msg);
    virtual ~ParseError() throw();

    const size_t pos, line, col;
};

struct Tree;
struct Node;
struct NodeIterator;

namespace detail {
template<typename T>
struct ashelper {
    typedef T arg_type;
    static inline T defval() { return 0; }
};
template<>
struct ashelper<std::string> {
    typedef const std::string& arg_type;
    static inline std::string defval() { return std::string(); }
};
}

struct Node {
    typedef NodeIterator iterator;
    typedef NodeIterator const_iterator;

    inline Node() :node(0) {}

    inline void swap(Node& o) {
        std::swap(node, o.node);
    }

    // validity (is this a real node)

    inline bool valid() const { return node!=0; }

#if __cplusplus>=201103L
    explicit operator bool() const { return valid(); }
#else
private:
    typedef void (Node::*bool_type)(Node&);
public:
    inline operator bool_type() const { return valid() ? &Node::swap : 0; }
#endif

    // map lookup and array element access

    inline Node lookup(const char *key, unsigned flags=0)
    { return Node(ejtLookup(node, key, flags)); }
    inline Node lookup(const std::string& key, unsigned flags=0)
    { return Node(ejtLookup(node, key.c_str(), flags)); }

    inline Node index(size_t idx)
    { return Node(ejtIndex(node, idx)); }

    inline Node operator[](const char *key) { return lookup(key); }
    inline Node operator[](const std::string& key) { return lookup(key); }

    // iteration
    inline NodeIterator begin() const;
    inline NodeIterator end() const;

    // value access

    inline epicsJNodeType type() const { return ejtType(node); }
    inline size_t size() const { return ejtCount(node); }

    inline bool as(bool& val, unsigned flags = 0) const {
        int bval;
        bool ret = ejtAsBool(node, flags, &bval);
        if(ret)
            val = !!bval;
        return ret;
    }
    inline bool as(long long& val, unsigned flags = 0) const {
        return ejtAsInteger(node, flags, &val);
    }
    inline bool as(double& val, unsigned flags = 0) const {
        return ejtAsReal(node, flags, &val);
    }
    inline bool as(const char*& val, unsigned flags = 0) const {
        return ejtAsString(node, flags, &val);
    }
    bool as(std::string& val, unsigned flags = 0) const {
        const char *sval = 0;
        bool ret = ejtAsString(node, flags, &sval);
        if(ret)
            val = sval;
        return ret;
    }

    template<typename T>
    inline T as() {
        T ret(detail::ashelper<T>::defval());
        as(ret, 0);
        return ret;
    }

    template<typename T>
    inline T as(typename detail::ashelper<T>::arg_type defval) {
        T ret;
        if(!as(ret))
            ret = defval;
        return ret;
    }

private:
    const epicsJNode *node;

    friend struct Tree;
    friend struct NodeIterator;

    inline explicit Node(const epicsJNode *node) :node(node) {}
    inline explicit Node(const Tree& tree);
};

struct NodeIterator {
    typedef std::input_iterator_tag iterator_category;
    typedef std::pair<const char*, Node> value_type;
    typedef std::ptrdiff_t difference_type;
    typedef value_type& reference;
    typedef value_type* pointer;

    NodeIterator() :m_node(0), m_iter(0), m_pair((const char*)0, Node()) {}

    inline bool operator==(const NodeIterator& o) const {
        return (!m_node && !o.m_node) || m_pair.second.node==o.m_pair.second.node;
    }
    inline bool operator!=(const NodeIterator& o) const {
        return !((*this)==o);
    }

    inline NodeIterator& operator++() { advance(); return *this; }
    inline NodeIterator operator++(int) { NodeIterator ret(*this); advance(); return ret; }
    inline reference operator*() { return m_pair; }
    inline pointer operator->() { return &m_pair; }
private:
    const epicsJNode* m_node;
    epicsJNodeIterator m_iter;
    value_type m_pair;

    friend struct Node;

    NodeIterator(const epicsJNode* node) :m_node(node), m_iter(0), m_pair((const char*)0, Node())
    { advance(); }

    inline void advance() {
        if(!ejtIterate(m_node, &m_iter, &m_pair.first, &m_pair.second.node)) {
            m_node = 0;
        }
    }
};

struct epicsShareClass Tree {

    Tree() :tree(0) {}
    inline ~Tree() { ejtFree(tree); }

#if __cplusplus>=201103L
    inline Tree(Tree&& o) :tree(o.tree) {
        o.tree = 0;
    }
#endif

    inline void swap(Tree& o) {
        std::swap(tree, o.tree);
    }

    void parse(const char *inp, size_t inplen, unsigned flags=0);

    inline Node root() const { return Node(ejtRoot(tree)); }

    inline bool valid() const { return tree!=0; }

#if __cplusplus>=201103L
    explicit operator bool() const { return valid(); }
#else
private:
    typedef void (Tree::*bool_type)(Tree&);
public:
    inline operator bool_type() const { return valid() ? &Tree::swap : 0; }
#endif

private:
    const epicsJTree *tree;

    friend struct Node;

    explicit Tree(const epicsJTree* tree) :tree(tree) {}

    Tree(const Tree&);
    Tree& operator=(const Tree&);
};

inline Node::Node(const Tree& tree) :node(ejtRoot(tree.tree)) {}

inline NodeIterator Node::begin() const { return NodeIterator(node); }
inline NodeIterator Node::end() const { return NodeIterator(); }

}} // namespace epics::jtree

#endif /* __cplusplus */

#endif // EPICSJTREE_H
