
#include "main.hpp"

#include <fstream>
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
                if (data_item["name"] == "PIOC") {
                    int level = data_item["level"];
                    int i = -1;
                    pioc_config::pioc_type_e search;
                    if (data_item["mode"] == "Phase"){
                        search = pioc_config::pioc_type_e::Phase;
                    }else{
                        search = pioc_config::pioc_type_e::Neutral;
                    }
                    for(auto& prot : ied_conf.prot.pioc) {
                        if (ied_conf.prot.pioc[i].type == search){
                            if (i == level) break;
                            else {
                                if (i==-1) i+=2;
                                else i++;
                            }
                        }
                    }
                    if (i != level) continue;
                    
                    if (data_item["value"] == "pickup") {
                        go_conf_item.data_pointers.push_back(&ied_conf.prot.pioc[i].pickup_flag);
                    } else if (data_item["value"] == "trip") {
                        go_conf_item.data_pointers.push_back(&ied_conf.prot.pioc[i].trip_flag);
                    }


                } else if (data_item["name"] == "PTOC") {
                    int level = data_item["level"];
                    int i = -1;
                    pioc_config::pioc_type_e search;
                    if (data_item["mode"] == "Phase"){
                        search = pioc_config::pioc_type_e::Phase;
                    }else{
                        search = pioc_config::pioc_type_e::Neutral;
                    }
                    for(auto& prot : ied_conf.prot.pioc) {
                        if (ied_conf.prot.pioc[i].type == search){
                            if (i == level) break;
                            else {
                                if (i==-1) i+=2;
                                else i++;
                            }
                        }
                    }
                    if (i != level) continue;

                    if (data_item["value"] == "pickup") {
                        go_conf_item.data_pointers.push_back(&ied_conf.prot.ptoc[i].pickup_flag);
                    } else if (data_item["value"] == "trip") {
                        go_conf_item.data_pointers.push_back(&ied_conf.prot.ptoc[i].trip_flag);
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

    for (auto& item : j) {
        std::string type = item.value("type", "");
        std::string mode = item.value("mode", "");

        if (type == "PIOC") {
            pioc_config conf;
            conf.pickup = item["pickup"].get<double>();
            conf.time_dial = item["time_dial"].get<double>();
            if (mode == "Phase"){
                conf.type = conf.Phase;
            }else if (mode == "Neutral"){
                conf.type = conf.Neutral;
            }
            prot_conf.pioc.push_back(conf);
        } 
        
        else if (type == "PTOC") {
            ptoc_config conf;
            conf.pickup = item["pickup"].get<double>();
            conf.time_dial = item["time_dial"].get<double>();
            conf.curve = item["curve"].get<std::string>();
            if (mode == "Phase"){
                conf.type = ptoc_config::ptoc_type_e::Phase;
            }else if (mode == "Neutral"){
                conf.type = ptoc_config::ptoc_type_e::Neutral;
            }
            prot_conf.ptoc.push_back(conf);
        }

        else if (type == "PDIS") {
            pdis_config conf;
            conf.pickup = item["pickup"].get<double>();
            conf.time_dial = item["time_dial"].get<double>();
            if (mode == "Impedance"){
                conf.type = conf.Impedance;
            }else if (mode == "Reactance"){
                conf.type = conf.Reactance;
            }else if (mode == "Admittance"){
                conf.type = conf.Admittance;
                conf.torque = item["rTorque"].get<double>();
            }
            
            prot_conf.pdis.push_back(conf);
        } 
        
        else if (type == "PDIR"){
            pdir_config conf;
            conf.pickup = item["pickup"].get<double>();
            conf.time_dial = item["time_dial"].get<double>();
            conf.torque = item["torque"].get<double>();

            if (item["direction"] == "Forward"){
                conf.direction = pdir_config::direction_e::Forward;
            }else {
                conf.direction = pdir_config::direction_e::Reverse;
            }

            if (item["dir_mode"] == "Quadrant"){
                conf.dir_mode = pdir_config::dir_mode_e::Quadrant;
            }else{}

            if (mode == "Phase"){
                conf.type = pdir_config::pioc_type_e::Phase;
            }else{
                conf.type = pdir_config::pioc_type_e::Neutral;
            }

            prot_conf.pdir.push_back(conf);
        }

        else {
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

void test_pioc(IED_class &Ied){
    
    std::vector<double> times;

    Ied.sniffer.phasor_mod[0] = 0;
    Ied.sniffer.phasor_mod[1] = 0;
    Ied.sniffer.phasor_mod[2] = 0;
    Ied.sniffer.phasor_mod[3] = 0;

    Ied.sniffer.phasor_mod[4] = 1000;
    Ied.sniffer.phasor_mod[5] = 1000;
    Ied.sniffer.phasor_mod[6] = 1000;
    Ied.sniffer.phasor_mod[7] = 0;

    

    for (int _ =0; _<5;_++){
        // dis
        Ied.sniffer.phasor_mod[0] = 0;
        Ied.sniffer.phasor_ang[0] = 0;
        Ied.sniffer.phasor_mod[4] = 1000;
        Ied.sniffer.phasor_ang[4] = 0;
        // neutral
        Ied.sniffer.phasor_mod[3] = 0;
    
        struct timespec t0, t1;
        clock_gettime(CLOCK_MONOTONIC, &t0);
        double tdif;
        do{
            clock_gettime(CLOCK_MONOTONIC, &t1);
            pthread_cond_broadcast(&Ied.sniffer.sniffer_cond);
            tdif = (t1.tv_sec - t0.tv_sec) + (t1.tv_nsec - t0.tv_nsec)/1e9;
        }while (tdif < 1);

        clock_gettime(CLOCK_MONOTONIC, &t0);

        // Distance
        // Ied.sniffer.phasor_mod[0] = 4000;
        // Ied.sniffer.phasor_ang[0] = 0;

        // PTOC & PIOC
        Ied.sniffer.phasor_mod[0] = 900;
        Ied.sniffer.phasor_ang[0] = 0;

        // neutral
        Ied.sniffer.phasor_mod[3] = 4000;

        pthread_mutex_lock(&Ied.sniffer.sniffer_mutex);
        pthread_cond_broadcast(&Ied.sniffer.sniffer_cond);
        pthread_mutex_unlock(&Ied.sniffer.sniffer_mutex);
        while(Ied.prot.ptoc[0].trip_flag != 1){
            continue;
        }
        clock_gettime(CLOCK_MONOTONIC, &t1);
        std::cout << (t1.tv_sec - t0.tv_sec) + (t1.tv_nsec - t0.tv_nsec)/1e9 << std::endl;
        times.push_back((t1.tv_sec - t0.tv_sec) + (t1.tv_nsec - t0.tv_nsec)/1e9);
    }

    // Get mean value
    std::cout << std::fixed << std::setprecision(5);
    std::cout << "error: " << abs(std::accumulate(times.begin(), times.end(), 0.0)/times.size() - 0.8)/0.8*100 << "%" << std::endl;

}

void test_pdir(IED_class &Ied){

    // Test PDIR
    Ied.sniffer.phasor_mod[0] = 400;
    Ied.sniffer.phasor_mod[1] = 400;
    Ied.sniffer.phasor_mod[2] = 400;

    Ied.sniffer.phasor_mod[4] = 13800;
    Ied.sniffer.phasor_mod[5] = 13800;
    Ied.sniffer.phasor_mod[6] = 13800;

    Ied.sniffer.phasor_ang[0] = 0;
    Ied.sniffer.phasor_ang[1] = 2.0/3.0 * M_PI;
    Ied.sniffer.phasor_ang[2] = -2.0/3.0 * M_PI;

    Ied.sniffer.phasor_ang[4] = 0;
    Ied.sniffer.phasor_ang[5] = 2.0/3.0 * M_PI;
    Ied.sniffer.phasor_ang[6] = -2.0/3.0 * M_PI;
    
    pthread_cond_broadcast(&Ied.sniffer.sniffer_cond);
    sleep(0.4);
    pthread_cond_broadcast(&Ied.sniffer.sniffer_cond);
    sleep(0.4);

    std::cout << Ied.prot.pdir[0].pickup_flag << std::endl;

    Ied.sniffer.phasor_ang[0] = -1 * M_PI;
    Ied.sniffer.phasor_ang[1] = (2.0/3.0 * M_PI) - M_PI;
    Ied.sniffer.phasor_ang[2] = (-2.0/3.0 * M_PI) - M_PI;

    Ied.sniffer.phasor_ang[4] = 0;
    Ied.sniffer.phasor_ang[5] = 2.0/3.0 * M_PI;
    Ied.sniffer.phasor_ang[6] = -2.0/3.0 * M_PI;

    pthread_cond_broadcast(&Ied.sniffer.sniffer_cond);

    sleep(0.4);

    std::cout << Ied.prot.pdir[0].pickup_flag << std::endl;

}

int main(int, char**){

    IED_class Ied;
    load_config(Ied, "files/ied_config.json");

    Ied.init();
    // Ied.sniffer.startThread();
    Ied.prot.startThread();
    Ied.goose.startThread();

    test_pdir(Ied);

    // ied_conf.stopIED();
    // Ied.sniffer.stopThread();
    Ied.prot.stopThread();
    Ied.goose.stopThread();

    return 0;
}
