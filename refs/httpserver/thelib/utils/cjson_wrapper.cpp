//#include <cocos2d.h>
#include "simpleppdef.h"
#include "simple_ptr.h"
#include "nsconv.h"
#include "xxfsutility.h"
#include "oslib.h"
#include "cjson_wrapper.h"
#include <assert.h>
#define NEX_UNREACHABLE assert(false)
using namespace thelib::gc;
using namespace cjsonwrapper;

const cjsonw::value_type object::empty_value;

object::object(JsonValueType type)
{
    switch ( type )
    {
    case cJSON_NULL:
        this->entity = cJSON_CreateNull();
        break;
    case cJSON_Integer:
        this->entity = cJSON_CreateInteger(0);
        break;
	case cJSON_Real:
		this->entity = cJSON_CreateReal(0.0);
		break;
    case cJSON_String:
        this->entity = cJSON_CreateString(nullptr);
        break;
    case cJSON_Boolean:
        this->entity = cJSON_CreateBool(0);
        break;
    case cJSON_Object:
        this->entity = cJSON_CreateObject();
        break;
    case cJSON_Array:
        this->entity = cJSON_CreateArray();
        break;
    default:
        NEX_UNREACHABLE;
    }
}

bool object::parseFile(const char* filename)
{
    // std::string path = CCFileUtils::sharedFileUtils()->fullPathForFilename(filename);

    unsigned long size = 0;
    // simple_ptr<unsigned char> data(CCFileUtils::sharedFileUtils()->getFileData(path.c_str(), "rt", &size) );
    managed_cstring data (fsutil::read_file_data(filename) );
    if(data.c_str() != nullptr) 
    {
        return this->parseString((char*)data.c_str());
    }
    // CCLOG("xg::utils::json::document: failed to open file: %s", filename);
    return false;
}

bool object::parseString(const char* value)
{
    return (this->entity = cJSON_Parse(value)) != nullptr;
}

JsonValueType object::get_type(void) const
{
    return (JsonValueType)this->entity->type;
}

object::object(node_type* entity)
{
    this->entity = entity;
}

object::object(const object& right)
{
	this->entity = right.entity;
}

object::object(int value)
{
    this->entity = cJSON_CreateInteger(value);
}

object::object(bool value)
{
    this->entity = cJSON_CreateBool(value);
}

object::object(double value)
{
    this->entity = cJSON_CreateReal(value);
}

object::object(const char* value)
{
    this->entity = cJSON_CreateString(value);
}

object::object(const std::string& value)
{
    this->entity = cJSON_CreateString(value.c_str());
}

object::~object(void)
{
    this->release();
}

object& object::operator=(node_type* entity)
{
    if(this->isNil())
    {
        this->entity = entity;
    }
    else {

    }
    return *this;
}

object& object::operator=(int value)
{
    this->setValue(value);
    return *this;
}

object& object::operator=(bool value)
{
    this->setValue(value);
    return *this;
}

object& object::operator=(double value)
{
	this->setValue(value);
	return *this;
}

object& object::operator=(const char* value)
{
    this->setValue(value);
    return *this;
}

object& object::operator=(const std::string& value)
{
    this->setValue(value);
    return *this;
}

object& object::operator=(object& value)
{
	this->setValue(value);
	return *this;
}

void        object::setValue(int value)
{
    if(_IsNotNull(this->entity))
    {
        switch(this->entity->type)
        {
        case cJSON_NULL:
            this->entity->type = cJSON_Integer;
        case cJSON_Integer:
            cJSON_SetIntValue(this->entity, value);
            break;
        default:;
        }
    }
}

void        object::setValue(bool value)
{
    if(_IsNotNull(this->entity))
    {
        switch(this->entity->type)
        {
        case cJSON_NULL:
            this->entity->type = cJSON_Boolean;
        case cJSON_Boolean:
            cJSON_SetIntValue(this->entity, value);
            break;
        default:;
        }
    }
}

void        object::setValue(double value)
{
    if(_IsNotNull(this->entity))
    {
        switch(this->entity->type)
        {
        case cJSON_NULL:
            this->entity->type = cJSON_Real;
        case cJSON_Real:
            cJSON_SetRealValue(this->entity, value);
            break;
        default:;
        }
    }
}

void        object::setValue(const char* value)
{
    if(_IsNotNull(this->entity))
    {
        switch(this->entity->type)
        {
        case cJSON_NULL:
            this->entity->type = cJSON_String;
        case cJSON_String:
            this->entity->value.valuestring = cJSON_strreset(this->entity->value.valuestring, value);
            break;
        default:;
        }
    }
}

void        object::setValue(const std::string& value)
{
    if(_IsNotNull(this->entity))
    {
        switch(this->entity->type)
        {
        case cJSON_NULL:
            this->entity->type = cJSON_String;
        case cJSON_String:
            this->entity->value.valuestring = cJSON_strreset(this->entity->value.valuestring, value.c_str());
            break;
        default:;
        }
    }
}

void  object::setValue(const object& value)
{
    if(_IsNotNull(this->entity))
    {
        this->entity->type = value.get_type();
        
        switch(this->entity->type)
        {
        //case cJSON_NULL:
        //    this->entity->type = value.get_type();
        //    break;
        case cJSON_String:
            this->entity->value.valuestring = cJSON_strreset(this->entity->value.valuestring, value.asString());
            break;
        case cJSON_Integer:
			this->entity->value.valuelonglong = value.asLargestInt();
            break;
		case cJSON_Real:
			this->entity->value.valuedouble = value.asFloat();
			break;
        case cJSON_Boolean:
            this->entity->value.valuebool = value.asBool();
            break;
		case cJSON_Array:
			this->entity->child = value.entity->child;
			value.entity->child = nullptr;
			break;
        default:;
        }
    }
}

//void object::setValue(const object& value)
//{
//}

object object::operator[](int index) const
{
    return this->getObject(index);
}

object object::operator[](const char* name) const
{
    return this->getObject(name);
}

object object::operator[](const std::string& key) const
{
    return this->getObject(key.c_str());
}

object object::operator[](const char* name)
{
    object obj = this->getObject(name);
    if(obj.isNil())
    {
        return this->addObject(name, cJSON_NULL);
    }
    return obj;
}

object object::operator[](const std::string& name)
{
    return this->operator[](name.c_str());
}

/* append object */
void        object::append(int value)
{
    object obj = value;
    this->append(obj);
}

void        object::append(double value)
{
    object obj = value;
    this->append(obj);
}

void        object::append(bool value)
{
    object obj = value;
    this->append(obj);
}

void        object::append(const char* value)
{
    object obj = value;
    this->append(obj);
}

void        object::append(const std::string& value)
{
    object obj = value;
    this->append(obj);
}

void        object::append(object& obj)
{
	if(_IsNull(this->entity))
		this->entity = cJSON_CreateArray();

    if(_IsNotNull(this->entity))
    {
        switch(this->entity->type)
        {
        case cJSON_NULL:
            this->entity->type = cJSON_Array;
        case cJSON_Array:
            cJSON_AddItemToArray(this->entity, obj);
            break;
        default:;
        }
    }
}


/* insert named object */
bool        object::insert(const char* key, object& obj)
{
    if(_IsNotNull(this->entity))
    {
        switch(this->entity->type)
        {
        case cJSON_NULL:
            this->entity->type = cJSON_Object;
        case cJSON_Object:
            return _IsNotNull(cJSON_AddItemToObject(this->entity, key, obj) );
        default:;
        }
    }
    return false;
}


/* Get 'item' for key */
bool        object::getBool(const char* key, bool defaultValue) const
{
    return this->getObject(key).asBool(defaultValue);
}
int         object::getInt(const char* key, int defaultValue) const
{
    return this->getObject(key).asInt(defaultValue);
}
long long   object::getLargestInt(const char* key, long long defaultValue) const
{
	return this->getObject(key).asLargestInt(defaultValue);
}
float       object::getFloat(const char* key, float defaultValue) const
{
    return this->getObject(key).asFloat(defaultValue);
}
const char* object::getString(const char* key, const char* defaultValue) const
{
    return this->getObject(key).asString(defaultValue);
}
object      object::getObject(const char* key, const object& defaultValue) const
{
    if(_IsNotNull(this->entity)) {
        if(this->entity->type == cJSON_Object)
        {
            return cJSON_GetObjectItem(this->entity, key);
        }
    }
    return defaultValue;
}

object      object::getObjectByValue(const char* key, int value) const
{
    if(_IsNotNull(this->entity)) {
        if(this->entity->type == cJSON_Array)
        {
            object obj;
            this->cforeach([this,key,value,&obj](const object& child)->void{
                if(child.getInt(key) == value && !obj.isNotNil())
                {
                    obj = child;
                }
            });
            return obj;
        }
        else if(this->entity->type == cJSON_Object)
        {
            return cJSON_GetObjectItem(this->entity, key);
        }
    }
    return (node_type*)0;
}

/* Get 'item' value for index */
bool        object::getBool(int index, bool defaultValue) const
{
    return this->getObject(index).asBool(defaultValue);
}
int         object::getInt(int index, int defaultValue) const
{
    return this->getObject(index).asInt(defaultValue);
}
float       object::getFloat(int index, float defaultValue) const
{
    return this->getObject(index).asFloat(defaultValue);
}
const char* object::getString(int index, const char* defaultValue) const
{
    return this->getObject(index).asString(defaultValue);
}
object      object::getObject(int index) const
{
    if(_IsNotNull(this->entity)) {
        return cJSON_GetArrayItem(this->entity, index);
    }
    return (node_type*)0;
}

/// Get int, float, string, value
bool        object::asBool(bool defaultValue) const
{
    if (_IsNotNull(this->entity))
    {
        switch (this->entity->type)
        {
        case cJSON_Boolean:
        case cJSON_Integer:
		case cJSON_Real:
            return this->entity->value.valuebool;
        case cJSON_String:
            return (bool)atoi(this->entity->value.valuestring);
        default:; // do nothing
        }
    }
    return defaultValue;
}

int         object::asInt(int defaultValue) const
{
    if(_IsNotNull(this->entity))
    {
        switch(this->entity->type)
        {
        case cJSON_Integer:
			return (int)(this->entity->value.valueint);
		case cJSON_Real:
			return (int)(this->entity->value.valuedouble);
        case cJSON_String:
            return atoi(this->entity->value.valuestring);
        default: ; // do nothing
        }
    }
    return defaultValue;
}

long long   object::asLargestInt(long long defaultValue) const
{
	if(_IsNotNull(this->entity))
    {
        switch(this->entity->type)
        {
        case cJSON_Integer:
			return this->entity->value.valuelonglong;
		case cJSON_Real:
			return (long long) this->entity->value.valuedouble;
			break;
        case cJSON_String:
            return atoll(this->entity->value.valuestring);
        default: ; // do nothing
        }
    }
    return defaultValue;
}

float       object::asFloat(float defaultValue) const
{
    if(_IsNotNull(this->entity))
    {
        switch(this->entity->type)
        {
        case cJSON_Real:
            return (float) (this->entity->value.valuedouble);
		case cJSON_Integer:
			return (float) (this->entity->value.valueint);
        case cJSON_String:
            return (float) atof(this->entity->value.valuestring);
        default: ; // do nothing
        }
    }
    return defaultValue;
}

const char* object::asString(const char* defaultValue) const
{
    if(_IsNotNull(this->entity) && this->entity->type == cJSON_String) {
        return this->entity->value.valuestring;
    }
    return defaultValue;
}

void        object::remove(const char* key)
{
    if(_IsNotNull(this->entity)) {
        cJSON_DeleteItemFromObject(this->entity, key);
    }
}

void        object::remove(const std::string& key)
{
    this->remove(key.c_str());
}

bool  object::hasMember(const char* name) const
{
    return this->getObject(name).isNotNil();
}

const char* object::getName(void) const
{
    if(_IsNotNull(this->entity)) {
        return this->entity->string;
    }
    return "";
}

int object::count(void) const
{
    return cJSON_GetArraySize(this->entity);
}

object object::addObject(int type)
{
    if( _IsNotNull(this->entity) )
    {
        switch(this->entity->type)
        {
        case cJSON_NULL:
            this->entity->type = cJSON_Array;
        case cJSON_Array:
            return cJSON_AddItemToArray(this->entity, cJSON_CreateNode(type));
        default:;
        }
    }
    return (node_type*)0;
}

object object::addObject(const char* name, int type)
{
	if(_IsNull(this->entity))
		this->entity = cJSON_CreateObject();
    if(_IsNotNull(this->entity))
    {
        switch(this->entity->type)
        {
        case cJSON_NULL:
            this->entity->type = cJSON_Object;
        case cJSON_Object:
            return cJSON_AddItemToObject(this->entity, name, cJSON_CreateNode(type));
        default:;
        }
    }
    return (node_type*)0;
}

std::string object::toString(bool formatted) const
{
    if(_IsNotNull(this->entity)) {
        char* text = cJSON_Print(this->entity, formatted);
        std::string valueret(text);
        cJSON_free(text);
        return std::move(valueret);
    }
    return "";
}

//object::operator    std::string(void)
//{
//    return this->toString();
//}
//
//object::operator const std::string(void) const
//{
//    return this->toString();
//}

object object::clone(void) const
{
    if(_IsNotNull(this->entity))
    {
        cJSON_Duplicate(this->entity, true);
    }
    return (node_type*)0;
}

node_type* object::release(void)
{
    if(this->isNotNil() && _IsNull(this->entity->parent)) 
    {
        cJSON_Delete(this->entity); 
		return nullptr;
    }
	auto temp = this->entity;
	this->entity = nullptr;
	return temp;
}

object::operator node_type*(void)
{
    return this->entity;
}

object::operator const node_type*(void) const
{
    return this->entity;
}

bool object::empty(void) const
{
    if(this->isNil()) {
        return true;
    }

    switch(this->entity->type)
    {
    case cJSON_NULL:
    case cJSON_Array:
    case cJSON_Object:
        return 0 == this->count();
    default:;
    }
    return false;
}

bool object::isNil(void) const
{
    return _IsNull(this->entity) || this->entity == empty_value.entity;
}

bool object::isNotNil(void) const
{
    return !this->isNil();
}

/* ---------------------- friendly split line ------------------------- */

document::document(void) : _Root((node_type*)0)
{
}

document::document(const char* filename)
{
    this->open(filename);
}

document::~document(void)
{
    this->close();
}

bool document::open(const char* filename)
{
    if(!this->isOpen()) {
        this->filename = filename;
        if(!this->_Root.parseFile(filename))
        {
            this->_Root = cJSON_CreateObject();
        }
    }
    return true;
}

/**
@brief set value by key, if the key exist, update, else add new.
*/
//void  document::insert(const char* key, const value_type& value)
//{
//
//    object newObj;
//    bool hasMember = this->_Root.hasMember(key);
//    if(this->_Root.hasMember(key))
//    { 
//        newObj = this->_Root.getObject(key);
//    }
//    else {
//        newObj = this->_Root.addObject(key, value.get_type());
//    }
//
//    newObj.setValue(value);
//    this->flush();
//
//}

/**
@brief set value by key, if the key exist, update, else add new.
*/
void              document::insert(const char* key, const int& value)
{
    object newObj;
    bool hasMember = this->_Root.hasMember(key);
    if(this->_Root.hasMember(key))
    { 
        newObj = this->_Root.getObject(key);
    }
    else {
        newObj = this->_Root.addObject(key, cJSON_Integer);
    }

    newObj.setValue(value);
    this->flush();
}

/**
@brief set value by key, if the key exist, update, else add new.
*/
void              document::insert(const char* key, const bool& value)
{
    object newObj;
    bool hasMember = this->_Root.hasMember(key);
    if(this->_Root.hasMember(key))
    { 
        newObj = this->_Root.getObject(key);
    }
    else {
        newObj = this->_Root.addObject(key, cJSON_Boolean);
    }

    newObj.setValue(value);
    this->flush();
}

/**
@brief set value by key, if the key exist, update, else add new.
*/
void              document::insert(const char* key, const float& value)
{
    object newObj;
    bool hasMember = this->_Root.hasMember(key);
    if(this->_Root.hasMember(key))
    { 
        newObj = this->_Root.getObject(key);
    }
    else {
        newObj = this->_Root.addObject(key, cJSON_Integer);
    }

    newObj.setValue(value);
    this->flush();
}

/**
@brief set value by key, if the key exist, update, else add new.
*/
void              document::insert(const char* key, const char* value)
{
    object newObj;
    bool hasMember = this->_Root.hasMember(key);
    if(hasMember)
    { 
        newObj = this->_Root.getObject(key);
    }
    else {
        newObj = this->_Root.addObject(key, cJSON_String);
    }

    newObj.setValue(value);
    this->flush();
}

bool              document::get(const char* key, bool defaultValue)
{
    return this->_Root[key].asBool(defaultValue);
}

int               document::get(const char* key, int defaultValue)
{
    return this->_Root[key].asInt(defaultValue);
}

float             document::get(const char* key, float defaultValue)
{
    return this->_Root[key].asFloat(defaultValue);
}

const char*       document::get(const char* key, const char* defaultValue)
{
    return this->_Root[key].asString(defaultValue);
}

/**
@brief erase value by key, if the key doesn't exist, do nothing.
*/
void document::erase(const char* key)
{
    this->_Root.remove(key);
    this->flush();
}

/**
@brief whether contains key, if the key doesn't exist, do nothing.
*/
bool document::contains(const char* key) const
{
    return _Root.hasMember(key);
}

void document::flush(const char* file)
{
    if(!this->isOpen()) {
        return;
    }
    std::string data = this->toString(true);
    if(file == nullptr)
    {
        fsutil::write_file_data(this->filename.c_str(), data.c_str(), data.size());
    }
    else {
        fsutil::write_file_data(file, data.c_str(), data.size());
    }
}

void document::close(void)
{
    if(this->isOpen())
    {
        this->flush();
        // this->_Root.release(); warnning: // no need to release, ROOT release self
    }
}

std::string document::toString(bool formatted) const
{
    return this->_Root.toString(formatted);
}

//static void* palloc(size_t sz)
//{
//    return s_ngx_pool.get(sz);
//}
//static void pfree(void* ptr)
//{
//    return s_ngx_pool.release(ptr);
//}

void cjsonwrapper::init_cjson_hooks(void)
{
    /*cJSON_Hooks hooks;
    hooks.malloc_fn = palloc;
    hooks.free_fn = pfree;
    cJSON_InitHooks(&hooks);*/
}

//static const int _Ivv = (init_cjson_hooks(), 0);

