#ifndef _ZM_STORAGE_H_
#define _ZM_STORAGE_H_

typedef struct zmDB zmDB;
typedef int (*zmDBhandle)(struct  zmDB* db, void* argus);

/*
 *  The type enum of data storage structure.
 *  Use to specify which DB type you want to create.
 */
typedef enum _zmDBTyep
{
    /* Main body is linked, and use arrays to impove query. */
    ZM_DB_TYPE_KV=0
} zmDBType;

struct zmDB {
    char* name;
    // The data structure which DB based for.
    zmDBType category;

    // Data in the memory. 
    void* data;    
    
    // Add, del, save, query, modify.
    // Don't change the order of those function. 
    zmDBhandle load;
    zmDBhandle add;
    zmDBhandle del;
    zmDBhandle modify;
    zmDBhandle query;
    zmDBhandle save;
    zmDBhandle destory;
};

typedef struct _zmStorage {
  zmDB* db;
  struct _zmStorage* next;
} zmStorage;

/* New a Storage
 * @category: Data storage structure which you need.
 * @agrus: Some init agruments.
 */
int newzmStorage(zmStorage* storage, zmDBType category, void* agrus);

/* Actually, we have many storage objects. every object is a db which implement with different structure.
 * And you choose one of them.
 * @ storage: It's may be a head of a linked-list.
 * @ dbName: The name of db which you want to get.
 */
zmDB* getzmStorage(zmStorage* storage, char* dbName);

/*
 * Destory a storage
 * @storage: The storage which you want destory.
 */
int destoryzmStorage(zmStorage* storage);

#endif


