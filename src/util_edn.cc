#include <iostream>
#include <iomanip>
#include <algorithm>
#include <ostream>
#include <vector>

//#define CHECK_CAP_CHANGE
//#define RUBY_FORMATTED_DECIMALS

#include "base_types.h"
#include "util_edn.h"

namespace pdftoedn
{
    namespace util
    {
        namespace edn
        {
            // =============================================
            // output operator for pairs
            //
            std::ostream& operator<<(std::ostream& o, const std::pair<EDNNode,EDNNode>& p) {
                // hash pairs are separated by a space
                o << p.first << " " << p.second;
                return o;
            }

            // =============================================
            // Node proxy class to manage storage for EDN output to an
            // ostream
            //
            EDNNode::EDNNode(const pdftoedn::Coord& c) :
                type(UVAL_OBJ), val(reinterpret_cast<const gemable*>(&c)), owner(false) {
            }
            EDNNode::EDNNode(pdftoedn::Coord&& c) :
                type(UVAL_OBJ), val(new pdftoedn::Coord(c)), owner(true) {
            }
            EDNNode::EDNNode(const pdftoedn::BoundingBox& b) :
                type(UVAL_OBJ), val(reinterpret_cast<const gemable*>(&b)), owner(false) {
            }
            EDNNode::EDNNode(pdftoedn::BoundingBox&& b) :
                type(UVAL_OBJ), val(new pdftoedn::BoundingBox(b)), owner(true) {
            }
            EDNNode::EDNNode(const pdftoedn::Bounds& b) :
                type(UVAL_OBJ), val(reinterpret_cast<const gemable*>(&b)), owner(false) {
            }
            EDNNode::EDNNode(const pdftoedn::Symbol& v) :
                type(UVAL_OBJ), val(reinterpret_cast<const gemable*>(&v)), owner(false) {
            }
            EDNNode::EDNNode(pdftoedn::Symbol&& v) :
                type(UVAL_OBJ), val(new pdftoedn::Symbol(v)), owner(true) {
            }
            EDNNode::EDNNode(Vector& v) :
                type(UVAL_OBJ), val(new Vector(v)), owner(true) {
            }
            EDNNode::EDNNode(Hash& h) :
                type(UVAL_OBJ), val(new Hash(h)), owner(true) {
            }
            EDNNode& EDNNode::operator=(EDNNode n) {
                type = n.type;
                val = n.val;
                owner = n.owner;
                n.owner = false;
                return *this;
            }
            EDNNode::~EDNNode() {
                if (owner) {
                    switch (type)
                    {
                      case UVAL_STRING: delete val.str; break;
                      case UVAL_OBJ:    delete val.obj; break;
                      default:
                          std::cerr << "ERROR DELETING NODE OF TYPE " << type << std::endl;
                          break;
                    }
                }
            }
            std::ostream& EDNNode::to_edn(std::ostream& o) const {
                switch (type)
                {
                  case UVAL_BOOL:   o << std::boolalpha << val.b;   break;
                  case UVAL_UINT:   o << std::dec << val.ui;        break;
                  case UVAL_INT:    o << std::dec << val.i;         break;
#ifdef RUBY_FORMATTED_DECIMALS
                  case UVAL_DOUBLE:
                      {
                          double ip;
                          if (modf(val.d, &ip) == 0.0) {
                              o << std::dec << (intmax_t) val.d << ".0";
                          }
                          else
                          {
                              std::streamsize p = o.precision();
                              o.precision(17);
                              o << std::dec << val.d;
                              o.precision(p);
                          }
                          break;
                      }
#else
                  case UVAL_DOUBLE: o << std::dec << val.d;         break;
#endif

                  case UVAL_OBJ:    o << *(val.obj);                break;
                  case UVAL_STRING:
                      o << '"';
                      // need to escape double quotes
                      std::for_each( val.str->begin(), val.str->end(),
                                     [&](const char c) {
                                         switch (c) {
                                           case '"':
                                           case '\\':
                                               o << '\\';
                                               break;
                                         }
                                         o << c;
                                     } );
                      o << '"';
                      break;
                  default:
                      std::cerr << "attempt to output UNDEF node" << std::endl;
                      break;
                }
                return o;
            }

            // =============================================
            // EDN output sequence container
            //
            template <class T>
            std::ostream& Container<T>::to_edn(std::ostream& o) const
            {
                o << open_chars();
                if (elems) {
                    if (elems->size() > 1) {
                        // output each node with a space in between up to the penultimate one
                        std::for_each( elems->begin(), elems->end() - 1,
                                       [&](const T& n) {
                                           o << n << sep_chars();
                                       });
                    }
                    if (!elems->empty()) {
                        o << elems->back();
                    }
                }
                o << close_chars();
                return o;
            }

#ifdef CHECK_CAP_CHANGE
            template <class T>
            void Container<T>::push_elem(const T& n)
            {
                if (!elems) {
                    elems = new std::vector<T>;
                }
                uintmax_t c1 = elems->capacity();
                elems->push_back(n);
                uintmax_t c2 = elems->capacity();
                if (c1 != c2 && (c1 != 0 && c2 != 1)) {
                    std::cerr << "CAP changed from " << c1 << " to " << c2 << std::endl;
                }
            }
#endif

            // =============================================
            // Hash's push cerates the pair to be stored
            void Hash::push(const EDNNode& n1, const EDNNode& n2) {
                push_elem(std::pair<EDNNode, EDNNode>(n1, n2));
            }
        }
    }
} // namespace

