#ifndef __SYMTAB_HPP__
#define __SYMTAB_HPP__
#include <cstdio>
#include <cstring>
#include <map>
#include <cstdlib>
#include <iostream>
#include <cassert>
#include <vector>

class arrayDecl;
class exprNode;

/* A class that represents vector. Needed for parsing */
template <typename T>
class vecTemplate {
	protected:
		std::vector<T> vec_list;
	public:
		virtual void push_back (T);
		virtual void set_element (T, int);
		virtual std::vector<T> get_list (void);
};

template <typename T>
inline void vecTemplate<T>::push_back (T value) {
	vec_list.push_back (value);
}

template <typename T>
inline void vecTemplate<T>::set_element (T value, int pos) {
	assert ((int)vec_list.size () >= pos);
	vec_list[pos] = value;
}

template <typename T>
inline std::vector<T> vecTemplate<T>::get_list (void) {
	return vec_list;
} 

/* A class that represents a vector of args */
class argList : public vecTemplate<std::string> {
	private:
		std::vector<std::string> inouts;
	public:
		void push_inout (std::string);
		std::vector<std::string> get_inouts (void);
};

inline void argList::push_inout (std::string value) {
	std::cout << "Pushing " << value << std::endl;
	inouts.push_back (value);
}

inline std::vector<std::string> argList::get_inouts (void) {
	return inouts;
}

/* A class that represents symbol table. Basically a map from string to 
   any data structure. */
template <typename T>
class symtabTemplate {
	protected:
		std::map<std::string, T> symbol_map;
	public:
		void push_symbol (std::string, T);
		void push_symbol (char *, T);
		void delete_symbol (char *);
		void delete_symbol (std::string);
		void delete_symbols (void);
		T find_symbol (char *);
		T find_symbol (std::string);
		bool symbol_present (std::string);
		bool symbol_present (char *);	
		std::map<std::string, T> get_symbol_map (void); 
};

template <typename T>
inline std::map<std::string, T> symtabTemplate<T>::get_symbol_map (void) {
	return symbol_map;
}

template <typename T>
inline void symtabTemplate<T>::push_symbol (std::string s, T value) {
    if (symbol_map.find (s) != symbol_map.end ()) {
        assert (symbol_map[s]==value && "Pushing symbol that is already present with different type");
    }
	else 
		symbol_map.insert (make_pair (s, value));
}

template <typename T>
inline void symtabTemplate<T>::push_symbol (char *s, T value) {
	std::string key = std::string (s);
    if (symbol_map.find (key) != symbol_map.end ()) {
        assert (symbol_map[key]==value && "Pushing symbol that is already present with different type");
    }
    else 
		symbol_map.insert (make_pair (key, value));
}

template <typename T>
inline void symtabTemplate<T>::delete_symbol (char *s) {
	std::string key = std::string (s);
	if (symbol_map.find (key) != symbol_map.end ())
		symbol_map.erase (key);
}

template <typename T>
inline void symtabTemplate<T>::delete_symbol (std::string key) {
	if (symbol_map.find (key) != symbol_map.end ())
		symbol_map.erase (key);
}

template <typename T>
inline void symtabTemplate<T>::delete_symbols (void) {
	symbol_map.clear ();
}

template <typename T>
inline T symtabTemplate<T>::find_symbol (char *s) {
	std::string key = std::string (s);
	typename std::map <std::string, T>::iterator it = symbol_map.find (key);
	assert (it != symbol_map.end ());
	return it->second;
}

template <typename T>
inline T symtabTemplate<T>::find_symbol (std::string key) {
	typename std::map <std::string, T>::iterator it = symbol_map.find (key);
	assert (it != symbol_map.end ());
	return it->second;
}

template <typename T>
inline bool symtabTemplate<T>::symbol_present (char *s) {
	std::string key = std::string (s);
	typename std::map <std::string, T>::iterator it = symbol_map.find (key);
	return it != symbol_map.end ();
}

template <typename T>
inline bool symtabTemplate<T>::symbol_present (std::string key) {
	typename std::map <std::string, T>::iterator it = symbol_map.find (key);
	return it != symbol_map.end ();
}

#endif
