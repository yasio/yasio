/*
Copyright (c) 2009 Dave Gamble

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#ifndef cJSON__h
#define cJSON__h

#include <string>

#ifdef __cplusplus
extern "C"
{
#endif

    /* cJSON Types: */
    enum JsonValueType { 
        cJSON_NULL,
        cJSON_Boolean,
        cJSON_Integer,
		cJSON_Real,
        cJSON_String,
        cJSON_Array,
        cJSON_Object,	
        cJSON_IsReference = 256
    };

    /* The cJSON structure: */
    typedef struct cJSON {
        struct cJSON *next,*prev;	/* next/prev allow you to walk array/object chains. Alternatively, use GetArraySize/GetArrayItem/GetObjectItem */
        struct cJSON *child,*parent;		/* An array or object item will have a child pointer pointing to a chain of the items in the array/object. */

        int type;					/* The type of the item, as above. */
        union {
            bool     valuebool;   /* cJSON_Boolean */
            double   valuedouble; /* cJSON_Real */
			int       valueint; /* cJSON_Integer normal integer */
			long long valuelonglong; /* cJSON_Integer long long */
            char*    valuestring; /* cJSON_String */
        } value;

        char *string;				/* The item's name string, if this item is the child of, or is in the list of subitems of an object. */
    } cJSON;

    typedef struct cJSON_Hooks {
        void *(*malloc_fn)(size_t sz);
        void (*free_fn)(void *ptr);
    } cJSON_Hooks;

    /* Supply malloc, realloc and free functions to cJSON */
    extern void cJSON_InitHooks(cJSON_Hooks* hooks);

    extern void *(*cJSON_malloc)(size_t sz);
    extern void (*cJSON_free)(void *ptr);

    extern int cJSON_strcasecmp(const char *s1,const char *s2);
    extern char* cJSON_strdup(const char* str);
    extern char* cJSON_strreset(char* _Original, const char* _New);

    /* Supply a block of JSON, and this returns a cJSON object you can interrogate. Call cJSON_Delete when finished. */
    extern cJSON *cJSON_Parse(const char *value);
    /* Render a cJSON entity to text for transfer/storage. Free the char* when finished. */
    extern char  *cJSON_Print(cJSON *item, bool formatted = false);
    /* Render a cJSON entity to text for transfer/storage without any formatting. Free the char* when finished. */
    extern char  *cJSON_PrintUnformatted(cJSON *item);
    /* Delete a cJSON entity and all subentities. */
    extern void   cJSON_Delete(cJSON *c);

    /* Returns the number of items in an array (or object). */
    extern int	  cJSON_GetArraySize(cJSON *array);
    /* Retrieve item number "item" from array "array". Returns NULL if unsuccessful. */
    extern cJSON *cJSON_GetArrayItem(cJSON *array,int item);
    extern cJSON *cJSON_GetArrayItemByField(cJSON *array, const char* field);
    /* Get item "string" from object. Case insensitive. */
    extern cJSON *cJSON_GetObjectItem(cJSON *object,const char *string);

    /* For analysing failed parses. This returns a pointer to the parse error. You'll probably need to look a few chars back to make sense of it. Defined when cJSON_Parse() returns 0. 0 when cJSON_Parse() succeeds. */
    extern const char *cJSON_GetErrorPtr(void);

    /* These calls create a cJSON item of the appropriate type. */
    extern cJSON *cJSON_CreateNode(int type);
    extern cJSON *cJSON_CreateNull(void);
    extern cJSON *cJSON_CreateTrue(void);
    extern cJSON *cJSON_CreateFalse(void);
    extern cJSON *cJSON_CreateBool(int b);
    extern cJSON *cJSON_CreateReal(double num);
	extern cJSON *cJSON_CreateInteger(long long num);
    extern cJSON *cJSON_CreateString(const char *string);
    extern cJSON *cJSON_CreateArray(void);
    extern cJSON *cJSON_CreateObject(void);

    /* These utilities create an Array of count items. */
    extern cJSON *cJSON_CreateIntArray(const int *numbers,int count);
    extern cJSON *cJSON_CreateFloatArray(const float *numbers,int count);
    extern cJSON *cJSON_CreateDoubleArray(const double *numbers,int count);
    extern cJSON *cJSON_CreateStringArray(const char **strings,int count);

    /* Append item to the specified array/object. */
    extern cJSON* cJSON_AddItemToArray(cJSON *array, cJSON *item);
    extern cJSON* cJSON_AddItemToObject(cJSON *object,const char *string,cJSON *item);
    /* Append reference to item to the specified array/object. Use this when you want to add an existing cJSON to a new cJSON, but don't want to corrupt your existing cJSON. */
    extern void cJSON_AddItemReferenceToArray(cJSON *array, cJSON *item);
    extern void	cJSON_AddItemReferenceToObject(cJSON *object,const char *string,cJSON *item);

    /* Remove/Detatch items from Arrays/Objects. */
    extern cJSON *cJson_detachItemFromArray(cJSON *array,int which);
    extern void   cJSON_DeleteItemFromArray(cJSON *array,int which);
    extern cJSON *cJSON_DetachItemFromObject(cJSON *object,const char *string);
    extern void   cJSON_DeleteItemFromObject(cJSON *object,const char *string);

    /* Update array items. */
    extern void cJSON_ReplaceItemInArray(cJSON *array,int which,cJSON *newitem);
    extern void cJSON_ReplaceItemInObject(cJSON *object,const char *string,cJSON *newitem);

    /* Duplicate a cJSON item */
    extern cJSON *cJSON_Duplicate(cJSON *item,int recurse);
    /* Duplicate will create a new, identical cJSON item to the one you pass, in new memory that will
    need to be released. With recurse!=0, it will duplicate any children connected to the item.
    The item->next and ->prev pointers are always zero on return from Duplicate. */

    /* ParseWithOpts allows you to require (and check) that the JSON is null terminated, and to retrieve the pointer to the final byte parsed. */
    extern cJSON *cJSON_ParseWithOpts(const char *value,const char **return_parse_end,int require_null_terminated);

    extern void cJSON_Minify(char *json);

    extern void cJSON_thread_init(void);
    extern void cJSON_thread_end(void);

    /* Macros for creating things quickly. */
#define cJSON_AddNullToObject(object,name)		cJSON_AddItemToObject(object, name, cJSON_CreateNull())
#define cJSON_AddTrueToObject(object,name)		cJSON_AddItemToObject(object, name, cJSON_CreateTrue())
#define cJSON_AddFalseToObject(object,name)		cJSON_AddItemToObject(object, name, cJSON_CreateFalse())
#define cJSON_AddBoolToObject(object,name,b)	cJSON_AddItemToObject(object, name, cJSON_CreateBool(b))
#define cJSON_AddNumberToObject(object,name,n)	cJSON_AddItemToObject(object, name, cJSON_CreateNumber(n))
#define cJSON_AddStringToObject(object,name,s)	cJSON_AddItemToObject(object, name, cJSON_CreateString(s))

    /* When assigning an integer value, it needs to be propagated to valuedouble too. */
#define cJSON_SetIntValue(object,val)			((object)?(object)->value.valueint=(val):(val))
#define cJSON_SetBoolValue(object,val)			((object)?(object)->value.valuebool=(val):(val))
#define cJSON_SetRealValue(object,val)			((object)?(object)->value.valuedouble=(val):(val))
#ifdef __cplusplus
}
#endif

#endif
