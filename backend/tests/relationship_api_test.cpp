#ifndef RELATIONSHIP_API_TEST_H
#define RELATIONSHIP_API_TEST_H

#include "../libs/httplib.h"
#include "../libs/json.hpp"
#include <iostream>
#include <cassert>
#include <string>
#include <thread>
#include "../include/common.h"

using json = nlohmann::json;

void run_relationship_test() {
    httplib::Client cli("localhost", 8080);
    cli.set_connection_timeout(5, 0);

    // 1. 创建两个进程 A、B
    json reqA = {{"name", "进程A"}, {"memory_size", 4096}};
    auto resA = cli.Post("/api/v1/processes", reqA.dump(), "application/json");
    assert(resA && resA->status == 201);
    ProcessID pidA = json::parse(resA->body)["data"]["pid"];

    json reqB = {{"name", "进程B"}, {"memory_size", 4096}};
    auto resB = cli.Post("/api/v1/processes", reqB.dump(), "application/json");
    assert(resB && resB->status == 201);
    ProcessID pidB = json::parse(resB->body)["data"]["pid"];

    // 2. 创建同步关系
    json relBody = {{"pid1", pidA}, {"pid2", pidB}, {"relation_type", "SYNC"}};
    auto relRes = cli.Post("/api/v1/processes/relationship", relBody.dump(), "application/json");
    assert(relRes && relRes->status == 201);

    // 查询关系列表接口
    auto relListRes = cli.Get("/api/v1/processes/relationships");
    assert(relListRes && relListRes->status == 200);
    {
        auto relListBody = json::parse(relListRes->body);
        assert(relListBody["status"] == "success");
        bool found = false;
        for (const auto& item : relListBody["data"]) {
            if ((item["pid1"] == pidA && item["pid2"] == pidB) ||
                (item["pid1"] == pidB && item["pid2"] == pidA)) {
                assert(item["relation_type"] == "SYNC");
                found = true;
            }
        }
        assert(found);
        std::cout << "查询进程关系列表接口: PASSED" << std::endl;
    }

    // 3. 将 A 阻塞
    json blockBody = {{"state", "BLOCKED"}};
    auto blkRes = cli.Put("/api/v1/processes/" + std::to_string(pidA) + "/state", blockBody.dump(), "application/json");
    assert(blkRes && blkRes->status == 200);

    // 查询 B 状态
    auto allRes = cli.Get("/api/v1/processes");
    assert(allRes && allRes->status == 200);
    json allList = json::parse(allRes->body)["data"];
    std::string stateA, stateB;
    for (auto& p: allList) {
        if (p["pid"] == pidA) stateA = p["state"];
        if (p["pid"] == pidB) stateB = p["state"];
    }
    assert(stateA == "BLOCKED" && stateB == "BLOCKED");
    std::cout << "同步阻塞传播: PASSED" << std::endl;

    // 4. 解除 A 阻塞 (READY)
    json readyBody = {{"state", "READY"}};
    auto rdyRes = cli.Put("/api/v1/processes/" + std::to_string(pidA) + "/state", readyBody.dump(), "application/json");
    assert(rdyRes && rdyRes->status == 200);

    // 再次检查
    auto allRes2 = cli.Get("/api/v1/processes");
    assert(allRes2 && allRes2->status == 200);
    json allList2 = json::parse(allRes2->body)["data"];
    for (auto& p: allList2) {
        if (p["pid"] == pidA) stateA = p["state"];
        if (p["pid"] == pidB) stateB = p["state"];
    }
    assert(stateA == "READY" && stateB == "READY");
    std::cout << "同步解除阻塞传播: PASSED" << std::endl;
}

#endif // RELATIONSHIP_API_TEST_H 