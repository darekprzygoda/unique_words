#ifndef TRIE_HPP
#define TRIE_HPP

#include <deque>
#include <iostream>
#include <string_view>

namespace util {

    constexpr std::size_t kSize = 26; // lowercase letters only

    // std::size_t count;

    class Trie {
        struct Node {
            Node() {
                // ++count;
                // std::cerr << "Node() " << count << std::endl;
            }
            ~Node() {
                // --count;
                // std::cerr << "~Node()" << count << std::endl;
                for ( Node*& child : children ) {
                    delete child;
                    child = nullptr;
                }
            }
            Node* children[ kSize ] = {};
            bool leaf = false;
        };
        Node root;

        void mergeHelper( Node* from, Node* to ) {
            for ( std::size_t i = 0; i < kSize; ++i ) {
                Node*& fc = from->children[ i ];
                if ( !fc )
                    continue; //
                Node*& c = to->children[ i ];
                if ( !c ) { // move whole subtree from other to this
                    c = fc;
                    fc = nullptr;
                } else {
                    if ( fc->leaf )
                        c->leaf = true;
                    mergeHelper( fc, c );
                }
            }
        }

      public:
        explicit Trie() {}
        ~Trie() { clear(); }
        void clear() {
            // CAUTION: potential problem with stack overflow, use std::deque< Node* > nodes to iterate over all data
            for ( Node*& child : root.children ) {
                delete child;
                child = nullptr;
            }
        }
        bool is_empty() const {
            for ( auto c : root.children ) {
                if ( c )
                    return false;
            }
            return true;
        }
        std::size_t size() const {
            std::size_t count = 0;
            std::deque< Node const* > nodes;
            for ( Node* child : root.children ) {
                if ( child )
                    nodes.push_back( child );
            }

            while ( !nodes.empty() ) {
                Node const* n = nodes.front();
                nodes.pop_front();
                if ( n->leaf )
                    ++count;
                for ( Node* child : n->children ) {
                    if ( child )
                        nodes.push_back( child );
                }
            }
            return count;
        }
        bool emplace( std::string_view word ) { return insert( word ); }
        bool insert( std::string_view word ) {
            Node* node = &root;
            for ( char c : word ) {
                std::size_t idx = c - 'a';
                // make a new path if doesn't exist yet
                if ( !node->children[ idx ] )
                    node->children[ idx ] = new Node();
                node = node->children[ idx ];
            }
            if ( node->leaf )
                return false; // already present
            else {
                node->leaf = true;
                return true; // just inserted
            }
        }
        bool contains( std::string_view word ) const {
            Node const* node = &root;
            for ( char c : word ) {
                std::size_t idx = c - 'a';
                if ( !node->children[ idx ] )
                    return false;
                node = node->children[ idx ];
            }
            return node->leaf;
        }

        void merge( Trie& other ) {
            if ( other.is_empty() )
                return;
            mergeHelper( &other.root, &root );
        }
    };

} // namespace util

#endif
