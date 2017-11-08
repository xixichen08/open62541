/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stdio.h>
#include <stdlib.h>

#include "ua_types.h"
#include "ua_config_default.h"
#include "check.h"

#ifdef __clang__
//required for ck_assert_ptr_eq and const casting
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wincompatible-pointer-types-discards-qualifiers"
#endif

START_TEST(Server_addNamespace_ShallWork) {
    UA_ServerConfig *config = UA_ServerConfig_new_default();
    UA_Server *server = UA_Server_new(config);

    UA_UInt16 a = UA_Server_addNamespace(server, "http://nameOfNamespace");
    UA_UInt16 b = UA_Server_addNamespace(server, "http://nameOfNamespace");
    UA_UInt16 c = UA_Server_addNamespace(server, "http://nameOfNamespace2");

    ck_assert_uint_gt(a, 0);
    ck_assert_uint_eq(a,b);
    ck_assert_uint_ne(a,c);

    UA_Server_delete(server);
    UA_ServerConfig_delete(config);
}
END_TEST

START_TEST(Server_addNamespace_writeService) {
    UA_ServerConfig *config = UA_ServerConfig_new_default();
    UA_Server *server = UA_Server_new(config);

    UA_Variant namespaces;
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_Server_readValue(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_NAMESPACEARRAY),
                        &namespaces);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_ptr_eq(namespaces.type, &UA_TYPES[UA_TYPES_STRING]);

    namespaces.data = UA_realloc(namespaces.data, (namespaces.arrayLength + 1) * sizeof(UA_String));
    ++namespaces.arrayLength;
    UA_String *ns = (UA_String*)namespaces.data;
    ns[namespaces.arrayLength-1] = UA_STRING_ALLOC("test");
    size_t nsSize = namespaces.arrayLength;

    retval = UA_Server_writeValue(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_NAMESPACEARRAY),
                                  namespaces);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_Variant_deleteMembers(&namespaces);

    /* Now read again */
    UA_Server_readValue(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_NAMESPACEARRAY),
                        &namespaces);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(namespaces.arrayLength, nsSize);

    UA_Variant_deleteMembers(&namespaces);
    UA_Server_delete(server);
    UA_ServerConfig_delete(config);
}
END_TEST

struct nodeIterData {
    UA_NodeId id;
    UA_Boolean isInverse;
    UA_NodeId referenceTypeID;
    UA_Boolean hit;
};
#define NODE_ITER_DATA_SIZE 3

static UA_StatusCode
nodeIter(UA_NodeId childId, UA_Boolean isInverse, UA_NodeId referenceTypeId, void *handle) {
    struct nodeIterData* objectsFolderChildren = ( struct nodeIterData*)handle;

    ck_assert_int_eq(childId.namespaceIndex, 0);
    ck_assert(childId.identifierType == UA_NODEIDTYPE_NUMERIC);

    int i;

    for (i=0; i<NODE_ITER_DATA_SIZE; i++) {
        if (UA_NodeId_equal(&childId, &objectsFolderChildren[i].id)) {
            break;
        }
    }
    ck_assert_int_lt(i, NODE_ITER_DATA_SIZE);

    ck_assert(objectsFolderChildren[i].isInverse == isInverse);

    ck_assert(!objectsFolderChildren[i].hit);
    objectsFolderChildren[i].hit = UA_TRUE;

    ck_assert(UA_NodeId_equal(&referenceTypeId, &objectsFolderChildren[i].referenceTypeID));

    return UA_STATUSCODE_GOOD;
}

START_TEST(Server_forEachChildNodeCall) {

    UA_ServerConfig *config = UA_ServerConfig_new_default();
    UA_Server *server = UA_Server_new(config);

    /* List all the children/references of the objects folder
     * The forEachChildNodeCall has to hit all of them */
    struct nodeIterData objectsFolderChildren[] = {
        {
                UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER),
                UA_FALSE,
                UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                UA_FALSE
        },{
                UA_NODEID_NUMERIC(0, UA_NS0ID_ROOTFOLDER),
                UA_TRUE,
                UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                UA_FALSE
        },{
                UA_NODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE),
                UA_FALSE,
                UA_NODEID_NUMERIC(0, UA_NS0ID_HASTYPEDEFINITION),
                UA_FALSE
        }
    };

    UA_StatusCode retval = UA_Server_forEachChildNodeCall(server, UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER), nodeIter, &objectsFolderChildren);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

    /* Check if all nodes are hit */
    for (int i=0; i<NODE_ITER_DATA_SIZE; i++) {
        ck_assert(objectsFolderChildren[i].hit);
    }

    UA_Server_delete(server);
    UA_ServerConfig_delete(config);

} END_TEST


static Suite* testSuite_ServerUserspace(void) {
    Suite *s = suite_create("ServerUserspace");
    TCase *tc_core = tcase_create("Core");
    tcase_add_test(tc_core, Server_addNamespace_ShallWork);
    tcase_add_test(tc_core, Server_addNamespace_writeService);
    tcase_add_test(tc_core, Server_forEachChildNodeCall);

    suite_add_tcase(s,tc_core);
    return s;
}

int main(void) {
    int number_failed = 0;

    Suite *s;
    SRunner *sr;

    s = testSuite_ServerUserspace();
    sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr,CK_NORMAL);
    number_failed += srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

#ifdef __clang__
#pragma clang diagnostic pop
#endif
