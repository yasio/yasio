#ifndef _CJSON_WRAPPER_H_
#define _CJSON_WRAPPER_H_
#include "cjson.h"
#include "simpleppdef.h"
#include <sstream>
#include <iomanip>

namespace cjsonwrapper{};
namespace cjsonw = cjsonwrapper;
namespace jsonw = cjsonw;
namespace cjsonwrapper {
    
    typedef cJSON node_type;
    typedef class object value_type;
    class document;
    class object
    {
        friend class document;
    public:
        static const object empty_value;
    public:
        object(JsonValueType type);
        object(node_type* ref = nullptr);
		object(const object& right);
        object(int);
		object(long long);
        object(bool);
        object(double);
        object(const char*);
        object(const std::string&);

        object& operator=(node_type*);
        object& operator=(int);
        object& operator=(bool);
        object& operator=(double);
        object& operator=(const char*);
        object& operator=(const std::string&);
		object& operator=(object& obj);

        bool    parseFile(const char* filename);
        bool    parseString(const char* value);

        JsonValueType get_type(void) const;

        ~object(void);

        object      operator[](int index) const;
        object      operator[](const char* name) const;
        object      operator[](const std::string& key) const;
        object      operator[](const char* name);
        object      operator[](const std::string &key );

        /* set 'item' value */

        void        setValue(int);
        void        setValue(bool);
        void        setValue(double);
        void        setValue(const char*);
        void        setValue(const std::string&);
        void        setValue(const object& value);

        /* append item of array */
        void        append(int);
        void        append(double);
        void        append(bool);
        void        append(const char*);
        void        append(const std::string&);
        void        append(object&);

        /* insert named object */
        bool        insert(const char* key, object&);

        bool        empty(void) const;
        bool        isNil(void) const;
        bool        isNotNil(void) const;

        /* Get 'item' for key */
        bool        getBool(const char* key, bool defaultValue = false) const;
        int         getInt(const char* key, int defaultValue = 0) const;
		long long   getLargestInt(const char* key, long long defaultValue = 0) const;
        float       getFloat(const char* key, float defaultValue = 0.0f) const;
        const char* getString(const char* key, const char* defaultValue = "") const;
        object      getObject(const char* key, const object& defaultValue = empty_value) const;
        object      getObjectByValue(const char* key, int value) const;

        /* Get 'item' value for index */
        bool        getBool(int index, bool defaultValue = false) const;
        int         getInt(int index, int defaultValue = 0) const;
        long long   getLargestInt(int index, long long defaultValue = 0) const;
        float       getFloat(int index, float defaultValue = 0.0f) const;
        const char* getString(int index, const char* defaultValue = "") const;
        object      getObject(int index) const;

        /// Get int, float, string, value
        bool        asBool(bool defaultValue = false) const;
        int         asInt(int defaultValue = 0) const;
		long long   asLargestInt(long long defaultValue = 0) const;
        float       asFloat(float defaultValue = 0.0f) const;
        const char* asString(const char* defaultValue = "") const;

        /* Returns the number of items in an array (or object). */
        int         count(void) const;

        // Get name of object
        const char* getName(void) const;

        bool        hasMember(const char* name) const;

        void        remove(int index);
        void        remove(const char* key);
        void        remove(const std::string& key);

        std::string toString(bool formatted = false) const;

        object      clone(void) const;

        node_type*  release(void);

        operator    node_type*(void);
        operator    const node_type*(void) const;

        /*operator    std::string(void);
        operator const std::string(void) const;*/

        template<typename _Operation>
        void cforeach(_Operation op) const
        {
            if( _IsNotNull(entity) ) {
                node_type* child = entity->child;
                while( _IsNotNull(child) )
                {
                    op(child);
                    child = child->next;
                }
            }
        }

    private:
        // addChild For Array
        object      addObject(int type);

        // addChild For Object
        object      addObject(const char* key, int type);

    private:
        node_type* entity;
    }; /* class modern::json::object */

    class document
    {
    public:
        document(void);
        document(const char* filename);
        ~document(void);

        bool              open(const char* filename);

        bool              isOpen(void) const { return _Root.isNotNil(); }

        /**
        @brief set value by key, if the key exist, update, else add new.
        */
        void              insert(const char* key, const int& value);

        /**
        @brief set value by key, if the key exist, update, else add new.
        */
        void              insert(const char* key, const bool& value);

        /**
        @brief set value by key, if the key exist, update, else add new.
        */
        void              insert(const char* key, const float& value);

        /**
        @brief set value by key, if the key exist, update, else add new.
        */
        void              insert(const char* key, const char* value);

        /**
        @brief get value by key, if the key doesn't exist, create new.
        */
        bool              get(const char* key, bool defaultValue);

        int               get(const char* key, int value);

        float             get(const char* key, float value);

        const char*       get(const char* key, const char* value);

        /**
        @brief erase value by key, if the key doesn't exist, do nothing.
        */
        void              erase(const char* key);

        /**
        @brief whether contains key, if the key doesn't exist, do nothing.
        */
        bool              contains(const char* key) const;           

        void              flush(const char* filename = nullptr);

        void              close(void);

        object&           getRoot(void) { return _Root; };
        const object&     getRoot(void) const { return _Root; };

        std::string       toString(bool formatted = false) const;

        /* For analysing failed parses. This returns a pointer to the parse error. You'll probably need to look a few chars back to make sense of it. Defined when Json_create() returns 0. 0 when Json_create() succeeds. */
        static const char* getLastError (void);

    private:
        object      _Root;
        std::string filename;
    }; /* class ccjson::json::document */

    void init_cjson_hooks(void);

}; /* namespace ccjson */


#endif

