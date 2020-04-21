#include "zm_utils.h"
#include "zm_storage.h"
#include "KV/zm_storage_kv.h"

void storageTest(zmStorage* storage){
    zmKVDBConf conf;
    strncpy(conf.name, "KVDB", ZM_KV_DB_NAME_LEN);
    strncpy(conf.path, "./demo.db", ZM_KV_DB_PATH_LEN);
    conf.dataSource = KV_FILE_FORMAT;
    newzmStorage(storage, ZM_DB_TYPE_KV, &conf);

    char *data = "{\"Time\":\"123\",\"id\":\"11\",\"Data\":{\"id\":\"11\",\"name\":\"ewe\",\"gender\":\"grd\",\"age\":\"34\"}}";
    char *data1 = "{\"Time\":\"123\",\"id\":\"12\",\"Data\":{\"id\":\"13\",\"name\":\"ewe\",\"gender\":\"grd\",\"age\":\"34\"}}";
    char *data2 = "{\"Time\":\"123\",\"id\":\"13\",\"Data\":{\"id\":\"12\",\"name\":\"ewe\",\"gender\":\"grd\",\"age\":\"34\"}}";

    char *data3  = "5_ewe_grd_38";
    char *data4  = "6_ewe_grd_38|8_yyy_ttt_99|7_ewe_grd_100";
    zmDB* db =  getzmStorage(storage, "KVDB");
    zmKVContainer* containor = db->data;
    
    zmKVRecord* record = createKVRecord(data, KV_FILE_DATA);
    zmKVRecord* record1 = createKVRecord(data1, KV_FILE_DATA);
    zmKVRecord* record2 = createKVRecord(data2, KV_FILE_DATA);
    zmKVRecord* record3 = createKVRecord(data3, KV_HTTP_DATA);
    db->add(db, record);
    db->add(db, record1);
    db->add(db, record2);
    db->add(db, record3);
    db->save(db, containor->path);

    zmKVDBQuery query;
    char* queryBuff = malloc(ZM_KV_DB_RECORD_LEN);
    initzmKVDBQuery(&query, 0, 1, 10, KV_HTTP_DATA, queryBuff, ZM_KV_DB_RECORD_LEN);
    printf("query init sucess %p\n", record);
    db->query(db, &query);
    printf("query sucess %s\n", query.result);
    destoryzmKVDBQuery(&query);
}

int main(int argc, char** argv) {
    zmStorage storage;
    storage.db = NULL;
    storage.next = NULL;
    storageTest(&storage);
    return 0;
}

