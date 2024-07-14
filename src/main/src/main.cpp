
#include "main.hpp"

#include <fstream>
#include <filesystem>
#include <goose.hpp>

std::string root_folder = "oi";

void taskFunction(int *id) {
    std::cout << "Task " << *id << " started\n" << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(2));
    std::cout << "Task " << *id << " completed\n" << std::endl;
    return;
}

using json = nlohmann::json;

void json_get_goose_config(json& j, GooseClass& go_conf, IED_class& ied_conf) {
    
    // Example
    // "Goose":[
    //     {
    //         "marDst": "01:0C:CD:01:00:01",
    //         "appID": 4000,
    //         "vLan_id": 100,
    //         "vLan_priority": 4,
    //         "minTime": 100,
    //         "maxTime": 4000,
    //         "confRev": 1,
    //         "datSet": "GODat_01",
    //         "goID": "GO_01", // Optional (might have or not)
    //         "gocbRef": "GCBR_01",
    //         "data":[
            //     {
            //         "block": "protection",
            //         "name": "pioc_phase",
            //         "value": "pickup",
            //         "type": "Boolean"
            // "level": 0,
            //     }
            // ]
    //     }
    // ]

    for (auto& item : j) {
        Goose_Config go_conf_item;
        go_conf_item.srcMac = "00:00:00:00:00:00";
        go_conf_item.dstMac = item["marDst"];
        go_conf_item.vlanId = item["vLan_id"];
        go_conf_item.vlanPcp = item["vLan_priority"];
        go_conf_item.appId = item["appID"];
        go_conf_item.gocbRef = item["gocbRef"];
        go_conf_item.minTime = item["minTime"];
        go_conf_item.maxTime = item["maxTime"];
        go_conf_item.dataSet = item["datSet"];
        go_conf_item.confRev = item["confRev"];

        if (item.contains("goID")) {
            go_conf_item.goID = item["goID"];
        }

        for (auto& data_item : item["data"]) {
            if (data_item["block"] == "protection") {
                if (data_item["name"] == "pioc_phase") {
                    if (data_item["value"] == "pickup") {
                        go_conf_item.data_pointers.push_back(&ied_conf.prot.pioc_phase[data_item["level"]].pickup_flag);
                    } else if (data_item["value"] == "trip") {
                        go_conf_item.data_pointers.push_back(&ied_conf.prot.pioc_phase[data_item["level"]].trip_flag);
                    }
                } else if (data_item["name"] == "pioc_neutral") {
                    if (data_item["value"] == "pickup") {
                        go_conf_item.data_pointers.push_back(&ied_conf.prot.pioc_neutral[data_item["level"]].pickup_flag);
                    } else if (data_item["value"] == "trip") {
                        go_conf_item.data_pointers.push_back(&ied_conf.prot.pioc_neutral[data_item["level"]].trip_flag);
                    }
                } else if (data_item["name"] == "ptoc_phase") {
                    if (data_item["value"] == "pickup") {
                        go_conf_item.data_pointers.push_back(&ied_conf.prot.ptoc_phase[data_item["level"]].pickup_flag);
                    } else if (data_item["value"] == "trip") {
                        go_conf_item.data_pointers.push_back(&ied_conf.prot.ptoc_phase[data_item["level"]].trip_flag);
                    }
                } else if (data_item["name"] == "ptoc_neutral") {
                    if (data_item["value"] == "pickup") {
                        go_conf_item.data_pointers.push_back(&ied_conf.prot.ptoc_neutral[data_item["level"]].pickup_flag);
                    } else if (data_item["value"] == "trip") {
                        go_conf_item.data_pointers.push_back(&ied_conf.prot.ptoc_neutral[data_item["level"]].trip_flag);
                    }
                }

                if (data_item["type"] == "Boolean") {
                    go_conf_item.data_type.push_back(Data(Data::Type::Boolean));
                }
            }
        }
        
        go_conf.goose_config.push_back(go_conf_item);
    }
}

void json_get_protection_config(json& j, ProtectionClass& prot_conf){

    // Example
    // "Protection": [
    //     {
    //         "type": "pioc_phase",
    //         "pickup": 800,
    //         "time_dial": 0.8
    //     },
    //     {
    //         "type": "pioc_neutral",
    //         "pickup": 200,
    //         "time_dial": 0.8
    //     },
    //     {
    //         "type": "ptoc_phase",
    //         "pickup": 300,
    //         "time_dial": 0.8,
    //         "curve": "U1"
    //     }
    // ]

    for (auto& item : j) {
        std::string type = item["type"];

        if (type == "pioc_phase") {
            prot_conf.pioc_phase.push_back({
                item["pickup"].get<double>(),
                item["time_dial"].get<double>()
            });
        } else if (type == "pioc_neutral") {
            prot_conf.pioc_neutral.push_back({
                item["pickup"].get<double>(),
                item["time_dial"].get<double>()
            });
        } else if (type == "ptoc_phase") {
            prot_conf.ptoc_phase.push_back({
                item["pickup"].get<double>(),
                item["time_dial"].get<double>(),
                item["curve"].get<std::string>()
            });
        } else if (type == "ptoc_neutral") {
            prot_conf.ptoc_neutral.push_back({
                item["pickup"].get<double>(),
                item["time_dial"].get<double>(),
                item["curve"].get<std::string>()
            });
        } else {
            std::cerr << "Unknown protection type: " << type << std::endl;
        }
    }

}

void json_get_sampledValue_info(json& j, sampledValue_info& sv_info){

    // Example
    // "SampledValue": {
    //     "svID": "SV_Bus05",
    //     "mac_dst": [1, 12, 205, 1, 0, 4],
    //     "channel_conf": [0, 1, 2, 3, 4, 5, 6, 7],
    //     "vLan_priority": 4,
    //     "vLan_id": 100,
    //     "smpRate": 80,
    //     "noChannels": 8,
    //     "frequency": 60
    // },

    sv_info.svID = j["svID"];
    auto mac_dst_array = j.at("mac_dst").get<std::vector<uint8_t>>();
    std::copy(mac_dst_array.begin(), mac_dst_array.end(), sv_info.mac_dst);
    sv_info.channel_conf = j["channel_conf"].get<std::vector<int>>();
    sv_info.vLan_priority = j["vLan_priority"];
    sv_info.vLan_id = j["vLan_id"];
    sv_info.smpRate = j["smpRate"];
    sv_info.noChannels = j["noChannels"];
    sv_info.frequency = j["frequency"];
}

void json_get_ied_config(json& j, IED_class& conf) {
    json_get_sampledValue_info(j["SampledValue"], conf.sniffer.sv_info);
    json_get_protection_config(j["Protection"], conf.prot);
    json_get_goose_config(j["Goose"], conf.goose, conf);
}

void load_config(IED_class& conf, std::string file_name) {
    // Load from file

    std::ifstream file(file_name);
    if (!file.is_open()) {
        std::cerr << "Could not open the file: " << file_name << std::endl;
        exit(-1);
    }

    json jobj;
    file >> jobj;

    try{
        json_get_ied_config(jobj, conf);
    } catch (json::exception& e) {
        std::cerr << "Error parsing json: " << e.what() << std::endl;
        exit(-1);
    }
    file.close();
}


int main(int, char**){


    IED_class Ied;

    load_config(Ied, "Main/files/ied_config.json");

    Ied.init();

    Ied.sniffer.phasor_mod[2] = 0;
    Ied.sniffer.phasor_mod[3] = 0;

    Ied.prot.startThread();
    Ied.goose.startThread();

    
    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    double tdif;
    do{
        clock_gettime(CLOCK_MONOTONIC, &t1);
        pthread_cond_broadcast(&Ied.sniffer.sniffer_cond);
        tdif = (t1.tv_sec - t0.tv_sec) + (t1.tv_nsec - t0.tv_nsec)/1e9;
    }while (tdif < 2);

    clock_gettime(CLOCK_MONOTONIC, &t0);
    pthread_mutex_lock(&Ied.sniffer.sniffer_mutex);
    Ied.sniffer.phasor_mod[2] = 1200;
    Ied.sniffer.phasor_mod[3] = 1200;
    pthread_cond_broadcast(&Ied.sniffer.sniffer_cond);
    pthread_mutex_unlock(&Ied.sniffer.sniffer_mutex);
    while(Ied.prot.ptoc_phase[0].trip_flag != 1){
        continue;
    }
    clock_gettime(CLOCK_MONOTONIC, &t1);
    std::cout << (t1.tv_sec - t0.tv_sec) + (t1.tv_nsec - t0.tv_nsec)/1e9 << std::endl;

    // ied_conf.stopIED();
    Ied.prot.stopThread();
    Ied.goose.stopThread();

    return 0;
}
